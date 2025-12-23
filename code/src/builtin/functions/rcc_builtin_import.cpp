//
// Created by RestRegular on 2025/9/16.
//

#include <cstring>

#include "../../../declarations/builtin/functions/rcc_builtin_import_dec.h"

#include <filesystem>

namespace builtin
{

    namespace rcc_import_utils
    {
        using namespace utils;

        std::pair<bool, std::string> isSystemExtensionFile(const std::string &extName)
        {
            if (extName.find('.') != std::string::npos)
            {
                if (const auto &outerExtPath = getAbsolutePath(extName, base::RCC_LIB_DIR);
                    std::filesystem::exists(outerExtPath))
                {
                    return {true, outerExtPath};
                }
                return {false, ""};
            }
            if (const auto &extPath = getAbsolutePath(extName + ".rio", base::RCC_BUILTIN_LIB_DIR);
                std::filesystem::exists(extPath))
            {
                return {true, extPath};
            }
            const auto& extDir = getAbsolutePath(extName, base::RCC_LIB_DIR);
            if (const auto& extInfoPath = getAbsolutePath(RCC_EXT_INFO_FILE_NAME, extDir);
                std::filesystem::exists(extDir) &&
                std::filesystem::exists(extInfoPath))
            {
                auto rjParser = rjson::rj::RJsonParser();
                rjParser.parse(extInfoPath);
                const auto& entryPath = getAbsolutePath(
                    parseStringFormat(rjParser.getAtKey("main")->getStringValue()), extDir);
                if (std::filesystem::exists(entryPath))
                {
                    return {true, entryPath};
                }
            }
            return {false, ""};
        }

        std::pair<std::string, std::optional<std::vector<std::string>>>
        resolveImportedFilePath(
            const ast::CompileVisitor& visitor, const std::string& extName)
        {
            // 1. 先检查是否是系统扩展文件（完整名称）
            if (const auto& [isBuiltin, builtinExtPath] = isSystemExtensionFile(extName);
                isBuiltin)
            {
                return {builtinExtPath, std::nullopt};
            }

            // 2. 查找冒号
            const auto firstColonPos = std::ranges::find(extName, ':');
            if (firstColonPos == extName.end())
            {
                // 没有冒号，整个字符串都是路径
                std::string importPath = getAbsolutePath(
                    extName, getFileDirFromPath(visitor.getProgramTargetFilePath()));
                return {std::move(importPath), std::nullopt};
            }

            // 3. 提取前缀（冒号前部分）
            const std::string_view extPrefixName{extName.begin(), firstColonPos};
            std::string importPath;

            // 检查前缀是否是系统扩展文件
            if (const auto& [isBuiltin, builtinExtPath] =
                isSystemExtensionFile(std::string(extPrefixName));
                isBuiltin)
            {
                importPath = builtinExtPath;
            } else
            {
                importPath = getAbsolutePath(
                    std::string(extPrefixName),
                    getFileDirFromPath(visitor.getProgramTargetFilePath()));
            }

            // 4. 安全地检查冒号后字符并提取后缀
            std::optional<std::vector<std::string>> symbolParts;

            if (const auto afterColonPos = std::next(firstColonPos);
                afterColonPos != extName.end())
            {
                if (const auto afterColon = std::next(afterColonPos);
                    afterColon != extName.end())
                {
                    const std::string_view specSymbolView{afterColon, extName.end()};
                    std::string trimmed = StringManager::trim(std::string(specSymbolView));
                    symbolParts = std::vector{std::move(trimmed)};
                }
            }

            return {std::move(importPath), std::move(symbolParts)};
        }

        std::string generateVariableIdentifier(ast::CompileVisitor& visitor, const std::string& ident,
                                             const std::string& importedFilePath, bool isAutomaticForm)
        {
            if (isAutomaticForm) {
                return ident;
            }
            return "extension_" + getFileNameFromPath(importedFilePath) + "_" + ast::CompileVisitor::getNewTempVarName();
        }

        std::string handleRegisteredExtension(ast::CompileVisitor& visitor,
                                            const std::shared_ptr<symbol::ClassSymbol>& registeredExtension,
                                            const ast::VarID& tempVarId,
                                            bool isAutomaticForm)
        {
            std::string raCode;
            const auto &extensionSymbol = std::static_pointer_cast<symbol::ClassSymbol>(
            registeredExtension->transform(tempVarId.getNameField(), tempVarId.getVid(), visitor.curScopeLevel()));
            symbol::TypeLabelSymbol::createCustomType(extensionSymbol);
            raCode += ri::ALLOT({extensionSymbol->getRaVal()}).toRACode()
                    + ri::PUT(registeredExtension->getRaVal(), extensionSymbol->getRaVal()).toRACode();
            visitor.getSymbolTable().insert(extensionSymbol, true);
            if (!isAutomaticForm)
            {
                const auto &clasTypeSymbol = symbol::TypeLabelSymbol::clasTypeSymbol(getUnknownPos(), visitor.curScopeLevel());
                visitor.pushIdentItem(tempVarId, clasTypeSymbol, clasTypeSymbol, extensionSymbol);
            }
            return raCode;
        }

        void compileAndProcessNewExtension(ast::CompileVisitor& visitor,
                                           const std::string& importedFilePath,
                                           const ast::VarID& tempVarId,
                                           const Pos& pos,
                                           const std::optional<std::vector<std::string>>& importingSymbols)
        {
            const auto &clasTypeSymbol = symbol::TypeLabelSymbol::clasTypeSymbol(getUnknownPos(), visitor.curScopeLevel());
            if (ast::CompileVisitor::checkIsRecursiveImportByLexerPath(importedFilePath))
            {
                throw base::RCCCompilerError::recursiveImportError(
                    ast::CompileVisitor::currentPos().toString(),
                    ast::CompileVisitor::topLexer()->getCodeLine(ast::CompileVisitor::currentPos()),
                    importedFilePath,
                    "Extension import chain: " + vectorJoin(
                        [&]
                        {
                            std::vector<std::string> result{};
                            for (const auto& filepath : ast::CompileVisitor::getLexerFilePaths())
                            {
                                const auto& res = getFileFromPath(filepath);
                                result.push_back(filepath == importedFilePath ? "[" + res + "]" : res);
                            }
                            return result;
                        }(), " -> ") + " ~> [" + getFileFromPath(importedFilePath) + "]",
                    {
                        "Adjust the import order to avoid forming a closed loop.",
                        "Refactor the code structure and extract the parts with circular dependencies into an independent public extension."
                    });
            }
            ast::CompileVisitor importVisitor(visitor.getProgramEntryFilePath(), importedFilePath, "", false);
            try {
                if (!importVisitor.compile()) {
                    throw std::runtime_error("Failed to import extension.");
                }
            } catch (const base::RCCError &) {
                throw; // 重新抛出已知异常，保持原始堆栈信息
            } catch (const std::exception &e) {
                // 统一处理所有标准异常类型
                std::vector details = {
                    "Failed imported extension: " + tempVarId.getNameField(),
                    "Automatically detected path: " + importedFilePath
                };

                // 只在有异常消息时添加错误详情
                if (e.what() && std::strlen(e.what()) > 0) {
                    details.push_back("Error details: " + std::string(e.what()));
                }

                const std::vector<std::string> suggestions = {
                    "If the automatically detected path of the extension does not match the real absolute path,",
                    "please use the real absolute path for importing."
                };

                throw base::RCCCompilerError::extensionLoadinError(
                    pos.toString(),
                    ast::CompileVisitor::getCodeLine(pos),
                    std::move(details),
                    std::move(suggestions)
                );
            }
            const auto extensionSymbol = std::make_shared<symbol::ClassSymbol>(
                Pos{1, 1, 0, tempVarId.getFileField()},
                tempVarId.getNameField(), tempVarId.getVid(),
                std::unordered_set<std::shared_ptr<symbol::LabelSymbol>>{},
                visitor.curScopeLevel(),
                visitor.getSymbolTable());
            visitor.getSymbolTable().insert(extensionSymbol, false);
            visitor.getExtensionHeaderCodeBuilder()
            << importVisitor.getExtensionHeaderCodeBuilder().buildAll()
            << ri::TP_DEF(extensionSymbol->getRaVal());
            processImportedSymbols(visitor, importVisitor, extensionSymbol, importingSymbols);
            extensionSymbol->setCollectionFinished();
            extensionSymbol->setVisitPermission(symbol::PermissionLabel::PUBLIC);
            symbol::TypeLabelSymbol::createCustomType(extensionSymbol);
            ast::CompileVisitor::registerExtension(importedFilePath, extensionSymbol);
            visitor.pushIdentItem(tempVarId, clasTypeSymbol, clasTypeSymbol, extensionSymbol);
        }

        void processImportedSymbols(const ast::CompileVisitor& visitor,
                                    ast::CompileVisitor& importVisitor,
                                    const std::shared_ptr<symbol::ClassSymbol>& extensionSymbol,
                                    const std::optional<std::vector<std::string>>& importingSymbols)
        {
            importVisitor.getSymbolTable().enterGlobalScope();

            const auto &ordinaryLabel = std::make_shared<symbol::LabelSymbol>(
                getUnknownPos(), "ordinary", "", visitor.curScopeLevel(), symbol::LabelType::LIFE_CYCLE_LABEL);
            const auto &publicLabel = std::make_shared<symbol::LabelSymbol>(
                getUnknownPos(), "public", "", visitor.curScopeLevel(), symbol::LabelType::PERMISSION_LABEL);

            // 辅助函数：处理单个符号的导入
            auto processSymbol = [&](const std::shared_ptr<symbol::Symbol>& member)
            {
                if (member->hasMarked())
                {
                    return;
                }
                if (member->is(symbol::SymbolType::VARIABLE))
                {
                    const auto &varSymbol = std::static_pointer_cast<symbol::VariableSymbol>(member);
                    varSymbol->getLabelMarkManager().resetPermissionLabelMarks();
                    varSymbol->getLabelMarkManager().markLabels({ordinaryLabel, publicLabel});
                    extensionSymbol->addMember(varSymbol, false);
                    varSymbol->appendImportCode(ri::TP_ADD_TP_FIELD(extensionSymbol->getRaVal(),
                                StringManager::toStringFormat(varSymbol->getVal()),
                                varSymbol->getRaVal()).toRACode());
                } else if (member->is(symbol::SymbolType::FUNCTION))
                {
                    if (const auto &funcSymbol = std::static_pointer_cast<symbol::FunctionSymbol>(member);
                        funcSymbol->getBuiltInType() != symbol::TypeOfBuiltin::PURE_BUILTIN)
                    {
                        funcSymbol->getLabelMarkManager().resetPermissionLabelMarks();
                        funcSymbol->getLabelMarkManager().markLabels({ordinaryLabel, publicLabel});
                        funcSymbol->setFunctionType(symbol::FunctionType::METHOD);
                        extensionSymbol->addMember(funcSymbol, false);
                        funcSymbol->appendImportCode(ri::TP_ADD_TP_FIELD(extensionSymbol->getRaVal(),
                                    StringManager::toStringFormat(funcSymbol->getVal()),
                                    funcSymbol->getRaVal()).toRACode());
                    }
                } else if (member->is(symbol::SymbolType::CLASS))
                {
                    const auto &classSymbol = std::static_pointer_cast<symbol::ClassSymbol>(member);
                    classSymbol->getLabelMarkManager().resetPermissionLabelMarks();
                    classSymbol->getLabelMarkManager().markLabels({ordinaryLabel, publicLabel});
                    extensionSymbol->addMember(classSymbol, false);
                    classSymbol->appendImportCode(ri::TP_ADD_TP_FIELD(extensionSymbol->getRaVal(),
                                StringManager::toStringFormat(classSymbol->getVal()),
                                classSymbol->getRaVal()).toRACode());
                } else
                {
                    throw std::runtime_error("Cannot resolve symbol: '" + member->getVal() + "'");
                }
                member->setMarked();
            };

            std::function<void(const std::shared_ptr<symbol::Symbol>&)> processSymbolDependency =
                [&](const std::shared_ptr<symbol::Symbol>& member)
                {
                    if (member->hasMarked())
                    {
                        return;
                    }
                    for (const auto& [raVal, dependency] : member->getDependencies())
                    {
                        try
                        {
                            processSymbolDependency(dependency);
                            if (importVisitor.getSymbolTable().containsRID(dependency->getRaVal()))
                            {
                                processSymbol(dependency);
                            } else
                            {
                                dependency->setMarked();
                            }
                        } catch (std::exception &e)
                        {
                            throw std::runtime_error(std::string(e.what())
                                + "; Cannot find dependency symbol: '" + dependency->getVal() + "'");
                        }
                    }
                };

            // 根据是否传入了指定的导入符号列表进行处理
            if (importingSymbols.has_value() && !importingSymbols->empty()) {
                // 仅导入指定的符号
                for (const auto& symbolNames = importingSymbols.value();
                    const auto& symbolName : symbolNames)
                {
                    interruptWhen(symbolName == "print", []
                    {
                        pass();
                    });

                    // 查找指定名称的符号
                    if (const auto& [level, member] = importVisitor.getSymbolTable().findByName(symbolName);
                        level >= 0 && member)
                    {
                        // TODO: 此处可以添加依赖分析预留位置
                        // 在导入此符号前，可以检查其依赖的其他符号
                        // 例如：对于函数，可能需要导入其参数类型相关的类
                        // 对于类，可能需要导入其基类或成员类型

                        processSymbolDependency(member);
                        processSymbol(member);
                        continue;
                    }

                    throw std::runtime_error("Cannot find imported symbol: '" + symbolName + "'");
                }

                // TODO: 依赖收集与处理预留位置
                // 可以在这里对已导入的符号进行依赖分析，确保必要的依赖也被导入
                // 例如：
                // 1. 收集所有导入符号的直接依赖
                // 2. 检查这些依赖是否已经在导入列表中
                // 3. 如果不在，可以选择自动导入或报错
                // 4. 递归处理依赖的依赖

            } else {
                // 全量导入：导入当前作用域中的所有符号
                for (const auto &member : importVisitor.getSymbolTable().currentNameMapScope()) {
                    interruptWhen(member->getVal() == "print", []
                    {
                        pass();
                    });
                    processSymbolDependency(member);
                    processSymbol(member);
                }
            }
        }
    }

    BuiltinFuncRetType rcc_import(ast::CompileVisitor &visitor, const CallInfos& callInfos)
    {
        using namespace rcc_import_utils;
        bool isAutomaticForm = false;
        bool hasPositionalArgs = false;
        int index = 0;
        for (const auto& [ident, value] : callInfos.originalArgs) {
            if (ident.empty()) {
                hasPositionalArgs = true;
            } else {
                isAutomaticForm = true;
            }
            if (!isStringFormat(value))
            {
                throw base::RCCCompilerError::semanticError(
                    callInfos.orderedArgOpItems[index].second.getPos().toString(),
                    ast::CompileVisitor::getCodeLine(callInfos.callPos),
                    "The argument of the `import` function must be a string literal.",
                    {
                        "Please ensure that the argument of the `import` function is a string literal."
                    });
            }
            index ++;
        }
        if (hasPositionalArgs && isAutomaticForm) {
            throw base::RCCCompilerError::semanticError(
                callInfos.callPos.toString(), ast::CompileVisitor::getCodeLine(callInfos.callPos),
                "You cannot use both positional arguments and keyword arguments for the `import` function at the same time.",
                {
                    "Please ensure that only one import mode is used.",
                    "It is recommended to use the keyword argument import mode."
                });
        }
        if (hasPositionalArgs && callInfos.originalArgs.size() != 1) {
            throw base::RCCCompilerError::semanticError(
                callInfos.callPos.toString(), ast::CompileVisitor::getCodeLine(callInfos.callPos),
                "Explicitly defining the variable import mode allows only one extension to be imported at a time.",
                {
                    "Please ensure that only one extension is imported at a time when using the explicit variable definition import mode."
                });
        }
        std::string raCode;
        index = 0;
        for (const auto &[ident, extName] : callInfos.originalArgs) {
            const auto &unescapedExtNameArg = StringManager::parseStringFormat(extName);
            const auto &[importedFilePath, importSymbols] = resolveImportedFilePath(visitor, unescapedExtNameArg);
            const auto &varIDName = generateVariableIdentifier(visitor, ident, importedFilePath, isAutomaticForm);
            const auto &tempVarId = ast::VarID(
                varIDName, visitor.getCurrentProcessingFilePath(),
                visitor.curScopeField(), visitor.curScopeLevel());
            ast::CompileVisitor::recordProcessingExtension(importedFilePath, unescapedExtNameArg);
            if (const auto &registeredExtension = ast::CompileVisitor::getRegisteredExtension(importedFilePath)) {
                raCode += handleRegisteredExtension(visitor, registeredExtension, tempVarId, isAutomaticForm);
            } else {
                compileAndProcessNewExtension(visitor, importedFilePath, tempVarId, callInfos.orderedArgOpItems[index].second.getPos(), importSymbols);
            }
            ast::CompileVisitor::popProcessingExtension();
            index ++;
        }
        return raCode;
    }

    const PureBuiltinFunction& getImportFunction()
    {
        static const PureBuiltinFunction IMPORT(
            true, "import",
            {
                getPosVarArgsParam("args"),
                getKeywordVarArgsParam("kwargs")
            }, rcc_import);
        return IMPORT;
    }

}

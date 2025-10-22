//
// Created by RestRegular on 2025/9/16.
//

#include "../../../include/builtin/functions/rcc_builtin_import.h"

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
                if (const auto &outerExtPath = getAbsolutePath(extName, getWindowsRCCDir() + "\\Lib");
                    std::filesystem::exists(outerExtPath))
                {
                    return {true, outerExtPath};
                }
                return {false, ""};
            }
            // 系统内置拓展文件
            if (const auto &extPath = getAbsolutePath(".\\Lib\\builtin\\" + extName + ".rio", getWindowsRCCDir());
                std::filesystem::exists(extPath))
            {
                return {true, extPath};
            }
            // 系统外部拓展文件
            const auto &outerExtPath = getAbsolutePath(".\\Lib\\" + extName + "\\" + extName + ".rio", getWindowsRCCDir());
            return {std::filesystem::exists(outerExtPath), outerExtPath};
        }

        std::string resolveImportedFilePath(const ast::CompileVisitor& visitor, const std::string& extName)
        {
            if (const auto &[isBuiltin, builtinExtPath] = isSystemExtensionFile(extName);
                isBuiltin) {
                return builtinExtPath;
            }
            return getAbsolutePath(extName, getFileDirFromPath(visitor.getProgramTargetFilePath()));
        }

        std::string generateVariableIdentifier(ast::CompileVisitor& visitor, const std::string& ident,
                                             const std::string& importedFilePath, bool isAutomaticForm)
        {
            if (isAutomaticForm) {
                return ident;
            }
            return getFileNameFromPath(importedFilePath) + "_extension_" + visitor.getNewTempVarName();
        }

        std::string handleRegisteredExtension(ast::CompileVisitor& visitor,
                                            const std::shared_ptr<symbol::ClassSymbol>& registeredExtension,
                                            const ast::CompileVisitor::VarID& tempVarId,
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
                const auto &clasTypeSymbol = symbol::TypeLabelSymbol::clasTypeSymbol(Pos::UNKNOW_POS, visitor.curScopeLevel());
                visitor.pushIdentItem(tempVarId, clasTypeSymbol, clasTypeSymbol, extensionSymbol);
            }
            return raCode;
        }

        std::string compileAndProcessNewExtension(ast::CompileVisitor& visitor,
                                                  const std::string& importedFilePath,
                                                  const ast::CompileVisitor::VarID& tempVarId)
        {
            std::string raCode;
            const auto &clasTypeSymbol = symbol::TypeLabelSymbol::clasTypeSymbol(Pos::UNKNOW_POS, visitor.curScopeLevel());
            if (visitor.checkIsRecursiveImportByLexerPath(importedFilePath))
            {
                throw base::RCCCompilerError::recursiveImportError(
                    visitor.currentPos().toString(),
                    visitor.topLexer()->getCodeLine(visitor.currentPos()),
                    importedFilePath,
                    "Extension import chain: " + vectorJoin(
                [&]{
                    std::vector<std::string> result{};
                    for (const auto &filepath : visitor.getLexerFilePaths())
                    {
                        const auto &res = getFileFromPath(filepath);
                        result.push_back(filepath == importedFilePath ?
                            "[" + res + "]" : res);
                    }
                    return result;
                }(), " -> ") + " ~> [" + getFileFromPath(importedFilePath) + "]",
                {
                    "Adjust the import order to avoid forming a closed loop.",
                    "Refactor the code structure and extract the parts with circular dependencies into an independent public extension."
                });
            }
            ast::CompileVisitor importVisitor(visitor.getProgramEntryFilePath(), importedFilePath, "", false);
            if (!importVisitor.compile()) {
                throw std::runtime_error("Failure to import file: '" + importedFilePath + "'");
            }
            raCode += importVisitor.getCompileResult();
            const auto extensionSymbol = std::make_shared<symbol::ClassSymbol>(
                Pos(1, 1, 0, importedFilePath),
                tempVarId.getNameField(), tempVarId.getVid(),
                std::unordered_set<std::shared_ptr<symbol::LabelSymbol>>{},
                visitor.curScopeLevel(),
                visitor.getSymbolTable());
            visitor.getSymbolTable().insert(extensionSymbol, true);
            raCode += ri::TP_DEF(extensionSymbol->getRaVal()).toRACode();
            raCode += processImportedSymbols(visitor, importVisitor, extensionSymbol);
            extensionSymbol->setCollectionFinished();
            extensionSymbol->setVisitPermission(symbol::PermissionLabel::PUBLIC);
            symbol::TypeLabelSymbol::createCustomType(extensionSymbol);
            visitor.registerExtension(importedFilePath, extensionSymbol);
            visitor.pushIdentItem(tempVarId, clasTypeSymbol, clasTypeSymbol, extensionSymbol);
            return raCode;
        }

        std::string processImportedSymbols(const ast::CompileVisitor& visitor,
                                         ast::CompileVisitor& importVisitor,
                                         const std::shared_ptr<symbol::ClassSymbol>& extensionSymbol)
        {
            std::string raCode;
            importVisitor.getSymbolTable().enterGlobalScope();
            const auto &ordinaryLabel = std::make_shared<symbol::LabelSymbol>(
                Pos::UNKNOW_POS, "ordinary", "", visitor.curScopeLevel(), symbol::LabelType::LIFE_CYCLE_LABEL);
            const auto &publicLabel = std::make_shared<symbol::LabelSymbol>(
                Pos::UNKNOW_POS, "public", "", visitor.curScopeLevel(), symbol::LabelType::PERMISSION_LABEL);
            for (const auto &member : importVisitor.getSymbolTable().currentNameMapScope()) {
                if (member->is(symbol::SymbolType::VARIABLE)) {
                    const auto &varSymbol = std::static_pointer_cast<symbol::VariableSymbol>(member);
                    varSymbol->getLabelMarkManager().resetPermissionLabelMarks();
                    varSymbol->getLabelMarkManager().markLabels({ordinaryLabel, publicLabel});
                    extensionSymbol->addMember(varSymbol, false);
                    raCode += ri::TP_ADD_TP_FIELD(extensionSymbol->getRaVal(),
                                StringManager::toStringFormat(varSymbol->getRaVal()),
                                varSymbol->getRaVal()).toRACode();
                } else if (member->is(symbol::SymbolType::FUNCTION)) {
                    if (const auto &funcSymbol = std::static_pointer_cast<symbol::FunctionSymbol>(member); funcSymbol->getBuiltInType() != symbol::TypeOfBuiltin::PURE_BUILTIN) {
                        funcSymbol->getLabelMarkManager().resetPermissionLabelMarks();
                        funcSymbol->getLabelMarkManager().markLabels({ordinaryLabel, publicLabel});
                        funcSymbol->setFunctionType(symbol::FunctionType::METHOD);
                        extensionSymbol->addMember(funcSymbol, false);
                        raCode += ri::TP_ADD_TP_FIELD(extensionSymbol->getRaVal(),
                                    StringManager::toStringFormat(funcSymbol->getRaVal()),
                                    funcSymbol->getRaVal()).toRACode();
                    }
                } else if (member->is(symbol::SymbolType::CLASS)) {
                    const auto &classSymbol = std::static_pointer_cast<symbol::ClassSymbol>(member);
                    classSymbol->getLabelMarkManager().resetPermissionLabelMarks();
                    classSymbol->getLabelMarkManager().markLabels({ordinaryLabel, publicLabel});
                    extensionSymbol->addMember(classSymbol, false);
                    raCode += ri::TP_ADD_TP_FIELD(extensionSymbol->getRaVal(),
                                StringManager::toStringFormat(classSymbol->getRaVal()),
                                classSymbol->getRaVal()).toRACode();
                } else {
                    throw std::runtime_error("Cannot resolve symbol: '" + member->getVal() + "'");
                }
            }
            return raCode;
        }
    }

    BuiltinFuncRetType rcc_import(ast::CompileVisitor &visitor, const CallInfos& callInfos)
    {
        using namespace rcc_import_utils;
        bool isAutomaticForm = false;
        bool hasPositionalArgs = false;
        for (const auto& ident : callInfos.originalArgs | std::views::keys) {
            if (ident.empty()) {
                hasPositionalArgs = true;
            } else {
                isAutomaticForm = true;
            }
        }
        if (hasPositionalArgs && isAutomaticForm) {
            throw std::runtime_error("You cannot use both positional arguments and keyword arguments for the `import` function at the same time.");
        }
        if (hasPositionalArgs && callInfos.originalArgs.size() != 1) {
            throw std::runtime_error("Explicitly defining the variable import mode allows only one extension to be imported at a time.");
        }
        std::string raCode = "";
        for (const auto &[ident, extName] : callInfos.originalArgs) {
            const auto &unescapedExtNameArg = StringManager::parseStringFormat(extName);
            const auto &importedFilePath = resolveImportedFilePath(visitor, unescapedExtNameArg);
            const auto &varIDName = generateVariableIdentifier(visitor, ident, importedFilePath, isAutomaticForm);
            const auto &tempVarId = ast::CompileVisitor::VarID(
                varIDName, visitor.getProgramTargetFilePath(), visitor.curScopeField(), visitor.curScopeLevel());
            if (const auto &registeredExtension = visitor.getRegisteredExtension(importedFilePath)) {
                raCode += handleRegisteredExtension(visitor, registeredExtension, tempVarId, isAutomaticForm);
            } else {
                raCode += compileAndProcessNewExtension(visitor, importedFilePath, tempVarId);
            }
        }

        return raCode;
    }

    const PureBuiltinFunction IMPORT (true, "import",
        {
            getPosVarArgsParam("args"),
            getKeywordVarArgsParam("kwargs")
        }, rcc_import);

}

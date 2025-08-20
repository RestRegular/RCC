//
// Created by RestRegular on 2025/8/5.
//

#include "../../include/builtin/rcc_builtin.h"
#include "../../include/components/ri/rcc_ri.h"

namespace builtin
{
    using namespace utils;

    void initializeFullBuiltinEnvironment(ast::CompileVisitor& visitor)
    {
        visitor.enterGlobalScope();
        const auto &argsId = ast::CompileVisitor::VarID("args", Pos::UNKNOW_POS.getFileField(), visitor.curScopeField());
        const auto &kwargsId = ast::CompileVisitor::VarID("kwargs", Pos::UNKNOW_POS.getFileField(), visitor.curScopeField());
        for (const auto& [funcName, funcPair] : pureBuiltinFunctionMap)
        {
            const auto &funcId = ast::CompileVisitor::VarID(funcName, Pos::UNKNOW_POS.getFileField(), visitor.curScopeField());
            const auto &argsParam = std::make_shared<symbol::ParameterSymbol>(symbol::ParamType::PARAM_VAR_LEN_POSITIONAL, Pos::UNKNOW_POS,
            argsId.getNameField(), argsId.getVid(),
            std::unordered_set<std::shared_ptr<symbol::LabelSymbol>>{},
            std::nullopt, visitor.curScopeLevel(),
            symbol::SymbolType::PARAMETER, nullptr);
            const auto &kwargsParam = std::make_shared<symbol::ParameterSymbol>(symbol::ParamType::PARAM_VAR_LEN_KEYWORD, Pos::UNKNOW_POS,
            kwargsId.getNameField(), kwargsId.getVid(),
            std::unordered_set<std::shared_ptr<symbol::LabelSymbol>>{},
            std::nullopt, visitor.curScopeLevel(),
            symbol::SymbolType::PARAMETER, nullptr);
            visitor.getSymbolTable().insert(std::make_shared<symbol::FunctionSymbol>(
                nullptr, Pos::UNKNOW_POS, funcName, funcId.getVid(),
                std::unordered_set<std::shared_ptr<symbol::LabelSymbol>>{
                    funcPair.first ?
                    symbol::TypeLabelSymbol::anyTypeSymbol(Pos::UNKNOW_POS, visitor.curScopeLevel()) :
                    symbol::TypeLabelSymbol::voidTypeSymbol(Pos::UNKNOW_POS, visitor.curScopeLevel())
                },
                std::vector{
                    argsParam, kwargsParam
                }, 0, symbol::TypeOfBuiltin::PURE_BUILTIN,
                symbol::FunctionType::FUNCTION, nullptr));
        }
    }

    bool isPureBuiltinFunction(const std::string& funcName)
    {
        return pureBuiltinFunctionMap.contains(funcName);
    }

    bool isBuiltinFunction(const std::string& funcName)
    {
        return builtinFunctionMap.contains(funcName);
    }

    bool isBuiltin(const std::string& funcName)
    {
        return isBuiltinFunction(funcName) || isPureBuiltinFunction(funcName);
    }

    BuiltinFuncRetType callBuiltinFunction(
        ast::CompileVisitor &visitor,
        const std::string &funcName, const std::vector<std::string> &processedArgs, const std::vector<std::string> &originalArgs)
    {
        if (const auto &it = pureBuiltinFunctionMap.find(funcName);
            it != pureBuiltinFunctionMap.end())
        {
            return it->second.second(visitor, processedArgs, originalArgs);
        }
        if (const auto &it = builtinFunctionMap.find(funcName);
            it != builtinFunctionMap.end())
        {
            return it->second(visitor, processedArgs, originalArgs);
        }
        throw std::runtime_error("Non-existent built-in function: '" + funcName + "'");
    }

    BuiltinFuncRetType rcc_import(ast::CompileVisitor &visitor, const std::vector<std::string> &processedArgs, const std::vector<std::string> &originalArgs)
    {
        if (originalArgs.size() != 1)
        {
            throw std::runtime_error("Failure to match parameters of the built-in function: 'import'");
        }
        const auto &importedFilePath = getAbsolutePath(StringManager::getInstance().unescape(originalArgs[0]),
            getFileDirFromPath(visitor.getProgramTargetFilePath()));
        const auto &anyTypeSymbol = symbol::TypeLabelSymbol::anyTypeSymbol(Pos::UNKNOW_POS, visitor.curScopeLevel());
        std::shared_ptr<symbol::ClassSymbol> extensionSymbol;
        std::string raCode = "";
        if (const auto &registeredExtension = visitor.getRegisteredExtension(importedFilePath))
        {
            extensionSymbol = registeredExtension;
            visitor.pushOpItem(ast::CompileVisitor::OpItemType::IDENTIFIER, anyTypeSymbol,
                extensionSymbol->getVal(), extensionSymbol->getRaVal(), extensionSymbol);
        } else
        {
            if (auto importVisitor = ast::CompileVisitor(visitor.getProgramEntryFilePath(), importedFilePath, "", false);
                importVisitor.compile())
            {
                raCode = importVisitor.getCompileResult();
                const auto &extension_name = "ext_" + getFileNameFromPath(importedFilePath) + "_" + visitor.getNewTempVarName();
                const auto &tempVarId = ast::CompileVisitor::VarID(extension_name, visitor.getProgramTargetFilePath(), visitor.curScopeField());
                extensionSymbol = std::make_shared<symbol::ClassSymbol>(
                    Pos(1, 1, 0, importedFilePath),
                    tempVarId.getNameField(), tempVarId.getVid(),
                    std::unordered_set<std::shared_ptr<symbol::LabelSymbol>>{},
                    visitor.curScopeLevel());
                visitor.getSymbolTable().insert(extensionSymbol);
                raCode += ri::TP_DEF(extensionSymbol->getRaVal()).toRACode();
                importVisitor.getSymbolTable().enterGlobalScope();
                const auto &ordinaryLabel = std::make_shared<symbol::LabelSymbol>(
                    Pos::UNKNOW_POS, "ordinary", "", visitor.curScopeLevel(), symbol::LabelType::LIFE_CYCLE_LABEL);
                const auto &publicLabel = std::make_shared<symbol::LabelSymbol>(
                    Pos::UNKNOW_POS, "public", "", visitor.curScopeLevel(), symbol::LabelType::PERMISSION_LABEL);
                for (const auto &member: importVisitor.getSymbolTable().currentScope())
                {
                    if (member->is(symbol::SymbolType::VARIABLE))
                    {
                        const auto &varSymbol = std::static_pointer_cast<symbol::VariableSymbol>(member);
                        varSymbol->getLabelMarkManager().resetPermissionLabelMarks();
                        varSymbol->getLabelMarkManager().markLabels({ordinaryLabel, publicLabel});
                        extensionSymbol->addMember(varSymbol);
                        raCode += ri::TP_ADD_TP_FIELD(extensionSymbol->getRaVal(),
                            StringManager::toStringFormat(varSymbol->getRaVal()),
                            varSymbol->getRaVal()).toRACode();
                    } else if (member->is(symbol::SymbolType::FUNCTION))
                    {
                        if (const auto &funcSymbol = std::static_pointer_cast<symbol::FunctionSymbol>(member);
                            funcSymbol->getBuiltInType() != symbol::TypeOfBuiltin::PURE_BUILTIN)
                        {
                            funcSymbol->getLabelMarkManager().resetPermissionLabelMarks();
                            funcSymbol->getLabelMarkManager().markLabels({ordinaryLabel, publicLabel});
                            funcSymbol->setFunctionType(symbol::FunctionType::METHOD);
                            extensionSymbol->addMember(funcSymbol);
                            raCode += ri::TP_ADD_TP_FIELD(extensionSymbol->getRaVal(),
                                StringManager::toStringFormat(funcSymbol->getRaVal()),
                                funcSymbol->getRaVal()).toRACode();
                        }
                    }
                    else if (member->is(symbol::SymbolType::CLASS))
                    {
                        const auto &classSymbol = std::static_pointer_cast<symbol::ClassSymbol>(member);
                        classSymbol->getLabelMarkManager().resetPermissionLabelMarks();
                        classSymbol->getLabelMarkManager().markLabels({ordinaryLabel, publicLabel});
                        extensionSymbol->addMember(classSymbol);
                        raCode += ri::TP_ADD_TP_FIELD(extensionSymbol->getRaVal(),
                            StringManager::toStringFormat(classSymbol->getRaVal()),
                            classSymbol->getRaVal()).toRACode();
                    }
                    else
                    {
                        throw std::runtime_error("Cannot resolve symbol: '" + member->getVal() + "'");
                    }
                }
                extensionSymbol->setCollectionFinished();
                extensionSymbol->setVisitPermission(symbol::PermissionLabel::PUBLIC);
                symbol::TypeLabelSymbol::createCustomType(extensionSymbol->getVal(), extensionSymbol->getRaVal(), extensionSymbol);
                visitor.registerExtension(importedFilePath, extensionSymbol);
                visitor.pushIdentItem(
                    tempVarId, anyTypeSymbol, anyTypeSymbol, extensionSymbol);
            } else
            {
                throw std::runtime_error("Failure to import file: '" + importedFilePath + "'");
            }
        }
        return raCode;
    }

    BuiltinFuncRetType rcc_export(ast::CompileVisitor &visitor, const std::vector<std::string> &processedArgs, const std::vector<std::string> &originalArgs)
    {
        for (const auto &arg: originalArgs)
        {
            if (const auto &[level, argSymbol] = visitor.getSymbolTable().find(arg);
                level >= 0 && argSymbol)
            {
                argSymbol->setScopeLevel(0);
                visitor.getSymbolTable().remove(argSymbol->getVal());
                visitor.getSymbolTable().insert(argSymbol);
            } else
            {
                throw std::runtime_error("Non-existent symbol: '" + arg + "'");
            }
        }
        return "";
    }

    BuiltinFuncRetType rcc_sout(ast::CompileVisitor &visitor, const std::vector<std::string> &processedArgs, const std::vector<std::string> &originalArgs)
    {
        if (processedArgs.size() != 2)
        {
            throw std::runtime_error("Failure to match parameters of the built-in function: 'sout'");
        }
        const auto &raCode = ri::SOUT("s-l", {"s-unpack", processedArgs[0], processedArgs[1]}).toRACode();
        return raCode;
    }

    BuiltinFuncRetType rcc_size(ast::CompileVisitor &visitor, const std::vector<std::string> &processedArgs, const std::vector<std::string> &originalArgs)
    {
        if (processedArgs.size() != 1)
        {
            throw std::runtime_error("Failure to match parameters of the built-in function: 'size'");
        }
        visitor.pushTemOpVarItemWithRecord(Pos::UNKNOW_POS, symbol::TypeLabelSymbol::intTypeSymbol(Pos::UNKNOW_POS, visitor.getSymbolTable().curScopeLevel()));
        const auto &raCode = ri::ITER_SIZE(processedArgs[0], visitor.topOpRaVal()).toRACode();
        return raCode;
    }

    BuiltinFuncRetType rcc_sin(ast::CompileVisitor &visitor, const std::vector<std::string> &processedArgs, const std::vector<std::string> &originalArgs)
    {
        if (processedArgs.size() != 1)
        {
            throw std::runtime_error("Failure to match parameters of the built-in function: 'sin'");
        }
        visitor.pushTemOpVarItemWithRecord(Pos::UNKNOW_POS, symbol::TypeLabelSymbol::strTypeSymbol(Pos::UNKNOW_POS, visitor.getSymbolTable().curScopeLevel()));

        const auto &raCode = ri::SOUT("s-l", {processedArgs[0]}).toRACode()
        + ri::SIN({"s-m", visitor.topOpRaVal()}).toRACode();
        return raCode;
    }

    BuiltinFuncRetType rcc_type(ast::CompileVisitor &visitor, const std::vector<std::string> &processedArgs, const std::vector<std::string> &originalArgs)
    {
        if (processedArgs.size() != 1)
        {
            throw std::runtime_error("Failure to match parameters of the built-in function: 'type'");
        }
        visitor.pushTemOpVarItemWithRecord(Pos::UNKNOW_POS, symbol::TypeLabelSymbol::strTypeSymbol(Pos::UNKNOW_POS, visitor.getSymbolTable().curScopeLevel()));
        const auto &raCode = ri::TP_GET(processedArgs[0], visitor.topOpRaVal()).toRACode();
        return raCode;
    }

    BuiltinFuncRetType rcc_setType(ast::CompileVisitor &visitor, const std::vector<std::string> &processedArgs, const std::vector<std::string> &originalArgs)
    {
        if (processedArgs.size() != 2)
        {
            throw std::runtime_error("Failure to match parameters of the built-in function: 'setType'");
        }
        const auto &typeSymbol = symbol::TypeLabelSymbol::getTypeLabelSymbolByStr(
            StringManager::getInstance().unescape(processedArgs[1]), visitor.getSymbolTable().curScopeLevel());
        const auto &raCode = ri::TP_SET(typeSymbol->getRaVal(), processedArgs[0]).toRACode();
        return raCode;
    }

    BuiltinFuncRetType rcc_entry(ast::CompileVisitor &visitor, const std::vector<std::string> &processedArgs, const std::vector<std::string> &originalArgs)
    {
        if (processedArgs.size() < 1)
        {
            throw std::runtime_error("Failure to match parameters of the built-in function: 'entry'");
        }
        const auto &calledFuncName = originalArgs[0];
        if (auto [level, symbol] = visitor.getSymbolTable().find(calledFuncName);
            level >= 0 && symbol)
        {
            if (symbol->is(symbol::SymbolType::VARIABLE))
            {
                symbol = ast::CompileVisitor::getReferenceTargetSymbol(std::static_pointer_cast<symbol::VariableSymbol>(symbol));
            }
            if (symbol->isNot(symbol::SymbolType::FUNCTION))
            {
                throw std::runtime_error("'" + calledFuncName + "' is not a function");
            }
            const auto &funcSymbol = std::static_pointer_cast<symbol::FunctionSymbol>(symbol);
            if (const auto &parameters = funcSymbol->getParameters();
                parameters.size() != 2 || parameters[0]->getParamType() != symbol::ParamType::PARAM_VAR_LEN_POSITIONAL ||
                parameters[1]->getParamType() != symbol::ParamType::PARAM_VAR_LEN_KEYWORD)
            {
                throw std::runtime_error("The function called in the pure-built-in function 'entry' fails to match the passed parameters: '" + funcSymbol->getVal() + "'");
            }
            const std::vector<std::string> &args = {processedArgs.begin() + 1, processedArgs.end()};
            if (checkPathEqual(visitor.getProgramEntryFilePath(), visitor.getCurrentProcessingFilePath()))
            {
                if (funcSymbol->hasReturnValue())
                {
                    visitor.pushTemOpVarItemWithRecord(funcSymbol->getPos(), funcSymbol->getReturnType());
                    return ri::IVOK(processedArgs[0], args, visitor.topOpRaVal()).toRACode();
                }
                return ri::CALL(processedArgs[0], args).toRACode();
            }
            if (funcSymbol->hasReturnValue())
            {
                visitor.pushOpItem(
                    ast::CompileVisitor::OpItemType::LITERAL_VALUE,
                    symbol::TypeLabelSymbol::nulTypeSymbol(Pos::UNKNOW_POS, visitor.curScopeLevel()),
                    "null", "null");
                return "";
            }
            return "";
        }
        throw std::runtime_error("Non-existent symbol: '" + calledFuncName + "'");
    }

    std::unordered_map<std::string, std::pair<bool, BuiltinFunc>> pureBuiltinFunctionMap = {
        // full-built-in functions
        {"import", {true, rcc_import}},
        {"export", {false, rcc_export}},
    };

    std::unordered_map<std::string, BuiltinFunc> builtinFunctionMap = {
        // built-in functions
        {"sout", rcc_sout},
        {"size", rcc_size},
        {"sin", rcc_sin},
        {"type", rcc_type},
        {"setType", rcc_setType},
        {"entry", rcc_entry},
    };
}
//
// Created by RestRegular on 2025/9/16.
//

#include "../../../declarations/builtin/functions/rcc_builtin_datatype_functions_dec.h"

namespace builtin
{
    BuiltinFuncRetType rcc_type(ast::CompileVisitor &visitor, const CallInfos& callInfos)
    {
        if (callInfos.processedArgs.size() != 1)
        {
            throw std::runtime_error("Failure to match parameters of the built-in function: 'type'");
        }
        std::shared_ptr<symbol::ClassSymbol> classSymbol = nullptr;
        const auto& clasTypeLabel = symbol::TypeLabelSymbol::clasTypeSymbol(utils::getUnknownPos(), visitor.curScopeLevel());
        std::shared_ptr<symbol::TypeLabelSymbol> typeSymbol = symbol::TypeLabelSymbol::strTypeSymbol(
            utils::getUnknownPos(), visitor.curScopeLevel());
        if (const auto& [_, symbol] = visitor.getSymbolTable().findByRID(callInfos.processedArgs[0]);
            symbol)
        {
            if (const auto& symbolTypeLabel = visitor.getTypeLabelFromSymbol(symbol);
                symbolTypeLabel->isCustomType())
            {
                classSymbol = symbolTypeLabel->getCustomClassSymbol();
                typeSymbol = classSymbol->getClassTypeLabelSymbol(utils::getUnknownPos());
                clasTypeLabel->appendLastLabelDes({typeSymbol});
            }
        }
        const auto& resultType = classSymbol ? clasTypeLabel : typeSymbol;
        visitor.pushTemOpVarItemWithRecord(utils::getUnknownPos(),
            resultType, nullptr, true, resultType);
        const auto &raCode = ri::TP_GET(callInfos.processedArgs[0], visitor.topOpRaVal()).toRACode();
        return raCode;
    }

    namespace detail {
        inline std::shared_ptr<symbol::TypeLabelSymbol> resolveTypeSymbol(
            ast::CompileVisitor& visitor,
            const std::string& argStr,
            bool& outIsClass,
            std::string& outRaCode,
            const std::string& setTarget)
        {
            std::shared_ptr<symbol::TypeLabelSymbol> typeSymbol = nullptr;
            outIsClass = false;
            outRaCode.clear();

            if (const auto& [level, symbol] = visitor.getSymbolTable().findByRID(argStr);
                level >= 0 && symbol)
            {
                if (symbol->is(symbol::SymbolType::CLASS))
                {
                    const auto& classSymbol = std::static_pointer_cast<symbol::ClassSymbol>(symbol);
                    typeSymbol = classSymbol->getClassTypeLabelSymbol(utils::getUnknownPos());
                    if (!setTarget.empty())
                    {
                        outRaCode += ri::TP_SET(classSymbol->getRaVal(), setTarget).toRACode();
                    }
                    outIsClass = true;
                }
                else if (symbol->is(symbol::SymbolType::VARIABLE))
                {
                    if (const auto& targetSymbol = visitor.getReferenceTargetSymbol(
                            std::static_pointer_cast<symbol::VariableSymbol>(symbol));
                        targetSymbol->is(symbol::SymbolType::CLASS))
                    {
                        const auto& classSymbol = std::static_pointer_cast<symbol::ClassSymbol>(targetSymbol);
                        typeSymbol = classSymbol->getClassTypeLabelSymbol(utils::getUnknownPos());
                        if (!setTarget.empty())
                        {
                            outRaCode += ri::TP_SET(classSymbol->getRaVal(), setTarget).toRACode();
                        }
                        outIsClass = true;
                    }
                }
            }

            if (!outIsClass)
            {
                typeSymbol = symbol::TypeLabelSymbol::getTypeLabelSymbolByStr(
                    utils::StringManager::parseStringFormat(argStr),
                    visitor.getSymbolTable().curScopeLevel());

                if (!setTarget.empty())
                {
                    outRaCode += ri::TP_SET(typeSymbol->getRaVal(), setTarget).toRACode();
                }
            }

            return typeSymbol;
        }

        inline void validateAndSetTypeSymbol(
            ast::CompileVisitor& visitor,
            const CallInfos& callInfos,
            const std::shared_ptr<symbol::TypeLabelSymbol>& typeSymbol,
            const std::shared_ptr<symbol::VariableSymbol>& varSymbol,
            std::string& raCode)
        {
            if (typeSymbol->isCustomType() &&
                !varSymbol->getTypeLabel()->relatedTo(typeSymbol))
            {
                throw base::RCCCompilerError::typeMissmatchError(
                    callInfos.callPos.toString(),
                    visitor.getCodeLine(callInfos.callPos),
                    {
                        "The custom type to be set is inconsistent with the type label restriction of the variable or has no association."
                        "Error symbol: " + varSymbol->toString()
                    },
                    varSymbol->getTypeLabel()->toString(),
                    typeSymbol->toString(),
                    {
                        "Ensure that the type to be set is consistent with the type label restriction of the variable or inherits association."
                    },
                    "Actual type: ",
                    "Setting type: ");
            }

            varSymbol->setTypeLabel(typeSymbol);
            varSymbol->setValueType(typeSymbol);
            visitor.pushTemOpVarItemWithRecord(utils::getUnknownPos(), typeSymbol, varSymbol, true, typeSymbol);
            raCode += ri::PUT(varSymbol->getRaVal(), visitor.topOpRaVal()).toRACode();
        }

        inline void validateArgCount(const CallInfos& callInfos, const std::string& funcName)
        {
            if (callInfos.processedArgs.size() != 2)
            {
                throw std::runtime_error("Failure to match parameters of the built-in function: '" + funcName + "'");
            }
        }

        inline std::shared_ptr<symbol::VariableSymbol> findTargetVariable(
            ast::CompileVisitor& visitor,
            const std::string& varName)
        {
            if (const auto& [_, symbol] = visitor.getSymbolTable().findVariableSymbolByName(varName, false);
                symbol)
            {
                return std::static_pointer_cast<symbol::VariableSymbol>(symbol);
            }
            return nullptr;
        }
    }

    BuiltinFuncRetType rcc_setType(ast::CompileVisitor& visitor, const CallInfos& callInfos)
    {
        detail::validateArgCount(callInfos, "setType");

        bool isClass = false;
        std::string raCode = "";

        auto typeSymbol = detail::resolveTypeSymbol(
            visitor,
            callInfos.processedArgs[1],
            isClass,
            raCode,
            callInfos.processedArgs[0]);

        if (const auto varSymbol = detail::findTargetVariable(visitor, callInfos.originalArgs[0].second))
        {
            detail::validateAndSetTypeSymbol(visitor, callInfos, typeSymbol, varSymbol, raCode);
            return raCode;
        }

        throw std::runtime_error("Failed to find variable '" + callInfos.originalArgs[0].second + "'");
    }

    BuiltinFuncRetType rcc_setLabel(ast::CompileVisitor& visitor, const CallInfos& callInfos)
    {
        detail::validateArgCount(callInfos, "setLabel");

        bool isClass = false;
        std::string raCode = "";

        const auto typeSymbol = detail::resolveTypeSymbol(
            visitor,
            callInfos.processedArgs[1],
            isClass,
            raCode,
            ""); // setLabel 不需要生成 TP_SET

        if (auto varSymbol = detail::findTargetVariable(visitor, callInfos.originalArgs[0].second))
        {
            detail::validateAndSetTypeSymbol(visitor, callInfos, typeSymbol, varSymbol, raCode);
            return raCode;
        }

        throw std::runtime_error("Failed to find variable '" + callInfos.originalArgs[0].second + "'");
    }

    BuiltinFuncRetType rcc_check_type(ast::CompileVisitor& visitor, const CallInfos& callInfos)
    {
        const auto &checkArg = callInfos.originalArgs[0].second;
        const auto &expectedType = callInfos.originalArgs[1].second;
        if (utils::isStringFormat(checkArg))
        {

        } else
        {
            if (const auto &[level, symbol] = visitor.getSymbolTable().findByName(checkArg);
                level > 0 && symbol)
            {

            } else
            {

            }
        }
        return "";
    }

    BuiltinFuncRetType rcc_relate(ast::CompileVisitor& visitor, const CallInfos& callInfos)
    {
        if (callInfos.processedArgs.size() != 2)
        {
            throw std::runtime_error("Failure to match parameters of the built-in function: 'relate'");
        }
        const auto &checkArg = callInfos.processedArgs[0];
        const auto &expectedType = callInfos.processedArgs[1];
        visitor.pushTemOpVarItemWithRecord(callInfos.callPos, symbol::TypeLabelSymbol::boolTypeSymbol(callInfos.callPos, visitor.curScopeLevel()),
            nullptr, true);
        return ri::TP_CHECK_RELATE(checkArg, expectedType, visitor.topOpRaVal()).toRACode();
    }
}

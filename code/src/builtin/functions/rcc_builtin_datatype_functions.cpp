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
        visitor.pushTemOpVarItemWithRecord(utils::getUnknownPos(), symbol::TypeLabelSymbol::strTypeSymbol(utils::getUnknownPos(), visitor.getSymbolTable().curScopeLevel()));
        const auto &raCode = ri::TP_GET(callInfos.processedArgs[0], visitor.topOpRaVal()).toRACode();
        return raCode;
    }

    BuiltinFuncRetType rcc_setType(ast::CompileVisitor &visitor, const CallInfos& callInfos)
    {
        if (callInfos.processedArgs.size() != 2)
        {
            throw std::runtime_error("Failure to match parameters of the built-in function: 'setType'");
        }
        std::shared_ptr<symbol::TypeLabelSymbol> typeSymbol = nullptr;
        std::string raCode = "";
        if (const auto &[level, symbol] = visitor.getSymbolTable().findByRID(callInfos.processedArgs[1]);
            level >= 0 && symbol && symbol->is(symbol::SymbolType::CLASS))
        {
            const auto &classSymbol = std::static_pointer_cast<symbol::ClassSymbol>(symbol);
            typeSymbol = classSymbol->getClassTypeLabelSymbol(utils::getUnknownPos());
            raCode += ri::ANNOTATION("No need to explicitly set '" + classSymbol->getVal() + "' type .").toRACode();
        } else
        {
            typeSymbol = symbol::TypeLabelSymbol::getTypeLabelSymbolByStr(
                utils::StringManager::parseStringFormat(callInfos.processedArgs[1]), visitor.getSymbolTable().curScopeLevel());
            raCode += ri::TP_SET(typeSymbol->getRaVal(), callInfos.processedArgs[0]).toRACode();
        }
        const auto &[_, symbol] = visitor.getSymbolTable().findVariableSymbolByName(callInfos.originalArgs[0].second);
        symbol->setTypeLabel(typeSymbol);
        symbol->setValueType(typeSymbol);
        return raCode;
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

}

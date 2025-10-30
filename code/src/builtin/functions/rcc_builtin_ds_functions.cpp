//
// Created by RestRegular on 2025/9/16.
//

#include "../../../declarations/builtin/functions/rcc_builtin_ds_functions_dec.h"

namespace builtin
{
    BuiltinFuncRetType rcc_size(ast::CompileVisitor &visitor, const CallInfos& callInfos)
    {
        if (callInfos.processedArgs.size() != 1)
        {
            throw std::runtime_error("Failure to match parameters of the built-in function: 'size'");
        }
        visitor.pushTemOpVarItemWithRecord(utils::getUnknownPos(), symbol::TypeLabelSymbol::intTypeSymbol(utils::getUnknownPos(), visitor.getSymbolTable().curScopeLevel()));
        return ri::ITER_SIZE(callInfos.processedArgs[0], visitor.topOpRaVal()).toRACode();
    }

    BuiltinFuncRetType rcc_copy(ast::CompileVisitor& visitor, const CallInfos& callInfos)
    {
        if (callInfos.processedArgs.size() != 1)
        {
            throw std::runtime_error("Failure to match parameters of the built-in function: 'copy'");
        }
        const auto &[typeLabel, valueType] = visitor.getTypesFromOpItem(callInfos.posArgOpItems.front());
        visitor.pushTemOpVarItemWithRecord(utils::getUnknownPos(), valueType, nullptr, true, typeLabel);
        return ri::COPY(callInfos.processedArgs[0], visitor.topOpRaVal()).toRACode();
    }

    BuiltinFuncRetType rcc_listRemove(ast::CompileVisitor& visitor, const CallInfos& callInfos)
    {
        if (const auto &[typeLabel, valueType] = visitor.getTypesFromOpItem(callInfos.posArgOpItems.front());
            typeLabel->is("list") || typeLabel->is("str") ||
            (typeLabel->is("any") && (valueType->is("list") ||
                valueType->is("str") || valueType->is("any"))))
        {
            return ri::ITER_DEL(callInfos.processedArgs[0], callInfos.processedArgs[1]).toRACode();
        }
        const auto &firstSymbol = visitor.getSymbolFromOpItem(callInfos.posArgOpItems.front());
        const auto &errorPos = firstSymbol ? firstSymbol->getPos() : callInfos.callPos;
        throw base::RCCCompilerError::typeMissmatchError(errorPos.toString(), visitor.getCodeLine(errorPos),
            "The builtin function 'listRemove' expected the first argument to be a list or a str.",
            visitor.getListFormatString({"list", "str", "any"}),
            firstSymbol ? visitor.getTypeLabelFromSymbol(firstSymbol)->toString() :
            callInfos.posArgOpItems.front().getValueType()->toString(), {});
    }

    BuiltinFuncRetType rcc_dictRemove(ast::CompileVisitor& visitor, const CallInfos& callInfos)
    {
        if (const auto &[typeLabel, valueType] = visitor.getTypesFromOpItem(callInfos.posArgOpItems.front());
                    typeLabel->is("dict") ||
                    (typeLabel->is("any") && (valueType->is("dict") ||
                        valueType->is("any"))))
        {
            return ri::DICT_DEL(callInfos.processedArgs[0], callInfos.processedArgs[1]).toRACode();
        }
        const auto &firstSymbol = visitor.getSymbolFromOpItem(callInfos.posArgOpItems.front());
        const auto &errorPos = firstSymbol ? firstSymbol->getPos() : callInfos.callPos;
        throw base::RCCCompilerError::typeMissmatchError(errorPos.toString(), visitor.getCodeLine(errorPos),
            "The builtin function 'listRemove' expected the first argument to be a dict.",
            visitor.getListFormatString({"dict", "any"}),
            firstSymbol ? visitor.getTypeLabelFromSymbol(firstSymbol)->toString() :
            callInfos.posArgOpItems.front().getValueType()->toString(), {});
    }

    BuiltinFuncRetType rcc_dictKeys(ast::CompileVisitor& visitor, const CallInfos& callInfos)
    {
        visitor.pushTemOpVarItemWithRecord(utils::getUnknownPos(), symbol::TypeLabelSymbol::listTypeSymbol(utils::getUnknownPos(), visitor.getSymbolTable().curScopeLevel()));
        return ri::DICT_KEYS(callInfos.processedArgs[0], visitor.topOpRaVal()).toRACode();
    }

    BuiltinFuncRetType rcc_dictValues(ast::CompileVisitor& visitor, const CallInfos& callInfos)
    {
        visitor.pushTemOpVarItemWithRecord(utils::getUnknownPos(), symbol::TypeLabelSymbol::listTypeSymbol(utils::getUnknownPos(), visitor.getSymbolTable().curScopeLevel()));
        return ri::DICT_VALUES(callInfos.processedArgs[0], visitor.topOpRaVal()).toRACode();
    }
}

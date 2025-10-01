//
// Created by RestRegular on 2025/9/16.
//

#include "../../../include/builtin/functions/rcc_builtin_ds_functions.h"

namespace builtin
{
    BuiltinFuncRetType rcc_size(ast::CompileVisitor &visitor, const CallInfos& callInfos)
    {
        if (callInfos.processedArgs.size() != 1)
        {
            throw std::runtime_error("Failure to match parameters of the built-in function: 'size'");
        }
        visitor.pushTemOpVarItemWithRecord(utils::Pos::UNKNOW_POS, symbol::TypeLabelSymbol::intTypeSymbol(utils::Pos::UNKNOW_POS, visitor.getSymbolTable().curScopeLevel()));
        const auto &raCode = ri::ITER_SIZE(callInfos.processedArgs[0], visitor.topOpRaVal()).toRACode();
        return raCode;
    }
}
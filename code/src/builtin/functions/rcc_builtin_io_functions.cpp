//
// Created by RestRegular on 2025/9/16.
//

#include "../../../include/builtin/functions/rcc_builtin_io_functions.h"

namespace builtin
{
    BuiltinFuncRetType rcc_sout(ast::CompileVisitor &visitor, const CallInfos& callInfos)
    {
        if (callInfos.processedArgs.size() != 2)
        {
            throw std::runtime_error("Failure to match parameters of the built-in function: 'sout'");
        }
        const auto &raCode = ri::SOUT("s-l", {"s-unpack", callInfos.processedArgs[0], callInfos.processedArgs[1]}).toRACode();
        return raCode;
    }

    BuiltinFuncRetType rcc_sin(ast::CompileVisitor &visitor, const CallInfos& callInfos)
    {
        if (callInfos.processedArgs.size() != 1)
        {
            throw std::runtime_error("Failure to match parameters of the built-in function: 'sin'");
        }
        visitor.pushTemOpVarItemWithRecord(utils::Pos::UNKNOW_POS, symbol::TypeLabelSymbol::strTypeSymbol(utils::Pos::UNKNOW_POS, visitor.getSymbolTable().curScopeLevel()));

        const auto &raCode = ri::SOUT("s-l", {callInfos.processedArgs[0]}).toRACode()
        + ri::SIN({"s-m", visitor.topOpRaVal()}).toRACode();
        return raCode;
    }
}
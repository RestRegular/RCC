//
// Created by RestRegular on 2025/9/16.
//

#include "../../../include/builtin/functions/rcc_builtin_export.h"

namespace builtin
{

    BuiltinFuncRetType rcc_export(ast::CompileVisitor& visitor, const CallInfos& callInfos)
    {
        for (const auto& arg : callInfos.originalArgs | std::views::values)
        {
            if (const auto &[level, argSymbol] = visitor.getSymbolTable().findByName(arg);
                level >= 0 && argSymbol)
            {
                argSymbol->setScopeLevel(0);
                visitor.getSymbolTable().removeByName(argSymbol->getVal());
                visitor.getSymbolTable().insert(argSymbol, false);
            } else
            {
                throw std::runtime_error("Non-existent symbol: '" + arg + "'");
            }
        }
        return "";
    }

    const PureBuiltinFunction EXPORT (false, "export",
        {getPosVarArgsParam("args"), getKeywordVarArgsParam("kwargs")},
        rcc_export);
}

//
// Created by RestRegular on 2025/9/22.
//

#include "../../../include/builtin/functions/rcc_builtin_lowlevel_functions.h"

namespace builtin
{
    BuiltinFuncRetType rcc_id(ast::CompileVisitor& visitor, const CallInfos& callInfos)
    {
        visitor.pushOpItem(
            ast::CompileVisitor::OpItemType::LITERAL_VALUE,
            symbol::TypeLabelSymbol::strTypeSymbol(
                utils::Pos::UNKNOW_POS, visitor.curScopeLevel()),
                utils::StringManager::toStringFormat(callInfos.processedArgs[0]));
        return "";
    }

    BuiltinFuncRetType rcc_rasm(ast::CompileVisitor& visitor, const CallInfos& callInfos)
    {
        return "";
    }

}
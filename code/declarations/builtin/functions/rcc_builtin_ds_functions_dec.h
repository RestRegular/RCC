//
// Created by RestRegular on 2025/9/16.
//

#ifndef RCC_RCC_BUILTIN_DS_FUNCTIONS_H
#define RCC_RCC_BUILTIN_DS_FUNCTIONS_H

#include "../../../include/builtin/rcc_builtin_core.h"

namespace builtin
{

    BuiltinFuncRetType rcc_size(ast::CompileVisitor &visitor, const CallInfos& callInfos);

    BuiltinFuncRetType rcc_copy(ast::CompileVisitor &visitor, const CallInfos& callInfos);

    BuiltinFuncRetType rcc_listAppend(ast::CompileVisitor &visitor, const CallInfos& callInfos);

    BuiltinFuncRetType rcc_listRemove(ast::CompileVisitor &visitor, const CallInfos& callInfos);

    BuiltinFuncRetType rcc_dictRemove(ast::CompileVisitor &visitor, const CallInfos& callInfos);

    BuiltinFuncRetType rcc_dictKeys(ast::CompileVisitor &visitor, const CallInfos& callInfos);

    BuiltinFuncRetType rcc_dictValues(ast::CompileVisitor &visitor, const CallInfos& callInfos);

    BuiltinFuncRetType rcc_repeat(ast::CompileVisitor &visitor, const CallInfos& callInfos);

}

#endif //RCC_RCC_BUILTIN_DS_FUNCTIONS_H
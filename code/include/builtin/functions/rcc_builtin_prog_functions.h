//
// Created by RestRegular on 2025/9/16.
//

#ifndef RCC_RCC_BUILTIN_PROG_FUNCTIONS_H
#define RCC_RCC_BUILTIN_PROG_FUNCTIONS_H

#include "../rcc_builtin_core.h"

namespace builtin
{

    BuiltinFuncRetType rcc_entry(ast::CompileVisitor &visitor, const CallInfos& callInfos);

}

#endif //RCC_RCC_BUILTIN_PROG_FUNCTIONS_H
//
// Created by RestRegular on 2025/9/16.
//

#ifndef RCC_RCC_BUILTIN_IO_FUNCTIONS_H
#define RCC_RCC_BUILTIN_IO_FUNCTIONS_H

#include "../rcc_builtin_core.h"

namespace builtin
{
    BuiltinFuncRetType rcc_sout(ast::CompileVisitor &visitor, const CallInfos& callInfos);

    BuiltinFuncRetType rcc_sin(ast::CompileVisitor &visitor, const CallInfos& callInfos);
}

#endif //RCC_RCC_BUILTIN_IO_FUNCTIONS_H
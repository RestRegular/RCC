//
// Created by RestRegular on 2025/9/22.
//

#ifndef RCC_RCC_BUILTIN_LOWLEVEL_FUNCTIONS_H
#define RCC_RCC_BUILTIN_LOWLEVEL_FUNCTIONS_H

#include "../rcc_builtin_core.h"

namespace builtin
{

    BuiltinFuncRetType rcc_id(ast::CompileVisitor &visitor, const CallInfos& callInfos);

    BuiltinFuncRetType rcc_rasm(ast::CompileVisitor &visitor, const CallInfos& callInfos);

}

#endif //RCC_RCC_BUILTIN_LOWLEVEL_FUNCTIONS_H
//
// Created by RestRegular on 2025/9/30.
//

#ifndef RCC_RCC_BUILTIN_SUPER_H
#define RCC_RCC_BUILTIN_SUPER_H

#include "../rcc_builtin_core.h"

namespace builtin
{

    BuiltinFuncRetType rcc_super(ast::CompileVisitor &visitor, const CallInfos& callInfos);

    extern const PureBuiltinFunction SUPER;
}

#endif //RCC_RCC_BUILTIN_SUPER_H
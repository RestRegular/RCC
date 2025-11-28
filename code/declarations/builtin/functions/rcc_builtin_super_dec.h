//
// Created by RestRegular on 2025/9/30.
//

#ifndef RCC_RCC_BUILTIN_SUPER_H
#define RCC_RCC_BUILTIN_SUPER_H

#include "../../../include/builtin/rcc_builtin_core.h"

namespace builtin
{

    BuiltinFuncRetType rcc_super(ast::CompileVisitor &visitor, const CallInfos& callInfos);
    const PureBuiltinFunction& getSupperFunction();
}

#endif //RCC_RCC_BUILTIN_SUPER_H
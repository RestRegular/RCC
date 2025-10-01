//
// Created by RestRegular on 2025/9/16.
//

#ifndef RCC_RCC_BUILTIN_EXPORT_H
#define RCC_RCC_BUILTIN_EXPORT_H

#include "../rcc_builtin_core.h"

namespace builtin
{

    BuiltinFuncRetType rcc_export(ast::CompileVisitor &visitor, const CallInfos& callInfos);

    extern const PureBuiltinFunction EXPORT;
}

#endif //RCC_RCC_BUILTIN_EXPORT_H
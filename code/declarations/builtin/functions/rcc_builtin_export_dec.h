//
// Created by RestRegular on 2025/9/16.
//

#ifndef RCC_RCC_BUILTIN_EXPORT_H
#define RCC_RCC_BUILTIN_EXPORT_H

#include "../../../include/builtin/rcc_builtin_core.h"

namespace builtin
{
    BuiltinFuncRetType rcc_export(ast::CompileVisitor &visitor, const CallInfos& callInfos);
    const PureBuiltinFunction& getExportFunction();
}

#endif //RCC_RCC_BUILTIN_EXPORT_H
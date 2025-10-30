//
// Created by RestRegular on 2025/8/5.
//

#ifndef RCC_BUILTIN_H
#define RCC_BUILTIN_H

#include <unordered_map>
#include "./rcc_builtin_core.h"
#include "../../declarations/builtin/rcc_builtin_dec.h"

namespace builtin
{
    extern std::unordered_map<std::string, PureBuiltinFunction> pureBuiltinFunctionMap;
    extern std::unordered_map<std::string, BuiltinFunc> builtinFunctionMap;
}

#endif //RCC_BUILTIN_H

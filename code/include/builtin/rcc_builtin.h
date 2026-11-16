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
    // 静态初始化顺序安全：原先这里是两个命名空间作用域的 unordered_map，
    // 其中 pureBuiltinFunctionMap 在初始化时就调用 getImportFunction() /
    // getExportFunction() / getSupperFunction()，而这些函数内部会构造
    // ParameterSymbol / TypeLabelSymbol，进而访问 TypeLabelSymbol 的静态类型表。
    // 跨编译单元的静态初始化顺序未定义，一旦在类型表初始化前触发，就会在
    // main 之前崩溃（unordered_map::find 里 hash % 0 触发整数除零 0xC0000094）。
    // 改为 Meyers singleton，首次使用时才初始化。
    std::unordered_map<std::string, PureBuiltinFunction>& pureBuiltinFunctionMapRef();
    std::unordered_map<std::string, BuiltinFunc>& builtinFunctionMapRef();
}

#endif //RCC_BUILTIN_H

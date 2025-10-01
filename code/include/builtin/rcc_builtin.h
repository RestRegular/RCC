//
// Created by RestRegular on 2025/8/5.
//

#ifndef RCC_BUILTIN_H
#define RCC_BUILTIN_H

#include <unordered_map>
#include "./rcc_builtin_core.h"

namespace builtin
{

    extern std::unordered_map<std::string, PureBuiltinFunction> pureBuiltinFunctionMap;
    extern std::unordered_map<std::string, BuiltinFunc> builtinFunctionMap;

    void initializePureBuiltinEnvironment(ast::CompileVisitor &visitor);

    bool isPureBuiltinFunction(const std::string &funcName);
    bool isBuiltinFunction(const std::string &funcName);
    bool isBuiltin(const std::string &funcName);

    BuiltinFuncRetType callBuiltinFunction(
        ast::CompileVisitor &visitor, const std::string &funcName, const CallInfos& callInfos);

}

#endif //RCC_BUILTIN_H

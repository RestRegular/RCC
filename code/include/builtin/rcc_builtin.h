//
// Created by RestRegular on 2025/8/5.
//

#ifndef RCC_BUILTIN_H
#define RCC_BUILTIN_H

#include <unordered_map>
#include <functional>
#include "../visitors/rcc_compiler_visitor.h"

namespace builtin
{
    typedef std::string BuiltinFuncRetType;

    typedef std::function<BuiltinFuncRetType(
        ast::CompileVisitor &visitor,
        const std::vector<std::string> &,
        const std::vector<std::string> &)> BuiltinFunc;

    extern std::unordered_map<std::string, std::pair<bool, BuiltinFunc>> pureBuiltinFunctionMap;
    extern std::unordered_map<std::string, BuiltinFunc> builtinFunctionMap;

    void initializePureBuiltinEnvironment(ast::CompileVisitor &visitor);

    bool isPureBuiltinFunction(const std::string &funcName);
    bool isBuiltinFunction(const std::string &funcName);
    bool isBuiltin(const std::string &funcName);

    BuiltinFuncRetType callBuiltinFunction(
        ast::CompileVisitor &visitor, const std::string &funcName,
        const std::vector<std::string> &processedArgs,
        const std::vector<std::string> &originalArgs);
}

#endif //RCC_BUILTIN_H

//
// Created by RestRegular on 2025/10/22.
//

#ifndef RCC_RCC_BUILTIN_DEC_H
#define RCC_RCC_BUILTIN_DEC_H

#include <functional>
#include <memory>

#include "../analyzer/rcc_ast_dec.h"
#include "../../declarations/components/symbol/rcc_symbol_dec.h"

namespace builtin
{
    struct CallInfos;
    class PureBuiltinFunction;

    using BuiltinFuncRetType = std::string;
    using BuiltinFunc = std::function<BuiltinFuncRetType(
        ast::CompileVisitor &visitor,
        const CallInfos &)>;

    void initializePureBuiltinEnvironment(ast::CompileVisitor &visitor);

    bool isPureBuiltinFunction(const std::string &funcName);
    bool isBuiltinFunction(const std::string &funcName);
    bool isBuiltin(const std::string &funcName);

    BuiltinFuncRetType callBuiltinFunction(
        ast::CompileVisitor &visitor, const std::string &funcName, const CallInfos& callInfos);

    void registerExternalPureBuiltinFunction(const std::string &funcName, const PureBuiltinFunction &func);
    void registerExternalBuiltinFunction(const std::string &funcName, const BuiltinFunc &func);

    ast::VarID getParamVarId(const std::string &argName);
    std::shared_ptr<symbol::ParameterSymbol> getPosVarArgsParam(const std::string &argName);
    std::shared_ptr<symbol::ParameterSymbol> getKeywordVarArgsParam(const std::string &argName);
    std::shared_ptr<symbol::ParameterSymbol> getPosArgParam(const std::string &argName);
    std::shared_ptr<symbol::ParameterSymbol> getKeywordArgParam(const std::string &argName, const std::string &defaultValue);
}

#endif //RCC_RCC_BUILTIN_DEC_H
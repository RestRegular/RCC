//
// Created by RestRegular on 2025/9/16.
//

#ifndef RCC_RCC_BUILTIN_CORE_H
#define RCC_RCC_BUILTIN_CORE_H

#include <functional>
#include "../visitors/rcc_compile_visitor.h"

namespace builtin
{
    struct CallInfos
    {
        std::vector<std::string> processedArgs;
        std::vector<std::pair<std::string, std::string>> originalArgs;
        std::queue<ast::CompileVisitor::OpItem> posArgsOpItems;
        std::unordered_map<std::string, ast::CompileVisitor::OpItem> namedArgOpItems;
        utils::Pos callPos;
        std::vector<std::pair<std::string, ast::CompileVisitor::OpItem>> orderedArgOpItems;
        std::shared_ptr<ast::FunctionCallNode> callNode;
    };

    typedef std::string BuiltinFuncRetType;

    typedef std::function<BuiltinFuncRetType(
        ast::CompileVisitor &visitor,
        const CallInfos &)> BuiltinFunc;

    class PureBuiltinFunction
    {
        bool _hasReturnValue;
        std::string _name;
        std::vector<std::shared_ptr<symbol::ParameterSymbol>> _params;
        BuiltinFunc _call;
        std::shared_ptr<symbol::TypeLabelSymbol> signature;
    public:
        PureBuiltinFunction(
            const bool &hasReturnValue, const std::string &name,
            const std::vector<std::shared_ptr<symbol::ParameterSymbol>> &params,
            const BuiltinFunc &call);

        [[nodiscard]] bool hasReturnValue() const;
        [[nodiscard]] std::string getName() const;
        [[nodiscard]] std::shared_ptr<symbol::TypeLabelSymbol> getSignature() const;
        [[nodiscard]] std::vector<std::shared_ptr<symbol::ParameterSymbol>> getParams() const;
        [[nodiscard]] BuiltinFuncRetType call(ast::CompileVisitor &visitor,
                             const CallInfos &callInfos) const;
    };

    ast::CompileVisitor::VarID getParamVarId(const std::string &argName);
    std::shared_ptr<symbol::ParameterSymbol> getPosVarArgsParam(const std::string &argName);
    std::shared_ptr<symbol::ParameterSymbol> getKeywordVarArgsParam(const std::string &argName);
    std::shared_ptr<symbol::ParameterSymbol> getPosArgParam(const std::string &argName);
    std::shared_ptr<symbol::ParameterSymbol> getKeywordArgParam(const std::string &argName, const std::string &defaultValue);

}

#endif //RCC_RCC_BUILTIN_CORE_H
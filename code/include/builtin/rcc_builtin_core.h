//
// Created by RestRegular on 2025/9/16.
//

#ifndef RCC_RCC_BUILTIN_CORE_H
#define RCC_RCC_BUILTIN_CORE_H

#include "../../declarations/builtin/rcc_builtin_dec.h"
#include "../visitors/rcc_compile_visitor.h"
#include "../interfaces/rcc_builtin_interface.h"

namespace builtin
{
    struct CallInfos
    {
        std::vector<std::string> processedArgs;
        std::vector<std::pair<std::string, std::string>> originalArgs;
        std::queue<ast::OpItem> posArgOpItems;
        std::unordered_map<std::string, ast::OpItem> namedArgOpItems;
        utils::Pos callPos;
        std::vector<std::pair<std::string, ast::OpItem>> orderedArgOpItems;
        std::shared_ptr<ast::FunctionCallNode> callNode;
    };

    void freeCallInfosI(IRCCCallInfos* pCallInfos);
    IRCCCallInfos* transformCallInfos(ast::IRCCCompileInterface* compileVisitorI, const CallInfos& callInfos);
    CallInfos transformCallInfos(IRCCCallInfos* pCallInfosI);

    class PureBuiltinFunction
    {
        bool _hasReturnValue;
        std::string _name;
        std::vector<std::shared_ptr<symbol::ParameterSymbol>> _params;
        BuiltinFunc _call;
        std::shared_ptr<symbol::TypeLabelSymbol> signature;
    public:
        PureBuiltinFunction(
            const bool &hasReturnValue, std::string name,
            const std::vector<std::shared_ptr<symbol::ParameterSymbol>> &params,
            BuiltinFunc call);

        [[nodiscard]] bool hasReturnValue() const;
        [[nodiscard]] std::string getName() const;
        [[nodiscard]] std::shared_ptr<symbol::TypeLabelSymbol> getSignature() const;
        [[nodiscard]] std::vector<std::shared_ptr<symbol::ParameterSymbol>> getParams() const;
        [[nodiscard]] BuiltinFuncRetType call(ast::CompileVisitor &visitor,
                             const CallInfos &callInfos) const;
    };
}

#endif //RCC_RCC_BUILTIN_CORE_H
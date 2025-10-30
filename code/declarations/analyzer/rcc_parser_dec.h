//
// Created by RestRegular on 2025/10/22.
//

#ifndef RCC_RCC_PARSER_DEC_H
#define RCC_RCC_PARSER_DEC_H

namespace parser
{
    class Parser;
    using ExpressionNodePtr = std::shared_ptr<ast::ExpressionNode>;
    using PrefixExpressionBuilder = std::function<ExpressionNodePtr(Parser*)>;
    using InfixExpressionBuilder = std::function<ExpressionNodePtr(Parser*, const ExpressionNodePtr &)>;
    using PostfixExpressionBuilder = std::function<ExpressionNodePtr(Parser*, const ExpressionNodePtr &)>;
    void enableDebugMode(bool cond);
}

#endif //RCC_RCC_PARSER_DEC_H
//
// Created by RestRegular on 2025/6/30.
//

#include "../../../../include/analyzer/rcc_ast.h"
#include "../../../../include/analyzer/rcc_parser.h"

namespace parser {

    static std::unordered_map<TokenType, NodeType> postfixNodeTypeMap = {
        {TokenType::TOKEN_DOUBLE_PLUS, NodeType::INCREMENT},
        {TokenType::TOKEN_DOUBLE_MINUS, NodeType::DECREMENT},
    };

    ExpressionNodePtr Parser::buildPostfixExpression(const ExpressionNodePtr &left) {
        const auto &it = postfixNodeTypeMap.find(currentToken().getType());
        if (it == postfixNodeTypeMap.end()) {
            recordUnexpectedTokenTypeError(currentToken(),
                getTokenTypeName(TokenType::TOKEN_DOUBLE_PLUS)
                + " or " + getTokenTypeName(TokenType::TOKEN_DOUBLE_MINUS));
            return nullptr;
        }
        return std::make_shared<PostfixExpressionNode>(currentToken(), it->second, left);
    }

    ExpressionNodePtr Parser::buildCallExpression(const ExpressionNodePtr &left){
        if (!currentTokenIs(TokenType::TOKEN_LPAREN)) {
            recordUnexpectedTokenTypeError(currentToken(), TokenType::TOKEN_LPAREN);
            return left;
        }
        const auto &mainToken = left->getMainToken();
        const auto &expression = buildRangerExpression();
        return std::make_shared<FunctionCallNode>(mainToken, left, expression);
    }

    ExpressionNodePtr Parser::buildIndexExpression(const ExpressionNodePtr &left) {
        auto bracketNode = buildBracketExpression();
        return std::make_shared<IndexExpressionNode>(left->getMainToken(), left, bracketNode);
    }
}

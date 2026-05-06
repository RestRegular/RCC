//
// Created by RestRegular on 2025/6/30.
//

#include "../../../../include/analyzer/rcc_parser.h"

namespace parser {

    static std::unordered_map<core::TokenType, NodeType> infixExpressionNodeTypeMap = {
        {TokenType::TOKEN_PLUS, NodeType::PLUS},
        {TokenType::TOKEN_MINUS, NodeType::MINUS},
        {TokenType::TOKEN_STAR, NodeType::MULTIPLY},
        {TokenType::TOKEN_SLASH, NodeType::DIVIDE},
        {TokenType::TOKEN_MODULO, NodeType::MODULO},
        {TokenType::TOKEN_DOUBLE_STAR, NodeType::POWER},
        {TokenType::TOKEN_ASSIGN, NodeType::ASSIGNMENT},
        {TokenType::TOKEN_COMMA, NodeType::PARALLEL},
        {TokenType::TOKEN_GREATER, NodeType::COMPARE},
        {TokenType::TOKEN_GREATER_EQUAL, NodeType::COMPARE},
        {TokenType::TOKEN_LESS, NodeType::COMPARE},
        {TokenType::TOKEN_LESS_EQUAL, NodeType::COMPARE},
        {TokenType::TOKEN_EQUAL, NodeType::COMPARE},
        {TokenType::TOKEN_NOT_EQUAL, NodeType::COMPARE},
        {TokenType::TOKEN_AND, NodeType::LOGICAL},
        {TokenType::TOKEN_OR, NodeType::LOGICAL},
        {TokenType::TOKEN_PLUS_ASSIGN, NodeType::ARGUMENT_ASSIGNMENT},
        {TokenType::TOKEN_MINUS_ASSIGN, NodeType::ARGUMENT_ASSIGNMENT},
        {TokenType::TOKEN_STAR_ASSIGN, NodeType::ARGUMENT_ASSIGNMENT},
        {TokenType::TOKEN_SLASH_ASSIGN, NodeType::ARGUMENT_ASSIGNMENT},
        {TokenType::TOKEN_MODULO_ASSIGN, NodeType::ARGUMENT_ASSIGNMENT},
        {TokenType::TOKEN_COLON, NodeType::PAIR},
        {TokenType::TOKEN_INDICATOR, NodeType::ANON_FUNCTION_DEFINITION},
        {TokenType::TOKEN_DOT, NodeType::ATTRIBUTE_EXPRESSION}
    };

    static std::unordered_set negligibleNewLineNodeTypeMap = {
        NodeType::PARALLEL, NodeType::PAIR
    };

    ExpressionNodePtr Parser::buildInfixExpression(const ExpressionNodePtr &left) {
        auto opToken = currentToken();
        const auto &it = infixExpressionNodeTypeMap.find(opToken.getType());
        if (it == infixExpressionNodeTypeMap.end()) {
            recordSyntaxError(opToken, opToken, "Invalid infix operator");
            return nullptr;
        }
        const auto &precedence = currentTokenPrecedence();
        next();
        if (negligibleNewLineNodeTypeMap.contains(it->second)) {
            while (currentTokenIs(TokenType::TOKEN_NEWLINE)) next();
        }
        ExpressionNodePtr right = nullptr;
        if (it->second == NodeType::ANON_FUNCTION_DEFINITION) {
            if (currentTokenIs(TokenType::TOKEN_LBRACE))
            {
                right = buildBraceExpression();
            } else if (currentTokenIs(TokenType::TOKEN_RETURN))
            {
                right = buildReturnExpression();
                skipNextNewLineToken();
            } else
            {
                const auto &expression = buildExpression(Precedence::LOWEST);
                right = std::make_shared<ReturnExpressionNode>(expression->getMainToken(), expression);
                skipNextNewLineToken();
            }
            return std::make_shared<AnonFunctionDefinitionNode>(opToken, left, nullptr, std::vector<std::shared_ptr<LabelNode>>{}, opToken, right);
        }
        right = buildExpression(precedence);
        return std::make_shared<InfixExpressionNode>(opToken, it->second, opToken, left, right);
    }

    ExpressionNodePtr Parser::buildAssignExpression(const ExpressionNodePtr &left){
        const auto &opToken = currentToken();
        const auto &precedence = currentTokenPrecedence();
        next();
        const auto &right = buildExpression(precedence);
        return std::make_shared<AssignmentNode>(opToken, std::pair{left, right});
    }

    ExpressionNodePtr Parser::buildCompoundExpression(const ExpressionNodePtr &left){
        return nullptr;
    }

    ExpressionNodePtr Parser::buildTernaryExpression(const ExpressionNodePtr &left){
        return nullptr;
    }

    ExpressionNodePtr Parser::buildAttributeExpression(const ExpressionNodePtr &left){
        return nullptr;
    }
}

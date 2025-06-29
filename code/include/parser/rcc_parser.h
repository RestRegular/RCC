//
// Created by RestRegular on 2025/6/29.
//

#ifndef RCC_RCC_PARSER_H
#define RCC_RCC_PARSER_H

#include <memory>
#include "../rcc_base.h"
#include "./rcc_ast.h"
#include "./rcc_ast_components.h"

namespace parser {
    typedef ast::AstNode AstNode;
    typedef core::Token Token;
    typedef core::TokenType TokenType;
    typedef base::RCCParserError RCCParserError;
    typedef base::RCCSyntaxError RCCSyntaxError;

    class Parser {
        std::vector<Token> tokens;
        size_t currentIndex = 0;
    public:
        explicit Parser(std::vector<Token> tokens);
        void next();
        void last();
        void reset();
        [[nodiscard]] const Token &lastToken() const;
        [[nodiscard]] Token &currentToken();
        [[nodiscard]] const Token &nextToken() const;
        [[nodiscard]] bool hasNext() const;
        [[nodiscard]] bool hasLast() const;
        void consumeCurrentToken();
        void consumeLastToken();
        void consumeNextToken();
        std::unique_ptr<ast::ProgramNode> parse();
        std::unique_ptr<ast::StringLiteralNode> parseStringLiteral();
        std::unique_ptr<ast::IntegerLiteralNode> parseIntegerLiteral();
        std::unique_ptr<ast::FloatLiteralNode> parseFloatLiteral();
        std::unique_ptr<ast::BooleanLiteralNode> parseBooleanLiteral();
        std::unique_ptr<ast::CharacterLiteralNode> parseCharacterLiteral();
        std::unique_ptr<ast::LiteralNode> parseLiteral();
        std::unique_ptr<ast::VariableDefinitionNode> parseVariableDefinition();
        std::unique_ptr<ast::VariableAssignmentNode> parseVariableAssignment();
        std::unique_ptr<ast::ParameterNode> parseParameter();
        std::unique_ptr<ast::FunctionDeclarationNode> parseFunctionDeclaration();
        std::unique_ptr<ast::FunctionDefinitionNode> parseFunctionDefinition();
        std::unique_ptr<ast::ArgumentNode> parseArgument();
        std::unique_ptr<ast::FunctionCallNode> parseFunctionCall();
        std::unique_ptr<ast::ClassDeclarationNode> parseClassDeclaration();
        std::unique_ptr<ast::ConstructorDefinitionNode> parseConstructorDefinition();
        std::unique_ptr<ast::ClassDefinitionNode> parseClassDefinition();
    };

} // parser

#endif //RCC_RCC_PARSER_H

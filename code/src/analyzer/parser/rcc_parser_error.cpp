//
// Created by RestRegular on 2025/6/30.
//

#include "../../../include/analyzer/rcc_parser.h"

namespace parser {

    void Parser::recordUnexpectedTokenTypeError(const Token& token, core::TokenType expectedType) {
        appendErrorMsg(RCCParserError::unexpectedTokenTypeError(
                token.getPosString(), RCC_UNKNOWN_CONST,
                token.briefString(),
                getTokenTypeName(expectedType)));
    }

    void Parser::recordUnexpectedTokenTypeError(const Token &token, const std::string &expectedType) {
        appendErrorMsg(RCCParserError::unexpectedTokenTypeError(
            token.getPosString(), RCC_UNKNOWN_CONST,
            token.briefString(),
            expectedType
        ));
    }


    void Parser::recordPrefixBuilderNotFoundError(const Token& token, core::TokenType errorType) {
        appendErrorMsg(RCCParserError::expressionBuilderNotFoundError(
                token.getPosString(), "prefix",
                getTokenTypeName(errorType)));
    }

    void Parser::recordInfixBuilderNotFoundError(const Token& token, core::TokenType errorType) {
        appendErrorMsg(RCCParserError::expressionBuilderNotFoundError(
                token.getPosString(), "infix",
                getTokenTypeName(errorType)));
    }

    void Parser::recordSyntaxError(const Token &token, const Token &errorToken, const std::string &errorMsg) {
        appendErrorMsg(RCCParserError::syntaxError(token.getPosString(), errorMsg,
            "Error occurred at " + errorToken.getPosString()));
    }

    void Parser::recordUnclosedExpressionError(const Token &beginToken, const Token &endToken, core::TokenType expectedType) {
        appendErrorMsg(RCCParserError::unclosedExpressionError(
                beginToken.getPosString(),
                beginToken.briefString(),
                endToken.briefString(),
                getTokenTypeName(expectedType)));
    }

}
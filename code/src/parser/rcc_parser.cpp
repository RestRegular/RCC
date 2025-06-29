//
// Created by RestRegular on 2025/6/29.
//

#include "../../include/rcc_base.h"
#include "../../include/parser/rcc_parser.h"

namespace parser {
    Parser::Parser(std::vector<Token> tokens)
    : tokens(std::move(tokens)) {}

    const Token &Parser::lastToken() const {
        if (!hasLast()) throw std::out_of_range("No last token");
        return tokens[currentIndex - 1];
    }

    Token &Parser::currentToken() {
        return tokens[currentIndex];
    }

    const Token & Parser::nextToken() const {
        if (!hasNext()) throw std::out_of_range("No next token");
        return tokens[currentIndex + 1];
    }

    void Parser::next() {
        currentIndex++;
    }

    void Parser::last() {
        currentIndex--;
    }

    void Parser::reset() {
        currentIndex = 0;
    }

    bool Parser::hasNext() const {
        return currentIndex < tokens.size();
    }

    bool Parser::hasLast() const {
        return currentIndex > 0;
    }


    void Parser::consumeCurrentToken() {
        tokens.erase(tokens.begin() + static_cast<int>(currentIndex));
    }

    void Parser::consumeNextToken() {
        if (!hasNext()) return;
        tokens.erase(tokens.begin() + static_cast<int>(currentIndex + 1));
    }

    void Parser::consumeLastToken() {
        if (!hasLast()) return;
        tokens.erase(tokens.begin() + static_cast<int>(currentIndex - 1));
    }

    std::unique_ptr<ast::ProgramNode> Parser::parse() {
        return std::unique_ptr<ast::ProgramNode>();
    }

    std::unique_ptr<ast::StringLiteralNode> Parser::parseStringLiteral() {
        if (currentToken().getType() == TokenType::RCC_STRING) {
            return std::make_unique<ast::StringLiteralNode>(currentToken());
        }
        throw RCCParserError::unexpectedTokenError(currentToken().getPosString(), RCC_UNKNOWN_CONST, currentToken().professionalString());
    }

    std::unique_ptr<ast::IntegerLiteralNode> Parser::parseIntegerLiteral() {
        if (currentToken().getType() == TokenType::RCC_NUMBER) {
            if (!hasNext() || nextToken().getType() != TokenType::RCC_OPERATOR || nextToken().getValue() != ".") {
                if (hasLast() && lastToken().getType() == TokenType::RCC_OPERATOR && lastToken().getValue() == "-") {
                    currentToken().setPos(lastToken().getPos());
                    currentToken().setValue("-" + currentToken().getValue());
                    consumeLastToken();
                    last();
                }
                return std::make_unique<ast::IntegerLiteralNode>(currentToken());
            }
        }
        throw RCCParserError::unexpectedTokenError(currentToken().getPosString(), RCC_UNKNOWN_CONST, currentToken().professionalString());
    }

    std::unique_ptr<ast::FloatLiteralNode> Parser::parseFloatLiteral() {
        if (currentToken().getType() == TokenType::RCC_NUMBER) {
            std::string floatValue = currentToken().getValue();

            // �����һ������Ƿ�ΪС����
            if (nextToken().getType() == TokenType::RCC_OPERATOR && nextToken().getValue() == ".") {
                // ���浱ǰλ�ã����ڴ�����
                auto pos = currentToken().getPos();

                // ������������
                consumeCurrentToken();
                // ����С����
                consumeCurrentToken();

                // ���С������Ƿ�������
                if (currentToken().getType() == TokenType::RCC_NUMBER) {
                    floatValue += "." + currentToken().getValue();

                    // �����������ǰһ�������'-'��
                    if (lastToken().getType() == TokenType::RCC_OPERATOR &&
                        lastToken().getValue() == "-") {
                        floatValue = "-" + floatValue;
                        pos = lastToken().getPos();
                        // ���ĸ���
                        last();
                        consumeCurrentToken();
                    }
                    currentToken().setPos(utils::Pos(
                            pos.getLine(), pos.getColumn(),
                            floatValue.size(), pos.getFilepath()));
                    currentToken().setValue(floatValue);
                    return std::make_unique<ast::FloatLiteralNode>(currentToken());
                }

                // ���С�����û�����֣��׳�����
                throw RCCParserError::unexpectedTokenError(pos.toString(), RCC_UNKNOWN_CONST, "Expected digit after '.'");
            }
        }

        throw RCCParserError::unexpectedTokenError(currentToken().getPosString(), RCC_UNKNOWN_CONST, currentToken().professionalString());
    }

    std::unique_ptr<ast::BooleanLiteralNode> Parser::parseBooleanLiteral() {
        if (currentToken().getType() == TokenType::RCC_BOOLEAN) {
            return std::make_unique<ast::BooleanLiteralNode>(currentToken());
        }
        throw RCCParserError::unexpectedTokenError(currentToken().getPosString(), RCC_UNKNOWN_CONST, currentToken().professionalString());
    }

    std::unique_ptr<ast::CharacterLiteralNode> Parser::parseCharacterLiteral() {
        if (currentToken().getType() == TokenType::RCC_CHAR) {
            return std::make_unique<ast::CharacterLiteralNode>(currentToken());
        }
        throw RCCParserError::unexpectedTokenError(currentToken().getPosString(), RCC_UNKNOWN_CONST, currentToken().professionalString());
    }

    std::unique_ptr<ast::LiteralNode> Parser::parseLiteral() {
        const auto &currentType = currentToken().getType();
        switch (currentType) {
            case TokenType::RCC_STRING:
                return parseStringLiteral();
            case TokenType::RCC_NUMBER: {
                try {
                    return parseIntegerLiteral();
                } catch (const RCCParserError &_) {
                    return parseFloatLiteral();
                }
            }
            case TokenType::RCC_BOOLEAN:
                return parseBooleanLiteral();
            case TokenType::RCC_CHAR:
                return parseCharacterLiteral();
            default:
                throw RCCParserError::unexpectedTokenError(currentToken().getPosString(), RCC_UNKNOWN_CONST, currentToken().professionalString());
        }
    }

    std::unique_ptr<ast::VariableDefinitionNode> Parser::parseVariableDefinition() {
        if (currentToken().getType() == TokenType::RCC_KEYWORD
            && currentToken().getValue() == "var") {
            const auto &varToken = currentToken();
            // ���� var �ؼ���
            consumeCurrentToken();
            // ToDo: ��ʱ��֧��һ�� var ����һ������
            if (currentToken().getType() == TokenType::RCC_IDENTIFIER) {
                const auto &nameToken = currentToken();
                consumeCurrentToken();
                if (currentToken().getType() == TokenType::RCC_OPERATOR &&
                    currentToken().getValue() == "=") {
                    const auto &equalSignToken = currentToken();
                    consumeCurrentToken();
                    // ToDo: ����������ʼֵ
                } else {
                    throw RCCSyntaxError::illegalOperatorError(currentToken().getPosString(), RCC_UNKNOWN_CONST, currentToken().getValue(), "=");
                }
            } else {
                throw RCCSyntaxError::invalidIdentifierError(currentToken().getPosString(), RCC_UNKNOWN_CONST, currentToken().professionalString());
            }
        }
        throw RCCParserError::unexpectedTokenError(currentToken().getPosString(), RCC_UNKNOWN_CONST, currentToken().professionalString());
    }

    std::unique_ptr<ast::VariableAssignmentNode> Parser::parseVariableAssignment() {
        // ToDo: ��ʱ��֧�ֵ�������ֵ����
        if (currentToken().getType() == TokenType::RCC_IDENTIFIER) {
            const auto nameToken = currentToken();
            // ���ı�ʶ��
            consumeCurrentToken();
            if (currentToken().getType() == TokenType::RCC_OPERATOR &&
                currentToken().getValue() == "=") {
                const auto &equalSignToken = currentToken();
                // ���ĵȺ�
                consumeCurrentToken();
            }
        }
        throw RCCParserError::unexpectedTokenError(currentToken().getPosString(), RCC_UNKNOWN_CONST, currentToken().professionalString());
    }


} // parser
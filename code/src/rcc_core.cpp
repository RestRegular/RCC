//
// Created by RestRegular on 2025/6/28.
//

#include <utility>
#include "../include/rcc_base.h"
#include "../include/rcc_core.h"

namespace core {

    Token::Token(utils::Pos pos, std::string content)
            : pos(std::move(pos)), value(std::move(content)), type(getType()) {}

    const utils::Pos &Token::getPos() const {
        return pos;
    }

    const std::string &Token::getValue() const {
        return value;
    }

    TokenType Token::getType() const {
        if (value.empty() || value == RCC_UNDEFINED_CONST) {
            return TokenType::RCC_UNDEFINED;
        } else if (value == RCC_NULL_CONST) {
            return TokenType::RCC_NULL;
        } else if (utils::StringManager::isStringFormat(value)) {
            return TokenType::RCC_STRING;
        } else if (utils::StringManager::isCharFormat(value)) {
            return TokenType::RCC_CHAR;
        } else if (base::KEYWORDS.contains(value)) {
            return TokenType::RCC_KEYWORD;
        } else if (base::DESCRIBE_LABELS.contains(value)) {
            return TokenType::RCC_LABEL;
        } else if (base::DELIMITERS.contains(value)) {
            return TokenType::RCC_DELIMITER;
        } else if (base::RANGERS.contains(value)) {
            return TokenType::RCC_RANGER;
        } else if (base::OPERATORS.contains(value)) {
            return TokenType::RCC_OPERATOR;
        } else if (utils::isNumber(value)) {
            return TokenType::RCC_NUMBER;
        } else if (value == "true" || value == "false") {
            return TokenType::RCC_BOOLEAN;
        } else if (utils::isValidIdentifier(value)) {
            return TokenType::RCC_IDENTIFIER;
        }
        return TokenType::RCC_UNDEFINED;
    }

    std::string Token::toString() const {
        return "[Token(" + getTokenTypeName(type) + "): \"" + utils::StringManager::escape(value) + "\" at " +
               pos.toString() + "]";
    }

    std::string Token::briefString() const {
        return "[<" + getTokenTypeName(type) + "> \"" + utils::StringManager::escape(value) + "\" (" +
               pos.briefString() + ")]";
    }

    std::string Token::professionalString() const {
        return "Token{type=" + getTokenTypeName(type) + ", value=\"" + utils::StringManager::escape(value) +
               "\", pos=" + pos.professionalString() + "}";
    }

    void Token::setValue(std::string value_) {
        this->value = std::move(value_);
    }

    std::string Token::getPosString() const {
        return pos.toString();
    }

    void Token::setPos(utils::Pos pos_) {
        this->pos = std::move(pos_);
    }

    void Token::setType(TokenType type_) {
        this->type = type_;
    }


    std::string getTokenTypeName(const TokenType &type) {
        switch (type) {
            case TokenType::RCC_UNKNOWN:
                return "UNKNOWN";
            case TokenType::RCC_UNDEFINED:
                return "UNDEFINED";
            case TokenType::RCC_IDENTIFIER:
                return "IDENTIFIER";
            case TokenType::RCC_KEYWORD:
                return "KEYWORD";
            case TokenType::RCC_OPERATOR:
                return "OPERATOR";
            case TokenType::RCC_NUMBER:
                return "NUMBER";
            case TokenType::RCC_NUM_STR:
                return "NUM_STR";
            case TokenType::RCC_STRING:
                return "STRING";
            case TokenType::RCC_CHAR:
                return "CHAR";
            case TokenType::RCC_BOOLEAN:
                return "BOOLEAN";
            case TokenType::RCC_NULL:
                return "NULL";
            case TokenType::RCC_DELIMITER:
                return "DELIMITER";
            case TokenType::RCC_RANGER:
                return "RANGER";
            case TokenType::RCC_LABEL:
                return "LABEL";
            case TokenType::RCC_CALL:
                return "CALL";
            case TokenType::RCC_ARG:
                return "ARGUMENT";
            case TokenType::RCC_ATTR:
                return "ATTRIBUTE";
            case TokenType::RCC_EXPR:
                return "EXPRESSION";
            case TokenType::RCC_SUBSCRIPT:
                return "SUBSCRIPT";
            case TokenType::RCC_BLOCK:
                return "BLOCK";
            case TokenType::RCC_ANNO_FUNC:
                return "ANNO_FUNC";
            case TokenType::RCC_PAIR:
                return "PAIR";
            default:
                return "UNDEFINED";
        }

    }
}
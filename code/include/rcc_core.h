//
// Created by RestRegular on 2025/6/28.
//

#ifndef TOKEN_RCC_CORE_H
#define TOKEN_RCC_CORE_H

#include "./lib/rcc_utils.h"
#include "lib/RJson/RJson.h"
#include "../declarations/rcc_core_dec.h"

namespace core {

    // 类定义
    class Token final : utils::Object {
        utils::Pos pos;
        std::string value;
        TokenType type;
        [[nodiscard]] TokenType parseType();
    public:
        Token();
        explicit Token(const utils::Pos &pos, std::string content);
        explicit Token(utils::Pos pos);
        [[nodiscard]] utils::Pos getPos() const;
        [[nodiscard]] std::string getPosString() const;
        [[nodiscard]] const std::string &getValue() const;
        void setValue(std::string value_);
        void setPos(utils::Pos pos_);
        void setType(TokenType type_);
        [[nodiscard]] TokenType getType() const;
        [[nodiscard]] std::string toString() const override;
        [[nodiscard]] std::string briefString() const override;
        [[nodiscard]] std::string professionalString() const override;
        [[nodiscard]] std::string formatString(size_t indent, size_t level) const override;
        void acceptRJsonBuilder(rjson::rj::RJsonBuilder& builder) const;
        [[nodiscard]] rjson::RJValue toRJson() const;
        [[nodiscard]] rjson::RJPair toRJPair() const;
        [[nodiscard]] rjson::RJValue toRJValue() const;
    };

    // 枚举类定义
    enum class Precedence  {
        LOWEST = 0,
        PARALLEL, // ,
        KV_SEP, // :
        ASSIGN, // =, +=, -=, *=, /=, %=, &=, |=, !=, ^=
        LOGIC, // &, |, ^, &&, ||
        TERNARY, // ... ? ... : ...
        COMPARE, // <, <=, >, >=, ==, !=
        SUM, // +, -
        PRODUCT, // *, /, %
        PREFIX, // -, !, ~
        CALL, // function(x, y)
        POSTFIX, // ++, --
        INDEX, // list[index]
        ATTRIBUTE, // object.attr or object.func()
        INDICATOR, // ->
    };
    enum class CommentType {
        NONE,
        SINGLE_LINE_COMMENT,
        DOC_COMMENT
    };
    enum class TokenType {
        TOKEN_AND,
        TOKEN_ASSIGN,
        TOKEN_AT,
        TOKEN_BIT_AND,
        TOKEN_BIT_AND_ASSIGN,
        TOKEN_BIT_LEFT_SHIFT,
        TOKEN_BIT_OR,
        TOKEN_BIT_OR_ASSIGN,
        TOKEN_BIT_RIGHT_SHIFT,
        TOKEN_BIT_XOR,
        TOKEN_BIT_XOR_ASSIGN,
        TOKEN_BOOL,
        TOKEN_BREAK,
        TOKEN_CHAR,
        TOKEN_CLASS,
        TOKEN_COLON,
        TOKEN_COMMA,
        TOKEN_CONTINUE,
        TOKEN_CTOR,
        TOKEN_DELIMITER,
        TOKEN_DOUBLE_COLON,
        TOKEN_DOUBLE_MINUS,
        TOKEN_DOUBLE_PLUS,
        TOKEN_DOUBLE_STAR,
        TOKEN_DOT,
        TOKEN_ELIF,
        TOKEN_ELSE,
        TOKEN_ENCAPSULATED,
        TOKEN_EQUAL,
        TOKEN_FALSE,
        TOKEN_FLOAT,
        TOKEN_FOR,
        TOKEN_FUNCTION,
        TOKEN_GREATER,
        TOKEN_GREATER_EQUAL,
        TOKEN_IDENTIFIER,
        TOKEN_IF,
        TOKEN_INDICATOR,
        TOKEN_INTEGER,
        TOKEN_KEYWORD,
        TOKEN_LABEL,
        TOKEN_LBRACE,
        TOKEN_LBRACKET,
        TOKEN_LINK,
        TOKEN_LPAREN,
        TOKEN_LESS,
        TOKEN_LESS_EQUAL,
        TOKEN_MINUS,
        TOKEN_MINUS_ASSIGN,
        TOKEN_MODULO,
        TOKEN_MODULO_ASSIGN,
        TOKEN_NEWLINE,
        TOKEN_NOT,
        TOKEN_NOT_EQUAL,
        TOKEN_NULL,
        TOKEN_OPERATOR,
        TOKEN_OR,
        TOKEN_PASS,
        TOKEN_PLUS,
        TOKEN_PLUS_ASSIGN,
        TOKEN_PROGRAM,
        TOKEN_QUESTION,
        TOKEN_RANGER,
        TOKEN_RPAREN,
        TOKEN_RBRACKET,
        TOKEN_RBRACE,
        TOKEN_UNTIL,
        TOKEN_RETURN,
        TOKEN_SLASH,
        TOKEN_SLASH_ASSIGN,
        TOKEN_STREAM_END,
        TOKEN_STREAM_START,
        TOKEN_STRING,
        TOKEN_STAR,
        TOKEN_STAR_ASSIGN,
        TOKEN_TRY,
        TOKEN_TRUE,
        TOKEN_TILDE,
        TOKEN_UNDEFINED,
        TOKEN_UNKNOWN,
        TOKEN_VAR,
        TOKEN_WHILE,
        TOKEN_SKIP_NEWLINE,
        TOKEN_CATCH,
        TOKEN_FINALLY,
        TOKEN_THROW,
    };
    enum class StatementType {
        VAR_DEF, // Complete
        ASSIGNMENT, // Complete
        FUNC_DEF, // Complete
        ANNO_FUNC_DEF, // Complete
        CONDITIONAL, // Complete
        RETURN, // Complete
        BREAK, // Complete
        FUNC_CALL, // Complete
        EXPRESSION, // Complete
        PASS, // Complete
        WHILE, // Complete
        UNTIL, // Complete
        FOR, // Complete
        ENCAPSULATED, // Complete
        CTOR_DEF, // Complete
        CLASS_DEC, // Complete
        CLASS_DEF, // Complete
        DICTIONARY, // Complete
        LIST, // Complete
        TUPLE, // Complete
        SUBSCRIPT, // Complete
        TRY, // Todo
        CATCH, // Todo
        THROW, // Todo
        FINALLY, // Todo
        RASM, // ToDo
    };
    enum class ExpressionType {
        PARENS_EXPRESSION, // generic
        CALCULATE_EXPRESSION,
        ARGUMENT_EXPRESSION,
        PARAMETER_EXPRESSION,
        SUBSCRIPT_EXPRESSION, // generic
        LIST_EXPRESSION,
        SLICE_EXPRESSION
    };
    enum class BlockType {
        EMPTY_BLOCK,
        CODE_BLOCK,
        DICT_BLOCK,
        FUNC_BLOCK,
        COND_IF_BLOCK,
        COND_ELSE_BLOCK,
        FOR_BLOCK,
        WHILE_BLOCK,
        TRY_BLOCK,
        CATCH_BLOCK,
        FINALLY_BLOCK,
        CLASS_BLOCK
    };
    enum class SubscriptType {
        RANGE,
        INDEX
    };
    enum class CallExpressionType {
        CALL_GENERIC,
        CALL_FUNC,
        DEF_FUNC
    };
    // enum class FunctionType {
    //     STD_FUNC,
    //     ANNO_FUNC,
    //     CTOR_FUNC
    // };
    enum class CondBranchType
    {
        IF_BRANCH,
        ELSE_BRANCH,
        ELIF_BRANCH
    };
    enum class LoopType
    {
        WHILE,
        FOR,
        UNTIL
    };

}


#endif //TOKEN_RCC_CORE_H

//
// Created by RestRegular on 2025/6/28.
//

#ifndef RCC_RCC_CORE_H
#define RCC_RCC_CORE_H

#include <iostream>
#include "./lib/rcc_utils.h"

namespace core {
    // 枚举类声明
    enum class CommentType;
    enum class TokenType;
    enum class StatementType;
    enum class ExpressionType;
    enum class BlockType;
    enum class ArgumentType;
    enum class SubscriptType;
    enum class CallExpressionType;
    enum class FunctionType;
    enum class CondBranchType;

    // 类声明
    class Token;
    class Statement;

    // 函数声明
    std::string getTokenTypeName(const TokenType& type);

    // 类定义
    class Token: utils::Object {
        utils::Pos pos;
        std::string value;
        TokenType type;

    public:
        explicit Token(utils::Pos pos, std::string content);

        [[nodiscard]] const utils::Pos &getPos() const;

        [[nodiscard]] std::string getPosString() const;

        [[nodiscard]] const std::string &getValue() const;

        void setValue(std::string value_);

        void setPos(utils::Pos pos_);

        void setType(TokenType type_);

        [[nodiscard]] TokenType getType() const;

        [[nodiscard]] std::string toString() const override;

        [[nodiscard]] std::string briefString() const override;

        [[nodiscard]] std::string professionalString() const override;
    };

    // 枚举类定义
    enum class CommentType {
        NONE,
        RCC_COMMENT,
        RCC_DOC_COMMENT
    };
    enum class TokenType {
        RCC_UNKNOWN,
        RCC_UNDEFINED,
        RCC_IDENTIFIER,
        RCC_KEYWORD,
        RCC_OPERATOR,
        RCC_NUMBER,
        RCC_NUM_STR,
        RCC_STRING,
        RCC_CHAR,
        RCC_BOOLEAN,
        RCC_NULL,
        RCC_DELIMITER,
        RCC_RANGER,
        RCC_LABEL,
        RCC_CALL,
        RCC_ARG,
        RCC_ATTR,
        RCC_EXPR,
        RCC_SUBSCRIPT,
        RCC_BLOCK,
        RCC_ANNO_FUNC,
        RCC_PAIR
    };
    enum class StatementType {
        VAR_DEF,
        ASSIGNMENT,
        FUNC_DEF,
        CONDITIONAL,
        RETURN,
        BREAK,
        FUNC_CALL,
        EXPRESSION,
        PASS,
        WHILE,
        UNTIL,
        FOR,
        REPEAT,
        ITERATE,
        ENCAPSULATED,
        CLASS_DEF,
        TRY,
        CATCH,
        THROW,
        FINALLY,
        RASM,
        IMPORT
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
    enum class ArgumentType {
        POSITIONAL,
        KEYWORD,
        DEFAULT,
        POSITIONAL_VARARGS,
        KEYWORD_VARARGS
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
    enum class FunctionType {
        STD_FUNC,
        ANNO_FUNC,
        CTOR_FUNC
    };
    enum class CondBranchType {
        IF_BRANCH,
        ELSE_BRANCH,
        ELIF_BRANCH
    };

}


#endif //RCC_RCC_CORE_H

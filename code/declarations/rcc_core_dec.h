//
// Created by RestRegular on 2025/10/22.
//

#ifndef RCC_RCC_CORE_DEC_H
#define RCC_RCC_CORE_DEC_H
#include <string>

namespace core
{
    // 枚举类声明
    enum class Precedence;
    enum class CommentType;
    enum class TokenType;
    enum class StatementType;
    enum class ExpressionType;
    enum class BlockType;
    enum class SubscriptType;
    enum class CallExpressionType;
    // enum class FunctionType;
    enum class CondBranchType;
    enum class LoopType;

    // 类声明
    class Token;
    class Statement;

    // 函数声明
    std::string getTokenTypeName(const TokenType& type);
}

#endif //RCC_RCC_CORE_DEC_H
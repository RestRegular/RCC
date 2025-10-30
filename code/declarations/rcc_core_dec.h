//
// Created by RestRegular on 2025/10/22.
//

#ifndef RCC_RCC_CORE_DEC_H
#define RCC_RCC_CORE_DEC_H
#include <string>

namespace core
{
    // ö��������
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

    // ������
    class Token;
    class Statement;

    // ��������
    std::string getTokenTypeName(const TokenType& type);
}

#endif //RCC_RCC_CORE_DEC_H
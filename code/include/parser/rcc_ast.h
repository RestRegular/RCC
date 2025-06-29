//
// Created by RestRegular on 2025/6/29.
//

#ifndef RCC_RCC_AST_H
#define RCC_RCC_AST_H

#include <memory>
#include "../rcc_core.h"
#include "../lib/rcc_utils.h"

namespace ast {
    // 枚举类声明
    enum class AstNodeType {
        AND, // 新增逻辑与节点
        ASSIGNMENT,
        ATTRIBUTE_ASSIGNMENT,
        ATTRIBUTE_EXPRESSION,
        BLOCK,
        BOOL,
        BREAK,
        CALL,
        CLASS,
        COMPOUND,
        CONTINUE,
        DECORATOR,
        DICT,
        DIVIDE, // 除法
        EQUALITY, // 新增相等比较节点
        EXPRESSION_STATEMENT,
        FLOAT,
        FLOOR_DIVIDE, // 整除
        FOR,
        FUNCTION,
        GREATER, // 新增大于比较节点
        GREATER_EQUAL, // 新增大于等于比较节点
        IDENTIFIER,
        IF,
        IMPORT,
        INDEX_ASSIGNMENT,
        INDEX_EXPRESSION,
        INTEGER,
        LESS, // 新增小于比较节点
        LESS_EQUAL, // 新增小于等于比较节点
        LIST,
        METHOD,
        MINUS, // 减法
        MODULE, // 新增模块节点
        MODULO, // 取余
        MULTIPLY, // 乘法
        NOT, // 新增逻辑非节点
        NOT_EQUAL, // 新增不等比较节点
        NULL_NODE,
        OR, // 新增逻辑或节点
        PLUS, // 新增加法节点
        // POSTFIX,
        POWER, // 平方/幂运算
        // PREFIX,
        PROGRAM,
        RETURN,
        STRING,
        SUPER,
        TERNARY,
        THIS,
        WHILE
    };

    // 类声明
    // --- 工具类 ---

    // --- 访问器 ---
    class AstVisitor;
    // --- 节点类 ---
    class AstNode;
    class ExpressionNode;
    class StatementNode;
    class LiteralNode;
    class StringLiteralNode;
    class NumberLiteralNode;
    class IntegerLiteralNode;
    class FloatLiteralNode;
    class BooleanLiteralNode;
    class CharacterLiteralNode;
    class VariableDefinitionNode;
    class VariableAssignmentNode;
    class ParameterNode;
    class ArgumentNode;
    class FunctionDeclarationNode;
    class FunctionDefinitionNode;
    class ConstructorDefinitionNode;
    class ClassDeclarationNode;
    class ClassDefinitionNode;
    class FunctionCallNode;
    class ProgramNode;
    // --- 语法树类 ---
    class Ast;

    // 函数声明

    // 全局变量声明

    // 类定义

    // 枚举类定义

} // ast

#endif //RCC_RCC_AST_H

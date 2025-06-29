//
// Created by RestRegular on 2025/6/29.
//

#ifndef RCC_RCC_AST_H
#define RCC_RCC_AST_H

#include <memory>
#include "../rcc_core.h"
#include "../lib/rcc_utils.h"

namespace ast {
    // ö��������
    enum class AstNodeType {
        AND, // �����߼���ڵ�
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
        DIVIDE, // ����
        EQUALITY, // ������ȱȽϽڵ�
        EXPRESSION_STATEMENT,
        FLOAT,
        FLOOR_DIVIDE, // ����
        FOR,
        FUNCTION,
        GREATER, // �������ڱȽϽڵ�
        GREATER_EQUAL, // �������ڵ��ڱȽϽڵ�
        IDENTIFIER,
        IF,
        IMPORT,
        INDEX_ASSIGNMENT,
        INDEX_EXPRESSION,
        INTEGER,
        LESS, // ����С�ڱȽϽڵ�
        LESS_EQUAL, // ����С�ڵ��ڱȽϽڵ�
        LIST,
        METHOD,
        MINUS, // ����
        MODULE, // ����ģ��ڵ�
        MODULO, // ȡ��
        MULTIPLY, // �˷�
        NOT, // �����߼��ǽڵ�
        NOT_EQUAL, // �������ȱȽϽڵ�
        NULL_NODE,
        OR, // �����߼���ڵ�
        PLUS, // �����ӷ��ڵ�
        // POSTFIX,
        POWER, // ƽ��/������
        // PREFIX,
        PROGRAM,
        RETURN,
        STRING,
        SUPER,
        TERNARY,
        THIS,
        WHILE
    };

    // ������
    // --- ������ ---

    // --- ������ ---
    class AstVisitor;
    // --- �ڵ��� ---
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
    // --- �﷨���� ---
    class Ast;

    // ��������

    // ȫ�ֱ�������

    // �ඨ��

    // ö���ඨ��

} // ast

#endif //RCC_RCC_AST_H

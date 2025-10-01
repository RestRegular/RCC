//
// Created by RestRegular on 2025/6/29.
//

#ifndef RCC_RCC_AST_H
#define RCC_RCC_AST_H

#include "../rcc_core.h"

namespace ast {
    // ö��������
    enum class NodeType;

    // ������
    // --- ������ ---

    // --- ������ ---
    class Visitor;
    class PrintVisitor;
    class JsonVisitor;
    // --- �ڵ��� ---
    class Node;
    class ExpressionNode;
    class StatementNode;
    class ExpressionStatementNode;
    class PrefixExpressionNode;
    class LiteralNode;
    class StringLiteralNode;
    class NumberLiteralNode;
    class IntegerLiteralNode;
    class FloatLiteralNode;
    class BooleanLiteralNode;
    class CharacterLiteralNode;
    class PairExpressionNode;
    class DictionaryExpressionNode;
    class ListExpressionNode;
    class RangerNode;
    class ParenRangerNode;
    class BlockRangerNode;
    class IdentifierNode;
    class LabelNode;
    class InfixExpressionNode;
    class UnaryExpressionNode;
    class PostfixExpressionNode;
    class BracketExpressionNode;
    class IndexExpressionNode;
    class VariableDefinitionNode;
    class AssignmentNode;
    class ParameterNode;
    class ArgumentNode;
    class FunctionDeclarationNode;
    class FunctionDefinitionNode;
    class AnonFunctionDefinitionNode;
    class ConditionNode;
    class BranchNode;
    class LoopNode;
    class WhileLoopNode;
    class UntilLoopNode;
    class ForLoopNode;
    class PassExpressionNode;
    class EncapsulatedExpressionNode;
    class ConstructorDefinitionNode;
    class ClassDeclarationNode;
    class ClassDefinitionNode;
    class FunctionCallNode;
    class ReturnExpressionNode;
    class BreakExpressionNode;
    class ProgramNode;
    // --- �﷨���� ---
    class Ast;

    // ��������
    std::string getNodeTypeName(NodeType nodeType);
    std::string getNodeTypeFormatName(NodeType nodeType);

    // ȫ�ֱ�������

    // �ඨ��

    // ö���ඨ��
    enum class NodeType {
        AND, // �߼�������ڵ�
        ANON_FUNCTION_DEFINITION, // ������������ڵ�
        ASSIGNMENT, // ��ֵ�����ڵ�
        ARGUMENT_ASSIGNMENT, // ���Ը�ֵ�ڵ�
        ATTRIBUTE_EXPRESSION, // ���Է��ʱ��ʽ�ڵ�
        BLOCK, // �����ڵ�
        BOOL, // ���������������ڵ�
        BREAK, // break���ڵ�
        BRACE, // �����Žڵ�
        BRACKET, // �����Žڵ�
        BRANCH, // ��֧�ڵ�
        CALL, // �������ýڵ�
        CHAR, // �ַ������������ڵ�
        CLASS_DECLARATION, // �������ڵ�
        CLASS_DEFINITION, // �ඨ��ڵ�
        COMPOUND, // ������䣨��������֧�ֶ���������ϣ�
        COMPARE, // �Ƚ�����ڵ�
        CONDITION, // �������
        CONSTRUCTOR, // ���캯������ڵ�
        CONTINUE, // continue���ڵ�
        DECORATOR, // װ�����ڵ�
        DICTIONARY, // �ֵ�ṹ�ڵ�
        DIVIDE, // ��������ڵ�
        ENCAPSULATED, // ��װ���ʽ�ڵ�
        EQUALITY, // ���ڱȽϲ����ڵ�
        EXPRESSION_STATEMENT, // ���ʽ��Ϊ�������Ľڵ�
        FLOAT, // �������������ڵ�
        FLOOR_DIVIDE, // ����ȡ����������ڵ�
        FOR, // forѭ�����ƽṹ�ڵ�
        FUNCTION, // ��������������ڵ�
        FUNCTION_DECLARATION, // ���������ڵ�
        FUNCTION_DEFINITION, // �����嶨��ڵ�
        GREATER, // ���ڱȽϲ����ڵ�
        GREATER_EQUAL, // ���ڵ��ڱȽϲ����ڵ�
        IDENTIFIER, // ��ʶ�����ýڵ�
        IMPORT, // ģ�鵼�����ڵ�
        INCREMENT,  // ���������ڵ�
        DECREMENT,  // �Լ������ڵ�
        INDEX_ASSIGNMENT, // ������ֵ�����ڵ�
        INDEX_EXPRESSION, // �������ʱ��ʽ�ڵ�
        INFIX, // ��׺���ʽͨ�ýڵ�
        INTEGER, // �����������ڵ�
        LABEL, // ��ǩ�ڵ�
        LESS, // С�ڱȽϲ����ڵ�
        LESS_EQUAL, // С�ڵ��ڱȽϲ����ڵ�
        LIST, // �б�ṹ�ڵ�
        LOGICAL, // �߼�����ڵ�
        LOOP, // ѭ�����ƽṹ�ڵ�
        METHOD, // ��������ڵ�
        MINUS, // �����򸺺�����ڵ�
        MODULE, // ģ�鶥��ṹ�ڵ�
        MODULO, // ȡģ����ڵ�
        MULTIPLY, // �˷�����ڵ�
        NOT, // �߼��ǲ����ڵ�
        NOT_EQUAL, // �����ڱȽϲ����ڵ�
        NULL_NODE, // ��ֵ(null)�������ڵ�
        OR, // �߼�������ڵ�
        PARALLEL, // ��ʾ�ö��ŷָ��Ĳ��нڵ�
        PAIR, // ��ʾ��ð�ŷָ��ļ�ֵ�Խڵ�
        PAREN, // ��ʾ���ű��ʽ�ڵ�
        PASS, // pass���ڵ�
        PLUS, // �ӷ�����������ڵ�
        POSTFIX, // ��׺���ʽ�ڵ�
        POWER, // ������ڵ�
        PREFIX, // ǰ׺���ʽ�ڵ�
        PROGRAM, // �����﷨�����ڵ㣬��ʾ��������ṹ
        RANGER, // ��ʾ��Χ�ڵ�
        RETURN, // return�������ڵ�
        STRING, // �ַ����������ڵ�
        TERNARY, // ��Ԫ�������ʽ�ڵ�
        UNARY, // һԪ���ʽ�ڵ�
        UNDEFINED, // δ����ֵ�ڵ�
        UNKNOWN, // δ֪���ͽڵ㣨�������Ԥ����
        VAR, // ��������ڵ�
        WHILE, // whileѭ�����ƽṹ�ڵ�
    };

} // analyzer

#endif //RCC_RCC_AST_H

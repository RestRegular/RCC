//
// Created by RestRegular on 2025/10/22.
//

#ifndef RCC_RCC_AST_DEC_H
#define RCC_RCC_AST_DEC_H
#include <string>

namespace ast
{
    // ö��������
    enum class NodeType;

    // ������
    // --- ������ ---
    class RaCodeBuilder;
    class VarID;
    class SetID;
    class OpItem;

    // --- ������ ---
    class Visitor;
    class PrintVisitor;
    class JsonVisitor;
    class CompileVisitor;
    class CollectSymbolVisitor;
    // --- �ڵ��� ---
    class Node;
    class ExpressionNode;
    class StatementNode;
    class ExpressionStatementNode;
    class PrefixExpressionNode;
    class LiteralNode;
    class NullLiteralNode;
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

    // ����������ö��
    enum class OpItemType: int;

    // ����������ö��
    enum class ScopeType: int;

    // ��������ö��
    enum class BuiltinType: int;

    std::string opItemTypeToString(const OpItemType& type);
    std::string opItemTypeToFormatString(const OpItemType& type);
}

#endif //RCC_RCC_AST_DEC_H
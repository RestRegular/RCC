//
// Created by RestRegular on 2025/10/22.
//

#ifndef RCC_RCC_AST_DEC_H
#define RCC_RCC_AST_DEC_H
#include <string>

namespace ast
{
    // 枚举类声明
    enum class NodeType;

    // 类声明
    // --- 工具类 ---
    class ContentBuilder;
    class VarID;
    class SetID;
    class OpItem;

    // --- 访问器 ---
    class Visitor;
    class PrintVisitor;
    class JsonVisitor;
    class CompileVisitor;
    class CollectSymbolVisitor;
    // --- 节点类 ---
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
    class ThrowNode;
    class TryNode;
    // --- 语法树类 ---
    class Ast;

    // 函数声明
    std::string getNodeTypeName(NodeType nodeType);
    std::string getNodeTypeFormatName(NodeType nodeType);

    // 操作数类型枚举
    enum class OpItemType: int;

    // 作用域类型枚举
    enum class ScopeType: int;

    // 内置类型枚举
    enum class BuiltinType: int;

    std::string opItemTypeToString(const OpItemType& type);
    std::string opItemTypeToFormatString(const OpItemType& type);
}

#endif //RCC_RCC_AST_DEC_H
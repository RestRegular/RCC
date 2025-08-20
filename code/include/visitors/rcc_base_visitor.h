//
// Created by RestRegular on 2025/6/30.
//

#ifndef RCC_RCC_VISITOR_H
#define RCC_RCC_VISITOR_H

#include "../analyzer/rcc_ast.h"
#include "../analyzer/rcc_ast_components.h"
#include "../components/symbol/rcc_symbol.h"
#include "../lib/RJson/RJson.h"

namespace ast {
    class NullLiteralNode;

    using namespace rjson::rj;

    class Visitor;
    class PrintVisitor;
    class JsonVisitor;
    class CompileVisitor;
    class CollectSymbolVisitor;

    class Visitor : public utils::Object {
    public:
        Visitor() = default;

        virtual void visitLiteralNode(LiteralNode &node) = 0;

        virtual void visitNullLiteralNode(NullLiteralNode& node) = 0;

        virtual void visitStringLiteralNode(StringLiteralNode &node) = 0;

        virtual void visitNumberLiteralNode(NumberLiteralNode &node) = 0;

        virtual void visitIntegerLiteralNode(IntegerLiteralNode &node) = 0;

        virtual void visitFloatLiteralNode(FloatLiteralNode &node) = 0;

        virtual void visitBooleanLiteralNode(BooleanLiteralNode &node) = 0;

        virtual void visitCharacterLiteralNode(CharacterLiteralNode &node) = 0;

        virtual void visitIdentifierNode(IdentifierNode &node) = 0;

        virtual void visitVariableDefinitionNode(VariableDefinitionNode &node) = 0;

        virtual void visitAssignmentNode(AssignmentNode &node) = 0;

        virtual void visitParameterNode(ParameterNode &node) = 0;

        virtual void visitArgumentNode(ArgumentNode &node) = 0;

        virtual void visitFunctionDeclarationNode(FunctionDeclarationNode &node) = 0;

        virtual void visitConstructorDefinitionNode(ConstructorDefinitionNode &node) = 0;

        virtual void visitClassDeclarationNode(ClassDeclarationNode &node) = 0;

        virtual void visitClassDefinitionNode(ClassDefinitionNode &node) = 0;

        virtual void visitFunctionCallNode(FunctionCallNode &node) = 0;

        virtual void visitProgramNode(ProgramNode &node) = 0;

        virtual void visitInfixNode(InfixExpressionNode &node) = 0;

        virtual void visitUnaryExpressionNode(UnaryExpressionNode &node) = 0;

        virtual void visitExpressionStatementNode(ExpressionStatementNode &node) = 0;

        virtual void visitPrefixExpressionNode(PrefixExpressionNode &node) = 0;

        virtual void visitPostfixNode(PostfixExpressionNode &node) = 0;

        virtual void visitParenRangerNode(ParenRangerNode &node) = 0;

        virtual void visitBlockRangerNode(BlockRangerNode &node) = 0;

        virtual void visitFunctionDefinitionNode(FunctionDefinitionNode &node) = 0;

        virtual void visitLabelNode(LabelNode &node) = 0;

        virtual void visitBranchNode(BranchNode &node) = 0;

        virtual void visitConditionNode(ConditionNode &node) = 0;

        virtual void visitLoopNode(LoopNode &node) = 0;

        virtual void visitForLoopNode(ForLoopNode &node) = 0;

        virtual void visitPassExpressionNode(PassExpressionNode &node) = 0;

        virtual void visitEncapsulatedExpressionNode(EncapsulatedExpressionNode &node) = 0;

        virtual void visitReturnExpressionNode(ReturnExpressionNode &node) = 0;

        virtual void visitBreakExpressionNode(BreakExpressionNode &node) = 0;

        virtual void visitAnonFunctionDefinitionNode(AnonFunctionDefinitionNode &node) = 0;

        virtual void visitPairExpressionNode(PairExpressionNode &node) = 0;

        virtual void visitDictionaryExpressionNode(DictionaryExpressionNode &node) = 0;

        virtual void visitListExpressionNode(ListExpressionNode &node) = 0;

        virtual void visitBracketExpressionNode(BracketExpressionNode &node) = 0;

        virtual void visitIndexExpressionNode(IndexExpressionNode &node) = 0;
    };

}

#endif //RCC_RCC_VISITOR_H

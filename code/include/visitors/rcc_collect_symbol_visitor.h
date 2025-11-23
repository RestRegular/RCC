//
// Created by RestRegular on 2025/7/20.
//

#ifndef RCC_COLLECT_SYMBOL_VISITOR_H
#define RCC_COLLECT_SYMBOL_VISITOR_H

#include "./rcc_base_visitor.h"
#include "../components/symbol/rcc_symbol.h"

namespace ast {

    class CollectSymbolVisitor final : public Visitor {
        symbol::SymbolTableManager symbolTable;

    public:
        symbol::SymbolTableManager getSymbolTable() const;

        void visitLiteralNode(LiteralNode &node) override;

        void visitNullLiteralNode(NullLiteralNode& node) override;

        void visitStringLiteralNode(StringLiteralNode &node) override;

        void visitNumberLiteralNode(NumberLiteralNode &node) override;

        void visitIntegerLiteralNode(IntegerLiteralNode &node) override;

        void visitFloatLiteralNode(FloatLiteralNode &node) override;

        void visitBooleanLiteralNode(BooleanLiteralNode &node) override;

        void visitCharacterLiteralNode(CharacterLiteralNode &node) override;

        void visitIdentifierNode(IdentifierNode &node) override;

        void visitParameterNode(ParameterNode &node) override;

        void visitArgumentNode(ArgumentNode &node) override;

        void visitFunctionDeclarationNode(FunctionDeclarationNode &node) override;

        void visitConstructorDefinitionNode(ConstructorDefinitionNode &node) override;

        void visitClassDeclarationNode(ClassDeclarationNode &node) override;

        void visitClassDefinitionNode(ClassDefinitionNode &node) override;

        void visitFunctionCallNode(FunctionCallNode &node) override;

        void visitProgramNode(ProgramNode &node) override;

        void visitInfixNode(InfixExpressionNode &node) override;

        void visitUnaryExpressionNode(UnaryExpressionNode &node) override;

        void visitExpressionStatementNode(ExpressionStatementNode &node) override;

        void visitPrefixExpressionNode(PrefixExpressionNode &node) override;

        void visitPostfixNode(PostfixExpressionNode &node) override;

        void visitParenRangerNode(ParenRangerNode &node) override;

        void visitBlockRangerNode(BlockRangerNode &node) override;

        void visitFunctionDefinitionNode(FunctionDefinitionNode &node) override;

        void visitLabelNode(LabelNode &node) override;

        void visitBranchNode(BranchNode &node) override;

        void visitConditionNode(ConditionNode &node) override;

        void visitLoopNode(LoopNode &node) override;

        void visitForLoopNode(ForLoopNode &node) override;

        void visitPassExpressionNode(PassExpressionNode &node) override;

        void visitEncapsulatedExpressionNode(EncapsulatedExpressionNode &node) override;

        void visitReturnExpressionNode(ReturnExpressionNode &node) override;

        void visitBreakExpressionNode(BreakExpressionNode &node) override;

        void visitAnonFunctionDefinitionNode(AnonFunctionDefinitionNode &node) override;

        void visitPairExpressionNode(PairExpressionNode &node) override;

        void visitDictionaryExpressionNode(DictionaryExpressionNode &node) override;

        void visitListExpressionNode(ListExpressionNode &node) override;

        void visitBracketExpressionNode(BracketExpressionNode &node) override;

        void visitIndexExpressionNode(IndexExpressionNode &node) override;

        void visitTryNode(TryNode& try_node) override;

        void visitThrowNode(ThrowNode& node) override;

        void visitVariableDefinitionNode(VariableDefinitionNode &node) override;

        void visitAssignmentNode(AssignmentNode &node) override;
    };

}

#endif //RCC_COLLECT_SYMBOL_VISITOR_H

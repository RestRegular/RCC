//
// Created by RestRegular on 2025/6/30.
//

#ifndef RCC_RCC_VISITOR_H
#define RCC_RCC_VISITOR_H

#include "../rcc_ast.h"
#include "../../lib/RJson/RJson.h"

namespace ast {
    using namespace rjson::rj;

    class Visitor : public utils::Object {
    public:
        Visitor() = default;

        virtual void visitLiteralNode(LiteralNode &node) = 0;
        virtual void visitStringLiteralNode(StringLiteralNode &node) = 0;
        virtual void visitNumberLiteralNode(NumberLiteralNode &node) = 0;
        virtual void visitIntegerLiteralNode(IntegerLiteralNode &node) = 0;
        virtual void visitFloatLiteralNode(FloatLiteralNode &node) = 0;
        virtual void visitBooleanLiteralNode(BooleanLiteralNode &node) = 0;
        virtual void visitCharacterLiteralNode(CharacterLiteralNode &node) = 0;
        virtual void visitIdentifierNode(IdentifierNode &node) = 0;
        // virtual void visitVariableDefinitionNode(VariableDefinitionNode &node) = 0;
        // virtual void visitVariableAssignmentNode(VariableAssignmentNode &node) = 0;
        virtual void visitParameterNode(ParameterNode &node) = 0;
        virtual void visitArgumentNode(ArgumentNode &node) = 0;
        virtual void visitFunctionDeclarationNode(FunctionDeclarationNode &node) = 0;
        virtual void visitConstructorDefinitionNode(ConstructorDefinitionNode &node) = 0;
        virtual void visitClassDeclarationNode(ClassDeclarationNode &node) = 0;
        virtual void visitClassDefinitionNode(ClassDefinitionNode &node) = 0;
        virtual void visitFunctionCallNode(FunctionCallNode &node) = 0;
        virtual void visitProgramNode(ProgramNode &node) = 0;
        virtual void visitInfixNode(const InfixExpressionNode& node) = 0;
        virtual void visitUnaryExpressionNode(UnaryExpressionNode node) = 0;
        virtual void visitExpressionStatementNode(ExpressionStatementNode node) = 0;
        virtual void visitPrefixExpressionNode(PrefixExpressionNode & node) = 0;
        virtual void visitPostfixNode(PostfixExpressionNode &node) = 0;
        virtual void visitParenRangerNode(ParenRangerNode &node) = 0;
        virtual void visitBlockRangerNode(BlockRangerNode &node) = 0;
        virtual void visitFunctionDefinitionNode(FunctionDefinitionNode &node) = 0;
        virtual void visitLabelNode(LabelNode & node) = 0;
        virtual void visitBranchNode(BranchNode & node) = 0;
        virtual void visitConditionNode(ConditionNode & node) = 0;
        virtual void visitLoopNode(LoopNode & node) = 0;
        virtual void visitForLoopNode(ForLoopNode & node) = 0;
        virtual void visitPassExpressionNode(PassExpressionNode & node) = 0;
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

    class PrintVisitor: public Visitor {
        std::ostringstream oss{};
        size_t indent = 4;
        size_t level = 0;
    public:
        explicit PrintVisitor(const size_t &indent);

        void printAllNode() const;

        void visitLiteralNode(LiteralNode &node) override;

        void visitStringLiteralNode(StringLiteralNode &node) override;

        void visitNumberLiteralNode(NumberLiteralNode &node) override;

        void visitIntegerLiteralNode(IntegerLiteralNode &node) override;

        void visitFloatLiteralNode(FloatLiteralNode &node) override;

        void visitBooleanLiteralNode(BooleanLiteralNode &node) override;

        void visitCharacterLiteralNode(CharacterLiteralNode &node) override;

        // void visitVariableDefinitionNode(VariableDefinitionNode &node) override;

        // void visitVariableAssignmentNode(VariableAssignmentNode &node) override;

        void visitParameterNode(ParameterNode &node) override;

        void visitArgumentNode(ArgumentNode &node) override;

        void visitFunctionDeclarationNode(FunctionDeclarationNode &node) override;

        void visitConstructorDefinitionNode(ConstructorDefinitionNode &node) override;

        void visitClassDeclarationNode(ClassDeclarationNode &node) override;

        void visitClassDefinitionNode(ClassDefinitionNode &node) override;

        void visitFunctionCallNode(FunctionCallNode &node) override;

        void visitProgramNode(ProgramNode &node) override;

        void visitInfixNode(const InfixExpressionNode &node) override;

        void visitUnaryExpressionNode(UnaryExpressionNode node) override;

        void visitExpressionStatementNode(ExpressionStatementNode node) override;

        void visitIdentifierNode(IdentifierNode &node) override;

        void visitPrefixExpressionNode(PrefixExpressionNode &node) override;

        void visitParenRangerNode(ParenRangerNode &node) override;

        void visitBlockRangerNode(BlockRangerNode &node) override;

        void visitFunctionDefinitionNode(FunctionDefinitionNode &node) override;

        void visitLabelNode(LabelNode &node) override;

        void visitBranchNode(BranchNode &node) override;

        void visitConditionNode(ConditionNode &node) override;

        void visitLoopNode(LoopNode &node) override;

        void visitForLoopNode(ForLoopNode &node) override;

        void visitPostfixNode(PostfixExpressionNode &node) override;

        void visitPassExpressionNode(PassExpressionNode &node) override;

        void visitEncapsulatedExpressionNode(EncapsulatedExpressionNode &node) override;

        void visitReturnExpressionNode(ReturnExpressionNode &node) override;

        void visitBreakExpressionNode(BreakExpressionNode &node) override;

        void visitAnonFunctionDefinitionNode(AnonFunctionDefinitionNode &node) override;

        void visitPairExpressionNode(PairExpressionNode &node) override;

        void visitDictionaryExpressionNode(DictionaryExpressionNode &node) override;

        void visitListExpressionNode(ListExpressionNode &node) override;

        void visitIndexExpressionNode(IndexExpressionNode &node) override;

        void visitBracketExpressionNode(BracketExpressionNode &node) override;
    };

    class JsonVisitor: public Visitor {
        RJsonBuilder builder;
    public:
        explicit JsonVisitor();

        bool saveJsonToFile (const std::string &filepath, const size_t &indent) const;

        [[nodiscard]] std::string getJsonString() const;

        [[nodiscard]] RJsonBuilder getRJBuilder() const;

        void visitLiteralNode(LiteralNode &node) override;

        void visitStringLiteralNode(StringLiteralNode &node) override;

        void visitNumberLiteralNode(NumberLiteralNode &node) override;

        void visitIntegerLiteralNode(IntegerLiteralNode &node) override;

        void visitFloatLiteralNode(FloatLiteralNode &node) override;

        void visitUnaryExpressionNode(UnaryExpressionNode node) override;

        void visitBooleanLiteralNode(BooleanLiteralNode &node) override;

        void visitCharacterLiteralNode(CharacterLiteralNode &node) override;

        // void visitVariableDefinitionNode(VariableDefinitionNode &node) override;

        // void visitVariableAssignmentNode(VariableAssignmentNode &node) override;

        void visitParameterNode(ParameterNode &node) override;

        void visitArgumentNode(ArgumentNode &node) override;

        void visitFunctionDeclarationNode(FunctionDeclarationNode &node) override;

        void visitConstructorDefinitionNode(ConstructorDefinitionNode &node) override;

        void visitClassDeclarationNode(ClassDeclarationNode &node) override;

        void visitClassDefinitionNode(ClassDefinitionNode &node) override;

        void visitFunctionCallNode(FunctionCallNode &node) override;

        void visitProgramNode(ProgramNode &node) override;

        void visitInfixNode(const InfixExpressionNode &node) override;

        void visitExpressionStatementNode(ExpressionStatementNode node) override;

        void visitIdentifierNode(IdentifierNode &node) override;

        void visitPrefixExpressionNode(PrefixExpressionNode &node) override;

        void visitParenRangerNode(ParenRangerNode &node) override;

        void visitBlockRangerNode(BlockRangerNode &node) override;

        void visitFunctionDefinitionNode(FunctionDefinitionNode &node) override;

        void visitLabelNode(LabelNode &node) override;

        void visitBranchNode(BranchNode &node) override;

        void visitConditionNode(ConditionNode &node) override;

        void visitLoopNode(LoopNode &node) override;

        void visitForLoopNode(ForLoopNode &node) override;

        void visitPostfixNode(PostfixExpressionNode &node) override;

        void visitPassExpressionNode(PassExpressionNode &node) override;

        void visitEncapsulatedExpressionNode(EncapsulatedExpressionNode &node) override;

        void visitReturnExpressionNode(ReturnExpressionNode &node) override;

        void visitBreakExpressionNode(BreakExpressionNode &node) override;

        void visitAnonFunctionDefinitionNode(AnonFunctionDefinitionNode &node) override;

        void visitPairExpressionNode(PairExpressionNode &node) override;

        void visitDictionaryExpressionNode(DictionaryExpressionNode &node) override;

        void visitListExpressionNode(ListExpressionNode &node) override;

        void visitIndexExpressionNode(IndexExpressionNode &node) override;

        void visitBracketExpressionNode(BracketExpressionNode &node) override;
    };

}

#endif //RCC_RCC_VISITOR_H

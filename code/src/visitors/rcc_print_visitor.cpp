//
// Created by RestRegular on 2025/6/30.
//


#include "../../include/analyzer/rcc_ast_components.h"
#include "../../include/visitors/rcc_print_visitor.h"

namespace ast {

    void PrintVisitor::visitLiteralNode(LiteralNode &node) {
        oss << node.formatString(indent, level);
    }

    void PrintVisitor::visitNullLiteralNode(NullLiteralNode& node)
    {
        oss << node.formatString(indent, level);
    }

    void PrintVisitor::visitStringLiteralNode(StringLiteralNode &node) {
        oss << node.formatString(indent, level);
    }

    void PrintVisitor::visitNumberLiteralNode(NumberLiteralNode &node) {
        oss << node.formatString(indent, level);
    }

    void PrintVisitor::visitIntegerLiteralNode(IntegerLiteralNode &node) {
        oss << node.formatString(indent, level);
    }

    void PrintVisitor::visitFloatLiteralNode(FloatLiteralNode &node) {
        oss << node.formatString(indent, level);
    }

    void PrintVisitor::visitBooleanLiteralNode(BooleanLiteralNode &node) {
        oss << node.formatString(indent, level);
    }

    void PrintVisitor::visitCharacterLiteralNode(CharacterLiteralNode &node) {
        oss << node.formatString(indent, level);
    }

    void PrintVisitor::visitVariableDefinitionNode(VariableDefinitionNode &node) {
    }

    void PrintVisitor::visitAssignmentNode(AssignmentNode &node) {
    }

    void PrintVisitor::visitParameterNode(ParameterNode &node) {
        oss << node.formatString(indent, level);
    }

    void PrintVisitor::visitArgumentNode(ArgumentNode &node) {
        oss << node.formatString(indent, level);
    }

    void PrintVisitor::visitFunctionDeclarationNode(FunctionDeclarationNode &node) {
        oss << node.formatString(indent, level);
    }

    void PrintVisitor::visitConstructorDefinitionNode(ConstructorDefinitionNode &node) {
        oss << node.formatString(indent, level);
    }

    void PrintVisitor::visitClassDeclarationNode(ClassDeclarationNode &node) {
        oss << node.formatString(indent, level);
    }

    void PrintVisitor::visitClassDefinitionNode(ClassDefinitionNode &node) {
        oss << node.formatString(indent, level);
    }

    void PrintVisitor::visitFunctionCallNode(FunctionCallNode &node) {
        oss << node.formatString(indent, level);
    }

    void PrintVisitor::visitProgramNode(ProgramNode &node) {
        oss << spaceString(indent * level) << "Program={\n";
        level++;
        const auto &statements = node.getStatements();
        for (const auto &statement: statements) {
            statement->acceptVisitor(*this);
            oss << std::endl;
        }
        level--;
        oss << spaceString(indent * level) << "}";
    }

    void PrintVisitor::visitInfixNode(InfixExpressionNode &node) {
        oss << node.formatString(indent, level);
    }

    void PrintVisitor::visitUnaryExpressionNode(UnaryExpressionNode &node) {
        oss << node.formatString(indent, level);
    }

    void PrintVisitor::visitExpressionStatementNode(ExpressionStatementNode &node) {
        oss << spaceString(indent * level) << "ExpressionStatement={\n";
        level++;
        node.getExpression()->acceptVisitor(*this);
        oss << std::endl;
        level--;
        oss << spaceString(indent * level) << "}";
    }

    void PrintVisitor::visitIdentifierNode(IdentifierNode &node) {
        oss << spaceString(indent * level) << "IdentifierNode={\n";
        level++;
        oss << spaceString(indent * level) << "name=" << node.getName() << "\n";
        level--;
        oss << spaceString(indent * level) << "}";
    }

    void PrintVisitor::visitPrefixExpressionNode(PrefixExpressionNode &node) {

    }

    void PrintVisitor::visitParenRangerNode(ParenRangerNode &node) {

    }

    void PrintVisitor::visitBlockRangerNode(BlockRangerNode &node) {

    }

    void PrintVisitor::visitFunctionDefinitionNode(FunctionDefinitionNode &node) {

    }

    void PrintVisitor::visitLabelNode(LabelNode &node) {

    }

    void PrintVisitor::visitBranchNode(BranchNode &node) {

    }

    void PrintVisitor::visitConditionNode(ConditionNode &node) {

    }

    void PrintVisitor::visitLoopNode(LoopNode &node) {

    }

    void PrintVisitor::visitForLoopNode(ForLoopNode &node) {

    }

    void PrintVisitor::visitPostfixNode(PostfixExpressionNode &node) {

    }

    void PrintVisitor::visitPassExpressionNode(PassExpressionNode &node) {

    }

    void PrintVisitor::visitEncapsulatedExpressionNode(EncapsulatedExpressionNode &node) {

    }

    void PrintVisitor::visitReturnExpressionNode(ReturnExpressionNode &node) {

    }

    void PrintVisitor::visitBreakExpressionNode(BreakExpressionNode &node) {

    }

    void PrintVisitor::visitAnonFunctionDefinitionNode(AnonFunctionDefinitionNode &node) {

    }

    void PrintVisitor::visitPairExpressionNode(PairExpressionNode &node) {

    }

    void PrintVisitor::visitDictionaryExpressionNode(DictionaryExpressionNode &node) {

    }

    void PrintVisitor::visitListExpressionNode(ListExpressionNode &node) {

    }

    void PrintVisitor::visitBracketExpressionNode(BracketExpressionNode &node) {

    }

    void PrintVisitor::visitIndexExpressionNode(IndexExpressionNode &node) {

    }

    PrintVisitor::PrintVisitor(const size_t &indent = 4): indent(indent){}

    void PrintVisitor::printAllNode() const {
        std::cout << oss.str() << std::endl;
    }

}
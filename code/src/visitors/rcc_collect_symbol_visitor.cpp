//
// Created by RestRegular on 2025/7/14.
//

#include <ranges>

#include "../../include/analyzer/rcc_ast_components.h"
#include "../../include/visitors/rcc_collect_symbol_visitor.h"
#include "../../include/lib/RLogSystem/rlog_system.h"
#include "../../include/components/symbol/rcc_symbol.h"

namespace ast {
    using namespace rlog;
    using namespace symbol;

    SymbolTableManager CollectSymbolVisitor::getSymbolTable() const {
        return symbolTable;
    }

    void CollectSymbolVisitor::visitLiteralNode(LiteralNode &node) {
    }

    void CollectSymbolVisitor::visitNullLiteralNode(NullLiteralNode& node)
    {

    }

    void CollectSymbolVisitor::visitStringLiteralNode(StringLiteralNode &node) {
    }

    void CollectSymbolVisitor::visitNumberLiteralNode(NumberLiteralNode &node) {
    }

    void CollectSymbolVisitor::visitIntegerLiteralNode(IntegerLiteralNode &node) {
    }

    void CollectSymbolVisitor::visitFloatLiteralNode(FloatLiteralNode &node) {
    }

    void CollectSymbolVisitor::visitBooleanLiteralNode(BooleanLiteralNode &node) {
    }

    void CollectSymbolVisitor::visitCharacterLiteralNode(CharacterLiteralNode &node) {
    }

    void CollectSymbolVisitor::visitIdentifierNode(IdentifierNode &node) {
        std::unordered_set<std::shared_ptr<LabelSymbol>> labels {};
        if (node.getColonTokenPtr()) {
            CollectSymbolVisitor labelVisitor {};
            for (const auto &label: node.getLabels()) {
                label->acceptVisitor(labelVisitor);
            }
            for (const auto &label: labelVisitor.getSymbolTable().currentNameMapScope()) {
                if (label->getType() == SymbolType::LABEL) {
                    labels.insert(std::static_pointer_cast<LabelSymbol>(label));

                }
            }
        }
        // symbolTable.insert(std::make_shared<VariableSymbol>(node.getPos(),
        //     node.getName(), , labels, symbolTable.curScopeLevel()));
    }

    void CollectSymbolVisitor::visitParameterNode(ParameterNode &node) {
    }

    void CollectSymbolVisitor::visitArgumentNode(ArgumentNode &node) {
    }

    void CollectSymbolVisitor::visitFunctionDeclarationNode(FunctionDeclarationNode &node) {
    }

    void CollectSymbolVisitor::visitConstructorDefinitionNode(ConstructorDefinitionNode &node) {
    }

    void CollectSymbolVisitor::visitClassDeclarationNode(ClassDeclarationNode &node) {
    }

    void CollectSymbolVisitor::visitClassDefinitionNode(ClassDefinitionNode &node) {
    }

    void CollectSymbolVisitor::visitFunctionCallNode(FunctionCallNode &node) {
    }

    void CollectSymbolVisitor::visitInfixNode(InfixExpressionNode &node) {
        node.getLeftNode()->acceptVisitor(*this);
        node.getRightNode()->acceptVisitor(*this);
    }

    void CollectSymbolVisitor::visitUnaryExpressionNode(UnaryExpressionNode &node) {
    }

    void CollectSymbolVisitor::visitPrefixExpressionNode(PrefixExpressionNode &node) {
        LogManager::rlog("collect prefix expression node");
        node.getNode()->acceptVisitor(*this);
    }

    void CollectSymbolVisitor::visitPostfixNode(PostfixExpressionNode &node) {
    }

    void CollectSymbolVisitor::visitParenRangerNode(ParenRangerNode &node) {
    }

    void CollectSymbolVisitor::visitBlockRangerNode(BlockRangerNode &node) {
    }

    void CollectSymbolVisitor::visitFunctionDefinitionNode(FunctionDefinitionNode &node) {
    }

    void CollectSymbolVisitor::visitLabelNode(LabelNode &node) {
    }

    void CollectSymbolVisitor::visitBranchNode(BranchNode &node) {
    }

    void CollectSymbolVisitor::visitConditionNode(ConditionNode &node) {
    }

    void CollectSymbolVisitor::visitLoopNode(LoopNode &node) {
    }

    void CollectSymbolVisitor::visitForLoopNode(ForLoopNode &node) {
    }

    void CollectSymbolVisitor::visitPassExpressionNode(PassExpressionNode &node) {
    }

    void CollectSymbolVisitor::visitEncapsulatedExpressionNode(EncapsulatedExpressionNode &node) {
    }

    void CollectSymbolVisitor::visitReturnExpressionNode(ReturnExpressionNode &node) {
    }

    void CollectSymbolVisitor::visitBreakExpressionNode(BreakExpressionNode &node) {
    }

    void CollectSymbolVisitor::visitAnonFunctionDefinitionNode(AnonFunctionDefinitionNode &node) {
    }

    void CollectSymbolVisitor::visitPairExpressionNode(PairExpressionNode &node) {
    }

    void CollectSymbolVisitor::visitDictionaryExpressionNode(DictionaryExpressionNode &node) {
    }

    void CollectSymbolVisitor::visitListExpressionNode(ListExpressionNode &node) {
    }

    void CollectSymbolVisitor::visitBracketExpressionNode(BracketExpressionNode &node) {
    }

    void CollectSymbolVisitor::visitIndexExpressionNode(IndexExpressionNode &node) {
    }

    void CollectSymbolVisitor::visitTryNode(TryNode& try_node)
    {
    }

    void CollectSymbolVisitor::visitThrowNode(ThrowNode& node)
    {
    }

    void CollectSymbolVisitor::visitVariableDefinitionNode(VariableDefinitionNode &node) {
    }

    void CollectSymbolVisitor::visitAssignmentNode(AssignmentNode &node) {
    }

    void CollectSymbolVisitor::visitProgramNode(ProgramNode &node) {
        for (const auto &statements = node.getStatements();
            const auto &statement: statements) {
            statement->acceptVisitor(*this);
        }
    }

    void CollectSymbolVisitor::visitExpressionStatementNode(ExpressionStatementNode &node) {
        node.getExpression()->acceptVisitor(*this);
    }
}

//
// Created by RestRegular on 2025/6/30.
//

#include "../../include/visitors/rcc_json_visitor.h"
#include "../../include/analyzer/rcc_ast_components.h"
#include "../../include/lib/rcc_utils.h"

namespace ast {
    using namespace rjson::rj;

    JsonVisitor::JsonVisitor()
    : Visitor(), builder(rjson::RJType::RJ_OBJECT){}

    bool JsonVisitor::saveJsonToFile(const std::string &filepath, const size_t &indent) const {
        try {
            writeFile(filepath, builder.formatString(indent, 0));
            return true;
        } catch (const std::exception &e) {
            std::cerr << e.what() << std::endl;
            return false;
        }
    }

    std::string JsonVisitor::getJsonString() const {
        return builder.toJsonString();
    }

    RJsonBuilder JsonVisitor::getRJBuilder() const {
        return builder;
    }

    void JsonVisitor::visitLiteralNode(LiteralNode &node) {
        builder.insertString("nodeType", getNodeTypeName(node.getType()))
        .insertObject("node", {
            node.getMainToken().toRJPair(),
            {"literalString", node.literalString()}
        });
    }

    void JsonVisitor::visitNullLiteralNode(NullLiteralNode& node)
    {
        visitLiteralNode(node);
    }

    void JsonVisitor::visitStringLiteralNode(StringLiteralNode &node) {
        visitLiteralNode(node);
    }

    void JsonVisitor::visitNumberLiteralNode(NumberLiteralNode &node) {
        visitLiteralNode(node);
    }

    void JsonVisitor::visitIntegerLiteralNode(IntegerLiteralNode &node) {
        visitLiteralNode(node);
    }

    void JsonVisitor::visitFloatLiteralNode(FloatLiteralNode &node) {
        visitLiteralNode(node);
    }

    void JsonVisitor::visitBooleanLiteralNode(BooleanLiteralNode &node) {
        visitLiteralNode(node);
    }

    void JsonVisitor::visitCharacterLiteralNode(CharacterLiteralNode &node) {
        visitLiteralNode(node);
    }

    void JsonVisitor::visitVariableDefinitionNode(VariableDefinitionNode &node) {
    }

    void JsonVisitor::visitAssignmentNode(AssignmentNode &node) {
    }

    void JsonVisitor::visitParameterNode(ParameterNode &node) {

    }

    void JsonVisitor::visitArgumentNode(ArgumentNode &node) {

    }

    void JsonVisitor::visitFunctionDeclarationNode(FunctionDeclarationNode &node) {
        auto builderRecord = builder;
        builderRecord.insertString("nodeType", getNodeTypeName(NodeType::FUNCTION_DECLARATION));
        auto functionNode = RJsonBuilder(rjson::RJType::RJ_OBJECT);
        functionNode.insertRJValue("mainToken", node.getMainToken().toRJValue())
        .insertString("opSign", node.getMainToken().getValue());
        builder = RJsonBuilder(rjson::RJType::RJ_OBJECT);
        node.getCallNode()->acceptVisitor(*this);
        functionNode.insertObject("callNode", builder.build().getObjectValue());
        if (!node.getLabelNodes().empty()) {
            functionNode.insertObject("colonToken", node.getColonToken().toRJValue().getObjectValue());
            auto labelList = RJsonBuilder(rjson::RJType::RJ_LIST);
            for (const auto &label : node.getLabelNodes()){
                builder = RJsonBuilder(rjson::RJType::RJ_OBJECT);
                label->acceptVisitor(*this);
                labelList.appendObject(builder.build().getObjectValue());
            }
            functionNode.insertList("labelNode", labelList.build().getListValue());
        }
        builderRecord.insertObject("node", functionNode.build().getObjectValue());
        builder = builderRecord;
    }

    void JsonVisitor::visitConstructorDefinitionNode(ConstructorDefinitionNode &node) {
        auto builderRecord = builder;
        builderRecord.insertString("nodeType", getNodeTypeName(NodeType::CONSTRUCTOR));
        auto ctorNode = RJsonBuilder(rjson::RJType::RJ_OBJECT);
        builder = RJsonBuilder(rjson::RJType::RJ_OBJECT);
        node.getParamNode()->acceptVisitor(*this);
        ctorNode.insertObject("paramNode", builder.build().getObjectValue());
        if (node.getColonToken()) {
            ctorNode.insertObject("colonToken", node.getColonToken()->toRJValue().getObjectValue());
            auto labelList = RJsonBuilder(rjson::RJType::RJ_LIST);
            for (const auto &label : node.getLabelNodes()){
                builder = RJsonBuilder(rjson::RJType::RJ_OBJECT);
                label->acceptVisitor(*this);
                labelList.appendObject(builder.build().getObjectValue());
            }
            ctorNode.insertList("labelNodes", labelList.build().getListValue());
        }
        builder = RJsonBuilder(rjson::RJType::RJ_OBJECT);
        node.getBodyNode()->acceptVisitor(*this);
        ctorNode.insertObject("bodyNode", builder.build().getObjectValue());
        builderRecord.insertObject("node", ctorNode.build().getObjectValue());
        builder = builderRecord;
    }

    void JsonVisitor::visitClassDeclarationNode(ClassDeclarationNode &node) {
        auto builderRecord = builder;
        builderRecord.insertString("nodeType", getNodeTypeName(NodeType::CLASS_DECLARATION));
        auto classNode = RJsonBuilder(rjson::RJType::RJ_OBJECT);
        classNode.insertObject("mainToken", node.getMainToken().toRJValue().getObjectValue())
        .insertString("opSign", node.getMainToken().getValue());
        builder = RJsonBuilder(rjson::RJType::RJ_OBJECT);
        node.getNameNode()->acceptVisitor(*this);
        classNode.insertObject("nameNode", builder.build().getObjectValue());
        builderRecord.insertObject("node", classNode.build().getObjectValue());
        builder = builderRecord;
    }

    void JsonVisitor::visitClassDefinitionNode(ClassDefinitionNode &node) {
        auto builderRecord = builder;
        builderRecord.insertString("nodeType", getNodeTypeName(NodeType::CLASS_DEFINITION));
        auto classNode = RJsonBuilder(rjson::RJType::RJ_OBJECT);
        classNode.insertObject("mainToken", node.getMainToken().toRJValue().getObjectValue())
        .insertString("opSign", node.getMainToken().getValue());
        builder = RJsonBuilder(rjson::RJType::RJ_OBJECT);
        node.getNameNode()->acceptVisitor(*this);
        classNode.insertObject("nameNode", builder.build().getObjectValue());
        builder = RJsonBuilder(rjson::RJType::RJ_OBJECT);
        node.getBodyNode()->acceptVisitor(*this);
        classNode.insertObject("bodyNode", builder.build().getObjectValue());
        builderRecord.insertObject("node", classNode.build().getObjectValue());
        builder = builderRecord;
    }

    void JsonVisitor::visitFunctionCallNode(FunctionCallNode &node) {
        auto builderRecord = builder;
        builderRecord.insertString("nodeType", getNodeTypeName(node.getInfixType()));
        auto infixNode = RJsonBuilder(rjson::RJType::RJ_OBJECT);
        infixNode.insertRJValue("mainToken", node.getMainToken().toRJValue());
        builder = RJsonBuilder(rjson::RJType::RJ_OBJECT);
        node.getLeftNode()->acceptVisitor(*this);
        infixNode.insertRJValue("leftNode", builder.build());
        builder = RJsonBuilder(rjson::RJType::RJ_OBJECT);
        node.getRightNode()->acceptVisitor(*this);
        infixNode.insertRJValue("rightNode", builder.build());
        builderRecord.insertRJValue("node", infixNode.build());
        builder = builderRecord;
    }

    void JsonVisitor::visitInfixNode(InfixExpressionNode &node) {
        auto builderRecord = builder;
        builderRecord.insertString("nodeType", getNodeTypeName(node.getInfixType()));
        auto infixNode = RJsonBuilder(rjson::RJType::RJ_OBJECT);
        infixNode.insertRJValue("mainToken", node.getMainToken().toRJValue())
        .insertString("opSign", node.getOpToken().getValue());
        builder = RJsonBuilder(rjson::RJType::RJ_OBJECT);
        node.getLeftNode()->acceptVisitor(*this);
        infixNode.insertRJValue("leftNode", builder.build());
        builder = RJsonBuilder(rjson::RJType::RJ_OBJECT);
        node.getRightNode()->acceptVisitor(*this);
        infixNode.insertRJValue("rightNode", builder.build());
        builderRecord.insertRJValue("node", infixNode.build());
        builder = builderRecord;
    }

    void JsonVisitor::visitExpressionStatementNode(ExpressionStatementNode &node) {
        builder.insertRJValue("mainToken", node.getMainToken().toRJValue())
        .insertString("nodeType", getNodeTypeName(node.getExpression()->getType()));
        auto builderRecord = builder;
        builder = RJsonBuilder(rjson::RJType::RJ_OBJECT);
        node.getExpression()->acceptVisitor(*this);
        builderRecord.insertRJValue("node", builder.build());
        builder = builderRecord;
    }

    void JsonVisitor::visitIdentifierNode(IdentifierNode &node) {
        auto builderRecord = builder;
        builderRecord.insertString("nodeType", getNodeTypeName(node.getType()));
        auto identifierNode = RJsonBuilder(rjson::RJType::RJ_OBJECT);
        identifierNode.insertObject("mainToken", node.getMainToken().toRJValue().getObjectValue())
        .insertString("name", node.getName());
        if (!node.getLabels().empty()){
            identifierNode.insertObject("colonToken", node.getColonToken().toRJValue().getObjectValue());
            auto labels = RJsonBuilder(rjson::RJType::RJ_LIST);
            for (const auto &label: node.getLabels()) {
                builder = RJsonBuilder(rjson::RJType::RJ_OBJECT);
                label->acceptVisitor(*this);
                labels.appendObject(builder.build().getObjectValue());
            }
            identifierNode.insertRJValue("labelNodes", labels.build());
        }
        builderRecord.insertRJValue("node", identifierNode.build());
        builder = builderRecord;
    }

    void JsonVisitor::visitPrefixExpressionNode(PrefixExpressionNode &node) {
        auto builderRecord = builder;
        builder = RJsonBuilder(rjson::RJType::RJ_OBJECT);
        node.getNode()->acceptVisitor(*this);
        builderRecord.insertString("nodeType", getNodeTypeName(node.getPrefixType()))
        .insertRJValue("mainToken", node.getMainToken().toRJValue())
        .insertString("opSign", node.getMainToken().getValue())
        .insertRJValue("node", builder.build());
        builder = builderRecord;
    }

    void JsonVisitor::visitPostfixNode(PostfixExpressionNode &node) {
        auto builderRecord = builder;
        builder = RJsonBuilder(rjson::RJType::RJ_OBJECT);
        node.getNode()->acceptVisitor(*this);
        builderRecord.insertString("nodeType", getNodeTypeName(node.getType()))
        .insertRJValue("mainToken", node.getMainToken().toRJValue())
        .insertString("opSign", node.getMainToken().getValue())
        .insertRJValue("node", builder.build());
        builder = builderRecord;
    }

    void JsonVisitor::visitPassExpressionNode(PassExpressionNode &node) {
        builder.insertString("nodeType", getNodeTypeName(NodeType::PASS))
        .insertRJValue("opToken", node.getMainToken().toRJValue());
    }

    void JsonVisitor::visitEncapsulatedExpressionNode(EncapsulatedExpressionNode &node) {
        builder.insertString("nodeType", getNodeTypeName(NodeType::ENCAPSULATED))
        .insertRJValue("opToken", node.getMainToken().toRJValue());
    }

    void JsonVisitor::visitReturnExpressionNode(ReturnExpressionNode &node) {
        auto builderRecord = builder;
        builderRecord.insertString("nodeType", getNodeTypeName(NodeType::RETURN))
        .insertObject("mainToken", node.getMainToken().toRJValue().getObjectValue());
        builder = RJsonBuilder(rjson::RJType::RJ_OBJECT);
        node.getReturnNode()->acceptVisitor(*this);
        builderRecord.insertRJValue("node", builder.build());
        builder = builderRecord;
    }

    void JsonVisitor::visitBreakExpressionNode(BreakExpressionNode &node) {
        builder.insertString("nodeType", getNodeTypeName(NodeType::BREAK))
        .insertObject("mainToken", node.getMainToken().toRJValue().getObjectValue());
    }

    void JsonVisitor::visitAnonFunctionDefinitionNode(AnonFunctionDefinitionNode &node) {
        auto builderRecord = builder;
        builderRecord.insertString("nodeType", getNodeTypeName(NodeType::ANON_FUNCTION_DEFINITION))
        .insertObject("mainToken", node.getMainToken().toRJValue().getObjectValue());
        builder = RJsonBuilder(rjson::RJType::RJ_OBJECT);
        node.getParamNode()->acceptVisitor(*this);
        builderRecord.insertRJValue("paramNode", builder.build());
        if (node.getColonToken()) {
            builderRecord.insertObject("colonToken", node.getColonToken()->toRJValue().getObjectValue());
            auto labels = RJsonBuilder(rjson::RJType::RJ_LIST);
            for (const auto &label: node.getLabelNodes()) {
                builder = RJsonBuilder(rjson::RJType::RJ_OBJECT);
                label->acceptVisitor(*this);
                labels.appendObject(builder.build().getObjectValue());
            }
            builderRecord.insertList("labelNodes", labels.build().getListValue());
        }
        builderRecord.insertObject("indicatorToken", node.getIndicatorToken().toRJValue().getObjectValue());
        builder = RJsonBuilder(rjson::RJType::RJ_OBJECT);
        node.getBodyNode()->acceptVisitor(*this);
        builderRecord.insertObject("bodyNode", builder.build().getObjectValue());
        builder = builderRecord;
    }

    void JsonVisitor::visitPairExpressionNode(PairExpressionNode &node) {
        auto builderRecord = builder;
        builderRecord.insertString("nodeType", getNodeTypeName(node.getType()))
        .insertObject("mainToken", node.getMainToken().toRJValue().getObjectValue());
        auto pairNode = RJsonBuilder(rjson::RJType::RJ_OBJECT);
        pairNode.insertString("nodeType", getNodeTypeName(NodeType::PAIR))
        .insertObject("colonToken", node.getColonToken().toRJValue().getObjectValue());
        builder = RJsonBuilder(rjson::RJType::RJ_OBJECT);
        node.getLeftNode()->acceptVisitor(*this);
        pairNode.insertObject("leftNode", builder.build().getObjectValue());
        builder = RJsonBuilder(rjson::RJType::RJ_OBJECT);
        node.getRightNode()->acceptVisitor(*this);
        pairNode.insertObject("rightNode", builder.build().getObjectValue());
        builderRecord.insertObject("pairNode", pairNode.build().getObjectValue());
        builder = builderRecord;
    }

    void JsonVisitor::visitDictionaryExpressionNode(DictionaryExpressionNode &node) {
        auto builderRecord = builder;
        builderRecord.insertString("nodeType", getNodeTypeName(NodeType::DICTIONARY));
        auto dictNode = RJsonBuilder(rjson::RJType::RJ_OBJECT);
        dictNode.insertObject("lBraceToken", node.getRangerStartToken().toRJValue().getObjectValue())
        .insertObject("rBraceToken", node.getRangerEndToken().toRJValue().getObjectValue());
        builder = RJsonBuilder(rjson::RJType::RJ_OBJECT);
        node.getBodyNode()->acceptVisitor(*this);
        dictNode.insertObject("bodyNode", builder.build().getObjectValue());
        builderRecord.insertObject("dictNode", dictNode.build().getObjectValue());
        builder = builderRecord;
    }

    void JsonVisitor::visitListExpressionNode(ListExpressionNode &node) {
        auto builderRecord = builder;
        builderRecord.insertString("nodeType", getNodeTypeName(NodeType::LIST));
        auto listNode = RJsonBuilder(rjson::RJType::RJ_OBJECT);
        listNode.insertObject("lBracketToken", node.getRangerStartToken().toRJValue().getObjectValue())
        .insertObject("rBracketToken", node.getRangerEndToken().toRJValue().getObjectValue());
        builder = RJsonBuilder(rjson::RJType::RJ_OBJECT);
        node.getBodyNode()->acceptVisitor(*this);
        listNode.insertObject("bodyNode", builder.build().getObjectValue());
        builderRecord.insertObject("listNode", listNode.build().getObjectValue());
        builder = builderRecord;
    }

    void JsonVisitor::visitBracketExpressionNode(BracketExpressionNode &node) {
        auto builderRecord = builder;
        builderRecord.insertString("nodeType", getNodeTypeName(NodeType::BRACKET));
        auto bracketNode = RJsonBuilder(rjson::RJType::RJ_OBJECT);
        bracketNode.insertObject("lBracketToken", node.getRangerStartToken().toRJValue().getObjectValue())
        .insertObject("rBracketToken", node.getRangerEndToken().toRJValue().getObjectValue());
        builder = RJsonBuilder(rjson::RJType::RJ_OBJECT);
        node.getBodyNode()->acceptVisitor(*this);
        bracketNode.insertObject("bodyNode", builder.build().getObjectValue());
        builderRecord.insertObject("node", bracketNode.build().getObjectValue());
        builder = builderRecord;
    }

    void JsonVisitor::visitIndexExpressionNode(IndexExpressionNode &node) {
        auto builderRecord = builder;
        builderRecord.insertString("nodeType", getNodeTypeName(node.getType()))
        .insertObject("mainToken", node.getMainToken().toRJValue().getObjectValue());
        auto indexNode = RJsonBuilder(rjson::RJType::RJ_OBJECT);
        builder = RJsonBuilder(rjson::RJType::RJ_OBJECT);
        node.getLeftNode()->acceptVisitor(*this);
        indexNode.insertObject("leftNode", builder.build().getObjectValue());
        builder = RJsonBuilder(rjson::RJType::RJ_OBJECT);
        node.getIndexNode()->acceptVisitor(*this);
        indexNode.insertObject("indexNode", builder.build().getObjectValue());
        builderRecord.insertObject("node", indexNode.build().getObjectValue());
        builder = builderRecord;
    }

    void JsonVisitor::visitTryNode(TryNode& try_node)
    {
        pass();
    }

    void JsonVisitor::visitThrowNode(ThrowNode& node)
    {
        pass();
    }

    void JsonVisitor::visitParenRangerNode(ParenRangerNode &node) {
        auto builderRecord = builder;
        builderRecord.insertString("nodeType", getNodeTypeName(node.getRangerType()))
        .insertRJValue("lParenToken", node.getRangerStartToken().toRJson())
        .insertRJValue("rParenToken", node.getRangerEndToken().toRJson());
        if (node.getRangerNode()) {
            builder = RJsonBuilder(rjson::RJType::RJ_OBJECT);
            node.getRangerNode()->acceptVisitor(*this);
            builderRecord.insertRJValue("node", builder.build());
        } else {
            builderRecord.insertNull("node");
        }
        builder = builderRecord;
    }

    void JsonVisitor::visitBlockRangerNode(BlockRangerNode &node) {
        auto builderRecord = builder;
        auto expressionList = RJsonBuilder(rjson::RJType::RJ_LIST);
        for (const auto &child : node.getBodyExpressions()) {
            builder = RJsonBuilder(rjson::RJType::RJ_OBJECT);
            child->acceptVisitor(*this);
            expressionList.appendObject({
                {"nodeType", getNodeTypeName(child->getType())},
                {"node", builder.build()}
            });
        }
        builderRecord.insertString("nodeType", getNodeTypeName(node.getRangerType()))
        .insertRJValue("lBlockToken", node.getRangerStartToken().toRJson())
        .insertRJValue("rBlockToken", node.getRangerEndToken().toRJson())
        .insertRJValue("blockBody", expressionList.build());
        builder = builderRecord;
    }

    void JsonVisitor::visitFunctionDefinitionNode(FunctionDefinitionNode &node) {
        auto builderRecord = builder;
        builderRecord.insertString("nodeType", getNodeTypeName(NodeType::FUNCTION_DEFINITION));
        auto functionNode = RJsonBuilder(rjson::RJType::RJ_OBJECT);
        functionNode.insertRJValue("mainToken", node.getMainToken().toRJValue())
        .insertString("opSign", node.getMainToken().getValue());
        builder = RJsonBuilder(rjson::RJType::RJ_OBJECT);
        node.getCallNode()->acceptVisitor(*this);
        functionNode.insertObject("callNode", builder.build().getObjectValue());
        if (!node.getLabelNodes().empty()) {
            functionNode.insertObject("colonToken", node.getColonToken().toRJValue().getObjectValue());
            auto labelList = RJsonBuilder(rjson::RJType::RJ_LIST);
            for (const auto &label : node.getLabelNodes()){
                builder = RJsonBuilder(rjson::RJType::RJ_OBJECT);
                label->acceptVisitor(*this);
                labelList.appendObject(builder.build().getObjectValue());
            }
            functionNode.insertList("labelNodes", labelList.build().getListValue());
        }
        if (node.getIndicatorToken()) {
            functionNode.insertObject("indicatorToken", node.getIndicatorToken()->toRJValue().getObjectValue());
        }
        builder = RJsonBuilder(rjson::RJType::RJ_OBJECT);
        node.getBodyNode()->acceptVisitor(*this);
        functionNode.insertObject("bodyNode", builder.build().getObjectValue());
        builderRecord.insertObject("node", functionNode.build().getObjectValue());
        builder = builderRecord;
    }

    void JsonVisitor::visitLabelNode(LabelNode &node) {
        builder.insertString("nodeType", getNodeTypeName(node.getType()))
        .insertObject("node", {
            node.getMainToken().toRJPair(),
            {"label", node.getLabel()}
        });
    }

    void JsonVisitor::visitBranchNode(BranchNode &node) {
        auto builderRecord = builder;
        builderRecord.insertString("nodeType", getNodeTypeName(node.getType()))
        .insertObject("mainToken", node.getMainToken().toRJValue().getObjectValue())
        .insertString("opSign", node.getMainToken().getValue());
        builder = RJsonBuilder(rjson::RJType::RJ_OBJECT);
        node.getConditionNode()->acceptVisitor(*this);
        builderRecord.insertRJValue("conditionNode", builder.build());
        builder = RJsonBuilder(rjson::RJType::RJ_OBJECT);
        node.getBodyNode()->acceptVisitor(*this);
        builderRecord.insertRJValue("bodyNode", builder.build());
        builder = builderRecord;
    }

    void JsonVisitor::visitConditionNode(ConditionNode &node) {
        auto builderRecord = builder;
        builderRecord.insertString("nodeType", getNodeTypeName(node.getType()));
        auto branchNodes = RJsonBuilder(rjson::RJType::RJ_LIST);
        for (const auto &branch: node.getBranchNodes()) {
            builder = RJsonBuilder(rjson::RJType::RJ_OBJECT);
            branch->acceptVisitor(*this);
            branchNodes.appendRJValue(builder.build());
        }
        builderRecord.insertRJValue("branchNodes", branchNodes.build());
        builder = builderRecord;
    }

    void JsonVisitor::visitLoopNode(LoopNode &node) {
        auto builderRecord = builder;
        builderRecord.insertString("opSign", node.getMainToken().getValue());
        builder = RJsonBuilder(rjson::RJType::RJ_OBJECT);
        node.getConditionNode()->acceptVisitor(*this);
        builderRecord.insertRJValue("conditionNode", builder.build());
        builder = RJsonBuilder(rjson::RJType::RJ_OBJECT);
        node.getBodyNode()->acceptVisitor(*this);
        builderRecord.insertRJValue("bodyNode", builder.build());
        builder = builderRecord;
    }

    void JsonVisitor::visitForLoopNode(ForLoopNode &node) {
        auto builderRecord = builder;
        builderRecord.insertString("opSign", node.getMainToken().getValue());
        builder = RJsonBuilder(rjson::RJType::RJ_OBJECT);
        node.getInitNode()->acceptVisitor(*this);
        builderRecord.insertRJValue("initNode", builder.build());
        builder = RJsonBuilder(rjson::RJType::RJ_OBJECT);
        node.getConditionNode()->acceptVisitor(*this);
        builderRecord.insertRJValue("conditionNode", builder.build());
        builder = RJsonBuilder(rjson::RJType::RJ_OBJECT);
        node.getUpdateNode()->acceptVisitor(*this);
        builderRecord.insertRJValue("updateNode", builder.build());
        builder = RJsonBuilder(rjson::RJType::RJ_OBJECT);
        node.getBodyNode()->acceptVisitor(*this);
        builderRecord.insertRJValue("bodyNode", builder.build());
        builder = builderRecord;
    }

    void JsonVisitor::visitUnaryExpressionNode(UnaryExpressionNode &node) {
        auto builderRecord = builder;
        builder = RJsonBuilder(rjson::RJType::RJ_OBJECT);
        node.getRightNode()->acceptVisitor(*this);
        builderRecord.insertString("nodeType", getNodeTypeName(node.getType()))
        .insertObject("node", {
            node.getMainToken().toRJPair(),
            {"opSign", node.getOpToken().getValue()},
            {"rightNode", builder.build()}
        });
        builder = builderRecord;
    }

    void JsonVisitor::visitProgramNode(ProgramNode &node) {
        auto builderRecord = builder;
        auto programNodeBuilder = RJsonBuilder(rjson::RJType::RJ_OBJECT);
        programNodeBuilder.insertRJValue("programInfo", node.getMainToken().toRJValue());
        auto statementList = RJsonBuilder(rjson::RJType::RJ_LIST);
        for (const auto &statement: node.getStatements()) {
            builder = RJsonBuilder(rjson::RJType::RJ_OBJECT);
            statement->acceptVisitor(*this);
            statementList.appendObject({
                {"nodeType", getNodeTypeName(statement->getType())},
                {"node", builder.build()}
            });
        }
        programNodeBuilder.insertRJValue("Statements", statementList.build());
        builderRecord.insertRJValue("node", programNodeBuilder.build());
        builder = builderRecord;
    }

}

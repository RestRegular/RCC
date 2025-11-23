//
// Created by RestRegular on 2025/6/29.
//

#include <utility>

#include "../../../include/rcc_base.h"
#include "../../../include/analyzer/rcc_ast_components.h"

namespace ast {
    std::string Node::toString() const {
        return "[Node(" + getNodeTypeName(type) + "): " + mainToken.toString() + " " + getPos().toString() + "]";
    }

    std::string Node::briefString() const {
        return "[Node(" + getNodeTypeName(type) + "): " + mainToken.briefString() + " " + getPos().briefString() + "]";
    }

    std::string Node::professionalString() const {
        return "Node{nodeType=" + getNodeTypeName(type) + ", mainToken=" + mainToken.professionalString() + ", pos=" + getPos().professionalString() + "}";
    }

    const Token &Node::getMainToken() const {
        return mainToken;
    }

    void Node::setMainToken(const Token &mainToken_) {
        Node::mainToken = mainToken_;
    }

    NodeType Node::getType() const {
        return type;
    }

    void Node::setType(NodeType type_) {
        type = type_;
    }

    NodeType ExpressionNode::getRealType() const {
        return getType();
    }

    Pos Node::getPos() const {
        return mainToken.getPos();
    }

    std::string Node::getPosStr() const
    {
        return getPos().toString();
    }

    std::string Node::formatString(size_t indent, size_t level) const {
        return spaceString(indent * level) + "Node={\n"
        + spaceString(indent * (level + 1)) + "mainToken={\n" + mainToken.formatString(indent, level + 2) + "\n"
        + spaceString(indent * (level + 1)) + "},\n"
        + spaceString(indent * (level + 1)) + "nodeType=" + getNodeTypeName(type) + ",\n"
        + spaceString(indent * (level + 1)) + "pos={\n" + getPos().formatString(indent, level + 2) + "\n"
        + spaceString(indent * (level + 1)) + "},\n"
        + spaceString(indent * level) + "}";
    }

    [[maybe_unused]] LiteralNode::LiteralNode(const core::Token& token, NodeType type_)
    : ExpressionNode(token, type_), token(token) {}

    utils::Pos LiteralNode::getPos() const {
        return token.getPos();
    }

    void LiteralNode::acceptVisitor(Visitor &visitor) {
        visitor.visitLiteralNode(*this);
    }

    std::string LiteralNode::professionalString() const {
        return "LiteralNode{literalValue=\"" + utils::StringManager::escape(literalString()) + "\", " + getPos().professionalString() + "}";
    }

    std::string LiteralNode::formatString(size_t indent, size_t level) const {
        return spaceString(indent * level) + "LiteralNode={\n"
        + spaceString(indent * (level + 1)) + "literal=\"" + StringManager::escape(literalString()) + "\",\n"
        + spaceString(indent * (level + 1)) + "mainToken={\n" + token.formatString(indent, level + 2) + "\n"
        + spaceString(indent * (level + 1)) + "}\n"
        + spaceString(indent * level) + "}";
    }

    // VariableDefinitionNode::VariableDefinitionNode(
    //         Token keywordToken, std::shared_ptr<ExpressionNode> variableAssignment)
    //         : ExpressionNode(keywordToken, NodeType::VAR),
    //         varToken(std::move(keywordToken)),
    //         assignExpressionNode(std::move(variableAssignment)){}
    //
    // Pos VariableDefinitionNode::getPos() const {
    //     return varToken.getPos();
    // }
    //
    // void VariableDefinitionNode::acceptVisitor(Visitor &visitor) {
    //     visitor.visitVariableDefinitionNode(*this);
    // }
    //
    // Token VariableDefinitionNode::getVarToken() const {
    //     return varToken;
    // }
    //
    // std::shared_ptr<ExpressionNode> VariableDefinitionNode::getVariableAssignmentNode() const {
    //     return assignExpressionNode;
    // }

    ParameterNode::ParameterNode(
            Token nameToken, std::shared_ptr<Token> colonToken,
            std::vector<std::shared_ptr<Token>> labelTokens, std::shared_ptr<Token> defaultValue)
            : Node(), nameToken(std::move(nameToken)), colonToken(std::move(colonToken)),
              labelTokens(std::move(labelTokens)), defaultValue(std::move(defaultValue)){}

    void ParameterNode::acceptVisitor(Visitor &visitor) {
        visitor.visitParameterNode(*this);
    }

    Pos ParameterNode::getPos() const {
        return nameToken.getPos();
    }

    FunctionDeclarationNode::FunctionDeclarationNode(
        const Token &mainToken_,
        const std::shared_ptr<ExpressionNode> &callNode,
        const std::shared_ptr<Token> &colonToken_,
        const std::vector<std::shared_ptr<LabelNode>> &labelNodes_)
            : ExpressionNode(mainToken_, NodeType::PREFIX), callNode(callNode),
             colonToken(colonToken_), labelNodes(labelNodes_) {}

    std::shared_ptr<ExpressionNode> FunctionDeclarationNode::getCallNode() const {
        return callNode;
    }

    std::vector<std::shared_ptr<LabelNode>> FunctionDeclarationNode::getLabelNodes() const {
        return labelNodes;
    }

    Token FunctionDeclarationNode::getColonToken() const {
        return *colonToken;
    }

    Pos FunctionDeclarationNode::getPos() const {
        return getMainToken().getPos();
    }

    void FunctionDeclarationNode::acceptVisitor(Visitor &visitor) {
        visitor.visitFunctionDeclarationNode(*this);
    }

    FunctionDefinitionNode::FunctionDefinitionNode(const Token &mainToken_,
        const std::shared_ptr<ExpressionNode> &callNode,
        const std::shared_ptr<Token> &colonToken_,
        const std::vector<std::shared_ptr<LabelNode>> &labelNodes_,
        const std::shared_ptr<Token> &indicatorToken_,
        const std::shared_ptr<ExpressionNode> &bodyNode)
            : ExpressionNode(mainToken_, NodeType::PREFIX),
             callNode(callNode), colonToken(colonToken_),
    labelNodes(labelNodes_), indicatorToken(indicatorToken_), bodyNode(bodyNode){}

    Token FunctionDefinitionNode::getColonToken() const {
        return *colonToken;
    }

    std::shared_ptr<Token> FunctionDefinitionNode::getIndicatorToken() const {
        return indicatorToken;
    }

    AnonFunctionDefinitionNode::AnonFunctionDefinitionNode(const Token &mainToken_,
        const std::shared_ptr<ExpressionNode> &paramNode, const std::shared_ptr<Token> &colonToken_,
        const std::vector<std::shared_ptr<LabelNode>> &labelNodes_, Token indicatorToken_,
        const std::shared_ptr<ExpressionNode> &bodyNode)
            : ExpressionNode(mainToken_, NodeType::ANON_FUNCTION_DEFINITION),
             paramNode(paramNode), colonToken(colonToken_),
     labelNodes(labelNodes_), indicatorToken(std::move(indicatorToken_)), bodyNode(bodyNode){}

    std::shared_ptr<ExpressionNode> AnonFunctionDefinitionNode::getParamNode() const {
        return paramNode;
    }

    std::shared_ptr<ExpressionNode> AnonFunctionDefinitionNode::getBodyNode() const {
        return bodyNode;
    }

    std::vector<std::shared_ptr<LabelNode>> AnonFunctionDefinitionNode::getLabelNodes() const {
        return labelNodes;
    }

    Token AnonFunctionDefinitionNode::getIndicatorToken() const {
        return indicatorToken;
    }

    std::shared_ptr<Token> AnonFunctionDefinitionNode::getColonToken() const {
        return colonToken;
    }

    void AnonFunctionDefinitionNode::acceptVisitor(Visitor &visitor) {
        visitor.visitAnonFunctionDefinitionNode(*this);
    }

    void FunctionDefinitionNode::acceptVisitor(Visitor &visitor) {
        visitor.visitFunctionDefinitionNode(*this);
    }

    std::shared_ptr<ExpressionNode> FunctionDefinitionNode::getCallNode() const {
        return callNode;
    }

    std::vector<std::shared_ptr<LabelNode>> FunctionDefinitionNode::getLabelNodes() const {
        return labelNodes;
    }

    std::shared_ptr<ExpressionNode> FunctionDefinitionNode::getBodyNode() const {
        return bodyNode;
    }

    ArgumentNode::ArgumentNode(std::shared_ptr<Node> argNode)
            : argNode(std::move(argNode)) {}

    Pos ArgumentNode::getPos() const {
        return argNode->getPos();
    }

    void ArgumentNode::acceptVisitor(Visitor &visitor) {
        visitor.visitArgumentNode(*this);
    }

    FunctionCallNode::FunctionCallNode(const Token &mainToken_, const std::shared_ptr<ExpressionNode> &calledFuncNode,
        const std::shared_ptr<ExpressionNode> &argsNode)
            : InfixExpressionNode(calledFuncNode->getMainToken(), NodeType::CALL, mainToken_, calledFuncNode, argsNode){}

    Pos FunctionCallNode::getPos() const {
        return InfixExpressionNode::getPos();
    }

    void FunctionCallNode::acceptVisitor(Visitor &visitor) {
        visitor.visitFunctionCallNode(*this);
    }

    ReturnExpressionNode::ReturnExpressionNode(const Token &mainToken_,
        const std::shared_ptr<ExpressionNode> &returnNode)
            : ExpressionNode(mainToken_, NodeType::PREFIX),
     returnNode(returnNode){}

    std::shared_ptr<ExpressionNode> ReturnExpressionNode::getReturnNode() const {
        return returnNode;
    }

    void ReturnExpressionNode::acceptVisitor(Visitor &visitor) {
        visitor.visitReturnExpressionNode(*this);
    }

    BreakExpressionNode::BreakExpressionNode(const Token &mainToken_)
        : ExpressionNode(mainToken_, NodeType::PREFIX) {}

    void BreakExpressionNode::acceptVisitor(Visitor &visitor) {
        visitor.visitBreakExpressionNode(*this);
    }

    BranchNode::BranchNode(const Token &branchOpToken, const std::shared_ptr<ExpressionNode> &conditionNode,
                           const std::shared_ptr<ExpressionNode> &bodyNode)
            : ExpressionNode(branchOpToken, NodeType::BRANCH),
    conditionNode(conditionNode), bodyNode(bodyNode) {}

    std::shared_ptr<ExpressionNode> BranchNode::getConditionNode() const {
        return conditionNode;
    }

    std::shared_ptr<ExpressionNode> BranchNode::getBodyNode() const {
        return bodyNode;
    }

    void BranchNode::acceptVisitor(Visitor &visitor) {
        visitor.visitBranchNode(*this);
    }

    ConditionNode::ConditionNode(const std::vector<std::shared_ptr<ExpressionNode>> &branchNodes)
        : ExpressionNode(branchNodes[0]->getMainToken(), NodeType::CONDITION), branchNodes(branchNodes) {}

    std::vector<std::shared_ptr<ExpressionNode>> ConditionNode::getBranchNodes() const {
        return branchNodes;
    }

    void ConditionNode::acceptVisitor(Visitor &visitor) {
        visitor.visitConditionNode(*this);
    }

    static std::unordered_map<std::string, LoopType> loopTypeMap = {
        {"for", LoopType::FOR},
        {"until", LoopType::UNTIL},
        {"while", LoopType::WHILE}
    };

    LoopNode::LoopNode(const Token& loopToken, const std::shared_ptr<ExpressionNode> &conditionNode,
        const std::shared_ptr<ExpressionNode> &bodyNode)
            : ExpressionNode(loopToken, NodeType::LOOP),
     conditionNode(conditionNode), bodyNode(bodyNode)
    {
        if (const auto &it = loopTypeMap.find(loopToken.getValue());
            it != loopTypeMap.end())
        {
            loopType = it->second;
        } else
        {
            throw std::runtime_error("Invalid loop type");
        }
    }

    std::shared_ptr<ExpressionNode> LoopNode::getConditionNode() const {
        return conditionNode;
    }

    std::shared_ptr<ExpressionNode> LoopNode::getBodyNode() const {
        return bodyNode;
    }

    void LoopNode::acceptVisitor(Visitor &visitor) {
        visitor.visitLoopNode(*this);
    }

    LoopType LoopNode::getLoopType() const
    {
        return loopType;
    }

    WhileLoopNode::WhileLoopNode(const Token &whileToken, const std::shared_ptr<ExpressionNode> &conditionNode,
                                 const std::shared_ptr<ExpressionNode> &bodyNode)
            : LoopNode(whileToken, conditionNode, bodyNode) {}

    UntilLoopNode::UntilLoopNode(const Token &untilToken, const std::shared_ptr<ExpressionNode> &conditionNode,
        const std::shared_ptr<ExpressionNode> &bodyNode)
            : LoopNode(untilToken, conditionNode, bodyNode) {}

    ForLoopNode::ForLoopNode(const Token &forToken, const std::shared_ptr<ExpressionNode> &initNode,
        const std::shared_ptr<ExpressionNode> &conditionNode, const std::shared_ptr<ExpressionNode> &updateNode,
        const std::shared_ptr<ExpressionNode> &bodyNode)
            : LoopNode(forToken, conditionNode, bodyNode),
     initNode(initNode), updateNode(updateNode) {}

    std::shared_ptr<ExpressionNode> ForLoopNode::getInitNode() const {
        return initNode;
    }

    std::shared_ptr<ExpressionNode> ForLoopNode::getUpdateNode() const {
        return updateNode;
    }

    void ForLoopNode::acceptVisitor(Visitor &visitor) {
        visitor.visitForLoopNode(*this);
    }

    PassExpressionNode::PassExpressionNode(const Token &passToken)
        : ExpressionNode(passToken, NodeType::PASS){}

    void PassExpressionNode::acceptVisitor(Visitor &visitor) {
        visitor.visitPassExpressionNode(*this);
    }

    EncapsulatedExpressionNode::EncapsulatedExpressionNode(const Token &mainToken_)
        : ExpressionNode(mainToken_, NodeType::PREFIX){}

    void EncapsulatedExpressionNode::acceptVisitor(Visitor &visitor) {
        visitor.visitEncapsulatedExpressionNode(*this);
    }

    ConstructorDefinitionNode::ConstructorDefinitionNode(const Token &ctorToken,
        const std::shared_ptr<ExpressionNode> &paramNode,
        const std::shared_ptr<Token> &colonToken,
        const std::vector<std::shared_ptr<LabelNode>> &labelNodes,
        const std::shared_ptr<ExpressionNode> &bodyNode)
            : ExpressionNode(ctorToken, NodeType::PREFIX),
    paramNode(paramNode), colonToken(colonToken), labelNodes(labelNodes),
    bodyNode(bodyNode){}

    void ConstructorDefinitionNode::acceptVisitor(Visitor &visitor) {
        visitor.visitConstructorDefinitionNode(*this);
    }

    std::shared_ptr<Token> ConstructorDefinitionNode::getColonToken() const {
        return colonToken;
    }

    std::vector<std::shared_ptr<LabelNode>> ConstructorDefinitionNode::getLabelNodes() const {
        return labelNodes;
    }

    std::shared_ptr<ExpressionNode> ConstructorDefinitionNode::getParamNode() const {
        return paramNode;
    }

    std::shared_ptr<ExpressionNode> ConstructorDefinitionNode::getBodyNode() const {
        return bodyNode;
    }

    ClassDeclarationNode::ClassDeclarationNode(const Token &classToken,
        const std::shared_ptr<ExpressionNode> &nameNode_)
            : ExpressionNode(classToken, NodeType::PREFIX),
               nameNode(nameNode_) {}

    std::shared_ptr<ExpressionNode> ClassDeclarationNode::getNameNode() const {
        return nameNode;
    }

    void ClassDeclarationNode::acceptVisitor(Visitor &visitor) {
        visitor.visitClassDeclarationNode(*this);
    }


    ClassDefinitionNode::ClassDefinitionNode(const Token &classToken,
                                             const std::shared_ptr<ExpressionNode> &nameNode_,
                                             const std::shared_ptr<ExpressionNode> &bodyNode_)
            : ExpressionNode(classToken, NodeType::PREFIX),
     nameNode(nameNode_), bodyNode(bodyNode_){}

    std::vector<std::shared_ptr<LabelNode>> ClassDefinitionNode::getLabelNode() const {
        return std::static_pointer_cast<IdentifierNode>(nameNode)->getLabels();
    }

    std::shared_ptr<ExpressionNode> ClassDefinitionNode::getBodyNode() const {
        return bodyNode;
    }

    std::shared_ptr<ExpressionNode> ClassDefinitionNode::getNameNode() const {
        return nameNode;
    }

    Pos ClassDefinitionNode::getPos() const {
        return getMainToken().getPos();
    }

    void ClassDefinitionNode::acceptVisitor(Visitor &visitor) {
        visitor.visitClassDefinitionNode(*this);
    }

    ProgramNode::ProgramNode(const Token &mainToken, std::vector<std::shared_ptr<StatementNode>> statements)
            : StatementNode(mainToken, NodeType::PROGRAM), statements(std::move(statements)){}

    void ProgramNode::acceptVisitor(Visitor &visitor) {
        visitor.visitProgramNode(*this);
    }

    Pos ProgramNode::getPos() const {
        if (statements.empty()) {
            return Node::getPos();
        }
        return statements[0]->getPos();
    }

    const std::vector<std::shared_ptr<StatementNode>> &ProgramNode::getStatements() const {
        return statements;
    }

    NodeType ProgramNode::getRealType() const {
        return NodeType::PROGRAM;
    }

    TryNode::TryNode(const Token &mainToken,
            const std::shared_ptr<BlockRangerNode> &tryBody,
            const std::vector<std::pair<std::shared_ptr<IdentifierNode>,
            std::shared_ptr<BlockRangerNode>>> &catchBodies,
            const std::shared_ptr<BlockRangerNode> &finallyBody)
            : ExpressionNode(mainToken, NodeType::TRY),
    tryBody(tryBody), catchBodies(catchBodies), finallyBody(finallyBody) {}

    void TryNode::acceptVisitor(Visitor& visitor)
    {
        visitor.visitTryNode(*this);
    }

    NodeType TryNode::getRealType() const
    {
        return NodeType::TRY;
    }

    std::shared_ptr<BlockRangerNode> TryNode::getTryBody() const
    {
        return tryBody;
    }

    std::vector<std::pair<std::shared_ptr<IdentifierNode>, std::shared_ptr<BlockRangerNode>>> TryNode::
    getCatchBodies() const
    {
        return catchBodies;
    }

    std::shared_ptr<BlockRangerNode> TryNode::getFinallyBody() const
    {
        return finallyBody;
    }

    ThrowNode::ThrowNode(const Token& mainToken, const std::shared_ptr<ExpressionNode>& throwExpression)
        : ExpressionNode(mainToken, NodeType::THROW), throwExpression(throwExpression) {}

    std::shared_ptr<ExpressionNode> ThrowNode::getThrowExpression() const
    {
        return throwExpression;
    }

    void ThrowNode::acceptVisitor(Visitor& visitor)
    {
        visitor.visitThrowNode(*this);
    }

    StringLiteralNode::StringLiteralNode(const Token& token)
    : LiteralNode(token, NodeType::STRING),
    value(StringManager::unescape(token.getValue())) {}

    Pos StringLiteralNode::getPos() const {
        return LiteralNode::getPos();
    }

    void StringLiteralNode::acceptVisitor(Visitor &visitor) {
        visitor.visitStringLiteralNode(*this);
    }

    std::string StringLiteralNode::literalString() const {
        return value;
    }

    [[maybe_unused]] NumberLiteralNode::NumberLiteralNode(const core::Token& token, NodeType type_)
    : LiteralNode(token, type_) {}

    Pos NumberLiteralNode::getPos() const {
        return LiteralNode::getPos();
    }

    void NumberLiteralNode::acceptVisitor(Visitor &visitor) {
        visitor.visitNumberLiteralNode(*this);
    }

    IntegerLiteralNode::IntegerLiteralNode(const core::Token& token)
    : NumberLiteralNode(token, NodeType::INTEGER),
    value(stringToInt(LiteralNode::token.getValue())) {}

    Pos IntegerLiteralNode::getPos() const {
        return NumberLiteralNode::getPos();
    }

    void IntegerLiteralNode::acceptVisitor(Visitor &visitor) {
        visitor.visitIntegerLiteralNode(*this);
    }

    std::string IntegerLiteralNode::literalString() const {
        return std::to_string(value);
    }

    FloatLiteralNode::FloatLiteralNode(const core::Token& token)
    : NumberLiteralNode(token, NodeType::FLOAT),
    value(stringToDouble(LiteralNode::token.getValue())){}

    Pos FloatLiteralNode::getPos() const {
        return NumberLiteralNode::getPos();
    }

    void FloatLiteralNode::acceptVisitor(Visitor &visitor) {
        visitor.visitFloatLiteralNode(*this);
    }

    std::string FloatLiteralNode::literalString() const {
        return doubleToString(value);
    }

    BooleanLiteralNode::BooleanLiteralNode(const core::Token& token)
    : LiteralNode(token, NodeType::BOOL),
    value(stringToBool(LiteralNode::token.getValue())){}

    Pos BooleanLiteralNode::getPos() const {
        return LiteralNode::getPos();
    }

    void BooleanLiteralNode::acceptVisitor(Visitor &visitor) {
        visitor.visitBooleanLiteralNode(*this);
    }

    std::string BooleanLiteralNode::literalString() const {
        return value ? "true" : "false";
    }

    CharacterLiteralNode::CharacterLiteralNode(const Token& token)
    : LiteralNode(token, NodeType::CHAR),
    value(stringToChar(LiteralNode::token.getValue())) {}

    Pos CharacterLiteralNode::getPos() const {
        return LiteralNode::getPos();
    }

    void CharacterLiteralNode::acceptVisitor(Visitor &visitor) {
        visitor.visitCharacterLiteralNode(*this);
    }

    std::string CharacterLiteralNode::literalString() const {
        return std::string{value};
    }

    NullLiteralNode::NullLiteralNode(const Token &token)
        : LiteralNode(token, NodeType::NULL_NODE){}

    Pos NullLiteralNode::getPos() const {
        return LiteralNode::getPos();
    }

    void NullLiteralNode::acceptVisitor(Visitor &visitor) {
        visitor.visitNullLiteralNode(*this);
    }

    std::string NullLiteralNode::literalString() const {
        return "null";
    }

    IdentifierNode::IdentifierNode(const Token &token)
        : ExpressionNode(token, NodeType::IDENTIFIER), name(token.getValue()),
         labels({}){}

    IdentifierNode::IdentifierNode(const Token &token, const Token &colonToken_,
                                   const std::vector<std::shared_ptr<LabelNode>> &labels_)
        : ExpressionNode(token, NodeType::IDENTIFIER), name(token.getValue()),
    colonToken(std::make_shared<Token>(colonToken_)), labels(labels_){}

    void IdentifierNode::acceptVisitor(Visitor &visitor) {
        visitor.visitIdentifierNode(*this);
    }

    Pos IdentifierNode::getPos() const {
        return ExpressionNode::getPos();
    }

    std::string IdentifierNode::getName() const {
        return name;
    }

    Token IdentifierNode::getColonToken() const {
        return *colonToken;
    }

    std::shared_ptr<Token> IdentifierNode::getColonTokenPtr() const {
        return colonToken;
    }

    std::vector<std::shared_ptr<LabelNode>> IdentifierNode::getLabels() const {
        return labels;
    }

    LabelNode::LabelNode(const Token &token)
        : ExpressionNode(token, NodeType::LABEL), label(token.getValue()) {}

    void LabelNode::appendLabelDesNode(const std::shared_ptr<ListExpressionNode>& labelDesNode)
    {
        labelDesNodes.push_back(labelDesNode);
    }

    void LabelNode::acceptVisitor(Visitor &visitor) {
        visitor.visitLabelNode(*this);
    }

    std::string LabelNode::getLabel() const {
        return label;
    }

    std::vector<std::shared_ptr<ListExpressionNode>> LabelNode::getLabelDesNodes() const
    {
        return labelDesNodes;
    }

    RangerNode::RangerNode(const NodeType rangerType_,
                           Token lToken, Token rToken)
            : ExpressionNode(NodeType::RANGER),
            rangerType(rangerType_),
            rStartToken(std::move(lToken)),
            rEndToken(std::move(rToken)){}

    Token RangerNode::getRangerStartToken() const {
        return rStartToken;
    }

    Token RangerNode::getRangerEndToken() const {
        return rEndToken;
    }

    NodeType RangerNode::getRangerType() const {
        return rangerType;
    }

    void RangerNode::acceptVisitor(Visitor &visitor) {

    }

    ParenRangerNode::ParenRangerNode(const Token &lParenToken, const Token &rParenToken,
                                     const std::shared_ptr<ExpressionNode> &node)
            : RangerNode(NodeType::PAREN, lParenToken, rParenToken), rangerNode(node){}

    void ParenRangerNode::acceptVisitor(Visitor &visitor) {
        visitor.visitParenRangerNode(*this);
    }

    std::shared_ptr<ExpressionNode> ParenRangerNode::getRangerNode() const {
        return rangerNode;
    }

    BlockRangerNode::BlockRangerNode(const Token &lParenToken, const Token &rParenToken,
        const std::vector<std::shared_ptr<ExpressionNode>> &bodyExpressions)
            : RangerNode(NodeType::BLOCK, lParenToken, rParenToken), bodyExpressions(bodyExpressions){}

    void BlockRangerNode::acceptVisitor(Visitor &visitor) {
        visitor.visitBlockRangerNode(*this);
    }

    std::vector<std::shared_ptr<ExpressionNode>> BlockRangerNode::getBodyExpressions() const {
        return bodyExpressions;
    }

    Pos BlockRangerNode::getPos() const
    {
        return getRangerStartToken().getPos();
    }

    BracketExpressionNode::BracketExpressionNode(
        const Token &lRangerToken, const Token &rRangerToken,
        const std::shared_ptr<ExpressionNode> &bodyNode)
            : RangerNode(NodeType::INDEX_EXPRESSION, lRangerToken, rRangerToken),
            bodyNode(bodyNode){}

    std::shared_ptr<ExpressionNode> BracketExpressionNode::getBodyNode() const {
        return bodyNode;
    }

    void BracketExpressionNode::acceptVisitor(Visitor &visitor) {
        visitor.visitBracketExpressionNode(*this);
    }

    Pos BracketExpressionNode::getPos() const
    {
        return getRangerStartToken().getPos();
    }

    IndexExpressionNode::IndexExpressionNode(const Token &token_,
                                             const std::shared_ptr<ExpressionNode> &leftNode,
                                             const std::shared_ptr<ExpressionNode> &indexNode_)
            : PostfixExpressionNode(token_,
                NodeType::INDEX_EXPRESSION, leftNode),
             indexNode(indexNode_) {}

    std::shared_ptr<ExpressionNode> IndexExpressionNode::getIndexNode() const {
        return indexNode;
    }

    void IndexExpressionNode::acceptVisitor(Visitor &visitor) {
        visitor.visitIndexExpressionNode(*this);
    }

    std::shared_ptr<ExpressionNode> IndexExpressionNode::getLeftNode() const {
        return getNode();
    }

    NodeType IndexExpressionNode::getRealType() const
    {
        return NodeType::INDEX_EXPRESSION;
    }

    PairExpressionNode::PairExpressionNode(const Token &mainToken_, const std::shared_ptr<ExpressionNode> &leftNode,
                                           Token colonToken, const std::shared_ptr<ExpressionNode> &rightNode)
            : ExpressionNode(mainToken_, NodeType::PAIR), leftNode(leftNode), colonToken(std::move(colonToken)),
     rightNode(rightNode){}

    void PairExpressionNode::acceptVisitor(Visitor &visitor) {
        visitor.visitPairExpressionNode(*this);
    }

    std::shared_ptr<ExpressionNode> PairExpressionNode::getLeftNode() const {
        return leftNode;
    }

    Token PairExpressionNode::getColonToken() const {
        return colonToken;
    }

    std::shared_ptr<ExpressionNode> PairExpressionNode::getRightNode() const {
        return rightNode;
    }

    DictionaryExpressionNode::DictionaryExpressionNode(const Token &lToken,
        const Token &rToken, const std::shared_ptr<ExpressionNode> &pairNodes)
            : RangerNode(NodeType::BLOCK, lToken, rToken), bodyNode(pairNodes){}

    std::shared_ptr<ExpressionNode>
    DictionaryExpressionNode::getBodyNode() const {
        return bodyNode;
    }

    void DictionaryExpressionNode::acceptVisitor(Visitor &visitor) {
        visitor.visitDictionaryExpressionNode(*this);
    }

    ListExpressionNode::ListExpressionNode(const Token &lToken, const Token &rToken,
        const std::shared_ptr<ExpressionNode> &bodyNode)
            : RangerNode(NodeType::BLOCK, lToken, rToken), bodyNode(bodyNode){}

    std::shared_ptr<ExpressionNode> ListExpressionNode::getBodyNode() const {
        return bodyNode;
    }

    void ListExpressionNode::acceptVisitor(Visitor &visitor) {
        visitor.visitListExpressionNode(*this);
    }

    AssignmentNode::AssignmentNode(const Token &opToken,
        const std::pair<std::shared_ptr<ExpressionNode>, std::shared_ptr<ExpressionNode>> &assignPair)
            : ExpressionNode(opToken, NodeType::ASSIGNMENT), assignPair(assignPair) {}

    std::pair<std::shared_ptr<ExpressionNode>, std::shared_ptr<ExpressionNode>> AssignmentNode::
    getAssignPair() const {
        return assignPair;
    }

    void AssignmentNode::acceptVisitor(Visitor &visitor) {
        visitor.visitAssignmentNode(*this);
    }

    VariableDefinitionNode::VarDefData::VarDefData(const std::shared_ptr<IdentifierNode> &nameNode,
        const bool &hasLabels, const std::vector<std::shared_ptr<LabelNode>> &labelNodes, const bool &hasInitialValue,
        const std::shared_ptr<ExpressionNode> &valueNode)
            : nameNode(nameNode), hasLabels_(hasLabels),
        labelNodes(labelNodes), hasInitialValue_(hasInitialValue),
        valueNode(valueNode) {}

    std::shared_ptr<IdentifierNode> VariableDefinitionNode::VarDefData::getNameNode() const {
        return nameNode;
    }

    bool VariableDefinitionNode::VarDefData::hasLabels() const {
        return hasLabels_;
    }

    std::vector<std::shared_ptr<LabelNode>> VariableDefinitionNode::VarDefData::getLabelNodes() const {
        return labelNodes;
    }

    bool VariableDefinitionNode::VarDefData::hasInitialValue() const {
        return hasInitialValue_;
    }

    std::shared_ptr<ExpressionNode> VariableDefinitionNode::VarDefData::getValueNode() const {
        return valueNode;
    }

    VariableDefinitionNode::VariableDefinitionNode(const Token &varToken,
                                                   const std::vector<std::shared_ptr<VarDefData>> &varDefs)
            : ExpressionNode(varToken, NodeType::VAR), varDefs(varDefs) {}

    std::vector<std::shared_ptr<VariableDefinitionNode::VarDefData>> VariableDefinitionNode::getVarDefs() const {
        return varDefs;
    }

    void VariableDefinitionNode::acceptVisitor(Visitor &visitor) {
        visitor.visitVariableDefinitionNode(*this);
    }

    const Token &InfixExpressionNode::getOpToken() const {
        return opToken;
    }

    void InfixExpressionNode::setOpToken(const Token &opToken_) {
        opToken = opToken_;
    }

    const std::shared_ptr<ExpressionNode> &InfixExpressionNode::getLeftNode() const {
        return leftNode;
    }

    void InfixExpressionNode::setLeftNode(const std::shared_ptr<ExpressionNode> &leftNode_) {
        leftNode = leftNode_;
    }

    const std::shared_ptr<ExpressionNode> &InfixExpressionNode::getRightNode() const {
        return rightNode;
    }

    NodeType InfixExpressionNode::getInfixType() const {
        return infixType;
    }

    NodeType InfixExpressionNode::getRealType() const {
        return getInfixType();
    }

    void InfixExpressionNode::setRightNode(const std::shared_ptr<ExpressionNode> &rightNode_) {
        rightNode = rightNode_;
    }

    Token UnaryExpressionNode::getOpToken() const {
        return opToken;
    }

    std::shared_ptr<ExpressionNode> UnaryExpressionNode::getRightNode() const {
        return rightNode;
    }

    std::string UnaryExpressionNode::toString() const {
        return ExpressionNode::toString();
    }

    std::string UnaryExpressionNode::briefString() const {
        return ExpressionNode::briefString();
    }

    std::string UnaryExpressionNode::professionalString() const {
        return ExpressionNode::professionalString();
    }

    PostfixExpressionNode::PostfixExpressionNode(const Token& opToken, const NodeType postfixType,
        const std::shared_ptr<ExpressionNode> &leftNode)
            : ExpressionNode(opToken, NodeType::POSTFIX),
             postfixType(postfixType), node(leftNode){}

    std::shared_ptr<ExpressionNode> PostfixExpressionNode::getNode() const {
        return node;
    }

    void PostfixExpressionNode::acceptVisitor(Visitor &visitor) {
        visitor.visitPostfixNode(*this);
    }

    NodeType PostfixExpressionNode::getPostfixType() const
    {
        return postfixType;
    }

    void UnaryExpressionNode::setOpToken(const Token &op_token) {
        opToken = op_token;
    }

    void UnaryExpressionNode::setRightNode(const std::shared_ptr<ExpressionNode> &right_node) {
        rightNode = right_node;
    }

    void UnaryExpressionNode::acceptVisitor(Visitor &visitor) {
        visitor.visitUnaryExpressionNode(*this);
    }

    UnaryExpressionNode::UnaryExpressionNode(const Token &mainToken, Token opSign,
        const std::shared_ptr<ExpressionNode> &rightNode)
            : ExpressionNode(mainToken, NodeType::UNARY),
             opToken(std::move(opSign)), rightNode(rightNode){}

    std::string UnaryExpressionNode::formatString(const size_t indent, const size_t level) const {
        return ExpressionNode::formatString(indent, level);
    }

    PrefixExpressionNode::PrefixExpressionNode(const Token &token, const NodeType nodeType, const std::shared_ptr<ExpressionNode> &node_)
        : ExpressionNode(token, NodeType::PREFIX), prefixType(nodeType), node(node_){}

    void PrefixExpressionNode::acceptVisitor(Visitor &visitor) {
        visitor.visitPrefixExpressionNode(*this);
    }

    std::shared_ptr<ExpressionNode> PrefixExpressionNode::getNode() const {
        return node;
    }

    NodeType PrefixExpressionNode::getPrefixType() const {
        return prefixType;
    }

    InfixExpressionNode::InfixExpressionNode(
        const Token &mainToken, const NodeType nodeType,
        Token opToken,
        const std::shared_ptr<ExpressionNode> &left,
        const std::shared_ptr<ExpressionNode> &right)
    : ExpressionNode(mainToken, NodeType::INFIX), infixType(nodeType),
    opToken(std::move(opToken)), leftNode(left), rightNode(right){}

    Pos InfixExpressionNode::getPos() const {
        return getMainToken().getPos();
    }

    void InfixExpressionNode::acceptVisitor(Visitor &visitor) {
        visitor.visitInfixNode(*this);
    }

    std::string InfixExpressionNode::toString() const {
        return "[InfixNode: (mainToken: " + getMainToken().toString() +
        ") <Operator: \"" + opToken.getValue() + "\"> [LeftNode: " +
        leftNode->toString()+ "] [RightNode: " +
        rightNode->toString() + "]]";
    }

    std::string InfixExpressionNode::briefString() const {
        return "[InfixNode: (" + getMainToken().briefString() + ") <\"" +
        opToken.getValue() + "\"> [L: " + leftNode->briefString() + "] [R: " + rightNode->briefString() + "]]";
    }

    std::string InfixExpressionNode::professionalString() const {
        return "InfixNode{mainToken=" + getMainToken().professionalString() + ", opSign=\"" +
        opToken.getValue() + "\", leftNode=" + leftNode->professionalString() + ", rightNode=" +
        rightNode->professionalString() + "}";
    }

    std::string InfixExpressionNode::formatString(size_t indent, size_t level) const {
        return spaceString(indent * level) + "InfixNode={\n"
        + spaceString(indent * (level + 1)) + "mainToken={\n" + getMainToken().formatString(indent, level + 2) + "\n"
        + spaceString(indent * (level + 1)) + "},\n"
        + spaceString(indent * (level + 1)) + "opSign=\"" + opToken.getValue() + "\",\n"
        + spaceString(indent * (level + 1)) + "leftNode={\n" + leftNode->formatString(indent, level + 2) + "\n"
        + spaceString(indent * (level + 1)) + "},\n"
        + spaceString(indent * (level + 1)) + "rightNode={\n" + rightNode->formatString(indent, level + 2) + "\n"
        + spaceString(indent * (level + 1)) + "}\n"
        + spaceString(indent * level) + "}";
    }


    ExpressionStatementNode::ExpressionStatementNode(const Token &token, const std::shared_ptr<ExpressionNode> &expression)
    : StatementNode(token, NodeType::EXPRESSION_STATEMENT), expression(expression) {}

    ExpressionStatementNode::ExpressionStatementNode(): StatementNode(NodeType::EXPRESSION_STATEMENT){}

    void ExpressionStatementNode::acceptVisitor(Visitor &visitor) {
        visitor.visitExpressionStatementNode(*this);
    }

    std::shared_ptr<ExpressionNode> ExpressionStatementNode::getExpression() const {
        return expression;
    }

    NodeType ExpressionStatementNode::getRealType() const {
        return NodeType::EXPRESSION_STATEMENT;
    }

    void PrefixExpressionNode::setNode(const std::shared_ptr<ExpressionNode> &node_) {
        node = node_;
    }
}

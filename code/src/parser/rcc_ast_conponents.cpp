//
// Created by RestRegular on 2025/6/29.
//

#include "../../include/rcc_base.h"
#include "../../include/parser/rcc_ast_components.h"

namespace ast {
    std::string AstNode::toString() const {
        return "[AstNode: " + getPos().toString() + "]";
    }

    std::string AstNode::briefString() const {
        return "[AstNode: " + getPos().briefString() + "]";
    }

    std::string AstNode::professionalString() const {
        return "[AstNode: " + getPos().professionalString() + "]";
    }

    LiteralNode::LiteralNode(core::Token token): AstNode(), token(std::move(token)) {}

    utils::Pos LiteralNode::getPos() const {
        return token.getPos();
    }

    void LiteralNode::acceptVisitor(AstVisitor &visitor) {
        visitor.visitLiteralNode(*this);
    }

    std::string LiteralNode::professionalString() const {
        return "LiteralNode{literalValue=\"" + utils::StringManager::escape(literalString()) + "\", " + getPos().professionalString() + "}";
    }

    VariableDefinitionNode::VariableDefinitionNode(
            VariableDefinitionNode::Token keywordToken, VariableDefinitionNode::VariableDeclarationData declarationData)
            : AstNode(), varToken(std::move(keywordToken)), declarationData(std::move(declarationData)){}

    utils::Pos VariableDefinitionNode::getPos() const {
        return varToken.getPos();
    }

    void VariableDefinitionNode::acceptVisitor(AstVisitor &visitor) {
        visitor.visitVariableDeclarationNode(*this);
    }

    VariableAssignmentNode::VariableAssignmentNode(
            std::map<std::unique_ptr<Token>, std::unique_ptr<Token>> assignments)
            : AstNode(), assignments(std::move(assignments)){}

    utils::Pos VariableAssignmentNode::getPos() const {
        if (!assignments.empty()) {
            return assignments.begin()->first->getPos();
        } else {
            return {0, 1, 0, RCC_UNKNOWN_CONST};
        }
    }

    void VariableAssignmentNode::acceptVisitor(AstVisitor &visitor) {
        visitor.visitVariableAssignmentNode(*this);
    }

    ParameterNode::ParameterNode(
            Token nameToken, std::unique_ptr<Token> colonToken,
            std::vector<std::unique_ptr<Token>> labelTokens, std::unique_ptr<Token> defaultValue)
            : AstNode(), nameToken(std::move(nameToken)), colonToken(std::move(colonToken)),
              labelTokens(std::move(labelTokens)), defaultValue(std::move(defaultValue)){}

    void ParameterNode::acceptVisitor(AstVisitor &visitor) {
        visitor.visitParameterNode(*this);
    }

    utils::Pos ParameterNode::getPos() const {
        return nameToken.getPos();
    }

    FunctionDeclarationNode::FunctionDeclarationNode(
            FunctionDeclarationNode::Token funToken,
            FunctionDeclarationNode::Token nameToken,
            std::vector<std::unique_ptr<ProgramNode>> parameterNodes,
            std::vector<std::unique_ptr<Token>> labelTokens)
            : AstNode(), funToken(std::move(funToken)), nameToken(std::move(nameToken)),
              paramNodes(std::move(parameterNodes)), labelTokens(std::move(labelTokens)) {}

    utils::Pos FunctionDeclarationNode::getPos() const {
        return funToken.getPos();
    }

    void FunctionDeclarationNode::acceptVisitor(AstVisitor &visitor) {
        visitor.visitFunctionDeclarationNode(*this);
    }

    ArgumentNode::ArgumentNode(std::unique_ptr<AstNode> argNode)
            : AstNode(), argNode(std::move(argNode)) {}

    utils::Pos ArgumentNode::getPos() const {
        return argNode->getPos();
    }

    void ArgumentNode::acceptVisitor(AstVisitor &visitor) {
        visitor.visitArgumentNode(*this);
    }

    FunctionCallNode::FunctionCallNode(
            FunctionCallNode::Token nameToken,
            std::vector<std::unique_ptr<ArgumentNode>> argNodes)
            : AstNode(), nameToken(std::move(nameToken)), argNodes(std::move(argNodes)){}

    utils::Pos FunctionCallNode::getPos() const {
        return nameToken.getPos();
    }

    void FunctionCallNode::acceptVisitor(AstVisitor &visitor) {
        visitor.visitFunctionCallNode(*this);
    }

    FunctionDefinitionNode::FunctionDefinitionNode(
            FunctionDefinitionNode::Token funToken,
            FunctionDefinitionNode::Token nameToken,
            std::vector<std::unique_ptr<ProgramNode>> parameterNodes,
            std::vector<std::unique_ptr<Token>> labelTokens,
            std::unique_ptr<AstNode> bodyNode)
            : FunctionDeclarationNode(
            std::move(funToken), std::move(nameToken),
            std::move(parameterNodes), std::move(labelTokens)),
              bodyNode(std::move(bodyNode)){}

    ConstructorDefinitionNode::ConstructorDefinitionNode(
            ConstructorDefinitionNode::Token ctorToken,
            std::vector<std::unique_ptr<ParameterNode>> paramNodes,
            std::unique_ptr<AstNode> bodyNode)
            : AstNode(), ctorToken(std::move(ctorToken)), paramNodes(std::move(paramNodes)),
              bodyNode(std::move(bodyNode)){}

    utils::Pos ConstructorDefinitionNode::getPos() const {
        return ctorToken.getPos();
    }

    void ConstructorDefinitionNode::acceptVisitor(AstVisitor &visitor) {
        visitor.visitConstructorDefinitionNode(*this);
    }

    ClassDeclarationNode::ClassDeclarationNode(
            ClassDeclarationNode::Token classToken, Token nameToken,
            std::unique_ptr<Token> colonToken,
            std::vector<std::unique_ptr<Token>> labelTokens)
            : AstNode(), classToken(std::move(classToken)), nameToken(std::move(nameToken)),
              colonToken(std::move(colonToken)), labelTokens(std::move(labelTokens)){}

    void ClassDeclarationNode::acceptVisitor(AstVisitor &visitor) {
        visitor.visitClassDeclarationNode(*this);
    }

    utils::Pos ClassDeclarationNode::getPos() const {
        return classToken.getPos();
    }

    ClassDefinitionNode::ClassDefinitionNode(
            Token classToken_,
            Token nameToken_, std::unique_ptr<Token> colonToken_,
            std::vector<std::unique_ptr<Token>> labelTokens_,
            std::vector<std::unique_ptr<ConstructorDefinitionNode>> ctorNodes,
            std::vector<std::unique_ptr<VariableDefinitionNode>> fieldNodes,
            std::vector<std::unique_ptr<FunctionDeclarationNode>> methodDeclarationNodes,
            std::vector<std::unique_ptr<FunctionDefinitionNode>> methodDefinitionNodes)
            : ClassDeclarationNode(std::move(classToken_),std::move(nameToken_),
                                   std::move(colonToken_),std::move(labelTokens_)),
              ctorNodes(std::move(ctorNodes)), fieldNodes(std::move(fieldNodes)),
              methodDeclarationNodes(std::move(methodDeclarationNodes)),
              methodDefinitionNodes(std::move(methodDefinitionNodes)){}

    utils::Pos ClassDefinitionNode::getPos() const {
        return ClassDeclarationNode::getPos();
    }

    void ClassDefinitionNode::acceptVisitor(AstVisitor &visitor) {
        visitor.visitClassDefinitionNode(*this);
    }

    ProgramNode::ProgramNode(std::vector<std::unique_ptr<AstNode>> statements)
            : statements(std::move(statements)){}

    void ProgramNode::acceptVisitor(AstVisitor &visitor) {
        visitor.visitProgramNode(*this);
    }

    utils::Pos ProgramNode::getPos() const {
        if (statements.empty()) {
            return {0, 0, 0, RCC_UNKNOWN_CONST};
        } else {
            return statements[0]->getPos();
        }
    }

    StringLiteralNode::StringLiteralNode(core::Token token)
    : LiteralNode(std::move(token)), value(LiteralNode::token.getValue()) {}

    utils::Pos StringLiteralNode::getPos() const {
        return LiteralNode::getPos();
    }

    void StringLiteralNode::acceptVisitor(AstVisitor &visitor) {
        visitor.visitStringLiteralNode(*this);
    }

    std::string StringLiteralNode::literalString() const {
        return value;
    }

    NumberLiteralNode::NumberLiteralNode(core::Token token)
    : LiteralNode(std::move(token)) {}

    utils::Pos NumberLiteralNode::getPos() const {
        return LiteralNode::getPos();
    }

    void NumberLiteralNode::acceptVisitor(AstVisitor &visitor) {
        LiteralNode::acceptVisitor(visitor);
    }

    IntegerLiteralNode::IntegerLiteralNode(core::Token token)
    : NumberLiteralNode(std::move(token)),
    value(utils::stringToInt(LiteralNode::token.getValue())) {}

    utils::Pos IntegerLiteralNode::getPos() const {
        return NumberLiteralNode::getPos();
    }

    void IntegerLiteralNode::acceptVisitor(AstVisitor &visitor) {
        visitor.visitIntegerLiteralNode(*this);
    }

    std::string IntegerLiteralNode::literalString() const {
        return std::to_string(value);
    }

    FloatLiteralNode::FloatLiteralNode(core::Token token)
    : NumberLiteralNode(std::move(token)),
    value(utils::stringToDouble(LiteralNode::token.getValue())){}

    utils::Pos FloatLiteralNode::getPos() const {
        return NumberLiteralNode::getPos();
    }

    void FloatLiteralNode::acceptVisitor(AstVisitor &visitor) {
        visitor.visitFloatLiteralNode(*this);
    }

    std::string FloatLiteralNode::literalString() const {
        return std::to_string(value);
    }

    BooleanLiteralNode::BooleanLiteralNode(core::Token token)
    : LiteralNode(std::move(token)),
    value(utils::stringToBool(LiteralNode::token.getValue())){}

    utils::Pos BooleanLiteralNode::getPos() const {
        return LiteralNode::getPos();
    }

    void BooleanLiteralNode::acceptVisitor(AstVisitor &visitor) {
        visitor.visitBooleanLiteralNode(*this);
    }

    std::string BooleanLiteralNode::literalString() const {
        return value ? "true" : "false";
    }

    CharacterLiteralNode::CharacterLiteralNode(core::Token token)
    : LiteralNode(std::move(token)),
    value(utils::stringToChar(LiteralNode::token.getValue())) {}

    utils::Pos CharacterLiteralNode::getPos() const {
        return LiteralNode::getPos();
    }

    void CharacterLiteralNode::acceptVisitor(AstVisitor &visitor) {
        visitor.visitCharacterLiteralNode(*this);
    }

    std::string CharacterLiteralNode::literalString() const {
        return std::to_string(value);
    }
}
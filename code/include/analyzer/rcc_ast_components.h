//
// Created by RestRegular on 2025/6/29.
//

#ifndef RCC_RCC_AST_COMPONENTS_H
#define RCC_RCC_AST_COMPONENTS_H

#include <utility>

#include "../rcc_base.h"
#include "./rcc_ast.h"
#include "visitors/rcc_visitor.h"

namespace ast {
    using namespace utils;
    using namespace base;
    using namespace core;

    // 自定义类型

    // 枚举类声明

    // 类声明
    // --- 节点类 ---

    /* ToDo: 更多 Node 类声明 */

    // 类定义

    class Node : public utils::Object {
        Token mainToken;
        NodeType type;
    public:
        explicit Node() : mainToken(), type(NodeType::UNDEFINED) {}

        explicit Node(NodeType type) : type(type) {}

        Node(Token token, NodeType type): mainToken(std::move(token)), type(type) {}

        ~Node() override = default;

        [[nodiscard]] virtual utils::Pos getPos() const;

        virtual void acceptVisitor(Visitor &visitor) = 0;

        [[nodiscard]] std::string toString() const override;

        [[nodiscard]] std::string briefString() const override;

        [[nodiscard]] std::string professionalString() const override;

        [[nodiscard]] std::string formatString(size_t indent, size_t level) const override;

        [[nodiscard]] const Token &getMainToken() const;

        void setMainToken(const Token &mainToken_);

        [[nodiscard]] NodeType getType() const;

        [[nodiscard]] virtual NodeType getRealType() const = 0;

        void setType(NodeType type_);
    };

    class ExpressionNode : public Node {
    public:
        ExpressionNode() {}
        explicit ExpressionNode(NodeType type) : Node(type) {}
        explicit ExpressionNode(const Token& token, NodeType type): Node(token, type) {}
        [[nodiscard]] NodeType getRealType() const override;
    };

    class StatementNode : public Node {
    public:
        StatementNode() {}
        explicit StatementNode(NodeType type) : Node(type) {}
        StatementNode(Token mainToken, NodeType type): Node(std::move(mainToken), type) {};
    };

    class ExpressionStatementNode: public StatementNode {
        std::shared_ptr<ExpressionNode> expression;
    public:
        ExpressionStatementNode();
        ExpressionStatementNode(const Token &token, const std::shared_ptr<ExpressionNode> &expression);
        void acceptVisitor(Visitor &visitor) override;
        [[nodiscard]] std::shared_ptr<ExpressionNode> getExpression() const;
        [[nodiscard]] NodeType getRealType() const override;
    };

    class PrefixExpressionNode : public ExpressionNode {
        NodeType prefixType{};
        std::shared_ptr<ExpressionNode> node;
    protected:
        void setNode(const std::shared_ptr<ExpressionNode> &node_);
    public:
        PrefixExpressionNode() {}
        explicit PrefixExpressionNode(const Token &token, NodeType nodeType, const std::shared_ptr<ExpressionNode> &node_);
        void acceptVisitor(Visitor &visitor) override;
        std::shared_ptr<ExpressionNode> getNode() const;
        [[nodiscard]] NodeType getPrefixType() const;
    };

    class InfixExpressionNode : public ExpressionNode {
        NodeType infixType{};
        Token opToken;
        std::shared_ptr<ExpressionNode> leftNode;
        std::shared_ptr<ExpressionNode> rightNode;
    protected:
        void setOpToken(const Token &opToken_);
        void setLeftNode(const std::shared_ptr<ExpressionNode> &leftNode_);
        void setRightNode(const std::shared_ptr<ExpressionNode> &rightNode_);
    public:
        InfixExpressionNode(){}
        InfixExpressionNode(const Token &mainToken, NodeType nodeType,
                            const Token &opToken,
                            const std::shared_ptr<ExpressionNode> &left,
                            const std::shared_ptr<ExpressionNode> &right);
        [[nodiscard]] Pos getPos() const override;
        void acceptVisitor(Visitor &visitor) override;
        std::string toString() const override;
        std::string briefString() const override;
        std::string professionalString() const override;
        [[nodiscard]] std::string formatString(size_t indent, size_t level) const override;
        [[nodiscard]] const Token &getOpToken() const;
        [[nodiscard]] const std::shared_ptr<ExpressionNode> &getLeftNode() const;
        [[nodiscard]] const std::shared_ptr<ExpressionNode> &getRightNode() const;
        [[nodiscard]] NodeType getInfixType() const;
        [[nodiscard]] NodeType getRealType() const override;
    };

    class PostfixExpressionNode : public ExpressionNode {
        NodeType postfixType{};
        std::shared_ptr<ExpressionNode> node;
    public:
        PostfixExpressionNode(Token opToken,
            NodeType postfixType,
            const std::shared_ptr<ExpressionNode> &leftNode);
        [[nodiscard]] std::shared_ptr<ExpressionNode> getNode() const;
        void acceptVisitor(Visitor &visitor) override;
    };

    class UnaryExpressionNode : public ExpressionNode {
        Token opToken;
        std::shared_ptr<ExpressionNode> rightNode;
    protected:
        void setOpToken(const Token &op_token);
        void setRightNode(const std::shared_ptr<ExpressionNode> &right_node);
    public:
        void acceptVisitor(Visitor &visitor) override;
        UnaryExpressionNode(){}
        UnaryExpressionNode(const Token &mainToken, const Token &opSign,
            const std::shared_ptr<ExpressionNode> &rightNode);
        [[nodiscard]] std::string formatString(size_t indent, size_t level) const override;
        [[nodiscard]] Token getOpToken() const;
        [[nodiscard]] std::shared_ptr<ExpressionNode> getRightNode() const;
        [[nodiscard]] std::string toString() const override;
        [[nodiscard]] std::string briefString() const override;
        [[nodiscard]] std::string professionalString() const override;
    };

    class LiteralNode : public ExpressionNode {
    protected:
        Token token;
    public:
        [[maybe_unused]] explicit LiteralNode(const core::Token& token, NodeType type);

        [[nodiscard]] Pos getPos() const override;

        void acceptVisitor(Visitor &visitor) override;

        [[nodiscard]] virtual std::string literalString() const = 0;

        [[nodiscard]] std::string professionalString() const override;

        [[nodiscard]] std::string formatString(size_t indent, size_t level) const override;
    };

    class StringLiteralNode : public LiteralNode {
        std::string value;
    public:
        explicit StringLiteralNode(const core::Token& token);

        [[nodiscard]] Pos getPos() const override;

        void acceptVisitor(Visitor &visitor) override;

        [[nodiscard]] std::string literalString() const override;
    };

    class NumberLiteralNode : public LiteralNode {
    public:
        [[maybe_unused]] explicit NumberLiteralNode(const core::Token& token, NodeType type_);

        [[nodiscard]] Pos getPos() const override;

        void acceptVisitor(Visitor &visitor) override;
    };

    class IntegerLiteralNode : public NumberLiteralNode {
        int value;
    public:
        explicit IntegerLiteralNode(const core::Token& token);

        [[nodiscard]] utils::Pos getPos() const override;

        void acceptVisitor(Visitor &visitor) override;

        [[nodiscard]] std::string literalString() const override;
    };

    class FloatLiteralNode : public NumberLiteralNode {
        double value;
    public:
        explicit FloatLiteralNode(const core::Token& token);

        [[nodiscard]] utils::Pos getPos() const override;

        void acceptVisitor(Visitor &visitor) override;

        [[nodiscard]] std::string literalString() const override;
    };

    class BooleanLiteralNode : public LiteralNode {
        bool value;
    public:
        explicit BooleanLiteralNode(const core::Token& token);

        [[nodiscard]] utils::Pos getPos() const override;

        void acceptVisitor(Visitor &visitor) override;

        [[nodiscard]] std::string literalString() const override;
    };

    class CharacterLiteralNode : public LiteralNode {
        char value;
    public:
        explicit CharacterLiteralNode(const core::Token& token);

        [[nodiscard]] utils::Pos getPos() const override;

        void acceptVisitor(Visitor &visitor) override;

        [[nodiscard]] std::string literalString() const override;
    };

    class NullLiteralNode : public LiteralNode {
    public:
        explicit NullLiteralNode(const Token& token);
        [[nodiscard]] Pos getPos() const override;
        void acceptVisitor(Visitor &visitor) override;
        [[nodiscard]] std::string literalString() const override;
    };

    class IdentifierNode : public ExpressionNode {
        std::string name;
        std::shared_ptr<Token> colonToken;
        std::vector<std::shared_ptr<LabelNode>> labels;
    public:
        explicit IdentifierNode(const Token& token);
        explicit IdentifierNode(const Token& token, const Token &colonToken_,
            const std::vector<std::shared_ptr<LabelNode>> &labels_);
        void acceptVisitor(Visitor &visitor) override;
        [[nodiscard]] Pos getPos() const override;
        [[nodiscard]] std::string getName() const;
        [[nodiscard]] Token getColonToken() const;
        [[nodiscard]] std::shared_ptr<Token> getColonTokenPtr() const;
        [[nodiscard]] std::vector<std::shared_ptr<LabelNode>> getLabels() const;
    };

    class LabelNode: public ExpressionNode {
        std::string label;
    public:
        explicit LabelNode(const Token& token);
        void acceptVisitor(Visitor &visitor) override;
        [[nodiscard]] std::string getLabel() const;
    };

    class RangerNode : public ExpressionNode {
        NodeType rangerType;
        Token rStartToken;
        Token rEndToken;
    public:
        RangerNode(NodeType rangerType_, const Token &lToken, const Token &rToken);
        [[nodiscard]] Token getRangerStartToken() const;
        [[nodiscard]] Token getRangerEndToken() const;
        [[nodiscard]] NodeType getRangerType() const;
        void acceptVisitor(Visitor &visitor) override;
    };

    class ParenRangerNode: public RangerNode {
        std::shared_ptr<ExpressionNode> rangerNode;
    public:
        ParenRangerNode(const Token &lParenToken, const Token &rParenToken,
            const std::shared_ptr<ExpressionNode> &node);
        void acceptVisitor(Visitor &visitor) override;
        [[nodiscard]] std::shared_ptr<ExpressionNode> getRangerNode() const;
    };

    class BlockRangerNode: public RangerNode {
        std::vector<std::shared_ptr<ExpressionNode>> bodyExpressions;
    public:
        BlockRangerNode(const Token &lParenToken, const Token &rParenToken,
            const std::vector<std::shared_ptr<ExpressionNode>> &bodyExpressions);
        void acceptVisitor(Visitor &visitor) override;
        [[nodiscard]] std::vector<std::shared_ptr<ExpressionNode>> getBodyExpressions() const;
    };

    class BracketExpressionNode : public RangerNode {
        std::shared_ptr<ExpressionNode> bodyNode;
    public:
        BracketExpressionNode(
            const Token &lRangerToken,
            const Token &rRangerToken,
            const std::shared_ptr<ExpressionNode> &bodyNode);
        [[nodiscard]] std::shared_ptr<ExpressionNode> getBodyNode() const;
        void acceptVisitor(Visitor &visitor) override;
    };

    class IndexExpressionNode: public PostfixExpressionNode {
        std::shared_ptr<ExpressionNode> indexNode;
    public:
        IndexExpressionNode(
            const Token &token_,
            const std::shared_ptr<ExpressionNode> &leftNode,
            const std::shared_ptr<ExpressionNode> &indexNode_);
        [[nodiscard]] std::shared_ptr<ExpressionNode> getIndexNode() const;
        void acceptVisitor(Visitor &visitor) override;
        [[nodiscard]] std::shared_ptr<ExpressionNode> getLeftNode() const;
    };

    class PairExpressionNode : public ExpressionNode {
        std::shared_ptr<ExpressionNode> leftNode;
        Token colonToken;
        std::shared_ptr<ExpressionNode> rightNode;
    public:
        PairExpressionNode(const Token& mainToken_,
            const std::shared_ptr<ExpressionNode> &leftNode,
            const Token &colonToken,
            const std::shared_ptr<ExpressionNode> &rightNode);
        void acceptVisitor(Visitor &visitor) override;
        [[nodiscard]] std::shared_ptr<ExpressionNode> getLeftNode() const;
        [[nodiscard]] Token getColonToken() const;
        [[nodiscard]] std::shared_ptr<ExpressionNode> getRightNode() const;
    };

    class DictionaryExpressionNode : public RangerNode {
        std::shared_ptr<ExpressionNode> bodyNode;
    public:
        DictionaryExpressionNode(
            const Token &lToken,
            const Token &rToken,
            const std::shared_ptr<ExpressionNode> &bodyNode);
        [[nodiscard]] std::shared_ptr<ExpressionNode> getBodyNode() const;
        void acceptVisitor(Visitor &visitor) override;
    };

    class ListExpressionNode : public RangerNode {
        std::shared_ptr<ExpressionNode> bodyNode;
    public:
        ListExpressionNode(
        const Token &lToken,
        const Token &rToken,
        const std::shared_ptr<ExpressionNode> &bodyNode);
        [[nodiscard]] std::shared_ptr<ExpressionNode> getBodyNode() const;
        void acceptVisitor(Visitor &visitor) override;
    };

    class ParameterNode : public Node {
        typedef Token Token;
        Token nameToken;
        std::shared_ptr<Token> colonToken;
        std::vector<std::shared_ptr<Token>> labelTokens;
        std::shared_ptr<Token> defaultValue;
    public:
        explicit ParameterNode(Token nameToken,
                               std::shared_ptr<Token> colonToken,
                               std::vector<std::shared_ptr<Token>> labelTokens,
                               std::shared_ptr<Token> defaultValue);

        void acceptVisitor(Visitor &visitor) override;

        [[nodiscard]] Pos getPos() const override;
    };

    class FunctionDeclarationNode : public ExpressionNode {
        std::shared_ptr<ExpressionNode> callNode;
        std::shared_ptr<Token> colonToken;
        std::vector<std::shared_ptr<LabelNode>> labelNodes;
    public:
        explicit FunctionDeclarationNode(const Token &mainToken_,
            const std::shared_ptr<ExpressionNode> &callNode,
            const std::shared_ptr<Token> &colonToken_,
            const std::vector<std::shared_ptr<LabelNode>> &labelNodes_);
        [[nodiscard]] Pos getPos() const override;
        void acceptVisitor(Visitor &visitor) override;
        [[nodiscard]] std::shared_ptr<ExpressionNode> getCallNode() const;
        [[nodiscard]] std::vector<std::shared_ptr<LabelNode>> getLabelNodes() const;
        [[nodiscard]] Token getColonToken() const;
    };

    class FunctionDefinitionNode : public ExpressionNode {
        std::shared_ptr<ExpressionNode> callNode;
        std::shared_ptr<Token> colonToken;
        std::vector<std::shared_ptr<LabelNode>> labelNodes;
        std::shared_ptr<Token> indicatorToken;
        std::shared_ptr<ExpressionNode> bodyNode;
    public:
        explicit FunctionDefinitionNode(const Token &mainToken_,
            const std::shared_ptr<ExpressionNode> &callNode,
            const std::shared_ptr<Token> &colonToken_,
            const std::vector<std::shared_ptr<LabelNode>> &labelNodes_,
            const std::shared_ptr<Token> &indicatorToken_,
            const std::shared_ptr<ExpressionNode> &bodyNode);
        void acceptVisitor(Visitor &visitor) override;
        [[nodiscard]] std::shared_ptr<ExpressionNode> getCallNode() const;
        [[nodiscard]] std::vector<std::shared_ptr<LabelNode>> getLabelNodes() const;
        [[nodiscard]] std::shared_ptr<ExpressionNode> getBodyNode() const;
        [[nodiscard]] Token getColonToken() const;
        [[nodiscard]] std::shared_ptr<Token> getIndicatorToken() const;
    };

    class AnonFunctionDefinitionNode : public ExpressionNode {
        std::shared_ptr<ExpressionNode> paramNode;
        std::shared_ptr<Token> colonToken;
        std::vector<std::shared_ptr<LabelNode>> labelNodes;
        Token indicatorToken;
        std::shared_ptr<ExpressionNode> bodyNode;
    public:
        AnonFunctionDefinitionNode(const Token &mainToken_,
            const std::shared_ptr<ExpressionNode> &paramNode,
            const std::shared_ptr<Token> &colonToken_,
            const std::vector<std::shared_ptr<LabelNode>> &labelNodes_,
            const Token &indicatorToken_,
            const std::shared_ptr<ExpressionNode> &bodyNode);
        [[nodiscard]] std::shared_ptr<ExpressionNode> getParamNode() const;
        [[nodiscard]] std::shared_ptr<ExpressionNode> getBodyNode() const;
        [[nodiscard]] std::vector<std::shared_ptr<LabelNode>> getLabelNodes() const;
        [[nodiscard]] Token getIndicatorToken() const;
        [[nodiscard]] std::shared_ptr<Token> getColonToken() const;
        void acceptVisitor(Visitor &visitor) override;
    };

    class ArgumentNode : public Node {
        typedef core::Token Token;
        std::shared_ptr<Node> argNode;
    public:
        explicit ArgumentNode(std::shared_ptr<Node> argNode);

        [[nodiscard]] utils::Pos getPos() const override;

        void acceptVisitor(Visitor &visitor) override;
    };

    class FunctionCallNode : public InfixExpressionNode {
    public:
        explicit FunctionCallNode(const Token &mainToken_,
            const std::shared_ptr<ExpressionNode> &calledFuncNode,
            const std::shared_ptr<ExpressionNode> &argsNode);

        [[nodiscard]] Pos getPos() const override;

        void acceptVisitor(Visitor &visitor) override;
    };

    class ReturnExpressionNode : public ExpressionNode {
        std::shared_ptr<ExpressionNode> returnNode;
    public:
        ReturnExpressionNode(const Token &mainToken_,
            const std::shared_ptr<ExpressionNode> &returnNode);
        [[nodiscard]] std::shared_ptr<ExpressionNode> getReturnNode() const;
        void acceptVisitor(Visitor &visitor) override;
    };

    class BreakExpressionNode : public ExpressionNode {
    public:
        explicit BreakExpressionNode(const Token &mainToken_);
        void acceptVisitor(Visitor &visitor) override;
    };

    class BranchNode : public ExpressionNode {
        std::shared_ptr<ExpressionNode> conditionNode;
        std::shared_ptr<ExpressionNode> bodyNode;
    public:
        explicit BranchNode(
            const Token &branchOpToken,
            const std::shared_ptr<ExpressionNode> &conditionNode,
            const std::shared_ptr<ExpressionNode> &bodyNode);
        [[nodiscard]] std::shared_ptr<ExpressionNode> getConditionNode() const;
        [[nodiscard]] std::shared_ptr<ExpressionNode> getBodyNode() const;
        void acceptVisitor(Visitor &visitor) override;
    };

    class ConditionNode : public ExpressionNode {
        std::vector<std::shared_ptr<ExpressionNode>> branchNodes;
    public:
        explicit ConditionNode(const std::vector<std::shared_ptr<ExpressionNode>> &branchNodes);
        [[nodiscard]] std::vector<std::shared_ptr<ExpressionNode>> getBranchNodes() const;
        void acceptVisitor(Visitor &visitor) override;
    };

    class LoopNode : public ExpressionNode {
        std::shared_ptr<ExpressionNode> conditionNode;
        std::shared_ptr<ExpressionNode> bodyNode;
    public:
        explicit LoopNode(Token loopToken,
            const std::shared_ptr<ExpressionNode> &conditionNode,
            const std::shared_ptr<ExpressionNode> &bodyNode);
        [[nodiscard]] std::shared_ptr<ExpressionNode> getConditionNode() const;
        [[nodiscard]] std::shared_ptr<ExpressionNode> getBodyNode() const;
        void acceptVisitor(Visitor &visitor) override;
    };

    class WhileLoopNode : public LoopNode {
    public:
        explicit WhileLoopNode(const Token &whileToken,
            const std::shared_ptr<ExpressionNode> &conditionNode,
            const std::shared_ptr<ExpressionNode> &bodyNode);
    };

    class UntilLoopNode : public LoopNode {
    public:
        explicit UntilLoopNode(const Token &untilToken,
            const std::shared_ptr<ExpressionNode> &conditionNode,
            const std::shared_ptr<ExpressionNode> &bodyNode);
    };

    class ForLoopNode : public LoopNode {
        std::shared_ptr<ExpressionNode> initNode;
        std::shared_ptr<ExpressionNode> updateNode;
    public:
        ForLoopNode(const Token &forToken,
            const std::shared_ptr<ExpressionNode> &initNode,
            const std::shared_ptr<ExpressionNode> &conditionNode,
            const std::shared_ptr<ExpressionNode> &updateNode,
            const std::shared_ptr<ExpressionNode> &bodyNode);
        [[nodiscard]] std::shared_ptr<ExpressionNode> getInitNode() const;
        [[nodiscard]] std::shared_ptr<ExpressionNode> getUpdateNode() const;
        void acceptVisitor(Visitor &visitor) override;
    };

    class PassExpressionNode : public ExpressionNode {
    public:
        PassExpressionNode(const Token &passToken);
        void acceptVisitor(Visitor &visitor) override;
    };

    class EncapsulatedExpressionNode: public ExpressionNode {
    public:
        EncapsulatedExpressionNode(const Token &mainToken_);
        void acceptVisitor(Visitor &visitor) override;
    };

    class ConstructorDefinitionNode : public ExpressionNode {
        std::shared_ptr<ExpressionNode> paramNode;
        std::shared_ptr<Token> colonToken;
        std::vector<std::shared_ptr<LabelNode>> labelNodes;
        std::shared_ptr<ExpressionNode> bodyNode;
    public:
        explicit ConstructorDefinitionNode(const Token &ctorToken,
            const std::shared_ptr<ExpressionNode> &paramNode,
            const std::shared_ptr<Token> &colonToken,
            const std::vector<std::shared_ptr<LabelNode>> &labelNodes,
            const std::shared_ptr<ExpressionNode> &bodyNode);
        void acceptVisitor(Visitor &visitor) override;
        [[nodiscard]] std::shared_ptr<Token> getColonToken() const;
        [[nodiscard]] std::vector<std::shared_ptr<LabelNode>> getLabelNodes() const;
        [[nodiscard]] std::shared_ptr<ExpressionNode> getParamNode() const;
        [[nodiscard]] std::shared_ptr<ExpressionNode> getBodyNode() const;
    };

    class ClassDeclarationNode : public Node {
        typedef Token Token;
        Token classToken;
        Token nameToken;
        std::shared_ptr<Token> colonToken;
        std::vector<std::shared_ptr<Token>> labelTokens;
    public:
        explicit ClassDeclarationNode(Token classToken, Token nameToken, std::shared_ptr<Token> colonToken,
                                      std::vector<std::shared_ptr<Token>> labelTokens);

        void acceptVisitor(Visitor &visitor) override;

        [[nodiscard]] Pos getPos() const override;
    };

    class ClassDefinitionNode : public ExpressionNode {
        std::shared_ptr<ExpressionNode> nameNode;
        std::shared_ptr<ExpressionNode> bodyNode;
    public:
        explicit ClassDefinitionNode(
                const Token &classToken,
                const std::shared_ptr<ExpressionNode> &labelNode,
                const std::shared_ptr<ExpressionNode> &bodyNode);
        [[nodiscard]] std::shared_ptr<ExpressionNode> getLabelNode() const;
        [[nodiscard]] std::shared_ptr<ExpressionNode> getBodyNode() const;
        [[nodiscard]] std::shared_ptr<ExpressionNode> getNameNode() const;
        [[nodiscard]] Pos getPos() const override;
        void acceptVisitor(Visitor &visitor) override;
    };

    class ProgramNode : public StatementNode {
        std::vector<std::shared_ptr<StatementNode>> statements;
    public:
        explicit ProgramNode(const Token &mainToken, std::vector<std::shared_ptr<StatementNode>> statements);

        void acceptVisitor(Visitor &visitor) override;

        [[nodiscard]] utils::Pos getPos() const override;

        [[nodiscard]] const std::vector<std::shared_ptr<StatementNode>> &getStatements() const;

        [[nodiscard]] NodeType getRealType() const override;
    };
}


#endif //RCC_RCC_AST_COMPONENTS_H

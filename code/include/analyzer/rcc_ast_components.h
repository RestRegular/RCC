//
// Created by RestRegular on 2025/6/29.
//

#ifndef RCC_RCC_AST_COMPONENTS_H
#define RCC_RCC_AST_COMPONENTS_H

#include <utility>
#include "./rcc_ast.h"
#include "../rcc_base.h"
#include "../../include/rcc_core.h"

namespace ast {
    using namespace utils;
    using namespace base;
    using namespace core;

    // IdentifierNode / LabelNode 互相引用，且都早于对方定义出现，
    // 这里需要前置声明（以前是 visitor 头文件顺手带进来的）。
    class LabelNode;
    class ListExpressionNode;

    // 类定义
    class Node : public Object {
        Token mainToken;
        NodeType type;
    public:
        explicit Node() : type(NodeType::UNDEFINED) {}

        explicit Node(const NodeType type) : type(type) {}

        Node(Token token, const NodeType type): mainToken(std::move(token)), type(type) {}

        ~Node() override = default;

        [[nodiscard]] virtual Pos getPos() const;

        [[nodiscard]] std::string getPosStr() const;


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
        ExpressionNode() = default;
        explicit ExpressionNode(NodeType type) : Node(type) {}
        explicit ExpressionNode(const Token& token, const NodeType type): Node(token, type) {}
        [[nodiscard]] NodeType getRealType() const override;
    };

    class StatementNode : public Node {
    public:
        StatementNode() = default;
        explicit StatementNode(const NodeType type) : Node(type) {}
        StatementNode(Token mainToken, const NodeType type): Node(std::move(mainToken), type) {};
    };

    class ExpressionStatementNode final : public StatementNode {
        std::shared_ptr<ExpressionNode> expression;
    public:
        ExpressionStatementNode();
        ExpressionStatementNode(const Token &token, const std::shared_ptr<ExpressionNode> &expression);
        [[nodiscard]] std::shared_ptr<ExpressionNode> getExpression() const;
        [[nodiscard]] NodeType getRealType() const override;
    };

    class PrefixExpressionNode final : public ExpressionNode {
        NodeType prefixType{};
        std::shared_ptr<ExpressionNode> node;
    protected:
        void setNode(const std::shared_ptr<ExpressionNode> &node_);
    public:
        PrefixExpressionNode() = default;
        explicit PrefixExpressionNode(const Token &token, NodeType nodeType, const std::shared_ptr<ExpressionNode> &node_);
        [[nodiscard]] std::shared_ptr<ExpressionNode> getNode() const;
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
        InfixExpressionNode() = default;
        InfixExpressionNode(const Token &mainToken, NodeType nodeType,
                            Token opToken, const std::shared_ptr<ExpressionNode> &left,
                            const std::shared_ptr<ExpressionNode> &right);
        [[nodiscard]] Pos getPos() const override;
        [[nodiscard]] std::string toString() const override;
        [[nodiscard]] std::string briefString() const override;
        [[nodiscard]] std::string professionalString() const override;
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
        PostfixExpressionNode(const Token& opToken,
            NodeType postfixType,
            const std::shared_ptr<ExpressionNode> &leftNode);
        [[nodiscard]] std::shared_ptr<ExpressionNode> getNode() const;
        [[nodiscard]] NodeType getPostfixType() const;
    };

    class UnaryExpressionNode final : public ExpressionNode {
        Token opToken;
        std::shared_ptr<ExpressionNode> rightNode;
    protected:
        void setOpToken(const Token &op_token);
        void setRightNode(const std::shared_ptr<ExpressionNode> &right_node);
    public:
        UnaryExpressionNode() = default;
        UnaryExpressionNode(const Token &mainToken, Token opSign,
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
        [[maybe_unused]] explicit LiteralNode(const Token& token, NodeType type);

        [[nodiscard]] Pos getPos() const override;


        [[nodiscard]] virtual std::string literalString() const = 0;

        [[nodiscard]] std::string professionalString() const override;

        [[nodiscard]] std::string formatString(size_t indent, size_t level) const override;
    };

    class StringLiteralNode final : public LiteralNode {
        std::string value;
    public:
        explicit StringLiteralNode(const Token& token);

        [[nodiscard]] Pos getPos() const override;


        [[nodiscard]] std::string literalString() const override;

        [[nodiscard]] std::string unescapedString() const;
    };

    class NumberLiteralNode : public LiteralNode {
    public:
        [[maybe_unused]] explicit NumberLiteralNode(const core::Token& token, NodeType type_);

        [[nodiscard]] Pos getPos() const override;

    };

    class IntegerLiteralNode final : public NumberLiteralNode {
        // P12 任务 2：Rio 的 int 是 64 位，这里原先用 int 会让 2147483648
        // 以上的字面量在解析期就溢出（词法阶段 stringToInt 报错）
        long long value;
    public:
        explicit IntegerLiteralNode(const Token& token);

        [[nodiscard]] Pos getPos() const override;


        [[nodiscard]] std::string literalString() const override;
    };

    class FloatLiteralNode final : public NumberLiteralNode {
        double value;
    public:
        explicit FloatLiteralNode(const Token& token);

        [[nodiscard]] Pos getPos() const override;


        [[nodiscard]] std::string literalString() const override;
    };

    class BooleanLiteralNode final : public LiteralNode {
        bool value;
    public:
        explicit BooleanLiteralNode(const core::Token& token);

        [[nodiscard]] utils::Pos getPos() const override;


        [[nodiscard]] std::string literalString() const override;
    };

    class CharacterLiteralNode : public LiteralNode {
        char value;
    public:
        explicit CharacterLiteralNode(const core::Token& token);

        [[nodiscard]] utils::Pos getPos() const override;


        [[nodiscard]] std::string literalString() const override;
    };

    class NullLiteralNode final : public LiteralNode {
    public:
        explicit NullLiteralNode(const Token& token);
        [[nodiscard]] Pos getPos() const override;
        [[nodiscard]] std::string literalString() const override;
    };

    class IdentifierNode final : public ExpressionNode {
        std::string name;
        std::shared_ptr<Token> colonToken;
        std::vector<std::shared_ptr<LabelNode>> labels;
    public:
        explicit IdentifierNode(const Token& token);
        explicit IdentifierNode(const Token& token, const Token &colonToken_,
            const std::vector<std::shared_ptr<LabelNode>> &labels_);
        [[nodiscard]] Pos getPos() const override;
        [[nodiscard]] std::string getName() const;
        [[nodiscard]] Token getColonToken() const;
        [[nodiscard]] std::shared_ptr<Token> getColonTokenPtr() const;
        [[nodiscard]] std::vector<std::shared_ptr<LabelNode>> getLabels() const;
    };

    class LabelNode final : public ExpressionNode {
        std::vector<std::string> labelPath;
        std::vector<std::shared_ptr<ListExpressionNode>> labelDesNodes {};
        // 决议 B：类型标签后面跟了 `?`，表示「可空」。
        // `var x: int? = f()` / `fun f(): int?` 都靠这一位表达。
        bool nullable = false;
    public:
        explicit LabelNode(const std::vector<Token> &labelPathToken);
        void appendLabelDesNode(const std::shared_ptr<ListExpressionNode> &labelDesNode);
        [[nodiscard]] std::string getFullLabel() const;
        [[nodiscard]] std::vector<std::string> getLabelPath() const;
        [[nodiscard]] std::vector<std::shared_ptr<ListExpressionNode>> getLabelDesNodes() const;
        void appendLabelPath(const Token& token);
        void setNullable(bool value);
        [[nodiscard]] bool isNullable() const;
    };

    class RangerNode : public ExpressionNode {
        NodeType rangerType;
        Token rStartToken;
        Token rEndToken;
    public:
        RangerNode(NodeType rangerType_, Token lToken, Token rToken);
        [[nodiscard]] Token getRangerStartToken() const;
        [[nodiscard]] Token getRangerEndToken() const;
        [[nodiscard]] NodeType getRangerType() const;
    };

    class ParenRangerNode final : public RangerNode {
        std::shared_ptr<ExpressionNode> rangerNode;
    public:
        ParenRangerNode(const Token &lParenToken, const Token &rParenToken,
            const std::shared_ptr<ExpressionNode> &node);
        [[nodiscard]] std::shared_ptr<ExpressionNode> getRangerNode() const;
    };

    class BlockRangerNode final : public RangerNode {
        std::vector<std::shared_ptr<ExpressionNode>> bodyExpressions;
    public:
        BlockRangerNode(const Token &lParenToken, const Token &rParenToken,
            const std::vector<std::shared_ptr<ExpressionNode>> &bodyExpressions);
        [[nodiscard]] std::vector<std::shared_ptr<ExpressionNode>> getBodyExpressions() const;
        [[nodiscard]] Pos getPos() const override;
    };

    class BracketExpressionNode final : public RangerNode {
        std::shared_ptr<ExpressionNode> bodyNode;
    public:
        BracketExpressionNode(
            const Token &lRangerToken,
            const Token &rRangerToken,
            const std::shared_ptr<ExpressionNode> &bodyNode);
        [[nodiscard]] std::shared_ptr<ExpressionNode> getBodyNode() const;
        [[nodiscard]] Pos getPos() const override;
    };

    class IndexExpressionNode final : public PostfixExpressionNode {
        std::shared_ptr<ExpressionNode> indexNode;
    public:
        IndexExpressionNode(
            const Token &token_,
            const std::shared_ptr<ExpressionNode> &leftNode,
            const std::shared_ptr<ExpressionNode> &indexNode_);
        [[nodiscard]] std::shared_ptr<ExpressionNode> getIndexNode() const;
        [[nodiscard]] std::shared_ptr<ExpressionNode> getLeftNode() const;
        NodeType getRealType() const override;
    };

    class PairExpressionNode : public ExpressionNode {
        std::shared_ptr<ExpressionNode> leftNode;
        Token colonToken;
        std::shared_ptr<ExpressionNode> rightNode;
    public:
        PairExpressionNode(const Token& mainToken_,
            const std::shared_ptr<ExpressionNode> &leftNode,
            Token colonToken,
            const std::shared_ptr<ExpressionNode> &rightNode);
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
    };

    class ListExpressionNode : public RangerNode {
        std::shared_ptr<ExpressionNode> bodyNode;
    public:
        ListExpressionNode(
        const Token &lToken,
        const Token &rToken,
        const std::shared_ptr<ExpressionNode> &bodyNode);
        [[nodiscard]] std::shared_ptr<ExpressionNode> getBodyNode() const;
    };

    class AssignmentNode: public ExpressionNode {
        std::pair<std::shared_ptr<ExpressionNode>, std::shared_ptr<ExpressionNode>> assignPair;
    public:
        AssignmentNode(
            const Token &opToken,
            const std::pair<std::shared_ptr<ExpressionNode>, std::shared_ptr<ExpressionNode>> &assignPair);
        [[nodiscard]] std::pair<std::shared_ptr<ExpressionNode>, std::shared_ptr<ExpressionNode>> getAssignPair() const;
    };

    class VariableDefinitionNode: public ExpressionNode {
    public:
        class VarDefData {
            std::shared_ptr<IdentifierNode> nameNode;
            bool hasLabels_{};
            std::vector<std::shared_ptr<LabelNode>> labelNodes;
            bool hasInitialValue_{};
            std::shared_ptr<ExpressionNode> valueNode;
        public:
            VarDefData(
                const std::shared_ptr<IdentifierNode> &nameNode,
                const bool &hasLabels,
                const std::vector<std::shared_ptr<LabelNode>> &labelNodes,
                const bool &hasInitialValue,
                const std::shared_ptr<ExpressionNode> &valueNode);
            [[nodiscard]] std::shared_ptr<IdentifierNode> getNameNode() const;
            [[nodiscard]] bool hasLabels() const;
            [[nodiscard]] std::vector<std::shared_ptr<LabelNode>> getLabelNodes() const;
            [[nodiscard]] bool hasInitialValue() const;
            [[nodiscard]] std::shared_ptr<ExpressionNode> getValueNode() const;
        };
        VariableDefinitionNode(
            const Token &varToken,
            const std::vector<std::shared_ptr<VarDefData>> &varDefs);
        [[nodiscard]] std::vector<std::shared_ptr<VarDefData>> getVarDefs() const;
    private:
        std::vector<std::shared_ptr<VarDefData>> varDefs;
    };

    class ParameterNode : public Node {
        Token nameToken;
        std::shared_ptr<Token> colonToken;
        std::vector<std::shared_ptr<Token>> labelTokens;
        std::shared_ptr<Token> defaultValue;
    public:
        explicit ParameterNode(Token nameToken,
                               std::shared_ptr<Token> colonToken,
                               std::vector<std::shared_ptr<Token>> labelTokens,
                               std::shared_ptr<Token> defaultValue);


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
        [[nodiscard]] std::shared_ptr<ExpressionNode> getCallNode() const;
        [[nodiscard]] std::vector<std::shared_ptr<LabelNode>> getLabelNodes() const;
        [[nodiscard]] Token getColonToken() const;
    };

    class FunctionDefinitionNode final : public ExpressionNode {
        std::shared_ptr<ExpressionNode> callNode;
        std::shared_ptr<Token> colonToken;
        std::vector<std::shared_ptr<LabelNode>> labelNodes;
        std::shared_ptr<Token> indicatorToken;
        std::shared_ptr<ExpressionNode> bodyNode;
        // P13-2/L3-F（决议 §9）：签名子句 from[...] / writes[...] / ensures[...]。
        // 每个 LabelNode 的 fullLabel 是子句名，labelDesNodes[0]->getBodyNode()
        // 是参数表达式列表。空 = 未写子句。
        std::vector<std::shared_ptr<LabelNode>> signatureClauses;
    public:
        explicit FunctionDefinitionNode(const Token &mainToken_,
            const std::shared_ptr<ExpressionNode> &callNode,
            const std::shared_ptr<Token> &colonToken_,
            const std::vector<std::shared_ptr<LabelNode>> &labelNodes_,
            const std::shared_ptr<Token> &indicatorToken_,
            const std::shared_ptr<ExpressionNode> &bodyNode,
            const std::vector<std::shared_ptr<LabelNode>> &signatureClauses_ = {});
        [[nodiscard]] std::shared_ptr<ExpressionNode> getCallNode() const;
        [[nodiscard]] std::vector<std::shared_ptr<LabelNode>> getLabelNodes() const;
        [[nodiscard]] std::shared_ptr<ExpressionNode> getBodyNode() const;
        [[nodiscard]] Token getColonToken() const;
        [[nodiscard]] std::shared_ptr<Token> getIndicatorToken() const;
        [[nodiscard]] const std::vector<std::shared_ptr<LabelNode>>& getSignatureClauses() const;
        void setSignatureClauses(const std::vector<std::shared_ptr<LabelNode>>& clauses);
    };

    class AnonFunctionDefinitionNode final : public ExpressionNode {
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
            Token indicatorToken_,
            const std::shared_ptr<ExpressionNode> &bodyNode);
        [[nodiscard]] std::shared_ptr<ExpressionNode> getParamNode() const;
        [[nodiscard]] std::shared_ptr<ExpressionNode> getBodyNode() const;
        [[nodiscard]] std::vector<std::shared_ptr<LabelNode>> getLabelNodes() const;
        [[nodiscard]] Token getIndicatorToken() const;
        [[nodiscard]] std::shared_ptr<Token> getColonToken() const;
    };

    class ArgumentNode : public Node {
        std::shared_ptr<Node> argNode;
    public:
        explicit ArgumentNode(std::shared_ptr<Node> argNode);

        [[nodiscard]] utils::Pos getPos() const override;

    };

    class FunctionCallNode : public InfixExpressionNode {
    public:
        explicit FunctionCallNode(const Token &mainToken_,
            const std::shared_ptr<ExpressionNode> &calledFuncNode,
            const std::shared_ptr<ExpressionNode> &argsNode);

        [[nodiscard]] Pos getPos() const override;

    };

    class ReturnExpressionNode : public ExpressionNode {
        std::shared_ptr<ExpressionNode> returnNode;
        // P13-2/L1：`return` 只退函数（true）；`ret` 退最近可返回值作用域（false）。
        bool _exitsFunction{true};
    public:
        ReturnExpressionNode(const Token &mainToken_,
            const std::shared_ptr<ExpressionNode> &returnNode,
            bool exitsFunction_ = true);
        [[nodiscard]] std::shared_ptr<ExpressionNode> getReturnNode() const;
        [[nodiscard]] bool exitsFunction() const;
    };

    // P13-2/L3（决议 §7.2）：`&bob` 壳子取引用，产出 Ref[T]。
    // 只接受 Body 变量（IdentifierNode）；子引用 &p.name 暂不支持。
    class RefExpressionNode final : public ExpressionNode {
        std::shared_ptr<ExpressionNode> operand;
    public:
        explicit RefExpressionNode(const Token& opToken,
                                   const std::shared_ptr<ExpressionNode>& operand);
        [[nodiscard]] const std::shared_ptr<ExpressionNode>& getOperand() const;
        [[nodiscard]] NodeType getRealType() const override;
    };
    class BreakExpressionNode : public ExpressionNode {
        // P13-2/L2(Q-C)：`break value` 的携带值（空 = 不带值）
        std::shared_ptr<ExpressionNode> valueNode;
    public:
        explicit BreakExpressionNode(const Token &mainToken_,
                                     const std::shared_ptr<ExpressionNode> &valueNode_ = nullptr);
        [[nodiscard]] const std::shared_ptr<ExpressionNode>& getValueNode() const;
    };

    class ContinueExpressionNode : public ExpressionNode {
    public:
        explicit ContinueExpressionNode(const Token &mainToken_);
    };

    class BranchNode : public ExpressionNode {
        std::shared_ptr<ExpressionNode> conditionNode;
        std::shared_ptr<ExpressionNode> bodyNode;
        // P13-2/L2(Q-C)：`if (cond): T { ... }` 的 `: T` 标注（空 = 未标注）
        std::vector<std::shared_ptr<LabelNode>> resultLabels;
        // 决议 B：`if var p = <expr> { ... }` 的绑定变量名。
        //
        // 非空时语义是「从可空表达式里取值」：conditionNode 就是那个可空
        // 表达式本身，p 只在 then 块里可见且类型是解包后的非空 T。
        // 空串 = 普通条件分支。
        std::string bindingName;
    public:
        explicit BranchNode(
            const Token &branchOpToken,
            const std::shared_ptr<ExpressionNode> &conditionNode,
            const std::shared_ptr<ExpressionNode> &bodyNode,
            const std::vector<std::shared_ptr<LabelNode>> &resultLabels_ = {});
        [[nodiscard]] std::shared_ptr<ExpressionNode> getConditionNode() const;
        [[nodiscard]] std::shared_ptr<ExpressionNode> getBodyNode() const;
        [[nodiscard]] const std::vector<std::shared_ptr<LabelNode>>& getResultLabels() const;
        void setBindingName(const std::string& name);
        [[nodiscard]] const std::string& getBindingName() const;
    };

    class ConditionNode final : public ExpressionNode {
        std::vector<std::shared_ptr<ExpressionNode>> branchNodes;
        // P13-2/L2：整个 if 结构的结果类型标注（取第一个分支的 `: T`）
        std::vector<std::shared_ptr<LabelNode>> resultLabels;
    public:
        explicit ConditionNode(const std::vector<std::shared_ptr<ExpressionNode>> &branchNodes);
        [[nodiscard]] std::vector<std::shared_ptr<ExpressionNode>> getBranchNodes() const;
        void setResultLabels(const std::vector<std::shared_ptr<LabelNode>>& labels);
        [[nodiscard]] const std::vector<std::shared_ptr<LabelNode>>& getResultLabels() const;
    };

    class LoopNode : public ExpressionNode {
        std::shared_ptr<ExpressionNode> conditionNode;
        std::shared_ptr<ExpressionNode> bodyNode;
        LoopType loopType;
        // P13-2/L2(Q-C)：`while (cond): T { ... }` 的 `: T` 标注（空 = 未标注）
        std::vector<std::shared_ptr<LabelNode>> resultLabels;
    public:
        explicit LoopNode(const Token& loopToken,
            const std::shared_ptr<ExpressionNode> &conditionNode,
            const std::shared_ptr<ExpressionNode> &bodyNode,
            const std::vector<std::shared_ptr<LabelNode>> &resultLabels_ = {});
        [[nodiscard]] std::shared_ptr<ExpressionNode> getConditionNode() const;
        [[nodiscard]] std::shared_ptr<ExpressionNode> getBodyNode() const;
        [[nodiscard]] LoopType getLoopType() const;
        [[nodiscard]] const std::vector<std::shared_ptr<LabelNode>>& getResultLabels() const;
    };

    class WhileLoopNode final : public LoopNode {
    public:
        explicit WhileLoopNode(const Token &whileToken,
            const std::shared_ptr<ExpressionNode> &conditionNode,
            const std::shared_ptr<ExpressionNode> &bodyNode,
            const std::vector<std::shared_ptr<LabelNode>> &resultLabels_ = {});
    };

    class UntilLoopNode final : public LoopNode {
    public:
        explicit UntilLoopNode(const Token &untilToken,
            const std::shared_ptr<ExpressionNode> &conditionNode,
            const std::shared_ptr<ExpressionNode> &bodyNode,
            const std::vector<std::shared_ptr<LabelNode>> &resultLabels_ = {});
    };

    class ForLoopNode final : public LoopNode {
        std::shared_ptr<ExpressionNode> initNode;
        std::shared_ptr<ExpressionNode> updateNode;
    public:
        ForLoopNode(const Token &forToken,
            const std::shared_ptr<ExpressionNode> &initNode,
            const std::shared_ptr<ExpressionNode> &conditionNode,
            const std::shared_ptr<ExpressionNode> &updateNode,
            const std::shared_ptr<ExpressionNode> &bodyNode,
            const std::vector<std::shared_ptr<LabelNode>> &resultLabels_ = {});
        [[nodiscard]] std::shared_ptr<ExpressionNode> getInitNode() const;
        [[nodiscard]] std::shared_ptr<ExpressionNode> getUpdateNode() const;
    };

    class PassExpressionNode final : public ExpressionNode {
    public:
        explicit PassExpressionNode(const Token &passToken);
    };

    class EncapsulatedExpressionNode final : public ExpressionNode {
    public:
        explicit EncapsulatedExpressionNode(const Token &mainToken_);
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
        [[nodiscard]] std::shared_ptr<Token> getColonToken() const;
        [[nodiscard]] std::vector<std::shared_ptr<LabelNode>> getLabelNodes() const;
        [[nodiscard]] std::shared_ptr<ExpressionNode> getParamNode() const;
        [[nodiscard]] std::shared_ptr<ExpressionNode> getBodyNode() const;
    };

    class ClassDeclarationNode final : public ExpressionNode {
        std::shared_ptr<ExpressionNode> nameNode;
    public:
        explicit ClassDeclarationNode(
            const Token &classToken,
            const std::shared_ptr<ExpressionNode> &nameNode_);
        [[nodiscard]] std::shared_ptr<ExpressionNode> getNameNode() const;
    };

    class ClassDefinitionNode : public ExpressionNode {
        std::shared_ptr<ExpressionNode> nameNode;
        std::shared_ptr<ExpressionNode> bodyNode;
    public:
        explicit ClassDefinitionNode(
                const Token &classToken,
                const std::shared_ptr<ExpressionNode> &nameNode_,
                const std::shared_ptr<ExpressionNode> &bodyNode_);
        [[nodiscard]] std::vector<std::shared_ptr<LabelNode>> getLabelNode() const;
        [[nodiscard]] std::shared_ptr<ExpressionNode> getBodyNode() const;
        [[nodiscard]] std::shared_ptr<ExpressionNode> getNameNode() const;
        [[nodiscard]] Pos getPos() const override;
    };

    // A（docs/pending_designs.md §A）：值语义 struct 定义。
    //
    // 与 ClassDefinitionNode 结构一致（名字 + 花括号体），但语义完全不同：
    //   * 无继承 —— 名字节点上的标签一律忽略/报错
    //   * 字段只能是标量（int/float/bool/char），布局按声明顺序**内联**
    //     （不是 8 字节槽数组，而是按各自 getSize() 紧凑排列）
    //   * 赋值 = 逐字节拷贝（值语义），不参与 Content 状态格与移动/消耗
    // 之所以不复用 ClassDefinitionNode 加个 bool：两者的登记表
    // （_classes vs _structs）、布局规则、拷贝路径全不同，混在一起
    // 只会让每条路径都要判一次「这个类是 struct 吗」。
    class StructDefinitionNode final : public ExpressionNode {
        std::shared_ptr<ExpressionNode> nameNode;
        std::shared_ptr<ExpressionNode> bodyNode;
    public:
        explicit StructDefinitionNode(
                const Token &structToken,
                const std::shared_ptr<ExpressionNode> &nameNode_,
                const std::shared_ptr<ExpressionNode> &bodyNode_);
        [[nodiscard]] std::shared_ptr<ExpressionNode> getBodyNode() const;
        [[nodiscard]] std::shared_ptr<ExpressionNode> getNameNode() const;
        [[nodiscard]] Pos getPos() const override;
    };

    class ProgramNode final : public StatementNode {
        std::vector<std::shared_ptr<StatementNode>> statements;
    public:
        explicit ProgramNode(const Token &mainToken, std::vector<std::shared_ptr<StatementNode>> statements);


        [[nodiscard]] Pos getPos() const override;

        [[nodiscard]] const std::vector<std::shared_ptr<StatementNode>> &getStatements() const;

        [[nodiscard]] NodeType getRealType() const override;
    };

    class TryNode final : public ExpressionNode
    {
        std::shared_ptr<BlockRangerNode> tryBody;
        std::vector<std::pair<std::shared_ptr<IdentifierNode>,
        std::shared_ptr<BlockRangerNode>>> catchBodies;
        std::shared_ptr<BlockRangerNode> finallyBody;
    public:
        explicit TryNode(
            const Token &mainToken,
            const std::shared_ptr<BlockRangerNode> &tryBody,
            const std::vector<std::pair<std::shared_ptr<IdentifierNode>,
            std::shared_ptr<BlockRangerNode>>> &catchBodies,
            const std::shared_ptr<BlockRangerNode> &finallyBody);
        [[nodiscard]] NodeType getRealType() const override;
        [[nodiscard]] std::shared_ptr<BlockRangerNode> getTryBody() const;
        [[nodiscard]] std::vector<std::pair<std::shared_ptr<IdentifierNode>,
        std::shared_ptr<BlockRangerNode>>> getCatchBodies() const;
        [[nodiscard]] std::shared_ptr<BlockRangerNode> getFinallyBody() const;
    };

    class ThrowNode final : public ExpressionNode
    {
        std::shared_ptr<ExpressionNode> throwExpression;
    public:
        explicit ThrowNode(const Token &mainToken,
            const std::shared_ptr<ExpressionNode>& throwExpression);

        [[nodiscard]] std::shared_ptr<ExpressionNode> getThrowExpression() const;
    };
}


#endif //RCC_RCC_AST_COMPONENTS_H

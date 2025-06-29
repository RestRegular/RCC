//
// Created by RestRegular on 2025/6/29.
//

#ifndef RCC_RCC_AST_COMPONENTS_H
#define RCC_RCC_AST_COMPONENTS_H

#include "./rcc_ast.h"

namespace ast {
    // 自定义类型

    // 枚举类声明

    // 类声明
    // --- 节点类 ---

    /* ToDo: 更多 Node 类声明 */

    // 类定义
    class AstVisitor: public utils::Object {
    public:
        virtual void visitLiteralNode(LiteralNode &node) = 0;
        virtual void visitStringLiteralNode(StringLiteralNode &node) = 0;
        virtual void visitNumberLiteralNode(NumberLiteralNode &node) = 0;
        virtual void visitIntegerLiteralNode(IntegerLiteralNode &node) = 0;
        virtual void visitFloatLiteralNode(FloatLiteralNode &node) = 0;
        virtual void visitBooleanLiteralNode(BooleanLiteralNode &node) = 0;
        virtual void visitCharacterLiteralNode(CharacterLiteralNode &node) = 0;
        virtual void visitVariableDeclarationNode(VariableDefinitionNode &node) = 0;
        virtual void visitVariableAssignmentNode(VariableAssignmentNode &node) = 0;
        virtual void visitParameterNode(ParameterNode &node) = 0;
        virtual void visitArgumentNode(ArgumentNode &node) = 0;
        virtual void visitFunctionDeclarationNode(FunctionDeclarationNode &node) = 0;
        virtual void visitConstructorDefinitionNode(ConstructorDefinitionNode &node) = 0;
        virtual void visitClassDeclarationNode(ClassDeclarationNode &node) = 0;
        virtual void visitClassDefinitionNode(ClassDefinitionNode &node) = 0;
        virtual void visitFunctionCallNode(FunctionCallNode &node) = 0;
        virtual void visitProgramNode(ProgramNode &node) = 0;
    };

    class AstNode: public utils::Object {
    public:
        ~AstNode() override = default;
        [[nodiscard]] virtual utils::Pos getPos() const = 0;
        virtual void acceptVisitor(AstVisitor &visitor) = 0;
        [[nodiscard]] std::string toString() const override;
        [[nodiscard]] std::string briefString() const override;
        [[nodiscard]] std::string professionalString() const override;
    };

    class ExpressionNode: public AstNode {

    };

    class StatementNode: public AstNode {

    };

    class LiteralNode: public AstNode {
    protected:
        core::Token token;
    public:
        explicit LiteralNode(core::Token token);
        [[nodiscard]] utils::Pos getPos() const override;
        void acceptVisitor(AstVisitor &visitor) override;
        [[nodiscard]] virtual std::string literalString () const = 0;
        [[nodiscard]] std::string professionalString() const override;
    };

    class StringLiteralNode: public LiteralNode {
        std::string value;
    public:
        explicit StringLiteralNode(core::Token token);
        [[nodiscard]] utils::Pos getPos() const override;
        void acceptVisitor(AstVisitor &visitor) override;

        [[nodiscard]] std::string literalString() const override;
    };

    class NumberLiteralNode: public LiteralNode {
    public:
        explicit NumberLiteralNode(core::Token token);
        [[nodiscard]] utils::Pos getPos() const override;
        void acceptVisitor(AstVisitor &visitor) override;
    };

    class IntegerLiteralNode: public NumberLiteralNode {
        int value;
    public:
        explicit IntegerLiteralNode(core::Token token);
        [[nodiscard]] utils::Pos getPos() const override;
        void acceptVisitor(AstVisitor &visitor) override;

        [[nodiscard]] std::string literalString() const override;
    };

    class FloatLiteralNode: public NumberLiteralNode {
        double value;
    public:
        explicit FloatLiteralNode(core::Token token);
        [[nodiscard]] utils::Pos getPos() const override;
        void acceptVisitor(AstVisitor &visitor) override;

        [[nodiscard]] std::string literalString() const override;
    };

    class BooleanLiteralNode: public LiteralNode {
        bool value;
    public:
        explicit BooleanLiteralNode(core::Token token);
        [[nodiscard]] utils::Pos getPos() const override;
        void acceptVisitor(AstVisitor &visitor) override;

        [[nodiscard]] std::string literalString() const override;
    };

    class CharacterLiteralNode: public LiteralNode {
        char value;
    public:
        explicit CharacterLiteralNode(core::Token token);
        [[nodiscard]] utils::Pos getPos() const override;
        void acceptVisitor(AstVisitor &visitor) override;

        [[nodiscard]] std::string literalString() const override;
    };

    class VariableDefinitionNode: public AstNode {
        typedef core::Token Token;
        struct VariableDeclarationData {
            Token nameToken;
            std::unique_ptr<Token> colonToken;
            std::vector<std::unique_ptr<Token>> labelTokens;
            std::unique_ptr<Token> equalSignToken;
            std::unique_ptr<AstNode> initialValue;
        };
        Token varToken;
        VariableDeclarationData declarationData;
    public:
        explicit VariableDefinitionNode(Token keywordToken, VariableDeclarationData declarationData);
        [[nodiscard]] utils::Pos getPos() const override;
        void acceptVisitor(AstVisitor &visitor) override;
    };

    class VariableAssignmentNode: public AstNode {
        typedef core::Token Token;
        std::map<std::unique_ptr<Token>, std::unique_ptr<Token>> assignments;
    public:
        explicit VariableAssignmentNode(std::map<std::unique_ptr<Token>, std::unique_ptr<Token>> assignments);
        [[nodiscard]] utils::Pos getPos() const override;
        void acceptVisitor(AstVisitor &visitor) override;
    };

    class ParameterNode: public AstNode {
        typedef core::Token Token;
        Token nameToken;
        std::unique_ptr<Token> colonToken;
        std::vector<std::unique_ptr<Token>> labelTokens;
        std::unique_ptr<Token> defaultValue;
    public:
        explicit ParameterNode(Token nameToken,
                               std::unique_ptr<Token> colonToken,
                               std::vector<std::unique_ptr<Token>> labelTokens,
                               std::unique_ptr<Token> defaultValue);
        void acceptVisitor(AstVisitor &visitor) override;
        [[nodiscard]] utils::Pos getPos() const override;
    };

    class FunctionDeclarationNode: public AstNode {
        typedef core::Token Token;
        Token funToken;
        Token nameToken;
        std::vector<std::unique_ptr<ProgramNode>> paramNodes;
        std::vector<std::unique_ptr<Token>> labelTokens;
    public:
        explicit FunctionDeclarationNode(Token funToken, Token nameToken, std::vector<std::unique_ptr<ProgramNode>> parameterNodes, std::vector<std::unique_ptr<Token>> labelTokens);
        [[nodiscard]] utils::Pos getPos() const override;
        void acceptVisitor(AstVisitor &visitor) override;
    };

    class FunctionDefinitionNode: public FunctionDeclarationNode {
        typedef core::Token Token;
        std::unique_ptr<AstNode> bodyNode;
    public:
        explicit FunctionDefinitionNode(Token funToken, Token nameToken, std::vector<std::unique_ptr<ProgramNode>> parameterNodes, std::vector<std::unique_ptr<Token>> labelTokens, std::unique_ptr<AstNode> bodyNode);
    };

    class ArgumentNode: public AstNode {
        typedef core::Token Token;
        std::unique_ptr<AstNode> argNode;
    public:
        explicit ArgumentNode(std::unique_ptr<AstNode> argNode);
        [[nodiscard]] utils::Pos getPos() const override;
        void acceptVisitor(AstVisitor &visitor) override;
    };

    class FunctionCallNode: public AstNode {
        typedef core::Token Token;
        Token nameToken;
        std::vector<std::unique_ptr<ArgumentNode>> argNodes;
    public:
        explicit FunctionCallNode(Token nameToken, std::vector<std::unique_ptr<ArgumentNode>> argNodes);
        [[nodiscard]] utils::Pos getPos() const override;
        void acceptVisitor(AstVisitor &visitor) override;
    };

    class ConstructorDefinitionNode: public AstNode {
        typedef core::Token Token;
        Token ctorToken;
        std::vector<std::unique_ptr<ParameterNode>> paramNodes;
        std::unique_ptr<AstNode> bodyNode;
    public:
        explicit ConstructorDefinitionNode(Token ctorToken, std::vector<std::unique_ptr<ParameterNode>> paramNodes, std::unique_ptr<AstNode> bodyNode);
        void acceptVisitor(AstVisitor &visitor) override;
        [[nodiscard]] utils::Pos getPos() const override;
    };

    class ClassDeclarationNode: public AstNode {
        typedef core::Token Token;
        Token classToken;
        Token nameToken;
        std::unique_ptr<Token> colonToken;
        std::vector<std::unique_ptr<Token>> labelTokens;
    public:
        explicit ClassDeclarationNode(Token classToken, Token nameToken, std::unique_ptr<Token> colonToken, std::vector<std::unique_ptr<Token>> labelTokens);
        void acceptVisitor(AstVisitor &visitor) override;
        [[nodiscard]] utils::Pos getPos() const override;
    };

    class ClassDefinitionNode: public ClassDeclarationNode {
        typedef core::Token Token;
        std::vector<std::unique_ptr<ConstructorDefinitionNode>> ctorNodes;
        std::vector<std::unique_ptr<VariableDefinitionNode>> fieldNodes;
        std::vector<std::unique_ptr<FunctionDeclarationNode>> methodDeclarationNodes;
        std::vector<std::unique_ptr<FunctionDefinitionNode>> methodDefinitionNodes;
    public:
        explicit ClassDefinitionNode(
                Token classToken_,
                Token nameToken_,
                std::unique_ptr<Token> colonToken_,
                std::vector<std::unique_ptr<Token>> labelTokens_,
                std::vector<std::unique_ptr<ConstructorDefinitionNode>> ctorNodes,
                std::vector<std::unique_ptr<VariableDefinitionNode>> fieldNodes,
                std::vector<std::unique_ptr<FunctionDeclarationNode>> methodDeclarationNodes,
                std::vector<std::unique_ptr<FunctionDefinitionNode>> methodDefinitionNodes);
        [[nodiscard]] utils::Pos getPos() const override;
        void acceptVisitor(AstVisitor &visitor) override;
    };

    class ProgramNode: public AstNode {
        std::vector<std::unique_ptr<AstNode>> statements;
    public:
        explicit ProgramNode(std::vector<std::unique_ptr<AstNode>> statements);
        void acceptVisitor(AstVisitor &visitor) override;
        [[nodiscard]] utils::Pos getPos() const override;
    };
}


#endif //RCC_RCC_AST_COMPONENTS_H

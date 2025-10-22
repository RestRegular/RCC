//
// Created by RestRegular on 2025/6/29.
//

#ifndef RCC_RCC_PARSER_H
#define RCC_RCC_PARSER_H

#include <memory>
#include <functional>
#include <list>
#include <queue>

#include "../rcc_base.h"
#include "./rcc_ast.h"
#include "./rcc_ast_components.h"

namespace parser {
    using namespace ast;
    using namespace base;
    using namespace utils;
    using namespace core;

    class Parser;

    typedef std::shared_ptr<ExpressionNode> ExpressionNodePtr;
    typedef std::function<ExpressionNodePtr(Parser*)> PrefixExpressionBuilder;
    typedef std::function<ExpressionNodePtr(Parser*, const ExpressionNodePtr &)> InfixExpressionBuilder;
    typedef std::function<ExpressionNodePtr(Parser*, const ExpressionNodePtr &)> PostfixExpressionBuilder;

    class Parser {
    public:
        explicit Parser(std::queue<std::shared_ptr<Token>> tokens);
        std::pair<bool, std::shared_ptr<ProgramNode>> parse();
        [[nodiscard]] StringVector getErrorMsgs() const;
        void printParserErrors(std::ostream &os = std::cerr) const;
        [[nodiscard]] bool hasError() const;
    private:
        static Token STREAM_START_TOKEN;
        static Token STREAM_END_TOKEN;

        // 解析状态
        bool isInDictRanger = false;

        // 词法分析结果
        std::queue<std::shared_ptr<Token>> _tokens;
        // 先前的 Token
        std::stack<std::shared_ptr<Token>> _previous_tokens;
        // 当前的 Token
        std::shared_ptr<Token> _current_token;
        // 临时 Token
        std::shared_ptr<Token> _next_token;
        // 添加的临时 Token，先添加先使用
        std::list<std::shared_ptr<Token>> _tempTokens;
        // 语法分析错误列表
        std::vector<std::string> errorMsgs;
        // 前缀表达式构建函数映射表
        static std::map<TokenType, PrefixExpressionBuilder> prefixExpressionBuilders;
        // 中缀表达式构建函数映射表
        static std::map<TokenType, InfixExpressionBuilder> infixExpressionBuilders;
        // 后缀表达式构建函数映射表
        static std::map<TokenType, PostfixExpressionBuilder> postfixExpressionBuilders;
        // 运算符优先级映射表
        static std::map<TokenType, Precedence> precedenceMap;

        bool isAtEnd();
        void previous();
        const Token &next();
        void consumeNext();
        void reset();
        const Token &previousToken() const;
        [[nodiscard]] const Token &currentToken();
        [[nodiscard]] const Token &nextToken() const;
        [[nodiscard]] bool hasNext() const;
        [[nodiscard]] bool previousTokenIs(TokenType type) const;
        [[nodiscard]] bool currentTokenIs(TokenType type);
        [[nodiscard]] bool nextTokenIs(TokenType type) const;
        void skipCurrentNewLineToken();
        void skipNextNewLineToken();
        bool expectedNextTokenAndConsume(TokenType type);
        void consumeNextIfNextTokenIs(TokenType type);
        bool expectedNextToken(TokenType type);
        void appendErrorMsg(const RCCError &error);
        [[nodiscard]] Precedence currentTokenPrecedence();
        [[nodiscard]] Precedence nextTokenPrecedence() const;

        // 临时 Token 相关函数
        void appendTemToken(const Token &token);
        void clearTemTokens();

        // 错误处理
        void recordUnexpectedTokenTypeError(const Token& token, TokenType expectedType);
        void recordUnexpectedTokenTypeError(const Token& token, const std::string &expectedType);
        void recordUnclosedExpressionError(const Token& beginToken, const Token &endToken, TokenType expectedType);
        void recordPrefixBuilderNotFoundError(const Token &token, TokenType errorType);
        void recordInfixBuilderNotFoundError(const Token &token, TokenType errorType);
        void recordSyntaxError(const Token &token, const Token &errorToken, const std::string &errorMsg);

        // 根节点构建函数
        std::shared_ptr<ProgramNode> buildProgram();
        std::shared_ptr<StatementNode> buildStatement();
        std::shared_ptr<ExpressionStatementNode> buildExpressionStatement();

        // 表达式构建函数
        ExpressionNodePtr buildExpression(Precedence precedence);

        // 前缀表达式构建函数
        ExpressionNodePtr buildUnaryExpression();
        ExpressionNodePtr buildBoolExpression();
        ExpressionNodePtr buildBreakExpression();
        ExpressionNodePtr buildClassExpression();
        ExpressionNodePtr buildContinueExpression();
        ExpressionNodePtr buildFloatExpression();
        ExpressionNodePtr buildForExpression();
        ExpressionNodePtr buildFunctionExpression();
        ExpressionNodePtr buildIdentifierExpression();
        ExpressionNodePtr buildIfExpression();
        ExpressionNodePtr buildConditionExpression();
        ExpressionNodePtr buildImportExpression();
        ExpressionNodePtr buildIntegerExpression();
        ExpressionNodePtr buildLabelExpression();
        ExpressionNodePtr buildRangerExpression();
        ExpressionNodePtr buildBraceExpression();
        ExpressionNodePtr buildNullExpression();
        ExpressionNodePtr buildPrefixExpression();
        ExpressionNodePtr buildReturnExpression();
        ExpressionNodePtr buildStringExpression();
        ExpressionNodePtr buildCharExpression();
        ExpressionNodePtr buildSuperExpression();
        ExpressionNodePtr buildThisExpression();
        ExpressionNodePtr buildWhileExpression();
        ExpressionNodePtr buildUntilExpression();
        ExpressionNodePtr buildPassExpression();
        ExpressionNodePtr buildEncapsulatedExpression();
        ExpressionNodePtr buildConstructorExpression();
        ExpressionNodePtr buildDictionaryExpression();
        ExpressionNodePtr buildListExpression();
        ExpressionNodePtr buildVariableExpression();
        ExpressionNodePtr skipNewLineExpression();

        // 中缀表达式构建函数
        ExpressionNodePtr buildInfixExpression(const ExpressionNodePtr &left);
        ExpressionNodePtr buildAssignExpression(const ExpressionNodePtr &left);
        ExpressionNodePtr buildCompoundExpression(const ExpressionNodePtr &left);
        ExpressionNodePtr buildTernaryExpression(const ExpressionNodePtr &left);
        ExpressionNodePtr buildAttributeExpression(const ExpressionNodePtr &left);

        // 后缀表达式构建函数
        ExpressionNodePtr buildPostfixExpression(const ExpressionNodePtr &left);
        ExpressionNodePtr buildCallExpression(const ExpressionNodePtr &left);
        ExpressionNodePtr buildIndexExpression(const ExpressionNodePtr &left);

        // 语句构建辅助函数
        bool validateForRangeExpression(const ExpressionNodePtr& expr, const Token& forToken);
        ExpressionNodePtr buildBracketExpression();

    };

    void enableDebugMode(bool cond);
} // analyzer

#endif //RCC_RCC_PARSER_H

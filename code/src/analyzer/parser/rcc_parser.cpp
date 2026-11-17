//
// Created by RestRegular on 2025/6/29.
//

#include "../../../include/rcc_base.h"
#include "../../../include/analyzer/rcc_parser.h"

namespace parser {
    std::pair<bool, std::shared_ptr<ProgramNode>> Parser::parse() {
        try
        {
            const auto &result = buildProgram();
            // 注意语义：返回值的 first 表示"是否出错"（true = 有错误）。
            // 这是历史约定（P12 起旧的 CompileVisitor 已删除，但返回类型保持不变
            // 以免影响既有调用点）；新代码请用 !first。
            return {hasError(), result};
        } catch (const std::exception &)
        {
            if (hasError())
            {
                printParserErrors();
            }
            throw;
        }
    }

    Parser::Parser(std::queue<std::shared_ptr<Token>> tokens)
    : _tokens(std::move(tokens)) {
        next();
        next();
    }

    Token Parser::STREAM_START_TOKEN = Token(Pos(), RCC_TOKEN_STREAM_START);
    Token Parser::STREAM_END_TOKEN = Token(Pos(), RCC_TOKEN_STREAM_END);

    std::map<core::TokenType, PrefixExpressionBuilder> Parser::prefixExpressionBuilders {
        // 字面量
        {TokenType::TOKEN_INTEGER, &Parser::buildIntegerExpression},
        {TokenType::TOKEN_FLOAT, &Parser::buildFloatExpression},
        {TokenType::TOKEN_STRING, &Parser::buildStringExpression},
        {TokenType::TOKEN_CHAR, &Parser::buildCharExpression},
        {TokenType::TOKEN_TRUE, &Parser::buildBoolExpression},
        {TokenType::TOKEN_FALSE, &Parser::buildBoolExpression},
        {TokenType::TOKEN_NULL, &Parser::buildNullExpression},
        // 范围符
        {TokenType::TOKEN_LPAREN, &Parser::buildRangerExpression},
        {TokenType::TOKEN_LBRACKET, &Parser::buildListExpression},
        {TokenType::TOKEN_LBRACE, &Parser::buildDictionaryExpression},
        // 标识符
        {TokenType::TOKEN_IDENTIFIER, &Parser::buildIdentifierExpression},
        {TokenType::TOKEN_LABEL, &Parser::buildLabelExpression},
        // 一元前缀运算符：+、-、!、*、**、:
        {TokenType::TOKEN_PLUS, &Parser::buildPrefixExpression},
        {TokenType::TOKEN_MINUS, &Parser::buildPrefixExpression},
        {TokenType::TOKEN_NOT, &Parser::buildUnaryExpression},
        // P13-2/L3-F（决议 §9.2）：ensures[?a] 的 ? 标记（一元前缀）
        {TokenType::TOKEN_QUESTION, &Parser::buildUnaryExpression},
        // P13-2/L3(Q-E)：`&` 前缀取引用（位运算暂不做，前缀无歧义）
        {TokenType::TOKEN_BIT_AND, &Parser::buildRefExpression},
        {TokenType::TOKEN_STAR, &Parser::buildUnaryExpression},
        {TokenType::TOKEN_DOUBLE_STAR, &Parser::buildUnaryExpression},
        // 变量定义
        {TokenType::TOKEN_VAR, &Parser::buildVariableExpression},
        // 函数定义
        {TokenType::TOKEN_FUNCTION, &Parser::buildFunctionExpression},
        // 条件语句
        {TokenType::TOKEN_IF, &Parser::buildConditionExpression},
        // 循环语句
        {TokenType::TOKEN_WHILE, &Parser::buildWhileExpression},
        {TokenType::TOKEN_UNTIL, &Parser::buildUntilExpression},
        {TokenType::TOKEN_FOR, &Parser::buildForExpression},
        // pass 语句
        {TokenType::TOKEN_PASS, &Parser::buildPassExpression},
        // encapsulated 语句
        {TokenType::TOKEN_ENCAPSULATED, &Parser::buildEncapsulatedExpression},
        // 构造函数
        {TokenType::TOKEN_CTOR, &Parser::buildConstructorExpression},
        // 类定义
        {TokenType::TOKEN_CLASS, &Parser::buildClassExpression},
        // 返回语句
        {TokenType::TOKEN_RETURN, &Parser::buildReturnExpression},
        // P13-2/L1：`ret` 退最近可返回值作用域，与 return 分开 token、共用构建函数。
        {TokenType::TOKEN_RET, &Parser::buildReturnExpression},
        // 退出循环语句
        {TokenType::TOKEN_BREAK, &Parser::buildBreakExpression},
        // 跳过本次循环
        {TokenType::TOKEN_CONTINUE, &Parser::buildContinueExpression},
        // 跳过新行
        {TokenType::TOKEN_SKIP_NEWLINE, &Parser::skipNewLineExpression},
        // try 语句
        {TokenType::TOKEN_TRY, &Parser::buildTryExpression},
        // throw 语句
        {TokenType::TOKEN_THROW, &Parser::buildThrowExpression}
    };

    std::map<core::TokenType, InfixExpressionBuilder> Parser::infixExpressionBuilders {
            {TokenType::TOKEN_PLUS, &Parser::buildInfixExpression},
            {TokenType::TOKEN_MINUS, &Parser::buildInfixExpression},
            {TokenType::TOKEN_STAR, &Parser::buildInfixExpression},
            {TokenType::TOKEN_SLASH, &Parser::buildInfixExpression},
            {TokenType::TOKEN_GREATER, &Parser::buildInfixExpression},
            {TokenType::TOKEN_LESS, &Parser::buildInfixExpression},
            {TokenType::TOKEN_GREATER_EQUAL, &Parser::buildInfixExpression},
            {TokenType::TOKEN_LESS_EQUAL, &Parser::buildInfixExpression},
            {TokenType::TOKEN_EQUAL, &Parser::buildInfixExpression},
            {TokenType::TOKEN_NOT_EQUAL, &Parser::buildInfixExpression},
            {TokenType::TOKEN_AND, &Parser::buildInfixExpression},
            {TokenType::TOKEN_OR, &Parser::buildInfixExpression},
            {TokenType::TOKEN_MODULO, &Parser::buildInfixExpression},
            {TokenType::TOKEN_ASSIGN, &Parser::buildAssignExpression},
            {TokenType::TOKEN_COMMA, &Parser::buildInfixExpression},
            {TokenType::TOKEN_PLUS_ASSIGN, &Parser::buildInfixExpression},
            {TokenType::TOKEN_MINUS_ASSIGN, &Parser::buildInfixExpression},
            {TokenType::TOKEN_STAR_ASSIGN, &Parser::buildInfixExpression},
            {TokenType::TOKEN_SLASH_ASSIGN, &Parser::buildInfixExpression},
            {TokenType::TOKEN_MODULO_ASSIGN, &Parser::buildInfixExpression},
            {TokenType::TOKEN_COLON, &Parser::buildInfixExpression},
            {TokenType::TOKEN_INDICATOR, &Parser::buildInfixExpression},
            {TokenType::TOKEN_DOT, &Parser::buildInfixExpression},
            {TokenType::TOKEN_LPAREN, &Parser::buildCallExpression},
            {TokenType::TOKEN_LBRACKET, &Parser::buildIndexExpression}
    };

    std::map<core::TokenType, PostfixExpressionBuilder> Parser::postfixExpressionBuilders {
            {TokenType::TOKEN_DOUBLE_PLUS, &Parser::buildPostfixExpression},
            {TokenType::TOKEN_DOUBLE_MINUS, &Parser::buildPostfixExpression},
            {TokenType::TOKEN_LBRACKET, &Parser::buildIndexExpression}
    };

    std::map<core::TokenType, Precedence> Parser::precedenceMap = {
            {TokenType::TOKEN_PLUS, Precedence::SUM},
            {TokenType::TOKEN_MINUS, Precedence::SUM},
            {TokenType::TOKEN_STAR, Precedence::PRODUCT},
            {TokenType::TOKEN_SLASH, Precedence::PRODUCT},
            {TokenType::TOKEN_MODULO, Precedence::PRODUCT},
            {TokenType::TOKEN_ASSIGN, Precedence::ASSIGN},
            {TokenType::TOKEN_PLUS_ASSIGN, Precedence::ASSIGN},
            {TokenType::TOKEN_MINUS_ASSIGN, Precedence::ASSIGN},
            {TokenType::TOKEN_SLASH_ASSIGN, Precedence::ASSIGN},
            {TokenType::TOKEN_STAR_ASSIGN, Precedence::ASSIGN},
            {TokenType::TOKEN_MODULO_ASSIGN, Precedence::ASSIGN},
            {TokenType::TOKEN_COMMA, Precedence::PARALLEL},
            {TokenType::TOKEN_COLON, Precedence::KV_SEP},
            {TokenType::TOKEN_GREATER, Precedence::COMPARE},
            {TokenType::TOKEN_GREATER_EQUAL, Precedence::COMPARE},
            {TokenType::TOKEN_LESS, Precedence::COMPARE},
            {TokenType::TOKEN_LESS_EQUAL, Precedence::COMPARE},
            {TokenType::TOKEN_EQUAL, Precedence::COMPARE},
            {TokenType::TOKEN_NOT_EQUAL, Precedence::COMPARE},
            {TokenType::TOKEN_AND, Precedence::LOGIC},
            {TokenType::TOKEN_OR, Precedence::LOGIC},
            {TokenType::TOKEN_DOUBLE_PLUS, Precedence::POSTFIX},
            {TokenType::TOKEN_DOUBLE_MINUS, Precedence::POSTFIX},
            {TokenType::TOKEN_INDICATOR, Precedence::INDICATOR},
            {TokenType::TOKEN_DOT, Precedence::ATTRIBUTE},
            {TokenType::TOKEN_LPAREN, Precedence::CALL},
            {TokenType::TOKEN_LBRACKET, Precedence::INDEX}
    };

    const Token &Parser::currentToken()
    {
        if (!_current_token)
        {
            appendErrorMsg(RCCParserError::parserError(
                previousToken().getPosString(),
                "Current processing token is nullptr.",
                "Next token is " + nextToken().toString()));
            throw std::runtime_error("Encountered parser error");
        }
        return *_current_token;
    }

    const Token &Parser::nextToken() const {
        if (!hasNext()) return STREAM_END_TOKEN;
        return _next_token ?
        *_next_token :
        _tempTokens.empty() ?
        *_tokens.front() :
        *_tempTokens.front();
    }

    bool Parser::isAtEnd() {
        return currentTokenIs(TokenType::TOKEN_STREAM_END);
    }

    void Parser::previous() {
        if (_previous_tokens.empty()) throw std::runtime_error("No previous token");
        _tempTokens.push_front(_next_token);
        _next_token = _current_token;
        _current_token = _previous_tokens.top();
        _previous_tokens.pop();
    }

    const Token & Parser::previousToken() const {
        if (_previous_tokens.empty()) throw std::runtime_error("No previous token");
        return *_previous_tokens.top();
    }

    const Token &Parser::next() {
        _previous_tokens.push(_current_token);
        _current_token = _next_token;
        if (!_tempTokens.empty()) {
            _next_token = _tempTokens.front();
            _tempTokens.pop_front();
        } else if (!_tokens.empty()) {
            _next_token = _tokens.front();
            _tokens.pop();
        } else {
            _next_token = nullptr;
        }
        // 原先写成 `_current_token ? currentToken() : *_current_token`：
        // 空指针分支恰好执行 *_current_token，必然触发 shared_ptr 的
        // __p != nullptr 断言。而构造函数里两次 next() 中的第一次，
        // _current_token 本来就还是 nullptr（合法的初始化状态），
        // 所以旧代码在 Parser 构造时就必定崩溃。
        // 这里改为：当前 token 为空时返回流结束标记，绝不空解引用。
        return _current_token ? currentToken() : STREAM_END_TOKEN;
    }

    void Parser::consumeNext() {
        if (!_tempTokens.empty()) {
            _next_token = _tempTokens.front();
            _tempTokens.pop_front();
        } else if (!_tokens.empty()) {
            _next_token = _tokens.front();
            _tokens.pop();
        } else {
            _next_token = nullptr;
        }
    }

    void Parser::reset() {
        _previous_tokens = {};
        _current_token = nullptr;
        _next_token = nullptr;
        // 清空临时 token 队列
        _tempTokens.clear();
    }

    bool Parser::hasNext() const {
        return _next_token != nullptr;
    }

    bool Parser::previousTokenIs(core::TokenType type) const {
        return previousToken().getType() == type;
    }

    bool Parser::currentTokenIs(core::TokenType type) {
        return currentToken().getType() == type;
    }

    bool Parser::nextTokenIs(core::TokenType type) const {
        return nextToken().getType() == type;
    }

    void Parser::skipCurrentNewLineToken()
    {
        while (currentTokenIs(core::TokenType::TOKEN_NEWLINE)) next();
    }

    void Parser::skipNextNewLineToken()
    {
        while (nextTokenIs(core::TokenType::TOKEN_NEWLINE)) next();
    }

    // 检查下一个 token 的类型，如果是，则消耗当前 token ，否则记录错误
    bool Parser::expectedNextTokenAndConsume(core::TokenType type) {
        if (nextTokenIs(type)) {
            next();
            return true;
        }
        recordUnexpectedTokenTypeError(nextToken(), type);
        return false;
    }

    void Parser::consumeNextIfNextTokenIs(core::TokenType type) {
        if (nextTokenIs(type)) {
            next();
        }
    }

    // 检查下一个 token 的类型，如果不是，则记录错误
    bool Parser::expectedNextToken(core::TokenType type) {
        if (nextTokenIs(type)) {
            return true;
        }
        recordUnexpectedTokenTypeError(nextToken(), type);
        return false;
    }

    void Parser::appendErrorMsg(const RCCError &error) {
        errorMsgs.emplace_back(error.briefString());
    }

    Precedence Parser::currentTokenPrecedence()
    {
        if (const auto it = precedenceMap.find(currentToken().getType());
            it != precedenceMap.end()) {
            return it->second;
        }
        return Precedence::LOWEST;
    }

    Precedence Parser::nextTokenPrecedence() const {
        if (const auto it = precedenceMap.find(nextToken().getType());
            it != precedenceMap.end()) {
            return it->second;
        }
        return Precedence::LOWEST;
    }

    void Parser::appendTemToken(const Token &token) {
        _tempTokens.push_back(std::make_shared<Token>(token));
    }

    void Parser::clearTemTokens() {
        _tempTokens.clear();
    }

    void Parser::printParserErrors(std::ostream &os) const {
        for (const auto &errorMsg : errorMsgs) {
            os << errorMsg;
        }
        exit(-1);
    }

    bool Parser::hasError() const {
        return !errorMsgs.empty();
    }

    ExpressionNodePtr Parser::buildExpression(Precedence precedence) {
        const auto &prefixBuilder = prefixExpressionBuilders.find(currentToken().getType());
        if (prefixBuilder == prefixExpressionBuilders.end()) {
            recordPrefixBuilderNotFoundError(currentToken(), currentToken().getType());
            return nullptr;
        }
        auto expressionNode = prefixBuilder->second(this);
        while (!isAtEnd() && !nextTokenIs(core::TokenType::TOKEN_NEWLINE)){
            // 检查当前 token 是否为后缀运算符
            if (const auto &it = postfixExpressionBuilders.find(nextToken().getType());
                it != postfixExpressionBuilders.end()) {
                next();
                expressionNode = it->second(this, expressionNode);
                continue;
            }

            if (nextTokenIs(core::TokenType::TOKEN_INTEGER) || nextTokenIs(core::TokenType::TOKEN_FLOAT)) {
                if (const std::string &value = nextToken().getValue();
                    value.starts_with("-")) {
                    // 处理：当后一个 Token 是负数且省略中缀运算符时，将负号调整为减法运算符
                    const auto &next_token = nextToken();
                    auto nextPos = next_token.getPos();
                    nextPos.setOffset(1);
                    appendTemToken(Token(nextPos, "-"));
                    Pos newPos = nextPos;
                    newPos.setColumn(nextPos.getColumn() + 1);
                    newPos.setOffset(nextPos.getOffset() - 1);
                    Token numToken(newPos,value.substr(1));
                    appendTemToken(numToken);
                    consumeNext();
                    continue;
                }
            }

            if (nextTokenIs(core::TokenType::TOKEN_SKIP_NEWLINE))
            {
                next();
                skipNextNewLineToken();
            }

            if (precedence < nextTokenPrecedence()) {
                const auto &infixBuilder = infixExpressionBuilders.find(nextToken().getType());
                if (infixBuilder == infixExpressionBuilders.end()) {
                    return expressionNode;
                }
                next();
                expressionNode = infixBuilder->second(this, expressionNode);
            } else {
                break;
            }
        }
        return expressionNode;
    }

    bool Parser::peekBraceIsDictionary() const {
        // 调用点：currentToken 是 `{`，nextToken 是第一个内容 token。
        // Q-D 裁决：空 `{}` 一律是空字典；顶层出现 `:`（键值分隔）即字典；
        // 否则是裸块（若干语句，单行的分号由 buildBraceExpression 复用现有逻辑）。
        // 只读前瞻：复制未消费的 token 队列，绝不改动 _tokens / _tempTokens / _next_token。
        if (nextTokenIs(core::TokenType::TOKEN_RBRACE)) return true;

        auto queueCopy = _tokens;
        std::shared_ptr<Token> tok = _next_token;
        int depth = 0;        // 只追踪 () / [] / {} 嵌套；顶层 depth==0
        bool typeAnnotationAhead = false; // 语句以 var/fun/class 开头 => 其 `:` 是类型标注
        auto isDelimiter = [](const std::shared_ptr<Token>& t) {
            return t->getType() == core::TokenType::TOKEN_NEWLINE ||
                   (t->getType() == core::TokenType::TOKEN_DELIMITER &&
                    t->getValue() == ";");
        };
        while (tok) {
            const auto type = tok->getType();
            if (depth == 0) {
                if (type == core::TokenType::TOKEN_VAR ||
                    type == core::TokenType::TOKEN_FUNCTION ||
                    type == core::TokenType::TOKEN_CLASS) {
                    typeAnnotationAhead = true;
                } else if (isDelimiter(tok)) {
                    typeAnnotationAhead = false;
                }
            }
            if (type == core::TokenType::TOKEN_LBRACE ||
                type == core::TokenType::TOKEN_LBRACKET ||
                type == core::TokenType::TOKEN_LPAREN) {
                ++depth;
            } else if (type == core::TokenType::TOKEN_RBRACE) {
                if (depth == 0) break; // 匹配到本 `{` 的右括号，扫描结束
                --depth;
            } else if (type == core::TokenType::TOKEN_RBRACKET ||
                       type == core::TokenType::TOKEN_RPAREN) {
                if (depth > 0) --depth;
            } else if (depth == 0 && type == core::TokenType::TOKEN_COLON &&
                       !typeAnnotationAhead) {
                return true; // 顶层冒号且不是类型标注 = 键值对 = 字典
            }
            tok = queueCopy.empty() ? nullptr : queueCopy.front();
            if (!queueCopy.empty()) queueCopy.pop();
        }
        return false; // 无顶层冒号：裸块
    }
    std::shared_ptr<ProgramNode> Parser::buildProgram() {
        // 注意：lexer 从不产出 TOKEN_PROGRAM，首 token 是值为空的占位符
        // （TOKEN_UNDEFINED）。因此这里不能强制要求 TOKEN_PROGRAM，
        // 否则任何源文件都会立刻报错。改为：若首 token 不是 PROGRAM，
        // 就把它当作流起始占位符跳过。
        if (currentTokenIs(core::TokenType::TOKEN_PROGRAM)) {
            next();
        }
        const auto &programToken = currentToken();
        std::vector<std::shared_ptr<StatementNode>> statementNodes;
        while (hasNext()) {
            // TOKEN_UNDEFINED 是 lexer 放在流首的空占位符，直接跳过。
            if (!currentTokenIs(core::TokenType::TOKEN_NEWLINE) &&
                !currentTokenIs(core::TokenType::TOKEN_UNDEFINED)){
                if (auto statement = buildStatement()) {
                    statementNodes.emplace_back(statement);
                }
            }
            next();
        }
        return std::make_shared<ProgramNode>(programToken, statementNodes);
    }

    std::shared_ptr<StatementNode> Parser::buildStatement() {
        return buildExpressionStatement();
    }

    std::shared_ptr<ExpressionStatementNode> Parser::buildExpressionStatement() {
        const auto &mainToken = currentToken();
        const auto &expression = buildExpression(Precedence::LOWEST);
        while (nextTokenIs(TokenType::TOKEN_NEWLINE)) next();
        return std::make_shared<ExpressionStatementNode>(mainToken, expression);
    }

    StringVector Parser::getErrorMsgs() const {
        return errorMsgs;
    }
} // analyzer
//
// Created by RestRegular on 2025/6/30.
//

#include "../../../../include/analyzer/rcc_ast.h"
#include "../../../../include/analyzer/rcc_parser.h"

namespace parser {
    static std::unordered_map<TokenType, NodeType> prefixExpressionNodeTypeMap = {
        {TokenType::TOKEN_VAR, NodeType::VAR},
        {TokenType::TOKEN_PLUS, NodeType::UNARY},
        {TokenType::TOKEN_MINUS, NodeType::UNARY},
        {TokenType::TOKEN_NOT, NodeType::UNARY},
        {TokenType::TOKEN_FUNCTION, NodeType::FUNCTION},
        {TokenType::TOKEN_STAR, NodeType::UNARY},
        {TokenType::TOKEN_DOUBLE_STAR, NodeType::UNARY}
    };

    ExpressionNodePtr Parser::buildPrefixExpression(){
        auto recordToken = currentToken();
        const auto &precedence = currentTokenPrecedence();
        const auto &it = prefixExpressionNodeTypeMap.find(recordToken.getType());
        if (it == prefixExpressionNodeTypeMap.end()) {
            recordSyntaxError(recordToken, currentToken(), "Expected prefix token.");
            return nullptr;
        }
        next();
        auto expression = buildExpression(precedence);
        if (!expression) {
            recordSyntaxError(recordToken, currentToken(), "Expected expression");
            return nullptr;
        }
        return std::make_shared<PrefixExpressionNode>(recordToken, it->second, expression);
    }

    ExpressionNodePtr Parser::buildUnaryExpression() {
        auto opToken = currentToken();
        const auto &precedence = currentTokenPrecedence();
        next();
        const auto &expression = buildExpression(precedence);
        return std::make_shared<UnaryExpressionNode>(opToken, opToken, expression);
    }

    ExpressionNodePtr Parser::buildIntegerExpression() {
        return std::make_shared<IntegerLiteralNode>(currentToken());
    }

    ExpressionNodePtr Parser::buildBoolExpression(){
        return std::make_shared<BooleanLiteralNode>(currentToken());
    }

    ExpressionNodePtr Parser::buildBreakExpression(){
        return std::make_shared<BreakExpressionNode>(currentToken());
    }

    ExpressionNodePtr Parser::buildClassExpression(){
        const auto &classToken = currentToken();
        next();
        auto nameNode = buildExpression(Precedence::LOWEST);
        if (nextTokenIs(TokenType::TOKEN_NEWLINE)) {
            return std::make_shared<ClassDeclarationNode>(classToken, nameNode);
        }
        if (!expectedNextTokenAndConsume(TokenType::TOKEN_LBRACE)) {
            recordUnexpectedTokenTypeError(nextToken(), TokenType::TOKEN_LBRACE);
            return nullptr;
        }
        auto bodyNode = buildBraceExpression();
        return std::make_shared<ClassDefinitionNode>(classToken, nameNode, bodyNode);
    }

    ExpressionNodePtr Parser::buildContinueExpression(){
        return nullptr;
    }

    ExpressionNodePtr Parser::buildFloatExpression(){
        return std::make_shared<FloatLiteralNode>(currentToken());
    }

    ExpressionNodePtr Parser::buildFunctionExpression(){
        auto funToken = currentToken();
        const auto &precedence = currentTokenPrecedence();
        if (const auto &it = prefixExpressionNodeTypeMap.find(funToken.getType());
            it == prefixExpressionNodeTypeMap.end()) {
            recordSyntaxError(funToken, currentToken(), "Expected prefix token.");
            return nullptr;
            }
        next();
        auto callNode = buildExpression(precedence);
        if (!callNode) {
            recordSyntaxError(funToken, currentToken(), "Expected expression");
            return nullptr;
        }
        std::vector<std::shared_ptr<LabelNode>> labelNodes{};
        std::shared_ptr<Token> colonToken = nullptr;
        if (currentTokenIs(TokenType::TOKEN_COLON)) {
            colonToken = std::make_shared<Token>(currentToken());
            while (nextTokenIs(TokenType::TOKEN_LABEL)) {
                next();
                labelNodes.push_back(std::make_shared<LabelNode>(currentToken()));
            }
        }
        if (nextTokenIs(TokenType::TOKEN_NEWLINE))
        {
            // 函数声明
            if (callNode->getRealType() == NodeType::CALL)
            {
                if (const auto callFuncNode = std::static_pointer_cast<FunctionCallNode>(callNode);
                    callFuncNode->getRightNode()->getRealType() == NodeType::ANON_FUNCTION_DEFINITION) {
                    if (const auto &funcDefNode
                        = std::static_pointer_cast<AnonFunctionDefinitionNode>(
                            callFuncNode->getRightNode()))
                    {
                        return std::make_shared<FunctionDefinitionNode>(funToken, callNode, colonToken, labelNodes,
                            std::make_shared<Token>(funcDefNode->getIndicatorToken()),
                            funcDefNode->getBodyNode());
                    }
                 }
            } else if (callNode->getRealType() == NodeType::ANON_FUNCTION_DEFINITION &&
                callNode->getType() == NodeType::INFIX)
            {
                if (const auto &funcDefNode = std::static_pointer_cast<InfixExpressionNode>(callNode))
                {
                    return std::make_shared<FunctionDefinitionNode>(funToken, funcDefNode->getLeftNode(), colonToken, labelNodes,
                        std::make_shared<Token>(funcDefNode->getOpToken()),
                        funcDefNode->getRightNode());
                }
            }
            return std::make_shared<FunctionDeclarationNode>(funToken, callNode, colonToken, labelNodes);
        }
        std::shared_ptr<Token> indicatorToken = nullptr;
        if (nextTokenIs(TokenType::TOKEN_INDICATOR)){
            next();
            indicatorToken = std::make_shared<Token>(currentToken());
        }
        if (!expectedNextTokenAndConsume(TokenType::TOKEN_LBRACE)) {
            recordUnexpectedTokenTypeError(nextToken(), TokenType::TOKEN_LBRACE);
            return nullptr;
        }
        // 函数定义
        const auto &bodyNode = buildBraceExpression();
        return std::make_shared<FunctionDefinitionNode>(funToken, callNode, colonToken, labelNodes, indicatorToken, bodyNode);
    }

    ExpressionNodePtr Parser::buildIdentifierExpression(){
        // 创建标识符节点，但是需要根据后续节点判断是否为其他节点类型，例如函数调用、函数定义、类定义等
        const auto &nameToken = currentToken();
        if (nextTokenIs(TokenType::TOKEN_COLON)) {
            // 为变量添加标签
            next();
            if (nextTokenIs(TokenType::TOKEN_LABEL) || nextTokenIs(TokenType::TOKEN_IDENTIFIER)) {
                const auto &colonToken = currentToken();
                std::vector<std::shared_ptr<LabelNode> > labels{};
                while (nextTokenIs(TokenType::TOKEN_LABEL) || nextTokenIs(TokenType::TOKEN_IDENTIFIER)) {
                    next();
                    labels.push_back(std::make_shared<LabelNode>(currentToken()));
                }
                return std::make_shared<IdentifierNode>(nameToken, colonToken,
                                                        labels);
            }
            previous();
        }
        return std::make_shared<IdentifierNode>(nameToken);
    }

    ExpressionNodePtr Parser::buildIfExpression(){
        const auto &ifToken = currentToken();
        next();
        const auto &conditionNode = buildExpression(Precedence::LOWEST);
        if (!nextTokenIs(TokenType::TOKEN_LBRACE)) {
            recordUnexpectedTokenTypeError(nextToken(), TokenType::TOKEN_LBRACE);
            return nullptr;
        }
        next();
        const auto &bodyNode = buildBraceExpression();
        return std::make_shared<BranchNode>(ifToken, conditionNode, bodyNode);
    }

    ExpressionNodePtr Parser::buildConditionExpression() {
        std::vector<ExpressionNodePtr> branchNodes{};
        while (hasNext()) {
            if (currentTokenIs(TokenType::TOKEN_IF)) {
                // 处理初始 if 分支
                branchNodes.push_back(buildIfExpression());
            }
            else if (currentTokenIs(TokenType::TOKEN_ELIF)) {
                // 处理 elif 分支
                branchNodes.push_back(buildIfExpression());
            }
            else if (currentTokenIs(TokenType::TOKEN_ELSE)) {
                // 处理 else 分支
                const auto &elseToken = currentToken();
                next(); // 消耗 ELSE 标记

                if (currentTokenIs(TokenType::TOKEN_IF)) {
                    // 这是 else-if 分支，继续循环处理
                    continue;
                }

                // 这是普通 else 分支
                if (!currentTokenIs(TokenType::TOKEN_LBRACE)) {
                    recordUnexpectedTokenTypeError(currentToken(), TokenType::TOKEN_LBRACE);
                    return nullptr;
                }

                const auto &bodyNode = buildBraceExpression();
                const auto &elseBranchNode = std::make_shared<BranchNode>(
                    elseToken,
                    std::make_shared<BooleanLiteralNode>(
                        Token(elseToken.getPos(), "true")),
                    bodyNode
                );

                branchNodes.push_back(elseBranchNode);
                break; // else 分支是最后一个，退出循环
            }
            else if (currentTokenIs(TokenType::TOKEN_NEWLINE)) {
                // 遇到 "\n" 标记，结束处理
                break;
            }
            else {
                // 遇到不期望的标记类型
                recordUnexpectedTokenTypeError(currentToken(),
                    getTokenTypeName(TokenType::TOKEN_IF)
                    + " or " + getTokenTypeName(TokenType::TOKEN_ELIF)
                    + " or " + getTokenTypeName(TokenType::TOKEN_ELSE));
                return nullptr;
            }

            next(); // 移动到下一个标记
        }
        return std::make_shared<ConditionNode>(branchNodes);
    }


    ExpressionNodePtr Parser::buildImportExpression(){
        return nullptr;
    }

    ExpressionNodePtr Parser::buildLabelExpression() {
        return std::make_shared<LabelNode>(currentToken());
    }

    std::unordered_map<TokenType, std::pair<TokenType, NodeType>> rangerExpressionTypeMap = {
        {TokenType::TOKEN_LPAREN, {TokenType::TOKEN_RPAREN, NodeType::PAREN}},
        {TokenType::TOKEN_LBRACE, {TokenType::TOKEN_RBRACE, NodeType::BLOCK}},
        {TokenType::TOKEN_LBRACKET, {TokenType::TOKEN_RBRACKET, NodeType::BRACKET}},
    };

    ExpressionNodePtr Parser::buildRangerExpression() {
        const Token& beginToken = currentToken();
        const auto &it = rangerExpressionTypeMap.find(beginToken.getType());
        if (it == rangerExpressionTypeMap.end()) {
            recordUnexpectedTokenTypeError(beginToken,
                getTokenTypeName(TokenType::TOKEN_LPAREN)
                + " or " + getTokenTypeName(TokenType::TOKEN_LBRACE)
                + " or " + getTokenTypeName(TokenType::TOKEN_LBRACKET));
            return nullptr;
        }
        ExpressionNodePtr rangerNode = nullptr;
        if (currentTokenIs(TokenType::TOKEN_LPAREN)) {
            if (!nextTokenIs(TokenType::TOKEN_RPAREN)) {
                next(); // 跳过左括号
                rangerNode = buildExpression(Precedence::LOWEST);
                if (!nextTokenIs(TokenType::TOKEN_RPAREN)) {
                    recordUnclosedExpressionError(beginToken, nextToken(), TokenType::TOKEN_RPAREN);
                    return nullptr;
                }
            }
            next();
            rangerNode = std::make_shared<ParenRangerNode>(beginToken, currentToken(), rangerNode);
            // 需要检查是否是anno function
            std::vector<std::shared_ptr<LabelNode>> labels{};
            if (nextTokenIs(TokenType::TOKEN_COLON)) {
                next();
                if (nextTokenIs(TokenType::TOKEN_LABEL)){
                    /* FixMe: It is necessary to check here whether it is a custom type.
                    || nextTokenIs(TokenType::TOKEN_IDENTIFIER) */
                    const auto &colonToken = std::make_shared<Token>(currentToken());
                    while (nextTokenIs(TokenType::TOKEN_LABEL)) {
                        next();
                        labels.push_back(std::make_shared<LabelNode>(currentToken()));
                    }
                    if (nextTokenIs(TokenType::TOKEN_INDICATOR)) {
                        next();
                        const auto indicatorToken = currentToken();
                        next();
                        auto bodyNode = buildBraceExpression();
                        return std::make_shared<AnonFunctionDefinitionNode>(
                            indicatorToken, rangerNode,
                            colonToken, labels, indicatorToken, bodyNode);
                    }
                    for (const auto &_: labels) previous();
                }
            }
            return rangerNode;
        }
        if (currentTokenIs(TokenType::TOKEN_LBRACE)) {
            rangerNode = buildBraceExpression();
        }
        return rangerNode;
    }

    ExpressionNodePtr Parser::buildBraceExpression() {
        Token beginToken = currentToken();
        next();
        std::vector<std::shared_ptr<ExpressionNode>> nodes;
        while (hasNext() && !currentTokenIs(TokenType::TOKEN_RBRACE)) {
            if (!currentTokenIs(TokenType::TOKEN_NEWLINE)) {
                const auto &expression = buildExpression(Precedence::LOWEST);
                while (nextTokenIs(TokenType::TOKEN_NEWLINE)) next();
                nodes.push_back(expression);
            }
            next();
        }
        if (!currentTokenIs(TokenType::TOKEN_RBRACE)) {
            recordUnclosedExpressionError(beginToken, currentToken(), TokenType::TOKEN_RBRACE);
            return nullptr;
        }
        return std::make_shared<BlockRangerNode>(beginToken, currentToken(), nodes);
    }

    ExpressionNodePtr Parser::buildNullExpression(){
        return std::make_shared<NullLiteralNode>(currentToken());
    }

    ExpressionNodePtr Parser::buildReturnExpression(){
        const auto &returnToken = currentToken();
        ExpressionNodePtr expression = nullptr;
        if (!nextTokenIs(TokenType::TOKEN_NEWLINE))
        {
            next();
            expression = buildExpression(Precedence::LOWEST);
        }
        return std::make_shared<ReturnExpressionNode>(returnToken, expression);
    }

    ExpressionNodePtr Parser::buildStringExpression(){
        return std::make_shared<StringLiteralNode>(currentToken());
    }

    ExpressionNodePtr Parser::buildCharExpression() {
        return std::make_shared<CharacterLiteralNode>(currentToken());
    }

    ExpressionNodePtr Parser::buildSuperExpression() {
        return nullptr;
    }

    ExpressionNodePtr Parser::buildThisExpression() {
        return nullptr;
    }

    ExpressionNodePtr Parser::buildWhileExpression() {
        const auto &whileToken = currentToken();
        next();
        const auto &condition = buildExpression(Precedence::LOWEST);
        if (!nextTokenIs(TokenType::TOKEN_LBRACE)) {
            recordUnexpectedTokenTypeError(currentToken(), TokenType::TOKEN_LBRACE);
            return nullptr;
        }
        next();
        const auto &body = buildBraceExpression();
        return std::make_shared<WhileLoopNode>(whileToken, condition, body);
    }

    ExpressionNodePtr Parser::buildUntilExpression() {
        const auto &untilToken = currentToken();
        next();
        const auto &condition = buildExpression(Precedence::LOWEST);
        if (!nextTokenIs(TokenType::TOKEN_LBRACE)) {
            recordUnexpectedTokenTypeError(currentToken(), TokenType::TOKEN_LBRACE);
            return nullptr;
        }
        next();
        const auto &body = buildBraceExpression();
        return std::make_shared<UntilLoopNode>(untilToken, condition, body);
    }

    ExpressionNodePtr Parser::buildPassExpression() {
        return std::make_shared<PassExpressionNode>(currentToken());
    }

    ExpressionNodePtr Parser::buildEncapsulatedExpression() {
        return std::make_shared<EncapsulatedExpressionNode>(currentToken());
    }

    ExpressionNodePtr Parser::buildConstructorExpression() {
        const auto &ctorToken = currentToken();
        if (!expectedNextTokenAndConsume(TokenType::TOKEN_LPAREN)) {
            recordUnexpectedTokenTypeError(nextToken(), TokenType::TOKEN_LPAREN);
            return nullptr;
        }
        auto paramNode = buildExpression(Precedence::LOWEST);
        std::shared_ptr<Token> colonToken = nullptr;
        std::vector<std::shared_ptr<LabelNode>> labelNodes{};
        if (nextTokenIs(TokenType::TOKEN_COLON)) {
            next();
            colonToken = std::make_shared<Token>(currentToken());
            while (nextTokenIs(TokenType::TOKEN_LABEL)) {
                next();
                labelNodes.push_back(std::make_shared<LabelNode>(currentToken()));
            }
        }
        if (!expectedNextTokenAndConsume(TokenType::TOKEN_LBRACE)) {
            recordUnexpectedTokenTypeError(nextToken(), TokenType::TOKEN_LBRACE);
            return nullptr;
        }
        auto bodyNode = buildBraceExpression();
        return std::make_shared<ConstructorDefinitionNode>(ctorToken, paramNode, colonToken, labelNodes, bodyNode);
    }

    ExpressionNodePtr Parser::buildDictionaryExpression() {
        const auto &beginToken = currentToken();
        next();
        while (currentTokenIs(TokenType::TOKEN_NEWLINE)) next();
        auto bodyNode = buildExpression(currentTokenPrecedence());
        next();
        while (currentTokenIs(TokenType::TOKEN_NEWLINE)) next();
        if (!currentTokenIs(TokenType::TOKEN_RBRACE)) {
            recordUnexpectedTokenTypeError(nextToken(), TokenType::TOKEN_RBRACE);
            return nullptr;
        }
        return std::make_shared<DictionaryExpressionNode>(beginToken, currentToken(), bodyNode);
    }

    ExpressionNodePtr Parser::buildListExpression() {
        const auto &beginToken = currentToken();
        next();
        while (currentTokenIs(TokenType::TOKEN_NEWLINE)) next();
        ExpressionNodePtr bodyNode = nullptr;
        if (!currentTokenIs(TokenType::TOKEN_RBRACKET))
        {
            bodyNode = buildExpression(currentTokenPrecedence());
            next();
        }
        while (currentTokenIs(TokenType::TOKEN_NEWLINE)) next();
        if (!currentTokenIs(TokenType::TOKEN_RBRACKET)) {
            recordUnexpectedTokenTypeError(nextToken(), TokenType::TOKEN_RBRACKET);
            return nullptr;
        }
        return std::make_shared<ListExpressionNode>(beginToken, currentToken(), bodyNode);
    }

    ExpressionNodePtr Parser::buildVariableExpression() {
        typedef VariableDefinitionNode::VarDefData VarDefData;
        auto varToken = currentToken();
        next();
        const auto &expression = buildExpression(Precedence::LOWEST);
        if (!expression) {
            recordUnexpectedTokenTypeError(currentToken(), TokenType::TOKEN_IDENTIFIER);
            return nullptr;
        }

        std::vector<std::shared_ptr<VarDefData>> varDefs{};

        std::function<void(const ExpressionNodePtr &)> processExpressionNode =
            [&](const ExpressionNodePtr &exp) {
                if (exp->getType() == NodeType::INFIX) {
                    const auto infixNode = std::static_pointer_cast<InfixExpressionNode>(exp);
                    if (exp->getRealType() == NodeType::PARALLEL) {
                        processExpressionNode(infixNode->getLeftNode());
                        processExpressionNode(infixNode->getRightNode());
                    }
                    else {
                        pass("There has another condition when exp type was infix.");
                    }
                }
                else if (exp->getType() == NodeType::ASSIGNMENT) {
                    const auto &assignNode = std::static_pointer_cast<AssignmentNode>(exp);
                    const auto &[nameNode,
                        valueNode] = assignNode->getAssignPair();
                    if (nameNode->getType() != NodeType::IDENTIFIER) {
                        recordUnexpectedTokenTypeError(nameNode->getMainToken(), TokenType::TOKEN_IDENTIFIER);
                        return;
                    }
                    const auto &identNode = std::static_pointer_cast<IdentifierNode>(nameNode);
                    varDefs.push_back(std::make_shared<VarDefData>(identNode,
                        identNode->getColonTokenPtr() != nullptr,
                        identNode->getLabels(),
                        valueNode != nullptr, valueNode));
                }
                else if (exp->getType() == NodeType::IDENTIFIER) {
                    const auto &identNode = std::static_pointer_cast<IdentifierNode>(exp);
                    varDefs.push_back(std::make_shared<VarDefData>(identNode,
                        identNode->getColonTokenPtr() != nullptr,
                        identNode->getLabels(),
                        false,
                        nullptr));
                }
                else {
                    pass("another condition: " + getNodeTypeName(exp->getType()));
                }
        };

        processExpressionNode(expression);
        return std::make_shared<VariableDefinitionNode>(varToken, varDefs);
    }

    ExpressionNodePtr Parser::buildForExpression() {
        const auto& forToken = currentToken();
        next();
        const auto expression = buildExpression(Precedence::LOWEST);
        if (!validateForRangeExpression(expression, forToken)) {
            return nullptr;
        }
        const auto parenNode = std::static_pointer_cast<ParenRangerNode>(expression);
        const auto braceNode = std::static_pointer_cast<BlockRangerNode>(parenNode->getRangerNode());
        const auto& expressions = braceNode->getBodyExpressions();
        if (expressions.size() != 3) {
            recordSyntaxError(forToken, braceNode->getRangerStartToken(),
                "For statement requires exactly 3 expressions: init; condition; update");
            return nullptr;
        }
        const auto& initNode = expressions[0];
        const auto& conditionNode = expressions[1];
        const auto& updateNode = expressions[2];
        if (!expectedNextTokenAndConsume(TokenType::TOKEN_LBRACE)) {
            return nullptr;
        }
        auto bodyNode = buildBraceExpression();
        return std::make_shared<ForLoopNode>(forToken, initNode, conditionNode, updateNode, bodyNode);
    }

    bool Parser::validateForRangeExpression(ExpressionNodePtr expr, const Token& forToken) {
        if (!expr || expr->getType() != NodeType::RANGER) {
            recordSyntaxError(forToken, expr ? expr->getMainToken() : forToken,
                "For loop requires a range expression after 'for' keyword");
            return false;
        }
        auto exprNode = std::static_pointer_cast<RangerNode>(expr);
        if (exprNode->getRangerType() != NodeType::PAREN) {
            recordSyntaxError(forToken, expr->getMainToken(),
                "For loop requires parentheses around the loop control structure");
            return false;
        }
        auto rangerNode = std::static_pointer_cast<ParenRangerNode>(exprNode)->getRangerNode();
        if (rangerNode->getType() != NodeType::RANGER) {
            recordSyntaxError(forToken, exprNode->getRangerStartToken(),
                "For loop requires a range expression inside parentheses");
            return false;
        }
        auto blockNode = std::static_pointer_cast<RangerNode>(rangerNode);
        if (blockNode->getRangerType() != NodeType::BLOCK) {
            recordSyntaxError(forToken, blockNode->getRangerStartToken(),
                "For loop requires a block expression inside parentheses");
            return false;
        }
        return true;
    }

    ExpressionNodePtr Parser::buildBracketExpression() {
        const auto &beginToken = currentToken();
        next();
        auto indexNode = buildExpression(currentTokenPrecedence());
        if (!expectedNextTokenAndConsume(TokenType::TOKEN_RBRACKET)) {
            recordUnexpectedTokenTypeError(nextToken(), TokenType::TOKEN_LBRACKET);
            return nullptr;
        }
        return std::make_shared<BracketExpressionNode>(beginToken, currentToken(), indexNode);
    }
}

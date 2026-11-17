//
// Created by RestRegular on 2025/6/30.
//

#include "../../../../include/analyzer/rcc_ast.h"
#include "../../../../include/analyzer/rcc_parser.h"

namespace parser {
    static std::unordered_map<core::TokenType, NodeType> prefixExpressionNodeTypeMap = {
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

    ExpressionNodePtr Parser::buildRefExpression() {
        // P13-2/L3（决议 §7.2）：`&bob` 只接受 Body 变量（标识符），
        // 产出 Ref[T]。子引用 &p.name / 字面量 / 表达式暂不支持，明确报错。
        const auto opToken = currentToken();
        next();
        const auto operand = buildExpression(Precedence::LOWEST);
        // P13-2/L3-E（决议 §4.1）：`&p.name` 子引用（字段访问）也合法；
        // 字面量 / 运算表达式仍拒绝。
        const bool isIdent = std::dynamic_pointer_cast<IdentifierNode>(operand) != nullptr;
        const bool isField = [&] {
            if (const auto inf = std::dynamic_pointer_cast<InfixExpressionNode>(operand))
                return inf->getOpToken().getType() == TokenType::TOKEN_DOT;
            return false;
        }();
        if (!isIdent && !isField)        {
            recordSemanticError(opToken,
                "L3: '&' requires a Body variable or a field path (&p.name); "
                "literals and expressions are not supported");
            return nullptr;
        }
        return std::make_shared<RefExpressionNode>(opToken, operand);
    }

    ExpressionNodePtr Parser::buildIntegerExpression() {
        return std::make_shared<IntegerLiteralNode>(currentToken());
    }

    ExpressionNodePtr Parser::buildBoolExpression(){
        return std::make_shared<BooleanLiteralNode>(currentToken());
    }

    ExpressionNodePtr Parser::buildBreakExpression(){
        // P13-2/L2(Q-C)：`break value` 收集携带值（AST 的 valueNode）。
        // 语义校验移到 codegen：无 `: T` 标注的循环里 break value 才报错
        // （防止值被静默丢弃），有标注时参与值合流。
        // 决议 §12.1：循环体块的 ret value 等价 break value，
        // 所以 emitRetFromLoop 与 emitBreak 共用同一套值槽机制。
        const auto breakToken = currentToken();
        const auto t = breakToken.getType();
        const bool hasValue =
            !nextTokenIs(TokenType::TOKEN_NEWLINE) &&
            !nextTokenIs(TokenType::TOKEN_RBRACE) &&
            !nextTokenIs(TokenType::TOKEN_STREAM_END) &&
            t != TokenType::TOKEN_STREAM_END;
        ExpressionNodePtr value = nullptr;
        if (hasValue)        {
            next();
            value = buildExpression(Precedence::LOWEST);
        }
        return std::make_shared<BreakExpressionNode>(breakToken, value);
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
        // 原先直接返回 nullptr，导致 `continue` 永远报
        // "failure to find the prefix expression construction function"。
        return std::make_shared<ContinueExpressionNode>(currentToken());
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
        // P13-2/L0-3：`fun ctor(...)` 必须报错 —— ctor 是保留名，
        // 定义处必须省略 fun。实测 AST：`fun ctor(n: int) {...}` 解析成
        // FunctionDeclarationNode(callNode=ConstructorDefinitionNode)，
        // 所以它走的是 buildFunctionExpression 的声明分支（nextTokenIs NEWLINE）。
        // 这里在 callNode 上识别 ConstructorDefinitionNode / 名为 ctor 的标识符即可。
        const bool wrapsCtor =
            std::dynamic_pointer_cast<ConstructorDefinitionNode>(callNode) != nullptr ||
            [&] {
                if (const auto call = std::dynamic_pointer_cast<FunctionCallNode>(callNode))            {
                    if (const auto target = std::dynamic_pointer_cast<IdentifierNode>(call->getLeftNode()))
                        return target->getName() == "ctor";
                }
                if (const auto id = std::dynamic_pointer_cast<IdentifierNode>(callNode))
                    return id->getName() == "ctor";
                return false;
            }();
        if (wrapsCtor)        {
            recordSemanticError(callNode->getMainToken(),
                "L0-3: 'ctor' is a reserved name; define constructors without 'fun'");
            return nullptr;
        }
        std::vector<std::shared_ptr<LabelNode>> labelNodes{};
        std::shared_ptr<Token> colonToken = nullptr;
        if (currentTokenIs(TokenType::TOKEN_COLON)) {
            colonToken = std::make_shared<Token>(currentToken());
            while (nextTokenIs(TokenType::TOKEN_LABEL) || nextTokenIs(TokenType::TOKEN_IDENTIFIER)) {
                next();
                labelNodes.push_back(std::static_pointer_cast<LabelNode>(buildLabelExpression()));
            }
        }
        // P13-2/L0-1：签名子句白名单。
        // 实测 `: int garbage[a]` 会被上面的循环收进 labelNodes：
        //   label[0]="int"（返回类型），label[1]="garbage[a]"（带 [...] 的描述）。
        // 决议 §9.1：返回类型之后只允许 from[...] / writes[...] / ensures[...]，
        // 且不允许空方括号。任何其它名字一律编译报错。
        // 因为 `int` 等类型是 TOKEN_LABEL 而 from 等是 TOKEN_IDENTIFIER，
        // 但实参名也可能是 LABEL，这里只看 label[1..] 的**名字**。
        static const std::unordered_set<std::string> allowedClauses = {
            "from", "writes", "ensures"
        };
        for (size_t i = 1; i < labelNodes.size(); ++i)        {
            const auto& label = labelNodes[i];
            if (!label) continue;
            const std::string name = label->getFullLabel();
            if (!allowedClauses.contains(name))            {
                recordSemanticError(label->getMainToken(),
                    "L0-1: unknown signature clause '" + name +
                    "'; expected from[...], writes[...] or ensures[...]");
                return nullptr;
            }
            const auto des = label->getLabelDesNodes();
            if (des.empty() || !des[0] || !des[0]->getBodyNode())            {
                recordSemanticError(label->getMainToken(),
                    "L0-1: signature clause '" + name + "' requires a non-empty [...] list");
                return nullptr;
            }
        }
        // P13-2/L3-F：把签名子句从返回类型标签里分离，存进 FunctionDefinitionNode。
        std::vector<std::shared_ptr<LabelNode>> signatureClauses;
        if (labelNodes.size() > 1)        {
            signatureClauses.assign(labelNodes.begin() + 1, labelNodes.end());
            labelNodes.resize(1); // 只保留 label[0] = 返回类型（可能为空）
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
                            funcDefNode->getBodyNode(), signatureClauses);
                    }
                 }
            } else if (callNode->getRealType() == NodeType::ANON_FUNCTION_DEFINITION &&
                callNode->getType() == NodeType::INFIX)
            {
                if (const auto &funcDefNode = std::static_pointer_cast<InfixExpressionNode>(callNode))
                {
                    return std::make_shared<FunctionDefinitionNode>(funToken, funcDefNode->getLeftNode(), colonToken, labelNodes,
                        std::make_shared<Token>(funcDefNode->getOpToken()),
                        funcDefNode->getRightNode(), signatureClauses);
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
        return std::make_shared<FunctionDefinitionNode>(funToken, callNode, colonToken, labelNodes, indicatorToken, bodyNode, signatureClauses);
    }

    ExpressionNodePtr Parser::buildIdentifierExpression(){
        // 创建标识符节点，但是需要根据后续节点判断是否为其他节点类型，例如函数调用、函数定义、类定义等
        const auto &nameToken = currentToken();
        if (nextTokenIs(TokenType::TOKEN_COLON)) {
            // 为变量添加标签
            next();
            if (nextTokenIs(TokenType::TOKEN_LABEL) || nextTokenIs(TokenType::TOKEN_IDENTIFIER)) {
                const auto &colonToken = currentToken();
                std::vector<std::shared_ptr<LabelNode>> labels{};
                while (nextTokenIs(TokenType::TOKEN_LABEL) || nextTokenIs(TokenType::TOKEN_IDENTIFIER)) {
                    next();
                    if (const auto &labelNode = buildLabelExpression();
                        labelNode->getType() == NodeType::LABEL)
                    {
                        labels.push_back(std::static_pointer_cast<LabelNode>(labelNode));
                    }
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
        auto condition = buildExpression(Precedence::LOWEST);
        // P13-2/L2(Q-C)：`if (cond): T { ... }` 的 `: T` 标注。
        // buildExpression 会把 `:` 当 infix 吃进 condition，形成
        //   InfixExpressionNode(op=":", left=cond, right=LabelNode(T))。
        // 这里把它拆开：左侧作为真 condition，右侧作为结果类型标注。
        std::vector<std::shared_ptr<LabelNode>> resultLabels{};
        if (const auto infix = std::dynamic_pointer_cast<InfixExpressionNode>(condition))            {
            if (infix->getOpToken().getType() == TokenType::TOKEN_COLON)                {
                if (const auto label = std::dynamic_pointer_cast<LabelNode>(infix->getRightNode()))
                    resultLabels.push_back(label);
                condition = infix->getLeftNode();
            }
        }
        // 决议 §12.2 的 `if (cond): T { ... }` 形式：buildExpression 解析完
        // `(cond)` 后 currentToken 停在 `:`（不是 nextToken），next 是 T。
        if (currentTokenIs(TokenType::TOKEN_COLON))        {
            while (nextTokenIs(TokenType::TOKEN_LABEL) ||
                   nextTokenIs(TokenType::TOKEN_IDENTIFIER))            {
                next();
                if (const auto label = std::dynamic_pointer_cast<LabelNode>(buildLabelExpression()))
                    resultLabels.push_back(label);
            }
        }


        if (!nextTokenIs(TokenType::TOKEN_LBRACE)) {
            recordUnexpectedTokenTypeError(nextToken(), TokenType::TOKEN_LBRACE);
            return nullptr;
        }
        next();
        const auto &bodyNode = buildBraceExpression();
        return std::make_shared<BranchNode>(ifToken, condition, bodyNode, resultLabels);
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
        if (hasError()) return nullptr;
        // P13-2/L2(Q-C)：把第一个分支的 `: T` 提升为整个 if 的结果标注。
        // 决议 §12.2：`: T` 是权威校验，所有正常出口必须产出 T。
        auto condition = std::make_shared<ConditionNode>(branchNodes);
        if (!branchNodes.empty())        {
            if (const auto first = std::dynamic_pointer_cast<BranchNode>(branchNodes.front()))                {
                condition->setResultLabels(first->getResultLabels());
            }
        }
        return condition;
    }


    ExpressionNodePtr Parser::buildImportExpression(){
        return nullptr;
    }

    ExpressionNodePtr Parser::buildLabelExpression() {
        if (currentTokenIs(TokenType::TOKEN_LABEL) || currentTokenIs(TokenType::TOKEN_IDENTIFIER))
        {
            const auto &labelNode = std::make_shared<LabelNode>(std::vector{currentToken()});
            while (true)
            {
                if (nextTokenIs(TokenType::TOKEN_LBRACKET)) {
                    next();
                    if (const auto &labelDesNode = buildListExpression())
                    {
                        labelNode->appendLabelDesNode(std::static_pointer_cast<ListExpressionNode>(labelDesNode));
                    }
                } else if (nextTokenIs(TokenType::TOKEN_DOT))
                {
                    next();
                    if (nextTokenIs(TokenType::TOKEN_IDENTIFIER) || nextTokenIs(TokenType::TOKEN_LABEL))
                    {
                        labelNode->appendLabelPath(nextToken());
                        next();
                    } else
                    {
                        recordUnexpectedTokenTypeError(nextToken(), vectorJoin(
                            {getTokenTypeName(TokenType::TOKEN_IDENTIFIER),
                                getTokenTypeName(TokenType::TOKEN_LABEL)}));
                        return nullptr;
                    }
                } else
                {
                    break;
                }
            }
            return labelNode;
        }
        return nullptr;
    }

    std::unordered_map<core::TokenType, std::pair<core::TokenType, NodeType>> rangerExpressionTypeMap = {
        {TokenType::TOKEN_LPAREN, {TokenType::TOKEN_RPAREN, NodeType::PAREN}},
        {TokenType::TOKEN_LBRACE, {TokenType::TOKEN_RBRACE, NodeType::BLOCK}},
        {TokenType::TOKEN_LBRACKET, {TokenType::TOKEN_RBRACKET, NodeType::BRACKET}},
    };

    ExpressionNodePtr Parser::buildRangerExpression() {
        const Token& beginToken = currentToken();
        if (const auto &it = rangerExpressionTypeMap.find(beginToken.getType());
            it == rangerExpressionTypeMap.end()) {
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
                skipCurrentNewLineToken();
                rangerNode = buildExpression(Precedence::LOWEST);
                skipNextNewLineToken();
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
                if (nextTokenIs(TokenType::TOKEN_LABEL) || nextTokenIs(TokenType::TOKEN_IDENTIFIER)){
                    const auto &colonToken = std::make_shared<Token>(currentToken());
                    while (nextTokenIs(TokenType::TOKEN_LABEL) || nextTokenIs(TokenType::TOKEN_IDENTIFIER)) {
                        next();
                        labels.push_back(std::make_shared<LabelNode>(std::vector{currentToken()}));
                    }
                    if (nextTokenIs(TokenType::TOKEN_INDICATOR)) {
                        next();
                        const auto indicatorToken = currentToken();
                        next();
                        std::shared_ptr<ExpressionNode> bodyNode = nullptr;
                        if (currentTokenIs(TokenType::TOKEN_LBRACE))
                        {
                            bodyNode = buildBraceExpression();
                        } else if (currentTokenIs(TokenType::TOKEN_RETURN) ||
                                   currentTokenIs(TokenType::TOKEN_RET))
                        {
                            bodyNode = buildReturnExpression();
                            skipNextNewLineToken();
                        } else
                        {
                            const auto &expression = buildExpression(Precedence::LOWEST);
                            // P13-2/L1：匿名函数的简写 `-> expr` 等价于 `-> ret expr`，
                            // 退最近作用域，所以 exitsFunction=false。
                            bodyNode = std::make_shared<ReturnExpressionNode>(expression->getMainToken(), expression, false);
                            skipNextNewLineToken();
                        }
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
        // P12/G7：`ret` 后面可以没有表达式，不只有「换行」一种情况。
        // `fun f(): void { ret }` 写在同一行时，`ret` 后面直接是 `}`，
        // 旧实现会把它当表达式起始，于是报 TOKEN_RBRACE 找不到前缀构造。
        // 手写 Rio 代码几乎一定会踩这个坑，而收益极大。
        const auto t = currentToken().getType();
        const bool noValue =
            nextTokenIs(TokenType::TOKEN_NEWLINE) ||
            nextTokenIs(TokenType::TOKEN_RBRACE) ||
            nextTokenIs(TokenType::TOKEN_STREAM_END) ||
            t == TokenType::TOKEN_STREAM_END;
        if (!noValue)
        {
            next();
            expression = buildExpression(Precedence::LOWEST);
        }
        // P13-2/L1：`return` 只退函数（exitsFunction=true），
        // `ret` 退最近可返回值作用域（exitsFunction=false）。
        const bool exitsFunction = currentTokenIs(TokenType::TOKEN_RETURN);
        return std::make_shared<ReturnExpressionNode>(returnToken, expression, exitsFunction);
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
        auto condition = buildExpression(Precedence::LOWEST);
        // P13-2/L2(Q-C)：`while (cond): T { ... }` 的 `: T` 标注。
        // 冒号是 infix，若被吃进 condition 则拆出右侧 LabelNode；
        // 若 currentToken 停在冒号则直接消费其后的 label。
        std::vector<std::shared_ptr<LabelNode>> resultLabels{};
        if (const auto infix = std::dynamic_pointer_cast<InfixExpressionNode>(condition)) {
            if (infix->getOpToken().getType() == TokenType::TOKEN_COLON) {
                if (const auto label = std::dynamic_pointer_cast<LabelNode>(infix->getRightNode()))
                    resultLabels.push_back(label);
                condition = infix->getLeftNode();
            }
        }
        if (currentTokenIs(TokenType::TOKEN_COLON)) {
            while (nextTokenIs(TokenType::TOKEN_LABEL) || nextTokenIs(TokenType::TOKEN_IDENTIFIER)) {
                next();
                if (const auto label = std::dynamic_pointer_cast<LabelNode>(buildLabelExpression()))
                    resultLabels.push_back(label);
            }
        }
        if (!nextTokenIs(TokenType::TOKEN_LBRACE)) {
            recordUnexpectedTokenTypeError(currentToken(), TokenType::TOKEN_LBRACE);
            return nullptr;
        }
        next();
        const auto &body = buildBraceExpression();
        return std::make_shared<WhileLoopNode>(whileToken, condition, body, resultLabels);
    }

    ExpressionNodePtr Parser::buildUntilExpression() {
        const auto &untilToken = currentToken();
        next();
        auto condition = buildExpression(Precedence::LOWEST);
        std::vector<std::shared_ptr<LabelNode>> resultLabels{};
        if (const auto infix = std::dynamic_pointer_cast<InfixExpressionNode>(condition)) {
            if (infix->getOpToken().getType() == TokenType::TOKEN_COLON) {
                if (const auto label = std::dynamic_pointer_cast<LabelNode>(infix->getRightNode()))
                    resultLabels.push_back(label);
                condition = infix->getLeftNode();
            }
        }
        if (currentTokenIs(TokenType::TOKEN_COLON)) {
            while (nextTokenIs(TokenType::TOKEN_LABEL) || nextTokenIs(TokenType::TOKEN_IDENTIFIER)) {
                next();
                if (const auto label = std::dynamic_pointer_cast<LabelNode>(buildLabelExpression()))
                    resultLabels.push_back(label);
            }
        }
        if (!nextTokenIs(TokenType::TOKEN_LBRACE)) {
            recordUnexpectedTokenTypeError(currentToken(), TokenType::TOKEN_LBRACE);
            return nullptr;
        }
        next();
        const auto &body = buildBraceExpression();
        return std::make_shared<UntilLoopNode>(untilToken, condition, body, resultLabels);
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
                labelNodes.push_back(std::make_shared<LabelNode>(std::vector{currentToken()}));
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
        // P13-2/L1(Q-D)：`{` 前缀按结构分流——顶层有 `:` 的是字典，
        // 否则是裸块（语句序列）。空 `{}` 一律是空字典。
        if (!peekBraceIsDictionary()) {
            // 裸块：当前 token 仍是 `{`，buildBraceExpression 会消费它并
            // 走与函数体相同的语句列表解析，分号分隔语句因此自动可用（Q-D）。
            return buildBraceExpression();
        }
        next();
        skipCurrentNewLineToken();
        if (currentTokenIs(TokenType::TOKEN_RBRACE))
        {
            return std::make_shared<DictionaryExpressionNode>(beginToken, currentToken(), nullptr);
        }
        auto bodyNode = buildExpression(currentTokenPrecedence());
        next();
        skipCurrentNewLineToken();
        if (!currentTokenIs(TokenType::TOKEN_RBRACE)) {
            recordUnexpectedTokenTypeError(nextToken(), TokenType::TOKEN_RBRACE);
            return nullptr;
        }
        return std::make_shared<DictionaryExpressionNode>(beginToken, currentToken(), bodyNode);
    }

    ExpressionNodePtr Parser::buildListExpression() {
        const auto &beginToken = currentToken();
        next();
        skipCurrentNewLineToken();
        ExpressionNodePtr bodyNode = nullptr;
        if (!currentTokenIs(TokenType::TOKEN_RBRACKET))
        {
            bodyNode = buildExpression(Precedence::LOWEST);
            next();
        }
        skipCurrentNewLineToken();
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

    ExpressionNodePtr Parser::skipNewLineExpression()
    {
        next();
        skipCurrentNewLineToken();
        const auto &mainToken = currentToken();
        if (const auto &prefixBuilder = prefixExpressionBuilders.find(currentToken().getType());
            prefixBuilder != prefixExpressionBuilders.end())
        {
            return prefixBuilder->second(this);
        }
        recordPrefixBuilderNotFoundError(mainToken, mainToken.getType());
        return nullptr;
    }

    ExpressionNodePtr Parser::buildTryExpression()
    {
        const auto& mainToken = currentToken();
        skipNextNewLineToken();
        if (!nextTokenIs(TokenType::TOKEN_LBRACE))
        {
            recordUnexpectedTokenTypeError(nextToken(), TokenType::TOKEN_LBRACE);
            return nullptr;
        }
        next();
        const auto& tryBodyNode = buildRangerExpression();
        if (!currentTokenIs(TokenType::TOKEN_RBRACE))
        {
            recordUnexpectedTokenTypeError(currentToken(), TokenType::TOKEN_RBRACE);
            return nullptr;
        }
        const auto& tryBody = std::static_pointer_cast<BlockRangerNode>(tryBodyNode);
        skipNextNewLineToken();
        std::vector<std::pair<std::shared_ptr<IdentifierNode>,
            std::shared_ptr<BlockRangerNode>>> catchBodies {};
        while (nextTokenIs(TokenType::TOKEN_CATCH))
        {
            if (nextTokenIs(TokenType::TOKEN_CATCH))
            {
                next();
                const auto& catchToken = currentToken();
                if (!nextTokenIs(TokenType::TOKEN_LPAREN))
                {
                    recordUnexpectedTokenTypeError(nextToken(), TokenType::TOKEN_LPAREN);
                    return nullptr;
                }
                next();
                const auto& catchParenNode = buildRangerExpression();
                std::shared_ptr<IdentifierNode> catchParamNode = nullptr;
                if (!currentTokenIs(TokenType::TOKEN_RPAREN))
                {
                    recordUnexpectedTokenTypeError(currentToken(), TokenType::TOKEN_RPAREN);
                    return nullptr;
                }
                if (catchParenNode && catchParenNode->getRealType() == NodeType::RANGER)
                {
                    const auto& catchParen = std::static_pointer_cast<ParenRangerNode>(catchParenNode);
                    if (const auto& catchParamNode_ = catchParen->getRangerNode();
                        catchParamNode_->getRealType() == NodeType::IDENTIFIER)
                    {
                        catchParamNode = std::static_pointer_cast<IdentifierNode>(catchParamNode_);
                    } else
                    {
                        recordSyntaxError(catchToken, catchParamNode_->getMainToken(), "Invalid catch item.");
                        return nullptr;
                    }
                } else
                {
                    recordSyntaxError(catchToken, catchParenNode->getMainToken(), "Invalid catch item.");
                    return nullptr;
                }
                next();
                if (!currentTokenIs(TokenType::TOKEN_LBRACE))
                {
                    recordUnexpectedTokenTypeError(currentToken(), TokenType::TOKEN_LBRACE);
                    return nullptr;
                }
                const auto& catchBodyNode = buildRangerExpression();
                if (!currentTokenIs(TokenType::TOKEN_RBRACE))
                {
                    recordUnexpectedTokenTypeError(currentToken(), TokenType::TOKEN_RBRACE);
                    return nullptr;
                }
                const auto& catchBody = std::static_pointer_cast<BlockRangerNode>(catchBodyNode);
                catchBodies.push_back({catchParamNode, catchBody});
            }
            skipNextNewLineToken();
        }
        std::shared_ptr<BlockRangerNode> finallyBody = {};
        if (nextTokenIs(TokenType::TOKEN_FINALLY))
        {
            next();
            if (!nextTokenIs(TokenType::TOKEN_LBRACE))
            {
                recordUnexpectedTokenTypeError(nextToken(), TokenType::TOKEN_LBRACE);
                return nullptr;
            }
            next();
            const auto& finallyBodyNode = buildRangerExpression();
            if (!currentTokenIs(TokenType::TOKEN_RBRACE))
            {
                recordUnexpectedTokenTypeError(currentToken(), TokenType::TOKEN_RBRACE);
                return nullptr;
            }
            finallyBody = std::static_pointer_cast<BlockRangerNode>(finallyBodyNode);
        }
        return std::make_shared<TryNode>(mainToken, tryBody, catchBodies, finallyBody);
    }

    ExpressionNodePtr Parser::buildThrowExpression()
    {
        const auto& mainToken = currentToken();
        next();
        const auto& throwExpression = buildExpression(Precedence::LOWEST);
        return std::make_shared<ThrowNode>(mainToken, throwExpression);
    }

    ExpressionNodePtr Parser::buildForExpression() {
        const auto& forToken = currentToken();
        if (!expectedNextTokenAndConsume(TokenType::TOKEN_LPAREN))
        {
            recordUnexpectedTokenTypeError(nextToken(), TokenType::TOKEN_LPAREN);
            return nullptr;
        }
        if (!expectedNextTokenAndConsume(TokenType::TOKEN_LBRACE))
        {
            recordUnexpectedTokenTypeError(nextToken(), TokenType::TOKEN_LBRACE);
            return nullptr;
        }
        const auto expression = buildBraceExpression();
        if (!expectedNextTokenAndConsume(TokenType::TOKEN_RPAREN))
        {
            recordUnexpectedTokenTypeError(nextToken(), TokenType::TOKEN_RPAREN);
            return nullptr;
        }
        if (!validateForRangeExpression(expression, forToken))
        {
            return nullptr;
        }
        const auto braceNode = std::static_pointer_cast<BlockRangerNode>(expression);
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

    bool Parser::validateForRangeExpression(const ExpressionNodePtr& expr, const Token& forToken) {
        if (!expr || expr->getType() != NodeType::RANGER) {
            recordSyntaxError(forToken, expr ? expr->getMainToken() : forToken,
                "For loop requires a range expression after 'for' keyword");
            return false;
        }
        if (const auto blockNode = std::static_pointer_cast<RangerNode>(expr);
            blockNode->getRangerType() != NodeType::BLOCK) {
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

    void enableDebugMode(bool cond)
    {

    }
}

//
// rcc_ast_dump.cpp —— AST 树状 dump 的实现
//

#include <sstream>
#include <string>
#include <vector>

#include "../include/analyzer/rcc_ast_components.h"
#include "../include/analyzer/rcc_ast_dump.h"

namespace ast
{
    namespace
    {
        std::string indentOf(size_t level)
        {
            return std::string(level * 2, ' ');
        }

        std::string tokenText(const core::Token& token)
        {
            const std::string& v = token.getValue();
            if (v == "\n") return "\\n";
            return "\"" + v + "\"";
        }

        // 节点种类：具体 C++ 类型名 + NodeType 枚举名，两者都可能有用。
        // 容器节点的 NodeType 是 BLOCK（历史遗留），所以类型名优先。
        std::string describe(const std::shared_ptr<ExpressionNode>& node);

        void dumpNode(const std::shared_ptr<ExpressionNode>& node,
                      size_t level,
                      std::ostringstream& oss);

        // 统一的子节点输出：`key: <递归>`，子节点为空时打 <null>
        void dumpChild(const std::string& key,
                       const std::shared_ptr<ExpressionNode>& child,
                       size_t level,
                       std::ostringstream& oss)
        {
            oss << indentOf(level) << key << ": ";
            if (!child)
            {
                oss << "<null>\n";
                return;
            }
            oss << "\n";
            dumpNode(child, level + 1, oss);
        }

        // 类型标注里的 `[...]` 实参（`list[float]` 的 float）。
        // getFullLabel() 只拼 labelPath，`[...]` 全存在 labelDesNodes 里，
        // 不 dump 出来就没法判断后端到底能拿到什么。
        void dumpLabelDesNodes(const std::shared_ptr<LabelNode>& label,
                               size_t level,
                               std::ostringstream& oss)
        {
            const auto desNodes = label->getLabelDesNodes();
            for (size_t i = 0; i < desNodes.size(); ++i)
            {
                oss << indentOf(level) << "typeArg[" << i << "]:\n";
                if (!desNodes[i])
                {
                    oss << indentOf(level + 1) << "<null>\n";
                    continue;
                }
                dumpChild("body", desNodes[i]->getBodyNode(), level + 1, oss);
            }
        }

        std::string describe(const std::shared_ptr<ExpressionNode>& node)
        {
            if (!node) return "<null>";

            auto name = [&](const char* concrete) -> std::string
            {
                return std::string(concrete) + " (NodeType=" + getNodeTypeName(node->getType()) + ")";
            };

            if (std::dynamic_pointer_cast<IntegerLiteralNode>(node)) return name("IntegerLiteralNode");
            if (std::dynamic_pointer_cast<FloatLiteralNode>(node)) return name("FloatLiteralNode");
            if (std::dynamic_pointer_cast<StringLiteralNode>(node)) return name("StringLiteralNode");
            if (std::dynamic_pointer_cast<BooleanLiteralNode>(node)) return name("BooleanLiteralNode");
            if (std::dynamic_pointer_cast<CharacterLiteralNode>(node)) return name("CharacterLiteralNode");
            if (std::dynamic_pointer_cast<NullLiteralNode>(node)) return name("NullLiteralNode");
            if (std::dynamic_pointer_cast<IdentifierNode>(node)) return name("IdentifierNode");
            if (std::dynamic_pointer_cast<LabelNode>(node)) return name("LabelNode");
            if (std::dynamic_pointer_cast<IndexExpressionNode>(node)) return name("IndexExpressionNode");
            if (std::dynamic_pointer_cast<BracketExpressionNode>(node)) return name("BracketExpressionNode");
            if (std::dynamic_pointer_cast<ParenRangerNode>(node)) return name("ParenRangerNode");
            if (std::dynamic_pointer_cast<BlockRangerNode>(node)) return name("BlockRangerNode");
            if (std::dynamic_pointer_cast<DictionaryExpressionNode>(node)) return name("DictionaryExpressionNode");
            if (std::dynamic_pointer_cast<ListExpressionNode>(node)) return name("ListExpressionNode");
            if (std::dynamic_pointer_cast<PairExpressionNode>(node)) return name("PairExpressionNode");
            if (std::dynamic_pointer_cast<AssignmentNode>(node)) return name("AssignmentNode");
            if (std::dynamic_pointer_cast<VariableDefinitionNode>(node)) return name("VariableDefinitionNode");
            if (std::dynamic_pointer_cast<FunctionCallNode>(node)) return name("FunctionCallNode");
            if (std::dynamic_pointer_cast<FunctionDefinitionNode>(node)) return name("FunctionDefinitionNode");
            if (std::dynamic_pointer_cast<FunctionDeclarationNode>(node)) return name("FunctionDeclarationNode");
            if (std::dynamic_pointer_cast<AnonFunctionDefinitionNode>(node)) return name("AnonFunctionDefinitionNode");
            if (std::dynamic_pointer_cast<ConstructorDefinitionNode>(node)) return name("ConstructorDefinitionNode");
            if (std::dynamic_pointer_cast<ClassDefinitionNode>(node)) return name("ClassDefinitionNode");
            if (std::dynamic_pointer_cast<ClassDeclarationNode>(node)) return name("ClassDeclarationNode");
            if (std::dynamic_pointer_cast<ReturnExpressionNode>(node)) return name("ReturnExpressionNode");
            if (std::dynamic_pointer_cast<BreakExpressionNode>(node)) return name("BreakExpressionNode");
            if (std::dynamic_pointer_cast<ContinueExpressionNode>(node)) return name("ContinueExpressionNode");
            if (std::dynamic_pointer_cast<ForLoopNode>(node)) return name("ForLoopNode");
            if (std::dynamic_pointer_cast<WhileLoopNode>(node)) return name("WhileLoopNode");
            if (std::dynamic_pointer_cast<UntilLoopNode>(node)) return name("UntilLoopNode");
            if (std::dynamic_pointer_cast<BranchNode>(node)) return name("BranchNode");
            if (std::dynamic_pointer_cast<ConditionNode>(node)) return name("ConditionNode");
            if (std::dynamic_pointer_cast<PassExpressionNode>(node)) return name("PassExpressionNode");
            if (std::dynamic_pointer_cast<EncapsulatedExpressionNode>(node)) return name("EncapsulatedExpressionNode");
            if (std::dynamic_pointer_cast<TryNode>(node)) return name("TryNode");
            if (std::dynamic_pointer_cast<ThrowNode>(node)) return name("ThrowNode");
            if (std::dynamic_pointer_cast<PostfixExpressionNode>(node)) return name("PostfixExpressionNode");
            if (std::dynamic_pointer_cast<UnaryExpressionNode>(node)) return name("UnaryExpressionNode");
            if (std::dynamic_pointer_cast<PrefixExpressionNode>(node)) return name("PrefixExpressionNode");
            if (std::dynamic_pointer_cast<InfixExpressionNode>(node)) return name("InfixExpressionNode");
            return name("UnknownExpressionNode");
        }

        void dumpNode(const std::shared_ptr<ExpressionNode>& node,
                      size_t level,
                      std::ostringstream& oss)
        {
            if (!node)
            {
                oss << indentOf(level) << "<null>\n";
                return;
            }

            oss << indentOf(level) << describe(node) << "\n";

            // ---------- 字面量 ----------
            if (const auto lit = std::dynamic_pointer_cast<LiteralNode>(node))
            {
                oss << indentOf(level + 1) << "value: " << lit->literalString() << "\n";
                return;
            }

            // ---------- 标识符 / 标签 ----------
            if (const auto label = std::dynamic_pointer_cast<LabelNode>(node))
            {
                oss << indentOf(level + 1) << "text: " << label->getFullLabel() << "\n";
                dumpLabelDesNodes(label, level + 1, oss);
                return;
            }
            if (const auto id = std::dynamic_pointer_cast<IdentifierNode>(node))
            {
                oss << indentOf(level + 1) << "name: " << id->getName() << "\n";
                if (id->getColonTokenPtr())
                {
                    oss << indentOf(level + 1) << "hasTypeAnnotation: true\n";
                }
                for (const auto& label : id->getLabels())
                {
                    if (!label) continue;
                    oss << indentOf(level + 1) << "label: " << label->getFullLabel() << "\n";
                    dumpLabelDesNodes(label, level + 2, oss);
                }
                return;
            }

            // ---------- 中缀（含调用） ----------
            if (const auto infix = std::dynamic_pointer_cast<InfixExpressionNode>(node))
            {
                oss << indentOf(level + 1) << "op: " << tokenText(infix->getOpToken()) << "\n";
                dumpChild("left", infix->getLeftNode(), level + 1, oss);
                dumpChild("right", infix->getRightNode(), level + 1, oss);
                return;
            }

            // ---------- 后缀 / 索引 ----------
            if (const auto idx = std::dynamic_pointer_cast<IndexExpressionNode>(node))
            {
                dumpChild("target", idx->getLeftNode(), level + 1, oss);
                dumpChild("index", idx->getIndexNode(), level + 1, oss);
                return;
            }
            if (const auto post = std::dynamic_pointer_cast<PostfixExpressionNode>(node))
            {
                dumpChild("node", post->getNode(), level + 1, oss);
                return;
            }

            // ---------- 一元 / 前缀 ----------
            if (const auto unary = std::dynamic_pointer_cast<UnaryExpressionNode>(node))
            {
                oss << indentOf(level + 1) << "op: " << tokenText(unary->getOpToken()) << "\n";
                dumpChild("right", unary->getRightNode(), level + 1, oss);
                return;
            }
            if (const auto prefix = std::dynamic_pointer_cast<PrefixExpressionNode>(node))
            {
                oss << indentOf(level + 1) << "prefixType: " << getNodeTypeName(prefix->getPrefixType()) << "\n";
                dumpChild("node", prefix->getNode(), level + 1, oss);
                return;
            }

            // ---------- 括号 / 块 / 容器 ----------
            if (const auto paren = std::dynamic_pointer_cast<ParenRangerNode>(node))
            {
                dumpChild("body", paren->getRangerNode(), level + 1, oss);
                return;
            }
            if (const auto bracket = std::dynamic_pointer_cast<BracketExpressionNode>(node))
            {
                dumpChild("body", bracket->getBodyNode(), level + 1, oss);
                return;
            }
            if (const auto block = std::dynamic_pointer_cast<BlockRangerNode>(node))
            {
                size_t i = 0;
                for (const auto& stmt : block->getBodyExpressions())
                {
                    dumpChild("stmt[" + std::to_string(i++) + "]", stmt, level + 1, oss);
                }
                if (i == 0) oss << indentOf(level + 1) << "<empty block>\n";
                return;
            }
            if (const auto dict = std::dynamic_pointer_cast<DictionaryExpressionNode>(node))
            {
                dumpChild("body", dict->getBodyNode(), level + 1, oss);
                return;
            }
            if (const auto list = std::dynamic_pointer_cast<ListExpressionNode>(node))
            {
                dumpChild("body", list->getBodyNode(), level + 1, oss);
                return;
            }
            if (const auto pair = std::dynamic_pointer_cast<PairExpressionNode>(node))
            {
                dumpChild("key", pair->getLeftNode(), level + 1, oss);
                dumpChild("value", pair->getRightNode(), level + 1, oss);
                return;
            }

            // ---------- 赋值 / 变量定义 ----------
            if (const auto assign = std::dynamic_pointer_cast<AssignmentNode>(node))
            {
                const auto [target, value] = assign->getAssignPair();
                dumpChild("target", target, level + 1, oss);
                dumpChild("value", value, level + 1, oss);
                return;
            }
            if (const auto varDef = std::dynamic_pointer_cast<VariableDefinitionNode>(node))
            {
                size_t i = 0;
                for (const auto& def : varDef->getVarDefs())
                {
                    if (!def) continue;
                    const std::string key = "def[" + std::to_string(i++) + "]";
                    oss << indentOf(level + 1) << key << ":\n";
                    dumpChild("name", def->getNameNode(), level + 2, oss);
                    if (def->hasLabels())
                    {
                        for (const auto& label : def->getLabelNodes())
                        {
                            if (!label) continue;
                            oss << indentOf(level + 2) << "label: " << label->getFullLabel() << "\n";
                            dumpLabelDesNodes(label, level + 3, oss);
                        }
                    }
                    if (def->hasInitialValue())
                    {
                        dumpChild("value", def->getValueNode(), level + 2, oss);
                    }
                }
                return;
            }

            // ---------- 函数 ----------
            if (const auto fn = std::dynamic_pointer_cast<FunctionDefinitionNode>(node))
            {
                dumpChild("callNode", fn->getCallNode(), level + 1, oss);
                dumpChild("body", fn->getBodyNode(), level + 1, oss);
                for (const auto& clause : fn->getSignatureClauses())                {
                    if (!clause) continue;
                    oss << indentOf(level + 1) << "clause: " << clause->getFullLabel() << "\n";
                }
                return;
            }
            if (const auto decl = std::dynamic_pointer_cast<FunctionDeclarationNode>(node))
            {
                dumpChild("callNode", decl->getCallNode(), level + 1, oss);
                return;
            }
            if (const auto anon = std::dynamic_pointer_cast<AnonFunctionDefinitionNode>(node))
            {
                dumpChild("params", anon->getParamNode(), level + 1, oss);
                dumpChild("body", anon->getBodyNode(), level + 1, oss);
                return;
            }

            // ---------- 类 ----------
            if (const auto ctor = std::dynamic_pointer_cast<ConstructorDefinitionNode>(node))
            {
                dumpChild("params", ctor->getParamNode(), level + 1, oss);
                dumpChild("body", ctor->getBodyNode(), level + 1, oss);
                return;
            }
            if (const auto cls = std::dynamic_pointer_cast<ClassDefinitionNode>(node))
            {
                dumpChild("name", cls->getNameNode(), level + 1, oss);
                dumpChild("body", cls->getBodyNode(), level + 1, oss);
                return;
            }
            if (const auto clsDecl = std::dynamic_pointer_cast<ClassDeclarationNode>(node))
            {
                dumpChild("name", clsDecl->getNameNode(), level + 1, oss);
                return;
            }

            // ---------- 控制流 ----------
            if (const auto ret = std::dynamic_pointer_cast<ReturnExpressionNode>(node))
            {
                dumpChild("value", ret->getReturnNode(), level + 1, oss);
                return;
            }
            if (const auto cond = std::dynamic_pointer_cast<ConditionNode>(node))
            {
                size_t i = 0;
                for (const auto& branch : cond->getBranchNodes())
                {
                    dumpChild("branch[" + std::to_string(i++) + "]", branch, level + 1, oss);
                }
                for (const auto& label : cond->getResultLabels())
                {
                    if (label) oss << indentOf(level + 1) << "resultType: " << label->getFullLabel() << "\n";
                }
                return;
            }
            if (const auto branch = std::dynamic_pointer_cast<BranchNode>(node))
            {
                dumpChild("condition", branch->getConditionNode(), level + 1, oss);
                dumpChild("body", branch->getBodyNode(), level + 1, oss);
                return;
            }
            if (const auto forLoop = std::dynamic_pointer_cast<ForLoopNode>(node))
            {
                dumpChild("init", forLoop->getInitNode(), level + 1, oss);
                dumpChild("condition", forLoop->getConditionNode(), level + 1, oss);
                dumpChild("update", forLoop->getUpdateNode(), level + 1, oss);
                dumpChild("body", forLoop->getBodyNode(), level + 1, oss);
                return;
            }
            if (const auto loop = std::dynamic_pointer_cast<LoopNode>(node))
            {
                dumpChild("condition", loop->getConditionNode(), level + 1, oss);
                dumpChild("body", loop->getBodyNode(), level + 1, oss);
                return;
            }

            // ---------- 异常 ----------
            if (const auto tryNode = std::dynamic_pointer_cast<TryNode>(node))
            {
                dumpChild("try", tryNode->getTryBody(), level + 1, oss);
                for (const auto& [ident, body] : tryNode->getCatchBodies())
                {
                    oss << indentOf(level + 1)
                        << "catch(" << (ident ? ident->getName() : std::string("?")) << "):\n";
                    dumpNode(body, level + 2, oss);
                }
                if (tryNode->getFinallyBody())
                {
                    dumpChild("finally", tryNode->getFinallyBody(), level + 1, oss);
                }
                return;
            }
            if (const auto throwNode = std::dynamic_pointer_cast<ThrowNode>(node))
            {
                dumpChild("value", throwNode->getThrowExpression(), level + 1, oss);
                return;
            }

            // 其余节点（break/continue/pass/encapsulated）没有子节点
        }
    }

    std::string dumpAstTree(const std::shared_ptr<ProgramNode>& program)
    {
        std::ostringstream oss;
        if (!program)
        {
            oss << "<null program>\n";
            return oss.str();
        }
        oss << "ProgramNode (statements=" << program->getStatements().size() << ")\n";
        size_t i = 0;
        for (const auto& stmt : program->getStatements())
        {
            if (!stmt) continue;
            oss << indentOf(1) << "stmt[" << i++ << "]:\n";
            // 顶层语句通常是 ExpressionStatementNode，需要剥一层
            if (const auto exprStmt = std::dynamic_pointer_cast<ExpressionStatementNode>(stmt))
            {
                dumpNode(exprStmt->getExpression(), 2, oss);
            }
            else
            {
                oss << indentOf(2) << "<" << getNodeTypeName(stmt->getType()) << ">\n";
            }
        }
        return oss.str();
    }
}

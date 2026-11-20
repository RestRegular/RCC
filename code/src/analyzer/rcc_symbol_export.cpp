//
// rcc_symbol_export.cpp —— `-s` 符号表导出（P12 重写）
//
// 三条设计约束，决定了下面的写法：
//
//  1. **只读 AST**。编辑器 / 依赖分析工具要在源码还编译不过的时候也能建索引，
//     所以这里绝不调用 CodeGen、不要求类型解析成功。类型一律按源码原文报出，
//     解析不出来的也不报错 —— 那是编译器的活，不是索引工具的活。
//
//  2. **先收集成模型，再渲染**。所有格式都从 `SymbolTable` 渲染，
//     加格式不需要重写 AST 遍历，也不会出现「text 有、html 没有」的字段漂移。
//
//  3. **符号 id 可复现**。legacy 的 `name_vidN` 来自一个全局自增计数器，
//     同一份源码改个形参个数 id 就变，没法当稳定引用用。这里改成
//     `<kind>:<file>:<line>:<col>:<name>` 的 FNV-1a 短哈希 —— 位置不变 id 就不变。
//
#include "../../include/analyzer/rcc_symbol_export.h"
#include "../../include/analyzer/rcc_ast_components.h"
#include "../../include/backend/rcc_symbol_naming.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <iomanip>
#include <map>
#include <set>
#include <sstream>

namespace analyzer
{
    // ==================== 小工具 ====================
    namespace
    {
        std::string escapeHtml(const std::string& s)
        {
            std::string out;
            out.reserve(s.size() + 16);
            for (const char c : s)
            {
                switch (c)
                {
                case '&': out += "&amp;"; break;
                case '<': out += "&lt;"; break;
                case '>': out += "&gt;"; break;
                case '"': out += "&quot;"; break;
                case '\'': out += "&#39;"; break;
                default: out += c; break;
                }
            }
            return out;
        }

        std::string escapeJson(const std::string& s)
        {
            std::string out;
            out.reserve(s.size() + 16);
            for (const unsigned char c : s)
            {
                switch (c)
                {
                case '"': out += "\\\""; break;
                case '\\': out += "\\\\"; break;
                case '\b': out += "\\b"; break;
                case '\f': out += "\\f"; break;
                case '\n': out += "\\n"; break;
                case '\r': out += "\\r"; break;
                case '\t': out += "\\t"; break;
                default:
                    if (c < 0x20)
                    {
                        char buf[8];
                        std::snprintf(buf, sizeof(buf), "\\u%04x", c);
                        out += buf;
                    }
                    else out += static_cast<char>(c);
                    break;
                }
            }
            return out;
        }

        // 稳定 id：FNV-1a 32 位。不用 std::hash —— 它的值不保证跨实现一致，
        // 而 id 是要写进 JSON / ctags 被别的工具长期引用的。
        std::string stableId(const std::string& seed)
        {
            uint32_t hash = 2166136261u;
            for (const unsigned char c : seed)
            {
                hash ^= c;
                hash *= 16777619u;
            }
            char buf[16];
            std::snprintf(buf, sizeof(buf), "%08x", hash);
            return buf;
        }

        std::string fileNameOf(const std::string& path)
        {
            const auto pos = path.find_last_of("/\\");
            return pos == std::string::npos ? path : path.substr(pos + 1);
        }

        std::string lower(std::string s)
        {
            for (char& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            return s;
        }

        struct Location
        {
            std::string file;
            size_t line = 0;
            size_t column = 0;
        };

        Location locationOf(const ast::Node& node)
        {
            const auto pos = node.getPos();
            Location loc;
            loc.file = fileNameOf(pos.getFilepath());
            loc.line = pos.getLine();
            loc.column = pos.getColumn();
            return loc;
        }

        // 顶层语句剥掉 ExpressionStatementNode 外壳。
        // 注意与 module_loader 里的同名函数不同：这里还要接受「本身就是表达式」
        // 的语句节点（ProgramNode 里两种都可能出现）。
        std::shared_ptr<ast::ExpressionNode> expressionOf(
            const std::shared_ptr<ast::StatementNode>& stmt)
        {
            if (!stmt) return nullptr;
            if (const auto exprStmt =
                    std::dynamic_pointer_cast<ast::ExpressionStatementNode>(stmt))
            {
                return exprStmt->getExpression();
            }
            return std::dynamic_pointer_cast<ast::ExpressionNode>(stmt);
        }

        // 取 IdentifierNode 上的类型标注原文。空串 = 没标注。
        std::string annotationOf(const std::shared_ptr<ast::IdentifierNode>& id)
        {
            if (!id) return {};
            for (const auto& label : id->getLabels())
            {
                if (!label) continue;
                const auto full = label->getFullLabel();
                if (!full.empty()) return full;
            }
            return {};
        }

        // 类基名：跳过修饰标签（builtin/public/protected/private/hidden/extern），
        // 它们不是基类（2026-11-20 可见性改造）。
        std::string baseNameOf(const std::shared_ptr<ast::IdentifierNode>& id)
        {
            if (!id) return {};
            static const std::unordered_set<std::string> kMods = {
                "builtin", "extern", "public", "protected", "private", "hidden"
            };
            for (const auto& label : id->getLabels())
            {
                if (!label) continue;
                const auto full = label->getFullLabel();
                if (!full.empty() && !kMods.count(full)) return full;
            }
            return {};
        }

        // 取一个表达式的**源码文本**。
        // `Node::toString()` 是调试用的（`[Node(STRING): [Token(...)...]]`），
        // 直接塞进符号表既难看又没法用；字面量改走 literalString()。
        std::string sourceTextOf(const std::shared_ptr<ast::ExpressionNode>& node)
        {
            if (!node) return {};
            if (const auto lit = std::dynamic_pointer_cast<ast::LiteralNode>(node))
            {
                // StringLiteralNode::literalString() 返回的是**去引号后的内容**吗？
                // 不是 —— 它返回 token 原文，已经带引号（`"z"`）。所以这里
                // 不能再补引号，否则会变成 `""z""`。
                return lit->literalString();
            }
            return node->toString();
        }

        // ==================== 形参解析 ====================
        //
        // Rio 的形参在 AST 里没有统一的 ParameterNode 列表，而是表达式树：
        //   IdentifierNode("a")                       单个
        //   InfixExpressionNode(',', l, r)            逗号分隔
        //   UnaryExpressionNode('*', Identifier)      变长形参
        //   AssignmentNode(Identifier, value)         关键字形参（有默认值）
        //   ParenRangerNode / BlockRangerNode         括号包裹
        // 按同一套形状递归，与 CodeGen 的 collectParamNames 保持一致。

        void collectParams(const std::shared_ptr<ast::ExpressionNode>& node,
                           std::vector<SymbolParam>& out)
        {
            if (!node) return;

            if (const auto infix = std::dynamic_pointer_cast<ast::InfixExpressionNode>(node))
            {
                if (infix->getOpToken().getType() == core::TokenType::TOKEN_COMMA)
                {
                    collectParams(infix->getLeftNode(), out);
                    collectParams(infix->getRightNode(), out);
                    return;
                }
                if (infix->getOpToken().getType() == core::TokenType::TOKEN_COLON)
                {
                    SymbolParam p;
                    if (const auto id =
                            std::dynamic_pointer_cast<ast::IdentifierNode>(infix->getLeftNode()))
                    {
                        p.name = id->getName();
                        p.type = annotationOf(id);
                    }
                    if (p.type.empty() && infix->getRightNode())
                    {
                        p.type = infix->getRightNode()->toString();
                    }
                    if (!p.name.empty()) out.push_back(std::move(p));
                    return;
                }
                collectParams(infix->getLeftNode(), out);
                return;
            }

            // `*args`
            if (const auto unary = std::dynamic_pointer_cast<ast::UnaryExpressionNode>(node))
            {
                if (unary->getOpToken().getType() == core::TokenType::TOKEN_STAR)
                {
                    if (const auto id = std::dynamic_pointer_cast<ast::IdentifierNode>(
                            unary->getRightNode()))
                    {
                        SymbolParam p;
                        p.name = id->getName();
                        p.type = annotationOf(id);
                        p.variadic = true;
                        out.push_back(std::move(p));
                        return;
                    }
                }
            }

            // `k: str = "z"`
            if (const auto assign = std::dynamic_pointer_cast<ast::AssignmentNode>(node))
            {
                SymbolParam p;
                p.keyword = true;
                if (const auto id = std::dynamic_pointer_cast<ast::IdentifierNode>(
                        assign->getAssignPair().first))
                {
                    p.name = id->getName();
                    p.type = annotationOf(id);
                }
                if (assign->getAssignPair().second)
                {
                    p.defaultValue = sourceTextOf(assign->getAssignPair().second);
                }
                if (!p.name.empty()) out.push_back(std::move(p));
                return;
            }

            if (const auto block = std::dynamic_pointer_cast<ast::BlockRangerNode>(node))
            {
                for (const auto& e : block->getBodyExpressions()) collectParams(e, out);
                return;
            }
            if (const auto paren = std::dynamic_pointer_cast<ast::ParenRangerNode>(node))
            {
                collectParams(paren->getRangerNode(), out);
                return;
            }
            if (const auto list = std::dynamic_pointer_cast<ast::ListExpressionNode>(node))
            {
                collectParams(list->getBodyNode(), out);
                return;
            }
            if (const auto id = std::dynamic_pointer_cast<ast::IdentifierNode>(node))
            {
                SymbolParam p;
                p.name = id->getName();
                p.type = annotationOf(id);
                out.push_back(std::move(p));
                return;
            }
            // ParameterNode：`toString()` 形如 "a: any"
            if (std::dynamic_pointer_cast<ast::ParameterNode>(node))
            {
                const std::string text = node->toString();
                const auto colon = text.find(':');
                SymbolParam p;
                const std::string name = colon == std::string::npos ? text : text.substr(0, colon);
                for (const char c : name)
                {
                    if (std::isalnum(static_cast<unsigned char>(c)) || c == '_') p.name += c;
                }
                if (colon != std::string::npos) p.type = text.substr(colon + 1);
                if (!p.name.empty()) out.push_back(std::move(p));
            }
        }

        // ==================== 名字提取 ====================
        struct CalleeInfo
        {
            std::string name;
            std::vector<std::string> genericParams;
            std::shared_ptr<ast::ExpressionNode> paramsNode;
        };

        void collectGenericParams(const std::shared_ptr<ast::ExpressionNode>& node,
                                  std::vector<std::string>& out)
        {
            if (!node) return;
            if (const auto bracket = std::dynamic_pointer_cast<ast::BracketExpressionNode>(node))
            {
                collectGenericParams(bracket->getBodyNode(), out);
                return;
            }
            if (const auto infix = std::dynamic_pointer_cast<ast::InfixExpressionNode>(node))
            {
                if (infix->getOpToken().getType() == core::TokenType::TOKEN_COMMA)
                {
                    collectGenericParams(infix->getLeftNode(), out);
                    collectGenericParams(infix->getRightNode(), out);
                    return;
                }
                collectGenericParams(infix->getLeftNode(), out);
                return;
            }
            if (const auto id = std::dynamic_pointer_cast<ast::IdentifierNode>(node))
            {
                if (!id->getName().empty()) out.push_back(id->getName());
            }
        }

        // callNode 的三种形态（与 CodeGen::collectSignature 一致）：
        //   IdentifierNode("f")                       普通函数
        //   FunctionCallNode(left, ParenRanger)       带形参表 / 泛型
        //   IndexExpressionNode(Identifier, Bracket)  泛型实参
        CalleeInfo calleeOf(const std::shared_ptr<ast::ExpressionNode>& callNode)
        {
            CalleeInfo info;
            if (!callNode) return info;

            if (const auto id = std::dynamic_pointer_cast<ast::IdentifierNode>(callNode))
            {
                info.name = id->getName();
                return info;
            }
            if (const auto call = std::dynamic_pointer_cast<ast::FunctionCallNode>(callNode))
            {
                info.paramsNode = call->getRightNode();
                if (const auto id = std::dynamic_pointer_cast<ast::IdentifierNode>(call->getLeftNode()))
                {
                    info.name = id->getName();
                }
                else if (const auto index =
                             std::dynamic_pointer_cast<ast::IndexExpressionNode>(call->getLeftNode()))
                {
                    if (const auto id =
                            std::dynamic_pointer_cast<ast::IdentifierNode>(index->getLeftNode()))
                    {
                        info.name = id->getName();
                    }
                    collectGenericParams(index->getIndexNode(), info.genericParams);
                }
                return info;
            }
            if (const auto infix = std::dynamic_pointer_cast<ast::InfixExpressionNode>(callNode))
            {
                if (const auto id = std::dynamic_pointer_cast<ast::IdentifierNode>(infix->getLeftNode()))
                {
                    info.name = id->getName();
                }
                info.paramsNode = infix->getRightNode();
            }
            return info;
        }

        std::string returnTypeOf(const std::shared_ptr<ast::FunctionDefinitionNode>& node)
        {
            if (!node) return {};
            for (const auto& label : node->getLabelNodes())
            {
                if (!label) continue;
                const auto full = label->getFullLabel();
                if (!full.empty()) return full;
            }
            return {};
        }

        // ==================== 调用收集（依赖分析用） ====================
        //
        // 只收「名字形式的直接调用」，够画调用图与做影响面分析了。
        // 不做符号决议：导出器没有类型系统，也不需要 —— 编辑器索引关心的是
        // 「这段代码提到了哪些名字」，不是「实际绑到了哪一个」。
        void collectCalls(const std::shared_ptr<ast::ExpressionNode>& node,
                          std::vector<std::string>& out)
        {
            if (!node) return;
            if (const auto call = std::dynamic_pointer_cast<ast::FunctionCallNode>(node))
            {
                if (const auto id =
                        std::dynamic_pointer_cast<ast::IdentifierNode>(call->getLeftNode()))
                {
                    const auto& name = id->getName();
                    if (!name.empty()) out.push_back(name);
                }
            }
            if (const auto infix = std::dynamic_pointer_cast<ast::InfixExpressionNode>(node))
            {
                collectCalls(infix->getLeftNode(), out);
                collectCalls(infix->getRightNode(), out);
                return;
            }
            if (const auto unary = std::dynamic_pointer_cast<ast::UnaryExpressionNode>(node))
            {
                collectCalls(unary->getRightNode(), out);
                return;
            }
            if (const auto postfix = std::dynamic_pointer_cast<ast::PostfixExpressionNode>(node))
            {
                collectCalls(postfix->getNode(), out);
                return;
            }
            if (const auto block = std::dynamic_pointer_cast<ast::BlockRangerNode>(node))
            {
                for (const auto& e : block->getBodyExpressions()) collectCalls(e, out);
                return;
            }
            if (const auto paren = std::dynamic_pointer_cast<ast::ParenRangerNode>(node))
            {
                collectCalls(paren->getRangerNode(), out);
                return;
            }
            if (const auto list = std::dynamic_pointer_cast<ast::ListExpressionNode>(node))
            {
                collectCalls(list->getBodyNode(), out);
                return;
            }
            if (const auto dict = std::dynamic_pointer_cast<ast::DictionaryExpressionNode>(node))
            {
                collectCalls(dict->getBodyNode(), out);
                return;
            }
            if (const auto assign = std::dynamic_pointer_cast<ast::AssignmentNode>(node))
            {
                collectCalls(assign->getAssignPair().first, out);
                collectCalls(assign->getAssignPair().second, out);
                return;
            }
            if (const auto ret = std::dynamic_pointer_cast<ast::ReturnExpressionNode>(node))
            {
                collectCalls(ret->getReturnNode(), out);
                return;
            }
            if (const auto branch = std::dynamic_pointer_cast<ast::BranchNode>(node))
            {
                collectCalls(branch->getConditionNode(), out);
                collectCalls(branch->getBodyNode(), out);
                return;
            }
            if (const auto cond = std::dynamic_pointer_cast<ast::ConditionNode>(node))
            {
                for (const auto& b : cond->getBranchNodes()) collectCalls(b, out);
                return;
            }
            if (const auto loop = std::dynamic_pointer_cast<ast::LoopNode>(node))
            {
                collectCalls(loop->getConditionNode(), out);
                collectCalls(loop->getBodyNode(), out);
                return;
            }
            if (const auto forLoop = std::dynamic_pointer_cast<ast::ForLoopNode>(node))
            {
                collectCalls(forLoop->getInitNode(), out);
                collectCalls(forLoop->getUpdateNode(), out);
                return;
            }
            if (const auto tryNode = std::dynamic_pointer_cast<ast::TryNode>(node))
            {
                collectCalls(tryNode->getTryBody(), out);
                for (const auto& cb : tryNode->getCatchBodies()) collectCalls(cb.second, out);
                collectCalls(tryNode->getFinallyBody(), out);
                return;
            }
            if (const auto throwNode = std::dynamic_pointer_cast<ast::ThrowNode>(node))
            {
                collectCalls(throwNode->getThrowExpression(), out);
                return;
            }
            if (const auto index = std::dynamic_pointer_cast<ast::IndexExpressionNode>(node))
            {
                collectCalls(index->getLeftNode(), out);
                collectCalls(index->getIndexNode(), out);
                return;
            }
            if (const auto pair = std::dynamic_pointer_cast<ast::PairExpressionNode>(node))
            {
                collectCalls(pair->getLeftNode(), out);
                collectCalls(pair->getRightNode(), out);
                return;
            }
            // `var t: int = helper(n)` —— 局部变量定义的初始值里也可能有调用。
            // 漏了这一支会让 `main -> caller` 之后断链，调用图看起来是残的。
            if (const auto var = std::dynamic_pointer_cast<ast::VariableDefinitionNode>(node))
            {
                for (const auto& def : var->getVarDefs())
                {
                    if (def && def->hasInitialValue()) collectCalls(def->getValueNode(), out);
                }
            }
        }

        // ==================== 收集器 ====================
        class Collector
        {
        public:
            Collector(SymbolTable& table, const std::string& mainFile,
                      const std::vector<std::string>& exported)
                : _table(table), _mainFile(mainFile),
                  _exported(exported.begin(), exported.end())
            {
            }

            void run(const std::shared_ptr<ast::ProgramNode>& program)
            {
                if (!program) return;
                for (const auto& stmt : program->getStatements()) collectStatement(stmt, nullptr);

                // 重载判据必须与 CodeGen::registerOverloadSignatures 一致，
                // 否则报出的汇编符号会和实际目标文件对不上。
                std::map<std::string, int> defCount;
                for (const auto& sym : _table.symbols)
                {
                    if (sym.kind != SymbolKind::FUNCTION) continue;
                    if (sym.encapsulated) continue;
                    if (!sym.genericParams.empty()) continue;
                    if (sym.name == "main" || sym.name == "sout" || sym.name == "super") continue;
                    ++defCount[sym.name];
                }
                for (const auto& [name, count] : defCount)
                {
                    if (count >= 2) _table.overloadedNames.push_back(name);
                }
                std::sort(_table.overloadedNames.begin(), _table.overloadedNames.end());

                // 回填汇编符号：收集阶段还不知道「这个名字有没有重载」，
                // 而符号是否修饰完全取决于此。
                for (auto& sym : _table.symbols) fillAsmSymbol(sym);
            }

        private:
            SymbolTable& _table;
            std::string _mainFile;
            std::set<std::string> _exported;

            bool isOverloaded(const std::string& name) const
            {
                return std::find(_table.overloadedNames.begin(),
                                 _table.overloadedNames.end(), name) != _table.overloadedNames.end();
            }

            // 从标签区解析显式符号属性（builtin/extern/normal）与可见性。
            static void applyLabelAttrs(Symbol& sym,
                                        const std::vector<std::shared_ptr<ast::LabelNode>>& labels)
            {
                for (const auto& l : labels)
                {
                    if (!l) continue;
                    const std::string n = l->getFullLabel();
                    if (n == "builtin") sym.attr = Symbol::SymbolAttr::BUILTIN;
                    else if (n == "extern") sym.attr = Symbol::SymbolAttr::EXTERN;
                    else if (n == "public" || n == "protected" || n == "private")
                        sym.visibility = n;
                    else if (n == "hidden") sym.hidden = true;
                }
            }

            void add(Symbol sym, const Location& loc)
            {
                sym.file = loc.file;
                sym.line = loc.line;
                sym.column = loc.column;
                sym.exported = _exported.find(sym.name) != _exported.end();
                sym.fromModule = !_mainFile.empty() && loc.file != fileNameOf(_mainFile);
                sym.id = stableId(lower(symbolKindName(sym.kind)) + ":" + sym.file + ":" +
                                  std::to_string(loc.line) + ":" + std::to_string(loc.column) +
                                  ":" + sym.name);
                _table.symbols.push_back(std::move(sym));
            }

            void collectStatement(const std::shared_ptr<ast::StatementNode>& stmt,
                                  const std::string* owner)
            {
                const auto expr = expressionOf(stmt);
                if (!expr) return;

                if (const auto cls = std::dynamic_pointer_cast<ast::ClassDefinitionNode>(expr))
                {
                    collectClass(cls);
                    return;
                }
                if (const auto fn = std::dynamic_pointer_cast<ast::FunctionDefinitionNode>(expr))
                {
                    collectFunction(fn, owner);
                    return;
                }
                if (const auto var = std::dynamic_pointer_cast<ast::VariableDefinitionNode>(expr))
                {
                    collectVariables(var, owner);
                }
            }

            void collectClass(const std::shared_ptr<ast::ClassDefinitionNode>& cls)
            {
                std::string className;
                std::string base;
                Location loc = locationOf(*cls);
                if (const auto id =
                        std::dynamic_pointer_cast<ast::IdentifierNode>(cls->getNameNode()))
                {
                    className = id->getName();
                    base = baseNameOf(id);          // `class Box: Base`（跳过修饰标签）
                    loc = locationOf(*id);
                }
                if (className.empty()) return;

                Symbol sym;
                sym.kind = SymbolKind::CLASS;
                sym.name = className;
                sym.baseClass = base;
                sym.asmSymbol = className;
                // 类标签：builtin/public/protected/private/hidden 是修饰标签，
                // 不是基类名（annotationOf 已过滤），这里解析成显式属性。
                bool classBuiltin = false;
                std::string classDefaultVisibility;
                if (const auto id = std::dynamic_pointer_cast<ast::IdentifierNode>(cls->getNameNode()))
                {
                    applyLabelAttrs(sym, id->getLabels());
                    // 类上的 public/protected/private 是「成员默认可见性」，不是
                    // 类符号自身的可见性（类自身对外可见性仍由 export/hidden 决定）。
                    // 这里保存下来供方法继承；同时清掉误落到类符号上的成员可见性。
                    classBuiltin = (sym.attr == Symbol::SymbolAttr::BUILTIN);
                    if (sym.visibility == "public" || sym.visibility == "protected" ||
                        sym.visibility == "private")
                    {
                        classDefaultVisibility = sym.visibility;
                        sym.visibility.clear();
                    }
                }

                std::vector<std::string> members;
                std::vector<std::string> methods;
                bool hasCtor = false;

                if (const auto block =
                        std::dynamic_pointer_cast<ast::BlockRangerNode>(cls->getBodyNode()))
                {
                    for (const auto& e : block->getBodyExpressions())
                    {
                        const auto inner = std::dynamic_pointer_cast<ast::ExpressionNode>(e);
                        if (!inner) continue;

                        if (const auto var =
                                std::dynamic_pointer_cast<ast::VariableDefinitionNode>(inner))
                        {
                            // 字段：每个 def 一条符号，名字进 members。
                            for (const auto& def : var->getVarDefs())
                            {
                                if (!def || !def->getNameNode()) continue;
                                members.push_back(def->getNameNode()->getName());
                                collectVariableDef(def, &className);
                            }
                            continue;
                        }
                        if (const auto fn =
                                std::dynamic_pointer_cast<ast::FunctionDefinitionNode>(inner))
                        {
                            const auto info = calleeOf(fn->getCallNode());
                            if (!info.name.empty()) methods.push_back(info.name);
                            collectFunction(fn, &className, classBuiltin, classDefaultVisibility);
                            continue;
                        }
                        if (const auto ctor =
                                std::dynamic_pointer_cast<ast::ConstructorDefinitionNode>(inner))
                        {
                            collectConstructor(ctor, className);
                            hasCtor = true;
                        }
                    }
                }
                sym.members = members;
                sym.methods = methods;
                sym.hasConstructor = hasCtor;
                add(std::move(sym), loc);
            }

            void collectFunction(const std::shared_ptr<ast::FunctionDefinitionNode>& fn,
                                 const std::string* owner,
                                 bool classBuiltin = false,
                                 const std::string& classDefaultVisibility = std::string())
            {
                const auto info = calleeOf(fn->getCallNode());
                if (info.name.empty()) return;

                // 内建声明改造（2026-11-20）：encapsulated 桩改为 builtin/extern
                // 无体声明，用 fn->isDeclaration() 判定「实现由编译器/运行时提供」。
                const bool isExternalImpl = fn->isDeclaration();

                Symbol sym;
                sym.kind = owner ? SymbolKind::METHOD : SymbolKind::FUNCTION;
                sym.name = info.name;
                if (owner) sym.owner = *owner;
                sym.genericParams = info.genericParams;
                sym.returnType = returnTypeOf(fn);
                sym.encapsulated = isExternalImpl;
                applyLabelAttrs(sym, fn->getLabelNodes());
                // 类被 builtin 修饰时，方法可省略 builtin 标签 —— 继承类的属性。
                if (classBuiltin && sym.attr == Symbol::SymbolAttr::NORMAL)
                    sym.attr = Symbol::SymbolAttr::BUILTIN;
                // 方法未显式写访问级别时，取类标签区的默认级别。
                if (owner && sym.visibility.empty() && !classDefaultVisibility.empty())
                    sym.visibility = classDefaultVisibility;
                collectParams(info.paramsNode, sym.params);
                if (fn->getBodyNode()) collectCalls(fn->getBodyNode(), sym.calls);

                Location loc = locationOf(*fn);
                if (const auto id =
                        std::dynamic_pointer_cast<ast::IdentifierNode>(fn->getCallNode()))
                {
                    loc = locationOf(*id);
                }
                add(std::move(sym), loc);
            }

            void collectConstructor(const std::shared_ptr<ast::ConstructorDefinitionNode>& ctor,
                                    const std::string& className)
            {
                Symbol sym;
                sym.kind = SymbolKind::CONSTRUCTOR;
                sym.name = "__init__";
                sym.owner = className;
                collectParams(ctor->getParamNode(), sym.params);
                if (ctor->getBodyNode()) collectCalls(ctor->getBodyNode(), sym.calls);
                add(std::move(sym), locationOf(*ctor));
            }

            void collectVariables(const std::shared_ptr<ast::VariableDefinitionNode>& var,
                                  const std::string* owner)
            {
                for (const auto& def : var->getVarDefs()) collectVariableDef(def, owner);
            }

            void collectVariableDef(const std::shared_ptr<ast::VariableDefinitionNode::VarDefData>& def,
                                    const std::string* owner)
            {
                if (!def || !def->getNameNode()) return;
                Symbol sym;
                sym.kind = SymbolKind::VARIABLE;
                sym.name = def->getNameNode()->getName();
                if (owner) sym.owner = *owner;
                // 变量标签既含类型又含修饰（hidden/public/...），取类型时须跳过修饰。
                static const std::unordered_set<std::string> kNonType = {
                    "builtin", "extern", "hidden", "public", "protected", "private",
                    "static", "global", "const", "quote", "overwrite", "interface", "virtual"
                };
                sym.type = baseNameOf(def->getNameNode());
                if (sym.type.empty() && def->hasLabels())
                {
                    for (const auto& label : def->getLabelNodes())
                    {
                        const auto fn = label ? label->getFullLabel() : std::string{};
                        if (!fn.empty() && !kNonType.count(fn))
                        { sym.type = fn; break; }
                    }
                }
                applyLabelAttrs(sym, def->getLabelNodes());
                if (def->hasInitialValue() && def->getValueNode())
                {
                    sym.initializer = sourceTextOf(def->getValueNode());
                    collectCalls(def->getValueNode(), sym.calls);
                }
                add(std::move(sym), locationOf(*def->getNameNode()));
            }

            // 汇编符号。规则全部来自 backend::naming，绝不在这里另写一套。
            void fillAsmSymbol(Symbol& sym) const
            {
                switch (sym.kind)
                {
                case SymbolKind::FUNCTION:
                    if (sym.encapsulated)
                    {
                        sym.asmSymbol = sym.name;
                        return;
                    }
                    if (!sym.genericParams.empty() || isOverloaded(sym.name))
                    {
                        // 泛型声明不发射；重载集需要形参类型编码，而导出器没有
                        // 类型系统。这里**留空**而不是编一个占位 —— 编一个看起来
                        // 像真符号的名字，会让 objdump / perf 的对齐工具误判。
                        // 需要精确符号时用 ctags 之外的路径（后端自己知道）。
                        sym.asmSymbol.clear();
                        return;
                    }
                    sym.asmSymbol = backend::naming::applyEntryRename(sym.name);
                    return;
                case SymbolKind::METHOD:
                    sym.asmSymbol = backend::naming::methodSymbol(sym.owner, sym.name);
                    return;
                case SymbolKind::CONSTRUCTOR:
                    sym.asmSymbol = backend::naming::constructorSymbol(sym.owner);
                    return;
                case SymbolKind::CLASS:
                    sym.asmSymbol = sym.name;
                    return;
                case SymbolKind::VARIABLE:
                    sym.asmSymbol.clear();
                    return;
                }
            }
        };
    }

    // ==================== 格式注册表 ====================
    //
    // 加一种格式只要三步：
    //   1. SymbolFormat 加一项
    //   2. 写一个 renderXxx(const SymbolTable&)
    //   3. 在 kFormats 里加一行
    // CLI、模型、其它格式都不用动。
    namespace
    {
        using RenderFn = std::string (*)(const SymbolTable&);

        std::string renderText(const SymbolTable& table);
        std::string renderJson(const SymbolTable& table);
        std::string renderHtml(const SymbolTable& table);
        std::string renderCtags(const SymbolTable& table);
        std::string renderDot(const SymbolTable& table);
        std::string renderMarkdown(const SymbolTable& table);

        struct FormatEntry
        {
            const char* name;
            SymbolFormat format;
            const char* extension;
            const char* description;
            RenderFn render;
        };

        // ctags 里构造器的标签名后缀。源码里构造器写作 `ctor`，但消费者
        // 认的是 `Class.__init__`（与汇编符号一致），这里统一。
        constexpr const char* CTOR_TAG_SUFFIX = ".__init__";

        const FormatEntry kFormats[] = {
            {"text",     SymbolFormat::TEXT,     ".txt",  "human-readable aligned table", renderText},
            {"json",     SymbolFormat::JSON,     ".json", "machine-readable, versioned schema", renderJson},
            {"html",     SymbolFormat::HTML,     ".html", "self-contained browsable symbol browser", renderHtml},
            {"ctags",    SymbolFormat::CTAGS,    ".tags", "Universal Ctags for editor navigation", renderCtags},
            {"dot",      SymbolFormat::DOT,      ".dot",  "Graphviz: inheritance + call graph", renderDot},
            {"markdown", SymbolFormat::MARKDOWN, ".md",   "Markdown tables for docs / README", renderMarkdown},
        };
    }

    bool parseSymbolFormat(const std::string& name, SymbolFormat& out, std::string& errorMsg)
    {
        const auto key = lower(name);
        for (const auto& entry : kFormats)
        {
            if (key == entry.name)
            {
                out = entry.format;
                return true;
            }
        }
        errorMsg = "Unknown symbol format: '" + name + "'. Supported: " + symbolFormatList();
        return false;
    }

    std::string symbolFormatList()
    {
        std::string out;
        for (const auto& entry : kFormats)
        {
            if (!out.empty()) out += ", ";
            out += entry.name;
        }
        return out;
    }

    const char* symbolFormatExtension(const SymbolFormat format)
    {
        for (const auto& entry : kFormats)
        {
            if (entry.format == format) return entry.extension;
        }
        return ".txt";
    }

    const char* symbolKindName(const SymbolKind kind)
    {
        switch (kind)
        {
        case SymbolKind::FUNCTION:    return "Function";
        case SymbolKind::METHOD:      return "Method";
        case SymbolKind::CONSTRUCTOR: return "Constructor";
        case SymbolKind::CLASS:       return "Class";
        case SymbolKind::VARIABLE:    return "Variable";
        }
        return "Unknown";
    }

    // ==================== 收集入口 ====================

    namespace
    {
        // `export(a, b, c)` 的实参展开成名字列表。
        void gatherExportNames(const std::shared_ptr<ast::ExpressionNode>& node,
                               std::vector<std::string>& out)
        {
            if (!node) return;
            if (const auto infix = std::dynamic_pointer_cast<ast::InfixExpressionNode>(node))
            {
                if (infix->getOpToken().getType() == core::TokenType::TOKEN_COMMA)
                {
                    gatherExportNames(infix->getLeftNode(), out);
                    gatherExportNames(infix->getRightNode(), out);
                    return;
                }
            }
            if (const auto paren = std::dynamic_pointer_cast<ast::ParenRangerNode>(node))
            {
                gatherExportNames(paren->getRangerNode(), out);
                return;
            }
            if (const auto id = std::dynamic_pointer_cast<ast::IdentifierNode>(node))
            {
                if (!id->getName().empty()) out.push_back(id->getName());
            }
        }

        void gatherExportStatements(const std::shared_ptr<ast::StatementNode>& stmt,
                                    std::vector<std::string>& out)
        {
            const auto expr = expressionOf(stmt);
            if (!expr) return;
            if (const auto call = std::dynamic_pointer_cast<ast::FunctionCallNode>(expr))
            {
                const auto callee =
                    std::dynamic_pointer_cast<ast::IdentifierNode>(call->getLeftNode());
                if (callee && callee->getName() == "export")
                {
                    gatherExportNames(call->getRightNode(), out);
                }
            }
        }
    }

    std::vector<std::string> collectExportNames(
        const std::shared_ptr<ast::ProgramNode>& program)
    {
        std::vector<std::string> names;
        if (!program) return names;
        for (const auto& stmt : program->getStatements())
        {
            gatherExportStatements(stmt, names);
        }
        // 去重，保持首次出现顺序（导出列表通常很短，线性查重足够）。
        std::vector<std::string> unique;
        for (auto& name : names)
        {
            if (std::find(unique.begin(), unique.end(), name) == unique.end())
            {
                unique.push_back(std::move(name));
            }
        }
        return unique;
    }

    void markExported(SymbolTable& table, const std::vector<std::string>& names)
    {
        const std::set<std::string> exported(names.begin(), names.end());
        for (auto& sym : table.symbols)
        {
            sym.exported = exported.find(sym.name) != exported.end();
        }
    }

    SymbolTable collectSymbols(const std::shared_ptr<ast::ProgramNode>& program,
                               const std::string& mainFile,
                               const std::vector<std::string>& files,
                               const std::vector<std::string>& moduleNames)
    {
        SymbolTable table;
        table.mainFile = mainFile;
        table.files = files;
        table.moduleNames = moduleNames;
        // exported 标记由 markExported 单独打 —— 合并后的 AST 里已经没有
        // `export(...)` 语句了（见头文件说明）。
        Collector collector(table, mainFile, {});
        collector.run(program);

        // 输出顺序必须稳定：按种类（类 -> 函数 -> 方法 -> 构造器 -> 变量）、
        // 再按名字、再按位置。legacy 的 unordered_map 迭代序每次都不一样，
        // 是没法进版本控制的。
        std::stable_sort(table.symbols.begin(), table.symbols.end(),
                         [](const Symbol& a, const Symbol& b)
                         {
                             const int ra = static_cast<int>(a.kind);
                             const int rb = static_cast<int>(b.kind);
                             if (ra != rb) return ra < rb;
                             if (a.owner != b.owner) return a.owner < b.owner;
                             if (a.name != b.name) return a.name < b.name;
                             if (a.file != b.file) return a.file < b.file;
                             if (a.line != b.line) return a.line < b.line;
                             return a.column < b.column;
                         });
        return table;
    }

    void filterExportedOnly(SymbolTable& table)
    {
        std::vector<Symbol> kept;
        for (auto& sym : table.symbols)
        {
            if (sym.exported) kept.push_back(std::move(sym));
        }
        table.symbols = std::move(kept);
    }

    void filterByName(SymbolTable& table, const std::vector<std::string>& names,
                      std::vector<std::string>& missing)
    {
        std::set<std::string> wanted(names.begin(), names.end());
        std::set<std::string> found;
        std::vector<Symbol> kept;
        for (auto& sym : table.symbols)
        {
            if (wanted.find(sym.name) != wanted.end())
            {
                found.insert(sym.name);
                kept.push_back(std::move(sym));
            }
        }
        table.symbols = std::move(kept);
        for (const auto& name : names)
        {
            if (found.find(name) == found.end()) missing.push_back(name);
        }
    }

    std::string renderSymbols(const SymbolTable& table, const SymbolFormat format)
    {
        for (const auto& entry : kFormats)
        {
            if (entry.format == format) return entry.render(table);
        }
        return {};
    }

    // ==================== 渲染：text ====================
    namespace
    {
        // 一行的显示文本：`Box.get[T](v: T): T`
        std::string signatureOf(const Symbol& sym)
        {
            std::string out;
            if (sym.kind == SymbolKind::CONSTRUCTOR)
            {
                // 构造器在源码里没有名字，显示成 `Box(...)` 最好认。
                out = sym.owner;
            }
            else
            {
                if (!sym.owner.empty() && sym.kind != SymbolKind::CLASS)
                {
                    out += sym.owner;
                    out += ".";
                }
                out += sym.name;
            }
            if (!sym.genericParams.empty())
            {
                out += "[";
                for (size_t i = 0; i < sym.genericParams.size(); ++i)
                {
                    if (i) out += ", ";
                    out += sym.genericParams[i];
                }
                out += "]";
            }
            if (sym.kind == SymbolKind::CLASS || sym.kind == SymbolKind::VARIABLE)
            {
                if (!sym.type.empty())
                {
                    out += ": " + sym.type;
                }
                else if (!sym.baseClass.empty())
                {
                    out += ": " + sym.baseClass;
                }
                return out;
            }
            out += "(";
            for (size_t i = 0; i < sym.params.size(); ++i)
            {
                if (i) out += ", ";
                const auto& p = sym.params[i];
                if (p.variadic) out += "*";
                out += p.name;
                out += ": ";
                out += p.type.empty() ? "any" : p.type;
                if (!p.defaultValue.empty()) out += " = " + p.defaultValue;
            }
            out += ")";
            if (!sym.returnType.empty()) out += ": " + sym.returnType;
            return out;
        }

        std::string renderText(const SymbolTable& table)
        {
            std::ostringstream out;
            out << "Symbol table: " << fileNameOf(table.mainFile) << "\n";
            out << "Symbols: " << table.symbols.size();
            if (!table.moduleNames.empty())
            {
                out << "   Modules: ";
                for (size_t i = 0; i < table.moduleNames.size(); ++i)
                {
                    if (i) out << ", ";
                    out << table.moduleNames[i];
                }
            }
            out << "\n";

            // 按种类分组，组内按名字。人眼扫的时候分组比全表排序好读。
            const SymbolKind order[] = {
                SymbolKind::CLASS, SymbolKind::FUNCTION, SymbolKind::METHOD,
                SymbolKind::CONSTRUCTOR, SymbolKind::VARIABLE
            };
            for (const auto kind : order)
            {
                std::vector<const Symbol*> group;
                for (const auto& sym : table.symbols)
                {
                    if (sym.kind == kind) group.push_back(&sym);
                }
                if (group.empty()) continue;

                out << "\n" << symbolKindName(kind) << " (" << group.size() << ")\n";
                // 对齐：先算最宽的名字列。
                size_t width = 0;
                std::vector<std::string> sigs;
                for (const auto* sym : group)
                {
                    sigs.push_back(signatureOf(*sym));
                    width = std::max(width, sigs.back().size());
                }
                for (size_t i = 0; i < group.size(); ++i)
                {
                    const auto& sym = *group[i];
                    out << "  " << std::left << std::setw(static_cast<int>(width)) << sigs[i]
                        << "  " << sym.file << ":" << sym.line << ":" << sym.column;
                    if (sym.exported) out << "  [exported]";
                    if (sym.hidden) out << "  [hidden]";
                    if (sym.attr == Symbol::SymbolAttr::BUILTIN) out << "  [builtin]";
                    else if (sym.attr == Symbol::SymbolAttr::EXTERN) out << "  [extern]";
                    if (!sym.visibility.empty()) out << "  [" << sym.visibility << "]";
                    if (sym.fromModule) out << "  [module]";
                    if (!sym.asmSymbol.empty()) out << "  asm=" << sym.asmSymbol;
                    out << "\n";
                }
            }
            return out.str();
        }

        // ==================== 渲染：json ====================
        void jsonStr(std::ostringstream& out, const std::string& key, const std::string& value,
                     bool last = false)
        {
            out << "    \"" << escapeJson(key) << "\": \"" << escapeJson(value) << "\""
                << (last ? "" : ",") << "\n";
        }

        std::string renderJson(const SymbolTable& table)
        {
            std::ostringstream out;
            out << "{\n";
            // schemaVersion 是给消费者的契约：字段增删必须换版本号。
            out << "  \"schemaVersion\": 2,\n";
            out << "  \"tool\": \"rcc\",\n";
            out << "  \"mainFile\": \"" << escapeJson(fileNameOf(table.mainFile)) << "\",\n";

            out << "  \"files\": [";
            for (size_t i = 0; i < table.files.size(); ++i)
            {
                if (i) out << ", ";
                out << "\"" << escapeJson(fileNameOf(table.files[i])) << "\"";
            }
            out << "],\n";

            out << "  \"modules\": [";
            for (size_t i = 0; i < table.moduleNames.size(); ++i)
            {
                if (i) out << ", ";
                out << "\"" << escapeJson(table.moduleNames[i]) << "\"";
            }
            out << "],\n";

            out << "  \"overloadedNames\": [";
            for (size_t i = 0; i < table.overloadedNames.size(); ++i)
            {
                if (i) out << ", ";
                out << "\"" << escapeJson(table.overloadedNames[i]) << "\"";
            }
            out << "],\n";

            out << "  \"symbols\": [\n";
            for (size_t i = 0; i < table.symbols.size(); ++i)
            {
                const auto& sym = table.symbols[i];
                out << "    {\n";
                out << "      \"id\": \"" << escapeJson(sym.id) << "\",\n";
                out << "      \"kind\": \"" << symbolKindName(sym.kind) << "\",\n";
                out << "      \"name\": \"" << escapeJson(sym.name) << "\",\n";
                if (!sym.owner.empty())
                {
                    out << "      \"owner\": \"" << escapeJson(sym.owner) << "\",\n";
                }
                out << "      \"file\": \"" << escapeJson(sym.file) << "\",\n";
                out << "      \"line\": " << sym.line << ",\n";
                out << "      \"column\": " << sym.column << ",\n";
                if (!sym.asmSymbol.empty())
                {
                    out << "      \"asmSymbol\": \"" << escapeJson(sym.asmSymbol) << "\",\n";
                }
                if (!sym.returnType.empty())
                {
                    out << "      \"returnType\": \"" << escapeJson(sym.returnType) << "\",\n";
                }
                if (!sym.type.empty())
                {
                    out << "      \"type\": \"" << escapeJson(sym.type) << "\",\n";
                }
                if (!sym.baseClass.empty())
                {
                    out << "      \"baseClass\": \"" << escapeJson(sym.baseClass) << "\",\n";
                }
                if (!sym.initializer.empty())
                {
                    out << "      \"initializer\": \"" << escapeJson(sym.initializer) << "\",\n";
                }
                if (!sym.genericParams.empty())
                {
                    out << "      \"genericParams\": [";
                    for (size_t g = 0; g < sym.genericParams.size(); ++g)
                    {
                        if (g) out << ", ";
                        out << "\"" << escapeJson(sym.genericParams[g]) << "\"";
                    }
                    out << "],\n";
                }
                if (!sym.params.empty())
                {
                    out << "      \"params\": [\n";
                    for (size_t p = 0; p < sym.params.size(); ++p)
                    {
                        const auto& param = sym.params[p];
                        out << "        {\"name\": \"" << escapeJson(param.name) << "\"";
                        if (!param.type.empty())
                        {
                            out << ", \"type\": \"" << escapeJson(param.type) << "\"";
                        }
                        if (param.variadic) out << ", \"variadic\": true";
                        if (param.keyword) out << ", \"keyword\": true";
                        if (!param.defaultValue.empty())
                        {
                            out << ", \"defaultValue\": \"" << escapeJson(param.defaultValue) << "\"";
                        }
                        out << "}" << (p + 1 < sym.params.size() ? "," : "") << "\n";
                    }
                    out << "      ],\n";
                }
                if (!sym.members.empty())
                {
                    out << "      \"members\": [";
                    for (size_t m = 0; m < sym.members.size(); ++m)
                    {
                        if (m) out << ", ";
                        out << "\"" << escapeJson(sym.members[m]) << "\"";
                    }
                    out << "],\n";
                }
                if (!sym.methods.empty())
                {
                    out << "      \"methods\": [";
                    for (size_t m = 0; m < sym.methods.size(); ++m)
                    {
                        if (m) out << ", ";
                        out << "\"" << escapeJson(sym.methods[m]) << "\"";
                    }
                    out << "],\n";
                }
                if (sym.kind == SymbolKind::CLASS)
                {
                    out << "      \"hasConstructor\": " << (sym.hasConstructor ? "true" : "false")
                        << ",\n";
                }
                if (!sym.calls.empty())
                {
                    out << "      \"calls\": [";
                    for (size_t c = 0; c < sym.calls.size(); ++c)
                    {
                        if (c) out << ", ";
                        out << "\"" << escapeJson(sym.calls[c]) << "\"";
                    }
                    out << "],\n";
                }
                out << "      \"exported\": " << (sym.exported ? "true" : "false") << ",\n";
                out << "      \"hidden\": " << (sym.hidden ? "true" : "false") << ",\n";
                out << "      \"attr\": \""
                    << (sym.attr == Symbol::SymbolAttr::BUILTIN ? "builtin" :
                        sym.attr == Symbol::SymbolAttr::EXTERN ? "extern" : "normal")
                    << "\",\n";
                out << "      \"visibility\": \"" << escapeJson(sym.visibility) << "\",\n";
                out << "      \"fromModule\": " << (sym.fromModule ? "true" : "false") << "\n";
                out << "    }" << (i + 1 < table.symbols.size() ? "," : "") << "\n";
            }
            out << "  ]\n";
            out << "}\n";
            return out.str();
        }

        // ==================== 渲染：html ====================
        //
        // 单文件自包含：CSS 与 JS 内联，不依赖 CDN，离线可用。
        // 交互只有三样 —— 搜索框、种类筛选、点行展开详情 —— 都是纯原生 JS，
        // 不做框架，因为这份文件要能直接双击打开、也能塞进任何静态服务器。
        std::string renderHtml(const SymbolTable& table)
        {
            std::ostringstream out;
            out << "<!DOCTYPE html>\n<html lang=\"en\">\n<head>\n";
            out << "<meta charset=\"utf-8\">\n";
            out << "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n";
            out << "<title>Symbol table - " << escapeHtml(fileNameOf(table.mainFile))
                << "</title>\n";
            out << R"(<style>
:root {
  --bg: #ffffff; --fg: #1f2328; --muted: #656d76; --line: #d0d7de;
  --accent: #0969da; --chip: #ddf4ff; --chip-fg: #0550ae;
  --fn: #1a7f37; --cls: #8250df; --var: #bc4c00; --meth: #0969da;
}
@media (prefers-color-scheme: dark) {
  :root {
    --bg: #0d1117; --fg: #e6edf3; --muted: #8b949e; --line: #30363d;
    --accent: #58a6ff; --chip: #121d2f; --chip-fg: #79c0ff;
    --fn: #3fb950; --cls: #d2a8ff; --var: #ffa657; --meth: #58a6ff;
  }
}
* { box-sizing: border-box; }
body {
  margin: 0; padding: 24px; background: var(--bg); color: var(--fg);
  font: 14px/1.5 ui-sans-serif, -apple-system, "Segoe UI", sans-serif;
}
h1 { font-size: 20px; margin: 0 0 4px; }
.sub { color: var(--muted); margin-bottom: 20px; }
.controls { display: flex; gap: 8px; flex-wrap: wrap; margin-bottom: 16px; }
input[type=search] {
  flex: 1 1 240px; min-width: 200px; padding: 7px 10px;
  border: 1px solid var(--line); border-radius: 6px;
  background: var(--bg); color: var(--fg); font-size: 14px;
}
input[type=search]:focus { outline: 2px solid var(--accent); outline-offset: -1px; }
.chip {
  padding: 5px 11px; border: 1px solid var(--line); border-radius: 999px;
  background: transparent; color: var(--muted); cursor: pointer; font-size: 13px;
}
.chip[aria-pressed=true] { background: var(--chip); color: var(--chip-fg); border-color: var(--chip-fg); }
table { border-collapse: collapse; width: 100%; }
thead th {
  text-align: left; font-size: 12px; text-transform: uppercase; letter-spacing: .04em;
  color: var(--muted); border-bottom: 1px solid var(--line); padding: 6px 10px;
  position: sticky; top: 0; background: var(--bg);
}
tbody tr { border-bottom: 1px solid var(--line); cursor: pointer; }
tbody tr:hover { background: color-mix(in srgb, var(--accent) 8%, transparent); }
td { padding: 7px 10px; vertical-align: top; }
td.sig { font-family: ui-monospace, "Cascadia Code", Consolas, monospace; font-size: 13px; }
.kind { font-size: 11px; font-weight: 600; text-transform: uppercase; letter-spacing: .05em; }
.kind.Function { color: var(--fn); } .kind.Class { color: var(--cls); }
.kind.Variable { color: var(--var); } .kind.Method, .kind.Constructor { color: var(--meth); }
.loc { color: var(--muted); font-family: ui-monospace, Consolas, monospace; font-size: 12px; white-space: nowrap; }
.asm { color: var(--muted); font-family: ui-monospace, Consolas, monospace; font-size: 12px; }
.badge { display: inline-block; padding: 0 6px; border-radius: 4px; font-size: 11px; background: var(--chip); color: var(--chip-fg); }
.detail { display: none; background: color-mix(in srgb, var(--fg) 4%, transparent); }
tr.open + tr.detail { display: table-row; }
.detail dl { margin: 0; display: grid; grid-template-columns: max-content 1fr; gap: 4px 14px; }
.detail dt { color: var(--muted); font-size: 12px; }
.detail dd { margin: 0; font-family: ui-monospace, Consolas, monospace; font-size: 12px; }
.empty { color: var(--muted); padding: 24px 10px; text-align: center; }
</style>
)";
            out << "</head>\n<body>\n";
            out << "<h1>Symbol table</h1>\n";
            out << "<div class=\"sub\">" << escapeHtml(fileNameOf(table.mainFile))
                << " &middot; " << table.symbols.size() << " symbols";
            if (!table.moduleNames.empty())
            {
                out << " &middot; modules: " << escapeHtml([&]
                {
                    std::string s;
                    for (size_t i = 0; i < table.moduleNames.size(); ++i)
                    {
                        if (i) s += ", ";
                        s += table.moduleNames[i];
                    }
                    return s;
                }());
            }
            out << "</div>\n";

            out << "<div class=\"controls\">\n";
            out << "<input type=\"search\" id=\"q\" placeholder=\"Filter by name, owner, type, file...\" autofocus>\n";
            const SymbolKind kinds[] = {
                SymbolKind::CLASS, SymbolKind::FUNCTION, SymbolKind::METHOD,
                SymbolKind::CONSTRUCTOR, SymbolKind::VARIABLE
            };
            for (const auto kind : kinds)
            {
                size_t count = 0;
                for (const auto& sym : table.symbols)
                {
                    if (sym.kind == kind) ++count;
                }
                if (count == 0) continue;
                out << "<button class=\"chip\" data-kind=\"" << symbolKindName(kind)
                    << "\" aria-pressed=\"true\">" << symbolKindName(kind) << " " << count
                    << "</button>\n";
            }
            out << "</div>\n";

            out << "<table>\n<thead><tr><th>Kind</th><th>Signature</th><th>Location</th>"
                   "<th>Asm symbol</th></tr></thead>\n<tbody>\n";

            for (const auto& sym : table.symbols)
            {
                // data-* 里塞检索用的全文，JS 只做子串匹配，不需要额外索引。
                std::string haystack = sym.name + " " + sym.owner + " " + sym.type + " " +
                                       sym.returnType + " " + sym.file + " " + sym.asmSymbol + " " +
                                       symbolKindName(sym.kind) + " " + sym.baseClass;
                for (const auto& p : sym.params) haystack += " " + p.name + " " + p.type;

                out << "<tr class=\"sym\" data-kind=\"" << symbolKindName(sym.kind)
                    << "\" data-search=\"" << escapeHtml(lower(haystack)) << "\">";
                out << "<td><span class=\"kind " << symbolKindName(sym.kind) << "\">"
                    << symbolKindName(sym.kind) << "</span></td>";
                out << "<td class=\"sig\">" << escapeHtml(signatureOf(sym));
                if (sym.exported) out << " <span class=\"badge\">exported</span>";
                if (sym.fromModule) out << " <span class=\"badge\">module</span>";
                out << "</td>";
                out << "<td class=\"loc\">" << escapeHtml(sym.file) << ":" << sym.line << ":"
                    << sym.column << "</td>";
                out << "<td class=\"asm\">" << escapeHtml(sym.asmSymbol) << "</td>";
                out << "</tr>\n";

                // 详情行：默认折叠，点主行展开。
                out << "<tr class=\"detail\"><td colspan=\"4\"><dl>";
                out << "<dt>id</dt><dd>" << escapeHtml(sym.id) << "</dd>";
                if (!sym.baseClass.empty())
                {
                    out << "<dt>base class</dt><dd>" << escapeHtml(sym.baseClass) << "</dd>";
                }
                if (!sym.genericParams.empty())
                {
                    out << "<dt>generic params</dt><dd>";
                    for (size_t i = 0; i < sym.genericParams.size(); ++i)
                    {
                        if (i) out << ", ";
                        out << escapeHtml(sym.genericParams[i]);
                    }
                    out << "</dd>";
                }
                if (!sym.params.empty())
                {
                    out << "<dt>parameters</dt><dd>";
                    for (size_t i = 0; i < sym.params.size(); ++i)
                    {
                        if (i) out << "<br>";
                        const auto& p = sym.params[i];
                        out << escapeHtml(p.name) << ": " << escapeHtml(p.type.empty() ? "any" : p.type);
                        if (p.variadic) out << " (variadic)";
                        if (p.keyword) out << " (keyword)";
                        if (!p.defaultValue.empty()) out << " = " << escapeHtml(p.defaultValue);
                    }
                    out << "</dd>";
                }
                if (!sym.members.empty())
                {
                    out << "<dt>fields</dt><dd>";
                    for (size_t i = 0; i < sym.members.size(); ++i)
                    {
                        if (i) out << ", ";
                        out << escapeHtml(sym.members[i]);
                    }
                    out << "</dd>";
                }
                if (!sym.methods.empty())
                {
                    out << "<dt>methods</dt><dd>";
                    for (size_t i = 0; i < sym.methods.size(); ++i)
                    {
                        if (i) out << ", ";
                        out << escapeHtml(sym.methods[i]);
                    }
                    out << "</dd>";
                }
                if (!sym.calls.empty())
                {
                    out << "<dt>calls</dt><dd>";
                    for (size_t i = 0; i < sym.calls.size(); ++i)
                    {
                        if (i) out << ", ";
                        out << escapeHtml(sym.calls[i]);
                    }
                    out << "</dd>";
                }
                if (!sym.initializer.empty())
                {
                    out << "<dt>initializer</dt><dd>" << escapeHtml(sym.initializer) << "</dd>";
                }
                out << "</dl></td></tr>\n";
            }
            out << "</tbody>\n</table>\n";
            out << "<div class=\"empty\" id=\"empty\" hidden>No symbols match the filter.</div>\n";

            out << R"(<script>
(function () {
  var rows = Array.prototype.slice.call(document.querySelectorAll('tr.sym'));
  var q = document.getElementById('q');
  var empty = document.getElementById('empty');
  var chips = Array.prototype.slice.call(document.querySelectorAll('.chip'));
  var active = {};
  chips.forEach(function (c) { active[c.dataset.kind] = true; });

  function apply() {
    var needle = q.value.trim().toLowerCase();
    var shown = 0;
    rows.forEach(function (row) {
      var okKind = active[row.dataset.kind] !== false;
      var okText = !needle || row.dataset.search.indexOf(needle) !== -1;
      var show = okKind && okText;
      row.hidden = !show;
      if (!show && row.classList.contains('open')) {
        row.classList.remove('open');
        if (row.nextElementSibling) row.nextElementSibling.hidden = true;
      }
      if (show) shown++;
    });
    empty.hidden = shown !== 0;
  }

  chips.forEach(function (c) {
    c.addEventListener('click', function () {
      var on = c.getAttribute('aria-pressed') !== 'true';
      c.setAttribute('aria-pressed', String(on));
      active[c.dataset.kind] = on;
      apply();
    });
  });

  q.addEventListener('input', apply);

  rows.forEach(function (row) {
    row.addEventListener('click', function () {
      var detail = row.nextElementSibling;
      if (!detail) return;
      var open = row.classList.toggle('open');
      detail.hidden = !open;
    });
  });

  // 详情行初始隐藏
  document.querySelectorAll('tr.detail').forEach(function (d) { d.hidden = true; });
  apply();
})();
</script>
)";
            out << "</body>\n</html>\n";
            return out.str();
        }

        // ==================== 渲染：ctags ====================
        //
        // Universal Ctags 的扩展格式：`name<TAB>file<TAB>pattern;"<TAB>fields`
        // 编辑器（Vim / VS Code / Emacs）靠 pattern 在文件里定位符号，所以
        // pattern 必须是**真的能在源码里搜到**的文本：
        //   `/^[ \t]*<search>/;"`   —— `\t` 是正则里的制表符转义，不是字面量
        // 注意 `/` 是分隔符本身，只在首尾出现，中间不转义；`\` 要转义成 `\\`。
        std::string renderCtags(const SymbolTable& table)
        {
            std::ostringstream out;
            out << "!_TAG_FILE_FORMAT\t2\t/extended format/\n";
            out << "!_TAG_FILE_SORTED\t1\t/0=unsorted, 1=sorted/\n";
            out << "!_TAG_PROGRAM_NAME\tRCC\t//\n";

            auto kindLetter = [](const SymbolKind kind) -> const char*
            {
                switch (kind)
                {
                case SymbolKind::FUNCTION:    return "f";
                case SymbolKind::METHOD:      return "m";
                case SymbolKind::CONSTRUCTOR: return "c";
                case SymbolKind::CLASS:       return "C";
                case SymbolKind::VARIABLE:    return "v";
                }
                return "x";
            };

            for (const auto& sym : table.symbols)
            {
                // 在源码里搜什么：普通符号就是自己的名字；
                // 构造器在源码里没有名字，只能搜 `ctor` 关键字。
                std::string search = sym.name;
                if (sym.kind == SymbolKind::CONSTRUCTOR) search = "ctor";

                // 名字里的正则元字符要转义，否则 `a+b` 之类的名字会破坏模式。
                std::string escaped;
                for (const char c : search)
                {
                    if (c == '\\' || c == '/' || c == '.' || c == '*' || c == '+' ||
                        c == '?' || c == '[' || c == ']' || c == '(' || c == ')' ||
                        c == '{' || c == '}' || c == '^' || c == '$' || c == '|')
                    {
                        escaped += '\\';
                    }
                    escaped += c;
                }

                // 标签名：构造器报 `Class.__init__`，与汇编符号一致，
                // 这样 ctags 跳转结果能直接和 objdump 对上。
                const std::string tagName =
                    sym.kind == SymbolKind::CONSTRUCTOR
                        ? sym.owner + CTOR_TAG_SUFFIX
                        : sym.name;

                out << tagName << "\t" << sym.file << "\t/^[ \\t]*" << escaped << "/;\"";
                out << "\tkind:" << kindLetter(sym.kind);
                out << "\tline:" << sym.line;
                out << "\tcolumn:" << sym.column;
                out << "\tkindName:" << symbolKindName(sym.kind);
                if (!sym.owner.empty()) out << "\towner:" << sym.owner;
                if (!sym.type.empty()) out << "\ttype:" << sym.type;
                if (!sym.returnType.empty()) out << "\treturns:" << sym.returnType;
                if (!sym.asmSymbol.empty()) out << "\tasm:" << sym.asmSymbol;
                if (sym.exported) out << "\texported:1";
                out << "\n";
            }
            return out.str();
        }

        // ==================== 渲染：dot ====================
        //
        // 两张图放一个文件里用 subgraph 分开：
        //   cluster_inheritance  类继承关系（含基类不在本文件时的占位节点）
        //   cluster_calls        函数调用关系（只画指向本文件内定义的边）
        std::string renderDot(const SymbolTable& table)
        {
            std::ostringstream out;
            out << "digraph symbols {\n";
            out << "  graph [fontname=\"Helvetica\", rankdir=LR, compound=true];\n";
            out << "  node  [fontname=\"Helvetica\", shape=box, style=rounded];\n";
            out << "  edge  [fontname=\"Helvetica\"];\n";

            // 本文件定义的名字集合，用来判断调用边是否落在内部。
            std::set<std::string> defined;
            for (const auto& sym : table.symbols)
            {
                if (sym.kind == SymbolKind::FUNCTION || sym.kind == SymbolKind::METHOD)
                {
                    defined.insert(sym.name);
                }
            }
            (void)defined;

            // ---- 继承图 ----
            // 与调用图同理：只画主文件里的类。模块内联会把 stdlib 的
            // List/Iterable/DT 一起带进来，全画上就看不见用户自己的类了。
            bool anyInheritance = false;
            for (const auto& sym : table.symbols)
            {
                if (sym.fromModule) continue;
                if (sym.kind == SymbolKind::CLASS && !sym.baseClass.empty())
                {
                    anyInheritance = true;
                    break;
                }
            }
            if (anyInheritance)
            {
                out << "  subgraph cluster_inheritance {\n";
                out << "    label=\"Inheritance (main file only)\";\n";
                out << "    node [shape=record, color=\"#8250df\"];\n";
                for (const auto& sym : table.symbols)
                {
                    if (sym.fromModule) continue;
                    if (sym.kind != SymbolKind::CLASS) continue;
                    out << "    \"class:" << escapeJson(sym.name) << "\" [label=\"{"
                        << escapeJson(sym.name);
                    if (!sym.members.empty())
                    {
                        out << "|";
                        for (size_t i = 0; i < sym.members.size(); ++i)
                        {
                            if (i) out << "\\l";
                            out << escapeJson(sym.members[i]);
                        }
                        out << "\\l";
                    }
                    if (!sym.methods.empty())
                    {
                        out << "|";
                        for (size_t i = 0; i < sym.methods.size(); ++i)
                        {
                            if (i) out << "\\l";
                            out << escapeJson(sym.methods[i]) << "()";
                        }
                        out << "\\l";
                    }
                    out << "}\"];\n";
                }
                for (const auto& sym : table.symbols)
                {
                    if (sym.fromModule) continue;
                    if (sym.kind != SymbolKind::CLASS || sym.baseClass.empty()) continue;
                    out << "    \"class:" << escapeJson(sym.baseClass) << "\" -> \"class:"
                        << escapeJson(sym.name) << "\" [arrowhead=onormal];\n";
                }
                out << "  }\n";
            }

            // ---- 调用图 ----
            // 只画**主文件**里定义的函数之间的调用：import 会把整个 stdlib 内联
            // 进来（一个 hello world 就有 90+ 个符号），把模块内部的调用也画上，
            // 图会立刻变成一团毛线，对依赖分析毫无价值。要看模块内部结构，
            // 单独对模块文件跑一次 `-s --ext`。
            std::set<std::string> localFns;
            for (const auto& sym : table.symbols)
            {
                if (sym.fromModule) continue;
                if (sym.kind != SymbolKind::FUNCTION && sym.kind != SymbolKind::METHOD) continue;
                localFns.insert(sym.name);
            }

            bool anyCall = false;
            for (const auto& sym : table.symbols)
            {
                if (sym.fromModule) continue;
                for (const auto& callee : sym.calls)
                {
                    if (localFns.find(callee) != localFns.end() && callee != sym.name)
                    {
                        anyCall = true;
                        break;
                    }
                }
                if (anyCall) break;
            }
            if (anyCall)
            {
                out << "  subgraph cluster_calls {\n";
                out << "    label=\"Calls (main file only)\";\n";
                out << "    node [shape=box, color=\"#1a7f37\"];\n";
                std::set<std::pair<std::string, std::string>> edges;
                for (const auto& sym : table.symbols)
                {
                    if (sym.fromModule) continue;
                    if (sym.kind != SymbolKind::FUNCTION && sym.kind != SymbolKind::METHOD) continue;
                    for (const auto& callee : sym.calls)
                    {
                        if (localFns.find(callee) == localFns.end()) continue;
                        if (callee == sym.name) continue;      // 递归不画自环
                        edges.insert({sym.name, callee});
                    }
                }
                for (const auto& name : localFns)
                {
                    out << "    \"fn:" << escapeJson(name) << "\" [label=\""
                        << escapeJson(name) << "\"];\n";
                }
                for (const auto& [from, to] : edges)
                {
                    out << "    \"fn:" << escapeJson(from) << "\" -> \"fn:"
                        << escapeJson(to) << "\";\n";
                }
                out << "  }\n";
            }

            out << "}\n";
            return out.str();
        }

        // ==================== 渲染：markdown ====================
        std::string renderMarkdown(const SymbolTable& table)
        {
            std::ostringstream out;
            out << "# Symbol table: `" << fileNameOf(table.mainFile) << "`\n\n";
            out << "- Symbols: " << table.symbols.size() << "\n";
            if (!table.moduleNames.empty())
            {
                out << "- Modules: ";
                for (size_t i = 0; i < table.moduleNames.size(); ++i)
                {
                    if (i) out << ", ";
                    out << "`" << table.moduleNames[i] << "`";
                }
                out << "\n";
            }
            out << "\n";

            auto cell = [](const std::string& s)
            {
                // Markdown 表格里 `|` 必须转义，否则整行错列。
                std::string out2;
                for (const char c : s)
                {
                    if (c == '|') out2 += "\\|";
                    else if (c == '\n') out2 += " ";
                    else out2 += c;
                }
                return out2;
            };

            const SymbolKind order[] = {
                SymbolKind::CLASS, SymbolKind::FUNCTION, SymbolKind::METHOD,
                SymbolKind::CONSTRUCTOR, SymbolKind::VARIABLE
            };
            for (const auto kind : order)
            {
                std::vector<const Symbol*> group;
                for (const auto& sym : table.symbols)
                {
                    if (sym.kind == kind) group.push_back(&sym);
                }
                if (group.empty()) continue;

                // 复数：Class -> Classes，其余直接加 s。
                const std::string heading = kind == SymbolKind::CLASS
                    ? std::string("Classes")
                    : std::string(symbolKindName(kind)) + "s";
                out << "## " << heading << " (" << group.size() << ")\n\n";
                if (kind == SymbolKind::CLASS)
                {
                    out << "| Class | Base | Fields | Methods | Location |\n";
                    out << "| --- | --- | --- | --- | --- |\n";
                    for (const auto* sym : group)
                    {
                        out << "| `" << cell(sym->name) << "` | "
                            << (sym->baseClass.empty() ? "-" : "`" + cell(sym->baseClass) + "`")
                            << " | ";
                        if (sym->members.empty()) out << "-";
                        else
                        {
                            for (size_t i = 0; i < sym->members.size(); ++i)
                            {
                                if (i) out << ", ";
                                out << "`" << cell(sym->members[i]) << "`";
                            }
                        }
                        out << " | ";
                        if (sym->methods.empty()) out << "-";
                        else
                        {
                            for (size_t i = 0; i < sym->methods.size(); ++i)
                            {
                                if (i) out << ", ";
                                out << "`" << cell(sym->methods[i]) << "()`";
                            }
                        }
                        out << " | `" << cell(sym->file) << ":" << sym->line << "` |\n";
                    }
                }
                else if (kind == SymbolKind::VARIABLE)
                {
                    out << "| Name | Owner | Type | Initializer | Location |\n";
                    out << "| --- | --- | --- | --- | --- |\n";
                    for (const auto* sym : group)
                    {
                        out << "| `" << cell(sym->name) << "` | "
                            << (sym->owner.empty() ? "-" : cell(sym->owner))
                            << " | " << (sym->type.empty() ? "any" : "`" + cell(sym->type) + "`")
                            << " | "
                            << (sym->initializer.empty() ? "-" : "`" + cell(sym->initializer) + "`")
                            << " | `" << cell(sym->file) << ":" << sym->line << "` |\n";
                    }
                }
                else
                {
                    out << "| Signature | Location | Asm symbol |\n";
                    out << "| --- | --- | --- |\n";
                    for (const auto* sym : group)
                    {
                        out << "| `" << cell(signatureOf(*sym)) << "` | `" << cell(sym->file)
                            << ":" << sym->line << "` | "
                            << (sym->asmSymbol.empty() ? "-" : "`" + cell(sym->asmSymbol) + "`")
                            << " |\n";
                    }
                }
                out << "\n";
            }
            return out.str();
        }
    }
}


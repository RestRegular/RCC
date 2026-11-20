//
// rcc_module_loader.cpp —— 编译期模块内联（P8）+ 符号可见性隔离（2026-11-20）
//
// 见 rcc_module_loader.h 的设计说明。
//

#include "../../include/analyzer/rcc_module_loader.h"

#include <cctype>
#include <filesystem>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>
#include <vector>

#include "../../include/analyzer/rcc_lexer.h"
#include "../../include/analyzer/rcc_parser.h"
#include "../../include/analyzer/rcc_symbol_export.h"
#include "../../include/lib/rcc_utils.h"
#include "../../include/rcc_base.h"

namespace analyzer
{
    namespace
    {
        namespace fs = std::filesystem;

        // 规范化绝对路径，作为「同一个模块」的判据。
        std::string canonicalPath(const std::string& path)
        {
            std::error_code ec;
            fs::path abs = fs::weakly_canonical(fs::path(path), ec);
            if (ec)
            {
                abs = fs::absolute(fs::path(path), ec);
                if (ec) return utils::processRCCPath(path);
            }
            std::string s = abs.lexically_normal().string();
            for (char& c : s)
            {
#ifdef _WIN32
                if (c == '/') c = '\\';
#else
                if (c == '\\') c = '/';
#endif
                c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            }
            return s;
        }

        bool pathExists(const std::string& path)
        {
            std::error_code ec;
            return fs::is_regular_file(fs::path(path), ec);
        }

        std::string fileNameOf(const std::string& path)
        {
            return fs::path(path).filename().string();
        }

        // 从模块路径派生一个稳定的私有符号前缀（如 str -> __str）。
        std::string prefixOf(const std::string& moduleName)
        {
            std::string name = moduleName;
            if (name.size() >= 2 && name.front() == '"' && name.back() == '"')
                name = name.substr(1, name.size() - 2);
            std::string base = fs::path(name).stem().string();
            if (base.empty()) base = "mod";
            std::string p;
            for (char c : base)
            {
                if (std::isalnum(static_cast<unsigned char>(c))) p += c;
                else p += '_';
            }
            return "__" + p + "_";
        }

        // ext_info.json 里的 "main" 字段。
        std::string readExtInfoMain(const std::string& jsonPath)
        {
            const std::string content = utils::readFile(jsonPath);
            const auto keyPos = content.find("\"main\"");
            if (keyPos == std::string::npos) return {};
            const auto colonPos = content.find(':', keyPos);
            if (colonPos == std::string::npos) return {};
            const auto firstQuote = content.find('"', colonPos);
            if (firstQuote == std::string::npos) return {};
            const auto secondQuote = content.find('"', firstQuote + 1);
            if (secondQuote == std::string::npos) return {};
            return content.substr(firstQuote + 1, secondQuote - firstQuote - 1);
        }

        struct ImportRef
        {
            std::string moduleName;
            std::string alias;
        };

        using StatementPtr = std::shared_ptr<ast::StatementNode>;
        using StatementList = std::vector<StatementPtr>;

        std::shared_ptr<ast::ExpressionNode> expressionOf(const StatementPtr& stmt)
        {
            if (!stmt) return nullptr;
            if (const auto exprStmt = std::dynamic_pointer_cast<ast::ExpressionStatementNode>(stmt))
                return exprStmt->getExpression();
            return nullptr;
        }

        // 从语句里取出字符串字面量（import "str" 的第二条语句）。
        std::string stringOf(const StatementPtr& stmt)
        {
            const auto expr = expressionOf(stmt);
            if (!expr) return {};
            if (const auto str = std::dynamic_pointer_cast<ast::StringLiteralNode>(expr))
                return str->literalString();
            return {};
        }

        std::string identifierOf(const StatementPtr& stmt)
        {
            const auto expr = expressionOf(stmt);
            if (!expr) return {};
            if (const auto id = std::dynamic_pointer_cast<ast::IdentifierNode>(expr))
                return id->getName();
            return {};
        }

        // --- 顶层符号发现 ----------------------------------------------------

        struct ModuleSymbols
        {
            std::vector<std::string> all;      // 所有顶层可见候选符号
            std::set<std::string> hidden;      // 被 hidden 修饰的符号
        };

        bool hasHiddenLabel(const std::vector<std::shared_ptr<ast::LabelNode>>& labels)
        {
            for (const auto& l : labels)
                if (l && l->getFullLabel() == "hidden") return true;
            return false;
        }

        void discoverSymbols(const std::shared_ptr<ast::ProgramNode>& program,
                             ModuleSymbols& out)
        {
            if (!program) return;
            auto add = [&](const std::string& name, bool hidden) {
                if (name.empty()) return;
                out.all.push_back(name);
                if (hidden) out.hidden.insert(name);
            };
            for (const auto& stmt : program->getStatements())
            {
                const auto expr = expressionOf(stmt);
                if (!expr) continue;
                if (const auto fn = std::dynamic_pointer_cast<ast::FunctionDefinitionNode>(expr))
                {
                    std::string name;
                    const auto callNode = fn->getCallNode();
                    if (const auto id = std::dynamic_pointer_cast<ast::IdentifierNode>(callNode))
                        name = id->getName();
                    else if (const auto call = std::dynamic_pointer_cast<ast::FunctionCallNode>(callNode))
                    {
                        if (const auto id = std::dynamic_pointer_cast<ast::IdentifierNode>(call->getLeftNode()))
                            name = id->getName();
                        else if (const auto index = std::dynamic_pointer_cast<ast::IndexExpressionNode>(call->getLeftNode()))
                            if (const auto id = std::dynamic_pointer_cast<ast::IdentifierNode>(index->getLeftNode()))
                                name = id->getName();
                    }
                    else if (const auto infix = std::dynamic_pointer_cast<ast::InfixExpressionNode>(callNode))
                        if (const auto id = std::dynamic_pointer_cast<ast::IdentifierNode>(infix->getLeftNode()))
                            name = id->getName();
                    add(name, fn->hidden() || hasHiddenLabel(fn->getLabelNodes()));
                }
                else if (const auto var = std::dynamic_pointer_cast<ast::VariableDefinitionNode>(expr))
                {
                    for (const auto& vd : var->getVarDefs())
                    {
                        if (!vd) continue;
                        const auto id = vd->getNameNode();
                        if (!id) continue;
                        add(id->getName(), hasHiddenLabel(vd->getLabelNodes()));
                    }
                }
                else if (const auto cls = std::dynamic_pointer_cast<ast::ClassDefinitionNode>(expr))
                {
                    std::string name;
                    if (const auto id = std::dynamic_pointer_cast<ast::IdentifierNode>(cls->getNameNode()))
                        name = id->getName();
                    else if (const auto idx = std::dynamic_pointer_cast<ast::IndexExpressionNode>(cls->getNameNode()))
                        if (const auto id = std::dynamic_pointer_cast<ast::IdentifierNode>(idx->getLeftNode()))
                            name = id->getName();
                    std::vector<std::shared_ptr<ast::LabelNode>> labels;
                    if (const auto id = std::dynamic_pointer_cast<ast::IdentifierNode>(cls->getNameNode()))
                        labels = id->getLabels();
                    add(name, hasHiddenLabel(labels));
                }
                else if (const auto st = std::dynamic_pointer_cast<ast::StructDefinitionNode>(expr))
                {
                    std::string name;
                    std::vector<std::shared_ptr<ast::LabelNode>> labels;
                    if (const auto id = std::dynamic_pointer_cast<ast::IdentifierNode>(st->getNameNode()))
                    { name = id->getName(); labels = id->getLabels(); }
                    add(name, hasHiddenLabel(labels));
                }
            }
        }

        bool isExportStatement(const StatementPtr& stmt)
        {
            if (identifierOf(stmt) == "export") return true;
            const auto expr = expressionOf(stmt);
            if (!expr) return false;
            const auto call = std::dynamic_pointer_cast<ast::FunctionCallNode>(expr);
            if (!call) return false;
            const auto callee = std::dynamic_pointer_cast<ast::IdentifierNode>(call->getLeftNode());
            return callee && callee->getName() == "export";
        }

        // --- Token 级私有符号改名 -------------------------------------------
        // 把模块源码 token 流里的私有符号改名，定义与模块内部引用一起改。
        // 只改 TOKEN_IDENTIFIER，且跳过紧跟在 '.' 后面的 token（那是别的模块的
        // 成员/限定符号，不该被本模块的改名影响）。

        std::queue<std::shared_ptr<core::Token>>
        rewriteTokens(const std::queue<std::shared_ptr<core::Token>>& orig,
                      const std::map<std::string, std::string>& renameMap)
        {
            std::vector<std::shared_ptr<core::Token>> toks;
            for (auto q = orig; !q.empty(); q.pop()) toks.push_back(q.front());
            bool afterDot = false;
            for (auto& t : toks)
            {
                if (!t) continue;
                const bool isDot = (t->getType() == core::TokenType::TOKEN_DOT);
                const bool isIdent = (t->getType() == core::TokenType::TOKEN_IDENTIFIER);
                if (isIdent && !afterDot)
                {
                    const auto it = renameMap.find(t->getValue());
                    if (it != renameMap.end()) t->setValue(it->second);
                }
                // 只有真正的 '.' 影响 afterDot；换行/空白会重置由 lexer 决定是否产 token，
                // 这里保守起见只对紧接着 DOT 的下一个标识符跳过。
                afterDot = isDot;
            }
            std::queue<std::shared_ptr<core::Token>> out;
            for (const auto& t : toks) out.push(t);
            return out;
        }

        class ModuleLoader
        {
        public:
            ModuleLoadResult run(const std::string& sourcePath,
                                 const std::shared_ptr<ast::ProgramNode>& program)
            {
                _result.program = nullptr;
                _result.files.push_back(sourcePath);
                const std::string mainKey = canonicalPath(sourcePath);
                _loaded.insert(mainKey);
                _stack.push_back(mainKey);

                const auto statements = expandProgram(sourcePath, program);

                _stack.pop_back();
                _result.program = std::make_shared<ast::ProgramNode>(
                    program->getMainToken(), statements);
                return _result;
            }

        private:
            ModuleLoadResult _result;
            std::set<std::string> _loaded;
            std::vector<std::string> _stack;

            std::string resolveModulePath(const std::string& moduleName,
                                          const std::string& importerPath) const
            {
                std::string name = moduleName;
                if (name.size() >= 2 && name.front() == '"' && name.back() == '"')
                    name = name.substr(1, name.size() - 2);
                if (name.empty()) return {};
                const bool hasExt = fs::path(name).has_extension();
                const std::string withExt = hasExt ? name : name + ".rio";

                {
                    const std::string candidate =
                        utils::getAbsolutePath(withExt, base::RCC_BUILTIN_LIB_DIR);
                    if (pathExists(candidate)) return canonicalPath(candidate);
                }
                {
                    const std::string extDir = utils::getAbsolutePath(name, base::RCC_LIB_DIR);
                    const std::string infoPath = utils::getAbsolutePath("ext_info.json", extDir);
                    if (pathExists(infoPath))
                    {
                        const std::string entry = readExtInfoMain(infoPath);
                        if (!entry.empty())
                        {
                            const std::string entryPath = utils::getAbsolutePath(entry, extDir);
                            if (pathExists(entryPath)) return canonicalPath(entryPath);
                        }
                    }
                }
                {
                    const std::string dir = utils::getFileDirFromPath(importerPath);
                    const std::string candidate = utils::getAbsolutePath(withExt, dir);
                    if (pathExists(candidate)) return canonicalPath(candidate);
                }
                return {};
            }

            StatementList expandProgram(const std::string& filePath,
                                        const std::shared_ptr<ast::ProgramNode>& program)
            {
                StatementList result;
                if (!program) return result;
                const auto statements = program->getStatements();

                for (size_t i = 0; i < statements.size(); ++i)
                {
                    const auto& stmt = statements[i];
                    if (!stmt) continue;

                    // 单行 import：Identifier("import") 后跟一条语句。
                    if (identifierOf(stmt) == "import")
                    {
                        if (i + 1 >= statements.size())
                            throw std::runtime_error(
                                "Malformed import: expected a module path or alias after 'import' (" +
                                posOf(stmt) + ")");
                        const auto& nxt = statements[i + 1];
                        ImportRef ref;
                        if (const auto s = stringOf(nxt); !s.empty())
                        {
                            ref.moduleName = s;             // import "str"
                        }
                        else if (const auto expr = expressionOf(nxt))
                        {
                            if (const auto asg = std::dynamic_pointer_cast<ast::AssignmentNode>(expr))
                            {
                                // import lib = "lib"
                                const auto lhs = std::dynamic_pointer_cast<ast::IdentifierNode>(asg->getAssignPair().first);
                                const auto rhs = std::dynamic_pointer_cast<ast::StringLiteralNode>(asg->getAssignPair().second);
                                if (!lhs || !rhs)
                                    throw std::runtime_error(
                                        "Malformed aliased import: expected 'import name = \"module\"' (" +
                                        posOf(stmt) + ")");
                                ref.alias = lhs->getName();
                                ref.moduleName = rhs->literalString();
                            }
                        }
                        if (ref.moduleName.empty())
                            throw std::runtime_error(
                                "Malformed import: expected \"module\" or name = \"module\" (" +
                                posOf(stmt) + ")");
                        for (const auto& s : inlineModule(ref, filePath, stmt))
                            result.push_back(s);
                        ++i;  // 跳过模块路径那条语句
                        continue;
                    }

                    // 括号 import：import("str", tlib="./path")
                    if (const auto expr = expressionOf(stmt))
                    {
                        if (const auto call = std::dynamic_pointer_cast<ast::FunctionCallNode>(expr))
                        {
                            if (call->getMainToken().getValue() == "import" &&
                                std::dynamic_pointer_cast<ast::IdentifierNode>(call->getLeftNode()) &&
                                std::dynamic_pointer_cast<ast::IdentifierNode>(call->getLeftNode())
                                        ->getName() == "import")
                            {
                                std::vector<ImportRef> refs;
                                collectImportArgs(call->getRightNode(), refs);
                                if (refs.empty())
                                    throw std::runtime_error(
                                        "Malformed import: no module specified (" + posOf(stmt) + ")");
                                for (const auto& ref : refs)
                                    for (const auto& s : inlineModule(ref, filePath, stmt))
                                        result.push_back(s);
                                continue;
                            }
                        }
                    }

                    // export(...) 在合并后没有语义，整条丢掉。
                    if (isExportStatement(stmt))
                    {
                        if (identifierOf(stmt) == "export" && i + 1 < statements.size())
                        {
                            const auto nextExpr = expressionOf(statements[i + 1]);
                            if (const auto infix =
                                    std::dynamic_pointer_cast<ast::InfixExpressionNode>(nextExpr))
                            {
                                if (infix->getOpToken().getType() == core::TokenType::TOKEN_COMMA)
                                    ++i;
                            }
                            else if (std::dynamic_pointer_cast<ast::IdentifierNode>(nextExpr))
                                ++i;
                        }
                        continue;
                    }

                    result.push_back(stmt);
                }
                return result;
            }

            // 收集 import(...) 括号形式的参数。
            void collectImportArgs(const std::shared_ptr<ast::ExpressionNode>& node,
                                   std::vector<ImportRef>& out)
            {
                if (!node) return;
                if (const auto infix = std::dynamic_pointer_cast<ast::InfixExpressionNode>(node))
                {
                    if (infix->getOpToken().getType() == core::TokenType::TOKEN_COMMA)
                    {
                        collectImportArgs(infix->getLeftNode(), out);
                        collectImportArgs(infix->getRightNode(), out);
                        return;
                    }
                }
                if (const auto paren = std::dynamic_pointer_cast<ast::ParenRangerNode>(node))
                {
                    collectImportArgs(paren->getRangerNode(), out);
                    return;
                }
                if (const auto assign = std::dynamic_pointer_cast<ast::AssignmentNode>(node))
                {
                    const auto pair = assign->getAssignPair();
                    const auto aliasId = std::dynamic_pointer_cast<ast::IdentifierNode>(pair.first);
                    const auto valueStr = std::dynamic_pointer_cast<ast::StringLiteralNode>(pair.second);
                    if (aliasId && valueStr)
                        out.push_back({valueStr->literalString(), aliasId->getName()});
                    return;
                }
                if (const auto str = std::dynamic_pointer_cast<ast::StringLiteralNode>(node))
                    out.push_back({str->literalString(), {}});
            }

            StatementList inlineModule(const ImportRef& ref, const std::string& importerPath,
                                       const StatementPtr& importStmt)
            {
                StatementList empty;
                const std::string resolved = resolveModulePath(ref.moduleName, importerPath);
                std::string displayName = ref.moduleName;
                if (displayName.size() >= 2 && displayName.front() == '"' && displayName.back() == '"')
                    displayName = displayName.substr(1, displayName.size() - 2);
                if (resolved.empty())
                    throw std::runtime_error(
                        "Cannot find imported module '" + displayName + "' (imported at " +
                        posOf(importStmt) + "). Searched " +
                        base::RCC_BUILTIN_LIB_DIR + ", " + base::RCC_LIB_DIR +
                        " and the source file's directory.");

                const std::string prefix = prefixOf(ref.moduleName);
                // 去重键含前缀：同一模块被不同别名/可见性导入时生成独立副本。
                const std::string key = resolved + "|" + prefix;
                rememberName(ref.moduleName);
                if (!ref.alias.empty())
                {
                    rememberAlias(ref.alias);
                    _result.aliasPrefixes.push_back({ref.alias, prefix});
                }

                // 先查环、再去重。
                if (_loaded.find(key) != _loaded.end())
                {
                    for (const auto& entry : _stack)
                        if (entry == key)
                        {
                            std::string chain;
                            for (const auto& s : _stack) chain += fileNameOf(s) + " -> ";
                            chain += fileNameOf(resolved);
                            throw std::runtime_error("Recursive import detected: " + chain);
                        }
                    return empty;
                }
                _loaded.insert(key);
                _result.files.push_back(resolved);
                _stack.push_back(key);

                StatementList nested;
                try
                {
                    // 第一次解析：发现符号 / export / hidden。
                    const auto firstAst = parseRioFile(resolved);
                    ModuleSymbols syms;
                    discoverSymbols(firstAst, syms);
                    const auto exported = collectExportNames(firstAst);
                    const std::set<std::string> exportSet(exported.begin(), exported.end());

                    // hidden ∩ export → 编译错误。
                    for (const auto& h : syms.hidden)
                        if (exportSet.count(h))
                            throw std::runtime_error(
                                "Symbol '" + h + "' in module '" + ref.moduleName +
                                "' is both 'hidden' and exported — compilation error");

                    // 计算改名表：
                    //  - 带别名：全部顶层符号改名（外部只能通过 alias.x 访问）
                    //  - 无别名：有 export 时，仅导出符号保留原名，其余改名；
                    //            无 export 时，仅 hidden 符号改名。
                    std::map<std::string, std::string> renameMap;
                    for (const auto& name : syms.all)
                    {
                        // 带别名：全部符号改名，外部只能通过 alias.name 访问（符号隔离）。
                        // 无别名且有 export：仅导出符号保留原名（全局可见），其余改名。
                        // 无别名且无 export：仅 hidden 符号改名，其余全局可见。
                        bool visible;
                        if (!ref.alias.empty()) visible = false;
                        else if (exported.empty()) visible = !syms.hidden.count(name);
                        else visible = exportSet.count(name) > 0;
                        if (!visible) renameMap[name] = prefix + name;
                    }

                    // Token 级改名：读源码 -> 改 token -> 重新解析。
                    lexer::Lexer lexer(resolved);
                    auto tokens = lexer.tokenize();
                    auto renamed = rewriteTokens(tokens, renameMap);
                    parser::Parser parser(std::move(renamed));
                    const auto parsed = parser.parse();
                    if (parsed.first || !parsed.second)
                    {
                        std::string errors;
                        for (const auto& msg : parser.getErrorMsgs()) errors += msg + "\n";
                        throw std::runtime_error("Syntax errors in " + resolved + "\n" + errors);
                    }
                    nested = expandProgram(resolved, parsed.second);
                }
                catch (...)
                {
                    _stack.pop_back();
                    throw;
                }
                _stack.pop_back();
                return nested;
            }

            void rememberName(const std::string& name)
            {
                if (name.empty()) return;
                std::string bare = name;
                if (bare.size() > 4 && bare.substr(bare.size() - 4) == ".rio")
                    bare = bare.substr(0, bare.size() - 4);
                if (bare.size() >= 2 && bare.front() == '"' && bare.back() == '"')
                    bare = bare.substr(1, bare.size() - 2);
                for (const auto& n : _result.moduleNames)
                    if (n == bare) return;
                _result.moduleNames.push_back(bare);
            }

            void rememberAlias(const std::string& name)
            {
                if (name.empty()) return;
                for (const auto& n : _result.moduleAliases)
                    if (n == name) return;
                _result.moduleAliases.push_back(name);
            }

            static std::string posOf(const StatementPtr& stmt)
            {
                if (!stmt) return "<unknown>";
                try { return stmt->getPosStr(); }
                catch (...) { return "<unknown>"; }
            }
        };
    }

    std::shared_ptr<ast::ProgramNode> parseRioFile(const std::string& filePath)
    {
        if (!pathExists(filePath))
            throw std::runtime_error("Failed to read file: " + filePath);
        lexer::Lexer lexer(filePath);
        auto tokens = lexer.tokenize();
        parser::Parser parser(std::move(tokens));
        const auto parsed = parser.parse();
        if (parsed.first || !parsed.second)
        {
            std::string errors;
            for (const auto& msg : parser.getErrorMsgs())
                errors += msg + "\n";
            throw std::runtime_error("Syntax errors in " + filePath + "\n" + errors);
        }
        return parsed.second;
    }

    ModuleLoadResult loadProgramWithModules(const std::string& sourcePath,
                                            const std::shared_ptr<ast::ProgramNode>& program)
    {
        ModuleLoader loader;
        return loader.run(sourcePath, program);
    }
}

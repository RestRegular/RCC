//
// rcc_module_loader.cpp —— 编译期模块内联（P8）
//
// 见 rcc_module_loader.h 的设计说明。
//

#include "../../include/analyzer/rcc_module_loader.h"

#include <cctype>
#include <filesystem>
#include <set>
#include <stdexcept>
#include <vector>

#include "../../include/analyzer/rcc_lexer.h"
#include "../../include/analyzer/rcc_parser.h"
#include "../../include/lib/rcc_utils.h"
#include "../../include/rcc_base.h"

namespace analyzer
{
    namespace
    {
        namespace fs = std::filesystem;

        // 规范化绝对路径，作为「同一个模块」的判据。
        // Windows 下大小写不敏感，统一转小写，避免 ds.rio / DS.rio 被当成两个。
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

        // ext_info.json 里的 "main" 字段。这里不引 RJson（那是 legacy 层），
        // 用最小的字符串扫描：找到 "main" 键后取后面第一个字符串字面量。
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
            // 模块名（可能带 .rio 后缀或子路径）
            std::string moduleName;
            // import(alias="mod") 里的 alias；普通 import 写法为空
            std::string alias;
        };

        using StatementPtr = std::shared_ptr<ast::StatementNode>;
        using StatementList = std::vector<StatementPtr>;

        // 顶层语句剥掉 ExpressionStatementNode 外壳，取里面的表达式。
        std::shared_ptr<ast::ExpressionNode> expressionOf(const StatementPtr& stmt)
        {
            if (!stmt) return nullptr;
            if (const auto exprStmt = std::dynamic_pointer_cast<ast::ExpressionStatementNode>(stmt))
            {
                return exprStmt->getExpression();
            }
            return nullptr;
        }

        std::string identifierOf(const StatementPtr& stmt)
        {
            const auto expr = expressionOf(stmt);
            if (!expr) return {};
            if (const auto id = std::dynamic_pointer_cast<ast::IdentifierNode>(expr))
            {
                return id->getName();
            }
            return {};
        }

        // `export(...)` / `export a, b` 在合并后必须整条丢掉：
        // 模块被内联之后所有顶层定义都已经是同一翻译单元里的普通定义，
        // export 没有语义；留着它会变成对未定义符号 export 的调用（链接失败）。
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

        // `import(dt="datatype", ops="ops")` 的实参收集：
        // ParenRangerNode 里是逗号连接的一串 AssignmentNode。
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
                {
                    out.push_back({valueStr->literalString(), aliasId->getName()});
                }
                else if (const auto plainId = std::dynamic_pointer_cast<ast::IdentifierNode>(pair.second))
                {
                    // import(x=mod) 这种少见写法：值不是字符串，退化成标识符名
                    out.push_back({plainId->getName(), aliasId ? aliasId->getName() : std::string{}});
                }
                return;
            }
            if (const auto str = std::dynamic_pointer_cast<ast::StringLiteralNode>(node))
            {
                out.push_back({str->literalString(), {}});
            }
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
            std::set<std::string> _loaded;      // 已内联的文件（规范化路径）
            std::vector<std::string> _stack;    // 递归栈，用于环检测

            // 解析 import 名字 -> 绝对路径。找不到返回空串。
            // 顺序与 legacy 的 resolveImportedFilePath() 一致：
            //   1) <RCC>/Lib/builtin/<name>.rio
            //   2) <RCC>/Lib/<name>/ext_info.json 的 main 字段
            //   3) 相对导入者所在目录
            std::string resolveModulePath(const std::string& moduleName,
                                          const std::string& importerPath) const
            {
                std::string name = moduleName;
                if (name.size() >= 2 && name.front() == '"' && name.back() == '"')
                {
                    name = name.substr(1, name.size() - 2);
                }
                if (name.empty()) return {};
                const bool hasExt = fs::path(name).has_extension();
                const std::string withExt = hasExt ? name : name + ".rio";

                // 1) 内置库
                {
                    const std::string candidate =
                        utils::getAbsolutePath(withExt, base::RCC_BUILTIN_LIB_DIR);
                    if (pathExists(candidate)) return canonicalPath(candidate);
                }

                // 2) 外部扩展目录 + ext_info.json
                {
                    const std::string extDir = utils::getAbsolutePath(name, base::RCC_LIB_DIR);
                    const std::string infoPath =
                        utils::getAbsolutePath("ext_info.json", extDir);
                    if (pathExists(infoPath))
                    {
                        const std::string entry = readExtInfoMain(infoPath);
                        if (!entry.empty())
                        {
                            const std::string entryPath =
                                utils::getAbsolutePath(entry, extDir);
                            if (pathExists(entryPath)) return canonicalPath(entryPath);
                        }
                    }
                }

                // 3) 相对源文件目录
                {
                    const std::string dir = utils::getFileDirFromPath(importerPath);
                    const std::string candidate = utils::getAbsolutePath(withExt, dir);
                    if (pathExists(candidate)) return canonicalPath(candidate);
                }
                return {};
            }

            // 把 program 的语句展开成「无 import」的线性语句表；
            // 被导入模块的语句**原地**替换掉 import 语句，因此依赖总在导入者之前。
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

                    // ---------- 写法 1：`import <name>`（两条相邻语句）----------
                    if (identifierOf(stmt) == "import")
                    {
                        const std::string name = (i + 1 < statements.size())
                                                     ? identifierOf(statements[i + 1])
                                                     : std::string{};
                        if (name.empty())
                        {
                            throw std::runtime_error(
                                "Malformed import statement: 'import' must be followed by a module "
                                "name (" + posOf(stmt) + ")");
                        }
                        for (const auto& s : inlineModule({name, {}}, filePath, stmt))
                        {
                            result.push_back(s);
                        }
                        ++i;  // 跳过模块名那条语句
                        continue;
                    }

                    // ---------- 写法 2：`import(...)`（一条 FunctionCallNode）----------
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
                                {
                                    throw std::runtime_error(
                                        "Malformed import statement: no module specified (" +
                                        posOf(stmt) + ")");
                                }
                                for (const auto& ref : refs)
                                {
                                    for (const auto& s : inlineModule(ref, filePath, stmt))
                                    {
                                        result.push_back(s);
                                    }
                                }
                                continue;
                            }
                        }
                    }

                    // export(...) 在合并后没有语义，整条丢掉。
                    // 裸写形式 `export a, b` 会被 parser 拆成
                    //   ExpressionStatement(Identifier "export")
                    //   ExpressionStatement(Infix(',' a, b))
                    // 所以紧跟着的逗号链也要一起跳过。
                    if (isExportStatement(stmt))
                    {
                        if (identifierOf(stmt) == "export" && i + 1 < statements.size())
                        {
                            const auto nextExpr = expressionOf(statements[i + 1]);
                            if (const auto infix =
                                    std::dynamic_pointer_cast<ast::InfixExpressionNode>(nextExpr))
                            {
                                if (infix->getOpToken().getType() == core::TokenType::TOKEN_COMMA)
                                {
                                    ++i;
                                }
                            }
                            else if (std::dynamic_pointer_cast<ast::IdentifierNode>(nextExpr))
                            {
                                ++i;
                            }
                        }
                        continue;
                    }

                    result.push_back(stmt);
                }
                return result;
            }
            StatementList inlineModule(const ImportRef& ref, const std::string& importerPath,
                                       const StatementPtr& importStmt)
            {
                StatementList empty;
                const std::string resolved = resolveModulePath(ref.moduleName, importerPath);
                if (resolved.empty())
                {
                    throw std::runtime_error(
                        "Cannot find imported module '" + ref.moduleName + "' (imported at " +
                        posOf(importStmt) + "). Searched " +
                        base::RCC_BUILTIN_LIB_DIR + ", " + base::RCC_LIB_DIR +
                        " and the source file's directory.");
                }

                // 记下模块名（含别名），供 codegen 识别 `mod.fn(...)` 限定调用
                rememberName(ref.moduleName);
                // 别名（`import(u="utilx")` 里的 u）也是「模块名」的一种：
                // codegen 的 isModuleName 要靠它识别 `u.twice(21)` 这种限定调用。
                // 但它不该出现在用户看到的模块清单里（那会显示成导入了一个
                // 叫 u 的模块）。所以两边分开记。
                if (!ref.alias.empty()) rememberAlias(ref.alias);

                // 已内联过：不再重复插入语句（同一模块只展开一次）。
                // 注意顺序：**先**查环、再查去重。否则 A->B->A 这种真环会
                // 被去重静默吞掉（主文件一开始就在 _loaded 里），看起来像成功。
                if (_loaded.find(resolved) != _loaded.end())
                {
                    for (const auto& entry : _stack)
                    {
                        if (entry == resolved)
                        {
                            std::string chain;
                            for (const auto& s : _stack) chain += fileNameOf(s) + " -> ";
                            chain += fileNameOf(resolved);
                            throw std::runtime_error("Recursive import detected: " + chain);
                        }
                    }
                    return empty;
                }
                _loaded.insert(resolved);
                _result.files.push_back(resolved);
                _stack.push_back(resolved);
                StatementList nested;
                try
                {
                    nested = expandProgram(resolved, parseRioFile(resolved));
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
                // 去掉 .rio 后缀，只记模块名本身
                std::string bare = name;
                if (bare.size() > 4 && bare.substr(bare.size() - 4) == ".rio")
                {
                    bare = bare.substr(0, bare.size() - 4);
                }
                // import(io="io") 的模块名带引号，与裸名 `io` 是同一个模块；
                // 不剥引号会让 moduleNames 里出现 "io" 和 io 两份。
                if (bare.size() >= 2 && bare.front() == '"' && bare.back() == '"')
                {
                    bare = bare.substr(1, bare.size() - 2);
                }
                for (const auto& n : _result.moduleNames)
                {
                    if (n == bare) return;
                }
                _result.moduleNames.push_back(bare);
            }

            // 别名只进 codegen 用的名字集合，不进用户可见的模块清单。
            void rememberAlias(const std::string& name)
            {
                if (name.empty()) return;
                for (const auto& n : _result.moduleAliases)
                {
                    if (n == name) return;
                }
                _result.moduleAliases.push_back(name);
            }

            static std::string posOf(const StatementPtr& stmt)
            {
                if (!stmt) return "<unknown>";
                try
                {
                    return stmt->getPosStr();
                }
                catch (...)
                {
                    return "<unknown>";
                }
            }
        };
    }

    std::shared_ptr<ast::ProgramNode> parseRioFile(const std::string& filePath)
    {
        if (!pathExists(filePath))
        {
            throw std::runtime_error("Failed to read file: " + filePath);
        }
        lexer::Lexer lexer(filePath);
        auto tokens = lexer.tokenize();
        parser::Parser parser(std::move(tokens));
        const auto parsed = parser.parse();
        if (parsed.first || !parsed.second)
        {
            std::string errors;
            for (const auto& msg : parser.getErrorMsgs())
            {
                errors += msg;
                errors += "\n";
            }
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

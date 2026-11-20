//
// Created by RestRegular on 2025/6/28
//

#include <cerrno>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

#include "../include/rcc_base.h"
#include "../include/analyzer/rcc_lexer.h"
#include "../include/analyzer/rcc_module_loader.h"
#include "../include/analyzer/rcc_parser.h"
#include "../include/analyzer/rcc_ast_dump.h"
#include "../include/analyzer/rcc_symbol_export.h"
#include "../include/backend/rcc_codegen.h"
#include "../include/backend/rcc_toolchain.h"
#include "../include/lib/RJson/RJson_error.h"
#include "../include/lib/rcc_utils.h"

using namespace base;
using namespace utils;

namespace
{
    // utils 里没有 fileExists，这里用 filesystem 补一个
    bool fileExists(const std::string& path)
    {
        std::error_code ec;
        return std::filesystem::exists(path, ec) &&
               std::filesystem::is_regular_file(path, ec);
    }

    // 内建声明改造（2026-11-20）：隐式前导声明文件的加载。
    // 前导文件位于 RCC.exe 同级的 lib/std/builtins.rio（构建时由仓库
    // lib/std/builtins.rio 拷贝而来）。返回解析出的 AST；找不到返回 nullptr，
    // 由调用方按「库模式允许缺失、普通构建报库不完整」处理。
    std::shared_ptr<ast::ProgramNode> loadBuiltinPrelude(std::string& errorOut)
    {
        // 与 lib/std/*.rio 的构建时拷贝目标一致：release/Lib/builtin/。
        const std::string path = getAbsolutePath("./Lib/builtin/builtins.rio", getRCCDir());
        if (!fileExists(path))
        {
            errorOut = "builtin declaration prelude not found: " + path;
            return nullptr;
        }
        lexer::Lexer lexer(path);
        auto tokens = lexer.tokenize();
        parser::Parser parser(std::move(tokens));
        const auto [hasError, program] = parser.parse();
        if (hasError || !program)
        {
            for (const auto& msg : parser.getErrorMsgs()) errorOut += msg + "\n";
            return nullptr;
        }
        return program;
    }

    // 决议 C（2026-11-18 用户裁决第 2、3 条）：扫描**文件级**注释指令
    // `// @allow-cond-destroy`。
    //
    // 为什么在驱动层扫原始文本、而不在词法器里做：词法器（rcc_lexer.cpp）
    // 明确**丢弃**注释文本、不产 token，让词法器为一条指令改契约得不偿失；
    // 而「放行」本质是构建配置层面的决定，本来就不该进语法树。
    //
    // 只在**主文件**里找：指令是文件级的，import 进来的模块不该能替调用方
    // 放行 —— 否则「放行」会顺着 import 链悄悄扩散到所有使用者。
    bool scanCondDestroyDirective(const std::string& path)
    {
        std::ifstream in(path, std::ios::binary);
        if (!in)
            return false;
        std::string line;
        while (std::getline(in, line))
        {
            // 允许行首缩进与尾随空白；指令本身必须整体出现。
            const auto at = line.find("//");
            if (at == std::string::npos)
                continue;
            std::string tail = line.substr(at + 2);
            // 去掉行尾的 \r（Windows 换行）与首尾空白。
            while (!tail.empty() && (tail.back() == '\r' || tail.back() == ' ' || tail.back() == '\t'))
                tail.pop_back();
            const auto first = tail.find_first_not_of(" \t");
            if (first == std::string::npos)
                continue;
            if (tail.substr(first) == "@allow-cond-destroy")
                return true;
        }
        return false;
    }
}

// 全局参数变量
std::string __help_option_name__;
std::string __general_option_path__;
std::string __symbol_option_extension__;
std::string __general_option_output__;
std::string __working_directory__;
std::string __symbol_option_spec_symbol__;
bool __help_option__ = false;
bool __version_option__ = false;
bool __time_info__ = false;
bool __symbol_flag_builtin__ = false;
// `-s`：导出符号表
bool __symbol_flag__ = false;
// `-s -exp`：只导出 export(...) 列出的符号
bool __symbol_flag_export__ = false;
// `-s --format`：导出格式名（text/json/html/ctags/dot/markdown）
std::string __symbol_option_format__ = "text";
bool __build_flag__ = false;
// 库模式：--build 时只产出 .o，不链接可执行文件、不发射 main 入口
bool __emit_lib_flag__ = false;
// 调试：打印 AST 树后退出，不进入后端
bool __dump_ast_flag__ = false;
// 决议 C（2026-11-18）：启用「扫描文件级 `// @allow-cond-destroy` 指令」。
// 默认关闭 —— 关闭时文件里写了该指令也等于没写（用户裁决第 3 条）。
bool __allow_cond_destroy_flag__ = false;

// 参数解析器实例
ProgArgParser argParser{};

void initializeArgumentParser() {
    // Help and version flags & options
    argParser.addFlag("help", &__help_option__, false, true,
                      "Displays this help message and exits. "
                      "Use this flag to show a list of available options, flags, and their descriptions.",
                      {"h"})
    .addFlag("version", &__version_option__, false, true,
             "Displays the version information and exits. "
             "Use this flag to check the current version of the program.",
             {"v"})
    .addOption<std::string>("help-option", &__help_option_name__,
                                                     "",
                                                     "Specifies the name of the help option. "
                                                     "This option is used to customize the name of the help option displayed in the help message.",
                                                     {"ho"});

    // Symbol flag
    argParser.addFlag("symbol", &__symbol_flag__, false, true,
                      "Parse the file at the specified path and export its symbol table.",
                      {"s"})
    .addOption<std::string>("path", &__general_option_path__,
                                     "",
                                     "Specifies the target file path or directory path for the operation. ",
                                     {"p"})
    .addOption<std::string>("extension", &__symbol_option_extension__,
        "", "", {"e", "ext"})
    .addOption<std::string>("format", &__symbol_option_format__,
                                    "text",
                                    "Export format for the symbol table. "
                                    "One of: text, json, html, ctags, dot, markdown. Default 'text'.",
                                    {"fmt"})
    .addFlag("export", &__symbol_flag_export__, false,
             true, "Only display symbols listed in export(...) "
             "(all symbols are displayed by default)",
             {"exp"})
    .addOption<std::string>("output", &__general_option_output__, "console",
        "Write the result to this file instead of the console",
        {"o"})
    .addOption<std::string>("spec-symbol", &__symbol_option_spec_symbol__, "",
        "This option is used to specify the specific symbol name to be viewed for the symbol command and display a detailed description."
        "Multiple name split by spaces and the value need to be surrounded by quotes.",
        {"sp-sy", "sp-symbol", "ss"})
    .addFlag("builtin", &__symbol_flag_builtin__, false, true,
             "This flag is used to specify the target of analysis as a built-in extension.",
             {"b"})
    .addDependent(StringVector{"only-export", "specify-symbol"}, "symbol", ProgArgParser::CheckDir::UniDir)
    // 注：`--format` 不再强制要求 `--output`。那是旧实现的限制 ——
    // 旧的 json 走的是「先算完再拆字段」的路径，只能落文件；新导出器
    // 所有格式都能直接打到终端，管道里用 `-s --fmt json | jq` 很自然。
    .addMutuallyExclusive("path", "extension", ProgArgParser::CheckDir::BiDir)
    .addDependent("builtin", "extension", ProgArgParser::CheckDir::UniDir)
    .addDependent("export", "symbol", ProgArgParser::CheckDir::UniDir);

    // 原生编译 flag：Rio -> x86-64 汇编 -> .o -> .exe
    argParser.addFlag("build", &__build_flag__, false, true,
                      "Compile the target Rio file to a native Windows x64 executable.",
                      {"build"})
    .addDependent("build", "path", ProgArgParser::CheckDir::UniDir)
    .addMutuallyExclusive("build",
        std::vector<std::string>{"symbol"},
        ProgArgParser::CheckDir::BiDir);

    // 库模式：产出 .o 而不是 .exe，供 lib/rt、lib/std 这类要链接进用户程序的目标使用
    argParser.addFlag("emit-lib", &__emit_lib_flag__, false, true,
                      "With --build, emit an object file instead of an executable "
                      "(no 'main' entry point). Used for building the rt/std libraries.",
                      {"emit-lib"})
    .addDependent("emit-lib", "build", ProgArgParser::CheckDir::UniDir);

    // 调试开关：解析完成后打印 AST 树就退出，方便确认某类语法在树里的真实形态
    argParser.addFlag("dump-ast", &__dump_ast_flag__, false, true,
                      "Parse the target file and print the AST tree, then exit "
                      "without running the backend.",
                      {"dump-ast"})
    .addDependent("dump-ast", "path", ProgArgParser::CheckDir::UniDir);

    argParser.addFlag("allow-cond-destroy", &__allow_cond_destroy_flag__, false, true,
                      "Enable scanning of the file-level directive `// @allow-cond-destroy`. "
                      "A scope exit that needs a conditional destroy is a compile error by "
                      "default; it is released only when this switch is on AND the source file "
                      "carries the directive. Neither one alone releases it.",
                      {"acd"})
    .addDependent("allow-cond-destroy", "build", ProgArgParser::CheckDir::UniDir);

    argParser.addFlag("time-info", &__time_info__, false, true,
                      "Enables timing information during execution. "
                      "This flag outputs detailed timing metrics for the program's execution, "
                      "useful for performance analysis and optimization.",
                      {"ti"})
    .addOption<std::string>("working-dir", &__working_directory__,
                                     getDefaultDir(), "Specify working directory", {"wd"});

    // Core flag option exclusives
    argParser.addMutuallyExclusive(std::vector<std::string>{"help", "version"},
        std::vector<std::string>{"time-info", "symbol", "compile", "path",
            "extension", "export", "format", "output", "builtin", "spec-symbol"});
}

void handleSymbolFlag()
{
    // 目标文件解析：--path 直接给文件；--extension 从库目录里找。
    std::string targetPath;
    if (!__general_option_path__.empty())
    {
        targetPath = getAbsolutePath(__general_option_path__, __working_directory__);
    } else if (!__symbol_option_extension__.empty())
    {
        targetPath = getAbsolutePath(__symbol_flag_builtin__
            ? (__symbol_option_extension__.ends_with(".rio")
                ? __symbol_option_extension__
                : __symbol_option_extension__ + ".rio")
                : __symbol_option_extension__,
            __symbol_flag_builtin__ ? RCC_BUILTIN_LIB_DIR : RCC_LIB_DIR);
    } else
    {
        throw std::runtime_error("Missing option: --path or --extension.");
    }
    if (!fileExists(targetPath))
    {
        throw std::runtime_error("Failed to read file: " + targetPath);
    }

    // 格式
    analyzer::SymbolFormat format = analyzer::SymbolFormat::TEXT;
    std::string formatError;
    if (!analyzer::parseSymbolFormat(__symbol_option_format__, format, formatError))
    {
        throw std::runtime_error(formatError);
    }

    // 前端：词法 -> 语法 -> AST。与 --build 走同一条前端，
    // 但**不进入后端** —— 导出器只读 AST，源码编译不过时也能导出。
    lexer::Lexer lexer(targetPath);
    auto tokens = lexer.tokenize();
    parser::Parser parser(std::move(tokens));
    const auto [hasParseError, program] = parser.parse();
    if (hasParseError || !program)
    {
        std::string errors;
        for (const auto& msg : parser.getErrorMsgs()) errors += msg;
        throw std::runtime_error("Compilation failed: syntax errors.\n" + errors);
    }

    // `export(...)` 只存在于**合并前**的 AST 里 —— 模块内联会把它删掉，
    // 所以必须在 loadProgramWithModules 之前扫一遍。
    const auto exportNames = analyzer::collectExportNames(program);

    // 模块内联：import 的源码要一起进符号表，否则编辑器跳不到被导入的定义。
    analyzer::ModuleLoadResult merged;
    try
    {
        merged = analyzer::loadProgramWithModules(targetPath, program);
    }
    catch (const std::exception& e)
    {
        throw std::runtime_error(std::string("Module loading failed: ") + e.what());
    }

    auto table = analyzer::collectSymbols(merged.program, targetPath,
                                          merged.files, merged.moduleNames);
    analyzer::markExported(table, exportNames);

    // --export：只要 export(...) 列出的
    if (__symbol_flag_export__)
    {
        analyzer::filterExportedOnly(table);
    }

    // --spec-symbol：只要指定名字的
    if (!__symbol_option_spec_symbol__.empty())
    {
        const auto names = StringManager::split(__symbol_option_spec_symbol__, ' ');
        std::vector<std::string> missing;
        analyzer::filterByName(table, names, missing);
        if (!missing.empty())
        {
            std::string list;
            for (const auto& name : missing)
            {
                if (!list.empty()) list += ", ";
                list += name;
            }
            throw std::runtime_error("Symbol not found: " + list);
        }
    }

    const std::string result = analyzer::renderSymbols(table, format);

    // 输出
    if (__general_option_output__.empty() || __general_option_output__ == "console")
    {
        std::cout << result;
        return;
    }

    std::string outputPath =
        getAbsolutePath(__general_option_output__, __working_directory__);
    // 给的是目录（或没后缀）时，按格式补默认文件名。
    std::error_code ec;
    if (std::filesystem::is_directory(outputPath, ec) ||
        !std::filesystem::path(outputPath).has_extension())
    {
        std::filesystem::create_directories(outputPath, ec);
        std::string stem = targetPath;
        const auto slash = stem.find_last_of("/\\");
        if (slash != std::string::npos) stem = stem.substr(slash + 1);
        const auto dot = stem.find_last_of('.');
        if (dot != std::string::npos) stem = stem.substr(0, dot);
        outputPath = (std::filesystem::path(outputPath) /
                      (stem + analyzer::symbolFormatExtension(format))).string();
    }
    if (!writeFile(outputPath, result))
    {
        throw std::runtime_error("Failed to write output file: " + outputPath);
    }
    std::cout << "Analyze succeeded!\nOutput is saved to: " << outputPath << std::endl;
}

void handleDumpAstFlag()
{
    if (__general_option_path__.empty())
    {
        throw std::runtime_error("Missing option: --path.");
    }
    const std::string sourcePath = getAbsolutePath(__general_option_path__, __working_directory__);
    if (!fileExists(sourcePath))
    {
        throw std::runtime_error("Failed to read file: " + sourcePath);
    }

    lexer::Lexer lexer(sourcePath);
    auto tokens = lexer.tokenize();
    parser::Parser parser(std::move(tokens));
    const auto [hasParseError, program] = parser.parse();
    if (hasParseError || !program)
    {
        std::string errors;
        for (const auto& msg : parser.getErrorMsgs())
        {
            errors += msg;
        }
        throw std::runtime_error("Compilation failed: syntax errors.\n" + errors);
    }

    std::cout << ast::dumpAstTree(program);
}

void handleBuildFlag()
{
    // 解析源文件路径
    if (__general_option_path__.empty())
    {
        throw std::runtime_error("Missing option: --path.");
    }
    const std::string sourcePath = getAbsolutePath(__general_option_path__, __working_directory__);
    if (!fileExists(sourcePath))
    {
        throw std::runtime_error("Failed to read file: " + sourcePath);
    }

    // 前端：词法 -> 语法 -> AST
    lexer::Lexer lexer(sourcePath);
    auto tokens = lexer.tokenize();
    parser::Parser parser(std::move(tokens));
    // parse() 返回的 first 表示"是否出错"（历史约定，true = 有错误）
    const auto [hasParseError, program] = parser.parse();
    if (hasParseError || !program)
    {
        // 注意：不能用 printParserErrors()，它内部会 exit(-1)，
        // 会把编译器进程直接杀掉，绕过统一错误处理。
        std::string errors;
        for (const auto& msg : parser.getErrorMsgs())
        {
            errors += msg;
        }
        throw std::runtime_error("Compilation failed: syntax errors.\n" + errors);
    }

    // P8：模块加载。import 不是链接行为，而是「编译期把被导入的 .rio 源码
    // 读进来，与主文件合并成一个 ProgramNode」，因此这一步必须在 codegen 之前。
    // 合并后模块里的函数/类就是普通全局定义，后端完全不需要知道模块的存在。
    analyzer::ModuleLoadResult merged;
    try
    {
        merged = analyzer::loadProgramWithModules(sourcePath, program);
    }
    catch (const std::exception& e)
    {
        throw std::runtime_error(std::string("Module loading failed: ") + e.what());
    }

    // 后端：AST -> 汇编
    backend::Win64AbiInfo abi;
    backend::CodeGen codeGen(abi);
    codeGen.setLibraryMode(__emit_lib_flag__);
    // 决议 C：命令行开关只是「允不允许读指令」；文件里还得真有那条指令。
    // 两者都满足才放行条件销毁出口。
    codeGen.setAllowCondDestroy(__allow_cond_destroy_flag__);
    codeGen.setCondDestroyDirectiveSeen(__allow_cond_destroy_flag__ &&
                                        scanCondDestroyDirective(sourcePath));
    // 模块名 + 别名都要交给 codegen：isModuleName 用它识别 `mod.fn` / `u.fn`。
    codeGen.setModuleNames(merged.moduleNames, merged.moduleAliases);
    codeGen.setAliasPrefixes(merged.aliasPrefixes);
    // G1b：入口文件路径（绝对形式）交给后端，供 prog.entry 编译期守卫
    // 判断调用点是否来自入口文件（被 import 的模块内联后仍保留原路径）。
    codeGen.setEntryFilePath(sourcePath);
    // 内建声明改造（2026-11-20）：自动加载隐式前导声明表。
    // 库模式（-emit-lib，编译 rt/std 自身）允许前导缺失/为空——那时还没有
    // 稳定的声明表；普通构建必须加载到声明，否则后续调用点校验会全数失败，
    // 这里先明确报「库不完整」，给出可操作的提示。
    std::string preludeError;
    const auto preludeAst = loadBuiltinPrelude(preludeError);
    if (!preludeAst)
    {
        if (!__emit_lib_flag__)
            throw std::runtime_error(
                "Standard library incomplete: " + preludeError +
                "\nBuild/install lib/std/builtins.rio alongside RCC.exe.");
    }
    else
    {
        std::string declError;
        codeGen.registerDeclarations(preludeAst, &declError);
        if (!declError.empty())
            throw std::runtime_error("Standard library declaration error: " + declError);
    }
    std::string codegenError;
    const std::string assembly = codeGen.generate(merged.program, codegenError);
    if (assembly.empty())
    {
        throw std::runtime_error("Code generation failed: " + codegenError);
    }

    // 输出路径。注意 __general_option_output__ 的默认值是 "console"，
    // 那是给 -s 输出到终端的约定，对 --build 无意义，必须当作未指定。
    std::string outputBase;
    if (!__general_option_output__.empty() && __general_option_output__ != "console")
    {
        outputBase = getAbsolutePath(__general_option_output__, __working_directory__);
        if (outputBase.ends_with(".exe") || outputBase.ends_with(".obj") || outputBase.ends_with(".o")
            || outputBase.ends_with(".s"))
        {
            outputBase = outputBase.substr(0, outputBase.find_last_of('.'));
        }
    } else
    {
        outputBase = sourcePath.substr(0, sourcePath.find_last_of('.'));
    }

    const std::string asmPath = outputBase + ".s";
    const std::string objPath = outputBase + ".o";
    const std::string exePath = outputBase + ".exe";
    // 库模式只产出 .o，不链接
    const std::string finalPath = __emit_lib_flag__ ? objPath : exePath;

    // 直接写文件，不用 utils::writeFile —— 后者会经过 processRCCPath，
    // 对绝对路径可能做出非预期改写。
    {
        std::error_code ec;
        std::filesystem::path asmFsPath(asmPath);
        // 确保父目录存在
        if (asmFsPath.has_parent_path())
        {
            std::filesystem::create_directories(asmFsPath.parent_path(), ec);
        }
        std::ofstream asmFile(asmFsPath, std::ios::binary | std::ios::trunc);
        if (!asmFile.is_open())
        {
            throw std::runtime_error(
                "Failed to write assembly file: " + asmPath +
                " (errno=" + std::to_string(errno) + ")");
        }
        asmFile.write(assembly.data(), static_cast<std::streamsize>(assembly.size()));
        asmFile.close();
    }
    std::cout << "Generated assembly: " << asmPath << std::endl;

    // 工具链：汇编 -> 链接
    auto config = backend::detectToolchain();
    if (!__emit_lib_flag__ && (config.coreObject.empty() || !fileExists(config.coreObject)))
    {
        throw std::runtime_error(
            "Runtime core object not found: " + config.coreObject +
            "\nBuild it with: as -o lib/core/core_win64.o lib/core/core_win64.s");
    }

    const backend::Toolchain toolchain(config);

    std::string toolError;
    if (!toolchain.assemble(asmPath, objPath, toolError))
    {
        throw std::runtime_error("Assembler failed:\n" + toolError);
    }
    std::cout << "Assembled: " << objPath << std::endl;

    // 库模式到此为止
    if (__emit_lib_flag__)
    {
        std::cout << "Library object: " << finalPath << std::endl;
        return;
    }

    // 链接列表：用户程序 + core + rt
    std::vector<std::string> linkObjects{objPath, config.coreObject};
    if (!config.rtObject.empty()) linkObjects.push_back(config.rtObject);

    if (!toolchain.link(linkObjects, exePath, toolError))
    {
        throw std::runtime_error("Linker failed:\n" + toolError);
    }
    std::cout << "Linked executable: " << exePath << std::endl;
}

int main(const int argc, char *argv[]) {
    try {
        setDeveloperModel(true);

        // 解析命令行参数
        initializeArgumentParser();
        argParser.parse(argc, argv);

        // 处理帮助选项
        if (__help_option__) {
            if (__help_option_name__.empty()) {
                std::cout << argParser.getHelpString() << std::endl;
            } else {
                std::cout << argParser.getOptionDescription(__help_option_name__) << std::endl;
            }
        }

        // 检查导出符号信息
        if (__symbol_flag__)
        {
            handleSymbolFlag();
        }

        if (__dump_ast_flag__)
        {
            handleDumpAstFlag();
        }

        if (__build_flag__)
        {
            handleBuildFlag();
        }

        // 输出程序运行时间信息
        if (__time_info__) {
            std::cout << printProgramSpentTimeInfo();
        }

        // 处理版本选项
        if (__version_option__) {
            std::cout << "RCC(Rio Compiler Collection) " RCC_VERSION << std::endl;
            std::cout << "Published on " << std::string(__DATE__) << " at " << std::string(__TIME__) << std::endl;
            std::cout << "Copyright (C) 2025 RestRegular" << std::endl;
        }
    } catch (const RCCError &rccError) {
        std::cerr << rccError.toString() << std::endl;
        return 1;
    } catch (const error::RJsonError &rjsonError) {
        std::cerr << rjsonError.toString() << std::endl;
        return 1;
    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    } catch (...)
    {
        std::cerr << "RCC encountered internal error." << std::endl;
        return 1;
    }
    return 0;
}
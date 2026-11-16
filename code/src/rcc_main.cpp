//
// Created by RestRegular on 2025/6/28
//

#include <cerrno>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

#include "../declarations/builtin/functions/rcc_builtin_export_dec.h"
#include "../include/rcc_base.h"
#include "../include/analyzer/rcc_module_loader.h"
#include "../include/analyzer/rcc_parser.h"
#include "../include/analyzer/rcc_ast_dump.h"
#include "../include/backend/rcc_codegen.h"
#include "../include/backend/rcc_toolchain.h"
#include "../include/visitors/rcc_visitors.h"
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
bool __build_flag__ = false;
// 库模式：--build 时只产出 .o，不链接可执行文件、不发射 main 入口
bool __emit_lib_flag__ = false;
// 调试：打印 AST 树后退出，不进入后端
bool __dump_ast_flag__ = false;

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
    argParser.addFlag("symbol", &ast::CompileVisitor::__symbol_flag__, false, true,
                      "Parse the file at the specified path and output all exported symbol declarations.",
                      {"s"})
    .addOption<std::string>("path", &__general_option_path__,
                                     "",
                                     "Specifies the target file path or directory path for the operation. ",
                                     {"p"})
    .addOption<std::string>("extension", &__symbol_option_extension__,
        "", "", {"e", "ext"})
    .addOption<OutputFormat>("format", &ast::CompileVisitor::__symbol_option_format__,
                                    OutputFormat::TXT,
                                    "Specify the output mode for the analyzed content. The default mode is 'txt',"
                                    " and the supported modes include: 'json', 'txt'.",
                                    {"fmt"})
    .addFlag("export", &ast::CompileVisitor::__symbol_flag_export__, false,
             true, "Only display exported symbols (all symbols are displayed by default)",
             {"exp"})
    .addOption<std::string>("output", &__general_option_output__, "console",
        "Output results to a specified file (print to the console by default)",
        {"o"})
    .addOption<std::string>("spec-symbol", &__symbol_option_spec_symbol__, "",
        "This option is used to specify the specific symbol name to be viewed for the symbol command and display a detailed description."
        "Multiple name split by spaces and the value need to be surrounded by quotes.",
        {"sp-sy", "sp-symbol", "ss"})
    .addFlag("builtin", &__symbol_flag_builtin__, false, true,
             "This flag is used to specify the target of analysis as a built-in extension.",
             {"b"})
    .addDependent(StringVector{"only-export", "specify-symbol"}, "symbol", ProgArgParser::CheckDir::UniDir)
    .addDependent("format", "output", ProgArgParser::CheckDir::UniDir)
    .addMutuallyExclusive("spec-symbol", "fmt", ProgArgParser::CheckDir::BiDir)
    .addMutuallyExclusive("path", "extension", ProgArgParser::CheckDir::BiDir)
    .addDependent("builtin", "extension", ProgArgParser::CheckDir::UniDir)
    .addDependent("export", "symbol", ProgArgParser::CheckDir::UniDir);

    // 单独添加 compile flag 及相关配置
    argParser.addFlag("compile", &ast::CompileVisitor::__compile_flag__, false, true,
                      "Compile the target file or directory specified by path",
                      {"c"})
    .addOption<int>("compile-level", &ast::CompileVisitor::__compile_option_compile_level__, 0,
        "This option is used to specify the compile level for RCC."
        "0 -> Debug, 1 -> Testing, 2 -> Release, 3 -> Minified",
        {"cl"})
    .addDependent("compile", "output", ProgArgParser::CheckDir::UniDir)
    .addMutuallyExclusive("compile",
        std::vector<std::string>{"extension", "export", "format", "builtin", "spec-symbol", "symbol"},
        ProgArgParser::CheckDir::BiDir)
    .addDependent("compile-level", "compile", ProgArgParser::CheckDir::UniDir);

    // 原生编译 flag：Rio -> x86-64 汇编 -> .o -> .exe
    argParser.addFlag("build", &__build_flag__, false, true,
                      "Compile the target Rio file to a native Windows x64 executable.",
                      {"build"})
    .addDependent("build", "path", ProgArgParser::CheckDir::UniDir)
    .addMutuallyExclusive("build",
        std::vector<std::string>{"compile", "symbol"},
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
    if (!__symbol_option_spec_symbol__.empty())
    {
        ast::CompileVisitor::__symbol_option_format__ = OutputFormat::JSON;
    }

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
        throw std::runtime_error("Missing option: --file or --extension.");
    }

    std::string outputPath = __general_option_output__.empty() ? "console" : __general_option_output__;
    if (outputPath != "console")
    {
        outputPath = getAbsolutePath(outputPath, __working_directory__);
    }

    if (ast::CompileVisitor visitor(targetPath, targetPath,
        outputPath, false);
        visitor.compile())
    {
        const auto& result = visitor.getAnalyzeBuilder().buildAll();
        if (__symbol_option_spec_symbol__.empty())
        {
            if (outputPath == "console")
            {
                std::cout << result;
            } else if (writeFile(outputPath, result))
            {
                std::cout << "Analyze succeeded!\nOutput is saved to: " << outputPath << std::endl;
            }
        } else
        {
            const auto& specifiedNames = StringManager::split(__symbol_option_spec_symbol__, ' ');
            rjson::rj::RJsonParser parser (result);
            rjson::rj::RJsonBuilder builder (rjson::RJType::RJ_OBJECT);
            parser.parse();
            std::string errorName;
            try {
                for (const auto& name: specifiedNames)
                {
                    errorName = name;
                    builder.insertRJValue(name, *parser.getAtKey(name));
                }
                if (__general_option_output__ == "console")
                {
                    std::cout << builder.formatString(4);
                } else if (writeFile(outputPath, result))
                {
                    std::cout << "Analyze succeeded!\nOutput is saved to: " << outputPath << std::endl;
                }
            } catch (const error::RJsonError& e)
            {
                throw std::runtime_error(e.toString() + "\nNot find the specified symbol name: " + errorName);
            }
        }
    }
}

void handleCompileFlag()
{
    const auto& targetPath = getAbsolutePath(__general_option_path__, __working_directory__);
    const auto& outputPath = getAbsolutePath(__general_option_output__, __working_directory__);
    if (ast::CompileVisitor visitor (targetPath, targetPath, outputPath);
        visitor.compile())
    {
        std::cout << "Compilation succeeded!\nOutput is saved to: " << outputPath << std::endl;
    }
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

void handleLlvmFlag()
{
    throw std::runtime_error("The LLVM backend has been removed. Use --build to compile to a native executable.");
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
    codeGen.setModuleNames(merged.moduleNames);
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
        if (ast::CompileVisitor::__symbol_flag__)
        {
            handleSymbolFlag();
        }

        if (ast::CompileVisitor::__compile_flag__)
        {
            handleCompileFlag();
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
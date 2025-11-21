//
// Created by RestRegular on 2025/6/28
//

#include <chrono>
#include <iostream>
#include <string>

#include "../declarations/builtin/functions/rcc_builtin_export_dec.h"
#include "../include/rcc_base.h"
#include "../include/analyzer/rcc_parser.h"
#include "../include/visitors/rcc_visitors.h"
#include "../include/lib/RJson/RJson_error.h"
#include "../include/lib/rcc_utils.h"

using namespace base;
using namespace utils;

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

    argParser.addFlag("time-info", &__time_info__, false, true,
                      "Enables timing information during execution. "
                      "This flag outputs detailed timing metrics for the program's execution, "
                      "useful for performance analysis and optimization.",
                      {"ti"})
    .addOption<std::string>("working-dir", &__working_directory__,
                                     getWindowsDefaultDir(), "Specify working directory", {"wd"});

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
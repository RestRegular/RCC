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
std::string __target__;
std::string __output__;
std::string __working_directory__;
std::string __specify__;
bool __help_option__ = false;
bool __version_option__ = false;
bool __time_info__ = false;
bool __builtin__ = false;

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
    argParser.addFlag("symbol", &ast::CompileVisitor::_symbol_flag, false, true,
                      "Parse the file at the specified path and output all exported symbol declarations.",
                      {"s"})
    .addOption<std::string>("file", &__target__,
                                     "",
                                     "Specifies the target file path for the operation. ",
                                     {"f"})
    .addOption<OutputFormat>("format", &ast::CompileVisitor::_output_format,
                                    OutputFormat::TXT,
                                    "Specify the output mode for the analyzed content. The default mode is 'txt',"
                                    " and the supported modes include: 'json', 'txt'.",
                                    {"fmt"})
    .addFlag("only-export", &ast::CompileVisitor::_symbol_flag_only_export_option, false,
        true, "Only display exported symbols (all symbols are displayed by default)",
        {"e"})
    .addOption<std::string>("output", &__output__, "console",
        "Output results to a specified file (print to the console by default)",
        {"o"})
    .addOption<std::string>("spec-symbol", &__specify__, "",
        "This option is used to specify the specific symbol name to be viewed for the symbol command and display a detailed description."
        "Multiple name split by spaces and the value need to be surrounded by quotes.",
        {"sp-sy", "sp-symbol", "ss"})
    .addFlag("builtin", &__builtin__, false, true,
        "This flag is used to specify the target of analysis as a built-in extension.",
        {"b"})
    .addDependent(std::vector<std::string>{"symbol"},
        std::vector<std::string>{"file"}, ProgArgParser::CheckDir::UniDir)
    .addDependent(StringVector{"only-export", "specify-symbol"}, "symbol", ProgArgParser::CheckDir::UniDir)
    .addMutuallyExclusive("spec-symbol", "fmt", ProgArgParser::CheckDir::BiDir)
    .addDependent("builtin", "file");


    argParser.addFlag("time-info", &__time_info__, false, true,
                      "Enables timing information during execution. "
                      "This flag outputs detailed timing metrics for the program's execution, "
                      "useful for performance analysis and optimization.",
                      {"ti"})
    .addOption<std::string>("working-dir", &__working_directory__,
                                     getWindowsDefaultDir(), "Specify working directory", {"wd"});

    // Core flag option exclusives
    argParser.addMutuallyExclusive(std::vector<std::string>{"help", "version"},
        std::vector<std::string>{"time-info", "check-export-symbol"});
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
        if (ast::CompileVisitor::_symbol_flag)
        {
            if (!__specify__.empty())
            {
                ast::CompileVisitor::_output_format = OutputFormat::JSON;
            }

            // 处理目标文件绝对路径
            const std::string targetPath = getAbsolutePath(
                __builtin__
                    ? (__target__.ends_with(".rio")
                           ? __target__
                           : __target__ + ".rio")
                    : __target__,
                __builtin__
                    ? RCC_BUILTIN_LIB_DIR
                    : __working_directory__);

            // 确定输出方式（控制台或文件）
            std::string outputPath = __output__.empty() ? "console" : __output__;
            if (outputPath != "console")
            {
                outputPath = getAbsolutePath(outputPath, __working_directory__);
            }

            // 执行符号分析
            if (ast::CompileVisitor visitor(targetPath, targetPath,
                outputPath, false);
                visitor.compile())
            {
                const auto& result = visitor.getAnalyzeBuilder().buildAll();
                if (__specify__.empty())
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
                    const auto& specifiedNames = StringManager::split(__specify__, ' ');
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
                        if (__output__ == "console")
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

        // 输出程序运行时间信息
        if (__time_info__) {
            std::cout << printProgramSpentTimeInfo();
        }

        // 处理版本选项
        if (__version_option__) {
            std::cout << "Rio-Kit " RCC_VERSION << std::endl;
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
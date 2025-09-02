//
// Created by RestRegular on 2025/6/28
//

#include <chrono>
#include <iostream>
#include <string>
#include "../include/rcc_base.h"
#include "../include/analyzer/rcc_lexer.h"
#include "../include/analyzer/rcc_parser.h"
#include "../include/visitors/rcc_visitors.h"
#include "../include/lib/RJson/RJson_error.h"
#include "../include/lib/rcc_utils.h"

using namespace base;
using namespace utils;

// 全局参数变量
std::string __input_file_path__;
std::string __output_file_path__;
std::string __working_directory__;
std::string __log_file_path__;
std::string __help_option_name__;
bool __help_option__ = false;
bool __version_option__ = false;
bool __lexer_debug__ = false;
bool __parser_debug__ = false;
bool __visitor_debug__ = false;
bool __time_info__ = false;
bool __verbose__ = false;
bool __force_overwrite__ = false;
int __optimization_level__ = 0;

// 参数解析器实例
ProgArgParser argParser{};

void initializeArgumentParser() {
    // 帮助与版本选项
    argParser.addFlag("help", &__help_option__, false, true,
                      "显示帮助信息并退出", {"h"});
    argParser.addFlag("version", &__version_option__, false, true,
                      "显示版本信息并退出", {"v"});
    argParser.addOption<std::string>("help-option", &__help_option_name__, "",
                                    "指定要显示详细信息的选项名称", {"ho"});

    // 核心功能选项
    argParser.addOption<std::string>("input", &__input_file_path__, "",
                                    "指定输入文件路径", {"i", "in"});
    argParser.addOption<std::string>("output", &__output_file_path__,
        "", "指定输出文件路径", {"o", "out"});
    argParser.addOption<std::string>("working-dir", &__working_directory__,
                                    "", "指定工作目录", {"wd"});

    // 调试与日志选项
    argParser.addFlag("lexer-debug", &__lexer_debug__, false, true,
                      "启用词法分析器调试模式", {"ld"});
    argParser.addFlag("parser-debug", &__parser_debug__, false, true,
                      "启用语法分析器调试模式", {"pd"});
    argParser.addFlag("visitor-debug", &__visitor_debug__, false, true,
                      "启用访问器调试模式", {"vd"});
    argParser.addOption<std::string>("log-file", &__log_file_path__,
                                    "", "指定日志输出文件路径", {"lf"});
    argParser.addFlag("verbose", &__verbose__, false, true,
                      "启用详细输出模式", {"V"});

    // 编译选项
    argParser.addOption<int>("optimization", &__optimization_level__, 0,
                            "设置优化级别(0-3)", {"O"});
    argParser.addFlag("force", &__force_overwrite__, false, true,
                      "强制覆盖已存在的输出文件", {"f"});
    argParser.addFlag("time-info", &__time_info__, false, true,
                      "显示编译时间信息", {"ti"});

    // 互斥与依赖关系
    argParser.addMutuallyExclusive(std::vector<std::string>{"help", "version"},
                                  std::vector<std::string>{"input", "output", "lexer-debug",
                                                           "parser-debug", "visitor-debug"},
                                  ProgArgParser::CheckDir::BiDir);
    argParser.addMutuallyExclusive("help", "version", ProgArgParser::CheckDir::BiDir);
    argParser.addDependent("help-option", "help", ProgArgParser::CheckDir::UniDir);
    argParser.addDependent({"log-file"},
        std::vector<std::string>{"lexer-debug", "parser-debug", "visitor-debug", "verbose"},
        ProgArgParser::CheckDir::UniDir);
}

void processCompilation() {
    // 处理工作目录
    if (!__working_directory__.empty())
    {
        setProgramEnvDir(__working_directory__);
    }

    // 设置调试模式
    if (__lexer_debug__)
    {
        lexer::enableDebugMode(true);
    }
    if (__parser_debug__) {
        parser::enableDebugMode(true);
    }
    if (__visitor_debug__) {
        ast::CompileVisitor::enableDebugMode(true);
    }

    // 设置日志输出
    if (!__log_file_path__.empty()) {
        pass();
    }

    // 记录编译开始时间
    const auto start_time = std::chrono::high_resolution_clock::now();

    // 执行编译
    ast::CompileVisitor visitor(__input_file_path__,
                               __working_directory__.empty() ? __input_file_path__ : __working_directory__,
                               __output_file_path__);
    // visitor.setOptimizationLevel(__optimization_level__);
    // visitor.setVerbose(__verbose__);

    if (visitor.compile())
    {
        // 输出编译结果
        std::cout << "编译成功，结果已保存至：" << __output_file_path__ << std::endl;
    }
    // 输出时间信息
    if (__time_info__) {
        const auto end_time = std::chrono::high_resolution_clock::now();
        const auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        std::cout << "编译耗时: " << duration.count() << "ms" << std::endl;
    }
}

int main(int argc, char *argv[]) {
    try {
        initializeArgumentParser();
        argParser.parse(argc, argv);

        // 处理帮助选项
        if (__help_option__) {
            if (__help_option_name__.empty()) {
                std::cout << argParser.getHelpString() << std::endl;
            } else {
                std::cout << argParser.getOptionDescription(__help_option_name__) << std::endl;
            }
            return 0;
        }

        // 处理版本选项
        if (__version_option__) {
            std::cout << "RCC(Rio Compiler Collection) v1.0.0" << std::endl;
            std::cout << "Copyright (C) 2025 RestRegular" << std::endl;
            return 0;
        }

        // 执行编译处理
        processCompilation();

    } catch (const RCCError &rccError) {
        std::cerr << rccError.toString() << std::endl;
        return 1;
    } catch (const error::RJsonError &rjsonError) {
        std::cerr << "JSON错误: " << rjsonError.toString() << std::endl;
        return 1;
    } catch (const std::exception &e) {
        std::cerr << "错误: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
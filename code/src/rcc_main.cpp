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

// ȫ�ֲ�������
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

// ����������ʵ��
ProgArgParser argParser{};

void initializeArgumentParser() {
    // ������汾ѡ��
    argParser.addFlag("help", &__help_option__, false, true,
                      "��ʾ������Ϣ���˳�", {"h"});
    argParser.addFlag("version", &__version_option__, false, true,
                      "��ʾ�汾��Ϣ���˳�", {"v"});
    argParser.addOption<std::string>("help-option", &__help_option_name__, "",
                                    "ָ��Ҫ��ʾ��ϸ��Ϣ��ѡ������", {"ho"});

    // ���Ĺ���ѡ��
    argParser.addOption<std::string>("input", &__input_file_path__, "",
                                    "ָ�������ļ�·��", {"i", "in"});
    argParser.addOption<std::string>("output", &__output_file_path__,
        "", "ָ������ļ�·��", {"o", "out"});
    argParser.addOption<std::string>("working-dir", &__working_directory__,
                                    "", "ָ������Ŀ¼", {"wd"});

    // ��������־ѡ��
    argParser.addFlag("lexer-debug", &__lexer_debug__, false, true,
                      "���ôʷ�����������ģʽ", {"ld"});
    argParser.addFlag("parser-debug", &__parser_debug__, false, true,
                      "�����﷨����������ģʽ", {"pd"});
    argParser.addFlag("visitor-debug", &__visitor_debug__, false, true,
                      "���÷���������ģʽ", {"vd"});
    argParser.addOption<std::string>("log-file", &__log_file_path__,
                                    "", "ָ����־����ļ�·��", {"lf"});
    argParser.addFlag("verbose", &__verbose__, false, true,
                      "������ϸ���ģʽ", {"V"});

    // ����ѡ��
    argParser.addOption<int>("optimization", &__optimization_level__, 0,
                            "�����Ż�����(0-3)", {"O"});
    argParser.addFlag("force", &__force_overwrite__, false, true,
                      "ǿ�Ƹ����Ѵ��ڵ�����ļ�", {"f"});
    argParser.addFlag("time-info", &__time_info__, false, true,
                      "��ʾ����ʱ����Ϣ", {"ti"});

    // ������������ϵ
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
    // ������Ŀ¼
    if (!__working_directory__.empty())
    {
        setProgramEnvDir(__working_directory__);
    }

    // ���õ���ģʽ
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

    // ������־���
    if (!__log_file_path__.empty()) {
        pass();
    }

    // ��¼���뿪ʼʱ��
    const auto start_time = std::chrono::high_resolution_clock::now();

    // ִ�б���
    ast::CompileVisitor visitor(__input_file_path__,
                               __working_directory__.empty() ? __input_file_path__ : __working_directory__,
                               __output_file_path__);
    // visitor.setOptimizationLevel(__optimization_level__);
    // visitor.setVerbose(__verbose__);

    if (visitor.compile())
    {
        // ���������
        std::cout << "����ɹ�������ѱ�������" << __output_file_path__ << std::endl;
    }
    // ���ʱ����Ϣ
    if (__time_info__) {
        const auto end_time = std::chrono::high_resolution_clock::now();
        const auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        std::cout << "�����ʱ: " << duration.count() << "ms" << std::endl;
    }
}

int main(int argc, char *argv[]) {
    try {
        initializeArgumentParser();
        argParser.parse(argc, argv);

        // �������ѡ��
        if (__help_option__) {
            if (__help_option_name__.empty()) {
                std::cout << argParser.getHelpString() << std::endl;
            } else {
                std::cout << argParser.getOptionDescription(__help_option_name__) << std::endl;
            }
            return 0;
        }

        // ����汾ѡ��
        if (__version_option__) {
            std::cout << "RCC(Rio Compiler Collection) v1.0.0" << std::endl;
            std::cout << "Copyright (C) 2025 RestRegular" << std::endl;
            return 0;
        }

        // ִ�б��봦��
        processCompilation();

    } catch (const RCCError &rccError) {
        std::cerr << rccError.toString() << std::endl;
        return 1;
    } catch (const error::RJsonError &rjsonError) {
        std::cerr << "JSON����: " << rjsonError.toString() << std::endl;
        return 1;
    } catch (const std::exception &e) {
        std::cerr << "����: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
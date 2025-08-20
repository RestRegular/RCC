//
// Created by RestRegular on 2025/6/28
//

#include <iostream>
#include "../include/rcc_base.h"
#include "../include/analyzer/rcc_lexer.h"
#include "../include/analyzer/rcc_parser.h"
#include "../include/visitors/rcc_visitors.h"
#include "../include/lib/RJson/RJson_error.h"

void test_lexer_tokenize() {
    const auto &path = R"(D:\ClionProjects\RCC\test\test.rio)";
    auto tokens = lexer::Lexer::tokenize(path);
    while (!tokens.empty()) {
        const auto &token = tokens.front();
        std::cout << token->professionalString() << std::endl;
        tokens.pop();
    }
}

void test_parser() {
    const auto &path = R"(D:\ClionProjects\RCC\test\test.rio)";
    const auto &tokens = lexer::Lexer::tokenize(path);
    parser::Parser parser(tokens);
    const auto &[hasError, program] = parser.parse();
    auto printVisitor = ast::PrintVisitor(4);
    program->acceptVisitor(printVisitor);
    printVisitor.printAllNode();
}

void test_json_visitor() {
    const auto &path = R"(D:\ClionProjects\RCC\test\test.rio)";
    const auto &tokens = lexer::Lexer::tokenize(path);
    parser::Parser parser(tokens);
    if (const auto &[hasError, program] = parser.parse();
        hasError) {
        parser.printParserErrors();
    } else {
        auto jsonVisitor = ast::JsonVisitor();
        program->acceptVisitor(jsonVisitor);
        std::cout << jsonVisitor.getRJBuilder().formatString(2);
        const bool &_ = jsonVisitor.saveJsonToFile(R"(D:\ClionProjects\RCC\test\test.json)", 2);
    }
}

void test_compiler_visitor() {
    const auto &path = R"(D:\ClionProjects\RCC\test\test.rio)";
    const auto &output = R"(D:\ClionProjects\RVM\test\test.ra)";
    if (auto visitor = ast::CompileVisitor(path, path, output);
        visitor.compile()) {
        std::cout << visitor.getCompileResult() << std::endl;
    }
}

int main() {
    try {
        // test_lexer_tokenize();
        // test_parser();
        // test_json_visitor();
        test_compiler_visitor();
    } catch (const base::RCCError &rccError) {
        std::cerr << rccError.toString() << std::endl;
    } catch (const error::RJsonError &rjsonError) {
        std::cerr << rjsonError.toString() << std::endl;
    } catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
    }
    return 0;
}
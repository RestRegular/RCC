//
// Created by RestRegular on 2025/6/28
//

#include <iostream>
#include "../include/rcc_base.h"
#include "../include/parser/rcc_lexer.h"
#include "../include/parser/rcc_parser.h"

void test_lexer_tokenize() {
    const auto &path = R"(D:\ClionProjects\RCC\test\test.rio)";
    const auto &tokens = lexer::Lexer::tokenize(utils::readFile(path), path);
    for (const auto &token: tokens) {
        std::cout << token.professionalString() << std::endl;
    }
}

void test_parser() {
    const auto &path = R"(D:\ClionProjects\RCC\test\test.rio)";
    const auto &tokens = lexer::Lexer::tokenize(utils::readFile(path), path);
    parser::Parser parser(tokens);
    while (parser.hasNext()){
        try {
            auto node = parser.parseLiteral();
            std::cout << node->professionalString() << std::endl;
        } catch (const base::RCCParserError &_)
        {}
        parser.next();
    }
}

int main() {
    try {
//        test_lexer_tokenize();
        test_parser();
    } catch (const base::RCCError &rccError) {
        std::cerr << rccError.toString() << std::endl;
    } catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
    }
    return 0;
}
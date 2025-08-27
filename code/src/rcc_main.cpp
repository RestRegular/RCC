//
// Created by RestRegular on 2025/6/28
//

#include <iostream>
#include "../include/rcc_base.h"
#include "../include/analyzer/rcc_lexer.h"
#include "../include/analyzer/rcc_parser.h"
#include "../include/visitors/rcc_visitors.h"
#include "../include/lib/RJson/RJson_error.h"

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
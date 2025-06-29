//
// Created by RestRegular on 2025/6/28.
//

#ifndef RCC_RCC_LEXER_H
#define RCC_RCC_LEXER_H

#include <iostream>
#include <vector>
#include "../rcc_core.h"

namespace lexer {

    class Lexer {
    public:
        static std::vector<core::Token> tokenize(const std::string &code, const std::string &filepath);
    };

} // lexer

#endif //RCC_RCC_LEXER_H

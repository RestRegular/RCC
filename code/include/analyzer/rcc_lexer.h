//
// Created by RestRegular on 2025/6/28.
//

#ifndef RCC_RCC_LEXER_H
#define RCC_RCC_LEXER_H

#include <iostream>
#include <queue>
#include "../rcc_core.h"

namespace lexer {

    class Lexer {
    public:
        static std::queue<std::shared_ptr<core::Token>> tokenize(const std::string &filepath);
    };

} // lexer

#endif //RCC_RCC_LEXER_H

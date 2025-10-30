//
// Created by RestRegular on 2025/6/28.
//

#ifndef RCC_RCC_LEXER_H
#define RCC_RCC_LEXER_H

#include <queue>
#include "../rcc_core.h"
#include "../lib/rcc_utils.h"

namespace lexer {

    class Lexer {
        std::string _filepath;
        std::string _filecode;
        std::vector<std::string> _lines;
        void splitCodeToLines();
        std::queue<std::shared_ptr<core::Token>> tokens;
    public:
        explicit Lexer(const std::string &filepath, const std::string &dirpath="");

        [[nodiscard]] std::vector<std::string> getCodeLines() const;

        [[nodiscard]] std::string getCodeLine(const int &rowIndex) const;

        [[nodiscard]] std::string getCodeLine(const utils::Pos &pos) const;

        [[nodiscard]] std::string getFilepath() const;

        std::queue<std::shared_ptr<core::Token>> tokenize();
    };

    void enableDebugMode(bool cond);
} // lexer

#endif //RCC_RCC_LEXER_H

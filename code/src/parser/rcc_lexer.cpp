//
// Created by RestRegular on 2025/6/28.
//

#include <stack>
#include "../../include/rcc_base.h"
#include "../../include/parser/rcc_lexer.h"

namespace lexer {

    std::vector<core::Token> Lexer::tokenize(
            const std::string &code, const std::string &filepath=RCC_UNKNOWN_CONST) {
        int row = 1, column = 1;
        std::vector<core::Token> tokens;
        std::string t;
        std::string groupSign;
        std::stack<char> quoteStack;
        bool needEscape = false;
        core::CommentType commentType = core::CommentType::NONE;
        static std::unordered_set<char> operatorChars {
                ' ', '\n', '+', '-', '*', '/', '%', '(', ')',
                '[', ']', '{', '}', ':', ';', ',', '.', '=',
                '<', '>', '&', '!', '|', '?', '~', '^', '@'
        };

        for (char c: code) {
            column += 1;

            // 处理转义字符
            if (needEscape) {
                if (utils::StringManager::needEscape(c)) {
                    t += utils::StringManager::escapeChar(c);
                    needEscape = false;
                    continue;
                } else {
                    throw base::RCCSyntaxError::illegalEscapeCharError(
                            utils::Pos(row, column - 1, 1, filepath).getFilePosStr(), RCC_UNKNOWN_CONST, c);
                }
            }

            // 操作符或特殊字符处理
            if (operatorChars.contains(c)) {
                if (quoteStack.empty()) {
                    if (!t.empty() && commentType == core::CommentType::NONE) {
                        tokens.emplace_back(utils::Pos(row, column - t.size() - 1, t.size(), filepath), t);
                        t.clear();
                    }

                    if (c == ';') {
                        if (tokens.empty() || tokens.back().getValue() != "\n") {
                            tokens.emplace_back(utils::Pos(row, column - 1, 1, filepath), "\n");
                        }
                    } else if (c != ' ' && commentType == core::CommentType::NONE) {
                        if (c != '\n' || tokens.empty() || tokens.back().getValue() != "\n") {
                            // 直接使用字符，而不是转换为字符串
                            tokens.emplace_back(utils::Pos(row, column - 1, 1, filepath), std::string(1, c));
                        }
                    }

                    // 处理成组符号
                    if (groupSign.empty()) {
                        groupSign += c;
                    } else if (base::GROUP_SIGNS.contains(groupSign + c)) {
                        groupSign += c;
                        if (commentType == core::CommentType::NONE) {
                            tokens.pop_back();
                            tokens.pop_back();
                        }
                        if (groupSign == "//") {
                            commentType = core::CommentType::RCC_COMMENT;
                        } else if (groupSign == "/*") {
                            if (commentType == core::CommentType::NONE) {
                                if (!t.empty()) {
                                    tokens.emplace_back(utils::Pos(row, column - t.size() - 1, t.size(), filepath), t);
                                    t.clear();
                                }
                                commentType = core::CommentType::RCC_DOC_COMMENT;
                            }
                        } else if (groupSign == "*/") {
                            if (commentType == core::CommentType::NONE) {
                                throw base::RCCSyntaxError::illegalCharacterError(
                                        utils::Pos(row, column-1, groupSign.size(), filepath).toString(), RCC_UNKNOWN_CONST, groupSign[0]);
                            }
                            t.clear();
                            if (commentType == core::CommentType::RCC_DOC_COMMENT) {
                                commentType = core::CommentType::NONE;
                            }
                        } else if (commentType == core::CommentType::NONE) {
                            tokens.emplace_back(utils::Pos(row, column - 1, groupSign.size(), filepath), groupSign);
                        }
                        groupSign.clear();
                    } else {
                        // 重置groupSign为当前字符
                        groupSign = std::string(1, c);
                    }

                    // 处理换行符
                    if (c == '\n') {
                        if (commentType == core::CommentType::RCC_COMMENT) {
                            t.clear();
                            if (tokens.empty() || tokens.back().getValue() != "\n") {
                                tokens.emplace_back(utils::Pos(row, column - 1, 1, filepath), "\n");
                            }
                            commentType = core::CommentType::NONE;
                        }
                        row++;
                        column = 1;
                    }
                    continue;
                }
            }

            // 处理引号内的内容
            if (!quoteStack.empty() && c == '\\') {
                needEscape = true;
                continue;
            }

            if (quoteStack.empty()) {
                if (c != ' ' && c != '\n' && c != '\t' && commentType == core::CommentType::NONE) {
                    t += c;
                }
            } else {
                if (c == '\n' || c == '\t') {
                    throw base::RCCSyntaxError::undefinedExpressionError(
                            utils::Pos(row, column-1, 0, filepath).toString(), RCC_UNKNOWN_CONST);
                }
                t += c;
            }

            // 引号匹配（修复：仅检查单引号和双引号）
            if (c == '"' || c == '\'') {
                if (quoteStack.empty()) {
                    quoteStack.push(c);
                } else if (quoteStack.top() == c) {
                    quoteStack.pop();
                }
            }
        }

        // 检查未闭合的引号
        if (!quoteStack.empty()) {
            throw base::RCCSyntaxError::unclosedQuoteError(
                    utils::Pos(row, column, 0, filepath).toString(), RCC_UNKNOWN_CONST, quoteStack.top());
        }

        // 添加剩余的标记
        if (!t.empty()) {
            tokens.emplace_back(utils::Pos(row, column - t.size(), t.size(), filepath), t);
        }

        // 确保标记列表最后一项是 '\n'
        if (tokens.empty() || tokens.back().getValue() != "\n") {
            tokens.emplace_back(utils::Pos(0, 1, 0, filepath), "\n");
        }

        return tokens;
    }

}
//
// Created by RestRegular on 2025/6/28.
//

#include <stack>
#include "../../../include/rcc_base.h"
#include "../../../include/analyzer/rcc_lexer.h"
#include <queue>
#include <sstream>
#include <vector>

namespace lexer {

    std::queue<std::shared_ptr<core::Token>>
    Lexer::tokenize(const std::string &code,
                    const std::string &filepath = RCC_UNKNOWN_CONST) {
        size_t row = 1, column = 0;
        std::vector tokens {
            std::make_shared<core::Token>(utils::Pos(1, 0, 0, filepath))
        };
        std::stringstream t;
        std::string groupSign;
        std::stack<core::Token> quoteStack;
        bool needEscape = false;
        auto commentType = core::CommentType::NONE;
        static std::unordered_set operatorChars {
                ' ', '\n', '+', '-', '*', '/', '%', '(', ')',
                '[', ']', '{', '}', ':', ';', ',', '.', '=',
                '<', '>', '&', '!', '|', '?', '~', '^', '@'
        };

        for (size_t i = 0; i < code.size(); i++) {
            const char &c = code[i];
            column ++;

            // ����ת���ַ�
            if (needEscape) {
                if (utils::StringManager::needEscape(c)) {
                    t << c;
                    needEscape = false;
                    continue;
                }
                throw base::RCCSyntaxError::illegalEscapeCharError(
                        utils::Pos(row, column, 1, filepath).getFilePosStr(), RCC_UNKNOWN_CONST, c);
            }

            // ����������ַ�
            if (operatorChars.contains(c)) {
                if (quoteStack.empty()) {
                    // ����С��
                    if (c == '.' && t.tellp() > 0 && utils::isValidNumber(t.str()) && commentType == core::CommentType::NONE) {
                        t << c;
                        for (++i; i < code.size() && std::isdigit(code[i]); ++i) {
                            t << code[i];
                            column ++;
                        }
                        --i;
                        continue;
                    }

                    if (t.tellp() > 0 && commentType == core::CommentType::NONE) {
                        tokens.emplace_back(std::make_shared<core::Token>(utils::Pos(row, column - t.str().size(), t.str().size(), filepath), t.str()));
                        t.str("");
                        t.clear();
                    }

                    // ������
                    if (c == '-' && t.tellp() == 0 && commentType == core::CommentType::NONE) {
                        std::stringstream t_;
                        t_ << c;
                        for (size_t j = i + 1; j < code.size() && std::isdigit(code[j]); ++j) {
                            t_ << code[j];
                        }
                        std::string numStr = t_.str();
                        if (numStr.size() > 1 && utils::isValidNumber(numStr)) {
                            i += numStr.size() - 1;
                            tokens.emplace_back(std::make_shared<core::Token>(utils::Pos(row, column, numStr.size(), filepath), numStr));
                            column += numStr.size() - 1;
                            continue;
                        }
                    }

                    if (c == ';') {
                        if (tokens.empty() || tokens.back()->getValue() != "\n") {
                            tokens.emplace_back(std::make_shared<core::Token>(utils::Pos(row, column, 1, filepath), "\n"));
                        }
                    } else if (c != ' ' && commentType == core::CommentType::NONE) {
                        if (c != '\n' || tokens.empty() || tokens.back()->getValue() != "\n") {
                            tokens.emplace_back(std::make_shared<core::Token>(utils::Pos(row, column, 1, filepath), std::string(1, c)));
                        }
                    }

                    // ������Ϸ���
                    if (groupSign.empty()) {
                        groupSign = c;
                    } else if (base::GROUP_SIGNS.contains(groupSign + c)) {
                        groupSign += c;
                        if (commentType == core::CommentType::NONE) {
                            if (tokens.size() >= 2) {
                                tokens.resize(tokens.size() - 2);
                            }
                        }
                        if (groupSign == "//") {
                            commentType = core::CommentType::TOKEN_COMMENT;
                        } else if (groupSign == "/*") {
                            if (commentType == core::CommentType::NONE) {
                                if (t.tellp() > 0) {
                                    tokens.emplace_back(std::make_shared<core::Token>(utils::Pos(row, column - t.str().size(), t.str().size(), filepath), t.str()));
                                    t.str("");
                                    t.clear();
                                }
                                commentType = core::CommentType::TOKEN_DOC_COMMENT;
                            }
                        } else if (groupSign == "*/") {
                            if (commentType == core::CommentType::NONE) {
                                throw base::RCCSyntaxError::illegalCharacterError(
                                        utils::Pos(row, column, groupSign.size(), filepath).toString(), RCC_UNKNOWN_CONST, groupSign[0]);
                            }
                            t.str("");
                            t.clear();
                            if (commentType == core::CommentType::TOKEN_DOC_COMMENT) {
                                commentType = core::CommentType::NONE;
                            }
                        } else if (commentType == core::CommentType::NONE) {
                            tokens.emplace_back(std::make_shared<core::Token>(utils::Pos(row, column - (groupSign.size() - 1), groupSign.size(), filepath), groupSign));
                        }
                        groupSign.clear();
                    } else {
                        groupSign = std::string(1, c);
                    }

                    // �����з�
                    if (c == '\n') {
                        if (commentType == core::CommentType::TOKEN_COMMENT) {
                            t.str("");
                            t.clear();
                            if (tokens.empty() || tokens.back()->getValue() != "\n") {
                                tokens.emplace_back(std::make_shared<core::Token>(utils::Pos(row, column, 1, filepath), "\n"));
                            }
                            commentType = core::CommentType::NONE;
                        }
                        ++row;
                        column = 0;
                    }
                    continue;
                }
            }

            // ����ת���ַ�
            if (!quoteStack.empty() && c == '\\') {
                needEscape = true;
                t << c;
                continue;
            }

            // �����ַ���
            if (quoteStack.empty()) {
                if (c != ' ' && c != '\n' && c != '\t' && commentType == core::CommentType::NONE) {
                    t << c;
                }
            }
            else {
                if (c == '\n' || c == '\t') {
                    throw base::RCCSyntaxError::undefinedExpressionError(
                            utils::Pos(row, column, 0, filepath).toString(), RCC_UNKNOWN_CONST);
                }
                t << c;
            }

            // ����ƥ��
            if (c == '"' || c == '\'') {
                if (quoteStack.empty()) {
                    quoteStack.push(core::Token(utils::Pos(row, column, 1, filepath), std::string{c}));
                } else if (quoteStack.top().getValue()[0] == c) {
                    quoteStack.pop();
                }
            }
        }

        // ����δ�պϵ�����
        if (!quoteStack.empty()) {
            throw base::RCCSyntaxError::unclosedQuoteError(
                utils::Pos(row, column + 1, 0, filepath).toString(), quoteStack.top().getPosString(), RCC_UNKNOWN_CONST, quoteStack.top().getValue()[0]);
        }

        // ����ʣ����ı�
        if (!t.str().empty()) {
            tokens.emplace_back(std::make_shared<core::Token>(utils::Pos(row, column - t.str().size() + 1, t.str().size(), filepath), t.str()));
        }

        // ȷ�� token �б����һ���� '\n'
        if (tokens.empty() || tokens.back()->getValue() != "\n") {
            tokens.emplace_back(std::make_shared<core::Token>(utils::Pos(row, column + 1, 1, filepath), "\n"));
        }

        // �� vector ת��Ϊ queue
        std::queue<std::shared_ptr<core::Token>> result;
        for (const auto& token : tokens) {
            result.push(token);
        }

        return result;
    }

}
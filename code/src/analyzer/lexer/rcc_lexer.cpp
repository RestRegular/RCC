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
    Lexer::Lexer(const std::string& filepath, const std::string &dirpath)
        : _filepath(dirpath.empty() ? filepath : utils::getAbsolutePath(filepath, dirpath)){
        _filecode = utils::readFile(filepath);
        splitCodeToLines();
    }

    std::vector<std::string> Lexer::getCodeLines() const
    {
        return _lines;
    }

    std::string Lexer::getCodeLine(const int& rowIndex) const
    {
        if (rowIndex >= _lines.size() || rowIndex < 0) {
            return "";
        }
        return _lines[rowIndex - 1];
    }

    std::string Lexer::getCodeLine(const utils::Pos& pos) const
    {
        if (!utils::checkPathEqual(pos.getFilepath(), _filepath))
        {
            return "";
        }
        if (pos.getLine() >= _lines.size()) {
            return "";
        }
        return _lines[pos.getLine() - 1];
    }

    void Lexer::splitCodeToLines()
    {
        const std::string &code = utils::readFile(_filepath);
        std::stringstream ss(code);
        std::string line;
        while (std::getline(ss, line)) {
            _lines.push_back(line);
        }
    }

    std::queue<std::shared_ptr<core::Token>>
    Lexer::tokenize()
    {
        if (!tokens.empty())
        {
            return tokens;
        }
        size_t row = 1, column = 0;
        std::vector tempTokens {
            std::make_shared<core::Token>(utils::Pos(1, 0, 0, _filepath))
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

        for (size_t i = 0; i < _filecode.size(); i++) {
            const char &c = _filecode[i];
            column ++;

            // 处理转义字符
            if (needEscape) {
                if (utils::StringManager::needEscape(c)) {
                    t << c;
                    needEscape = false;
                    continue;
                }
                throw base::RCCSyntaxError::illegalEscapeCharError(
                        utils::Pos(row, column, 1, _filepath).getFilePosStr(), RCC_UNKNOWN_CONST, c);
            }

            // 处理操作符字符
            if (operatorChars.contains(c)) {
                if (quoteStack.empty()) {
                    // 处理小数
                    if (c == '.' && t.tellp() > 0 && utils::isValidNumber(t.str()) && commentType == core::CommentType::NONE) {
                        t << c;
                        for (++i; i < _filecode.size() && std::isdigit(_filecode[i]); ++i) {
                            t << _filecode[i];
                            column ++;
                        }
                        --i;
                        continue;
                    }

                    if (t.tellp() > 0 && commentType == core::CommentType::NONE) {
                        tempTokens.emplace_back(std::make_shared<core::Token>(utils::Pos(row, column - t.str().size(), t.str().size(), _filepath), t.str()));
                        t.str("");
                        t.clear();
                    }

                    // 处理负数
                    if (c == '-' && t.tellp() == 0 && commentType == core::CommentType::NONE) {
                        std::stringstream t_;
                        t_ << c;
                        for (size_t j = i + 1; j < _filecode.size() && std::isdigit(_filecode[j]); ++j) {
                            t_ << _filecode[j];
                        }
                        if (std::string numStr = t_.str();
                            numStr.size() > 1 && utils::isValidNumber(numStr)) {
                            i += numStr.size() - 1;
                            tempTokens.emplace_back(std::make_shared<core::Token>(utils::Pos(row, column, numStr.size(), _filepath), numStr));
                            column += numStr.size() - 1;
                            continue;
                        }
                    }

                    if (c == ';') {
                        if (tempTokens.empty() || tempTokens.back()->getValue() != "\n") {
                            tempTokens.emplace_back(std::make_shared<core::Token>(utils::Pos(row, column, 1, _filepath), "\n"));
                        }
                    } else if (c != ' ' && commentType == core::CommentType::NONE) {
                        if (c != '\n' || tempTokens.empty() || tempTokens.back()->getValue() != "\n") {
                            tempTokens.emplace_back(std::make_shared<core::Token>(utils::Pos(row, column, 1, _filepath), std::string(1, c)));
                        }
                    }

                    // 处理组合符号
                    if (groupSign.empty()) {
                        groupSign = c;
                    } else if (base::GROUP_SIGNS.contains(groupSign + c)) {
                        groupSign += c;
                        if (commentType == core::CommentType::NONE) {
                            if (tempTokens.size() >= 2) {
                                tempTokens.resize(tempTokens.size() - 2);
                            }
                        }
                        if (groupSign == "//" && commentType == core::CommentType::NONE) {
                            commentType = core::CommentType::SINGLE_LINE_COMMENT;
                        } else if (groupSign == "/*") {
                            if (commentType == core::CommentType::NONE) {
                                if (t.tellp() > 0) {
                                    tempTokens.emplace_back(std::make_shared<core::Token>(utils::Pos(row, column - t.str().size(), t.str().size(), _filepath), t.str()));
                                    t.str("");
                                    t.clear();
                                }
                                commentType = core::CommentType::DOC_COMMENT;
                            }
                        } else if (groupSign == "*/") {
                            if (commentType == core::CommentType::NONE) {
                                throw base::RCCSyntaxError::illegalSymbolError(
                                    utils::Pos(row, column, groupSign.size(), _filepath).toString(), RCC_UNKNOWN_CONST, groupSign);
                            }
                            t.str("");
                            t.clear();
                            if (commentType == core::CommentType::DOC_COMMENT) {
                                commentType = core::CommentType::NONE;
                            }
                        } else if (commentType == core::CommentType::NONE) {
                            tempTokens.emplace_back(std::make_shared<core::Token>(utils::Pos(row, column - (groupSign.size() - 1), groupSign.size(), _filepath), groupSign));
                        }
                        groupSign.clear();
                    } else {
                        groupSign = std::string(1, c);
                    }

                    // 处理换行符
                    if (c == '\n') {
                        if (commentType == core::CommentType::SINGLE_LINE_COMMENT) {
                            t.str("");
                            t.clear();
                            if (tempTokens.empty() || tempTokens.back()->getValue() != "\n") {
                                tempTokens.emplace_back(std::make_shared<core::Token>(utils::Pos(row, column, 1, _filepath), "\n"));
                            }
                            commentType = core::CommentType::NONE;
                        }
                        ++row;
                        column = 0;
                    }
                    continue;
                }
            }

            // 处理转义字符
            if (!quoteStack.empty() && c == '\\') {
                needEscape = true;
                t << c;
                continue;
            }

            // 处理字符串
            if (quoteStack.empty()) {
                if (c != ' ' && c != '\n' && c != '\t' && commentType == core::CommentType::NONE) {
                    t << c;
                }
            }
            else {
                if (c == '\n' || c == '\t') {
                    throw base::RCCSyntaxError::undefinedExpressionError(
                            utils::Pos(row, column, 0, _filepath).toString(), RCC_UNKNOWN_CONST);
                }
                t << c;
            }

            // 引号匹配
            if (c == '"' || c == '\'') {
                if (quoteStack.empty()) {
                    quoteStack.push(core::Token(utils::Pos(row, column, 1, _filepath), std::string{c}));
                } else if (quoteStack.top().getValue()[0] == c) {
                    quoteStack.pop();
                }
            }
        }

        // 处理未闭合的引号
        if (!quoteStack.empty()) {
            throw base::RCCSyntaxError::unclosedQuoteError(
                utils::Pos(row, column + 1, 0, _filepath).toString(), quoteStack.top().getPosString(), RCC_UNKNOWN_CONST, quoteStack.top().getValue()[0]);
        }

        // 处理剩余的文本
        if (!t.str().empty() && commentType == core::CommentType::NONE) {
            tempTokens.emplace_back(std::make_shared<core::Token>(utils::Pos(row, column - t.str().size() + 1, t.str().size(), _filepath), t.str()));
        }

        // 确保 token 列表最后一个是 '\n'
        if (tempTokens.empty() || tempTokens.back()->getValue() != "\n") {
            tempTokens.emplace_back(std::make_shared<core::Token>(utils::Pos(row, column + 1, 1, _filepath), "\n"));
        }

        // 将 vector 转换为 queue
        for (const auto& token : tempTokens) {
            tokens.push(token);
        }

        return tokens;
    }

}
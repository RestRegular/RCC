//
// Created by RestRegular on 2025/6/28.
//

#include <utility>

#include "../include/rcc_base.h"

namespace base {

    std::stack<std::string> PROGRAM_WORKING_DIRECTORY_STACK{};

    const StringSet KEYWORDS = {
            "var", "fun", "if", "else", "elif", "for", "while", "until",
            "repeat", "ret", "break", "class", "iter", "super",
            "pass", "ctor", "encapsulated", "try", "catch", "finally", "throw",
            "rasm", "link"
    };

    const StringSet OPERATORS = {
            "+", "-", "*", "/", "%", "^", "=", "+=", "-=", "*=", "/=", "%=",
            "^=", "==", "!=", ">", "<", ">=", "<=", "&&", "||", "!", ".", "?",
            "->", ".*", "**", "~", "++", "--", "\\", "&"
    };

    const StringSet ARITHMETIC_OPERATORS = {
            "+", "-", "*", "/", "%", "^"
    };

    const StringSet ASSIGN_OPERATORS = {
            "=", "+=", "-=", "*=", "/=", "%=", "^=", "++", "--"
    };

    const StringSet ENHANCED_ASSIGN_OPERATORS = {
            "+=", "-=", "*=", "/=", "%=", "^=", "++", "--"
    };

    // 使用unordered_map替代Python的字典和有序map
    const StringMap ENH_MAT_ORD_OPTORS = {
            {"+=", "+"}, {"-=", "-"},
            {"*=", "*"}, {"/=", "/"},
            {"%=", "%"}, {"^=", "^"},
            {"++", "+"}, {"--", "-"}
    };

    const StringSet COMPARISON_OPERATOR = {
            "==", "!=", ">", "<", ">=", "<="
    };

    const StringSet RELATIONAL_OPERATOR = {
            "&&", "||", "!"
    };

    const StringSet OTHER_OPERATOR = {
            ".", "->", "?", ".*", "**", "~", "\\", "&"
    };

    const StringSet COMMENT_OPERATOR = {
            "//", "/*", "*/"
    };

    const std::unordered_map<std::string, std::shared_ptr<StringSet>> CLASSIFY_OPERATORS = {
            {"arithmetic", std::make_shared<StringSet>(ARITHMETIC_OPERATORS)},
            {"assign", std::make_shared<StringSet>(ASSIGN_OPERATORS)},
            {"comparison", std::make_shared<StringSet>(COMPARISON_OPERATOR)},
            {"relational", std::make_shared<StringSet>(RELATIONAL_OPERATOR)},
            {"other", std::make_shared<StringSet>(OTHER_OPERATOR)},
            {"enhanced_assign", std::make_shared<StringSet>(ENHANCED_ASSIGN_OPERATORS)}
    };
    
    const std::vector<std::shared_ptr<StringSet>> OPERATOR_LIST = {
            std::make_shared<StringSet>(ARITHMETIC_OPERATORS),
            std::make_shared<StringSet>(ASSIGN_OPERATORS),
            std::make_shared<StringSet>(COMPARISON_OPERATOR),
            std::make_shared<StringSet>(RELATIONAL_OPERATOR),
            std::make_shared<StringSet>(OTHER_OPERATOR)
    };

    const StringSet DELIMITERS = {";", "\n", ",", ":", "@"};

    const StringSet RANGERS = {"{", "}", "(", ")", "[", "]"};

    const StringMap RANGER_MATCH = {
            {"{", "}"},
            {"(", ")"},
            {"[", "]"}
    };

    const StringSet CONSTANTS = {"true", "false", "null", "NaN"};

    const StringSet TYPES = {
            "int", "float", "str", "bool", "char", "any",
            "list", "dict", "series", "func", "void"
    };

    const StringSet DESCRIBE_LABELS = {
        // access permission label
        "public", "private", "protected", "builtin",
        // restrict label
        "const", "quote",
        // life circle label
        "static", "global",
        // object-oriented feature label
        "overwrite", "interface", "virtual",
        // data type label
        "int", "float", "str", "bool", "char", "any",
        "list", "dict", "series", "func", "void", "nul"
    };

    const std::string NULL_VALUE = "null";

    const std::unordered_map<std::string, int> OPERATOR_PRECEDENCE = {
            {"+", 1}, {"-", 1},
            {"*", 2}, {"/", 2}, {"%", 2},
            {"^", 3},
            {"==", 4}, {"!=", 4},
            {">", 5}, {"<", 5}, {">=", 5}, {"<=", 5},
            {"&&", 6},
            {"||", 7},
            {"!", 8}
    };

    const StringVector OPERATOR_PRECEDENCE_LIST = {
            "!",    // 逻辑非
            "^",    // 次方运算（右结合）
            "*", "/", "%",  // 乘法、除法、取模
            "+", "-",  // 加法、减法
            "<", "<=", ">", ">=",  // 关系运算符
            "==", "!=",  // 相等性比较
            "&&",  // 逻辑与
            "||",  // 逻辑或
    };

    const StringSet GROUP_SIGNS = {
            "+=", "-=", "*=", "/=", "==", ">=", "<=",
            "->", "//", "/*", "*/", "||", "&&", "!=", "**", ".*", "++", "--"
    };

    const StringMap RELATION_MAP = {
        {"==", "RE"}, {"!=", "RNE"}, {">", "RG"}, {">=", "RGE"},
        {"<", "RL"}, {"<=", "RLE"}, {"&&", "AND"}, {"||", "OR"}
    };

    bool containsKeyword(const std::string &str) {
        return KEYWORDS.contains(str);
    }

    std::string getErrorTypeName(const ErrorType &error_type) {
        switch (error_type)
        {
        case ErrorType::SYNTAX_ERROR: return "SyntaxError";
        case ErrorType::PARSER_ERROR: return "ParserError";
        case ErrorType::COMPILER_ERROR: return "CompilerError";
        default: return "UndefinedError";
        }
    }

    RCCError::RCCError(ErrorType error_type, std::string error_position, std::string error_line,
                       std::vector<std::string> error_info, std::vector<std::string> repair_tips)
                       : error_type(error_type), error_type_name(getErrorTypeName(error_type)), error_position(std::move(error_position)),
                         error_line(std::move(error_line)), error_info(std::move(error_info)), repair_tips(std::move(repair_tips)){}

    [[maybe_unused]] RCCError::RCCError(std::string error_type, std::string error_position, std::string error_line,
                       std::vector<std::string> error_info, std::vector<std::string> repair_tips)
                       : error_type(), error_type_name(std::move(error_type)), error_position(std::move(error_position)),
                         error_line(std::move(error_line)), error_info(std::move(error_info)), repair_tips(std::move(repair_tips)){}

    std::string RCCError::getErrorLine(const size_t &spaceSize) const {
        std::ostringstream  oss;
        if (!error_position.empty() && error_position != RCC_UNKNOWN_CONST) {
            oss << std::string(spaceSize, ' ') << "[ Line ] : "
                << utils::StringManager::wrapText(error_position, 80, spaceSize + 10, "", "~ ") << "\n";
            if (!error_line.empty() && error_line != RCC_UNKNOWN_CONST) {
                oss << std::string(spaceSize + 9, ' ') << "| "
                        << utils::StringManager::wrapText(error_line, 80, spaceSize + 9, "", "| ~ ") << "\n";
            }
            oss << "\n";
        }
        return oss.str();
    }

    std::string RCCError::getErrorPosition(const size_t &spaceSize) const {
        if (!error_position.empty()) {
            return std::string(spaceSize, ' ') + "[ Pos  ] : " +
            utils::StringManager::wrapText(error_position, 80, spaceSize + 10, "", "~ ") + "\n\n";
        }
        return {};
    }

    std::string RCCError::getErrorTip(const std::size_t &spaceSize) const {
        if (!repair_tips.empty()) {
            std::stringstream ss;
            ss << utils::spaceString(spaceSize) << "[ Tips ] : ";
            for (size_t i = 0; i < repair_tips.size(); ++i) {
                ss << utils::StringManager::wrapText(repair_tips[i], 80, spaceSize + 10, "", "~ ");
                if (i < repair_tips.size() - 1) {
                    ss << "\n" << std::string(spaceSize + 9, ' ') << "- ";
                }
            }
            ss << "\n";
            return ss.str();
        }
        return {};
    }

    std::string RCCError::toString() const {
        const auto &space_size = space.size();
        std::ostringstream oss;

        oss << getErrorTitle();

        if (!trace_info.empty()) {
            oss << " [ Trace Back ]\n" << utils::listJoin(trace_info) << "\n";
        }

        if (!error_line.empty()) {
            oss << getErrorLine(space_size);
        }

        if (!error_info.empty()) {
            oss << getErrorInfo(space_size);
        }

        if (!repair_tips.empty()) {
            oss << getErrorTip(space_size);
        }

        return oss.str();
    }

    std::string RCCError::getErrorInfo(const size_t &spaceSize) const {
        std::ostringstream oss;
        oss << std::string(spaceSize, ' ') << "[ Info ] : ";
        for (size_t i = 0; i < error_info.size(); ++i) {
            if (!error_info[i].empty() && error_info[i] != RCC_UNKNOWN_CONST) {
                oss << utils::StringManager::wrapText(error_info[i], 80, spaceSize + 10, "", "~ ", true);
                if (i < error_info.size() - 1 &&
                    !error_info[i + 1].empty() && error_info[i + 1] != RCC_UNKNOWN_CONST) {
                    oss << "\n" << std::string(spaceSize + 9, ' ') << "- ";
                }
            }
        }
        oss << "\n\n";
        return oss.str();
    }

    std::string RCCError::getErrorTitle() const {
        std::ostringstream oss;
        oss << "\n" << std::string(20, '=');
        oss << "[ " << error_type_name << " ]" << std::string(60, '=') << "\n";
        return oss.str();
    }

    std::string scopeLeaderRecord = "";

    std::string RCCError::makeTraceInfo(const std::string &file_record_, const std::string &error_pos_filepath,
                                     const std::string &utils_getPosStrFromFilePath,
                                     const std::string &makeFileIdentiFromPath, const std::string &trace_info,
                                     const std::string &error_pos_str, const std::string &raw_code,
                                     const std::string &scope_leader_pos, const std::string &scope_leader_code) {
        std::stringstream ss;
        if (!scope_leader_pos.empty() && !scope_leader_code.empty())
        {
            ss << std::string(7, ' ') << "| " << utils::StringManager::wrapText(scope_leader_pos, 80, 17, "", "~ ")
               << "\n";
            ss << std::string(7, ' ') << "v " << utils::StringManager::wrapText(scope_leader_code, 80, 15, "", "| ~ ")
               << "\n";
        }
        if (!error_pos_str.empty() && !raw_code.empty() && scopeLeaderRecord != raw_code)
        {
            ss << std::string(15, ' ') << "| " << utils::StringManager::wrapText(error_pos_str, 80, 9, "", "~ ")
               << "\n";
            ss << std::string(15, ' ')
               << (trace_info.empty() ? "->" : "v ") << utils::StringManager::wrapText(raw_code, 80, 7, "", "| ~ ")
               << "\n";
        }
        scopeLeaderRecord = scope_leader_code;
        return ss.str();
    }

    std::list<std::string> RCCError::getTraceInfo() const
    {
        return trace_info;
    }


    void RCCError::addTraceInfo(const std::string &traceInfo) {
        this->trace_info.push_front(traceInfo);
    }

    std::string RCCError::briefString() const {
        return getErrorTitle() + getErrorPosition(4) + getErrorInfo(4) + getErrorTip(4);
    }

    RCCSyntaxError::RCCSyntaxError(std::string error_position, std::string error_line, StringVector error_info,StringVector repair_tips)
    : RCCError(ErrorType::SYNTAX_ERROR, std::move(error_position), std::move(error_line), std::move(error_info), std::move(repair_tips)) {}

    RCCSyntaxError
    RCCSyntaxError::illegalEscapeCharError(const std::string &error_position, const std::string &error_line,
                                           const char &c) {
        return RCCSyntaxError(error_position, error_line,
                              {"This error is caused by the use of illegal escape character.",
                               "Illegal character: " + std::to_string(c)},
                              {"Legal escape characters include the following characters: "
                               R"(["\n", "\t", "\'", "\"", "\\"])"});
    }

    RCCSyntaxError
    RCCSyntaxError::illegalSymbolError(const std::string &error_position, const std::string &error_line,
                                       const std::string& c) {
        return RCCSyntaxError(error_position, error_line,
                              {"This error is caused by the use of illegal symbol.",
                               "Illegal symbol: '" + c + "'"},
                              {"Please check the legal symbol set in Rio by accessing the official "
                               "documentation at the https://github.com/RestRegular/Rio."});
    }

    RCCSyntaxError
    RCCSyntaxError::undefinedExpressionError(const std::string &error_position, const std::string &error_line) {
        return RCCSyntaxError(error_position, error_line,
                              {"This error is caused by the use of an undefined "
                               "expression, which makes the compiler unable to correctly "
                               "recognize and parse."},
                              {"Please check the legal expression set in Rio by accessing the official "
                               "documentation at the https://github.com/RestRegular/Rio."});
    }

    RCCSyntaxError RCCSyntaxError::unclosedQuoteError(const std::string &error_position, const std::string &unclosed_quote_sign_pos,
                                                      const std::string &error_line, const char &quote_type) {
        return RCCSyntaxError(error_position, error_line,
                              {"This error is caused by the use of unclosed quotation marks.",
                               "Unclosed quote sign: '" + std::string(1, quote_type) + "' at " + unclosed_quote_sign_pos},
                              {"Please check if there is a missing corresponding quotation mark somewhere "
                               "in the code."});
    }

    RCCSyntaxError
    RCCSyntaxError::invalidIdentifierError(const std::string &error_position, const std::string &error_line,
                                           const std::string &error_identifier) {
        return RCCSyntaxError(error_position, error_line,
                              {"This error is caused by the use of a malformed identifier.",
                               "Error identifier: " + error_identifier},
                              {"Identifiers must start with a letter or underscore (_)."});
    }

    RCCSyntaxError
    RCCSyntaxError::illegalOperatorError(const std::string &error_position, const std::string &error_line,
                                         const std::string &illegal_operator, const std::string &expected_operator) {
        return RCCSyntaxError(error_position, error_line,
                              {"This error is caused by the use of the illegal operator.",
                               "Illegal operator: '" + illegal_operator + "'",
                               "Expected operator: '" + expected_operator + "'"},
                              {"Please check whether the operator at the position of this error is legal "
                               "by accessing the official documentation at the https://github.com/RestRegular/Rio."});
    }

    RCCSyntaxError RCCSyntaxError::duplicateTypeLabelError(const std::string &error_position,
        const std::string &error_line, const std::string &type_label) {
        return RCCSyntaxError(error_position, error_line,
                              {"This error is caused by using a duplicat type label.",
                              "Duplicate type label: " + type_label},
                              {"Please delete the redundant type labels from the duplicate type labels."});
    }

    RCCParserError::RCCParserError(std::string error_position, std::string error_line,
                                   StringVector error_info)
                                   : RCCError(ErrorType::PARSER_ERROR,
                                              std::move(error_position), std::move(error_line),
                                              std::move(error_info), {}){}

    RCCParserError
    RCCParserError::unexpectedTokenTypeError(const std::string &error_position, const std::string &error_line,
                                             const std::string &unexpected_token, const std::string &expected_token_type) {
        return RCCParserError(error_position, error_line,
                              {"This error occurred because an unexpected token appeared, "
                               "causing the analyzer to fail to recognize it correctly.",
                               "Unexpected token: " + unexpected_token,
                               "Expected token type: " + expected_token_type});
    }

    RCCParserError
    RCCParserError::expressionBuilderNotFoundError(const std::string &error_position, const std::string &fixName,
                                                   const std::string &errorType) {
        return RCCParserError(error_position, RCC_UNKNOWN_CONST,
                              {"This error is caused by the failure to find the " + fixName + " expression construction "
                               "function for the corresponding token type.",
                               "Error token type: " + errorType});
    }

    RCCParserError
    RCCParserError::unclosedExpressionError(const std::string &error_position, const std::string &expressionBeginToken,
                                            const std::string &errorToken, const std::string &expectedTokenType) {
        return RCCParserError(error_position, RCC_UNKNOWN_CONST,
                              {"This error is caused by an unclosed expression.",
                               "Expression begin token: " + expressionBeginToken,
                               "Expression end token: " + errorToken,
                               "Expected end token type: " + expectedTokenType});
    }

    RCCParserError RCCParserError::syntaxError(const std::string &error_position, const std::string &syntaxErrorMsg, const std::string &errorDetailMsg) {
        return RCCParserError(error_position, RCC_UNKNOWN_CONST,
                              {"This error is caused by a syntax error.",
                              "Syntax error: " + syntaxErrorMsg,
                              "Details: " + errorDetailMsg});
    }

    RCCCompilerError::RCCCompilerError(std::string error_position, std::string error_line, StringVector error_info,
        StringVector repair_tips)
            : RCCError(ErrorType::COMPILER_ERROR, std::move(error_position), std::move(error_line),
                std::move(error_info), std::move(repair_tips)) {}

    RCCCompilerError RCCCompilerError::typeMissmatchError(const std::string& error_position,
                                                          const std::string& error_line, const std::string& error_info, const std::string& expected_type,
                                                          const std::string& actual_type, const std::vector<std::string>& repair_tips)
    {
        return RCCCompilerError(error_position, error_line, {
            "This error is caused by a type missmatch.",
            error_info,
            "Expected type: " + expected_type,
            "Actual type: " + actual_type
        }, [repair_tips]
        {
            StringVector tips {"Please check if the type restrictions meet expectations."};
            if (!repair_tips.empty()) tips.insert(tips.end(), repair_tips.begin(), repair_tips.end());
            return tips;
        }());
    }

    RCCCompilerError RCCCompilerError::typeMissmatchError(const std::string& error_position,
        const std::string& error_line, const StringVector& error_infos, const std::string& expected_type,
        const std::string& actual_type, const std::vector<std::string>& repair_tips)
    {
        return RCCCompilerError(error_position, error_line, [error_infos, expected_type, actual_type]
        {
            StringVector result = {"This error is caused by a type missmatch."};
            result.insert(result.end(), error_infos.begin(), error_infos.end());
            result.push_back("Expected type: " + expected_type);
            result.push_back("Actual type: " + actual_type);
            return result;
        }(), [repair_tips]
        {
            StringVector tips {"Please check if the type restrictions meet expectations."};
            if (!repair_tips.empty()) tips.insert(tips.end(), repair_tips.begin(), repair_tips.end());
            return tips;
        }());
    }

    RCCCompilerError RCCCompilerError::compilerError(const std::string& error_position, const std::string& error_line,
                                                     const std::string& rcc_error_code, const std::string& error_info)
    {
        return RCCCompilerError(error_position, error_line, {
            "This error occurs inside the compiler.",
            error_info,
            "Error code in compiler: " + rcc_error_code
        }, {
            "If you encounter this error, you are welcome to report it to the Rio team! Please contact us on https://github.com/RestRegular/Rio."
        });
    }

    RCCCompilerError RCCCompilerError::symbolNotFoundError(const std::string& error_position,
        const std::string& error_line, const std::string& symbol_name, const std::string& error_info,
        const StringVector& repair_tips)
    {
        return RCCCompilerError(error_position, error_line, {
            "This error is caused by a symbol not found.",
            "Non-existent symbol: " + symbol_name,
            error_info
        }, [repair_tips]
        {
            StringVector tips {"Please check if the symbol exists."};
            if (!repair_tips.empty()) tips.insert(tips.end(), repair_tips.begin(), repair_tips.end());
            return tips;
        }());
    }

    RCCCompilerError RCCCompilerError::symbolNotFoundError(const std::string& error_position,
        const std::string& error_line, const std::string& symbol_name, const StringVector& error_infos,
        const StringVector& repair_tips)
    {
        return RCCCompilerError(error_position, error_line, [error_infos, symbol_name]
        {
            StringVector result = {
                "This error is caused by a symbol not found.",
                "Non-existent symbol: " + symbol_name
            };
            if (!error_infos.empty()) result.insert(result.end(), error_infos.begin(), error_infos.end());
            return result;
        }(), [repair_tips]
        {
            StringVector tips {"Please check if the symbol exists."};
            if (!repair_tips.empty()) tips.insert(tips.begin(), repair_tips.begin(), repair_tips.end());
            return tips;
        }());
    }

    RCCCompilerError RCCCompilerError::scopeError(const std::string& error_position, const std::string& error_line,
                                                  const std::string& expected_scope_field, const std::string& actual_scope_field, const StringVector& error_infos,
                                                  const StringVector& repair_tips)
    {
        return RCCCompilerError(error_position, error_line, [error_infos, expected_scope_field, actual_scope_field]{
            StringVector infos = {"This error is caused by a scope error."};
            infos.insert(infos.end(), error_infos.begin(), error_infos.end());
            infos.push_back("Expected scope field: " + expected_scope_field);
            infos.push_back("Actual scope field: " + actual_scope_field);
            return infos;
        }(), [repair_tips]
        {
            StringVector tips {"Please check if the scope field meets expectations."};
            if (!repair_tips.empty()) tips.insert(tips.end(), repair_tips.begin(), repair_tips.end());
            return tips;
        }());
    }

    RCCCompilerError RCCCompilerError::argumentError(const std::string& error_position, const std::string& error_line,
        const StringVector& error_infos, const StringVector& repair_tips)
    {
        return RCCCompilerError(error_position, error_line, [error_infos]
        {
            StringVector result = {"This error is caused by an argument error."};
            if (!error_infos.empty()) result.insert(result.end(), error_infos.begin(), error_infos.end());
            return result;
        }(), [repair_tips]
        {
            StringVector tips {"Please check if the arguments meet expectations."};
            if (!repair_tips.empty()) tips.insert(tips.end(), repair_tips.begin(), repair_tips.end());
            return tips;
        }());
    }

    RCCCompilerError RCCCompilerError::semanticError(const std::string& error_position, const std::string& error_line,
        const std::string& error_info, const StringVector& repair_tips)
    {
        return RCCCompilerError(error_position, error_line, {
            "This error is caused by a semantic error.",
            error_info
        }, [repair_tips]
        {
            StringVector tips {"Please check if the semantic of code meets expectations."};
            if (!repair_tips.empty()) tips.insert(tips.end(), repair_tips.begin(), repair_tips.end());
            return tips;
        }());
    }

    RCCCompilerError RCCCompilerError::semanticError(const std::string& error_position, const std::string& error_line,
        const StringVector& error_infos, const StringVector& repair_tips)
    {
        return RCCCompilerError(error_position, error_line, [error_infos]
        {
            StringVector result{"This error is caused by a semantic error."};
            if (!error_infos.empty()) result.insert(result.end(), error_infos.begin(), error_infos.end());
            return result;
        }(), [repair_tips]
        {
            StringVector result{"Please check if the semantic of code meets expectations."};
            if (!repair_tips.empty()) result.insert(result.end(), repair_tips.begin(), repair_tips.end());
            return result;
        }());
    }
}

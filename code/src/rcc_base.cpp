//
// Created by RestRegular on 2025/6/28.
//

#include "../include/rcc_base.h"

namespace base {

    std::stack<std::string> PROGRAM_WORKING_DIRECTORY_STACK{};

    const StringSet KEYWORDS = {
            "var", "fun", "if", "else", "elif", "for", "while", "until",
            "repeat", "ret", "break", "class", "iter", "this", "super",
            "pass", "ctor", "encapsulated", "try", "catch", "finally", "throw",
            "rasm", "import", "link"
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
            "public", "private", "static", "protected", "internal",
            "const", "global", "quote", "override", "interface",
            "virtual", "int", "float", "str", "bool", "char", "any",
            "list", "dict", "series", "func", "void"
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

    bool containsKeyword(const std::string &str) {
        return KEYWORDS.contains(str);
    }

    std::string getErrorTypeName(const ErrorType &error_type) {
        switch (error_type) {
            case ErrorType::SYNTAX_ERROR:
                return "SyntaxError";
            default:
                return "UndefinedError";
        }
    }

    RCCError::RCCError(ErrorType error_type, std::string error_position, std::string error_line,
                       std::vector<std::string> error_info, std::vector<std::string> repair_tips)
                       : error_type(error_type), error_type_name(getErrorTypeName(error_type)), error_position(error_position),
                         error_line(error_line), error_info(std::move(error_info)), repair_tips(std::move(repair_tips)){}

    RCCError::RCCError(std::string error_type, std::string error_position, std::string error_line,
                       std::vector<std::string> error_info, std::vector<std::string> repair_tips)
                       : error_type_name(error_type), error_position(error_position),
                         error_line(error_line), error_info(std::move(error_info)), repair_tips(std::move(repair_tips)){}

    std::string RCCError::toString() const {
        const auto &space_size = space.size();
        std::ostringstream oss;

        oss << getErrorTitle();

        if (!trace_info.empty()) {
            oss << " [ Trace Back ]\n" << trace_info << "\n";
        }

        if (!error_position.empty() && error_position != RCC_UNKNOWN_CONST) {
            oss << space << "[ Line ] : "
                << utils::StringManager::wrapText(error_position, 80, space_size + 10, "", "~ ") << "\n"
                << std::string(space_size + 9, ' ') << "| "
                << utils::StringManager::wrapText(error_line, 80, space_size + 9, "", "| ~ ") << "\n\n";
        }

        if (!error_info.empty()) {
            oss << getErrorInfo();
        }

        if (!repair_tips.empty()) {
            oss << space << "[ Tips ] : ";
            for (size_t i = 0; i < repair_tips.size(); ++i) {
                oss << utils::StringManager::wrapText(repair_tips[i], 80, space_size + 10, "", "~ ");
                if (i < repair_tips.size() - 1) {
                    oss << "\n" << std::string(space_size + 9, ' ') << "- ";
                }
            }
            oss << "\n";
        }

        return oss.str();
    }

    std::string RCCError::getErrorInfo() const {
        std::ostringstream oss;
        oss << space << "[ Info ] : ";
        for (size_t i = 0; i < error_info.size(); ++i) {
            oss << utils::StringManager::wrapText(error_info[i], 80, space.size() + 10, "", "~ ");
            if (i < error_info.size() - 1) {
                oss << "\n" << std::string(space.size() + 9, ' ') << "- ";
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

    void RCCError::addTraceInfo(const std::string &traceInfo) {
        this->trace_info += traceInfo;
    }

    RCCSyntaxError::RCCSyntaxError(std::string error_position, std::string error_line, StringVector error_info,StringVector repair_tips)
    : RCCError(ErrorType::SYNTAX_ERROR, error_position, error_line, error_info, repair_tips) {}

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
    RCCSyntaxError::illegalCharacterError(const std::string &error_position, const std::string &error_line,
                                          const char &c) {
        return RCCSyntaxError(error_position, error_line,
                              {"This error is caused by the use of illegal character.",
                               "Illegal character: " + std::string(1, c)},
                              {"Please check the legal character set in Rio by accessing the official "
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

    RCCSyntaxError RCCSyntaxError::unclosedQuoteError(const std::string &error_position, const std::string &error_line,
                                                      const char &quote_type) {
        return RCCSyntaxError(error_position, error_line,
                              {"This error is caused by the use of unclosed quotation marks.",
                               "Unclosed quote sign: " + std::string(1, quote_type)},
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

    RCCParserError::RCCParserError(std::string error_position, std::string error_line,
                                   StringVector error_info, StringVector repair_tips)
                                   : RCCError(ErrorType::PARSER_ERROR,
                                              std::move(error_position), std::move(error_line),
                                              std::move(error_info), std::move(repair_tips)){}

    RCCParserError
    RCCParserError::unexpectedTokenError(const std::string &error_position, const std::string &error_line,
                                         const std::string &unexpected_token) {
        return RCCParserError(error_position, error_line,
                              {"This error occurred because an unexpected token appeared, "
                               "causing the parser to fail to recognize it correctly.",
                               "Unexpected token: " + unexpected_token},
                              {"Please check the code for errors, or contact the developer for help."});
    }
}
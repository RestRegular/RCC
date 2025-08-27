//
// Created by RestRegular on 2025/6/28.
//

#ifndef RCC_RCC_BASE_H
#define RCC_RCC_BASE_H

#include <iostream>
#include <stack>
#include <unordered_set>
#include <unordered_map>
#include <vector>
#include <memory>
#include "lib/rcc_utils.h"

#define RIO_PROGRAM_SIGN "Rio Program & compiled by RCC(Rio Compiler Collection)"

#define RCC_VERSION_MAJOR "1"
#define RCC_VERSION_MINOR "0"
#define RCC_VERSION_PATCH "0"

#define RCC_VERSION "v" RCC_VERSION_MAJOR "." RCC_VERSION_MINOR "." RCC_VERSION_PATCH

#define RCC_UNDEFINED_CONST "RCC-UNDEFINED"
#define RCC_UNKNOWN_CONST "RCC-UNKNOWN"
#define RCC_NULL_CONST "RCC-NULL"
#define RCC_TOKEN_STREAM_START "$RCC_TOKEN_STREAM_START$"
#define RCC_TOKEN_STREAM_END "$RCC_TOKEN_STREAM_END$"

#define RCC_TRUE        "true"
#define RCC_FALSE       "false"
#define RCC_NULL        "null"
#define RCC_REL_RE      "RE"
#define RCC_REL_RNE     "RNE"
#define RCC_REL_RGE     "RGE"
#define RCC_REL_RG      "RG"
#define RCC_REL_RLE     "RLE"
#define RCC_REL_RL      "RL"
#define RCC_REL_AND     "AND"
#define RCC_REL_OR      "OR"

namespace base {
    // 自定义类型
    typedef std::stack<std::string> StringStack;
    typedef std::vector<std::string> StringVector;
    typedef std::unordered_set<std::string> StringSet;
    typedef std::unordered_map<std::string, std::string> StringMap;

    // 枚举类声明
    enum class ErrorType;

    // 类声明
    class RCCError;
    class RCCSyntaxError;

    // 函数声明
    bool containsKeyword(const std::string &str);
    std::string getErrorTypeName(const ErrorType &error_type);

    // 全局变量声明
    extern StringStack PROGRAM_WORKING_DIRECTORY_STACK; // Program working directory
    extern const StringSet KEYWORDS;
    extern const StringSet OPERATORS;
    extern const StringSet ARITHMETIC_OPERATORS;
    extern const StringSet ASSIGN_OPERATORS;
    extern const StringSet ENHANCED_ASSIGN_OPERATORS;
    extern const StringMap ENH_MAT_ORD_OPTORS;
    extern const StringSet COMPARISON_OPERATOR;
    extern const StringSet RELATIONAL_OPERATOR;
    extern const StringSet OTHER_OPERATOR;
    extern const StringSet COMMENT_OPERATOR;
    extern const std::unordered_map<std::string, std::shared_ptr<StringSet>> CLASSIFY_OPERATORS;
    extern const std::vector<std::shared_ptr<StringSet>> OPERATOR_LIST;
    extern const StringSet DELIMITERS;
    extern const StringSet RANGERS;
    extern const StringMap RANGER_MATCH;
    extern const StringSet CONSTANTS;
    extern const StringSet TYPES;
    extern const StringSet DESCRIBE_LABELS;
    extern const std::string NULL_VALUE;
    extern const std::unordered_map<std::string, int> OPERATOR_PRECEDENCE;
    extern const StringVector OPERATOR_PRECEDENCE_LIST;
    extern const StringSet GROUP_SIGNS;
    extern const StringMap RELATION_MAP;

    // 类定义
    class RCCError: public std::exception, public utils::Object {
        ErrorType error_type;
        std::string error_type_name;
        std::string error_position;
        std::string error_line;
        StringVector error_info;
        StringVector repair_tips;
        std::list<std::string> trace_info;
        std::string space = std::string(4, ' ');
    public:
        std::string getErrorTip(const std::size_t & spaceSize) const;

        [[nodiscard]] std::string toString() const override;

        [[nodiscard]] std::string getErrorTitle() const;

        [[nodiscard]] std::string getErrorInfo(const size_t &spaceSize) const;

        [[nodiscard]] std::string getErrorLine(const size_t &spaceSize) const;

        [[nodiscard]] std::string getErrorPosition(const size_t &spaceSize) const;

        void addTraceInfo(const std::string &traceInfo);

        [[nodiscard]] std::string briefString() const override;

        static std::string makeTraceInfo(
                const std::string &file_record_, const std::string &error_pos_filepath,
                const std::string &utils_getPosStrFromFilePath, const std::string &makeFileIdentiFromPath,
                const std::string &trace_info, const std::string &error_pos_str,
                const std::string &raw_code, const std::string &scope_leader_pos,
                const std::string &scope_leader_code);

        [[nodiscard]] std::list<std::string> getTraceInfo() const;

    protected:
        RCCError(ErrorType error_type, std::string error_position, std::string error_line,
                 StringVector error_info, StringVector repair_tips);

        [[maybe_unused]] RCCError(std::string error_type, std::string error_position, std::string error_line,
                 StringVector error_info, StringVector repair_tips);

    };

    class RCCSyntaxError final : public RCCError {
        RCCSyntaxError(std::string error_position, std::string error_line,
                       StringVector error_info, StringVector repair_tips);
    public:
        static RCCSyntaxError illegalEscapeCharError(const std::string &error_position, const std::string &error_line, const char &c);
        static RCCSyntaxError illegalSymbolError(const std::string &error_position, const std::string &error_line, const std::string& c);
        static RCCSyntaxError undefinedExpressionError(const std::string &error_position, const std::string &error_line);
        static RCCSyntaxError unclosedQuoteError(const std::string &error_position, const std::string &unclosed_quote_sign_pos, const std::string &error_line, const char &quote_type);
        static RCCSyntaxError invalidIdentifierError(const std::string &error_position, const std::string &error_line, const std::string &error_identifier);
        static RCCSyntaxError illegalOperatorError(const std::string &error_position, const std::string &error_line, const std::string &illegal_operator, const std::string &expected_operator);
        static RCCSyntaxError duplicateTypeLabelError(const std::string &error_position, const std::string &error_line, const std::string &type_label);
    };

    class RCCParserError final : public RCCError {
        RCCParserError(std::string error_position, std::string error_line,
                       StringVector error_info);
    public:
        static RCCParserError unexpectedTokenTypeError(const std::string &error_position, const std::string &error_line,
                                                       const std::string &unexpected_token,
                                                       const std::string &expected_token_type);
        static RCCParserError expressionBuilderNotFoundError(const std::string &error_position, const std::string &fixName,
                                                             const std::string &errorType);
        static RCCParserError unclosedExpressionError(const std::string &error_position, const std::string &expressionBeginToken,
                                                      const std::string &errorToken, const std::string &expectedTokenType);
        static RCCParserError syntaxError(const std::string &error_position, const std::string &syntaxErrorMsg, const std::string &errorDetailMsg);
    };

    class RCCCompilerError final : public RCCError
    {
        RCCCompilerError(std::string error_position, std::string error_line,
            StringVector error_info, StringVector repair_tips);
    public:
        static RCCCompilerError typeMissmatchError(const std::string &error_position, const std::string &error_line,
                                                   const std::string& error_info, const std::string &expected_type,
                                                   const std::string &actual_type, const std::vector<std::string> &repair_tips);
        static RCCCompilerError typeMissmatchError(const std::string &error_position, const std::string &error_line,
                                                   const StringVector& error_infos, const std::string &expected_type,
                                                   const std::string &actual_type, const std::vector<std::string> &repair_tips);
        static RCCCompilerError compilerError(const std::string &error_position, const std::string &error_line,
                                              const std::string &rcc_error_code, const std::string& error_info);
        static RCCCompilerError symbolNotFoundError(const std::string &error_position, const std::string &error_line,
            const std::string &symbol_name, const std::string &error_info, const StringVector &repair_tips);
        static RCCCompilerError symbolNotFoundError(const std::string &error_position, const std::string &error_line,
            const std::string &symbol_name, const StringVector &error_infos, const StringVector &repair_tips);
        static RCCCompilerError scopeError(const std::string &error_position, const std::string &error_line,
                                           const std::string &expected_scope_field, const std::string &actual_scope_field, const StringVector& error_infos,
                                           const StringVector &repair_tips);
        static RCCCompilerError argumentError(const std::string &error_position, const std::string &error_line,
            const StringVector &error_infos, const StringVector &repair_tips);
        static RCCCompilerError semanticError(const std::string &error_position, const std::string &error_line,
            const std::string &error_info, const StringVector &repair_tips);
        static RCCCompilerError semanticError(const std::string &error_position, const std::string &error_line,
            const StringVector &error_infos, const StringVector &repair_tips);
    };

    // 枚举类定义
    enum class ErrorType {
        SYNTAX_ERROR,
        PARSER_ERROR,
        COMPILER_ERROR,
    };
}


#endif //RCC_RCC_BASE_H

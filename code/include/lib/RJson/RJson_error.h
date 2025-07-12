//
// Created by RestRegular on 2025/7/1.
//

#ifndef RJSON_ERROR_H
#define RJSON_ERROR_H

#include <iostream>
#include <vector>
#include "../rcc_utils.h"

namespace error {
#define RJ_ERROR_UNKNOWN "unknown"

    using namespace utils;

    typedef std::vector<std::string> StringVector;

    enum class ErrorType {
        UNDEFINED,
        SYNTAX_ERROR,
        PARSER_ERROR,
    };

    std::string getErrorTypeName(const ErrorType& error_type);

    class RJsonError : std::exception, public Object {
        ErrorType error_type = ErrorType::UNDEFINED;
        std::string error_type_name;
        std::string error_position;
        std::string error_line;
        StringVector error_info;
        StringVector repair_tips;
        std::string trace_info;
        std::string space = std::string(4, ' ');

    public:
        [[nodiscard]] std::string toString() const override;

        [[nodiscard]] std::string getErrorTitle() const;

        [[nodiscard]] std::string getErrorInfo(const size_t &spaceSize) const;

        [[nodiscard]] std::string getErrorLine(const size_t &spaceSize) const;

        [[nodiscard]] std::string getErrorPosition(const size_t &spaceSize) const;

        void addTraceInfo(const std::string &traceInfo);

        [[nodiscard]] std::string briefString() const override;

        RJsonError(ErrorType error_type, std::string error_position, std::string error_line,
                 StringVector error_info, StringVector repair_tips);

        [[maybe_unused]] RJsonError(std::string error_type, std::string error_position, std::string error_line,
                                  StringVector error_info, StringVector repair_tips);
    };
}


#endif //RJSON_ERROR_H

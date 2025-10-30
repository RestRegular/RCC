//
// Created by RestRegular on 2025/10/22.
//

#ifndef RCC_RCC_BASE_DEC_H
#define RCC_RCC_BASE_DEC_H

#include <string>
#include "../include/rcc_version.h"

namespace base
{
#define RIO_PROGRAM_SIGN "Rio Program & compiled by RCC(Rio Compiler Collection)"

#define STRINGIFY(x) #x
#define VERSION_STR(major, minor, patch) "v" STRINGIFY(major) "." STRINGIFY(minor) "." STRINGIFY(patch)
#define RCC_VERSION VERSION_STR(VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH)

#define RCC_UNDEFINED_CONST "RCC-UNDEFINED"
#define RCC_UNKNOWN_CONST "RCC-UNKNOWN"
#define RCC_NULL_CONST "RCC-NULL"
#define RCC_TOKEN_STREAM_START "$RCC_TOKEN_STREAM_START$"
#define RCC_TOKEN_STREAM_END "$RCC_TOKEN_STREAM_END$"
#define ANONYMOUS_FUNCTION_PREFIX "anon_func_"

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

    // 枚举类声明
    enum class ErrorType;

    // 类声明
    class RCCError;
    class RCCSyntaxError;

    // 函数声明
    bool containsKeyword(const std::string& str);
    std::string getErrorTypeName(const ErrorType& error_type);
}

#endif //RCC_RCC_BASE_DEC_H
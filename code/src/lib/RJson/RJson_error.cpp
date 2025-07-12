//
// Created by RestRegular on 2025/7/1.
//

#include <sstream>
#include "../../../include/lib/RJson/RJson_error.h"

namespace error {
    std::string getErrorTypeName(const ErrorType &error_type) {
        switch (error_type) {
            case ErrorType::SYNTAX_ERROR: return "SyntaxError";
            case ErrorType::PARSER_ERROR: return "ParserError";
            case ErrorType::UNDEFINED:
            default: return "Undefined";
        }
    }

    RJsonError::RJsonError(ErrorType error_type, std::string error_position, std::string error_line,
                           std::vector<std::string> error_info, std::vector<std::string> repair_tips)
        : error_type(error_type), error_type_name(getErrorTypeName(error_type)), error_position(std::move(error_position)),
          error_line(std::move("\"" + StringManager::escape(error_line)) + "\""), error_info(std::move(error_info)), repair_tips(std::move(repair_tips)) {
    }

    [[maybe_unused]] RJsonError::RJsonError(std::string error_type, std::string error_position, std::string error_line,
                                        std::vector<std::string> error_info, std::vector<std::string> repair_tips)
        : error_type(), error_type_name(std::move(error_type)), error_position(std::move(error_position)),
          error_line(std::move("\"" + StringManager::escape(error_line) + "\"")), error_info(std::move(error_info)), repair_tips(std::move(repair_tips)) {
    }

    std::string RJsonError::getErrorLine(const size_t &spaceSize) const {
        std::ostringstream  oss;
        if (!error_position.empty() && error_position != RJ_ERROR_UNKNOWN) {
            oss << std::string(spaceSize, ' ') << "[ Line ] : "
                << utils::StringManager::wrapText(error_position, 80, spaceSize + 10, "", "~ ") << "\n"
                << std::string(spaceSize + 9, ' ') << "| "
                << utils::StringManager::wrapText(error_line, 80, spaceSize + 9, "", "| ~ ") << "\n\n";
        } else if (!error_line.empty() && error_line != "\"\"") {
            oss << std::string(spaceSize, ' ') << "[ Line ] | "
                << utils::StringManager::wrapText(error_line, 80, spaceSize + 9, "", "| ~ ") << "\n\n";
        }
        return oss.str();
    }

std::string RJsonError::getErrorPosition(const size_t &spaceSize) const {
        if (!error_position.empty()) {
            return std::string(spaceSize, ' ') + "[ Pos ]  : " +
            utils::StringManager::wrapText(error_position, 80, spaceSize + 10, "", "~ ") + "\n";
        }
        return {};
    }

    std::string RJsonError::toString() const {
        const auto &space_size = space.size();
        std::ostringstream oss;

        oss << getErrorTitle();

        if (!trace_info.empty()) {
            oss << " [ Trace Back ]\n" << trace_info << "\n";
        }

        if (!error_line.empty()) {
            oss << getErrorLine(space_size);
        }

        if (!error_info.empty()) {
            oss << getErrorInfo(space_size);
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

    std::string RJsonError::getErrorInfo(const size_t &spaceSize) const {
        std::ostringstream oss;
        oss << std::string(spaceSize, ' ') << "[ Info ] : ";
        for (size_t i = 0; i < error_info.size(); ++i) {
            oss << utils::StringManager::wrapText(error_info[i], 80, spaceSize + 10, "", "~ ");
            if (i < error_info.size() - 1) {
                oss << "\n" << std::string(spaceSize + 9, ' ') << "- ";
            }
        }
        oss << "\n\n";
        return oss.str();
    }

    std::string RJsonError::getErrorTitle() const {
        std::ostringstream oss;
        oss << "\n" << std::string(20, '=');
        oss << "[ " << error_type_name << " ]" << std::string(60, '=') << "\n";
        return oss.str();
    }

    void RJsonError::addTraceInfo(const std::string &traceInfo) {
        this->trace_info += traceInfo;
    }

    std::string RJsonError::briefString() const {
        return getErrorTitle() + getErrorPosition(4) + getErrorInfo(4);
    }
}

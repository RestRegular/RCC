//
// Created by RestRegular on 2025/1/17.
//

#include <iostream>
#include <limits>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <random>
#include <utility>
#include <set>
#include <windows.h>
#include <chrono>
#include <functional>
#include "../../include/lib/rcc_utils.h"
#include "../../include/rcc_base.h"

namespace utils {

    size_t Object::_id = 0;

    Object::Object(): id(_id++) {}

    std::string Object::toString() const {
        return "[Object: " + std::to_string(id) + "]";
    }

    std::string Object::briefString() const {
        return toString();
    }

    std::string Object::professionalString() const {
        return "Object{id=" + std::to_string(id) + "}";
    }

    std::string Object::formatString(size_t indent, size_t level) const {
        return {};
    }

    std::string Object::toJsonString() const {
        return "\"" + StringManager::escape(toString()) + "\"";
    }

    const auto &program_start_time = std::chrono::high_resolution_clock::now();

    // ���л� ArgType ���������ļ�
    void serializeArgType(std::ostream &os, const ArgType &argType) {
        // �� ArgType ת��Ϊ�ײ���������
        const int value = static_cast<int>(argType);
        os.write(reinterpret_cast<const char*>(&value), sizeof(value));
    }

    // �����л� ArgType �Ӷ������ļ�
    ArgType deserializeArgType(std::istream &is) {
        int value;
        is.read(reinterpret_cast<char*>(&value), sizeof(value));

        // ������ת���� ArgType ö������
        return static_cast<ArgType>(value);
    }

    Number::Number(const int int_value) :
            type(NumType::int_type), int_value(int_value), double_value(0) {}

    Number::Number(const double double_value) :
            type(NumType::double_type), int_value(0), double_value(double_value) {}

    std::string Number::toString() const {
        if (type == NumType::int_type) {
            return std::to_string(int_value);
        } else if (type == NumType::double_type) {
            return std::to_string(double_value);
        }
        return "";
    }

    std::ostream &operator<<(std::ostream &os, const Number &num) {
        if (num.type == NumType::int_type) {
            os << num.int_value;
        } else if (num.type == NumType::double_type) {
            os << num.double_value;
        }
        return os;
    }

    Number::Number(): type(NumType::NotNumber), int_value(0), double_value(0) {}

    // ��ȡ�ļ���
    std::string getFileNameFromPath(const std::string &path) {
        auto file_full_name = std::filesystem::path(path).filename().string();
        return file_full_name.substr(0, file_full_name.rfind('.'));
    }

    // ��ȡ�ļ���չ��
    std::string getFileExtFromPath(const std::string &path) {
        std::filesystem::path file_path(path);
        if (file_path.has_extension()) {
            return file_path.extension().string().substr(1); // ȥ�����
        }
        return ""; // ���û����չ�������ؿ��ַ���
    }

    // ��ȡ�ļ�������չ��
    std::pair<std::string, std::string> getFileInfoFromPath(const std::string &path) {
        auto file_full_name = std::filesystem::path(path).filename().string();
        auto file_name = file_full_name.substr(0, file_full_name.rfind('.'));
        auto file_ext = file_full_name.substr(file_full_name.rfind('.') + 1);
        return {file_name, file_ext};
    }

    std::string getFileFromPath(const std::string &path){
        return std::filesystem::path(path).filename().string();
    }

    std::string getObjectFormatString(const std::string &type, const std::string &name){
        return "[" + type + ": " + name + "]";
    }

    std::string getWindowsRVMDir() {
        char path[MAX_PATH];
        GetModuleFileNameA(nullptr, path, MAX_PATH);
        return std::filesystem::path(path).parent_path().string();
    }

    std::string getWindowsDefaultDir() {
        std::filesystem::path current_path = std::filesystem::current_path();
        return current_path.string();
    }

    // �ַ���ת����
    int stringToInt(const std::string &str) {
        if (str.empty()) {
            throw std::invalid_argument("Invalid argument: empty string");
        }
        size_t i = 0;
        bool isNegative = false;
        // Handle optional sign
        if (str[i] == '+' || str[i] == '-') {
            isNegative = (str[i] == '-');
            ++i;
        }
        // Check if the remaining characters are digits
        if (i == str.size()) {
            throw std::invalid_argument("Invalid argument: " + str);
        }
        int result = 0;
        for (; i < str.size(); ++i) {
            if (str[i] < '0' || str[i] > '9') {
                throw std::invalid_argument("Invalid argument: " + str);
            }
            // Check for overflow
            if (result > (std::numeric_limits<int>::max() - (str[i] - '0')) / 10) {
                throw std::overflow_error("Overflow error: " + str);
            }
            result = result * 10 + (str[i] - '0');
        }
        return isNegative ? -result : result;
    }

    // �ַ���ת������
    double stringToDouble(const std::string &str) {
        if (str.empty()) {
            throw std::invalid_argument("Invalid argument: empty string");
        }

        size_t i = 0;
        bool isNegative = false;

        // Handle optional sign
        if (str[i] == '+' || str[i] == '-') {
            isNegative = (str[i] == '-');
            ++i;
        }

        // Check if the remaining characters are valid
        if (i == str.size()) {
            throw std::invalid_argument("Invalid argument: " + str);
        }

        double result = 0.0;
        double fraction = 1.0;
        bool hasFraction = false;
        bool hasExponent = false;
        int exponent = 0;
        bool isExponentNegative = false;

        // Parse integer and fractional parts
        for (; i < str.size(); ++i) {
            if (str[i] == '.') {
                if (hasFraction) {
                    throw std::invalid_argument("Invalid argument: " + str);
                }
                hasFraction = true;
            } else if (str[i] == 'e' || str[i] == 'E') {
                hasExponent = true;
                ++i;
                if (i < str.size() && (str[i] == '+' || str[i] == '-')) {
                    isExponentNegative = (str[i] == '-');
                    ++i;
                }
                // Parse exponent
                for (; i < str.size(); ++i) {
                    if (str[i] < '0' || str[i] > '9') {
                        throw std::invalid_argument("Invalid argument: " + str);
                    }
                    exponent = exponent * 10 + (str[i] - '0');
                }
                break;
            } else if (str[i] >= '0' && str[i] <= '9') {
                if (hasFraction) {
                    fraction *= 0.1;
                    result += (str[i] - '0') * fraction;
                } else {
                    result = result * 10 + (str[i] - '0');
                }
            } else {
                throw std::invalid_argument("Invalid argument: " + str);
            }
        }

        // Apply exponent
        if (hasExponent) {
            result *= std::pow(10.0, isExponentNegative ? -exponent : exponent);
        }

        return isNegative ? -result : result;
    }

    // ���ַ���ת��Ϊchar
    char stringToChar(const std::string &str) {
        if (str.size() == 1) {
            return str[0];
        }
        if (str.size() == 3 && str[0] == '\'' && str[2] == '\'') {
            return str[1];
        }
        if (str.size() == 4 && str[0] == '\'' && str[1] == '\\' && str[3] == '\'') {
            return StringManager::getInstance().escapeChar(str[2]);
        }
        throw std::invalid_argument("Invalid character string: " + str);
    }

    // ���ַ���ת��Ϊbool
    bool stringToBool(const std::string &str) {
        if (str == "true" || str == "1") {
            return true;
        } else if (str == "false" || str == "0") {
            return false;
        } else {
            throw std::invalid_argument("Invalid boolean string: " + str);
        }
    }

    // �������������Խ��ַ���ת��Ϊint�����ʧ����ת��Ϊdouble
    Number stringToNumber(const std::string &str) {
        try {
            return Number{stringToInt(str)};
        } catch (const std::invalid_argument &_) {
            try {
                return Number{stringToDouble(str)};
            } catch (const std::invalid_argument &_){
                return Number();
            } catch (const std::out_of_range &_) {
                return Number{stringToDouble(str)};
            }
        } catch (const std::out_of_range &_) {
            return Number{stringToDouble(str)};
        }
    }

    // ��doubleת��Ϊ�ַ���
    std::string doubleToString(double value) {
        std::ostringstream oss;
        oss << value;
        return oss.str();
    }

    // �ж��ַ����Ƿ�Ϊ�ַ�����ʽ
    bool isStringFormat(const std::string &str) {
        return str.front() == '"' && str.back() == '"';
    }

    // �����ַ�����ʽ
    std::string parseStringFormat(const std::string &result) {
        if (isStringFormat(result)) {
            return result.substr(1, result.size() - 2);
        } else {
            return result;
        }
    }

    // �����ַ�����ʽ�������ؽ��
    void parseStringFormat_noReturn(std::string &result) {
        if (isStringFormat(result)) {
            result = result.substr(1, result.size() - 2);
        }
    }

    bool isNumber(const std::string &str) {
        if (str.empty()) return false;
        size_t i = 0;
        // Handle sign
        if (str[i] == '+' || str[i] == '-') ++i;
        bool hasDigit = false, hasDot = false;
        while (i < str.size()) {
            if (std::isdigit(str[i])) {
                hasDigit = true;
            } else if (str[i] == '.' && !hasDot) { // Only one dot allowed
                hasDot = true;
            } else {
                return false; // Invalid character found
            }
            ++i;
        }
        return hasDigit; // Ensure at least one digit is present
    }

    std::string getPosStrFromFilePath(const std::string &file_path) {
        return getAbsolutePath(file_path) + ":1:1, line 1";
    }

    std::unordered_set<char> StringManager::escapeChars {
        '\\', '\'', '"', 't', 'n'
    };

    std::unordered_set<char> StringManager::rawEscapeChars = {
        '\\', '\'', '"', '\t', '\n'
    };

    std::unordered_set<char> StringManager::spaceChars = {
        ' ', '\t', '\n', '\r'
    };


    StringManager &StringManager::getInstance() {
        static StringManager instance;
        return instance;
    }

    StringManager::StringManager() {
        registerEscapeHandler('n', []() { return "\n"; });
        registerEscapeHandler('t', []() { return "\t"; });
        registerEscapeHandler('"', []() { return "\""; });
        registerEscapeHandler('\'', []() { return "\'"; });
        registerEscapeHandler('\\', []() { return "\\"; });
        registerEscapeHandler('r', []() { return "\r"; });
    }

    void StringManager::registerEscapeHandler(char escapeChar, std::function<std::string(void)> handler) {
        escapeHandlers[escapeChar] = std::move(handler);
    }

    std::string StringManager::handleEscapeSequence(std::string_view input, size_t &pos) {
        if (pos + 1 >= input.length()) {
            throw std::runtime_error("Invalid escape sequence at end of string");
        }
        char escapeChar = input[++pos];
        auto it = escapeHandlers.find(escapeChar);
        if (it != escapeHandlers.end()) {
            return it->second();
        }
        std::cerr << "[RCC Warning]: Unknown escape sequence '\\" << escapeChar << "'" << std::endl;
        return {1, escapeChar};
    }

    std::string StringManager::processQuotedString(std::string_view input) {
        std::string result;
        result.reserve(input.length());
        for (size_t i = 0; i < input.length(); ++i) {
            if (input[i] == '\\') {
                result += handleEscapeSequence(input, i);
            } else if (input[i] != '"') {
                result += input[i];
            }
        }
        return result;
    }

    std::vector<std::string> StringManager::split(const std::string &content, char delimiter) {
        std::vector<std::string> result;
        result.reserve(std::count(content.begin(), content.end(), delimiter) + 1);
        if (!content.empty()) {
            std::string current;
            bool inQuotes = false;
            for (size_t i = 0; i < content.length(); ++i) {
                char c = content[i];
                if (inQuotes && c == '\n') {
                    inQuotes = false; // ǿ�ƽ�������
                }
                if (c == '"') {
                    int backslashCount = 0;
                    for (int j = static_cast<int>(i) - 1; j >= 0 && content[j] == '\\'; --j) {
                        ++backslashCount;
                    }
                    if (backslashCount % 2 == 0) {
                        inQuotes = !inQuotes;
                    }
                }
                if (c == delimiter && !inQuotes) {
                    result.push_back(std::move(current));
                    current.clear();
                } else {
                    current += c;
                }
            }
            result.push_back(std::move(current));
        }
        return result;
    }

    std::string StringManager::parseStringFormat(const std::string &input) {
        auto result = input;
        parseStringFormat_nret(result);
        return result;
    }

    void StringManager::parseStringFormat_nret(std::string &result) {
        if (isStringFormat(result)) {
            result = result.substr(1, result.length() - 2);
        }
    }

    std::string StringManager::unescape(const std::string &input) {
        std::string result;
        result.reserve(input.length());
        bool inQuotes = false;
        for (size_t i = 0; i < input.length(); ++i) {
            char c = input[i];
            if (c == '"' && (i == 0 || input[i - 1] != '\\')) {
                inQuotes = !inQuotes;
                result += c;
            } else if (inQuotes && c == '\\') {
                result += handleEscapeSequence(input, i);
            } else {
                result += c;
            }
        }
        parseStringFormat_nret(result);
        return result;
    }

    void StringManager::unescape_nret(std::string &input) {
        auto result = unescape(input);
        input = std::move(result);
    }

    inline bool StringManager::isStringFormat(const std::string &str) {
        return str.size() >= 2 && str.front() == '"' && str.back() == '"';
    }

    bool StringManager::isStrictValidStringFormat(const std::string &str) {
        if (!isStringFormat(str)) return false;
        const auto &strContent = str.substr(1, str.size() - 2);
        for (size_t i = 0; i < strContent.size(); ++i) {
            const auto &c = strContent[i];
            if (rawEscapeChars.contains(c)) {
                if (c == '\\') {
                    i ++;
                    continue;
                }
                return false;
            }
        }
        return true;
    }

    bool StringManager::isCharFormat(const std::string &str) {
        return (str.front() == '\'' && str.back() == '\'') && (str.size() == 3 || (str.size() == 4 && str[1] == '\\'));
    }

    void StringManager::trim(std::string &str) {
        str.erase(0, str.find_first_not_of(" \t"));
        str.erase(str.find_last_not_of(" \t") + 1);
    }

    std::string StringManager::trim(const std::string &str) {
        auto str_ = str;
        trim(str_);
        return str_;
    }


    std::string StringManager::toStringFormat(const std::string &str) {
        return "\"" + str + "\"";
    }

    std::map<std::string, std::string>
    StringManager::splitStringByChars(const std::string &input, const std::string &delimiters) {
        std::map<std::string, std::string> result;
        std::string_view input_view(input);
        size_t start = 0;
        // �����һ�����֣�prefix��
        size_t end = input_view.find_first_of(delimiters, start);
        result.emplace("prefix", std::string(input_view.substr(start, end - start)));
        start = (end == std::string_view::npos) ? input_view.size() : end;
        // ����ʣ�ಿ��
        while (start < input_view.size()) {
            // �ҵ��ָ���
            char delimiter = input_view[start];
            std::string delimiter_key(1, delimiter); // ���ָ���ת��Ϊ�ַ���
            start++; // �����ָ���
            // �ҵ���һ���ָ������ַ���ĩβ
            end = input_view.find_first_of(delimiters, start);
            if (end == std::string_view::npos) {
                end = input_view.size();
            }
            // ��ȡֵ�����浽 map
            result.emplace(std::move(delimiter_key), std::string(input_view.substr(start, end - start)));
            start = end;
        }
        return result;
    }

    std::string StringManager::escape(const std::string &input) {
        std::string result;
        result.reserve(input.size() * 2); // Ԥ���㹻�Ŀռ䣬����Ƶ�������ڴ�
        for (char c: input) {
            switch (c) {
                case '\n':
                    result.append("\\n");
                    break;  // ���з�
                case '\t':
                    result.append("\\t");
                    break;  // �Ʊ��
                case '\r':
                    result.append("\\r");
                    break;  // �س���
                case '\"':
                    result.append("\\\"");
                    break; // ˫����
                case '\\':
                    result.append("\\\\");
                    break; // ��б��
                default:
                    result.push_back(c);
                    break;        // �����ַ�ֱ��׷��
            }
        }
        return result;
    }

    std::string StringManager::wrapText(const std::string &text, size_t lineWidth, size_t indent, const std::string& last_line_suffix, const std::string& next_line_prefix){
        std::ostringstream oss;
        size_t currentWidth = 0;
        std::istringstream words(text);
        std::string word;

        while (words >> word) {
            if (currentWidth + word.size() + (currentWidth > 0 ? 1 : 0) > lineWidth) {
                oss << last_line_suffix << "\n" << std::string(indent, ' ') << next_line_prefix;
                currentWidth = 0;
            }
            if (currentWidth > 0) {
                oss << " ";
                currentWidth++;
            }
            oss << word;
            currentWidth += word.size();
        }
        return oss.str();
    }

    std::string merge_adjacent_quotes(const std::vector<std::string>& parts) {
        std::stringstream result_stream;
        std::string current_string;
        size_t parts_size = parts.size();

        for (size_t i = 0; i < parts_size; ++i) {
            const std::string& part = parts[i];
            if (!current_string.empty() && current_string.back() == '"' && parts[i - 1] != ",") {
                if (part.front() == '"') {
                    current_string.pop_back();
                    current_string += part.substr(1);
                } else if (part.front() == ' ' && i + 1 < parts_size && parts[i + 1].front() == '"') {
                    current_string.pop_back();
                    current_string += parts[i + 1].substr(1);
                    result_stream << part << "  ";
                    ++i;
                } else {
                    current_string += part;
                }
            } else if (part.front() == '"' && part.back() == '"') {
                current_string = part;
            } else {
                if (!current_string.empty()) {
                    result_stream << current_string;
                    current_string.clear();
                }
                result_stream << part;
            }
        }

        if (!current_string.empty()) {
            result_stream << current_string;
        }

        return result_stream.str();
    }

    std::string StringManager::combineNearbyString(const std::string &input, const int &line_row) {
        if (input.find('\n') != std::string::npos) {
            throw std::runtime_error("Error: Input is not a single-line string. Line: " + std::to_string(line_row));
        }

        std::vector<std::string> parts;
        parts.reserve(input.size() / 4); // Ԥ�����ڴ�

        std::string currentPart;
        currentPart.reserve(input.size() / 4); // Ԥ�����ڴ�

        bool inQuotes = false;
        bool isEscaped = false;
        bool prevWasQuote = false;
        size_t lastDelimiterPos = std::string::npos;
        bool hasNonWhitespaceSinceDelimiter = true; // ���ٷָ������Ƿ��зǿո�

        for (size_t i = 0; i < input.length(); ++i) {
            char c = input[i];
            if (isEscaped) {
                currentPart += c;
                isEscaped = false;
            } else if (c == '\\') {
                currentPart += c;
                isEscaped = true;
            } else if (c == '"') {
                inQuotes = !inQuotes;
                if (!inQuotes) {
                    currentPart += c;
                    parts.push_back(std::move(currentPart)); // �ƶ�����
                    currentPart.clear();
                    prevWasQuote = true;
                    hasNonWhitespaceSinceDelimiter = true;
                } else {
                    if (!currentPart.empty()) {
                        if (prevWasQuote) {
                            throw std::runtime_error("Error: Non-quoted string between two quoted strings. Line: " + std::to_string(line_row));
                        }
                        parts.push_back(std::move(currentPart));
                        currentPart.clear();
                    }
                    currentPart += c;
                    prevWasQuote = false;
                }
            } else if (c == ',' && !inQuotes) {
                if (lastDelimiterPos != std::string::npos) {
                    if (!hasNonWhitespaceSinceDelimiter) {
                        throw std::runtime_error("Error: Adjacent delimiters contain only whitespace. Line: " + std::to_string(line_row));
                    }
                }
                hasNonWhitespaceSinceDelimiter = false; // ���ñ�־
                if (!currentPart.empty()) {
                    if (prevWasQuote) {
                        throw std::runtime_error("Error: Non-quoted string between two quoted strings. Line: " + std::to_string(line_row));
                    }
                    parts.push_back(std::move(currentPart));
                    currentPart.clear();
                }
                parts.emplace_back(1, c); // ֱ�ӹ��춺�Ų���
                prevWasQuote = false;
                lastDelimiterPos = i;
            } else if (inQuotes) {
                currentPart += c;
            } else if (std::isspace(c)) {
                if (!currentPart.empty()) {
                    if (prevWasQuote) {
                        throw std::runtime_error("Error: Non-quoted string between two quoted strings. Line: " + std::to_string(line_row));
                    }
                    parts.push_back(std::move(currentPart));
                    currentPart.clear();
                }
                do {
                    currentPart += input[i++];
                } while (i < input.length() && std::isspace(input[i]));
                i--;
                parts.push_back(std::move(currentPart));
                currentPart.clear();
            } else {
                if (lastDelimiterPos != std::string::npos) {
                    hasNonWhitespaceSinceDelimiter = true; // ��Ǵ��ڷǿո�
                }
                currentPart += c;
            }
        }

        // �쳣���
        if (isEscaped) {
            throw std::runtime_error("Error: Unescaped backslash at the end of input. Line: " + std::to_string(line_row));
        }
        if (inQuotes) {
            throw std::runtime_error("Error: Unclosed quote in the input. Line: " + std::to_string(line_row));
        }
        if (!currentPart.empty()) {
            if (prevWasQuote) {
                throw std::runtime_error("Error: Non-quoted string between two quoted strings. Line: " + std::to_string(line_row));
            }
            parts.push_back(std::move(currentPart));
        }

        return merge_adjacent_quotes(parts);
    }

    bool StringManager::needEscape(const char &c) {
        return escapeChars.contains(c);
    }

    bool StringManager::isSpace(const char &c) {
        return spaceChars.contains(c);
    }

    std::string StringManager::escapeCharToStr(const char &c) {
        return needEscape(c) ?
            getInstance().escapeHandlers[c]() :
            std::string {1, c};
    }

    char StringManager::escapeChar(const char &c) {
        return needEscape(c) ?
            escapeCharToStr(c)[0] :
            c;
    }

    // Pos����ʵ��
    Pos::Pos(size_t line, size_t column, size_t offset, std::string filepath):
    line(line), column(column), offset(offset), filepath(std::move(filepath)) {}

    size_t Pos::getLine() const {
        return line;
    }

    size_t Pos::getColumn() const {
        return column;
    }

    size_t Pos::getOffset() const {
        return offset;
    }

    std::ostream &operator<<(std::ostream &out, const Pos &pos) {
        return out << "line: " << pos.getLine() << ", column: " << pos.getColumn();
    }

    std::string Pos::toString() const {
        return filepath + ":" + std::to_string(line) + ":" + (column > 0 ? std::to_string(column) : "1") + ", line " +
        std::to_string(line) + (column > 1 ? ", column " + std::to_string(column) : "");
    }

    std::string Pos::getFilepath() const {
        return filepath;
    }

    std::string Pos::getFilePosStr() const {
        return "\"" + this->getFilepath() + ":1:1\", line 1";
    }

    void Pos::serialize(std::ostream &out, const utils::SerializationProfile &profile) const {
        if (profile < utils::SerializationProfile::Release) {
            // д���������͵ĳ�Ա����
            out.write(reinterpret_cast<const char *>(&line), sizeof(line));
            out.write(reinterpret_cast<const char *>(&column), sizeof(column));

            // д���ַ����ĳ���
            size_t filepathLength = filepath.size();
            out.write(reinterpret_cast<const char *>(&filepathLength), sizeof(filepathLength));

            // д���ַ���������
            if (filepathLength > 0) {
                out.write(filepath.c_str(), static_cast<int>(filepathLength));
            }
        }
    }

    void Pos::deserialize(std::istream &in, const utils::SerializationProfile &profile) {
        if (profile < utils::SerializationProfile::Release){
            // ��ȡ�������͵ĳ�Ա����
            in.read(reinterpret_cast<char *>(&line), sizeof(line));
            in.read(reinterpret_cast<char *>(&column), sizeof(column));

            // ��ȡ�ַ����ĳ���
            size_t filepathLength;
            in.read(reinterpret_cast<char *>(&filepathLength), sizeof(filepathLength));

            // ��ȡ�ַ���������
            if (filepathLength > 0) {
                filepath.resize(filepathLength);  // �����ַ�����С
                in.read(&filepath[0], static_cast<int>(filepathLength));
            } else {
                filepath.clear();  // �������Ϊ0������ַ���
            }
        }
    }

    void Pos::setLine(size_t line) {
        this->line = line;
    }

    void Pos::setColumn(size_t column) {
        this->column = column;
    }

    void Pos::setOffset(size_t offset) {
        this->offset = offset;
    }

    void Pos::setFilepath(const std::string &filepath) {
        this->filepath = filepath;
    }

    std::string Pos::briefString() const {
        return getFileFromPath(filepath) + ":" + std::to_string(line) + ":" + std::to_string(column);
    }

    std::string Pos::professionalString() const {
        return "Pos{line=" + std::to_string(line) + ", column=" + std::to_string(column) + ", offset=" + std::to_string(offset) + ", file=" + getFileFromPath(filepath) + "}";
    }

    std::string Pos::formatString(size_t indent, size_t level) const {
        return spaceString(indent * level) + "Pos{\n" +
        spaceString(indent * (level + 1)) + "line=" + std::to_string(line) + ",\n" +
        spaceString(indent * (level + 1)) + "column=" + std::to_string(column) + ",\n" +
        spaceString(indent * (level + 1)) + "offset=" + std::to_string(offset) + ",\n" +
        spaceString(indent * (level + 1)) + "file=" + getFileFromPath(filepath) + "\n" +
        spaceString(indent * level) + "}";
    }

    ArgType getArgType(const std::string &str) {
        if (str.empty()) {
            return ArgType::unknown;
        }
        if (isStringFormat(str)) {
            return ArgType::string;
        }
        if (isNumber(str)) {
            return ArgType::number;
        }
        if (base::containsKeyword(str)) {
            return ArgType::keyword;
        }
        if (isValidIdentifier(str)){
            return ArgType::identifier;
        }
        return ArgType::unknown;
    }

    std::string getArgTypeName(const ArgType &argType) {
        switch (argType) {
            case ArgType::identifier:
                return "identifier";
            case ArgType::keyword:
                return "keyword";
            case ArgType::number:
                return "number";
            case ArgType::string:
                return "string";
            default:
                return "unknown";
        }
    }

    std::string getTypeFormatString(const ArgType &argType){
        return "[ArgType: " + getArgTypeName(argType) + "]";
    }

    // Arg����ʵ��
    Arg::Arg(Pos pos, const std::string &value) : pos(std::move(pos)) {
        this->type = utils::getArgType(value);
        if (this->type == ArgType::string){
            this->value = std::move(utils::StringManager::getInstance().unescape(value));
        } else {
            this->value = value;
        }
    }

    const Pos &Arg::getPos() const {
        return pos;
    }

    std::string Arg::getPosStr() const {
        return pos.toString();
    }

    utils::ArgType Arg::getType() const {
        return type;
    }

    const std::string &Arg::getValue() const {
        return value;
    }

    std::string Arg::toString() const {
        switch (type) {
            case ArgType::unknown:
            case ArgType::identifier:
            case ArgType::keyword:
                return "[Arg(" + utils::getArgTypeName(type) + "): '" + value + "']";
            case ArgType::number:
                return "[Arg(" + utils::getArgTypeName(type) + "): " + value + "]";
            case ArgType::string:
                return "[Arg(" + utils::getArgTypeName(type) + "): \"" + value + "\"]";
        }
        return "Error";
    }

    Arg::Arg(const std::string& value): Arg(Pos(-1, -1, 0, ""), value) {}

    // ���л�����
    void Arg::serialize(std::ostream &out, const utils::SerializationProfile &profile) const {
        // ���л� Pos
        pos.serialize(out, profile);

        // ���л� ArgType
        utils::serializeArgType(out, type);

        // ���л��ַ����ĳ���
        size_t valueLength = value.size();
        out.write(reinterpret_cast<const char*>(&valueLength), sizeof(valueLength));

        // ���л��ַ���������
        if (valueLength > 0) {
            out.write(value.c_str(), static_cast<int>(valueLength));
        }
    }

    // �����л�����
    void Arg::deserialize(std::istream &in, const SerializationProfile &profile) {
        // �����л� Pos
        pos.deserialize(in, profile);

        // �����л� ArgType
        type = utils::deserializeArgType(in);

        // �����л��ַ����ĳ���
        size_t valueLength;
        in.read(reinterpret_cast<char*>(&valueLength), sizeof(valueLength));

        // �����л��ַ���������
        if (valueLength > 0) {
            value.resize(valueLength);
            in.read(&value[0], static_cast<int>(valueLength));
        } else {
            value.clear();
        }
    }

    std::string Arg::briefString() const {
        return "[(" + getArgTypeName(type) + ")" +  value + "]";
    }

    std::string readFile(const std::string &filepath) {
        std::ifstream inFile(filepath);
        if (!std::filesystem::exists(filepath)) {
            // ToDo: ��Ӵ������߼�
            throw std::runtime_error("File not found: " + filepath);
        }
        if (!inFile.is_open()) {
            // ToDo: ��Ӵ������߼�
            throw std::runtime_error("Unable to open file: " + filepath);
        }
        std::stringstream buffer;
        buffer << inFile.rdbuf();
        inFile.close();
        std::string content = buffer.str();
        return content;
    }

    std::vector<std::string> readFileToLines(const std::string &filepath){
        std::ifstream inFile(filepath);
        if (!std::filesystem::exists(filepath)) {
            std::cerr << "This file [path: " << filepath << "] does not exist." << std::endl;
            return {};
        }
        if (!inFile.is_open()) {
            std::cerr << "This file [path: " << filepath << "] unable to open for reading." << std::endl;
            return {};
        }
        std::vector<std::string> lines;
        std::string line;
        while (inFile) {
            std::getline(inFile, line);
            lines.push_back(line);
        }
        inFile.close();
        return lines;
    }

    // д���ļ�������ģʽ��
    bool writeFile(const std::string &filepath, const std::string &content) {
        // �Զ�����ģʽ���ļ�������ԭ������
        std::ofstream file(filepath, std::ios::binary);
        if (!file.is_open()) {
            return false; // �ļ���ʧ��
        }

        // д������
        file.write(content.data(), static_cast<int>(content.size()));
        return file.good(); // ���д���Ƿ�ɹ�
    }

    // ׷���ļ���׷��ģʽ��
    bool appendFile(const std::string &filepath, const std::string &content) {
        // �Զ�����ģʽ��׷��ģʽ���ļ�
        std::ofstream file(filepath, std::ios::binary | std::ios::app);
        if (!file.is_open()) {
            return false; // �ļ���ʧ��
        }

        // д������
        file.write(content.data(), static_cast<int>(content.size()));
        return file.good(); // ���д���Ƿ�ɹ�
    }

    bool isValidIdentifier(const std::string &content) {
        // Check if the string is empty or if the first character is not a letter or an underscore
        if (content.empty() || !isalpha(content[0]) && content[0] != '_') {
            return false;
        }
        return std::all_of(content.begin() + 1, content.end(), [](char c) {
            return isalnum(c) || c == '_';
        });
    }

    inline bool isDigit(char c) {
        return c >= '0' && c <= '9';
    }

    inline bool isSign(char c) {
        return c == '+' || c == '-';
    }

    inline bool isDoubleQuoted(const std::string &content) {
        return content.size() >= 2 && content.front() == '"' && content.back() == '"';
    }

    bool isValidNumber(const std::string &content) {
        if (content.empty()) return false;

        size_t start = 0;
        size_t end = content.size();

        // Skip leading sign if present
        if (isSign(content[start])) {
            if (end <= 1) return false; // Only a sign is not a valid number
            ++start;
        }

        // Check for decimal point and digits
        bool hasDecimalPoint = false;
        for (size_t i = start; i < end; ++i) {
            if (content[i] == '.') {
                if (hasDecimalPoint || i == start || i == end - 1) {
                    return false;
                }
                hasDecimalPoint = true;
            } else if (!isDigit(content[i])) {
                return false;
            }
        }

        // Ensure there's at least one digit
        return !hasDecimalPoint || (end - start > 1);
    }

    bool isValidKeyWord(const std::string &content) {
        return base::containsKeyword(content);
    }

    bool isValidParameter(const std::string &content) {
        if (content.empty()) {
            return false;
        }

        // Check if the string is enclosed in double quotes
        if (isDoubleQuoted(content)) {
            return true;
        }

        // Check if the string is a valid number
        if (isValidNumber(content)) {
            return true;
        }

        // Check if the string is a valid keyword
        if (isValidKeyWord(content)){
            return true;
        }

        // Delegate to isValidIdentifier for further checks
        return isValidIdentifier(content);
    }

    std::string getSpaceFormatString(const std::string& name, const std::string& value){
        return "[Space(" + name + "): " + value + "]";
    }

    size_t getSpaceFrontOfLineCode(const std::string& lineCode){
        size_t num = 0;
        for (const auto c: lineCode){
            if (c == ' ' || c == '\t'){
                num ++;
            } else {
                return num;
            }
        }
        return num;
    }

    void addCharBeforeNewline(std::string &code, const char &c) {
        std::string new_code;
        size_t prev = 0;
        size_t pos = 0;

        while ((pos = code.find('\n', pos)) != std::string::npos) {
            // ����prev��ʼ�����з������������Ĳ�����ӵ����ַ���
            new_code += code.substr(prev, pos - prev);
            // ���Ŀ���ַ�
            new_code += c;
            // ��ӻ��з�
            new_code += '\n';
            // ������һ�β��ҵ���ʼλ��Ϊ��ǰ���з�֮��
            prev = pos + 1;
            pos = prev;  // ��Ҫ���´β��Ҵӵ�ǰ���з�֮��ʼ
        }

        // ������һ�����з�֮�������ʣ������
        new_code += code.substr(prev);
        code.swap(new_code);
    }

    std::string getSerializationProfileName(const SerializationProfile &profile){
        switch (profile) {
            case SerializationProfile::Debug:
                return "Debug";
            case SerializationProfile::Minified:
                return "Minified";
            case SerializationProfile::Release:
                return "Release";
            case SerializationProfile::Testing:
                return "Testing";
            default:
                return "Unknown";
        }
    }

    void setProgramEnvDir(const std::string &dir_path) {
        namespace fs = std::filesystem;
        std::error_code ec;
        fs::current_path(fs::path(dir_path), ec); // ���ù���Ŀ¼
        if (ec) {
            throw std::runtime_error(ec.message());
        }
    }

    std::string printProgramSpentTimeInfo() {
        // ��ȡ����ʱ���
        auto end = std::chrono::high_resolution_clock::now();
        // ����ʱ���
        auto duration = std::chrono::duration_cast<std::chrono::duration<double>>(end - program_start_time); // ���ִ��ʱ��
        std::ostringstream oss;
        oss << "\nProcess finished in " << duration.count() << " seconds.\n";
        return oss.str();
    }

    std::string getAbsolutePath(const std::string &path, const std::string &dir_path)  {
        if (path.empty()) return "";
        namespace fs = std::filesystem;
        try {
            // ������·��ת��Ϊ filesystem::path ����
            fs::path input_path(path);

            // ���·���Ѿ��Ǿ���·����ֱ�ӷ���
            if (input_path.is_absolute()) {
                return input_path.string();
            }

            // ���򣬽����·���뵱ǰ����Ŀ¼��ϣ����ɾ���·��
            fs::path absolute_path;
            if (dir_path.empty() || dir_path == RCC_UNDEFINED_CONST) {
                absolute_path = fs::absolute(input_path);
            } else {
                absolute_path = fs::path(dir_path) / input_path;
            }
            return fs::weakly_canonical(absolute_path).string();
        } catch (const std::exception& e) {
            throw std::runtime_error("Failed to convert path to absolute path: " + std::string(e.what()));
        }
    }

    std::tuple<char, char, char, char, char> getSeparators(TimeFormat format)  {
        switch (format) {
            case TimeFormat::ISO:
                return {'-', '-', ' ', ':', ':'};
            case TimeFormat::US:
            case TimeFormat::European:
                return {'/', '/', ' ', ':', ':'};
            case TimeFormat::Timestamp:
            default:
                throw std::runtime_error("Unsupported date format");
        }
    }

    bool parseDateFromString(const std::string &dateString, TimeFormat format,
                             int &year, int &month, int &day, int &hour, int &minute, int &second)  {
        auto [sep1, sep2, sep3, sep4, sep5] = getSeparators(format);

        std::istringstream iss(dateString);
        char c1, c2, c3, c4, c5;

        iss >> year >> c1 >> month >> c2 >> day >> std::noskipws >> c3 >> std::skipws >> hour >> c4 >> minute >> c5 >> second;

        // ���ָ����Ƿ�ƥ��
        if (c1 != sep1 || c2 != sep2 || c3 != sep3 || c4 != sep4 || c5 != sep5) {
            return false;
        }

        // �������ֵ�Ƿ���Ч
        if (year < 0 || month < 1 || month > 12 || day < 1 || day > 31 ||
            hour < 0 || hour > 23 || minute < 0 || minute > 59 || second < 0 || second > 59) {
            return false;
        }

        return true;
    }

    // ��ӱ�־������֧�ֱ�����
    void ProgArgParser::addFlag(const std::string& name, bool* var, bool defaultValue, [[maybe_unused]] bool whenPresent,
                                const std::string& description, const std::vector<std::string>& aliases) {
        *var = defaultValue;
        flags_.emplace_back(name, var, whenPresent, aliases, description);
    }

    void ProgArgParser::parse(int argc, char* argv[]) {
        std::set<std::string> providedOptions;

        for (int i = 1; i < argc; ) {
            std::string arg = argv[i];
            if (arg.size() >= 2 && arg.substr(0, 2) == "--") {
                std::string name_part = arg.substr(2);
                size_t eq_pos = name_part.find('=');
                std::string name = name_part.substr(0, eq_pos);

                // ��ѡ��������Ƽ��� providedOptions
                providedOptions.insert(getOptionMainName(name));

                handleOption(arg, argc, argv, i);
            } else {
                throw std::runtime_error("Unexpected argument: " + arg);
            }
        }

        // ��黥�����
        for (const auto& rule : mutuallyExclusive_) {
            std::string opt1 = getOptionMainName(rule.opt1);
            std::string opt2 = getOptionMainName(rule.opt2);

            if (providedOptions.contains(opt1) && providedOptions.contains(opt2)) {
                throw std::runtime_error("Options '--" + rule.opt1 + "' and '--" + rule.opt2 + "' are mutually exclusive");
            }

            // �����˫���飬����Ҫ��鷴�����
            if (rule.direction == CheckDir::BiDir) {
                if (providedOptions.contains(opt2) && providedOptions.contains(opt1)) {
                    throw std::runtime_error("Options '--" + rule.opt2 + "' and '--" + rule.opt1 + "' are mutually exclusive");
                }
            }
        }

        // ����������
        for (const auto& rule : dependent_) {
            std::string opt1 = getOptionMainName(rule.opt1);
            std::string opt2 = getOptionMainName(rule.opt2);

            if (providedOptions.contains(opt1)) {
                if (!providedOptions.contains(opt2)) {
                    throw std::runtime_error("Option '--" + rule.opt1 + "' requires '--" + rule.opt2 + "'");
                }
            }

            // �����˫���飬����Ҫ��鷴�����
            if (rule.direction == CheckDir::BiDir) {
                if (providedOptions.contains(opt2)) {
                    if (!providedOptions.contains(opt1)) {
                        throw std::runtime_error("Option '--" + rule.opt2 + "' requires '--" + rule.opt1 + "'");
                    }
                }
            }
        }
    }

    void ProgArgParser::handleOption(const std::string& arg, int argc, char* argv[], int& i) {
        std::string name_part = arg.substr(2);
        const size_t eq_pos = name_part.find('=');
        const std::string name = name_part.substr(0, eq_pos);
        const bool has_value = (eq_pos != std::string::npos);
        const std::string value = has_value ? name_part.substr(eq_pos + 1) : "";

        // ����ؼ��ֲ���
        if (const auto* opt = findOption(name)) {
            try {
                if (has_value) {
                    opt->setter(value);
                    i++;
                } else {
                    if (++i >= argc) throw std::runtime_error("Missing value");
                    if (std::string(argv[i]).substr(0, 2) == "--") {
                        throw std::runtime_error("Value expected");
                    }
                    opt->setter(argv[i]);
                    i++;
                }
            } catch (const std::exception& e) {
                throw std::runtime_error("Option '--" + name + "': " + e.what());
            }
            return;
        }

        // �����־����
        if (const auto* flag = findFlag(name)) {
            if (has_value) throw std::runtime_error("Flag doesn't accept value");
            *(flag->var) = flag->whenPresent;
            i++;
            return;
        }

        throw std::runtime_error("Unknown option: '--" + name + "'");
    }

    std::string ProgArgParser::getOptionDescription(const std::string &name) const {
        // ���ؼ��ֲ���
        for (const auto& opt : options_) {
            if (opt.name == name) return opt.toString();
            for (const auto& alias : opt.aliases) {
                if (alias == name) return opt.toString();
            }
        }
        // ����־����
        for (const auto& flag : flags_) {
            if (flag.name == name) return flag.toString();
            for (const auto& alias : flag.aliases) {
                if (alias == name) return flag.toString();
            }
        }
        throw std::runtime_error("Unknown option: '" + name + "'");
    }

    // ���ɸ�ʽ���İ����ַ���
    std::string ProgArgParser::getHelpString(size_t lineWidth, size_t optionIndent, size_t descriptionIndent) const {
        std::ostringstream oss;

        oss << "Usage:\n";
        oss << "  RVM.exe [options]\n\n";
        oss << "Flags:\n";

        // ��ӱ�־�����İ�����Ϣ
        for (const auto& flag : flags_) {
            // ѡ�����ƺͱ���
            oss << std::string(optionIndent, ' ') << "['--" << flag.name;
            for (const auto& alias : flag.aliases) {
                oss << "' | '--" << alias;
            }
            oss << "']\n";

            // ��ʾ��Ϣ�������۵���
            std::string wrappedDescription = StringManager::wrapText(flag.description, lineWidth, descriptionIndent);
            oss << std::string(descriptionIndent, ' ') << wrappedDescription << "\n\n";
        }

        oss << "Options:\n";

        // ��ӹؼ��ֲ����İ�����Ϣ
        for (const auto& opt : options_) {
            // ѡ�����ƺͱ���
            oss << std::string(optionIndent, ' ') << "['--" << opt.name;
            for (const auto& alias : opt.aliases) {
                oss << "' | '--" << alias;
            }
            oss << "'] <value>\n";

            // ��ʾ��Ϣ�������۵���
            std::string wrappedDescription = StringManager::wrapText(opt.description, lineWidth, descriptionIndent);
            oss << std::string(descriptionIndent, ' ') << wrappedDescription << "\n\n";
        }

        return oss.str();
    }

    std::string ProgArgParser::getOptionMainName(const std::string &name) const {
        // ���ؼ��ֲ���
        for (const auto& opt : options_) {
            if (opt.name == name) return opt.name;
            for (const auto& alias : opt.aliases) {
                if (alias == name) return opt.name;
            }
        }
        // ����־����
        for (const auto& flag : flags_) {
            if (flag.name == name) return flag.name;
            for (const auto& alias : flag.aliases) {
                if (alias == name) return flag.name;
            }
        }
        return name; // ���û���ҵ�������ԭʼ����
    }

    ProgArgParser::FlagInfo *ProgArgParser::findFlag(const std::string &name) {
        for (auto& flag : flags_) {
            if (flag.name == name) return &flag;
            for (const auto& alias : flag.aliases) {
                if (alias == name) return &flag;
            }
        }
        return nullptr;
    }

    ProgArgParser::OptionInfo *ProgArgParser::findOption(const std::string &name) {
        for (auto& opt : options_) {
            if (opt.name == name) return &opt;
            for (const auto& alias : opt.aliases) {
                if (alias == name) return &opt;
            }
        }
        return nullptr;
    }

    void ProgArgParser::addMutuallyExclusive(const std::vector<std::string> &options, const std::string &target,
                                             ProgArgParser::CheckDir direction) {
        for (const auto& opt : options) {
            addMutuallyExclusive(opt, target, direction);
        }
    }

    void ProgArgParser::addMutuallyExclusive(const std::vector<std::string> &options,
                                             const std::vector<std::string> &targets,
                                             ProgArgParser::CheckDir direction) {
        for (const auto& opt : options) {
            for (const auto& target : targets) {
                addMutuallyExclusive(opt, target, direction);
            }
        }
    }

    void ProgArgParser::addDependent(const std::vector<std::string> &options, const std::string &target,
                                     ProgArgParser::CheckDir direction) {
        for (const auto& opt : options) {
            addDependent(opt, target, direction);
        }
    }

    void ProgArgParser::addDependent(const std::vector<std::string> &options, const std::vector<std::string> &targets,
                                     ProgArgParser::CheckDir direction) {
        for (const auto& opt : options) {
            for (const auto& target : targets) {
                addDependent(opt, target, direction);
            }
        }
    }

    void ProgArgParser::addMutuallyExclusiveGroup(const std::vector<std::string> &options,
                                                  ProgArgParser::CheckDir direction) {
        for (size_t i = 0; i < options.size(); ++i) {
            for (size_t j = i + 1; j < options.size(); ++j) {
                addMutuallyExclusive(options[i], options[j], direction);
            }
        }
    }

    void ProgArgParser::addDependentGroup(const std::vector<std::string> &options, ProgArgParser::CheckDir direction) {
        for (size_t i = 0; i < options.size(); ++i) {
            for (size_t j = i + 1; j < options.size(); ++j) {
                addDependent(options[i], options[j], direction);
            }
        }
    }

    void ProgArgParser::addMutuallyExclusive(const std::string &opt1, const std::string &opt2,
                                             [[maybe_unused]] ProgArgParser::CheckDir direction) {
        mutuallyExclusive_.emplace_back(opt1, opt2, direction);
    }

    void
    ProgArgParser::addDependent(const std::string &opt1, const std::string &opt2, [[maybe_unused]] ProgArgParser::CheckDir direction) {
        dependent_.emplace_back(opt1, opt2, direction);
    }

    void ProgArgParser::addMutuallyExclusive(const std::string &opt1, const std::vector<std::string> &opt2,
                                             ProgArgParser::CheckDir direction) {
        for (const auto &opt: opt2) {
            addMutuallyExclusive(opt1, opt, direction);
        }
    }

    std::string ProgArgParser::FlagInfo::toString() const {
        std::ostringstream oss;
        oss << "[Flag: '" << name;
        for (const auto& alias : aliases) {
            oss << "' | '" << alias;
        }
        oss << "']: ";
        oss << StringManager::wrapText(description, 80, oss.str().size());
        return oss.str();
    }

    ProgArgParser::FlagInfo::FlagInfo(std::string name, bool *var, bool whenPresent, std::vector<std::string> aliases,
                                      std::string description)
                                      : name(std::move(name)), var(var), whenPresent(whenPresent), aliases(std::move(aliases)),
                                      description(std::move(description)){}

    int getRandomInt(int min, int max){
        if (min > max) {
            throw std::invalid_argument("Invalid range: min > max");
        }
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> distrib(min, max);
        return distrib(gen);
    }

    /**
     * ����һ������ min �� max ֮��������������������ָ����С��λ����
     *
     * @param min ��Сֵ��������
     * @param max ���ֵ��������
     * @param decimalPlaces ������С��λ��������Ϊ�Ǹ�����
     * @return ���ɵ����������
     * @throws std::invalid_argument ��� min > max �� decimalPlaces Ϊ����
     */
    double getRandomFloat(double min, double max, int decimalPlaces) {
        // ����У��
        if (min > max) {
            throw std::invalid_argument("Invalid range: min > max");
        }
        if (decimalPlaces < 0) {
            throw std::invalid_argument("Invalid decimalPlaces: must be non-negative");
        }

        // ��̬������������������ظ�����
        static std::random_device rd;  // ����豸��������������
        static std::mt19937 gen(rd()); // ʹ�� Mersenne Twister �㷨���������
        std::uniform_real_distribution<> distrib(min, max); // ���帡�����ֲ���Χ

        // �������������
        double randomValue = distrib(gen);

        // ����ָ����С��λ��
        if (decimalPlaces > 0) {
            double factor = std::pow(10, decimalPlaces); // ���� 10^decimalPlaces
            randomValue = std::round(randomValue * factor) / factor; // ��������
        }

        return randomValue;
    }

    std::string getFileDirFromPath(const std::string &path) {
        std::filesystem::path file_path(path);
        return file_path.parent_path().string();
    }

    std::string getEscapedPathFormatString(const std::string &path) {
        return "\"" + StringManager::escape(path) + "\"";
    }

    void appendProgramWorkingDir(const std::string &path) {
        base::PROGRAM_WORKING_DIRECTORY_STACK.push(getAbsolutePath(getFileDirFromPath(path)));
    }

    ProgArgParser::OptionInfo::OptionInfo(std::string name, std::function<void(const std::string &)> setter,
                                          std::vector<std::string> aliases, std::string description)
                                          : name(std::move(name)), setter(std::move(setter)), aliases(std::move(aliases)),
                                           description(std::move(description)){}

    ProgArgParser::MutuallyExclusiveRule::MutuallyExclusiveRule(std::string opt1, std::string opt2,
                                                                ProgArgParser::CheckDir direction)
                                                                : opt1(std::move(opt1)), opt2(std::move(opt2)), direction(direction){}

    ProgArgParser::DependentRule::DependentRule(std::string opt1, std::string opt2, ProgArgParser::CheckDir direction)
    : opt1(std::move(opt1)), opt2(std::move(opt2)), direction(direction){}

    RangerPos::RangerPos(size_t startLine, size_t startColumn, size_t endLine, size_t endColumn, std::string filepath)
    : Pos(startLine, startColumn, -1, filepath), endLine(endLine), endColumn(endColumn){}

    std::string RangerPos::toString() const {
        return filepath + ":" + std::to_string(line) + ":" + (column > 0 ? std::to_string(column) : "1") + ", line " +
                std::to_string(line) + (column > 1 ? ", column " + std::to_string(column) : "") + " to " +
                "line " + std::to_string(endLine) + (endColumn > 1 ? ", column " + std::to_string(endColumn) : "");
    }

    std::string RangerPos::briefString() const {
        return getFileFromPath(filepath) + ":" + std::to_string(line) + ":" + std::to_string(column) + " ~ " + std::to_string(endLine) + ":" + std::to_string(endColumn);
    }

    std::string RangerPos::professionalString() const {
        return "Pos{startLine=" + std::to_string(line) + ", startColumn=" + std::to_string(column) + ", endLine=" + std::to_string(endLine) + ", endColumn=" + std::to_string(endColumn) + ", file=" + getFileFromPath(filepath) + "}";
    }

    size_t RangerPos::getEndLine() const {
        return endLine;
    }

    size_t RangerPos::getEndColumn() const {
        return endColumn;
    }

    std::ostream &operator<<(std::ostream &out, const RangerPos &rangerPos) {
        return out << "line: " << rangerPos.getLine() << ", column: " << rangerPos.getColumn() << " to " << "line: " << rangerPos.getEndLine() << ", column: " << rangerPos.getEndColumn();
    }

    std::string spaceString(size_t n) {
        return std::string(n, ' ');
    }
} // utils
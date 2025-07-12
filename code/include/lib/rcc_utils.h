//
// Created by RestRegular on 2025/1/17.
//

#ifndef RVM_RA_UTILS_H
#define RVM_RA_UTILS_H

#include <functional>
#include <map>
#include <vector>
#include <unordered_set>
#include <sstream>

namespace utils {

    // === 基础类型定义 ===
    class Object {
        static size_t _id;
        size_t id;
    public:
        Object();
        [[nodiscard]] virtual std::string toString() const;
        [[nodiscard]] virtual std::string briefString() const;
        [[nodiscard]] virtual std::string professionalString() const;
        [[nodiscard]] virtual std::string formatString(size_t indent, size_t level) const;
        [[nodiscard]] virtual std::string toJsonString() const;
        virtual ~Object() = default;
    };

    struct Number;
    enum class NumType {
        double_type,
        int_type,
        NotNumber
    };

    enum class ArgType{
        number,
        string,
        identifier,
        keyword,
        unknown
    };

    enum class FieldType{
        InstField,
        TPField
    };

    enum class SerializationProfile {
        Debug,         // 完整信息：源码、符号表、行号、版本（开发/调试）
        Testing,       // 移除源码但保留符号表 & 行号（CI/CD自动化测试）
        Release,       // 仅指令流 + 必要元数据 + 版本（生产环境）
        Minified       // 极致精简（无版本/元数据，仅用于封闭沙盒）
    };

    enum class TimeFormat {
        ISO,        // YYYY-MM-DD
        US,         // MM/DD/YYYY
        European,   // DD/MM/YYYY
        Timestamp   // Unix timestamp
    };

    // === 字符串处理 ===
    struct StringManager: Object {
    public:
        static StringManager &getInstance();
        static std::vector<std::string> split(const std::string &content, char delimiter = ',');
        std::string processQuotedString(std::string_view input);
        void registerEscapeHandler(char escapeChar, std::function<std::string(void)> handler);
        std::string unescape(const std::string &input);
        void unescape_nret(std::string &input);
        static std::string parseStringFormat(const std::string &input);
        static void parseStringFormat_nret(std::string &result);
        static bool isStringFormat(const std::string &str);
        static bool isStrictValidStringFormat(const std::string &str);
        static bool isCharFormat(const std::string &str);
        static void trim(std::string &str);
        static std::string trim(const std::string &str);
        static std::string toStringFormat(const std::string &str);
        static std::map<std::string, std::string> splitStringByChars(const std::string& input, const std::string& delimiters);
        static std::string escape(const std::string &input);
        static std::string escapeCharToStr(const char &c);
        static char escapeChar(const char &c);
        static std::string wrapText(const std::string& text, size_t lineWidth, size_t indent = 0,
                                    const std::string& last_line_suffix = "", const std::string& next_line_prefix = "");
        static std::string combineNearbyString(const std::string &input, const int &line_row);
        static bool needEscape(const char &c);
        static bool isSpace(const char &c);
    private:
        StringManager();
        std::string handleEscapeSequence(std::string_view input, size_t &pos);
        static constexpr size_t INITIAL_BUFFER_SIZE = 1024;
        std::map<char, std::function<std::string()>> escapeHandlers;
        static std::unordered_set<char> escapeChars;
        static std::unordered_set<char> rawEscapeChars;
        static std::unordered_set<char> spaceChars;
    };

    std::string doubleToString(double value);
    bool isStringFormat(const std::string& str);
    std::string parseStringFormat(const std::string& str);
    void parseStringFormat_noReturn(std::string& result);
    std::string getSpaceFormatString(const std::string& name, const std::string& value);
    std::string getObjectFormatString(const std::string &type, const std::string &name);
    std::string getTypeFormatString(const ArgType &argType);
    std::string getArgTypeName(const ArgType& argType);
    std::string getSerializationProfileName(const SerializationProfile &profile);
    std::string printProgramSpentTimeInfo();
    std::string getPosStrFromFilePath(const std::string &file_path);
    size_t getSpaceFrontOfLineCode(const std::string& lineCode);
    void addCharBeforeNewline(std::string &code, const char &c= ';');
    std::string spaceString(size_t n);

    // === 路径处理 ===
    std::string getFileNameFromPath(const std::string &path);
    std::string getFileExtFromPath(const std::string &path);
    std::pair<std::string, std::string> getFileInfoFromPath(const std::string &path);
    std::string getFileFromPath(const std::string &path);
    std::string getFileDirFromPath(const std::string &path);
    std::string getWindowsRVMDir();
    std::string getWindowsDefaultDir();
    std::string getAbsolutePath(const std::string& path, const std::string &dir_path = "");
    std::string getEscapedPathFormatString(const std::string& path);
    void setProgramEnvDir(const std::string &dir_path);
    void appendProgramWorkingDir(const std::string &path);

    // === 数据类型转换 ===
    int stringToInt(const std::string& str);
    double stringToDouble(const std::string& str);
    char stringToChar(const std::string& str);
    bool stringToBool(const std::string& str);
    Number stringToNumber(const std::string& str);
    ArgType getArgType(const std::string& str);
    bool isValidNumber(const std::string &content);
    bool isValidIdentifier(const std::string &content);
    bool isValidKeyWord(const std::string &content);
    bool isValidParameter(const std::string &content);
    bool isNumber(const std::string &str);
    inline bool isDigit(char c);
    inline bool isSign(char c);
    inline bool isDoubleQuoted(const std::string &content);

    // === 序列化/反序列化 ===
    void serializeArgType(std::ostream &os, const ArgType &argType);
    ArgType deserializeArgType(std::istream &is);
    std::tuple<char, char, char, char, char> getSeparators(TimeFormat format);
    bool parseDateFromString(const std::string &dateString, TimeFormat format,
                             int &year, int &month, int &day, int &hour, int &minute, int &second);

    // === 文件操作 ===
    std::string readFile(const std::string& filepath);
    std::vector<std::string> readFileToLines(const std::string &filepath);
    bool writeFile(const std::string &filepath, const std::string &content);
    bool appendFile(const std::string &filepath, const std::string &content);

    // === 位置信息 ===
    struct Pos : Object {
        Pos() = default;
        Pos(size_t line, size_t column, size_t offset, std::string filepath);
        [[nodiscard]] size_t getLine() const;
        [[nodiscard]] size_t getColumn() const;
        [[nodiscard]] size_t getOffset() const;
        [[nodiscard]] std::string getFilepath() const;
        [[nodiscard]] std::string getFilePosStr() const;
        friend std::ostream& operator<<(std::ostream& out, const Pos& pos);
        [[nodiscard]] std::string toString() const override; // 获取位置信息详细字符串
        [[nodiscard]] std::string briefString() const override; // 获取位置信息简略字符串
        [[nodiscard]] std::string professionalString() const override; // 获取位置信息详细字符串
        [[nodiscard]] std::string formatString(size_t indent, size_t level) const override;
        void serialize(std::ostream& out, const SerializationProfile &profile) const;
        void deserialize(std::istream& in, const SerializationProfile &profile);
        void setLine(size_t line);
        void setColumn(size_t column);
        void setOffset(size_t offset);
        void setFilepath(const std::string &filepath);
    protected:
        size_t line{0};
        size_t column{0};
        size_t offset{0};
        std::string filepath{};
    };

    struct RangerPos: Pos {
        RangerPos() = default;
        RangerPos(size_t startLine, size_t startColumn, size_t endLine, size_t endColumn, std::string filepath);

        [[nodiscard]] std::string toString() const override;

        [[nodiscard]] std::string briefString() const override;

        [[nodiscard]] std::string professionalString() const override;

        friend std::ostream& operator<<(std::ostream& out, const RangerPos& rangerPos);

        [[nodiscard]] size_t getEndLine() const;

        [[nodiscard]] size_t getEndColumn() const;

    protected:
        size_t endLine{0};
        size_t endColumn{0};
    };

    // === 参数处理 ===
    struct Arg : Object {
    public:
        Arg() = default;
        Arg(Pos pos, const std::string &value);
        explicit Arg(const std::string& value);
        [[nodiscard]] const Pos &getPos() const;
        [[nodiscard]] utils::ArgType getType() const;
        [[nodiscard]] const std::string &getValue() const;
        [[nodiscard]] std::string toString() const override;
        [[nodiscard]] std::string briefString() const override;
        [[nodiscard]] std::string getPosStr() const;
        void serialize(std::ostream& out, const SerializationProfile &profile) const;
        void deserialize(std::istream& in, const SerializationProfile &profile);
    private:
        Pos pos{};
        utils::ArgType type{};
        std::string value;
    };

    // === 命令行参数解析器 ===
    class ProgArgParser: Object {
    public:
        enum class CheckDir {
            UniDir,
            BiDir,
            Unidirectional = UniDir,
            Bidirectional = BiDir
        };

        void addMutuallyExclusive(const std::vector<std::string>& options,
                                  const std::string& target, CheckDir direction = CheckDir::Bidirectional);
        void addMutuallyExclusive(const std::vector<std::string>& options,
                                  const std::vector<std::string>& targets,
                                  CheckDir direction = CheckDir::Bidirectional);
        void addDependent(const std::vector<std::string>& options,
                          const std::string& target, CheckDir direction = CheckDir::Bidirectional);
        void addDependent(const std::vector<std::string>& options,
                          const std::vector<std::string>& targets,
                          CheckDir direction = CheckDir::Bidirectional);
        void addMutuallyExclusiveGroup(const std::vector<std::string>& options,
                                       CheckDir direction = CheckDir::Bidirectional);
        void addDependentGroup(const std::vector<std::string>& options,
                               CheckDir direction = CheckDir::Bidirectional);
        void addMutuallyExclusive(const std::string& opt1, const std::string& opt2,
                                  CheckDir direction = CheckDir::UniDir);
        void addMutuallyExclusive(const std::string& opt1, const std::vector<std::string>& opt2,
                                  CheckDir direction = CheckDir::UniDir);
        void addDependent(const std::string& opt1, const std::string& opt2,
                          CheckDir direction = CheckDir::UniDir);
        void addFlag(const std::string& name, bool* var, bool defaultValue, [[maybe_unused]] bool whenPresent,
                     const std::string& description = "", const std::vector<std::string>& aliases = {});
        template<typename T>
        void addOption(const std::string& name, T* var, const T& defaultValue, const std::string& description = "", const std::vector<std::string>& aliases = {}) {
            *var = defaultValue;
            setupOptionSetter(name, var, aliases, description);
        }
        void parse(int argc, char* argv[]);
        [[nodiscard]] std::string getOptionDescription(const std::string& name) const;
        [[nodiscard]] std::string getHelpString(size_t lineWidth = 80, size_t optionIndent = 2, size_t descriptionIndent = 6) const;

    private:
        class FlagInfo: Object {
        public:
            std::string name;
            bool* var;
            bool whenPresent;
            std::vector<std::string> aliases; // 存储别名
            std::string description;         // 存储描述
            FlagInfo(std::string name, bool* var, bool whenPresent, std::vector<std::string> aliases, std::string description);
            [[nodiscard]] std::string toString() const override;
        };

        class OptionInfo: Object {
        public:
            std::string name;
            std::function<void(const std::string&)> setter;
            std::vector<std::string> aliases; // 存储别名
            std::string description;         // 存储描述
            OptionInfo(std::string name, std::function<void(const std::string&)> setter, std::vector<std::string> aliases, std::string description);
            [[nodiscard]] std::string toString() const override {
                std::ostringstream oss;
                oss << "[Option: '" << name;
                for (const auto& alias : aliases) {
                    oss << "' | '" << alias;
                }
                oss << "']: ";
                oss << StringManager::wrapText(description, 80, oss.str().size());
                return oss.str();
            }
        };

        std::vector<FlagInfo> flags_;
        std::vector<OptionInfo> options_;
        OptionInfo* findOption(const std::string& name);
        FlagInfo* findFlag(const std::string& name);
        [[nodiscard]] std::string getOptionMainName(const std::string& name) const;

        class MutuallyExclusiveRule: Object {
        public:
            std::string opt1;
            std::string opt2;
            CheckDir direction;
            MutuallyExclusiveRule(std::string opt1, std::string opt2, CheckDir direction);
        };
        std::vector<MutuallyExclusiveRule> mutuallyExclusive_;

        class DependentRule: Object {
        public:
            std::string opt1;
            std::string opt2;
            CheckDir direction;
            DependentRule(std::string opt1, std::string opt2, CheckDir direction);
        };
        std::vector<DependentRule> dependent_;

        template<typename T>
        void setupOptionSetter(const std::string& name, T* var, const std::vector<std::string>& aliases,
                               const std::string& description) {
            if constexpr (std::is_same_v<T, std::string>) {
                options_.push_back({name, [var](const std::string& value) { *var = value; }, aliases, description});
            } else if constexpr (std::is_same_v<T, int>) {
                options_.push_back({name, [var](const std::string& value) {
                    try { *var = std::stoi(value); }
                    catch (...) { throw std::invalid_argument("Invalid integer value"); }
                }, aliases, description});
            } else if constexpr (std::is_same_v<T, double>) {
                options_.push_back({name, [var](const std::string& value) {
                    try { *var = std::stod(value); }
                    catch (...) { throw std::invalid_argument("Invalid double value"); }
                }, aliases, description});
            } else if constexpr (std::is_same_v<T, bool>) {
                options_.push_back({name, [var](const std::string& value) {
                    if (value == "true" || value == "1") *var = true;
                    else if (value == "false" || value == "0") *var = false;
                    else throw std::invalid_argument("Invalid boolean value");
                }, aliases, description});
            } else if constexpr (std::is_same_v<T, SerializationProfile>) {
                options_.push_back({name, [var](const std::string& value) {
                    if (value == "Debug" || value == "-db" || value == "0" || value == "L0")
                        *var = SerializationProfile::Debug;
                    else if (value == "Testing" || value == "-tst" || value == "1" || value == "L1")
                        *var = SerializationProfile::Testing;
                    else if (value == "Release" || value == "-rel" || value == "2" || value == "L2")
                        *var = SerializationProfile::Release;
                    else if (value == "Minified" || value == "-min" || value == "3" || value == "L3")
                        *var = SerializationProfile::Minified;
                    else throw std::invalid_argument("Invalid SerializationProfile value");
                }, aliases, description});
            } else {
                static_assert(!std::is_same_v<T, T>, "Unsupported type");
            }
        }

        void handleOption(const std::string& arg, int argc, char* argv[], int& i);
    };

    // === 数值处理 ===
    struct Number: Object {
        NumType type;
        int int_value;
        double double_value;
        explicit Number();
        explicit Number(int int_value);
        explicit Number(double double_value);
        [[nodiscard]] std::string toString() const override;
        friend std::ostream& operator<<(std::ostream& os, const Number& num);
    };

    int getRandomInt(int min, int max);
    double getRandomFloat(double min, double max, int decimalPlaces = 6);

    // === Utils ===
    template<typename T>
    size_t vectorLastIndex(const std::vector<T> &vec) {
        return vec.empty() ? -1 : vec.size() - 1;
    }
} // utils

#endif //RVM_RA_UTILS_H
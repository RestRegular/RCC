//
// Created by RestRegular on 2025/1/17.
//

#ifndef RCC_RA_UTILS_H
#define RCC_RA_UTILS_H

#include <functional>
#include <map>
#include <vector>
#include <unordered_set>
#include <sstream>
#include <list>
#include <memory>

#include "../interfaces/rcc_object_interface.h"
#include "../interfaces/rcc_pos_interface.h"

namespace utils {

    // === 基础类型定义 ===
    class Object:
    virtual public IRCCObjectInterface
    {
        static size_t _id;
        size_t id;
        std::string _hasCode;
        std::string _uniqueId;
    public:
        Object();
        ~Object() override;
        [[nodiscard]] virtual std::string toString() const;
        const char* ToString() const override;
        [[nodiscard]] virtual std::string briefString() const;
        const char* BriefString() const override;
        [[nodiscard]] virtual std::string professionalString() const;
        const char* ProfessionalString() const override;
        [[nodiscard]] virtual std::string formatString(size_t indent, size_t level) const;
        const char* FormatString(const size_t &indent, const size_t &level) const override;
        [[nodiscard]] virtual std::string toJsonString() const;
        const char* ToJsonString() const override;
        [[nodiscard]] virtual std::string hashCode();
        const char* HasCode() override;
        [[nodiscard]] virtual std::string uniqueId();
        const char* UniqueId() override;
        [[nodiscard]] virtual std::shared_ptr<Object> copySelf() const;
        const IRCCObjectInterface* CopySelf() override;
        const Object* Transform() const override;
        [[nodiscard]] virtual size_t getId() const;
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

    enum class OutputFormat
    {
        TXT,
        JSON
    };

    enum class TimeFormat {
        ISO,             // YYYY-MM-DD
        ISO_WITH_TIME,   // YYYY-MM-DD HH:MM:SS
        US,              // MM/DD/YYYY
        US_WITH_TIME,    // MM/DD/YYYY HH:MM:SS
        European,        // DD/MM/YYYY
        European_WITH_TIME, // DD/MM/YYYY HH:MM:SS
        Timestamp,       // Unix timestamp (seconds since epoch)
        TimestampMS      // Unix timestamp with milliseconds
    };

    // === 字符串处理 ===
    struct StringManager final : Object {
        static StringManager &getInstance();
        static std::vector<std::string> split(const std::string &content, char delimiter = ',');
        std::string processQuotedString(std::string_view input);
        void registerEscapeHandler(char escapeChar, std::function<std::string(void)> handler);
        static std::string unescape(const std::string &input);
        static void unescape_nret(std::string &input);
        static std::string parseStringFormat(const std::string &input);
        static void parseStringFormat_nret(std::string &result);
        static std::pair<bool, std::string> isStringFormat(const std::string& str);
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
                                    const std::string& lastLineSuffix = "", const std::string& nextLinePrefix = "",
                                    const bool& retainPreSpaces = false);
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
    std::string makeFileIdentStr(const std::string &content, const bool& isExtension = false);
    std::string listJoin(const std::list<std::string>& strList, const std::string& delimiter="");
    std::string vectorJoin(const std::vector<std::string>& strVector, const std::string& delimiter="");


    // === 路径处理 ===
    std::string getFileNameFromPath(const std::string &path);
    std::string getFileExtFromPath(const std::string &path);
    std::pair<std::string, std::string> getFileInfoFromPath(const std::string &path);
    std::string getFileFromPath(const std::string &path);
    std::string getFileDirFromPath(const std::string &path);
    std::string getRCCDir();
    std::string getDefaultDir();
    std::string getAbsolutePath(const std::string& relPath, const std::string &dir_path = "");
    bool isValidPath(const std::string &path);
    bool isAbsolutePath(const std::string &path);
    std::string getEscapedPathFormatString(const std::string& path);
    void setProgramEnvDir(const std::string &dir_path);
    void appendProgramWorkingDir(const std::string &path);
    bool checkPathEqual(const std::string &path1, const std::string &path2, const bool &recursion = true);
    std::string processRCCPath(const std::string &path);

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
    std::string readFile(const std::string& path);
    std::vector<std::string> readFileToLines(const std::string &path);
    bool writeFile(const std::string &path, const std::string &content);
    bool appendFile(const std::string &path, const std::string &content);
    std::string getLineFromFile(const std::string& filePath, size_t lineNum);

    // === 位置信息 ===
    class Pos:
    public Object,
    public IRCCPosInterface {
    public:
        Pos() = default;
        ~Pos() override;
        Pos(size_t line, size_t column, size_t offset, std::string filepath);
        [[nodiscard]] size_t getLine() const;
        size_t GetLine() const override;
        [[nodiscard]] size_t getColumn() const;
        size_t GetColumn() const override;
        [[nodiscard]] size_t getOffset() const;
        size_t GetOffset() const override;
        [[nodiscard]] std::string getFilepath() const;
        const char* GetFilepath() const override;
        [[nodiscard]] std::string getFileField() const;
        const char* GetFileField() const override;
        [[nodiscard]] std::string getFilePosStr() const;
        const char* GetFilePosStr() const override;
        friend std::ostream& operator<<(std::ostream& out, const Pos& pos);
        [[nodiscard]] std::string toString() const override; // 获取位置信息详细字符串
        [[nodiscard]] std::string briefString() const override; // 获取位置信息简略字符串
        const char* ToString() const override;
        const char* BriefString() const override;
        [[nodiscard]] std::string professionalString() const override; // 获取位置信息详细字符串
        [[nodiscard]] std::string formatString(size_t indent, size_t level) const override;
        const char* ProfessionalString() const override;
        const char* FormatString(const size_t& indent, const size_t& level) const override;
        const char* ToJsonString() const override;
        const char* HasCode() override;
        const char* UniqueId() override;
        const IRCCObjectInterface* CopySelf() override;
        void serialize(std::ostream& out, const SerializationProfile &profile) const;
        void deserialize(std::istream& in, const SerializationProfile &profile);
        void setLine(size_t line_);
        void SetLine(const size_t& line_) override;
        void setColumn(size_t column_);
        void SetColumn(const size_t& column_) override;
        void setOffset(size_t offset_);
        void SetOffSet(const size_t& offset_) override;
        void setFilepath(const std::string &filepath_);
        void SetFilepath(const char* filepath_) override;
        bool compare(const Pos &other) const;
        bool Compare(const IRCCPosInterface* other) override;
        const Pos* TransformToPI() const override;
    protected:
        size_t line{0};
        size_t column{0};
        size_t offset{0};
        std::string filepath{};
    };

    const Pos &getUnknownPos();

    Pos* getUnknownPosP();

    class  RangerPos final : Pos {
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
    struct Arg final : Object {
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
        ArgType type{};
        std::string value;
    };

    // === 命令行参数解析器 ===
    class ProgArgParser final : Object {
    public:
        enum class ProgArgType
        {
            FLAG,
            OPTION,
            FLAG_ALIAS
        };

        enum class CheckDir {
            UniDir,
            BiDir,
            Unidirectional = UniDir,
            Bidirectional = BiDir
        };

        ProgArgParser& addMutuallyExclusive(const std::vector<std::string>& options,
                                            const std::string& target, CheckDir direction = CheckDir::Bidirectional);
        ProgArgParser& addMutuallyExclusive(const std::vector<std::string>& options,
                                            const std::vector<std::string>& targets,
                                            CheckDir direction = CheckDir::Bidirectional);
        ProgArgParser& addDependent(const std::vector<std::string>& options,
                                    const std::string& target, CheckDir direction = CheckDir::Bidirectional);
        ProgArgParser& addDependent(const std::vector<std::string>& options,
                                    const std::vector<std::string>& targets,
                                    CheckDir direction = CheckDir::Bidirectional);
        ProgArgParser& addMutuallyExclusiveGroup(const std::vector<std::string>& options,
                                                 CheckDir direction = CheckDir::Bidirectional);
        ProgArgParser& addDependentGroup(const std::vector<std::string>& options,
                                         CheckDir direction = CheckDir::Bidirectional);
        ProgArgParser& addMutuallyExclusive(const std::string& opt1, const std::string& opt2,
                                            CheckDir direction = CheckDir::UniDir);
        ProgArgParser& addMutuallyExclusive(const std::string& opt1, const std::vector<std::string>& opt2,
                                            CheckDir direction = CheckDir::UniDir);
        ProgArgParser& addDependent(const std::string& opt1, const std::string& opt2,
                                    CheckDir direction = CheckDir::UniDir);
        ProgArgParser& addFlag(const std::string& name, bool* var, bool defaultValue, [[maybe_unused]] bool whenPresent,
                               const std::string& description = "", const std::vector<std::string>& aliases = {});
        template<typename T>
        ProgArgParser& addOption(const std::string& name, T* var, const T& defaultValue,
                                 const std::string& description = "", const std::vector<std::string>& aliases = {}) {
            *var = defaultValue;
            setupOptionSetter(name, var, aliases, description);
            return *this;
        }
        void parse(int argc, char* argv[]);
        [[nodiscard]] std::string getOptionDescription(const std::string& name) const;
        [[nodiscard]] std::string getHelpString(size_t lineWidth = 80, size_t optionIndent = 2, size_t descriptionIndent = 6) const;

    private:
        class FlagInfo final : Object {
        public:
            std::string name;
            bool* var;
            bool whenPresent;
            std::vector<std::string> aliases; // 存储别名
            std::string description;         // 存储描述
            FlagInfo(std::string name, bool* var, bool whenPresent, std::vector<std::string> aliases, std::string description);
            [[nodiscard]] std::string toString() const override;
        };

        class OptionInfo final : Object {
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
        std::pair<ProgArgType, std::string> parseArgTypeAndNamePart(const std::string &arg);
        static std::string parseOptionNamePartFromArg(const std::string &arg);
        void validateArgument(const ProgArgType &type, const std::string &arg, const std::string& name_part);
        [[nodiscard]] std::string getOptionMainName(const std::string& name) const;

        class MutuallyExclusiveRule final : Object {
        public:
            std::string opt1;
            std::string opt2;
            CheckDir direction;
            MutuallyExclusiveRule(std::string opt1, std::string opt2, CheckDir direction);
        };
        std::vector<MutuallyExclusiveRule> mutuallyExclusive_;

        class DependentRule final : Object {
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
            } else if constexpr (std::is_same_v<T, SerializationProfile>)
            {
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
            } else if constexpr (std::is_same_v<T, OutputFormat>) {
                options_.push_back({name, [var](const std::string& value)
                {
                    if (value == "txt" || value == "TXT" || value == "t" || value == "T")
                    {
                        *var = OutputFormat::TXT;
                    } else if (value == "json" || value == "JSON" || value == "j" || value == "J")
                    {
                        *var = OutputFormat::JSON;
                    } else throw std::invalid_argument("Invalid OutputMode value");
                }, aliases, description});
            } else {
                static_assert(!std::is_same_v<T, T>, "Unsupported type");
            }
        }

        void handleOption(const std::string& name_part, int argc, char* argv[], int& i);
    };

    // === 数值处理 ===
    struct Number final : Object {
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

    // === 时间处理 ===
    std::string getCurrentTime(TimeFormat format = TimeFormat::ISO);

    // === Utils ===
    template<typename T>
    size_t vectorLastIndex(const std::vector<T> &vec) {
        return vec.empty() ? -1 : vec.size() - 1;
    }

    bool isDeveloperModel();

    void setDeveloperModel(const bool &model = true);

    void pass(const std::string &annotation="");

    void passes(const std::vector<std::string> &annotations);

    void interruptWhen(const bool &condition = false, void (*handler) () = nullptr);

    void interruptError(const std::function<void()> &detectCode, void (*handler) () = nullptr);

    std::string generateUniqueId(const std::string& str);

    // FNV-1a 哈希算法实现
    uint64_t hashToCode(const std::string& str);
    std::string hashToStr(const std::string& str);

    // === System ===
    std::string getSystemUserName();
} // utils

#endif //RCC_RA_UTILS_H
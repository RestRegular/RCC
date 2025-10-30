//
// Created by RestRegular on 2025/6/30.
//

#ifndef RJSON_H
#define RJSON_H

#include <memory>

#include "../rcc_utils.h"
#include "../../../declarations/lib/RJson/RJson_dec.h"

namespace rjson {
    using namespace utils;

    class RJNull final : public Object {
    public:
        RJNull() = default;
        [[nodiscard]] std::string toString() const override;
        [[nodiscard]] std::string toJsonString() const override;
    };

    class RJKey : public Object {
        std::string key;
    public:
        RJKey() = default;
        explicit RJKey(std::string key);
        [[nodiscard]] std::string getKey() const;

        [[nodiscard]] std::string toString() const override;

        [[nodiscard]] std::string toJsonString() const override;
    };

    class RJValue : public Object {
    protected:
        struct Value final : Object {
            RJType valueType{};
            std::string stringValue;
            Number numberValue{};
            bool boolValue{};
            RJNull nullValue{};
            std::vector<std::shared_ptr<RJValue>> listValue;
            std::vector<std::shared_ptr<RJPair>> objectValue;
            Value() = default;
            explicit Value(const RJType &type);
            explicit Value(const int &value);
            explicit Value(const double &value);
            explicit Value(Number value);
            explicit Value(std::string value);
            explicit Value(const bool &value);
            explicit Value(RJNull value);
            explicit Value(const std::vector<std::shared_ptr<RJValue>> &value);
            explicit Value(const std::vector<std::shared_ptr<RJPair>> &value);
            ~Value() override = default;
            [[nodiscard]] std::string toString() const override;
            [[nodiscard]] std::string toJsonString() const override;
        };
        std::string strValue;
        Value rjValue;
        [[nodiscard]] static Value parseValueString(const RJType &type, const std::string &value) ;
    public:
        RJValue();
        // 传入字符串和类型的构造函数需要解析传入的类型
        explicit RJValue(const RJType &type);
        RJValue(const RJType &type, const std::string& value);
        // 构造函数
        RJValue(const RJValue &value_);
        explicit RJValue(std::string value);
        explicit RJValue(const int &value);
        explicit RJValue(const double &value);
        explicit RJValue(const bool &value);
        explicit RJValue(const RJNull &value);
        explicit RJValue(const std::vector<std::shared_ptr<RJValue>> &value);
        explicit RJValue(const std::vector<std::shared_ptr<RJPair>> &value);
        [[nodiscard]] static RJType parseValueType(const std::string &value);
        [[nodiscard]] std::string toString() const override;
        [[nodiscard]] std::string toJsonString() const override;
        void add(const std::shared_ptr<RJValue> &value);
        [[nodiscard]] RJType getType() const;
        [[nodiscard]] Value _getValue() const;
        [[nodiscard]] std::shared_ptr<RJValue> getAtIndex(const int &index) const;
        [[nodiscard]] std::shared_ptr<RJValue> getAtKey(const std::string &key) const;
        [[nodiscard]] std::string getStringValue() const;
        [[nodiscard]] int getIntValue() const;
        [[nodiscard]] double getDoubleValue() const;
        [[nodiscard]] std::vector<std::shared_ptr<RJValue>> getListValue() const;
        [[nodiscard]] std::vector<std::shared_ptr<RJPair>> getObjectValue() const;
        friend void operator<<(std::ostream &os, const RJValue &value);
        friend void operator<<(std::ostream &os, const std::shared_ptr<RJValue> &value);
        [[nodiscard]] std::string formatString(size_t indent, size_t level) const override;
    };

    class RJPair final : public Object {
        RJKey key;
        std::shared_ptr<RJValue> value;
    public:
        explicit RJPair(const std::string &rawString);
        RJPair(std::string key, const RJValue& value_);
        RJPair(std::string key, const std::string& value_);
        RJPair(std::string key, const int & value_);
        RJPair(std::string key, const double& value_);
        RJPair(std::string key, const bool& bool_sign, const bool &bool_value);

        [[nodiscard]] std::string formatString(size_t indent, size_t level) const override;

        [[nodiscard]] std::string toString() const override;

        [[nodiscard]] std::string toJsonString() const override;

        [[nodiscard]] std::string getKey() const {
            return key.getKey();
        }

        [[nodiscard]] std::shared_ptr<RJValue> getValue() const {
            return value;
        }
    };

    class RJList final : public RJValue {
        using ValueList = std::vector<std::shared_ptr<RJValue>>;
    public:
        RJList();
        explicit RJList(const std::string &rawString);
        explicit RJList(const std::vector<std::shared_ptr<RJValue>>& values);
        [[nodiscard]] static ValueList parseRJList(const std::string &rawString);
    };

    class RJObject final : public RJValue {
        using PairList = std::vector<std::shared_ptr<RJPair>>;
    public:
        RJObject();
        explicit RJObject(const std::vector<std::shared_ptr<RJPair>>& pairs);
        explicit RJObject(const std::string &rawString);
        static PairList parseRJObject(const std::string & string);
    };

    namespace rj {
        RJValue strRJV(const std::string &value);
        RJValue numRJV(int value);
        RJValue numRJV(double value);
        RJValue boolRJV(bool value);
        RJValue nullRJV();
        RJValue listRJV(const std::vector<RJValue> &value);
        RJValue objectRJV(const std::vector<RJPair> &value);

        class RJsonParser final : public Object {
            std::string rawString{};
            std::shared_ptr<RJValue> parsedValue{};

        public:
            std::pair<RJType, std::string> _collectRangerString(size_t &index);

            static std::unordered_map<char, std::pair<RJType, char> > rangerMap;

            RJsonParser();

            explicit RJsonParser(std::string jsonString);

            void parse();

            void parse(const std::string &filepath);

            [[nodiscard]] std::string getRawString() const;

            [[nodiscard]] std::string getJsonString() const;

            [[nodiscard]] std::shared_ptr<RJValue> getAtIndex(int index) const;

            [[nodiscard]] std::shared_ptr<RJValue> getAtKey(const std::string &key) const;

            [[nodiscard]] std::string formatString(size_t indent, size_t level) const override;
        };

        class RJsonBuilder final : public Object {
            RJType type;
            std::shared_ptr<RJValue> value;

            RJsonBuilder &add(const std::shared_ptr<RJValue> &value_);

        public:
            explicit RJsonBuilder(const RJType &type);

            explicit RJsonBuilder(const std::vector<RJValue> &values);

            explicit RJsonBuilder(std::unordered_map<std::string, RJValue> pairs);

            [[nodiscard]] RJValue build() const;

            // 构建函数
            // 对象
            RJsonBuilder &insertPair(const RJPair &pair);

            RJsonBuilder &insertString(const std::string &key, const std::string &value_);

            RJsonBuilder &insertNumber(const std::string &key, const int &value_);

            RJsonBuilder &insertNumber(const std::string &key, const double &value_);

            RJsonBuilder &insertBool(const std::string &key, const bool &value_);

            RJsonBuilder &insertList(const std::string &key, const std::vector<RJValue> &value_);

            RJsonBuilder &insertList(const std::string &key, const std::vector<std::shared_ptr<RJValue>> &value_);

            RJsonBuilder &insertObject(const std::string &key, const std::vector<RJPair> &value_);

            RJsonBuilder &insertObject(const std::string &key, const std::vector<std::shared_ptr<RJPair>> &value_);

            RJsonBuilder &insertNull(const std::string &key);

            RJsonBuilder &insertRJValue(const std::string &key, const RJValue &value_);

            // 列表
            RJsonBuilder &appendPair(const std::pair<std::string, std::string> &pair);

            RJsonBuilder &append(const std::string &value_);

            RJsonBuilder &append(const int &value_);

            RJsonBuilder &append(const double &value_);

            RJsonBuilder &append(const bool &value_);

            RJsonBuilder &append(const RJValue &value_);

            RJsonBuilder &append(const std::vector<RJValue> &value_);

            RJsonBuilder &appendList(const std::vector<RJValue> &value_);

            RJsonBuilder &appendList(const std::vector<std::shared_ptr<RJValue>> &value_);

            RJsonBuilder &appendObject(const std::vector<RJPair> &value_);

            RJsonBuilder &appendObject(const std::vector<std::shared_ptr<RJPair>> &value_);

            RJsonBuilder &appendNull();

            RJsonBuilder &appendRJValue(const RJValue &value_);

            [[nodiscard]] std::string toString() const override;

            [[nodiscard]] std::string toJsonString() const override;

            [[nodiscard]] std::string formatString(size_t indent) const;

            [[nodiscard]] std::string formatString(size_t indent, size_t level) const override;
        };
    }

    enum class RJType {
        RJ_UNDEFINED,
        RJ_NULL,
        RJ_BOOL,
        RJ_NUMBER,
        RJ_STRING,
        RJ_LIST,
        RJ_OBJECT
    };

}


#endif //RJSON_H

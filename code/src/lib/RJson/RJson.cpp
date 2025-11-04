//
// Created by RestRegular on 2025/6/30.
//


#include <sstream>

#include "../../../include/lib/RJson/RJson.h"
#include "../../../include/lib/RJson/RJson_error.h"
#include "../../../include/lib/rcc_utils.h"

namespace rjson {
    using namespace error;

    RJKey::RJKey(std::string key): key(std::move(key)) {}

    std::string RJKey::getKey() const {
        return key;
    }

    std::string RJKey::toString() const {
        return "[RJsonKey: \"" + key + "\"]";
    }

    std::string RJKey::toJsonString() const {
        return "\"" + key + "\"";
    }

    std::string getRJTypeName(RJType type) {
        switch (type) {
            case RJType::RJ_NULL: return "RJ_NULL";
            case RJType::RJ_BOOL: return "RJ_BOOL";
            case RJType::RJ_NUMBER: return "RJ_NUMBER";
            case RJType::RJ_STRING: return "RJ_STRING";
            case RJType::RJ_LIST: return "RJ_LIST";
            case RJType::RJ_OBJECT: return "RJ_OBJECT";
            case RJType::RJ_UNDEFINED:
            default: return "RJ_UNDEFINED";
        }
    }

    std::string RJNull::toString() const {
        return "[RJNull]";
    }

    std::string RJNull::toJsonString() const {
        return "null";
    }

    RJValue::Value::Value(const RJType &type)
        : valueType(type){}

    RJValue::Value::Value(const int &value)
        : valueType(RJType::RJ_NUMBER), numberValue(value) {}

    RJValue::Value::Value(std::string value)
        : valueType(RJType::RJ_STRING), stringValue(std::move(value)){}

    RJValue::Value::Value(const double &value)
        : valueType(RJType::RJ_NUMBER), numberValue(value) {}

    RJValue::Value::Value(Number value):
    valueType(RJType::RJ_NUMBER),
    numberValue(std::move(value)){}

    RJValue::Value::Value(const bool &value)
        : valueType(RJType::RJ_BOOL), boolValue(value){}

    RJValue::Value::Value(RJNull value)
        : valueType(RJType::RJ_NULL), nullValue(std::move(value)) {}

    RJValue::Value::Value(const std::vector<std::shared_ptr<RJValue>> &value)
        : valueType(RJType::RJ_LIST), listValue(value) {}

    RJValue::Value::Value(const std::vector<std::shared_ptr<RJPair>> &value)
        : valueType(RJType::RJ_OBJECT), objectValue(value) {}

    std::string RJValue::Value::toString() const {
        return "[" + getRJTypeName(valueType) + ": " + toJsonString() + "]";
    }

    std::string RJValue::Value::toJsonString() const {
        switch (valueType) {
            case RJType::RJ_NULL: return "null";
            case RJType::RJ_BOOL: return boolValue ? "true" : "false";
            case RJType::RJ_NUMBER: return numberValue.toJsonString();
            case RJType::RJ_STRING: return "\"" + stringValue + "\"";
            case RJType::RJ_LIST: {
                std::stringstream ss;
                size_t index = 0;
                for (const auto &item: listValue) {
                    ss << item->toJsonString();
                    if (index != listValue.size() - 1) {
                        ss << ", ";
                    }
                    index++;
                }
                return "[" + ss.str() + "]";
            }
            case RJType::RJ_OBJECT: {
                std::stringstream ss;
                size_t index = 0;
                for (const auto &item: objectValue) {
                    ss << item->toJsonString();
                    if (index != objectValue.size() - 1) {
                        ss << ", ";
                    }
                    index++;
                }
                return "{" + ss.str() + "}";
            }
            case RJType::RJ_UNDEFINED:
            default: return "undefined";
        }
    }

    RJType RJValue::parseValueType(const std::string &value) {
        if (isValidNumber(value)) {
            return RJType::RJ_NUMBER;
        }
        if (value == "true" || value == "false") {
            return RJType::RJ_BOOL;
        }
        if (value == "null") {
            return RJType::RJ_NULL;
        }
        if (value[0] == '[') {
            return RJType::RJ_LIST;
        }
        if (value[0] == '{') {
            return RJType::RJ_OBJECT;
        }
        if (StringManager::isStrictValidStringFormat(value)) {
            return RJType::RJ_STRING;
        }
        return RJType::RJ_UNDEFINED;
    }

    RJValue RJValue::getObjectRJValue(const std::string& objectRawString, const bool& isRange)
    {
        return RJValue(RJObject::parseRJObject(objectRawString, isRange));
    }

    RJValue::Value RJValue::parseValueString(const RJType &type, const std::string &value) {
        switch (type) {
            case RJType::RJ_NULL: return Value(RJNull());
            case RJType::RJ_BOOL: return Value(value == "true");
            case RJType::RJ_NUMBER: return Value(stringToNumber(value));
            case RJType::RJ_STRING: return Value(value);
            case RJType::RJ_LIST: return Value(RJList::parseRJList(value));
            case RJType::RJ_OBJECT: return Value(RJObject::parseRJObject(value));
            case RJType::RJ_UNDEFINED:
            default: throw RJsonError(error::ErrorType::PARSER_ERROR, "unknown", "",
                                      {"Invalid RJson Value type"},
                                      {"Check the value format and ensure it matches one of the valid JSON types"});
        }
    }

    RJValue::RJValue()
        : rjValue(RJType::RJ_NULL) {}

    RJValue::RJValue(const RJType &type)
        : rjValue(type) {}

    RJValue::RJValue(const RJType &type, const std::string& value)
        : strValue(value), rjValue(parseValueString(type, value)){}

    RJValue::RJValue(const RJValue &value_)
        : strValue(value_.strValue), rjValue(value_.rjValue) {}

    RJValue::RJValue(std::string value)
        : strValue(std::move(value)), rjValue(strValue) {}

    RJValue::RJValue(const int &value)
        : strValue(std::to_string(value)), rjValue(value){}

    RJValue::RJValue(const double &value)
        : strValue(std::to_string(value)), rjValue(value){}

    RJValue::RJValue(const bool &value)
        : strValue(value ? "true" : "false"), rjValue(value){}

    RJValue::RJValue(const RJNull &value)
        : strValue("null"), rjValue(value){}

    RJValue::RJValue(const std::vector<std::shared_ptr<RJValue>> &value)
        : rjValue(value) {}

    RJValue::RJValue(const std::vector<std::shared_ptr<RJPair>> &value)
        : rjValue(value){}

    std::string RJValue::toString() const {
        return "[RJsonValue: " + rjValue.toString() + "]";
    }

    std::string RJValue::toJsonString() const {
        return rjValue.toJsonString();
    }

    void RJValue::add(const std::shared_ptr<RJValue> &value) {
        if (rjValue.valueType != RJType::RJ_LIST && rjValue.valueType != RJType::RJ_OBJECT) {
            throw RJsonError(ErrorType::PARSER_ERROR, "unknown", "",
                    {"Cannot add value to non-container type"},
                    {"Ensure the target is a list or object before adding values"});
        }
        if (rjValue.valueType == RJType::RJ_LIST) {
            rjValue.listValue.push_back(value);
        } else {
            if (value->getType() != RJType::RJ_OBJECT) {
                throw RJsonError(ErrorType::PARSER_ERROR, "unknown", "",
                    {"Cannot merge non-object value into object"},
                    {"Ensure the value being added is a valid JSON object"});
            }
            for (const auto &item: value->_getValue().objectValue) {
                rjValue.objectValue.emplace_back(item);
            }
        }
    }

    RJType RJValue::getType() const {
        return rjValue.valueType;
    }

    RJValue::Value RJValue::_getValue() const {
        return rjValue;
    }

    std::shared_ptr<RJValue> RJValue::getAtIndex(const int &index) const {
        if (rjValue.valueType == RJType::RJ_LIST) {
            if (index >= rjValue.listValue.size() || index < 0) {
                throw RJsonError(ErrorType::PARSER_ERROR, "unknown", "",
                    {"Index out of range: " + std::to_string(index)},
                    {"Check the list size and ensure index is within bounds"});
            }
            return rjValue.listValue[index];
        }
        throw RJsonError(ErrorType::PARSER_ERROR, "unknown", "",
                {"Value is not a list"},
                {"Check the type before accessing as list"});
    }

    std::shared_ptr<RJValue> RJValue::getAtKey(const std::string &key) const {
        if (rjValue.valueType == RJType::RJ_OBJECT) {
            for (auto &pair: rjValue.objectValue) {
                if (pair->getKey() == key) {
                    return pair->getValue();
                }
            }
            throw RJsonError(ErrorType::PARSER_ERROR, "unknown", "",
                {"Key not found: \"" + StringManager::escape(key) + "\""},
                {"Check the key exists in the object"});
        }
        throw RJsonError(ErrorType::PARSER_ERROR, "unknown", "",
                {"Value is not an object"},
                {"Check the type before accessing as object"});
    }

    std::string RJValue::getStringValue() const {
        if (rjValue.valueType == RJType::RJ_STRING) {
            return StringManager::unescape(toJsonString());
        }
        throw RJsonError(ErrorType::PARSER_ERROR, "unknown", "",
                {"Value is not a string"},
                {"Check the type before accessing as string"});
    }

    int RJValue::getIntValue() const {
        if (rjValue.valueType == RJType::RJ_NUMBER && rjValue.numberValue.type == NumType::int_type) {
            return rjValue.numberValue.int_value;
        }
        throw RJsonError(ErrorType::PARSER_ERROR, "unknown", "",
                {"Value is not an integer number"},
                {"Check the type and number format before accessing as integer"});
    }

    double RJValue::getDoubleValue() const {
        if (rjValue.valueType == RJType::RJ_NUMBER && rjValue.numberValue.type == NumType::double_type) {
            return rjValue.numberValue.double_value;
        }
        throw RJsonError(ErrorType::PARSER_ERROR, "unknown", "",
                {"Value is not a double number"},
                {"Check the type and number format before accessing as double"});
    }

    std::vector<std::shared_ptr<RJValue>> RJValue::getListValue() const {
        if (rjValue.valueType == RJType::RJ_LIST) {
            return rjValue.listValue;
        }
        throw RJsonError(ErrorType::PARSER_ERROR, "unknown", "",
                {"Value is not a list"},
                {"Check the type before accessing as list"});
    }

    std::vector<std::shared_ptr<RJPair>> RJValue::getObjectValue() const {
        if (rjValue.valueType == RJType::RJ_OBJECT) {
            return rjValue.objectValue;
        }
        throw RJsonError(ErrorType::PARSER_ERROR, "unknown", "",
                {"Value is not an object"},
                {"Check the type before accessing as object"});
    }

    std::string RJValue::formatString(const size_t indent, const size_t level) const {
        std::stringstream ss;
        switch (rjValue.valueType) {
            case RJType::RJ_LIST: {
                ss << "[";
                if (!rjValue.listValue.empty()) {
                    ss << "\n";
                    size_t index = 0;
                    for (const auto &item: rjValue.listValue) {
                        ss << spaceString(indent * (level + 1)) << item->formatString(indent, level + 1);
                        if (index != rjValue.listValue.size() - 1) {
                            ss << ",";
                        }
                        ss << "\n";
                        index++;
                    }
                    ss << spaceString(indent * level);
                }
                ss << "]";
            } break;
            case RJType::RJ_OBJECT: {
                ss << "{";
                if (!rjValue.objectValue.empty()){
                    ss << "\n";
                    size_t i = 0;
                    for (auto &pair: rjValue.objectValue) {
                        ss << spaceString(indent * (level + 1)) << pair->formatString(indent, level + 1);
                        if (i != rjValue.objectValue.size() - 1) {
                            ss << ",";
                        }
                        ss << "\n";
                        i++;
                    }
                    ss << spaceString(indent * level);
                }
                ss << "}";
            } break;
            case RJType::RJ_STRING:
            case RJType::RJ_NUMBER:
            case RJType::RJ_BOOL:
            case RJType::RJ_NULL:
            case RJType::RJ_UNDEFINED:
            default: ss << toJsonString(); break;
        }
        return ss.str();
    }

    namespace rj {
        RJValue strRJV(const std::string &value) {
            return RJValue(value);
        }

        RJValue numRJV(const int value) {
            return RJValue(value);
        }

        RJValue numRJV(const double value) {
            return RJValue(value);
        }

        RJValue boolRJV(const bool value) {
            return RJValue(value);
        }

        RJValue nullRJV() {
            return RJValue(RJNull());
        }

        RJValue listRJV(const std::vector<RJValue> &value) {
            std::vector<std::shared_ptr<RJValue>> value_list;
            for (auto &item: value) {
                value_list.push_back(std::make_shared<RJValue>(item));
            }
            return RJValue(value_list);
        }

        RJValue objectRJV(const std::vector<RJPair> &value) {
            std::vector<std::shared_ptr<RJPair>> value_list;
            for (auto &item: value) {
                value_list.push_back(std::make_shared<RJPair>(item));
            }
            return RJValue(value_list);
        }
    }

    RJPair::RJPair(const std::string &rawString) {
        rj::RJsonParser parser(rawString);
        std::stringstream ss;
        bool isKey = true;
        bool hasKey = false;
        bool hasValue = false;
        for (size_t i = 0; i < rawString.size(); i++) {
            const char &c = rawString[i];
            if (StringManager::isSpace(c)) continue;
            if (rj::RJsonParser::rangerMap.contains(c)) {
                const auto &[type, ranger] = parser._collectRangerString(i);
                if (isKey) {
                    if (type != RJType::RJ_STRING) {
                        throw RJsonError(ErrorType::SYNTAX_ERROR, "unknown", rawString,
                {"Invalid JSON key format"},
                {"Ensure keys are properly quoted strings"});
                    }
                    key = RJKey(ranger);
                    hasKey = true;
                } else if (hasKey && !hasValue) {
                    value = std::make_shared<RJValue>(type, ranger);
                    hasValue = true;
                } else {
                    throw RJsonError(ErrorType::SYNTAX_ERROR, "unknown", rawString,
                        {"Invalid JSON value format"},
                        {"Check the value syntax according to JSON specification"});
                }
                continue;
            }
            if (c == ':') {
                isKey = false;
                continue;
            }
            if (c == ',') {
                if (!hasKey || !hasValue) throw RJsonError(ErrorType::SYNTAX_ERROR, "unknown", rawString,
                {"Invalid JSON key-value pair format"},
                {"Ensure pairs are in 'key:value' format, separated by commas"});
            }
            ss << c;
        }
        if (!hasKey) throw RJsonError(ErrorType::SYNTAX_ERROR, "unknown", rawString,
                {"Invalid JSON key-value pair format"},
                {"Ensure pairs are in 'key:value' format, separated by commas"});
        if (!hasValue) {
            if (ss.str().empty()) throw RJsonError(ErrorType::SYNTAX_ERROR, "unknown", rawString,
                {"Invalid JSON key-value pair format"},
                {"Ensure pairs are in 'key:value' format, separated by commas"});
            value = std::make_shared<RJValue>(RJValue::parseValueType(ss.str()), ss.str());
        }
    }

    RJPair::RJPair(std::string key, const RJValue& value_)
        : key(RJKey(std::move(key))), value(std::make_shared<RJValue>(value_)){}

    RJPair::RJPair(std::string key, const std::string &value_)
        : key(RJKey(std::move(key))), value(std::make_shared<RJValue>(StringManager::escape(value_))){}

    RJPair::RJPair(std::string key, const int &value_)
        : key(RJKey(std::move(key))), value(std::make_shared<RJValue>(value_)){}

    RJPair::RJPair(std::string key, const double &value_)
        : key(RJKey(std::move(key))), value(std::make_shared<RJValue>(value_)){}

    RJPair::RJPair(std::string key, const bool &bool_sign, const bool &bool_value)
        : key(RJKey(std::move(key))), value(std::make_shared<RJValue>(bool_value)){}

    std::string RJPair::formatString(const size_t indent, const size_t level) const {
        return key.toJsonString() + ": " + value->formatString(indent, level);
    }

    std::string RJPair::toString() const {
        return "[RJPair: "
        + key.toString() + " : "
        + value->toString() + "]";
    }

    std::string RJPair::toJsonString() const {
        return key.toJsonString() + ": " + value->toJsonString();
    }

    RJList::RJList()
        : RJValue(RJType::RJ_LIST) {}

    RJList::RJList(const std::string &rawString)
        : RJValue(parseRJList(rawString)){}

    RJList::RJList(const ValueList& values)
        : RJValue(values){}

    RJList::ValueList RJList::parseRJList(const std::string &rawString) {
        rj::RJsonParser parser(rawString);
        std::stringstream ss;
        ValueList values;
        bool hasPushedValue = false;
        for (size_t i = 0; i < rawString.size(); i++) {
            const char &c = rawString[i];
            if (StringManager::isSpace(c)) continue;
            if (rj::RJsonParser::rangerMap.contains(c)) {
                const auto &[type, ranger] = parser._collectRangerString(i);
                values.push_back(std::make_shared<RJValue>(type, ranger));
                hasPushedValue = true;
                continue;
            }
            if (c == ',') {
                if (hasPushedValue) {
                    hasPushedValue = false;
                    ss.str("");
                    continue;
                }
                if (!ss.str().empty()) {
                    values.push_back(std::make_shared<RJValue>(
                        parseValueType(ss.str()), ss.str()));
                    ss.str("");
                    continue;
                }
                throw RJsonError(ErrorType::SYNTAX_ERROR, "unknown", rawString,
                {"Invalid JSON list format"},
                {"Ensure list elements are properly separated by commas"});
            }
            ss << c;
        }
        if (!hasPushedValue && !ss.str().empty()) {
            values.push_back(std::make_shared<RJValue>(
                        parseValueType(ss.str()), ss.str()));
        }
        return values;
    }

    RJObject::RJObject()
        : RJValue(RJType::RJ_OBJECT) {}

    RJObject::RJObject(const PairList& pairs)
        : RJValue(pairs) {}

    RJObject::PairList RJObject::parseRJObject(const std::string &rawString, const bool& isRange) {
        auto objectString = rawString;
        if (isRange)
        {
            rj::RJsonParser parser (rawString);
            parser.parse();
            objectString = parser.getJsonString();
            objectString = objectString.substr(1, objectString.size() - 2);
        }
        const auto& objectInnerString = objectString;
        rj::RJsonParser parser(objectInnerString);
        std::stringstream ss;
        PairList pairs;
        for (size_t i = 0; i < objectInnerString.size(); i++) {
            const char &c = objectInnerString[i];
            if (StringManager::isSpace(c)) continue;
            if (c == ',') {
                pairs.push_back(std::make_shared<RJPair>(ss.str()));
                ss.str("");
                continue;
            }
            if (const auto &it = rj::RJsonParser::rangerMap.find(c);
                it != rj::RJsonParser::rangerMap.end())
            {
                const auto &[type, ranger] = parser._collectRangerString(i);
                ss << it->first << ranger << it->second.second;
                continue;
            }
            ss << c;
        }
        if (!ss.str().empty()) {
            pairs.push_back(std::make_shared<RJPair>(ss.str()));
        }
        return pairs;
    }

    RJObject::RJObject(const std::string &rawString)
        : RJValue(parseRJObject(rawString)){}

    std::unordered_map<char, std::pair<RJType, char>> rj::RJsonParser::rangerMap = {
        {'{', {RJType::RJ_OBJECT, '}'}},
        {'[', {RJType::RJ_LIST, ']'}},
        {'"', {RJType::RJ_STRING, '"'}}
    };

    rj::RJsonParser::RJsonParser()
        : parsedValue(nullptr){}

    void rj::RJsonParser::parse(const std::string &filepath) {
        rawString = readFile(filepath);
        parse();
    }

    rj::RJsonParser::RJsonParser(std::string jsonString)
        : rawString(std::move(jsonString)) {}

    std::string rj::RJsonParser::getRawString() const {
        return rawString;
    }

    std::pair<RJType, std::string> rj::RJsonParser::_collectRangerString(size_t &index) {
        const auto &rangerStartChar = rawString[index];
        const auto &it = rangerMap.find(rangerStartChar);
        if (it == rangerMap.end()) {
            return {RJType::RJ_UNDEFINED, ""};
        }
        const auto &[rangerType, rangerEndChar] = it->second;
        std::stringstream ss;
        index ++;
        while (index < rawString.size()) {
            if (const auto &c = rawString[index]; c == '\\') {
                ss << c << rawString[++index];
            } else {
                if (const auto &tem_it = rangerMap.find(c);
                    tem_it != rangerMap.end()) {
                    if (c == '"' && rangerEndChar == '"') {
                        break;
                    }
                    const auto &[_, ranger] = _collectRangerString(index);
                    bool isValid = false;
                    while (index < rawString.size()) {
                        if (rawString[index] == tem_it->second.second) {
                            isValid = true;
                            break;
                        }
                        index ++;
                    }
                    if (!isValid) throw RJsonError(ErrorType::SYNTAX_ERROR, "unknown", rawString,
                                {"Invalid JSON format",
                                "Unclosed ranger char: '" + std::string(1, tem_it->first) + "'"},
                                {"Ensure ranger scope are properly closed."});
                    ss << c << ranger << tem_it->second.second;
                    } else if (c == rangerEndChar) {
                        break;
                    } else {
                        ss << c;
                    }
            }
            index ++;
        }
        return {rangerType, ss.str()};
    }

    void rj::RJsonParser::parse() {
        bool hasValue = false;
        for (size_t i = 0; i < rawString.size(); i++) {
            const char &c = rawString[i];
            if (StringManager::isSpace(c)) continue;
            if (hasValue) throw RJsonError(ErrorType::SYNTAX_ERROR, "unknown", rawString,
                {"Invalid JSON format"},
                {"Check the JSON syntax according to specification"});
            if (const auto &it = rangerMap.find(c); it != rangerMap.end()) {
                switch (const auto &[rangerType, ranger] = _collectRangerString(i);
                    rangerType) {
                        case RJType::RJ_LIST: {
                            parsedValue = std::make_shared<RJList>(ranger);
                        } break;
                        case RJType::RJ_OBJECT: {
                            parsedValue = std::make_shared<RJObject>(ranger);
                        } break;
                        default: throw RJsonError(ErrorType::SYNTAX_ERROR, "unknown", rawString,
                        {"Invalid JSON format"},
                        {"Check the JSON syntax according to specification"});
                    }
                bool isValid = false;
                while (i < rawString.size()) {
                    if (!StringManager::isSpace(rawString[i])) {
                        isValid = rawString[i] == it->second.second;
                        break;
                    }
                    i ++;
                }
                hasValue = isValid;
            }
        }
        if (!hasValue) throw RJsonError(ErrorType::SYNTAX_ERROR, "unknown", rawString,
                {"Invalid JSON format"},
                {"Check the JSON syntax according to specification"});
    }

    std::string rj::RJsonParser::getJsonString() const {
        return parsedValue->toJsonString();
    }

    std::shared_ptr<RJValue> rj::RJsonParser::getAtIndex(const int index) const {
        if (parsedValue->getType() == RJType::RJ_OBJECT) {
            throw RJsonError(ErrorType::PARSER_ERROR, "unknown", "",
                {"Cannot access by index - not a list"},
                {"Check the type before accessing by index"});
        }
        return parsedValue->_getValue().listValue[index];
    }

    std::shared_ptr<RJValue> rj::RJsonParser::getAtKey(const std::string& key) const {
        if (parsedValue->getType() == RJType::RJ_LIST) {
            throw RJsonError(ErrorType::PARSER_ERROR, "unknown", "",
                {"Cannot access by key - not an object"},
                {"Check the type before accessing by key"});
        }
        for (const auto &pair: parsedValue->_getValue().objectValue) {
            if (pair->getKey() == key) {
                return pair->getValue();
            }
        }
        throw RJsonError(ErrorType::PARSER_ERROR, "unknown", "",
                {"Key not found: \"" + StringManager::escape(key) + "\""},
                {"Check the key exists in the object"});
    }

    std::string rj::RJsonParser::formatString(const size_t indent, const size_t level) const {
        return parsedValue->formatString(indent, level);
    }

    std::shared_ptr<RJValue> rj::RJsonParser::getParsedValue() const
    {
        return parsedValue;
    }

    void throwTypeError() {
        throw RJsonError(ErrorType::PARSER_ERROR, "unknown", "",
                {"Invalid operation for current builder type"},
                {"Check the builder type before performing this operation"});
    }

    rj::RJsonBuilder::RJsonBuilder(const RJType &type)
        : type(type) {
        switch (type) {
            case RJType::RJ_LIST: value = std::make_shared<RJList>(); break;
            case RJType::RJ_OBJECT: value = std::make_shared<RJObject>(); break;
            default: throw RJsonError(ErrorType::PARSER_ERROR, "unknown", "",
                {"Invalid builder type"},
                {"Builder must be initialized with either LIST or OBJECT type"});
        }
    }

    rj::RJsonBuilder::RJsonBuilder(const std::vector<RJValue> &values)
        : type(RJType::RJ_LIST) {
        for (auto &value_: values) {
            append(value_);
        }
    }

    rj::RJsonBuilder::RJsonBuilder(std::unordered_map<std::string, RJValue> pairs)
        : type(RJType::RJ_OBJECT) {
        for (auto &[fst, snd]: pairs) {
            insertRJValue(fst, snd);
        }
    }

    rj::RJsonBuilder::RJsonBuilder(const std::shared_ptr<RJValue>& value)
        : type(value->getType()), value(value) {}

    RJValue rj::RJsonBuilder::build() const {
        return *value;
    }

    rj::RJsonBuilder& rj::RJsonBuilder::insertPair(const RJPair& pair) {
        if (type != RJType::RJ_OBJECT) {
            throwTypeError();
        }
        std::vector pairs = {std::make_shared<RJPair>(pair)};
        value->add(std::make_shared<RJObject>(pairs));
        return *this;
    }

    rj::RJsonBuilder& rj::RJsonBuilder::insertString(const std::string &key, const std::string &value_) {
        if (type != RJType::RJ_OBJECT) {
            throwTypeError();
        }
        std::vector pairs = {std::make_shared<RJPair>(key, RJValue{StringManager::escape(value_)})};
        value->add(std::make_shared<RJObject>(pairs));
        return *this;
    }

    rj::RJsonBuilder& rj::RJsonBuilder::insertNumber(const std::string &key, const int &value_) {
        if (type != RJType::RJ_OBJECT) {
            throwTypeError();
        }
        std::vector pairs = {std::make_shared<RJPair>(key, value_)};
        value->add(std::make_shared<RJObject>(pairs));
        return *this;
    }

    rj::RJsonBuilder& rj::RJsonBuilder::insertNumber(const std::string &key, const double &value_) {
        if (type != RJType::RJ_OBJECT) {
            throwTypeError();
        }
        std::vector pairs = {std::make_shared<RJPair>(key, value_)};
        value->add(std::make_shared<RJObject>(pairs));
        return *this;
    }

    rj::RJsonBuilder& rj::RJsonBuilder::insertBool(const std::string &key, const bool &value_) {
        if (type != RJType::RJ_OBJECT) {
            throwTypeError();
        }
        std::vector pairs = {std::make_shared<RJPair>(key, value_)};
        value->add(std::make_shared<RJObject>(pairs));
        return *this;
    }

    rj::RJsonBuilder& rj::RJsonBuilder::insertList(const std::string &key, const std::vector<RJValue> &value_) {
        if (type != RJType::RJ_OBJECT) {
            throwTypeError();
        }
        value->add(std::make_shared<RJObject>(std::vector{std::make_shared<RJPair>(key, listRJV(value_))}));
        return *this;
    }

    rj::RJsonBuilder & rj::RJsonBuilder::insertList(const std::string &key,
        const std::vector<std::shared_ptr<RJValue>> &value_) {
        if (type != RJType::RJ_OBJECT) {
            throwTypeError();
        }
        add(std::make_shared<RJObject>(std::vector{std::make_shared<RJPair>(key, RJList(value_))}));
        return *this;
    }

    rj::RJsonBuilder& rj::RJsonBuilder::insertObject(const std::string &key, const std::vector<RJPair> &value_) {
        if (type != RJType::RJ_OBJECT) {
            throwTypeError();
        }
        value->add(std::make_shared<RJObject>(std::vector{std::make_shared<RJPair>(key, objectRJV(value_))}));
        return *this;
    }

    rj::RJsonBuilder & rj::RJsonBuilder::insertObject(const std::string &key,
        const std::vector<std::shared_ptr<RJPair>> &value_) {
        if (type != RJType::RJ_OBJECT) {
            throwTypeError();
        }
        value->add(std::make_shared<RJObject>(std::vector{std::make_shared<RJPair>(key, RJObject(value_))}));
        return *this;
    }

    rj::RJsonBuilder& rj::RJsonBuilder::insertNull(const std::string &key) {
        if (type != RJType::RJ_OBJECT) {
            throwTypeError();
        }
        value->add(std::make_shared<RJObject>(std::vector{std::make_shared<RJPair>(key, nullRJV())}));
        return *this;
    }

    rj::RJsonBuilder& rj::RJsonBuilder::insertRJValue(const std::string& key, const RJValue& value_) {
        if (type != RJType::RJ_OBJECT) {
            throwTypeError();
        }
        std::vector pairs = {std::make_shared<RJPair>(key, value_)};
        value->add(std::make_shared<RJObject>(pairs));
        return *this;
    }

    rj::RJsonBuilder& rj::RJsonBuilder::add(const std::shared_ptr<RJValue> &value_) {
        value->add(value_);
        return *this;
    }

    rj::RJsonBuilder& rj::RJsonBuilder::appendPair(const std::pair<std::string, std::string> &pair) {
        if (type != RJType::RJ_LIST) {
            throwTypeError();
        }
        const auto &[key, value_] = pair;
        std::vector pairs = {std::make_shared<RJPair>(key, value_)};
        value->add(std::make_shared<RJObject>(pairs));
        return *this;
    }

    rj::RJsonBuilder& rj::RJsonBuilder::append(const std::string &value_) {
        if (type != RJType::RJ_LIST) {
            throwTypeError();
        }
        value->add(std::make_shared<RJValue>(StringManager::escape(value_)));
        return *this;
    }

    rj::RJsonBuilder& rj::RJsonBuilder::append(const int &value_) {
        if (type != RJType::RJ_LIST) {
            throwTypeError();
        }
        value->add(std::make_shared<RJValue>(value_));
        return *this;
    }

    rj::RJsonBuilder& rj::RJsonBuilder::append(const double &value_) {
        if (type != RJType::RJ_LIST) {
            throwTypeError();
        }
        value->add(std::make_shared<RJValue>(value_));
        return *this;
    }

    rj::RJsonBuilder& rj::RJsonBuilder::append(const bool &value_) {
        if (type != RJType::RJ_LIST) {
            throwTypeError();
        }
        value->add(std::make_shared<RJValue>(value_));
        return *this;
    }

    rj::RJsonBuilder& rj::RJsonBuilder::append(const RJValue &value_) {
        if (type != RJType::RJ_LIST) {
            throwTypeError();
        }
        value->add(std::make_shared<RJValue>(value_));
        return *this;
    }

    rj::RJsonBuilder& rj::RJsonBuilder::append(const std::vector<RJValue> &value_) {
        if (type != RJType::RJ_LIST) {
            throwTypeError();
        }
        for (const auto &v: value_) {
            value->add(std::make_shared<RJValue>(v));
        }
        return *this;
    }

    rj::RJsonBuilder& rj::RJsonBuilder::appendList(const std::vector<RJValue> &value_) {
        if (type != RJType::RJ_LIST) {
            throwTypeError();
        }
        std::vector<std::shared_ptr<RJValue>> vs;
        for (const auto &v: value_) {
            vs.push_back(std::make_shared<RJValue>(v));
        }
        value->add(std::make_shared<RJList>(vs));
        return *this;
    }

    rj::RJsonBuilder & rj::RJsonBuilder::appendList(const std::vector<std::shared_ptr<RJValue>> &value_) {
        if (type != RJType::RJ_LIST) {
            throwTypeError();
        }
        value->add(std::make_shared<RJList>(value_));
        return *this;
    }

    rj::RJsonBuilder& rj::RJsonBuilder::appendObject(const std::vector<RJPair> &value_) {
        if (type != RJType::RJ_LIST) {
            throwTypeError();
        }
        std::vector<std::shared_ptr<RJPair>> vs;
        for (const auto &v: value_) {
            vs.emplace_back(std::make_shared<RJPair>(v));
        }
        value->add(std::make_shared<RJObject>(vs));
        return *this;
    }

    rj::RJsonBuilder & rj::RJsonBuilder::appendObject(const std::vector<std::shared_ptr<RJPair>> &value_) {
        if (type != RJType::RJ_LIST) {
            throwTypeError();
        }
        value->add(std::make_shared<RJObject>(value_));
        return *this;
    }

    rj::RJsonBuilder& rj::RJsonBuilder::appendNull() {
        if (type != RJType::RJ_LIST) {
            throwTypeError();
        }
        value->add(std::make_shared<RJValue>(RJNull()));
        return *this;
    }

    rj::RJsonBuilder & rj::RJsonBuilder::appendRJValue(const RJValue &value_) {
        if (type != RJType::RJ_LIST) {
            throwTypeError();
        }
        value->add(std::make_shared<RJValue>(value_));
        return *this;
    }

    std::string rj::RJsonBuilder::toString() const {
        return value->toString();
    }

    std::string rj::RJsonBuilder::toJsonString() const {
        return value->toJsonString();
    }

    std::string rj::RJsonBuilder::formatString(const size_t indent) const {
        return value->formatString(indent, 0);
    }

    std::string rj::RJsonBuilder::formatString(size_t indent, size_t level) const {
        return value->formatString(indent, level);
    }

    void operator<<(std::ostream &os, const RJValue &value) {
        if (value.getType() == RJType::RJ_STRING) {
            os << value.getStringValue();
        } else {
            os << value.toJsonString();
        }
    }
    void operator<<(std::ostream &os, const std::shared_ptr<RJValue> &value) {
        if (value->getType() == RJType::RJ_STRING) {
            os << value->getStringValue();
        } else {
            os << value->toJsonString();
        }
    }
}

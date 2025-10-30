//
// Created by RestRegular on 2025/7/12.
//

#include <ranges>
#include <utility>
#include <algorithm>

#include "../../../include/rcc_base.h"
#include "../../../include/components/symbol/rcc_symbol.h"

#include <memory_resource>

#include "../../../include/visitors/rcc_compile_visitor.h"

namespace symbol {
    std::string symbolTypeToString(const SymbolType &type) {
        switch (type) {
            case SymbolType::VARIABLE: return "VARIABLE";
            case SymbolType::FUNCTION: return "FUNCTION";
            case SymbolType::LABEL: return "LABEL";
            case SymbolType::PARAMETER: return "PARAMETER";
            case SymbolType::CLASS: return "CLASS";
            default: return "UNKNOWN";
        }
    }

    std::string symbolTypeToFormatString(const SymbolType &type) {
        return "[SymbolType: " + symbolTypeToString(type) + "]";
    }

    std::string labelTypeToString(const LabelType &type) {
        switch (type) {
            case LabelType::TYPE_LABEL: return "TYPE";
            case LabelType::PERMISSION_LABEL: return "PERMISSION";
            case LabelType::LIFE_CYCLE_LABEL: return "LIFE_CYCLE";
            case LabelType::RESTRICTION_LABEL: return "RESTRICTION";
            default: return "UNKNOWN";
        }
    }

    std::string builtinTypeLabelToString(const BuiltinTypeLabel& type)
    {
        switch (type)
        {
        case BuiltinTypeLabel::INT: return "int";
        case BuiltinTypeLabel::FLOAT: return "float";
        case BuiltinTypeLabel::STR: return "str";
        case BuiltinTypeLabel::BOOL: return "bool";
        case BuiltinTypeLabel::CHAR: return "char";
        case BuiltinTypeLabel::VOID_BTL: return "void";
        case BuiltinTypeLabel::NUL: return "nul";
        case BuiltinTypeLabel::ANY: return "any";
        case BuiltinTypeLabel::FUNC: return "func";
        case BuiltinTypeLabel::CLS: return "cls";
        case BuiltinTypeLabel::LIST: return "list";
        case BuiltinTypeLabel::DICT: return "dict";
        case BuiltinTypeLabel::SERIES: return "series";
        case BuiltinTypeLabel::FLAG: return "flag";
        default: return RCC_UNKNOWN_CONST;
        }
    }

    std::string permissionLabelToString(const PermissionLabel& label)
    {
        switch (label)
        {
        case PermissionLabel::PUBLIC: return "public";
        case PermissionLabel::PRIVATE: return "private";
        case PermissionLabel::PROTECTED: return "protected";
        case PermissionLabel::BUILTIN: return "builtin";
        default: return RCC_UNKNOWN_CONST;
        }
    }

    std::string objectOrientedLabelToString(const ObjectOrientedLabel& label)
    {
        switch (label)
        {
        case ObjectOrientedLabel::OVERWRITE: return "overwrite";
        case ObjectOrientedLabel::INTERFACE: return "interface";
        case ObjectOrientedLabel::VIRTUAL: return "virtual";
        default: return RCC_UNKNOWN_CONST;
        }
    }

    std::string restrictionLabelToString(const RestrictionLabel& label)
    {
        switch (label)
        {
        case RestrictionLabel::CONST_RL: return "const";
        case RestrictionLabel::QUOTE: return "quote";
        default: return RCC_UNKNOWN_CONST;
        }
    }

    std::string lifeCycleLabelToString(const LifeCycleLabel& label)
    {
        switch (label)
        {
        case LifeCycleLabel::STATIC: return "static";
        case LifeCycleLabel::GLOBAL: return "global";
        case LifeCycleLabel::ORDINARY: return "ordinary";
        default: return RCC_UNKNOWN_CONST;
        }
    }

    std::string functionTypeToString(const FunctionType& type)
    {
        switch (type)
        {
        case FunctionType::FUNCTION: return "FUNCTION";
        case FunctionType::CONSTRUCTOR: return "CONSTRUCTOR";
        case FunctionType::METHOD: return "METHOD";
        case FunctionType::ANONYMOUS: return "ANONYMOUS";
        default: return RCC_UNKNOWN_CONST;
        }
    }

    std::string functionTypeToFormatString(const FunctionType& type)
    {
        return "[FunctionType: " + functionTypeToString(type) + "]";
    }

    std::string paramTypeToString(const ParamType& type)
    {
        switch (type)
        {
        case ParamType::NO_PARAM: return "NO_PARAM";
        case ParamType::PARAM_POSITIONAL: return "PARAM_POSITIONAL";
        case ParamType::PARAM_KEYWORD: return "PARAM_KEYWORD";
        case ParamType::PARAM_VAR_LEN_POSITIONAL: return "PARAM_VAR_LEN_POSITIONAL";
        case ParamType::PARAM_VAR_LEN_KEYWORD: return "PARAM_VAR_LEN_KEYWORD";
        default: return RCC_UNKNOWN_CONST;
        }
    }

    std::string paramTypeToFormatString(const ParamType& type)
    {
        return "[ParamType: " + paramTypeToString(type) + "]";
    }

    std::unordered_map<std::string, LabelType> labelTypeMap = {
        {"int", LabelType::TYPE_LABEL},
        {"float", LabelType::TYPE_LABEL},
        {"char", LabelType::TYPE_LABEL},
        {"str", LabelType::TYPE_LABEL},
        {"bool", LabelType::TYPE_LABEL},
        {"void", LabelType::TYPE_LABEL},
        {"nul", LabelType::TYPE_LABEL},
        {"list", LabelType::TYPE_LABEL},
        {"func", LabelType::TYPE_LABEL},
        {"dict", LabelType::TYPE_LABEL},
        {"series", LabelType::TYPE_LABEL},
        {"flag", LabelType::TYPE_LABEL},
        {"any", LabelType::TYPE_LABEL},
        {"public", LabelType::PERMISSION_LABEL},
        {"private", LabelType::PERMISSION_LABEL},
        {"protected", LabelType::PERMISSION_LABEL},
        {"builtin", LabelType::PERMISSION_LABEL},
        {"static", LabelType::LIFE_CYCLE_LABEL},
        {"global", LabelType::LIFE_CYCLE_LABEL},
        {"const", LabelType::RESTRICTION_LABEL},
        {"quote", LabelType::RESTRICTION_LABEL},
        {"overwrite", LabelType::OBJECT_ORIENTED_LABEL},
        {"interface", LabelType::OBJECT_ORIENTED_LABEL},
        {"virtual", LabelType::OBJECT_ORIENTED_LABEL},
    };

    LabelType getLabelTypeByName(const std::string &name) {
        if (const auto &it = labelTypeMap.find(name);
            it != labelTypeMap.end()) {
            return it->second;
        }
        throw std::runtime_error("Unknown label type: " + name);
    }

    std::unordered_map<std::string, int> builtinLabelMap = {
        {"private", static_cast<int>(PermissionLabel::PRIVATE)},
        {"protected", static_cast<int>(PermissionLabel::PROTECTED)},
        {"public", static_cast<int>(PermissionLabel::PUBLIC)},
        {"builtin", static_cast<int>(PermissionLabel::BUILTIN)},
        {"interface", static_cast<int>(ObjectOrientedLabel::INTERFACE)},
        {"overwrite", static_cast<int>(ObjectOrientedLabel::OVERWRITE)},
        {"virtual", static_cast<int>(ObjectOrientedLabel::VIRTUAL)},
        {"global", static_cast<int>(LifeCycleLabel::GLOBAL)},
        {"static", static_cast<int>(LifeCycleLabel::STATIC)},
        {"ordinary", static_cast<int>(LifeCycleLabel::ORDINARY)},
        {"const", static_cast<int>(RestrictionLabel::CONST_RL)},
        {"quote", static_cast<int>(RestrictionLabel::QUOTE)},
    };

    PermissionLabel getPermissionLabelByName(const std::string& name)
    {
        if (const auto &it = builtinLabelMap.find(name);
            it != builtinLabelMap.end())
        {
            return static_cast<PermissionLabel>(it->second);
        }
        throw std::invalid_argument("Invalid permission label name: " + name);
    }

    RestrictionLabel getRestrictionLabelByName(const std::string &name)
    {
        if (const auto it = builtinLabelMap.find(name); it != builtinLabelMap.end())
        {
            return static_cast<RestrictionLabel>(it->second);
        }
        throw std::invalid_argument("Invalid restriction label name: " + name);
    }

    LifeCycleLabel getLifeCycleLabelByName(const std::string &name)
    {
        if (const auto it = builtinLabelMap.find(name); it != builtinLabelMap.end())
        {
            return static_cast<LifeCycleLabel>(it->second);
        }
        throw std::invalid_argument("Invalid life cycle label name: " + name);
    }

    ObjectOrientedLabel getObjectOrientedLabelByName(const std::string &name)
    {
        if (const auto it = builtinLabelMap.find(name); it != builtinLabelMap.end())
        {
            return static_cast<ObjectOrientedLabel>(it->second);
        }
        throw std::invalid_argument("Invalid object oriented label name: " + name);
    }

    bool checkVisitPermissionAllowed(const PermissionLabel& visitPermission, const PermissionLabel& permission)
    {
        switch (visitPermission)
        {
        case PermissionLabel::PRIVATE: return permission == PermissionLabel::PUBLIC;
        case PermissionLabel::PROTECTED: return permission >= PermissionLabel::PROTECTED;
        case PermissionLabel::PUBLIC: return true;
        case PermissionLabel::BUILTIN: return false;
        case PermissionLabel::COUNT:
        default: throw std::invalid_argument("Invalid permission label");
        }
    }

    Symbol::Symbol(utils::Pos pos,
                   const SymbolType &symbolType,
                   std::string symbolValue,
                   std::string symbolRaValue,
                   const size_t &scopeLevel)
        : pos(std::move(pos)), type(symbolType), value(std::move(symbolValue)), raValue(std::move(symbolRaValue)),
          scopeLevel(scopeLevel){}

    Symbol::~Symbol() {}

    IRCCSymbolInterface::~IRCCSymbolInterface() {}

    bool Symbol::is(const SymbolType& symbolType) const
    {
        return type == symbolType;
    }

    bool Symbol::Is(const SymbolType& tp) const
    {
        return is(tp);
    }

    bool Symbol::isNot(const SymbolType& symbolType) const
    {
        return type != symbolType;
    }

    bool Symbol::IsNot(const SymbolType& tp) const
    {
        return isNot(tp);
    }

    std::string Symbol::getRaVal() const {
        return raValue;
    }

    const char* Symbol::GetVal() const
    {
        return getVal().c_str();
    }

    const char* Symbol::GetRaVal() const
    {
        return getRaVal().c_str();
    }


    SymbolType Symbol::getType() const {
        return type;
    }

    SymbolType Symbol::GetType() const
    {
        return getType();
    }

    std::string Symbol::getVal() const {
        return value;
    }

    size_t Symbol::GetScopeLevel() const
    {
        return getScopeLevel();
    }

    size_t Symbol::getScopeLevel() const {
        return scopeLevel;
    }

    void Symbol::setScopeLevel(const size_t& level)
    {
        scopeLevel = level;
    }

    void Symbol::SetScopeLevel(const size_t& level)
    {
        setScopeLevel(level);
    }

    utils::Pos Symbol::getPos() const
    {
        return pos;
    }

    const utils::IRCCPosInterface* Symbol::GetPos() const
    {
        return &pos;
    }

    std::string Symbol::toString() const
    {
        return "[Symbol(" + symbolTypeToString(type)
            + "): " + value + "(" + raValue + ") @ " + std::to_string(scopeLevel) + "]";
    }

    bool Symbol::equalWith(const Symbol& other) const
    {
        return toString() == other.toString();
    }

    bool Symbol::equalWith(const std::shared_ptr<Symbol>& other) const
    {
        return toString() == other->toString();
    }

    std::shared_ptr<utils::Object> Symbol::copySymbolPtr(const std::shared_ptr<Symbol>& symbolPtr)
    {
        if (symbolPtr)
        {
            return symbolPtr->copySelf();
        }
        return nullptr;
    }

    const Symbol* Symbol::TransformToSI() const
    {
        return this;
    }

    const char* Symbol::ToString() const
    {
        return toString().c_str();
    }

    const char* Symbol::BriefString() const
    {
        return briefString().c_str();
    }

    const char* Symbol::ProfessionalString() const
    {
        return professionalString().c_str();
    }

    const char* Symbol::FormatString(const size_t& indent, const size_t& level) const
    {
        return formatString(indent, level).c_str();
    }

    const char* Symbol::ToJsonString() const
    {
        return toJsonString().c_str();
    }

    const utils::IRCCObjectInterface* Symbol::CopySelf()
    {
        return copySelf().get();
    }

    LabelSymbol::LabelSymbol(const utils::Pos &pos,
                             const std::string &name, const std::string &raValue,
                             const size_t &scopeLevel, const LabelType &labelType)
        : Symbol(pos, SymbolType::LABEL, name, raValue, scopeLevel),
          isBuiltIn_(isBuiltInLabel(name)),
          labelType(labelType == LabelType::UNKNOWN_TYPE_LABEL ? getLabelTypeByName(name) : labelType) {}

    LabelSymbol::~LabelSymbol() {}

    IRCCLabelSymbolInterface::~IRCCLabelSymbolInterface() {}

    bool LabelSymbol::isBuiltInLabel(const std::string &name) {
        return base::DESCRIBE_LABELS.contains(name);
    }

    bool IRCCLabelSymbolInterface::IsBuiltInLabel(const char* name)
    {
        return LabelSymbol::isBuiltInLabel(name);
    }

    LabelType LabelSymbol::getLabelType() const {
        return labelType;
    }

    LabelType LabelSymbol::GetLabelType() const
    {
        return getLabelType();
    }

    std::vector<LabelSymbol::LabelDes> LabelSymbol::getLabelDesS() const
    {
        return labelDesS;
    }

    const IRCCLabelSymbolInterface::LabelDesI* LabelSymbol::GetLabelDesIS() const {
        // 动态生成接口指针数组（注意：这里用 static 避免重复分配，但需确保线程安全和数据同步）
        static std::vector<LabelDesI> tempArray;
        tempArray.clear();

        // 遍历内部 shared_ptr 容器，转换为接口指针
        for (const auto& ldi : m_labelDesIS) {
            // 若 shared_ptr 非空，转换为接口指针（因 LabelSymbol 继承自接口）
            if (ldi) {
                tempArray.push_back(ldi);
            } else {
                tempArray.push_back(nullptr);
            }
        }
        return tempArray.empty() ? nullptr : tempArray.data();
    }

    void LabelSymbol::appendLabelDes(const LabelDes& labelDes)
    {
        labelDesS.push_back(labelDes);
        syncLabelDesIS();
    }

    void LabelSymbol::appendLastLabelDes(const LabelDes& labelDes)
    {
        appendLabelDes(labelDes);
        handleLabelDesRecorded();
    }

    void LabelSymbol::handleLabelDesRecorded()
    {
        if (getVal() == "funi" && labelDesS.size() == 1)
        {
            appendLabelDes({TypeLabelSymbol::anyTypeSymbol(utils::getUnknownPos(), 0)});
        }
    }

    bool LabelSymbol::isBuiltIn() const {
        return isBuiltIn_;
    }

    bool LabelSymbol::IsBuiltin() const
    {
        return isBuiltIn();
    }

    std::string LabelSymbol::toString() const {
        std::string result = "[<" + labelTypeToString(labelType) + "_LABEL> " + getVal();
        if (!labelDesS.empty()) {
            std::string desStr;
            for (const auto& desS : labelDesS) {
                desStr += "[";
                bool isFirst = true;
                for (const auto& des : desS) {
                    if (!isFirst) {
                        desStr += ", ";
                    }
                    desStr += des->briefString();
                    isFirst = false;
                }
                desStr += "]";
            }
            result += desStr;
        }
        result += "]";
        return result;
    }

    const LabelSymbol* LabelSymbol::TransformToLSI() const
    {
        return this;
    }

    std::string LabelSymbol::briefString() const {
        std::string result = getVal();
        if (!labelDesS.empty()) {
            std::string desStr;
            for (const auto& desS : labelDesS) {
                desStr += "[";
                bool isFirst = true;
                for (const auto& des : desS) {
                    if (!isFirst) {
                        desStr += ", ";
                    }
                    desStr += des->getVal();
                    isFirst = false;
                }
                desStr += "]";
            }
            result += desStr;
        }
        return result;
    }

    std::shared_ptr<utils::Object> LabelSymbol::copySelf() const
    {
        return std::make_shared<LabelSymbol>(
            getPos(), getVal(), getRaVal(), getScopeLevel(), getLabelType());
    }

    const char* LabelSymbol::ToString() const
    {
        return toString().c_str();
    }

    const char* LabelSymbol::BriefString() const
    {
        return briefString().c_str();
    }

    const utils::IRCCObjectInterface* LabelSymbol::CopySelf()
    {
        return copySelf().get();
    }

    std::shared_ptr<Symbol> LabelSymbol::transform(
        const std::string& value, const std::string& raValue, const size_t &scopeLevel) const
    {
        return std::make_shared<LabelSymbol>(
            getPos(), value, raValue, scopeLevel < 0 ? getScopeLevel() : scopeLevel,
            getLabelType());
    }

    void LabelSymbol::syncLabelDesIS() const
    {
        m_labelDesIS.clear();
        for (const auto& des : labelDesS) {
            for (const auto& sp : des) {
                IRCCLabelSymbolInterface* iface = sp.get();
                m_labelDesIS.push_back(iface);
            }
        }
    }

    bool LabelSymbol::Is(const SymbolType& tp) const
    {
        return Symbol::Is(tp);
    }

    bool LabelSymbol::IsNot(const SymbolType& tp) const
    {
        return Symbol::IsNot(tp);
    }

    const char* LabelSymbol::GetVal() const
    {
        return Symbol::GetVal();
    }

    const char* LabelSymbol::GetRaVal() const
    {
        return Symbol::GetRaVal();
    }

    SymbolType LabelSymbol::GetType() const
    {
        return Symbol::GetType();
    }

    size_t LabelSymbol::GetScopeLevel() const
    {
        return Symbol::GetScopeLevel();
    }

    void LabelSymbol::SetScopeLevel(const size_t& level)
    {
        Symbol::SetScopeLevel(level);
    }

    const utils::IRCCPosInterface* LabelSymbol::GetPos() const
    {
        return Symbol::GetPos();
    }

    const Symbol* LabelSymbol::TransformToSI() const
    {
        return Symbol::TransformToSI();
    }

    std::unordered_set<std::string> TypeLabelSymbol::builtInTypes = {
        "int", "float", "char", "str", "bool", "void", "nul",
        "list", "func", "funi", "dict", "series", "any", "flag",
        "clas", "..", ".*"
    };
    std::unordered_map<std::string, std::string> TypeLabelSymbol::builtInTypeRaCodeMap = {
        {"..", "tp-args"},
        {".*", "tp-kwargs"},
        {"nul", "tp-null"}
    };
    std::unordered_map<std::string, std::string> TypeLabelSymbol::customTypeMap = {};

    const char* IRCCTypeLabelSymbolInterface::GetTypeLabelRaCode(
        const std::string &name, const std::string &raValue) {
        return TypeLabelSymbol::getTypeLabelRaCode(name, raValue).c_str();
    }

    std::unordered_map<std::string, std::shared_ptr<ClassSymbol>> TypeLabelSymbol::customClassSymbolMap = {};

    TypeLabelSymbol::TypeLabelSymbol(const utils::Pos &pos, const std::string &name,
                                     const size_t &scopeLevel, const std::string &uid)
        : LabelSymbol(pos, name, getTypeLabelRaCode(name, uid), scopeLevel, LabelType::TYPE_LABEL) {}

    TypeLabelSymbol::~TypeLabelSymbol() {}

    IRCCTypeLabelSymbolInterface::~IRCCTypeLabelSymbolInterface() {}

    std::string TypeLabelSymbol::getTypeLabelRaCode(const std::string& name, const std::string& raValue)
    {
        if (const auto &it = builtInTypeRaCodeMap.find(name);
            it != builtInTypeRaCodeMap.end())
        {
            return it->second;
        }
        if (builtInTypes.contains(name)) {
            return "tp-" + name;
        }
        if (customTypeMap.contains(raValue)) {
            return raValue;
        }
        return "tp-any";
    }

    bool TypeLabelSymbol::isTypeLabel(const std::string &name) {
        return isBuiltInType(name) || isCustomType(name);
    }

    bool IRCCTypeLabelSymbolInterface::IsTypeLabel(const char* name)
    {
        return TypeLabelSymbol::isTypeLabel(name);
    }


    bool TypeLabelSymbol::isBuiltInType(const std::string &name) {
        return builtInTypes.contains(name);
    }

    bool IRCCTypeLabelSymbolInterface::IsBuiltInType(const char* name)
    {
        return TypeLabelSymbol::isBuiltInType(name);
    }

    bool TypeLabelSymbol::isCustomType(const std::string &uid) {
        return customTypeMap.contains(uid);
    }

    bool IRCCTypeLabelSymbolInterface::IsCustomType(const char* uid)
    {
        return TypeLabelSymbol::isCustomType(uid);
    }

    void TypeLabelSymbol::createCustomType(
        const std::string &name, const std::string &uid, const std::shared_ptr<ClassSymbol> &classSymbol) {
        customTypeMap[uid] = name;
        labelTypeMap[uid] = LabelType::TYPE_LABEL;
        customClassSymbolMap[uid] = classSymbol;
    }

    void TypeLabelSymbol::createCustomType(const std::shared_ptr<ClassSymbol>& classSymbol)
    {
        createCustomType(classSymbol->getVal(), classSymbol->getRaVal(), classSymbol);
    }

    void TypeLabelSymbol::deleteCustomType(const std::string &uid)
    {
        if (customTypeMap.contains(uid)) {
            customTypeMap.erase(uid);
            labelTypeMap.erase(uid);
        } else
        {
            throw base::RCCCompilerError::symbolNotFoundError(
                ast::CompileVisitor::currentPos().toString(),
                ast::CompileVisitor::getCodeLine(ast::CompileVisitor::currentPos()),
                uid,
                "Custom type not found",
                {
                    "Check if the custom type name is correct.",
                    "Ensure the custom type has been properly defined.",
                    "Verify the custom type exists in the current scope."
                }
            );
        }
    }

    std::shared_ptr<ClassSymbol> TypeLabelSymbol::getCustomClassSymbol(const std::string& uid)
    {
        if (const auto& it = customClassSymbolMap.find(uid); it != customClassSymbolMap.end())
        {
            return it->second;
        }
        throw base::RCCCompilerError::symbolNotFoundError(
            ast::CompileVisitor::currentPos().toString(),
            ast::CompileVisitor::getCodeLine(ast::CompileVisitor::currentPos()),
            uid,
            "Custom type not found",
            {
                "Check if the custom type name is correct.",
                "Ensure the custom type has been properly defined.",
                "Verify the custom type exists in the current scope."
            }
        );
    }

    std::shared_ptr<TypeLabelSymbol> TypeLabelSymbol::getCustomTypeLabelSymbol(const std::string& uid, const size_t &scopeLevel)
    {
        const auto &classSymbol = getCustomClassSymbol(uid);
        return std::make_shared<TypeLabelSymbol>(utils::getUnknownPos(), classSymbol->getVal(), scopeLevel, classSymbol->getRaVal());
    }

    bool TypeLabelSymbol::is(const std::string& name) const
    {
        return getVal() == name;
    }

    bool TypeLabelSymbol::Is(const char* name) const
    {
        return is(name);
    }

    bool TypeLabelSymbol::isNot(const std::string& name) const
    {
        return getVal() != name;
    }

    bool TypeLabelSymbol::IsNot(const char* name) const
    {
        return isNot(name);
    }

    bool TypeLabelSymbol::isCustomType() const
    {
        return isCustomType(getRaVal());
    }

    bool TypeLabelSymbol::IsCustomType() const
    {
        return isCustomType();
    }

    std::shared_ptr<ClassSymbol> TypeLabelSymbol::getCustomClassSymbol() const
    {
        return getCustomClassSymbol(getRaVal());
    }

    bool TypeLabelSymbol::isIterable() const
    {
        return is("str") || is("list") || is("dict") || is("series");
    }

    bool TypeLabelSymbol::equalWith(const std::shared_ptr<Symbol>& other) const
    {
        if (!other->is(SymbolType::LABEL))
        {
            return Symbol::equalWith(other);
        }
        if (const auto &otherLabelSymbol = std::static_pointer_cast<LabelSymbol>(other);
            otherLabelSymbol->getLabelType() != LabelType::TYPE_LABEL)
        {
            return Symbol::equalWith(other);
        }
        if (const auto &otherTypeLabelSymbol = std::static_pointer_cast<TypeLabelSymbol>(other);
            isCustomType() && otherTypeLabelSymbol->isCustomType())
        {
            return getCustomClassSymbol()->equalWith(otherTypeLabelSymbol->getCustomClassSymbol());
        } else
        {
            if ((isNot("any") && otherTypeLabelSymbol->isNot("any") &&
                getVal() != otherTypeLabelSymbol->getVal()) ||
                getLabelDesS().size() != otherTypeLabelSymbol->getLabelDesS().size())
            {
                return false;
            }
            for (int i = 0; i < getLabelDesS().size(); i ++)
            {
                auto thisLabelDesS = getLabelDesS()[i];
                auto otherLabelDesS = otherTypeLabelSymbol->getLabelDesS()[i];
                if (thisLabelDesS.size() != otherLabelDesS.size()) return false;
                for (int o = 0; o < thisLabelDesS.size(); o ++)
                {
                    if (!thisLabelDesS[o]->equalWith(otherLabelDesS[o]))
                    {
                        return false;
                    }
                }
            }
            return true;
        }
    }

    bool TypeLabelSymbol::relatedTo(const std::shared_ptr<TypeLabelSymbol>& other) const
    {
        if (equalWith(other)) return true;
        if (!isCustomType() && !other->isCustomType()) return false;
        const auto &selfClassSymbol = getCustomClassSymbol();
        const auto &otherClassSymbol = other->getCustomClassSymbol();
        return selfClassSymbol->relatedTo(otherClassSymbol);
    }

    std::shared_ptr<Symbol> TypeLabelSymbol::transform(const std::string& value, const std::string& raValue,
                                                       const size_t& scopeLevel) const
    {
        return std::make_shared<TypeLabelSymbol>(getPos(), value, scopeLevel, raValue);
    }

    const LabelSymbol* TypeLabelSymbol::TransformToLSI() const
    {
        return LabelSymbol::TransformToLSI();
    }

    std::shared_ptr<utils::Object> TypeLabelSymbol::copySelf() const
    {
        return std::make_shared<TypeLabelSymbol>(
            getPos(), getVal(), getScopeLevel(), getRaVal());
    }

    bool TypeLabelSymbol::Is(const SymbolType& tp) const
    {
        return Symbol::Is(tp);
    }

    bool TypeLabelSymbol::IsNot(const SymbolType& tp) const
    {
        return Symbol::IsNot(tp);
    }

    const char* TypeLabelSymbol::GetVal() const
    {
        return Symbol::GetVal();
    }

    const char* TypeLabelSymbol::GetRaVal() const
    {
        return Symbol::GetRaVal();
    }

    SymbolType TypeLabelSymbol::GetType() const
    {
        return Symbol::GetType();
    }

    size_t TypeLabelSymbol::GetScopeLevel() const
    {
        return Symbol::GetScopeLevel();
    }

    void TypeLabelSymbol::SetScopeLevel(const size_t& level)
    {
        Symbol::SetScopeLevel(level);
    }

    const utils::IRCCPosInterface* TypeLabelSymbol::GetPos() const
    {
        return Symbol::GetPos();
    }

    const Symbol* TypeLabelSymbol::TransformToSI() const
    {
        return Symbol::TransformToSI();
    }

    LabelType TypeLabelSymbol::GetLabelType() const
    {
        return LabelSymbol::GetLabelType();
    }

    const IRCCLabelSymbolInterface::LabelDesI* TypeLabelSymbol::GetLabelDesIS() const
    {
        return LabelSymbol::GetLabelDesIS();
    }

    bool TypeLabelSymbol::IsBuiltin() const
    {
        return LabelSymbol::IsBuiltin();
    }

    const TypeLabelSymbol* TypeLabelSymbol::TransformToTLSI() const
    {
        return this;
    }

    std::shared_ptr<TypeLabelSymbol> TypeLabelSymbol::intTypeSymbol(const utils::Pos &pos, const size_t &scopeLevel) {
        return std::make_shared<TypeLabelSymbol>(pos, "int", scopeLevel);
    }

    std::shared_ptr<TypeLabelSymbol> TypeLabelSymbol::floatTypeSymbol(const utils::Pos &pos, const size_t &scopeLevel) {
        return std::make_shared<TypeLabelSymbol>(pos, "float", scopeLevel);
    }

    std::shared_ptr<TypeLabelSymbol> TypeLabelSymbol::strTypeSymbol(const utils::Pos &pos, const size_t &scopeLevel) {
        return std::make_shared<TypeLabelSymbol>(pos, "str", scopeLevel);
    }

    std::shared_ptr<TypeLabelSymbol> TypeLabelSymbol::boolTypeSymbol(const utils::Pos &pos, const size_t &scopeLevel) {
        return std::make_shared<TypeLabelSymbol>(pos, "bool", scopeLevel);
    }

    std::shared_ptr<TypeLabelSymbol> TypeLabelSymbol::voidTypeSymbol(const utils::Pos &pos, const size_t &scopeLevel) {
        return std::make_shared<TypeLabelSymbol>(pos, "void", scopeLevel);
    }

    std::shared_ptr<TypeLabelSymbol> TypeLabelSymbol::nulTypeSymbol(const utils::Pos &pos, const size_t &scopeLevel) {
        return std::make_shared<TypeLabelSymbol>(pos, "nul", scopeLevel);
    }

    std::shared_ptr<TypeLabelSymbol> TypeLabelSymbol::anyTypeSymbol(const utils::Pos &pos, const size_t &scopeLevel) {
        return std::make_shared<TypeLabelSymbol>(pos, "any", scopeLevel);
    }

    std::shared_ptr<TypeLabelSymbol> TypeLabelSymbol::charTypeSymbol(const utils::Pos &pos, const size_t &scopeLevel) {
        return std::make_shared<TypeLabelSymbol>(pos, "char", scopeLevel);
    }

    std::shared_ptr<TypeLabelSymbol> TypeLabelSymbol::listTypeSymbol(const utils::Pos &pos, const size_t &scopeLevel) {
        return std::make_shared<TypeLabelSymbol>(pos, "list", scopeLevel);
    }

    std::shared_ptr<TypeLabelSymbol> TypeLabelSymbol::dictTypeSymbol(const utils::Pos& pos, const size_t& scopeLevel)
    {
        return std::make_shared<TypeLabelSymbol>(pos, "dict", scopeLevel);
    }

    std::shared_ptr<TypeLabelSymbol> TypeLabelSymbol::flagTypeSymbol(const utils::Pos& pos, const size_t& scopeLevel)
    {
        return std::make_shared<TypeLabelSymbol>(pos, "flag", scopeLevel);
    }

    std::shared_ptr<TypeLabelSymbol> TypeLabelSymbol::seriesTypeSymbol(const utils::Pos& pos, const size_t& scopeLevel)
    {
        return std::make_shared<TypeLabelSymbol>(pos, "series", scopeLevel);
    }

    std::shared_ptr<TypeLabelSymbol> TypeLabelSymbol::funcTypeSymbol(const utils::Pos& pos, const size_t& scopeLevel)
    {
        return std::make_shared<TypeLabelSymbol>(pos, "func", scopeLevel);
    }

    std::shared_ptr<TypeLabelSymbol> TypeLabelSymbol::funiTypeSymbol(const utils::Pos& pos, const size_t& scopeLevel)
    {
        return std::make_shared<TypeLabelSymbol>(pos, "funi", scopeLevel);
    }

    std::shared_ptr<TypeLabelSymbol> TypeLabelSymbol::clasTypeSymbol(const utils::Pos& pos, const size_t& scopeLevel)
    {
        return std::make_shared<TypeLabelSymbol>(pos, "clas", scopeLevel);
    }

    std::shared_ptr<TypeLabelSymbol> TypeLabelSymbol::varArgTypeSymbol(const utils::Pos& pos, const size_t& scopeLevel)
    {
        return std::make_shared<TypeLabelSymbol>(pos, "..", scopeLevel);
    }

    std::shared_ptr<TypeLabelSymbol> TypeLabelSymbol::kwVarArgTypeSymbol(const utils::Pos& pos,
        const size_t& scopeLevel)
    {
        return std::make_shared<TypeLabelSymbol>(pos, ".*", scopeLevel);
    }

    std::shared_ptr<TypeLabelSymbol> TypeLabelSymbol::getTypeLabelSymbolByStr(const std::string& str,
                                                                              const size_t& scopeLevel)
    {
        return std::make_shared<TypeLabelSymbol>(utils::getUnknownPos(), str, scopeLevel);
    }

    void LabelMarkManager::markLabels(const std::unordered_set<std::shared_ptr<LabelSymbol>>& labels)
    {
        for (const auto& label : labels)
        {
            markLabel(label);
        }
        displayResult();
    }

    void LabelMarkManager::markLabel(const std::shared_ptr<LabelSymbol>& label, const bool& refresh)
    {
        switch (label->getLabelType())
        {
        case LabelType::UNKNOWN_TYPE_LABEL: throw std::runtime_error("Unknown label type");
        case LabelType::TYPE_LABEL:
            {
                markTypeLabel(label->getVal());
            } break;
        case LabelType::PERMISSION_LABEL:
            {
                markPermissionLabel(getPermissionLabelByName(label->getVal()));
            } break;
        case LabelType::LIFE_CYCLE_LABEL:
            {
                markLifeCycleLabel(getLifeCycleLabelByName(label->getVal()));
            }break;
        case LabelType::RESTRICTION_LABEL:
            {
                markRestrictionLabel(getRestrictionLabelByName(label->getVal()));
            } break;
        case LabelType::OBJECT_ORIENTED_LABEL:
            {
                markObjectOrientedLabel(getObjectOrientedLabelByName(label->getVal()));
            } break;
        }
        if (refresh) displayResult();
    }

    bool LabelMarkManager::isPermissionLabelMarked(const PermissionLabel& permissionLabel) const
    {
        return permissionLabelMark == static_cast<int>(permissionLabel);
    }

    bool LabelMarkManager::isObjectOrientedLabelMarked(const ObjectOrientedLabel& objectOrientedLabel) const
    {
        return (objectOrientedLabelMarks & 1 << static_cast<int>(objectOrientedLabel)) != 0;
    }

    bool LabelMarkManager::isLifeCycleLabelMarked(const LifeCycleLabel& lifeCycleLabel) const
    {
        return (lifeCycleLabelMarks & 1 << static_cast<int>(lifeCycleLabel)) != 0;
    }

    bool LabelMarkManager::isRestrictionLabelMarked(const RestrictionLabel& restrictionLabel) const
    {
        return (restrictionLabelMarks & 1 << static_cast<int>(restrictionLabel)) != 0;
    }

    bool LabelMarkManager::isTypeLabelMarked(const std::string& typeLabel) const
    {
        return typeLabelMarks.contains(typeLabel);
    }

    void LabelMarkManager::cancelPermissionLabelMark()
    {
        resetPermissionLabelMarks();
    }

    void LabelMarkManager::cancelObjectOrientedLabelMark(const ObjectOrientedLabel& objectOrientedLabel)
    {
        objectOrientedLabelMarks &= ~(1 << static_cast<int>(objectOrientedLabel));
    }

    void LabelMarkManager::cancelLifeCycleLabelMark(const LifeCycleLabel& lifeCycleLabel)
    {
        lifeCycleLabelMarks &= ~(1 << static_cast<int>(lifeCycleLabel));
    }

    void LabelMarkManager::cancelRestrictionLabelMark(const RestrictionLabel& restrictionLabel)
    {
        restrictionLabelMarks &= ~(1 << static_cast<int>(restrictionLabel));
    }

    void LabelMarkManager::cancelTypeLabelMark(const std::string& typeLabel)
    {
        typeLabelMarks.erase(typeLabel);
    }

    void LabelMarkManager::resetPermissionLabelMarks()
    {
        permissionLabelMark = -1;
    }

    void LabelMarkManager::resetObjectOrientedLabelMarks()
    {
        objectOrientedLabelMarks = 0;
    }

    void LabelMarkManager::resetLifeCycleLabelMarks()
    {
        lifeCycleLabelMarks = 0;
    }

    void LabelMarkManager::resetRestrictionLabelMarks()
    {
        restrictionLabelMarks = 0;
    }

    void LabelMarkManager::resetTypeLabelMarks()
    {
        typeLabelMarks.clear();
    }

    void LabelMarkManager::displayResult()
    {
        auto buildLabelSection = [this](auto labelCount, auto isMarkedFunc, auto toStringFunc, const std::string& sectionName) {
            resultDisplay += sectionName + "[";
            bool firstItem = true;

            for (int i = 0; i < labelCount; ++i) {
                // ReSharper disable once CppRedundantCastExpression
                if (isMarkedFunc(static_cast<decltype(i)>(i))) {
                    if (!firstItem) {
                        resultDisplay += ", ";
                    }
                    // ReSharper disable once CppRedundantCastExpression
                    resultDisplay += toStringFunc(static_cast<decltype(i)>(i));
                    firstItem = false;
                }
            }
            resultDisplay += "]";
        };

        // Build each section
        resultDisplay = "Perm: [" + permissionLabelToString(static_cast<PermissionLabel>(permissionLabelMark)) + "]";

        buildLabelSection(
            static_cast<int>(ObjectOrientedLabel::COUNT),
            [this](auto label) { return isObjectOrientedLabelMarked(static_cast<ObjectOrientedLabel>(label)); },
            [](auto label) { return objectOrientedLabelToString(static_cast<ObjectOrientedLabel>(label)); },
            "\nObj: "
        );

        buildLabelSection(
            static_cast<int>(LifeCycleLabel::COUNT),
            [this](auto label) { return isLifeCycleLabelMarked(static_cast<LifeCycleLabel>(label)); },
            [](auto label) { return lifeCycleLabelToString(static_cast<LifeCycleLabel>(label)); },
            "\nLife: "
        );

        buildLabelSection(
            static_cast<int>(RestrictionLabel::COUNT),
            [this](auto label) { return isRestrictionLabelMarked(static_cast<RestrictionLabel>(label)); },
            [](auto label) { return restrictionLabelToString(static_cast<RestrictionLabel>(label)); },
            "\nRestr: "
        );

        // Special handling for typeLabelMarks which is different
        resultDisplay += "\nType: [";
        bool firstType = true;
        for (const auto& typeLabel : typeLabelMarks) {
            if (!firstType) {
                resultDisplay += ", ";
            }
            resultDisplay += typeLabel;
            firstType = false;
        }
        resultDisplay += "]";
    }

    std::optional<PermissionLabel> LabelMarkManager::getPermissionLabelMark() const
    {
        return permissionLabelMark == -1 ?
                   std::nullopt :
                   std::optional{static_cast<PermissionLabel>(permissionLabelMark)};
    }

    std::unordered_set<std::string> LabelMarkManager::getTypeLabelMarks() const
    {
        return typeLabelMarks;
    }

    void LabelMarkManager::markPermissionLabel(const PermissionLabel& permissionLabel)
    {
        if (permissionLabelMark != -1)
            throw std::runtime_error("Permission label has been marked");
        permissionLabelMark = static_cast<int>(permissionLabel);
    }

    void LabelMarkManager::markObjectOrientedLabel(const ObjectOrientedLabel& objectOrientedLabel)
    {
        objectOrientedLabelMarks |= 1 << static_cast<int>(objectOrientedLabel);
    }

    void LabelMarkManager::markLifeCycleLabel(const LifeCycleLabel& lifeCycleLabel)
    {
        lifeCycleLabelMarks |= 1 << static_cast<int>(lifeCycleLabel);
    }

    void LabelMarkManager::markRestrictionLabel(const RestrictionLabel& restrictionLabel)
    {
        restrictionLabelMarks |= 1 << static_cast<int>(restrictionLabel);
    }

    void LabelMarkManager::markTypeLabel(const std::string& typeLabel)
    {
        typeLabelMarks.insert(typeLabel);
    }

    ParameterSymbol::ParameterSymbol(
        const ParamType &paramType,
        const utils::Pos &pos,
        const std::string &name, const std::string &nameID,
        const std::unordered_set<std::shared_ptr<LabelSymbol>> &labels,
        const std::optional<std::string> &defaultValue,
        const size_t &scopeLevel,
        const SymbolType &symbolType,
        const std::shared_ptr<TypeLabelSymbol> &valueType)
        : Symbol(pos, symbolType, name, nameID, scopeLevel),
          valueType(valueType), labels(labels), defaultValue(defaultValue), paramType(paramType) {
        for (const auto &label: labels) {
            if (label->getLabelType() == LabelType::TYPE_LABEL) {
                if (!typeLabel) {
                    typeLabel = std::static_pointer_cast<TypeLabelSymbol>(label);
                } else {
                    throw base::RCCSyntaxError::duplicateTypeLabelError(pos.toString(), RCC_UNKNOWN_CONST,
                                                                        label->toString());
                }
            }
        }
        if (!typeLabel) {
            typeLabel = std::make_shared<TypeLabelSymbol>(pos, "any", scopeLevel);
        }
        if (!this->valueType)
        {
            this->valueType = TypeLabelSymbol::nulTypeSymbol(pos, scopeLevel);
        }
        labelMarkManager.markLabels(this->labels);
    }

    ParameterSymbol::~ParameterSymbol() {}

    bool ParameterSymbol::typeIs(const std::string& type) const
    {
        return typeLabel->is(type) || valueType->is(type) || typeLabel->is("any") ||
            valueType->is("any");
    }

    bool ParameterSymbol::typeIsNot(const std::string& type) const
    {
        return !typeIs(type);
    }

    IRCCParameterSymbolInterface::~IRCCParameterSymbolInterface() {}

    std::shared_ptr<TypeLabelSymbol> ParameterSymbol::getTypeLabel() const {
        return typeLabel;
    }

    std::unordered_set<std::shared_ptr<LabelSymbol>> ParameterSymbol::getLabels() const {
        return labels;
    }

    std::optional<std::string> ParameterSymbol::getDefaultValue() const
    {
        return defaultValue;
    }

    void ParameterSymbol::setDefaultValue(const std::optional<std::string>& value)
    {
        this->defaultValue = value;
    }

    ParamType ParameterSymbol::getParamType() const
    {
        return paramType;
    }

    void ParameterSymbol::reSetLabels(const std::unordered_set<std::shared_ptr<LabelSymbol>>& labelSymbols)
    {
        std::shared_ptr<TypeLabelSymbol> typeLabelSymbol = nullptr;
        for (const auto &label: labelSymbols) {
            if (label->getLabelType() == LabelType::TYPE_LABEL) {
                if (!typeLabelSymbol) {
                    typeLabelSymbol = std::static_pointer_cast<TypeLabelSymbol>(label);
                } else {
                    throw base::RCCSyntaxError::duplicateTypeLabelError(getPos().toString(), RCC_UNKNOWN_CONST,
                                                                        label->toString());
                }
            }
        }
        if (typeLabelSymbol && !typeLabelSymbol->equalWith(typeLabel)) {
            typeLabel = typeLabelSymbol;
        }
    }

    void ParameterSymbol::setTypeLabel(const std::shared_ptr<TypeLabelSymbol>& labelSymbol)
    {
        this->typeLabel = labelSymbol;
    }

    void ParameterSymbol::setValueType(const std::shared_ptr<TypeLabelSymbol>& labelSymbol)
    {
        if (typeLabel->is("any") || labelSymbol->is("any") ||
            typeLabel->equalWith(labelSymbol) || typeLabel->relatedTo(labelSymbol))
        {
            this->valueType = labelSymbol;
        } else
        {
            const auto &currentPos = ast::CompileVisitor::currentPos();
            throw base::RCCCompilerError::typeMissmatchError(
                currentPos.toString(),
                ast::CompileVisitor::getCodeLine(currentPos),
                "This error is caused by changing a symbol's type to mismatched target type.",
                ast::CompileVisitor::getListFormatString({
                    "any", typeLabel->toString()
                }), labelSymbol->toString(),
                {
                    "Check the target type is correct, avoid modifying the symbol's type to an unrelated or mismatched type."
                });
        }
    }

    std::shared_ptr<TypeLabelSymbol> ParameterSymbol::getValueType() const
    {
        return valueType;
    }

    LabelMarkManager& ParameterSymbol::getLabelMarkManager()
    {
        return labelMarkManager;
    }

    LabelMarkManager ParameterSymbol::getLabelMarkManager() const
    {
        return labelMarkManager;
    }

    std::shared_ptr<Symbol> ParameterSymbol::transform(const std::string& value, const std::string& raValue,
                                                       const size_t& scopeLevel) const
    {
        return std::make_shared<ParameterSymbol>(
            getParamType(), getPos(), value, raValue, getLabels(),
            getDefaultValue(), scopeLevel, getType(), getValueType());
    }

    std::string ParameterSymbol::toString() const
    {
        return "[<Param> " + getVal() + ": " + getTypeLabel()->briefString() +
            (getDefaultValue().has_value() ? " = " + getDefaultValue().value() : "") + "]";
    }

    std::shared_ptr<utils::Object> ParameterSymbol::copySelf() const
    {
        std::unordered_set<std::shared_ptr<LabelSymbol>> labels_ = {};
        for (const auto &l: labels)
        {
            labels_.insert(std::static_pointer_cast<LabelSymbol>(l->copySelf()));
        }
        return std::make_shared<ParameterSymbol>(
            paramType, getPos(), getVal(), getRaVal(), labels_, defaultValue, getScopeLevel(),
            getType(), std::static_pointer_cast<TypeLabelSymbol>(copySymbolPtr(getValueType())));
    }

    IRCCTypeLabelSymbolInterface* ParameterSymbol::GetTypeLabelSymbolI() const
    {
        return getTypeLabel().get();
    }

    IRCCTypeLabelSymbolInterface* ParameterSymbol::GetValueTypeLabelSymbolI() const
    {
        return getValueType().get();
    }

    void ParameterSymbol::SetTypeLabelSymbol(IRCCTypeLabelSymbolInterface* typeLabelI)
    {
        setTypeLabel(std::static_pointer_cast<TypeLabelSymbol>(typeLabelI->TransformToTLSI()->copySelf()));
    }

    void ParameterSymbol::SetValueTypeLabelSymbol(IRCCTypeLabelSymbolInterface* valueTypeI)
    {
        setValueType(std::static_pointer_cast<TypeLabelSymbol>(valueTypeI->TransformToTLSI()->copySelf()));
    }

    IRCCParameterSymbolInterface::Labels ParameterSymbol::GetLabels() const
    {
        Labels labels_ (labels.size());
        size_t i = 0;
        for (const auto &l: labels)
        {
            labels_.labels[i] = l.get();
            i ++;
        }
        return labels_;
    }

    void ParameterSymbol::FreeLabels(Labels& labels_)
    {
        if (labels_.labels)
        {
            delete labels_.labels;
            labels_.labels = nullptr;
            labels_.count = 0;
        }
    }

    void ParameterSymbol::ResetLabels(const Labels& labels_)
    {
        std::unordered_set<std::shared_ptr<LabelSymbol>> newLabels {};
        for (int i = 0; i < labels_.count; i ++)
        {
            newLabels.insert(std::static_pointer_cast<LabelSymbol>(labels_.labels[i]->TransformToLSI()->copySelf()));
        }
        reSetLabels(newLabels);
    }

    const char* ParameterSymbol::GetDefaultValue() const
    {
        return getDefaultValue().has_value() ? getDefaultValue().value().c_str() : nullptr;
    }

    void ParameterSymbol::SetDefaultValue(const char* value)
    {
        return setDefaultValue(value ? std::optional<std::string>(value) : std::nullopt);
    }

    ParamType ParameterSymbol::GetParamType() const
    {
        return paramType;
    }

    const ParameterSymbol* ParameterSymbol::TransformToPSI() const
    {
        return this;
    }

    const char* ParameterSymbol::ToString() const
    {
        return toString().c_str();
    }

    bool ParameterSymbol::Is(const SymbolType& tp) const
    {
        return Symbol::Is(tp);
    }

    bool ParameterSymbol::IsNot(const SymbolType& tp) const
    {
        return Symbol::IsNot(tp);
    }

    const char* ParameterSymbol::GetVal() const
    {
        return Symbol::GetVal();
    }

    const char* ParameterSymbol::GetRaVal() const
    {
        return Symbol::GetRaVal();
    }

    SymbolType ParameterSymbol::GetType() const
    {
        return Symbol::GetType();
    }

    size_t ParameterSymbol::GetScopeLevel() const
    {
        return Symbol::GetScopeLevel();
    }

    void ParameterSymbol::SetScopeLevel(const size_t& level)
    {
        Symbol::SetScopeLevel(level);
    }

    const utils::IRCCPosInterface* ParameterSymbol::GetPos() const
    {
        return Symbol::GetPos();
    }

    const Symbol* ParameterSymbol::TransformToSI() const
    {
        return Symbol::TransformToSI();
    }

    VariableSymbol::VariableSymbol(const utils::Pos &pos,
                                   const std::string &varName,
                                   const std::string &vid,
                                   const std::unordered_set<std::shared_ptr<LabelSymbol>> &labels,
                                   const size_t &scopeLevel,
                                   const bool &bindValueTypeToTypeLabel,
                                   const std::shared_ptr<TypeLabelSymbol> &valueType,
                                   const std::shared_ptr<ClassSymbol> &classSymbol,
                                   const std::shared_ptr<Symbol> &referencedSymbol,
                                   const std::shared_ptr<ast::ExpressionNode> &defaultValueNode):
    ParameterSymbol(ParamType::NO_PARAM, pos, varName, vid, labels,
    std::nullopt, scopeLevel, SymbolType::VARIABLE, valueType),
    classSymbol(classSymbol), referencedSymbol(referencedSymbol),
    defaultValueNode(defaultValueNode)
    {
        if (bindValueTypeToTypeLabel)
        {
            setValueType(getTypeLabel());
        }
    }

    std::shared_ptr<ClassSymbol> VariableSymbol::getClassSymbol() const {
        return classSymbol;
    }

    void VariableSymbol::setClassSymbol(const std::shared_ptr<ClassSymbol>& classSymbol_)
    {
        this->classSymbol = classSymbol_;
    }

    std::shared_ptr<VariableSymbol> VariableSymbol::paramSymbolToVarSymbol(
        const std::shared_ptr<ParameterSymbol>& paramSymbol, const size_t &scopeLevel)
    {
        return std::make_shared<VariableSymbol>(paramSymbol->getPos(), paramSymbol->getVal(), paramSymbol->getRaVal(),
                                                paramSymbol->getLabels(), scopeLevel, true, nullptr, nullptr);
    }

    std::shared_ptr<Symbol> VariableSymbol::transform(const std::string& value, const std::string& raValue,
                                                      const size_t& scopeLevel) const
    {
        return std::make_shared<VariableSymbol>(
            getPos(), value, raValue, getLabels(), scopeLevel, false,
            getValueType(), getClassSymbol());
    }

    PermissionLabel VariableSymbol::getPermissionLabel() const
    {
        const auto &permission = getLabelMarkManager().getPermissionLabelMark();
        return permission.has_value() ?
                   permission.value() : classSymbol->getDefaultVisitPermission();
    }

    std::shared_ptr<Symbol> VariableSymbol::getReferencedSymbol() const
    {
        return referencedSymbol;
    }

    void VariableSymbol::setReferencedSymbol(const std::shared_ptr<Symbol>& refSymbol)
    {
        this->referencedSymbol = refSymbol;
    }

    std::string VariableSymbol::toString() const
    {
        return "[<Variable> " + getVal() + ": " + getTypeLabel()->briefString() +
            (getDefaultValue().has_value() ? " = " + getDefaultValue().value() : "") + "]";
    }

    std::shared_ptr<ast::ExpressionNode> VariableSymbol::getDefaultValueNode() const
    {
        return defaultValueNode;
    }

    std::shared_ptr<utils::Object> VariableSymbol::copySelf() const
    {
        std::unordered_set<std::shared_ptr<LabelSymbol>> labels_ = {};
        for (const auto &l: getLabels())
        {
            labels_.insert(std::static_pointer_cast<LabelSymbol>(copySymbolPtr(l)));
        }
        return std::make_shared<VariableSymbol>(
            getPos(), getVal(), getRaVal(), labels_, getScopeLevel(), false,
            std::static_pointer_cast<TypeLabelSymbol>(copySymbolPtr(getValueType())),
            std::static_pointer_cast<ClassSymbol>(copySymbolPtr(getClassSymbol())),
            std::static_pointer_cast<Symbol>(copySymbolPtr(getReferencedSymbol())),
            getDefaultValueNode());
    }

    IRCCTypeLabelSymbolInterface* VariableSymbol::GetTypeLabelSymbolI() const
    {
        return ParameterSymbol::GetTypeLabelSymbolI();
    }

    IRCCTypeLabelSymbolInterface* VariableSymbol::GetValueTypeLabelSymbolI() const
    {
        return ParameterSymbol::GetValueTypeLabelSymbolI();
    }

    void VariableSymbol::SetTypeLabelSymbol(IRCCTypeLabelSymbolInterface* typeLabel)
    {
        ParameterSymbol::SetTypeLabelSymbol(typeLabel);
    }

    void VariableSymbol::SetValueTypeLabelSymbol(IRCCTypeLabelSymbolInterface* valueType)
    {
        ParameterSymbol::SetValueTypeLabelSymbol(valueType);
    }

    IRCCParameterSymbolInterface::Labels VariableSymbol::GetLabels() const
    {
        return ParameterSymbol::GetLabels();
    }

    void VariableSymbol::FreeLabels(Labels& labels)
    {
        ParameterSymbol::FreeLabels(labels);
    }

    void VariableSymbol::ResetLabels(const Labels& labels)
    {
        ParameterSymbol::ResetLabels(labels);
    }

    const char* VariableSymbol::GetDefaultValue() const
    {
        return ParameterSymbol::GetDefaultValue();
    }

    void VariableSymbol::SetDefaultValue(const char* value)
    {
        ParameterSymbol::SetDefaultValue(value);
    }

    ParamType VariableSymbol::GetParamType() const
    {
        return ParameterSymbol::GetParamType();
    }

    const ParameterSymbol* VariableSymbol::TransformToPSI() const
    {
        return ParameterSymbol::TransformToPSI();
    }

    const char* VariableSymbol::ToString() const
    {
        return toString().c_str();
    }

    IRCCClassSymbolInterface* VariableSymbol::GetClassSymbolI() const
    {
        return getClassSymbol().get();
    }

    PermissionLabel VariableSymbol::GetPermissionLabel() const
    {
        return getPermissionLabel();
    }

    IRCCSymbolInterface* VariableSymbol::GetReferencedSymbol() const
    {
        return getReferencedSymbol().get();
    }

    void VariableSymbol::SetReferencedSymbol(const IRCCSymbolInterface* symbolI)
    {
        setReferencedSymbol(std::static_pointer_cast<Symbol>(symbolI->TransformToSI()->copySelf()));
    }

    void VariableSymbol::SetClassSymbol(const IRCCClassSymbolInterface* classSymbolI)
    {
        setClassSymbol(std::static_pointer_cast<ClassSymbol>(classSymbolI->TransformToCSI()->copySelf()));
    }

    FunctionSymbol::FunctionSymbol(
        const std::shared_ptr<ast::ExpressionNode> &definitionNode,
        const utils::Pos &pos, const std::string &name,
        const std::string &nameID,
        const std::unordered_set<std::shared_ptr<LabelSymbol>> &labels,
        const std::vector<std::shared_ptr<ParameterSymbol>> &parameters,
        const std::shared_ptr<TypeLabelSymbol> &signature,
        size_t scopeLevel, const TypeOfBuiltin &builtinType,
        const FunctionType &functionType,
        const std::shared_ptr<ClassSymbol> &className)
        : Symbol(pos, SymbolType::FUNCTION, name, nameID, scopeLevel),
          labels(labels), builtinType(builtinType),
          classSymbol(className), parameters(parameters), definitionNode(definitionNode),
          functionType(functionType), signature(signature)
    {
        for (const auto &label: labels)
        {
            if (label->getLabelType() == LabelType::TYPE_LABEL)
            {
                if (!returnType)
                {
                    returnType = std::static_pointer_cast<TypeLabelSymbol>(label);
                }
                else
                {
                    throw base::RCCSyntaxError::duplicateTypeLabelError(pos.toString(), RCC_UNKNOWN_CONST,
                                                                        label->toString());
                }
            }
        }
        if (!returnType)
        {
            returnType = TypeLabelSymbol::anyTypeSymbol(pos, scopeLevel);
        }
        labelMarkManager.markLabels(labels);
    }

    std::shared_ptr<TypeLabelSymbol> FunctionSymbol::getReturnType() const
    {
        return returnType;
    }

    void FunctionSymbol::setReturnType(const std::shared_ptr<TypeLabelSymbol>& type)
    {
        if (!returnType)
        {
            returnType = type;
        } else
        {
            throw std::runtime_error("Return type already set.");
        }
    }

    void FunctionSymbol::reSetReturnType(const std::shared_ptr<TypeLabelSymbol>& type)
    {
        returnType = type;
    }

    bool FunctionSymbol::hasReturnValue() const
    {
        return returnType != nullptr && returnType->isNot("void");
    }

    bool FunctionSymbol::hasSetReturnType() const
    {
        return returnType != nullptr;
    }

    bool FunctionSymbol::hasReturned() const
    {
        return hasReturned_;
    }

    void FunctionSymbol::setHasReturned(bool hasReturned)
    {
        hasReturned_ = hasReturned;
    }

    void FunctionSymbol::setFunctionType(const FunctionType& funcType)
    {
        this->functionType = funcType;
    }

    void FunctionSymbol::setClassSymbol(const std::shared_ptr<ClassSymbol>& symbol)
    {
        this->classSymbol = symbol;
    }

    void FunctionSymbol::setBuiltInType(const TypeOfBuiltin& type)
    {
        this->builtinType = type;
    }

    void FunctionSymbol::setSignature(const std::shared_ptr<TypeLabelSymbol>& signature_)
    {
        signature = signature_;
        if (hasReturnValue() && signature->getLabelDesS().size() == 1)
        {
            signature->appendLabelDes({getReturnType()});
        }
    }

    std::shared_ptr<TypeLabelSymbol> FunctionSymbol::getSignature() const
    {
        return signature;
    }

    std::string FunctionSymbol::getSignatureString() const
    {
        std::string paramSignature = "(";
        for (int i = 0;
             const auto &param: parameters)
        {
            paramSignature += param->getVal() + ": " + param->getTypeLabel()->briefString() +
                (param->getDefaultValue().has_value() ?
                    " = " + param->getDefaultValue().value() : "");
            if (i != parameters.size() - 1)
            {
                paramSignature += ", ";
            }
            i++;
        }
        paramSignature += ")";
        return getVal() + paramSignature + ": " + (hasReturnValue() ?
            getReturnType()->briefString() : "void");
    }

    std::string FunctionSymbol::toString() const
    {
        return "[<Function> " + getSignatureString() + "]";
    }

    std::unordered_set<std::shared_ptr<LabelSymbol>> FunctionSymbol::getLabels() const {
        return labels;
    }

    std::vector<std::shared_ptr<ParameterSymbol>> FunctionSymbol::getParameters() const {
        return parameters;
    }

    TypeOfBuiltin FunctionSymbol::getBuiltInType() const {
        return builtinType;
    }

    std::shared_ptr<ClassSymbol> FunctionSymbol::getClassSymbol() const {
        return classSymbol;
    }

    std::shared_ptr<ast::ExpressionNode> FunctionSymbol::getDefinitionNode() const
    {
        return definitionNode;
    }

    LabelMarkManager& FunctionSymbol::getLabelMarkManager()
    {
        return labelMarkManager;
    }

    FunctionType FunctionSymbol::getFunctionType() const
    {
        return functionType;
    }

    PermissionLabel FunctionSymbol::getPermissionLabel() const
    {
        const auto &permission = labelMarkManager.getPermissionLabelMark();
        return permission.has_value() ? permission.value() : classSymbol->getDefaultVisitPermission();
    }

    std::shared_ptr<Symbol> FunctionSymbol::transform(const std::string& value, const std::string& raValue,
                                                      const size_t& scopeLevel) const
    {
        return std::make_shared<FunctionSymbol>(
            getDefinitionNode(), getPos(), value, raValue, labels, parameters,
            signature, scopeLevel, builtinType, functionType, classSymbol);
    }

    bool FunctionSymbol::is(const TypeOfBuiltin& type) const
    {
        return builtinType == type;
    }

    std::shared_ptr<TypeLabelSymbol> FunctionSymbol::getFunctionTypeLabel() const
    {
        if (hasReturnValue())
        {
            return TypeLabelSymbol::funiTypeSymbol(utils::getUnknownPos(), getScopeLevel());
        }
        return TypeLabelSymbol::funcTypeSymbol(utils::getUnknownPos(), getScopeLevel());
    }

    std::shared_ptr<utils::Object> FunctionSymbol::copySelf() const
    {
        std::unordered_set<std::shared_ptr<LabelSymbol>> labels_ = {};
        for (const auto &l: getLabels())
        {
            labels_.insert(std::static_pointer_cast<LabelSymbol>(copySymbolPtr(l)));
        }
        std::vector<std::shared_ptr<ParameterSymbol>> params = {};
        for (const auto &p: getParameters())
        {
            params.push_back(std::static_pointer_cast<ParameterSymbol>(copySymbolPtr(p)));
        }
        return std::make_shared<FunctionSymbol>(
            definitionNode, getPos(), getVal(), getRaVal(), labels_,
            params, std::static_pointer_cast<TypeLabelSymbol>(copySymbolPtr(signature)),
            getScopeLevel(), getBuiltInType(), getFunctionType(),
            std::static_pointer_cast<ClassSymbol>(copySymbolPtr(getClassSymbol())));
    }

    bool FunctionSymbol::Is(const SymbolType& tp) const
    {
        return Symbol::Is(tp);
    }

    bool FunctionSymbol::IsNot(const SymbolType& tp) const
    {
        return Symbol::IsNot(tp);
    }

    const char* FunctionSymbol::GetVal() const
    {
        return Symbol::GetVal();
    }

    const char* FunctionSymbol::GetRaVal() const
    {
        return Symbol::GetRaVal();
    }

    SymbolType FunctionSymbol::GetType() const
    {
        return Symbol::GetType();
    }

    size_t FunctionSymbol::GetScopeLevel() const
    {
        return Symbol::GetScopeLevel();
    }

    void FunctionSymbol::SetScopeLevel(const size_t& level)
    {
        Symbol::SetScopeLevel(level);
    }

    const utils::IRCCPosInterface* FunctionSymbol::GetPos() const
    {
        return Symbol::GetPos();
    }

    const Symbol* FunctionSymbol::TransformToSI() const
    {
        return Symbol::TransformToSI();
    }

    IRCCTypeLabelSymbolInterface* FunctionSymbol::GetReturnTypeLabelI() const
    {
        return getReturnType().get();
    }

    void FunctionSymbol::SetReturnType(const IRCCTypeLabelSymbolInterface* typeSymbolI)
    {
        setReturnType(std::static_pointer_cast<TypeLabelSymbol>(typeSymbolI->TransformToTLSI()->copySelf()));
    }

    void FunctionSymbol::ReSetReturnType(const IRCCTypeLabelSymbolInterface* typeSymbolI)
    {
        reSetReturnType(std::static_pointer_cast<TypeLabelSymbol>(typeSymbolI->TransformToTLSI()->copySelf()));
    }

    bool FunctionSymbol::HasReturnValue() const
    {
        return hasReturnValue();
    }

    bool FunctionSymbol::HasSetReturnType() const
    {
        return hasSetReturnType();
    }

    bool FunctionSymbol::HasReturned() const
    {
        return hasReturned();
    }

    void FunctionSymbol::SetHasReturned(const bool& hasReturned)
    {
        setHasReturned(hasReturned);
    }

    void FunctionSymbol::SetFunctionType(const FunctionType& funcType)
    {
        setFunctionType(funcType);
    }

    void FunctionSymbol::SetClassSymbol(const IRCCClassSymbolInterface* classSymbolI)
    {
        setClassSymbol(std::static_pointer_cast<ClassSymbol>(classSymbolI->TransformToCSI()->copySelf()));
    }

    void FunctionSymbol::SetBuiltInType(const TypeOfBuiltin& type)
    {
        setBuiltInType(type);
    }

    void FunctionSymbol::SetSignature(const IRCCTypeLabelSymbolInterface* signatureSymbolI)
    {
        setSignature(std::static_pointer_cast<TypeLabelSymbol>(signatureSymbolI->TransformToTLSI()->copySelf()));
    }

    IRCCTypeLabelSymbolInterface* FunctionSymbol::GetSignatureSymbolI() const
    {
        return getSignature().get();
    }

    const char* FunctionSymbol::GetSignatureString() const
    {
        return getSignatureString().c_str();
    }

    IRCCFunctionSymbolInterface::Labels FunctionSymbol::GetLabels() const
    {
        Labels labels(getLabels().size());
        size_t i = 0;
        for (const auto &l: getLabels())
        {
            labels.labels[i] = l.get();
            i ++;
        }
        return labels;
    }

    IRCCFunctionSymbolInterface::ParamSymbols FunctionSymbol::GetParamSymbols() const
    {
        ParamSymbols paramSymbols (parameters.size());
        for (size_t i = 0; i < paramSymbols.count; i ++)
        {
            paramSymbols.paramSymbols[i] = parameters[i].get();
        }
        return paramSymbols;
    }

    TypeOfBuiltin FunctionSymbol::GetBuiltInType() const
    {
        return getBuiltInType();
    }

    IRCCClassSymbolInterface* FunctionSymbol::GetClassSymbolI() const
    {
        return getClassSymbol().get();
    }

    FunctionType FunctionSymbol::GetFunctionType() const
    {
        return getFunctionType();
    }

    PermissionLabel FunctionSymbol::GetPermissionLabel() const
    {
        return getPermissionLabel();
    }

    const FunctionSymbol* FunctionSymbol::TransformToFSI() const
    {
        return this;
    }

    bool FunctionSymbol::Is(const TypeOfBuiltin& type) const
    {
        return is(type);
    }

    IRCCTypeLabelSymbolInterface* FunctionSymbol::GetFunctionTypeLabelSymbolI() const
    {
        return getFunctionTypeLabel().get();
    }

    void FunctionSymbol::FreeLabels(Labels& labels_)
    {
        if (labels_.labels)
        {
            delete labels_.labels;
            labels_.labels = nullptr;
            labels_.count = 0;
        }
    }

    void FunctionSymbol::FreeParamSymbols(ParamSymbols& paramSymbols)
    {
        if (paramSymbols.paramSymbols)
        {
            delete paramSymbols.paramSymbols;
            paramSymbols.paramSymbols = nullptr;
            paramSymbols.count = 0;
        }
    }

    ClassSymbol::ClassSymbol(
        const utils::Pos &pos,
        const std::string &name,
        const std::string &nameID,
        const std::unordered_set<std::shared_ptr<LabelSymbol>> &labels,
        const size_t scopeLevel,
        const SymbolTableManager &symbolTable,
        std::vector<std::shared_ptr<ClassSymbol>> baseClasses,
        const std::vector<std::shared_ptr<ClassSymbol>>& derivedClasses)
        : Symbol(pos, SymbolType::CLASS, name, nameID, scopeLevel),
          baseClasses(std::move(baseClasses)), derivedClasses(derivedClasses)
    {
        labelMarkManager.markLabels(labels);
        visitPermission = labelMarkManager.isPermissionLabelMarked(PermissionLabel::PUBLIC) ? PermissionLabel::PUBLIC
                              : labelMarkManager.isPermissionLabelMarked(PermissionLabel::PRIVATE) ? PermissionLabel::PRIVATE
                              : PermissionLabel::PROTECTED;
        if (const auto &typeLabelMarks = labelMarkManager.getTypeLabelMarks();
            typeLabelMarks.size() == 1)
        {
            const auto &parentClassName = *typeLabelMarks.begin();
            if (const auto &[level, symbol] = symbolTable.findByName(parentClassName);
                level >= 0 && symbol)
            {
                const auto &parentClassSymbol = TypeLabelSymbol::getCustomClassSymbol(symbol->getRaVal());
                setBaseClasses(parentClassSymbol);
                parentClassSymbol->setDerivedClasses(std::make_shared<ClassSymbol>(*this));
            }
        } else if (typeLabelMarks.size() > 1)
        {
            // TODO: To improve the error info.
            throw std::runtime_error("A class is not allowed to inherit from multiple parent classes.");
        }
    }

    ClassSymbol::ClassSymbol(
        const utils::Pos &pos,
        const std::string &name,
        const std::string &nameID,
        LabelMarkManager labelMarkManager,
        const size_t scopeLevel,
        std::vector<std::shared_ptr<ClassSymbol>> baseClasses,
        const std::vector<std::shared_ptr<ClassSymbol>>& derivedClasses,
        const std::shared_ptr<SymbolTable> &constructors,
        const std::shared_ptr<SymbolTable> &staticMembers,
        const bool &collectionFinished,
        const PermissionLabel &visitPermission)
        : Symbol(pos, SymbolType::CLASS, name, nameID, scopeLevel),
          baseClasses(std::move(baseClasses)), derivedClasses(derivedClasses),
          staticMembers(staticMembers), constructors(constructors),
          labelMarkManager(std::move(labelMarkManager)), collectionFinished(collectionFinished),
          visitPermission(visitPermission), scope_level_(scopeLevel) {}

    ClassSymbol::~ClassSymbol() {}

    IRCCVariableSymbolInterface::~IRCCVariableSymbolInterface() {}

    IRCCFunctionSymbolInterface::~IRCCFunctionSymbolInterface() {}

    IRCCClassSymbolInterface::~IRCCClassSymbolInterface() {}

    std::vector<std::shared_ptr<ClassSymbol>> &ClassSymbol::getBaseClasses() {
        return baseClasses;
    }

    IRCCClassSymbolInterface::Classes ClassSymbol::GetBaseClasses()
    {
        Classes result(baseClasses.size());
        for (size_t i = 0; i < result.count; ++i) {
            result.classes[i] = baseClasses[i].get();
        }
        return result;
    }

    void ClassSymbol::FreeClasses(Classes& classes)
    {
        if (classes.classes)
        {
            delete classes.classes;
            classes.classes = nullptr;
            classes.count = 0;
        }
    }

    std::vector<std::shared_ptr<ClassSymbol>> &ClassSymbol::getDerivedClasses() {
        return derivedClasses;
    }

    IRCCClassSymbolInterface::Classes ClassSymbol::GetDeriveClasses()
    {
        Classes result (derivedClasses.size());
        for (size_t i = 0; i < result.count; i ++)
        {
            result.classes[i] = derivedClasses[i].get();
        }
        return result;
    }

    std::shared_ptr<SymbolTable> &ClassSymbol::getMembers() {
        return members;
    }

    IRCCSymbolTableInterface* ClassSymbol::GetMembers() const
    {
        return members.get();
    }

    std::shared_ptr<SymbolTable>& ClassSymbol::getStaticMembers()
    {
        return staticMembers;
    }

    IRCCSymbolTableInterface* ClassSymbol::GetStaticMembers() const
    {
        return staticMembers.get();
    }

    std::shared_ptr<SymbolTable>& ClassSymbol::getConstructors()
    {
        return constructors;
    }

    IRCCSymbolTableInterface* ClassSymbol::GetConstructors() const
    {
        return constructors.get();
    }

    void ClassSymbol::addMember(const std::shared_ptr<Symbol>& symbol, const bool& sysDefined) const
    {
        switch (symbol->getType())
        {
        case SymbolType::VARIABLE:
            {
                if (const auto &varSymbol = std::static_pointer_cast<VariableSymbol>(symbol);
                    varSymbol->getLabelMarkManager().isLifeCycleLabelMarked(LifeCycleLabel::STATIC) ||
                    varSymbol->getLabelMarkManager().isLifeCycleLabelMarked(LifeCycleLabel::ORDINARY))
                {
                    staticMembers->insertByName(varSymbol, sysDefined);
                } else
                {
                    members->insertByName(varSymbol, sysDefined);
                }
            } break;
        case SymbolType::FUNCTION:
            {
                if (const auto funcSymbol = std::static_pointer_cast<FunctionSymbol>(symbol))
                {
                    switch (funcSymbol->getFunctionType())
                    {
                    case FunctionType::CONSTRUCTOR:
                        {
                            constructors->insertByName(funcSymbol, sysDefined);
                        } break;
                    case FunctionType::METHOD:
                        {
                            if (funcSymbol->getLabelMarkManager().isLifeCycleLabelMarked(LifeCycleLabel::STATIC) ||
                                funcSymbol->getLabelMarkManager().isLifeCycleLabelMarked(LifeCycleLabel::ORDINARY))
                            {
                                staticMembers->insertByName(funcSymbol, sysDefined);
                            } else
                            {
                                members->insertByName(funcSymbol, sysDefined);
                            }
                        } break;
                    case FunctionType::FUNCTION:
                    case FunctionType::ANONYMOUS:
                    default: throw std::runtime_error("Invalid function type");
                    }
                }
            } break;
        case SymbolType::CLASS:
            {
                if (const auto &classSymbol = std::static_pointer_cast<ClassSymbol>(symbol);
                    classSymbol->getLabelMarkManager().isLifeCycleLabelMarked(LifeCycleLabel::STATIC) ||
                    classSymbol->getLabelMarkManager().isLifeCycleLabelMarked(LifeCycleLabel::ORDINARY))
                {
                    staticMembers->insertByName(classSymbol, sysDefined);
                } else
                {
                    members->insertByName(classSymbol, sysDefined);
                }
            } break;
        default:
            throw std::runtime_error("Invalid class member symbol type: " + symbolTypeToString(symbol->getType()));
        }
    }

    void ClassSymbol::_addDerivedClass(const std::shared_ptr<ClassSymbol>& classSymbol)
    {
        derivedClasses.push_back(classSymbol);
    }

    void ClassSymbol::_addBaseClass(const std::shared_ptr<ClassSymbol>& classSymbol)
    {
        baseClasses.push_back(classSymbol);
    }

    void ClassSymbol::setDerivedClasses(const std::shared_ptr<ClassSymbol>& classSymbol)
    {
        // This function is used to set the derived symbols of such symbols,
        // and it will add the passed-in derived symbol together with the symbols
        // of its own base class.
        // That is to say, the passed-in symbol is a newly created one.
        for (const auto &bcs: baseClasses)
        {
            bcs->_addDerivedClass(classSymbol);
        }
        _addDerivedClass(classSymbol);
    }

    void ClassSymbol::setBaseClasses(const std::shared_ptr<ClassSymbol>& classSymbol)
    {
        // This function is used to set the base class symbol of such symbols,
        // and it will also set the base class symbol of the passed-in base class symbol.
        // That is to say, this symbol is a newly created one.
        for (const auto &bcs: classSymbol->getBaseClasses())
        {
            _addBaseClass(bcs);
        }
        _addBaseClass(classSymbol);
    }

    bool ClassSymbol::hasInheritClass() const
    {
        return !baseClasses.empty();
    }

    std::shared_ptr<ClassSymbol> ClassSymbol::getDirectlyInheritedClassSymbol() const
    {
        if (hasInheritClass())
        {
            return baseClasses.back();
        }
        return nullptr;
    }

    bool ClassSymbol::hasMember(const std::string& name) const
    {
        return members->contains(name) || staticMembers->contains(name);
    }

    std::pair<std::shared_ptr<Symbol>, LifeCycleLabel> ClassSymbol::findMemberSymbol(const std::string& name) const
    {
        if (const auto &instMember = members->find(name))
        {
            return {instMember, LifeCycleLabel::INSTANCE};
        }
        if (const auto &staticMember = staticMembers->find(name))
        {
            return {staticMember, LifeCycleLabel::STATIC};
        }
        return {nullptr, LifeCycleLabel::COUNT};
    }

    std::pair<std::shared_ptr<Symbol>, LifeCycleLabel> ClassSymbol::findMemberSymbolInPermission(
        const std::string& name) const
    {
        const auto &[member, label] = findMemberSymbol(name);
        if (!member) return {nullptr, label};
        auto getPermissionLabel = [&]
        {
            if (member->getType() == SymbolType::VARIABLE) {
                return std::static_pointer_cast<VariableSymbol>(member)->getPermissionLabel();
            }
            if (member->getType() == SymbolType::FUNCTION) {
                return std::static_pointer_cast<FunctionSymbol>(member)->getPermissionLabel();
            }
            if (member->getType() == SymbolType::CLASS) {
                return std::static_pointer_cast<ClassSymbol>(member)->getDefaultVisitPermission();
            }
            throw std::runtime_error("Invalid symbol type");
        };
        const auto &permission = getPermissionLabel();
        return checkVisitPermissionAllowed(visitPermission, permission) ?
                   std::pair{member, label} : std::pair{nullptr, label};
    }

    std::shared_ptr<TypeLabelSymbol> ClassSymbol::getClassTypeLabelSymbol(const utils::Pos& pos) const
    {
        return std::make_shared<TypeLabelSymbol>(pos, getVal(), getScopeLevel(), getRaVal());
    }

    LabelMarkManager& ClassSymbol::getLabelMarkManager()
    {
        return labelMarkManager;
    }

    void ClassSymbol::setCollectionFinished()
    {
        collectionFinished = true;
    }

    void ClassSymbol::setVisitPermission(const PermissionLabel& permission)
    {
        visitPermission = permission;
    }

    void ClassSymbol::setDefaultVisitPermission()
    {
        visitPermission = getDefaultVisitPermission();
    }

    PermissionLabel ClassSymbol::getDefaultVisitPermission() const
    {
        const auto &permission = labelMarkManager.getPermissionLabelMark();
        return  permission.has_value() ?
                    permission.value() :
                    PermissionLabel::PROTECTED;
    }

    bool ClassSymbol::hasCollectionFinished() const
    {
        return collectionFinished;
    }

    PermissionLabel ClassSymbol::getVisitPermission() const
    {
        return visitPermission;
    }

    std::shared_ptr<Symbol> ClassSymbol::transform(const std::string& value, const std::string& raValue,
                                                   const size_t& scopeLevel) const
    {
        return std::make_shared<ClassSymbol>(
            getPos(), value, raValue,
            labelMarkManager, scopeLevel, baseClasses, derivedClasses, constructors,
            staticMembers, collectionFinished, visitPermission);
    }

    std::string ClassSymbol::toString() const
    {
        return "[Class(" + getRaVal() + "): " + getVal() + "]";
    }

    std::shared_ptr<utils::Object> ClassSymbol::copySelf() const
    {
        std::vector<std::shared_ptr<ClassSymbol>> baseClasses_ = {};
        for (const auto &bc: baseClasses)
        {
            baseClasses_.push_back(std::static_pointer_cast<ClassSymbol>(copySymbolPtr(bc)));
        }
        std::vector<std::shared_ptr<ClassSymbol>> deriveClasses_ = {};
        for (const auto &dc: derivedClasses)
        {
            deriveClasses_.push_back(std::static_pointer_cast<ClassSymbol>(copySymbolPtr(dc)));
        }
        return std::make_shared<ClassSymbol>(getPos(), getVal(), getRaVal(), labelMarkManager,
            getScopeLevel(), baseClasses_, deriveClasses_,
            constructors,
            staticMembers,
            collectionFinished, visitPermission);
    }

    bool ClassSymbol::relatedTo(const std::shared_ptr<ClassSymbol>& other) const
    {
        bool result = false;
        if (equalWith(other) ||
            std::ranges::any_of(baseClasses.begin(), baseClasses.end(),
                [other](const auto &bc)
        {
            return bc->equalWith(other);
        })) {
            result = true;
        } else {
            result = std::ranges::any_of(derivedClasses.begin(), derivedClasses.end(),
                [other](const auto &dc)
            {
                return dc->equalWith(other);
            });
        }
        return result;
    }


    bool ClassSymbol::isSupperClassOf(const std::shared_ptr<ClassSymbol>& other, const bool &restrict) const
    {
        if (!restrict && equalWith(other)) return true;
        return std::ranges::any_of(derivedClasses.begin(), derivedClasses.end(), [other](const auto &dc)
        {
            return dc->equalWith(other);
        });
    }

    void ClassSymbol::AddMember(const IRCCSymbolInterface* symbolI) const
    {
        addMember(std::static_pointer_cast<ClassSymbol>(symbolI->TransformToSI()->copySelf()), true);
    }

    void ClassSymbol::SetDerivedClasses(const IRCCClassSymbolInterface* classSymbolI)
    {
        setDerivedClasses(std::static_pointer_cast<ClassSymbol>(classSymbolI->TransformToCSI()->copySelf()));
    }

    void ClassSymbol::SetBaseClasses(const IRCCClassSymbolInterface* classSymbolI)
    {
        setBaseClasses(std::static_pointer_cast<ClassSymbol>(classSymbolI->TransformToCSI()->copySelf()));
    }

    bool ClassSymbol::HasInheritClass() const
    {
        return hasInheritClass();
    }

    IRCCClassSymbolInterface* ClassSymbol::GetDirectlyInheritedClassSymbolI() const
    {
        return getDirectlyInheritedClassSymbol().get();
    }

    bool ClassSymbol::HasMember(const char* name) const
    {
        return hasMember(name);
    }

    IRCCClassSymbolInterface::MemberSymbolResult ClassSymbol::FindMemberSymbolI(const char* name) const
    {
        const auto &[symbol, lifeCycleLabel] = findMemberSymbol(name);
        return MemberSymbolResult(symbol.get(), lifeCycleLabel);
    }

    IRCCClassSymbolInterface::MemberSymbolResult ClassSymbol::FindMemberSymbolIInPermission(const char* name) const
    {
        const auto &[symbol, lifeCycleLabel] = findMemberSymbolInPermission(name);
        return MemberSymbolResult(symbol.get(), lifeCycleLabel);
    }

    IRCCTypeLabelSymbolInterface* ClassSymbol::GetClassTypeLabelSymbolI(const utils::IRCCPosInterface* posI) const
    {
        return getClassTypeLabelSymbol(*posI->TransformToPI()).get();
    }

    void ClassSymbol::SetCollectionFinished()
    {
        setCollectionFinished();
    }

    void ClassSymbol::SetVisitPermission(const PermissionLabel& permission)
    {
        setVisitPermission(permission);
    }

    void ClassSymbol::SetDefaultVisitPermission()
    {
        setDefaultVisitPermission();
    }

    PermissionLabel ClassSymbol::GetDefaultVisitPermission() const
    {
        return getDefaultVisitPermission();
    }

    bool ClassSymbol::HasCollectionFinished() const
    {
        return hasCollectionFinished();
    }

    PermissionLabel ClassSymbol::GetVisitPermission() const
    {
        return getVisitPermission();
    }

    const ClassSymbol* ClassSymbol::TransformToCSI() const
    {
        return this;
    }

    bool ClassSymbol::RelatedTo(const IRCCClassSymbolInterface* otherI) const
    {
        return relatedTo(std::static_pointer_cast<ClassSymbol>(otherI->TransformToCSI()->copySelf()));
    }

    bool ClassSymbol::IsSupperClassOf(const IRCCClassSymbolInterface* otherI, const bool& restrict) const
    {
        return isSupperClassOf(std::static_pointer_cast<ClassSymbol>(otherI->TransformToCSI()->copySelf()));
    }

    bool ClassSymbol::Is(const SymbolType& tp) const
    {
        return Symbol::Is(tp);
    }

    bool ClassSymbol::IsNot(const SymbolType& tp) const
    {
        return Symbol::IsNot(tp);
    }

    const char* ClassSymbol::GetVal() const
    {
        return Symbol::GetVal();
    }

    const char* ClassSymbol::GetRaVal() const
    {
        return Symbol::GetRaVal();
    }

    SymbolType ClassSymbol::GetType() const
    {
        return Symbol::GetType();
    }

    size_t ClassSymbol::GetScopeLevel() const
    {
        return Symbol::GetScopeLevel();
    }

    void ClassSymbol::SetScopeLevel(const size_t& level)
    {
        Symbol::SetScopeLevel(level);
    }

    const utils::IRCCPosInterface* ClassSymbol::GetPos() const
    {
        return Symbol::GetPos();
    }

    const Symbol* ClassSymbol::TransformToSI() const
    {
        return Symbol::TransformToSI();
    }

    SymbolTable::~SymbolTable() {}

    IRCCSymbolTableInterface::~IRCCSymbolTableInterface() {}

    std::shared_ptr<Symbol> SymbolTable::insertByName(const std::shared_ptr<Symbol> &symbol, const bool& sysDefined) {
        const std::string symbolName = symbol->getVal();
        if (symbolName != "_" && table.contains(symbolName)) {
            throw std::runtime_error("Symbol " + symbolName + " already exists");
        }
        table[symbolName] = {static_cast<int>(nameIndex.size()), symbol};
        nameIndex.push_back(symbolName);
        if (sysDefined)
        {
            sysDefinitionRecord.push_back(symbol->getRaVal());
        }
        return symbol;
    }

    void SymbolTable::InsertByName(IRCCSymbolInterface* symbolI)
    {
        const auto &symbol = std::static_pointer_cast<Symbol>(symbolI->TransformToSI()->copySelf());
        insertByName(symbol, true);
    }

    std::shared_ptr<Symbol> SymbolTable::insertByRID(const std::shared_ptr<Symbol> &symbol, const bool& sysDefined) {
        const std::string rid = symbol->getRaVal();
        if (symbol->getVal() != "_" && table.contains(rid)) {
            throw std::runtime_error("Symbol " + rid + " already exists");
        }
        table[rid] = {static_cast<int>(nameIndex.size()), symbol};
        nameIndex.push_back(rid);
        if (sysDefined)
        {
            sysDefinitionRecord.push_back(rid);
        }
        return symbol;
    }

    std::shared_ptr<Symbol> SymbolTable::find(const std::string &name) const {
        const auto & it = table.find(name);
        return it != table.end() ? it->second.second : nullptr;
    }

    IRCCSymbolInterface* SymbolTable::Find(const char* name)
    {
        const auto &res = find(name);
        return res ? res.get() : nullptr;
    }

    bool SymbolTable::contains(const std::string &name) const {
        return table.contains(name);
    }

    bool SymbolTable::Contains(const char* name)
    {
        return contains(name);
    }

    std::unordered_map<std::string, std::pair<int, std::shared_ptr<Symbol>>> SymbolTable::getTable() const {
        return table;
    }

    std::vector<std::string> SymbolTable::getNameIndex() const {
        return nameIndex;
    }

    void SymbolTable::remove(const std::string& name)
    {
        if (const auto &it = table.find(name);
            it != table.end())
        {
            nameIndex.erase(nameIndex.begin() + it->second.first);
            table.erase(name);
        } else
        {
            throw std::runtime_error("SymbolTable::remove:: cannot remove '" + name + "' because it does not exist.");
        }
    }

    void SymbolTable::Remove(const char* name)
    {
        remove(name);
    }

    std::shared_ptr<utils::Object> SymbolTable::copySelf() const
    {
        const auto &copied = std::make_shared<SymbolTable>();
        for (const auto &s : *this)
        {
            copied->insertByName(std::static_pointer_cast<Symbol>(Symbol::copySymbolPtr(s)),
                std::ranges::find(sysDefinitionRecord, s->getRaVal())
                != sysDefinitionRecord.end());
        }
        return copied;
    }

    void SymbolTableManager::appendScope(const std::shared_ptr<SymbolTable>& nameScope,
        const std::shared_ptr<SymbolTable>& idScope)
    {
        scopes.emplace_back(nameScope, idScope);
    }

    SymbolTableManager::SymbolTableManager() {
        scopes.emplace_back(std::make_shared<SymbolTable>(),
            std::make_shared<SymbolTable>());
        currentScopeLevel_ = 0; // 全局作用域
    }

    SymbolTableManager::~SymbolTableManager() {}

    IRCCSymbolTableManagerInterface::~IRCCSymbolTableManagerInterface() {}

    SymbolTable &SymbolTableManager::currentNameMapScope() const
    {
        return *scopes[currentScopeLevel_].first;
    }

    IRCCSymbolTableInterface* SymbolTableManager::CurrentNameMapScope() const
    {
        return &currentNameMapScope();
    }

    SymbolTable &SymbolTableManager::currentRIDMapScope() const
    {
        return *scopes[currentScopeLevel_].second;
    }

    IRCCSymbolTableInterface* SymbolTableManager::CurrentRIDMapScope() const
    {
        return &currentRIDMapScope();
    }

    std::pair<SymbolTable, SymbolTable> SymbolTableManager::currentScopeCopied() const
    {
        return {currentNameMapScope(), currentRIDMapScope()};
    }

    void SymbolTableManager::enterScope() {
        scopes.insert(scopes.begin() + static_cast<int>(currentScopeLevel_) + 1,
                      {std::make_shared<SymbolTable>(),
                          std::make_shared<SymbolTable>()});
        currentScopeLevel_++;
    }

    void SymbolTableManager::EnterScope()
    {
        enterScope();
    }

    void SymbolTableManager::enterScope(size_t scopeLevel) {
        if (scopeLevel > currentScopeLevel_) {
            throw std::runtime_error("Invalid scope level");
        }
        currentScopeLevel_ = scopeLevel;
    }

    void SymbolTableManager::enterTopScope() {
        currentScopeLevel_ = scopes.size() - 1;
    }

    void SymbolTableManager::enterGlobalScope() {
        enterScope(0);
    }

    void SymbolTableManager::exitScope() {
        if (currentScopeLevel_ > 0) {
            // 删除当前作用域中自定义的类型标签
            for (const auto & [_, symbol]:
                 currentNameMapScope() | std::views::values) {
                if (symbol->getType() == SymbolType::CLASS) {
                    TypeLabelSymbol::deleteCustomType(symbol->getRaVal());
                }
            }
            scopes.erase(scopes.begin() + static_cast<int>(currentScopeLevel_));
            currentScopeLevel_--;
        }
        else {
            throw std::runtime_error("Can not exit global scope");
        }
    }

    void SymbolTableManager::insert(const std::shared_ptr<Symbol> &symbol, const bool &systemDefined) const
    {
        scopes[symbol->getScopeLevel()].first->insertByName(symbol, systemDefined);
        scopes[symbol->getScopeLevel()].second->insertByRID(symbol, systemDefined);
    }

    void SymbolTableManager::removeByName(const std::string& name, const std::optional<size_t>& specifiedLevel) const
    {
        if (const auto &[level, symbol] = findByName(name, specifiedLevel);
            level >= 0 && symbol)
        {
            scopes[level].second->remove(symbol->getRaVal());
            scopes[level].first->remove(name);
        } else
        {
            throw std::runtime_error("SymbolTableManager::remove:: cannot remove '" + name + "' because it does not exist.");
        }
    }

    void SymbolTableManager::removeByRID(const std::string& rid, const std::optional<size_t>& specifiedLevel) const
    {
        if (const auto &[level, symbol] = findByName(rid, specifiedLevel);
            level >= 0 && symbol)
        {
            scopes[level].first->remove(symbol->getVal());
            scopes[level].second->remove(rid);
        } else
        {
            throw std::runtime_error("SymbolTableManager::remove:: cannot remove '" + rid + "' because it does not exist.");
        }
    }

    std::pair<long long int, std::shared_ptr<Symbol>> SymbolTableManager::findByName(const std::string &name, const std::optional<size_t>& specifiedLevel) const {
        for (int i = static_cast<int>(specifiedLevel.has_value()
            ? specifiedLevel.value() : currentScopeLevel_); i >= 0; --i) {
            const auto &scope = scopes[i].first;
            if (const auto &it = scope->find(name)) {
                return {i, it};
            }
        }
        return {-1, nullptr};
    }

    std::pair<long long int, std::shared_ptr<Symbol>> SymbolTableManager::findByRID(const std::string &rid) const {
        for (int i = static_cast<int>(currentScopeLevel_); i >= 0; --i) {
            const auto &scope = scopes[i].second;
            if (const auto &it = scope->find(rid)) {
                return {i, it};
            }
        }
        return {-1, nullptr};
    }

    std::shared_ptr<Symbol> SymbolTableManager::findInCurrentScope(const std::string &name) const {
        return currentNameMapScope().find(name);
    }

    bool SymbolTableManager::containsName(const std::string &name) const {
        return std::ranges::any_of(scopes.rbegin(), scopes.rend(),
            [name](const auto &it)
        {
            return it.first->contains(name);
        });
    }

    bool SymbolTableManager::currentScopeContains(const std::string &name) const {
        return currentNameMapScope().contains(name);
    }

    size_t SymbolTableManager::curScopeLevel() const {
        return currentScopeLevel_;
    }

    std::shared_ptr<utils::Object> SymbolTableManager::copySelf() const
    {
        const auto &stm = std::make_shared<SymbolTableManager>();
        for (const auto &[nameScope,
            idScope]: scopes)
        {
            stm->appendScope(nameScope, idScope);
        }
        stm->currentScopeLevel_ = currentScopeLevel_;
        return stm;
    }

    void SymbolTableManager::EnterScope(const size_t& scopeLevel)
    {
        enterScope(scopeLevel);
    }

    void SymbolTableManager::EnterTopScope()
    {
        enterScope();
    }

    void SymbolTableManager::EnterGlobalScope()
    {
        enterGlobalScope();
    }

    void SymbolTableManager::ExitScope()
    {
        exitScope();
    }

    void SymbolTableManager::Insert(IRCCSymbolInterface* symbolI) const
    {
        insert(std::static_pointer_cast<Symbol>(symbolI->TransformToSI()->copySelf()), true);
    }

    void SymbolTableManager::RemoveByName(const char* name, const size_t* specifiedLevel) const
    {
        removeByName(name, specifiedLevel ? std::optional(*specifiedLevel) : std::nullopt);
    }

    void SymbolTableManager::RemoveByRID(const char* rid, const size_t* specifiedLevel) const
    {
        removeByRID(rid, specifiedLevel ? std::optional(*specifiedLevel) : std::nullopt);
    }

    IRCCSymbolInterface* SymbolTableManager::FindByName(const char* name, const size_t* specifiedLevel) const
    {
        return findByName(name, specifiedLevel ? std::optional(*specifiedLevel) : std::nullopt).second.get();
    }

    IRCCSymbolInterface* SymbolTableManager::FindByRID(const char* rid) const
    {
        return findByRID(rid).second.get();
    }

    IRCCSymbolInterface* SymbolTableManager::FindInCurrentScope(const char* name) const
    {
        return findInCurrentScope(name).get();
    }

    bool SymbolTableManager::ContainsName(const char* name) const
    {
        return containsName(name);
    }

    bool SymbolTableManager::CurrentScopeContains(const char* name) const
    {
        return currentScopeContains(name);
    }

    size_t SymbolTableManager::CurScopeLevel() const
    {
        return curScopeLevel();
    }

    std::pair<long long int, std::shared_ptr<VariableSymbol>>
    SymbolTableManager::findVariableSymbolByName(const std::string &name) const {
        for (int i = static_cast<int>(currentScopeLevel_); i >= 0; --i) {
            const auto &scope = scopes[i].first;
            if (const auto &it = scope->find(name)) {
                if (it->getType() == SymbolType::VARIABLE) {
                    return {i, std::static_pointer_cast<VariableSymbol>(it)};
                }
            }
        }
        return {-1, nullptr};
    }

    std::pair<long long int, std::shared_ptr<VariableSymbol>>
    SymbolTableManager::findVariableSymbolByRID(const std::string &rid) const {
        for (int i = static_cast<int>(currentScopeLevel_); i >= 0; --i) {
            const auto &scope = scopes[i].second;
            if (const auto &it = scope->find(rid)) {
                if (it->getType() == SymbolType::VARIABLE) {
                    return {i, std::static_pointer_cast<VariableSymbol>(it)};
                }
            }
        }
        return {-1, nullptr};
    }
}

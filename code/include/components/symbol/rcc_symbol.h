//
// Created by RestRegular on 2025/7/12.
//

#ifndef RCC_SYMBOL_H
#define RCC_SYMBOL_H

#include <memory>
#include <unordered_set>
#include <ranges>

#include "rcc_symbol.h"
#include "rcc_symbol.h"
#include "rcc_symbol.h"
#include "../../analyzer/rcc_ast_components.h"
#include "../../lib/rcc_utils.h"

namespace symbol {
    class SymbolTable;

    enum class SymbolType: int;
    enum class LabelType: int;
    enum class ParamType: int;
    enum class ArgumentType: int;
    enum class FunctionType: int;
    enum class BuiltinTypeLabel: int;
    enum class PermissionLabel: int;
    enum class LifeCycleLabel: int;
    enum class RestrictionLabel: int;
    enum class ObjectOrientedLabel: int;

    class Symbol;
    class LabelSymbol;
    class VariableSymbol;
    class ParameterSymbol;
    class FunctionSymbol;
    class ClassSymbol;
    class SymbolTableManager;

    extern std::unordered_map<std::string, LabelType> labelTypeMap;

    std::string symbolTypeToString(const SymbolType &type);
    std::string symbolTypeToFormatString(const SymbolType &type);
    std::string labelTypeToString(const LabelType &type);
    std::string builtinTypeLabelToString(const BuiltinTypeLabel &label);
    std::string permissionLabelToString(const PermissionLabel &label);
    std::string objectOrientedLabelToString(const ObjectOrientedLabel &label);
    std::string restrictionLabelToString(const RestrictionLabel &label);
    std::string lifeCycleLabelToString(const LifeCycleLabel &label);
    std::string functionTypeToString(const FunctionType &type);
    std::string functionTypeToFormatString(const FunctionType &type);
    std::string paramTypeToString(const ParamType &type);
    std::string paramTypeToFormatString(const ParamType &type);

    LabelType getLabelTypeByName(const std::string &name);
    PermissionLabel getPermissionLabelByName(const std::string &name);
    RestrictionLabel getRestrictionLabelByName(const std::string &name);
    LifeCycleLabel getLifeCycleLabelByName(const std::string &name);
    ObjectOrientedLabel getObjectOrientedLabelByName(const std::string &name);

    bool checkVisitPermissionAllowed(const PermissionLabel &visitPermission, const PermissionLabel &permission);

    class Symbol: public utils::Object {
        utils::Pos pos;
        SymbolType type;
        std::string value;
        std::string raValue;
        size_t scopeLevel;
    public:
        Symbol(const utils::Pos &pos,
            const SymbolType &symbolType,
            const std::string &symbolValue,
            const std::string &symbolRaValue,
            const size_t &scopeLevel);

        [[nodiscard]] bool is(const SymbolType &symbolType) const;
        [[nodiscard]] bool isNot(const SymbolType &symbolType) const;
        [[nodiscard]] std::string getRaVal() const;
        [[nodiscard]] SymbolType getType() const;
        [[nodiscard]] std::string getVal() const;
        [[nodiscard]] size_t getScopeLevel() const;
        void setScopeLevel(const size_t& level);
        [[nodiscard]] utils::Pos getPos() const;
        std::string toString() const override;

        bool equalWith(const Symbol &other) const;
        virtual bool equalWith(const std::shared_ptr<Symbol> &other) const;

        virtual std::shared_ptr<Symbol> transform(
            const std::string &value, const std::string &raValue, const size_t &scopeLevel = -1)const = 0;
    };

    class LabelSymbol: public Symbol {
        bool isBuiltIn_;
        LabelType labelType;
    public:
        LabelSymbol(const utils::Pos &pos,
            const std::string &name, const std::string &raValue,
            const size_t &scopeLevel, const LabelType &labelType=static_cast<LabelType>(0));

        [[nodiscard]] static bool isBuiltInLabel(const std::string &name);

        [[nodiscard]] LabelType getLabelType() const;
        [[nodiscard]] bool isBuiltIn() const;
        std::string toString() const override;

        std::shared_ptr<Symbol> transform(
            const std::string& value, const std::string& raValue, const size_t &scopeLevel = -1) const override;
    };

    class TypeLabelSymbol final : public LabelSymbol {
        [[nodiscard]] static std::string getTypeLabelRaCode(
            const std::string &name,
            const std::string &raValue);
        static std::unordered_set<std::string> builtInTypes;
        static std::unordered_map<std::string, std::string> customTypeMap;
        static std::unordered_map<std::string, std::shared_ptr<ClassSymbol>> customClassSymbolMap;
    public:
        TypeLabelSymbol(const utils::Pos &pos, const std::string &name,
            const size_t &scopeLevel, const std::string &uid = "");

        [[nodiscard]] static bool isTypeLabel(const std::string &name);
        [[nodiscard]] static bool isBuiltInType(const std::string &name);
        [[nodiscard]] static bool isCustomType(const std::string &uid);
        static void createCustomType(const std::string &name, const std::string &uid, const std::shared_ptr<ClassSymbol> &classSymbol);
        static void createCustomType(const std::shared_ptr<ClassSymbol> &classSymbol);
        static void deleteCustomType(const std::string &uid);
        static std::shared_ptr<ClassSymbol> getCustomClassSymbol(const std::string &uid);
        static std::shared_ptr<TypeLabelSymbol> getCustomTypeLabelSymbol(const std::string &uid, const size_t &scopeLevel);

        [[nodiscard]] bool is(const std::string &name) const;
        [[nodiscard]] bool isNot(const std::string &name) const;
        [[nodiscard]] bool isCustomType() const;
        [[nodiscard]] std::shared_ptr<ClassSymbol> getCustomClassSymbol() const;
        [[nodiscard]] bool isIterable() const;
        bool equalWith(const std::shared_ptr<Symbol>& other) const override;
        std::shared_ptr<Symbol> transform(const std::string& value, const std::string& raValue,
                                          const size_t& scopeLevel = -1) const override;

        [[nodiscard]] static std::shared_ptr<TypeLabelSymbol> intTypeSymbol(const utils::Pos &pos, const size_t &scopeLevel);
        [[nodiscard]] static std::shared_ptr<TypeLabelSymbol> floatTypeSymbol(const utils::Pos &pos, const size_t &scopeLevel);
        [[nodiscard]] static std::shared_ptr<TypeLabelSymbol> strTypeSymbol(const utils::Pos &pos, const size_t &scopeLevel);
        [[nodiscard]] static std::shared_ptr<TypeLabelSymbol> boolTypeSymbol(const utils::Pos &pos, const size_t &scopeLevel);
        [[nodiscard]] static std::shared_ptr<TypeLabelSymbol> voidTypeSymbol(const utils::Pos &pos, const size_t &scopeLevel);
        [[nodiscard]] static std::shared_ptr<TypeLabelSymbol> nulTypeSymbol(const utils::Pos &pos, const size_t &scopeLevel);
        [[nodiscard]] static std::shared_ptr<TypeLabelSymbol> anyTypeSymbol(const utils::Pos &pos, const size_t &scopeLevel);
        [[nodiscard]] static std::shared_ptr<TypeLabelSymbol> charTypeSymbol(const utils::Pos &pos, const size_t &scopeLevel);
        [[nodiscard]] static std::shared_ptr<TypeLabelSymbol> listTypeSymbol(const utils::Pos &pos, const size_t &scopeLevel);
        [[nodiscard]] static std::shared_ptr<TypeLabelSymbol> dictTypeSymbol(const utils::Pos &pos, const size_t &scopeLevel);
        [[nodiscard]] static std::shared_ptr<TypeLabelSymbol> flagTypeSymbol(const utils::Pos &pos, const size_t &scopeLevel);
        [[nodiscard]] static std::shared_ptr<TypeLabelSymbol> seriesTypeSymbol(const utils::Pos &pos, const size_t &scopeLevel);
        [[nodiscard]] static std::shared_ptr<TypeLabelSymbol> funcTypeSymbol(const utils::Pos &pos, const size_t &scopeLevel);
        [[nodiscard]] static std::shared_ptr<TypeLabelSymbol> funiTypeSymbol(const utils::Pos &pos, const size_t &scopeLevel);
        [[nodiscard]] static std::shared_ptr<TypeLabelSymbol> clasTypeSymbol(const utils::Pos &pos, const size_t &scopeLevel);

        [[nodiscard]] static std::shared_ptr<TypeLabelSymbol> getTypeLabelSymbolByStr(const std::string &str, const size_t &scopeLevel);
    };

    class LabelMarkManager
    {
        int permissionLabelMark = -1;
        int objectOrientedLabelMarks = 0;
        int lifeCycleLabelMarks = 0;
        int restrictionLabelMarks = 0;
        std::unordered_set<std::string> typeLabelMarks{};
        std::string resultDisplay = "";
    public:
        LabelMarkManager(){}
        void markLabels(const std::unordered_set<std::shared_ptr<LabelSymbol>> &labels);
        void markLabel(const std::shared_ptr<LabelSymbol> &label, const bool& refresh = false);

        bool isPermissionLabelMarked(const PermissionLabel &permissionLabel) const;
        bool isObjectOrientedLabelMarked(const ObjectOrientedLabel &objectOrientedLabel) const;
        bool isLifeCycleLabelMarked(const LifeCycleLabel &lifeCycleLabel) const;
        bool isRestrictionLabelMarked(const RestrictionLabel &restrictionLabel) const;
        bool isTypeLabelMarked(const std::string &typeLabel) const;

        void cancelPermissionLabelMark();
        void cancelObjectOrientedLabelMark(const ObjectOrientedLabel &objectOrientedLabel);
        void cancelLifeCycleLabelMark(const LifeCycleLabel &lifeCycleLabel);
        void cancelRestrictionLabelMark(const RestrictionLabel &restrictionLabel);
        void cancelTypeLabelMark(const std::string &typeLabel);

        void resetPermissionLabelMarks();
        void resetObjectOrientedLabelMarks();
        void resetLifeCycleLabelMarks();
        void resetRestrictionLabelMarks();
        void resetTypeLabelMarks();

        void displayResult();
        std::optional<PermissionLabel> getPermissionLabelMark() const;
        std::unordered_set<std::string> getTypeLabelMarks() const;
    private:
        void markPermissionLabel(const PermissionLabel &permissionLabel);
        void markObjectOrientedLabel(const ObjectOrientedLabel &objectOrientedLabel);
        void markLifeCycleLabel(const LifeCycleLabel &lifeCycleLabel);
        void markRestrictionLabel(const RestrictionLabel &restrictionLabel);
        void markTypeLabel(const std::string &typeLabel);
    };

    class ParameterSymbol: public Symbol {
        std::shared_ptr<TypeLabelSymbol> typeLabel;
        std::shared_ptr<TypeLabelSymbol> valueType;
        std::unordered_set<std::shared_ptr<LabelSymbol>> labels;
        std::optional<std::string> defaultValue;
        ParamType paramType;
        LabelMarkManager labelMarkManager {};
    public:
        explicit ParameterSymbol(
            const ParamType &paramType,
            const utils::Pos &pos,
            const std::string &name,
            const std::string &nameID,
            const std::unordered_set<std::shared_ptr<LabelSymbol>> &labels,
            const std::optional<std::string> &defaultValue,
            const size_t &scopeLevel,
            const SymbolType &symbolType = static_cast<SymbolType>(3),
            const std::shared_ptr<TypeLabelSymbol> &valueType = nullptr);

        [[nodiscard]] std::shared_ptr<TypeLabelSymbol> getTypeLabel() const;
        [[nodiscard]] std::unordered_set<std::shared_ptr<LabelSymbol>> getLabels() const;
        [[nodiscard]] std::optional<std::string> getDefaultValue() const;
        void setDefaultValue(const std::optional<std::string> &value);
        [[nodiscard]] ParamType getParamType() const;
        void reSetLabels(const std::unordered_set<std::shared_ptr<LabelSymbol>> &labelSymbols);
        void setTypeLabel(const std::shared_ptr<TypeLabelSymbol> &labelSymbol);
        void setValueType(const std::shared_ptr<TypeLabelSymbol> &labelSymbol);
        [[nodiscard]] std::shared_ptr<TypeLabelSymbol> getValueType() const;
        [[nodiscard]] LabelMarkManager &getLabelMarkManager();
        [[nodiscard]] LabelMarkManager getLabelMarkManager() const;
        std::shared_ptr<Symbol> transform(const std::string& value, const std::string& raValue,
            const size_t& scopeLevel = -1) const override;
        std::string toString() const override;
    };

    class VariableSymbol final : public ParameterSymbol {
        std::shared_ptr<ClassSymbol> classSymbol;
        std::shared_ptr<Symbol> referencedSymbol;
        std::shared_ptr<ast::ExpressionNode> defaultValueNode;
    public:
        explicit VariableSymbol(const utils::Pos &pos,
            const std::string &varName,
            const std::string &vid,
            const std::unordered_set<std::shared_ptr<LabelSymbol>> &labels,
            const size_t &scopeLevel,
            const bool &bindValueTypeToTypeLabel = false,
            const std::shared_ptr<TypeLabelSymbol> &valueType = nullptr,
            const std::shared_ptr<ClassSymbol> &classSymbol = nullptr,
            const std::shared_ptr<Symbol> &referencedSymbol = nullptr,
            const std::shared_ptr<ast::ExpressionNode> &defaultValueNode = nullptr);
        [[nodiscard]] std::shared_ptr<ClassSymbol> getClassSymbol() const;
        void setClassSymbol(const std::shared_ptr<ClassSymbol> &classSymbol_);

        static std::shared_ptr<VariableSymbol> paramSymbolToVarSymbol(
            const std::shared_ptr<ParameterSymbol> &paramSymbol, const size_t &scopeLevel);
        std::shared_ptr<Symbol> transform(const std::string& value, const std::string& raValue,
            const size_t& scopeLevel) const override;
        PermissionLabel getPermissionLabel() const;
        std::shared_ptr<Symbol> getReferencedSymbol() const;
        void setReferencedSymbol(const std::shared_ptr<Symbol> &refSymbol);
        std::string toString() const override;
        std::shared_ptr<ast::ExpressionNode> getDefaultValueNode() const;
    };

    enum class TypeOfBuiltin: int {
        ORDINARY,
        BUILTIN,
        PURE_BUILTIN
    };

    class FunctionSymbol final : public Symbol {
        // 核心：返回值类型标签（nullptr 表示未设置；若为 void 类型则表示无返回值）
        std::shared_ptr<TypeLabelSymbol> returnType = nullptr;
        // 函数体中是否已有返回语句（独立于返回类型，用于检查完整性）
        bool hasReturned_ = false;
        // 其他原有成员
        std::unordered_set<std::shared_ptr<LabelSymbol>> labels;
        TypeOfBuiltin builtinType;
        std::shared_ptr<ClassSymbol> classSymbol;
        std::vector<std::shared_ptr<ParameterSymbol>> parameters;
        std::shared_ptr<ast::ExpressionNode> definitionNode;
        FunctionType functionType;
        LabelMarkManager labelMarkManager{};
    public:
        // 构造函数保持不变
        FunctionSymbol(
            const std::shared_ptr<ast::ExpressionNode> &definitionNode,
            const utils::Pos &pos,
            const std::string &name,
            const std::string &nameID,
            const std::unordered_set<std::shared_ptr<LabelSymbol>> &labels,
            const std::vector<std::shared_ptr<ParameterSymbol>> &parameters,
            size_t scopeLevel,
            const TypeOfBuiltin &builtinType,
            const FunctionType &functionType,
            const std::shared_ptr<ClassSymbol> &className=nullptr);

        // ======================== 返回值相关优化方法 ========================
        // 获取返回值类型（可能为 nullptr 或 void 类型）
        [[nodiscard]] std::shared_ptr<TypeLabelSymbol> getReturnType() const;

        // 设置返回值类型（自动维护“是否有返回值”的逻辑）
        // 注：若传入 void 类型，则表示“无返回值”
        void setReturnType(const std::shared_ptr<TypeLabelSymbol> &type);
        void reSetReturnType(const std::shared_ptr<TypeLabelSymbol> &type);

        // 判断是否有返回值（通过 returnType 推导：非 nullptr 且非 void 类型则为 true）
        [[nodiscard]] bool hasReturnValue() const;

        // 判断是否已设置返回类型（通过 returnType 是否为 nullptr 推导）
        [[nodiscard]] bool hasSetReturnType() const;

        // 原有返回语句相关方法（保持不变）
        [[nodiscard]] bool hasReturned() const;
        void setHasReturned(bool hasReturned);

        void setFunctionType(const FunctionType &funcType);
        void setClassSymbol(const std::shared_ptr<ClassSymbol> &symbol);
        void setBuiltInType(const TypeOfBuiltin& type);
        std::string toString() const override;

        // ======================== 其他原有方法（保持不变） ========================
        [[nodiscard]] std::unordered_set<std::shared_ptr<LabelSymbol>> getLabels() const;
        [[nodiscard]] std::vector<std::shared_ptr<ParameterSymbol>> getParameters() const;
        [[nodiscard]] TypeOfBuiltin getBuiltInType() const;
        [[nodiscard]] std::shared_ptr<ClassSymbol> getClassSymbol() const;
        [[nodiscard]] std::shared_ptr<ast::ExpressionNode> getDefinitionNode() const;
        [[nodiscard]] LabelMarkManager &getLabelMarkManager();
        [[nodiscard]] FunctionType getFunctionType() const;
        [[nodiscard]] PermissionLabel getPermissionLabel() const;
        std::shared_ptr<Symbol> transform(const std::string& value, const std::string& raValue,
            const size_t& scopeLevel = -1) const override;
        bool is(const TypeOfBuiltin &type) const;
        [[nodiscard]] std::shared_ptr<TypeLabelSymbol> getFunctionTypeLabel() const;
    };

    class ClassSymbol final : public Symbol {
        std::vector<std::shared_ptr<ClassSymbol>> baseClasses;
        std::vector<std::shared_ptr<ClassSymbol>> derivedClasses;
        std::shared_ptr<SymbolTable> members = std::make_shared<SymbolTable>();
        std::shared_ptr<SymbolTable> staticMembers = std::make_shared<SymbolTable>();
        std::shared_ptr<SymbolTable> constructors = std::make_shared<SymbolTable>();
        LabelMarkManager labelMarkManager{};
        bool collectionFinished = false;
        PermissionLabel visitPermission;
    public:
        ClassSymbol(
            const utils::Pos &pos,
            const std::string &name,
            const std::string &nameID,
            const std::unordered_set<std::shared_ptr<LabelSymbol>> &labels,
            size_t scopeLevel,
            const SymbolTableManager &symbolTable,
            std::vector<std::shared_ptr<ClassSymbol>> baseClasses = {},
            const std::vector<std::shared_ptr<ClassSymbol>>& derivedClasses = {});
        ClassSymbol(
            const utils::Pos &pos,
            const std::string &name,
            const std::string &nameID,
            const LabelMarkManager &labelMarkManager,
            size_t scopeLevel,
            std::vector<std::shared_ptr<ClassSymbol>> baseClasses = {},
            const std::vector<std::shared_ptr<ClassSymbol>>& derivedClasses = {},
            const std::shared_ptr<SymbolTable> &constructors = {},
            const std::shared_ptr<SymbolTable> &staticMembers = {},
            const bool &collectionFinished = true,
            const PermissionLabel &visitPermission = {});

        [[nodiscard]] std::vector<std::shared_ptr<ClassSymbol>> &getBaseClasses();
        [[nodiscard]] std::vector<std::shared_ptr<ClassSymbol>> &getDerivedClasses();
        [[nodiscard]] std::shared_ptr<SymbolTable> &getMembers();
        [[nodiscard]] std::shared_ptr<SymbolTable> &getStaticMembers();
        [[nodiscard]] std::shared_ptr<SymbolTable> &getConstructors();

        void addMember(const std::shared_ptr<Symbol> &symbol, const bool& sysDefined) const;
        void _addDerivedClass(const std::shared_ptr<ClassSymbol> &classSymbol);
        void _addBaseClass(const std::shared_ptr<ClassSymbol> &classSymbol);
        void setDerivedClasses(const std::shared_ptr<ClassSymbol> &classSymbol);
        void setBaseClasses(const std::shared_ptr<ClassSymbol> &classSymbol);
        bool hasInheritClass() const;
        std::shared_ptr<ClassSymbol> getDirectlyInheritedClassSymbol() const;

        [[nodiscard]] bool hasMember(const std::string &name) const;
        std::pair<std::shared_ptr<Symbol>, LifeCycleLabel> findMemberSymbol(const std::string& name) const;
        std::pair<std::shared_ptr<Symbol>, LifeCycleLabel> findMemberSymbolInPermission(const std::string& name) const;

        std::shared_ptr<TypeLabelSymbol> getClassTypeLabelSymbol(const utils::Pos &pos) const;
        [[nodiscard]] LabelMarkManager &getLabelMarkManager();

        void setCollectionFinished();
        void setVisitPermission(const PermissionLabel &permission);
        void setDefaultVisitPermission();

        PermissionLabel getDefaultVisitPermission() const;

        bool hasCollectionFinished() const;
        PermissionLabel getVisitPermission() const;
        std::shared_ptr<Symbol> transform(const std::string& value, const std::string& raValue,
            const size_t& scopeLevel = -1) const override;
        std::string toString() const override;
    };

    class SymbolTable final: public utils::Object {
        std::unordered_map<std::string, std::pair<int, std::shared_ptr<Symbol>>> table{};
        std::vector<std::string> nameIndex{};
        std::vector<std::string> sysDefinitionRecord{};
    public:
        SymbolTable(){}
        std::shared_ptr<Symbol> insertByName(const std::shared_ptr<Symbol> &symbol, const bool& sysDefined);
        std::shared_ptr<Symbol> insertByRID(const std::shared_ptr<Symbol>& symbol, const bool& sysDefined);
        std::shared_ptr<Symbol> find(const std::string &name) const;
        [[nodiscard]] bool contains(const std::string &name) const;
        [[nodiscard]] std::unordered_map<std::string, std::pair<int, std::shared_ptr<Symbol>>> getTable() const;
        [[nodiscard]] std::vector<std::string> getNameIndex() const;
        void remove(const std::string &name);

        // 支持管道操作符 |，用于范围适配
        template <std::ranges::viewable_range R>
        auto operator|(R&& r) const {
            // 将 SymbolTable 转换为一个视图，再应用适配器
            return std::views::all(table | std::views::values) | std::forward<R>(r);
        }

        // 特定重载，处理 std::views::elements<1>
        auto operator|(const std::views::_Elements<1>&) const {
            return std::views::all(table | std::views::values);
        }

        // 非常量迭代器
        class Iterator {
            using MapIterator = std::unordered_map<std::string, std::pair<int, std::shared_ptr<Symbol>>>::iterator;
            MapIterator mapIter;
        public:
            explicit Iterator(MapIterator it): mapIter(it) {}
            Iterator &operator++() {
                ++mapIter;
                return *this;
            }
            bool operator!=(const Iterator &other) const {
                return mapIter != other.mapIter;
            }
            std::shared_ptr<Symbol> operator*() const {
                return mapIter->second.second;
            }
        };

        // 常量迭代器
        class ConstIterator {
            using MapConstIterator = std::unordered_map<std::string, std::pair<int, std::shared_ptr<Symbol>>>::const_iterator;
            MapConstIterator mapIter;
        public:
            explicit ConstIterator(MapConstIterator it): mapIter(it) {}
            ConstIterator &operator++() {
                ++mapIter;
                return *this;
            }
            bool operator!=(const ConstIterator &other) const {
                return mapIter != other.mapIter;
            }

            std::shared_ptr<Symbol> operator*() const {
                return mapIter->second.second;
            }
        };

        Iterator begin() {
            return Iterator(table.begin());
        }

        Iterator end() {
            return Iterator(table.end());
        }

        ConstIterator begin() const {
            return ConstIterator(table.begin());
        }

        ConstIterator end() const {
            return ConstIterator(table.end());
        }

        ConstIterator cbegin() const {
            return ConstIterator(table.begin());
        }

        ConstIterator cend() const {
            return ConstIterator(table.end());
        }
    };

    class SymbolTableManager final : public utils::Object {
        std::vector<std::pair<std::shared_ptr<SymbolTable>,
            std::shared_ptr<SymbolTable>>> scopes; // first: val -> Symbol, second: raVal -> Symbol
        size_t currentScopeLevel_; // 0 为全局作用域
    public:
        SymbolTableManager();
        [[nodiscard]] SymbolTable &currentNameMapScope() const;
        SymbolTable& currentRIDMapScope() const;
        void enterScope();
        void enterScope(size_t scopeLevel);
        void enterTopScope();
        void enterGlobalScope();
        void exitScope();
        void insert(const std::shared_ptr<Symbol> &symbol, const bool &systemDefined) const;
        void removeByName(const std::string &name, const std::optional<size_t>& specifiedLevel = std::nullopt) const;
        void removeByRID(const std::string& rid, const std::optional<size_t>& specifiedLevel = std::nullopt) const;
        [[nodiscard]] std::pair<long long int, std::shared_ptr<Symbol>> findByName(const std::string &name, const std::optional<size_t>& specifiedLevel = std::nullopt) const;
        std::pair<long long int, std::shared_ptr<Symbol>> findByRID(const std::string& rid) const;
        [[nodiscard]] std::shared_ptr<Symbol> findInCurrentScope(const std::string &name) const;
        [[nodiscard]] bool containsName(const std::string &name) const;
        [[nodiscard]] bool currentScopeContains(const std::string &name) const;
        [[nodiscard]] size_t curScopeLevel() const;

        [[nodiscard]] std::pair<long long int, std::shared_ptr<VariableSymbol>> findVariableSymbolByName(const std::string &name) const;
        std::pair<long long int, std::shared_ptr<VariableSymbol>> findVariableSymbolByRID(const std::string& rid) const;

        template <typename ST>
        [[nodiscard]] std::pair<long long int, std::shared_ptr<ST>> findByNameAndTransform(const std::string &name, const SymbolType &type) const
        {
            for (size_t i = currentScopeLevel_; i > 0; --i) {
                const auto &scope = scopes[i].first;
                if (const auto &it = scope->find(name)) {
                    if (it->getType() == type) {
                        return {i, std::static_pointer_cast<ST>(it)};
                    }
                }
            }
            return {-1, nullptr};
        }

        template <typename ST>
        [[nodiscard]] std::pair<long long int, std::shared_ptr<ST>> findByRIDAndTransform(const std::string &rid, const SymbolType &type) const
        {
            for (size_t i = currentScopeLevel_; i > 0; --i) {
                const auto &scope = scopes[i].second;
                if (const auto &it = scope->find(rid)) {
                    if (it->getType() == type) {
                        return {i, std::static_pointer_cast<ST>(it)};
                    }
                }
            }
            return {-1, nullptr};
        }
    };

    enum class SymbolType: int {
        VARIABLE = 0,
        FUNCTION,
        LABEL,
        PARAMETER,
        CLASS
    };

    enum class LabelType: int {
        UNKNOWN_TYPE_LABEL = 0,
        TYPE_LABEL, // 数据类型标签
        PERMISSION_LABEL, // 权限标签
        LIFE_CYCLE_LABEL, // 生命周期标签
        RESTRICTION_LABEL, // 限制标签
        OBJECT_ORIENTED_LABEL, // 对象关系标签
    };

    enum class ParamType: int {
        NO_PARAM = 0,
        PARAM_POSITIONAL,
        PARAM_KEYWORD,
        PARAM_VAR_LEN_POSITIONAL,
        PARAM_VAR_LEN_KEYWORD
    };

    enum class ArgumentType: int {
        ARG_POSITIONAL = 0,
        ARG_KEYWORD,
    };

    enum class FunctionType: int {
        FUNCTION = 0,
        CONSTRUCTOR,
        METHOD,
        ANONYMOUS
    };

    enum class BuiltinTypeLabel: int {
        INT = 0,
        FLOAT,
        STR,
        BOOL,
        CHAR,
        VOID,
        NUL,
        ANY,
        FUNC,
        CLS,
        LIST,
        DICT,
        SERIES,
        FLAG
    };

    enum class PermissionLabel: int {
        PRIVATE = 0,
        PROTECTED,
        PUBLIC,
        BUILTIN,
        COUNT
    };

    enum class LifeCycleLabel: int {
        STATIC = 0,   // 对应 "static"
        GLOBAL,       // 对应 "global"
        ORDINARY,     // 对应 "ordinary"
        INSTANCE,     // 对应 "instance"
        COUNT
    };

    enum class RestrictionLabel: int {
        CONST = 0,   // 对应 "const"
        QUOTE,       // 对应 "quote"
        COUNT
    };

    enum class ObjectOrientedLabel: int {
        OVERWRITE = 0,  // 对应 "overwrite"
        INTERFACE,      // 对应 "interface"
        VIRTUAL,        // 对应 "virtual"
        COUNT
    };
}



#endif //RCC_SYMBOL_H

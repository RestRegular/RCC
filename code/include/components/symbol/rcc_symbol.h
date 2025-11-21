//
// Created by RestRegular on 2025/7/12.
//

#ifndef RCC_SYMBOL_H
#define RCC_SYMBOL_H

#include <memory>
#include <unordered_set>
#include <ranges>
#include <optional>

#include "../../analyzer/rcc_ast_components.h"
#include "../../lib/rcc_utils.h"
#include "../../../declarations/components/symbol/rcc_symbol_dec.h"
#include "../../interfaces/rcc_symbol_interface.h"

namespace symbol {

    class Symbol:
    public utils::Object,
    public IRCCSymbolInterface {
        utils::Pos pos;
        SymbolType type;
        std::string value;
        std::string raValue;
        size_t scopeLevel;
    public:
        Symbol(utils::Pos pos,
            const SymbolType &symbolType,
            std::string symbolValue,
            std::string symbolRaValue,
            const size_t &scopeLevel);
        ~Symbol() override;
        [[nodiscard]] bool is(const SymbolType &symbolType) const;
        bool Is(const SymbolType& tp) const override;
        [[nodiscard]] bool isNot(const SymbolType &symbolType) const;
        bool IsNot(const SymbolType& tp) const override;
        [[nodiscard]] std::string getRaVal() const;
        const char* GetRaVal() const override;
        const char* GetVal() const override;
        [[nodiscard]] SymbolType getType() const;
        SymbolType GetType() const override;
        [[nodiscard]] std::string getVal() const;
        size_t GetScopeLevel() const override;
        [[nodiscard]] size_t getScopeLevel() const;
        void setScopeLevel(const size_t& level);
        void SetScopeLevel(const size_t& level) override;
        [[nodiscard]] utils::Pos getPos() const;
        const utils::IRCCPosInterface* GetPos() const override;
        [[nodiscard]] std::string toString() const override;
        [[nodiscard]] std::string getSymbolPosInfoString() const;
        virtual std::string toDetailString() const;
        [[nodiscard]] std::string toJsonString() const override;

        [[nodiscard]] bool equalWith(const Symbol &other) const;
        [[nodiscard]] virtual bool equalWith(const std::shared_ptr<Symbol> &other) const;

        static std::shared_ptr<Object> copySymbolPtr(const std::shared_ptr<Symbol> &symbolPtr);

        [[nodiscard]] virtual std::shared_ptr<Symbol> transform(
            const std::string &value, const std::string &raValue, const size_t &scopeLevel)const = 0;
        const Symbol* TransformToSI() const override;
        const char* ToString() const override;
        const char* BriefString() const override;
        const char* ProfessionalString() const override;
        const char* FormatString(const size_t& indent, const size_t& level) const override;
        const char* ToJsonString() const override;
        const IRCCObjectInterface* CopySelf() override;
    };

    class LabelSymbol:
    public Symbol,
    public IRCCLabelSymbolInterface {
        using LabelDes = std::vector<std::shared_ptr<LabelSymbol>>;
        bool isBuiltIn_;
        LabelType labelType;
        std::vector<LabelDes> labelDesS{};
        mutable std::vector<LabelDesI> m_labelDesIS;
    public:
        LabelSymbol(
            const utils::Pos &pos,
            const std::string &name, const std::string &raValue,
            const size_t &scopeLevel, const LabelType &labelType);
        ~LabelSymbol() override;
        [[nodiscard]] static bool isBuiltInLabel(const std::string &name);
        [[nodiscard]] LabelType getLabelType() const;
        LabelType GetLabelType() const override;
        [[nodiscard]] std::vector<LabelDes> getLabelDesS() const;
        const LabelDesI* GetLabelDesIS() const override;
        void appendLabelDes(const LabelDes &labelDes);
        void appendLastLabelDes(const LabelDes &labelDes);
        void handleLabelDesRecorded();
        [[nodiscard]] bool isBuiltIn() const;
        bool IsBuiltin() const override;
        std::string toDetailString() const override;
        [[nodiscard]] std::string toJsonString() const override;
        [[nodiscard]] std::string getLabelDescString() const;
        [[nodiscard]] std::string toString() const override;
        const LabelSymbol* TransformToLSI() const override;
        [[nodiscard]] std::string briefString() const override;
        [[nodiscard]] std::shared_ptr<Object> copySelf() const override;
        const char* ToString() const override;
        const char* BriefString() const override;
        const IRCCObjectInterface* CopySelf() override;
        [[nodiscard]] std::shared_ptr<Symbol> transform(
            const std::string& value, const std::string& raValue, const size_t &scopeLevel) const override;
        void syncLabelDesIS() const;
        bool Is(const SymbolType& tp) const override;
        bool IsNot(const SymbolType& tp) const override;
        const char* GetVal() const override;
        const char* GetRaVal() const override;
        SymbolType GetType() const override;
        size_t GetScopeLevel() const override;
        void SetScopeLevel(const size_t& level) override;
        const utils::IRCCPosInterface* GetPos() const override;
        const Symbol* TransformToSI() const override;
};

    class TypeLabelSymbol final:
    public LabelSymbol,
    public IRCCTypeLabelSymbolInterface {
        static std::unordered_set<std::string> builtInTypes;
        static std::unordered_map<std::string, std::string> builtInTypeRaCodeMap;
        static std::unordered_map<std::string, std::string> customTypeMap;
        static std::unordered_map<std::string, std::shared_ptr<ClassSymbol>> customClassSymbolMap;
    public:
        TypeLabelSymbol(const utils::Pos &pos, const std::string &name,
            const size_t &scopeLevel, const std::string &uid = "");
        ~TypeLabelSymbol() override;
        static std::string getTypeLabelRaCode(const std::string &name, const std::string &raValue);
        [[nodiscard]] static bool isTypeLabel(const std::string &name);
        [[nodiscard]] static bool isBuiltInType(const std::string &name);
        [[nodiscard]] static bool isCustomType(const std::string &uid);
        static void createCustomType(const std::string &name, const std::string &uid, const std::shared_ptr<ClassSymbol> &classSymbol);
        static void createCustomType(const std::shared_ptr<ClassSymbol> &classSymbol);
        static void deleteCustomType(const std::string &uid);
        static std::shared_ptr<ClassSymbol> getCustomClassSymbol(const std::string &uid);
        static std::shared_ptr<TypeLabelSymbol> getCustomTypeLabelSymbol(const std::string &uid, const size_t &scopeLevel);

        [[nodiscard]] bool is(const std::string &name) const;
        bool Is(const char* name) const override;
        [[nodiscard]] bool isNot(const std::string &name) const;
        bool IsNot(const char* name) const override;
        [[nodiscard]] bool isCustomType() const;
        bool IsCustomType() const override;
        std::string toDetailString() const override;
        [[nodiscard]] std::string toString() const override;
        [[nodiscard]] std::string toJsonString() const override;
        [[nodiscard]] std::shared_ptr<ClassSymbol> getCustomClassSymbol() const;
        [[nodiscard]] bool isIterable() const;
        [[nodiscard]] bool equalWith(const std::shared_ptr<Symbol>& other) const override;
        [[nodiscard]] bool relatedTo(const std::shared_ptr<TypeLabelSymbol> &other) const;
        [[nodiscard]] std::shared_ptr<Symbol> transform(const std::string& value, const std::string& raValue,
                                          const size_t& scopeLevel) const override;
        const LabelSymbol* TransformToLSI() const override;
        bool Is(const SymbolType& tp) const override;
        bool IsNot(const SymbolType& tp) const override;
        const char* GetVal() const override;
        const char* GetRaVal() const override;
        SymbolType GetType() const override;
        size_t GetScopeLevel() const override;
        void SetScopeLevel(const size_t& level) override;
        const utils::IRCCPosInterface* GetPos() const override;
        const Symbol* TransformToSI() const override;
        LabelType GetLabelType() const override;
        const LabelDesI* GetLabelDesIS() const override;
        bool IsBuiltin() const override;
        const TypeLabelSymbol* TransformToTLSI() const override;
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
        [[nodiscard]] static std::shared_ptr<TypeLabelSymbol> varArgTypeSymbol(const utils::Pos &pos, const size_t &scopeLevel);
        [[nodiscard]] static std::shared_ptr<TypeLabelSymbol> kwVarArgTypeSymbol(const utils::Pos &pos, const size_t &scopeLevel);

        [[nodiscard]] static std::shared_ptr<TypeLabelSymbol> getTypeLabelSymbolByStr(const std::string &str, const size_t &scopeLevel);
    };

    class LabelMarkManager final : utils::Object
    {
        int permissionLabelMark = -1;
        int objectOrientedLabelMarks = 0;
        int lifeCycleLabelMarks = 0;
        int restrictionLabelMarks = 0;
        std::unordered_set<std::string> typeLabelMarks{};
        std::string resultDisplay;
    public:
        LabelMarkManager() = default;
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

    class ParameterSymbol:
    public Symbol,
    public IRCCParameterSymbolInterface {
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
        ~ParameterSymbol() override;
        [[nodiscard]] bool typeIs(const std::string& type) const;
        [[nodiscard]] bool typeIsNot(const std::string &type) const;
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
            const size_t& scopeLevel) const override;
        std::string toString() const override;
        std::string toDetailString() const override;
        [[nodiscard]] std::string toJsonString() const override;
        [[nodiscard]] std::shared_ptr<Object> copySelf() const override;
        IRCCTypeLabelSymbolInterface* GetTypeLabelSymbolI() const override;
        IRCCTypeLabelSymbolInterface* GetValueTypeLabelSymbolI() const override;
        void SetTypeLabelSymbol(IRCCTypeLabelSymbolInterface* typeLabelI) override;
        void SetValueTypeLabelSymbol(IRCCTypeLabelSymbolInterface* valueTypeI) override;
        Labels GetLabels() const override;
        void FreeLabels(Labels& labels_) override;
        void ResetLabels(const Labels& labels_) override;
        const char* GetDefaultValue() const override;
        void SetDefaultValue(const char* value) override;
        ParamType GetParamType() const override;
        const ParameterSymbol* TransformToPSI() const override;
        const char* ToString() const override;
        bool Is(const SymbolType& tp) const override;
        bool IsNot(const SymbolType& tp) const override;
        const char* GetVal() const override;
        const char* GetRaVal() const override;
        SymbolType GetType() const override;
        size_t GetScopeLevel() const override;
        void SetScopeLevel(const size_t& level) override;
        const utils::IRCCPosInterface* GetPos() const override;
        const Symbol* TransformToSI() const override;
};

    class VariableSymbol final:
    public ParameterSymbol,
    public IRCCVariableSymbolInterface
    {
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
        std::string toDetailString() const override;
        [[nodiscard]] std::string toJsonString() const override;
        std::shared_ptr<ast::ExpressionNode> getDefaultValueNode() const;
        [[nodiscard]] std::shared_ptr<Object> copySelf() const override;
        IRCCTypeLabelSymbolInterface* GetTypeLabelSymbolI() const override;
        IRCCTypeLabelSymbolInterface* GetValueTypeLabelSymbolI() const override;
        void SetTypeLabelSymbol(IRCCTypeLabelSymbolInterface* typeLabel) override;
        void SetValueTypeLabelSymbol(IRCCTypeLabelSymbolInterface* valueType) override;
        Labels GetLabels() const override;
        void FreeLabels(Labels& labels) override;
        void ResetLabels(const Labels& labels) override;
        const char* GetDefaultValue() const override;
        void SetDefaultValue(const char* value) override;
        ParamType GetParamType() const override;
        const ParameterSymbol* TransformToPSI() const override;
        const char* ToString() const override;
        IRCCClassSymbolInterface* GetClassSymbolI() const override;
        PermissionLabel GetPermissionLabel() const override;
        IRCCSymbolInterface* GetReferencedSymbol() const override;
        void SetReferencedSymbol(const IRCCSymbolInterface* symbolI) override;
        void SetClassSymbol(const IRCCClassSymbolInterface* classSymbolI) override;
    };

    class FunctionSymbol final:
    public Symbol,
    public IRCCFunctionSymbolInterface {
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
        std::shared_ptr<TypeLabelSymbol> signature;
    public:
        // 构造函数保持不变
        FunctionSymbol(
            const std::shared_ptr<ast::ExpressionNode> &definitionNode,
            const utils::Pos &pos,
            const std::string &name,
            const std::string &nameID,
            const std::unordered_set<std::shared_ptr<LabelSymbol>> &labels,
            const std::vector<std::shared_ptr<ParameterSymbol>> &parameters,
            const std::shared_ptr<TypeLabelSymbol> &signature,
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
        void setSignature(const std::shared_ptr<TypeLabelSymbol> &signature_);
        std::shared_ptr<TypeLabelSymbol> getSignature() const;
        std::string getSignatureString() const;
        std::string toString() const override;
        std::string toDetailString() const override;
        [[nodiscard]] std::string toJsonString() const override;

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
            const size_t& scopeLevel) const override;
        bool is(const TypeOfBuiltin &type) const;
        [[nodiscard]] std::shared_ptr<TypeLabelSymbol> getFunctionTypeLabel() const;
        [[nodiscard]] std::shared_ptr<Object> copySelf() const override;

        bool Is(const SymbolType& tp) const override;
        bool IsNot(const SymbolType& tp) const override;
        const char* GetVal() const override;
        const char* GetRaVal() const override;
        SymbolType GetType() const override;
        size_t GetScopeLevel() const override;
        void SetScopeLevel(const size_t& level) override;
        const utils::IRCCPosInterface* GetPos() const override;
        const Symbol* TransformToSI() const override;
        IRCCTypeLabelSymbolInterface* GetReturnTypeLabelI() const override;
        void SetReturnType(const IRCCTypeLabelSymbolInterface* typeSymbolI) override;
        void ReSetReturnType(const IRCCTypeLabelSymbolInterface* typeSymbolI) override;
        bool HasReturnValue() const override;
        bool HasSetReturnType() const override;
        bool HasReturned() const override;
        void SetHasReturned(const bool& hasReturned) override;
        void SetFunctionType(const FunctionType& funcType) override;
        void SetClassSymbol(const IRCCClassSymbolInterface* classSymbolI) override;
        void SetBuiltInType(const TypeOfBuiltin& type) override;
        void SetSignature(const IRCCTypeLabelSymbolInterface* signatureSymbolI) override;
        IRCCTypeLabelSymbolInterface* GetSignatureSymbolI() const override;
        const char* GetSignatureString() const override;
        Labels GetLabels() const override;
        ParamSymbols GetParamSymbols() const override;
        TypeOfBuiltin GetBuiltInType() const override;
        IRCCClassSymbolInterface* GetClassSymbolI() const override;
        FunctionType GetFunctionType() const override;
        PermissionLabel GetPermissionLabel() const override;
        const FunctionSymbol* TransformToFSI() const override;
        bool Is(const TypeOfBuiltin& type) const override;
        IRCCTypeLabelSymbolInterface* GetFunctionTypeLabelSymbolI() const override;
        void FreeLabels(Labels& labels_) override;
        void FreeParamSymbols(ParamSymbols& paramSymbols) override;
};

    class ClassSymbol final :
    public Symbol,
    public IRCCClassSymbolInterface
    {
        std::vector<std::shared_ptr<ClassSymbol>> baseClasses;
        std::vector<std::shared_ptr<ClassSymbol>> derivedClasses;
        std::shared_ptr<SymbolTable> members = std::make_shared<SymbolTable>();
        std::shared_ptr<SymbolTable> staticMembers = std::make_shared<SymbolTable>();
        std::shared_ptr<SymbolTable> constructors = std::make_shared<SymbolTable>();
        LabelMarkManager labelMarkManager{};
        bool collectionFinished = false;
        PermissionLabel visitPermission;
        size_t scope_level_{};
        // 用于处理普通容器
        template<typename Container>
        std::vector<rjson::RJValue> transformToRJValues(const Container& container) const
        {
            std::vector<rjson::RJValue> values;
            values.reserve(container.size());
            for (const auto& item : container) {
                values.emplace_back(rjson::RJType::RJ_OBJECT, item->toJsonString());
            }
            return values;
        }

        // 用于处理符号表（map-like结构）
        static std::vector<rjson::RJValue> transformSymbolTableToRJValues(const std::unordered_map<std::string, std::pair<int, std::shared_ptr<Symbol>>>& table)
        {
            std::vector<rjson::RJValue> values;
            values.reserve(table.size());
            for (const auto& [_, s] : table | std::views::values) {
                values.push_back(rjson::RJValue::getObjectRJValue(s->toJsonString()));
            }
            return values;
        }
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
            LabelMarkManager labelMarkManager,
            size_t scopeLevel,
            std::vector<std::shared_ptr<ClassSymbol>> baseClasses = {},
            const std::vector<std::shared_ptr<ClassSymbol>>& derivedClasses = {},
            const std::shared_ptr<SymbolTable> &constructors = {},
            const std::shared_ptr<SymbolTable> &staticMembers = {},
            const bool &collectionFinished = true,
            const PermissionLabel &visitPermission = {});
        ~ClassSymbol() override;
        [[nodiscard]] std::string getInheritanceChain() const;
        [[nodiscard]] std::string briefString() const override;
        void FreeClasses(Classes& classes) override;
        [[nodiscard]] std::vector<std::shared_ptr<ClassSymbol>> &getBaseClasses();
        Classes GetBaseClasses() override;
        [[nodiscard]] std::vector<std::shared_ptr<ClassSymbol>> &getDerivedClasses();
        Classes GetDeriveClasses() override;
        [[nodiscard]] std::shared_ptr<SymbolTable> &getMembers();
        IRCCSymbolTableInterface* GetMembers() const override;
        [[nodiscard]] std::shared_ptr<SymbolTable> &getStaticMembers();
        IRCCSymbolTableInterface* GetStaticMembers() const override;
        [[nodiscard]] std::shared_ptr<SymbolTable> &getConstructors();
        IRCCSymbolTableInterface* GetConstructors() const override;

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
            const size_t& scopeLevel) const override;
        std::string toString() const override;
        std::string toDetailString() const override;
        [[nodiscard]] std::string toJsonString() const override;
        [[nodiscard]] std::shared_ptr<Object> copySelf() const override;
        bool relatedTo(const std::shared_ptr<ClassSymbol> &other) const;
        bool isSupperClassOf(const std::shared_ptr<ClassSymbol> &other, const bool &restrict = true) const;
        void AddMember(const IRCCSymbolInterface* symbolI) const override;
        void SetDerivedClasses(const IRCCClassSymbolInterface* classSymbolI) override;
        void SetBaseClasses(const IRCCClassSymbolInterface* classSymbolI) override;
        bool HasInheritClass() const override;
        IRCCClassSymbolInterface* GetDirectlyInheritedClassSymbolI() const override;
        bool HasMember(const char* name) const override;
        MemberSymbolResult FindMemberSymbolI(const char* name) const override;
        MemberSymbolResult FindMemberSymbolIInPermission(const char* name) const override;
        IRCCTypeLabelSymbolInterface* GetClassTypeLabelSymbolI(const utils::IRCCPosInterface* posI) const override;
        void SetCollectionFinished() override;
        void SetVisitPermission(const PermissionLabel& permission) override;
        void SetDefaultVisitPermission() override;
        PermissionLabel GetDefaultVisitPermission() const override;
        bool HasCollectionFinished() const override;
        PermissionLabel GetVisitPermission() const override;
        const ClassSymbol* TransformToCSI() const override;
        bool RelatedTo(const IRCCClassSymbolInterface* otherI) const override;
        bool IsSupperClassOf(const IRCCClassSymbolInterface* otherI, const bool& restrict) const override;
        bool Is(const SymbolType& tp) const override;
        bool IsNot(const SymbolType& tp) const override;
        const char* GetVal() const override;
        const char* GetRaVal() const override;
        SymbolType GetType() const override;
        size_t GetScopeLevel() const override;
        void SetScopeLevel(const size_t& level) override;
        const utils::IRCCPosInterface* GetPos() const override;
        const Symbol* TransformToSI() const override;
    };

    class SymbolTable final:
    public utils::Object,
    public IRCCSymbolTableInterface {
        std::unordered_map<std::string, std::pair<int, std::shared_ptr<Symbol>>> table{};
        std::vector<std::string> nameIndex{};
        std::unordered_set<std::string> sysDefinitionRecord{};
    public:
        SymbolTable() = default;
        ~SymbolTable() override;
        std::shared_ptr<Symbol> insertByName(const std::shared_ptr<Symbol>& symbol, const bool& sysDefined);
        void InsertByName(IRCCSymbolInterface* symbolI) override;
        std::shared_ptr<Symbol> insertByRID(const std::shared_ptr<Symbol>& symbol, const bool& sysDefined);
        std::shared_ptr<Symbol> find(const std::string &name) const;
        IRCCSymbolInterface* Find(const char* name) override;
        [[nodiscard]] bool contains(const std::string &name) const;
        bool Contains(const char* name) override;
        [[nodiscard]] std::unordered_map<std::string, std::pair<int, std::shared_ptr<Symbol>>> getTable() const;
        [[nodiscard]] std::vector<std::string> getNameIndex() const;
        void remove(const std::string &name);
        void Remove(const char* name) override;
        [[nodiscard]] std::shared_ptr<Object> copySelf() const override;
        std::unordered_set<std::string> getSysDefinedRecord() const;

        // 支持管道操作符 |，用于范围适配
        template <std::ranges::viewable_range R>
        auto operator|(R&& r) const {
            // 将 SymbolTable 转换为一个视图，再应用适配器
            return std::views::all(table | std::views::values) | std::forward<R>(r);
        }

        auto operator|(auto&&) const
        -> decltype(std::views::all(table | std::views::values))
        {
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

    class SymbolTableManager final:
    public utils::Object,
    public IRCCSymbolTableManagerInterface {
        std::vector<std::pair<std::shared_ptr<SymbolTable>,
            std::shared_ptr<SymbolTable>>> scopes; // first: val -> Symbol, second: raVal -> Symbol
        size_t currentScopeLevel_; // 0 为全局作用域

        void appendScope(const std::shared_ptr<SymbolTable> &nameScope, const std::shared_ptr<SymbolTable> &idScope);
    public:
        SymbolTableManager();
        ~SymbolTableManager() override;
        [[nodiscard]] SymbolTable &currentNameMapScope() const;
        IRCCSymbolTableInterface* CurrentNameMapScope() const override;
        [[nodiscard]] SymbolTable &currentRIDMapScope() const;
        IRCCSymbolTableInterface* CurrentRIDMapScope() const override;
        [[nodiscard]] std::pair<SymbolTable, SymbolTable> currentScopeCopied() const;
        void enterScope();
        void EnterScope() override;
        void enterScope(size_t scopeLevel);
        void enterTopScope();
        void enterGlobalScope();
        void exitScope();
        void insert(const std::shared_ptr<Symbol> &symbol, const bool &systemDefined) const;
        void removeByName(const std::string &name, const std::optional<size_t>& specifiedLevel = std::nullopt) const;
        void removeByRID(const std::string& rid, const std::optional<size_t>& specifiedLevel = std::nullopt) const;
        [[nodiscard]] std::pair<long long int, std::shared_ptr<Symbol>> findByName(const std::string &name, const std::optional<size_t>& specifiedLevel = std::nullopt) const;
        [[nodiscard]] std::pair<long long int, std::shared_ptr<Symbol>> findByRID(const std::string& rid) const;
        [[nodiscard]] std::shared_ptr<Symbol> findInCurrentScope(const std::string &name) const;
        [[nodiscard]] bool containsName(const std::string &name) const;
        [[nodiscard]] bool currentScopeContains(const std::string &name) const;
        [[nodiscard]] size_t curScopeLevel() const;
        [[nodiscard]] std::shared_ptr<Object> copySelf() const override;
        void EnterScope(const size_t& scopeLevel) override;
        void EnterTopScope() override;
        void EnterGlobalScope() override;
        void ExitScope() override;
        void Insert(IRCCSymbolInterface* symbolI) const override;
        void RemoveByName(const char* name, const size_t* specifiedLevel) const override;
        void RemoveByRID(const char* rid, const size_t* specifiedLevel) const override;
        IRCCSymbolInterface* FindByName(const char* name, const size_t* specifiedLevel) const override;
        IRCCSymbolInterface* FindByRID(const char* rid) const override;
        IRCCSymbolInterface* FindInCurrentScope(const char* name) const override;
        bool ContainsName(const char* name) const override;
        bool CurrentScopeContains(const char* name) const override;
        size_t CurScopeLevel() const override;
        [[nodiscard]] std::pair<long long int, std::shared_ptr<VariableSymbol>> findVariableSymbolByName(const std::string &name) const;
        [[nodiscard]] std::pair<long long int, std::shared_ptr<VariableSymbol>> findVariableSymbolByRID(const std::string& rid) const;

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
        VOID_BTL,
        NUL,
        ANY,
        FUNC,
        CLS,
        LIST,
        DICT,
        SERIES,
        FLAG
    };

    enum class RestrictionLabel: int {
        CONST_RL = 0,   // 对应 "const"
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

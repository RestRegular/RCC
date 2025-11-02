//
// Created by RestRegular on 2025/10/22.
//

#ifndef RCC_RCC_SYMBOL_INTERFACE_H
#define RCC_RCC_SYMBOL_INTERFACE_H

#include <string>

#include "rcc_interface_dec.h"
#include "./rcc_object_interface.h"

namespace symbol
{
    enum class FunctionType;

    enum class SymbolType: int;
    enum class LabelType: int;
    enum class LifeCycleLabel: int;
    enum class PermissionLabel: int;
    enum class ParamType: int;
    enum class TypeOfBuiltin: int;

    class Symbol;
    class LabelSymbol;
    class TypeLabelSymbol;
    class ParameterSymbol;
    class VariableSymbol;
    class FunctionSymbol;
    class ClassSymbol;
    class SymbolTable;
    class IRCCSymbolInterface;
    class IRCCLabelSymbolInterface;
    class IRCCTypeLabelSymbolInterface;
    class IRCCParameterSymbolInterface;
    class IRCCVariableSymbolInterface;
    class IRCCFunctionSymbolInterface;
    class IRCCClassSymbolInterface;
    class IRCCSymbolTableInterface;
    class IRCCSymbolTableManagerInterface;

    class IRCCSymbolInterface:
    virtual public utils::IRCCObjectInterface
    {
    public:
        virtual ~IRCCSymbolInterface() SYS_INTERFACE_CALL = 0;
        virtual bool SYS_INTERFACE_CALL Is(const SymbolType &tp) const = 0;
        virtual bool SYS_INTERFACE_CALL IsNot(const SymbolType &tp) const = 0;
        virtual const char* SYS_INTERFACE_CALL GetVal() const = 0;
        virtual const char* SYS_INTERFACE_CALL GetRaVal() const = 0;
        virtual SymbolType SYS_INTERFACE_CALL GetType() const = 0;
        virtual size_t SYS_INTERFACE_CALL GetScopeLevel() const = 0;
        virtual void SYS_INTERFACE_CALL SetScopeLevel(const size_t& level) = 0;
        virtual const utils::IRCCPosInterface* SYS_INTERFACE_CALL GetPos() const = 0;
        virtual const Symbol* SYS_INTERFACE_CALL TransformToSI() const = 0;
    };

    class IRCCLabelSymbolInterface:
    virtual public IRCCSymbolInterface
    {
    public:
        using LabelDesI = IRCCLabelSymbolInterface*;
        ~IRCCLabelSymbolInterface() override SYS_INTERFACE_CALL = 0;
        static bool SYS_INTERFACE_CALL IsBuiltInLabel(const char *name);
        virtual LabelType SYS_INTERFACE_CALL GetLabelType() const = 0;
        virtual const LabelDesI* SYS_INTERFACE_CALL GetLabelDesIS() const = 0;
        virtual bool SYS_INTERFACE_CALL IsBuiltin() const = 0;
        virtual const LabelSymbol* SYS_INTERFACE_CALL TransformToLSI() const = 0;
    };

    class IRCCTypeLabelSymbolInterface:
    virtual public IRCCLabelSymbolInterface
    {
    public:
        ~IRCCTypeLabelSymbolInterface() override SYS_INTERFACE_CALL = 0;
        static const char* SYS_INTERFACE_CALL GetTypeLabelRaCode(const std::string &name, const std::string &raValue);
        static bool SYS_INTERFACE_CALL IsTypeLabel(const char* name);
        static bool SYS_INTERFACE_CALL IsBuiltInType(const char* name);
        static bool SYS_INTERFACE_CALL IsCustomType(const char* uid);
        virtual bool SYS_INTERFACE_CALL Is(const char* name) const = 0;
        virtual bool SYS_INTERFACE_CALL IsNot(const char* name) const = 0;
        virtual bool SYS_INTERFACE_CALL IsCustomType() const = 0;
        virtual const TypeLabelSymbol* SYS_INTERFACE_CALL TransformToTLSI() const = 0;
    };

    class IRCCParameterSymbolInterface:
    virtual public IRCCSymbolInterface
    {
    public:
        struct Labels
        {
            IRCCLabelSymbolInterface** labels;
            size_t count;

            explicit Labels(const size_t &count_)
                : count(count_)
            {
                labels = new IRCCLabelSymbolInterface*[count];
            }

            Labels(const Labels&) = delete;
            Labels& operator=(const Labels&) = delete;

            Labels(Labels&& other) noexcept
            : labels(other.labels), count(other.count) {
                other.labels = nullptr;
                other.count = 0;
            }
        };

        ~IRCCParameterSymbolInterface() override SYS_INTERFACE_CALL = 0;
        virtual IRCCTypeLabelSymbolInterface* SYS_INTERFACE_CALL GetTypeLabelSymbolI() const = 0;
        virtual IRCCTypeLabelSymbolInterface* SYS_INTERFACE_CALL GetValueTypeLabelSymbolI() const = 0;
        virtual void SYS_INTERFACE_CALL SetTypeLabelSymbol(IRCCTypeLabelSymbolInterface* typeLabel) = 0;
        virtual void SYS_INTERFACE_CALL SetValueTypeLabelSymbol(IRCCTypeLabelSymbolInterface* valueType) = 0;
        virtual Labels SYS_INTERFACE_CALL GetLabels() const = 0;
        virtual void SYS_INTERFACE_CALL FreeLabels(Labels &labels) = 0;
        virtual void SYS_INTERFACE_CALL ResetLabels(const Labels& labels) = 0;
        virtual const char* SYS_INTERFACE_CALL GetDefaultValue() const = 0;
        virtual void SYS_INTERFACE_CALL SetDefaultValue(const char* value) = 0;
        virtual ParamType SYS_INTERFACE_CALL GetParamType() const = 0;
        virtual const ParameterSymbol* SYS_INTERFACE_CALL TransformToPSI() const = 0;
    };

    class IRCCVariableSymbolInterface:
    virtual public IRCCParameterSymbolInterface
    {
    public:
        ~IRCCVariableSymbolInterface() override = 0;
        virtual IRCCClassSymbolInterface* SYS_INTERFACE_CALL GetClassSymbolI() const = 0;
        virtual void SYS_INTERFACE_CALL SetClassSymbol(const IRCCClassSymbolInterface* classSymbolI) = 0;
        virtual PermissionLabel SYS_INTERFACE_CALL GetPermissionLabel() const = 0;
        virtual IRCCSymbolInterface* GetReferencedSymbol() const = 0;
        virtual void SYS_INTERFACE_CALL SetReferencedSymbol(const IRCCSymbolInterface* symbolI) = 0;
    };

    class IRCCFunctionSymbolInterface:
    virtual public IRCCSymbolInterface
    {
    public:
        struct Labels
        {
            IRCCLabelSymbolInterface** labels;
            size_t count;

            explicit Labels(const size_t &count_)
                : count(count_)
            {
                labels = new IRCCLabelSymbolInterface*[count];
            }

            Labels(const Labels&) = delete;
            Labels& operator=(const Labels&) = delete;

            Labels(Labels&& other) noexcept
            : labels(other.labels), count(other.count) {
                other.labels = nullptr;
                other.count = 0;
            }
        };

        struct ParamSymbols
        {
            IRCCParameterSymbolInterface** paramSymbols;
            size_t count;

            explicit ParamSymbols(const size_t &count_)
                : count(count_)
            {
                paramSymbols = new IRCCParameterSymbolInterface*[count];
            }

            ParamSymbols(const ParamSymbols &) = delete;
            ParamSymbols& operator=(const ParamSymbols&) = delete;
            ParamSymbols(ParamSymbols&& other) noexcept
            : paramSymbols(other.paramSymbols), count(other.count) {
                other.count = 0;
                other.paramSymbols = nullptr;
            }
        };

        ~IRCCFunctionSymbolInterface() override SYS_INTERFACE_CALL;
        virtual void SYS_INTERFACE_CALL FreeLabels(Labels &labels) = 0;
        virtual void SYS_INTERFACE_CALL FreeParamSymbols(ParamSymbols &paramSymbols) = 0;
        virtual IRCCTypeLabelSymbolInterface* SYS_INTERFACE_CALL GetReturnTypeLabelI() const = 0;
        virtual void SYS_INTERFACE_CALL SetReturnType(const IRCCTypeLabelSymbolInterface* typeSymbolI) = 0;
        virtual void SYS_INTERFACE_CALL ReSetReturnType(const IRCCTypeLabelSymbolInterface* typeSymbolI) = 0;

        virtual bool SYS_INTERFACE_CALL HasReturnValue() const = 0;
        virtual bool SYS_INTERFACE_CALL HasSetReturnType() const = 0;
        virtual bool SYS_INTERFACE_CALL HasReturned() const = 0;
        virtual void SYS_INTERFACE_CALL SetHasReturned(const bool& hasReturned) = 0;
        virtual void SYS_INTERFACE_CALL SetFunctionType(const FunctionType &funcType) = 0;
        virtual void SYS_INTERFACE_CALL SetClassSymbol(const IRCCClassSymbolInterface* classSymbolI) = 0;
        virtual void SYS_INTERFACE_CALL SetBuiltInType(const TypeOfBuiltin& type) = 0;
        virtual void SYS_INTERFACE_CALL SetSignature(const IRCCTypeLabelSymbolInterface* signatureSymbolI) = 0;
        virtual IRCCTypeLabelSymbolInterface* SYS_INTERFACE_CALL GetSignatureSymbolI() const = 0;
        virtual const char* SYS_INTERFACE_CALL GetSignatureString() const = 0;
        virtual Labels SYS_INTERFACE_CALL GetLabels() const = 0;
        virtual ParamSymbols SYS_INTERFACE_CALL GetParamSymbols() const = 0;
        virtual TypeOfBuiltin SYS_INTERFACE_CALL GetBuiltInType() const = 0;
        virtual IRCCClassSymbolInterface* SYS_INTERFACE_CALL GetClassSymbolI() const = 0;
        virtual FunctionType SYS_INTERFACE_CALL GetFunctionType() const = 0;
        virtual PermissionLabel SYS_INTERFACE_CALL GetPermissionLabel() const = 0;
        virtual const FunctionSymbol* SYS_INTERFACE_CALL TransformToFSI() const = 0;
        virtual bool SYS_INTERFACE_CALL Is(const TypeOfBuiltin& type) const = 0;
        virtual IRCCTypeLabelSymbolInterface* GetFunctionTypeLabelSymbolI() const = 0;
    };

    class IRCCClassSymbolInterface:
    virtual public IRCCSymbolInterface
    {
    public:
        struct Classes
        {
            IRCCClassSymbolInterface** classes;
            size_t count;

            explicit Classes(const size_t count_): count(count_)
            {
                classes = new IRCCClassSymbolInterface*[count];
            }

            Classes(const Classes&) = delete;
            Classes& operator=(const Classes&) = delete;

            Classes(Classes&& other) noexcept
            : classes(other.classes), count(other.count) {
                other.classes = nullptr;
                other.count = 0;
            }
        };

        struct MemberSymbolResult
        {
            IRCCSymbolInterface* memberSymbolI;
            LifeCycleLabel lifeCycleLabel;

            MemberSymbolResult(IRCCSymbolInterface* memberSymbolI, const LifeCycleLabel lifeCycleLabel)
                : memberSymbolI(memberSymbolI), lifeCycleLabel(lifeCycleLabel) {}
        };

        ~IRCCClassSymbolInterface() override SYS_INTERFACE_CALL = 0;
        virtual void SYS_INTERFACE_CALL FreeClasses(Classes &classes) = 0;
        virtual Classes SYS_INTERFACE_CALL GetBaseClasses() = 0;
        virtual Classes SYS_INTERFACE_CALL GetDeriveClasses() = 0;
        virtual IRCCSymbolTableInterface* SYS_INTERFACE_CALL GetMembers() const = 0;
        virtual IRCCSymbolTableInterface* SYS_INTERFACE_CALL GetStaticMembers() const = 0;
        virtual IRCCSymbolTableInterface* SYS_INTERFACE_CALL GetConstructors() const = 0;
        virtual void SYS_INTERFACE_CALL AddMember(const IRCCSymbolInterface* symbolI) const = 0;
        virtual void SYS_INTERFACE_CALL SetDerivedClasses(const IRCCClassSymbolInterface* classSymbolI) = 0;
        virtual void SYS_INTERFACE_CALL SetBaseClasses(const IRCCClassSymbolInterface* classSymbolI) = 0;
        virtual bool SYS_INTERFACE_CALL HasInheritClass() const = 0;
        virtual IRCCClassSymbolInterface* GetDirectlyInheritedClassSymbolI() const = 0;
        virtual bool HasMember(const char* name) const = 0;
        virtual MemberSymbolResult SYS_INTERFACE_CALL FindMemberSymbolI(const char* name) const = 0;
        virtual MemberSymbolResult SYS_INTERFACE_CALL FindMemberSymbolIInPermission(const char* name) const = 0;
        virtual IRCCTypeLabelSymbolInterface* SYS_INTERFACE_CALL GetClassTypeLabelSymbolI(const utils::IRCCPosInterface* pos) const = 0;
        virtual void SYS_INTERFACE_CALL SetCollectionFinished() = 0;
        virtual void SYS_INTERFACE_CALL SetVisitPermission(const PermissionLabel &permission) = 0;
        virtual void SYS_INTERFACE_CALL SetDefaultVisitPermission() = 0;
        virtual PermissionLabel SYS_INTERFACE_CALL GetDefaultVisitPermission() const = 0;
        virtual bool SYS_INTERFACE_CALL HasCollectionFinished() const = 0;
        virtual PermissionLabel SYS_INTERFACE_CALL GetVisitPermission() const = 0;
        virtual const ClassSymbol* SYS_INTERFACE_CALL TransformToCSI() const = 0;
        virtual bool SYS_INTERFACE_CALL RelatedTo(const IRCCClassSymbolInterface* otherI) const = 0;
        virtual bool SYS_INTERFACE_CALL IsSupperClassOf(const IRCCClassSymbolInterface* otherI, const bool &restrict) const = 0;
    };

    class IRCCSymbolTableInterface:
    virtual public utils::IRCCObjectInterface
    {
    public:
        virtual ~IRCCSymbolTableInterface() SYS_INTERFACE_CALL = 0;
        virtual void SYS_INTERFACE_CALL InsertByName(IRCCSymbolInterface* symbolI) = 0;
        virtual IRCCSymbolInterface* Find(const char* name) = 0;
        virtual bool SYS_INTERFACE_CALL Contains(const char* name) = 0;
        virtual void SYS_INTERFACE_CALL Remove(const char* name) = 0;
    };

    class IRCCSymbolTableManagerInterface:
    virtual public utils::IRCCObjectInterface
    {
    public:
        virtual ~IRCCSymbolTableManagerInterface() SYS_INTERFACE_CALL = 0;
        virtual IRCCSymbolTableInterface* SYS_INTERFACE_CALL CurrentNameMapScope() const = 0;
        virtual IRCCSymbolTableInterface* SYS_INTERFACE_CALL CurrentRIDMapScope() const = 0;
        virtual void SYS_INTERFACE_CALL EnterScope() = 0;
        virtual void SYS_INTERFACE_CALL EnterScope(const size_t &scopeLevel) = 0;
        virtual void SYS_INTERFACE_CALL EnterTopScope() = 0;
        virtual void SYS_INTERFACE_CALL EnterGlobalScope() = 0;
        virtual void SYS_INTERFACE_CALL ExitScope() = 0;
        virtual void SYS_INTERFACE_CALL Insert(IRCCSymbolInterface* symbolI) const = 0;
        virtual void SYS_INTERFACE_CALL RemoveByName(const char* name, const size_t* specifiedLevel) const = 0;
        virtual void SYS_INTERFACE_CALL RemoveByRID(const char* name, const size_t* specifiedLevel) const = 0;
        virtual IRCCSymbolInterface* SYS_INTERFACE_CALL FindByName(const char* name, const size_t* specifiedLevel) const = 0;
        virtual IRCCSymbolInterface* SYS_INTERFACE_CALL FindByRID(const char* rid) const = 0;
        virtual IRCCSymbolInterface* SYS_INTERFACE_CALL FindInCurrentScope(const char* name) const = 0;
        virtual bool SYS_INTERFACE_CALL ContainsName(const char* name) const = 0;
        virtual bool SYS_INTERFACE_CALL CurrentScopeContains(const char* name) const = 0;
        virtual size_t SYS_INTERFACE_CALL CurScopeLevel() const = 0;
    };

    enum class SymbolType: int {
        VARIABLE = 0,
        FUNCTION,
        LABEL,
        PARAMETER,
        CLASS
    };

    enum class LabelType: int  {
        UNKNOWN_TYPE_LABEL = 0,
        TYPE_LABEL, // 数据类型标签
        PERMISSION_LABEL, // 权限标签
        LIFE_CYCLE_LABEL, // 生命周期标签
        RESTRICTION_LABEL, // 限制标签
        OBJECT_ORIENTED_LABEL, // 对象关系标签
    };

    enum class LifeCycleLabel: int {
        STATIC = 0,   // 对应 "static"
        GLOBAL,       // 对应 "global"
        ORDINARY,     // 对应 "ordinary"
        INSTANCE,     // 对应 "instance"
        COUNT
    };

    enum class PermissionLabel: int {
        PRIVATE = 0,
        PROTECTED,
        PUBLIC,
        BUILTIN,
        COUNT
    };

    enum class ParamType: int {
        NO_PARAM = 0,
        PARAM_POSITIONAL,
        PARAM_KEYWORD,
        PARAM_VAR_LEN_POSITIONAL,
        PARAM_VAR_LEN_KEYWORD
    };

    enum class TypeOfBuiltin: int {
        ORDINARY,
        BUILTIN,
        PURE_BUILTIN
    };
}

#endif //RCC_RCC_SYMBOL_INTERFACE_H
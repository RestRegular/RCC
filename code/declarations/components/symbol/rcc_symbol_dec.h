//
// Created by RestRegular on 2025/10/22.
//

#ifndef RCC_RCC_SYMBOL_DEC_H
#define RCC_RCC_SYMBOL_DEC_H
#include <string>
#include <unordered_map>

namespace symbol
{
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
}

#endif //RCC_RCC_SYMBOL_DEC_H
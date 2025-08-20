//
// Created by RestRegular on 2025/7/12.
//

#include <numeric>
#include <ranges>

#include "../../include/rcc_base.h"
#include "../../include/builtin/rcc_builtin.h"
#include "../../include/components/ri/rcc_ri.h"
#include "../../include/lib/RLogSystem/rlog_system.h"
#include "../../include/analyzer/rcc_ast_components.h"
#include "../../include/analyzer/rcc_lexer.h"
#include "../../include/analyzer/rcc_parser.h"
#include "../../include/components/symbol/rcc_symbol.h"
#include "../../include/visitors/rcc_compiler_visitor.h"

namespace ast
{
    using namespace symbol;

    RaCodeBuilder::RaCodeBuilder()
    {
        enterScope();
    }

    std::string RaCodeBuilder::buildAll()
    {
        if (raCode.empty()) {
            while (!raCodeStack.empty())
            {
                raCode += raCodeStack.top().str();
                raCodeStack.pop();
            }
        }
        return raCode;
    }

    RaCodeBuilder& RaCodeBuilder::appendCode(const std::string& code)
    {
        if (code.empty()) return *this;
        if (raCodeStack.empty()) throw std::runtime_error("RaCodeBuilder::appendCode: raCodeStack is empty.");
        raCodeStack.top() << code;
        return *this;
    }

    RaCodeBuilder& RaCodeBuilder::operator<<(const std::string& code)
    {
        if (code.empty()) return *this;
        if (raCodeStack.empty()) throw std::runtime_error("RaCodeBuilder::appendCode: raCodeStack is empty.");
        raCodeStack.top() << code;
        return *this;
    }

    RaCodeBuilder& RaCodeBuilder::operator<<(const ri::RI& ri)
    {
        *this << ri.toRACode();
        return *this;
    }

    void RaCodeBuilder::enterScope()
    {
        raCodeStack.push({});
    }

    void RaCodeBuilder::exitScope()
    {
        if (raCodeStack.empty()) throw std::runtime_error("RaCodeBuilder::exitScope: raCodeStack is empty.");
        raCodeStack.pop();
    }

    RaCodeBuilder& RaCodeBuilder::buildCurScope(std::string& target)
    {
        if (raCodeStack.empty()) throw std::runtime_error("RaCodeBuilder::buildCurScopeAndExit: raCodeStack is empty.");
        target = raCodeStack.top().str();
        return *this;
    }

    RaCodeBuilder& RaCodeBuilder::buildCurScope(RaCodeBuilder& builder)
    {
        if (raCodeStack.empty()) throw std::runtime_error("RaCodeBuilder::buildCurScopeAndExit: raCodeStack is empty.");
        builder << raCodeStack.top().str();
        return *this;
    }

    RaCodeBuilder& RaCodeBuilder::operator>>(std::string& target)
    {
        if (!raCodeStack.empty())
        {
            target = raCodeStack.top().str();
            raCodeStack.pop();
        } else
        {
            target = "";
        }
        return *this;
    }

    RaCodeBuilder& RaCodeBuilder::operator>>(RaCodeBuilder& builder)
    {
        if (!raCodeStack.empty())
        {
            builder << raCodeStack.top().str();
            raCodeStack.pop();
        }
        return *this;
    }

    size_t CompileVisitor::VarID::_varId = 0;

    std::string CompileVisitor::VarID::rccVarPrefixField = "v";

    std::string CompileVisitor::VarID::_toVarID() {
        return rccVarPrefixField + uniqueId();
    }

    CompileVisitor::VarID::VarID(const std::string &name, const std::string &fileField, const std::string &scopeField)
        : id(_varId ++), fileField(fileField),
          scopeField(scopeField), nameField(name),
          vid(_toVarID()) {}

    std::string CompileVisitor::VarID::toString() const {
        return rccVarPrefixField + "_" + nameField + "_" + fileField + "_" + scopeField + "_" + std::to_string(id);
    }

    std::string CompileVisitor::VarID::getNameField() const {
        return nameField;
    }

    std::string CompileVisitor::VarID::getScopeField() const {
        return scopeField;
    }

    std::string CompileVisitor::VarID::getFileField() const {
        return fileField;
    }

    size_t CompileVisitor::VarID::getId() const {
        return id;
    }

    std::string CompileVisitor::VarID::getVid() const {
        return vid;
    }

    size_t CompileVisitor::SetID::_setId = 0;

    std::string CompileVisitor::SetID::rccSetPrefixField = "s";

    std::string CompileVisitor::SetID::_toSetID()
    {
        return rccSetPrefixField + uniqueId();
    }

    CompileVisitor::SetID::SetID(
        const std::string& name, const std::string& fileField,
        const std::string& scopeField)
        : id(_setId ++), fileField(fileField),
          scopeField(scopeField), nameField(name), sid(_toSetID()){}

    std::string CompileVisitor::SetID::toString() const
    {
        return Object::toString();
    }

    std::string CompileVisitor::SetID::getNameField() const
    {
        return nameField;
    }

    std::string CompileVisitor::SetID::getScopeField() const
    {
        return scopeField;
    }

    std::string CompileVisitor::SetID::getFileField() const
    {
        return fileField;
    }

    size_t CompileVisitor::SetID::getId() const
    {
        return id;
    }

    std::string CompileVisitor::SetID::getSid() const
    {
        return sid;
    }

    CompileVisitor::OpItem::OpItem(
        const OpItemType &type,
        const std::shared_ptr<TypeLabelSymbol> &typeLabelSymbol,
        const std::string &value,
        const std::string &raValue,
        const std::shared_ptr<TypeLabelSymbol> &valueType,
        const std::shared_ptr<Symbol> &referencedSymbol)
        : value(value), raValue(type == OpItemType::LITERAL_VALUE || type == OpItemType::SET_LABEL ?
                                    value : raValue), type(type), typeLabel(typeLabelSymbol), valueType(valueType), referencedSymbol(referencedSymbol)
    {
        if (type == OpItemType::LITERAL_VALUE || type == OpItemType::SET_LABEL)
        {
            this->valueType = typeLabel;
        }
        if (!this->valueType)
        {
            this->valueType = TypeLabelSymbol::nulTypeSymbol(
                typeLabelSymbol->getPos(),
                typeLabelSymbol->getScopeLevel());
        }
    }

    bool CompileVisitor::OpItem::is(const OpItemType& opItemType) const
    {
        return opItemType == this->type;
    }

    bool CompileVisitor::OpItem::isNot(const OpItemType& opItemType) const
    {
        return opItemType != this->type;
    }

    std::string CompileVisitor::OpItem::getVal() const {
        return value;
    }

    CompileVisitor::OpItemType CompileVisitor::OpItem::getType() const {
        return type;
    }

    std::string CompileVisitor::OpItem::getRaVal(const SymbolTableManager &table) const
    {
        if (type == OpItemType::LITERAL_VALUE || type == OpItemType::SET_LABEL)
        {
            return value;
        }
        if (const auto &[level, symbol] = table.find(value);
            level >= 0 && symbol)
        {
            return symbol->getRaVal();
        }
        return raValue;
    }

    std::shared_ptr<TypeLabelSymbol> CompileVisitor::OpItem::getTypeLabel() const
    {
        return typeLabel;
    }

    std::shared_ptr<TypeLabelSymbol> CompileVisitor::OpItem::getValueType() const
    {
        return valueType;
    }

    std::shared_ptr<Symbol> CompileVisitor::OpItem::getBelonging() const
    {
        return belonging;
    }

    std::string CompileVisitor::OpItem::getBelongAttrRaValue() const
    {
        return belongAttrRaValue;
    }

    std::shared_ptr<Symbol> CompileVisitor::OpItem::getReferencedSymbol() const
    {
        return referencedSymbol;
    }

    void CompileVisitor::OpItem::setTypeLabel(const std::shared_ptr<TypeLabelSymbol>& typeLabelSymbol)
    {
        this->typeLabel = typeLabelSymbol;
    }

    void CompileVisitor::OpItem::setValueType(const std::shared_ptr<TypeLabelSymbol>& valueTypeSymbol)
    {
        if (typeLabel->is("any") || valueTypeSymbol->is("any") || typeLabel->equalWith(valueTypeSymbol))
        {
            this->valueType = valueTypeSymbol;
        } else
        {
            throw std::runtime_error("CompileVisitor::OpItem::setValueType: type mismatch.");
        }
    }

    void CompileVisitor::OpItem::setBelonging(
        const std::shared_ptr<Symbol>& belongingSymbol, const std::string &belongAttrRaVal)
    {
        this->belonging = belongingSymbol;
        this->belongAttrRaValue = belongAttrRaVal;
    }

    void CompileVisitor::OpItem::setReferencedSymbol(const std::shared_ptr<Symbol>& symbol)
    {
        this->referencedSymbol = symbol;
    }

    size_t CompileVisitor::_temVarId = 0;

    size_t CompileVisitor::_setId = 0;

    std::unordered_map<std::string,
            std::shared_ptr<ClassSymbol>> CompileVisitor::extensionMap = {};

    // 辅助函数：获取符号的类型标签
    std::shared_ptr<TypeLabelSymbol> getTypeLabelFromSymbol(const std::shared_ptr<Symbol>& symbol) {
        if (!symbol) return nullptr;
        switch (symbol->getType()) {
        case SymbolType::VARIABLE:
        case SymbolType::PARAMETER:
            return std::static_pointer_cast<VariableSymbol>(symbol)->getTypeLabel();
        case SymbolType::FUNCTION: {
                return std::static_pointer_cast<FunctionSymbol>(symbol)->hasReturnValue() ?
                TypeLabelSymbol::funiTypeSymbol(symbol->getPos(), symbol->getScopeLevel()) :
                TypeLabelSymbol::funcTypeSymbol(symbol->getPos(), symbol->getScopeLevel());
        }
        case SymbolType::CLASS:
            return TypeLabelSymbol::clasTypeSymbol(symbol->getPos(), symbol->getScopeLevel());
        case SymbolType::LABEL:
            return std::static_pointer_cast<TypeLabelSymbol>(symbol);
        default:
            return nullptr;
        }
    }

    // 辅助函数：获取符号的类型标签
    std::shared_ptr<TypeLabelSymbol> getValueTypeFromSymbol(const std::shared_ptr<Symbol>& symbol) {
        if (!symbol) return nullptr;

        switch (symbol->getType()) {
        case SymbolType::VARIABLE:
        case SymbolType::PARAMETER:
            return std::static_pointer_cast<VariableSymbol>(symbol)->getValueType();
        case SymbolType::FUNCTION: {
                const auto &funcSymbol = std::static_pointer_cast<FunctionSymbol>(symbol);
                return funcSymbol->getReturnType();
        }
        case SymbolType::CLASS:
            return TypeLabelSymbol::clasTypeSymbol(Pos::UNKNOW_POS, symbol->getScopeLevel());
        case SymbolType::LABEL:
            return std::static_pointer_cast<TypeLabelSymbol>(symbol);
        default:
            return nullptr;
        }
    }

    // 检查两个TypeLabelSymbol是否匹配
    bool CompileVisitor::checkTypeMatch(const std::shared_ptr<TypeLabelSymbol>& leftTypeSymbol,
                                        const std::shared_ptr<TypeLabelSymbol>& rightTypeSymbol)
    {
        // 空指针检查
        if (!leftTypeSymbol || !rightTypeSymbol) {
            // 可以根据需要添加日志或调试信息
            return false;
        }

        // 类型完全匹配
        if (leftTypeSymbol->equalWith(rightTypeSymbol)) {
            return true;
        }

        // 处理any类型，any可以匹配任何类型
        if (leftTypeSymbol->is("any") || rightTypeSymbol->is("any")) {
            return true;
        }

        // 处理null类型，通常null只能匹配引用类型或any
        if (leftTypeSymbol->is("nul")) {
            // 假设引用类型以"*"结尾或有其他标识
            return /* rightTypeSymbol->isReferenceType() || */ rightTypeSymbol->is("any");
        }
        if (rightTypeSymbol->is("nul")) {
            return /* leftTypeSymbol->isReferenceType() || */ leftTypeSymbol->is("any");
        }

        // // 检查类型继承关系（如果支持）
        // if (leftTypeSymbol->isSubtypeOf(rightTypeSymbol) ||
        //     rightTypeSymbol->isSubtypeOf(leftTypeSymbol)) {
        //     return true;
        // }

        return false;
    }

    // 检查两个Symbol是否类型匹配
    bool CompileVisitor::checkTypeMatch(const std::shared_ptr<Symbol>& leftSymbol,
                                        const std::shared_ptr<Symbol>& rightSymbol)
    {
        // 空指针检查
        if (!leftSymbol || !rightSymbol) {
            return false;
        }

        // 如果是相同的符号，直接返回true
        if (leftSymbol->equalWith(rightSymbol)) {
            return true;
        }

        // 获取双方的类型标签
        const auto leftType = getTypeLabelFromSymbol(leftSymbol);

        if (const auto rightType = getTypeLabelFromSymbol(rightSymbol);
            rightType && rightType->isNot("any"))
        {
            return checkTypeMatch(leftType, rightType);
        }

        const auto rightValueType = getValueTypeFromSymbol(rightSymbol);

        // 检查类型标签是否匹配
        return checkTypeMatch(leftType, rightValueType);
    }

    // 从OpItem获取符号
    std::shared_ptr<Symbol> CompileVisitor::getSymbolFromOpItem(const OpItem& opItem) const
    {
        if (opItem.getType() == OpItemType::IDENTIFIER) {
            if (const auto &[level, symbol] = symbolTable.find(opItem.getVal());
                level >= 0 && symbol) {
                return symbol;
            }
        }
        return nullptr;
    }

    std::string CompileVisitor::raVal(const OpItem& opItem) const
    {
        return opItem.getRaVal(symbolTable);
    }

    // 检查Symbol与OpItem是否类型匹配
    bool CompileVisitor::checkTypeMatch(const std::shared_ptr<Symbol>& leftSymbol,
                                        const OpItem& rightOpItem) const
    {
        if (!leftSymbol) {
            return false;
        }

        // 从OpItem获取符号
        if (const auto& rightSymbol = getSymbolFromOpItem(rightOpItem)) {
            // 如果获取到符号，使用符号间的类型检查
            return checkTypeMatch(leftSymbol, rightSymbol);
        }

        // 处理没有符号的情况，直接使用OpItem的类型标签
        const auto& rightTypeLabel = rightOpItem.getTypeLabel();
        if (!rightTypeLabel) {
            return false;
        }

        // 获取左符号的类型标签
        const auto leftTypeLabel = getTypeLabelFromSymbol(leftSymbol);
        if (!leftTypeLabel) {
            return false;
        }

        // 特殊处理any类型
        if (rightTypeLabel->is("any")) {
            return true;
        }

        // 检查类型标签匹配
        return checkTypeMatch(leftTypeLabel, rightOpItem.getValueType());
    }


    void CompileVisitor::processTypeAutoChange(const std::shared_ptr<Symbol>& sourceSymbol,
                                               const OpItem &targetOpItem) const
    {
        switch (sourceSymbol->getType())
        {
        case SymbolType::VARIABLE:
            {
                // 尝试将sourceSymbol转换为VariableSymbol
                const auto &varSymbol = std::static_pointer_cast<VariableSymbol>(sourceSymbol);
                const auto &targetSymbol = getSymbolFromOpItem(targetOpItem);
                const auto &targetValueType = targetSymbol
                ? getValueTypeFromSymbol(targetSymbol)
                : targetOpItem.getValueType();
                const auto &targetTypeLabel = targetSymbol
                ? getTypeLabelFromSymbol(targetSymbol)
                : targetOpItem.getTypeLabel();

                // 源类型为"any"时，直接采用目标值类型
                if (varSymbol->getTypeLabel()->is("any")) {
                    varSymbol->setValueType(targetValueType);
                    return; // 早期返回，减少嵌套
                }

                // 目标类型为"any"的情况处理
                if (targetTypeLabel->is("any"))
                {
                    // 确保目标值类型不为空
                    if (!targetValueType) {
                        throw std::runtime_error("[CompileVisitor::processTypeAutoChange] targetOpItem->getValueType() is null");
                    }

                    // 类型不匹配则抛出异常
                    if (targetValueType->isNot("any") && !varSymbol->getTypeLabel()->equalWith(targetValueType)) {
                        throw std::runtime_error("[CompileVisitor::processTypeAutoChange] varSymbol->getTypeLabel() != "
                                                 "targetOpItem->getValueType()");
                    }
                    varSymbol->setValueType(targetValueType);
                    return;
                }

                // 源类型和目标类型都不是"any"时，检查是否匹配
                if (!varSymbol->getTypeLabel()->equalWith(targetTypeLabel)) {
                    throw std::runtime_error("[CompileVisitor::processTypeAutoChange] varSymbol->getTypeLabel() != targetOpItem->getTypeLabel()");
                }
            } break;
        case SymbolType::FUNCTION:
            {

            } break;
        case SymbolType::LABEL:
            {

            } break;
        case SymbolType::PARAMETER:
            {

            } break;
        case SymbolType::CLASS:
            {

            } break;
        }
    }

    void CompileVisitor::processTypeAutoChange(const std::shared_ptr<Symbol>& sourceSymbol,
        const std::shared_ptr<OpItem>& targetOpItem) const
    {
        processTypeAutoChange(sourceSymbol, *targetOpItem);
    }

    std::shared_ptr<TypeLabelSymbol> CompileVisitor::getBuiltinTypeSymbol(const Pos& pos, const BuiltinType& type) const
    {
        std::string name;
        switch (type)
        {
        case B_ANY: name = "any";
            break;
        case B_BOOL: name = "bool";
            break;
        case B_CHAR: name = "char";
            break;
        // ReSharper disable once CppDFAUnreachableCode
        case B_DICT: name = "dict";
            break;
        case B_FLOAT: name = "float";
            break;
        case B_INT: name = "int";
            break;
        // ReSharper disable once CppDFAUnreachableCode
        case B_LIST: name = "list";
            break;
        case B_STR: name = "str";
            break;
        case B_VOID: name = "void";
            break;
        case B_NUL: name = "nul";
            break;
        case B_FLAG: name = "flag";
            break;
        case B_FUNC: name = "func";
            break;
        case B_FUNI: name = "funi";
            break;
        default:
            pass("To process other builtin type");
        }
        return std::make_shared<TypeLabelSymbol>(pos, name, symbolTable.curScopeLevel());
    }

    std::pair<std::shared_ptr<TypeLabelSymbol>, std::shared_ptr<TypeLabelSymbol>> CompileVisitor::
    getTypesFromOpItem(const OpItem& opItem) const
    {
        const auto &symbol = getSymbolFromOpItem(opItem);
        const auto &typeLabel = symbol
        ? getTypeLabelFromSymbol(symbol)
        : opItem.getTypeLabel();
        const auto &valueType = symbol
        ? getValueTypeFromSymbol(symbol)
        : opItem.getValueType();
        return {typeLabel, valueType};
    }

    std::string CompileVisitor::formatAttrField(const std::string& field)
    {
        return "\"" + StringManager::escape(field) + "\"";
    }

    void CompileVisitor::annotatePos(const Pos& pos)
    {
        raCodeBuilder << ri::ANNOTATION(pos.toString());
    }

    std::shared_ptr<Symbol> CompileVisitor::getReferenceTargetSymbol(const OpItem& opItem)
    {
        std::shared_ptr<Symbol> reference = opItem.getReferencedSymbol();
        while (reference)
        {
            if (reference->is(SymbolType::VARIABLE))
            {
                if (const auto ref =
                    std::static_pointer_cast<VariableSymbol>(reference)->getReferencedSymbol())
                    reference = ref;
                else
                    break;
            } else
            {
                break;
            }
        }
        return reference;
    }

    std::shared_ptr<Symbol> CompileVisitor::getReferenceTargetSymbol(
        const std::shared_ptr<VariableSymbol>& varSymbol)
    {
        std::shared_ptr<Symbol> reference = varSymbol->getReferencedSymbol();
        while (reference)
        {
            if (reference->is(SymbolType::VARIABLE))
            {
                if (const auto &ref = std::dynamic_pointer_cast<VariableSymbol>(reference)->getReferencedSymbol())
                    reference = ref;
                else
                    break;
            } else
            {
                break;
            }
        }
        return reference;
    }

    void CompileVisitor::registerExtension(const std::string& extensionPath,
        const std::shared_ptr<ClassSymbol>& extensionClass)
    {
        extensionMap[extensionPath] = extensionClass;
    }

    std::shared_ptr<ClassSymbol> CompileVisitor::getRegisteredExtension(const std::string& extensionPath)
    {
        if (const auto &it = extensionMap.find(extensionPath);
            it != extensionMap.end())
        {
            return it->second;
        }
        return nullptr;
    }

    bool CompileVisitor::isExtensionRegistered(const std::string& extensionPath)
    {
        return extensionMap.contains(extensionPath);
    }

    void CompileVisitor::setSymbolBuiltinType(const std::shared_ptr<Symbol>& processingSymbol, const symbol::TypeOfBuiltin& type)
    {
        if (processingSymbol->is(SymbolType::FUNCTION))
        {
            const auto &funcSymbol = std::static_pointer_cast<FunctionSymbol>(processingSymbol);
            funcSymbol->setBuiltInType(type);
        }
        throw std::runtime_error("[CompileVisitor::setSymbolBuiltinType] processingSymbol->is(SymbolType::FUNCTION)");
    }

    bool CompileVisitor::checkSymbolExists(const OpItem& opItem) const
    {
        return opItem.is(OpItemType::IDENTIFIER) && symbolTable.contains(opItem.getVal());
    }

    bool CompileVisitor::checkSymbolExists(const std::shared_ptr<OpItem>& opItem) const
    {
        return opItem && opItem->is(OpItemType::IDENTIFIER) &&
            symbolTable.contains(opItem->getVal());
    }

    bool CompileVisitor::checkSymbolExists(const std::shared_ptr<Symbol>& processingSymbol) const
    {
        return processingSymbol && symbolTable.contains(processingSymbol->getVal());
    }

    void CompileVisitor::checkExists(const OpItem& opItem) const
    {
        if (opItem.is(OpItemType::IDENTIFIER) && !checkSymbolExists(opItem))
        {
            throw std::runtime_error("[CompileVisitor::checkExists] !checkSymbolExists(opItem): '" + opItem.getVal() + "'");
        }
    }

    void CompileVisitor::checkExists(const std::shared_ptr<OpItem>& opItem) const
    {
        if (opItem->is(OpItemType::IDENTIFIER) && !checkSymbolExists(opItem))
        {
            throw std::runtime_error("[CompileVisitor::checkExists] !checkSymbolExists(opItem): '" + opItem->getVal() + "'");
        }
    }

    void CompileVisitor::checkExists(const std::shared_ptr<symbol::Symbol>& processingSymbol) const
    {
        if (!checkSymbolExists(processingSymbol))
        {
            throw std::runtime_error("[CompileVisitor::checkExists] !checkSymbolExists(processingSymbol): '" + processingSymbol->getVal() + "'");
        }
    }

    void CompileVisitor::pushNewProcessingSymbol(const std::shared_ptr<Symbol>& symbol)
    {
        processingSymbolStack.push(symbol);
    }

    std::shared_ptr<Symbol> CompileVisitor::topProcessingSymbol()
    {
        return processingSymbolStack.top();
    }

    void CompileVisitor::popProcessingSymbol()
    {
        processingSymbolStack.pop();
    }

    std::shared_ptr<Symbol> CompileVisitor::rPopProcessingSymbol()
    {
        auto symbol = processingSymbolStack.top();
        processingSymbolStack.pop();
        return symbol;
    }

    SymbolType CompileVisitor::topProcessingSymbolType()
    {
        return topProcessingSymbol()->getType();
    }

    std::string CompileVisitor::topProcessingSymbolRaVal()
    {
        return topProcessingSymbol()->getRaVal();
    }

    std::string CompileVisitor::topProcessingSymbolVal()
    {
        return topProcessingSymbol()->getVal();
    }

    bool CompileVisitor::isProcessingSymbol() const
    {
        return !processingSymbolStack.empty();
    }

    std::string CompileVisitor::getNewTempVarName() {
        _temVarId ++;
        return "tv" + std::to_string(_temVarId);
    }

    std::string CompileVisitor::getNewSetLabelName()
    {
        _setId ++;
        return "sl" + std::to_string(_setId);
    }

    CompileVisitor::OpItem CompileVisitor::pushTemOpVarItemWithRecord(
        const Pos &pos, const std::shared_ptr<TypeLabelSymbol>& valueType,
        const std::shared_ptr<Symbol> &referencedSymbol)
    {
        const auto &tempVarId = VarID(getNewTempVarName(), pos.getFileField(), curScopeField());
        pushIdentItem(tempVarId, getBuiltinTypeSymbol(pos, B_ANY), valueType, referencedSymbol);
        const auto &anyType = TypeLabelSymbol::anyTypeSymbol(pos, symbolTable.curScopeLevel());
        symbolTable.insert(std::make_shared<VariableSymbol>(
            pos, tempVarId.getNameField(), topOpItem().getRaVal(symbolTable),
            std::unordered_set<std::shared_ptr<LabelSymbol>>{
                anyType},
            symbolTable.curScopeLevel(),
            false,
            valueType ? valueType : anyType,
            nullptr, referencedSymbol));
        raCodeBuilder << ri::ALLOT({tempVarId.getVid()});
        return topOpItem();
    }

    CompileVisitor::OpItem CompileVisitor::pushTemOpSetItem(const Pos& pos)
    {
        const auto &setId = SetID(getNewSetLabelName(), pos.getFileField(), curScopeField());
        pushOpItem(std::make_shared<OpItem>(
            OpItemType::SET_LABEL,
            TypeLabelSymbol::flagTypeSymbol(pos, symbolTable.curScopeLevel()),
            setId.getSid(),
            setId.getSid()));
        return topOpItem();
    }

    CompileVisitor::OpItem CompileVisitor::newTemOpSetItem(const Pos& pos)
    {
        const auto &setId = SetID(getNewSetLabelName(), pos.getFileField(), curScopeField());
        return OpItem{
            OpItemType::SET_LABEL,
            TypeLabelSymbol::flagTypeSymbol(pos, symbolTable.curScopeLevel()),
            setId.getSid(),
            setId.getSid()};
    }

    CompileVisitor::VarID CompileVisitor::getThisFieldVarID(const Pos& pos)
    {
        return VarID("this", pos.getFileField(), curScopeField());
    }

    std::shared_ptr<VariableSymbol> CompileVisitor::getThisFieldSymbol(const std::shared_ptr<ClassSymbol>& classSymbol)
    {
        const auto &thisFieldVarID = getThisFieldVarID(classSymbol->getPos());
        return std::make_shared<VariableSymbol>(
            Pos::UNKNOW_POS,
            thisFieldVarID.getNameField(),
            thisFieldVarID.getVid(),
            std::unordered_set<std::shared_ptr<LabelSymbol>>{
                TypeLabelSymbol::getCustomTypeLabelSymbol(classSymbol->getRaVal(), symbolTable.curScopeLevel())},
                symbolTable.curScopeLevel(), true, nullptr, nullptr);
    }

    void CompileVisitor::pushOpItem(const std::shared_ptr<OpItem> &opItem) {
        opItemStack.push(opItem);
    }

    CompileVisitor::OpItem CompileVisitor::rPopOpItem() {
        if (!hasNextOpItem()) {
            throw std::runtime_error("OpItem stack is empty, can not pop.");
        }
        auto item = *opItemStack.top();
        opItemStack.pop();
        return item;
    }

    void CompileVisitor::popOpItem() {
        if (!hasNextOpItem()) {
            throw std::runtime_error("OpItem stack is empty, can not pop.");
        }
        opItemStack.pop();
    }

    void CompileVisitor::pushOpItem(
        const OpItemType &type,
        const std::shared_ptr<TypeLabelSymbol> &typeLabelSymbol,
        const std::string &value,
        const std::string &racode,
        const std::shared_ptr<Symbol>& referencedSymbol, const std::shared_ptr<TypeLabelSymbol>& valueTypeSymbol) {
        pushOpItem(std::make_shared<OpItem>(
            type, typeLabelSymbol, value, racode,
            valueTypeSymbol, referencedSymbol));
    }

    void CompileVisitor::pushOpItem(
        const std::string &name,
        const std::string &fileField,
        const std::string &scopeField,
        const std::shared_ptr<TypeLabelSymbol> &typeLabelSymbol) {
        pushOpItem(std::make_shared<OpItem>(
            OpItemType::IDENTIFIER,
            typeLabelSymbol,
            name,
            VarID(name, fileField, scopeField).getVid()));
    }

    void CompileVisitor::pushIdentItem(
        const VarID &varID,
        const std::shared_ptr<TypeLabelSymbol> &typeLabelSymbol,
        const std::shared_ptr<TypeLabelSymbol> &valueType,
        const std::shared_ptr<Symbol> &referencedSymbol)
    {
        pushOpItem(std::make_shared<OpItem>(
            OpItemType::IDENTIFIER,
            typeLabelSymbol,
            varID.getNameField(),
            varID.getVid(),
            valueType,
            referencedSymbol));
    }

    CompileVisitor::OpItem CompileVisitor::topOpItem() const {
        if (!hasNextOpItem()) {
            throw std::runtime_error("OpItem stack is empty, can not get top OpItem.");
        }
        return *opItemStack.top();
    }

    std::shared_ptr<CompileVisitor::OpItem> CompileVisitor::topOpItemPtr() const
    {
        if (!hasNextOpItem()) {
            throw std::runtime_error("OpItem stack is empty, can not get top OpItem.");
        }
        return opItemStack.top();
    }

    std::string CompileVisitor::topOpRaVal() const {
        return topOpItem().getRaVal(symbolTable);
    }

    std::string CompileVisitor::rPopOpItemRaVal() {
        return rPopOpItem().getRaVal(symbolTable);
    }

    std::string CompileVisitor::rPopOpItemVal()
    {
        return rPopOpItem().getVal();
    }

    bool CompileVisitor::hasNextOpItem() const {
        return !opItemStack.empty();
    }

    CompileVisitor::CompileVisitor(
        const std::string& programEntryFilePath,
        const std::string& programTagetFilePath,
        const std::string& compileOutputFilePath,
        const bool &needSaveOutput)
        : programEntryFilePath(programEntryFilePath), programTagetFilePath(programTagetFilePath),
    compileOutputFilePath(compileOutputFilePath), needSaveOutputToFile(needSaveOutput)
    {
        builtin::initializeFullBuiltinEnvironment(*this);
    }

    SymbolTableManager& CompileVisitor::getSymbolTable()
    {
        return symbolTable;
    }

    std::stack<std::shared_ptr<Symbol>>& CompileVisitor::getProcessingSymbolStack()
    {
        return processingSymbolStack;
    }

    std::stack<std::shared_ptr<CompileVisitor::OpItem>>& CompileVisitor::getOpItemStack()
    {
        return opItemStack;
    }

    std::stack<CompileVisitor::ScopeType>& CompileVisitor::getScopeTypeStack()
    {
        return scopeTypeStack;
    }

    std::string CompileVisitor::getProgramEntryFilePath() const
    {
        return programEntryFilePath;
    }

    std::string CompileVisitor::getProgramTargetFilePath() const
    {
        return programTagetFilePath;
    }

    std::string CompileVisitor::getCompileOutputFilePath() const
    {
        return compileOutputFilePath;
    }

    std::string CompileVisitor::getCurrentProcessingFilePath() const
    {
        return currentProcessingFilePath;
    }

    void CompileVisitor::setCurrentProcessingFilePath(const std::string& filePath)
    {
        currentProcessingFilePath = filePath;
    }

    std::string CompileVisitor::scopeTypeToString(ScopeType scopeType) {
        switch (scopeType) {
        case ScopeType::TEMPORARY: return "temporary";
        case ScopeType::GLOBAL: return "global";
        case ScopeType::PROGRAM: return "program";
        case ScopeType::EXPRESSION: return "expression";
        case ScopeType::CLASS: return "class";
        case ScopeType::FUNCTION: return "function";
        case ScopeType::LOOP: return "loop";
        case ScopeType::CONDITION: return "condition";
        default: return "unknown";
        }
    }

    std::string CompileVisitor::curScopeField() {
        return scopeTypeToString(scopeTypeStack.top());
    }

    void CompileVisitor::enterScope(ScopeType scopeType) {
        scopeTypeStack.push(scopeType);
        symbolTable.enterScope();
    }

    void CompileVisitor::enterScope(size_t scopeLevel) {
        symbolTable.enterScope(scopeLevel);
    }

    void CompileVisitor::enterTopScope() {
        symbolTable.enterTopScope();
    }

    void CompileVisitor::enterGlobalScope() {
        scopeTypeStack.push(ScopeType::GLOBAL);
        symbolTable.enterGlobalScope();
    }

    void CompileVisitor::exitScope(ScopeType scopeType) {
        if (scopeTypeStack.empty()) {
            rlog::LogManager::rerror("Scope filed is empty, can not exit global scope.");
            return;
        }
        if (scopeTypeStack.top() != scopeType) {
            rlog::LogManager::rerror("Scope type is not equal, can not exit scope.");
            return;
        }
        scopeTypeStack.pop();
        symbolTable.exitScope();
    }

    size_t CompileVisitor::curScopeLevel() const
    {
        return symbolTable.curScopeLevel();
    }

    std::vector<std::shared_ptr<ExpressionNode>> CompileVisitor::visitParallelNode(
        const std::shared_ptr<ExpressionNode> &node)
    {
        std::vector<std::shared_ptr<ExpressionNode>> result{};
        if (!node)
        {
            return result;
        }
        std::shared_ptr<ExpressionNode> p = nullptr;
        std::queue<std::shared_ptr<ExpressionNode>> nodeQueue{};
        nodeQueue.push(node);
        while (!nodeQueue.empty())
        {
            p = nodeQueue.front();
            if (p->getType() == NodeType::INFIX && p->getRealType() == NodeType::PARALLEL)
            {
                const auto& pNode = std::static_pointer_cast<InfixExpressionNode>(p);
                nodeQueue.push(pNode->getRightNode());
                nodeQueue.push(pNode->getLeftNode());
            }
            else
            {
                result.push_back(p);
            }
            nodeQueue.pop();
        }
        std::reverse(result.begin(), result.end());
        return result;
    }

    std::unordered_set<std::shared_ptr<LabelSymbol>> CompileVisitor::processLabelNodes(
        const std::vector<std::shared_ptr<LabelNode>>& labels)
    {
        enterScope(ScopeType::TEMPORARY);
        for (const auto& label: labels) {
            label->acceptVisitor(*this);
        }
        std::unordered_set<std::shared_ptr<LabelSymbol>> result {};
        for (const auto& [_, symbol]: symbolTable.currentScope().getTable()) {
            if (symbol.second->getType() != SymbolType::LABEL)
            {
                throw std::runtime_error("Symbol is not a label");
            }
            result.insert(std::static_pointer_cast<LabelSymbol>(symbol.second));
        }
        exitScope(ScopeType::TEMPORARY);
        return result;
    }

    bool CompileVisitor::compile()
    {
        const auto &tokens = lexer::Lexer::tokenize(programTagetFilePath);
        parser::Parser parser (tokens);
        const auto &[hasError, programNode] = parser.parse();
        if (hasError) {
            parser.printParserErrors();
            return false;
        }
        if (needSaveOutputToFile)
        {
            raCodeBuilder
            << ri::ANNOTATION(std::vector<std::string>{
                RIO_PROGRAM_SIGN " " RCC_VERSION,
                "Source file: " + programTagetFilePath + ":1:1",
                "Target file: " + compileOutputFilePath + ":1:1",
                "Author: @RestRegular",
                "Time: " + getCurrentTime(TimeFormat::ISO_WITH_TIME),
                ""
            });
        }
        programNode->acceptVisitor(*this);
        const std::string raCode = raCodeBuilder.buildAll();
        if (needSaveOutputToFile)
        {
            writeFile(compileOutputFilePath, raCode);
        }
        return true;
    }

    std::string CompileVisitor::getCompileResult()
    {
        return raCodeBuilder.buildAll();
    }

    void CompileVisitor::visitLiteralNode(LiteralNode &node) {
        rlog::LogManager::rerror("This should not be execute.");
    }

    void CompileVisitor::visitNullLiteralNode(NullLiteralNode& node)
    {
        pushOpItem(
            OpItemType::LITERAL_VALUE,
            getBuiltinTypeSymbol(node.getPos(), B_NUL),
            node.literalString());
    }

    void CompileVisitor::visitStringLiteralNode(StringLiteralNode &node) {
        pushOpItem(
            OpItemType::LITERAL_VALUE,
            getBuiltinTypeSymbol(node.getPos(), B_STR),
            "\"" + StringManager::escape(node.literalString()) + "\"");
    }

    void CompileVisitor::visitNumberLiteralNode(NumberLiteralNode &node) {
        rlog::LogManager::rerror("This should not be execute.");
    }

    void CompileVisitor::visitIntegerLiteralNode(IntegerLiteralNode &node) {
        pushOpItem(
            OpItemType::LITERAL_VALUE,
            getBuiltinTypeSymbol(node.getPos(), B_INT),
            node.literalString());
    }

    void CompileVisitor::visitFloatLiteralNode(FloatLiteralNode &node) {
        pushOpItem(
            OpItemType::LITERAL_VALUE,
            getBuiltinTypeSymbol(node.getPos(), B_FLOAT),
            node.literalString());
    }

    void CompileVisitor::visitBooleanLiteralNode(BooleanLiteralNode &node) {
        pushOpItem(
            OpItemType::LITERAL_VALUE,
            getBuiltinTypeSymbol(node.getPos(), B_BOOL),
            node.literalString());
    }

    void CompileVisitor::visitCharacterLiteralNode(CharacterLiteralNode &node) {
        pushOpItem(
            OpItemType::LITERAL_VALUE,
            getBuiltinTypeSymbol(node.getPos(), B_CHAR),
            "\"" + std::string(1, StringManager::escapeChar(node.literalString()[0]))
        + "\"");
    }

    void CompileVisitor::visitIdentifierNode(IdentifierNode &node) {
        const auto &anyTypeLabel = getBuiltinTypeSymbol(node.getPos(), B_ANY);
        if (const auto &[level, symbol] = symbolTable.find(node.getName());
            level >= 0 && symbol)
        {
            pushOpItem(OpItemType::IDENTIFIER, anyTypeLabel,
                symbol->getVal(), symbol->getRaVal(), symbol, anyTypeLabel);
            return;
        }
        pushIdentItem({node.getName(), node.getPos().getFileField(), curScopeField()},
            getBuiltinTypeSymbol(node.getPos(), B_ANY), anyTypeLabel, nullptr);
    }

    void CompileVisitor::visitParameterNode(ParameterNode &node) {

        pass("To visit " + getNodeTypeName(node.getRealType()) + " type node.");
    }

    void CompileVisitor::visitArgumentNode(ArgumentNode &node) {

        pass("To visit " + getNodeTypeName(node.getRealType()) + " type node.");
    }

    void CompileVisitor::visitFunctionDeclarationNode(FunctionDeclarationNode &node) {

        pass("To visit " + getNodeTypeName(node.getRealType()) + " type node.");
    }

    void CompileVisitor::visitConstructorDefinitionNode(ConstructorDefinitionNode &node) {
        // 仅仅记录此构造函数，编译推迟到记录结束后进行
        enterScope(ScopeType::FUNCTION);
        const auto &topSymbol = topProcessingSymbol();
        if (topSymbol->getType() != SymbolType::CLASS)
        {
            throw std::runtime_error("Constructor definition must be in a class.");
        }
        const auto &classSymbol = std::static_pointer_cast<ClassSymbol>(topSymbol);
        const auto &paramItems = visitParallelNode(std::static_pointer_cast<ParenRangerNode>(node.getParamNode())->getRangerNode());
        std::vector<std::shared_ptr<ParameterSymbol>> paramSymbols {};
        std::vector<std::string> paramIdents {};
        for (const auto &item: paramItems)
        {
            auto paramType = ParamType::PARAM_POSITIONAL;
            if (item->getRealType() != NodeType::IDENTIFIER &&
                item->getRealType() != NodeType::ASSIGNMENT &&
                item->getRealType() != NodeType::UNARY)
            {
                throw std::runtime_error("Ctor call parameter must be an identifier or an assignment or a unary.");
            }
            auto param = item;
            std::optional<OpItem> defaultValue = std::nullopt;
            if (item->getRealType() == NodeType::ASSIGNMENT)
            {
                const auto &[ident, value] =
                    std::static_pointer_cast<AssignmentNode>(item)->getAssignPair();
                param = ident;
                value->acceptVisitor(*this);
                defaultValue = rPopOpItem();
                paramType = ParamType::PARAM_KEYWORD;
            } else if (item->getRealType() == NodeType::UNARY)
            {
                if (const auto &unaryParam =
                    std::static_pointer_cast<UnaryExpressionNode>(item);
                    unaryParam->getOpToken().getType() == TokenType::TOKEN_STAR)
                {
                    param = unaryParam->getRightNode();
                    paramType = ParamType::PARAM_VAR_LEN_POSITIONAL;
                } else if (unaryParam->getOpToken().getType() == TokenType::TOKEN_DOUBLE_STAR)
                {
                    param = unaryParam->getRightNode();
                    paramType = ParamType::PARAM_VAR_LEN_KEYWORD;
                } else
                {
                    throw std::runtime_error("Ctor call parameter must be an identifier or an assignment or a unary.");
                }
            }
            const auto &paramNode = std::static_pointer_cast<IdentifierNode>(param);
            paramNode->acceptVisitor(*this);
            const auto &paramOpItem = rPopOpItem();
            paramIdents.push_back(paramOpItem.getRaVal(symbolTable));
            std::unordered_set<std::shared_ptr<LabelSymbol>> labels = processLabelNodes(paramNode->getLabels());
            const auto &paramSymbol = std::make_shared<ParameterSymbol>(
                paramType,
                paramNode->getPos(),
                paramOpItem.getVal(),
                paramOpItem.getRaVal(symbolTable),
                labels,
                std::nullopt,
                symbolTable.curScopeLevel());
            if (defaultValue.has_value())
            {
                if (!checkTypeMatch(paramSymbol, defaultValue.value()))
                {
                    throw std::runtime_error("Function call parameter type mismatch.");
                }
                paramSymbol->setDefaultValue(defaultValue.value().getRaVal(symbolTable));
            }
            paramSymbols.push_back(paramSymbol);
            symbolTable.insert(std::make_shared<VariableSymbol>(
                item->getPos(),
                paramOpItem.getVal(),
                paramOpItem.getRaVal(symbolTable),
                labels,
                symbolTable.curScopeLevel(),
                true));
        }
        auto getCtorName = [&]
        {
            // 拼接所有参数标识符，用下划线分隔
            const std::string paramIdent = std::accumulate(
                paramSymbols.begin(),
                paramSymbols.end(),
                std::string(),
                [](const std::string& acc, const std::shared_ptr<ParameterSymbol>& ident) {
                    const auto &typeName = ident->getTypeLabel()->getVal();
                    return acc.empty() ? typeName : acc + "_" + typeName;
                }
            );
            return classSymbol->getVal() + (paramIdent.empty() ? "_empty" : "_" + paramIdent);
        };
        const auto &ctorName = getCtorName();
        const auto &ctorID = VarID(ctorName, node.getPos().getFileField(), curScopeField());
        const auto &funcNameOpItem = OpItem(
            OpItemType::IDENTIFIER, TypeLabelSymbol::funcTypeSymbol(Pos::UNKNOW_POS, symbolTable.curScopeLevel()),
            ctorName, ctorID.getVid());
        const auto &labels = processLabelNodes(node.getLabelNodes());
        const auto &functionSymbol = std::make_shared<FunctionSymbol>(
            std::make_shared<ConstructorDefinitionNode>(node),
            node.getPos(),
            funcNameOpItem.getVal(),
            funcNameOpItem.getRaVal(symbolTable),
            labels,
            paramSymbols,
            symbolTable.curScopeLevel() - 1,
            TypeOfBuiltin::ORDINARY, FunctionType::CONSTRUCTOR, classSymbol);
        if (!functionSymbol->getReturnType())
        {
            functionSymbol->setReturnType(getBuiltinTypeSymbol(functionSymbol->getPos(), B_ANY));
        }
        exitScope(ScopeType::FUNCTION);
        classSymbol->addMember(functionSymbol);
    }

    void CompileVisitor::visitClassDeclarationNode(ClassDeclarationNode &node) {
        pass("To visit " + getNodeTypeName(node.getRealType()) + " type node.");
    }

    std::string CompileVisitor::compileConstructorNode(
        const std::shared_ptr<FunctionSymbol> &ctorSymbol)
    {
        std::stringstream code{};
        const auto &node = std::static_pointer_cast<ConstructorDefinitionNode>(ctorSymbol->getDefinitionNode());
        enterScope(ScopeType::FUNCTION);
        const auto &topSymbol = topProcessingSymbol();
        if (topSymbol->getType() != SymbolType::CLASS)
        {
            throw std::runtime_error("Constructor can only be defined in class scope");
        }
        const auto &classSymbol = std::static_pointer_cast<ClassSymbol>(topSymbol);
        const auto &thisFieldSymbol = getThisFieldSymbol(classSymbol);
        symbolTable.insert(thisFieldSymbol);
        std::vector<std::string> paramIdents {};
        for (const auto &param: ctorSymbol->getParameters())
        {
            paramIdents.push_back(param->getRaVal());
            symbolTable.insert(VariableSymbol::paramSymbolToVarSymbol(param, symbolTable.curScopeLevel()));
        }
        // 定义构造函数，构造一个 this 字段用于存储构造的对象
        code
        << ri::FUNI(ctorSymbol->getRaVal(), paramIdents).toRACode()
        << ri::ALLOT({thisFieldSymbol->getRaVal()}).toRACode()
        << ri::TP_NEW(classSymbol->getRaVal(), thisFieldSymbol->getRaVal()).toRACode();
        // 执行构造函数体
        raCodeBuilder.enterScope();
        node->getBodyNode()->acceptVisitor(*this);
        std::string ctorBody {};
        raCodeBuilder.buildCurScope(ctorBody).exitScope();
        // 构造函数返回 this
        code
        << ctorBody
        << ri::RET(thisFieldSymbol->getRaVal()).toRACode()
        << ri::END(ctorSymbol->getRaVal()).toRACode();
        ctorSymbol->reSetReturnType(TypeLabelSymbol::getCustomTypeLabelSymbol(classSymbol->getRaVal(), curScopeLevel()));
        exitScope(ScopeType::FUNCTION);
        return code.str();
    }

    void CompileVisitor::visitClassDefinitionNode(ClassDefinitionNode &node) {
        const auto &classNameNode = std::static_pointer_cast<IdentifierNode>(node.getNameNode());
        raCodeBuilder.enterScope();
        classNameNode->acceptVisitor(*this);
        raCodeBuilder.exitScope();
        const auto &classLabels = processLabelNodes(classNameNode->getLabels());
        const auto &classNameItem = rPopOpItem();
        const auto &classSymbol = std::make_shared<ClassSymbol>(
            node.getPos(), classNameItem.getVal(),
            classNameItem.getRaVal(symbolTable),
            classLabels,
            symbolTable.curScopeLevel());
        TypeLabelSymbol::createCustomType(classSymbol->getVal(), classSymbol->getRaVal(), classSymbol);
        classSymbol->setVisitPermission(PermissionLabel::PUBLIC);
        pushNewProcessingSymbol(classSymbol);
        symbolTable.insert(classSymbol);
        // 收集类的成员变量、方法和构造函数
        for (const auto &members = std::static_pointer_cast<BlockRangerNode>(node.getBodyNode())->getBodyExpressions();
            const auto &member: members)
        {
            switch (member->getRealType())
            {
            case NodeType::VAR:
                {
                    enterScope(ScopeType::TEMPORARY);
                    raCodeBuilder.enterScope();
                    member->acceptVisitor(*this);
                    const auto &varTable = symbolTable.currentScope().getTable();
                    exitScope(ScopeType::TEMPORARY);
                    raCodeBuilder.exitScope();
                    for (const auto& symbol : varTable | std::views::values)
                    {
                        const auto &varSymbol = std::static_pointer_cast<VariableSymbol>(symbol.second);
                        varSymbol->setClassName(classSymbol);
                        varSymbol->setScopeLevel(curScopeLevel());
                        classSymbol->addMember(varSymbol);
                    }
                } break;
            case NodeType::PREFIX:
                {
                    enterScope(ScopeType::TEMPORARY);
                    raCodeBuilder.enterScope();
                    member->acceptVisitor(*this);
                    exitScope(ScopeType::TEMPORARY);
                    raCodeBuilder.exitScope();
                } break;
            default: break;
            }
        }
        classSymbol->setCollectionFinished();
        // 编译类和类的成员
        const auto &classIdent = classSymbol->getRaVal();
        raCodeBuilder.enterScope();
        raCodeBuilder << ri::TP_DEF(classIdent);
        for (const auto &member: *classSymbol->getMembers())
        {
            std::optional<std::string> defaultValue = std::nullopt;
            if (member->getType() == SymbolType::VARIABLE)
            {
                defaultValue = std::static_pointer_cast<VariableSymbol>(member)->getDefaultValue();
            }
            else if (member->getType() == SymbolType::FUNCTION)
            {
                const auto &functionSymbol = std::static_pointer_cast<FunctionSymbol>(member);
                if (functionSymbol->getFunctionType() != FunctionType::METHOD)
                {
                    throw std::runtime_error("Function type error. The function type of a class member function should be METHOD.");
                }
                functionSymbol->setScopeLevel(curScopeLevel());
                symbolTable.insert(functionSymbol);
                raCodeBuilder.enterScope();
                functionSymbol->getDefinitionNode()->acceptVisitor(*this);
                std::string methodCode {};
                raCodeBuilder
                .buildCurScope(methodCode)
                .exitScope();
                raCodeBuilder
                << ri::ANNOTATION(member->getPos().toString()).toRACode()
                << methodCode;
                defaultValue = member->getRaVal();
            }
            annotatePos(member->getPos());
            raCodeBuilder
            << ri::TP_ADD_INST_FIELD(classIdent, formatAttrField(member->getRaVal()),
                defaultValue.has_value() ? defaultValue.value() : "null");
        }
        for (const auto &member: *classSymbol->getStaticMembers())
        {
            std::optional<std::string> defaultValue = std::nullopt;
            if (member->getType() == SymbolType::VARIABLE)
            {
                defaultValue = std::static_pointer_cast<VariableSymbol>(member)->getDefaultValue();
            }
            annotatePos(member->getPos());
            raCodeBuilder
            << ri::TP_ADD_TP_FIELD(classIdent, member->getRaVal(),
                defaultValue.has_value() ? defaultValue.value() : "null");

        }
        for (const auto &ctor : *classSymbol->getConstructors())
        {
            const auto &ctorCode = compileConstructorNode(
                std::static_pointer_cast<FunctionSymbol>(ctor));
            annotatePos(ctor->getPos());
            raCodeBuilder << ctorCode;
        }
        std::string classDefCode{};
        raCodeBuilder
        .buildCurScope(classDefCode)
        .exitScope();
        raCodeBuilder << classDefCode;
        popProcessingSymbol();
        classSymbol->setDefaultVisitPermission();
    }

    void CompileVisitor::classifyFuncArgs(
        const FunctionCallNode& node,
        std::queue<OpItem>& posArgs,
        std::unordered_map<std::string, OpItem>& namedArgs,
        std::vector<OpItem>& orderedArgs)
    {
        const auto &argNode = std::static_pointer_cast<ParenRangerNode>(node.getRightNode());
        const auto &argNodes = visitParallelNode(argNode->getRangerNode());
        posArgs = {};
        namedArgs = {};
        orderedArgs = {};
        for (const auto &arg: argNodes)
        {
            if (arg->getRealType() == NodeType::ASSIGNMENT)
            {
                const auto &[identNode, valueNode] =
                    std::static_pointer_cast<AssignmentNode>(arg)->getAssignPair();
                identNode->acceptVisitor(*this);
                const auto &identName = rPopOpItemVal();
                valueNode->acceptVisitor(*this);
                orderedArgs.push_back(topOpItem());
                namedArgs.insert({identName, rPopOpItem()});
            } else
            {
                arg->acceptVisitor(*this);
                orderedArgs.push_back(topOpItem());
                posArgs.push(rPopOpItem());
            }
        }
    }

    std::shared_ptr<FunctionSymbol> CompileVisitor::getCtorSymbol(
    const std::shared_ptr<ClassSymbol>& classSymbol,
    const std::queue<OpItem>& posArgs,
    const std::unordered_map<std::string, OpItem>& namedArgs) const
    {
        // 将队列转换为向量以便索引访问
        std::vector<OpItem> posArgsVec;
        std::queue<OpItem> tempQueue = posArgs; // 复制队列
        while (!tempQueue.empty()) {
            posArgsVec.push_back(tempQueue.front());
            tempQueue.pop();
        }
        for (const auto& ctor : *classSymbol->getConstructors())
        {
            size_t posArgIndex = 0;
            auto tempNamedArgs = namedArgs; // 仍需复制，但比复制整个参数环境开销小
            bool matched = true;
            auto funcSymbol = std::static_pointer_cast<FunctionSymbol>(ctor);
            for (const auto& param : funcSymbol->getParameters())
            {
                bool matchedParam = false;
                switch (param->getParamType())
                {
                case ParamType::PARAM_POSITIONAL:
                    {
                        if (posArgIndex < posArgsVec.size())
                        {
                            matchedParam = checkTypeMatch(param, posArgsVec[posArgIndex]);
                            posArgIndex++;
                        }
                    } break;
                case ParamType::PARAM_KEYWORD:
                    {
                        if (auto it = tempNamedArgs.find(param->getVal());
                            it != tempNamedArgs.end())
                        {
                            matchedParam = checkTypeMatch(param, it->second);
                            tempNamedArgs.erase(it);
                        }
                    } break;
                case ParamType::PARAM_VAR_LEN_POSITIONAL:
                    while (posArgIndex < posArgsVec.size())
                    {
                        matchedParam = checkTypeMatch(param, posArgsVec[posArgIndex]);
                        if (!matchedParam) break;
                        posArgIndex++;
                    }
                    break;
                case ParamType::PARAM_VAR_LEN_KEYWORD:
                    for (const auto& arg : tempNamedArgs | std::views::values)
                    {
                        matchedParam = checkTypeMatch(param, arg);
                        if (!matchedParam) break;
                    }
                    tempNamedArgs.clear();
                    break;
                case ParamType::NO_PARAM:
                default:
                    throw std::runtime_error("unexpected param type");
                }
                if (!matchedParam) {
                    matched = false;
                    break;
                }
            }
            if (matched && posArgIndex == posArgsVec.size() && tempNamedArgs.empty())
            {
                return funcSymbol;
            }
        }
        throw std::runtime_error("no matching ctor found for class: '" + classSymbol->getVal() + "'");
    }

    void CompileVisitor::visitFunctionCallNode(FunctionCallNode &node) {
        // 提前处理函数调用的参数
        std::queue<OpItem> posArgs;
        std::unordered_map<std::string, OpItem> namedArgs;
        std::vector<OpItem> orderedArgs;
        std::vector<std::string> fullProcessedArgs {};
        std::vector<std::string> originalArgs {};
        std::vector<std::string> halfProcessedArgs {};
        classifyFuncArgs(node, posArgs, namedArgs, orderedArgs);
        for (const auto &argItem: orderedArgs)
        {
            originalArgs.push_back(argItem.getVal());
            halfProcessedArgs.push_back(raVal(argItem));
        }
        // 查找调用的函数符号
        std::shared_ptr<FunctionSymbol> funcSymbol = nullptr;
        node.getLeftNode()->acceptVisitor(*this);
        const auto &funcNameOpItem = rPopOpItem();
        std::string customTypeVid = funcNameOpItem.getRaVal(symbolTable);
        if (const auto &belongingSymbol = funcNameOpItem.getBelonging())
        {
            if (const auto &referenceSymbol = getReferenceTargetSymbol(funcNameOpItem);
                referenceSymbol && referenceSymbol->is(SymbolType::FUNCTION))
            {
                funcSymbol = std::static_pointer_cast<FunctionSymbol>(referenceSymbol);
                if (!funcSymbol->getLabelMarkManager().isLifeCycleLabelMarked(LifeCycleLabel::ORDINARY))
                {
                    fullProcessedArgs.push_back(belongingSymbol->getRaVal());
                }
            } else if (referenceSymbol && referenceSymbol->is(SymbolType::CLASS))
            {
                customTypeVid = referenceSymbol->getRaVal();
            } else
            {
                fullProcessedArgs.push_back(belongingSymbol->getRaVal());
            }
        } else if (const auto &reference = getReferenceTargetSymbol(funcNameOpItem))
        {
            if (reference->is(SymbolType::CLASS))
            {
                customTypeVid = reference->getRaVal();
            }
        }
        if (TypeLabelSymbol::isCustomType(customTypeVid))
        {
            const auto &classSymbol = TypeLabelSymbol::getCustomClassSymbol(customTypeVid);
            funcSymbol = getCtorSymbol(classSymbol, posArgs, namedArgs);
        }
        else if (const auto& [fst, snd] =
            symbolTable.find(funcNameOpItem.getVal());
            snd)
        {
            if (snd->is(SymbolType::FUNCTION))
            {
                funcSymbol = std::static_pointer_cast<FunctionSymbol>(snd);
            } else if (snd->is(SymbolType::VARIABLE))
            {
                if (const auto &refSymbol =
                    getReferenceTargetSymbol(std::static_pointer_cast<VariableSymbol>(snd));
                    refSymbol && refSymbol->is(SymbolType::FUNCTION))
                {
                    funcSymbol = std::static_pointer_cast<FunctionSymbol>(refSymbol);
                }
            }
        }
        if (!funcSymbol)
        {
            if (const auto &funcNameSymbol = getSymbolFromOpItem(funcNameOpItem);
                !funcNameSymbol)
            {
                throw std::runtime_error("Function '" + funcNameOpItem.getVal() + "' not found.");
            }
            else if (const auto &funcType = getTypeLabelFromSymbol(funcNameSymbol);
                funcType->is("func"))
            {
                raCodeBuilder << ri::CALL(funcNameSymbol->getRaVal(), halfProcessedArgs);
            } else if (funcType->is("funi"))
            {
                pushTemOpVarItemWithRecord(node.getPos(), getBuiltinTypeSymbol(Pos::UNKNOW_POS, B_ANY));
                raCodeBuilder << ri::IVOK(funcNameSymbol->getRaVal(), halfProcessedArgs, topOpRaVal());
            } else
            {
                throw std::runtime_error("Invalid function type: " + funcType->toString());
            }
            return;
        }
        const auto &parameters = funcSymbol->getParameters();
        if (node.getRightNode()->getRealType() != NodeType::RANGER)
        {
            throw std::runtime_error("Function call node's right node must be ranger node.");
        }
        for (const auto &param : parameters)
        {
            if (param->getParamType() == ParamType::PARAM_POSITIONAL || param->getParamType() == ParamType::PARAM_KEYWORD) {
                if (const auto &it = namedArgs.find(param->getVal());
                   it != namedArgs.end())
                {
                    fullProcessedArgs.push_back(it->second.getRaVal(symbolTable));
                    namedArgs.erase(it);
                } else if (posArgs.size() > 0)
                {
                    if (!checkTypeMatch(param, posArgs.front()))
                    {
                        throw std::runtime_error("Type mismatch.");
                    }
                    fullProcessedArgs.push_back(posArgs.front().getRaVal(symbolTable));
                    posArgs.pop();
                } else if (param->getDefaultValue().has_value())
                {
                    fullProcessedArgs.push_back(param->getDefaultValue().value());
                } else
                {
                    throw std::runtime_error("Not enough arguments.");
                }
            } else if (param->getParamType() == ParamType::PARAM_VAR_LEN_POSITIONAL)
            {
                std::vector<std::string> items;
                items.reserve(posArgs.size());
                while (!posArgs.empty())
                {
                    const auto& argItem = posArgs.front();
                    // 先检查类型匹配
                    if (!checkTypeMatch(param, argItem))
                    {
                        throw std::runtime_error("Type mismatch.");
                    }
                    // 根据不同类型处理参数
                    std::string item;
                    switch (argItem.getType())
                    {
                    case OpItemType::IDENTIFIER:
                        {
                            const auto& argSymbol = getSymbolFromOpItem(argItem);
                            if (!argSymbol) {
                                throw std::runtime_error("Invalid argument '" + argItem.getVal() + "'.");
                            }
                            item = argSymbol->getRaVal();
                        } break;
                    case OpItemType::LITERAL_VALUE:
                        {
                            item = raVal(argItem);
                        } break;
                    default: throw std::runtime_error("Invalid argument '" + argItem.getVal() + "'.");
                    }
                    items.push_back(std::move(item));
                    posArgs.pop();
                }
                pushTemOpVarItemWithRecord(node.getPos());
                raCodeBuilder
                << ri::TP_SET(TypeLabelSymbol::listTypeSymbol(node.getPos(), symbolTable.curScopeLevel())->getRaVal(),
                    topOpRaVal());
                if (!items.empty())
                {
                    raCodeBuilder << ri::ITER_APND(items, topOpRaVal());
                }
                fullProcessedArgs.push_back(rPopOpItemRaVal());
            } else if (param->getParamType() == ParamType::PARAM_VAR_LEN_KEYWORD)
            {
                std::vector<std::string> items {};
                for (const auto &[name, opItem]: namedArgs)
                {
                    if (!checkTypeMatch(param, opItem))
                    {
                        throw std::runtime_error("Type mismatch.");
                    }
                    pushTemOpVarItemWithRecord(node.getPos());
                    raCodeBuilder
                    << ri::PAIR_SET(StringManager::toStringFormat(name), opItem.getRaVal(symbolTable), topOpRaVal());
                    items.push_back(rPopOpItemRaVal());
                }
                pushTemOpVarItemWithRecord(node.getPos());
                raCodeBuilder
                << ri::TP_SET(TypeLabelSymbol::dictTypeSymbol(node.getPos(), symbolTable.curScopeLevel())->getRaVal(),
                    topOpRaVal());
                if (!items.empty())
                {
                    raCodeBuilder << ri::ITER_APND(items, topOpRaVal());
                }
                fullProcessedArgs.push_back(rPopOpItemRaVal());
            }
        }
        // 检查是否是 builtin function
        if ((funcSymbol->isBuiltinType(TypeOfBuiltin::BUILTIN) ||
            funcSymbol->isBuiltinType(TypeOfBuiltin::PURE_BUILTIN)) &&
            builtin::isBuiltin(funcSymbol->getVal()))
        {
            const auto &raCode = builtin::callBuiltinFunction(
                *this, funcSymbol->getVal(), fullProcessedArgs, originalArgs);
            raCodeBuilder << raCode;
        } else
        {
            if (funcSymbol->hasReturnValue())
            {
                pushTemOpVarItemWithRecord(node.getPos(), funcSymbol->getReturnType());
                raCodeBuilder << ri::IVOK(funcSymbol->getRaVal(), fullProcessedArgs, topOpRaVal());
            } else
            {
                raCodeBuilder
                << ri::CALL(funcSymbol->getRaVal(), fullProcessedArgs);
            }
        }
    }

    void CompileVisitor::visitProgramNode(ProgramNode &node) {
        setCurrentProcessingFilePath(node.getPos().getFilepath());
        enterScope(ScopeType::PROGRAM);
        for (auto &statement: node.getStatements()) {
            statement->acceptVisitor(*this);
        }
        exitScope(ScopeType::PROGRAM);
        setCurrentProcessingFilePath();
    }

    void CompileVisitor::visitInfixNode(InfixExpressionNode &node)
    {
        node.getLeftNode()->acceptVisitor(*this);
        const auto &left = rPopOpItem();
        checkExists(left);
        const auto &[leftTypeLabel, leftValueType] = getTypesFromOpItem(left);
        node.getRightNode()->acceptVisitor(*this);
        const auto &right = rPopOpItem();
        const auto &[rightTypeLabel, rightValueType] = getTypesFromOpItem(right);
        std::shared_ptr<TypeLabelSymbol> resultValueType = nullptr;
        switch (node.getInfixType())
        {
        case NodeType::PLUS:
            {
                if (leftValueType->is("any") || rightValueType->is("any"))
                {
                    resultValueType = TypeLabelSymbol::anyTypeSymbol(node.getPos(), symbolTable.curScopeLevel());
                } else if (leftValueType->is("str") || rightValueType->is("str"))
                {
                    resultValueType = TypeLabelSymbol::strTypeSymbol(node.getPos(), symbolTable.curScopeLevel());
                } else if (leftValueType->is("int") && rightValueType->is("int"))
                {
                    resultValueType = TypeLabelSymbol::intTypeSymbol(node.getPos(), symbolTable.curScopeLevel());
                } else if (leftValueType->is("float") || rightValueType->is("float"))
                {
                    resultValueType = TypeLabelSymbol::floatTypeSymbol(node.getPos(), symbolTable.curScopeLevel());
                } else
                {
                    throw std::runtime_error("Type mismatch.");
                }
                pushTemOpVarItemWithRecord(node.getPos(), resultValueType);
                raCodeBuilder << ri::ADD(raVal(left), raVal(right), topOpRaVal());
            } break;
        case NodeType::DIVIDE:
            {
                pushTemOpVarItemWithRecord(node.getPos());
                raCodeBuilder << ri::DIV(raVal(left), raVal(right), topOpRaVal());
            } break;
        case NodeType::MINUS:
            {
                pushTemOpVarItemWithRecord(node.getPos());
                raCodeBuilder << ri::OPP(raVal(right), topOpRaVal())
                << ri::ADD(raVal(left), topOpRaVal(), topOpRaVal());
            } break;
        case NodeType::MULTIPLY:
            {
                pushTemOpVarItemWithRecord(node.getPos());
                raCodeBuilder << ri::MUL(raVal(left), raVal(right), topOpRaVal());
            } break;
        case NodeType::MODULO:
            {
                pushTemOpVarItemWithRecord(node.getPos());
                raCodeBuilder << ri::MOD(raVal(left), raVal(right), topOpRaVal());
            } break;
        case NodeType::PAIR:
            {
                pushTemOpVarItemWithRecord(node.getPos());
                raCodeBuilder << ri::PAIR_SET(raVal(left), raVal(right), topOpRaVal());
            } break;
        case NodeType::COMPARE:
        case NodeType::LOGICAL:
            {
                pushTemOpVarItemWithRecord(node.getPos());
                if (const auto &it = RELATION_MAP.find(node.getOpToken().getValue());
                    it != RELATION_MAP.end())
                {
                    raCodeBuilder << ri::CMP(raVal(left), raVal(right), topOpRaVal())
                    << ri::CREL(topOpRaVal(), it->second, topOpRaVal());
                } else
                {
                    throw std::runtime_error("Incorrect relational operator");
                }
            } break;
        case NodeType::ARGUMENT_ASSIGNMENT:
            {
                raCodeBuilder
                << ri::ADD(raVal(left), raVal(right), raVal(left));
            } break;
        case NodeType::CALL:
            {

                pass();
            } break;
        case NodeType::PARALLEL:
            {

                pass();
            } break;
        case NodeType::ATTRIBUTE_EXPRESSION:
            {
                const auto &leftReferencedSymbol = getReferenceTargetSymbol(left);
                const std::shared_ptr<ClassSymbol> &classSymbol = leftReferencedSymbol && leftReferencedSymbol->is(SymbolType::CLASS) ?
                std::static_pointer_cast<ClassSymbol>(leftReferencedSymbol) :
                TypeLabelSymbol::getCustomClassSymbol(leftTypeLabel->getRaVal());
                if (const auto &memberSymbol = classSymbol->findMemberSymbolInPermission(right.getVal()))
                {
                    const auto &typeLabelSymbol = getTypeLabelFromSymbol(memberSymbol);
                    const VarID attrVarID (getNewTempVarName(), node.getPos().getFileField(), curScopeField());
                    pushIdentItem(attrVarID, typeLabelSymbol, typeLabelSymbol, memberSymbol);
                    topOpItemPtr()->setBelonging(getSymbolFromOpItem(left), memberSymbol->getRaVal());
                    if (memberSymbol->is(SymbolType::VARIABLE))
                    {
                        symbolTable.insert(
                        memberSymbol->transform(attrVarID.getNameField(), attrVarID.getVid(), curScopeLevel()));
                    }
                    raCodeBuilder
                    << ri::ALLOT({attrVarID.getVid()})
                    << ri::TP_GET_FIELD(left.getRaVal(symbolTable), "\"" + StringManager::escape(memberSymbol->getRaVal()) + "\"", topOpRaVal());
                } else
                {
                    throw std::runtime_error("Available member was not found in '" + classSymbol->getVal() + "': '" + right.getVal() + "'");
                }
            } break;
        default:
            pass("ToDo: process this infix node type '" + getNodeTypeName(node.getInfixType()) + "'");
        }
    }

    void CompileVisitor::visitUnaryExpressionNode(UnaryExpressionNode &node) {
        pass("To visit " + getNodeTypeName(node.getRealType()) + " type node.");

    }

    void CompileVisitor::visitExpressionStatementNode(ExpressionStatementNode &node) {
        annotatePos(node.getPos());
        node.getExpression()->acceptVisitor(*this);
        while (hasNextOpItem())
        {
            popOpItem();
        }
    }

    void CompileVisitor::visitPrefixExpressionNode(PrefixExpressionNode &node) {

        pass("To visit " + getNodeTypeName(node.getRealType()) + " type node.");
    }

    void CompileVisitor::visitPostfixNode(PostfixExpressionNode &node) {
        node.getNode()->acceptVisitor(*this);
        const auto &leftVal = rPopOpItemRaVal();
        switch (node.getPostfixType())
        {
        case NodeType::INCREMENT:
            {
                raCodeBuilder << ri::ADD(leftVal, "1", leftVal);
            } break;
        case NodeType::DECREMENT:
            {
                raCodeBuilder << ri::ADD(leftVal, "-1", leftVal);
            } break;
        default:
            {
                pass("To compile other postfix type node: " + getNodeTypeName(node.getPostfixType()));
            }
        }
    }

    void CompileVisitor::visitParenRangerNode(ParenRangerNode &node) {
        node.getRangerNode()->acceptVisitor(*this);
    }

    void CompileVisitor::visitBlockRangerNode(BlockRangerNode &node) {
        for (const auto &statement: node.getBodyExpressions()) {
            annotatePos(statement->getPos());
            statement->acceptVisitor(*this);
        }
    }

    void CompileVisitor::visitFunctionDefinitionNode(FunctionDefinitionNode &node)
    {
        enterScope(ScopeType::FUNCTION);
        if (node.getCallNode()->getRealType() != NodeType::CALL)
        {
            throw std::runtime_error("FunctionCallNode::acceptVisitor: node.getCallNode() is not call type.");
        }
        const auto &callNode = std::static_pointer_cast<FunctionCallNode>(
            node.getCallNode());
        callNode->getLeftNode()->acceptVisitor(*this);
        const auto &funcNameOpItem = rPopOpItem();
        std::shared_ptr<ExpressionNode> paramRangerNode = callNode->getRightNode();
        if (callNode->getRightNode()->getRealType() == NodeType::ANON_FUNCTION_DEFINITION)
        {
            const auto &paramNode =
                std::static_pointer_cast<AnonFunctionDefinitionNode>(callNode->getRightNode());
            paramRangerNode = paramNode->getParamNode();
        }
        const auto &paramNode = std::static_pointer_cast<ParenRangerNode>(paramRangerNode);
        const auto &paramItems = visitParallelNode(paramNode->getRangerNode());
        std::vector<std::shared_ptr<ParameterSymbol>> paramSymbols {};
        std::vector<std::string> paramIdents {};
        for (const auto &item: paramItems)
        {
            auto paramType = ParamType::PARAM_POSITIONAL;
            if (item->getRealType() != NodeType::IDENTIFIER &&
                item->getRealType() != NodeType::ASSIGNMENT &&
                item->getRealType() != NodeType::UNARY)
            {
                throw std::runtime_error("Function call parameter must be an identifier or an assignment or a unary.");
            }
            auto param = item;
            std::optional<OpItem> defaultValue = std::nullopt;
            if (item->getRealType() == NodeType::ASSIGNMENT)
            {
                const auto &[ident, value] =
                    std::static_pointer_cast<AssignmentNode>(item)->getAssignPair();
                param = ident;
                value->acceptVisitor(*this);
                defaultValue = rPopOpItem();
                paramType = ParamType::PARAM_KEYWORD;
            } else if (item->getRealType() == NodeType::UNARY)
            {
                if (const auto &unaryParam =
                    std::static_pointer_cast<UnaryExpressionNode>(item);
                    unaryParam->getOpToken().getType() == TokenType::TOKEN_STAR)
                {
                    param = unaryParam->getRightNode();
                    paramType = ParamType::PARAM_VAR_LEN_POSITIONAL;
                } else if (unaryParam->getOpToken().getType() == TokenType::TOKEN_DOUBLE_STAR)
                {
                    param = unaryParam->getRightNode();
                    paramType = ParamType::PARAM_VAR_LEN_KEYWORD;
                } else
                {
                    throw std::runtime_error("Function call parameter must be an identifier or an assignment or a unary.");
                }
            }
            const auto &tempParamNode = std::static_pointer_cast<IdentifierNode>(param);
            tempParamNode->acceptVisitor(*this);
            const auto &paramOpItem = rPopOpItem();
            paramIdents.push_back(paramOpItem.getRaVal(symbolTable));
            std::unordered_set<std::shared_ptr<LabelSymbol>> labels = processLabelNodes(tempParamNode->getLabels());
            const auto &paramSymbol = std::make_shared<ParameterSymbol>(
                paramType,
                tempParamNode->getPos(),
                paramOpItem.getVal(),
                paramOpItem.getRaVal(symbolTable),
                labels,
                std::nullopt,
                symbolTable.curScopeLevel());
            if (defaultValue.has_value())
            {
                if (!checkTypeMatch(paramSymbol, defaultValue.value()))
                {
                    throw std::runtime_error("Function call parameter type mismatch.");
                }
                paramSymbol->setDefaultValue(defaultValue.value().getRaVal(symbolTable));
            }
            paramSymbols.push_back(paramSymbol);
            symbolTable.insert(std::make_shared<VariableSymbol>(
                item->getPos(),
                paramOpItem.getVal(),
                paramOpItem.getRaVal(symbolTable),
                labels,
                symbolTable.curScopeLevel(),
                true));
        }
        const auto &labels = processLabelNodes(node.getLabelNodes());
        std::shared_ptr<FunctionSymbol> functionSymbol = nullptr;
        std::shared_ptr<ClassSymbol> classSymbol = nullptr;
        auto createFunctionSymbol = [&](const FunctionType &funcType)
        {
            return std::make_shared<FunctionSymbol>(
            std::make_shared<FunctionDefinitionNode>(node),
            node.getPos(),
            funcNameOpItem.getVal(),
            funcNameOpItem.getRaVal(symbolTable),
            labels,
            paramSymbols,
            curScopeLevel() - 1,
            TypeOfBuiltin::ORDINARY, funcType);
        };
        if (isProcessingSymbol())
        {
            if (const auto &topSymbol = topProcessingSymbol();
                topSymbol->getType() == SymbolType::CLASS)
            {
                if (classSymbol = std::static_pointer_cast<ClassSymbol>(topSymbol);
                    !classSymbol->hasCollectionFinished())
                {
                    functionSymbol = createFunctionSymbol(FunctionType::METHOD);
                    functionSymbol->setClassName(classSymbol);
                    classSymbol->addMember(functionSymbol);
                    exitScope(ScopeType::FUNCTION);
                    return;
                }
                if (classSymbol && classSymbol->hasCollectionFinished())
                {
                    const auto & [fst, snd] = symbolTable.find(funcNameOpItem.getVal());
                    if (fst < 0 || snd->getType() != SymbolType::FUNCTION)
                    {
                        throw std::runtime_error("Function '" + funcNameOpItem.getVal() + "' not found in '" + classSymbol->getVal() + "'");
                    }
                    functionSymbol = std::static_pointer_cast<FunctionSymbol>(snd);
                }
            }
        }
        if (classSymbol)
        {
            if (functionSymbol->getLabelMarkManager().isLifeCycleLabelMarked(LifeCycleLabel::STATIC))
            {
                pass();
            } else
            {
                const auto thisFieldSymbol = getThisFieldSymbol(classSymbol);
                paramIdents.insert(paramIdents.begin(), thisFieldSymbol->getRaVal());
                symbolTable.insert(thisFieldSymbol);
            }
        }
        else if (!functionSymbol) {
            functionSymbol = createFunctionSymbol(FunctionType::FUNCTION);
            symbolTable.insert(functionSymbol);
        }
        if (!functionSymbol->getReturnType())
        {
            functionSymbol->setReturnType(getBuiltinTypeSymbol(functionSymbol->getPos(), B_ANY));
        }
        pushNewProcessingSymbol(functionSymbol);
        raCodeBuilder << (functionSymbol->hasReturnValue() ?
        ri::FUNI(funcNameOpItem.getRaVal(symbolTable), paramIdents).toRACode() :
        ri::FUNC(funcNameOpItem.getRaVal(symbolTable), paramIdents).toRACode());
        node.getBodyNode()->acceptVisitor(*this);
        if (!functionSymbol->isBuiltinType(TypeOfBuiltin::BUILTIN) &&
            functionSymbol->hasReturnValue() && !functionSymbol->hasReturned())
        {
            if (functionSymbol->getReturnType()->isNot("any"))
            {
                throw std::runtime_error("Function '" + funcNameOpItem.getVal() + "' must return a value.");
            }
            raCodeBuilder << ri::RET("null");
        }
        raCodeBuilder << ri::END(funcNameOpItem.getRaVal(symbolTable));
        exitScope(ScopeType::FUNCTION);
        popProcessingSymbol();
    }

    void CompileVisitor::visitLabelNode(LabelNode &node) {
        if (const auto &label = node.getLabel();
            TypeLabelSymbol::isTypeLabel(label)) {
            symbolTable.insert(std::make_shared<TypeLabelSymbol>(node.getPos(),
                label, symbolTable.curScopeLevel()));
            // ToDo: type label
            }
        else {
            if (const auto &[fst, snd] = symbolTable.find(label);
                fst >= 0)
            {
                if (snd->getType() != SymbolType::CLASS)
                {
                    throw std::runtime_error("'" + label + "' is an invalid custom type label.");
                }
                symbolTable.insert(std::make_shared<TypeLabelSymbol>(node.getPos(),
                    label, symbolTable.curScopeLevel(), snd->getRaVal()));
            }
            else {
                symbolTable.insert(std::make_shared<LabelSymbol>(node.getPos(),
                    label, RCC_UNDEFINED_CONST, symbolTable.curScopeLevel()));
            }
        }
    }

    void CompileVisitor::visitBranchNode(BranchNode &node) {
        raCodeBuilder
        << ri::ANNOTATION(node.getPos().toString());
        const auto &endSetLabel = rPopOpItemRaVal();
        node.getConditionNode()->acceptVisitor(*this);
        const auto &conditionVal = rPopOpItemRaVal();
        const auto &skipSetLabel = newTemOpSetItem(node.getPos()).getRaVal(symbolTable);
        raCodeBuilder
        << ri::JF(conditionVal, skipSetLabel);
        node.getBodyNode()->acceptVisitor(*this);
        raCodeBuilder
        << ri::JMP(endSetLabel)
        << ri::SET(skipSetLabel);
    }

    void CompileVisitor::visitConditionNode(ConditionNode &node) {
        enterScope(ScopeType::CONDITION);
        // 条件语句的结束 SET 标签
        const auto &endSetLabel = std::make_shared<OpItem>(
            OpItemType::SET_LABEL,
            getBuiltinTypeSymbol(node.getPos(), B_FLAG),
            SetID(getNewSetLabelName(), node.getPos().getFileField(), curScopeField()).getSid());
        for (const auto &branchNode: node.getBranchNodes()) {
            pushOpItem(endSetLabel);
            branchNode->acceptVisitor(*this);
        }
        raCodeBuilder << ri::SET(endSetLabel->getRaVal(symbolTable));
        exitScope(ScopeType::CONDITION);
    }

    void CompileVisitor::visitLoopNode(LoopNode &node) {
        switch (node.getLoopType())
        {
        case LoopType::WHILE:
            {
                pushTemOpVarItemWithRecord(node.getPos());
                raCodeBuilder
                << ri::CMP(RCC_TRUE, RCC_FALSE, topOpRaVal())
                << ri::UNTIL(rPopOpItemRaVal(), RCC_REL_RE);
                node.getConditionNode()->acceptVisitor(*this);
                const auto &continueSetLabel = newTemOpSetItem(node.getPos());
                raCodeBuilder
                << ri::JT(rPopOpItemRaVal(), continueSetLabel.getRaVal(symbolTable))
                << ri::EXIT("UNTIL")
                << ri::SET(continueSetLabel.getRaVal(symbolTable));
                enterScope(ScopeType::LOOP);
                node.getBodyNode()->acceptVisitor(*this);
                exitScope(ScopeType::LOOP);
                raCodeBuilder
                << ri::END("UNTIL");
            } break;
        case LoopType::FOR:
        case LoopType::UNTIL:
        default:
            pass("To process other loop type node: " + getNodeTypeName(node.getRealType()));
        }
    }

    void CompileVisitor::visitForLoopNode(ForLoopNode &node) {
        pass("To visit " + getNodeTypeName(node.getRealType()) + " type node.");
    }

    void CompileVisitor::visitPassExpressionNode(PassExpressionNode &node) {
        pass("To visit " + getNodeTypeName(node.getRealType()) + " type node.");
    }

    void CompileVisitor::visitEncapsulatedExpressionNode(EncapsulatedExpressionNode &node) {
        if (const auto &processionSymbol = topProcessingSymbol();
            processionSymbol->is(SymbolType::FUNCTION))
        {
            const auto &funcSymbol = std::static_pointer_cast<FunctionSymbol>(processionSymbol);
            if (!builtin::isBuiltinFunction(funcSymbol->getVal()))
            {
                throw std::runtime_error("Non-built-in function cannot be encapsulated: '" + funcSymbol->getVal() + "'");
            }
            funcSymbol->setBuiltInType(TypeOfBuiltin::BUILTIN);
        }
        raCodeBuilder
        << ri::PASS("This function is encapsulated.");
    }

    void CompileVisitor::visitReturnExpressionNode(ReturnExpressionNode &node)
    {
        if (topProcessingSymbolType() != SymbolType::FUNCTION)
        {
            throw std::runtime_error("Return expression node must be in function.");
        }
        const auto& funcSymbol = std::static_pointer_cast<FunctionSymbol>(topProcessingSymbol());
        funcSymbol->setHasReturned(true);
        if (funcSymbol->hasReturnValue() && !node.getReturnNode())
        {
            if (funcSymbol->getReturnType()->isNot("any"))
            {
                throw std::runtime_error("Functions with a return value type must have a return value.");
            }
            raCodeBuilder << ri::RET("null");
            return;
        }
        if (!funcSymbol->hasReturnValue() && node.getReturnNode())
        {
            throw std::runtime_error("Functions with a return type of void are not allowed to have a return value.");
        }
        if (node.getReturnNode()) {
            node.getReturnNode()->acceptVisitor(*this);
            const auto &returnVal = rPopOpItem();
            if (!checkTypeMatch(funcSymbol->getReturnType(), returnVal))
            {
                throw std::runtime_error("Return value type does not match function return value type.");
            }
            processTypeAutoChange(funcSymbol, returnVal);
            raCodeBuilder
            << ri::RET(returnVal.getRaVal(symbolTable));
        } else
        {
            raCodeBuilder << ri::RET();
        }
    }

    void CompileVisitor::visitBreakExpressionNode(BreakExpressionNode &node) {
        pass("To visit " + getNodeTypeName(node.getRealType()) + " type node.");
    }

    void CompileVisitor::visitAnonFunctionDefinitionNode(AnonFunctionDefinitionNode &node) {
        enterScope(ScopeType::ANONYMOUS);
        std::vector<std::shared_ptr<ParameterSymbol>> paramSymbols {};
        std::vector<std::string> paramIdents {};
        if (node.getParamNode())
        {
            const auto &paramNode = std::static_pointer_cast<ParenRangerNode>(node.getParamNode());
            for (const auto &paramItems = visitParallelNode(paramNode->getRangerNode());
                const auto &item: paramItems)
            {
                auto paramType = ParamType::PARAM_POSITIONAL;
                if (item->getRealType() != NodeType::IDENTIFIER &&
                    item->getRealType() != NodeType::ASSIGNMENT &&
                    item->getRealType() != NodeType::UNARY)
                {
                    throw std::runtime_error("Function call parameter must be an identifier or an assignment or a unary.");
                }
                auto param = item;
                std::optional<OpItem> defaultValue = std::nullopt;
                if (item->getRealType() == NodeType::ASSIGNMENT)
                {
                    const auto &[ident, value] =
                        std::static_pointer_cast<AssignmentNode>(item)->getAssignPair();
                    param = ident;
                    value->acceptVisitor(*this);
                    defaultValue = rPopOpItem();
                    paramType = ParamType::PARAM_KEYWORD;
                } else if (item->getRealType() == NodeType::UNARY)
                {
                    if (const auto &unaryParam =
                        std::static_pointer_cast<UnaryExpressionNode>(item);
                        unaryParam->getOpToken().getType() == TokenType::TOKEN_STAR)
                    {
                        param = unaryParam->getRightNode();
                        paramType = ParamType::PARAM_VAR_LEN_POSITIONAL;
                    } else if (unaryParam->getOpToken().getType() == TokenType::TOKEN_DOUBLE_STAR)
                    {
                        param = unaryParam->getRightNode();
                        paramType = ParamType::PARAM_VAR_LEN_KEYWORD;
                    } else
                    {
                        throw std::runtime_error("Function call parameter must be an identifier or an assignment or a unary.");
                    }
                }
                const auto &tempParamNode = std::static_pointer_cast<IdentifierNode>(param);
                tempParamNode->acceptVisitor(*this);
                const auto &paramOpItem = rPopOpItem();
                paramIdents.push_back(paramOpItem.getRaVal(symbolTable));
                std::unordered_set<std::shared_ptr<LabelSymbol>> labels = processLabelNodes(tempParamNode->getLabels());
                const auto &paramSymbol = std::make_shared<ParameterSymbol>(
                    paramType,
                    tempParamNode->getPos(),
                    paramOpItem.getVal(),
                    paramOpItem.getRaVal(symbolTable),
                    labels,
                    std::nullopt,
                    symbolTable.curScopeLevel());
                if (defaultValue.has_value())
                {
                    if (!checkTypeMatch(paramSymbol, defaultValue.value()))
                    {
                        throw std::runtime_error("Function call parameter type mismatch.");
                    }
                    paramSymbol->setDefaultValue(defaultValue.value().getRaVal(symbolTable));
                }
                paramSymbols.push_back(paramSymbol);
                symbolTable.insert(std::make_shared<VariableSymbol>(
                    item->getPos(),
                    paramOpItem.getVal(),
                    paramOpItem.getRaVal(symbolTable),
                    labels,
                    symbolTable.curScopeLevel(),
                    true));
            }
        }
        const auto &labels = processLabelNodes(node.getLabelNodes());
        std::shared_ptr<FunctionSymbol> functionSymbol = nullptr;
        std::shared_ptr<ClassSymbol> classSymbol = nullptr;
        auto createFunctionSymbol = [&](const FunctionType &funcType)
        {
            const VarID anonFuncVarID (ANONYMOUS_FUNCTION_PREFIX + getNewTempVarName(), node.getPos().getFileField(), curScopeField());
            return std::make_shared<FunctionSymbol>(
                std::make_shared<AnonFunctionDefinitionNode>(node),
                node.getPos(),
                anonFuncVarID.getNameField(),
                anonFuncVarID.getVid(),
                labels,
                paramSymbols,
                curScopeLevel() - 1,
                TypeOfBuiltin::ORDINARY, funcType
            );
        };
        functionSymbol = createFunctionSymbol(FunctionType::ANONYMOUS);
        symbolTable.insert(functionSymbol);
        if (!functionSymbol->getReturnType())
        {
            functionSymbol->setReturnType(getBuiltinTypeSymbol(functionSymbol->getPos(), B_ANY));
        }
        pushNewProcessingSymbol(functionSymbol);
        raCodeBuilder << (functionSymbol->hasReturnValue() ?
            ri::FUNI(functionSymbol->getRaVal(), paramIdents).toRACode() :
            ri::FUNC(functionSymbol->getRaVal(), paramIdents).toRACode());
        node.getBodyNode()->acceptVisitor(*this);
        if (!builtin::isBuiltinFunction(functionSymbol->getVal()) && functionSymbol->hasReturnValue() && !functionSymbol->hasReturned())
        {
            if (functionSymbol->getReturnType()->isNot("any"))
            {
                throw std::runtime_error("Anonymous function at " + node.getPos().toString() + " must return a value.");
            }
            raCodeBuilder << ri::RET("null");
        }
        raCodeBuilder << ri::END(functionSymbol->getRaVal());
        exitScope(ScopeType::ANONYMOUS);
        popProcessingSymbol();
        pushTemOpVarItemWithRecord(node.getPos(),
            getBuiltinTypeSymbol(Pos::UNKNOW_POS,
                functionSymbol->hasReturnValue() ? B_FUNI : B_FUNC), functionSymbol);
        raCodeBuilder << ri::PUT(functionSymbol->getRaVal(), topOpRaVal());
    }

    void CompileVisitor::visitPairExpressionNode(PairExpressionNode &node) {
        pass("To visit " + getNodeTypeName(node.getRealType()) + " type node.");
    }

    void CompileVisitor::visitDictionaryExpressionNode(DictionaryExpressionNode &node) {
        std::vector<std::string> dictItems{};
        for (const auto & itemNodes = visitParallelNode(node.getBodyNode());
            const auto &itemNode: itemNodes) {
            if (itemNode->getRealType() != NodeType::PAIR) {
                throw std::runtime_error("The wrong data type is in the dictionary.");
            }
            itemNode->acceptVisitor(*this);
            dictItems.push_back(rPopOpItemRaVal());
            }
        pushTemOpVarItemWithRecord(node.getPos());
        raCodeBuilder << ri::TP_SET(TypeLabelSymbol::dictTypeSymbol(
            node.getPos(), symbolTable.curScopeLevel())->getRaVal(), topOpRaVal())
        << ri::ITER_APND(dictItems, topOpRaVal());
    }

    void CompileVisitor::visitListExpressionNode(ListExpressionNode &node) {
        std::vector<std::string> listItems{};
        for (const auto & itemNodes = visitParallelNode(node.getBodyNode());
            const auto &itemNode: itemNodes) {
            itemNode->acceptVisitor(*this);
            listItems.push_back(rPopOpItemRaVal());
            }
        const auto &listType = TypeLabelSymbol::listTypeSymbol(
            node.getPos(), symbolTable.curScopeLevel());
        pushTemOpVarItemWithRecord(node.getPos(), listType);
        raCodeBuilder << ri::TP_SET(listType->getRaVal(),
            topOpRaVal())
        << ri::ITER_APND(listItems, topOpRaVal());
    }

    void CompileVisitor::visitBracketExpressionNode(BracketExpressionNode &node) {
        for (const auto & itemNodes= visitParallelNode(node.getBodyNode());
            const auto &itemNode: itemNodes) {
            itemNode->acceptVisitor(*this);
            }
    }

    void CompileVisitor::visitIndexExpressionNode(IndexExpressionNode &node)
    {
        node.getLeftNode()->acceptVisitor(*this);
        const auto &targetItem = rPopOpItem();
        node.getIndexNode()->acceptVisitor(*this);
        const auto &indexItem = rPopOpItem();
        if (const auto &[targetTypeLabel, targetValueType] = getTypesFromOpItem(targetItem);
            targetTypeLabel->is("any"))
        {
            if (targetValueType->is("any"))
            {
                pushTemOpVarItemWithRecord(node.getPos());
                raCodeBuilder
                << ri::ITER_GET(targetItem.getRaVal(symbolTable),
                    indexItem.getRaVal(symbolTable), topOpRaVal());
            } else if (targetValueType->isIterable())
            {
                pushTemOpVarItemWithRecord(node.getPos());
                raCodeBuilder
                << ri::ITER_GET(targetItem.getRaVal(symbolTable),
                    indexItem.getRaVal(symbolTable), topOpRaVal());
            } else
            {
                throw std::runtime_error("The wrong data type is in the index expression.");
            }
        } else if (targetTypeLabel->isIterable())
        {
            pushTemOpVarItemWithRecord(node.getPos());
            raCodeBuilder
            << ri::ITER_GET(targetItem.getRaVal(symbolTable),
                indexItem.getRaVal(symbolTable), topOpRaVal());
        } else
        {
            throw std::runtime_error("The wrong data type is in the index expression.");
        }
    }

    void CompileVisitor::visitVariableDefinitionNode(VariableDefinitionNode &node) {
        std::vector<OpItem> varIDs{};
        std::vector<std::string> vids{};
        std::vector<std::optional<OpItem>> values{};
        for (const auto &varDef: node.getVarDefs())
        {
            varDef->getNameNode()->acceptVisitor(*this);
            const auto& var = rPopOpItem();
            varIDs.push_back(var);
            vids.push_back(var.getRaVal(symbolTable));
            const auto labels = processLabelNodes(varDef->getLabelNodes());
            symbolTable.insert(std::make_shared<VariableSymbol>(
                varDef->getNameNode()->getPos(),
                var.getVal(),
                var.getRaVal(symbolTable),
                labels,
                symbolTable.curScopeLevel()));
            if (varDef->getValueNode())
            {
                // visit value node
                varDef->getValueNode()->acceptVisitor(*this);
                values.push_back(rPopOpItem());
                continue;
            }
            values.push_back(std::nullopt);
        }
        raCodeBuilder << ri::ALLOT(vids);
        for (size_t i = 0; i < vids.size(); i++) {
            const auto &vid = vids[i];
            const auto &[_, symbol] = symbolTable.findVariableSymbol(varIDs[i].getVal());
            if(!symbol->getTypeLabel()->isCustomType()) {
                raCodeBuilder << ri::TP_SET(symbol->getTypeLabel()->getRaVal(), vid);
            }
            if (values[i].has_value()) {
                const auto &value = values[i].value();
                if (!checkTypeMatch(symbol->getTypeLabel(), value.getTypeLabel()))
                {
                    throw std::runtime_error("The value type is not the same as the variable type.");
                }
                processTypeAutoChange(symbol, value);
                const auto &raValue = value.getRaVal(symbolTable);
                symbol->setDefaultValue(raValue);
                symbol->setReferencedSymbol(getReferenceTargetSymbol(value));
                raCodeBuilder << ri::PUT(raValue, vid);
            }
        }
    }

    void CompileVisitor::visitAssignmentNode(AssignmentNode &node)
    {
        const auto &[identNode, valueNode] = node.getAssignPair();
        identNode->acceptVisitor(*this);
        const auto &identItem = rPopOpItem();
        if (identNode->getRealType() == NodeType::IDENTIFIER)
        {
            const auto &labels = processLabelNodes(std::static_pointer_cast<IdentifierNode>(identNode)->getLabels());
            if (const auto& [level, identSymbol] = symbolTable.findAndTransform<VariableSymbol>(
                identItem.getVal(), SymbolType::VARIABLE); level >= 0 && identSymbol)
            {
                const auto &typeLabelRecord = identSymbol->getTypeLabel();
                identSymbol->reSetLabels(labels);
                if (!typeLabelRecord->equalWith(identSymbol->getTypeLabel()))
                {
                    raCodeBuilder
                    << ri::TP_SET(identSymbol->getTypeLabel()->getRaVal(), identSymbol->getRaVal());
                }
            }
        }
        node.getAssignPair().second->acceptVisitor(*this);
        if (const auto &[identSymbolLevel, identSymbol]
            = symbolTable.findVariableSymbol(identItem.getVal());
            identSymbolLevel >= 0 && identSymbol)
        {
            std::string value {};
            const auto &valueItem = rPopOpItem();
            if (!checkTypeMatch(identSymbol->getTypeLabel(), valueItem.getTypeLabel()))
            {
                throw std::runtime_error("The value type is not the same as the variable type.");
            }
            if (valueItem.getType() == OpItemType::IDENTIFIER)
            {
                if (const auto &[valueSymbolLevel, valueSymbol]
                    = symbolTable.findVariableSymbol(valueItem.getVal());
                    valueSymbolLevel >= 0 && valueSymbol)
                {
                    value = valueSymbol->getRaVal();
                    identSymbol->setReferencedSymbol(valueSymbol);
                } else
                {
                    value = valueItem.getRaVal(symbolTable);
                    identSymbol->setReferencedSymbol(getReferenceTargetSymbol(valueItem));
                }
            } else if (valueItem.getType() == OpItemType::LITERAL_VALUE)
            {
                value = valueItem.getRaVal(symbolTable);
            }
            if (identItem.getBelonging())
            {
                raCodeBuilder
                << ri::TP_SET_FIELD(identItem.getBelonging()->getRaVal(), formatAttrField(identItem.getBelongAttrRaValue()), value);
            } else
            {
                raCodeBuilder << ri::PUT(value, identSymbol->getRaVal());
            }
        } else {
            throw std::runtime_error("Identifier '" + identItem.getVal() + "' not found.");
        }
    }
}

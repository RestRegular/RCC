//
// Created by RestRegular on 2025/7/12.
//

#include <cstring>
#include <numeric>
#include <ranges>
#include <windows.h>

#include "../../include/rcc_base.h"
#include "../../include/builtin/rcc_builtin.h"
#include "../../include/components/ri/rcc_ri.h"
#include "../../include/lib/RLogSystem/rlog_system.h"
#include "../../include/analyzer/rcc_ast_components.h"
#include "../../include/analyzer/rcc_parser.h"
#include "../../include/components/symbol/rcc_symbol.h"
#include "../../include/lib/RJson/RJson_error.h"
#include "../../include/visitors/rcc_compile_visitor.h"

namespace ast
{
    using namespace symbol;

    ContentBuilder::ContentBuilder()
    {
        enterScope();
    }

    std::string ContentBuilder::buildAll()
    {
        if (raCode.empty())
        {
            while (!raCodeStack.empty())
            {
                raCode += raCodeStack.top().str();
                raCodeStack.pop();
            }
        }
        return raCode;
    }

    ContentBuilder& ContentBuilder::appendCode(const std::string& code)
    {
        if (code.empty()) return *this;
        if (raCodeStack.empty()) throw std::runtime_error("RaCodeBuilder::appendCode: raCodeStack is empty.");
        raCodeStack.top() << code;
        return *this;
    }

    ContentBuilder& ContentBuilder::operator<<(const std::string& code)
    {
        if (code.empty()) return *this;
        if (raCodeStack.empty()) throw std::runtime_error("RaCodeBuilder::appendCode: raCodeStack is empty.");
        raCodeStack.top() << code;
        return *this;
    }

    ContentBuilder& ContentBuilder::operator<<(const ri::RI& ri)
    {
        *this << ri.toRACode();
        return *this;
    }

    void ContentBuilder::enterScope()
    {
        raCodeStack.emplace();
    }

    void ContentBuilder::exitScope()
    {
        if (raCodeStack.empty()) throw std::runtime_error("RaCodeBuilder::exitScope: raCodeStack is empty.");
        raCodeStack.pop();
    }

    ContentBuilder& ContentBuilder::buildCurScope(std::string& target)
    {
        if (raCodeStack.empty()) throw std::runtime_error("RaCodeBuilder::buildCurScopeAndExit: raCodeStack is empty.");
        target = raCodeStack.top().str();
        return *this;
    }

    ContentBuilder& ContentBuilder::buildCurScope(ContentBuilder& builder)
    {
        if (raCodeStack.empty()) throw std::runtime_error("RaCodeBuilder::buildCurScopeAndExit: raCodeStack is empty.");
        builder << raCodeStack.top().str();
        return *this;
    }

    ContentBuilder& ContentBuilder::operator>>(std::string& target)
    {
        if (!raCodeStack.empty())
        {
            target = raCodeStack.top().str();
            raCodeStack.pop();
        }
        else
        {
            target = "";
        }
        return *this;
    }

    ContentBuilder& ContentBuilder::operator>>(ContentBuilder& builder)
    {
        if (!raCodeStack.empty())
        {
            builder << raCodeStack.top().str();
            raCodeStack.pop();
        }
        return *this;
    }

    std::string ContentBuilder::getCurScopeResult() const
    {
        return raCodeStack.top().str();
    }

    std::string opItemTypeToString(const OpItemType& type)
    {
        switch (type)
        {
        case OpItemType::IDENTIFIER: return "IDENTIFIER";
            break;
        case OpItemType::LITERAL_VALUE: return "LITERAL_VALUE";
            break;
        case OpItemType::SET_LABEL: return "SET_LABEL";
            break;
        default: return RCC_UNDEFINED_CONST;
        }
    }

    std::string opItemTypeToFormatString(const OpItemType& type)
    {
        return "[OpItemType: " + opItemTypeToString(type) + "]";
    }

    size_t VarID::_varId = 0;

    std::string VarID::rccVarPrefixField = "v";

    std::string VarID::_toVarID()
    {
        return (CompileVisitor::__compile_option_compile_level__ < 2 ?
        nameField + "_" : "") + rccVarPrefixField + uniqueId();
    }

    VarID::VarID(const std::string& name, const std::string& fileField, const std::string& scopeField,
                 const size_t& scopeLevel)
        : id(_varId++), fileField(fileField),
          scopeField(scopeField), nameField(name),
          vid(_toVarID()), scopeLevel(scopeLevel)
    {
    }

    std::string VarID::toString() const
    {
        return rccVarPrefixField + "_" + nameField + "_" + fileField + "_" + scopeField + "_" + std::to_string(id) + "_"
            + std::to_string(scopeLevel);
    }

    std::string VarID::getNameField() const
    {
        return nameField;
    }

    std::string VarID::getScopeField() const
    {
        return scopeField;
    }

    std::string VarID::getFileField() const
    {
        return fileField;
    }

    size_t VarID::getScopeLevel() const
    {
        return scopeLevel;
    }

    size_t VarID::getId() const
    {
        return id;
    }

    std::string VarID::getVid() const
    {
        return vid;
    }

    size_t SetID::_setId = 0;

    std::string SetID::rccSetPrefixField = "s";

    std::string SetID::_toSetID()
    {
        return (CompileVisitor::__compile_option_compile_level__ < 2 ?
        nameField + "_" : "") + rccSetPrefixField + uniqueId();
    }

    SetID::SetID(
        const std::string& name, const std::string& fileField,
        const std::string& scopeField)
        : id(_setId++), fileField(fileField),
          scopeField(scopeField), nameField(name), sid(_toSetID())
    {
    }

    std::string SetID::toString() const
    {
        return Object::toString();
    }

    std::string SetID::getNameField() const
    {
        return nameField;
    }

    std::string SetID::getScopeField() const
    {
        return scopeField;
    }

    std::string SetID::getFileField() const
    {
        return fileField;
    }

    size_t SetID::getId() const
    {
        return id;
    }

    std::string SetID::getSid() const
    {
        return sid;
    }

    OpItem::OpItem(
        const OpItemType& type,
        const std::shared_ptr<TypeLabelSymbol>& typeLabelSymbol,
        const std::string& value,
        const std::string& raValue,
        const std::shared_ptr<TypeLabelSymbol>& valueType,
        const std::shared_ptr<Symbol>& referencedSymbol,
        const Pos& pos)
        : value(value), raValue(type == OpItemType::LITERAL_VALUE || type == OpItemType::SET_LABEL ? value : raValue),
          type(type), typeLabel(typeLabelSymbol), valueType(valueType), referencedSymbol(referencedSymbol), pos(pos)
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

    bool OpItem::is(const OpItemType& opItemType) const
    {
        return opItemType == this->type;
    }

    bool OpItem::isNot(const OpItemType& opItemType) const
    {
        return opItemType != this->type;
    }

    std::string OpItem::toString() const
    {
        std::string result = "[<OpItem> ";

        if (is(OpItemType::LITERAL_VALUE))
        {
            // 字面量类型：[(OpItem) (类型) 值]
            result += "(" + getTypeLabel()->briefString() + ") " + (value.empty() ? "?" : value);
        }
        else
        {
            // 非字面量类型：[(OpItem) 名称: 类型 = (值类型) 值]
            result += value + ": " + getTypeLabel()->briefString()
                + " = (" + getValueType()->briefString() + ") "
                + (raValue.empty() ? "?" : raValue);
        }

        result += "]";
        return result;
    }

    Pos OpItem::getPos() const
    {
        return pos;
    }

    void OpItem::setPos(const Pos& pos_)
    {
        this->pos = pos_;
    }

    std::string OpItem::getVal() const
    {
        return value;
    }

    OpItemType OpItem::getType() const
    {
        return type;
    }

    std::string OpItem::getRaVal(const SymbolTableManager& table, const bool& needSearch) const
    {
        if (type == OpItemType::LITERAL_VALUE || type == OpItemType::SET_LABEL)
        {
            return value;
        }
        if (needSearch)
        {
            if (const auto& [level, symbol] = table.findByName(value);
                level >= 0 && symbol)
            {
                return symbol->getRaVal();
            }
        }
        return raValue;
    }

    std::shared_ptr<TypeLabelSymbol> OpItem::getTypeLabel() const
    {
        return typeLabel;
    }

    std::shared_ptr<TypeLabelSymbol> OpItem::getValueType() const
    {
        return valueType;
    }

    std::shared_ptr<Symbol> OpItem::getBelonging() const
    {
        return belonging;
    }

    std::string OpItem::getBelongAttrRaValue() const
    {
        return belongAttrRaValue;
    }

    std::shared_ptr<Symbol> OpItem::getReferencedSymbol() const
    {
        return referencedSymbol;
    }

    void OpItem::setTypeLabel(const std::shared_ptr<TypeLabelSymbol>& typeLabelSymbol)
    {
        this->typeLabel = typeLabelSymbol;
    }

    void OpItem::setValueType(const std::shared_ptr<TypeLabelSymbol>& valueTypeSymbol)
    {
        if (typeLabel->is("any") || valueTypeSymbol->is("any") || typeLabel->equalWith(valueTypeSymbol))
        {
            this->valueType = valueTypeSymbol;
        }
        else
        {
            throw RCCCompilerError::typeMissmatchError(valueTypeSymbol->getPos().toString(),
                                                       CompileVisitor::topLexer()->getCodeLine(
                                                           valueTypeSymbol->getPos()),
                                                       "Error symbol: " + valueTypeSymbol->toString(),
                                                       CompileVisitor::getListFormatString({
                                                           "any", typeLabel->getVal()
                                                       }),
                                                       valueTypeSymbol->getVal(),
                                                       {"You can try using the `any` type."});
        }
    }

    void OpItem::setBelonging(
        const std::shared_ptr<Symbol>& belongingSymbol, const std::string& belongAttrRaVal)
    {
        this->belonging = belongingSymbol;
        this->belongAttrRaValue = belongAttrRaVal;
    }

    void OpItem::setReferencedSymbol(const std::shared_ptr<Symbol>& symbol)
    {
        this->referencedSymbol = symbol;
    }

    bool OpItem::Is(const OpItemType& opItemType)
    {
        return is(opItemType);
    }

    bool OpItem::IsNot(const OpItemType& opItemType)
    {
        return isNot(opItemType);
    }

    const char* OpItem::ToString() const
    {
        return toString().c_str();
    }

    const char* OpItem::GetVal() const
    {
        return getVal().c_str();
    }

    const char* OpItem::GetRaVal(const IRCCSymbolTableManagerInterface* table, const bool& needSearch) const
    {
        return CompileVisitor::CreateCharP(getRaVal(*static_cast<const SymbolTableManager*>(table->Transform()), needSearch));
    }

    OpItemType OpItem::GetType() const
    {
        return getType();
    }

    IRCCTypeLabelSymbolInterface* OpItem::GetTypeLabelSymbolI() const
    {
        return getTypeLabel().get();
    }

    IRCCTypeLabelSymbolInterface* OpItem::GetValueTypeSymbolI() const
    {
        return getValueType().get();
    }

    IRCCSymbolInterface* OpItem::GetBelongingSymbolI() const
    {
        return getBelonging().get();
    }

    const char* OpItem::GetBelongAttrRaValue() const
    {
        return getBelongAttrRaValue().c_str();
    }

    IRCCSymbolInterface* OpItem::GetReferencedSymbol() const
    {
        return getReferencedSymbol().get();
    }

    void OpItem::SetTypeLabel(const IRCCTypeLabelSymbolInterface* typeLabelSymbolI)
    {
        setTypeLabel(std::static_pointer_cast<TypeLabelSymbol>(typeLabelSymbolI->TransformToTLSI()->copySelf()));
    }

    void OpItem::SetValueType(const IRCCTypeLabelSymbolInterface* valueTypeSymbolI)
    {
        setValueType(std::static_pointer_cast<TypeLabelSymbol>(valueTypeSymbolI->TransformToTLSI()->copySelf()));
    }

    void OpItem::SetBelonging(const IRCCSymbolInterface* belongingSymbolI, const char* belongAttrRaVal)
    {
        setBelonging(std::static_pointer_cast<Symbol>(belongingSymbolI->TransformToSI()->copySelf()), belongAttrRaVal);
    }

    void OpItem::SetReferencedSymbol(const IRCCSymbolInterface* symbolI)
    {
        setReferencedSymbol(std::static_pointer_cast<Symbol>(symbolI->TransformToSI()->copySelf()));
    }

    const OpItem* OpItem::TransformToOII() const
    {
        return this;
    }

    size_t CompileVisitor::_temVarId = 0;

    size_t CompileVisitor::_setId = 0;

    std::unordered_map<std::string,
                       std::shared_ptr<ClassSymbol>> CompileVisitor::extensionMap = {};

    std::stack<std::shared_ptr<lexer::Lexer>> CompileVisitor::_lexers{};

    std::list<std::string> CompileVisitor::_lexer_paths = {};

    std::string CompileVisitor::fileRecord{};

    std::unordered_map<std::string, std::string> CompileVisitor::extensionPathNameMap = {};

    std::stack<std::string> CompileVisitor::processingExtensionStack = {};

    OutputFormat CompileVisitor::__symbol_option_format__ = OutputFormat::TXT;

    bool CompileVisitor::__symbol_flag__ = false;

    bool CompileVisitor::__symbol_flag_export__ = false;

    bool CompileVisitor::__compile_flag__ = false;

    int CompileVisitor::__compile_option_compile_level__ = 0;

    // 辅助函数：获取符号的类型标签
    std::shared_ptr<TypeLabelSymbol> CompileVisitor::getTypeLabelFromSymbol(const std::shared_ptr<Symbol>& symbol)
    {
        if (!symbol) return nullptr;
        switch (symbol->getType())
        {
        case SymbolType::VARIABLE:
        case SymbolType::PARAMETER:
            return std::static_pointer_cast<VariableSymbol>(symbol)->getTypeLabel();
        case SymbolType::FUNCTION:
            return std::static_pointer_cast<FunctionSymbol>(symbol)->getSignature();
        case SymbolType::CLASS:
            return TypeLabelSymbol::clasTypeSymbol(symbol->getPos(), symbol->getScopeLevel());
        case SymbolType::LABEL:
            return std::static_pointer_cast<TypeLabelSymbol>(symbol);
        default:
            return nullptr;
        }
    }

    void CompileVisitor::recordProcessingExtension(const std::string& extensionPath, const std::string& extensionName)
    {
        extensionPathNameMap.insert({extensionPath, extensionName});
        processingExtensionStack.push(extensionPath);
    }

    void CompileVisitor::popProcessingExtension()
    {
        if (processingExtensionStack.empty())
        {
            throw std::runtime_error("Invalid operation.");
        }
        processingExtensionStack.pop();
    }

    bool CompileVisitor::checkIsProcessedExtension(const std::string& extensionPath)
    {
        return extensionPathNameMap.contains(extensionPath);
    }

    std::string CompileVisitor::getExtensionName(const std::string& extensionPath)
    {
        if (!checkIsProcessedExtension(extensionPath))
        {
            throw std::runtime_error("Invalid operation.");
        }
        return extensionPathNameMap.at(extensionPath);
    }

    std::string CompileVisitor::topProcessingExtensionPath()
    {
        if (processingExtensionStack.empty())
        {
            throw std::runtime_error("Invalid operation.");
        }
        return processingExtensionStack.top();
    }

    void CompileVisitor::processSymbolFlagOperation()
    {
        if (__symbol_flag__ && !__symbol_flag_export__)
        {
            std::string result;
            if (__symbol_option_format__ == OutputFormat::JSON)
            {
                if (analyzeBuilder.getCurScopeResult().empty())
                {
                    analyzeBuilder.enterScope();
                    analyzeBuilder << "{}";
                }
                result = analyzeBuilder.getCurScopeResult();
                analyzeBuilder.exitScope();
            }
            for (const auto& st: symbolTable.currentNameMapScope())
            {
                if (const auto& sysDefs = symbolTable.currentNameMapScope().getSysDefinedRecord();
                    sysDefs.contains(st->getRaVal()))
                {
                    continue;
                }
                if (__symbol_option_format__ == OutputFormat::TXT)
                {
                    analyzeBuilder << st->toDetailString() << "\n";
                } else if (__symbol_option_format__ == OutputFormat::JSON)
                {

                    RJsonParser rjParser (result);
                    rjParser.parse();
                    RJsonBuilder builder (rjParser.getParsedValue());
                    builder.insertRJValue(st->getVal(), rjson::RJValue::getObjectRJValue(st->toJsonString()));
                    result = builder.formatString(4);
                }
            }
            if (__symbol_option_format__ == OutputFormat::JSON)
            {
                analyzeBuilder.enterScope();
                analyzeBuilder << result;
            }
        }
    }

    // 辅助函数：获取符号的类型标签
    std::shared_ptr<TypeLabelSymbol> getValueTypeFromSymbol(const std::shared_ptr<Symbol>& symbol)
    {
        if (!symbol) return nullptr;

        switch (symbol->getType())
        {
        case SymbolType::VARIABLE:
        case SymbolType::PARAMETER:
            return std::static_pointer_cast<VariableSymbol>(symbol)->getValueType();
        case SymbolType::FUNCTION:
            {
                const auto& funcSymbol = std::static_pointer_cast<FunctionSymbol>(symbol);
                return funcSymbol->getSignature();
            }
        case SymbolType::CLASS:
            return TypeLabelSymbol::clasTypeSymbol(getUnknownPos(), symbol->getScopeLevel());
        case SymbolType::LABEL:
            return std::static_pointer_cast<TypeLabelSymbol>(symbol);
        default:
            return nullptr;
        }
    }

    // 检查两个TypeLabelSymbol是否匹配
    bool CompileVisitor::checkTypeMatch(const std::shared_ptr<TypeLabelSymbol>& leftTypeSymbol,
                                        const std::shared_ptr<TypeLabelSymbol>& rightTypeSymbol,
                                        const bool& restrict)
    {
        // 空指针检查
        if (!leftTypeSymbol || !rightTypeSymbol)
        {
            // 可以根据需要添加日志或调试信息
            return false;
        }

        // 类型完全匹配
        if (leftTypeSymbol->equalWith(rightTypeSymbol))
        {
            return true;
        }

        if (!restrict && leftTypeSymbol->relatedTo(rightTypeSymbol))
        {
            return true;
        }

        // 处理any类型，any可以匹配任何类型
        if (leftTypeSymbol->is("any") || rightTypeSymbol->is("any"))
        {
            return true;
        }

        // 处理null类型，通常null只能匹配引用类型或any
        if (leftTypeSymbol->is("nul") || rightTypeSymbol->is("nul"))
        {
            return true;
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
        if (!leftSymbol || !rightSymbol)
        {
            return false;
        }

        // 如果是相同的符号，直接返回true
        if (leftSymbol->equalWith(rightSymbol))
        {
            return true;
        }

        // 获取双方的类型标签
        const auto leftType = getTypeLabelFromSymbol(leftSymbol);

        if (const auto rightType = getTypeLabelFromSymbol(rightSymbol);
            rightType && rightType->isNot("any"))
        {
            return checkTypeMatch(leftType, rightType, true);
        }

        const auto rightValueType = getValueTypeFromSymbol(rightSymbol);

        // 检查类型标签是否匹配
        return checkTypeMatch(leftType, rightValueType, true);
    }

    std::stack<Pos> CompileVisitor::_currentProcessingPos {};

    Pos CompileVisitor::currentPos()
    {
        return _currentProcessingPos.empty() ? getUnknownPos() : _currentProcessingPos.top();
    }

    void CompileVisitor::pushProcessingPos(const Pos& pos)
    {
        _currentProcessingPos.push(pos);
    }

    void CompileVisitor::popProcessingPos()
    {
        _currentProcessingPos.pop();
    }

    // 从OpItem获取符号
    std::shared_ptr<Symbol> CompileVisitor::getSymbolFromOpItem(const OpItem& opItem) const
    {
        if (opItem.getReferencedSymbol())
        {
            return opItem.getReferencedSymbol();
        }
        if (opItem.getType() == OpItemType::IDENTIFIER)
        {
            if (const auto& [level, symbol] = symbolTable.findByName(opItem.getVal());
                level >= 0 && symbol)
            {
                return symbol;
            }
        }
        return nullptr;
    }

    std::string CompileVisitor::raVal(const OpItem& opItem, const bool& needSearch) const
    {
        return opItem.getRaVal(symbolTable, needSearch);
    }

    // 检查Symbol与OpItem是否类型匹配
    bool CompileVisitor::checkTypeMatch(const std::shared_ptr<Symbol>& leftSymbol,
                                        const OpItem& rightOpItem) const
    {
        if (!leftSymbol)
        {
            return false;
        }

        // 从OpItem获取符号
        if (const auto& rightSymbol = getSymbolFromOpItem(rightOpItem))
        {
            // 如果获取到符号，使用符号间的类型检查
            return checkTypeMatch(leftSymbol, rightSymbol);
        }

        // 处理没有符号的情况，直接使用OpItem的类型标签
        const auto& rightTypeLabel = rightOpItem.getTypeLabel();
        if (!rightTypeLabel)
        {
            return false;
        }

        // 获取左符号的类型标签
        const auto leftTypeLabel = getTypeLabelFromSymbol(leftSymbol);
        if (!leftTypeLabel)
        {
            return false;
        }

        // 特殊处理any类型
        if (rightTypeLabel->is("any"))
        {
            return true;
        }

        // 检查类型标签匹配
        return checkTypeMatch(leftTypeLabel, rightOpItem.getValueType(), true);
    }


    void CompileVisitor::processTypeAutoChange(const std::shared_ptr<Symbol>& sourceSymbol,
                                               const OpItem& targetOpItem) const
    {
        switch (sourceSymbol->getType())
        {
        case SymbolType::VARIABLE:
            {
                // 尝试将sourceSymbol转换为VariableSymbol
                const auto& varSymbol = std::static_pointer_cast<VariableSymbol>(sourceSymbol);
                const auto& targetSymbol = getSymbolFromOpItem(targetOpItem);
                const auto& targetValueType = targetSymbol
                                                  ? getValueTypeFromSymbol(targetSymbol)
                                                  : targetOpItem.getValueType();
                const auto& targetTypeLabel = targetSymbol
                                                  ? getTypeLabelFromSymbol(targetSymbol)
                                                  : targetOpItem.getTypeLabel();

                // 源类型为"any"时，直接采用目标值类型
                if (varSymbol->getTypeLabel()->is("any"))
                {
                    varSymbol->setValueType(targetValueType);
                    return; // 早期返回，减少嵌套
                }

                // 目标类型为"any"的情况处理
                if (targetTypeLabel->is("any"))
                {
                    // 确保目标值类型不为空
                    if (!targetValueType)
                    {
                        throw RCCCompilerError::compilerError(sourceSymbol->getPos().toString(),
                                                              topLexer()->getCodeLine(sourceSymbol->getPos()),
                                                              "[CompileVisitor::processTypeAutoChange] if (!targetValueType)  // true",
                                                              "We must ensure that the value type of the target operation item is not null.");
                    }

                    // 类型不匹配则抛出异常
                    if (targetValueType->isNot("any") &&
                        !checkTypeMatch(varSymbol->getTypeLabel(),
                                        targetValueType, false))
                    {
                        throw RCCCompilerError::typeMissmatchError(currentPos().toString(),
                                                                   topLexer()->getCodeLine(currentPos()),
                                                                   "Error symbol: " + varSymbol->toString(),
                                                                   getListFormatString({
                                                                       "any", varSymbol->getTypeLabel()->getVal()
                                                                   }),
                                                                   targetValueType->getVal(),
                                                                   {"You can try using the `any` type."});
                    }
                    varSymbol->setValueType(targetValueType);
                    return;
                }

                // 源类型和目标类型都不是"any"时，检查是否匹配
                if (!varSymbol->getTypeLabel()->equalWith(targetTypeLabel))
                {
                    throw RCCCompilerError::typeMissmatchError(currentPos().toString(),
                                                               topLexer()->getCodeLine(currentPos()),
                                                               "Error symbol: " + varSymbol->toString(),
                                                               getListFormatString({
                                                                   "any", varSymbol->getTypeLabel()->getVal()
                                                               }),
                                                               targetTypeLabel->getVal(),
                                                               {"You can try using the `any` type."});
                }
            }
            break;
        case SymbolType::FUNCTION:
            {
            }
            break;
        case SymbolType::LABEL:
            {
            }
            break;
        case SymbolType::PARAMETER:
            {
            }
            break;
        case SymbolType::CLASS:
            {
            }
            break;
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
        case BuiltinType::B_ANY: name = "any";
            break;
        case BuiltinType::B_BOOL: name = "bool";
            break;
        case BuiltinType::B_CHAR: name = "char";
            break;
        // ReSharper disable once CppDFAUnreachableCode
        case BuiltinType::B_DICT: name = "dict";
            break;
        case BuiltinType::B_FLOAT: name = "float";
            break;
        case BuiltinType::B_INT: name = "int";
            break;
        // ReSharper disable once CppDFAUnreachableCode
        case BuiltinType::B_LIST: name = "list";
            break;
        case BuiltinType::B_STR: name = "str";
            break;
        case BuiltinType::B_VOID: name = "void";
            break;
        case BuiltinType::B_NUL: name = "nul";
            break;
        case BuiltinType::B_FLAG: name = "flag";
            break;
        case BuiltinType::B_FUNC: name = "func";
            break;
        case BuiltinType::B_FUNI: name = "funi";
            break;
        default:
            pass("To process other builtin type");
        }
        return std::make_shared<TypeLabelSymbol>(pos, name, symbolTable.curScopeLevel());
    }

    std::pair<std::shared_ptr<TypeLabelSymbol>, std::shared_ptr<TypeLabelSymbol>> CompileVisitor::
    getTypesFromOpItem(const OpItem& opItem) const
    {
        const auto& symbol = getSymbolFromOpItem(opItem);
        const auto& typeLabel = symbol
                                    ? getTypeLabelFromSymbol(symbol)
                                    : opItem.getTypeLabel();
        const auto& valueType = symbol
                                    ? getValueTypeFromSymbol(symbol)
                                    : opItem.getValueType();
        return {typeLabel, valueType};
    }

    std::shared_ptr<TypeLabelSymbol> CompileVisitor::getDefiniteTypeLabelSymbolFromOpItem(
        const OpItem& opItem) const
    {
        const auto& [lt, vt] = getTypesFromOpItem(opItem);
        return lt->is("any") ? vt : lt;
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
            }
            else
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
                if (const auto& ref = std::dynamic_pointer_cast<VariableSymbol>(reference)->getReferencedSymbol())
                    reference = ref;
                else
                    break;
            }
            else
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
        if (const auto& it = extensionMap.find(extensionPath);
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

    void CompileVisitor::setSymbolBuiltinType(const std::shared_ptr<Symbol>& processingSymbol,
                                              const TypeOfBuiltin& type)
    {
        if (processingSymbol->is(SymbolType::FUNCTION))
        {
            const auto& funcSymbol = std::static_pointer_cast<FunctionSymbol>(processingSymbol);
            funcSymbol->setBuiltInType(type);
        }
        throw RCCCompilerError::compilerError(processingSymbol->getPos().getFilepath(),
                                              getCodeLine(processingSymbol->getPos()),
                                              "[void CompileVisitor::setSymbolBuiltinType] if (processingSymbol->is(SymbolType::FUNCTION))  // false",
                                              "When setting the built-in type of a symbol, we must ensure that the symbol is of function type.");
    }

    bool CompileVisitor::checkSymbolExists(const OpItem& opItem) const
    {
        return opItem.is(OpItemType::IDENTIFIER) && symbolTable.containsName(opItem.getVal());
    }

    bool CompileVisitor::checkSymbolExists(const std::shared_ptr<OpItem>& opItem) const
    {
        return opItem && opItem->is(OpItemType::IDENTIFIER) &&
            symbolTable.containsName(opItem->getVal());
    }

    bool CompileVisitor::checkSymbolExists(const std::shared_ptr<Symbol>& processingSymbol) const
    {
        return processingSymbol && symbolTable.containsName(processingSymbol->getVal());
    }

    void CompileVisitor::checkExists(const OpItem& opItem, const std::optional<Pos>& pos) const
    {
        const auto& itemPos = pos.has_value() ? pos.value() : opItem.getPos();
        if (opItem.is(OpItemType::IDENTIFIER) && !checkSymbolExists(opItem))
        {
            throw RCCCompilerError::symbolNotFoundError(itemPos.toString(), getCodeLine(itemPos), opItem.getVal(),
                                                        "Error operation item: " + opItem.toString(),
                                                        {
                                                            "If the name of this operation item is not defined by you, then this error may occur within the compiler."
                                                            " You are welcome to report this issue to the Rio team."
                                                        });
        }
    }

    void CompileVisitor::checkExists(const std::shared_ptr<OpItem>& opItem, const Pos& pos) const
    {

        if (opItem->is(OpItemType::IDENTIFIER) && !checkSymbolExists(opItem))
        {
            throw RCCCompilerError::symbolNotFoundError(pos.toString(), getCodeLine(pos), opItem->getVal(),
                                                           "Error operation item: " + opItem->toString(),
                                                           {
                                                               "If the name of this operation item is not defined by you, then this error may occur within the compiler."
                                                               " You are welcome to report this issue to the Rio team."
                                                           });
        }
    }

    void CompileVisitor::checkExists(const std::shared_ptr<Symbol>& processingSymbol) const
    {
        if (!checkSymbolExists(processingSymbol))
        {
            throw RCCCompilerError::symbolNotFoundError(processingSymbol->getPos().toString(),
                                                        getCodeLine(processingSymbol->getPos()),
                                                        processingSymbol->getVal(),
                                                        "Error operation item: " + processingSymbol->toString(),
                                                        {
                                                            "If the name of this operation item is not defined by you, then this error may occur within the compiler."
                                                            " You are welcome to report this issue to the Rio team."
                                                        });
        }
    }

    std::string CompileVisitor::getListFormatString(const std::vector<std::string>& list)
    {
        const size_t size = list.size();
        if (size == 0)
        {
            return "[]";
        }
        // 计算总长度
        size_t totalLength = 2; // []
        for (const auto& item : list)
        {
            totalLength += item.length();
        }
        totalLength += 2 * (size - 1); // ", " 分隔符
        // 一次性分配内存并构建字符串
        std::string result;
        result.resize(totalLength);
        char* ptr = result.data();
        *ptr++ = '[';
        // 处理第一个元素
        const std::string& first = list[0];
        std::memcpy(ptr, first.data(), first.length());
        ptr += first.length();
        // 处理剩余元素
        for (size_t i = 1; i < size; ++i)
        {
            *ptr++ = ',';
            *ptr++ = ' ';
            const std::string& item = list[i];
            std::memcpy(ptr, item.data(), item.length());
            ptr += item.length();
        }
        *ptr = ']';
        return result;
    }

    std::string CompileVisitor::getCodeLine(const Pos& pos)
    {
        if (pos.getFilepath() != topLexerPath())
        {
            return getLineFromFile(getAbsolutePath(pos.getFilepath(), getWindowsDefaultDir()),
                                   pos.getLine());
        }
        return topLexer()->getCodeLine(pos);
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

    std::string CompileVisitor::getNewTempVarName()
    {
        _temVarId++;
        return "tv" + std::to_string(_temVarId);
    }

    std::string CompileVisitor::getNewSetLabelName()
    {
        _setId++;
        return "sl" + std::to_string(_setId);
    }

    OpItem CompileVisitor::pushTemOpVarItemWithRecord(
        const Pos& pos, const std::shared_ptr<TypeLabelSymbol>& valueType,
        const std::shared_ptr<Symbol>& referencedSymbol, const bool& sysDefined,
        const std::shared_ptr<TypeLabelSymbol>& typeLabel)
    {
        const auto& anyType = getBuiltinTypeSymbol(pos, BuiltinType::B_ANY);
        const auto& tempVarId = VarID(getNewTempVarName(), pos.getFileField(), curScopeField(), curScopeLevel());
        pushIdentItem(tempVarId, typeLabel ? typeLabel : anyType, valueType, referencedSymbol, pos);
        symbolTable.insert(std::make_shared<VariableSymbol>(
                               pos, tempVarId.getNameField(), topOpItem().getRaVal(symbolTable),
                               std::unordered_set<std::shared_ptr<LabelSymbol>>{
                                   typeLabel ? typeLabel : anyType
                               },
                               symbolTable.curScopeLevel(),
                               false,
                               valueType ? valueType : anyType,
                               nullptr, referencedSymbol), sysDefined);
        raCodeBuilder << ri::ALLOT({tempVarId.getVid()});
        return topOpItem();
    }

    OpItem CompileVisitor::pushTemOpSetItem(const Pos& pos)
    {
        const auto& setId = SetID(getNewSetLabelName(), pos.getFileField(), curScopeField());
        pushOpItem(std::make_shared<OpItem>(
            OpItemType::SET_LABEL,
            TypeLabelSymbol::flagTypeSymbol(pos, symbolTable.curScopeLevel()),
            setId.getSid(),
            setId.getSid()));
        return topOpItem();
    }

    OpItem CompileVisitor::newTemOpSetItem(const Pos& pos) const
    {
        const auto& setId = SetID(getNewSetLabelName(), pos.getFileField(), curScopeField());
        return OpItem{
            OpItemType::SET_LABEL,
            TypeLabelSymbol::flagTypeSymbol(pos, symbolTable.curScopeLevel()),
            setId.getSid(),
            setId.getSid()
        };
    }

    VarID CompileVisitor::getThisFieldVarID(const Pos& pos) const
    {
        return VarID("this", pos.getFileField(), curScopeField(), curScopeLevel());
    }

    std::shared_ptr<VariableSymbol> CompileVisitor::getThisFieldSymbol(
        const std::shared_ptr<ClassSymbol>& classSymbol) const
    {
        const auto& thisFieldVarID = getThisFieldVarID(classSymbol->getPos());
        return std::make_shared<VariableSymbol>(
            getUnknownPos(),
            thisFieldVarID.getNameField(),
            thisFieldVarID.getVid(),
            std::unordered_set<std::shared_ptr<LabelSymbol>>{
                TypeLabelSymbol::getCustomTypeLabelSymbol(classSymbol->getRaVal(), symbolTable.curScopeLevel())
            },
            symbolTable.curScopeLevel(), true, nullptr, nullptr);
    }

    void CompileVisitor::pushLexer(const std::shared_ptr<lexer::Lexer>& lexer)
    {
        _lexers.push(lexer);
        _lexer_paths.push_back(lexer->getFilepath());
    }

    void CompileVisitor::popLexer()
    {
        if (_lexers.empty())
        {
            throw RCCCompilerError::compilerError(RCC_UNKNOWN_CONST,
                                                  getCodeLine(currentPos()),
                                                  "[void CompileVisitor::popLexer] if (_lexers.empty())  // true",
                                                  "Before removing the lexical analyzer, ensure that there is at least one lexical analyzer already available.");
        }
        _lexers.pop();
        _lexer_paths.pop_back();
    }

    std::shared_ptr<lexer::Lexer> CompileVisitor::topLexer()
    {
        return _lexers.top();
    }

    std::string CompileVisitor::topLexerPath()
    {
        return _lexer_paths.back();
    }

    std::list<std::string> CompileVisitor::getLexerFilePaths()
    {
        return _lexer_paths;
    }

    bool CompileVisitor::checkIsRecursiveImportByLexerPath(const std::string& extPath)
    {
        return std::ranges::find(_lexer_paths, extPath) != _lexer_paths.end();
    }

    void CompileVisitor::pushOpItem(const std::shared_ptr<OpItem>& opItem)
    {
        opItemStack.push(opItem);
    }

    OpItem CompileVisitor::rPopOpItem()
    {
        if (!hasNextOpItem())
        {
            throw RCCCompilerError::compilerError(RCC_UNKNOWN_CONST, RCC_UNKNOWN_CONST,
                                                  "[OpItem CompileVisitor::rPopOpItem] if (!hasNextOpItem())  // true"
                                                  , {
                                                      "When obtaining operation items, we must ensure that the operation item stack is not empty,"
                                                      " that is, we must first add new operation items to the stack."
                                                  });
        }
        auto item = *opItemStack.top();
        opItemStack.pop();
        return item;
    }

    void CompileVisitor::popOpItem()
    {
        if (!hasNextOpItem())
        {
            throw RCCCompilerError::compilerError(RCC_UNKNOWN_CONST, RCC_UNKNOWN_CONST,
                                                  "[void CompileVisitor::popOpItem] if (!hasNextOpItem())  // true"
                                                  , {
                                                      "When obtaining operation items, we must ensure that the operation item stack is not empty,"
                                                      " that is, we must first add new operation items to the stack."
                                                  });
        }
        opItemStack.pop();
    }

    void CompileVisitor::pushOpItem(
        const OpItemType& type,
        const std::shared_ptr<TypeLabelSymbol>& typeLabelSymbol,
        const std::string& value,
        const std::string& racode,
        const std::shared_ptr<Symbol>& referencedSymbol, const std::shared_ptr<TypeLabelSymbol>& valueTypeSymbol,
        const Pos& pos)
    {
        pushOpItem(std::make_shared<OpItem>(
            type, typeLabelSymbol, value, racode,
            valueTypeSymbol, referencedSymbol, pos));
    }

    void CompileVisitor::pushOpItem(
        const std::string& name,
        const std::string& fileField,
        const std::string& scopeField,
        const std::shared_ptr<TypeLabelSymbol>& typeLabelSymbol)
    {
        pushOpItem(std::make_shared<OpItem>(
            OpItemType::IDENTIFIER,
            typeLabelSymbol,
            name,
            VarID(name, fileField, scopeField, curScopeLevel()).getVid()));
    }

    void CompileVisitor::pushIdentItem(
        const VarID& varID,
        const std::shared_ptr<TypeLabelSymbol>& typeLabelSymbol,
        const std::shared_ptr<TypeLabelSymbol>& valueType,
        const std::shared_ptr<Symbol>& referencedSymbol, const Pos& pos)
    {
        pushOpItem(std::make_shared<OpItem>(
            OpItemType::IDENTIFIER,
            typeLabelSymbol,
            varID.getNameField(),
            varID.getVid(),
            valueType,
            referencedSymbol,
            pos));
    }

    OpItem CompileVisitor::topOpItem() const
    {
        if (!hasNextOpItem())
        {
            throw RCCCompilerError::compilerError(RCC_UNKNOWN_CONST, RCC_UNKNOWN_CONST,
                                                  "[OpItem CompileVisitor::topOpItem] if (!hasNextOpItem())  // true"
                                                  , {
                                                      "When obtaining operation items, we must ensure that the operation item stack is not empty,"
                                                      " that is, we must first add new operation items to the stack."
                                                  });
        }
        return *opItemStack.top();
    }

    std::shared_ptr<OpItem> CompileVisitor::topOpItemPtr() const
    {
        if (!hasNextOpItem())
        {
            throw RCCCompilerError::compilerError(RCC_UNKNOWN_CONST, RCC_UNKNOWN_CONST,
                                                  "[std::shared_ptr<OpItem> CompileVisitor::topOpItemPtr] if (!hasNextOpItem())  // true"
                                                  , {
                                                      "When obtaining operation items, we must ensure that the operation item stack is not empty,"
                                                      " that is, we must first add new operation items to the stack."
                                                  });
        }
        return opItemStack.top();
    }

    std::string CompileVisitor::topOpRaVal() const
    {
        return topOpItem().getRaVal(symbolTable);
    }

    std::string CompileVisitor::rPopOpItemRaVal()
    {
        return rPopOpItem().getRaVal(symbolTable);
    }

    std::string CompileVisitor::rPopOpItemVal()
    {
        return rPopOpItem().getVal();
    }

    bool CompileVisitor::hasNextOpItem() const
    {
        return !opItemStack.empty();
    }

    void CompileVisitor::clearOpItemStack()
    {
        while (hasNextOpItem()) popOpItem();
    }

    CompileVisitor::CompileVisitor(
        const std::string& programEntryFilePath,
        const std::string& programTargetFilePath,
        const std::string& compileOutputFilePath,
        const bool& needSaveOutput)
        : programEntryFilePath(programEntryFilePath),
          programTagetFilePath(programTargetFilePath),
          compileOutputFilePath(compileOutputFilePath),
          needSaveOutputToFile(needSaveOutput) {}

    SymbolTableManager& CompileVisitor::getSymbolTable()
    {
        return symbolTable;
    }

    ContentBuilder& CompileVisitor::getRaCodeBuilder()
    {
        return raCodeBuilder;
    }

    ContentBuilder& CompileVisitor::getAnalyzeBuilder()
    {
        return analyzeBuilder;
    }

    std::stack<std::shared_ptr<Symbol>>& CompileVisitor::getProcessingSymbolStack()
    {
        return processingSymbolStack;
    }

    std::stack<std::shared_ptr<OpItem>>& CompileVisitor::getOpItemStack()
    {
        return opItemStack;
    }

    std::stack<ScopeType>& CompileVisitor::getScopeTypeStack()
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

    void CompileVisitor::enableDebugMode(bool cond)
    {
    }

    std::string CompileVisitor::scopeTypeToString(ScopeType scopeType)
    {
        switch (scopeType)
        {
        case ScopeType::TEMPORARY: return "temporary";
        case ScopeType::GLOBAL: return "global";
        case ScopeType::PROGRAM: return "program";
        case ScopeType::EXPRESSION: return "expression";
        case ScopeType::CLASS: return "class";
        case ScopeType::FUNCTION: return "function";
        case ScopeType::LOOP: return "loop";
        case ScopeType::CONDITION: return "condition";
        case ScopeType::TRY: return "try";
        case ScopeType::CATCH: return "catch";
        case ScopeType::FINALLY: return "finally";
        default: return "unknown";
        }
    }

    std::string builtinTypeToString(const BuiltinType& type)
    {
        switch (type)
        {
        case BuiltinType::B_ANY: return "tp-any";
        case BuiltinType::B_BOOL: return "tp-bool";
        case BuiltinType::B_CHAR: return "tp-char";
        case BuiltinType::B_DICT: return "tp-dict";
        case BuiltinType::B_FLOAT: return "tp-float";
        case BuiltinType::B_INT: return "tp-int";
        case BuiltinType::B_LIST: return "tp-list";
        case BuiltinType::B_STR: return "tp-str";
        case BuiltinType::B_VOID: return "tp-void";
        case BuiltinType::B_NUL: return "tp-null";
        case BuiltinType::B_FLAG: return "tp-flag";
        case BuiltinType::B_FUNC: return "tp-func";
        case BuiltinType::B_FUNI: return "tp-funi";
        case BuiltinType::B_CLAS: return "tp-class";
        case BuiltinType::B_SERIES: return "tp-series";
        default: return "unknown";
        }
    }

    IRCCOpItemInterface::~IRCCOpItemInterface()
    {
    }

    void IRCCOpItemInterface::FreeOpItem(OpItem*& opItem)
    {
        delete opItem;
        opItem = nullptr;
    }

    IRCCCompileInterface::~IRCCCompileInterface()
    {
    }

    const char* IRCCCompileInterface::ScopeTypeToString(const ScopeType& scopeType)
    {
        return CompileVisitor::scopeTypeToString(scopeType).c_str();
    }

    const char* IRCCCompileInterface::ScopeTypeToFormatString(const ScopeType& scopeType)
    {
        return CompileVisitor::scopeTypeToFormatString(scopeType).c_str();
    }

    const char* CompileVisitor::GetNewRaValCorrespondingToNewVal(const char* val)
    {
        if (const auto& it = varIdMap.find(val);
            it != varIdMap.end())
        {
            return it->second->getVid().c_str();
        }
        const auto& varId = std::make_shared<
            VarID>(val, CurrentPos()->GetFileField(), CurScopeField(), CurScopeLevel());
        varIdMap.insert({val, varId});
        return varId->getVid().c_str();
    }

    const char* CompileVisitor::RaVal(const IRCCOpItemInterface* opItemI) const
    {
        return opItemI->GetRaVal(&symbolTable, true);
    }

    IRCCTypeLabelSymbolInterface* CompileVisitor::GetTypeLabelSymbolIByStr(const char* name,
                                                                           const std::size_t scopeLevel)
    {
        IRCCTypeLabelSymbolInterface* p = new TypeLabelSymbol(
            *TypeLabelSymbol::getTypeLabelSymbolByStr(name, scopeLevel));
        return p;
    }

    void CompileVisitor::FreeTypeLabelSymbolI(symbol::IRCCTypeLabelSymbolInterface*& symbol)
    {
        if (symbol)
        {
            delete symbol;
            symbol = nullptr;
        }
    }

    const char* CompileVisitor::GetNewTempVarName()
    {
        const auto& temp = getNewTempVarName();
        const auto result = new char[temp.length() + 1];
        strcpy(result, temp.c_str());
        return result;
    }

    void CompileVisitor::FreeCharP(const char* data)
    {
        delete[] data;
    }

    const char* CompileVisitor::GetNewSetLabelName()
    {
        return getNewSetLabelName().c_str();
    }

    bool CompileVisitor::CheckTypeMatch(const symbol::IRCCTypeLabelSymbolInterface* leftTypeSymbolI,
                                        const symbol::IRCCTypeLabelSymbolInterface* rightTypeSymbolI,
                                        const bool& restrict) const
    {
        return checkTypeMatch(std::static_pointer_cast<TypeLabelSymbol>(leftTypeSymbolI->TransformToTLSI()->copySelf()),
                              std::static_pointer_cast<
                                  TypeLabelSymbol>(rightTypeSymbolI->TransformToTLSI()->copySelf()), restrict);
    }

    utils::IRCCPosInterface* CompileVisitor::CurrentPos() const
    {
        return &_currentProcessingPos.top();
    }

    void CompileVisitor::SetCurrentPos(const utils::IRCCPosInterface* pos)
    {
        pushProcessingPos(*pos->TransformToPI());
    }

    void CompileVisitor::ResetCurrentPos()
    {
        popProcessingPos();
    }

    void CompileVisitor::FreeOpItemI(IRCCOpItemInterface*& opItemI)
    {
        if (opItemI)
        {
            delete opItemI;
            opItemI = nullptr;
        }
    }

    IRCCPosInterface* CompileVisitor::GetUnknownPosI() const
    {
        return getUnknownPosP();
    }

    const char* CompileVisitor::CreateCharP(const std::string& data)
    {
        const auto result = new char[data.length() + 1];
        strcpy(result, data.c_str());
        return result;
    }

    std::string CompileVisitor::scopeTypeToFormatString(const ScopeType scopeType)
    {
        return "[ScopeType: " + scopeTypeToString(scopeType) + "]";
    }

    std::string CompileVisitor::curScopeField() const
    {
        return scopeTypeToString(scopeTypeStack.top());
    }

    ScopeType CompileVisitor::curScopeType() const
    {
        return scopeTypeStack.top();
    }

    void CompileVisitor::enterScope(const ScopeType scopeType)
    {
        scopeTypeStack.push(scopeType);
        symbolTable.enterScope();
    }

    void CompileVisitor::enterScope(size_t scopeLevel)
    {
        symbolTable.enterScope(scopeLevel);
    }

    void CompileVisitor::enterTopScope()
    {
        symbolTable.enterTopScope();
    }

    void CompileVisitor::enterGlobalScope()
    {
        scopeTypeStack.push(ScopeType::GLOBAL);
        symbolTable.enterGlobalScope();
    }

    void CompileVisitor::exitScope(ScopeType scopeType)
    {
        if (scopeTypeStack.empty())
        {
            rlog::LogManager::rerror("Scope filed is empty, can not exit global scope.");
            return;
        }
        if (scopeTypeStack.top() != scopeType)
        {
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

    void CompileVisitor::enterLoopScope()
    {
        enterScope(ScopeType::LOOP);
        loopScopeStack.push(ScopeType::LOOP);
    }

    void CompileVisitor::exitLoopScope()
    {
        exitScope(ScopeType::LOOP);
        loopScopeStack.pop();
    }

    bool CompileVisitor::isInLoopScope() const
    {
        return !loopScopeStack.empty();
    }

    std::vector<std::shared_ptr<ExpressionNode>> CompileVisitor::visitParallelNode(
        const std::shared_ptr<ExpressionNode>& node)
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
        for (const auto& label : labels)
        {
            label->acceptVisitor(*this);
        }
        std::unordered_set<std::shared_ptr<LabelSymbol>> result{};
        for (const auto& [_, symbol] : symbolTable.currentNameMapScope().getTable())
        {
            if (symbol.second->getType() != SymbolType::LABEL)
            {
                throw RCCCompilerError::typeMissmatchError(symbol.second->getPos().toString(),
                                                           getCodeLine(symbol.second->getPos()),
                                                           "Error symbol: " + symbol.second->toString(),
                                                           symbolTypeToFormatString(SymbolType::LABEL),
                                                           symbolTypeToFormatString(symbol.second->getType()),
                                                           {"Please ensure that the labels you use are valid labels."});
            }
            result.insert(std::static_pointer_cast<LabelSymbol>(symbol.second));
        }
        exitScope(ScopeType::TEMPORARY);
        return result;
    }

    std::vector<std::shared_ptr<LabelSymbol>> CompileVisitor::processLabelNodesOnOrder(
        const std::vector<std::shared_ptr<LabelNode>>& labels)
    {
        std::vector<std::shared_ptr<LabelSymbol>> result{};
        enterScope(ScopeType::TEMPORARY);
        for (const auto& label : labels)
        {
            label->acceptVisitor(*this);
            if (const auto& [_, symbol] = symbolTable.findByName(label->getFullLabel());
                symbol->is(SymbolType::LABEL))
            {
                result.push_back(std::static_pointer_cast<LabelSymbol>(symbol));
                symbolTable.removeByName(label->getFullLabel());
            }
            else
            {
                throw RCCCompilerError::typeMissmatchError(symbol->getPos().toString(),
                                                           getCodeLine(symbol->getPos()),
                                                           "Error symbol: " + symbol->toString(),
                                                           symbolTypeToFormatString(SymbolType::LABEL),
                                                           symbolTypeToFormatString(symbol->getType()),
                                                           {"Please ensure that the labels you use are valid labels."});
            }
        }
        exitScope(ScopeType::TEMPORARY);
        return result;
    }

    bool CompileVisitor::compile()
    {
        const auto& lexer = std::make_shared<lexer::Lexer>(programTagetFilePath);
        pushLexer(lexer);
        parser::Parser parser(topLexer()->tokenize());
        const auto& [hasError, programNode] = parser.parse();
        if (hasError)
        {
            parser.printParserErrors();
            return false;
        }
        if (needSaveOutputToFile)
        {
            raCodeBuilder
                << ri::ANNOTATION(std::vector<std::string>{
                    RIO_PROGRAM_SIGN " " RCC_VERSION,
                    "Source file: " + getAbsolutePath(processRCCPath(programTagetFilePath), getWindowsDefaultDir()) +
                    ":1:1",
                    "Target file: " + getAbsolutePath(processRCCPath(compileOutputFilePath), getWindowsDefaultDir()) +
                    ":1:1",
                    "Author: @" + getSystemUserName(),
                    "Time: " + getCurrentTime(TimeFormat::ISO_WITH_TIME),
                    ""
                });
        }
        try
        {
            programNode->acceptVisitor(*this);
            const std::string raCode = raCodeBuilder.buildAll();
            if (needSaveOutputToFile)
            {
                if (!writeFile(compileOutputFilePath, raCode))
                {
                    throw std::runtime_error("Can not write file: " + compileOutputFilePath);
                }
            }
            processSymbolFlagOperation();
        }
        catch (const RCCError& _)
        {
            popLexer();
            throw;
        }
        popLexer();
        return true;
    }

    std::string CompileVisitor::getCompileResult()
    {
        return raCodeBuilder.buildAll();
    }

    void CompileVisitor::visitLiteralNode(LiteralNode& node)
    {
        rlog::LogManager::rerror("This should not be execute.");
    }

    void CompileVisitor::visitNullLiteralNode(NullLiteralNode& node)
    {
        pushOpItem(
            OpItemType::LITERAL_VALUE,
            getBuiltinTypeSymbol(node.getPos(), BuiltinType::B_NUL),
            node.literalString(), node.literalString(),
            nullptr, nullptr, node.getPos());
    }

    void CompileVisitor::visitStringLiteralNode(StringLiteralNode& node)
    {
        pushOpItem(
            OpItemType::LITERAL_VALUE,
            getBuiltinTypeSymbol(node.getPos(), BuiltinType::B_STR),
            node.literalString(), node.literalString(),
            nullptr, nullptr, node.getPos());
    }

    void CompileVisitor::visitNumberLiteralNode(NumberLiteralNode& node)
    {
        rlog::LogManager::rerror("This should not be execute.");
    }

    void CompileVisitor::visitIntegerLiteralNode(IntegerLiteralNode& node)
    {
        pushOpItem(
            OpItemType::LITERAL_VALUE,
            getBuiltinTypeSymbol(node.getPos(), BuiltinType::B_INT),
            node.literalString(), node.literalString(),
            nullptr, nullptr, node.getPos());
    }

    void CompileVisitor::visitFloatLiteralNode(FloatLiteralNode& node)
    {
        pushOpItem(
            OpItemType::LITERAL_VALUE,
            getBuiltinTypeSymbol(node.getPos(), BuiltinType::B_FLOAT),
            node.literalString(), node.literalString(),
            nullptr, nullptr, node.getPos());
    }

    void CompileVisitor::visitBooleanLiteralNode(BooleanLiteralNode& node)
    {
        pushOpItem(
            OpItemType::LITERAL_VALUE,
            getBuiltinTypeSymbol(node.getPos(), BuiltinType::B_BOOL),
            node.literalString(), node.literalString(),
            nullptr, nullptr, node.getPos());
    }

    void CompileVisitor::visitCharacterLiteralNode(CharacterLiteralNode& node)
    {
        pushOpItem(
            OpItemType::LITERAL_VALUE,
            getBuiltinTypeSymbol(node.getPos(), BuiltinType::B_CHAR),
            "\"" + std::string(1, StringManager::escapeChar(node.literalString()[0]))
            + "\"", node.literalString(),
            nullptr, nullptr, node.getPos());
    }

    void CompileVisitor::visitIdentifierNode(IdentifierNode& node)
    {
        const auto& anyTypeLabel = getBuiltinTypeSymbol(node.getPos(), BuiltinType::B_ANY);
        size_t searchLevel = curScopeLevel();
        // FixMe:
        if (isProcessingSymbol() && topProcessingSymbolType() == SymbolType::CLASS && curScopeType() ==
            ScopeType::CLASS)
        {
            searchLevel--;
        }
        if (const auto& [level, symbol] = symbolTable.findByName(node.getName(), searchLevel);
            level >= 0 && symbol)
        {
            auto symbolTypeLabel = anyTypeLabel;
            if (symbol->is(SymbolType::VARIABLE))
            {
                symbolTypeLabel = std::static_pointer_cast<VariableSymbol>(symbol)->getTypeLabel();
            }
            else if (symbol->is(SymbolType::FUNCTION))
            {
                symbolTypeLabel = std::static_pointer_cast<FunctionSymbol>(symbol)->getFunctionTypeLabel();
            }
            else if (symbol->is(SymbolType::CLASS))
            {
                symbolTypeLabel = TypeLabelSymbol::clasTypeSymbol(getUnknownPos(), curScopeLevel());
            }
            pushOpItem(OpItemType::IDENTIFIER, symbolTypeLabel,
                       symbol->getVal(), symbol->getRaVal(), symbol,
                       anyTypeLabel, node.getPos());
            return;
        }
        pushIdentItem({node.getName(), node.getPos().getFileField(), curScopeField(), curScopeLevel()},
                      anyTypeLabel, anyTypeLabel, nullptr, node.getPos());
    }

    void CompileVisitor::visitParameterNode(ParameterNode& node)
    {
        pass("To visit " + getNodeTypeName(node.getRealType()) + " type node.");
    }

    void CompileVisitor::visitArgumentNode(ArgumentNode& node)
    {
        pass("To visit " + getNodeTypeName(node.getRealType()) + " type node.");
    }

    void CompileVisitor::visitFunctionDeclarationNode(FunctionDeclarationNode& node)
    {
        pass("To visit " + getNodeTypeName(node.getRealType()) + " type node.");
    }

    void CompileVisitor::visitConstructorDefinitionNode(ConstructorDefinitionNode& node)
    {
        // 仅仅记录此构造函数，编译推迟到记录结束后进行
        const auto& topSymbol = topProcessingSymbol();
        if (!topSymbol || topSymbol->getType() != SymbolType::CLASS)
        {
            throw RCCCompilerError::scopeError(node.getPos().toString(), getCodeLine(node.getPos()),
                                               scopeTypeToFormatString(ScopeType::CLASS),
                                               scopeTypeToFormatString(scopeTypeStack.top()),
                                               {
                                                   "The constructor of a class can only appear within the scope of the class."
                                                   "Processing symbol: " + topSymbol->toString()
                                               },
                                               {"Please check if this constructor is within the scope of a class."});
        }
        enterScope(ScopeType::FUNCTION);
        const auto& classSymbol = std::static_pointer_cast<ClassSymbol>(topSymbol);
        const auto& paramItems = visitParallelNode(
            std::static_pointer_cast<ParenRangerNode>(node.getParamNode())->getRangerNode());
        std::vector<std::shared_ptr<ParameterSymbol>> paramSymbols{};
        std::vector<std::string> paramIdents{};
        std::vector<std::shared_ptr<LabelSymbol>> paramLabelDes{};
        processFunctionParams(paramItems, paramSymbols, paramIdents, paramLabelDes);
        auto getCtorName = [&]
        {
            // 拼接所有参数标识符，用下划线分隔
            const std::string paramIdent = std::accumulate(
                paramSymbols.begin(),
                paramSymbols.end(),
                std::string(),
                [](const std::string& acc, const std::shared_ptr<ParameterSymbol>& ident)
                {
                    const auto& typeName = ident->getTypeLabel()->getVal();
                    return acc.empty() ? typeName : acc + "_" + typeName;
                }
            );
            return classSymbol->getVal() + (paramIdent.empty() ? "_empty" : "_" + paramIdent);
        };
        const auto& ctorName = getCtorName();
        const auto& ctorID = VarID(ctorName, node.getPos().getFileField(), curScopeField(), curScopeLevel());
        const auto& funcNameOpItem = OpItem(
            OpItemType::IDENTIFIER, TypeLabelSymbol::funcTypeSymbol(getUnknownPos(), symbolTable.curScopeLevel()),
            ctorName, ctorID.getVid());
        const auto& labels = processLabelNodes(node.getLabelNodes());
        const auto& functionSymbol = std::make_shared<FunctionSymbol>(
            std::make_shared<ConstructorDefinitionNode>(node),
            node.getPos(),
            funcNameOpItem.getVal(),
            funcNameOpItem.getRaVal(symbolTable),
            labels, paramSymbols, nullptr,
            symbolTable.curScopeLevel() - 1,
            TypeOfBuiltin::ORDINARY, FunctionType::CONSTRUCTOR, classSymbol);
        const auto& signature = functionSymbol->hasReturnValue()
                                    ? TypeLabelSymbol::funiTypeSymbol(getUnknownPos(), curScopeLevel())
                                    : TypeLabelSymbol::funcTypeSymbol(getUnknownPos(), curScopeLevel());
        signature->appendLabelDes(paramLabelDes);
        if (functionSymbol->hasReturnValue()) signature->appendLabelDes({functionSymbol->getReturnType()});
        functionSymbol->setSignature(signature);
        if (!functionSymbol->getReturnType())
        {
            functionSymbol->setReturnType(getBuiltinTypeSymbol(functionSymbol->getPos(), BuiltinType::B_ANY));
        }
        exitScope(ScopeType::FUNCTION);
        classSymbol->addMember(functionSymbol, false);
    }

    void CompileVisitor::visitClassDeclarationNode(ClassDeclarationNode& node)
    {
        pass("To visit " + getNodeTypeName(node.getRealType()) + " type node.");
    }

    std::string CompileVisitor::compileConstructorNode(
        const std::shared_ptr<FunctionSymbol>& ctorSymbol)
    {
        std::stringstream code{};
        const auto& node = std::static_pointer_cast<ConstructorDefinitionNode>(ctorSymbol->getDefinitionNode());
        const auto& topSymbol = topProcessingSymbol();
        if (topSymbol->getType() != SymbolType::CLASS)
        {
            throw RCCCompilerError::scopeError(ctorSymbol->getPos().toString(), getCodeLine(ctorSymbol->getPos()),
                                               scopeTypeToFormatString(ScopeType::CLASS),
                                               scopeTypeToFormatString(scopeTypeStack.top()),
                                               {
                                                   "The constructor of a class can only appear within the scope of the class."
                                                   "Processing symbol: " + topSymbol->toString()
                                               },
                                               {"Please check if this constructor is within the scope of a class."});
        }
        enterScope(ScopeType::FUNCTION);
        const auto& classSymbol = std::static_pointer_cast<ClassSymbol>(topSymbol);
        const auto& thisFieldSymbol = getThisFieldSymbol(classSymbol);
        symbolTable.insert(thisFieldSymbol, true);
        std::vector<std::string> paramIdents{};
        for (const auto& param : ctorSymbol->getParameters())
        {
            paramIdents.push_back(param->getRaVal());
            symbolTable.insert(VariableSymbol::paramSymbolToVarSymbol(param, symbolTable.curScopeLevel()), false);
        }
        // 定义构造函数，构造一个 this 字段用于存储构造的对象
        code
            << ri::FUNI(ctorSymbol->getRaVal(), paramIdents).toRACode()
            << ri::ALLOT({thisFieldSymbol->getRaVal()}).toRACode()
            << ri::TP_NEW(classSymbol->getRaVal(), thisFieldSymbol->getRaVal()).toRACode();
        // 执行构造函数体
        raCodeBuilder.enterScope();
        node->getBodyNode()->acceptVisitor(*this);
        std::string ctorBody{};
        raCodeBuilder.buildCurScope(ctorBody).exitScope();
        // 构造函数返回 this
        code
            << ctorBody
            << ri::RET(thisFieldSymbol->getRaVal()).toRACode()
            << ri::END(ctorSymbol->getRaVal()).toRACode();
        ctorSymbol->
            reSetReturnType(TypeLabelSymbol::getCustomTypeLabelSymbol(classSymbol->getRaVal(), curScopeLevel()));
        exitScope(ScopeType::FUNCTION);
        return code.str();
    }

    void CompileVisitor::compileClassMembers(const std::string& classIdent, const std::shared_ptr<SymbolTable>& members,
                                             const LifeCycleLabel& memberType)
    {
        for (const auto& memberName : members->getNameIndex())
        {
            const auto& member = members->find(memberName);
            std::optional<std::string> defaultValue = std::nullopt;
            if (member->getType() == SymbolType::VARIABLE)
            {
                const auto& varSymbol = std::static_pointer_cast<VariableSymbol>(member);
                if (varSymbol->getDefaultValueNode())
                {
                    varSymbol->getDefaultValueNode()->acceptVisitor(*this);
                    defaultValue = rPopOpItemRaVal();
                }
                else if (varSymbol->getDefaultValue().has_value())
                {
                    defaultValue = varSymbol->getDefaultValue().value();
                }
            }
            else if (member->getType() == SymbolType::FUNCTION)
            {
                const auto& functionSymbol = std::static_pointer_cast<FunctionSymbol>(member);
                if (functionSymbol->getFunctionType() != FunctionType::METHOD)
                {
                    throw RCCCompilerError::typeMissmatchError(member->getPos().toString(),
                                                               getCodeLine(member->getPos()),
                                                               "Error symbol: " + functionSymbol->toString(),
                                                               functionTypeToFormatString(FunctionType::METHOD),
                                                               functionTypeToFormatString(
                                                                   functionSymbol->getFunctionType()),
                                                               {
                                                                   "The type of a class's member function must be of the "
                                                                   + functionTypeToFormatString(FunctionType::METHOD) +
                                                                   "."
                                                               });
                }
                functionSymbol->setScopeLevel(curScopeLevel());
                // symbolTable.insert(functionSymbol, false);
                raCodeBuilder.enterScope();
                functionSymbol->getDefinitionNode()->acceptVisitor(*this);
                std::string methodCode{};
                raCodeBuilder
                    .buildCurScope(methodCode)
                    .exitScope();
                raCodeBuilder
                    << ri::ANNOTATION(member->getPos().toString()).toRACode()
                    << methodCode;
                defaultValue = member->getRaVal();
            }
            annotatePos(member->getPos());
            if (memberType == LifeCycleLabel::INSTANCE)
            {
                raCodeBuilder
                    << ri::TP_ADD_INST_FIELD(classIdent, formatAttrField(member->getRaVal()),
                                             defaultValue.has_value() ? defaultValue.value() : "null");
            }
            else
            {
                raCodeBuilder
                    << ri::TP_ADD_TP_FIELD(classIdent, formatAttrField(member->getRaVal()),
                                           defaultValue.has_value() ? defaultValue.value() : "null");
            }
        }
    }

    void CompileVisitor::visitClassDefinitionNode(ClassDefinitionNode& node)
    {
        const auto& classNameNode = std::static_pointer_cast<IdentifierNode>(node.getNameNode());
        raCodeBuilder.enterScope();
        classNameNode->acceptVisitor(*this);
        raCodeBuilder.exitScope();
        const auto& classLabels = processLabelNodes(classNameNode->getLabels());
        const auto& classNameItem = rPopOpItem();
        const auto& classSymbol = std::make_shared<ClassSymbol>(
            node.getPos(), classNameItem.getVal(), classNameItem.getRaVal(symbolTable),
            classLabels, symbolTable.curScopeLevel(), symbolTable);
        TypeLabelSymbol::createCustomType(classSymbol->getVal(), classSymbol->getRaVal(), classSymbol);
        classSymbol->setVisitPermission(PermissionLabel::PUBLIC);
        pushNewProcessingSymbol(classSymbol);
        symbolTable.insert(classSymbol, false);
        // 收集类的成员变量、方法和构造函数，它们的初始化赋值应放到收集完成后，依据先静态后实例的顺序进行编译
        // FixMe: 需要重写成员变量的编译过程，不能使用变量定义的编译方案
        for (const auto& members = std::static_pointer_cast<BlockRangerNode>(node.getBodyNode())->getBodyExpressions();
             const auto& member : members)
        {
            switch (member->getRealType())
            {
            case NodeType::VAR:
                {
                    // 我们先收集类的变量声明，关于变量的初始值我们后续进行
                    for (const auto& varDefMemberNode = std::static_pointer_cast<VariableDefinitionNode>(member);
                         const auto& varDef : varDefMemberNode->getVarDefs())
                    {
                        VarID varId(varDef->getNameNode()->getName(), node.getPos().getFileField(), curScopeField(),
                                    curScopeLevel());
                        classSymbol->addMember(std::make_shared<VariableSymbol>(
                                                   varDef->getNameNode()->getPos(), varId.getNameField(),
                                                   varId.getVid(),
                                                   processLabelNodes(varDef->getLabelNodes()), curScopeLevel(),
                                                   true, nullptr, classSymbol,
                                                   nullptr, varDef->getValueNode()), false);
                    }
                }
                break;
            case NodeType::PREFIX:
                {
                    enterScope(ScopeType::TEMPORARY);
                    raCodeBuilder.enterScope();
                    member->acceptVisitor(*this);
                    exitScope(ScopeType::TEMPORARY);
                    raCodeBuilder.exitScope();
                }
                break;
            default: pass(
                    "To record " + getNodeTypeName(member->getRealType()) + " type node member for class definition.");
            }
        }
        classSymbol->setCollectionFinished();
        // 编译类和类的成员
        const auto& classIdent = classSymbol->getRaVal();
        raCodeBuilder.enterScope();
        if (const auto& parentClassSymbol = classSymbol->getDirectlyInheritedClassSymbol())
        {
            raCodeBuilder << ri::TP_DEF(classIdent, parentClassSymbol->getRaVal());
        }
        else
        {
            raCodeBuilder << ri::TP_DEF(classIdent);
        }
        enterScope(ScopeType::CLASS);
        compileClassMembers(classIdent, classSymbol->getStaticMembers(), LifeCycleLabel::STATIC);
        compileClassMembers(classIdent, classSymbol->getMembers(), LifeCycleLabel::INSTANCE);
        for (const auto& ctor : *classSymbol->getConstructors())
        {
            const auto& ctorCode = compileConstructorNode(
                std::static_pointer_cast<FunctionSymbol>(ctor));
            annotatePos(ctor->getPos());
            raCodeBuilder << ctorCode;
        }
        exitScope(ScopeType::CLASS);
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
        std::vector<std::pair<std::string, OpItem>>& orderedArgs)
    {
        const auto& argNode = std::static_pointer_cast<ParenRangerNode>(node.getRightNode());
        const auto& argNodes = visitParallelNode(argNode->getRangerNode());
        posArgs = {};
        namedArgs = {};
        orderedArgs = {};
        for (const auto& arg : argNodes)
        {
            if (arg->getRealType() == NodeType::ASSIGNMENT)
            {
                const auto& [identNode, valueNode] =
                    std::static_pointer_cast<AssignmentNode>(arg)->getAssignPair();
                identNode->acceptVisitor(*this);
                const auto& identName = rPopOpItemVal();
                valueNode->acceptVisitor(*this);
                auto resultOpItem = rPopOpItem();
                resultOpItem.setPos(valueNode->getPos());
                checkExists(resultOpItem);
                orderedArgs.push_back({identName, resultOpItem});
                namedArgs.insert({identName, resultOpItem});
            }
            else
            {
                arg->acceptVisitor(*this);
                auto resultOpItem = rPopOpItem();
                resultOpItem.setPos(arg->getPos());
                orderedArgs.push_back({"", resultOpItem});
                posArgs.push(resultOpItem);
            }
        }
    }

    std::shared_ptr<FunctionSymbol> CompileVisitor::getCtorSymbol(
        const std::shared_ptr<ClassSymbol>& classSymbol,
        const std::queue<OpItem>& posArgs,
        const std::unordered_map<std::string, OpItem>& namedArgs,
        const Pos& ctorCallPos,
        const std::vector<std::pair<std::string, OpItem>>& orderedArgs) const
    {
        const auto& argTypeStrings = [orderedArgs]
        {
            std::vector<std::string> typeStrs = {};
            for (const auto& arg : orderedArgs | std::views::values)
            {
                typeStrs.push_back(arg.getTypeLabel()->toString());
            }
            return typeStrs;
        }();
        // 将队列转换为向量以便索引访问
        std::vector<OpItem> posArgsVec;
        std::queue<OpItem> tempQueue = posArgs; // 复制队列
        while (!tempQueue.empty())
        {
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
                    }
                    break;
                case ParamType::PARAM_KEYWORD:
                    {
                        bool hasNamed = false;
                        if (auto it = tempNamedArgs.find(param->getVal());
                            it != tempNamedArgs.end())
                        {
                            matchedParam = checkTypeMatch(param, it->second);
                            tempNamedArgs.erase(it);
                            hasNamed = true;
                        }
                        if (!hasNamed && posArgIndex < posArgsVec.size())
                        {
                            matchedParam = checkTypeMatch(param, posArgsVec[posArgIndex]);
                            posArgIndex++;
                        }
                    }
                    break;
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
                    const auto& paramTypes = getListFormatString({
                        paramTypeToFormatString(ParamType::PARAM_POSITIONAL),
                        paramTypeToFormatString(ParamType::PARAM_KEYWORD),
                        paramTypeToFormatString(ParamType::PARAM_VAR_LEN_POSITIONAL),
                        paramTypeToFormatString(ParamType::PARAM_VAR_LEN_KEYWORD)
                    });
                    throw RCCCompilerError::typeMissmatchError(param->getPos().toString(), getCodeLine(param->getPos()),
                                                               "Error symbol: " + param->toString(),
                                                               paramTypes,
                                                               paramTypeToFormatString(param->getParamType()), {
                                                                   "There are four types of formal parameters: " +
                                                                   paramTypes,
                                                                   paramTypeToFormatString(ParamType::PARAM_POSITIONAL)
                                                                   + " param form: `arg: any`",
                                                                   paramTypeToFormatString(ParamType::PARAM_KEYWORD) +
                                                                   " param form: `arg: any = default_value`",
                                                                   paramTypeToFormatString(
                                                                       ParamType::PARAM_VAR_LEN_POSITIONAL) +
                                                                   " param form: `*args`",
                                                                   paramTypeToFormatString(
                                                                       ParamType::PARAM_VAR_LEN_KEYWORD) +
                                                                   " param form: `**kwargs`"
                                                               });
                }
                if (!matchedParam)
                {
                    matched = false;
                    break;
                }
            }
            if (matched && posArgIndex == posArgsVec.size() && tempNamedArgs.empty())
            {
                return funcSymbol;
            }
        }
        throw RCCCompilerError::symbolNotFoundError(
            ctorCallPos.toString(),
            getCodeLine(ctorCallPos),
            classSymbol->getVal(),
            std::vector<std::string>{
                "No constructor matching the argument types of this call was found.",
                "Formal parameter type list: " + getListFormatString(argTypeStrings)
            },
            {
                "Ensure that the constructor you call has been defined and the types of the passed"
                " parameters match the definition of the constructor."
            });
    }

    void CompileVisitor::visitFunctionCallNode(FunctionCallNode& node)
    {
        // 将具体逻辑委托给模块化后的实现（位于 rcc_function_call_compiler.cpp）
        processFunctionCallNode(node);
    }

    void CompileVisitor::visitProgramNode(ProgramNode& node)
    {
        auto errorPos = getUnknownPos();
        try
        {
            setCurrentProcessingFilePath(node.getPos().getFilepath());
            pushProcessingPos(node.getPos());
            builtin::initializePureBuiltinEnvironment(*this);
            enterScope(ScopeType::PROGRAM);
            for (auto& statement : node.getStatements())
            {
                errorPos = statement->getPos();
                pushProcessingPos(errorPos);
                statement->acceptVisitor(*this);
                popProcessingPos();
                clearOpItemStack();
            }
            processSymbolFlagOperation();
            exitScope(ScopeType::PROGRAM);
            setCurrentProcessingFilePath("");
            popProcessingPos();
        }
        catch (RCCError& e)
        {
            const auto& errorFilePath = errorPos.getFilepath();
            if (fileRecord.empty())
            {
                fileRecord = errorFilePath;
            }
            const auto& isExt = checkIsProcessedExtension(errorFilePath);
            e.addTraceInfo(RCCError::makeTraceInfo(
                listJoin(e.getTraceInfo()),
                errorPos.toString(),
                getCodeLine(errorPos),
                getPosStrFromFilePath(errorFilePath),
                makeFileIdentStr(
                    isExt
                    ? extensionPathNameMap.at(errorFilePath)
                    : errorFilePath,
                    isExt)));
            if (fileRecord != errorFilePath)
            {
                fileRecord = errorFilePath;
            }
            throw;
        }
    }

    void CompileVisitor::visitInfixNode(InfixExpressionNode& node)
    {
        node.getLeftNode()->acceptVisitor(*this);
        const auto& left = rPopOpItem();
        checkExists(left, node.getLeftNode()->getPos());
        const auto& [leftLabelType, leftValueType] = getTypesFromOpItem(left);
        const auto& leftType =
            leftLabelType && leftLabelType->equalWith(leftValueType) && leftLabelType->isNot("any")
                ? leftLabelType
                : leftValueType && leftValueType->is("nul")
                ? leftLabelType
                : leftValueType;
        node.getRightNode()->acceptVisitor(*this);
        const auto& right = rPopOpItem();
        const auto& [rightLabelType, rightValueType] = getTypesFromOpItem(right);
        const auto& rightType =
            rightLabelType && rightLabelType->equalWith(rightValueType) && rightLabelType->isNot("any")
                ? rightLabelType
                : rightValueType && rightValueType->is("nul")
                ? rightLabelType
                : rightValueType;
        std::shared_ptr<TypeLabelSymbol> resultValueType = nullptr;
        switch (node.getInfixType())
        {
        case NodeType::PLUS:
            {
                if (leftType->is("any") || rightType->is("any"))
                {
                    resultValueType = TypeLabelSymbol::anyTypeSymbol(node.getPos(), symbolTable.curScopeLevel());
                }
                else if (leftType->is("str") || rightType->is("str"))
                {
                    resultValueType = TypeLabelSymbol::strTypeSymbol(node.getPos(), symbolTable.curScopeLevel());
                }
                else if (leftType->is("int") && rightType->is("int"))
                {
                    resultValueType = TypeLabelSymbol::intTypeSymbol(node.getPos(), symbolTable.curScopeLevel());
                }
                else if ((leftType->is("int") || leftType->is("float")) &&
                    (rightType->is("int") || rightType->is("float")) &&
                    (leftType->is("float") || rightType->is("float")))
                {
                    resultValueType = TypeLabelSymbol::floatTypeSymbol(node.getPos(), symbolTable.curScopeLevel());
                }
                else if (leftType->is("list") || rightType->is("list"))
                {
                    resultValueType = TypeLabelSymbol::listTypeSymbol(node.getPos(), symbolTable.curScopeLevel());
                }
                else
                {
                    throw RCCCompilerError::typeMissmatchError(node.getPos().toString(), getCodeLine(node.getPos()),
                                                               "This error is of type mismatch in addition operation operands",
                                                               getListFormatString({
                                                                   "(any, any)", "(str, str)", "(int, int)",
                                                                   "(float | int, float | int)", "(list | any, any | list)"
                                                               }), "(" + leftType->getVal() + ", " + rightType->getVal()
                                                               + ")",
                                                               {});
                }
                pushTemOpVarItemWithRecord(node.getPos(), resultValueType);
                raCodeBuilder << ri::ADD(raVal(left, true), raVal(right, true), topOpRaVal());
            }
            break;
        case NodeType::DIVIDE:
            {
                pushTemOpVarItemWithRecord(node.getPos());
                raCodeBuilder << ri::DIV(raVal(left, true), raVal(right, true), topOpRaVal());
            }
            break;
        case NodeType::MINUS:
            {
                pushTemOpVarItemWithRecord(node.getPos());
                raCodeBuilder << ri::OPP(raVal(right, true), topOpRaVal())
                    << ri::ADD(raVal(left, true), topOpRaVal(), topOpRaVal());
            }
            break;
        case NodeType::MULTIPLY:
            {
                pushTemOpVarItemWithRecord(node.getPos());
                raCodeBuilder << ri::MUL(raVal(left, true), raVal(right, true), topOpRaVal());
            }
            break;
        case NodeType::MODULO:
            {
                pushTemOpVarItemWithRecord(node.getPos());
                raCodeBuilder << ri::MOD(raVal(left, true), raVal(right, true), topOpRaVal());
            }
            break;
        case NodeType::PAIR:
            {
                pushTemOpVarItemWithRecord(node.getPos());
                raCodeBuilder << ri::PAIR_SET(raVal(left, true), raVal(right, true), topOpRaVal());
            }
            break;
        case NodeType::COMPARE:
        case NodeType::LOGICAL:
            {
                pushTemOpVarItemWithRecord(node.getPos());
                if (const auto& it = RELATION_MAP.find(node.getOpToken().getValue());
                    it != RELATION_MAP.end())
                {
                    raCodeBuilder << ri::CMP(raVal(left, true), raVal(right, true), topOpRaVal())
                        << ri::CREL(topOpRaVal(), it->second, topOpRaVal());
                }
                else
                {
                    throw RCCCompilerError::symbolNotFoundError(node.getPos().toString(), getCodeLine(node.getPos()),
                                                                node.getOpToken().toString(),
                                                                "No logical relation operation corresponding to this symbol was found.",
                                                                {
                                                                    "Please ensure that the logical relational operators used are legal."
                                                                });
                }
            }
            break;
        case NodeType::ARGUMENT_ASSIGNMENT:
            {
                if (node.getOpToken().getValue() == "+=")
                {
                    raCodeBuilder
                        << ri::ADD(raVal(left, true), raVal(right, true), raVal(left, true));
                }
                else if (node.getOpToken().getValue() == "-=")
                {
                    pushTemOpVarItemWithRecord(node.getPos(), rightValueType, right.getReferencedSymbol());
                    raCodeBuilder
                        << ri::OPP(raVal(right, true), topOpRaVal())
                        << ri::ADD(raVal(left, true), rPopOpItemRaVal(), raVal(left, true));
                }
                else if (node.getOpToken().getValue() == "*=")
                {
                    raCodeBuilder
                        << ri::MUL(raVal(left, true), raVal(right, true), raVal(left, true));
                }
                else if (node.getOpToken().getValue() == "/=")
                {
                    raCodeBuilder
                        << ri::DIV(raVal(left, true), raVal(right, true), raVal(left, true));
                }
                else if (node.getOpToken().getValue() == "%=")
                {
                    raCodeBuilder
                        << ri::MOD(raVal(left, true), raVal(right, true), raVal(left, true));
                }
            }
            break;
        case NodeType::CALL:
            {
                pass();
            }
            break;
        case NodeType::PARALLEL:
            {
                pass();
            }
            break;
        case NodeType::ATTRIBUTE_EXPRESSION:
            {
                const auto& leftReferencedSymbol = getReferenceTargetSymbol(left);
                const std::shared_ptr<ClassSymbol>& classSymbol = leftReferencedSymbol && leftReferencedSymbol->
                                                                  is(SymbolType::CLASS)
                                                                      ? std::static_pointer_cast<ClassSymbol>(
                                                                          leftReferencedSymbol)
                                                                      : TypeLabelSymbol::getCustomClassSymbol(
                                                                          leftType->getRaVal());
                if (const auto& [memberSymbol, label] = classSymbol->findMemberSymbolInPermission(right.getVal());
                    memberSymbol)
                {
                    if (leftReferencedSymbol && leftReferencedSymbol->is(SymbolType::CLASS)
                        && label == LifeCycleLabel::INSTANCE)
                    {
                        throw RCCCompilerError::semanticError(
                            node.getPosStr(), getCodeLine(node.getPos()),
                            std::vector<std::string>{
                                "Incorrectly accessing an instance member through the class name.",
                                "Class symbol: " + classSymbol->toString(),
                                "Member symbol: " + memberSymbol->toString()
                            },
                            {
                                "Instance members should be accessed through class instances, not the class name itself.",
                                "Did you forget to create an instance of the class first?"
                            }
                        );
                    }
                    const auto& typeLabelSymbol = getTypeLabelFromSymbol(memberSymbol);
                    const VarID attrVarID(getNewTempVarName(), node.getPos().getFileField(), curScopeField(),
                                          curScopeLevel());
                    pushIdentItem(attrVarID, typeLabelSymbol, typeLabelSymbol, memberSymbol, node.getPos());
                    topOpItemPtr()->setBelonging(getSymbolFromOpItem(left), memberSymbol->getRaVal());
                    std::shared_ptr<Symbol> attrSymbol;
                    if (memberSymbol->is(SymbolType::VARIABLE) || memberSymbol->is(SymbolType::CLASS))
                    {
                        attrSymbol = memberSymbol->transform(attrVarID.getNameField(), attrVarID.getVid(),
                                                             curScopeLevel());
                    }
                    else if (memberSymbol->is(SymbolType::FUNCTION))
                    {
                        const auto& funcSymbol = std::static_pointer_cast<FunctionSymbol>(memberSymbol);
                        attrSymbol = std::make_shared<VariableSymbol>(getUnknownPos(), attrVarID.getNameField(),
                                                                      attrVarID.getVid(),
                                                                      std::unordered_set<std::shared_ptr<LabelSymbol>>{
                                                                          funcSymbol->hasReturnValue()
                                                                              ? getBuiltinTypeSymbol(
                                                                                  getUnknownPos(), BuiltinType::B_FUNI)
                                                                              : getBuiltinTypeSymbol(
                                                                                  getUnknownPos(), BuiltinType::B_FUNC)
                                                                      }, curScopeLevel(),
                                                                      true, nullptr, funcSymbol->getClassSymbol(),
                                                                      funcSymbol);
                    }
                    else
                    {
                        throw RCCCompilerError::typeMissmatchError(node.getPosStr(), getCodeLine(node.getPos()),
                                                                   "The type of the attribute symbol of the parent symbol does not match the expected type.",
                                                                   listJoin({
                                                                       symbolTypeToFormatString(SymbolType::VARIABLE),
                                                                       symbolTypeToFormatString(SymbolType::FUNCTION),
                                                                       symbolTypeToFormatString(SymbolType::CLASS)
                                                                   }), symbolTypeToFormatString(
                                                                       memberSymbol->getType()),
                                                                   {
                                                                       "The type of attribute of parent symbol can only be variable, class or function."
                                                                   });
                    }
                    symbolTable.insert(attrSymbol, true);
                    if (attrSymbol->is(SymbolType::CLASS))
                    {
                        TypeLabelSymbol::createCustomType(std::static_pointer_cast<ClassSymbol>(attrSymbol));
                    }
                    raCodeBuilder
                        << ri::ALLOT({attrSymbol->getRaVal()})
                        << ri::TP_GET_FIELD(raVal(left, true), "\"" + StringManager::escape(memberSymbol->getRaVal()) + "\"",
                                            topOpRaVal());
                }
                else
                {
                    throw RCCCompilerError::symbolNotFoundError(node.getPos().toString(), getCodeLine(node.getPos()),
                                                                memberSymbol
                                                                    ? memberSymbol->toString()
                                                                    : right.toString(),
                                                                StringVector{
                                                                    "The member symbol was not found when accessing the member attributes of the parent symbol.",
                                                                    "Parent symbol: " + (leftReferencedSymbol
                                                                        ? leftReferencedSymbol->toString()
                                                                        : getSymbolFromOpItem(left)->toString())
                                                                },
                                                                {
                                                                    "Please ensure that the member symbol to be accessed already exists."
                                                                });
                }
            }
            break;
        default:
            {
                throw RCCCompilerError::compilerError(node.getPos().toString(), getCodeLine(node.getPos()),
                                                      "[void CompileVisitor::visitInfixNode] switch (node.getInfixType())  // "
                                                      + getNodeTypeFormatName(node.getInfixType()),
                                                      "The compilation function of this infix expression type has not been implemented yet.");
            }
        }
    }

    void CompileVisitor::visitUnaryExpressionNode(UnaryExpressionNode& node)
    {
        if (node.getOpToken().getType() == TokenType::TOKEN_NOT)
        {
            node.getRightNode()->acceptVisitor(*this);
            auto resultOpItem = rPopOpItem();
            resultOpItem.setPos(node.getRightNode()->getPos());
            pushTemOpVarItemWithRecord(node.getPos(), TypeLabelSymbol::boolTypeSymbol(node.getPos(), curScopeLevel()),
                nullptr, true, nullptr);
            raCodeBuilder
            << ri::OPP(raVal(resultOpItem, true), topOpRaVal());
            return;
        }
        pass("To visit " + getNodeTypeName(node.getRealType()) + " type node.");
    }

    void CompileVisitor::visitExpressionStatementNode(ExpressionStatementNode& node)
    {
        pushProcessingPos(node.getPos());
        annotatePos(node.getPos());
        node.getExpression()->acceptVisitor(*this);
        clearOpItemStack();
        popProcessingPos();
    }

    void CompileVisitor::visitPrefixExpressionNode(PrefixExpressionNode& node)
    {
        pass("To visit " + getNodeTypeName(node.getRealType()) + " type node.");
    }

    void CompileVisitor::visitPostfixNode(PostfixExpressionNode& node)
    {
        node.getNode()->acceptVisitor(*this);
        const auto& leftVal = rPopOpItemRaVal();
        switch (node.getPostfixType())
        {
        case NodeType::INCREMENT:
            {
                raCodeBuilder << ri::ADD(leftVal, "1", leftVal);
            }
            break;
        case NodeType::DECREMENT:
            {
                raCodeBuilder << ri::ADD(leftVal, "-1", leftVal);
            }
            break;
        default:
            {
                pass("To compile other postfix type node: " + getNodeTypeName(node.getPostfixType()));
            }
        }
    }

    void CompileVisitor::visitParenRangerNode(ParenRangerNode& node)
    {
        if (node.getRangerNode()->getRealType() == NodeType::PARALLEL)
        {
            // series 字面量
            const auto& pos = node.getPos();
            const auto& seriesNode = std::static_pointer_cast<InfixExpressionNode>(node.getRangerNode());
            const auto& seriesTypeLabel = TypeLabelSymbol::seriesTypeSymbol(pos, curScopeLevel());
            LabelSymbol::LabelDes seriesLabelDes {};
            std::vector<std::string> itemIds {};
            for (const auto& seriesItems = visitParallelNode(seriesNode);
                const auto& item: seriesItems)
            {
                item->acceptVisitor(*this);
                const auto& opItem = rPopOpItem();
                checkExists(opItem);
                itemIds.push_back(raVal(opItem));
                seriesLabelDes.push_back(getDefiniteTypeLabelSymbolFromOpItem(opItem));
            }
            seriesTypeLabel->appendLastLabelDes(seriesLabelDes);
            pushTemOpVarItemWithRecord(pos, seriesTypeLabel,
                nullptr, true);
            const auto& tempOpItemRaVal = topOpRaVal();
            raCodeBuilder
            << ri::TP_SET(builtinTypeToString(BuiltinType::B_SERIES), tempOpItemRaVal)
            << (itemIds.empty()
            ? ri::ANNOTATION("Empty series is no need to used `ITER_APND` RI to set value.").toRACode()
            : ri::ITER_APND(itemIds, tempOpItemRaVal).toRACode());
        } else
        {
            node.getRangerNode()->acceptVisitor(*this);
        }
    }

    void CompileVisitor::visitBlockRangerNode(BlockRangerNode& node)
    {
        auto errorPos = getUnknownPos();
        try
        {
            for (const auto& statement : node.getBodyExpressions())
            {
                errorPos = statement->getMainToken().getPos();
                pushProcessingPos(errorPos);
                annotatePos(errorPos);
                statement->acceptVisitor(*this);
                popProcessingPos();
            }
        }
        catch (RCCError& e)
        {
            const auto& errorFilePath = errorPos.getFilepath();
            if (fileRecord.empty())
            {
                fileRecord = errorFilePath;
            }
            e.addTraceInfo(RCCError::makeTraceInfo(
                listJoin(e.getTraceInfo()),
                errorPos.toString(),
                getCodeLine(errorPos),
                node.getPosStr(),
                getCodeLine(node.getPos())));
            if (fileRecord != errorFilePath)
            {
                fileRecord = errorFilePath;
            }
            throw;
        }
    }

    void CompileVisitor::processFunctionParams(const std::vector<std::shared_ptr<ExpressionNode>>& paramItems,
                                               std::vector<std::shared_ptr<ParameterSymbol>>& paramSymbols,
                                               std::vector<std::string>& paramIdents,
                                               std::vector<std::shared_ptr<LabelSymbol>>& paramLabelDes)
    {
        for (const auto& item : paramItems)
        {
            bool isPosVarParam = false, isKWVarParam = false, isValidItem = false;
            auto paramType = ParamType::PARAM_POSITIONAL;
            auto param = item;
            std::optional<OpItem> defaultValue = std::nullopt;
            if (item->getRealType() == NodeType::ASSIGNMENT)
            {
                const auto& [ident, value] =
                    std::static_pointer_cast<AssignmentNode>(item)->getAssignPair();
                param = ident;
                value->acceptVisitor(*this);
                defaultValue = rPopOpItem();
                defaultValue->setPos(value->getPos());
                checkExists(defaultValue.value(), value->getPos());
                paramType = ParamType::PARAM_KEYWORD;
                isValidItem = true;
            }
            else if (item->getRealType() == NodeType::UNARY)
            {
                if (const auto& unaryParam =
                        std::static_pointer_cast<UnaryExpressionNode>(item);
                    unaryParam->getOpToken().getType() == TokenType::TOKEN_STAR)
                {
                    param = unaryParam->getRightNode();
                    paramType = ParamType::PARAM_VAR_LEN_POSITIONAL;
                    isValidItem = true;
                    isPosVarParam = true;
                }
                else if (unaryParam->getOpToken().getType() == TokenType::TOKEN_DOUBLE_STAR)
                {
                    param = unaryParam->getRightNode();
                    paramType = ParamType::PARAM_VAR_LEN_KEYWORD;
                    isValidItem = true;
                    isKWVarParam = true;
                }
            }
            else if (item->getRealType() == NodeType::IDENTIFIER)
            {
                isValidItem = true;
            }
            if (!isValidItem)
            {
                throw RCCCompilerError::typeMissmatchError(item->getPos().toString(), getCodeLine(item->getPos()),
                                                           "Error node: " + item->toString(),
                                                           getListFormatString({
                                                               getNodeTypeFormatName(NodeType::IDENTIFIER),
                                                               getNodeTypeFormatName(NodeType::ASSIGNMENT),
                                                               getNodeTypeFormatName(NodeType::UNARY)
                                                           }),
                                                           getNodeTypeFormatName(item->getRealType()), {
                                                               "The node types of the parameter node only include the following four types: "
                                                               + getListFormatString(
                                                                   {
                                                                       getNodeTypeFormatName(NodeType::IDENTIFIER),
                                                                       getNodeTypeFormatName(NodeType::ASSIGNMENT),
                                                                       getNodeTypeFormatName(NodeType::UNARY)
                                                                   }),
                                                               getNodeTypeFormatName(NodeType::IDENTIFIER) +
                                                               " param form: `arg: any`.",
                                                               getNodeTypeFormatName(NodeType::ASSIGNMENT) +
                                                               " param form: `arg: any = default_value`.",
                                                               getNodeTypeFormatName(NodeType::UNARY) +
                                                               " param form: `*args` or `**kwargs`."
                                                           });
            }
            const auto& tempParamNode = std::static_pointer_cast<IdentifierNode>(param);
            tempParamNode->acceptVisitor(*this);
            const auto& paramOpItem = rPopOpItem();
            paramIdents.push_back(paramOpItem.getRaVal(symbolTable));
            std::unordered_set<std::shared_ptr<LabelSymbol>> labels = processLabelNodes(tempParamNode->getLabels());
            const auto& paramSymbol = std::make_shared<ParameterSymbol>(
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
                    const auto& [fst, snd] = getTypesFromOpItem(defaultValue.value());
                    throw RCCCompilerError::typeMissmatchError(paramSymbol->getPos().toString(),
                                                               getCodeLine(paramSymbol->getPos()),
                                                               std::vector{
                                                                   "Error symbol: " + paramSymbol->toString(),
                                                                   "Type mismatched value: " + defaultValue.value().
                                                                   toString()
                                                               }, paramSymbol->getTypeLabel()->toString(),
                                                               fst
                                                                   ? fst->toString()
                                                                   : snd
                                                                   ? snd->toString()
                                                                   : RCC_UNKNOWN_CONST,
                                                               {
                                                                   "You can try using the `any` type to set the parameter types more loosely."
                                                               });
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
                                   true), true);
            paramLabelDes.push_back(isPosVarParam
                                        ? TypeLabelSymbol::varArgTypeSymbol(getUnknownPos(), curScopeLevel())
                                        : isKWVarParam
                                        ? TypeLabelSymbol::kwVarArgTypeSymbol(getUnknownPos(), curScopeLevel())
                                        : paramSymbol->getTypeLabel());
        }
    }

    void CompileVisitor::visitFunctionDefinitionNode(FunctionDefinitionNode& node)
    {
        enterScope(ScopeType::FUNCTION);
        if (node.getCallNode()->getRealType() != NodeType::CALL)
        {
            throw RCCCompilerError::compilerError(node.getPos().toString(), getCodeLine(node.getPos()),
                                                  "[void CompileVisitor::visitFunctionDefinitionNode] node.getCallNode()->getRealType() !="
                                                  " NodeType::CALL  // true, " + getNodeTypeName(
                                                      node.getCallNode()->getRealType()),
                                                  "There is a type error in the function call node, which may be caused by an error in the parser"
                                                  " when constructing the function call node.");
        }
        const auto& callNode = std::static_pointer_cast<FunctionCallNode>(
            node.getCallNode());
        callNode->getLeftNode()->acceptVisitor(*this);
        const auto& funcId = VarID(rPopOpItemVal(), node.getPos().getFileField(), curScopeField(), curScopeLevel());
        auto funcNameOpItem = OpItem(OpItemType::IDENTIFIER, TypeLabelSymbol::anyTypeSymbol(callNode->getPos(),
                                         curScopeLevel()), funcId.getNameField(), funcId.getVid(),
                                     nullptr, nullptr);
        if (funcNameOpItem.getReferencedSymbol() && !isProcessingSymbol())
        {
            const auto& anyTypeLabel = getBuiltinTypeSymbol(node.getPos(), BuiltinType::B_ANY);
            const auto& funcNameVarID = VarID(funcNameOpItem.getVal(), node.getPos().getFileField(), curScopeField(),
                                              curScopeLevel());
            funcNameOpItem = OpItem(
                OpItemType::IDENTIFIER,
                anyTypeLabel,
                funcNameOpItem.getVal(),
                funcNameVarID.getVid(),
                anyTypeLabel,
                nullptr);
        }
        std::shared_ptr<ExpressionNode> paramRangerNode = callNode->getRightNode();
        if (callNode->getRightNode()->getRealType() == NodeType::ANON_FUNCTION_DEFINITION)
        {
            const auto& paramNode =
                std::static_pointer_cast<AnonFunctionDefinitionNode>(callNode->getRightNode());
            paramRangerNode = paramNode->getParamNode();
        }
        const auto& paramNode = std::static_pointer_cast<ParenRangerNode>(paramRangerNode);
        const auto& paramItems = visitParallelNode(paramNode->getRangerNode());
        std::vector<std::shared_ptr<ParameterSymbol>> paramSymbols{};
        std::vector<std::string> paramIdents{};
        std::vector<std::shared_ptr<LabelSymbol>> paramLabelDes{};
        processFunctionParams(paramItems, paramSymbols, paramIdents, paramLabelDes);
        const auto& labels = processLabelNodes(node.getLabelNodes());
        std::shared_ptr<FunctionSymbol> functionSymbol = nullptr;
        std::shared_ptr<ClassSymbol> classSymbol = nullptr;
        auto createFunctionSymbol = [&](const FunctionType& funcType)
        {
            const auto& funcSymbol = std::make_shared<FunctionSymbol>(
                std::make_shared<FunctionDefinitionNode>(node),
                node.getPos(), funcNameOpItem.getVal(),
                funcType == FunctionType::METHOD ? funcId.getVid() : funcNameOpItem.getRaVal(symbolTable),
                labels, paramSymbols, nullptr,
                curScopeLevel() - 1, TypeOfBuiltin::ORDINARY, funcType
            );
            const auto& signature = funcSymbol->hasReturnValue()
                                        ? TypeLabelSymbol::funiTypeSymbol(getUnknownPos(), curScopeLevel())
                                        : TypeLabelSymbol::funcTypeSymbol(getUnknownPos(), curScopeLevel());
            signature->appendLabelDes(paramLabelDes);
            if (funcSymbol->hasReturnValue()) signature->appendLabelDes({funcSymbol->getReturnType()});
            funcSymbol->setSignature(signature);
            return funcSymbol;
        };
        if (isProcessingSymbol())
        {
            if (const auto& topSymbol = topProcessingSymbol();
                topSymbol->getType() == SymbolType::CLASS)
            {
                if (classSymbol = std::static_pointer_cast<ClassSymbol>(topSymbol);
                    !classSymbol->hasCollectionFinished())
                {
                    functionSymbol = createFunctionSymbol(FunctionType::METHOD);
                    functionSymbol->setClassSymbol(classSymbol);
                    classSymbol->addMember(functionSymbol, false);
                    exitScope(ScopeType::FUNCTION);
                    return;
                }
                if (classSymbol && classSymbol->hasCollectionFinished())
                {
                    const auto& [fst, snd] = classSymbol->findMemberSymbol(funcNameOpItem.getVal());
                    if (!fst || snd == LifeCycleLabel::COUNT)
                    {
                        throw RCCCompilerError::symbolNotFoundError(node.getPos().toString(),
                                                                    getCodeLine(node.getPos()),
                                                                    funcNameOpItem.toString(),
                                                                    StringVector{
                                                                        "This function symbol was not found in the member properties of the class.",
                                                                        "Class symbol: " + classSymbol->toString(),
                                                                        "Class member attributes: " +
                                                                        getListFormatString([classSymbol]
                                                                        {
                                                                            StringVector memberNames{};
                                                                            for (const auto& member : *classSymbol->
                                                                                getMembers())
                                                                            {
                                                                                memberNames.push_back(member->getVal());
                                                                            }
                                                                            for (const auto& member : *classSymbol->
                                                                                getStaticMembers())
                                                                            {
                                                                                memberNames.push_back(member->getVal());
                                                                            }
                                                                            return memberNames;
                                                                        }())
                                                                    },
                                                                    {
                                                                        "Please ensure that the class member attributes being called have indeed been defined."
                                                                    });
                    }
                    functionSymbol = std::static_pointer_cast<FunctionSymbol>(fst);
                }
            }
        }
        if (classSymbol)
        {
            if (!functionSymbol->getLabelMarkManager().isLifeCycleLabelMarked(LifeCycleLabel::STATIC))
            {
                const auto thisFieldSymbol = getThisFieldSymbol(classSymbol);
                paramIdents.insert(paramIdents.begin(), thisFieldSymbol->getRaVal());
                symbolTable.insert(thisFieldSymbol, true);
            }
        }
        else if (!functionSymbol)
        {
            functionSymbol = createFunctionSymbol(FunctionType::FUNCTION);
            symbolTable.insert(functionSymbol, false);
        }
        if (!functionSymbol->getReturnType())
        {
            functionSymbol->setReturnType(getBuiltinTypeSymbol(functionSymbol->getPos(), BuiltinType::B_ANY));
        }
        pushNewProcessingSymbol(functionSymbol);
        raCodeBuilder << (functionSymbol->hasReturnValue()
                              ? ri::FUNI(functionSymbol->getRaVal(), paramIdents).toRACode()
                              : ri::FUNC(functionSymbol->getRaVal(), paramIdents).toRACode());
        node.getBodyNode()->acceptVisitor(*this);
        if (!functionSymbol->is(TypeOfBuiltin::BUILTIN) &&
            functionSymbol->hasReturnValue() && !functionSymbol->hasReturned())
        {
            if (functionSymbol->getReturnType()->isNot("any"))
            {
                throw RCCCompilerError::semanticError(node.getPos().toString(), getCodeLine(node.getPos()),
                                                      StringVector{
                                                          "A necessary return statement is missing when defining a function with a return value.",
                                                          "Defining function symbol: " + functionSymbol->toString(),
                                                          "Return type: " + functionSymbol->getReturnType()->toString()
                                                      },
                                                      {
                                                          "When the return type of a function with a return value is not `any`,"
                                                          " the `ret` keyword must be used to return a value that matches the return type."
                                                      });
            }
            raCodeBuilder << ri::RET("null");
        }
        raCodeBuilder << ri::END(functionSymbol->getRaVal());
        exitScope(ScopeType::FUNCTION);
        popProcessingSymbol();
    }

    void CompileVisitor::processLabelDesNode(const std::shared_ptr<ExpressionNode>& labelNode,
                                             std::vector<std::shared_ptr<LabelNode>>& desLabelNodes)
    {
        if (labelNode->getType() == NodeType::LABEL)
        {
            desLabelNodes.push_back(std::static_pointer_cast<LabelNode>(labelNode));
        }
        else if (labelNode->getRealType() == NodeType::INDEX_EXPRESSION)
        {
            // TODO
            pass("Process index expression type label node.");
        } else if (labelNode->getRealType() == NodeType::IDENTIFIER)
        {
            desLabelNodes.push_back(std::make_shared<LabelNode>(std::vector{labelNode->getMainToken()}));
        }
    }

    bool CompileVisitor::isVisitingProgramEntry() const
    {
        return getCurrentProcessingFilePath() == programEntryFilePath;
    }

    std::shared_ptr<Symbol> CompileVisitor::processAttribute(const std::shared_ptr<Symbol> &leftSymbol, const std::string &attr, const LifeCycleLabel& lifeCycleLabel)
    {
        if (leftSymbol->is(SymbolType::CLASS))
        {
            const auto& classSymbol = std::static_pointer_cast<ClassSymbol>(leftSymbol);
            if (const auto& [attrSymbol, lifeCycle] = classSymbol->findMemberSymbolInPermission(attr);
                attrSymbol && lifeCycle == lifeCycleLabel)
            {
                return attrSymbol;
            }
            throw std::runtime_error("");
        }
        if (leftSymbol->is(SymbolType::VARIABLE))
        {
            const auto &variableSymbol = std::static_pointer_cast<VariableSymbol>(leftSymbol);
            if (const auto &refSymbol = getReferenceTargetSymbol(variableSymbol))
            {
                return processAttribute(refSymbol, attr, lifeCycleLabel);
            }
            if (const auto& labelType = variableSymbol->getTypeLabel();
                                TypeLabelSymbol::isCustomType(labelType->getRaVal()))
            {
                const auto&classSymbol = TypeLabelSymbol::getCustomClassSymbol(labelType->getRaVal());
                return processAttribute(classSymbol, attr, LifeCycleLabel::INSTANCE);
            }
            if (const auto& valueType = variableSymbol->getValueType();
                TypeLabelSymbol::isCustomType(valueType->getRaVal()))
            {
                const auto&classSymbol = TypeLabelSymbol::getCustomClassSymbol(valueType->getRaVal());
                return processAttribute(classSymbol, attr, LifeCycleLabel::INSTANCE);
            }
        }
        throw std::runtime_error("");
    }

    void CompileVisitor::visitLabelNode(LabelNode& node)
    {
        if (const auto& label = node.getFullLabel();
            TypeLabelSymbol::isTypeLabel(label))
        {
            const auto& labelSymbol = std::make_shared<TypeLabelSymbol>(
                node.getPos(), label, curScopeLevel());
            for (const auto& labelDesNodes = node.getLabelDesNodes();
                 const auto& desListNode : labelDesNodes)
            {
                std::vector<std::shared_ptr<LabelNode>> desLabelNodes{};
                for (const auto& labelNode : visitParallelNode(desListNode->getBodyNode()))
                {
                    processLabelDesNode(labelNode, desLabelNodes);
                }
                labelSymbol->appendLabelDes(processLabelNodesOnOrder(desLabelNodes));
            }
            labelSymbol->handleLabelDesRecorded();
            symbolTable.insert(labelSymbol, true);
            // ToDo: type label
        }
        else if (const auto& labelPath = node.getLabelPath();
            labelPath.size() > 1)
        {
            std::shared_ptr<Symbol> leftSymbol = nullptr;
            for (const auto& lp: labelPath)
            {
                if (!leftSymbol) {
                    if (const auto& [level, symbol] = symbolTable.findByName(lp);
                       level >= 0 && symbol)
                    {
                        leftSymbol = symbol;
                    }
                }
                else
                {
                    leftSymbol = processAttribute(leftSymbol, lp);
                }
                if (!leftSymbol)
                {
                    throw RCCCompilerError::labelError(node.getPos().toString(),
                        getCodeLine(node.getPos()), {
                            "Full label: " + node.getFullLabel(),
                            "Error part: " + lp
                        }, {
                            "Please ensure the label you used is correct."
                        });
                }
            }
            symbolTable.insert(std::make_shared<TypeLabelSymbol>(
                node.getPos(), leftSymbol->getVal(), curScopeLevel(), leftSymbol->getRaVal()),
                false);
        } else
        {
            if (const auto& [fst, snd] = symbolTable.findByName(label);
                fst >= 0)
            {
                std::shared_ptr<ClassSymbol> classSymbol;
                bool isValidLabel = false;
                if (snd->is(SymbolType::CLASS))
                {
                    classSymbol = std::static_pointer_cast<ClassSymbol>(snd);
                    isValidLabel = true;
                }
                else if (snd->is(SymbolType::VARIABLE))
                {
                    const auto& variableSymbol = std::static_pointer_cast<VariableSymbol>(snd);
                    if (const auto& refSymbol = getReferenceTargetSymbol(variableSymbol);
                        refSymbol->is(SymbolType::CLASS))
                    {
                        classSymbol = std::static_pointer_cast<ClassSymbol>(refSymbol);
                        isValidLabel = true;
                    }
                }
                if (!isValidLabel)
                {
                    throw RCCCompilerError::typeMissmatchError(node.getPosStr(), getCodeLine(node.getPos()),
                                                               StringVector{
                                                                   "There is an error in the symbol type of the custom type label.",
                                                                   "Error label: " + label
                                                               }, symbolTypeToFormatString(SymbolType::CLASS),
                                                               symbolTypeToFormatString(snd->getType()), {
                                                                   "Please ensure that the symbol type of the custom type label is "
                                                                   +
                                                                   symbolTypeToFormatString(SymbolType::CLASS)
                                                               });
                }
                symbolTable.insert(std::make_shared<TypeLabelSymbol>(node.getPos(),
                                                                     label, symbolTable.curScopeLevel(),
                                                                     classSymbol->getRaVal()), false);
            }
            else
            {
                try
                {
                    symbolTable.insert(std::make_shared<LabelSymbol>(
                                       node.getPos(), label, "RID-" + label,
                                       symbolTable.curScopeLevel(), LabelType::UNKNOWN_TYPE_LABEL),
                                   false);
                } catch (...)
                {
                    throw RCCCompilerError::labelError(node.getPos().toString(),
                        getCodeLine(node.getPos()), {
                            "Error label: " + node.getFullLabel()
                        }, {
                            "Please ensure the label you used is correct."
                        });
                }
            }
        }
    }

    void CompileVisitor::visitBranchNode(BranchNode& node)
    {
        raCodeBuilder
            << ri::ANNOTATION(node.getPos().toString());
        const auto& endSetLabel = rPopOpItemRaVal();
        node.getConditionNode()->acceptVisitor(*this);
        const auto& conditionVal = rPopOpItemRaVal();
        const auto& skipSetLabel = newTemOpSetItem(node.getPos()).getRaVal(symbolTable);
        raCodeBuilder
            << ri::JF(conditionVal, skipSetLabel);
        node.getBodyNode()->acceptVisitor(*this);
        raCodeBuilder
            << ri::JMP(endSetLabel)
            << ri::SET(skipSetLabel);
    }

    void CompileVisitor::visitConditionNode(ConditionNode& node)
    {
        const auto& [nameMapScope, ridMapScope]
            = symbolTable.currentScopeCopied();
        const auto& nameMapScopeRecord = std::static_pointer_cast<SymbolTable>(nameMapScope.copySelf());
        const auto& ridMapScopeRecord = std::static_pointer_cast<SymbolTable>(ridMapScope.copySelf());
        // 条件语句的结束 SET 标签
        const auto& endSetLabel = std::make_shared<OpItem>(
            OpItemType::SET_LABEL,
            getBuiltinTypeSymbol(node.getPos(), BuiltinType::B_FLAG),
            SetID(getNewSetLabelName(), node.getPos().getFileField(), curScopeField()).getSid());
        for (const auto& branchNode : node.getBranchNodes())
        {
            pushOpItem(endSetLabel);
            enterScope(ScopeType::CONDITION);
            branchNode->acceptVisitor(*this);
            exitScope(ScopeType::CONDITION);
            symbolTable.currentNameMapScope() = *std::static_pointer_cast<SymbolTable>(nameMapScopeRecord->copySelf());
            symbolTable.currentRIDMapScope() = *std::static_pointer_cast<SymbolTable>(ridMapScopeRecord->copySelf());
        }
        raCodeBuilder << ri::SET(endSetLabel->getRaVal(symbolTable));
    }

    void CompileVisitor::visitLoopNode(LoopNode& node)
    {
        enterLoopScope();
        switch (node.getLoopType())
        {
        case LoopType::FOR:
        case LoopType::WHILE:
            {
                pushTemOpVarItemWithRecord(node.getPos());
                raCodeBuilder
                    << ri::CMP(RCC_TRUE, RCC_FALSE, topOpRaVal())
                    << ri::UNTIL(rPopOpItemRaVal(), RCC_REL_RE);
                node.getConditionNode()->acceptVisitor(*this);
                const auto& continueSetLabel = newTemOpSetItem(node.getPos());
                raCodeBuilder
                    << ri::JT(rPopOpItemRaVal(), continueSetLabel.getRaVal(symbolTable))
                    << ri::EXIT("UNTIL")
                    << ri::SET(continueSetLabel.getRaVal(symbolTable));
                enterScope(ScopeType::LOOP);
                node.getBodyNode()->acceptVisitor(*this);
                exitScope(ScopeType::LOOP);
                raCodeBuilder
                    << ri::END("UNTIL");
            }
            break;
        case LoopType::UNTIL:
        default:
            pass("To process other loop type node: " + getNodeTypeName(node.getRealType()));
        }
        exitLoopScope();
    }

    void CompileVisitor::visitForLoopNode(ForLoopNode& node)
    {
        enterLoopScope();
        node.getInitNode()->acceptVisitor(*this);
        pushTemOpVarItemWithRecord(node.getPos());
        raCodeBuilder
            << ri::CMP(RCC_TRUE, RCC_FALSE, topOpRaVal())
            << ri::UNTIL(rPopOpItemRaVal(), RCC_REL_RE);
        node.getConditionNode()->acceptVisitor(*this);
        const auto& continueSetLabel = newTemOpSetItem(node.getPos());
        raCodeBuilder
            << ri::JT(rPopOpItemRaVal(), continueSetLabel.getRaVal(symbolTable))
            << ri::EXIT("UNTIL")
            << ri::SET(continueSetLabel.getRaVal(symbolTable));
        enterScope(ScopeType::LOOP);
        node.getBodyNode()->acceptVisitor(*this);
        exitScope(ScopeType::LOOP);
        node.getUpdateNode()->acceptVisitor(*this);
        raCodeBuilder
            << ri::END("UNTIL");
        exitLoopScope();
    }

    void CompileVisitor::visitPassExpressionNode(PassExpressionNode& node)
    {
        pass();
    }

    void CompileVisitor::visitEncapsulatedExpressionNode(EncapsulatedExpressionNode& node)
    {
        if (const auto& processionSymbol = topProcessingSymbol();
            processionSymbol->is(SymbolType::FUNCTION))
        {
            const auto& funcSymbol = std::static_pointer_cast<FunctionSymbol>(processionSymbol);
            if (!builtin::isBuiltinFunction(funcSymbol->getVal()))
            {
                throw RCCCompilerError::semanticError(node.getPosStr(), getCodeLine(node.getPos()),
                                                      StringVector{
                                                          "Misuse of the `encapsulated` keyword for ordinary functions",
                                                          "Defining function: " + funcSymbol->toString()
                                                      },
                                                      {
                                                          "The `encapsulated` keyword can only be used for built-in symbol's definition."
                                                      });
            }
            funcSymbol->setBuiltInType(TypeOfBuiltin::BUILTIN);
        }
        raCodeBuilder
            << ri::ANNOTATION("The implementation is encapsulated.");
    }

    void CompileVisitor::visitReturnExpressionNode(ReturnExpressionNode& node)
    {
        if (!isProcessingSymbol() || topProcessingSymbolType() != SymbolType::FUNCTION)
        {
            throw RCCCompilerError::semanticError(node.getPosStr(), getCodeLine(node.getPos()),
                                                  StringVector{
                                                      "Using the `ret` return statement outside of a function scope is incorrect.",
                                                      "Procession symbol: " + (isProcessingSymbol()
                                                                                   ? topProcessingSymbol()->toString()
                                                                                   : "[null]"),
                                                      "Current scope field: " + scopeTypeToFormatString(
                                                          scopeTypeStack.top())
                                                  },
                                                  {
                                                      "The return statement with the `ret` keyword can only be used within the function scope."
                                                  });
        }
        const auto& funcSymbol = std::static_pointer_cast<FunctionSymbol>(topProcessingSymbol());
        funcSymbol->setHasReturned(true);
        if (funcSymbol->hasReturnValue() && !node.getReturnNode())
        {
            if (funcSymbol->getReturnType()->isNot("any"))
            {
                throw RCCCompilerError::semanticError(
                    node.getPosStr(),
                    getCodeLine(node.getPos()),
                    StringVector{
                        "Functions with a return value type must have a return value."
                        "Procession symbol: " + topProcessingSymbol()->toString(),
                    },
                    {
                        "Add a return statement with the appropriate return value type",
                        "Or change the function return type to 'any' if appropriate"
                    }
                );
            }
            raCodeBuilder << ri::RET("null");
            return;
        }
        if (!funcSymbol->hasReturnValue() && node.getReturnNode())
        {
            throw RCCCompilerError::semanticError(
                node.getPosStr(),
                getCodeLine(node.getPos()),
                StringVector{
                    "Functions with a return type of void are not allowed to have a return value.",
                    "Procession symbol: " + topProcessingSymbol()->toString()
                },
                {
                    "Remove the return value from the return statement",
                    "Or change the function return type to match the return value"
                }
            );
        }
        if (node.getReturnNode())
        {
            node.getReturnNode()->acceptVisitor(*this);
            const auto& returnVal = rPopOpItem();
            checkExists(returnVal);
            if (!checkTypeMatch(funcSymbol->getReturnType(), returnVal))
            {
                throw RCCCompilerError::typeMissmatchError(
                    node.getPosStr(),
                    getCodeLine(node.getPos()),
                    "Return value type does not match function return value type.",
                    funcSymbol->getReturnType()->toString(),
                    [this, returnVal]
                    {
                        const auto& [tl, vl] = getTypesFromOpItem(returnVal);
                        if (tl && tl->isNot("any")) return tl->toString();
                        if (vl) return vl->toString();
                        return returnVal.toString();
                    }(),
                    {
                        "Change the return value to match the function's return type: " + funcSymbol->getReturnType()->
                        toString(),
                        "Or modify the function return type to match the return value type: " +
                        opItemTypeToFormatString(returnVal.getType())
                    }
                );
            }
            processTypeAutoChange(funcSymbol, returnVal);
            raCodeBuilder
                << ri::RET(returnVal.getRaVal(symbolTable));
        }
        else
        {
            raCodeBuilder << ri::RET();
        }
    }

    void CompileVisitor::visitBreakExpressionNode(BreakExpressionNode& node)
    {
        if (!isInLoopScope())
        {
            throw RCCCompilerError::semanticError(
                node.getPosStr(),
                getCodeLine(node.getPos()),
                StringVector{
                    "Invalid use of `break` keyword.",
                    "The `break` keyword can only be used inside loop scopes (for, while).",
                    "Current scope type: " + scopeTypeToFormatString(curScopeType())
                },
                {
                    "Move the break statement inside a loop structure.",
                    "If you intended to exit a function, use `ret` instead."
                }
            );
        }
        raCodeBuilder << ri::EXIT("LOOP");
    }

    void CompileVisitor::visitAnonFunctionDefinitionNode(AnonFunctionDefinitionNode& node)
    {
        enterScope(ScopeType::ANONYMOUS);
        std::vector<std::shared_ptr<ParameterSymbol>> paramSymbols{};
        std::vector<std::string> paramIdents{};
        std::vector<std::shared_ptr<LabelSymbol>> paramLabelDes{};
        if (node.getParamNode())
        {
            const auto& paramNode = std::static_pointer_cast<ParenRangerNode>(node.getParamNode());
            const auto& paramItems = visitParallelNode(paramNode->getRangerNode());
            processFunctionParams(paramItems, paramSymbols, paramIdents, paramLabelDes);
        }
        const auto& labels = processLabelNodes(node.getLabelNodes());
        std::shared_ptr<FunctionSymbol> functionSymbol = nullptr;
        auto createFunctionSymbol = [&](const FunctionType& funcType)
        {
            const VarID anonFuncVarID(ANONYMOUS_FUNCTION_PREFIX + getNewTempVarName(), node.getPos().getFileField(),
                                      curScopeField(), curScopeLevel());
            const auto& funcSymbol = std::make_shared<FunctionSymbol>(
                std::make_shared<AnonFunctionDefinitionNode>(node),
                node.getPos(), anonFuncVarID.getNameField(),
                anonFuncVarID.getVid(), labels,
                paramSymbols, nullptr, curScopeLevel() - 1,
                TypeOfBuiltin::ORDINARY, funcType
            );
            const auto& signature = funcSymbol->hasReturnValue()
                                        ? TypeLabelSymbol::funiTypeSymbol(getUnknownPos(), curScopeLevel())
                                        : TypeLabelSymbol::funcTypeSymbol(getUnknownPos(), curScopeLevel());
            signature->appendLabelDes(paramLabelDes);
            if (funcSymbol->hasReturnValue()) signature->appendLabelDes({funcSymbol->getReturnType()});
            funcSymbol->setSignature(signature);
            return funcSymbol;
        };
        functionSymbol = createFunctionSymbol(FunctionType::ANONYMOUS);
        symbolTable.insert(functionSymbol, false);
        if (!functionSymbol->getReturnType())
        {
            functionSymbol->setReturnType(getBuiltinTypeSymbol(functionSymbol->getPos(), BuiltinType::B_ANY));
        }
        pushNewProcessingSymbol(functionSymbol);
        raCodeBuilder << (functionSymbol->hasReturnValue()
                              ? ri::FUNI(functionSymbol->getRaVal(), paramIdents).toRACode()
                              : ri::FUNC(functionSymbol->getRaVal(), paramIdents).toRACode());
        node.getBodyNode()->acceptVisitor(*this);
        if (!builtin::isBuiltinFunction(functionSymbol->getVal()) && functionSymbol->hasReturnValue() && !functionSymbol
            ->hasReturned())
        {
            if (functionSymbol->getReturnType()->isNot("any"))
            {
                throw RCCCompilerError::semanticError(
                    node.getPosStr(),
                    getCodeLine(node.getPos()),
                    StringVector{
                        "An anonymous function with a return type other than `any` must have a return statement.",
                        "Return type: " + functionSymbol->getReturnType()->toString()
                    },
                    {
                        "Add a return statement with the appropriate return value type",
                        "Or change the function return type to 'any' if appropriate"
                    }
                );
            }
            raCodeBuilder << ri::RET("null");
        }
        raCodeBuilder << ri::END(functionSymbol->getRaVal());
        exitScope(ScopeType::ANONYMOUS);
        popProcessingSymbol();
        pushTemOpVarItemWithRecord(node.getPos(),
                                   functionSymbol->getSignature(),
                                   functionSymbol, true);
        raCodeBuilder << ri::PUT(functionSymbol->getRaVal(), topOpRaVal());
    }

    void CompileVisitor::visitPairExpressionNode(PairExpressionNode& node)
    {
        pass("To visit " + getNodeTypeName(node.getRealType()) + " type node.");
    }

    void CompileVisitor::visitDictionaryExpressionNode(DictionaryExpressionNode& node)
    {
        std::vector<std::string> dictItems{};
        for (const auto& itemNodes = visitParallelNode(node.getBodyNode());
             const auto& itemNode : itemNodes)
        {
            if (itemNode->getRealType() != NodeType::PAIR)
            {
                throw RCCCompilerError::typeMissmatchError(
                    itemNode->getPosStr(),
                    getCodeLine(itemNode->getPos()),
                    "The wrong data type is in the dictionary.",
                    getNodeTypeFormatName(NodeType::PAIR),
                    getNodeTypeFormatName(itemNode->getRealType()),
                    {
                        "Only " + getNodeTypeFormatName(NodeType::PAIR) + " nodes are allowed in dictionaries",
                        "Check the data structure being added to the dictionary"
                    }
                );
            }
            itemNode->acceptVisitor(*this);
            dictItems.push_back(rPopOpItemRaVal());
        }
        pushTemOpVarItemWithRecord(node.getPos());
        raCodeBuilder << ri::TP_SET(TypeLabelSymbol::dictTypeSymbol(
                                        node.getPos(), symbolTable.curScopeLevel())->getRaVal(), topOpRaVal())
            << ri::ITER_APND(dictItems, topOpRaVal());
    }

    void CompileVisitor::visitListExpressionNode(ListExpressionNode& node)
    {
        std::vector<std::string> listItems{};
        for (const auto& itemNodes = visitParallelNode(node.getBodyNode());
             const auto& itemNode : itemNodes)
        {
            itemNode->acceptVisitor(*this);
            listItems.push_back(rPopOpItemRaVal());
        }
        const auto& listType = TypeLabelSymbol::listTypeSymbol(
            node.getPos(), symbolTable.curScopeLevel());
        pushTemOpVarItemWithRecord(node.getPos(), listType);
        raCodeBuilder << ri::TP_SET(listType->getRaVal(),
                                    topOpRaVal())
            << ri::ITER_APND(listItems, topOpRaVal());
    }

    void CompileVisitor::visitBracketExpressionNode(BracketExpressionNode& node)
    {
        Pos errorPos = getUnknownPos();
        try
        {
            for (const auto& itemNodes = visitParallelNode(node.getBodyNode());
                 const auto& itemNode : itemNodes)
            {
                errorPos = itemNode->getPos();
                pushProcessingPos(itemNode->getPos());
                itemNode->acceptVisitor(*this);
                popProcessingPos();
            }
        }
        catch (RCCError& e)
        {
            const auto& errorFilePath = errorPos.getFilepath();
            if (fileRecord.empty())
            {
                fileRecord = errorFilePath;
            }
            e.addTraceInfo(RCCError::makeTraceInfo(
                listJoin(e.getTraceInfo()),
                errorPos.toString(),
                getCodeLine(errorPos),
                node.getPos().toString(),
                getCodeLine(node.getPos())));
            if (fileRecord != errorFilePath)
            {
                fileRecord = errorFilePath;
            }
            throw;
        }
    }

    void CompileVisitor::visitIndexExpressionNode(IndexExpressionNode& node)
    {
        node.getLeftNode()->acceptVisitor(*this);
        const auto& targetItem = rPopOpItem();
        node.getIndexNode()->acceptVisitor(*this);
        const auto& indexItem = rPopOpItem();
        if (const auto& [targetTypeLabel, targetValueType] = getTypesFromOpItem(targetItem);
            targetTypeLabel->is("any"))
        {
            if (targetValueType->is("any") || targetValueType->is("nul"))
            {
                pushTemOpVarItemWithRecord(node.getPos());
                raCodeBuilder
                    << ri::ITER_GET(targetItem.getRaVal(symbolTable),
                                    indexItem.getRaVal(symbolTable), topOpRaVal());
            }
            else if (targetValueType->isIterable())
            {
                pushTemOpVarItemWithRecord(node.getPos());
                raCodeBuilder
                    << ri::ITER_GET(targetItem.getRaVal(symbolTable),
                                    indexItem.getRaVal(symbolTable), topOpRaVal());
            }
            else
            {
                throw RCCCompilerError::typeMissmatchError(
                    node.getPosStr(),
                    getCodeLine(node.getPos()),
                    "The wrong data type is in the index expression.",
                    "iterable type (list, dict, str, series, etc.)",
                    targetValueType->toString(),
                    {
                        "Only iterable types can be used with index expressions",
                        "Check the variable type being indexed"
                    }
                );
            }
        }
        else if (targetTypeLabel->isIterable())
        {
            pushTemOpVarItemWithRecord(node.getPos());
            raCodeBuilder
                << ri::ITER_GET(targetItem.getRaVal(symbolTable),
                                indexItem.getRaVal(symbolTable), topOpRaVal());
        }
        else
        {
            throw RCCCompilerError::typeMissmatchError(
                node.getPosStr(),
                getCodeLine(node.getPos()),
                "The wrong data type is in the index expression.",
                "iterable type (list, dict, str, series, etc.)",
                targetValueType->toString(),
                {
                    "Only iterable types can be used with index expressions",
                    "Check the variable type being indexed"
                }
            );
        }
    }

    void CompileVisitor::visitTryNode(TryNode& node)
    {
        enterScope(ScopeType::TRY);
        raCodeBuilder
        << ri::ATMP();
        node.getTryBody()->acceptVisitor(*this);
        for (const auto& [catchParam,
            catchBody] : node.getCatchBodies())
        {
            enterScope(ScopeType::CATCH);
            catchParam->acceptVisitor(*this);
            const auto& catchParamItem = rPopOpItem();
            VarID catchParamVarID (catchParamItem.getVal(), catchParamItem.getPos().getFileField(),
                curScopeField(), curScopeLevel());
            const auto& labels = processLabelNodes(catchParam->getLabels());
            const auto& catchParamSymbol = std::make_shared<ParameterSymbol>(
                ParamType::PARAM_POSITIONAL, catchParam->getPos(), catchParamVarID.getNameField(),
                catchParamVarID.getVid(), labels, std::nullopt, curScopeLevel());
            symbolTable.insert(catchParamSymbol, false);
            raCodeBuilder
            << ri::DETECT(catchParamSymbol->getTypeLabel()->getRaVal(), catchParamSymbol->getRaVal());
            catchBody->acceptVisitor(*this);
            raCodeBuilder
            << ri::END("DETECT");
            exitScope(ScopeType::CATCH);
        }
        if (const auto& finallyBody = node.getFinallyBody())
        {
            enterScope(ScopeType::FINALLY);
            finallyBody->acceptVisitor(*this);
            exitScope(ScopeType::FINALLY);
        }
        raCodeBuilder
        << ri::END("ATMP");
        exitScope(ScopeType::TRY);
    }

    void CompileVisitor::visitThrowNode(ThrowNode& node)
    {
        node.getThrowExpression()->acceptVisitor(*this);
        const auto& throwExpression = rPopOpItem();
        checkExists(throwExpression);
        raCodeBuilder
        << ri::EXPOSE(raVal(throwExpression));
    }

    void CompileVisitor::visitVariableDefinitionNode(VariableDefinitionNode& node)
    {
        std::vector<OpItem> varIDs{};
        std::vector<std::string> vids{};
        std::vector<std::optional<OpItem>> values{};
        for (const auto& varDef : node.getVarDefs())
        {
            varDef->getNameNode()->acceptVisitor(*this);
            if (const auto& varRefSymbol = topOpItem().getReferencedSymbol();
                varRefSymbol && varRefSymbol->getScopeLevel() == curScopeLevel())
            {
                throw RCCCompilerError::symbolDuplicateError(varDef->getNameNode()->getPosStr(),
                                                             getCodeLine(currentPos()),
                                                             varRefSymbol->toString(), {
                                                                 "Pre-defined pos: " + varRefSymbol->getPos().toString()
                                                             }, {});
            }
            else if (varRefSymbol)
            {
                const auto& anyTypeLabel = getBuiltinTypeSymbol(getUnknownPos(), BuiltinType::B_ANY);
                pushIdentItem({
                                  rPopOpItem().getVal(), varDef->getNameNode()->getPos().getFileField(),
                                  curScopeField(), curScopeLevel()
                              },
                              anyTypeLabel, anyTypeLabel, nullptr);
            }
            auto var = rPopOpItem();
            varIDs.push_back(var);
            vids.push_back(var.getRaVal(symbolTable, false));
            const auto labels = processLabelNodes(varDef->getLabelNodes());
            symbolTable.insert(std::make_shared<VariableSymbol>(
                                   varDef->getNameNode()->getPos(),
                                   var.getVal(),
                                   vids.back(),
                                   labels,
                                   symbolTable.curScopeLevel()), false);
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
        for (size_t i = 0; i < vids.size(); i++)
        {
            const auto& vid = vids[i];
            const auto& [_, symbol] = symbolTable.findVariableSymbolByName(varIDs[i].getVal());
            if (!symbol->getTypeLabel()->isCustomType())
            {
                raCodeBuilder << ri::TP_SET(symbol->getTypeLabel()->getRaVal(), vid);
            }
            if (values[i].has_value())
            {
                const auto& value = values[i].value();
                checkExists(value, value.getPos());
                if (!checkTypeMatch(symbol->getTypeLabel(),
                                    getDefiniteTypeLabelSymbolFromOpItem(value),
                                    false))
                {
                    throw RCCCompilerError::typeMissmatchError(
                        node.getPosStr(),
                        getCodeLine(node.getPos()),
                        StringVector{
                            "The value type is not the same as the variable type.",
                            "Processing variable: " + symbol->toString(),
                            "Processing value: " + value.toString()
                        },
                        symbol->getTypeLabel()->toString(),
                        getDefiniteTypeLabelSymbolFromOpItem(value)->toString(),
                        {
                            "Change the value to match the variable type: " + symbol->getTypeLabel()->toString(),
                            "Or modify the variable declaration to match the value type: " + value.getTypeLabel()->
                            toString()
                        }
                    );
                }
                processTypeAutoChange(symbol, value);
                const auto& raValue = value.getRaVal(symbolTable);
                symbol->setDefaultValue(raValue);
                symbol->setReferencedSymbol(getReferenceTargetSymbol(value));
                raCodeBuilder << ri::PUT(raValue, vid);
            }
        }
    }

    void CompileVisitor::visitAssignmentNode(AssignmentNode& node)
    {
        const auto& [identNode, valueNode] = node.getAssignPair();
        identNode->acceptVisitor(*this);
        const auto& identItem = rPopOpItem();
        if (identNode->getRealType() == NodeType::IDENTIFIER)
        {
            const auto& labels = processLabelNodes(std::static_pointer_cast<IdentifierNode>(identNode)->getLabels());
            if (const auto& [level, identSymbol] = symbolTable.findByNameAndTransform<VariableSymbol>(
                identItem.getVal(), SymbolType::VARIABLE); level >= 0 && identSymbol)
            {
                const auto& typeLabelRecord = identSymbol->getTypeLabel();
                identSymbol->reSetLabels(labels);
                if (!typeLabelRecord->equalWith(identSymbol->getTypeLabel()))
                {
                    raCodeBuilder
                        << ri::TP_SET(identSymbol->getTypeLabel()->getRaVal(), identSymbol->getRaVal());
                }
            }
        }
        node.getAssignPair().second->acceptVisitor(*this);
        if (const auto& [identSymbolLevel, identSymbol]
                = symbolTable.findVariableSymbolByName(identItem.getVal());
            identSymbolLevel >= 0 && identSymbol)
        {
            std::string value{};
            const auto& valueItem = rPopOpItem();
            checkExists(valueItem, node.getAssignPair().second->getPos());
            if (!checkTypeMatch(identSymbol->getTypeLabel(), valueItem.getTypeLabel(), true))
            {
                throw RCCCompilerError::typeMissmatchError(
                    node.getPosStr(),
                    getCodeLine(node.getPos()),
                    StringVector{
                        "The value type is not the same as the variable type.",
                        "Processing variable: " + identSymbol->toString(),
                        "Processing value: " + valueItem.toString()
                    },
                    identSymbol->getTypeLabel()->toString(),
                    getDefiniteTypeLabelSymbolFromOpItem(valueItem)->toString(),
                    {
                        "Change the value to match the variable type: " + identSymbol->getTypeLabel()->toString(),
                        "Or modify the variable declaration to match the value type: " + valueItem.getTypeLabel()->
                        toString()
                    }
                );
            }
            processTypeAutoChange(identSymbol, valueItem);
            if (valueItem.getType() == OpItemType::IDENTIFIER)
            {
                if (const auto& [valueSymbolLevel, valueSymbol]
                        = symbolTable.findVariableSymbolByName(valueItem.getVal());
                    valueSymbolLevel >= 0 && valueSymbol)
                {
                    value = valueSymbol->getRaVal();
                    identSymbol->setReferencedSymbol(valueSymbol);
                }
                else
                {
                    value = valueItem.getRaVal(symbolTable);
                    identSymbol->setReferencedSymbol(getReferenceTargetSymbol(valueItem));
                }
            }
            else if (valueItem.getType() == OpItemType::LITERAL_VALUE)
            {
                value = valueItem.getRaVal(symbolTable);
            }
            if (identItem.getBelonging())
            {
                raCodeBuilder
                    << ri::TP_SET_FIELD(identItem.getBelonging()->getRaVal(),
                                        formatAttrField(identItem.getBelongAttrRaValue()), value);
            }
            else
            {
                raCodeBuilder << ri::PUT(value, identSymbol->getRaVal());
            }
        }
        else
        {
            throw RCCCompilerError::symbolNotFoundError(
                node.getPosStr(),
                getCodeLine(node.getPos()),
                identItem.getVal(),
                "Identifier not found in current scope",
                {
                    "Check if the identifier is spelled correctly",
                    "Ensure the identifier is declared before use",
                    "Verify the identifier is in the current scope"
                }
            );
        }
    }

    IRCCSymbolTableManagerInterface* CompileVisitor::GetSymbolTableManagerI()
    {
        return &getSymbolTable();
    }

    void CompileVisitor::WriteRaCode(const char* raCode)
    {
        raCodeBuilder << raCode;
    }

    const char* CompileVisitor::GetProgramEntryFilePath() const
    {
        return getProgramEntryFilePath().c_str();
    }

    const char* CompileVisitor::GetProgramTargetFilePath() const
    {
        return getProgramTargetFilePath().c_str();
    }

    const char* CompileVisitor::GetCompileOutputFilePath() const
    {
        return getCompileOutputFilePath().c_str();
    }

    const char* CompileVisitor::GetCurrentProcessingFilePath() const
    {
        return getCurrentProcessingFilePath().c_str();
    }

    void CompileVisitor::SetCurrentProcessingFilePath(const char* filepath)
    {
        setCurrentProcessingFilePath(filepath);
    }

    const char* CompileVisitor::CurScopeField() const
    {
        return curScopeField().c_str();
    }

    ScopeType CompileVisitor::CurScopeType() const
    {
        return curScopeType();
    }

    void CompileVisitor::EnterScope(const ast::ScopeType& scopeType)
    {
        enterScope(scopeType);
    }

    void CompileVisitor::EnterScope(const std::size_t& level)
    {
        enterScope(level);
    }

    void CompileVisitor::EnterTopScope()
    {
        enterTopScope();
    }

    void CompileVisitor::EnterGlobalScope()
    {
        enterGlobalScope();
    }

    void CompileVisitor::ExitScope(const ast::ScopeType& scopeType)
    {
        exitScope(scopeType);
    }

    std::size_t CompileVisitor::CurScopeLevel() const
    {
        return curScopeLevel();
    }

    void CompileVisitor::EnterLoopScope()
    {
        enterLoopScope();
    }

    void CompileVisitor::ExitLoopScope()
    {
        exitLoopScope();
    }

    bool CompileVisitor::IsInLoopScope() const
    {
        return isInLoopScope();
    }

    bool CompileVisitor::CheckTypeMatch(const symbol::IRCCSymbolInterface* leftSymbolI,
                                        const IRCCOpItemInterface* rightOpItemI) const
    {
        return checkTypeMatch(std::static_pointer_cast<Symbol>(leftSymbolI->TransformToSI()->copySelf()),
                              *rightOpItemI->TransformToOII());
    }

    void CompileVisitor::ProcessingTypeAutoChange(const symbol::IRCCSymbolInterface* sourceSymbolI,
                                                  const IRCCOpItemInterface* targetOpItemI) const
    {
        return processTypeAutoChange(std::static_pointer_cast<Symbol>(sourceSymbolI->TransformToSI()->copySelf()),
                                     *targetOpItemI->TransformToOII());
    }

    void CompileVisitor::PushNewProcessingSymbol(const symbol::IRCCSymbolInterface* symbolI)
    {
        pushNewProcessingSymbol(std::static_pointer_cast<Symbol>(symbolI->TransformToSI()->copySelf()));
    }

    symbol::IRCCSymbolInterface* CompileVisitor::TopProcessingSymbolI()
    {
        return topProcessingSymbol().get();
    }

    void CompileVisitor::PopProcessingSymbolI()
    {
        popProcessingSymbol();
    }

    IRCCSymbolInterface* CompileVisitor::RPopProcessingSymbolI()
    {
        return rPopProcessingSymbol().get();
    }

    SymbolType CompileVisitor::TopProcessingSymbolType()
    {
        return topProcessingSymbolType();
    }

    const char* CompileVisitor::TopProcessingSymbolVal()
    {
        return topProcessingSymbolVal().c_str();
    }

    const char* CompileVisitor::TopProcessingSymbolRaVal()
    {
        return topProcessingSymbolRaVal().c_str();
    }

    bool CompileVisitor::IsProcessingSymbol() const
    {
        return isProcessingSymbol();
    }

    void CompileVisitor::PushOpItem(const IRCCOpItemInterface* opItemI)
    {
        pushOpItem(std::static_pointer_cast<OpItem>(opItemI->TransformToOII()->copySelf()));
    }

    void CompileVisitor::PushOpItem(const OpItemType& opItemType,
                                    const IRCCTypeLabelSymbolInterface* typeLabelSymbolI, const char* value,
                                    const char* raValue,
                                    const IRCCSymbolInterface* referencedSymbolI,
                                    const IRCCTypeLabelSymbolInterface* valueTypeSymbolI)
    {
        pushOpItem(
            opItemType,
            std::static_pointer_cast<TypeLabelSymbol>(typeLabelSymbolI->TransformToTLSI()->copySelf()),
            value, raValue,
            referencedSymbolI
                ? std::static_pointer_cast<Symbol>(referencedSymbolI->TransformToSI()->copySelf())
                : nullptr,
            valueTypeSymbolI != nullptr
                ? std::static_pointer_cast<TypeLabelSymbol>(valueTypeSymbolI->TransformToTLSI()->copySelf())
                : nullptr);
    }

    void CompileVisitor::PushOpItem(const char* name, const char* fileField, const char* scopeField,
                                    const IRCCTypeLabelSymbolInterface* typeLabelSymbolI)
    {
        pushOpItem(name, fileField, scopeField,
                   std::static_pointer_cast<TypeLabelSymbol>(typeLabelSymbolI->TransformToTLSI()->copySelf()));
    }

    IRCCOpItemInterface* CompileVisitor::RPopOpItemI()
    {
        return new OpItem(rPopOpItem());
    }

    void CompileVisitor::PopOpItem()
    {
        popOpItem();
    }

    IRCCOpItemInterface* CompileVisitor::TopOpItemI() const
    {
        return opItemStack.top().get();
    }

    const char* CompileVisitor::TopOpRaVal() const
    {
        return CreateCharP(topOpRaVal());
    }

    const char* CompileVisitor::RPopOpItemRaVal()
    {
        return rPopOpItemRaVal().c_str();
    }

    const char* CompileVisitor::RPopOpItemVal()
    {
        return rPopOpItemVal().c_str();
    }

    bool CompileVisitor::HasNextOpItem() const
    {
        return hasNextOpItem();
    }

    void CompileVisitor::PushTemOpVarItemWithRecord(const IRCCPosInterface* posI,
                                                    const symbol::IRCCTypeLabelSymbolInterface*
                                                    valueTypeSymbolI,
                                                    const symbol::IRCCSymbolInterface*
                                                    referencedSymbolI, const bool& sysDefined,
                                                    const symbol::IRCCTypeLabelSymbolInterface*
                                                    typeLabelSymbolI)
    {
        pushTemOpVarItemWithRecord(*posI->TransformToPI(),
                                   typeLabelSymbolI
                                       ? std::static_pointer_cast<TypeLabelSymbol>(
                                           valueTypeSymbolI->TransformToTLSI()->copySelf())
                                       : nullptr,
                                   referencedSymbolI
                                       ? std::static_pointer_cast<Symbol>(
                                           referencedSymbolI->TransformToSI()->copySelf())
                                       : nullptr,
                                   sysDefined, typeLabelSymbolI
                                                   ? std::static_pointer_cast<TypeLabelSymbol>(
                                                       typeLabelSymbolI->TransformToTLSI()->copySelf())
                                                   : nullptr);
    }

    IRCCOpItemInterface* CompileVisitor::PushTemOpSetItem(const IRCCPosInterface* posI)
    {
        return new OpItem(pushTemOpSetItem(*posI->TransformToPI()));
    }

    IRCCOpItemInterface* CompileVisitor::NewTemOpSetItem(const IRCCPosInterface* posI) const
    {
        return new OpItem(newTemOpSetItem(*posI->TransformToPI()));
    }

    const char* CompileVisitor::GetThisFieldRaVal(const IRCCPosInterface* posI) const
    {
        return getThisFieldVarID(*posI->TransformToPI()).getVid().c_str();
    }

    IRCCVariableSymbolInterface* CompileVisitor::GetThisFieldSymbol(
        const IRCCClassSymbolInterface* classSymbolI) const
    {
        return getThisFieldSymbol(std::static_pointer_cast<ClassSymbol>(classSymbolI->TransformToCSI()->copySelf())).
            get();
    }
}

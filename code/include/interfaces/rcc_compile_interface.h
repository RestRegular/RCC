//
// Created by RestRegular on 2025/10/22.
//

#ifndef RCC_RCC_COMPILE_INTERFACE_H
#define RCC_RCC_COMPILE_INTERFACE_H

#include <cstddef>

#include "rcc_interface_dec.h"

namespace ast
{
    enum class OpItemType: int {
        IDENTIFIER, // ��ʶ��
        LITERAL_VALUE, // ������ֵ
        SET_LABEL // ���ϱ�ǩ
    };

    enum class ScopeType
    {
        TEMPORARY = 0, // ��ʱ������
        GLOBAL, // ȫ��������
        PROGRAM, // ����������
        EXPRESSION, // ���ʽ������
        VAR_DEF_EXPRESSION, // ����������ʽ������
        CLASS, // ��������
        FUNCTION, // ����������
        ANONYMOUS, // ����������
        LOOP, // ѭ��������
        CONDITION // ����������
    };

    enum class BuiltinType
    {
        B_ANY = 0, // ��������
        B_BOOL, // ��������
        B_CHAR, // �ַ�����
        B_DICT, // �ֵ�����
        B_FLOAT, // ��������
        B_INT, // ��������
        B_LIST, // �б�����
        B_STR, // �ַ�������
        B_VOID, // ������
        B_NUL, // ��ֵ����
        B_FLAG, // ��־����
        B_FUNC, // �շ��غ�������
        B_FUNI, // ʵ���غ�������
        B_CLAS, // �Զ���������
    };

    class OpItem;

    class IRCCOpItemInterface
    {
    public:
        virtual ~IRCCOpItemInterface() SYS_INTERFACE_CALL;
        static void SYS_INTERFACE_CALL FreeOpItem(OpItem*& opItem);
        virtual bool SYS_INTERFACE_CALL Is(const OpItemType& opItemType) = 0;
        virtual bool SYS_INTERFACE_CALL IsNot(const OpItemType& opItemType) = 0;
        virtual const char* SYS_INTERFACE_CALL ToString() const = 0;
        virtual const char* SYS_INTERFACE_CALL GetVal() const = 0;
        virtual const char* SYS_INTERFACE_CALL GetRaVal(const symbol::IRCCSymbolTableManagerInterface* table, const bool &needSearch) const = 0;
        virtual OpItemType SYS_INTERFACE_CALL GetType() const = 0;
        virtual symbol::IRCCTypeLabelSymbolInterface* SYS_INTERFACE_CALL GetTypeLabelSymbolI() const = 0;
        virtual symbol::IRCCTypeLabelSymbolInterface* SYS_INTERFACE_CALL GetValueTypeSymbolI() const = 0;
        virtual symbol::IRCCSymbolInterface* SYS_INTERFACE_CALL GetBelongingSymbolI() const = 0;
        virtual const char* SYS_INTERFACE_CALL GetBelongAttrRaValue() const = 0;
        virtual symbol::IRCCSymbolInterface* SYS_INTERFACE_CALL GetReferencedSymbol() const = 0;
        virtual void SYS_INTERFACE_CALL SetTypeLabel(const symbol::IRCCTypeLabelSymbolInterface* typeLabelSymbolI) = 0;
        virtual void SYS_INTERFACE_CALL SetValueType(const symbol::IRCCTypeLabelSymbolInterface* valueTypeSymbolI) = 0;
        virtual void SYS_INTERFACE_CALL SetBelonging(const symbol::IRCCSymbolInterface* belongingSymbolI,
            const char* belongAttrRaVal) = 0;
        virtual void SYS_INTERFACE_CALL SetReferencedSymbol(const symbol::IRCCSymbolInterface* symbolI) = 0;
        virtual const OpItem* TransformToOII() const = 0;
    };

    class IRCCCompileInterface
    {
    public:
        virtual ~IRCCCompileInterface() SYS_INTERFACE_CALL = 0;

        // ���ܽӿ�����
        virtual symbol::IRCCSymbolTableManagerInterface* SYS_INTERFACE_CALL GetSymbolTableManagerI() = 0;
        virtual void SYS_INTERFACE_CALL WriteRaCode(const char* raCode) = 0;
        virtual const char* SYS_INTERFACE_CALL GetProgramEntryFilePath() const = 0;
        virtual const char* SYS_INTERFACE_CALL GetProgramTargetFilePath() const = 0;
        virtual const char* SYS_INTERFACE_CALL GetCompileOutputFilePath() const = 0;
        virtual const char* SYS_INTERFACE_CALL GetCurrentProcessingFilePath() const = 0;
        virtual void SYS_INTERFACE_CALL SetCurrentProcessingFilePath(const char* filepath) = 0;
        static const char* SYS_INTERFACE_CALL ScopeTypeToString(const ast::ScopeType& scopeType);
        static const char* SYS_INTERFACE_CALL ScopeTypeToFormatString(const ast::ScopeType& scopeType);
        virtual const char* SYS_INTERFACE_CALL CurScopeField() const = 0;
        virtual ScopeType SYS_INTERFACE_CALL CurScopeType() const = 0;
        virtual void SYS_INTERFACE_CALL EnterScope(const ast::ScopeType &scopeType) = 0;
        virtual void SYS_INTERFACE_CALL EnterScope(const std::size_t &level) = 0;
        virtual void SYS_INTERFACE_CALL EnterTopScope() = 0;
        virtual void SYS_INTERFACE_CALL EnterGlobalScope() = 0;
        virtual void SYS_INTERFACE_CALL ExitScope(const ast::ScopeType& scopeType) = 0;
        virtual std::size_t SYS_INTERFACE_CALL CurScopeLevel() const = 0;
        virtual void SYS_INTERFACE_CALL EnterLoopScope() = 0;
        virtual void SYS_INTERFACE_CALL ExitLoopScope() = 0;
        virtual bool SYS_INTERFACE_CALL IsInLoopScope() const = 0;
        static bool SYS_INTERFACE_CALL CheckTypeMatch(
            const symbol::IRCCTypeLabelSymbolInterface* leftTypeSymbolI,
            const symbol::IRCCTypeLabelSymbolInterface* rightTypeSymbolI,
            const bool& restrict = true);
        static utils::IRCCPosInterface* SYS_INTERFACE_CALL CurrentPos();
        static void SYS_INTERFACE_CALL SetCurrentPos(const utils::IRCCPosInterface* pos);
        static void SYS_INTERFACE_CALL ResetCurrentPos();
        virtual bool SYS_INTERFACE_CALL CheckTypeMatch(
            const symbol::IRCCSymbolInterface* leftSymbolI,
            const IRCCOpItemInterface* rightOpItemI) const = 0;
        virtual void SYS_INTERFACE_CALL ProcessingTypeAutoChange(
            const symbol::IRCCSymbolInterface* sourceSymbolI,
            const IRCCOpItemInterface* targetOpItemI) const = 0;
        virtual void SYS_INTERFACE_CALL PushNewProcessingSymbol(const symbol::IRCCSymbolInterface* symbol) = 0;
        virtual symbol::IRCCSymbolInterface* SYS_INTERFACE_CALL TopProcessingSymbolI() = 0;
        virtual void SYS_INTERFACE_CALL PopProcessingSymbolI() = 0;
        virtual symbol::IRCCSymbolInterface* SYS_INTERFACE_CALL RPopProcessingSymbolI() = 0;
        virtual symbol::SymbolType SYS_INTERFACE_CALL TopProcessingSymbolType() = 0;
        virtual const char* SYS_INTERFACE_CALL TopProcessingSymbolVal() = 0;
        virtual const char* SYS_INTERFACE_CALL TopProcessingSymbolRaVal() = 0;
        virtual bool SYS_INTERFACE_CALL IsProcessingSymbol() const = 0;

        virtual void SYS_INTERFACE_CALL PushOpItem(const IRCCOpItemInterface* opItemI) = 0;
        virtual void SYS_INTERFACE_CALL PushOpItem(
            const OpItemType &opItemType,
            const symbol::IRCCTypeLabelSymbolInterface* typeLabelSymbol,
            const char* value,
            const char* raValue,
            const symbol::IRCCSymbolInterface* referencedSymbolI,
            const symbol::IRCCTypeLabelSymbolInterface* valueTypeSymbolI) = 0;
        virtual void SYS_INTERFACE_CALL PushOpItem(
            const char* name,
            const char* fileField,
            const char* scopeField,
            const symbol::IRCCTypeLabelSymbolInterface* typeLabelSymbolI) = 0;
        virtual IRCCOpItemInterface* SYS_INTERFACE_CALL RPopOpItemI() = 0;
        virtual void SYS_INTERFACE_CALL PopOpItem() = 0;
        virtual IRCCOpItemInterface* SYS_INTERFACE_CALL TopOpItemI() const = 0;
        virtual const char* SYS_INTERFACE_CALL TopOpRaVal() const = 0;
        virtual const char* SYS_INTERFACE_CALL RPopOpItemRaVal() = 0;
        virtual const char* SYS_INTERFACE_CALL RPopOpItemVal() = 0;
        virtual bool SYS_INTERFACE_CALL HasNextOpItem() const = 0;

        static const char* SYS_INTERFACE_CALL GetNewTempVarName();
        static const char* SYS_INTERFACE_CALL GetNewSetLabelName();
        virtual IRCCOpItemInterface* SYS_INTERFACE_CALL PushTemOpSetItemWithRecord(
            const utils::IRCCPosInterface* posI,
            const symbol::IRCCTypeLabelSymbolInterface* valueTypeSymbolI,
            const symbol::IRCCSymbolInterface* referencedSymbolI,
            const bool &sysDefined,
            const symbol::IRCCTypeLabelSymbolInterface* typeLabelSymbolI) = 0;
        virtual IRCCOpItemInterface* SYS_INTERFACE_CALL PushTemOpSetItem(const utils::IRCCPosInterface* posI) = 0;
        virtual IRCCOpItemInterface* SYS_INTERFACE_CALL NewTemOpSetItem(const utils::IRCCPosInterface* posI) const = 0;
        virtual const char* SYS_INTERFACE_CALL GetThisFieldRaVal(const utils::IRCCPosInterface* posI) const = 0;
        virtual symbol::IRCCVariableSymbolInterface* SYS_INTERFACE_CALL GetThisFieldSymbol(
            const symbol::IRCCClassSymbolInterface* classSymbolI) const = 0;
    };

}

#endif //RCC_RCC_COMPILE_INTERFACE_H

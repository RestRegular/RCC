//
// Created by RestRegular on 2025/9/30.
//

#include <utility>

#include "../../include/builtin/rcc_builtin_core.h"

#include <cstring>

namespace builtin
{
    // ���������������ַ������·����char*������C����ַ���ת����
    char* copyString(const std::string& str)
    {
        const auto c_str = new char[str.size() + 1];
        std::strcpy(c_str, str.c_str());
        return c_str;
    }

    // �����������ͷ�IRCCStringArray�е��ַ�������
    void freeStringArray(IRCCStringArray& arr)
    {
        for (size_t i = 0; i < arr.size; ++i)
        {
            delete[] arr.stringArray[i];
        }
        delete[] arr.stringArray;
        arr.stringArray = nullptr;
        arr.size = 0;
    }

    // �����������ͷ�IRCCStringPair����
    void freeStringPairs(const IRCCStringPair* pairs, const size_t size)
    {
        for (size_t i = 0; i < size; ++i)
        {
            delete[] pairs[i].first;
            delete[] pairs[i].second;
        }
        delete[] pairs;
    }

    // �����������ͷ�IRCCStringOpItemMap�е�����
    void freeStringOpItemMap(IRCCStringOpItemMap& map)
    {
        for (size_t i = 0; i < map.size; ++i)
        {
            delete[] map.nameOpItemPair[i].name;
            // ע�⣺opItemI��ԭʼast::OpItemת�����������������ڴ��ͷţ���ԭ�������
        }
        delete[] map.nameOpItemPair;
        map.nameOpItemPair = nullptr;
        map.size = 0;
    }

    // �ͷ�IRCCCallInfos�����ڲ����ж�̬�������Դ
    void freeCallInfosI(IRCCCallInfos* pCallInfos)
    {
        if (!pCallInfos) return;

        // 1. �ͷ�processedArgs
        if (pCallInfos->processedArgs.stringArray)
        {
            for (size_t i = 0; i < pCallInfos->processedArgs.size; ++i)
            {
                delete[] pCallInfos->processedArgs.stringArray[i];
            }
            delete[] pCallInfos->processedArgs.stringArray;
            pCallInfos->processedArgs.stringArray = nullptr;
            pCallInfos->processedArgs.size = 0;
        }

        // 2. �ͷ�originalArgs
        if (pCallInfos->originalArgs.pairs)
        {
            for (size_t i = 0; i < pCallInfos->originalArgs.size; ++i)
            {
                delete[] pCallInfos->originalArgs.pairs[i].first;
                delete[] pCallInfos->originalArgs.pairs[i].second;
            }
            delete[] pCallInfos->originalArgs.pairs;
            pCallInfos->originalArgs.pairs = nullptr;
            pCallInfos->originalArgs.size = 0;
        }

        // 3. �ͷ�posArgOpItems������ӿ��ͷţ�
        if (pCallInfos->posArgOpItems.opItemIArray)
        {
            for (size_t i = 0; i < pCallInfos->posArgOpItems.size; ++i)
            {
                delete pCallInfos->posArgOpItems.opItemIArray[i];
            }
            delete[] pCallInfos->posArgOpItems.opItemIArray;
            pCallInfos->posArgOpItems.opItemIArray = nullptr;
            pCallInfos->posArgOpItems.size = 0;
        }

        // 4. �ͷ�namedArgOpItems������ӿ��ͷţ�
        if (pCallInfos->namedArgOpItems.nameOpItemPair)
        {
            for (size_t i = 0; i < pCallInfos->namedArgOpItems.size; ++i)
            {
                delete[] pCallInfos->namedArgOpItems.nameOpItemPair[i].name;
                delete pCallInfos->namedArgOpItems.nameOpItemPair[i].opItemI;
            }
            delete[] pCallInfos->namedArgOpItems.nameOpItemPair;
            pCallInfos->namedArgOpItems.nameOpItemPair = nullptr;
            pCallInfos->namedArgOpItems.size = 0;
        }

        // 5. �ͷ�callPosI
        delete pCallInfos->callPosI;
        pCallInfos->callPosI = nullptr;

        // 6. �ͷ�orderedArgOpItems������ӿ��ͷţ�
        if (pCallInfos->orderedArgOpItems.nameOpItemPair)
        {
            for (size_t i = 0; i < pCallInfos->orderedArgOpItems.size; ++i)
            {
                delete[] pCallInfos->orderedArgOpItems.nameOpItemPair[i].name;
                delete pCallInfos->orderedArgOpItems.nameOpItemPair[i].opItemI;
            }
            delete[] pCallInfos->orderedArgOpItems.nameOpItemPair;
            pCallInfos->orderedArgOpItems.nameOpItemPair = nullptr;
            pCallInfos->orderedArgOpItems.size = 0;
        }

        delete pCallInfos;
    }

    // CallInfos -> IRCCCallInfos��C++����תC���ݽṹ��
    IRCCCallInfos* transformCallInfos(const CallInfos& callInfos)
    {
        // ����IRCCCallInfos�ṹ��
        const auto irccCallInfos = new(std::nothrow) IRCCCallInfos;
        if (!irccCallInfos) return nullptr;

        try
        {
            // 1. ת��processedArgs��std::vector<std::string> -> IRCCStringArray��
            const size_t processedArgsSize = callInfos.processedArgs.size();
            irccCallInfos->processedArgs.stringArray = new char*[processedArgsSize];
            irccCallInfos->processedArgs.size = processedArgsSize;
            for (size_t i = 0; i < processedArgsSize; ++i)
            {
                irccCallInfos->processedArgs.stringArray[i] = copyString(callInfos.processedArgs[i]);
            }

            // 2. ת��originalArgs��vector<pair<string,string>> -> �����ṹ�壩
            const size_t originalArgsSize = callInfos.originalArgs.size();
            irccCallInfos->originalArgs.pairs = new IRCCStringPair[originalArgsSize];
            irccCallInfos->originalArgs.size = originalArgsSize;
            for (size_t i = 0; i < originalArgsSize; ++i)
            {
                irccCallInfos->originalArgs.pairs[i].first = copyString(callInfos.originalArgs[i].first);
                irccCallInfos->originalArgs.pairs[i].second = copyString(callInfos.originalArgs[i].second);
            }

            // 3. ת��posArgOpItems��queue<ast::OpItem> -> IRCCOpItemArray��
            // �Ƚ�queueתΪvector�������������ʣ�
            std::vector<ast::OpItem> posArgs;
            auto tempQueue = callInfos.posArgOpItems;
            posArgs.reserve(tempQueue.size());
            while (!tempQueue.empty())
            {
                posArgs.push_back(tempQueue.front());
                tempQueue.pop();
            }
            irccCallInfos->posArgOpItems.size = posArgs.size();
            irccCallInfos->posArgOpItems.opItemIArray = new ast::IRCCOpItemInterface*[posArgs.size()];
            for (size_t i = 0; i < posArgs.size(); ++i)
            {
                // ����ast::OpItem�з���ת��ΪIRCCOpItemInterface*���˴�Ϊʾ����
                irccCallInfos->posArgOpItems.opItemIArray[i] = new ast::OpItem(posArgs[i]);
                // ע�⣺ʵ�ʳ���������Ҫ����ӿ�ָ����������ڣ������ü�����
            }

            // 4. ת��namedArgOpItems��unordered_map -> IRCCStringOpItemMap��
            const size_t namedArgsSize = callInfos.namedArgOpItems.size();
            irccCallInfos->namedArgOpItems.size = namedArgsSize;
            irccCallInfos->namedArgOpItems.nameOpItemPair = new IRCCStringOpItemPair[namedArgsSize];
            size_t idx = 0;
            for (const auto& [name, opItem] : callInfos.namedArgOpItems)
            {
                irccCallInfos->namedArgOpItems.nameOpItemPair[idx].name = copyString(name);
                irccCallInfos->namedArgOpItems.nameOpItemPair[idx].opItemI = new ast::OpItem(opItem);
                ++idx;
            }

            // 5. ת��callPos��utils::Pos -> utils::IRCCPosInterface*��
            // ����utils::Pos�з���ת��Ϊ�ӿ�ָ��
            irccCallInfos->callPosI = new utils::Pos(callInfos.callPos);

            // 6. ת��orderedArgOpItems��vector<pair<string, OpItem>> -> IRCCStringOpItemMap��
            const size_t orderedArgsSize = callInfos.orderedArgOpItems.size();
            irccCallInfos->orderedArgOpItems.size = orderedArgsSize;
            irccCallInfos->orderedArgOpItems.nameOpItemPair = new IRCCStringOpItemPair[orderedArgsSize];
            for (size_t i = 0; i < orderedArgsSize; ++i)
            {
                irccCallInfos->orderedArgOpItems.nameOpItemPair[i].name = copyString(
                    callInfos.orderedArgOpItems[i].first);
                irccCallInfos->orderedArgOpItems.nameOpItemPair[i].opItemI = new ast::OpItem(
                    callInfos.orderedArgOpItems[i].second);
            }
        }
        catch (...)
        {
            // ת��ʧ��ʱ�ͷ��ѷ�����ڴ�
            freeStringArray(irccCallInfos->processedArgs);
            freeStringPairs(irccCallInfos->originalArgs.pairs, irccCallInfos->originalArgs.size);
            delete[] irccCallInfos->posArgOpItems.opItemIArray;
            freeStringOpItemMap(irccCallInfos->namedArgOpItems);
            freeStringOpItemMap(irccCallInfos->orderedArgOpItems);
            delete irccCallInfos;
            return nullptr;
        }

        return irccCallInfos;
    }


    // IRCCCallInfos* -> CallInfos��C���ݽṹתC++����
    CallInfos transformCallInfos(IRCCCallInfos* pCallInfosI)
    {
        CallInfos callInfos = {};
        if (!pCallInfosI) return callInfos; // ��ָ�뷵��Ĭ�϶���

        // 1. ת��processedArgs��IRCCStringArray -> vector<string>��
        for (size_t i = 0; i < pCallInfosI->processedArgs.size; ++i)
        {
            if (pCallInfosI->processedArgs.stringArray[i])
            {
                callInfos.processedArgs.emplace_back(pCallInfosI->processedArgs.stringArray[i]);
            }
        }

        // 2. ת��originalArgs�������ṹ�� -> vector<pair<string,string>>��
        for (size_t i = 0; i < pCallInfosI->originalArgs.size; ++i)
        {
            const IRCCStringPair& pair = pCallInfosI->originalArgs.pairs[i];
            std::string first = pair.first ? pair.first : "";
            std::string second = pair.second ? pair.second : "";
            callInfos.originalArgs.emplace_back(first, second);
        }

        // 3. ת��posArgOpItems��IRCCOpItemArray -> queue<ast::OpItem>��
        for (size_t i = 0; i < pCallInfosI->posArgOpItems.size; ++i)
        {
            if (const ast::IRCCOpItemInterface* opItemI = pCallInfosI->posArgOpItems.opItemIArray[i])
            {
                // ����ӿ��з���ת����ast::OpItem
                callInfos.posArgOpItems.push(*opItemI->TransformToOII());
            }
        }

        // 4. ת��namedArgOpItems��IRCCStringOpItemMap -> unordered_map��
        for (size_t i = 0; i < pCallInfosI->namedArgOpItems.size; ++i)
        {
            if (const auto& [name, opItemI] = pCallInfosI->namedArgOpItems.nameOpItemPair[i];
                name && opItemI)
            {
                callInfos.namedArgOpItems.emplace(
                    name,
                    *opItemI->TransformToOII()
                );
            }
        }

        // 5. ת��callPos��IRCCPosInterface* -> utils::Pos��
        if (pCallInfosI->callPosI)
        {
            callInfos.callPos = *pCallInfosI->callPosI->TransformToPI();
        }

        // 6. ת��orderedArgOpItems��IRCCStringOpItemMap -> vector<pair<string, OpItem>>��
        for (size_t i = 0; i < pCallInfosI->orderedArgOpItems.size; ++i)
        {
            const IRCCStringOpItemPair& pair = pCallInfosI->orderedArgOpItems.nameOpItemPair[i];
            if (pair.name && pair.opItemI)
            {
                callInfos.orderedArgOpItems.emplace_back(
                    pair.name,
                    *pair.opItemI->TransformToOII()
                );
            }
        }

        callInfos.callNode = nullptr;

        return callInfos;
    }

    PureBuiltinFunction::PureBuiltinFunction(const bool& hasReturnValue, std::string name,
                                             const std::vector<std::shared_ptr<symbol::ParameterSymbol>>& params,
                                             BuiltinFunc call)
        : _hasReturnValue(hasReturnValue), _name(std::move(name)), _params(params), _call(std::move(call))
    {
        signature = hasReturnValue
                        ? symbol::TypeLabelSymbol::funiTypeSymbol(utils::getUnknownPos(), 0)
                        : symbol::TypeLabelSymbol::funcTypeSymbol(utils::getUnknownPos(), 0);
        std::vector<std::shared_ptr<symbol::LabelSymbol>> labelDes{};
        for (const auto& param : params)
        {
            labelDes.push_back(param->getTypeLabel());
        }
        signature->appendLastLabelDes(labelDes);
    }

    bool PureBuiltinFunction::hasReturnValue() const
    {
        return _hasReturnValue;
    }

    std::string PureBuiltinFunction::getName() const
    {
        return _name;
    }

    std::shared_ptr<symbol::TypeLabelSymbol> PureBuiltinFunction::getSignature() const
    {
        return signature;
    }

    std::vector<std::shared_ptr<symbol::ParameterSymbol>> PureBuiltinFunction::getParams() const
    {
        return _params;
    }

    BuiltinFuncRetType PureBuiltinFunction::call(ast::CompileVisitor& visitor,
                                                 const CallInfos& callInfos) const
    {
        return _call(visitor, callInfos);
    }

    const auto& globalScopeField = ast::CompileVisitor::scopeTypeToString(ast::ScopeType::GLOBAL);

    ast::VarID getParamVarId(const std::string& argName)
    {
        return {argName, utils::getUnknownPos().getFileField(), globalScopeField, 0};
    }

    std::shared_ptr<symbol::ParameterSymbol> getPosVarArgsParam(const std::string& argName)
    {
        const auto& varId = getParamVarId(argName);
        return std::make_shared<symbol::ParameterSymbol>(
            symbol::ParamType::PARAM_VAR_LEN_POSITIONAL, utils::getUnknownPos(),
            varId.getNameField(), varId.getVid(),
            std::unordered_set<std::shared_ptr<symbol::LabelSymbol>>{},
            std::nullopt, 0,
            symbol::SymbolType::PARAMETER, nullptr);
    }

    std::shared_ptr<symbol::ParameterSymbol> getKeywordVarArgsParam(const std::string& argName)
    {
        const auto& varId = getParamVarId(argName);
        return std::make_shared<symbol::ParameterSymbol>(
            symbol::ParamType::PARAM_VAR_LEN_KEYWORD, utils::getUnknownPos(),
            varId.getNameField(), varId.getVid(),
            std::unordered_set<std::shared_ptr<symbol::LabelSymbol>>{},
            std::nullopt, 0,
            symbol::SymbolType::PARAMETER, nullptr);
    }

    std::shared_ptr<symbol::ParameterSymbol> getPosArgParam(const std::string& argName)
    {
        const auto& varId = getParamVarId(argName);
        return std::make_shared<symbol::ParameterSymbol>(
            symbol::ParamType::PARAM_POSITIONAL, utils::getUnknownPos(),
            varId.getNameField(), varId.getVid(),
            std::unordered_set<std::shared_ptr<symbol::LabelSymbol>>{},
            std::nullopt, 0,
            symbol::SymbolType::PARAMETER, nullptr);
    }

    std::shared_ptr<symbol::ParameterSymbol> getKeywordArgParam(const std::string& argName,
                                                                const std::string& defaultValue)
    {
        const auto& varId = getParamVarId(argName);
        return std::make_shared<symbol::ParameterSymbol>(
            symbol::ParamType::PARAM_KEYWORD, utils::getUnknownPos(),
            varId.getNameField(), varId.getVid(),
            std::unordered_set<std::shared_ptr<symbol::LabelSymbol>>{},
            defaultValue, 0,
            symbol::SymbolType::PARAMETER, nullptr);
    }
}

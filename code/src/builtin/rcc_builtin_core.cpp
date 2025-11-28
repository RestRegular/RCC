//
// Created by RestRegular on 2025/9/30.
//

#include <utility>

#include "../../include/builtin/rcc_builtin_core.h"

#include <cstring>

namespace builtin
{
    // 辅助函数：拷贝字符串到新分配的char*（用于C风格字符串转换）
    char* copyString(const std::string& str)
    {
        const auto c_str = new char[str.size() + 1];
        std::strcpy(c_str, str.c_str());
        return c_str;
    }

    // 辅助函数：释放IRCCStringArray中的字符串数组
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

    // 辅助函数：释放IRCCStringPair数组
    void freeStringPairs(const IRCCStringPair* pairs, const size_t size)
    {
        for (size_t i = 0; i < size; ++i)
        {
            delete[] pairs[i].first;
            delete[] pairs[i].second;
        }
        delete[] pairs;
    }

    // 辅助函数：释放IRCCStringOpItemMap中的数据
    void freeStringOpItemMap(IRCCStringOpItemMap& map)
    {
        for (size_t i = 0; i < map.size; ++i)
        {
            delete[] map.nameOpItemPairArray[i].name;
            // 注意：opItemI由原始ast::OpItem转换而来，假设无需在此释放（由原对象管理）
        }
        delete[] map.nameOpItemPairArray;
        map.nameOpItemPairArray = nullptr;
        map.size = 0;
    }

    // 释放IRCCCallInfos及其内部所有动态分配的资源
    void freeCallInfosI(IRCCCallInfos* pCallInfos)
    {
        if (!pCallInfos) return;

        // 1. 释放processedArgs
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

        // 2. 释放originalArgs
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

        // 3. 释放posArgOpItems（补充接口释放）
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

        // 4. 释放namedArgOpItems（补充接口释放）
        if (pCallInfos->namedArgOpItems.nameOpItemPairArray)
        {
            for (size_t i = 0; i < pCallInfos->namedArgOpItems.size; ++i)
            {
                delete[] pCallInfos->namedArgOpItems.nameOpItemPairArray[i].name;
                delete pCallInfos->namedArgOpItems.nameOpItemPairArray[i].opItemI;
            }
            delete[] pCallInfos->namedArgOpItems.nameOpItemPairArray;
            pCallInfos->namedArgOpItems.nameOpItemPairArray = nullptr;
            pCallInfos->namedArgOpItems.size = 0;
        }

        // 5. 释放callPosI
        delete pCallInfos->callPosI;
        pCallInfos->callPosI = nullptr;

        // 6. 释放orderedArgOpItems（补充接口释放）
        if (pCallInfos->orderedArgOpItems.nameOpItemPairArray)
        {
            for (size_t i = 0; i < pCallInfos->orderedArgOpItems.size; ++i)
            {
                delete[] pCallInfos->orderedArgOpItems.nameOpItemPairArray[i].name;
                delete pCallInfos->orderedArgOpItems.nameOpItemPairArray[i].opItemI;
            }
            delete[] pCallInfos->orderedArgOpItems.nameOpItemPairArray;
            pCallInfos->orderedArgOpItems.nameOpItemPairArray = nullptr;
            pCallInfos->orderedArgOpItems.size = 0;
        }

        delete pCallInfos;
    }

    // CallInfos -> IRCCCallInfos（C++对象转C兼容结构）
    IRCCCallInfos* transformCallInfos(ast::IRCCCompileInterface* compileVisitorI, const CallInfos& callInfos)
    {
        // 分配IRCCCallInfos结构体
        const auto irccCallInfos = new(std::nothrow) IRCCCallInfos;
        if (!irccCallInfos) return nullptr;

        try
        {
            irccCallInfos->compileVisitorI = compileVisitorI;

            // 1. 转换processedArgs（std::vector<std::string> -> IRCCStringArray）
            const size_t processedArgsSize = callInfos.processedArgs.size();
            irccCallInfos->processedArgs.stringArray = new char*[processedArgsSize];
            irccCallInfos->processedArgs.size = processedArgsSize;
            for (size_t i = 0; i < processedArgsSize; ++i)
            {
                irccCallInfos->processedArgs.stringArray[i] = copyString(callInfos.processedArgs[i]);
            }

            // 2. 转换originalArgs（vector<pair<string,string>> -> 匿名结构体）
            const size_t originalArgsSize = callInfos.originalArgs.size();
            irccCallInfos->originalArgs.pairs = new IRCCStringPair[originalArgsSize];
            irccCallInfos->originalArgs.size = originalArgsSize;
            for (size_t i = 0; i < originalArgsSize; ++i)
            {
                irccCallInfos->originalArgs.pairs[i].first = copyString(callInfos.originalArgs[i].first);
                irccCallInfos->originalArgs.pairs[i].second = copyString(callInfos.originalArgs[i].second);
            }

            // 3. 转换posArgOpItems（queue<ast::OpItem> -> IRCCOpItemArray）
            // 先将queue转为vector（便于索引访问）
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
                // 假设ast::OpItem有方法转换为IRCCOpItemInterface*（此处为示例）
                irccCallInfos->posArgOpItems.opItemIArray[i] = new ast::OpItem(posArgs[i]);
                // 注意：实际场景可能需要管理接口指针的生命周期（如引用计数）
            }

            // 4. 转换namedArgOpItems（unordered_map -> IRCCStringOpItemMap）
            const size_t namedArgsSize = callInfos.namedArgOpItems.size();
            irccCallInfos->namedArgOpItems.size = namedArgsSize;
            irccCallInfos->namedArgOpItems.nameOpItemPairArray = new IRCCStringOpItemPair[namedArgsSize];
            size_t idx = 0;
            for (const auto& [name, opItem] : callInfos.namedArgOpItems)
            {
                irccCallInfos->namedArgOpItems.nameOpItemPairArray[idx].name = copyString(name);
                irccCallInfos->namedArgOpItems.nameOpItemPairArray[idx].opItemI = new ast::OpItem(opItem);
                ++idx;
            }

            // 5. 转换callPos（utils::Pos -> utils::IRCCPosInterface*）
            // 假设utils::Pos有方法转换为接口指针
            irccCallInfos->callPosI = new utils::Pos(callInfos.callPos);

            // 6. 转换orderedArgOpItems（vector<pair<string, OpItem>> -> IRCCStringOpItemMap）
            const size_t orderedArgsSize = callInfos.orderedArgOpItems.size();
            irccCallInfos->orderedArgOpItems.size = orderedArgsSize;
            irccCallInfos->orderedArgOpItems.nameOpItemPairArray = new IRCCStringOpItemPair[orderedArgsSize];
            for (size_t i = 0; i < orderedArgsSize; ++i)
            {
                irccCallInfos->orderedArgOpItems.nameOpItemPairArray[i].name = copyString(
                    callInfos.orderedArgOpItems[i].first);
                irccCallInfos->orderedArgOpItems.nameOpItemPairArray[i].opItemI = new ast::OpItem(
                    callInfos.orderedArgOpItems[i].second);
            }
        }
        catch (...)
        {
            // 转换失败时释放已分配的内存
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


    // IRCCCallInfos* -> CallInfos（C兼容结构转C++对象）
    CallInfos transformCallInfos(IRCCCallInfos* pCallInfosI)
    {
        CallInfos callInfos = {};
        if (!pCallInfosI) return callInfos; // 空指针返回默认对象

        // 1. 转换processedArgs（IRCCStringArray -> vector<string>）
        for (size_t i = 0; i < pCallInfosI->processedArgs.size; ++i)
        {
            if (pCallInfosI->processedArgs.stringArray[i])
            {
                callInfos.processedArgs.emplace_back(pCallInfosI->processedArgs.stringArray[i]);
            }
        }

        // 2. 转换originalArgs（匿名结构体 -> vector<pair<string,string>>）
        for (size_t i = 0; i < pCallInfosI->originalArgs.size; ++i)
        {
            const IRCCStringPair& pair = pCallInfosI->originalArgs.pairs[i];
            std::string first = pair.first ? pair.first : "";
            std::string second = pair.second ? pair.second : "";
            callInfos.originalArgs.emplace_back(first, second);
        }

        // 3. 转换posArgOpItems（IRCCOpItemArray -> queue<ast::OpItem>）
        for (size_t i = 0; i < pCallInfosI->posArgOpItems.size; ++i)
        {
            if (const ast::IRCCOpItemInterface* opItemI = pCallInfosI->posArgOpItems.opItemIArray[i])
            {
                // 假设接口有方法转换回ast::OpItem
                callInfos.posArgOpItems.push(*opItemI->TransformToOII());
            }
        }

        // 4. 转换namedArgOpItems（IRCCStringOpItemMap -> unordered_map）
        for (size_t i = 0; i < pCallInfosI->namedArgOpItems.size; ++i)
        {
            if (const auto& [name, opItemI] = pCallInfosI->namedArgOpItems.nameOpItemPairArray[i];
                name && opItemI)
            {
                callInfos.namedArgOpItems.emplace(
                    name,
                    *opItemI->TransformToOII()
                );
            }
        }

        // 5. 转换callPos（IRCCPosInterface* -> utils::Pos）
        if (pCallInfosI->callPosI)
        {
            callInfos.callPos = *pCallInfosI->callPosI->TransformToPI();
        }

        // 6. 转换orderedArgOpItems（IRCCStringOpItemMap -> vector<pair<string, OpItem>>）
        for (size_t i = 0; i < pCallInfosI->orderedArgOpItems.size; ++i)
        {
            const IRCCStringOpItemPair& pair = pCallInfosI->orderedArgOpItems.nameOpItemPairArray[i];
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

    ast::VarID getParamVarId(const std::string& argName)
    {
        return {argName, utils::getUnknownPos().getFileField(), "global", 0};
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

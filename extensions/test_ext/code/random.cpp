//
// Created by RestRegular on 2025/10/30.
//

#include <cstring>
#include <stdexcept>
#include <unordered_map>
#include <string>

#include "../../../code/include/interfaces/rcc_builtin_interface.h"
#include "../../../code/include/interfaces/rcc_compile_interface.h"
#include "../../../code/include/interfaces/rcc_interface_dec.h"
#include "../../../code/include/interfaces/rcc_symbol_interface.h"

namespace rinterface
{

    extern "C" bool SYS_INTERFACE_CALL DllLoadin()
    {
        return true;
    }

    extern "C" void SYS_INTERFACE_CALL DllUnload()
    {

    }

    extern "C" const char* SYS_INTERFACE_CALL rcc_randomInt(builtin::IRCCCallInfos* callInfos)
    {
        const auto& compileVisitorI = callInfos->compileVisitorI;
        if (callInfos->processedArgs.size != 2)
        {
            throw std::runtime_error("This error is caused by argument count mismatch.");
        }
        const auto& lowLimit = callInfos->processedArgs.stringArray[0];
        const auto& highLimit = callInfos->processedArgs.stringArray[1];
        if (compileVisitorI)
        {
            const auto& newVarNameCharP = compileVisitorI->GetNewTempVarName();
            auto intTypeLabelSymbolI = compileVisitorI->GetTypeLabelSymbolIByStr(
                "int", compileVisitorI->CurScopeLevel());
            compileVisitorI->PushTemOpVarItemWithRecord(compileVisitorI->GetUnknownPosI(),
                intTypeLabelSymbolI, nullptr, true, intTypeLabelSymbolI);
            const auto& tempVarRaValCharP = compileVisitorI->TopOpRaVal();
            const auto& result = "RAND_INT: " + std::string(lowLimit) +
                ", " + std::string(highLimit) + ", " +
                std::string(tempVarRaValCharP) + "\n";
            compileVisitorI->FreeTypeLabelSymbolI(intTypeLabelSymbolI);
            compileVisitorI->FreeCharP(tempVarRaValCharP);
            compileVisitorI->FreeCharP(newVarNameCharP);
            const auto resData = new char[result.length() + 1];
            strcpy(resData, result.c_str());
            return resData;
        }
        throw std::runtime_error("This error is caused by a compiler error.");
    }

    extern "C" const char* SYS_INTERFACE_CALL rcc_randomFloat(builtin::IRCCCallInfos* callInfos)
    {
        const auto& compileVisitorI = callInfos->compileVisitorI;
        if (callInfos->processedArgs.size != 3)
        {
            throw std::runtime_error("This error is caused by argument count mismatch.");
        }
        const auto& lowLimit = callInfos->processedArgs.stringArray[0];
        const auto& highLimit = callInfos->processedArgs.stringArray[1];
        const auto& decimalPlaces = callInfos->processedArgs.stringArray[2];
        if (compileVisitorI)
        {
            const auto& newVarNameCharP = compileVisitorI->GetNewTempVarName();
            auto floatTypeLabelSymbolI = compileVisitorI->GetTypeLabelSymbolIByStr(
                "float", compileVisitorI->CurScopeLevel());
            compileVisitorI->PushTemOpVarItemWithRecord(compileVisitorI->GetUnknownPosI(),
                floatTypeLabelSymbolI, nullptr, true, floatTypeLabelSymbolI);
            const auto& tempVarRaValCharP = compileVisitorI->TopOpRaVal();
            const auto& result = "RAND_FLOAT: " + std::string(lowLimit) +
                ", " + std::string(highLimit) + ", " + std::string(decimalPlaces) + ", " +
                std::string(tempVarRaValCharP) + "\n";
            compileVisitorI->FreeTypeLabelSymbolI(floatTypeLabelSymbolI);
            compileVisitorI->FreeCharP(tempVarRaValCharP);
            compileVisitorI->FreeCharP(newVarNameCharP);
            const auto resData = new char[result.length() + 1];
            strcpy(resData, result.c_str());
            return resData;
        }
        throw std::runtime_error("This error is caused by a compiler error.");
    }

    std::unordered_map<std::string, DLL_EXT_FUNC> dllExtFuncs = {
        {"randomInt", rcc_randomInt},
        {"randomFloat", rcc_randomFloat},
    };

    extern "C" ExtFuncs SYS_INTERFACE_CALL DllObtainExtFuncs()
    {
        ExtFuncs extFuncs;
        extFuncs.extFuncs = new ExtFunc[dllExtFuncs.size()];
        int i = 0;
        for (const auto& [name, func] : dllExtFuncs)
        {
            ExtFunc extFunc;
            extFunc.dllExtFunc = func;
            extFunc.funcName = name.c_str();
            extFuncs.extFuncs[i] = extFunc;
            i++;
        }
        extFuncs.size = i;
        return extFuncs;
    }

    extern "C" void SYS_INTERFACE_CALL DllFreeExtFuncs(ExtFuncs* extFuncs)
    {
        if (extFuncs && extFuncs->extFuncs)
        {
            delete[] extFuncs->extFuncs;
            extFuncs->extFuncs = nullptr;
            extFuncs->size = 0;
        }
    }

    extern "C" void SYS_INTERFACE_CALL DllFreeCharP(const char* data)
    {
        delete[] data;
    }
}

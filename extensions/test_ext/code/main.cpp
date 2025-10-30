//
// Created by RestRegular on 2025/10/24.
//

#include <unordered_map>
#include <string>

#include "../../../code/include/interfaces/rcc_builtin_interface.h"
#include "../../../code/include/interfaces/rcc_compile_interface.h"
#include "../../../code/include/interfaces/rcc_interface_dec.h"

namespace rinterface
{
    ast::IRCCCompileInterface* pMainCompileInterface = nullptr;

    extern "C" bool SYS_INTERFACE_CALL DllLoadin(ast::IRCCCompileInterface* pCompileInterface)
    {
        // 保存主程序的编译接口
        pMainCompileInterface = pCompileInterface;
        return true;
    }

    extern "C" void SYS_INTERFACE_CALL DllUnload()
    {
        pMainCompileInterface = nullptr;
    }

    extern "C" const char* SYS_INTERFACE_CALL rcc_testdll(builtin::IRCCCallInfos *callInfos)
    {
        return "SOUT: s-m, \"This is implemented by dll ext.\"\n";
    }

    std::unordered_map<std::string, DLL_EXT_FUNC> dllExtFuncs = {
        {"testdll", rcc_testdll}
    };

    extern "C" ExtFuncs SYS_INTERFACE_CALL DllObtainExtFuncs()
    {
        ExtFuncs extFuncs;
        extFuncs.extFuncs = new ExtFunc[dllExtFuncs.size()];
        int i = 0;
        for (const auto &[name, func]: dllExtFuncs)
        {
            ExtFunc extFunc;
            extFunc.dllExtFunc = func;
            extFunc.funcName = name.c_str();
            extFuncs.extFuncs[i] = extFunc;
            i ++;
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
        if (pMainCompileInterface)
        {
            pMainCompileInterface = nullptr;
        }
    }

}
//
// Created by RestRegular on 2025/10/22.
//

#ifndef RCC_RCC_INTERFACE_DEC_H
#define RCC_RCC_INTERFACE_DEC_H
#include <cstddef>

#if defined(_WIN32) || defined(_WIN64)
#define SYS_INTERFACE_CALL __stdcall
#else
#define SYS_INTERFACE_CALL
#endif

namespace utils
{

    class IRCCObjectInterface;
    class IRCCPosInterface;

}

namespace ast
{
    enum class OpItemType: int;
    enum class ScopeType: int;
    enum class BuiltinType: int;

    class IRCCOpItemInterface;
    class IRCCCompileInterface;

}

namespace symbol
{
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
}

namespace builtin
{
    struct IRCCStringArray;
    struct IRCCStringPair;
    struct IRCCStringOpItemMap;
    struct IRCCStringOpItemPair;
    struct IRCCOpItemArray;
    struct IRCCCallInfos;
}

namespace rinterface
{
    extern "C" {
#define DLL_FUNC_LOADIN "DllLoadin"
#define DLL_FUNC_UNLOAD "DllUnload"
#define DLL_FUNC_OBTAIN_EXT_FUNCS "DllObtainExtFuncs"
#define DLL_FUNC_FREE_EXT_FUNCS "DllFreeExtFuncs"
        struct ExtFunc;
        struct ExtFuncs;

        using DLL_LOADIN = bool(*)(ast::IRCCCompileInterface* pCompileInterface);
        using DLL_UNLOAD = void(*)();
        using DLL_OBTAIN_EXT_FUNCS = ExtFuncs(*)();
        using DLL_FREE_EXT_FUNCS = void(*)(ExtFuncs* extFuncs);
        using DLL_EXT_FUNC = const char*(*)(builtin::IRCCCallInfos* callInfos);

        struct ExtFunc
        {
            const char* funcName;
            DLL_EXT_FUNC dllExtFunc;
        };

        struct ExtFuncs
        {
            size_t size;
            ExtFunc* extFuncs;
        };

        // 系统调用，DLL实现
        bool SYS_INTERFACE_CALL DllLoadin(ast::IRCCCompileInterface* pCompileInterface);
        void SYS_INTERFACE_CALL DllUnload();
        ExtFuncs SYS_INTERFACE_CALL DllObtainExtFuncs();
        void SYS_INTERFACE_CALL DllFreeExtFuncs(ExtFuncs* extFuncs);
    }
}

#endif //RCC_RCC_INTERFACE_DEC_H
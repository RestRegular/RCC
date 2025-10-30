//
// Created by RestRegular on 2025/10/24.
//

#ifndef RCC_RCC_DLL_EXTENSION_MANAGER_H
#define RCC_RCC_DLL_EXTENSION_MANAGER_H

#include <memory>
#include <string>
#include <unordered_map>
#include <windows.h>

#include "../../interfaces/rcc_interface_dec.h"

namespace rccdll
{
    void throwExtensionCallError(const std::string &funcName);

    using DllExtFuncs = std::unordered_map<std::string, rinterface::DLL_EXT_FUNC>;

    class DLLExtension
    {
        std::string dllFilepath = nullptr;
        HINSTANCE dllExt;
        rinterface::DLL_LOADIN dllLoadin;
        rinterface::DLL_UNLOAD dllUnload;
        DllExtFuncs dllExtFuncs {};
    public:
        explicit DLLExtension(const std::string &dllFilepath_, ast::IRCCCompileInterface* pCompileInterface);
        ~DLLExtension();

        bool hasFunc(const std::string &funcName) const;

        rinterface::DLL_EXT_FUNC find(const std::string &funcName) const;

        template<typename ExtFuncT>
        ExtFuncT loadExtFunc(const std::string &funcName)
        {
            if (const auto &extFunc = reinterpret_cast<ExtFuncT>(GetProcAddress(dllExt, funcName.c_str()));
                extFunc != nullptr)
            {
                return extFunc;
            }
            throwExtensionCallError(funcName);
            return nullptr;
        }
    };

    class DLLExtensionManager
    {
        static std::unordered_map<std::string, std::unique_ptr<DLLExtension>> dllExtMap;
        static std::unordered_map<std::string, std::string> dllExtPathAliasMap;

    public:

        static void registerDllExt(const std::string &dllFilepath_, ast::IRCCCompileInterface* pCompileInterface,
            const std::string& alias = "");

        static void removeDllExt(const std::string &dllExtIdent);

        static bool hasFunc(const std::string &funcName);

        static rinterface::DLL_EXT_FUNC find(const std::string &funcName);
    };

}

#endif //RCC_RCC_DLL_EXTENSION_MANAGER_H
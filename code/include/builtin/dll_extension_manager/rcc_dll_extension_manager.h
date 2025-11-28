//
// Created by RestRegular on 2025/10/24.
//

#ifndef RCC_RCC_DLL_EXTENSION_MANAGER_H
#define RCC_RCC_DLL_EXTENSION_MANAGER_H

#include <memory>
#include <string>
#include <unordered_map>

#include "../../interfaces/rcc_interface_dec.h"

#ifdef _WIN32
typedef HINSTANCE DllHandle;  // Windows 句柄类型
#elif __linux__
#include <dlfcn.h>  // 包含 dlopen、dlsym、dlclose 声明
typedef void* DllHandle;      // Linux 句柄类型（用 void* 表示）
#endif

namespace rccdll
{
    void throwExtensionCallError(const std::string &funcName);

    using DllExtFuncs = std::unordered_map<std::string, rinterface::DLL_EXT_FUNC>;

    class DLLExtension
    {
        std::string dllFilepath = nullptr;
        DllHandle dllExt;
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
            if (!dllExt) {
                throw std::runtime_error("Dynamic library not loaded");
            }
#if defined(_WIN32) || defined(_WIN64)
            auto extFunc = reinterpret_cast<ExtFuncT>(GetProcAddress(dllExt, funcName.c_str()));
#else
            auto extFunc = reinterpret_cast<ExtFuncT>(dlsym(dllExt, funcName.c_str()));
#endif
            if (extFunc != nullptr)
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

        static std::pair<DLLExtension*, rinterface::DLL_EXT_FUNC> find(const std::string &funcName);
    };

}

#endif //RCC_RCC_DLL_EXTENSION_MANAGER_H
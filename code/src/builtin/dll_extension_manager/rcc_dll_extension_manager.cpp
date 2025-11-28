//
// Created by RestRegular on 2025/10/24.
//

#include "../../../include/builtin/dll_extension_manager/rcc_dll_extension_manager.h"

#include "../../../declarations/builtin/rcc_builtin_dec.h"
#include "../../../include/rcc_base.h"
#include "../../../include/visitors/rcc_compile_visitor.h"

namespace rccdll
{
    void throwExtensionCallError(const std::string &funcName)
    {
        throw base::RCCCompilerError::extensionFunctionCallError(
                ast::CompileVisitor::currentPos().toString(),
                ast::CompileVisitor::getCodeLine(ast::CompileVisitor::currentPos()),
                {
                    "Error extension function: " + funcName
                },{
                    "If you are the developer of this DLL extension, please check if the extension "
                    "source file correctly includes this function signature and implements it properly.",
                    "For detailed function signature information, please refer to the interface documentation."
                });
    }

    DLLExtension::DLLExtension(const std::string& dllFilepath_, ast::IRCCCompileInterface* pCompileInterface)
        :dllFilepath(dllFilepath_)
    {
#ifdef _WIN32
        dllExt = LoadLibraryA(dllFilepath.c_str());
        const auto& errorMsg = std::to_string(GetLastError());
#elif __linux__
        dllExt = dlopen(dllFilepath.c_str(), RTLD_LAZY | RTLD_GLOBAL);
        auto errorMsg = std::string(dlerror());
        errorMsg = errorMsg.empty() ? RCC_UNKNOWN_CONST : errorMsg;
#endif
        if (!dllExt)
        {
            throw base::RCCCompilerError::extensionLoadinError(
                ast::CompileVisitor::currentPos().toString(),
                ast::CompileVisitor::getCodeLine(ast::CompileVisitor::currentPos()),
                {
                    "Error dll extension: " + dllFilepath_,
                    "Error info: " + errorMsg
                }, {
                    "Ensure the DLL extension file path is correct and meets the requirements."
                });
        }
        dllLoadin = loadExtFunc<rinterface::DLL_LOADIN>(DLL_FUNC_LOADIN);
        dllUnload = loadExtFunc<rinterface::DLL_UNLOAD>(DLL_FUNC_UNLOAD);
        const auto obtainExtFuncs = loadExtFunc<rinterface::DLL_OBTAIN_EXT_FUNCS>(DLL_FUNC_OBTAIN_EXT_FUNCS);

        dllLoadin();

        const auto [size, extFuncs] = obtainExtFuncs();
        for (int i = 0; i < size; i ++)
        {
            dllExtFuncs.insert({extFuncs[i].funcName, extFuncs[i].dllExtFunc});
        }
    }

    DLLExtension::~DLLExtension()
    {
        if (dllUnload)
        {
            dllUnload();
            dllUnload = nullptr;
        }
        if (dllLoadin)
        {
            dllLoadin = nullptr;
        }
        if (dllExt)
        {
#ifdef _WIN32
            FreeLibrary(dllExt);
#elif __linux__
            dlclose(dllExt);
#endif
            dllExt = nullptr;
        }
    }

    bool DLLExtension::hasFunc(const std::string& funcName) const
    {
        return dllExtFuncs.contains(funcName);
    }

    rinterface::DLL_EXT_FUNC DLLExtension::find(const std::string& funcName) const
    {
        if (const auto &it = dllExtFuncs.find(funcName);
            it != dllExtFuncs.end())
        {
            return it->second;
        }
        return nullptr;
    }

    std::unordered_map<std::string, std::unique_ptr<DLLExtension>> DLLExtensionManager::dllExtMap = {};

    std::unordered_map<std::string, std::string> DLLExtensionManager::dllExtPathAliasMap = {};

    void DLLExtensionManager::registerDllExt(const std::string& dllFilepath_,
                                             ast::IRCCCompileInterface* pCompileInterface,
                                             const std::string& alias)
    {

        auto dllExt = std::make_unique<DLLExtension>(dllFilepath_, pCompileInterface);
        dllExtMap.insert({dllFilepath_, std::move(dllExt)});
        dllExtPathAliasMap.insert({alias, dllFilepath_});
    }

    void DLLExtensionManager::removeDllExt(const std::string& dllExtIdent)
    {
        if (dllExtMap.contains(dllExtIdent))
        {
            dllExtMap.erase(dllExtIdent);
        } else if (dllExtPathAliasMap.contains(dllExtIdent))
        {
            dllExtMap.erase(dllExtPathAliasMap.at(dllExtIdent));
            dllExtPathAliasMap.erase(dllExtIdent);
        }
    }

    bool DLLExtensionManager::hasFunc(const std::string& funcName)
    {
        return std::any_of(dllExtMap.begin(), dllExtMap.end(), [funcName](const auto &it)
        {
            return it.second->hasFunc(funcName);
        });
    }

    std::pair<DLLExtension*, rinterface::DLL_EXT_FUNC> DLLExtensionManager::find(const std::string& funcName)
    {
        for (const auto &dllExt: dllExtMap | std::views::values)
        {
            if (const auto &dllExtFunc = dllExt->find(funcName))
            {
                return {dllExt.get(), dllExtFunc};
            }
        }
        return {nullptr, nullptr};
    }
}

//
// Created by RestRegular on 2025/8/5.
//

#include <filesystem>
#include "../../include/builtin/rcc_builtin.h"
#include "../../declarations/builtin/functions/rcc_builtin_functions_dec.h"
#include "../../include/builtin/dll_extension_manager/rcc_dll_extension_manager.h"
#include "../../include/components/ri/rcc_ri.h"

namespace builtin
{
    using namespace utils;

    void initializePureBuiltinEnvironment(ast::CompileVisitor& visitor)
    {
        visitor.enterGlobalScope();
        // 初始化 - 载入纯内置函数

        const bool &isProgramEntry = checkPathEqual(visitor.getProgramEntryFilePath(), visitor.getCurrentProcessingFilePath());
        for (const auto& [funcName, pureBuiltinFunc] : pureBuiltinFunctionMap)
        {
            const auto &funcId = ast::VarID(funcName, getUnknownPos().getFileField(), visitor.curScopeField(), visitor.curScopeLevel());
            const auto &funcSymbol = std::make_shared<symbol::FunctionSymbol>(
                            nullptr, getUnknownPos(), funcName, funcId.getVid(),
                            std::unordered_set<std::shared_ptr<symbol::LabelSymbol>>{
                                pureBuiltinFunc.hasReturnValue() ?
                                symbol::TypeLabelSymbol::anyTypeSymbol(getUnknownPos(), visitor.curScopeLevel()) :
                                symbol::TypeLabelSymbol::voidTypeSymbol(getUnknownPos(), visitor.curScopeLevel())
                            },
                            pureBuiltinFunc.getParams(), pureBuiltinFunc.getSignature(),
                            0, symbol::TypeOfBuiltin::PURE_BUILTIN,
                            symbol::FunctionType::FUNCTION, nullptr);
            visitor.getSymbolTable().insert(funcSymbol, true);
            if (isProgramEntry) {
                visitor.getRaCodeBuilder()
               << (pureBuiltinFunc.hasReturnValue() ?
               ri::FUNI(funcId.getVid(), {}).toRACode() :
               ri::FUNC(funcId.getVid(), {}).toRACode())
               << ri::ANNOTATION(base::StringVector{
                   "This is a pure-built-in function: `" + funcName + "`",
               })
               << ri::PASS("Function implementation is encapsulated.")
               << ri::END(funcId.getVid());
            }
        }
    }

    bool isPureBuiltinFunction(const std::string& funcName)
    {
        return pureBuiltinFunctionMap.contains(funcName);
    }

    bool isBuiltinFunction(const std::string& funcName)
    {
        return builtinFunctionMap.contains(funcName) ||
            rccdll::DLLExtensionManager::hasFunc(funcName);
    }

    bool isBuiltin(const std::string& funcName)
    {
        return isBuiltinFunction(funcName) || isPureBuiltinFunction(funcName);
    }

    BuiltinFuncRetType callBuiltinFunction(
        ast::CompileVisitor &visitor,
        const std::string &funcName, const CallInfos& callInfos)
    {
        if (const auto &it = pureBuiltinFunctionMap.find(funcName);
            it != pureBuiltinFunctionMap.end())
        {
            return it->second.call(visitor, callInfos);
        }
        if (const auto &it = builtinFunctionMap.find(funcName);
            it != builtinFunctionMap.end())
        {
            return it->second(visitor, callInfos);
        }
        if (const auto &it = rccdll::DLLExtensionManager::find(funcName))
        {
            const auto &callInfosI = transformCallInfos(callInfos);
            const auto &result = it(callInfosI);
            freeCallInfosI(callInfosI);
            return result;
        }
        throw std::runtime_error("Non-existent built-in function: '" + funcName + "'");
    }

    void registerExternalPureBuiltinFunction(const std::string& funcName, const PureBuiltinFunction& func)
    {
        pureBuiltinFunctionMap.insert({funcName, func});
    }

    void registerExternalBuiltinFunction(const std::string& funcName, const BuiltinFunc& func)
    {
        builtinFunctionMap.insert({funcName, func});
    }

    std::unordered_map<std::string, PureBuiltinFunction> pureBuiltinFunctionMap = {
        // pure-built-in functions
        {"import", IMPORT},
        {"export", EXPORT},
        {"super", SUPER}
    };

    std::unordered_map<std::string, BuiltinFunc> builtinFunctionMap = {
        // built-in functions
        // io.rio
        {"sout", rcc_sout},
        {"sin", rcc_sin},

        // ds.rio
        {"size", rcc_size},
        {"copy", rcc_copy},
        {"listRemove", rcc_listRemove},
        {"dictRemove", rcc_dictRemove},
        {"dictKeys", rcc_dictKeys},
        {"dictValues", rcc_dictValues},

        // datatype.rio
        {"type", rcc_type},
        {"setType", rcc_setType},
        {"checkType", rcc_check_type},

        // prog.rio
        {"entry", rcc_entry},
        {"breakpoint", rcc_breakpoint},

        // lowlevel.rio
        {"id", rcc_id},
        {"rasm", rcc_rasm},
        {"bindDllExt", rcc_bindDllExt},
        {"unbindDllExt", rcc_unbindDllExt}
    };
}
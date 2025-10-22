//
// Created by RestRegular on 2025/8/5.
//

#include <filesystem>
#include "../../include/builtin/rcc_builtin.h"
#include "../../include/builtin/functions/rcc_builtin_functions.h"
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
            const auto &funcId = ast::CompileVisitor::VarID(funcName, Pos::UNKNOW_POS.getFileField(), visitor.curScopeField(), visitor.curScopeLevel());
            const auto &funcSymbol = std::make_shared<symbol::FunctionSymbol>(
                            nullptr, Pos::UNKNOW_POS, funcName, funcId.getVid(),
                            std::unordered_set<std::shared_ptr<symbol::LabelSymbol>>{
                                pureBuiltinFunc.hasReturnValue() ?
                                symbol::TypeLabelSymbol::anyTypeSymbol(Pos::UNKNOW_POS, visitor.curScopeLevel()) :
                                symbol::TypeLabelSymbol::voidTypeSymbol(Pos::UNKNOW_POS, visitor.curScopeLevel())
                            },
                            pureBuiltinFunc.getParams(), pureBuiltinFunc.getSignature(),
                            0, symbol::TypeOfBuiltin::PURE_BUILTIN,
                            symbol::FunctionType::FUNCTION, nullptr);
            const auto &signature = funcSymbol->hasReturnValue() ?
                symbol::TypeLabelSymbol::funcTypeSymbol(Pos::UNKNOW_POS, visitor.curScopeLevel()) :
                symbol::TypeLabelSymbol::funiTypeSymbol(Pos::UNKNOW_POS, visitor.curScopeLevel());

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
        return builtinFunctionMap.contains(funcName);
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
        throw std::runtime_error("Non-existent built-in function: '" + funcName + "'");
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
        {"rasm", rcc_rasm}
    };
}
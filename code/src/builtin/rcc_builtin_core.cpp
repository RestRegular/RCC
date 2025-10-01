//
// Created by RestRegular on 2025/9/30.
//

#include "../../include/builtin/rcc_builtin_core.h"

namespace builtin
{

    PureBuiltinFunction::PureBuiltinFunction(const bool& hasReturnValue, const std::string& name,
        const std::vector<std::shared_ptr<symbol::ParameterSymbol>>& params,
        const BuiltinFunc &call)
            : _hasReturnValue(hasReturnValue), _name(name), _params(params), _call(call) {}

    bool PureBuiltinFunction::hasReturnValue() const
    {
        return _hasReturnValue;
    }

    std::string PureBuiltinFunction::getName() const
    {
        return _name;
    }

    std::vector<std::shared_ptr<symbol::ParameterSymbol>> PureBuiltinFunction::getParams() const
    {
        return _params;
    }

    BuiltinFuncRetType PureBuiltinFunction::call(ast::CompileVisitor &visitor,
                             const CallInfos &callInfos) const
    {
        return _call(visitor, callInfos);
    }

    const auto &globalScopeField = ast::CompileVisitor::scopeTypeToString(ast::CompileVisitor::ScopeType::GLOBAL);

    ast::CompileVisitor::VarID getParamVarId(const std::string &argName)
    {
        return {argName, utils::Pos::UNKNOW_POS.getFileField(), globalScopeField, 0};
    }

    std::shared_ptr<symbol::ParameterSymbol> getPosVarArgsParam(const std::string &argName)
    {
        const auto &varId = getParamVarId(argName);
        return std::make_shared<symbol::ParameterSymbol>(
            symbol::ParamType::PARAM_VAR_LEN_POSITIONAL, utils::Pos::UNKNOW_POS,
            varId.getNameField(), varId.getVid(),
            std::unordered_set<std::shared_ptr<symbol::LabelSymbol>>{},
            std::nullopt, 0,
            symbol::SymbolType::PARAMETER, nullptr);
    }

    std::shared_ptr<symbol::ParameterSymbol> getKeywordVarArgsParam(const std::string &argName)
    {
        const auto &varId = getParamVarId(argName);
        return std::make_shared<symbol::ParameterSymbol>(
            symbol::ParamType::PARAM_VAR_LEN_KEYWORD, utils::Pos::UNKNOW_POS,
            varId.getNameField(), varId.getVid(),
            std::unordered_set<std::shared_ptr<symbol::LabelSymbol>>{},
            std::nullopt, 0,
            symbol::SymbolType::PARAMETER, nullptr);
    }

    std::shared_ptr<symbol::ParameterSymbol> getPosArgParam(const std::string &argName)
    {
        const auto &varId = getParamVarId(argName);
        return std::make_shared<symbol::ParameterSymbol>(
            symbol::ParamType::PARAM_POSITIONAL, utils::Pos::UNKNOW_POS,
            varId.getNameField(), varId.getVid(),
            std::unordered_set<std::shared_ptr<symbol::LabelSymbol>>{},
            std::nullopt, 0,
            symbol::SymbolType::PARAMETER, nullptr);
    }

    std::shared_ptr<symbol::ParameterSymbol> getKeywordArgParam(const std::string &argName, const std::string &defaultValue)
    {
        const auto &varId = getParamVarId(argName);
        return std::make_shared<symbol::ParameterSymbol>(
            symbol::ParamType::PARAM_KEYWORD, utils::Pos::UNKNOW_POS,
            varId.getNameField(), varId.getVid(),
            std::unordered_set<std::shared_ptr<symbol::LabelSymbol>>{},
            defaultValue, 0,
            symbol::SymbolType::PARAMETER, nullptr);
    }

}

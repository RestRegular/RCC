//
// Created by RestRegular on 2025/9/16.
//

#include "../../../include/builtin/functions/rcc_builtin_prog_functions.h"

namespace builtin
{

    BuiltinFuncRetType rcc_entry(ast::CompileVisitor &visitor, const CallInfos& callInfos)
    {
        if (callInfos.processedArgs.size() < 1)
        {
            throw std::runtime_error("Failure to match parameters of the built-in function: 'entry'");
        }
        const auto &calledFuncName = callInfos.originalArgs[0].second;
        if (auto [level, symbol] = visitor.getSymbolTable().findByName(calledFuncName);
            level >= 0 && symbol)
        {
            if (symbol->is(symbol::SymbolType::VARIABLE))
            {
                symbol = ast::CompileVisitor::getReferenceTargetSymbol(std::static_pointer_cast<symbol::VariableSymbol>(symbol));
            }
            if (symbol->isNot(symbol::SymbolType::FUNCTION))
            {
                throw std::runtime_error("'" + calledFuncName + "' is not a function");
            }
            const auto &funcSymbol = std::static_pointer_cast<symbol::FunctionSymbol>(symbol);
            if (const auto &parameters = funcSymbol->getParameters();
                parameters.size() != 2 || parameters[0]->getParamType() != symbol::ParamType::PARAM_VAR_LEN_POSITIONAL ||
                parameters[1]->getParamType() != symbol::ParamType::PARAM_VAR_LEN_KEYWORD)
            {
                throw std::runtime_error("The function called in the pure-built-in function 'entry' fails to match the passed parameters: '" + funcSymbol->getVal() + "'");
            }
            const std::vector<std::string> &args = {callInfos.processedArgs.begin() + 1, callInfos.processedArgs.end()};
            if (utils::checkPathEqual(visitor.getProgramEntryFilePath(), visitor.getCurrentProcessingFilePath()))
            {
                if (funcSymbol->hasReturnValue())
                {
                    visitor.pushTemOpVarItemWithRecord(funcSymbol->getPos(), funcSymbol->getReturnType());
                    return ri::IVOK(callInfos.processedArgs[0], args, visitor.topOpRaVal()).toRACode();
                }
                return ri::CALL(callInfos.processedArgs[0], args).toRACode();
            }
            if (funcSymbol->hasReturnValue())
            {
                visitor.pushOpItem(
                    ast::CompileVisitor::OpItemType::LITERAL_VALUE,
                    symbol::TypeLabelSymbol::nulTypeSymbol(utils::Pos::UNKNOW_POS, visitor.curScopeLevel()),
                    "null", "null");
                return "";
            }
            return "";
        }
        throw std::runtime_error("Non-existent symbol: '" + calledFuncName + "'");
    }

    BuiltinFuncRetType rcc_breakpoint(ast::CompileVisitor& visitor, const CallInfos& callInfos)
    {
        return ri::BREAKPOINT().toRACode();
    }
}

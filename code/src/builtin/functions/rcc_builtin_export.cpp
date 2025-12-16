//
// Created by RestRegular on 2025/9/16.
//

#include "../../../declarations/builtin/functions/rcc_builtin_export_dec.h"
#include "../../../include/lib/RLogSystem/rlog_system.h"

namespace builtin
{
    BuiltinFuncRetType rcc_export(ast::CompileVisitor& visitor, const CallInfos& callInfos)
    {
        std::vector<std::string> exportedSymbol{};
        for (const auto& arg : callInfos.originalArgs | std::views::values)
        {
            if (const auto &[level, argSymbol] = visitor.getSymbolTable().findByName(arg);
                level >= 0 && argSymbol)
            {
                exportedSymbol.push_back(argSymbol->getVal());
                argSymbol->setScopeLevel(0);
                if (visitor.__symbol_flag__ && visitor.__symbol_flag_export__ &&
                    visitor.isVisitingProgramEntry())
                {
                    switch (visitor.__symbol_option_format__)
                    {
                    case utils::OutputFormat::TXT:
                        visitor.getAnalyzeBuilder() << argSymbol->toDetailString() << "\n";
                        break;
                    case utils::OutputFormat::JSON:
                        if (visitor.getAnalyzeBuilder().getCurScopeResult().empty())
                        {
                            visitor.getAnalyzeBuilder().enterScope();
                            visitor.getAnalyzeBuilder() << "{}";
                        }
                        rjson::rj::RJsonParser parser (visitor.getAnalyzeBuilder().getCurScopeResult());
                        parser.parse();
                        visitor.getAnalyzeBuilder().exitScope();
                        rjson::rj::RJsonBuilder builder (parser.getParsedValue());
                        builder.insertRJValue(
                            arg,
                            rjson::RJValue::getObjectRJValue(argSymbol->toJsonString()));
                        visitor.getAnalyzeBuilder().enterScope();
                        visitor.getAnalyzeBuilder() << builder.formatString(4);
                        break;
                    }
                }
                visitor.getSymbolTable().removeByName(argSymbol->getVal());
                visitor.getSymbolTable().insert(argSymbol, false);
            } else
            {
                throw std::runtime_error("Non-existent symbol: '" + arg + "'");
            }
        }
        return ri::ANNOTATION((exportedSymbol.size() == 1 ? "export symbol: " : "export symbols: ")
            + utils::vectorJoin(exportedSymbol, ", ")).toRACode();
    }

    const PureBuiltinFunction& getExportFunction()
    {
        static const PureBuiltinFunction EXPORT(
            false, "export",
            {getPosVarArgsParam("args"), getKeywordVarArgsParam("kwargs")},
            rcc_export);
        return EXPORT;
    }

}

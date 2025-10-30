//
// Created by RestRegular on 2025/9/30.
//

#include "../../../declarations/builtin/functions/rcc_builtin_super_dec.h"

namespace builtin
{

    BuiltinFuncRetType rcc_super(ast::CompileVisitor& visitor, const CallInfos& callInfos)
    {
        std::string raCode;
        if (!visitor.isProcessingSymbol())
        {
            throw std::runtime_error("super function can only be used in class ctor definitions.");
        }
        if (const auto &processingSymbol = visitor.topProcessingSymbol();
            processingSymbol && processingSymbol->is(symbol::SymbolType::CLASS))
        {
            const auto &dirInheritClassSymbol = std::static_pointer_cast<symbol::ClassSymbol>(processingSymbol)->getDirectlyInheritedClassSymbol();
            if (!dirInheritClassSymbol)
            {
                throw base::RCCCompilerError::semanticError(callInfos.callPos.toString(),
                    ast::CompileVisitor::getCodeLine(ast::CompileVisitor::currentPos()),
                    "This error is caused by 'super' builtin function called within a class that doesn't have any superclass relationship defined.",
                    {
                        "Please check the class inheritance relationship.",
                        "Remove the 'super' call if this class should not inherit from any parent class.",
                        "Or change the class definition to 'class ClassName: ParentClass { ... }' if inheritance is intended."
                    });
            }
            const auto ctorSymbol = visitor.getCtorSymbol(dirInheritClassSymbol, callInfos.posArgOpItems, callInfos.namedArgOpItems, callInfos.callPos, callInfos.orderedArgOpItems);
            if (const auto &[level, symbol] = visitor.getSymbolTable().findByName("this");
                level >= 0 && symbol && symbol->is(symbol::SymbolType::VARIABLE))
            {
                visitor.getRaCodeBuilder().enterScope();
                const auto &thisFieldSymbol = std::static_pointer_cast<symbol::VariableSymbol>(symbol);
                CallInfos ctorCallInfos {};
                visitor.classifyFuncArgs(*callInfos.callNode, ctorCallInfos.posArgOpItems, ctorCallInfos.namedArgOpItems,
                    ctorCallInfos.orderedArgOpItems);
                visitor.processParameters(ctorSymbol, ctorCallInfos.posArgOpItems,
                                          ctorCallInfos.namedArgOpItems, ctorCallInfos.orderedArgOpItems,
                                          ctorCallInfos.processedArgs, callInfos.callPos);
                visitor.emitFinalCall(ctorSymbol, ctorCallInfos);
                visitor.getRaCodeBuilder().buildCurScope(raCode).exitScope();
                raCode += ri::PUT(visitor.rPopOpItemRaVal(), thisFieldSymbol->getRaVal()).toRACode()
                + ri::TP_DERIVE(thisFieldSymbol->getRaVal(), processingSymbol->getRaVal()).toRACode();
            }
        }
        return raCode;
    }

    const PureBuiltinFunction SUPER (false, "super",
        {
            getPosVarArgsParam("args"),
            getKeywordVarArgsParam("kwargs")},
        rcc_super);
}

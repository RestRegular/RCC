//
// Created by RestRegular on 2025/9/22.
//

#include "../../../declarations/builtin/functions/rcc_builtin_lowlevel_functions_dec.h"
#include "../../../include/builtin/dll_extension_manager/rcc_dll_extension_manager.h"

namespace builtin
{
    BuiltinFuncRetType rcc_id(ast::CompileVisitor& visitor, const CallInfos& callInfos)
    {
        visitor.pushOpItem(
            ast::OpItemType::LITERAL_VALUE,
            symbol::TypeLabelSymbol::strTypeSymbol(
                utils::getUnknownPos(), visitor.curScopeLevel()),
                utils::StringManager::toStringFormat(callInfos.processedArgs[0]));
        return "";
    }

    BuiltinFuncRetType rcc_rasm(ast::CompileVisitor& visitor, const CallInfos& callInfos)
    {
        return "";
    }

    BuiltinFuncRetType rcc_bindDllExt(ast::CompileVisitor& visitor, const CallInfos& callInfos)
    {
        const auto &extAlias = visitor.getNewTempVarName();
        const auto &dllFilePath = utils::StringManager::parseStringFormat(callInfos.processedArgs[0]);
        rccdll::DLLExtensionManager::registerDllExt(utils::getAbsolutePath(
            dllFilePath, utils::getFileDirFromPath(visitor.getCurrentProcessingFilePath())),
            &visitor, extAlias);
        visitor.pushOpItem(ast::OpItemType::LITERAL_VALUE, symbol::TypeLabelSymbol::strTypeSymbol(
            utils::getUnknownPos(), visitor.curScopeLevel()), utils::StringManager::toStringFormat(extAlias),
            visitor.getNewTempVarName());
        return "";
    }

    BuiltinFuncRetType rcc_unbindDllExt(ast::CompileVisitor& visitor, const CallInfos& callInfos)
    {
        const auto& aliasItem = callInfos.posArgOpItems.front();
        if (aliasItem.is(ast::OpItemType::LITERAL_VALUE) && aliasItem.getTypeLabel()->is("str"))
        {
            rccdll::DLLExtensionManager::removeDllExt(utils::parseStringFormat(aliasItem.getVal()));
            return "";
        }
        if (const auto &symbol = visitor.getSymbolFromOpItem(aliasItem);
            symbol->is(symbol::SymbolType::VARIABLE))
        {
            if (const auto &varSymbol = std::static_pointer_cast<symbol::VariableSymbol>(symbol);
                varSymbol->typeIs("str") && varSymbol->getDefaultValue().has_value())
            {
                rccdll::DLLExtensionManager::removeDllExt(utils::parseStringFormat(varSymbol->getDefaultValue().value()));
                return "";
            }
        }
        return "";
    }
}

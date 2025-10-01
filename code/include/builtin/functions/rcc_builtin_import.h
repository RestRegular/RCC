//
// Created by RestRegular on 2025/9/16.
//

#ifndef RCC_BUILTIN_IMPORT_H
#define RCC_BUILTIN_IMPORT_H

#include "../rcc_builtin_core.h"

namespace builtin
{
    namespace rcc_import_utils {
        std::string resolveImportedFilePath(const ast::CompileVisitor& visitor, const std::string& extName);
        std::string generateVariableIdentifier(ast::CompileVisitor& visitor, const std::string& ident,
                                               const std::string& importedFilePath, bool isAutomaticForm);
        std::string handleRegisteredExtension(ast::CompileVisitor& visitor,
                                              const std::shared_ptr<symbol::ClassSymbol>& registeredExtension,
                                              const ast::CompileVisitor::VarID& tempVarId,
                                              bool isAutomaticForm);
        std::string compileAndProcessNewExtension(ast::CompileVisitor& visitor,
                                                  const std::string& importedFilePath,
                                                  const ast::CompileVisitor::VarID& tempVarId);
        std::string processImportedSymbols(const ast::CompileVisitor& visitor,
                                           ast::CompileVisitor& importVisitor,
                                           const std::shared_ptr<symbol::ClassSymbol>& extensionSymbol);
    }

    BuiltinFuncRetType rcc_import(ast::CompileVisitor &visitor, const CallInfos& callInfos);

    extern const PureBuiltinFunction IMPORT;
}

#endif //RCC_BUILTIN_IMPORT_H

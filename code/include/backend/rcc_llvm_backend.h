//
// LLVM Backend for RCC - Compiles Rio AST to LLVM IR
//

#ifndef RCC_LLVM_BACKEND_H
#define RCC_LLVM_BACKEND_H

#include <memory>
#include <string>
#include <vector>
#include <stack>
#include <unordered_map>

// Forward declare NodeType before including LLVM headers to avoid conflicts
namespace ast {
    enum class NodeType;
}

#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Verifier.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Intrinsics.h>

#include "backend/rcc_llvm_types.h"

// Forward declarations for AST nodes
namespace ast {
    class ProgramNode;
    class StatementNode;
    class ExpressionStatementNode;
    class ExpressionNode;
    class IntegerLiteralNode;
    class FloatLiteralNode;
    class BooleanLiteralNode;
    class StringLiteralNode;
    class NullLiteralNode;
    class CharacterLiteralNode;
    class IdentifierNode;
    class VariableDefinitionNode;
    class AssignmentNode;
    class FunctionDefinitionNode;
    class FunctionCallNode;
    class ConditionNode;
    class BranchNode;
    class WhileLoopNode;
    class UntilLoopNode;
    class ForLoopNode;
    class ReturnExpressionNode;
    class BreakExpressionNode;
    class PassExpressionNode;
    class InfixExpressionNode;
    class PrefixExpressionNode;
    class PostfixExpressionNode;
    class UnaryExpressionNode;
    class BlockRangerNode;
    class ListExpressionNode;
    class ParenRangerNode;
    class IndexExpressionNode;
    class BracketExpressionNode;
    class DictionaryExpressionNode;
    class PairExpressionNode;
    class LabelNode;
    class TryNode;
    class ThrowNode;
    class AnonFunctionDefinitionNode;
    class EncapsulatedExpressionNode;
}

namespace backend {

class LLVMBackend {
public:
    LLVMBackend();

    // Main compilation entry point: compile an entire program
    bool compile(const std::shared_ptr<ast::ProgramNode>& program);

    // Output LLVM IR to a file
    bool emitIR(const std::string& outputPath);

    // Compile to an executable file
    bool emitExecutable(const std::string& outputPath);

    // Get the generated LLVM module (for testing/debugging)
    [[nodiscard]] llvm::Module* getModule() const { return module.get(); }

private:
    // LLVM core structures
    llvm::LLVMContext context;
    std::unique_ptr<llvm::Module> module;
    std::unique_ptr<llvm::IRBuilder<>> builder;
    LLVMTypeManager typeManager;

    // Function stack (tracks current function being compiled)
    std::stack<llvm::Function*> functionStack;

    // Variable scope chain (vector of maps, index 0 = global scope)
    std::vector<std::unordered_map<std::string, llvm::Value*>> scopeStack;

    // Break/continue targets for loop control flow
    std::stack<llvm::BasicBlock*> breakTargets;
    std::stack<llvm::BasicBlock*> continueTargets;

    // Runtime function declarations (C linkage)
    llvm::FunctionCallee runtimeInit;
    llvm::FunctionCallee runtimeShutdown;
    llvm::FunctionCallee printInt;
    llvm::FunctionCallee printFloat;
    llvm::FunctionCallee printBool;
    llvm::FunctionCallee printString;
    llvm::FunctionCallee printNewline;
    llvm::FunctionCallee stringNew;
    llvm::FunctionCallee stringConcat;
    llvm::FunctionCallee gcAlloc;
    llvm::FunctionCallee listNew;
    llvm::FunctionCallee listAppend;
    llvm::FunctionCallee listGet;
    llvm::FunctionCallee listSet;
    llvm::FunctionCallee listLength;
    llvm::FunctionCallee printObject;

    // String constant pool (avoids duplicate creation)
    std::unordered_map<std::string, llvm::Value*> stringConstants;

    // ==================== Initialization ====================

    // Declare all runtime C functions
    void declareRuntimeFunctions();

    // ==================== Scope Management ====================

    void pushScope();
    void popScope();
    void setVariable(const std::string& name, llvm::Value* value);
    llvm::Value* getVariable(const std::string& name);
    bool hasVariable(const std::string& name);

    // ==================== AST Compilation ====================

    // Top-level program compilation
    void compileProgram(const std::shared_ptr<ast::ProgramNode>& node);

    // Statement compilation
    void compileStatement(const std::shared_ptr<ast::StatementNode>& node);

    // Expression compilation (dispatcher)
    llvm::Value* compileExpression(const std::shared_ptr<ast::ExpressionNode>& node);

    // ==================== Literal Nodes ====================

    llvm::Value* compileIntegerLiteral(const std::shared_ptr<ast::IntegerLiteralNode>& node);
    llvm::Value* compileFloatLiteral(const std::shared_ptr<ast::FloatLiteralNode>& node);
    llvm::Value* compileBoolLiteral(const std::shared_ptr<ast::BooleanLiteralNode>& node);
    llvm::Value* compileStringLiteral(const std::shared_ptr<ast::StringLiteralNode>& node);
    llvm::Value* compileCharLiteral(const std::shared_ptr<ast::CharacterLiteralNode>& node);
    llvm::Value* compileNullLiteral(const std::shared_ptr<ast::NullLiteralNode>& node);

    // ==================== Identifier & Variables ====================

    llvm::Value* compileIdentifier(const std::shared_ptr<ast::IdentifierNode>& node);
    llvm::Value* compileVariableDef(const std::shared_ptr<ast::VariableDefinitionNode>& node);
    llvm::Value* compileAssignment(const std::shared_ptr<ast::AssignmentNode>& node);

    // ==================== Functions ====================

    llvm::Value* compileFunctionDef(const std::shared_ptr<ast::FunctionDefinitionNode>& node);
    llvm::Value* compileFunctionCall(const std::shared_ptr<ast::FunctionCallNode>& node);
    llvm::Value* compileAnon(const std::shared_ptr<ast::AnonFunctionDefinitionNode>& node);

    // ==================== Control Flow ====================

    llvm::Value* compileCondition(const std::shared_ptr<ast::ConditionNode>& node);
    llvm::Value* compileWhileLoop(const std::shared_ptr<ast::WhileLoopNode>& node);
    llvm::Value* compileUntilLoop(const std::shared_ptr<ast::UntilLoopNode>& node);
    llvm::Value* compileForLoop(const std::shared_ptr<ast::ForLoopNode>& node);
    llvm::Value* compileReturn(const std::shared_ptr<ast::ReturnExpressionNode>& node);
    llvm::Value* compileBreak(const std::shared_ptr<ast::BreakExpressionNode>& node);
    llvm::Value* compileContinue();

    // ==================== Operators ====================

    llvm::Value* compileInfix(const std::shared_ptr<ast::InfixExpressionNode>& node);
    llvm::Value* compilePrefix(const std::shared_ptr<ast::PrefixExpressionNode>& node);
    llvm::Value* compilePostfix(const std::shared_ptr<ast::PostfixExpressionNode>& node);
    llvm::Value* compileUnary(const std::shared_ptr<ast::UnaryExpressionNode>& node);

    // ==================== Compound Expressions ====================

    llvm::Value* compileBlock(const std::shared_ptr<ast::BlockRangerNode>& node);
    llvm::Value* compileList(const std::shared_ptr<ast::ListExpressionNode>& node);
    llvm::Value* compileParen(const std::shared_ptr<ast::ParenRangerNode>& node);
    llvm::Value* compileBracket(const std::shared_ptr<ast::BracketExpressionNode>& node);
    llvm::Value* compileIndex(const std::shared_ptr<ast::IndexExpressionNode>& node);
    llvm::Value* compileDictionary(const std::shared_ptr<ast::DictionaryExpressionNode>& node);
    llvm::Value* compilePair(const std::shared_ptr<ast::PairExpressionNode>& node);
    llvm::Value* compilePass(const std::shared_ptr<ast::PassExpressionNode>& node);
    llvm::Value* compileEncapsulated(const std::shared_ptr<ast::EncapsulatedExpressionNode>& node);

    // ==================== Exception Handling ====================

    llvm::Value* compileTry(const std::shared_ptr<ast::TryNode>& node);
    llvm::Value* compileThrow(const std::shared_ptr<ast::ThrowNode>& node);

    // ==================== Helper Methods ====================

    // Flatten a ParallelNode (comma-separated list) into a vector of expressions
    std::vector<std::shared_ptr<ast::ExpressionNode>> flattenParallel(
        const std::shared_ptr<ast::ExpressionNode>& node);

    // Compile a binary operation between two LLVM values
    llvm::Value* compileBinaryOp(llvm::Value* left, llvm::Value* right, ast::NodeType op);

    // Compile a comparison operation
    llvm::Value* compileComparison(llvm::Value* left, llvm::Value* right, ast::NodeType op);

    // Compile a logical operation (with short-circuit evaluation)
    llvm::Value* compileLogicalOp(llvm::Value* left, llvm::Value* right, ast::NodeType op);

    // Create a RioString* from a C string constant
    llvm::Value* createStringConstant(const std::string& str);

    // Compile a sout/print call with runtime print functions
    llvm::Value* compileSoutCall(const std::vector<std::shared_ptr<ast::ExpressionNode>>& args);

    // Convert an LLVM value to i1 (boolean condition)
    llvm::Value* convertToBool(llvm::Value* value);

    // Type inference helpers (static analysis of AST nodes)
    bool isIntegerExpr(const std::shared_ptr<ast::ExpressionNode>& node);
    bool isFloatExpr(const std::shared_ptr<ast::ExpressionNode>& node);
    bool isStringExpr(const std::shared_ptr<ast::ExpressionNode>& node);
    bool isNullExpr(const std::shared_ptr<ast::ExpressionNode>& node);
};

} // namespace backend

#endif // RCC_LLVM_BACKEND_H

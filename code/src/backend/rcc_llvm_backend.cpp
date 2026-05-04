//
// LLVM Backend Implementation for RCC
// Compiles Rio language AST to LLVM IR
//

#include "../../include/backend/rcc_llvm_backend.h"

// AST headers
#include "analyzer/rcc_ast_components.h"
#include "analyzer/rcc_ast.h"
#include "rcc_core.h"

// LLVM headers
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Verifier.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Intrinsics.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Constants.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/Host.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetOptions.h>

#include <iostream>
#include <sstream>
#include <cstdlib>
#include <algorithm>

using namespace std;
using namespace ast;
using namespace llvm;

namespace backend {

// ============================================================================
// Helper: get the pointee type of an alloca (for LLVM 15 opaque pointers)
// Since opaque pointers don't carry element type info, we track it separately.
// ============================================================================

// Helper struct to track alloca types
static Type* getAllocaElementType(Value* alloca) {
    // In LLVM 15 with opaque pointers, we cannot get element type from pointer.
    // We use the pointer's name hint and fall back to i64 as default.
    // The actual type is determined at variable definition time.
    return Type::getInt64Ty(alloca->getContext());
}

// ============================================================================
// Constructor
// ============================================================================

LLVMBackend::LLVMBackend()
    : context(), typeManager(context) {
    module = make_unique<Module>("RioProgram", context);
    builder = make_unique<IRBuilder<>>(context);

    // Set module data layout and target triple to defaults
    module->setTargetTriple(sys::getDefaultTargetTriple());

    // Declare runtime functions
    declareRuntimeFunctions();
}

// ============================================================================
// Runtime Function Declarations
// ============================================================================

void LLVMBackend::declareRuntimeFunctions() {
    // void rio_runtime_init(void)
    runtimeInit = module->getOrInsertFunction(
        "rio_runtime_init",
        FunctionType::get(typeManager.getVoidType(), false)
    );

    // void rio_runtime_shutdown(void)
    runtimeShutdown = module->getOrInsertFunction(
        "rio_runtime_shutdown",
        FunctionType::get(typeManager.getVoidType(), false)
    );

    // void rio_print_int(int64_t)
    printInt = module->getOrInsertFunction(
        "rio_print_int",
        FunctionType::get(typeManager.getVoidType(), {typeManager.getIntType()}, false)
    );

    // void rio_print_float(double)
    printFloat = module->getOrInsertFunction(
        "rio_print_float",
        FunctionType::get(typeManager.getVoidType(), {typeManager.getFloatType()}, false)
    );

    // void rio_print_bool(int32_t)
    printBool = module->getOrInsertFunction(
        "rio_print_bool",
        FunctionType::get(typeManager.getVoidType(), {Type::getInt32Ty(context)}, false)
    );

    // void rio_print_string(RioString*)
    printString = module->getOrInsertFunction(
        "rio_print_string",
        FunctionType::get(typeManager.getVoidType(), {typeManager.getStringType()}, false)
    );

    // void rio_print_newline(void)
    printNewline = module->getOrInsertFunction(
        "rio_print_newline",
        FunctionType::get(typeManager.getVoidType(), false)
    );

    // RioString* rio_string_new(const char*, int64_t)
    stringNew = module->getOrInsertFunction(
        "rio_string_new",
        FunctionType::get(typeManager.getStringType(),
            {typeManager.getStringType(), typeManager.getIntType()}, false)
    );

    // RioString* rio_string_concat(RioString*, RioString*)
    stringConcat = module->getOrInsertFunction(
        "rio_string_concat",
        FunctionType::get(typeManager.getStringType(),
            {typeManager.getStringType(), typeManager.getStringType()}, false)
    );

    // RioObject* rio_gc_alloc(size_t, uint32_t)
    gcAlloc = module->getOrInsertFunction(
        "rio_gc_alloc",
        FunctionType::get(typeManager.getObjectType(),
            {typeManager.getIntType(), Type::getInt32Ty(context)}, false)
    );

    // List functions
    listNew = module->getOrInsertFunction("rio_list_new",
        typeManager.getPtrType(), typeManager.getIntType());
    listAppend = module->getOrInsertFunction("rio_list_append",
        Type::getVoidTy(context), typeManager.getPtrType(), typeManager.getPtrType());
    listGet = module->getOrInsertFunction("rio_list_get",
        typeManager.getPtrType(), typeManager.getPtrType(), typeManager.getIntType());
    listSet = module->getOrInsertFunction("rio_list_set",
        Type::getVoidTy(context), typeManager.getPtrType(), typeManager.getIntType(), typeManager.getPtrType());
    listLength = module->getOrInsertFunction("rio_list_length",
        typeManager.getIntType(), typeManager.getPtrType());
    printObject = module->getOrInsertFunction("rio_print_object",
        Type::getVoidTy(context), typeManager.getPtrType());
}

// ============================================================================
// Scope Management
// ============================================================================

void LLVMBackend::pushScope() {
    scopeStack.emplace_back();
}

void LLVMBackend::popScope() {
    if (!scopeStack.empty()) {
        scopeStack.pop_back();
    }
}

void LLVMBackend::setVariable(const string& name, llvm::Value* value) {
    if (scopeStack.empty()) {
        pushScope();
    }
    scopeStack.back()[name] = value;
}

llvm::Value* LLVMBackend::getVariable(const string& name) {
    // Search from innermost scope outward
    for (auto it = scopeStack.rbegin(); it != scopeStack.rend(); ++it) {
        auto found = it->find(name);
        if (found != it->end()) {
            return found->second;
        }
    }
    return nullptr;
}

bool LLVMBackend::hasVariable(const string& name) {
    return getVariable(name) != nullptr;
}

// ============================================================================
// Main Compilation Entry Point
// ============================================================================

bool LLVMBackend::compile(const shared_ptr<ProgramNode>& program) {
    if (!program) return false;

    try {
        compileProgram(program);

        // Verify the module
        string err;
        raw_string_ostream errStream(err);
        if (verifyModule(*module, &errStream)) {
            cerr << "LLVM Module verification failed: " << err << endl;
            return false;
        }
        return true;
    } catch (const exception& e) {
        cerr << "Compilation error: " << e.what() << endl;
        return false;
    }
}

// ============================================================================
// compileProgram - Creates the main() function and compiles all statements
// ============================================================================

void LLVMBackend::compileProgram(const shared_ptr<ProgramNode>& program) {
    // 1. Create main function: i32 main()
    auto mainFunc = Function::Create(
        FunctionType::get(Type::getInt32Ty(context), false),
        Function::ExternalLinkage, "main", module.get()
    );
    auto entry = BasicBlock::Create(context, "entry", mainFunc);
    builder->SetInsertPoint(entry);

    // 2. Call runtime initialization
    builder->CreateCall(runtimeInit);

    // 3. Enter global scope
    pushScope();

    // 4. Compile each statement
    for (auto& stmt : program->getStatements()) {
        compileStatement(stmt);
    }

    // 5. Call runtime cleanup
    builder->CreateCall(runtimeShutdown);

    // 6. Return 0
    builder->CreateRet(ConstantInt::get(Type::getInt32Ty(context), 0));

    popScope();
}

// ============================================================================
// compileStatement - Compile a single statement node
// ============================================================================

void LLVMBackend::compileStatement(const shared_ptr<StatementNode>& node) {
    if (!node) return;

    // Most statements are ExpressionStatementNode wrapping an ExpressionNode
    if (auto exprStmt = dynamic_pointer_cast<ExpressionStatementNode>(node)) {
        compileExpression(exprStmt->getExpression());
        return;
    }

    // Handle ProgramNode directly (should not normally happen at statement level)
    if (auto prog = dynamic_pointer_cast<ProgramNode>(node)) {
        compileProgram(prog);
        return;
    }

    // For any other StatementNode, try to treat it as an expression
    if (auto expr = dynamic_pointer_cast<ExpressionNode>(node)) {
        compileExpression(expr);
        return;
    }
}

// ============================================================================
// compileExpression - Dispatch to the appropriate compile method
// ============================================================================

llvm::Value* LLVMBackend::compileExpression(const shared_ptr<ExpressionNode>& node) {
    if (!node) return ConstantInt::get(typeManager.getIntType(), 0);

    // Use dynamic_pointer_cast to determine the actual node type
    if (auto n = dynamic_pointer_cast<IntegerLiteralNode>(node)) {
        return compileIntegerLiteral(n);
    }
    if (auto n = dynamic_pointer_cast<FloatLiteralNode>(node)) {
        return compileFloatLiteral(n);
    }
    if (auto n = dynamic_pointer_cast<BooleanLiteralNode>(node)) {
        return compileBoolLiteral(n);
    }
    if (auto n = dynamic_pointer_cast<StringLiteralNode>(node)) {
        return compileStringLiteral(n);
    }
    if (auto n = dynamic_pointer_cast<CharacterLiteralNode>(node)) {
        return compileCharLiteral(n);
    }
    if (auto n = dynamic_pointer_cast<NullLiteralNode>(node)) {
        return compileNullLiteral(n);
    }
    if (auto n = dynamic_pointer_cast<IdentifierNode>(node)) {
        return compileIdentifier(n);
    }
    if (auto n = dynamic_pointer_cast<VariableDefinitionNode>(node)) {
        return compileVariableDef(n);
    }
    if (auto n = dynamic_pointer_cast<AssignmentNode>(node)) {
        return compileAssignment(n);
    }
    if (auto n = dynamic_pointer_cast<FunctionDefinitionNode>(node)) {
        return compileFunctionDef(n);
    }
    if (auto n = dynamic_pointer_cast<FunctionCallNode>(node)) {
        return compileFunctionCall(n);
    }
    if (auto n = dynamic_pointer_cast<ConditionNode>(node)) {
        return compileCondition(n);
    }
    if (auto n = dynamic_pointer_cast<WhileLoopNode>(node)) {
        return compileWhileLoop(n);
    }
    if (auto n = dynamic_pointer_cast<UntilLoopNode>(node)) {
        return compileUntilLoop(n);
    }
    if (auto n = dynamic_pointer_cast<ForLoopNode>(node)) {
        return compileForLoop(n);
    }
    if (auto n = dynamic_pointer_cast<ReturnExpressionNode>(node)) {
        return compileReturn(n);
    }
    if (auto n = dynamic_pointer_cast<BreakExpressionNode>(node)) {
        return compileBreak(n);
    }
    if (auto n = dynamic_pointer_cast<ContinueExpressionNode>(node)) {
        return compileContinue();
    }
    if (auto n = dynamic_pointer_cast<InfixExpressionNode>(node)) {
        return compileInfix(n);
    }
    if (auto n = dynamic_pointer_cast<PrefixExpressionNode>(node)) {
        return compilePrefix(n);
    }
    if (auto n = dynamic_pointer_cast<PostfixExpressionNode>(node)) {
        return compilePostfix(n);
    }
    if (auto n = dynamic_pointer_cast<UnaryExpressionNode>(node)) {
        return compileUnary(n);
    }
    if (auto n = dynamic_pointer_cast<BlockRangerNode>(node)) {
        return compileBlock(n);
    }
    if (auto n = dynamic_pointer_cast<ListExpressionNode>(node)) {
        return compileList(n);
    }
    if (auto n = dynamic_pointer_cast<ParenRangerNode>(node)) {
        return compileParen(n);
    }
    if (auto n = dynamic_pointer_cast<BracketExpressionNode>(node)) {
        return compileBracket(n);
    }
    if (auto n = dynamic_pointer_cast<IndexExpressionNode>(node)) {
        return compileIndex(n);
    }
    if (auto n = dynamic_pointer_cast<DictionaryExpressionNode>(node)) {
        return compileDictionary(n);
    }
    if (auto n = dynamic_pointer_cast<PairExpressionNode>(node)) {
        return compilePair(n);
    }
    if (auto n = dynamic_pointer_cast<AnonFunctionDefinitionNode>(node)) {
        return compileAnon(n);
    }
    if (auto n = dynamic_pointer_cast<PassExpressionNode>(node)) {
        return compilePass(n);
    }
    if (auto n = dynamic_pointer_cast<EncapsulatedExpressionNode>(node)) {
        return compileEncapsulated(n);
    }
    if (auto n = dynamic_pointer_cast<TryNode>(node)) {
        return compileTry(n);
    }
    if (auto n = dynamic_pointer_cast<ThrowNode>(node)) {
        return compileThrow(n);
    }
    if (auto n = dynamic_pointer_cast<LabelNode>(node)) {
        // Label nodes are handled within identifier/function contexts
        return ConstantInt::get(typeManager.getIntType(), 0);
    }

    // Unknown node type - return default value
    cerr << "Warning: Unknown expression node type" << endl;
    return ConstantInt::get(typeManager.getIntType(), 0);
}

// ============================================================================
// Literal Nodes
// ============================================================================

llvm::Value* LLVMBackend::compileIntegerLiteral(const shared_ptr<IntegerLiteralNode>& node) {
    int64_t val = stoll(node->literalString());
    return ConstantInt::get(typeManager.getIntType(), val);
}

llvm::Value* LLVMBackend::compileFloatLiteral(const shared_ptr<FloatLiteralNode>& node) {
    double val = stod(node->literalString());
    return ConstantFP::get(typeManager.getFloatType(), val);
}

llvm::Value* LLVMBackend::compileBoolLiteral(const shared_ptr<BooleanLiteralNode>& node) {
    // Booleans are stored as i64: true=1, false=0
    bool val = (node->literalString() == "true" || node->literalString() == "True");
    return ConstantInt::get(typeManager.getIntType(), val ? 1 : 0);
}

llvm::Value* LLVMBackend::compileStringLiteral(const shared_ptr<StringLiteralNode>& node) {
    string raw = node->literalString();
    // Strip surrounding quotes if present
    if (raw.size() >= 2 && raw.front() == '"' && raw.back() == '"') {
        raw = raw.substr(1, raw.size() - 2);
    }
    return createStringConstant(raw);
}

llvm::Value* LLVMBackend::compileCharLiteral(const shared_ptr<CharacterLiteralNode>& node) {
    // Characters are stored as i64
    string litStr = node->literalString();
    int64_t charVal = 0;
    if (!litStr.empty()) {
        charVal = static_cast<int64_t>(litStr[0]);
    }
    return ConstantInt::get(typeManager.getIntType(), charVal);
}

llvm::Value* LLVMBackend::compileNullLiteral(const shared_ptr<NullLiteralNode>& /*node*/) {
    return ConstantPointerNull::get(cast<PointerType>(typeManager.getPtrType()));
}

// ============================================================================
// Identifier & Variables
// ============================================================================

llvm::Value* LLVMBackend::compileIdentifier(const shared_ptr<IdentifierNode>& node) {
    string name = node->getName();
    llvm::Value* alloca = getVariable(name);

    if (!alloca) {
        cerr << "Error: Undefined variable '" << name << "'" << endl;
        return ConstantInt::get(typeManager.getIntType(), 0);
    }

    // Load the value from the alloca using the correct element type
    Type* elemType = typeManager.getIntType();
    if (auto* allocaInst = dyn_cast<AllocaInst>(alloca)) {
        elemType = allocaInst->getAllocatedType();
    }
    return builder->CreateLoad(elemType, alloca, name + ".val");
}

llvm::Value* LLVMBackend::compileVariableDef(const shared_ptr<VariableDefinitionNode>& node) {
    for (auto& varDef : node->getVarDefs()) {
        auto nameIdent = varDef->getNameNode();
        string name = nameIdent->getName();

        llvm::Value* initVal = nullptr;
        llvm::Type* varType = typeManager.getIntType(); // default type

        if (varDef->hasInitialValue()) {
            initVal = compileExpression(varDef->getValueNode());
            // Determine the type from the initial value
            if (initVal->getType()->isDoubleTy()) {
                varType = typeManager.getFloatType();
            } else if (initVal->getType()->isPointerTy()) {
                varType = typeManager.getPtrType();
            } else {
                varType = typeManager.getIntType();
            }
        } else {
            // No initial value: default to 0
            initVal = ConstantInt::get(typeManager.getIntType(), 0);
        }

        // Create alloca in the current function's entry block
        auto alloca = builder->CreateAlloca(varType, nullptr, name);
        builder->CreateStore(initVal, alloca);
        setVariable(name, alloca);
    }
    return ConstantInt::get(typeManager.getIntType(), 0);
}

llvm::Value* LLVMBackend::compileAssignment(const shared_ptr<AssignmentNode>& node) {
    auto pair = node->getAssignPair();
    auto leftNode = pair.first;
    auto rightNode = pair.second;

    llvm::Value* rightVal = compileExpression(rightNode);

    // Handle assignment to identifier
    if (auto ident = dynamic_pointer_cast<IdentifierNode>(leftNode)) {
        string name = ident->getName();
        llvm::Value* alloca = getVariable(name);
        if (!alloca) {
            // Variable not found - create it in current scope
            llvm::Type* varType = typeManager.getIntType();
            if (rightVal->getType()->isDoubleTy()) {
                varType = typeManager.getFloatType();
            } else if (rightVal->getType()->isPointerTy()) {
                varType = typeManager.getPtrType();
            }
            alloca = builder->CreateAlloca(varType, nullptr, name);
            setVariable(name, alloca);
        }
        builder->CreateStore(rightVal, alloca);
        return rightVal;
    }

    // Handle assignment to index expression (list[index] = value)
    if (auto indexExpr = dynamic_pointer_cast<IndexExpressionNode>(leftNode)) {
        // For now, emit a warning - full index assignment support requires runtime list operations
        cerr << "Warning: Index assignment not fully supported in LLVM backend" << endl;
        return rightVal;
    }

    cerr << "Error: Invalid assignment target" << endl;
    return rightVal;
}

// ============================================================================
// Functions
// ============================================================================

llvm::Value* LLVMBackend::compileFunctionDef(const shared_ptr<FunctionDefinitionNode>& node) {
    // The callNode is a FunctionCallNode form:
    //   left = function name (IdentifierNode)
    //   right = parameter list (possibly ParenRangerNode containing ParallelNode)
    auto callNode = dynamic_pointer_cast<FunctionCallNode>(node->getCallNode());
    if (!callNode) {
        cerr << "Error: Function definition has invalid callNode" << endl;
        return ConstantInt::get(typeManager.getIntType(), 0);
    }

    auto nameIdent = dynamic_pointer_cast<IdentifierNode>(callNode->getLeftNode());
    if (!nameIdent) {
        cerr << "Error: Function definition has invalid function name" << endl;
        return ConstantInt::get(typeManager.getIntType(), 0);
    }

    string funcName = nameIdent->getName();

    // Parse parameters from the right node of the call
    auto params = flattenParallel(callNode->getRightNode());

    // Create function type: all params are i64, return type is i64
    vector<Type*> paramTypes(params.size(), typeManager.getIntType());
    auto funcType = FunctionType::get(typeManager.getIntType(), paramTypes, false);
    auto func = Function::Create(funcType, Function::ExternalLinkage, funcName, module.get());

    // Set parameter names (getArg returns Argument* in LLVM 15)
    for (size_t i = 0; i < params.size(); i++) {
        auto paramIdent = dynamic_pointer_cast<IdentifierNode>(params[i]);
        if (paramIdent) {
            func->getArg(i)->setName(paramIdent->getName());
        }
    }

    // Save current insert point
    auto savedBlock = builder->GetInsertBlock();

    // Create entry basic block
    auto bb = BasicBlock::Create(context, funcName + "_entry", func);
    builder->SetInsertPoint(bb);

    functionStack.push(func);
    pushScope();

    // Allocate parameters and store argument values
    for (auto& arg : func->args()) {
        auto alloca = builder->CreateAlloca(typeManager.getIntType(), nullptr, arg.getName());
        builder->CreateStore(&arg, alloca);
        setVariable(string(arg.getName()), alloca);
    }

    // Compile function body
    auto body = dynamic_pointer_cast<BlockRangerNode>(node->getBodyNode());
    if (body) {
        for (auto& expr : body->getBodyExpressions()) {
            compileExpression(expr);
        }
    }

    // If the current block has no terminator, add a default return
    if (builder->GetInsertBlock() && builder->GetInsertBlock()->getTerminator() == nullptr) {
        builder->CreateRet(ConstantInt::get(typeManager.getIntType(), 0));
    }

    popScope();
    functionStack.pop();

    // Restore previous insert point
    if (savedBlock) {
        builder->SetInsertPoint(savedBlock);
    }

    return ConstantInt::get(typeManager.getIntType(), 0);
}

// ============================================================================
// Anonymous Functions / Closures
// ============================================================================

llvm::Value* LLVMBackend::compileAnon(const shared_ptr<AnonFunctionDefinitionNode>& node) {
    static int anonCounter = 0;
    string funcName = "__anon_" + to_string(anonCounter++);

    // Parse parameters
    auto params = flattenParallel(node->getParamNode());

    // Create function type: all params i64, returns i64
    vector<Type*> paramTypes(params.size(), typeManager.getIntType());
    auto funcType = FunctionType::get(typeManager.getIntType(), paramTypes, false);
    auto func = Function::Create(funcType, Function::PrivateLinkage, funcName, module.get());

    // Set parameter names
    for (size_t i = 0; i < params.size() && i < func->arg_size(); i++) {
        if (auto paramIdent = dynamic_pointer_cast<IdentifierNode>(params[i])) {
            func->getArg(i)->setName(paramIdent->getName());
        }
    }

    // Save current insert point
    auto savedBB = builder->GetInsertBlock();

    // Create function body
    auto entryBB = BasicBlock::Create(context, funcName + ".entry", func);
    builder->SetInsertPoint(entryBB);

    functionStack.push(func);
    pushScope();

    // Allocate and store parameters
    for (auto& arg : func->args()) {
        auto alloca = builder->CreateAlloca(typeManager.getIntType(), nullptr, arg.getName());
        builder->CreateStore(&arg, alloca);
        setVariable(string(arg.getName()), alloca);
    }

    // Compile body
    if (node->getBodyNode()) {
        if (auto block = dynamic_pointer_cast<BlockRangerNode>(node->getBodyNode())) {
            for (auto& expr : block->getBodyExpressions()) {
                compileExpression(expr);
            }
        } else {
            compileExpression(node->getBodyNode());
        }
    }

    // Default return if no terminator
    if (builder->GetInsertBlock()->getTerminator() == nullptr) {
        builder->CreateRet(ConstantInt::get(typeManager.getIntType(), 0));
    }

    popScope();
    functionStack.pop();

    // Restore insert point
    builder->SetInsertPoint(savedBB);

    // Return function pointer as i64
    return builder->CreatePtrToInt(func, typeManager.getIntType());
}

llvm::Value* LLVMBackend::compileFunctionCall(const shared_ptr<FunctionCallNode>& node) {
    auto calledFunc = node->getLeftNode();

    // Special handling for sout/print
    if (auto ident = dynamic_pointer_cast<IdentifierNode>(calledFunc)) {
        if (ident->getName() == "sout" || ident->getName() == "print") {
            auto args = flattenParallel(node->getRightNode());
            return compileSoutCall(args);
        }
    }

    // Determine function name
    string funcName;
    if (auto ident = dynamic_pointer_cast<IdentifierNode>(calledFunc)) {
        funcName = ident->getName();
    }

    // Compile arguments
    auto args = flattenParallel(node->getRightNode());
    vector<Value*> argValues;
    for (auto& arg : args) {
        argValues.push_back(compileExpression(arg));
    }

    // Try to find a named function first
    llvm::Function* func = module->getFunction(funcName);
    if (func) {
        // Check argument count matches
        if (argValues.size() != func->arg_size()) {
            func->eraseFromParent();
            vector<Type*> paramTypes(argValues.size(), typeManager.getIntType());
            func = Function::Create(
                FunctionType::get(typeManager.getIntType(), paramTypes, false),
                Function::ExternalLinkage, funcName, module.get()
            );
        }
        return builder->CreateCall(func, argValues);
    }

    // Check if the called name is a variable holding a function pointer (anonymous function)
    if (!funcName.empty()) {
        if (auto alloca = getVariable(funcName)) {
            auto funcPtrVal = builder->CreateLoad(typeManager.getIntType(), alloca, funcName + ".val");
            auto funcPtr = builder->CreateIntToPtr(funcPtrVal, typeManager.getPtrType());
            vector<Type*> paramTypes(argValues.size(), typeManager.getIntType());
            auto indirectFuncType = FunctionType::get(typeManager.getIntType(), paramTypes, false);
            return builder->CreateCall(indirectFuncType, funcPtr, argValues);
        }
    }

    // If called expression is not a simple identifier, it might be a computed function pointer
    if (!dynamic_pointer_cast<IdentifierNode>(calledFunc)) {
        auto funcPtrVal = compileExpression(calledFunc);
        auto funcPtr = builder->CreateIntToPtr(funcPtrVal, typeManager.getPtrType());
        vector<Type*> paramTypes(argValues.size(), typeManager.getIntType());
        auto indirectFuncType = FunctionType::get(typeManager.getIntType(), paramTypes, false);
        return builder->CreateCall(indirectFuncType, funcPtr, argValues);
    }

    // Declare as external function with correct parameter count
    vector<Type*> paramTypes(argValues.size(), typeManager.getIntType());
    func = Function::Create(
        FunctionType::get(typeManager.getIntType(), paramTypes, false),
        Function::ExternalLinkage, funcName, module.get()
    );
    return builder->CreateCall(func, argValues);
}

// ============================================================================
// Control Flow - Condition (if/elif/else)
// ============================================================================

llvm::Value* LLVMBackend::compileCondition(const shared_ptr<ConditionNode>& node) {
    auto branches = node->getBranchNodes();
    if (branches.empty()) {
        return ConstantInt::get(typeManager.getIntType(), 0);
    }

    llvm::Function* func = builder->GetInsertBlock()->getParent();

    // Create merge block
    auto mergeBB = BasicBlock::Create(context, "cond.merge");

    // Pre-create all condition evaluation blocks and body blocks
    vector<BasicBlock*> condEvalBBs(branches.size());
    vector<BasicBlock*> bodyBBs(branches.size());
    for (size_t i = 0; i < branches.size(); i++) {
        condEvalBBs[i] = (i == 0) ? builder->GetInsertBlock()
                                   : BasicBlock::Create(context, "cond.eval" + to_string(i));
        bodyBBs[i] = BasicBlock::Create(context, "cond.branch" + to_string(i));
    }

    // Compile each branch
    for (size_t i = 0; i < branches.size(); i++) {
        auto branch = dynamic_pointer_cast<BranchNode>(branches[i]);
        if (!branch) continue;

        if (i > 0) {
            func->getBasicBlockList().push_back(condEvalBBs[i]);
            builder->SetInsertPoint(condEvalBBs[i]);
        }

        // Compile condition
        llvm::Value* cond = nullptr;
        if (branch->getConditionNode()) {
            cond = compileExpression(branch->getConditionNode());
        }

        if (cond) {
            cond = convertToBool(cond);
            BasicBlock* elseBB = (i + 1 < branches.size()) ? condEvalBBs[i + 1] : mergeBB;
            builder->CreateCondBr(cond, bodyBBs[i], elseBB);
        } else {
            builder->CreateBr(bodyBBs[i]);
        }

        // Add body block and compile
        func->getBasicBlockList().push_back(bodyBBs[i]);
        builder->SetInsertPoint(bodyBBs[i]);

        auto body = dynamic_pointer_cast<BlockRangerNode>(branch->getBodyNode());
        if (body) {
            pushScope();
            for (auto& expr : body->getBodyExpressions()) {
                compileExpression(expr);
            }
            popScope();
        }

        // Branch to merge if no terminator
        if (builder->GetInsertBlock()->getTerminator() == nullptr) {
            builder->CreateBr(mergeBB);
        }
    }

    // Add merge block to function
    func->getBasicBlockList().push_back(mergeBB);
    builder->SetInsertPoint(mergeBB);

    return ConstantInt::get(typeManager.getIntType(), 0);
}

// ============================================================================
// Control Flow - While Loop
// ============================================================================

llvm::Value* LLVMBackend::compileWhileLoop(const shared_ptr<WhileLoopNode>& node) {
    auto func = builder->GetInsertBlock()->getParent();

    auto condBB = BasicBlock::Create(context, "while.cond");
    auto bodyBB = BasicBlock::Create(context, "while.body");
    auto endBB = BasicBlock::Create(context, "while.end");

    // Jump to condition block
    builder->CreateBr(condBB);

    // Condition block
    func->getBasicBlockList().push_back(condBB);
    builder->SetInsertPoint(condBB);

    auto cond = compileExpression(node->getConditionNode());
    cond = convertToBool(cond);
    builder->CreateCondBr(cond, bodyBB, endBB);

    // Body block
    func->getBasicBlockList().push_back(bodyBB);
    builder->SetInsertPoint(bodyBB);

    breakTargets.push(endBB);
    continueTargets.push(condBB);

    auto body = dynamic_pointer_cast<BlockRangerNode>(node->getBodyNode());
    if (body) {
        pushScope();
        for (auto& expr : body->getBodyExpressions()) {
            compileExpression(expr);
        }
        popScope();
    }

    breakTargets.pop();
    continueTargets.pop();

    // Loop back to condition if no terminator
    if (builder->GetInsertBlock()->getTerminator() == nullptr) {
        builder->CreateBr(condBB);
    }

    // End block
    func->getBasicBlockList().push_back(endBB);
    builder->SetInsertPoint(endBB);

    return ConstantInt::get(typeManager.getIntType(), 0);
}

// ============================================================================
// Control Flow - Until Loop
// ============================================================================

llvm::Value* LLVMBackend::compileUntilLoop(const shared_ptr<UntilLoopNode>& node) {
    auto func = builder->GetInsertBlock()->getParent();

    auto bodyBB = BasicBlock::Create(context, "until.body");
    auto condBB = BasicBlock::Create(context, "until.cond");
    auto endBB = BasicBlock::Create(context, "until.end");

    // Jump to body block (until executes body first)
    builder->CreateBr(bodyBB);

    // Body block
    func->getBasicBlockList().push_back(bodyBB);
    builder->SetInsertPoint(bodyBB);

    breakTargets.push(endBB);
    continueTargets.push(condBB);

    auto body = dynamic_pointer_cast<BlockRangerNode>(node->getBodyNode());
    if (body) {
        pushScope();
        for (auto& expr : body->getBodyExpressions()) {
            compileExpression(expr);
        }
        popScope();
    }

    breakTargets.pop();
    continueTargets.pop();

    // Jump to condition check
    if (builder->GetInsertBlock()->getTerminator() == nullptr) {
        builder->CreateBr(condBB);
    }

    // Condition block
    func->getBasicBlockList().push_back(condBB);
    builder->SetInsertPoint(condBB);

    auto cond = compileExpression(node->getConditionNode());
    cond = convertToBool(cond);
    // Until: loop while condition is false (exit when true)
    builder->CreateCondBr(cond, endBB, bodyBB);

    // End block
    func->getBasicBlockList().push_back(endBB);
    builder->SetInsertPoint(endBB);

    return ConstantInt::get(typeManager.getIntType(), 0);
}

// ============================================================================
// Control Flow - For Loop (C-style: for(init; cond; update) { body })
// ============================================================================

llvm::Value* LLVMBackend::compileForLoop(const shared_ptr<ForLoopNode>& node) {
    auto func = builder->GetInsertBlock()->getParent();

    auto condBB = BasicBlock::Create(context, "for.cond");
    auto bodyBB = BasicBlock::Create(context, "for.body");
    auto updateBB = BasicBlock::Create(context, "for.update");
    auto endBB = BasicBlock::Create(context, "for.end");

    pushScope();

    // Compile init expression
    if (node->getInitNode()) {
        compileExpression(node->getInitNode());
    }

    // Jump to condition check
    builder->CreateBr(condBB);

    // Condition block
    func->getBasicBlockList().push_back(condBB);
    builder->SetInsertPoint(condBB);

    if (node->getConditionNode()) {
        auto cond = compileExpression(node->getConditionNode());
        cond = convertToBool(cond);
        builder->CreateCondBr(cond, bodyBB, endBB);
    } else {
        // No condition - infinite loop
        builder->CreateBr(bodyBB);
    }

    // Body block
    func->getBasicBlockList().push_back(bodyBB);
    builder->SetInsertPoint(bodyBB);

    breakTargets.push(endBB);
    continueTargets.push(updateBB);

    auto body = dynamic_pointer_cast<BlockRangerNode>(node->getBodyNode());
    if (body) {
        for (auto& expr : body->getBodyExpressions()) {
            compileExpression(expr);
        }
    }

    breakTargets.pop();
    continueTargets.pop();

    // Jump to update block
    if (builder->GetInsertBlock()->getTerminator() == nullptr) {
        builder->CreateBr(updateBB);
    }

    // Update block
    func->getBasicBlockList().push_back(updateBB);
    builder->SetInsertPoint(updateBB);

    if (node->getUpdateNode()) {
        compileExpression(node->getUpdateNode());
    }

    // Jump back to condition
    builder->CreateBr(condBB);

    // End block
    func->getBasicBlockList().push_back(endBB);
    builder->SetInsertPoint(endBB);

    popScope();

    return ConstantInt::get(typeManager.getIntType(), 0);
}

// ============================================================================
// Control Flow - Return
// ============================================================================

llvm::Value* LLVMBackend::compileReturn(const shared_ptr<ReturnExpressionNode>& node) {
    if (node->getReturnNode()) {
        auto val = compileExpression(node->getReturnNode());
        builder->CreateRet(val);
    } else {
        builder->CreateRet(ConstantInt::get(typeManager.getIntType(), 0));
    }
    // Return a sentinel value (code after ret is unreachable)
    return nullptr;
}

// ============================================================================
// Control Flow - Break
// ============================================================================

llvm::Value* LLVMBackend::compileBreak(const shared_ptr<BreakExpressionNode>& /*node*/) {
    if (!breakTargets.empty()) {
        builder->CreateBr(breakTargets.top());
    } else {
        cerr << "Error: break statement outside of loop" << endl;
    }
    return nullptr;
}

llvm::Value* LLVMBackend::compileContinue() {
    if (!continueTargets.empty()) {
        builder->CreateBr(continueTargets.top());
    } else {
        cerr << "Warning: continue outside of loop" << endl;
    }
    return ConstantInt::get(typeManager.getIntType(), 0);
}

// ============================================================================
// Infix Expressions (Binary Operations)
// ============================================================================

llvm::Value* LLVMBackend::compileInfix(const shared_ptr<InfixExpressionNode>& node) {
    auto op = node->getInfixType();

    // Direct assignment via infix (= operator)
    if (op == NodeType::ASSIGNMENT) {
        auto left = dynamic_pointer_cast<IdentifierNode>(node->getLeftNode());
        if (left) {
            auto rightVal = compileExpression(node->getRightNode());
            llvm::Value* alloca = getVariable(left->getName());
            if (!alloca) {
                // Create variable if it doesn't exist
                llvm::Type* varType = typeManager.getIntType();
                if (rightVal->getType()->isDoubleTy()) {
                    varType = typeManager.getFloatType();
                } else if (rightVal->getType()->isPointerTy()) {
                    varType = typeManager.getPtrType();
                }
                alloca = builder->CreateAlloca(varType, nullptr, left->getName());
                setVariable(left->getName(), alloca);
            }
            builder->CreateStore(rightVal, alloca);
            return rightVal;
        }
        // Fall through to compile as regular binary op
    }

    // Compound assignment (+=, -=, *=, /=, %=)
    // Rio parser uses NodeType::ARGUMENT_ASSIGNMENT for these
    if (op == NodeType::COMPOUND || op == NodeType::ARGUMENT_ASSIGNMENT) {
        auto left = dynamic_pointer_cast<IdentifierNode>(node->getLeftNode());
        if (left) {
            string opStr = node->getOpToken().getValue();
            char opChar = opStr.empty() ? '+' : opStr[0];

            llvm::Value* alloca = getVariable(left->getName());
            if (!alloca) {
                cerr << "Error: Undefined variable '" << left->getName() << "' in compound assignment" << endl;
                return ConstantInt::get(typeManager.getIntType(), 0);
            }

            llvm::Value* leftVal = builder->CreateLoad(typeManager.getIntType(), alloca, left->getName() + ".val");
            llvm::Value* rightVal = compileExpression(node->getRightNode());

            llvm::Value* result = nullptr;
            bool isFloat = leftVal->getType()->isDoubleTy() || rightVal->getType()->isDoubleTy();

            // Type promotion for float
            if (isFloat) {
                if (!leftVal->getType()->isDoubleTy())
                    leftVal = builder->CreateSIToFP(leftVal, typeManager.getFloatType());
                if (!rightVal->getType()->isDoubleTy())
                    rightVal = builder->CreateSIToFP(rightVal, typeManager.getFloatType());
            }

            switch (opChar) {
                case '+': result = isFloat ? builder->CreateFAdd(leftVal, rightVal) : builder->CreateAdd(leftVal, rightVal); break;
                case '-': result = isFloat ? builder->CreateFSub(leftVal, rightVal) : builder->CreateSub(leftVal, rightVal); break;
                case '*': result = isFloat ? builder->CreateFMul(leftVal, rightVal) : builder->CreateMul(leftVal, rightVal); break;
                case '/': result = isFloat ? builder->CreateFDiv(leftVal, rightVal) : builder->CreateSDiv(leftVal, rightVal); break;
                case '%': result = isFloat ? builder->CreateFRem(leftVal, rightVal) : builder->CreateSRem(leftVal, rightVal); break;
                case '&': result = builder->CreateAnd(leftVal, rightVal); break;
                case '|': result = builder->CreateOr(leftVal, rightVal); break;
                case '^': result = builder->CreateXor(leftVal, rightVal); break;
                default:
                    cerr << "Error: Unknown compound operator '" << opChar << "'" << endl;
                    result = leftVal;
            }

            if (result) {
                builder->CreateStore(result, alloca);
            }
            return result;
        }
    }

    // Logical AND with short-circuit evaluation
    // Note: Rio parser uses NodeType::LOGICAL for both && and ||
    if (op == NodeType::AND || op == NodeType::LOGICAL) {
        auto tokenType = node->getOpToken().getType();
        bool isAnd = (op == NodeType::AND) || (tokenType == core::TokenType::TOKEN_AND);
        bool isOr = (tokenType == core::TokenType::TOKEN_OR);

        if (isOr) {
            // Logical OR with short-circuit
            auto leftVal = compileExpression(node->getLeftNode());
            auto func = builder->GetInsertBlock()->getParent();

            auto lhsBoolBB = BasicBlock::Create(context, "or.lhs");
            auto rhsBB = BasicBlock::Create(context, "or.rhs");
            auto mergeBB = BasicBlock::Create(context, "or.merge");

            builder->CreateBr(lhsBoolBB);
            func->getBasicBlockList().push_back(lhsBoolBB);
            builder->SetInsertPoint(lhsBoolBB);

            auto leftBool = convertToBool(leftVal);
            builder->CreateCondBr(leftBool, mergeBB, rhsBB);

            func->getBasicBlockList().push_back(rhsBB);
            builder->SetInsertPoint(rhsBB);

            auto rightVal = compileExpression(node->getRightNode());
            auto rightBool = convertToBool(rightVal);
            builder->CreateBr(mergeBB);

            func->getBasicBlockList().push_back(mergeBB);
            builder->SetInsertPoint(mergeBB);

            auto phi = builder->CreatePHI(typeManager.getIntType(), 2);
            phi->addIncoming(ConstantInt::get(typeManager.getIntType(), 1), lhsBoolBB);
            phi->addIncoming(
                builder->CreateZExt(rightBool, typeManager.getIntType()),
                rhsBB
            );
            return phi;
        }

        // Logical AND with short-circuit
        auto leftVal = compileExpression(node->getLeftNode());
        auto func = builder->GetInsertBlock()->getParent();

        auto lhsBoolBB = BasicBlock::Create(context, "and.lhs");
        auto rhsBB = BasicBlock::Create(context, "and.rhs");
        auto mergeBB = BasicBlock::Create(context, "and.merge");

        builder->CreateBr(lhsBoolBB);
        func->getBasicBlockList().push_back(lhsBoolBB);
        builder->SetInsertPoint(lhsBoolBB);

        auto leftBool = convertToBool(leftVal);
        builder->CreateCondBr(leftBool, rhsBB, mergeBB);

        func->getBasicBlockList().push_back(rhsBB);
        builder->SetInsertPoint(rhsBB);

        auto rightVal = compileExpression(node->getRightNode());
        auto rightBool = convertToBool(rightVal);
        builder->CreateBr(mergeBB);

        func->getBasicBlockList().push_back(mergeBB);
        builder->SetInsertPoint(mergeBB);

        auto phi = builder->CreatePHI(typeManager.getIntType(), 2);
        phi->addIncoming(ConstantInt::get(typeManager.getIntType(), 0), lhsBoolBB);
        phi->addIncoming(
            builder->CreateZExt(rightBool, typeManager.getIntType()),
            rhsBB
        );
        return phi;
    }

    // Comparison operations (Rio parser uses NodeType::COMPARE for all comparisons)
    if (op == NodeType::COMPARE) {
        auto leftVal = compileExpression(node->getLeftNode());
        auto rightVal = compileExpression(node->getRightNode());
        bool isFloat = leftVal->getType()->isDoubleTy() || rightVal->getType()->isDoubleTy();
        if (isFloat) {
            if (!leftVal->getType()->isDoubleTy())
                leftVal = builder->CreateSIToFP(leftVal, typeManager.getFloatType());
            if (!rightVal->getType()->isDoubleTy())
                rightVal = builder->CreateSIToFP(rightVal, typeManager.getFloatType());
        }
        llvm::Value* cmp = nullptr;
        auto tokenType = node->getOpToken().getType();
        if (tokenType == core::TokenType::TOKEN_EQUAL || tokenType == core::TokenType::TOKEN_ASSIGN) {
            cmp = isFloat ? builder->CreateFCmpOEQ(leftVal, rightVal) : builder->CreateICmpEQ(leftVal, rightVal);
        } else if (tokenType == core::TokenType::TOKEN_NOT_EQUAL) {
            cmp = isFloat ? builder->CreateFCmpONE(leftVal, rightVal) : builder->CreateICmpNE(leftVal, rightVal);
        } else if (tokenType == core::TokenType::TOKEN_LESS) {
            cmp = isFloat ? builder->CreateFCmpOLT(leftVal, rightVal) : builder->CreateICmpSLT(leftVal, rightVal);
        } else if (tokenType == core::TokenType::TOKEN_GREATER) {
            cmp = isFloat ? builder->CreateFCmpOGT(leftVal, rightVal) : builder->CreateICmpSGT(leftVal, rightVal);
        } else if (tokenType == core::TokenType::TOKEN_LESS_EQUAL) {
            cmp = isFloat ? builder->CreateFCmpOLE(leftVal, rightVal) : builder->CreateICmpSLE(leftVal, rightVal);
        } else if (tokenType == core::TokenType::TOKEN_GREATER_EQUAL) {
            cmp = isFloat ? builder->CreateFCmpOGE(leftVal, rightVal) : builder->CreateICmpSGE(leftVal, rightVal);
        } else {
            cmp = builder->CreateICmpEQ(leftVal, rightVal);
        }
        return builder->CreateZExt(cmp, typeManager.getIntType());
    }

    // Regular binary operation
    auto leftVal = compileExpression(node->getLeftNode());
    auto rightVal = compileExpression(node->getRightNode());

    // String concatenation: if both operands are pointers (strings), use runtime concat
    if (op == NodeType::PLUS && leftVal->getType()->isPointerTy() && rightVal->getType()->isPointerTy()) {
        return builder->CreateCall(stringConcat, {leftVal, rightVal});
    }

    return compileBinaryOp(leftVal, rightVal, op);
}

// ============================================================================
// Prefix Expressions (-, !, ~)
// ============================================================================

llvm::Value* LLVMBackend::compilePrefix(const shared_ptr<PrefixExpressionNode>& node) {
    auto op = node->getPrefixType();
    auto operand = compileExpression(node->getNode());

    switch (op) {
        case NodeType::MINUS:
            if (operand->getType()->isDoubleTy())
                return builder->CreateFNeg(operand);
            return builder->CreateNeg(operand);

        case NodeType::NOT: {
            // Logical not: convert to bool, then invert
            auto boolVal = convertToBool(operand);
            auto inverted = builder->CreateNot(boolVal);
            return builder->CreateZExt(inverted, typeManager.getIntType());
        }

        case NodeType::INCREMENT: {
            // Pre-increment: ++x
            auto target = dynamic_pointer_cast<IdentifierNode>(node->getNode());
            if (target) {
                auto alloca = getVariable(target->getName());
                if (alloca) {
                    auto val = builder->CreateLoad(typeManager.getIntType(), alloca);
                    auto next = builder->CreateAdd(val, ConstantInt::get(typeManager.getIntType(), 1));
                    builder->CreateStore(next, alloca);
                    return next;
                }
            }
            return operand;
        }

        case NodeType::DECREMENT: {
            // Pre-decrement: --x
            auto target = dynamic_pointer_cast<IdentifierNode>(node->getNode());
            if (target) {
                auto alloca = getVariable(target->getName());
                if (alloca) {
                    auto val = builder->CreateLoad(typeManager.getIntType(), alloca);
                    auto prev = builder->CreateSub(val, ConstantInt::get(typeManager.getIntType(), 1));
                    builder->CreateStore(prev, alloca);
                    return prev;
                }
            }
            return operand;
        }

        default:
            return operand;
    }
}

// ============================================================================
// Postfix Expressions (++, --)
// ============================================================================

llvm::Value* LLVMBackend::compilePostfix(const shared_ptr<PostfixExpressionNode>& node) {
    auto op = node->getPostfixType();
    auto target = dynamic_pointer_cast<IdentifierNode>(node->getNode());

    if (!target) {
        cerr << "Warning: Postfix operator on non-identifier" << endl;
        return ConstantInt::get(typeManager.getIntType(), 0);
    }

    auto alloca = getVariable(target->getName());
    if (!alloca) {
        cerr << "Error: Undefined variable '" << target->getName() << "' in postfix expression" << endl;
        return ConstantInt::get(typeManager.getIntType(), 0);
    }

    auto val = builder->CreateLoad(typeManager.getIntType(), alloca, target->getName() + ".val");

    switch (op) {
        case NodeType::INCREMENT: {
            auto next = builder->CreateAdd(val, ConstantInt::get(typeManager.getIntType(), 1));
            builder->CreateStore(next, alloca);
            return val; // Postfix returns old value
        }
        case NodeType::DECREMENT: {
            auto prev = builder->CreateSub(val, ConstantInt::get(typeManager.getIntType(), 1));
            builder->CreateStore(prev, alloca);
            return val; // Postfix returns old value
        }
        default:
            return val;
    }
}

// ============================================================================
// Unary Expressions
// ============================================================================

llvm::Value* LLVMBackend::compileUnary(const shared_ptr<UnaryExpressionNode>& node) {
    auto operand = compileExpression(node->getRightNode());
    string opStr = node->getOpToken().getValue();

    if (opStr == "-" || opStr == "\xE2\x88\x92") { // minus or unicode minus
        if (operand->getType()->isDoubleTy())
            return builder->CreateFNeg(operand);
        return builder->CreateNeg(operand);
    }
    if (opStr == "!" || opStr == "not") {
        auto boolVal = convertToBool(operand);
        auto inverted = builder->CreateNot(boolVal);
        return builder->CreateZExt(inverted, typeManager.getIntType());
    }
    if (opStr == "~") {
        return builder->CreateNot(operand);
    }

    return operand;
}

// ============================================================================
// Compound Expressions
// ============================================================================

llvm::Value* LLVMBackend::compileBlock(const shared_ptr<BlockRangerNode>& node) {
    pushScope();
    for (auto& expr : node->getBodyExpressions()) {
        compileExpression(expr);
    }
    popScope();
    return ConstantInt::get(typeManager.getIntType(), 0);
}

llvm::Value* LLVMBackend::compileList(const shared_ptr<ListExpressionNode>& node) {
    // Create a new list with initial capacity
    auto cap = ConstantInt::get(typeManager.getIntType(), 8);
    auto listPtr = builder->CreateCall(listNew, {cap});

    // If the list has elements, add them
    if (node->getBodyNode()) {
        auto elements = flattenParallel(node->getBodyNode());
        for (auto& elem : elements) {
            auto val = compileExpression(elem);
            // Box integer/float values into RioObject for list storage
            if (val->getType()->isIntegerTy() && !val->getType()->isPointerTy()) {
                // For now, store integers directly as pointer-sized values
                // TODO: proper boxing when GC-based object model is fully integrated
                auto boxed = builder->CreateIntToPtr(val, typeManager.getPtrType());
                builder->CreateCall(listAppend, {listPtr, boxed});
            } else {
                builder->CreateCall(listAppend, {listPtr, val});
            }
        }
    }

    return listPtr;
}

llvm::Value* LLVMBackend::compileParen(const shared_ptr<ParenRangerNode>& node) {
    return compileExpression(node->getRangerNode());
}

llvm::Value* LLVMBackend::compileBracket(const shared_ptr<BracketExpressionNode>& /*node*/) {
    // Bracket expressions (e.g., list literals) - not fully supported
    cerr << "Warning: Bracket expressions not fully supported in LLVM backend" << endl;
    return ConstantPointerNull::get(cast<PointerType>(typeManager.getPtrType()));
}

llvm::Value* LLVMBackend::compileIndex(const shared_ptr<IndexExpressionNode>& node) {
    auto listVal = compileExpression(node->getNode());
    auto indexVal = compileExpression(node->getIndexNode());

    // Call runtime list_get
    auto elemPtr = builder->CreateCall(listGet, {listVal, indexVal});

    // Try to convert back from pointer to integer if possible
    // For now, return the raw pointer value cast to i64
    return builder->CreatePtrToInt(elemPtr, typeManager.getIntType());
}

llvm::Value* LLVMBackend::compileDictionary(const shared_ptr<DictionaryExpressionNode>& /*node*/) {
    // Dictionary literals - not fully supported
    cerr << "Warning: Dictionary literals not fully supported in LLVM backend" << endl;
    return ConstantPointerNull::get(cast<PointerType>(typeManager.getPtrType()));
}

llvm::Value* LLVMBackend::compilePair(const shared_ptr<PairExpressionNode>& /*node*/) {
    // Key-value pairs - not fully supported
    cerr << "Warning: Pair expressions not fully supported in LLVM backend" << endl;
    return ConstantInt::get(typeManager.getIntType(), 0);
}

llvm::Value* LLVMBackend::compilePass(const shared_ptr<PassExpressionNode>& /*node*/) {
    // Pass does nothing
    return ConstantInt::get(typeManager.getIntType(), 0);
}

llvm::Value* LLVMBackend::compileEncapsulated(const shared_ptr<EncapsulatedExpressionNode>& /*node*/) {
    // Encapsulated expressions - placeholder
    return ConstantInt::get(typeManager.getIntType(), 0);
}

// ============================================================================
// Exception Handling (stub implementation)
// ============================================================================

llvm::Value* LLVMBackend::compileTry(const shared_ptr<TryNode>& node) {
    // Try/catch/finally - not fully supported in LLVM backend
    // For now, just compile the try body
    cerr << "Warning: try/catch not fully supported in LLVM backend, compiling try body only" << endl;
    if (node->getTryBody()) {
        pushScope();
        for (auto& expr : node->getTryBody()->getBodyExpressions()) {
            compileExpression(expr);
        }
        popScope();
    }
    return ConstantInt::get(typeManager.getIntType(), 0);
}

llvm::Value* LLVMBackend::compileThrow(const shared_ptr<ThrowNode>& node) {
    // Throw - not fully supported
    cerr << "Warning: throw not fully supported in LLVM backend" << endl;
    if (node->getThrowExpression()) {
        compileExpression(node->getThrowExpression());
    }
    return ConstantInt::get(typeManager.getIntType(), 0);
}

// ============================================================================
// Binary Operation Compilation
// ============================================================================

llvm::Value* LLVMBackend::compileBinaryOp(llvm::Value* left, llvm::Value* right, ast::NodeType op) {
    bool isFloat = left->getType()->isDoubleTy() || right->getType()->isDoubleTy();

    // Type promotion: if either operand is float, promote the other
    if (isFloat) {
        if (!left->getType()->isDoubleTy())
            left = builder->CreateSIToFP(left, typeManager.getFloatType());
        if (!right->getType()->isDoubleTy())
            right = builder->CreateSIToFP(right, typeManager.getFloatType());
    }

    // Arithmetic operations
    switch (op) {
        case NodeType::PLUS:
            return isFloat ? builder->CreateFAdd(left, right) : builder->CreateAdd(left, right);
        case NodeType::MINUS:
            return isFloat ? builder->CreateFSub(left, right) : builder->CreateSub(left, right);
        case NodeType::MULTIPLY:
            return isFloat ? builder->CreateFMul(left, right) : builder->CreateMul(left, right);
        case NodeType::DIVIDE:
            return isFloat ? builder->CreateFDiv(left, right) : builder->CreateSDiv(left, right);
        case NodeType::MODULO:
            return isFloat ? builder->CreateFRem(left, right) : builder->CreateSRem(left, right);
        case NodeType::FLOOR_DIVIDE:
            return isFloat
                ? builder->CreateFPToSI(builder->CreateFDiv(left, right), typeManager.getIntType())
                : builder->CreateSDiv(left, right);
        case NodeType::POWER: {
            // Use llvm.pow intrinsic
            if (isFloat) {
                auto powFunc = Intrinsic::getDeclaration(module.get(), Intrinsic::pow, {typeManager.getFloatType()});
                return builder->CreateCall(powFunc, {left, right});
            }
            // Integer power: convert to float, compute, convert back
            auto leftFP = builder->CreateSIToFP(left, typeManager.getFloatType());
            auto rightFP = builder->CreateSIToFP(right, typeManager.getFloatType());
            auto powFunc = Intrinsic::getDeclaration(module.get(), Intrinsic::pow, {typeManager.getFloatType()});
            auto result = builder->CreateCall(powFunc, {leftFP, rightFP});
            return builder->CreateFPToSI(result, typeManager.getIntType());
        }

        // Comparison operations
    // Note: Rio parser uses NodeType::COMPARE for all comparison operators.
    // The specific comparison type is determined by the opToken.
    case NodeType::COMPARE:
        // This case is handled in compileInfix where we have access to opToken
        return ConstantInt::get(typeManager.getIntType(), 0);
    case NodeType::EQUALITY:
            return isFloat
                ? builder->CreateZExt(builder->CreateFCmpOEQ(left, right), typeManager.getIntType())
                : builder->CreateZExt(builder->CreateICmpEQ(left, right), typeManager.getIntType());
        case NodeType::NOT_EQUAL:
            return isFloat
                ? builder->CreateZExt(builder->CreateFCmpONE(left, right), typeManager.getIntType())
                : builder->CreateZExt(builder->CreateICmpNE(left, right), typeManager.getIntType());
        case NodeType::LESS:
            return isFloat
                ? builder->CreateZExt(builder->CreateFCmpOLT(left, right), typeManager.getIntType())
                : builder->CreateZExt(builder->CreateICmpSLT(left, right), typeManager.getIntType());
        case NodeType::GREATER:
            return isFloat
                ? builder->CreateZExt(builder->CreateFCmpOGT(left, right), typeManager.getIntType())
                : builder->CreateZExt(builder->CreateICmpSGT(left, right), typeManager.getIntType());
        case NodeType::LESS_EQUAL:
            return isFloat
                ? builder->CreateZExt(builder->CreateFCmpOLE(left, right), typeManager.getIntType())
                : builder->CreateZExt(builder->CreateICmpSLE(left, right), typeManager.getIntType());
        case NodeType::GREATER_EQUAL:
            return isFloat
                ? builder->CreateZExt(builder->CreateFCmpOGE(left, right), typeManager.getIntType())
                : builder->CreateZExt(builder->CreateICmpSGE(left, right), typeManager.getIntType());

        // Bitwise operations (integer only) - use AND/OR NodeType values
        case NodeType::AND:
            if (isFloat) {
                cerr << "Warning: Bitwise AND on float operands" << endl;
                return ConstantInt::get(typeManager.getIntType(), 0);
            }
            return builder->CreateAnd(left, right);
        case NodeType::OR:
            if (isFloat) {
                cerr << "Warning: Bitwise OR on float operands" << endl;
                return ConstantInt::get(typeManager.getIntType(), 0);
            }
            return builder->CreateOr(left, right);

        default:
            cerr << "Warning: Unknown binary operator in compileBinaryOp (NodeType=" << static_cast<int>(op) << ")" << endl;
            return ConstantInt::get(typeManager.getIntType(), 0);
    }
}

// ============================================================================
// Comparison Operation (separate for clarity)
// ============================================================================

llvm::Value* LLVMBackend::compileComparison(llvm::Value* left, llvm::Value* right, ast::NodeType op) {
    return compileBinaryOp(left, right, op);
}

// ============================================================================
// Logical Operation (separate for clarity)
// ============================================================================

llvm::Value* LLVMBackend::compileLogicalOp(llvm::Value* left, llvm::Value* right, ast::NodeType op) {
    // Logical ops are handled in compileInfix with short-circuit evaluation
    // This is a fallback for cases where short-circuit is not needed
    auto leftBool = convertToBool(left);
    auto rightBool = convertToBool(right);

    if (op == NodeType::AND) {
        return builder->CreateZExt(builder->CreateAnd(leftBool, rightBool), typeManager.getIntType());
    }
    if (op == NodeType::OR) {
        return builder->CreateZExt(builder->CreateOr(leftBool, rightBool), typeManager.getIntType());
    }

    return ConstantInt::get(typeManager.getIntType(), 0);
}

// ============================================================================
// String Constant Creation
// ============================================================================

llvm::Value* LLVMBackend::createStringConstant(const string& str) {
    // Check cache first
    auto it = stringConstants.find(str);
    if (it != stringConstants.end()) {
        return it->second;
    }

    // Create a global string constant
    auto globalStr = builder->CreateGlobalStringPtr(str, "str_const." + to_string(stringConstants.size()));

    // Call rio_string_new(cstr, len) to create a RioString object
    auto len = ConstantInt::get(typeManager.getIntType(), static_cast<int64_t>(str.size()));
    auto strPtr = builder->CreateCall(stringNew, {globalStr, len});

    // Cache the result
    stringConstants[str] = strPtr;
    return strPtr;
}

// ============================================================================
// sout/print Call Compilation
// ============================================================================

llvm::Value* LLVMBackend::compileSoutCall(const vector<shared_ptr<ExpressionNode>>& args) {
    for (size_t i = 0; i < args.size(); i++) {
        auto& arg = args[i];

        // Try to determine the type at compile time for optimal print call
        if (auto intLit = dynamic_pointer_cast<IntegerLiteralNode>(arg)) {
            (void)intLit;
            auto val = compileExpression(arg);
            builder->CreateCall(printInt, val);
        } else if (auto floatLit = dynamic_pointer_cast<FloatLiteralNode>(arg)) {
            (void)floatLit;
            auto val = compileExpression(arg);
            builder->CreateCall(printFloat, val);
        } else if (auto boolLit = dynamic_pointer_cast<BooleanLiteralNode>(arg)) {
            (void)boolLit;
            auto val = compileExpression(arg);
            // rio_print_bool takes int32_t
            auto boolVal = builder->CreateTrunc(val, Type::getInt32Ty(context));
            builder->CreateCall(printBool, boolVal);
        } else if (auto strLit = dynamic_pointer_cast<StringLiteralNode>(arg)) {
            (void)strLit;
            auto val = compileExpression(arg);
            builder->CreateCall(printString, val);
        } else if (auto charLit = dynamic_pointer_cast<CharacterLiteralNode>(arg)) {
            (void)charLit;
            auto val = compileExpression(arg);
            builder->CreateCall(printInt, val);
        } else if (isNullExpr(arg)) {
            // Print "null" for null values
            auto nullStr = createStringConstant("null");
            builder->CreateCall(printString, nullStr);
        } else {
            // Generic: compile the expression and print based on type
            auto val = compileExpression(arg);
            if (val && val->getType()->isDoubleTy()) {
                builder->CreateCall(printFloat, val);
            } else if (val && val->getType()->isPointerTy()) {
                builder->CreateCall(printObject, val);
            } else {
                builder->CreateCall(printInt, val);
            }
        }
    }

    // Print newline after all arguments
    builder->CreateCall(printNewline);
    return ConstantInt::get(typeManager.getIntType(), 0);
}

// ============================================================================
// Helper: Convert value to i1 (boolean)
// ============================================================================

llvm::Value* LLVMBackend::convertToBool(llvm::Value* value) {
    if (!value) return ConstantInt::get(Type::getInt1Ty(context), 0);

    if (value->getType()->isIntegerTy(1)) {
        return value; // Already i1
    }

    if (value->getType()->isDoubleTy()) {
        // Float: compare with 0.0
        return builder->CreateFCmpONE(
            value, ConstantFP::get(typeManager.getFloatType(), 0.0)
        );
    }

    if (value->getType()->isPointerTy()) {
        // Pointer: compare with null
        return builder->CreateICmpNE(
            value, ConstantPointerNull::get(cast<PointerType>(value->getType()))
        );
    }

    // Integer: compare with 0
    return builder->CreateICmpNE(value, ConstantInt::get(value->getType(), 0));
}

// ============================================================================
// Helper: Flatten ParallelNode (comma-separated list)
// ============================================================================

vector<shared_ptr<ExpressionNode>> LLVMBackend::flattenParallel(const shared_ptr<ExpressionNode>& node) {
    vector<shared_ptr<ExpressionNode>> result;

    if (!node) return result;

    // If the node is a ParenRangerNode, unwrap it first
    shared_ptr<ExpressionNode> unwrapped = node;
    if (auto paren = dynamic_pointer_cast<ParenRangerNode>(node)) {
        unwrapped = paren->getRangerNode();
    }

    // If the node is a BlockRangerNode, it may contain parallel expressions
    if (auto block = dynamic_pointer_cast<BlockRangerNode>(unwrapped)) {
        for (auto& expr : block->getBodyExpressions()) {
            result.push_back(expr);
        }
        return result;
    }

    // If the node is a ListExpressionNode, unwrap its body
    if (auto list = dynamic_pointer_cast<ListExpressionNode>(unwrapped)) {
        return flattenParallel(list->getBodyNode());
    }

    // If the node is an InfixExpressionNode with PARALLEL type (comma-separated),
    // recursively flatten left and right
    if (auto infix = dynamic_pointer_cast<InfixExpressionNode>(unwrapped)) {
        if (infix->getInfixType() == NodeType::PARALLEL) {
            auto left = flattenParallel(infix->getLeftNode());
            auto right = flattenParallel(infix->getRightNode());
            result.insert(result.end(), left.begin(), left.end());
            result.insert(result.end(), right.begin(), right.end());
            return result;
        }
    }

    // Single expression
    result.push_back(unwrapped);
    return result;
}

// ============================================================================
// Type Inference Helpers
// ============================================================================

bool LLVMBackend::isIntegerExpr(const shared_ptr<ExpressionNode>& node) {
    if (!node) return false;
    auto type = node->getType();
    return type == NodeType::INTEGER ||
           type == NodeType::BOOL ||
           type == NodeType::CHAR ||
           type == NodeType::INCREMENT ||
           type == NodeType::DECREMENT;
}

bool LLVMBackend::isFloatExpr(const shared_ptr<ExpressionNode>& node) {
    if (!node) return false;
    return node->getType() == NodeType::FLOAT;
}

bool LLVMBackend::isStringExpr(const shared_ptr<ExpressionNode>& node) {
    if (!node) return false;
    return node->getType() == NodeType::STRING;
}

bool LLVMBackend::isNullExpr(const shared_ptr<ExpressionNode>& node) {
    if (!node) return false;
    return node->getType() == NodeType::NULL_NODE;
}

// ============================================================================
// emitIR - Output LLVM IR to a file
// ============================================================================

bool LLVMBackend::emitIR(const string& outputPath) {
    string err;
    raw_string_ostream errStream(err);
    if (verifyModule(*module, &errStream)) {
        cerr << "Module verification failed: " << err << endl;
        return false;
    }

    error_code EC;
    raw_fd_ostream out(outputPath, EC, sys::fs::OF_None);
    if (EC) {
        cerr << "Cannot open output file '" << outputPath << "': " << EC.message() << endl;
        return false;
    }

    module->print(out, nullptr);
    return true;
}

// ============================================================================
// emitExecutable - Compile to object file and link into executable
// ============================================================================

bool LLVMBackend::emitExecutable(const string& outputPath) {
    // 1. Verify the module
    string err;
    raw_string_ostream errStream(err);
    if (verifyModule(*module, &errStream)) {
        cerr << "LLVM Module verification failed: " << err << endl;
        return false;
    }

    // 2. Initialize all LLVM targets
    InitializeAllTargetInfos();
    InitializeAllTargets();
    InitializeAllTargetMCs();
    InitializeAllAsmParsers();
    InitializeAllAsmPrinters();

    // 3. Set target triple
    auto targetTriple = sys::getDefaultTargetTriple();
    module->setTargetTriple(targetTriple);

    // 4. Look up the target
    string targetError;
    auto target = TargetRegistry::lookupTarget(targetTriple, targetError);
    if (!target) {
        cerr << "Target lookup failed: " << targetError << endl;
        return false;
    }

    // 5. Create target machine
    auto cpu = "generic";
    auto features = "";
    TargetOptions opt;
    auto RM = Optional<Reloc::Model>();
    auto machine = target->createTargetMachine(targetTriple, cpu, features, opt, RM);

    module->setDataLayout(machine->createDataLayout());

    // 6. Generate object file using raw_fd_ostream
    string objPath = outputPath + ".o";
    error_code EC;
    raw_fd_ostream dest(objPath, EC, sys::fs::OF_None);
    if (EC) {
        cerr << "Cannot open object file '" << objPath << "': " << EC.message() << endl;
        return false;
    }

    legacy::PassManager pass;
    auto fileType = CGFT_ObjectFile;

    if (machine->addPassesToEmitFile(pass, dest, nullptr, fileType, false)) {
        cerr << "Cannot add passes to emit object file" << endl;
        return false;
    }

    pass.run(*module);
    dest.flush();

    // 7. Link with runtime library using clang
    string runtimeLibDir = "/workspace/RCC/runtime_lib/build";
    string linkCmd = "clang-15 -no-pie " + objPath
        + " -L" + runtimeLibDir + " -lrio_runtime"
        + " -o " + outputPath
        + " -Wl,-rpath," + runtimeLibDir
        + " -lm";

    int ret = system(linkCmd.c_str());
    if (ret != 0) {
        cerr << "Linking failed with exit code " << ret << endl;
        // Try with cc as fallback
        linkCmd = "cc -no-pie " + objPath
            + " -L" + runtimeLibDir + " -lrio_runtime"
            + " -o " + outputPath
            + " -Wl,-rpath," + runtimeLibDir
            + " -lm";
        ret = system(linkCmd.c_str());
        if (ret != 0) {
            cerr << "Linking with cc also failed with exit code " << ret << endl;
            return false;
        }
    }

    return true;
}

} // namespace backend

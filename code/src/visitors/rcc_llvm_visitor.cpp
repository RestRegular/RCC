//
// Created by RestRegular on 2026/5/5.
//

#include "../../include/visitors/rcc_llvm_visitor.h"

ast::LLVMCodeGenVisitor::LLVMCodeGenVisitor(const std::string &moduleName)
            : ModuleName(moduleName)
{
    InitializeModule();
}

void ast::LLVMCodeGenVisitor::InitializeModule() {
    TheContext = std::make_unique<llvm::LLVMContext>();
    TheModule = std::make_unique<llvm::Module>(ModuleName, *TheContext);
    Builder = std::make_unique<llvm::IRBuilder<>>(*TheContext);

    if (DebugMode) {
      llvm::outs() << "[LLVM] Module initialized: " << ModuleName << "\n";
    }
}

llvm::Module *ast::LLVMCodeGenVisitor::getModule() const  {
    return TheModule.get();
}

void ast::LLVMCodeGenVisitor::printIR() const {
    TheModule->print(llvm::outs(), nullptr);
}

std::string ast::LLVMCodeGenVisitor::getIRString() const {
    std::string str;
    llvm::raw_string_ostream os(str);
    TheModule->print(os, nullptr);
    return str;
}

bool ast::LLVMCodeGenVisitor::verifyModule() const {
    std::string error;
    llvm::raw_string_ostream errorOS(error);
    if (llvm::verifyModule(*TheModule, &errorOS)) {
        llvm::errs() << "Module verification failed:\n" << error;
        return false;
    }
    return true;
}

void ast::LLVMCodeGenVisitor::writeToFile(const std::string &filename) const {
    std::error_code EC;
    llvm::raw_fd_ostream file(filename, EC);
    if (!EC) {
        TheModule->print(file, nullptr);
    }
}

llvm::Type *ast::LLVMCodeGenVisitor::getLLVMType(
const std::shared_ptr<symbol::TypeLabelSymbol> &typeLabel) {
    return Builder->getInt1Ty();
}

llvm::Type *ast::LLVMCodeGenVisitor::getIntType(unsigned bits) const {
    return llvm::Type::getIntNTy(*TheContext, bits);
}

void ast::LLVMCodeGenVisitor::pushValue(llvm::Value *value) {
    if (value) {
        ValueStack.push(value);
    }
}

llvm::Value *ast::LLVMCodeGenVisitor::popValue() {
    if (ValueStack.empty()) {
        return nullptr;
    }
    llvm::Value* v = ValueStack.top();
    ValueStack.pop();
    return v;
}

llvm::Value *ast::LLVMCodeGenVisitor::topValue() const {
    if (ValueStack.empty()) {
        return nullptr;
    }
    return ValueStack.top();
}

llvm::AllocaInst *ast::LLVMCodeGenVisitor::createEntryBlockAlloca(
    llvm::Function *func, const std::string &varName, llvm::Type *type) {
    llvm::IRBuilder<> tmpBuilder(&func->getEntryBlock(),
                                 func->getEntryBlock().begin());
    return tmpBuilder.CreateAlloca(type, nullptr, varName);
}

void ast::LLVMCodeGenVisitor::storeVariable(const std::string &name,
    llvm::Value *value) {
    auto it = NamedValues.find(name);
    if (it != NamedValues.end()) {
        Builder->CreateStore(value, it->second);
    }
}

llvm::Value *ast::LLVMCodeGenVisitor::loadVariable(const std::string &name) {
  if (const auto &it = NamedValues.find(name);
      it != NamedValues.end()) {
        llvm::Type* type = it->second->getAllocatedType();
        return Builder->CreateLoad(type, it->second, name);
    }
    return nullptr;
}

void ast::LLVMCodeGenVisitor::handleLogicalShortCircuit(InfixExpressionNode& node) {
    const Token& opToken = node.getOpToken();
    std::string op = opToken.getValue();

    // 获取当前函数
    llvm::Function* func = Builder->GetInsertBlock()->getParent();

    // 创建基本块
    llvm::BasicBlock* evalRightBB = llvm::BasicBlock::Create(*TheContext, "eval.right", func);
    llvm::BasicBlock* mergeBB = llvm::BasicBlock::Create(*TheContext, "merge", func);

    // 分配结果变量
    llvm::AllocaInst* resultAlloca = Builder->CreateAlloca(
        Builder->getInt1Ty(), nullptr, "logical.result"
    );

    // 访问左操作数
    node.getLeftNode()->acceptVisitor(*this);
    llvm::Value* leftVal = popValue();

    if (op == "&&") {
        // 对于 &&，如果左边为 false，跳过右边
        Builder->CreateCondBr(leftVal, evalRightBB, mergeBB);
    } else {
        // 对于 ||，如果左边为 true，跳过右边
        Builder->CreateCondBr(leftVal, mergeBB, evalRightBB);
    }

    // 右操作数块
    Builder->SetInsertPoint(evalRightBB);
    node.getRightNode()->acceptVisitor(*this);
    llvm::Value* rightVal = popValue();
    Builder->CreateStore(rightVal, resultAlloca);
    Builder->CreateBr(mergeBB);

    // 合并块
    Builder->SetInsertPoint(mergeBB);

    // 从 alloca 加载结果
    llvm::Value* result = Builder->CreateLoad(
        Builder->getInt1Ty(), resultAlloca, "logical.load"
    );
    pushValue(result);
}

void ast::LLVMCodeGenVisitor::enableDebug(bool enable) {
    DebugMode = enable;
}

void ast::LLVMCodeGenVisitor::reset() {
    NamedValues.clear();
    Functions.clear();
    while (!ValueStack.empty()) ValueStack.pop();
    CurrentFunction = nullptr;
    InitializeModule();
}

void ast::LLVMCodeGenVisitor::visitLiteralNode(LiteralNode &node) {}
void ast::LLVMCodeGenVisitor::visitNullLiteralNode(NullLiteralNode &node) {}
void ast::LLVMCodeGenVisitor::visitStringLiteralNode(StringLiteralNode &node) {}
void ast::LLVMCodeGenVisitor::visitNumberLiteralNode(NumberLiteralNode &node) {}
void ast::LLVMCodeGenVisitor::visitIntegerLiteralNode(
    IntegerLiteralNode &node) {}
void ast::LLVMCodeGenVisitor::visitFloatLiteralNode(FloatLiteralNode &node) {}
void ast::LLVMCodeGenVisitor::visitBooleanLiteralNode(
    BooleanLiteralNode &node) {}
void ast::LLVMCodeGenVisitor::visitCharacterLiteralNode(
    CharacterLiteralNode &node) {}
void ast::LLVMCodeGenVisitor::visitIdentifierNode(IdentifierNode &node) {}
void ast::LLVMCodeGenVisitor::visitVariableDefinitionNode(
    VariableDefinitionNode &node) {}
void ast::LLVMCodeGenVisitor::visitAssignmentNode(AssignmentNode &node) {}
void ast::LLVMCodeGenVisitor::visitParameterNode(ParameterNode &node) {}
void ast::LLVMCodeGenVisitor::visitArgumentNode(ArgumentNode &node) {}
void ast::LLVMCodeGenVisitor::visitFunctionDeclarationNode(
    FunctionDeclarationNode &node) {}
void ast::LLVMCodeGenVisitor::visitConstructorDefinitionNode(
    ConstructorDefinitionNode &node) {}
void ast::LLVMCodeGenVisitor::visitClassDeclarationNode(
    ClassDeclarationNode &node) {}
void ast::LLVMCodeGenVisitor::visitClassDefinitionNode(
    ClassDefinitionNode &node) {}
void ast::LLVMCodeGenVisitor::visitFunctionCallNode(FunctionCallNode &node) {}
void ast::LLVMCodeGenVisitor::visitProgramNode(ProgramNode &node) {}
void ast::LLVMCodeGenVisitor::visitInfixNode(InfixExpressionNode &node) {}
void ast::LLVMCodeGenVisitor::visitUnaryExpressionNode(
    UnaryExpressionNode &node) {}
void ast::LLVMCodeGenVisitor::visitExpressionStatementNode(
    ExpressionStatementNode &node) {}
void ast::LLVMCodeGenVisitor::visitPrefixExpressionNode(
    PrefixExpressionNode &node) {}
void ast::LLVMCodeGenVisitor::visitPostfixNode(PostfixExpressionNode &node) {}
void ast::LLVMCodeGenVisitor::visitParenRangerNode(ParenRangerNode &node) {}
void ast::LLVMCodeGenVisitor::visitBlockRangerNode(BlockRangerNode &node) {}
void ast::LLVMCodeGenVisitor::visitFunctionDefinitionNode(
    FunctionDefinitionNode &node) {}
void ast::LLVMCodeGenVisitor::visitLabelNode(LabelNode &node) {}
void ast::LLVMCodeGenVisitor::visitBranchNode(BranchNode &node) {}
void ast::LLVMCodeGenVisitor::visitConditionNode(ConditionNode &node) {}
void ast::LLVMCodeGenVisitor::visitLoopNode(LoopNode &node) {}
void ast::LLVMCodeGenVisitor::visitForLoopNode(ForLoopNode &node) {}
void ast::LLVMCodeGenVisitor::visitPassExpressionNode(
    PassExpressionNode &node) {}
void ast::LLVMCodeGenVisitor::visitEncapsulatedExpressionNode(
    EncapsulatedExpressionNode &node) {}
void ast::LLVMCodeGenVisitor::visitReturnExpressionNode(
    ReturnExpressionNode &node) {}
void ast::LLVMCodeGenVisitor::visitBreakExpressionNode(
    BreakExpressionNode &node) {}
void ast::LLVMCodeGenVisitor::visitAnonFunctionDefinitionNode(
    AnonFunctionDefinitionNode &node) {}
void ast::LLVMCodeGenVisitor::visitPairExpressionNode(
    PairExpressionNode &node) {}
void ast::LLVMCodeGenVisitor::visitDictionaryExpressionNode(
    DictionaryExpressionNode &node) {}
void ast::LLVMCodeGenVisitor::visitListExpressionNode(
    ListExpressionNode &node) {}
void ast::LLVMCodeGenVisitor::visitBracketExpressionNode(
    BracketExpressionNode &node) {}
void ast::LLVMCodeGenVisitor::visitIndexExpressionNode(
    IndexExpressionNode &node) {}
void ast::LLVMCodeGenVisitor::visitTryNode(TryNode &node) {}
void ast::LLVMCodeGenVisitor::visitThrowNode(ThrowNode &node) {}

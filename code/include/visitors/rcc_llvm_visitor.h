//
// Created by RestRegular on 2026/5/5.
//

#ifndef RCC_RCC_LLVM_VISITOR_H
#define RCC_RCC_LLVM_VISITOR_H

#include <memory>
#include <map>
#include <stack>
#include <string>

// LLVM 头文件
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/raw_ostream.h>

// 你的项目头文件
#include "../interfaces/rcc_compile_interface.h"
#include "../components/symbol/rcc_symbol.h"

namespace ast {

    /**
     * LLVM IR 代码生成访问者
     *
     * 将 AST 转换为 LLVM IR
     */
    class LLVMCodeGenVisitor : public Visitor {
    // ==================== LLVM 核心组件 ====================
        std::unique_ptr<llvm::LLVMContext> TheContext;
        std::unique_ptr<llvm::Module> TheModule;
        std::unique_ptr<llvm::IRBuilder<>> Builder;

        // ==================== 符号表 ====================
        // 变量名 -> 栈上分配地址 (alloca)
        std::map<std::string, llvm::AllocaInst*> NamedValues;

        // 函数名 -> LLVM 函数
        std::map<std::string, llvm::Function*> Functions;

        // ==================== 值栈 ====================
        // 用于在 visit 方法之间传递 LLVM Value
        std::stack<llvm::Value*> ValueStack;

        // ==================== 当前上下文 ====================
        // 当前正在构建的函数
        llvm::Function* CurrentFunction = nullptr;

        // 当前模块名
        std::string ModuleName;

        // 是否启用调试输出
        bool DebugMode = false;

    public:
        // ==================== 构造函数 ====================
        explicit LLVMCodeGenVisitor(const std::string& moduleName = "rcc_module");

        // ==================== 初始化 ====================
        void InitializeModule();

        // ==================== 获取生成的 IR ====================

        /**
         * 获取生成的 LLVM Module
         */
        llvm::Module* getModule() const;

        /**
         * 打印 IR 到标准输出
         */
        void printIR() const;

        /**
         * 打印 IR 到字符串
         */
        std::string getIRString() const;

        /**
         * 验证生成的 IR 是否正确
         */
        bool verifyModule() const;

        /**
         * 将 IR 输出到文件
         */
        void writeToFile(const std::string& filename) const;

        // ==================== 类型转换 ====================

        /**
         * 将你的类型系统转换为 LLVM 类型
         */
        llvm::Type* getLLVMType(const std::shared_ptr<symbol::TypeLabelSymbol>& typeLabel);

        /**
         * 获取 LLVM 整数类型（指定位数）
         */
        llvm::Type* getIntType(unsigned bits = 32) const;

        // ==================== 值栈操作 ====================

        void pushValue(llvm::Value* value);

        llvm::Value* popValue();

        llvm::Value* topValue() const;

        // ==================== 符号表操作 ====================

        /**
         * 在当前函数中创建变量分配
         */
        llvm::AllocaInst* createEntryBlockAlloca(llvm::Function* func,
                                                  const std::string& varName,
                                                  llvm::Type* type);

        /**
         * 存储变量值
         */
        void storeVariable(const std::string& name, llvm::Value* value);

        /**
         * 加载变量值
         */
        llvm::Value* loadVariable(const std::string& name);

        void handleLogicalShortCircuit(InfixExpressionNode& node);

    // ==================== 节点访问方法 ====================

        void visitLiteralNode(LiteralNode &node) override;
        void visitNullLiteralNode(NullLiteralNode &node) override;
        void visitStringLiteralNode(StringLiteralNode &node) override;
        void visitNumberLiteralNode(NumberLiteralNode &node) override;
        void visitIntegerLiteralNode(IntegerLiteralNode &node) override;
        void visitFloatLiteralNode(FloatLiteralNode &node) override;
        void visitBooleanLiteralNode(BooleanLiteralNode &node) override;
        void visitCharacterLiteralNode(CharacterLiteralNode &node) override;
        void visitIdentifierNode(IdentifierNode &node) override;
        void visitVariableDefinitionNode(VariableDefinitionNode &node) override;
        void visitAssignmentNode(AssignmentNode &node) override;
        void visitParameterNode(ParameterNode &node) override;
        void visitArgumentNode(ArgumentNode &node) override;
        void visitFunctionDeclarationNode(FunctionDeclarationNode &node) override;
        void visitConstructorDefinitionNode(ConstructorDefinitionNode &node) override;
        void visitClassDeclarationNode(ClassDeclarationNode &node) override;
        void visitClassDefinitionNode(ClassDefinitionNode &node) override;
        void visitFunctionCallNode(FunctionCallNode &node) override;
        void visitProgramNode(ProgramNode &node) override;
        void visitInfixNode(InfixExpressionNode &node) override;
        void visitUnaryExpressionNode(UnaryExpressionNode &node) override;
        void visitExpressionStatementNode(ExpressionStatementNode &node) override;
        void visitPrefixExpressionNode(PrefixExpressionNode &node) override;
        void visitPostfixNode(PostfixExpressionNode &node) override;
        void visitParenRangerNode(ParenRangerNode &node) override;
        void visitBlockRangerNode(BlockRangerNode &node) override;
        void visitFunctionDefinitionNode(FunctionDefinitionNode &node) override;
        void visitLabelNode(LabelNode &node) override;
        void visitBranchNode(BranchNode &node) override;
        void visitConditionNode(ConditionNode &node) override;
        void visitLoopNode(LoopNode &node) override;
        void visitForLoopNode(ForLoopNode &node) override;
        void visitPassExpressionNode(PassExpressionNode &node) override;
        void visitEncapsulatedExpressionNode(EncapsulatedExpressionNode &node) override;
        void visitReturnExpressionNode(ReturnExpressionNode &node) override;
        void visitBreakExpressionNode(BreakExpressionNode &node) override;
        void visitAnonFunctionDefinitionNode(
            AnonFunctionDefinitionNode &node) override;
        void visitPairExpressionNode(PairExpressionNode &node) override;
        void
        visitDictionaryExpressionNode(DictionaryExpressionNode &node) override;
        void visitListExpressionNode(ListExpressionNode &node) override;
        void visitBracketExpressionNode(BracketExpressionNode &node) override;
        void visitIndexExpressionNode(IndexExpressionNode &node) override;
        void visitTryNode(TryNode &node) override;
        void visitThrowNode(ThrowNode &node) override;

        // ==================== 调试工具 ====================

    /**
     * 启用调试模式
     */
    void enableDebug(bool enable = true);

    /**
     * 清空状态，准备重新生成
     */
    void reset();

};

} // namespace ast

#endif // RCC_RCC_LLVM_VISITOR_H

//
// Created by RestRegular on 2026/5/5.
//

#ifndef RCC_RCC_LLVM_VISITOR_H
#define RCC_RCC_LLVM_VISITOR_H

#include <memory>
#include <map>
#include <stack>
#include <string>
#include <vector>
#include <functional>
#include <unordered_set>

// LLVM 头文件
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>
#include <llvm/IR/Verifier.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Constants.h>
#include <llvm/Support/raw_ostream.h>

// 你的项目头文件
#include "../interfaces/rcc_compile_interface.h"
#include "../components/symbol/rcc_symbol.h"
#include "../analyzer/rcc_parser.h"
#include "../analyzer/rcc_lexer.h"

namespace ast {

    /**
     * LLVM IR 代码生成访问者
     *
     * 将 AST 转换为 LLVM IR
     *
     * 设计策略：
     * - 动态类型语言统一使用 opaque pointer (ptr) 作为值类型
     * - 所有值通过 ValueStack 在 visit 方法之间传递
     * - 变量通过 NamedValues 符号表管理（变量名 -> alloca）
     * - 函数通过 Functions 映射管理（函数名 -> llvm::Function）
     * - 控制流通过 BasicBlock 和 phi 节点实现
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

        // ==================== 控制流支持 ====================
        // break 目标 BasicBlock 栈
        std::stack<llvm::BasicBlock*> BreakTargetStack;

        // continue 目标 BasicBlock 栈
        std::stack<llvm::BasicBlock*> ContinueTargetStack;

        // 循环条件 BB 栈（用于 continue 跳回条件判断）
        std::stack<llvm::BasicBlock*> LoopCondBBStack;

        // 当前是否在循环中
        bool InLoop = false;

        // ==================== 内置函数支持 ====================
        // 内置函数 IR 生成器类型
        using BuiltinIRGenerator = std::function<void(
            LLVMCodeGenVisitor&,
            const std::vector<llvm::Value*>& /* args */,
            const std::map<std::string, llvm::Value*>& /* namedArgs */,
            const std::string& /* funcName */)>;

        // 内置函数注册表
        std::map<std::string, BuiltinIRGenerator> BuiltinIRGenerators;

        // 当前正在定义的函数是否为 encapsulated
        bool CurrentFunctionIsEncapsulated = false;

        // 被标记为 encapsulated 的函数名集合
        std::unordered_set<std::string> EncapsulatedFunctions;

        // 导出的符号名集合
        std::unordered_set<std::string> ExportedSymbols;

        // 已导入的模块路径集合（防止循环导入）
        std::unordered_set<std::string> ImportedModules;

        // ==================== 类支持 ====================
        // 类名 -> LLVM StructType
        std::map<std::string, llvm::StructType*> ClassTypes;

        // 类名 -> 方法表 (方法名 -> llvm::Function)
        std::map<std::string, std::map<std::string, llvm::Function*>> ClassMethodTables;

        // 当前正在编译的类名
        std::string CurrentClassName;

        // this 指针的 alloca
        llvm::AllocaInst* ThisAlloca = nullptr;

        // ==================== Tagged Struct 类型系统 ====================

        // 类型标签常量
        static constexpr int64_t TAG_NULL     = 0;
        static constexpr int64_t TAG_INT      = 1;
        static constexpr int64_t TAG_FLOAT    = 2;
        static constexpr int64_t TAG_BOOL     = 3;
        static constexpr int64_t TAG_STRING   = 4;
        static constexpr int64_t TAG_LIST     = 5;
        static constexpr int64_t TAG_DICT     = 6;
        static constexpr int64_t TAG_FUNCTION = 7;

        // RCCValue struct 类型: { i64 type_tag, ptr payload }
        // 在 InitializeModule 中创建一次
        llvm::StructType* RCCValueType = nullptr;

        // 获取 RCCValue struct 的 LLVM 类型
        llvm::StructType* getRCCValueType();

        // 创建一个 Tagged Value: alloca RCCValue, store type_tag and payload, 返回 ptr
        llvm::Value* createTaggedValue(int64_t typeTag, llvm::Value* payload);

        // 创建一个 Tagged Value (payload 为 null)
        llvm::Value* createTaggedValueNull(int64_t typeTag);

        // 创建一个 Tagged Value (payload 为 i64 编码的整数)
        llvm::Value* createTaggedInt(int64_t intVal);

        // 创建一个 Tagged Value (payload 为 double 编码)
        llvm::Value* createTaggedFloat(double floatVal);

        // 创建一个 Tagged Value (payload 为 bool)
        llvm::Value* createTaggedBool(bool boolVal);

        // 创建一个 Tagged Value (payload 为字符串指针)
        llvm::Value* createTaggedString(llvm::Value* strPtr);

        // 从 Tagged Value 中提取 type_tag (i64)
        llvm::Value* extractTag(llvm::Value* taggedPtr);

        // 从 Tagged Value 中提取 payload (ptr)
        llvm::Value* extractPayload(llvm::Value* taggedPtr);

        // 检查 Tagged Value 的类型标签是否等于给定标签，返回 i1
        llvm::Value* checkTag(llvm::Value* taggedPtr, int64_t expectedTag);

        // ==================== 辅助方法 ====================

        /**
         * 获取 void 类型
         */
        llvm::Type* getVoidType() const;

        /**
         * 获取统一值类型 (opaque pointer)
         */
        llvm::PointerType* getValueType() const;

        /**
         * 创建全局字符串常量
         */
        llvm::Value* createGlobalStringPtr(const std::string& str) const;

        /**
         * 获取当前插入点的 BasicBlock
         */
        llvm::BasicBlock* getCurrentBlock() const;

        /**
         * 将值转换为 i1 (布尔条件)
         */
        llvm::Value* coerceToBool(llvm::Value* value);

        /**
         * 获取或创建外部声明函数
         */
        llvm::Function* getOrCreateExternalFunc(
            const std::string& name,
            llvm::FunctionType* funcType) const;

        /**
         * 创建 printf 调用（用于调试输出）
         */
        llvm::Value* createDebugPrint(const std::string& message) const;

        /**
         * 生成未实现节点的警告日志
         */
        void logUnimplemented(const std::string& nodeName) const;

        /**
         * 注册所有内置函数的 IR 生成器
         */
        void registerBuiltinIRGenerators();

        /**
         * 尝试调用内置函数 IR 生成器
         * @return 是否成功调用
         */
        bool tryEmitBuiltinIR(const std::string& funcName,
                               const std::vector<llvm::Value*>& args,
                               const std::map<std::string, llvm::Value*>& namedArgs);

        /**
         * 获取或创建 LLVM printf 函数
         */
        llvm::Function* getPrintfFunction() const;

        /**
         * 获取或创建 LLVM puts 函数
         */
        llvm::Function* getPutsFunction() const;

        /**
         * 创建格式化字符串并调用 printf
         */
        llvm::Value* createPrintfCall(const std::string& format,
                                        const std::vector<llvm::Value*>& args) const;

        /**
         * 编译导入的模块文件并合并到当前 Module
         * @param importPath 导入路径
         * @return 是否成功
         */
        bool compileImportedModule(const std::string& importPath);

        /**
         * 将另一个 Module 的函数声明合并到当前 Module
         */
        void mergeModuleDeclarations(llvm::Module* sourceModule) const;

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
         * 获取当前 Module 的引用（用于模块合并）
         */
        llvm::Module& getModuleRef() const { return *TheModule; }

        /**
         * 获取导出符号集合
         */
        const std::unordered_set<std::string>& getExportedSymbols() const { return ExportedSymbols; }

        /**
         * 标记符号为已导出
         */
        void markExported(const std::string& name) { ExportedSymbols.insert(name); }

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

        /**
         * 编译源文件并生成 LLVM IR
         * @param filePath 源文件路径
         * @return 是否成功
         */
        bool compile(const std::string& filePath);

        // ==================== 类型转换 ====================

        /**
         * 将你的类型系统转换为 LLVM 类型
         */
        llvm::Type* getLLVMType(const std::shared_ptr<symbol::TypeLabelSymbol>& typeLabel) const;

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
        static llvm::AllocaInst* createEntryBlockAlloca(llvm::Function* func,
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

        void handleLogicalShortCircuit(const InfixExpressionNode& node);

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

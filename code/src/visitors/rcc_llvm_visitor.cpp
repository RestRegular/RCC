//
// Created by RestRegular on 2026/5/5.
//

#include "../../include/visitors/rcc_llvm_visitor.h"
#include "../../include/analyzer/rcc_ast_components.h"
#include "../../include/rcc_core.h"

#include <cstring>

// ============================================================================
// 辅助宏：简化调试日志输出
// ============================================================================
#define LLVM_DEBUG(msg)                                                                                                \
    do                                                                                                                 \
    {                                                                                                                  \
        if (DebugMode)                                                                                                 \
        {                                                                                                              \
            llvm::errs() << "[LLVMCodeGen] " << msg << "\n";                                                           \
        }                                                                                                              \
    }                                                                                                                  \
    while (0)

namespace ast
{
    // 匿名函数计数器，用于生成唯一函数名
    static size_t __anonFuncCounter = 0;

    // ============================================================================
    // 构造函数与初始化
    // ============================================================================

    LLVMCodeGenVisitor::LLVMCodeGenVisitor(const std::string& moduleName) : ModuleName(moduleName)
    {
        InitializeModule();
    }

    void LLVMCodeGenVisitor::InitializeModule()
    {
        TheContext = std::make_unique<llvm::LLVMContext>();
        TheModule = std::make_unique<llvm::Module>(ModuleName, *TheContext);
        Builder = std::make_unique<llvm::IRBuilder<>>(*TheContext);

        LLVM_DEBUG("Module initialized: " << ModuleName);
    }

    // ============================================================================
    // IR 获取与输出
    // ============================================================================

    llvm::Module* LLVMCodeGenVisitor::getModule() const { return TheModule.get(); }

    void LLVMCodeGenVisitor::printIR() const { TheModule->print(llvm::outs(), nullptr); }

    std::string LLVMCodeGenVisitor::getIRString() const
    {
        std::string str;
        llvm::raw_string_ostream os(str);
        TheModule->print(os, nullptr);
        return str;
    }

    bool LLVMCodeGenVisitor::verifyModule() const
    {
        std::string error;
        llvm::raw_string_ostream errorOS(error);
        if (llvm::verifyModule(*TheModule, &errorOS))
        {
            llvm::errs() << "Module verification failed:\n" << error;
            return false;
        }
        return true;
    }

    void LLVMCodeGenVisitor::writeToFile(const std::string& filename) const
    {
        std::error_code EC;
        llvm::raw_fd_ostream file(filename, EC);
        if (!EC)
        {
            TheModule->print(file, nullptr);
        }
    }

    // ============================================================================
    // 类型系统
    // ============================================================================

    llvm::Type* LLVMCodeGenVisitor::getLLVMType(const std::shared_ptr<symbol::TypeLabelSymbol>& typeLabel)
    {
        // 动态类型语言：统一使用 opaque pointer
        return getValueType();
    }

    llvm::Type* LLVMCodeGenVisitor::getIntType(unsigned bits) const { return llvm::Type::getIntNTy(*TheContext, bits); }

    llvm::Type* LLVMCodeGenVisitor::getVoidType() const { return llvm::Type::getVoidTy(*TheContext); }

    llvm::PointerType* LLVMCodeGenVisitor::getValueType() const
    {
        // 使用 opaque pointer 作为统一值类型
        return llvm::PointerType::get(*TheContext, 0);
    }

    // ============================================================================
    // 值栈操作
    // ============================================================================

    void LLVMCodeGenVisitor::pushValue(llvm::Value* value)
    {
        if (value)
        {
            ValueStack.push(value);
        }
    }

    llvm::Value* LLVMCodeGenVisitor::popValue()
    {
        if (ValueStack.empty())
        {
            return nullptr;
        }
        llvm::Value* v = ValueStack.top();
        ValueStack.pop();
        return v;
    }

    llvm::Value* LLVMCodeGenVisitor::topValue() const
    {
        if (ValueStack.empty())
        {
            return nullptr;
        }
        return ValueStack.top();
    }

    // ============================================================================
    // 符号表操作
    // ============================================================================

    llvm::AllocaInst* LLVMCodeGenVisitor::createEntryBlockAlloca(llvm::Function* func, const std::string& varName,
                                                                 llvm::Type* type)
    {
        llvm::IRBuilder<> tmpBuilder(&func->getEntryBlock(), func->getEntryBlock().begin());
        return tmpBuilder.CreateAlloca(type, nullptr, varName);
    }

    void LLVMCodeGenVisitor::storeVariable(const std::string& name, llvm::Value* value)
    {
        auto it = NamedValues.find(name);
        if (it != NamedValues.end())
        {
            Builder->CreateStore(value, it->second);
        }
    }

    llvm::Value* LLVMCodeGenVisitor::loadVariable(const std::string& name)
    {
        if (const auto& it = NamedValues.find(name); it != NamedValues.end())
        {
            llvm::Type* type = it->second->getAllocatedType();
            return Builder->CreateLoad(type, it->second, name + ".load");
        }
        return nullptr;
    }

    // ============================================================================
    // 辅助方法
    // ============================================================================

    llvm::Value* LLVMCodeGenVisitor::createGlobalStringPtr(const std::string& str) const
    {
        auto* constant = llvm::ConstantDataArray::getString(*TheContext, str);
        auto* global = new llvm::GlobalVariable(*TheModule, constant->getType(), true,
                                                llvm::GlobalValue::PrivateLinkage, constant, ".str");
        return llvm::ConstantExpr::getBitCast(global, getValueType());
    }

    llvm::BasicBlock* LLVMCodeGenVisitor::getCurrentBlock() const { return Builder->GetInsertBlock(); }

    llvm::Value* LLVMCodeGenVisitor::coerceToBool(llvm::Value* value)
    {
        if (!value)
        {
            return llvm::ConstantInt::getFalse(*TheContext);
        }
        // 如果已经是 i1，直接返回
        if (value->getType()->isIntegerTy(1))
        {
            return value;
        }
        // 如果是整数类型，比较不等于 0
        if (value->getType()->isIntegerTy())
        {
            return Builder->CreateICmpNE(value, llvm::ConstantInt::get(value->getType(), 0), "tobool");
        }
        // 如果是指针类型，比较不等于 null
        if (value->getType()->isPointerTy())
        {
            return Builder->CreateICmpNE(
                value, llvm::ConstantPointerNull::get(llvm::cast<llvm::PointerType>(value->getType())), "tobool");
        }
        // 默认：假设非 null 为 true
        return Builder->CreateICmpNE(value, llvm::Constant::getNullValue(value->getType()), "tobool");
    }

    llvm::Function* LLVMCodeGenVisitor::getOrCreateExternalFunc(const std::string& name, llvm::FunctionType* funcType)
    {
        auto* func = TheModule->getFunction(name);
        if (!func)
        {
            func = llvm::Function::Create(funcType, llvm::Function::ExternalLinkage, name, TheModule.get());
        }
        return func;
    }

    llvm::Value* LLVMCodeGenVisitor::createDebugPrint(const std::string& message)
    {
        // 声明 printf: i32 @printf(ptr, ...)
        auto* printfType =
            llvm::FunctionType::get(llvm::Type::getInt32Ty(*TheContext), {getValueType()}, /*isVarArg=*/true);
        auto* printfFunc = getOrCreateExternalFunc("printf", printfType);

        auto* fmtStr = createGlobalStringPtr(message + "\n");
        return Builder->CreateCall(printfFunc, {fmtStr}, "debug.print");
    }

    void LLVMCodeGenVisitor::logUnimplemented(const std::string& nodeName)
    {
        LLVM_DEBUG("UNIMPLEMENTED: visit" << nodeName);
    }

    void LLVMCodeGenVisitor::enableDebug(bool enable) { DebugMode = enable; }

    void LLVMCodeGenVisitor::reset()
    {
        NamedValues.clear();
        Functions.clear();
        while (!ValueStack.empty())
            ValueStack.pop();
        while (!BreakTargetStack.empty())
            BreakTargetStack.pop();
        while (!ContinueTargetStack.empty())
            ContinueTargetStack.pop();
        while (!LoopCondBBStack.empty())
            LoopCondBBStack.pop();
        CurrentFunction = nullptr;
        InLoop = false;
        InitializeModule();
    }

    // ============================================================================
    // 字面量节点
    // ============================================================================

    void LLVMCodeGenVisitor::visitLiteralNode(LiteralNode& node)
    {
        logUnimplemented("LiteralNode (base class, should not be called directly)");
        pushValue(llvm::ConstantPointerNull::get(getValueType()));
    }

    void LLVMCodeGenVisitor::visitNullLiteralNode(NullLiteralNode& node)
    {
        LLVM_DEBUG("NullLiteralNode: null");
        pushValue(llvm::ConstantPointerNull::get(getValueType()));
    }

    void LLVMCodeGenVisitor::visitStringLiteralNode(StringLiteralNode& node)
    {
        auto* strPtr = createGlobalStringPtr(node.literalString());
        LLVM_DEBUG("StringLiteralNode: \"" << node.literalString() << "\"");
        pushValue(strPtr);
    }

    void LLVMCodeGenVisitor::visitNumberLiteralNode(NumberLiteralNode& node)
    {
        logUnimplemented("NumberLiteralNode (base class)");
        pushValue(llvm::ConstantPointerNull::get(getValueType()));
    }

    void LLVMCodeGenVisitor::visitIntegerLiteralNode(IntegerLiteralNode& node)
    {
        // 整数字面量：将整数值编码为 ptr（inttoptr）
        // 在动态类型系统中，我们使用 inttoptr 将整数值存储为指针
        int64_t intVal = 0;
        try
        {
            intVal = std::stoll(node.literalString());
        }
        catch (...)
        {
            intVal = 0;
        }

        // 将整数值存储为 ptr（使用 inttoptr）
        auto* intConst = llvm::ConstantInt::get(llvm::Type::getInt64Ty(*TheContext), intVal);
        auto* ptrVal = llvm::ConstantExpr::getIntToPtr(intConst, getValueType());

        LLVM_DEBUG("IntegerLiteralNode: " << node.literalString());
        pushValue(ptrVal);
    }

    void LLVMCodeGenVisitor::visitFloatLiteralNode(FloatLiteralNode& node)
    {
        // 浮点字面量：将 double 值的位模式转换为 ptr
        double floatVal = 0.0;
        try
        {
            floatVal = std::stod(node.literalString());
        }
        catch (...)
        {
            floatVal = 0.0;
        }

        // 将 double 的位模式解释为 int64，再 inttoptr
        uint64_t bits = 0;
        std::memcpy(&bits, &floatVal, sizeof(bits));
        auto* intConst = llvm::ConstantInt::get(llvm::Type::getInt64Ty(*TheContext), bits);
        auto* ptrVal = llvm::ConstantExpr::getIntToPtr(intConst, getValueType());

        LLVM_DEBUG("FloatLiteralNode: " << node.literalString());
        pushValue(ptrVal);
    }

    void LLVMCodeGenVisitor::visitBooleanLiteralNode(BooleanLiteralNode& node)
    {
        // 布尔值：true = inttoptr(1), false = inttoptr(0)
        bool boolVal = (node.literalString() == "true");
        auto* intConst = llvm::ConstantInt::get(llvm::Type::getInt64Ty(*TheContext), boolVal ? 1 : 0);
        auto* ptrVal = llvm::ConstantExpr::getIntToPtr(intConst, getValueType());

        LLVM_DEBUG("BooleanLiteralNode: " << node.literalString());
        pushValue(ptrVal);
    }

    void LLVMCodeGenVisitor::visitCharacterLiteralNode(CharacterLiteralNode& node)
    {
        // 字符字面量：将 char 值编码为 ptr
        char charVal = 0;
        if (!node.literalString().empty())
        {
            charVal = node.literalString()[0];
        }

        auto* intConst = llvm::ConstantInt::get(llvm::Type::getInt64Ty(*TheContext), static_cast<int64_t>(charVal));
        auto* ptrVal = llvm::ConstantExpr::getIntToPtr(intConst, getValueType());

        LLVM_DEBUG("CharacterLiteralNode: '" << charVal << "'");
        pushValue(ptrVal);
    }

    // ============================================================================
    // 标识符节点
    // ============================================================================

    void LLVMCodeGenVisitor::visitIdentifierNode(IdentifierNode& node)
    {
        const std::string& name = node.getName();
        auto* val = loadVariable(name);
        if (val)
        {
            LLVM_DEBUG("IdentifierNode: " << name << " (loaded from alloca)");
            pushValue(val);
        }
        else
        {
            // 尝试从函数表中查找
            auto funcIt = Functions.find(name);
            if (funcIt != Functions.end())
            {
                LLVM_DEBUG("IdentifierNode: " << name << " (function reference)");
                pushValue(funcIt->second);
            }
            else
            {
                LLVM_DEBUG("IdentifierNode: " << name << " (not found, using null)");
                pushValue(llvm::ConstantPointerNull::get(getValueType()));
            }
        }
    }

    // ============================================================================
    // 变量定义节点
    // ============================================================================

    void LLVMCodeGenVisitor::visitVariableDefinitionNode(VariableDefinitionNode& node)
    {
        if (!CurrentFunction)
        {
            LLVM_DEBUG("VariableDefinitionNode: no current function, skipping");
            return;
        }

        for (const auto& varDef : node.getVarDefs())
        {
            const auto& nameNode = varDef->getNameNode();
            const std::string& varName = nameNode->getName();

            // 在函数入口创建 alloca
            auto* alloca = createEntryBlockAlloca(CurrentFunction, varName, getValueType());

            // 初始化为 null
            Builder->CreateStore(llvm::ConstantPointerNull::get(getValueType()), alloca);

            // 注册到符号表
            NamedValues[varName] = alloca;

            LLVM_DEBUG("VariableDefinitionNode: " << varName << " (alloca created)");

            // 如果有初始值
            if (varDef->hasInitialValue() && varDef->getValueNode())
            {
                varDef->getValueNode()->acceptVisitor(*this);
                auto* initVal = popValue();
                if (initVal)
                {
                    Builder->CreateStore(initVal, alloca);
                    LLVM_DEBUG("  initialized with value");
                }
            }
        }
    }

    // ============================================================================
    // 赋值节点
    // ============================================================================

    void LLVMCodeGenVisitor::visitAssignmentNode(AssignmentNode& node)
    {
        const auto& [identNode, valueNode] = node.getAssignPair();

        // 先计算右值
        valueNode->acceptVisitor(*this);
        auto* rVal = popValue();

        // 获取左值标识符名
        if (identNode->getRealType() == NodeType::IDENTIFIER)
        {
            auto* ident = static_cast<IdentifierNode*>(identNode.get());
            const std::string& name = ident->getName();

            if (rVal)
            {
                storeVariable(name, rVal);
                LLVM_DEBUG("AssignmentNode: " << name << " = value");
            }
            else
            {
                storeVariable(name, llvm::ConstantPointerNull::get(getValueType()));
                LLVM_DEBUG("AssignmentNode: " << name << " = null (no value)");
            }
            pushValue(rVal ? rVal : llvm::ConstantPointerNull::get(getValueType()));
        }
        else
        {
            LLVM_DEBUG("AssignmentNode: complex lvalue not fully supported");
            pushValue(rVal ? rVal : llvm::ConstantPointerNull::get(getValueType()));
        }
    }

    // ============================================================================
    // 参数与参数节点
    // ============================================================================

    void LLVMCodeGenVisitor::visitParameterNode(ParameterNode& node)
    {
        // 参数在函数定义时处理，此处为占位
        logUnimplemented("ParameterNode (handled in FunctionDefinition)");
    }

    void LLVMCodeGenVisitor::visitArgumentNode(ArgumentNode& node)
    {
        // 参数在函数调用时处理，此处为占位
        logUnimplemented("ArgumentNode (handled in FunctionCall)");
    }

    // ============================================================================
    // 函数声明节点
    // ============================================================================

    void LLVMCodeGenVisitor::visitFunctionDeclarationNode(FunctionDeclarationNode& node)
    {
        // 函数声明：仅声明不定义，创建外部函数原型
        logUnimplemented("FunctionDeclarationNode (forward declaration)");
        pushValue(llvm::ConstantPointerNull::get(getValueType()));
    }

    // ============================================================================
    // 构造函数定义节点
    // ============================================================================

    void LLVMCodeGenVisitor::visitConstructorDefinitionNode(ConstructorDefinitionNode& node)
    {
        logUnimplemented("ConstructorDefinitionNode");
        pushValue(llvm::ConstantPointerNull::get(getValueType()));
    }

    // ============================================================================
    // 类声明与定义节点
    // ============================================================================

    void LLVMCodeGenVisitor::visitClassDeclarationNode(ClassDeclarationNode& node)
    {
        logUnimplemented("ClassDeclarationNode");
        pushValue(llvm::ConstantPointerNull::get(getValueType()));
    }

    void LLVMCodeGenVisitor::visitClassDefinitionNode(ClassDefinitionNode& node)
    {
        logUnimplemented("ClassDefinitionNode");
        pushValue(llvm::ConstantPointerNull::get(getValueType()));
    }

    // ============================================================================
    // 函数调用节点
    // ============================================================================

    void LLVMCodeGenVisitor::visitFunctionCallNode(FunctionCallNode& node)
    {
        // FunctionCallNode 继承自 InfixExpressionNode
        // leftNode = 被调用的函数，rightNode = 参数列表
        const auto& callTarget = node.getLeftNode();
        const auto& argsNode = node.getRightNode();

        // 获取函数名
        std::string funcName;
        if (callTarget->getRealType() == NodeType::IDENTIFIER)
        {
            auto* ident = static_cast<IdentifierNode*>(callTarget.get());
            funcName = ident->getName();
        }

        // 收集参数值
        std::vector<llvm::Value*> argValues;
        if (argsNode)
        {
            // 参数可能是 BlockRangerNode 或 ParenRangerNode
            if (auto* blockRanger = dynamic_cast<BlockRangerNode*>(argsNode.get()))
            {
                for (const auto& expr : blockRanger->getBodyExpressions())
                {
                    expr->acceptVisitor(*this);
                    auto* argVal = popValue();
                    argValues.push_back(argVal ? argVal : llvm::ConstantPointerNull::get(getValueType()));
                }
            }
            else if (auto* parenRanger = dynamic_cast<ParenRangerNode*>(argsNode.get()))
            {
                if (parenRanger->getRangerNode())
                {
                    parenRanger->getRangerNode()->acceptVisitor(*this);
                    auto* argVal = popValue();
                    argValues.push_back(argVal ? argVal : llvm::ConstantPointerNull::get(getValueType()));
                }
            }
            else
            {
                // 直接表达式参数
                argsNode->acceptVisitor(*this);
                auto* argVal = popValue();
                if (argVal)
                {
                    argValues.push_back(argVal);
                }
            }
        }

        // 查找已定义的函数
        auto* callee = TheModule->getFunction(funcName);
        if (!callee)
        {
            // 尝试从 Functions 映射中查找
            auto it = Functions.find(funcName);
            if (it != Functions.end())
            {
                callee = it->second;
            }
        }

        if (callee)
        {
            // 创建调用
            auto* callInst = Builder->CreateCall(callee, argValues, "call." + funcName);
            LLVM_DEBUG("FunctionCallNode: " << funcName << " (" << argValues.size() << " args)");
            pushValue(callInst);
        }
        else
        {
            // 未找到函数：声明为外部函数并调用
            // 外部函数签名: ptr @funcName(ptr, ptr, ...)
            std::vector<llvm::Type*> paramTypes(argValues.size(), getValueType());
            auto* funcType = llvm::FunctionType::get(getValueType(), paramTypes, false);
            callee = getOrCreateExternalFunc(funcName, funcType);

            auto* callInst = Builder->CreateCall(callee, argValues, "call." + funcName);
            LLVM_DEBUG("FunctionCallNode: " << funcName << " (external, " << argValues.size() << " args)");
            pushValue(callInst);
        }
    }

    // ============================================================================
    // 程序节点（入口）
    // ============================================================================

    void LLVMCodeGenVisitor::visitProgramNode(ProgramNode& node)
    {
        LLVM_DEBUG("=== ProgramNode: start ===");

        // 创建 main 函数作为程序入口
        // int main(int argc, char** argv)
        auto* mainType = llvm::FunctionType::get(llvm::Type::getInt32Ty(*TheContext),
                                                 {llvm::Type::getInt32Ty(*TheContext), getValueType()}, false);
        auto* mainFunc = llvm::Function::Create(mainType, llvm::Function::ExternalLinkage, "main", TheModule.get());
        mainFunc->setDSOLocal(true);

        // 创建入口基本块
        auto* entryBB = llvm::BasicBlock::Create(*TheContext, "entry", mainFunc);
        Builder->SetInsertPoint(entryBB);

        // 设置当前函数上下文
        CurrentFunction = mainFunc;
        Functions["main"] = mainFunc;

        // 遍历所有语句
        for (const auto& statement : node.getStatements())
        {
            statement->acceptVisitor(*this);
            // 清空值栈，避免语句间值泄漏
            while (!ValueStack.empty())
                ValueStack.pop();
        }

        // 添加 return 0
        Builder->CreateRet(llvm::ConstantInt::get(llvm::Type::getInt32Ty(*TheContext), 0));

        // 恢复上下文
        CurrentFunction = nullptr;

        LLVM_DEBUG("=== ProgramNode: end ===");
    }

    // ============================================================================
    // 中缀表达式节点
    // ============================================================================

    void LLVMCodeGenVisitor::visitInfixNode(InfixExpressionNode& node)
    {
        const auto& infixType = node.getInfixType();
        const auto& opStr = node.getOpToken().getValue();

        // 逻辑短路求值
        if (infixType == NodeType::AND || infixType == NodeType::OR)
        {
            handleLogicalShortCircuit(node);
            return;
        }

        // 计算左右操作数
        node.getLeftNode()->acceptVisitor(*this);
        auto* left = popValue();
        node.getRightNode()->acceptVisitor(*this);
        auto* right = popValue();

        if (!left || !right)
        {
            LLVM_DEBUG("InfixNode: null operand for " << opStr);
            pushValue(llvm::ConstantPointerNull::get(getValueType()));
            return;
        }

        // 由于我们使用 opaque pointer 存储值，
        // 算术运算需要先将 ptr 转回 int64，运算后再转回 ptr
        auto* int64Ty = llvm::Type::getInt64Ty(*TheContext);

        // ptrtoint: 将 ptr 转为 i64 以进行运算
        auto* leftInt = Builder->CreatePtrToInt(left, int64Ty, "left.int");
        auto* rightInt = Builder->CreatePtrToInt(right, int64Ty, "right.int");

        llvm::Value* result = nullptr;

        switch (infixType)
        {
            // ==================== 算术运算 ====================
        case NodeType::PLUS:
            result = Builder->CreateAdd(leftInt, rightInt, "add");
            break;
        case NodeType::MINUS:
            result = Builder->CreateSub(leftInt, rightInt, "sub");
            break;
        case NodeType::MULTIPLY:
            result = Builder->CreateMul(leftInt, rightInt, "mul");
            break;
        case NodeType::DIVIDE:
            result = Builder->CreateSDiv(leftInt, rightInt, "div");
            break;
        case NodeType::MODULO:
            result = Builder->CreateSRem(leftInt, rightInt, "mod");
            break;
        case NodeType::FLOOR_DIVIDE:
            // 向下取整除法
            result = Builder->CreateSDiv(leftInt, rightInt, "floordiv");
            break;
        case NodeType::POWER:
            // 幂运算：调用外部 pow 函数
            {
                auto* powType = llvm::FunctionType::get(
                    llvm::Type::getDoubleTy(*TheContext),
                    {llvm::Type::getDoubleTy(*TheContext), llvm::Type::getDoubleTy(*TheContext)}, false);
                auto* powFunc = getOrCreateExternalFunc("pow", powType);
                auto* leftDouble = Builder->CreateSIToFP(leftInt, llvm::Type::getDoubleTy(*TheContext));
                auto* rightDouble = Builder->CreateSIToFP(rightInt, llvm::Type::getDoubleTy(*TheContext));
                auto* powResult = Builder->CreateCall(powFunc, {leftDouble, rightDouble}, "pow");
                auto* powInt = Builder->CreateFPToSI(powResult, int64Ty);
                result = powInt;
            }
            break;

            // ==================== 比较运算 ====================
        case NodeType::EQUALITY:
            {
                auto* cmp = Builder->CreateICmpEQ(leftInt, rightInt, "eq");
                result = Builder->CreateZExt(cmp, int64Ty, "eq.ext");
            }
            break;
        case NodeType::NOT_EQUAL:
            {
                auto* cmp = Builder->CreateICmpNE(leftInt, rightInt, "ne");
                result = Builder->CreateZExt(cmp, int64Ty, "ne.ext");
            }
            break;
        case NodeType::LESS:
            {
                auto* cmp = Builder->CreateICmpSLT(leftInt, rightInt, "lt");
                result = Builder->CreateZExt(cmp, int64Ty, "lt.ext");
            }
            break;
        case NodeType::LESS_EQUAL:
            {
                auto* cmp = Builder->CreateICmpSLE(leftInt, rightInt, "le");
                result = Builder->CreateZExt(cmp, int64Ty, "le.ext");
            }
            break;
        case NodeType::GREATER:
            {
                auto* cmp = Builder->CreateICmpSGT(leftInt, rightInt, "gt");
                result = Builder->CreateZExt(cmp, int64Ty, "gt.ext");
            }
            break;
        case NodeType::GREATER_EQUAL:
            {
                auto* cmp = Builder->CreateICmpSGE(leftInt, rightInt, "ge");
                result = Builder->CreateZExt(cmp, int64Ty, "ge.ext");
            }
            break;

            // ==================== 逻辑运算（非短路） ====================
        case NodeType::LOGICAL:
            {
                llvm::Value* boolResult = nullptr;
                if (opStr == "&&")
                {
                    boolResult = Builder->CreateAnd(coerceToBool(left), coerceToBool(right), "land");
                }
                else if (opStr == "||")
                {
                    boolResult = Builder->CreateOr(coerceToBool(left), coerceToBool(right), "lor");
                }
                else
                {
                    boolResult = llvm::ConstantInt::getFalse(*TheContext);
                }
                result = Builder->CreateZExt(boolResult, int64Ty, "logic.ext");
            }
            break;

            // ==================== 比较运算（通用） ====================
        case NodeType::COMPARE:
            {
                // COMPARE 是通用比较节点，通过 opStr 区分具体比较类型
                llvm::Value* cmp = nullptr;
                if (opStr == "==")
                {
                    cmp = Builder->CreateICmpEQ(leftInt, rightInt, "cmp.eq");
                }
                else if (opStr == "!=")
                {
                    cmp = Builder->CreateICmpNE(leftInt, rightInt, "cmp.ne");
                }
                else if (opStr == "<")
                {
                    cmp = Builder->CreateICmpSLT(leftInt, rightInt, "cmp.lt");
                }
                else if (opStr == "<=")
                {
                    cmp = Builder->CreateICmpSLE(leftInt, rightInt, "cmp.le");
                }
                else if (opStr == ">")
                {
                    cmp = Builder->CreateICmpSGT(leftInt, rightInt, "cmp.gt");
                }
                else if (opStr == ">=")
                {
                    cmp = Builder->CreateICmpSGE(leftInt, rightInt, "cmp.ge");
                }
                else
                {
                    cmp = Builder->CreateICmpEQ(leftInt, rightInt, "cmp.default");
                }
                result = Builder->CreateZExt(cmp, int64Ty, "cmp.ext");
            }
            break;

            // ==================== 位运算及其他 ====================
            // 注意：位运算在 NodeType 中没有专门的枚举值，
            // 它们可能映射为 LOGICAL 或其他类型，因此通过 opStr 判断
        default:
            if (opStr == "&")
            {
                result = Builder->CreateAnd(leftInt, rightInt, "bitand");
            }
            else if (opStr == "|")
            {
                result = Builder->CreateOr(leftInt, rightInt, "bitor");
            }
            else if (opStr == "^")
            {
                result = Builder->CreateXor(leftInt, rightInt, "bitxor");
            }
            else if (opStr == "<<")
            {
                result = Builder->CreateShl(leftInt, rightInt, "shl");
            }
            else if (opStr == ">>")
            {
                result = Builder->CreateAShr(leftInt, rightInt, "shr");
            }
            else
            {
                LLVM_DEBUG("InfixNode: unhandled operator: " << opStr);
                result = leftInt;
            }
            break;
        }

        if (result)
        {
            // inttoptr: 将 i64 结果转回 ptr
            auto* resultPtr = Builder->CreateIntToPtr(result, getValueType(), "result.ptr");
            pushValue(resultPtr);
        }
        else
        {
            pushValue(llvm::ConstantPointerNull::get(getValueType()));
        }
    }

    // ============================================================================
    // 逻辑短路求值
    // ============================================================================

    void LLVMCodeGenVisitor::handleLogicalShortCircuit(InfixExpressionNode& node)
    {
        const Token& opToken = node.getOpToken();
        std::string op = opToken.getValue();

        // 获取当前函数
        llvm::Function* func = Builder->GetInsertBlock()->getParent();

        // 创建基本块
        llvm::BasicBlock* evalRightBB = llvm::BasicBlock::Create(*TheContext, "eval.right", func);
        llvm::BasicBlock* mergeBB = llvm::BasicBlock::Create(*TheContext, "merge", func);

        // 记录条件分支前的 BasicBlock（用于 phi 节点）
        llvm::BasicBlock* condBB = getCurrentBlock();

        // 访问左操作数
        node.getLeftNode()->acceptVisitor(*this);
        llvm::Value* leftVal = popValue();
        auto* leftBool = coerceToBool(leftVal);

        if (op == "&&")
        {
            // 对于 &&，如果左边为 false，跳过右边
            Builder->CreateCondBr(leftBool, evalRightBB, mergeBB);
        }
        else
        {
            // 对于 ||，如果左边为 true，跳过右边
            Builder->CreateCondBr(leftBool, mergeBB, evalRightBB);
        }

        // 右操作数块
        Builder->SetInsertPoint(evalRightBB);
        node.getRightNode()->acceptVisitor(*this);
        llvm::Value* rightVal = popValue();
        Builder->CreateBr(mergeBB);

        // 合并块
        Builder->SetInsertPoint(mergeBB);

        // 使用 phi 节点合并结果
        auto* phi = Builder->CreatePHI(getValueType(), 2, "logical.phi");
        if (op == "&&")
        {
            // &&: 左 false -> inttoptr(0), 右 -> rightVal
            phi->addIncoming(llvm::ConstantExpr::getIntToPtr(
                                 llvm::ConstantInt::get(llvm::Type::getInt64Ty(*TheContext), 0), getValueType()),
                             condBB);
            phi->addIncoming(rightVal, evalRightBB);
        }
        else
        {
            // ||: 左 true -> inttoptr(1), 右 -> rightVal
            phi->addIncoming(llvm::ConstantExpr::getIntToPtr(
                                 llvm::ConstantInt::get(llvm::Type::getInt64Ty(*TheContext), 1), getValueType()),
                             condBB);
            phi->addIncoming(rightVal, evalRightBB);
        }

        pushValue(phi);
    }

    // ============================================================================
    // 一元表达式节点
    // ============================================================================

    void LLVMCodeGenVisitor::visitUnaryExpressionNode(UnaryExpressionNode& node)
    {
        const auto& opToken = node.getOpToken();
        const std::string& op = opToken.getValue();

        node.getRightNode()->acceptVisitor(*this);
        auto* operand = popValue();

        if (!operand)
        {
            pushValue(llvm::ConstantPointerNull::get(getValueType()));
            return;
        }

        auto* int64Ty = llvm::Type::getInt64Ty(*TheContext);
        auto* operandInt = Builder->CreatePtrToInt(operand, int64Ty, "unary.int");

        llvm::Value* result = nullptr;

        if (op == "-" || op == "~")
        {
            // 取反
            result = Builder->CreateNeg(operandInt, "neg");
        }
        else if (op == "!")
        {
            // 逻辑非
            auto* boolVal = coerceToBool(operand);
            auto* notVal = Builder->CreateNot(boolVal, "not");
            result = Builder->CreateZExt(notVal, int64Ty, "not.ext");
        }
        else
        {
            LLVM_DEBUG("UnaryExpressionNode: unknown operator: " << op);
            result = operandInt;
        }

        if (result)
        {
            auto* resultPtr = Builder->CreateIntToPtr(result, getValueType(), "unary.ptr");
            pushValue(resultPtr);
        }
        else
        {
            pushValue(llvm::ConstantPointerNull::get(getValueType()));
        }
    }

    // ============================================================================
    // 表达式语句节点
    // ============================================================================

    void LLVMCodeGenVisitor::visitExpressionStatementNode(ExpressionStatementNode& node)
    {
        LLVM_DEBUG("ExpressionStatementNode");
        if (node.getExpression())
        {
            node.getExpression()->acceptVisitor(*this);
        }
    }

    // ============================================================================
    // 前缀表达式节点
    // ============================================================================

    void LLVMCodeGenVisitor::visitPrefixExpressionNode(PrefixExpressionNode& node)
    {
        const auto& prefixType = node.getPrefixType();

        if (prefixType == NodeType::NOT)
        {
            // 逻辑非
            node.getNode()->acceptVisitor(*this);
            auto* val = popValue();
            auto* boolVal = coerceToBool(val);
            auto* notVal = Builder->CreateNot(boolVal, "prefix.not");
            auto* int64Ty = llvm::Type::getInt64Ty(*TheContext);
            auto* result = Builder->CreateZExt(notVal, int64Ty, "prefix.not.ext");
            pushValue(Builder->CreateIntToPtr(result, getValueType()));
        }
        else if (prefixType == NodeType::MINUS)
        {
            // 取负
            node.getNode()->acceptVisitor(*this);
            auto* val = popValue();
            auto* int64Ty = llvm::Type::getInt64Ty(*TheContext);
            auto* valInt = Builder->CreatePtrToInt(val, int64Ty, "prefix.neg.int");
            auto* negVal = Builder->CreateNeg(valInt, "prefix.neg");
            pushValue(Builder->CreateIntToPtr(negVal, getValueType()));
        }
        else
        {
            logUnimplemented("PrefixExpressionNode (type: " + std::to_string(static_cast<int>(prefixType)) + ")");
            node.getNode()->acceptVisitor(*this);
        }
    }

    // ============================================================================
    // 后缀表达式节点
    // ============================================================================

    void LLVMCodeGenVisitor::visitPostfixNode(PostfixExpressionNode& node)
    {
        const auto& postfixType = node.getPostfixType();
        auto* targetNode = node.getNode().get();

        if (targetNode->getRealType() == NodeType::IDENTIFIER)
        {
            auto* ident = static_cast<IdentifierNode*>(targetNode);
            const std::string& name = ident->getName();

            auto* int64Ty = llvm::Type::getInt64Ty(*TheContext);

            if (postfixType == NodeType::INCREMENT)
            {
                // x++
                auto* oldVal = loadVariable(name);
                auto* oldInt = Builder->CreatePtrToInt(oldVal, int64Ty, "post.inc.old");
                auto* newInt = Builder->CreateAdd(oldInt, llvm::ConstantInt::get(int64Ty, 1), "post.inc.new");
                auto* newPtr = Builder->CreateIntToPtr(newInt, getValueType());
                storeVariable(name, newPtr);
                pushValue(oldVal); // 后缀返回旧值
                LLVM_DEBUG("PostfixNode: " << name << "++");
            }
            else if (postfixType == NodeType::DECREMENT)
            {
                // x--
                auto* oldVal = loadVariable(name);
                auto* oldInt = Builder->CreatePtrToInt(oldVal, int64Ty, "post.dec.old");
                auto* newInt = Builder->CreateSub(oldInt, llvm::ConstantInt::get(int64Ty, 1), "post.dec.new");
                auto* newPtr = Builder->CreateIntToPtr(newInt, getValueType());
                storeVariable(name, newPtr);
                pushValue(oldVal); // 后缀返回旧值
                LLVM_DEBUG("PostfixNode: " << name << "--");
            }
            else
            {
                logUnimplemented("PostfixNode (type: " + std::to_string(static_cast<int>(postfixType)) + ")");
                targetNode->acceptVisitor(*this);
            }
        }
        else
        {
            logUnimplemented("PostfixNode (non-identifier target)");
            targetNode->acceptVisitor(*this);
        }
    }

    // ============================================================================
    // 括号表达式节点
    // ============================================================================

    void LLVMCodeGenVisitor::visitParenRangerNode(ParenRangerNode& node)
    {
        if (node.getRangerNode())
        {
            node.getRangerNode()->acceptVisitor(*this);
        }
        else
        {
            pushValue(llvm::ConstantPointerNull::get(getValueType()));
        }
    }

    // ============================================================================
    // 代码块节点
    // ============================================================================

    void LLVMCodeGenVisitor::visitBlockRangerNode(BlockRangerNode& node)
    {
        LLVM_DEBUG("BlockRangerNode: " << node.getBodyExpressions().size() << " expressions");

        // 依次执行块中的表达式
        // 最后一个表达式的值作为块的返回值
        llvm::Value* lastValue = llvm::ConstantPointerNull::get(getValueType());

        for (const auto& expr : node.getBodyExpressions())
        {
            expr->acceptVisitor(*this);
            auto* val = popValue();
            if (val)
            {
                lastValue = val;
            }
        }

        pushValue(lastValue);
    }

    // ============================================================================
    // 函数定义节点
    // ============================================================================

    void LLVMCodeGenVisitor::visitFunctionDefinitionNode(FunctionDefinitionNode& node)
    {
        // 获取函数名
        std::string funcName;
        const auto& callNode = node.getCallNode();

        if (callNode->getRealType() == NodeType::IDENTIFIER)
        {
            auto* ident = static_cast<IdentifierNode*>(callNode.get());
            funcName = ident->getName();
        }
        else if (callNode->getRealType() == NodeType::CALL)
        {
            // func foo(x, y) { ... } 中的 foo(x, y) 是一个 FunctionCallNode
            auto* funcCall = static_cast<FunctionCallNode*>(callNode.get());
            if (funcCall->getLeftNode()->getRealType() == NodeType::IDENTIFIER)
            {
                auto* ident = static_cast<IdentifierNode*>(funcCall->getLeftNode().get());
                funcName = ident->getName();
            }
        }

        if (funcName.empty())
        {
            LLVM_DEBUG("FunctionDefinitionNode: unable to determine function name");
            return;
        }

        LLVM_DEBUG("FunctionDefinitionNode: " << funcName);

        // 收集参数信息
        std::vector<std::string> paramNames;
        const auto& argsNode = callNode->getRealType() == NodeType::CALL
            ? static_cast<FunctionCallNode*>(callNode.get())->getRightNode()
            : nullptr;

        if (argsNode)
        {
            if (auto* parenRanger = dynamic_cast<ParenRangerNode*>(argsNode.get()))
            {
                if (parenRanger->getRangerNode())
                {
                    // 参数可能是 PARALLEL (逗号分隔的标识符列表)
                    if (parenRanger->getRangerNode()->getRealType() == NodeType::PARALLEL)
                    {
                        auto parallel = parenRanger->getRangerNode();
                        // 递归收集 PARALLEL 节点中的标识符
                        std::function<void(const std::shared_ptr<ExpressionNode>&)> collectParams;
                        collectParams = [&](const std::shared_ptr<ExpressionNode>& expr)
                        {
                            if (expr->getRealType() == NodeType::PARALLEL)
                            {
                                auto* p = static_cast<InfixExpressionNode*>(expr.get());
                                collectParams(p->getLeftNode());
                                collectParams(p->getRightNode());
                            }
                            else if (expr->getRealType() == NodeType::IDENTIFIER)
                            {
                                auto* id = static_cast<IdentifierNode*>(expr.get());
                                paramNames.push_back(id->getName());
                            }
                        };
                        collectParams(parallel);
                    }
                    else if (parenRanger->getRangerNode()->getRealType() == NodeType::IDENTIFIER)
                    {
                        auto* id = static_cast<IdentifierNode*>(parenRanger->getRangerNode().get());
                        paramNames.push_back(id->getName());
                    }
                }
            }
        }

        // 创建函数类型: ptr @funcName(ptr, ptr, ...) -> ptr
        std::vector<llvm::Type*> paramTypes(paramNames.size(), getValueType());
        auto* funcType = llvm::FunctionType::get(getValueType(), paramTypes, false);

        // 创建函数
        auto* func = llvm::Function::Create(funcType, llvm::Function::ExternalLinkage, funcName, TheModule.get());

        // 设置参数名
        unsigned idx = 0;
        for (auto& arg : func->args())
        {
            if (idx < paramNames.size())
            {
                arg.setName(paramNames[idx]);
            }
            idx++;
        }

        Functions[funcName] = func;

        // 保存当前上下文
        auto* prevFunction = CurrentFunction;
        auto prevNamedValues = NamedValues;
        auto prevInsertPoint = Builder->saveIP();

        CurrentFunction = func;
        NamedValues.clear();

        // 创建入口基本块
        auto* entryBB = llvm::BasicBlock::Create(*TheContext, "entry", func);
        Builder->SetInsertPoint(entryBB);

        // 为参数创建 alloca 并存储
        unsigned idx = 0;
        for (auto& arg : func->args())
        {
            if (idx < paramNames.size())
            {
                auto* alloca = createEntryBlockAlloca(func, paramNames[idx], getValueType());
                Builder->CreateStore(&arg, alloca);
                NamedValues[paramNames[idx]] = alloca;
            }
            idx++;
        }

        // 生成函数体
        if (node.getBodyNode())
        {
            node.getBodyNode()->acceptVisitor(*this);
        }

        // 如果当前基本块没有终止指令，添加 return null
        if (getCurrentBlock() && !getCurrentBlock()->hasTerminator())
        {
            Builder->CreateRet(llvm::ConstantPointerNull::get(getValueType()));
        }

        // 恢复上下文
        CurrentFunction = prevFunction;
        NamedValues = prevNamedValues;
        Builder->restoreIP(prevInsertPoint);

        LLVM_DEBUG("FunctionDefinitionNode: " << funcName << " (" << paramNames.size() << " params) defined");
    }

    // ============================================================================
    // 标签节点
    // ============================================================================

    void LLVMCodeGenVisitor::visitLabelNode(LabelNode& node)
    {
        logUnimplemented("LabelNode");
        pushValue(llvm::ConstantPointerNull::get(getValueType()));
    }

    // ============================================================================
    // 分支节点（if/elif/else 的单个分支）
    // ============================================================================

    void LLVMCodeGenVisitor::visitBranchNode(BranchNode& node)
    {
        // BranchNode 包含 condition 和 body
        // 在 ConditionNode 中使用，控制流由 ConditionNode 管理
        // 这里仅处理单个分支的条件判断和体执行

        llvm::Function* func = Builder->GetInsertBlock()->getParent();

        // 生成条件
        node.getConditionNode()->acceptVisitor(*this);
        auto* condVal = popValue();
        auto* condBool = coerceToBool(condVal);

        // 创建 then 和 merge 基本块
        auto* thenBB = llvm::BasicBlock::Create(*TheContext, "then", func);
        auto* elseBB = llvm::BasicBlock::Create(*TheContext, "else", func);
        auto* mergeBB = llvm::BasicBlock::Create(*TheContext, "merge", func);

        // 条件分支
        Builder->CreateCondBr(condBool, thenBB, elseBB);

        // then 块
        Builder->SetInsertPoint(thenBB);
        if (node.getBodyNode())
        {
            node.getBodyNode()->acceptVisitor(*this);
        }
        if (!getCurrentBlock()->hasTerminator())
        {
            Builder->CreateBr(mergeBB);
        }
        thenBB = getCurrentBlock(); // 更新 thenBB（可能已被修改）

        // else 块
        Builder->SetInsertPoint(elseBB);
        // else 块为空，直接跳到 merge
        Builder->CreateBr(mergeBB);

        // merge 块
        Builder->SetInsertPoint(mergeBB);

        LLVM_DEBUG("BranchNode: if-else branch generated");
    }

    // ============================================================================
    // 条件节点（if-elif-else 完整结构）
    // ============================================================================

    void LLVMCodeGenVisitor::visitConditionNode(ConditionNode& node)
    {
        llvm::Function* func = Builder->GetInsertBlock()->getParent();
        const auto& branches = node.getBranchNodes();

        if (branches.empty())
        {
            LLVM_DEBUG("ConditionNode: empty condition");
            return;
        }

        // 创建 merge 基本块
        auto* mergeBB = llvm::BasicBlock::Create(*TheContext, "cond.merge", func);

        for (size_t i = 0; i < branches.size(); i++)
        {
            const auto& branch = branches[i];
            auto* branchNode = static_cast<BranchNode*>(branch.get());

            // 获取条件节点
            // 第一个分支是 if，后续是 elif（条件非空）或 else（条件为空）
            auto condExpr = branchNode->getConditionNode();

            if (condExpr)
            {
                // 有条件：生成条件判断
                condExpr->acceptVisitor(*this);
                auto* condVal = popValue();
                auto* condBool = coerceToBool(condVal);

                auto* thenBB = llvm::BasicBlock::Create(*TheContext, "cond.then." + std::to_string(i), func);
                auto* nextBB = llvm::BasicBlock::Create(*TheContext, "cond.next." + std::to_string(i), func);

                Builder->CreateCondBr(condBool, thenBB, nextBB);

                // then 块
                Builder->SetInsertPoint(thenBB);
                if (branchNode->getBodyNode())
                {
                    branchNode->getBodyNode()->acceptVisitor(*this);
                }
                if (!getCurrentBlock()->hasTerminator())
                {
                    Builder->CreateBr(mergeBB);
                }

                // 继续下一个分支
                Builder->SetInsertPoint(nextBB);
            }
            else
            {
                // 无条件：else 分支，直接执行
                if (branchNode->getBodyNode())
                {
                    branchNode->getBodyNode()->acceptVisitor(*this);
                }
                if (!getCurrentBlock()->hasTerminator())
                {
                    Builder->CreateBr(mergeBB);
                }
                break; // else 是最后一个分支
            }
        }

        // 如果当前块没有终止指令，跳到 merge
        if (!getCurrentBlock()->hasTerminator())
        {
            Builder->CreateBr(mergeBB);
        }

        Builder->SetInsertPoint(mergeBB);

        LLVM_DEBUG("ConditionNode: " << branches.size() << " branches generated");
    }

    // ============================================================================
    // 循环节点（while/until）
    // ============================================================================

    void LLVMCodeGenVisitor::visitLoopNode(LoopNode& node)
    {
        llvm::Function* func = Builder->GetInsertBlock()->getParent();
        const auto loopType = node.getLoopType();

        // 创建基本块
        auto* condBB = llvm::BasicBlock::Create(*TheContext, "loop.cond", func);
        auto* bodyBB = llvm::BasicBlock::Create(*TheContext, "loop.body", func);
        auto* afterBB = llvm::BasicBlock::Create(*TheContext, "loop.after", func);

        // 跳到条件判断
        Builder->CreateBr(condBB);

        // 条件块
        Builder->SetInsertPoint(condBB);
        node.getConditionNode()->acceptVisitor(*this);
        auto* condVal = popValue();
        auto* condBool = coerceToBool(condVal);

        // while: 条件为 true 时执行体
        // until: 条件为 false 时执行体（即条件为 true 时退出）
        if (loopType == LoopType::WHILE || loopType == LoopType::FOR)
        {
            Builder->CreateCondBr(condBool, bodyBB, afterBB);
        }
        else
        {
            // UNTIL: 反转条件
            auto* notCond = Builder->CreateNot(condBool, "loop.until.not");
            Builder->CreateCondBr(notCond, bodyBB, afterBB);
        }

        // 设置循环上下文
        BreakTargetStack.push(afterBB);
        ContinueTargetStack.push(condBB);
        LoopCondBBStack.push(condBB);
        InLoop = true;

        // 循环体
        Builder->SetInsertPoint(bodyBB);
        if (node.getBodyNode())
        {
            node.getBodyNode()->acceptVisitor(*this);
        }

        // 循环体结束后跳回条件判断
        if (!getCurrentBlock()->hasTerminator())
        {
            Builder->CreateBr(condBB);
        }

        // 恢复循环上下文
        BreakTargetStack.pop();
        ContinueTargetStack.pop();
        LoopCondBBStack.pop();
        InLoop = false;

        // after 块
        Builder->SetInsertPoint(afterBB);

        LLVM_DEBUG("LoopNode: " << (loopType == LoopType::WHILE ? "while" : "until") << " loop generated");
    }

    // ============================================================================
    // For 循环节点
    // ============================================================================

    void LLVMCodeGenVisitor::visitForLoopNode(ForLoopNode& node)
    {
        llvm::Function* func = Builder->GetInsertBlock()->getParent();

        // 初始化
        if (node.getInitNode())
        {
            node.getInitNode()->acceptVisitor(*this);
        }

        // 创建基本块
        auto* condBB = llvm::BasicBlock::Create(*TheContext, "for.cond", func);
        auto* bodyBB = llvm::BasicBlock::Create(*TheContext, "for.body", func);
        auto* updateBB = llvm::BasicBlock::Create(*TheContext, "for.update", func);
        auto* afterBB = llvm::BasicBlock::Create(*TheContext, "for.after", func);

        // 跳到条件判断
        Builder->CreateBr(condBB);

        // 条件块
        Builder->SetInsertPoint(condBB);
        if (node.getConditionNode())
        {
            node.getConditionNode()->acceptVisitor(*this);
            auto* condVal = popValue();
            auto* condBool = coerceToBool(condVal);
            Builder->CreateCondBr(condBool, bodyBB, afterBB);
        }
        else
        {
            // 无条件：无限循环
            Builder->CreateBr(bodyBB);
        }

        // 设置循环上下文
        BreakTargetStack.push(afterBB);
        ContinueTargetStack.push(updateBB);
        LoopCondBBStack.push(condBB);
        InLoop = true;

        // 循环体
        Builder->SetInsertPoint(bodyBB);
        if (node.getBodyNode())
        {
            node.getBodyNode()->acceptVisitor(*this);
        }
        if (!getCurrentBlock()->hasTerminator())
        {
            Builder->CreateBr(updateBB);
        }

        // 更新块
        Builder->SetInsertPoint(updateBB);
        if (node.getUpdateNode())
        {
            node.getUpdateNode()->acceptVisitor(*this);
        }
        if (!getCurrentBlock()->hasTerminator())
        {
            Builder->CreateBr(condBB);
        }

        // 恢复循环上下文
        BreakTargetStack.pop();
        ContinueTargetStack.pop();
        LoopCondBBStack.pop();
        InLoop = false;

        // after 块
        Builder->SetInsertPoint(afterBB);

        LLVM_DEBUG("ForLoopNode: for loop generated");
    }

    // ============================================================================
    // Pass 节点
    // ============================================================================

    void LLVMCodeGenVisitor::visitPassExpressionNode(PassExpressionNode& node)
    {
        LLVM_DEBUG("PassExpressionNode");
        // pass 不生成任何 IR
        pushValue(llvm::ConstantPointerNull::get(getValueType()));
    }

    // ============================================================================
    // 封装表达式节点
    // ============================================================================

    void LLVMCodeGenVisitor::visitEncapsulatedExpressionNode(EncapsulatedExpressionNode& node)
    {
        LLVM_DEBUG("EncapsulatedExpressionNode: implementation is encapsulated");
        pushValue(llvm::ConstantPointerNull::get(getValueType()));
    }

    // ============================================================================
    // Return 节点
    // ============================================================================

    void LLVMCodeGenVisitor::visitReturnExpressionNode(ReturnExpressionNode& node)
    {
        if (!CurrentFunction)
        {
            LLVM_DEBUG("ReturnExpressionNode: no current function");
            return;
        }

        if (node.getReturnNode())
        {
            node.getReturnNode()->acceptVisitor(*this);
            auto* retVal = popValue();
            Builder->CreateRet(retVal ? retVal : llvm::ConstantPointerNull::get(getValueType()));
        }
        else
        {
            Builder->CreateRet(llvm::ConstantPointerNull::get(getValueType()));
        }

        LLVM_DEBUG("ReturnExpressionNode");
    }

    // ============================================================================
    // Break 节点
    // ============================================================================

    void LLVMCodeGenVisitor::visitBreakExpressionNode(BreakExpressionNode& node)
    {
        if (!BreakTargetStack.empty())
        {
            Builder->CreateBr(BreakTargetStack.top());
            // 创建一个 unreachable 块，因为 break 后的代码不可达
            llvm::Function* func = Builder->GetInsertBlock()->getParent();
            auto* unreachableBB = llvm::BasicBlock::Create(*TheContext, "break.unreachable", func);
            Builder->SetInsertPoint(unreachableBB);
        }
        else
        {
            LLVM_DEBUG("BreakExpressionNode: not inside a loop");
        }

        LLVM_DEBUG("BreakExpressionNode");
    }

    // ============================================================================
    // 匿名函数定义节点
    // ============================================================================

    void LLVMCodeGenVisitor::visitAnonFunctionDefinitionNode(AnonFunctionDefinitionNode& node)
    {
        // 生成唯一函数名
        std::string funcName = "__anon_func_" + std::to_string(__anonFuncCounter++);

        LLVM_DEBUG("AnonFunctionDefinitionNode: " << funcName);

        // 从 getParamNode() 收集参数名（递归遍历 PARALLEL 节点）
        std::vector<std::string> paramNames;
        const auto& paramNode = node.getParamNode();

        if (paramNode)
        {
            // 递归收集 PARALLEL 节点中的标识符
            std::function<void(const std::shared_ptr<ExpressionNode>&)> collectParams;
            collectParams = [&](const std::shared_ptr<ExpressionNode>& expr)
            {
                if (expr->getRealType() == NodeType::PARALLEL)
                {
                    auto* p = static_cast<InfixExpressionNode*>(expr.get());
                    collectParams(p->getLeftNode());
                    collectParams(p->getRightNode());
                }
                else if (expr->getRealType() == NodeType::IDENTIFIER)
                {
                    auto* id = static_cast<IdentifierNode*>(expr.get());
                    paramNames.push_back(id->getName());
                }
            };
            collectParams(paramNode);
        }

        // 创建函数类型: ptr (ptr, ptr, ...) -> ptr
        std::vector<llvm::Type*> paramTypes(paramNames.size(), getValueType());
        auto* funcType = llvm::FunctionType::get(getValueType(), paramTypes, false);

        // 创建函数
        auto* func = llvm::Function::Create(funcType, llvm::Function::PrivateLinkage, funcName, TheModule.get());

        // 设置参数名
        unsigned idx = 0;
        for (auto& arg : func->args())
        {
            if (idx < paramNames.size())
            {
                arg.setName(paramNames[idx]);
            }
            idx++;
        }

        Functions[funcName] = func;

        // 保存当前上下文
        auto* prevFunction = CurrentFunction;
        auto prevNamedValues = NamedValues;
        auto prevInsertPoint = Builder->saveIP();

        CurrentFunction = func;
        NamedValues.clear();

        // 创建入口基本块
        auto* entryBB = llvm::BasicBlock::Create(*TheContext, "entry", func);
        Builder->SetInsertPoint(entryBB);

        // 为参数创建 alloca 并存储
        idx = 0;
        for (auto& arg : func->args())
        {
            if (idx < paramNames.size())
            {
                auto* alloca = createEntryBlockAlloca(func, paramNames[idx], getValueType());
                Builder->CreateStore(&arg, alloca);
                NamedValues[paramNames[idx]] = alloca;
            }
            idx++;
        }

        // 生成函数体
        if (node.getBodyNode())
        {
            node.getBodyNode()->acceptVisitor(*this);
        }

        // 如果当前基本块没有终止指令，添加 return null
        if (getCurrentBlock() && !getCurrentBlock()->hasTerminator())
        {
            Builder->CreateRet(llvm::ConstantPointerNull::get(getValueType()));
        }

        // 恢复上下文
        CurrentFunction = prevFunction;
        NamedValues = prevNamedValues;
        Builder->restoreIP(prevInsertPoint);

        // 将函数指针作为值 push 到栈上
        pushValue(func);

        LLVM_DEBUG("AnonFunctionDefinitionNode: " << funcName << " (" << paramNames.size() << " params) defined");
    }

    // ============================================================================
    // 键值对节点
    // ============================================================================

    void LLVMCodeGenVisitor::visitPairExpressionNode(PairExpressionNode& node)
    {
        LLVM_DEBUG("PairExpressionNode");

        // 分别访问 left (key) 和 right (value)
        node.getLeftNode()->acceptVisitor(*this);
        auto* keyVal = popValue();

        node.getRightNode()->acceptVisitor(*this);
        auto* valueVal = popValue();

        if (!keyVal) keyVal = llvm::ConstantPointerNull::get(getValueType());
        if (!valueVal) valueVal = llvm::ConstantPointerNull::get(getValueType());

        // 调用外部运行时函数 __rcc_pair_create(ptr key, ptr value) -> ptr
        auto* pairFuncType = llvm::FunctionType::get(getValueType(), {getValueType(), getValueType()}, false);
        auto* pairFunc = getOrCreateExternalFunc("__rcc_pair_create", pairFuncType);

        auto* result = Builder->CreateCall(pairFunc, {keyVal, valueVal}, "pair");
        pushValue(result);
    }

    // ============================================================================
    // 字典表达式节点
    // ============================================================================

    void LLVMCodeGenVisitor::visitDictionaryExpressionNode(DictionaryExpressionNode& node)
    {
        LLVM_DEBUG("DictionaryExpressionNode");

        // 收集所有 key-value 对
        std::vector<llvm::Value*> dictArgs;

        const auto& bodyNode = node.getBodyNode();

        if (bodyNode)
        {
            // body 可能是 PARALLEL 节点包含多个 PairExpressionNode
            // 递归遍历 PARALLEL 节点收集所有 PairExpressionNode
            std::vector<std::shared_ptr<ExpressionNode>> pairs;

            std::function<void(const std::shared_ptr<ExpressionNode>&)> collectPairs;
            collectPairs = [&](const std::shared_ptr<ExpressionNode>& expr)
            {
                if (expr->getRealType() == NodeType::PARALLEL)
                {
                    auto* p = static_cast<InfixExpressionNode*>(expr.get());
                    collectPairs(p->getLeftNode());
                    collectPairs(p->getRightNode());
                }
                else
                {
                    pairs.push_back(expr);
                }
            };
            collectPairs(bodyNode);

            // 对每个 PairExpressionNode，分别访问 left (key) 和 right (value)
            for (const auto& pairExpr : pairs)
            {
                if (pairExpr->getRealType() == NodeType::PAIR)
                {
                    auto* pair = static_cast<PairExpressionNode*>(pairExpr.get());
                    pair->getLeftNode()->acceptVisitor(*this);
                    auto* keyVal = popValue();
                    pair->getRightNode()->acceptVisitor(*this);
                    auto* valueVal = popValue();

                    dictArgs.push_back(keyVal ? keyVal : llvm::ConstantPointerNull::get(getValueType()));
                    dictArgs.push_back(valueVal ? valueVal : llvm::ConstantPointerNull::get(getValueType()));
                }
            }
        }

        // 构建参数列表: [count, key0, val0, key1, val1, ...]
        auto* countVal = llvm::ConstantInt::get(llvm::Type::getInt64Ty(*TheContext),
                                                 static_cast<int64_t>(dictArgs.size() / 2));
        std::vector<llvm::Value*> callArgs;
        callArgs.push_back(countVal);
        callArgs.insert(callArgs.end(), dictArgs.begin(), dictArgs.end());

        // 调用外部运行时函数 __rcc_dict_create(i64 count, ptr key0, ptr val0, ...) -> ptr
        // 使用 isVarArg=true 的函数类型
        auto* dictFuncType = llvm::FunctionType::get(getValueType(),
                                                       {llvm::Type::getInt64Ty(*TheContext)},
                                                       /*isVarArg=*/true);
        auto* dictFunc = getOrCreateExternalFunc("__rcc_dict_create", dictFuncType);

        auto* result = Builder->CreateCall(dictFunc, callArgs, "dict");
        pushValue(result);

        LLVM_DEBUG("DictionaryExpressionNode: " << (dictArgs.size() / 2) << " entries");
    }

    // ============================================================================
    // 列表表达式节点
    // ============================================================================

    void LLVMCodeGenVisitor::visitListExpressionNode(ListExpressionNode& node)
    {
        LLVM_DEBUG("ListExpressionNode");

        // 收集所有元素值
        std::vector<llvm::Value*> elemValues;

        const auto& bodyNode = node.getBodyNode();

        if (bodyNode)
        {
            // body 可能是 PARALLEL 节点（逗号分隔的列表）或单个表达式
            std::vector<std::shared_ptr<ExpressionNode>> elements;

            std::function<void(const std::shared_ptr<ExpressionNode>&)> collectElements;
            collectElements = [&](const std::shared_ptr<ExpressionNode>& expr)
            {
                if (expr->getRealType() == NodeType::PARALLEL)
                {
                    auto* p = static_cast<InfixExpressionNode*>(expr.get());
                    collectElements(p->getLeftNode());
                    collectElements(p->getRightNode());
                }
                else
                {
                    elements.push_back(expr);
                }
            };
            collectElements(bodyNode);

            // 遍历所有元素，生成值
            for (const auto& elem : elements)
            {
                elem->acceptVisitor(*this);
                auto* val = popValue();
                elemValues.push_back(val ? val : llvm::ConstantPointerNull::get(getValueType()));
            }
        }

        // 构建参数列表: [count, elem0, elem1, ...]
        auto* countVal = llvm::ConstantInt::get(llvm::Type::getInt64Ty(*TheContext),
                                                 static_cast<int64_t>(elemValues.size()));
        std::vector<llvm::Value*> callArgs;
        callArgs.push_back(countVal);
        callArgs.insert(callArgs.end(), elemValues.begin(), elemValues.end());

        // 调用外部运行时函数 __rcc_list_create(i64 count, ptr elem0, ptr elem1, ...) -> ptr
        // 使用 isVarArg=true 的函数类型
        auto* listFuncType = llvm::FunctionType::get(getValueType(),
                                                       {llvm::Type::getInt64Ty(*TheContext)},
                                                       /*isVarArg=*/true);
        auto* listFunc = getOrCreateExternalFunc("__rcc_list_create", listFuncType);

        auto* result = Builder->CreateCall(listFunc, callArgs, "list");
        pushValue(result);

        LLVM_DEBUG("ListExpressionNode: " << elemValues.size() << " elements");
    }

    // ============================================================================
    // 方括号表达式节点
    // ============================================================================

    void LLVMCodeGenVisitor::visitBracketExpressionNode(BracketExpressionNode& node)
    {
        if (node.getBodyNode())
        {
            node.getBodyNode()->acceptVisitor(*this);
        }
        else
        {
            pushValue(llvm::ConstantPointerNull::get(getValueType()));
        }
    }

    // ============================================================================
    // 索引表达式节点
    // ============================================================================

    void LLVMCodeGenVisitor::visitIndexExpressionNode(IndexExpressionNode& node)
    {
        LLVM_DEBUG("IndexExpressionNode");

        // 分别访问 getLeftNode() (target) 和 getIndexNode() (index)
        node.getLeftNode()->acceptVisitor(*this);
        auto* targetVal = popValue();

        node.getIndexNode()->acceptVisitor(*this);
        auto* indexVal = popValue();

        if (!targetVal) targetVal = llvm::ConstantPointerNull::get(getValueType());
        if (!indexVal) indexVal = llvm::ConstantPointerNull::get(getValueType());

        // 调用外部运行时函数 __rcc_index_get(ptr target, ptr index) -> ptr
        auto* indexFuncType = llvm::FunctionType::get(getValueType(), {getValueType(), getValueType()}, false);
        auto* indexFunc = getOrCreateExternalFunc("__rcc_index_get", indexFuncType);

        auto* result = Builder->CreateCall(indexFunc, {targetVal, indexVal}, "index.get");
        pushValue(result);
    }

    // ============================================================================
    // Try 节点
    // ============================================================================

    void LLVMCodeGenVisitor::visitTryNode(TryNode& node)
    {
        LLVM_DEBUG("TryNode");

        // 声明运行时函数
        auto* tryBeginType = llvm::FunctionType::get(getValueType(), false);
        auto* tryBeginFunc = getOrCreateExternalFunc("__rcc_try_begin", tryBeginType);

        auto* tryEndType = llvm::FunctionType::get(llvm::Type::getInt32Ty(*TheContext), {getValueType()}, false);
        auto* tryEndFunc = getOrCreateExternalFunc("__rcc_try_end", tryEndType);

        auto* tryCatchType = llvm::FunctionType::get(getVoidType(), {getValueType(), getValueType()}, false);
        auto* tryCatchFunc = getOrCreateExternalFunc("__rcc_try_catch", tryCatchType);

        auto* tryFinallyType = llvm::FunctionType::get(getVoidType(), {getValueType()}, false);
        auto* tryFinallyFunc = getOrCreateExternalFunc("__rcc_try_finally", tryFinallyType);

        llvm::Function* func = Builder->GetInsertBlock()->getParent();

        // 调用 __rcc_try_begin 获取 context
        auto* context = Builder->CreateCall(tryBeginFunc, {}, "try.context");

        // 创建基本块
        auto* tryBB = llvm::BasicBlock::Create(*TheContext, "try.body", func);
        auto* catchBB = llvm::BasicBlock::Create(*TheContext, "try.catch", func);
        auto* finallyBB = llvm::BasicBlock::Create(*TheContext, "try.finally", func);
        auto* endBB = llvm::BasicBlock::Create(*TheContext, "try.end", func);

        // 保存 try 块的值栈状态
        auto savedStackSize = ValueStack.size();

        // 执行 try 块
        Builder->CreateBr(tryBB);
        Builder->SetInsertPoint(tryBB);

        if (node.getTryBody())
        {
            node.getTryBody()->acceptVisitor(*this);
        }

        // 检查 try 块是否有终止指令（如 return/break）
        if (!getCurrentBlock()->hasTerminator())
        {
            // 调用 __rcc_try_end 检查是否有异常
            Builder->CreateCall(tryEndFunc, {context}, "try.end.check");
            Builder->CreateBr(finallyBB);
        }

        // catch 块
        Builder->SetInsertPoint(catchBB);

        const auto& catchBodies = node.getCatchBodies();
        if (!catchBodies.empty())
        {
            for (const auto& [catchParam, catchBody] : catchBodies)
            {
                // 创建异常变量 alloca
                if (CurrentFunction)
                {
                    auto* excAlloca = createEntryBlockAlloca(CurrentFunction,
                        catchParam ? catchParam->getName() : "__exception", getValueType());
                    Builder->CreateStore(llvm::ConstantPointerNull::get(getValueType()), excAlloca);
                    NamedValues[catchParam ? catchParam->getName() : "__exception"] = excAlloca;
                }

                // 注册 catch handler
                Builder->CreateCall(tryCatchFunc, {context, llvm::ConstantPointerNull::get(getValueType())});

                // 执行 catch 体
                if (catchBody)
                {
                    catchBody->acceptVisitor(*this);
                }
            }
        }

        if (!getCurrentBlock()->hasTerminator())
        {
            Builder->CreateBr(finallyBB);
        }

        // finally 块
        Builder->SetInsertPoint(finallyBB);
        Builder->CreateCall(tryFinallyFunc, {context});

        if (node.getFinallyBody())
        {
            node.getFinallyBody()->acceptVisitor(*this);
        }

        if (!getCurrentBlock()->hasTerminator())
        {
            Builder->CreateBr(endBB);
        }

        // end 块
        Builder->SetInsertPoint(endBB);

        // 恢复值栈
        while (ValueStack.size() > savedStackSize)
        {
            ValueStack.pop();
        }
        pushValue(llvm::ConstantPointerNull::get(getValueType()));

        LLVM_DEBUG("TryNode: try-catch-finally generated");
    }

    // ============================================================================
    // Throw 节点
    // ============================================================================

    void LLVMCodeGenVisitor::visitThrowNode(ThrowNode& node)
    {
        LLVM_DEBUG("ThrowNode");

        // 声明运行时函数
        auto* throwType = llvm::FunctionType::get(getVoidType(), {getValueType()}, false);
        auto* throwFunc = getOrCreateExternalFunc("__rcc_throw", throwType);

        // 获取异常表达式值
        llvm::Value* excValue = llvm::ConstantPointerNull::get(getValueType());
        if (node.getThrowExpression())
        {
            node.getThrowExpression()->acceptVisitor(*this);
            auto* val = popValue();
            if (val)
            {
                excValue = val;
            }
        }

        // 调用 __rcc_throw
        Builder->CreateCall(throwFunc, {excValue});

        // throw 后不可达
        Builder->CreateUnreachable();

        // 创建新的基本块供后续代码使用（如果有的话）
        llvm::Function* func = Builder->GetInsertBlock()->getParent();
        auto* unreachableBB = llvm::BasicBlock::Create(*TheContext, "throw.unreachable", func);
        Builder->SetInsertPoint(unreachableBB);

        LLVM_DEBUG("ThrowNode: throw generated");
    }

    // ============================================================================
    // 编译入口
    // ============================================================================

    bool LLVMCodeGenVisitor::compile(const std::string& filePath)
    {
        LLVM_DEBUG("=== Compiling: " << filePath << " ===");

        // 词法分析
        const auto& lexer = std::make_shared<lexer::Lexer>(filePath);
        auto tokens = lexer->tokenize();

        // 语法分析
        parser::Parser parser(tokens);
        const auto& [hasError, programNode] = parser.parse();
        if (hasError)
        {
            parser.printParserErrors();
            return false;
        }

        // LLVM IR 生成
        try
        {
            programNode->acceptVisitor(*this);
        }
        catch (const std::exception& e)
        {
            llvm::errs() << "LLVM IR generation error: " << e.what() << "\n";
            return false;
        }

        LLVM_DEBUG("=== Compilation succeeded ===");
        return true;
    }

} // namespace ast

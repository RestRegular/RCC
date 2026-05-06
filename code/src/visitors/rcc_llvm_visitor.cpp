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

        // 创建 RCCValue struct 类型: { i64, ptr }
        RCCValueType = llvm::StructType::create(*TheContext, {llvm::Type::getInt64Ty(*TheContext), getValueType()}, "RCCValue");

        LLVM_DEBUG("Module initialized: " << ModuleName);
        registerBuiltinIRGenerators();
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

    llvm::Type* LLVMCodeGenVisitor::getLLVMType(const std::shared_ptr<symbol::TypeLabelSymbol>& typeLabel) const
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
    // Tagged Struct 类型系统辅助方法
    // ============================================================================

    llvm::StructType* LLVMCodeGenVisitor::getRCCValueType()
    {
        if (!RCCValueType)
        {
            RCCValueType = llvm::StructType::create(*TheContext, {llvm::Type::getInt64Ty(*TheContext), getValueType()}, "RCCValue");
        }
        return RCCValueType;
    }

    llvm::Value* LLVMCodeGenVisitor::createTaggedValue(int64_t typeTag, llvm::Value* payload)
    {
        // 使用 malloc 分配 RCCValue（堆内存，生命周期不受函数返回影响）
        auto* mallocType = llvm::FunctionType::get(getValueType(), {llvm::Type::getInt64Ty(*TheContext)}, false);
        auto* mallocFunc = getOrCreateExternalFunc("malloc", mallocType);
        auto* sizeVal = llvm::ConstantInt::get(llvm::Type::getInt64Ty(*TheContext), 16); // i64 + ptr = 16 bytes
        auto* mem = Builder->CreateCall(mallocFunc, {sizeVal}, "rcc.val.heap");

        // store type_tag
        auto* tagPtr = Builder->CreateStructGEP(getRCCValueType(), mem, 0, "tag.ptr");
        Builder->CreateStore(llvm::ConstantInt::get(llvm::Type::getInt64Ty(*TheContext), typeTag), tagPtr);

        // store payload
        auto* payloadPtr = Builder->CreateStructGEP(getRCCValueType(), mem, 1, "payload.ptr");
        Builder->CreateStore(payload ? payload : llvm::ConstantPointerNull::get(getValueType()), payloadPtr);

        // 返回指向 RCCValue 的指针（堆内存）
        return mem;
    }

    llvm::Value* LLVMCodeGenVisitor::createTaggedValueNull(int64_t typeTag)
    {
        return createTaggedValue(typeTag, llvm::ConstantPointerNull::get(getValueType()));
    }

    llvm::Value* LLVMCodeGenVisitor::createTaggedInt(int64_t intVal)
    {
        // 将整数编码为 ptr (inttoptr)
        auto* intConst = llvm::ConstantInt::get(llvm::Type::getInt64Ty(*TheContext), intVal);
        auto* payload = llvm::ConstantExpr::getIntToPtr(intConst, getValueType());
        return createTaggedValue(TAG_INT, payload);
    }

    llvm::Value* LLVMCodeGenVisitor::createTaggedFloat(double floatVal)
    {
        uint64_t bits = 0;
        std::memcpy(&bits, &floatVal, sizeof(bits));
        auto* intConst = llvm::ConstantInt::get(llvm::Type::getInt64Ty(*TheContext), bits);
        auto* payload = llvm::ConstantExpr::getIntToPtr(intConst, getValueType());
        return createTaggedValue(TAG_FLOAT, payload);
    }

    llvm::Value* LLVMCodeGenVisitor::createTaggedBool(bool boolVal)
    {
        auto* intConst = llvm::ConstantInt::get(llvm::Type::getInt64Ty(*TheContext), boolVal ? 1 : 0);
        auto* payload = llvm::ConstantExpr::getIntToPtr(intConst, getValueType());
        return createTaggedValue(TAG_BOOL, payload);
    }

    llvm::Value* LLVMCodeGenVisitor::createTaggedString(llvm::Value* strPtr)
    {
        return createTaggedValue(TAG_STRING, strPtr);
    }

    llvm::Value* LLVMCodeGenVisitor::extractTag(llvm::Value* taggedPtr)
    {
        auto* tagPtr = Builder->CreateStructGEP(getRCCValueType(), taggedPtr, 0, "tag.ptr");
        return Builder->CreateLoad(llvm::Type::getInt64Ty(*TheContext), tagPtr, "tag");
    }

    llvm::Value* LLVMCodeGenVisitor::extractPayload(llvm::Value* taggedPtr)
    {
        auto* payloadPtr = Builder->CreateStructGEP(getRCCValueType(), taggedPtr, 1, "payload.ptr");
        return Builder->CreateLoad(getValueType(), payloadPtr, "payload");
    }

    llvm::Value* LLVMCodeGenVisitor::checkTag(llvm::Value* taggedPtr, int64_t expectedTag)
    {
        auto* tag = extractTag(taggedPtr);
        auto* expected = llvm::ConstantInt::get(llvm::Type::getInt64Ty(*TheContext), expectedTag);
        return Builder->CreateICmpEQ(tag, expected, "tag.check");
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
        if (!value) return llvm::ConstantInt::getFalse(*TheContext);

        auto* tag = extractTag(value);
        auto* payload = extractPayload(value);
        auto* int64Ty = llvm::Type::getInt64Ty(*TheContext);

        // TAG_NULL → false
        auto* isNull = Builder->CreateICmpEQ(tag, llvm::ConstantInt::get(int64Ty, TAG_NULL), "is_null");

        // TAG_BOOL / TAG_INT → 检查 payload 是否为 0
        auto* isBoolOrInt = Builder->CreateOr(
            Builder->CreateICmpEQ(tag, llvm::ConstantInt::get(int64Ty, TAG_BOOL), "is_bool"),
            Builder->CreateICmpEQ(tag, llvm::ConstantInt::get(int64Ty, TAG_INT), "is_int"),
            "is_bool_or_int");
        auto* payloadInt = Builder->CreatePtrToInt(payload, int64Ty, "payload.int");
        auto* payloadNonZero = Builder->CreateICmpNE(payloadInt, llvm::ConstantInt::get(int64Ty, 0), "payload_nz");

        // 结果: isNull ? false : (isBoolOrInt ? payloadNonZero : true)
        auto* falseVal = llvm::ConstantInt::getFalse(*TheContext);
        auto* trueVal = llvm::ConstantInt::getTrue(*TheContext);

        auto* notNull = Builder->CreateNot(isNull, "not_null");
        auto* boolOrIntResult = Builder->CreateSelect(isBoolOrInt, payloadNonZero, trueVal, "bool_int_result");
        return Builder->CreateSelect(isNull, falseVal, boolOrIntResult, "coerced_bool");
    }

    llvm::Function* LLVMCodeGenVisitor::getOrCreateExternalFunc(const std::string& name, llvm::FunctionType* funcType) const
    {
        auto* func = TheModule->getFunction(name);
        if (!func)
        {
            func = llvm::Function::Create(funcType, llvm::Function::ExternalLinkage, name, TheModule.get());
        }
        return func;
    }

    llvm::Value* LLVMCodeGenVisitor::createDebugPrint(const std::string& message) const
    {
        // 声明 printf: i32 @printf(ptr, ...)
        auto* printfType =
            llvm::FunctionType::get(llvm::Type::getInt32Ty(*TheContext), {getValueType()}, /*isVarArg=*/true);
        auto* printfFunc = getOrCreateExternalFunc("printf", printfType);

        auto* fmtStr = createGlobalStringPtr(message + "\n");
        return Builder->CreateCall(printfFunc, {fmtStr}, "debug.print");
    }

    void LLVMCodeGenVisitor::logUnimplemented(const std::string& nodeName) const
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
        LLVM_DEBUG("NullLiteralNode");
        pushValue(createTaggedValueNull(TAG_NULL));
    }

    void LLVMCodeGenVisitor::visitStringLiteralNode(StringLiteralNode& node)
    {
        auto* strPtr = createGlobalStringPtr(node.unescapedString());
        LLVM_DEBUG("StringLiteralNode: " << node.literalString() << "");
        pushValue(createTaggedString(strPtr));
    }

    void LLVMCodeGenVisitor::visitNumberLiteralNode(NumberLiteralNode& node)
    {
        logUnimplemented("NumberLiteralNode (base class)");
        pushValue(llvm::ConstantPointerNull::get(getValueType()));
    }

    void LLVMCodeGenVisitor::visitIntegerLiteralNode(IntegerLiteralNode& node)
    {
        LLVM_DEBUG("IntegerLiteralNode: " << node.literalString());
        int64_t intVal = std::stoll(node.literalString());
        pushValue(createTaggedInt(intVal));
    }

    void LLVMCodeGenVisitor::visitFloatLiteralNode(FloatLiteralNode& node)
    {
        LLVM_DEBUG("FloatLiteralNode: " << node.literalString());
        double floatVal = std::stod(node.literalString());
        pushValue(createTaggedFloat(floatVal));
    }

    void LLVMCodeGenVisitor::visitBooleanLiteralNode(BooleanLiteralNode& node)
    {
        LLVM_DEBUG("BooleanLiteralNode: " << node.literalString());
        const bool boolVal = node.literalString() == "true";
        pushValue(createTaggedBool(boolVal));
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
        if (auto* val = loadVariable(name))
        {
            LLVM_DEBUG("IdentifierNode: " << name << " (loaded from alloca)");
            pushValue(val);
        }
        else
        {
            // 尝试从函数表中查找
            if (const auto funcIt = Functions.find(name); funcIt != Functions.end())
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
                if (auto* initVal = popValue())
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
        else if (identNode->getRealType() == NodeType::ATTRIBUTE_EXPRESSION)
        {
            // 属性赋值: obj.attr = value
            auto* attrExpr = static_cast<InfixExpressionNode*>(identNode.get());

            // 获取对象
            attrExpr->getLeftNode()->acceptVisitor(*this);
            auto* objVal = popValue();

            // 获取属性名
            std::string attrName;
            if (auto* ident = dynamic_cast<IdentifierNode*>(attrExpr->getRightNode().get()))
            {
                attrName = ident->getName();
            }
            else
            {
                LLVM_DEBUG("AssignmentNode: attribute name is not an identifier");
                pushValue(rVal ? rVal : llvm::ConstantPointerNull::get(getValueType()));
                return;
            }

            // 确定类名，并判断对象是否为 Tagged Value
            std::string className;
            bool isTaggedValue = true;
            llvm::Value* objPayload = nullptr;
            
            if (auto* ident = dynamic_cast<IdentifierNode*>(attrExpr->getLeftNode().get()))
            {
                if (ident->getName() == "this" && !CurrentClassName.empty())
                {
                    // 在构造函数/方法中，this 是直接的类实例指针，不是 Tagged Value
                    className = CurrentClassName;
                    isTaggedValue = false;
                    objPayload = objVal;
                }
            }
            
            // 如果不在类上下文中，需要从 Tagged Value 提取
            if (isTaggedValue)
            {
                objPayload = extractPayload(objVal);
                // 尝试推断类名（从变量名或字段查找）
                // 简化：这里使用之前的推断逻辑
                if (auto* ident = dynamic_cast<IdentifierNode*>(attrExpr->getLeftNode().get()))
                {
                    std::string varName = ident->getName();
                    if (ClassTypes.count(varName))
                    {
                        className = varName;
                    }
                    else if (!varName.empty())
                    {
                        std::string capitalized = varName;
                        capitalized[0] = std::toupper(capitalized[0]);
                        if (ClassTypes.count(capitalized))
                        {
                            className = capitalized;
                        }
                    }
                }
            }

            if (!className.empty())
            {
                auto it = ClassFieldNames.find(className);
                if (it != ClassFieldNames.end())
                {
                    const auto& fieldNames = it->second;
                    int fieldIndex = -1;
                    for (size_t i = 0; i < fieldNames.size(); ++i)
                    {
                        if (fieldNames[i] == attrName)
                        {
                            fieldIndex = static_cast<int>(i);
                            break;
                        }
                    }

                    if (fieldIndex >= 0)
                    {
                        // 获取类的 StructType
                        auto* structType = ClassTypes[className];

                        // 计算字段地址
                        auto* fieldPtr = Builder->CreateStructGEP(structType, objPayload, fieldIndex, attrName + ".ptr");

                        // 存储值到字段
                        Builder->CreateStore(rVal ? rVal : llvm::ConstantPointerNull::get(getValueType()), fieldPtr);

                        LLVM_DEBUG("AssignmentNode: " << className << "." << attrName << " = value [index " << fieldIndex << "]");
                        pushValue(rVal ? rVal : llvm::ConstantPointerNull::get(getValueType()));
                        return;
                    }
                }
            }

            LLVM_DEBUG("AssignmentNode: attribute assignment failed - " << attrName);
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
        if (CurrentClassName.empty())
        {
            LLVM_DEBUG("ConstructorDefinitionNode: not inside a class");
            return;
        }

        LLVM_DEBUG("ConstructorDefinitionNode: " << CurrentClassName);

        // 收集参数名
        std::vector<std::string> paramNames;
        if (node.getParamNode())
        {
            if (auto* parenRanger = dynamic_cast<ParenRangerNode*>(node.getParamNode().get()))
            {
                if (parenRanger->getRangerNode())
                {
                    if (parenRanger->getRangerNode()->getRealType() == NodeType::PARALLEL)
                    {
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
                        collectParams(parenRanger->getRangerNode());
                    }
                    else if (parenRanger->getRangerNode()->getRealType() == NodeType::IDENTIFIER)
                    {
                        paramNames.push_back(static_cast<IdentifierNode*>(
                            parenRanger->getRangerNode().get())->getName());
                    }
                }
            }
        }

        // 构造函数名: className.__init__
        std::string ctorName = CurrentClassName + ".__init__";

        // 创建函数类型: ptr (ptr this, ptr, ptr, ...) -> ptr
        std::vector<llvm::Type*> paramTypes(paramNames.size() + 1, getValueType());
        auto* funcType = llvm::FunctionType::get(getValueType(), paramTypes, false);
        auto* func = llvm::Function::Create(funcType, llvm::Function::ExternalLinkage, ctorName, TheModule.get());

        // 设置参数名
        unsigned idx = 0;
        func->getArg(idx)->setName("this");
        idx++;
        for (const auto& pname : paramNames)
        {
            if (idx < func->arg_size())
            {
                func->getArg(idx)->setName(pname);
            }
            idx++;
        }

        Functions[ctorName] = func;
        if (!CurrentClassName.empty())
        {
            ClassMethodTables[CurrentClassName]["__init__"] = func;
        }

        // 保存上下文
        auto* prevFunction = CurrentFunction;
        auto prevNamedValues = NamedValues;
        auto prevThisAlloca = ThisAlloca;
        auto prevInsertPoint = Builder->saveIP();

        CurrentFunction = func;
        NamedValues.clear();

        // 创建入口基本块
        auto* entryBB = llvm::BasicBlock::Create(*TheContext, "entry", func);
        Builder->SetInsertPoint(entryBB);

        // this 参数 alloca
        ThisAlloca = createEntryBlockAlloca(func, "this", getValueType());
        Builder->CreateStore(func->getArg(0), ThisAlloca);
        NamedValues["this"] = ThisAlloca;

        // 其他参数 alloca
        idx = 1;
        for (const auto& pname : paramNames)
        {
            if (idx < func->arg_size())
            {
                auto* alloca = createEntryBlockAlloca(func, pname, getValueType());
                Builder->CreateStore(func->getArg(idx), alloca);
                NamedValues[pname] = alloca;
            }
            idx++;
        }

        // 生成构造函数体
        if (node.getBodyNode())
        {
            node.getBodyNode()->acceptVisitor(*this);
        }

        // 返回 this
        if (getCurrentBlock() && !getCurrentBlock()->hasTerminator())
        {
            auto* thisVal = Builder->CreateLoad(getValueType(), ThisAlloca, "this.load");
            Builder->CreateRet(thisVal);
        }

        // 恢复上下文
        CurrentFunction = prevFunction;
        NamedValues = prevNamedValues;
        ThisAlloca = prevThisAlloca;
        Builder->restoreIP(prevInsertPoint);

        LLVM_DEBUG("ConstructorDefinitionNode: " << ctorName << " (" << paramNames.size() << " params) defined");
        pushValue(llvm::ConstantPointerNull::get(getValueType()));
    }

    // ============================================================================
    // 类声明与定义节点
    // ============================================================================

    void LLVMCodeGenVisitor::visitClassDeclarationNode(ClassDeclarationNode& node)
    {
        if (auto* ident = dynamic_cast<IdentifierNode*>(node.getNameNode().get()))
        {
            const std::string& className = ident->getName();
            // 创建不透明 struct 类型（前向声明）
            auto* structType = llvm::StructType::create(*TheContext, className);
            ClassTypes[className] = structType;
            LLVM_DEBUG("ClassDeclarationNode: " << className << " (forward declared)");
        }
        pushValue(llvm::ConstantPointerNull::get(getValueType()));
    }

    void LLVMCodeGenVisitor::visitClassDefinitionNode(ClassDefinitionNode& node)
    {
        // 获取类名
        std::string className;
        if (auto* ident = dynamic_cast<IdentifierNode*>(node.getNameNode().get()))
        {
            className = ident->getName();
        }
        if (className.empty())
        {
            LLVM_DEBUG("ClassDefinitionNode: unable to determine class name");
            return;
        }

        LLVM_DEBUG("ClassDefinitionNode: " << className);

        // 解析继承关系（从标签中获取父类名）
        auto labels = node.getLabelNode();
        if (!labels.empty())
        {
            // 第一个标签通常是父类名，如 class Child: Parent { }
            const auto& labelPath = labels[0]->getLabelPath();
            if (!labelPath.empty())
            {
                std::string parentName = labelPath[0];
                ClassParentNames[className] = parentName;
                LLVM_DEBUG("  inherits from: " << parentName);
            }
        }

        // 保存上下文
        auto prevClassName = CurrentClassName;
        auto prevThisAlloca = ThisAlloca;
        CurrentClassName = className;
        ThisAlloca = nullptr;

        // 收集类成员（包括父类的字段）
        std::vector<std::string> memberNames;
        std::vector<std::shared_ptr<ExpressionNode>> methodNodes;

        // 如果有父类，先添加父类的字段
        auto parentIt = ClassParentNames.find(className);
        if (parentIt != ClassParentNames.end())
        {
            auto fieldIt = ClassFieldNames.find(parentIt->second);
            if (fieldIt != ClassFieldNames.end())
            {
                memberNames = fieldIt->second; // 复制父类字段
                LLVM_DEBUG("  inherited " << memberNames.size() << " fields from parent");
            }
        }

        if (auto* body = dynamic_cast<BlockRangerNode*>(node.getBodyNode().get()))
        {
            for (const auto& expr : body->getBodyExpressions())
            {
                if (expr->getRealType() == NodeType::VAR)
                {
                    // 成员变量
                    if (auto* varDef = dynamic_cast<VariableDefinitionNode*>(expr.get()))
                    {
                        for (const auto& var : varDef->getVarDefs())
                        {
                            memberNames.push_back(var->getNameNode()->getName());
                        }
                    }
                }
                else if (expr->getRealType() == NodeType::FUNCTION_DEFINITION ||
                         expr->getRealType() == NodeType::ANON_FUNCTION_DEFINITION)
                {
                    // 方法
                    methodNodes.push_back(expr);
                }
                else if (expr->getRealType() == NodeType::CONSTRUCTOR ||
                         expr->getRealType() == NodeType::PREFIX)
                {
                    // 构造函数（ConstructorDefinitionNode 的 getRealType() 实际返回 PREFIX）
                    if (dynamic_cast<ConstructorDefinitionNode*>(expr.get()))
                    {
                        methodNodes.push_back(expr);
                    }
                }
            }
        }

        // 创建 LLVM StructType（所有字段为 ptr）
        std::vector<llvm::Type*> fieldTypes(memberNames.size(), getValueType());
        auto* structType = llvm::StructType::create(*TheContext, fieldTypes, className);
        ClassTypes[className] = structType;

        // 保存字段名列表（用于属性访问）
        ClassFieldNames[className] = memberNames;

        // 初始化方法表
        ClassMethodTables[className] = {};

        LLVM_DEBUG("  " << memberNames.size() << " members, " << methodNodes.size() << " methods");

        // 编译方法
        for (const auto& methodNode : methodNodes)
        {
            methodNode->acceptVisitor(*this);
        }

        // 恢复上下文
        CurrentClassName = prevClassName;
        ThisAlloca = prevThisAlloca;

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
        llvm::Value* receiverValue = nullptr; // 方法调用的接收者（this）

        if (callTarget->getRealType() == NodeType::IDENTIFIER)
        {
            auto* ident = static_cast<IdentifierNode*>(callTarget.get());
            funcName = ident->getName();
        }
        else if (callTarget->getRealType() == NodeType::CALL)
        {
            // 方法调用: obj.method(args)
            // callTarget 是一个 FunctionCallNode: leftNode=obj, rightNode=method名
            auto* methodCall = static_cast<FunctionCallNode*>(callTarget.get());
            if (methodCall->getLeftNode()->getRealType() == NodeType::IDENTIFIER)
            {
                // 获取接收者对象
                auto* objIdent = static_cast<IdentifierNode*>(methodCall->getLeftNode().get());
                receiverValue = loadVariable(objIdent->getName());
                if (!receiverValue)
                {
                    // 尝试从函数表查找（对象可能是函数引用）
                    auto funcIt = Functions.find(objIdent->getName());
                    if (funcIt != Functions.end())
                    {
                        receiverValue = funcIt->second;
                    }
                }
                if (!receiverValue)
                {
                    receiverValue = llvm::ConstantPointerNull::get(getValueType());
                }

                // 获取方法名
                if (methodCall->getRightNode()->getRealType() == NodeType::IDENTIFIER)
                {
                    funcName = static_cast<IdentifierNode*>(methodCall->getRightNode().get())->getName();
                }
            }
        }

        // 收集参数值（区分位置参数和命名参数）
        std::vector<llvm::Value*> argValues;
        std::map<std::string, llvm::Value*> namedArgValues;
        if (argsNode)
        {
            // 参数可能是 BlockRangerNode 或 ParenRangerNode
            if (const auto* blockRanger = dynamic_cast<BlockRangerNode*>(argsNode.get()))
            {
                for (const auto& expr : blockRanger->getBodyExpressions())
                {
                    // 命名参数: end="\n" -> ASSIGNMENT
                    if (expr->getRealType() == NodeType::ASSIGNMENT)
                    {
                        auto* assign = static_cast<InfixExpressionNode*>(expr.get());
                        if (assign->getLeftNode()->getRealType() == NodeType::IDENTIFIER)
                        {
                            std::string paramName = static_cast<IdentifierNode*>(assign->getLeftNode().get())->getName();
                            assign->getRightNode()->acceptVisitor(*this);
                            auto* val = popValue();
                            namedArgValues[paramName] = val ? val : llvm::ConstantPointerNull::get(getValueType());
                        }
                    }
                    else
                    {
                        expr->acceptVisitor(*this);
                        auto* argVal = popValue();
                        argValues.push_back(argVal ? argVal : llvm::ConstantPointerNull::get(getValueType()));
                    }
                }
            }
            else if (const auto* parenRanger = dynamic_cast<ParenRangerNode*>(argsNode.get()))
            {
                if (parenRanger->getRangerNode())
                {
                    if (const auto& ranger = parenRanger->getRangerNode();
                        ranger->getRealType() == NodeType::PARALLEL)
                    {
                        // 逗号分隔的多参数：递归遍历 PARALLEL 节点
                        std::function<void(const std::shared_ptr<ExpressionNode>&)> collectArgs;
                        collectArgs = [&](const std::shared_ptr<ExpressionNode>& expr)
                        {
                            if (expr->getRealType() == NodeType::PARALLEL)
                            {
                                auto* p = static_cast<InfixExpressionNode*>(expr.get());
                                collectArgs(p->getLeftNode());
                                collectArgs(p->getRightNode());
                            }
                            else if (expr->getRealType() == NodeType::ASSIGNMENT)
                            {
                                // 命名参数
                                auto* assign = static_cast<InfixExpressionNode*>(expr.get());
                                if (assign->getLeftNode()->getRealType() == NodeType::IDENTIFIER)
                                {
                                    std::string paramName = static_cast<IdentifierNode*>(assign->getLeftNode().get())->getName();
                                    assign->getRightNode()->acceptVisitor(*this);
                                    auto* val = popValue();
                                    namedArgValues[paramName] = val ? val : llvm::ConstantPointerNull::get(getValueType());
                                }
                            }
                            else
                            {
                                expr->acceptVisitor(*this);
                                auto* argVal = popValue();
                                argValues.push_back(argVal ? argVal : llvm::ConstantPointerNull::get(getValueType()));
                            }
                        };
                        collectArgs(ranger);
                    }
                    else
                    {
                        // 单个参数
                        ranger->acceptVisitor(*this);
                        auto* argVal = popValue();
                        argValues.push_back(argVal ? argVal : llvm::ConstantPointerNull::get(getValueType()));
                    }
                }
            }
            else
            {
                // 直接表达式参数
                argsNode->acceptVisitor(*this);
                if (auto* argVal = popValue())
                {
                    argValues.push_back(argVal);
                }
            }
        }

        // 尝试内置函数 IR 生成器
        if (tryEmitBuiltinIR(funcName, argValues, namedArgValues))
        {
            LLVM_DEBUG("FunctionCallNode: " << funcName << " (builtin IR generated)");
            return;
        }

        // 类实例化：当函数名匹配一个已定义的类时
        if (!receiverValue && ClassTypes.count(funcName))
        {
            auto* structType = ClassTypes[funcName];
            auto fieldIt = ClassFieldNames.find(funcName);
            size_t fieldCount = (fieldIt != ClassFieldNames.end()) ? fieldIt->second.size() : 0;

            // 在堆上分配类实例内存
            // 计算结构体大小：字段数 * sizeof(ptr)
            size_t structSize = fieldCount * sizeof(void*);
            auto* sizeVal = llvm::ConstantInt::get(
                llvm::Type::getInt64Ty(*TheContext),
                structSize);
            auto* mallocType = llvm::FunctionType::get(getValueType(), {llvm::Type::getInt64Ty(*TheContext)}, false);
            auto* mallocFunc = getOrCreateExternalFunc("malloc", mallocType);
            std::vector<llvm::Value*> mallocArgs = {sizeVal};
            auto* instancePtr = Builder->CreateCall(mallocType, mallocFunc, mallocArgs, "new." + funcName);

            // 将实例指针包装为 Tagged Value
            // 使用一个特殊的 tag 来标识类实例（暂时复用 TAG_NULL，后续应添加 TAG_OBJECT）
            auto* taggedInstance = createTaggedValue(TAG_NULL, instancePtr);

            // 查找并调用构造函数
            std::string ctorName = funcName + ".__init__";
            auto* ctorFunc = TheModule->getFunction(ctorName);
            if (!ctorFunc)
            {
                auto funcIt = Functions.find(ctorName);
                if (funcIt != Functions.end())
                    ctorFunc = funcIt->second;
            }

            if (ctorFunc)
            {
                // 参数：this + 用户参数
                std::vector<llvm::Value*> ctorArgs;
                ctorArgs.push_back(instancePtr);
                for (const auto& arg : argValues)
                    ctorArgs.push_back(arg);
                Builder->CreateCall(ctorFunc, ctorArgs, "ctor." + funcName);
            }

            LLVM_DEBUG("FunctionCallNode: " << funcName << " (class instantiation, " << fieldCount << " fields)");
            pushValue(taggedInstance);
            return;
        }

        // 方法调用：将 receiver 作为第一个参数
        if (receiverValue)
        {
            argValues.insert(argValues.begin(), receiverValue);
        }

        // 查找已定义的函数
        // 注意：用户定义的 main 被重命名为 __rio_main，需要特殊处理
        std::string lookupName = funcName;
        if (funcName == "main")
        {
            lookupName = "__rio_main";
        }

        auto* callee = TheModule->getFunction(lookupName);
        if (!callee)
        {
            // 尝试从 Functions 映射中查找
            auto it = Functions.find(funcName);
            if (it != Functions.end())
            {
                callee = it->second;
            }
        }

        // 方法调用：如果直接查找失败，尝试从所有类的方法表中查找
        if (!callee && receiverValue)
        {
            for (const auto& [clsName, methodTable] : ClassMethodTables)
            {
                auto methodIt = methodTable.find(funcName);
                if (methodIt != methodTable.end() && methodIt->second)
                {
                    callee = methodIt->second;
                    LLVM_DEBUG("FunctionCallNode: found method " << funcName << " in class " << clsName);
                    break;
                }
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
            // 尝试从变量中加载函数指针进行间接调用
            auto* varVal = loadVariable(funcName);
            if (varVal)
            {
                // varVal 是 Tagged Function，payload 是函数指针
                auto* funcPtr = extractPayload(varVal);

                // 创建函数类型: ptr (ptr, ptr, ...)
                std::vector<llvm::Type*> paramTypes(argValues.size(), getValueType());
                auto* funcType = llvm::FunctionType::get(getValueType(), paramTypes, false);

                auto* callInst = Builder->CreateCall(funcType, funcPtr, argValues, "call.indirect." + funcName);
                LLVM_DEBUG("FunctionCallNode: " << funcName << " (indirect, " << argValues.size() << " args)");
                pushValue(callInst);
            }
            else
            {
                // 未找到函数也未找到变量：声明为外部函数并调用
                std::vector<llvm::Type*> paramTypes(argValues.size(), getValueType());
                auto* funcType = llvm::FunctionType::get(getValueType(), paramTypes, false);
                callee = llvm::Function::Create(funcType, llvm::Function::ExternalLinkage, funcName, TheModule.get());

                auto* callInst = Builder->CreateCall(callee, argValues, "call." + funcName);
                LLVM_DEBUG("FunctionCallNode: " << funcName << " (external, " << argValues.size() << " args)");
                pushValue(callInst);
            }
        }
    }

    // ============================================================================
    // 程序节点（入口）
    // ============================================================================

    void LLVMCodeGenVisitor::visitProgramNode(ProgramNode& node)
    {
        LLVM_DEBUG("=== ProgramNode: start ===");

        // 创建 C main 函数作为程序入口
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

        // 遍历所有语句（函数定义、类定义、顶层表达式等）
        for (const auto& statement : node.getStatements())
        {
            statement->acceptVisitor(*this);
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

        // 增强赋值运算符 (+=, -=, *=, /=, %=)
        // 支持 INFIX 类型（通过opStr检测）或 ARGUMENT_ASSIGNMENT 类型
        if (infixType == NodeType::ARGUMENT_ASSIGNMENT ||
            opStr == "+=" || opStr == "-=" || opStr == "*=" || opStr == "/=" || opStr == "%=")
        {
            handleCompoundAssignment(node);
            return;
        }

        // 逻辑短路求值（同时检查 NodeType 和 opStr）
        if ((infixType == NodeType::AND || infixType == NodeType::OR || infixType == NodeType::LOGICAL)
            && (opStr == "&&" || opStr == "||"))
        {
            handleLogicalShortCircuit(node);
            return;
        }

        // ==================== 属性访问 ====================
        // 属性访问不走算术/浮点分支逻辑，单独处理
        if (infixType == NodeType::ATTRIBUTE_EXPRESSION)
        {
            // 属性访问: obj.attr
            LLVM_DEBUG("InfixNode: ATTRIBUTE_EXPRESSION");

            // 计算左操作数（对象）
            node.getLeftNode()->acceptVisitor(*this);
            auto* objVal = popValue();

            // 获取属性名
            std::string attrName;
            if (auto* ident = dynamic_cast<IdentifierNode*>(node.getRightNode().get()))
            {
                attrName = ident->getName();
            }
            else
            {
                LLVM_DEBUG("Attribute access: right node is not an identifier");
                pushValue(llvm::ConstantPointerNull::get(getValueType()));
                return;
            }

            if (!objVal)
            {
                LLVM_DEBUG("Attribute access: null object for " << attrName);
                pushValue(llvm::ConstantPointerNull::get(getValueType()));
                return;
            }

            // 确定类名，并判断对象是否为 Tagged Value
            std::string className;
            bool isTaggedValue = true;
            llvm::Value* objPayload = nullptr;
            
            // 1. 如果在类内部访问 this.field，使用 CurrentClassName
            // 在构造函数/方法中，this 是直接的类实例指针，不是 Tagged Value
            if (auto* ident = dynamic_cast<IdentifierNode*>(node.getLeftNode().get()))
            {
                if (ident->getName() == "this" && !CurrentClassName.empty())
                {
                    className = CurrentClassName;
                    isTaggedValue = false;
                    objPayload = objVal;
                }
            }
            
            // 如果不在类上下文中，需要从 Tagged Value 提取
            if (isTaggedValue)
            {
                objPayload = extractPayload(objVal);
                
                // 2. 尝试从变量名推断类名（变量名通常是小写的类名）
                if (auto* ident = dynamic_cast<IdentifierNode*>(node.getLeftNode().get()))
                {
                    std::string varName = ident->getName();
                    // 尝试直接匹配（类名可能和变量名相同）
                    if (ClassTypes.count(varName))
                    {
                        className = varName;
                    }
                    else
                    {
                        // 3. 尝试首字母大写的类名
                        if (!varName.empty())
                        {
                            std::string capitalized = varName;
                            capitalized[0] = std::toupper(capitalized[0]);
                            if (ClassTypes.count(capitalized))
                            {
                                className = capitalized;
                            }
                        }
                    }
                }
                
                // 4. 如果还是找不到，遍历所有类查找包含该字段的类
                if (className.empty())
                {
                    for (const auto& [clsName, fieldNames] : ClassFieldNames)
                    {
                        for (const auto& f : fieldNames)
                        {
                            if (f == attrName)
                            {
                                className = clsName;
                                break;
                            }
                        }
                        if (!className.empty()) break;
                    }
                }
            }

            if (!className.empty())
            {
                auto it = ClassFieldNames.find(className);
                if (it != ClassFieldNames.end())
                {
                    const auto& fieldNames = it->second;
                    int fieldIndex = -1;
                    for (size_t i = 0; i < fieldNames.size(); ++i)
                    {
                        if (fieldNames[i] == attrName)
                        {
                            fieldIndex = static_cast<int>(i);
                            break;
                        }
                    }

                    if (fieldIndex >= 0)
                    {
                        auto* structType = ClassTypes[className];
                        auto* fieldPtr = Builder->CreateStructGEP(structType, objPayload, fieldIndex, attrName + ".ptr");
                        auto* fieldVal = Builder->CreateLoad(getValueType(), fieldPtr, attrName + ".val");

                        LLVM_DEBUG("Attribute access: " << className << "." << attrName << " [index " << fieldIndex << "]");
                        pushValue(fieldVal);
                        return;
                    }
                }
            }

            LLVM_DEBUG("Attribute access: field not found - " << attrName);
            pushValue(llvm::ConstantPointerNull::get(getValueType()));
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

        // 由于我们使用 Tagged Struct 存储值，
        // 算术运算需要先提取 payload，运算后再包装为 Tagged Int
        // 浮点数需要使用浮点运算
        auto* int64Ty = llvm::Type::getInt64Ty(*TheContext);
        auto* doubleTy = llvm::Type::getDoubleTy(*TheContext);

        // 检查操作数是否为浮点数
        auto* leftTag = extractTag(left);
        auto* rightTag = extractTag(right);
        auto* floatTagConst = llvm::ConstantInt::get(int64Ty, TAG_FLOAT);
        auto* isFloatLeft = Builder->CreateICmpEQ(leftTag, floatTagConst, "is_float.left");
        auto* isFloatRight = Builder->CreateICmpEQ(rightTag, floatTagConst, "is_float.right");
        auto* isFloat = Builder->CreateOr(isFloatLeft, isFloatRight, "is_float");

        auto* currentFunc = Builder->GetInsertBlock()->getParent();

        // 浮点数分支
        auto* floatBB = llvm::BasicBlock::Create(*TheContext, "arith.float", currentFunc);
        // 整数分支
        auto* intBB = llvm::BasicBlock::Create(*TheContext, "arith.int", currentFunc);
        // 合并分支
        auto* arithMergeBB = llvm::BasicBlock::Create(*TheContext, "arith.merge", currentFunc);

        // 在分支前创建结果 alloca（满足 dominance）
        auto* resultAlloca = createEntryBlockAlloca(currentFunc, "arith.result", getValueType());
        Builder->CreateStore(llvm::ConstantPointerNull::get(getValueType()), resultAlloca);

        Builder->CreateCondBr(isFloat, floatBB, intBB);

        // --- 浮点数运算 ---
        Builder->SetInsertPoint(floatBB);
        {
            auto* leftPayload = extractPayload(left);
            auto* rightPayload = extractPayload(right);
            auto* leftBits = Builder->CreatePtrToInt(leftPayload, int64Ty, "left.bits");
            auto* rightBits = Builder->CreatePtrToInt(rightPayload, int64Ty, "right.bits");

            // 根据各自的 tag 选择 bitcast（FLOAT）或 sitofp（INT）
            auto* leftDouble = Builder->CreateSelect(
                isFloatLeft,
                Builder->CreateBitCast(leftBits, doubleTy, "left.bitcast"),
                Builder->CreateSIToFP(leftBits, doubleTy, "left.sitofp"),
                "left.double");
            auto* rightDouble = Builder->CreateSelect(
                isFloatRight,
                Builder->CreateBitCast(rightBits, doubleTy, "right.bitcast"),
                Builder->CreateSIToFP(rightBits, doubleTy, "right.sitofp"),
                "right.double");

            llvm::Value* floatResult = nullptr;
            if (infixType == NodeType::PLUS)
                floatResult = Builder->CreateFAdd(leftDouble, rightDouble, "fadd");
            else if (infixType == NodeType::MINUS)
                floatResult = Builder->CreateFSub(leftDouble, rightDouble, "fsub");
            else if (infixType == NodeType::MULTIPLY)
                floatResult = Builder->CreateFMul(leftDouble, rightDouble, "fmul");
            else if (infixType == NodeType::DIVIDE)
                floatResult = Builder->CreateFDiv(leftDouble, rightDouble, "fdiv");
            else if (infixType == NodeType::MODULO)
                floatResult = Builder->CreateFRem(leftDouble, rightDouble, "frem");
            else
                floatResult = leftDouble; // fallback

            // 将 double 结果转回 payload
            auto* resultBits = Builder->CreateBitCast(floatResult, int64Ty, "result.bits");
            auto* resultPayload = Builder->CreateIntToPtr(resultBits, getValueType(), "result.float.ptr");
            auto* resultVal = createTaggedValue(TAG_FLOAT, resultPayload);
            Builder->CreateStore(resultVal, resultAlloca);
            Builder->CreateBr(arithMergeBB);

            // 记录浮点结果用于 phi
            floatBB = Builder->GetInsertBlock(); // 更新 floatBB（可能有额外指令）
        }

        // --- 整数运算 ---
        Builder->SetInsertPoint(intBB);
        {
            // 从 Tagged Value 中提取 payload 作为运算操作数
            auto* leftPayload = extractPayload(left);
            auto* rightPayload = extractPayload(right);

            // ptrtoint: 将 ptr 转为 i64 以进行运算
            auto* leftInt = Builder->CreatePtrToInt(leftPayload, int64Ty, "left.int");
            auto* rightInt = Builder->CreatePtrToInt(rightPayload, int64Ty, "right.int");

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
            // 比较运算返回 Tagged Bool，其他运算返回 Tagged Int
            int64_t resultTag = TAG_INT;
            if (infixType == NodeType::EQUALITY || infixType == NodeType::NOT_EQUAL ||
                infixType == NodeType::LESS || infixType == NodeType::LESS_EQUAL ||
                infixType == NodeType::GREATER || infixType == NodeType::GREATER_EQUAL ||
                infixType == NodeType::COMPARE)
            {
                resultTag = TAG_BOOL;
            }
            auto* resultPtr = createTaggedValue(resultTag, Builder->CreateIntToPtr(result, getValueType(), "int.ptr"));
            Builder->CreateStore(resultPtr, resultAlloca);
            Builder->CreateBr(arithMergeBB);
            intBB = Builder->GetInsertBlock(); // 更新 intBB
        }
        else
        {
            Builder->CreateStore(llvm::ConstantPointerNull::get(getValueType()), resultAlloca);
            Builder->CreateBr(arithMergeBB);
            intBB = Builder->GetInsertBlock();
        }

        // --- 合并浮点和整数结果 ---
        Builder->SetInsertPoint(arithMergeBB);
        pushValue(Builder->CreateLoad(getValueType(), resultAlloca, "arith.result.load"));
        }
    }

    // ============================================================================
    // 逻辑短路求值
    // ============================================================================

    void LLVMCodeGenVisitor::handleLogicalShortCircuit(const InfixExpressionNode& node)
    {
        const Token& opToken = node.getOpToken();
        const std::string op = opToken.getValue();

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

        // 在分支前创建短路值（必须在 condBB 中，以满足 dominance）
        llvm::Value* shortCircuitVal = nullptr;
        if (op == "&&")
        {
            shortCircuitVal = createTaggedBool(false);
        }
        else
        {
            shortCircuitVal = createTaggedBool(true);
        }

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
        phi->addIncoming(shortCircuitVal, condBB);
        phi->addIncoming(rightVal, evalRightBB);

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
            if (op == "!")
            {
                // 逻辑非返回 Tagged Bool
                // result 是 i64 (0 或 1)，用 select 选择 payload
                auto* zero = llvm::ConstantInt::get(llvm::Type::getInt64Ty(*TheContext), 0);
                auto* isTrue = Builder->CreateICmpNE(result, zero, "not.is_true");
                auto* truePayload = Builder->CreateIntToPtr(
                    llvm::ConstantInt::get(llvm::Type::getInt64Ty(*TheContext), 1), getValueType());
                auto* falsePayload = llvm::ConstantPointerNull::get(getValueType());
                auto* payload = Builder->CreateSelect(isTrue, truePayload, falsePayload, "not.payload");
                pushValue(createTaggedValue(TAG_BOOL, payload));
            }
            else
            {
                // 其他一元运算返回 Tagged Int
                pushValue(createTaggedValue(TAG_INT, Builder->CreateIntToPtr(result, getValueType(), "unary.ptr")));
            }
        }
        else
        {
            pushValue(createTaggedValueNull(TAG_NULL));
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
            if (auto* val = popValue())
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

        // 检查函数体是否为 encapsulated（在创建函数定义之前）
        CurrentFunctionIsEncapsulated = false;
        bool isEncapsulated = false;
        if (node.getBodyNode())
        {
            if (const auto* bodyBlock = dynamic_cast<BlockRangerNode*>(node.getBodyNode().get());
                bodyBlock && !bodyBlock->getBodyExpressions().empty())
            {
                if (const auto lastExpr = bodyBlock->getBodyExpressions().back();
                    lastExpr && (lastExpr->getRealType() == NodeType::ENCAPSULATED
                                 || dynamic_cast<EncapsulatedExpressionNode*>(lastExpr.get())))
                {
                    isEncapsulated = true;
                    CurrentFunctionIsEncapsulated = true;
                    EncapsulatedFunctions.insert(funcName);
                    LLVM_DEBUG("FunctionDefinitionNode: " << funcName << " is encapsulated (builtin)");
                }
            }
        }

        // encapsulated 函数不生成函数定义，只注册名称
        // 实际的 IR 由调用处的内置 IR 生成器内联展开
        if (isEncapsulated)
        {
            Functions[funcName] = nullptr;
            return;
        }

        // 用户定义的 main 函数重命名为 __rio_main，避免与 C main 入口冲突
        std::string llvmFuncName = funcName;
        if (funcName == "main")
        {
            llvmFuncName = "__rio_main";
        }

        // 类方法：添加 className. 前缀，自动添加 this 参数
        bool isMethod = !CurrentClassName.empty();
        if (isMethod)
        {
            llvmFuncName = CurrentClassName + "." + funcName;
        }

        // 创建函数类型: ptr @funcName(ptr, ptr, ...) -> ptr
        // 类方法额外添加一个 this 参数
        size_t thisParamOffset = isMethod ? 1 : 0;
        std::vector<llvm::Type*> paramTypes(paramNames.size() + thisParamOffset, getValueType());
        auto* funcType = llvm::FunctionType::get(getValueType(), paramTypes, false);

        // 创建函数（类方法和导出函数使用 ExternalLinkage，否则 PrivateLinkage）
        const auto linkage = (ExportedSymbols.contains(funcName) || isMethod)
            ? llvm::Function::ExternalLinkage
            : llvm::Function::PrivateLinkage;
        auto* func = llvm::Function::Create(funcType, linkage, llvmFuncName, TheModule.get());

        // 设置参数名
        unsigned idx = 0;
        if (isMethod)
        {
            func->getArg(idx)->setName("this");
            idx++;
        }
        for (auto& arg : func->args())
        {
            if (idx < paramNames.size())
            {
                arg.setName(paramNames[idx]);
            }
            idx++;
        }

        Functions[funcName] = func;
        if (isMethod && !CurrentClassName.empty())
        {
            ClassMethodTables[CurrentClassName][funcName] = func;
        }

        // 保存当前上下文
        auto* prevFunction = CurrentFunction;
        auto prevNamedValues = NamedValues;
        auto prevThisAlloca = ThisAlloca;
        auto prevInsertPoint = Builder->saveIP();

        CurrentFunction = func;
        NamedValues.clear();

        // 创建入口基本块
        auto* entryBB = llvm::BasicBlock::Create(*TheContext, "entry", func);
        Builder->SetInsertPoint(entryBB);

        // 为参数创建 alloca 并存储
        idx = 0;
        if (isMethod)
        {
            // this 参数 alloca
            ThisAlloca = createEntryBlockAlloca(func, "this", getValueType());
            Builder->CreateStore(func->getArg(0), ThisAlloca);
            NamedValues["this"] = ThisAlloca;
            idx = 1;
        }
        for (const auto& pname : paramNames)
        {
            if (idx < func->arg_size())
            {
                auto* alloca = createEntryBlockAlloca(func, pname, getValueType());
                Builder->CreateStore(func->getArg(idx), alloca);
                NamedValues[pname] = alloca;
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
        ThisAlloca = prevThisAlloca;
        Builder->restoreIP(prevInsertPoint);

        LLVM_DEBUG("FunctionDefinitionNode: " << llvmFuncName << " (" << paramNames.size() << " params" << (isMethod ? ", method of " + CurrentClassName : "") << ") defined");
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
            const auto* branchNode = static_cast<BranchNode*>(branch.get());

            // 获取条件节点
            // 第一个分支是 if，后续是 elif（条件非空）或 else（条件为空）

            if (const auto condExpr = branchNode->getConditionNode())
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
        LLVM_DEBUG("EncapsulatedExpressionNode: marking current function as builtin");
        CurrentFunctionIsEncapsulated = true;
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

    void LLVMCodeGenVisitor::visitContinueExpressionNode(ContinueExpressionNode& node)
    {
        if (!ContinueTargetStack.empty())
        {
            Builder->CreateBr(ContinueTargetStack.top());
            // 创建一个 unreachable 块，因为 continue 后的代码不可达
            llvm::Function* func = Builder->GetInsertBlock()->getParent();
            auto* unreachableBB = llvm::BasicBlock::Create(*TheContext, "continue.unreachable", func);
            Builder->SetInsertPoint(unreachableBB);
        }
        else
        {
            LLVM_DEBUG("ContinueExpressionNode: not inside a loop");
        }

        LLVM_DEBUG("ContinueExpressionNode");
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

        if (const auto& paramNode = node.getParamNode())
        {
            // 递归收集 PARALLEL 节点中的标识符
            std::function<void(const std::shared_ptr<ExpressionNode>&)> collectParams;
            collectParams = [&](const std::shared_ptr<ExpressionNode>& expr)
            {
                if (!expr) return;

                auto nodeType = expr->getRealType();

                // 处理 BlockRangerNode / ParenRangerNode 包装
                if (auto* blockRanger = dynamic_cast<BlockRangerNode*>(expr.get()))
                {
                    for (const auto& bodyExpr : blockRanger->getBodyExpressions())
                    {
                        collectParams(bodyExpr);
                    }
                    return;
                }
                if (auto* parenRanger = dynamic_cast<ParenRangerNode*>(expr.get()))
                {
                    if (parenRanger->getRangerNode())
                    {
                        collectParams(parenRanger->getRangerNode());
                    }
                    return;
                }

                if (nodeType == NodeType::PARALLEL)
                {
                    auto* p = static_cast<InfixExpressionNode*>(expr.get());
                    collectParams(p->getLeftNode());
                    collectParams(p->getRightNode());
                }
                else if (nodeType == NodeType::IDENTIFIER)
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

        // 将函数指针作为 Tagged Function push 到栈上
        pushValue(createTaggedValue(TAG_FUNCTION, func));

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

        if (!keyVal) keyVal = createTaggedValueNull(TAG_NULL);
        if (!valueVal) valueVal = createTaggedValueNull(TAG_NULL);

        // 纯 IR 实现：创建 RCCPair struct: { ptr key, ptr value }
        auto* pairType = llvm::StructType::create(*TheContext, {getValueType(), getValueType()}, "RCCPair");
        auto* alloca = Builder->CreateAlloca(pairType, nullptr, "pair");
        auto* keyPtr = Builder->CreateStructGEP(pairType, alloca, 0, "pair.key");
        Builder->CreateStore(keyVal, keyPtr);
        auto* valPtr = Builder->CreateStructGEP(pairType, alloca, 1, "pair.val");
        Builder->CreateStore(valueVal, valPtr);

        // 返回 Tagged Dict (payload 指向 pair struct)
        pushValue(createTaggedValue(TAG_DICT, alloca));
    }

    // ============================================================================
    // 字典表达式节点
    // ============================================================================

    void LLVMCodeGenVisitor::visitDictionaryExpressionNode(DictionaryExpressionNode& node)
    {
        LLVM_DEBUG("DictionaryExpressionNode");

        // 收集所有 key-value 对
        std::vector<llvm::Value*> dictArgs;

        if (const auto& bodyNode = node.getBodyNode())
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
                    // PAIR 类型实际上是 InfixExpressionNode（冒号运算符）
                    auto* pair = static_cast<InfixExpressionNode*>(pairExpr.get());
                    pair->getLeftNode()->acceptVisitor(*this);
                    auto* keyVal = popValue();
                    pair->getRightNode()->acceptVisitor(*this);
                    auto* valueVal = popValue();

                    dictArgs.push_back(keyVal ? keyVal : llvm::ConstantPointerNull::get(getValueType()));
                    dictArgs.push_back(valueVal ? valueVal : llvm::ConstantPointerNull::get(getValueType()));
                }
            }
        }

        // 纯 IR 实现：用 malloc 分配 RCCDict struct 和 keys/values 数组

        // 声明 malloc
        auto* mallocType = llvm::FunctionType::get(getValueType(), {llvm::Type::getInt64Ty(*TheContext)}, false);
        auto* mallocFunc = getOrCreateExternalFunc("malloc", mallocType);

        int64_t count = dictArgs.size() / 2;

        // 分配 keys 数组: malloc(count * ptr_size)
        auto* keyBufSize = llvm::ConstantInt::get(llvm::Type::getInt64Ty(*TheContext), count * sizeof(void*));
        auto* keyBuf = Builder->CreateCall(mallocFunc, {keyBufSize}, "dict.keys");

        // 分配 values 数组: malloc(count * ptr_size)
        auto* valBufSize = llvm::ConstantInt::get(llvm::Type::getInt64Ty(*TheContext), count * sizeof(void*));
        auto* valBuf = Builder->CreateCall(mallocFunc, {valBufSize}, "dict.values");

        // 存储 keys（每个元素是 ptr，占 8 字节）
        auto* dictPtrSize = llvm::ConstantInt::get(llvm::Type::getInt64Ty(*TheContext), sizeof(void*));
        for (int64_t i = 0; i < count; i++)
        {
            auto* idx = llvm::ConstantInt::get(llvm::Type::getInt64Ty(*TheContext), i);
            auto* byteOffset = Builder->CreateMul(idx, dictPtrSize, "key.offset");
            auto* gep = Builder->CreateInBoundsGEP(llvm::Type::getInt8Ty(*TheContext), keyBuf, byteOffset, "key.gep");
            Builder->CreateStore(dictArgs[i * 2], gep);
        }

        // 存储 values（每个元素是 ptr，占 8 字节）
        for (int64_t i = 0; i < count; i++)
        {
            auto* idx = llvm::ConstantInt::get(llvm::Type::getInt64Ty(*TheContext), i);
            auto* byteOffset = Builder->CreateMul(idx, dictPtrSize, "val.offset");
            auto* gep = Builder->CreateInBoundsGEP(llvm::Type::getInt8Ty(*TheContext), valBuf, byteOffset, "val.gep");
            Builder->CreateStore(dictArgs[i * 2 + 1], gep);
        }

        // 创建 RCCDict struct: { i64 capacity, i64 size, ptr keys, ptr values }
        auto* dictStructType = llvm::StructType::create(*TheContext,
            {llvm::Type::getInt64Ty(*TheContext), llvm::Type::getInt64Ty(*TheContext), getValueType(), getValueType()}, "RCCDict");
        // 使用 malloc 分配 struct（堆内存，生命周期不受函数返回影响）
        auto* dictStructSize = llvm::ConstantInt::get(llvm::Type::getInt64Ty(*TheContext), 32); // i64 + i64 + ptr + ptr = 32 bytes
        auto* dictHeap = Builder->CreateCall(mallocFunc, {dictStructSize}, "dict.struct");

        Builder->CreateStore(llvm::ConstantInt::get(llvm::Type::getInt64Ty(*TheContext), count),
            Builder->CreateStructGEP(dictStructType, dictHeap, 0));
        Builder->CreateStore(llvm::ConstantInt::get(llvm::Type::getInt64Ty(*TheContext), count),
            Builder->CreateStructGEP(dictStructType, dictHeap, 1));
        Builder->CreateStore(keyBuf,
            Builder->CreateStructGEP(dictStructType, dictHeap, 2));
        Builder->CreateStore(valBuf,
            Builder->CreateStructGEP(dictStructType, dictHeap, 3));

        pushValue(createTaggedValue(TAG_DICT, dictHeap));

        LLVM_DEBUG("DictionaryExpressionNode: " << count << " entries");
    }

    // ============================================================================
    // 列表表达式节点
    // ============================================================================

    void LLVMCodeGenVisitor::visitListExpressionNode(ListExpressionNode& node)
    {
        LLVM_DEBUG("ListExpressionNode");

        // 收集所有元素值
        std::vector<llvm::Value*> elemValues;

        if (const auto& bodyNode = node.getBodyNode())
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

        // 纯 IR 实现：用 malloc 分配 RCCList struct 和 data 数组

        // 声明 malloc
        auto* mallocType = llvm::FunctionType::get(getValueType(), {llvm::Type::getInt64Ty(*TheContext)}, false);
        auto* mallocFunc = getOrCreateExternalFunc("malloc", mallocType);

        int64_t count = elemValues.size();

        // 分配 data 数组: malloc(count * ptr_size)
        auto* dataSize = llvm::ConstantInt::get(llvm::Type::getInt64Ty(*TheContext), count * sizeof(void*));
        auto* dataBuf = Builder->CreateCall(mallocFunc, {dataSize}, "list.data");

        // 存储元素（每个元素是 ptr，占 8 字节）
        auto* ptrSize = llvm::ConstantInt::get(llvm::Type::getInt64Ty(*TheContext), sizeof(void*));
        for (int64_t i = 0; i < count; i++)
        {
            auto* idx = llvm::ConstantInt::get(llvm::Type::getInt64Ty(*TheContext), i);
            auto* byteOffset = Builder->CreateMul(idx, ptrSize, "elem.offset");
            auto* gep = Builder->CreateInBoundsGEP(llvm::Type::getInt8Ty(*TheContext), dataBuf, byteOffset, "elem.gep");
            Builder->CreateStore(elemValues[i], gep);
        }

        // 创建 RCCList struct: { i64 capacity, i64 size, ptr data }
        auto* listStructType = llvm::StructType::create(*TheContext,
            {llvm::Type::getInt64Ty(*TheContext), llvm::Type::getInt64Ty(*TheContext), getValueType()}, "RCCList");
        // 使用 malloc 分配 struct（堆内存，生命周期不受函数返回影响）
        auto* listStructSize = llvm::ConstantInt::get(llvm::Type::getInt64Ty(*TheContext), 24); // i64 + i64 + ptr = 24 bytes
        auto* listHeap = Builder->CreateCall(mallocFunc, {listStructSize}, "list.struct");

        Builder->CreateStore(llvm::ConstantInt::get(llvm::Type::getInt64Ty(*TheContext), count),
            Builder->CreateStructGEP(listStructType, listHeap, 0));
        Builder->CreateStore(llvm::ConstantInt::get(llvm::Type::getInt64Ty(*TheContext), count),
            Builder->CreateStructGEP(listStructType, listHeap, 1));
        Builder->CreateStore(dataBuf,
            Builder->CreateStructGEP(listStructType, listHeap, 2));

        pushValue(createTaggedValue(TAG_LIST, listHeap));

        LLVM_DEBUG("ListExpressionNode: " << count << " elements");
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

        if (!targetVal) targetVal = createTaggedValueNull(TAG_NULL);
        if (!indexVal) indexVal = createTaggedValueNull(TAG_NULL);

        // 纯 IR 实现：根据 target 的 type_tag 分发到列表或字典索引
        llvm::Function* func = Builder->GetInsertBlock()->getParent();

        // 提取索引整数值
        auto* indexPayload = extractPayload(indexVal);
        auto* indexInt = Builder->CreatePtrToInt(indexPayload, llvm::Type::getInt64Ty(*TheContext), "index.int");

        // 提取 target 的 payload (指向 RCCList/RCCDict struct)
        auto* targetPayload = extractPayload(targetVal);
        auto* tag = extractTag(targetVal);

        // 结果 alloca（跨基本块传递结果）
        auto* resultAlloca = Builder->CreateAlloca(getValueType(), nullptr, "idx.result");
        Builder->CreateStore(createTaggedValueNull(TAG_NULL), resultAlloca);

        // 基本块
        auto* listBB = llvm::BasicBlock::Create(*TheContext, "idx.list", func);
        auto* dictBB = llvm::BasicBlock::Create(*TheContext, "idx.dict", func);
        auto* defaultBB = llvm::BasicBlock::Create(*TheContext, "idx.default", func);
        auto* mergeBB = llvm::BasicBlock::Create(*TheContext, "idx.merge", func);

        auto* switchInst = Builder->CreateSwitch(tag, defaultBB, 2);
        switchInst->addCase(llvm::ConstantInt::get(llvm::Type::getInt64Ty(*TheContext), TAG_LIST), listBB);
        switchInst->addCase(llvm::ConstantInt::get(llvm::Type::getInt64Ty(*TheContext), TAG_DICT), dictBB);

        // --- 列表索引 ---
        Builder->SetInsertPoint(listBB);
        {
            // RCCList: { i64 capacity, i64 size, ptr data }
            auto* listType = llvm::StructType::create(*TheContext,
                {llvm::Type::getInt64Ty(*TheContext), llvm::Type::getInt64Ty(*TheContext), getValueType()}, "RCCList");
            auto* dataPtr = Builder->CreateStructGEP(listType, targetPayload, 2, "list.data");
            auto* data = Builder->CreateLoad(getValueType(), dataPtr, "data");

            // data[index] - GEP（每个元素是 ptr，占 8 字节）
            auto* ptrSizeConst = llvm::ConstantInt::get(llvm::Type::getInt64Ty(*TheContext), sizeof(void*));
            auto* byteOffset = Builder->CreateMul(indexInt, ptrSizeConst, "idx.byte.offset");
            auto* elemPtr = Builder->CreateInBoundsGEP(llvm::Type::getInt8Ty(*TheContext), data, byteOffset, "elem.ptr");
            auto* elem = Builder->CreateLoad(getValueType(), elemPtr, "elem");
            Builder->CreateStore(elem, resultAlloca);
            Builder->CreateBr(mergeBB);
        }

        // --- 字典索引 ---
        Builder->SetInsertPoint(dictBB);
        {
            // RCCDict: { i64 capacity, i64 size, ptr keys, ptr values }
            auto* dictType = llvm::StructType::create(*TheContext,
                {llvm::Type::getInt64Ty(*TheContext), llvm::Type::getInt64Ty(*TheContext), getValueType(), getValueType()}, "RCCDict");
            auto* sizePtr = Builder->CreateStructGEP(dictType, targetPayload, 1, "dict.size");
            auto* size = Builder->CreateLoad(llvm::Type::getInt64Ty(*TheContext), sizePtr, "size");
            auto* keysPtr = Builder->CreateStructGEP(dictType, targetPayload, 2, "dict.keys");
            auto* keys = Builder->CreateLoad(getValueType(), keysPtr, "keys");
            auto* valsPtr = Builder->CreateStructGEP(dictType, targetPayload, 3, "dict.values");
            auto* vals = Builder->CreateLoad(getValueType(), valsPtr, "vals");

            // 每个元素是 ptr，占 8 字节
            auto* dictElemSize = llvm::ConstantInt::get(llvm::Type::getInt64Ty(*TheContext), sizeof(void*));

            // 线性搜索循环
            auto* loopHeaderBB = llvm::BasicBlock::Create(*TheContext, "dict.search.header", func);
            auto* loopBodyBB = llvm::BasicBlock::Create(*TheContext, "dict.search.body", func);
            auto* foundBB = llvm::BasicBlock::Create(*TheContext, "dict.found", func);
            auto* notFoundBB = llvm::BasicBlock::Create(*TheContext, "dict.notfound", func);
            auto* dictMergeBB = llvm::BasicBlock::Create(*TheContext, "dict.search.merge", func);

            auto* iAlloca = Builder->CreateAlloca(llvm::Type::getInt64Ty(*TheContext), nullptr, "dict.i");
            Builder->CreateStore(llvm::ConstantInt::get(llvm::Type::getInt64Ty(*TheContext), 0), iAlloca);
            Builder->CreateBr(loopHeaderBB);

            // loop header: 检查 i < size
            Builder->SetInsertPoint(loopHeaderBB);
            {
                auto* i = Builder->CreateLoad(llvm::Type::getInt64Ty(*TheContext), iAlloca, "i");
                auto* cond = Builder->CreateICmpSLT(i, size, "in.bounds");
                Builder->CreateCondBr(cond, loopBodyBB, notFoundBB);
            }

            // loop body: 比较 key
            Builder->SetInsertPoint(loopBodyBB);
            {
                auto* bi = Builder->CreateLoad(llvm::Type::getInt64Ty(*TheContext), iAlloca, "i.body");
                auto* keyByteOffset = Builder->CreateMul(bi, dictElemSize, "key.byte.offset");
                auto* keyGep = Builder->CreateInBoundsGEP(llvm::Type::getInt8Ty(*TheContext), keys, keyByteOffset, "key.gep");
                auto* curKey = Builder->CreateLoad(getValueType(), keyGep, "key.val");

                // 比较两个 Tagged Value 的 tag 和 payload
                auto* curKeyTag = extractTag(curKey);
                auto* indexTag = extractTag(indexVal);
                auto* tagEq = Builder->CreateICmpEQ(curKeyTag, indexTag, "tag.eq");

                auto* curKeyPayload = extractPayload(curKey);

                // 对于字符串类型，使用 strcmp 比较内容；其他类型使用指针比较
                // 用条件分支避免对非字符串调用 strcmp（会导致崩溃）
                auto* strTagConst = llvm::ConstantInt::get(llvm::Type::getInt64Ty(*TheContext), TAG_STRING);
                auto* isString = Builder->CreateICmpEQ(curKeyTag, strTagConst, "is.string");

                auto* strCmpBB = llvm::BasicBlock::Create(*TheContext, "cmp.str", func);
                auto* ptrCmpBB = llvm::BasicBlock::Create(*TheContext, "cmp.ptr", func);
                auto* cmpMergeBB = llvm::BasicBlock::Create(*TheContext, "cmp.merge", func);

                Builder->CreateCondBr(isString, strCmpBB, ptrCmpBB);

                // 字符串比较分支
                Builder->SetInsertPoint(strCmpBB);
                llvm::Value* strCmpEq = nullptr;
                {
                    auto* strcmpType = llvm::FunctionType::get(llvm::Type::getInt32Ty(*TheContext), {getValueType(), getValueType()}, false);
                    auto* strcmpFunc = getOrCreateExternalFunc("strcmp", strcmpType);
                    auto* strcmpResult = Builder->CreateCall(strcmpFunc, {curKeyPayload, indexPayload}, "strcmp.result");
                    strCmpEq = Builder->CreateICmpEQ(strcmpResult, llvm::ConstantInt::get(llvm::Type::getInt32Ty(*TheContext), 0), "strcmp.eq");
                    Builder->CreateBr(cmpMergeBB);
                }

                // 指针比较分支
                Builder->SetInsertPoint(ptrCmpBB);
                llvm::Value* ptrEq = nullptr;
                {
                    ptrEq = Builder->CreateICmpEQ(curKeyPayload, indexPayload, "ptr.eq");
                    Builder->CreateBr(cmpMergeBB);
                }

                // 合并
                Builder->SetInsertPoint(cmpMergeBB);
                auto* payloadEq = Builder->CreatePHI(llvm::Type::getInt1Ty(*TheContext), 2, "payload.eq");
                payloadEq->addIncoming(strCmpEq, strCmpBB);
                payloadEq->addIncoming(ptrEq, ptrCmpBB);

                auto* keyEq = Builder->CreateAnd(tagEq, payloadEq, "key.eq");

                // 保存当前索引 bi 到 alloca，供 foundBB 使用
                Builder->CreateStore(bi, iAlloca);

                // 如果 key 匹配，跳到 foundBB；否则递增 i 后继续循环
                auto* nextIterBB = llvm::BasicBlock::Create(*TheContext, "next.iter", func);
                Builder->CreateCondBr(keyEq, foundBB, nextIterBB);

                // next.iter: 递增 i，然后跳到 loopHeaderBB
                Builder->SetInsertPoint(nextIterBB);
                {
                    auto* nextI = Builder->CreateAdd(bi, llvm::ConstantInt::get(llvm::Type::getInt64Ty(*TheContext), 1), "next.i");
                    Builder->CreateStore(nextI, iAlloca);
                    Builder->CreateBr(loopHeaderBB);
                }
            }

            Builder->SetInsertPoint(foundBB);
            {
                // 使用保存的索引读取 value
                auto* fi = Builder->CreateLoad(llvm::Type::getInt64Ty(*TheContext), iAlloca, "i.found");
                auto* valByteOffset = Builder->CreateMul(fi, dictElemSize, "val.byte.offset");
                auto* valGep = Builder->CreateInBoundsGEP(llvm::Type::getInt8Ty(*TheContext), vals, valByteOffset, "val.gep");
                auto* val = Builder->CreateLoad(getValueType(), valGep, "val");
                Builder->CreateStore(val, resultAlloca);
                Builder->CreateBr(dictMergeBB);
            }

            Builder->SetInsertPoint(notFoundBB);
            {
                Builder->CreateBr(dictMergeBB);
            }

            Builder->SetInsertPoint(dictMergeBB);
            Builder->CreateBr(mergeBB);
        }

        // --- 默认 ---
        Builder->SetInsertPoint(defaultBB);
        {
            Builder->CreateBr(mergeBB);
        }

        // --- 合并 ---
        Builder->SetInsertPoint(mergeBB);
        auto* result = Builder->CreateLoad(getValueType(), resultAlloca, "idx.result");
        pushValue(result);
    }

    // ============================================================================
    // Try 节点
    // ============================================================================

    void LLVMCodeGenVisitor::visitTryNode(TryNode& node)
    {
        LLVM_DEBUG("TryNode");

        // 纯 IR 实现：简化为直接执行 try 块（纯 IR 无法实现异常）
        // 如果有 finally 块，在 try 之后执行
        llvm::Function* func = Builder->GetInsertBlock()->getParent();

        auto savedStackSize = ValueStack.size();

        // 创建基本块
        auto* tryBB = llvm::BasicBlock::Create(*TheContext, "try.body", func);
        auto* finallyBB = llvm::BasicBlock::Create(*TheContext, "try.finally", func);
        auto* endBB = llvm::BasicBlock::Create(*TheContext, "try.end", func);

        // 执行 try 块
        Builder->CreateBr(tryBB);
        Builder->SetInsertPoint(tryBB);

        if (node.getTryBody())
        {
            node.getTryBody()->acceptVisitor(*this);
        }

        if (!getCurrentBlock()->hasTerminator())
        {
            Builder->CreateBr(finallyBB);
        }

        // catch 块（简化：纯 IR 不支持异常捕获，跳过 catch 逻辑）
        // 如果有 catch 块，创建一个不可达的基本块
        const auto& catchBodies = node.getCatchBodies();
        if (!catchBodies.empty())
        {
            auto* catchBB = llvm::BasicBlock::Create(*TheContext, "try.catch", func);
            // catchBB 不可达，但需要存在以避免引用错误
            Builder->SetInsertPoint(catchBB);
            Builder->CreateBr(finallyBB);
        }

        // finally 块
        Builder->SetInsertPoint(finallyBB);

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
        pushValue(createTaggedValueNull(TAG_NULL));

        LLVM_DEBUG("TryNode: try-catch-finally generated (simplified, no exception support)");
    }

    // ============================================================================
    // Throw 节点
    // ============================================================================

    void LLVMCodeGenVisitor::visitThrowNode(ThrowNode& node)
    {
        LLVM_DEBUG("ThrowNode");

        // 纯 IR 实现：调用 exit(1) 终止程序
        auto* exitType = llvm::FunctionType::get(getVoidType(), {llvm::Type::getInt32Ty(*TheContext)}, false);
        auto* exitFunc = getOrCreateExternalFunc("exit", exitType);

        // 获取异常表达式值（仅用于求值副作用）
        if (node.getThrowExpression())
        {
            node.getThrowExpression()->acceptVisitor(*this);
            auto* val = popValue();
            (void)val; // 丢弃值
        }

        // 调用 exit(1)
        Builder->CreateCall(exitFunc, {llvm::ConstantInt::get(llvm::Type::getInt32Ty(*TheContext), 1)});

        // throw 后不可达
        Builder->CreateUnreachable();

        // 创建新的基本块供后续代码使用（如果有的话）
        llvm::Function* func = Builder->GetInsertBlock()->getParent();
        auto* unreachableBB = llvm::BasicBlock::Create(*TheContext, "throw.unreachable", func);
        Builder->SetInsertPoint(unreachableBB);

        LLVM_DEBUG("ThrowNode: throw generated (calls exit(1))");
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

    // ============================================================================
    // 内置函数 IR 生成器
    // ============================================================================

    llvm::Function* LLVMCodeGenVisitor::getPrintfFunction() const
    {
        auto* printfType = llvm::FunctionType::get(
            llvm::Type::getInt32Ty(*TheContext),
            {getValueType()},
            /*isVarArg=*/true);
        return getOrCreateExternalFunc("printf", printfType);
    }

    llvm::Function* LLVMCodeGenVisitor::getPutsFunction() const
    {
        auto* putsType = llvm::FunctionType::get(
            llvm::Type::getInt32Ty(*TheContext),
            {getValueType()}, false);
        return getOrCreateExternalFunc("puts", putsType);
    }

    llvm::Value* LLVMCodeGenVisitor::createPrintfCall(
        const std::string& format,
        const std::vector<llvm::Value*>& args) const
    {
        auto* printfFunc = getPrintfFunction();
        auto* fmtStr = createGlobalStringPtr(format);

        std::vector<llvm::Value*> callArgs;
        callArgs.push_back(fmtStr);
        callArgs.insert(callArgs.end(), args.begin(), args.end());

        return Builder->CreateCall(printfFunc, callArgs, "printf");
    }

    void LLVMCodeGenVisitor::registerBuiltinIRGenerators()
    {
        // ==================== IO 函数 ====================

        // sout(*args) - 标准输出（纯 IR 实现，不依赖 __rcc_* 运行时函数）
        // 将所有参数转为字符串并输出，每个参数用空格分隔，最后换行
        BuiltinIRGenerators["sout"] = [this](
            LLVMCodeGenVisitor& visitor,
            const std::vector<llvm::Value*>& args,
            const std::map<std::string, llvm::Value*>& namedArgs,
            const std::string& funcName)
        {
            LLVM_DEBUG("Builtin IR: " << funcName);

            // 声明 libc 函数
            auto* putcharType = llvm::FunctionType::get(llvm::Type::getInt32Ty(*TheContext), {llvm::Type::getInt32Ty(*TheContext)}, false);
            auto* putcharFunc = getOrCreateExternalFunc("putchar", putcharType);

            auto* printfType = llvm::FunctionType::get(llvm::Type::getInt32Ty(*TheContext), {getValueType()}, /*isVarArg=*/true);
            auto* printfFunc = getOrCreateExternalFunc("printf", printfType);

            // int strcmp(const char*, const char*)
            auto* strcmpType = llvm::FunctionType::get(llvm::Type::getInt32Ty(*TheContext), {getValueType(), getValueType()}, false);
            auto* strcmpFunc = getOrCreateExternalFunc("strcmp", strcmpType);

            llvm::Function* func = Builder->GetInsertBlock()->getParent();

            // 解析参数：最后一个命名参数 end 作为结尾符，其余为输出参数
            // sout(a, b, end="\n") -> 输出 a, b, 然后输出 \n
            // 简化实现：所有参数都输出，最后一个参数如果是字符串 "\n" 则作为 end
            // 这里我们直接输出所有参数，然后输出 \n

            // 输出每个参数（参数间不加空格）
            for (size_t i = 0; i < args.size(); i++)
            {
                llvm::Value* arg = args[i];

                auto* tag = extractTag(arg);
                auto* payload = extractPayload(arg);

                auto* intBB = llvm::BasicBlock::Create(*TheContext, "sout.int", func);
                auto* floatBB = llvm::BasicBlock::Create(*TheContext, "sout.float", func);
                auto* boolBB = llvm::BasicBlock::Create(*TheContext, "sout.bool", func);
                auto* strBB = llvm::BasicBlock::Create(*TheContext, "sout.str", func);
                auto* nullBB = llvm::BasicBlock::Create(*TheContext, "sout.null", func);
                auto* defaultBB = llvm::BasicBlock::Create(*TheContext, "sout.default", func);
                auto* mergeBB = llvm::BasicBlock::Create(*TheContext, "sout.merge", func);

                auto* switchInst = Builder->CreateSwitch(tag, defaultBB, 5);
                switchInst->addCase(llvm::ConstantInt::get(llvm::Type::getInt64Ty(*TheContext), TAG_INT), intBB);
                switchInst->addCase(llvm::ConstantInt::get(llvm::Type::getInt64Ty(*TheContext), TAG_FLOAT), floatBB);
                switchInst->addCase(llvm::ConstantInt::get(llvm::Type::getInt64Ty(*TheContext), TAG_BOOL), boolBB);
                switchInst->addCase(llvm::ConstantInt::get(llvm::Type::getInt64Ty(*TheContext), TAG_STRING), strBB);
                switchInst->addCase(llvm::ConstantInt::get(llvm::Type::getInt64Ty(*TheContext), TAG_NULL), nullBB);

                // --- TAG_INT ---
                Builder->SetInsertPoint(intBB);
                {
                    auto* intVal = Builder->CreatePtrToInt(payload, llvm::Type::getInt64Ty(*TheContext), "int.val");
                    auto* fmtStr = createGlobalStringPtr("%lld");
                    Builder->CreateCall(printfFunc, {fmtStr, intVal});
                }
                Builder->CreateBr(mergeBB);

                // --- TAG_FLOAT ---
                Builder->SetInsertPoint(floatBB);
                {
                    auto* bits = Builder->CreatePtrToInt(payload, llvm::Type::getInt64Ty(*TheContext), "float.bits");
                    auto* floatVal = Builder->CreateBitCast(bits, llvm::Type::getDoubleTy(*TheContext), "float.val");
                    auto* fmtStr = createGlobalStringPtr("%g");
                    Builder->CreateCall(printfFunc, {fmtStr, floatVal});
                }
                Builder->CreateBr(mergeBB);

                // --- TAG_BOOL ---
                Builder->SetInsertPoint(boolBB);
                {
                    auto* intVal = Builder->CreatePtrToInt(payload, llvm::Type::getInt64Ty(*TheContext), "bool.val");
                    auto* isTrue = Builder->CreateICmpNE(intVal, llvm::ConstantInt::get(llvm::Type::getInt64Ty(*TheContext), 0), "is.true");

                    auto* trueBB = llvm::BasicBlock::Create(*TheContext, "sout.true", func);
                    auto* falseBB = llvm::BasicBlock::Create(*TheContext, "sout.false", func);
                    auto* boolMergeBB = llvm::BasicBlock::Create(*TheContext, "sout.bool.merge", func);

                    Builder->CreateCondBr(isTrue, trueBB, falseBB);

                    Builder->SetInsertPoint(trueBB);
                    Builder->CreateCall(printfFunc, {createGlobalStringPtr("%s"), createGlobalStringPtr("true")});
                    Builder->CreateBr(boolMergeBB);

                    Builder->SetInsertPoint(falseBB);
                    Builder->CreateCall(printfFunc, {createGlobalStringPtr("%s"), createGlobalStringPtr("false")});
                    Builder->CreateBr(boolMergeBB);

                    Builder->SetInsertPoint(boolMergeBB);
                }
                Builder->CreateBr(mergeBB);

                // --- TAG_STRING ---
                Builder->SetInsertPoint(strBB);
                {
                    Builder->CreateCall(printfFunc, {createGlobalStringPtr("%s"), payload});
                }
                Builder->CreateBr(mergeBB);

                // --- TAG_NULL ---
                Builder->SetInsertPoint(nullBB);
                {
                    Builder->CreateCall(printfFunc, {createGlobalStringPtr("%s"), createGlobalStringPtr("null")});
                }
                Builder->CreateBr(mergeBB);

                // --- default ---
                Builder->SetInsertPoint(defaultBB);
                {
                    auto* fmtStr = createGlobalStringPtr("<ptr: %p>");
                    Builder->CreateCall(printfFunc, {fmtStr, payload});
                }
                Builder->CreateBr(mergeBB);

                Builder->SetInsertPoint(mergeBB);
            }

            // 末尾输出 end 参数（默认 "\n"）
            if (auto endIt = namedArgs.find("end"); endIt != namedArgs.end())
            {
                // end 参数是 Tagged String，提取 payload 并逐字符输出
                llvm::Value* endVal = endIt->second;
                auto* endTag = extractTag(endVal);
                auto* endPayload = extractPayload(endVal);

                // 简化：如果是 TAG_STRING，用 printf("%s") 输出
                // 否则默认输出 \n
                auto* isStr = checkTag(endVal, TAG_STRING);

                auto* strEndBB = llvm::BasicBlock::Create(*TheContext, "sout.end.str", func);
                auto* defaultEndBB = llvm::BasicBlock::Create(*TheContext, "sout.end.default", func);
                auto* endMergeBB = llvm::BasicBlock::Create(*TheContext, "sout.end.merge", func);

                Builder->CreateCondBr(isStr, strEndBB, defaultEndBB);

                Builder->SetInsertPoint(strEndBB);
                Builder->CreateCall(printfFunc, {createGlobalStringPtr("%s"), endPayload});
                Builder->CreateBr(endMergeBB);

                Builder->SetInsertPoint(defaultEndBB);
                Builder->CreateCall(putcharFunc, {llvm::ConstantInt::get(llvm::Type::getInt32Ty(*TheContext), '\n')});
                Builder->CreateBr(endMergeBB);

                Builder->SetInsertPoint(endMergeBB);
            }
            else
            {
                Builder->CreateCall(putcharFunc, {llvm::ConstantInt::get(llvm::Type::getInt32Ty(*TheContext), '\n')});
            }

            pushValue(createTaggedValueNull(TAG_NULL));
        };

        // sin() - 标准输入（纯 IR 实现，使用 getchar 循环读取）
        BuiltinIRGenerators["sin"] = [this](
            LLVMCodeGenVisitor& visitor,
            const std::vector<llvm::Value*>& args,
            const std::map<std::string, llvm::Value*>& namedArgs,
            const std::string& funcName)
        {
            LLVM_DEBUG("Builtin IR: " << funcName);

            // int getchar()
            auto* getcharType = llvm::FunctionType::get(llvm::Type::getInt32Ty(*TheContext), false);
            auto* getcharFunc = getOrCreateExternalFunc("getchar", getcharType);

            // void* malloc(size_t size)
            auto* mallocType = llvm::FunctionType::get(getValueType(), {llvm::Type::getInt64Ty(*TheContext)}, false);
            auto* mallocFunc = getOrCreateExternalFunc("malloc", mallocType);

            // 分配 256 字节缓冲区
            auto* bufSize = llvm::ConstantInt::get(llvm::Type::getInt64Ty(*TheContext), 256);
            auto* buf = Builder->CreateCall(mallocFunc, {bufSize}, "input.buf");

            // 逐字符读取直到换行或 EOF
            llvm::Function* func = Builder->GetInsertBlock()->getParent();
            auto* loopBB = llvm::BasicBlock::Create(*TheContext, "input.loop", func);
            auto* doneBB = llvm::BasicBlock::Create(*TheContext, "input.done", func);

            auto* idxAlloca = Builder->CreateAlloca(llvm::Type::getInt64Ty(*TheContext), nullptr, "input.idx");
            Builder->CreateStore(llvm::ConstantInt::get(llvm::Type::getInt64Ty(*TheContext), 0), idxAlloca);

            Builder->CreateBr(loopBB);
            Builder->SetInsertPoint(loopBB);

            auto* ch = Builder->CreateCall(getcharFunc, {}, "input.ch");
            auto* isNL = Builder->CreateICmpEQ(ch, llvm::ConstantInt::get(llvm::Type::getInt32Ty(*TheContext), '\n'), "is.nl");
            auto* isEOF = Builder->CreateICmpSLT(ch, llvm::ConstantInt::get(llvm::Type::getInt32Ty(*TheContext), 0), "is.eof");
            auto* isDone = Builder->CreateOr(isNL, isEOF, "is.done");
            Builder->CreateCondBr(isDone, doneBB, loopBB);

            // 在 loopBB 中：存储字符并继续
            // 需要在 CreateCondBr 之前插入存储指令
            // 重新设置插入点到 loopBB 开头（在 condBr 之前）
            {
                auto* loopTerm = loopBB->getTerminator();
                llvm::IRBuilder<> loopBuilder(loopBB, loopTerm->getIterator());

                auto* idx = loopBuilder.CreateLoad(llvm::Type::getInt64Ty(*TheContext), idxAlloca, "idx");
                auto* gep = loopBuilder.CreateInBoundsGEP(llvm::Type::getInt8Ty(*TheContext), buf, idx, "buf.gep");
                auto* chTrunc = loopBuilder.CreateTrunc(ch, llvm::Type::getInt8Ty(*TheContext), "ch.trunc");
                loopBuilder.CreateStore(chTrunc, gep);

                auto* nextIdx = loopBuilder.CreateAdd(idx, llvm::ConstantInt::get(llvm::Type::getInt64Ty(*TheContext), 1), "next.idx");
                loopBuilder.CreateStore(nextIdx, idxAlloca);
            }

            Builder->SetInsertPoint(doneBB);
            // 添加 null 终止符
            auto* idx = Builder->CreateLoad(llvm::Type::getInt64Ty(*TheContext), idxAlloca, "final.idx");
            auto* gep = Builder->CreateInBoundsGEP(llvm::Type::getInt8Ty(*TheContext), buf, idx, "null.gep");
            Builder->CreateStore(llvm::ConstantInt::get(llvm::Type::getInt8Ty(*TheContext), 0), gep);

            pushValue(createTaggedString(buf));
        };

        // ==================== 数据结构函数 ====================

        // size(obj) - 获取大小（纯 IR 实现）
        BuiltinIRGenerators["size"] = [this](
            LLVMCodeGenVisitor& visitor,
            const std::vector<llvm::Value*>& args,
            const std::map<std::string, llvm::Value*>& namedArgs,
            const std::string& funcName)
        {
            LLVM_DEBUG("Builtin IR: " << funcName);

            llvm::Value* arg = args.empty()
                ? createTaggedValueNull(TAG_NULL)
                : args[0];

            // 从 Tagged Value 的 payload 中读取列表/字典的 size 字段
            auto* tag = extractTag(arg);
            auto* payload = extractPayload(arg);

            llvm::Function* func = Builder->GetInsertBlock()->getParent();

            // 结果 alloca
            auto* resultAlloca = Builder->CreateAlloca(getValueType(), nullptr, "size.result");
            Builder->CreateStore(createTaggedInt(0), resultAlloca);

            auto* listBB = llvm::BasicBlock::Create(*TheContext, "size.list", func);
            auto* dictBB = llvm::BasicBlock::Create(*TheContext, "size.dict", func);
            auto* defaultBB = llvm::BasicBlock::Create(*TheContext, "size.default", func);
            auto* mergeBB = llvm::BasicBlock::Create(*TheContext, "size.merge", func);

            auto* switchInst = Builder->CreateSwitch(tag, defaultBB, 2);
            switchInst->addCase(llvm::ConstantInt::get(llvm::Type::getInt64Ty(*TheContext), TAG_LIST), listBB);
            switchInst->addCase(llvm::ConstantInt::get(llvm::Type::getInt64Ty(*TheContext), TAG_DICT), dictBB);

            // --- 列表 size ---
            Builder->SetInsertPoint(listBB);
            {
                // RCCList: { i64 capacity, i64 size, ptr data }
                auto* listType = llvm::StructType::create(*TheContext,
                    {llvm::Type::getInt64Ty(*TheContext), llvm::Type::getInt64Ty(*TheContext), getValueType()}, "RCCList");
                auto* sizePtr = Builder->CreateStructGEP(listType, payload, 1, "list.size");
                auto* size = Builder->CreateLoad(llvm::Type::getInt64Ty(*TheContext), sizePtr, "size");
                Builder->CreateStore(createTaggedValue(TAG_INT, Builder->CreateIntToPtr(size, getValueType(), "size.ptr")), resultAlloca);
                Builder->CreateBr(mergeBB);
            }

            // --- 字典 size ---
            Builder->SetInsertPoint(dictBB);
            {
                // RCCDict: { i64 capacity, i64 size, ptr keys, ptr values }
                auto* dictType = llvm::StructType::create(*TheContext,
                    {llvm::Type::getInt64Ty(*TheContext), llvm::Type::getInt64Ty(*TheContext), getValueType(), getValueType()}, "RCCDict");
                auto* sizePtr = Builder->CreateStructGEP(dictType, payload, 1, "dict.size");
                auto* size = Builder->CreateLoad(llvm::Type::getInt64Ty(*TheContext), sizePtr, "size");
                Builder->CreateStore(createTaggedValue(TAG_INT, Builder->CreateIntToPtr(size, getValueType(), "size.ptr")), resultAlloca);
                Builder->CreateBr(mergeBB);
            }

            // --- 默认：返回 0 ---
            Builder->SetInsertPoint(defaultBB);
            {
                Builder->CreateBr(mergeBB);
            }

            Builder->SetInsertPoint(mergeBB);
            auto* result = Builder->CreateLoad(getValueType(), resultAlloca, "size.result");
            pushValue(result);
        };

        // copy(obj) - 浅拷贝（纯 IR 实现：直接返回原值，Tagged Struct 中浅拷贝就是返回同一个指针）
        BuiltinIRGenerators["copy"] = [this](
            LLVMCodeGenVisitor& visitor,
            const std::vector<llvm::Value*>& args,
            const std::map<std::string, llvm::Value*>& namedArgs,
            const std::string& funcName)
        {
            LLVM_DEBUG("Builtin IR: " << funcName);

            llvm::Value* arg = args.empty()
                ? createTaggedValueNull(TAG_NULL)
                : args[0];
            // 浅拷贝：直接返回原值
            pushValue(arg);
        };

        // listAppend(list, elem) - 列表追加（纯 IR 实现）
        BuiltinIRGenerators["listAppend"] = [this](
            LLVMCodeGenVisitor& visitor,
            const std::vector<llvm::Value*>& args,
            const std::map<std::string, llvm::Value*>& namedArgs,
            const std::string& funcName)
        {
            LLVM_DEBUG("Builtin IR: " << funcName);

            llvm::Value* listVal = args.size() > 0 ? args[0] : createTaggedValueNull(TAG_NULL);
            llvm::Value* elem = args.size() > 1 ? args[1] : createTaggedValueNull(TAG_NULL);

            // 声明 malloc 和 memcpy
            auto* mallocType = llvm::FunctionType::get(getValueType(), {llvm::Type::getInt64Ty(*TheContext)}, false);
            auto* mallocFunc = getOrCreateExternalFunc("malloc", mallocType);

            // 提取列表 payload
            auto* payload = extractPayload(listVal);

            // RCCList: { i64 capacity, i64 size, ptr data }
            auto* listType = llvm::StructType::create(*TheContext,
                {llvm::Type::getInt64Ty(*TheContext), llvm::Type::getInt64Ty(*TheContext), getValueType()}, "RCCList");

            auto* capPtr = Builder->CreateStructGEP(listType, payload, 0, "list.cap");
            auto* capacity = Builder->CreateLoad(llvm::Type::getInt64Ty(*TheContext), capPtr, "capacity");

            auto* sizePtr = Builder->CreateStructGEP(listType, payload, 1, "list.size");
            auto* size = Builder->CreateLoad(llvm::Type::getInt64Ty(*TheContext), sizePtr, "size");

            auto* dataPtr = Builder->CreateStructGEP(listType, payload, 2, "list.data");
            // auto* data = Builder->CreateLoad(getValueType(), dataPtr, "data");

            // 检查是否需要扩容
            auto* needGrow = Builder->CreateICmpUGE(size, capacity, "need.grow");

            llvm::Function* func = Builder->GetInsertBlock()->getParent();
            auto* growBB = llvm::BasicBlock::Create(*TheContext, "append.grow", func);
            auto* noGrowBB = llvm::BasicBlock::Create(*TheContext, "append.nogrow", func);
            auto* storeBB = llvm::BasicBlock::Create(*TheContext, "append.store", func);

            Builder->CreateCondBr(needGrow, growBB, noGrowBB);

            // 扩容：分配新数组（容量翻倍或初始为 8）
            Builder->SetInsertPoint(growBB);
            {
                auto* newCap = Builder->CreateSelect(
                    Builder->CreateICmpEQ(capacity, llvm::ConstantInt::get(llvm::Type::getInt64Ty(*TheContext), 0)),
                    llvm::ConstantInt::get(llvm::Type::getInt64Ty(*TheContext), 8),
                    Builder->CreateShl(capacity, llvm::ConstantInt::get(llvm::Type::getInt64Ty(*TheContext), 1)),
                    "new.cap");

                auto* newBufSize = Builder->CreateMul(newCap,
                    llvm::ConstantInt::get(llvm::Type::getInt64Ty(*TheContext), sizeof(void*)), "new.buf.size");
                auto* newBuf = Builder->CreateCall(mallocFunc, {newBufSize}, "new.data");

                // 更新 capacity 和 data
                Builder->CreateStore(newCap, capPtr);
                Builder->CreateStore(newBuf, dataPtr);

                Builder->CreateBr(storeBB);
            }

            Builder->SetInsertPoint(noGrowBB);
            Builder->CreateBr(storeBB);

            // 存储元素
            Builder->SetInsertPoint(storeBB);
            {
                // 重新加载 data（可能已被 grow 分支修改）
                auto* curData = Builder->CreateLoad(getValueType(), dataPtr, "cur.data");
                auto* curSize = Builder->CreateLoad(llvm::Type::getInt64Ty(*TheContext), sizePtr, "cur.size");

                // data[size] = elem
                auto* elemGep = Builder->CreateInBoundsGEP(llvm::Type::getInt8Ty(*TheContext), curData, curSize, "elem.gep");
                Builder->CreateStore(elem, elemGep);

                // size++
                auto* newSize = Builder->CreateAdd(curSize, llvm::ConstantInt::get(llvm::Type::getInt64Ty(*TheContext), 1), "new.size");
                Builder->CreateStore(newSize, sizePtr);
            }

            pushValue(listVal);
        };

        // listRemove(list, index) - 列表删除（纯 IR 简化实现：暂不实现移动操作，返回原列表）
        BuiltinIRGenerators["listRemove"] = [this](
            LLVMCodeGenVisitor& visitor,
            const std::vector<llvm::Value*>& args,
            const std::map<std::string, llvm::Value*>& namedArgs,
            const std::string& funcName)
        {
            LLVM_DEBUG("Builtin IR: " << funcName << " (simplified: returns original list)");

            llvm::Value* list = args.size() > 0 ? args[0] : createTaggedValueNull(TAG_NULL);
            // 简化版：暂不实现移动操作，返回原列表
            pushValue(list);
        };

        // dictRemove(dict, key) - 字典删除（纯 IR 简化实现：暂不实现，返回原字典）
        BuiltinIRGenerators["dictRemove"] = [this](
            LLVMCodeGenVisitor& visitor,
            const std::vector<llvm::Value*>& args,
            const std::map<std::string, llvm::Value*>& namedArgs,
            const std::string& funcName)
        {
            LLVM_DEBUG("Builtin IR: " << funcName << " (simplified: returns original dict)");

            llvm::Value* dict = args.size() > 0 ? args[0] : createTaggedValueNull(TAG_NULL);
            // 简化版：暂不实现，返回原字典
            pushValue(dict);
        };

        // dictKeys(dict) - 获取字典所有键（纯 IR 实现）
        BuiltinIRGenerators["dictKeys"] = [this](
            LLVMCodeGenVisitor& visitor,
            const std::vector<llvm::Value*>& args,
            const std::map<std::string, llvm::Value*>& namedArgs,
            const std::string& funcName)
        {
            LLVM_DEBUG("Builtin IR: " << funcName);

            llvm::Value* dictVal = args.empty()
                ? createTaggedValueNull(TAG_NULL)
                : args[0];

            // 提取字典 payload
            auto* payload = extractPayload(dictVal);

            // RCCDict: { i64 capacity, i64 size, ptr keys, ptr values }
            auto* dictType = llvm::StructType::create(*TheContext,
                {llvm::Type::getInt64Ty(*TheContext), llvm::Type::getInt64Ty(*TheContext), getValueType(), getValueType()}, "RCCDict");

            auto* sizePtr = Builder->CreateStructGEP(dictType, payload, 1, "dict.size");
            auto* size = Builder->CreateLoad(llvm::Type::getInt64Ty(*TheContext), sizePtr, "size");
            auto* keysPtr = Builder->CreateStructGEP(dictType, payload, 2, "dict.keys");
            auto* keys = Builder->CreateLoad(getValueType(), keysPtr, "keys");

            // 创建 RCCList struct: { i64 capacity, i64 size, ptr data }
            // data 直接指向字典的 keys 数组
            auto* listStructType = llvm::StructType::create(*TheContext,
                {llvm::Type::getInt64Ty(*TheContext), llvm::Type::getInt64Ty(*TheContext), getValueType()}, "RCCList");
            auto* listAlloca = Builder->CreateAlloca(listStructType, nullptr, "dict.keys.list");

            Builder->CreateStore(size,
                Builder->CreateStructGEP(listStructType, listAlloca, 0));
            Builder->CreateStore(size,
                Builder->CreateStructGEP(listStructType, listAlloca, 1));
            Builder->CreateStore(keys,
                Builder->CreateStructGEP(listStructType, listAlloca, 2));

            pushValue(createTaggedValue(TAG_LIST, listAlloca));
        };

        // dictValues(dict) - 获取字典所有值（纯 IR 实现）
        BuiltinIRGenerators["dictValues"] = [this](
            LLVMCodeGenVisitor& visitor,
            const std::vector<llvm::Value*>& args,
            const std::map<std::string, llvm::Value*>& namedArgs,
            const std::string& funcName)
        {
            LLVM_DEBUG("Builtin IR: " << funcName);

            llvm::Value* dictVal = args.empty()
                ? createTaggedValueNull(TAG_NULL)
                : args[0];

            // 提取字典 payload
            auto* payload = extractPayload(dictVal);

            // RCCDict: { i64 capacity, i64 size, ptr keys, ptr values }
            auto* dictType = llvm::StructType::create(*TheContext,
                {llvm::Type::getInt64Ty(*TheContext), llvm::Type::getInt64Ty(*TheContext), getValueType(), getValueType()}, "RCCDict");

            auto* sizePtr = Builder->CreateStructGEP(dictType, payload, 1, "dict.size");
            auto* size = Builder->CreateLoad(llvm::Type::getInt64Ty(*TheContext), sizePtr, "size");
            auto* valsPtr = Builder->CreateStructGEP(dictType, payload, 3, "dict.values");
            auto* vals = Builder->CreateLoad(getValueType(), valsPtr, "vals");

            // 创建 RCCList struct: { i64 capacity, i64 size, ptr data }
            // data 直接指向字典的 values 数组
            auto* listStructType = llvm::StructType::create(*TheContext,
                {llvm::Type::getInt64Ty(*TheContext), llvm::Type::getInt64Ty(*TheContext), getValueType()}, "RCCList");
            auto* listAlloca = Builder->CreateAlloca(listStructType, nullptr, "dict.vals.list");

            Builder->CreateStore(size,
                Builder->CreateStructGEP(listStructType, listAlloca, 0));
            Builder->CreateStore(size,
                Builder->CreateStructGEP(listStructType, listAlloca, 1));
            Builder->CreateStore(vals,
                Builder->CreateStructGEP(listStructType, listAlloca, 2));

            pushValue(createTaggedValue(TAG_LIST, listAlloca));
        };

        // ==================== 数据类型函数 ====================

        // type(obj) - 获取类型名（纯 IR 实现：从 type_tag 读取，映射为类型名字符串）
        BuiltinIRGenerators["type"] = [this](
            LLVMCodeGenVisitor& visitor,
            const std::vector<llvm::Value*>& args,
            const std::map<std::string, llvm::Value*>& namedArgs,
            const std::string& funcName)
        {
            LLVM_DEBUG("Builtin IR: " << funcName);

            llvm::Value* arg = args.empty()
                ? createTaggedValueNull(TAG_NULL)
                : args[0];

            auto* tag = extractTag(arg);

            llvm::Function* func = Builder->GetInsertBlock()->getParent();

            // 结果 alloca
            auto* resultAlloca = Builder->CreateAlloca(getValueType(), nullptr, "type.result");
            Builder->CreateStore(createGlobalStringPtr("unknown"), resultAlloca);

            // 各类型分支
            auto* nullBB = llvm::BasicBlock::Create(*TheContext, "type.null", func);
            auto* intBB = llvm::BasicBlock::Create(*TheContext, "type.int", func);
            auto* floatBB = llvm::BasicBlock::Create(*TheContext, "type.float", func);
            auto* boolBB = llvm::BasicBlock::Create(*TheContext, "type.bool", func);
            auto* strBB = llvm::BasicBlock::Create(*TheContext, "type.str", func);
            auto* listBB = llvm::BasicBlock::Create(*TheContext, "type.list", func);
            auto* dictBB = llvm::BasicBlock::Create(*TheContext, "type.dict", func);
            auto* funcBB = llvm::BasicBlock::Create(*TheContext, "type.func", func);
            auto* defaultBB = llvm::BasicBlock::Create(*TheContext, "type.default", func);
            auto* mergeBB = llvm::BasicBlock::Create(*TheContext, "type.merge", func);

            auto* switchInst = Builder->CreateSwitch(tag, defaultBB, 8);
            switchInst->addCase(llvm::ConstantInt::get(llvm::Type::getInt64Ty(*TheContext), TAG_NULL), nullBB);
            switchInst->addCase(llvm::ConstantInt::get(llvm::Type::getInt64Ty(*TheContext), TAG_INT), intBB);
            switchInst->addCase(llvm::ConstantInt::get(llvm::Type::getInt64Ty(*TheContext), TAG_FLOAT), floatBB);
            switchInst->addCase(llvm::ConstantInt::get(llvm::Type::getInt64Ty(*TheContext), TAG_BOOL), boolBB);
            switchInst->addCase(llvm::ConstantInt::get(llvm::Type::getInt64Ty(*TheContext), TAG_STRING), strBB);
            switchInst->addCase(llvm::ConstantInt::get(llvm::Type::getInt64Ty(*TheContext), TAG_LIST), listBB);
            switchInst->addCase(llvm::ConstantInt::get(llvm::Type::getInt64Ty(*TheContext), TAG_DICT), dictBB);
            switchInst->addCase(llvm::ConstantInt::get(llvm::Type::getInt64Ty(*TheContext), TAG_FUNCTION), funcBB);

            Builder->SetInsertPoint(nullBB);
            Builder->CreateStore(createGlobalStringPtr("null"), resultAlloca);
            Builder->CreateBr(mergeBB);

            Builder->SetInsertPoint(intBB);
            Builder->CreateStore(createGlobalStringPtr("int"), resultAlloca);
            Builder->CreateBr(mergeBB);

            Builder->SetInsertPoint(floatBB);
            Builder->CreateStore(createGlobalStringPtr("float"), resultAlloca);
            Builder->CreateBr(mergeBB);

            Builder->SetInsertPoint(boolBB);
            Builder->CreateStore(createGlobalStringPtr("bool"), resultAlloca);
            Builder->CreateBr(mergeBB);

            Builder->SetInsertPoint(strBB);
            Builder->CreateStore(createGlobalStringPtr("string"), resultAlloca);
            Builder->CreateBr(mergeBB);

            Builder->SetInsertPoint(listBB);
            Builder->CreateStore(createGlobalStringPtr("list"), resultAlloca);
            Builder->CreateBr(mergeBB);

            Builder->SetInsertPoint(dictBB);
            Builder->CreateStore(createGlobalStringPtr("dict"), resultAlloca);
            Builder->CreateBr(mergeBB);

            Builder->SetInsertPoint(funcBB);
            Builder->CreateStore(createGlobalStringPtr("function"), resultAlloca);
            Builder->CreateBr(mergeBB);

            Builder->SetInsertPoint(defaultBB);
            Builder->CreateStore(createGlobalStringPtr("unknown"), resultAlloca);
            Builder->CreateBr(mergeBB);

            Builder->SetInsertPoint(mergeBB);
            auto* result = Builder->CreateLoad(getValueType(), resultAlloca, "type.result");
            pushValue(createTaggedString(result));
        };

        // checkType(obj, typeName) - 类型检查（纯 IR 实现：通过 strcmp 映射类型名到 tag，再比较）
        BuiltinIRGenerators["checkType"] = [this](
            LLVMCodeGenVisitor& visitor,
            const std::vector<llvm::Value*>& args,
            const std::map<std::string, llvm::Value*>& namedArgs,
            const std::string& funcName)
        {
            LLVM_DEBUG("Builtin IR: " << funcName);

            llvm::Value* obj = args.size() > 0 ? args[0] : createTaggedValueNull(TAG_NULL);
            llvm::Value* typeNameVal = args.size() > 1 ? args[1] : createTaggedValueNull(TAG_NULL);

            // typeName 是 Tagged String，payload 指向字符串
            auto* typePayload = extractPayload(typeNameVal);
            auto* objTag = extractTag(obj);

            llvm::Function* func = Builder->GetInsertBlock()->getParent();

            // 声明 strcmp
            auto* strcmpType = llvm::FunctionType::get(llvm::Type::getInt32Ty(*TheContext), {getValueType(), getValueType()}, false);
            auto* strcmpFunc = getOrCreateExternalFunc("strcmp", strcmpType);

            // 预创建全局字符串
            auto* nullStr = createGlobalStringPtr("null");
            auto* intStr = createGlobalStringPtr("int");
            auto* floatStr = createGlobalStringPtr("float");
            auto* boolStr = createGlobalStringPtr("bool");
            auto* strStr = createGlobalStringPtr("string");
            auto* listStr = createGlobalStringPtr("list");
            auto* dictStr = createGlobalStringPtr("dict");
            auto* funcStr = createGlobalStringPtr("function");

            // 链式 strcmp 基本块：每个块比较一种类型名，匹配则跳到 noMatchBB（携带对应 tag）
            auto* cmpNullBB = llvm::BasicBlock::Create(*TheContext, "ct.cmp.null", func);
            auto* cmpIntBB = llvm::BasicBlock::Create(*TheContext, "ct.cmp.int", func);
            auto* cmpFloatBB = llvm::BasicBlock::Create(*TheContext, "ct.cmp.float", func);
            auto* cmpBoolBB = llvm::BasicBlock::Create(*TheContext, "ct.cmp.bool", func);
            auto* cmpStrBB = llvm::BasicBlock::Create(*TheContext, "ct.cmp.str", func);
            auto* cmpListBB = llvm::BasicBlock::Create(*TheContext, "ct.cmp.list", func);
            auto* cmpDictBB = llvm::BasicBlock::Create(*TheContext, "ct.cmp.dict", func);
            auto* cmpFuncBB = llvm::BasicBlock::Create(*TheContext, "ct.cmp.func", func);
            auto* noMatchBB = llvm::BasicBlock::Create(*TheContext, "ct.nomatch", func);

            // 入口：跳到第一个比较块
            Builder->CreateBr(cmpNullBB);

            Builder->SetInsertPoint(cmpNullBB);
            {
                auto* cmp = Builder->CreateCall(strcmpFunc, {typePayload, nullStr}, "cmp");
                auto* eq = Builder->CreateICmpEQ(cmp, llvm::ConstantInt::get(llvm::Type::getInt32Ty(*TheContext), 0));
                Builder->CreateCondBr(eq, noMatchBB, cmpIntBB);
            }

            Builder->SetInsertPoint(cmpIntBB);
            {
                auto* cmp = Builder->CreateCall(strcmpFunc, {typePayload, intStr}, "cmp");
                auto* eq = Builder->CreateICmpEQ(cmp, llvm::ConstantInt::get(llvm::Type::getInt32Ty(*TheContext), 0));
                Builder->CreateCondBr(eq, noMatchBB, cmpFloatBB);
            }

            Builder->SetInsertPoint(cmpFloatBB);
            {
                auto* cmp = Builder->CreateCall(strcmpFunc, {typePayload, floatStr}, "cmp");
                auto* eq = Builder->CreateICmpEQ(cmp, llvm::ConstantInt::get(llvm::Type::getInt32Ty(*TheContext), 0));
                Builder->CreateCondBr(eq, noMatchBB, cmpBoolBB);
            }

            Builder->SetInsertPoint(cmpBoolBB);
            {
                auto* cmp = Builder->CreateCall(strcmpFunc, {typePayload, boolStr}, "cmp");
                auto* eq = Builder->CreateICmpEQ(cmp, llvm::ConstantInt::get(llvm::Type::getInt32Ty(*TheContext), 0));
                Builder->CreateCondBr(eq, noMatchBB, cmpStrBB);
            }

            Builder->SetInsertPoint(cmpStrBB);
            {
                auto* cmp = Builder->CreateCall(strcmpFunc, {typePayload, strStr}, "cmp");
                auto* eq = Builder->CreateICmpEQ(cmp, llvm::ConstantInt::get(llvm::Type::getInt32Ty(*TheContext), 0));
                Builder->CreateCondBr(eq, noMatchBB, cmpListBB);
            }

            Builder->SetInsertPoint(cmpListBB);
            {
                auto* cmp = Builder->CreateCall(strcmpFunc, {typePayload, listStr}, "cmp");
                auto* eq = Builder->CreateICmpEQ(cmp, llvm::ConstantInt::get(llvm::Type::getInt32Ty(*TheContext), 0));
                Builder->CreateCondBr(eq, noMatchBB, cmpDictBB);
            }

            Builder->SetInsertPoint(cmpDictBB);
            {
                auto* cmp = Builder->CreateCall(strcmpFunc, {typePayload, dictStr}, "cmp");
                auto* eq = Builder->CreateICmpEQ(cmp, llvm::ConstantInt::get(llvm::Type::getInt32Ty(*TheContext), 0));
                Builder->CreateCondBr(eq, noMatchBB, cmpFuncBB);
            }

            Builder->SetInsertPoint(cmpFuncBB);
            {
                auto* cmp = Builder->CreateCall(strcmpFunc, {typePayload, funcStr}, "cmp");
                auto* eq = Builder->CreateICmpEQ(cmp, llvm::ConstantInt::get(llvm::Type::getInt32Ty(*TheContext), 0));
                Builder->CreateCondBr(eq, noMatchBB, noMatchBB);
            }

            // noMatchBB: 使用 PHI 收集匹配的 tag 值，然后比较
            Builder->SetInsertPoint(noMatchBB);
            {
                auto* tagPhi = Builder->CreatePHI(llvm::Type::getInt64Ty(*TheContext), 9, "matched.tag");
                tagPhi->addIncoming(llvm::ConstantInt::get(llvm::Type::getInt64Ty(*TheContext), TAG_NULL), cmpNullBB);
                tagPhi->addIncoming(llvm::ConstantInt::get(llvm::Type::getInt64Ty(*TheContext), TAG_INT), cmpIntBB);
                tagPhi->addIncoming(llvm::ConstantInt::get(llvm::Type::getInt64Ty(*TheContext), TAG_FLOAT), cmpFloatBB);
                tagPhi->addIncoming(llvm::ConstantInt::get(llvm::Type::getInt64Ty(*TheContext), TAG_BOOL), cmpBoolBB);
                tagPhi->addIncoming(llvm::ConstantInt::get(llvm::Type::getInt64Ty(*TheContext), TAG_STRING), cmpStrBB);
                tagPhi->addIncoming(llvm::ConstantInt::get(llvm::Type::getInt64Ty(*TheContext), TAG_LIST), cmpListBB);
                tagPhi->addIncoming(llvm::ConstantInt::get(llvm::Type::getInt64Ty(*TheContext), TAG_DICT), cmpDictBB);
                tagPhi->addIncoming(llvm::ConstantInt::get(llvm::Type::getInt64Ty(*TheContext), TAG_FUNCTION), cmpFuncBB);
                tagPhi->addIncoming(llvm::ConstantInt::get(llvm::Type::getInt64Ty(*TheContext), -1), cmpFuncBB); // 不匹配任何类型名

                // -1 表示类型名不匹配，一定返回 false
                auto* isValid = Builder->CreateICmpNE(tagPhi, llvm::ConstantInt::get(llvm::Type::getInt64Ty(*TheContext), -1));
                auto* tagMatch = Builder->CreateICmpEQ(objTag, tagPhi);
                auto* match = Builder->CreateAnd(isValid, tagMatch, "checktype.match");
                auto* ext = Builder->CreateZExt(match, llvm::Type::getInt64Ty(*TheContext), "checktype.ext");
                pushValue(Builder->CreateIntToPtr(ext, getValueType(), "checktype.ptr"));
            }
        };

        // ==================== 程序控制函数 ====================

        // entry(func) - 程序入口
        BuiltinIRGenerators["entry"] = [this](
            LLVMCodeGenVisitor& visitor,
            const std::vector<llvm::Value*>& args,
            const std::map<std::string, llvm::Value*>& namedArgs,
            const std::string& funcName)
        {
            LLVM_DEBUG("Builtin IR: " << funcName << " (no-op in LLVM mode)");
            pushValue(llvm::ConstantPointerNull::get(getValueType()));
        };

        // breakpoint() - 调试断点
        BuiltinIRGenerators["breakpoint"] = [this](
            LLVMCodeGenVisitor& visitor,
            const std::vector<llvm::Value*>& args,
            const std::map<std::string, llvm::Value*>& namedArgs,
            const std::string& funcName)
        {
            LLVM_DEBUG("Builtin IR: " << funcName);
            // 调用 debugger trap
            auto* trapType = llvm::FunctionType::get(getVoidType(), false);
            auto* trapFunc = getOrCreateExternalFunc("llvm.debugtrap", trapType);
            Builder->CreateCall(trapFunc, {});
            pushValue(llvm::ConstantPointerNull::get(getValueType()));
        };

        // ==================== 底层函数 ====================

        // id() - 获取标识符
        BuiltinIRGenerators["id"] = [this](
            LLVMCodeGenVisitor& visitor,
            const std::vector<llvm::Value*>& args,
            const std::map<std::string, llvm::Value*>& namedArgs,
            const std::string& funcName)
        {
            LLVM_DEBUG("Builtin IR: " << funcName << " (returns empty string in LLVM mode)");
            pushValue(createGlobalStringPtr(""));
        };

        // rasm(code) - 内联汇编（LLVM 模式下不支持，返回 null）
        BuiltinIRGenerators["rasm"] = [this](
            LLVMCodeGenVisitor& visitor,
            const std::vector<llvm::Value*>& args,
            const std::map<std::string, llvm::Value*>& namedArgs,
            const std::string& funcName)
        {
            LLVM_DEBUG("Builtin IR: " << funcName << " (not supported in LLVM mode)");
            pushValue(llvm::ConstantPointerNull::get(getValueType()));
        };

        // repeat(count, func) - 重复执行（纯 IR 简化实现：空操作）
        BuiltinIRGenerators["repeat"] = [this](
            LLVMCodeGenVisitor& visitor,
            const std::vector<llvm::Value*>& args,
            const std::map<std::string, llvm::Value*>& namedArgs,
            const std::string& funcName)
        {
            LLVM_DEBUG("Builtin IR: " << funcName << " (simplified: no-op in pure IR mode)");
            pushValue(createTaggedValueNull(TAG_NULL));
        };

        // ==================== 模块化函数 ====================
    
    // export(*args) - 标记符号为导出
    BuiltinIRGenerators["export"] = [this](
        LLVMCodeGenVisitor& visitor,
        const std::vector<llvm::Value*>& args,
        const std::map<std::string, llvm::Value*>& namedArgs,
        const std::string& funcName)
    {
        LLVM_DEBUG("Builtin IR: " << funcName);
        // export 在 LLVM 模式下通过 ExportedSymbols 集合标记
        // 参数在编译期处理，此处仅记录
        pushValue(llvm::ConstantPointerNull::get(getValueType()));
    };
    
    // import(path) - 导入模块
    BuiltinIRGenerators["import"] = [this](
        LLVMCodeGenVisitor& visitor,
        const std::vector<llvm::Value*>& args,
        const std::map<std::string, llvm::Value*>& namedArgs,
        const std::string& funcName)
    {
        LLVM_DEBUG("Builtin IR: " << funcName);
        // import 在 LLVM 模式下通过 compileImportedModule 处理
        // 此处仅返回 null（实际导入在编译期处理）
        pushValue(llvm::ConstantPointerNull::get(getValueType()));
    };

    // super(*args, **kwargs) - 调用父类构造函数
    BuiltinIRGenerators["super"] = [this](
        LLVMCodeGenVisitor& visitor,
        const std::vector<llvm::Value*>& args,
        const std::map<std::string, llvm::Value*>& namedArgs,
        const std::string& funcName)
    {
        LLVM_DEBUG("Builtin IR: " << funcName);

        // 检查当前是否在类构造函数中
        if (CurrentClassName.empty() || !ThisAlloca)
        {
            LLVM_DEBUG("Builtin IR: super() called outside of class constructor");
            pushValue(llvm::ConstantPointerNull::get(getValueType()));
            return;
        }

        // 查找父类
        auto parentIt = ClassParentNames.find(CurrentClassName);
        if (parentIt == ClassParentNames.end())
        {
            LLVM_DEBUG("Builtin IR: super() called but class " << CurrentClassName << " has no parent");
            pushValue(llvm::ConstantPointerNull::get(getValueType()));
            return;
        }

        std::string parentName = parentIt->second;
        std::string parentCtorName = parentName + ".__init__";

        // 查找父类构造函数
        auto* parentCtor = TheModule->getFunction(parentCtorName);
        if (!parentCtor)
        {
            // 尝试从 Functions 映射中查找
            auto funcIt = Functions.find(parentCtorName);
            if (funcIt != Functions.end())
            {
                parentCtor = funcIt->second;
            }
        }

        if (!parentCtor)
        {
            LLVM_DEBUG("Builtin IR: parent constructor not found - " << parentCtorName);
            pushValue(llvm::ConstantPointerNull::get(getValueType()));
            return;
        }

        // 准备参数：this 指针 + super 的参数
        std::vector<llvm::Value*> ctorArgs;

        // 第一个参数是 this 指针
        auto* thisVal = Builder->CreateLoad(getValueType(), ThisAlloca, "this.load");
        ctorArgs.push_back(thisVal);

        // 添加 super 的参数（如果有）
        for (const auto& arg : args)
        {
            ctorArgs.push_back(arg);
        }

        // 调用父类构造函数
        Builder->CreateCall(parentCtor, ctorArgs, "super.call");

        LLVM_DEBUG("Builtin IR: super() called " << parentCtorName << " with " << ctorArgs.size() << " args");
        pushValue(llvm::ConstantPointerNull::get(getValueType()));
    };

    // iter(iterData, handler, reverse=false) - iterate over iterable data
    BuiltinIRGenerators["iter"] = [this](
        LLVMCodeGenVisitor& visitor,
        const std::vector<llvm::Value*>& args,
        const std::map<std::string, llvm::Value*>& namedArgs,
        const std::string& funcName)
    {
        LLVM_DEBUG("Builtin IR: " << funcName);

        llvm::Value* iterData = args.size() > 0 ? args[0] : createTaggedValueNull(TAG_NULL);
        llvm::Value* handler = args.size() > 1 ? args[1] : createTaggedValueNull(TAG_NULL);
        
        // Get reverse parameter (default false)
        bool reverse = false;
        if (auto it = namedArgs.find("reverse"); it != namedArgs.end())
        {
            reverse = false; // Simplified for now
        }

        llvm::Function* func = Builder->GetInsertBlock()->getParent();
        auto* dataTag = extractTag(iterData);
        auto* dataPayload = extractPayload(iterData);

        auto* resultAlloca = Builder->CreateAlloca(getValueType(), nullptr, "iter.result");
        Builder->CreateStore(createTaggedBool(true), resultAlloca);

        auto* listBB = llvm::BasicBlock::Create(*TheContext, "iter.list", func);
        auto* dictBB = llvm::BasicBlock::Create(*TheContext, "iter.dict", func);
        auto* strBB = llvm::BasicBlock::Create(*TheContext, "iter.str", func);
        auto* defaultBB = llvm::BasicBlock::Create(*TheContext, "iter.default", func);
        auto* mergeBB = llvm::BasicBlock::Create(*TheContext, "iter.merge", func);

        auto* switchInst = Builder->CreateSwitch(dataTag, defaultBB, 3);
        switchInst->addCase(llvm::ConstantInt::get(llvm::Type::getInt64Ty(*TheContext), TAG_LIST), listBB);
        switchInst->addCase(llvm::ConstantInt::get(llvm::Type::getInt64Ty(*TheContext), TAG_DICT), dictBB);
        switchInst->addCase(llvm::ConstantInt::get(llvm::Type::getInt64Ty(*TheContext), TAG_STRING), strBB);

        // List iteration
        Builder->SetInsertPoint(listBB);
        {
            auto* listType = llvm::StructType::create(*TheContext,
                {llvm::Type::getInt64Ty(*TheContext), llvm::Type::getInt64Ty(*TheContext), getValueType()}, "RCCList");
            
            auto* sizePtr = Builder->CreateStructGEP(listType, dataPayload, 1, "list.size.ptr");
            auto* size = Builder->CreateLoad(llvm::Type::getInt64Ty(*TheContext), sizePtr, "list.size");
            auto* dataPtr = Builder->CreateStructGEP(listType, dataPayload, 2, "list.data.ptr");
            auto* data = Builder->CreateLoad(getValueType(), dataPtr, "list.data");

            auto* condBB = llvm::BasicBlock::Create(*TheContext, "iter.list.cond", func);
            auto* bodyBB = llvm::BasicBlock::Create(*TheContext, "iter.list.body", func);
            auto* afterBB = llvm::BasicBlock::Create(*TheContext, "iter.list.after", func);

            auto* idxAlloca = Builder->CreateAlloca(llvm::Type::getInt64Ty(*TheContext), nullptr, "iter.idx");
            if (reverse)
            {
                Builder->CreateStore(Builder->CreateSub(size, llvm::ConstantInt::get(llvm::Type::getInt64Ty(*TheContext), 1)), idxAlloca);
            }
            else
            {
                Builder->CreateStore(llvm::ConstantInt::get(llvm::Type::getInt64Ty(*TheContext), 0), idxAlloca);
            }
            Builder->CreateBr(condBB);

            Builder->SetInsertPoint(condBB);
            {
                auto* idx = Builder->CreateLoad(llvm::Type::getInt64Ty(*TheContext), idxAlloca, "idx");
                llvm::Value* cond = nullptr;
                if (reverse)
                    cond = Builder->CreateICmpSGE(idx, llvm::ConstantInt::get(llvm::Type::getInt64Ty(*TheContext), 0), "iter.cond");
                else
                    cond = Builder->CreateICmpSLT(idx, size, "iter.cond");
                Builder->CreateCondBr(cond, bodyBB, afterBB);
            }

            Builder->SetInsertPoint(bodyBB);
            {
                auto* idx = Builder->CreateLoad(llvm::Type::getInt64Ty(*TheContext), idxAlloca, "idx");
                auto* ptrSize = llvm::ConstantInt::get(llvm::Type::getInt64Ty(*TheContext), sizeof(void*));
                auto* byteOffset = Builder->CreateMul(idx, ptrSize, "byte.offset");
                auto* elemPtr = Builder->CreateInBoundsGEP(llvm::Type::getInt8Ty(*TheContext), data, byteOffset, "elem.ptr");
                auto* elem = Builder->CreateLoad(getValueType(), elemPtr, "elem");

                auto* handlerTag = extractTag(handler);
                auto* isFunc = Builder->CreateICmpEQ(handlerTag, 
                    llvm::ConstantInt::get(llvm::Type::getInt64Ty(*TheContext), TAG_FUNCTION), "is.func");
                
                auto* callHandlerBB = llvm::BasicBlock::Create(*TheContext, "iter.call.handler", func);
                auto* skipHandlerBB = llvm::BasicBlock::Create(*TheContext, "iter.skip.handler", func);

                Builder->CreateCondBr(isFunc, callHandlerBB, skipHandlerBB);

                Builder->SetInsertPoint(callHandlerBB);
                {
                    auto* handlerPtr = extractPayload(handler);
                    std::vector<llvm::Type*> paramTypes(4, getValueType());
                    auto* handlerType = llvm::FunctionType::get(getValueType(), paramTypes, false);
                    
                    std::vector<llvm::Value*> handlerArgs = {
                        elem,
                        createTaggedInt(0),
                        createTaggedInt(0),
                        createTaggedBool(true)
                    };
                    
                    Builder->CreateCall(handlerType, handlerPtr, handlerArgs, "handler.call");
                    Builder->CreateBr(skipHandlerBB);
                }

                Builder->SetInsertPoint(skipHandlerBB);
                
                auto* curIdx = Builder->CreateLoad(llvm::Type::getInt64Ty(*TheContext), idxAlloca, "cur.idx");
                llvm::Value* nextIdx = nullptr;
                if (reverse)
                    nextIdx = Builder->CreateSub(curIdx, llvm::ConstantInt::get(llvm::Type::getInt64Ty(*TheContext), 1), "next.idx");
                else
                    nextIdx = Builder->CreateAdd(curIdx, llvm::ConstantInt::get(llvm::Type::getInt64Ty(*TheContext), 1), "next.idx");
                Builder->CreateStore(nextIdx, idxAlloca);
                Builder->CreateBr(condBB);
            }

            Builder->SetInsertPoint(afterBB);
            Builder->CreateBr(mergeBB);
        }

        // Dict iteration (simplified)
        Builder->SetInsertPoint(dictBB);
        Builder->CreateBr(mergeBB);

        // String iteration (simplified)
        Builder->SetInsertPoint(strBB);
        Builder->CreateBr(mergeBB);

        // Default (non-iterable)
        Builder->SetInsertPoint(defaultBB);
        {
            Builder->CreateStore(createTaggedBool(false), resultAlloca);
            Builder->CreateBr(mergeBB);
        }

        Builder->SetInsertPoint(mergeBB);
        auto* result = Builder->CreateLoad(getValueType(), resultAlloca, "iter.result.load");
        pushValue(result);
    };

    // forInRange(start, end, handler, sep=1) - iterate over numeric range
    BuiltinIRGenerators["forInRange"] = [this](
        LLVMCodeGenVisitor& visitor,
        const std::vector<llvm::Value*>& args,
        const std::map<std::string, llvm::Value*>& namedArgs,
        const std::string& funcName)
    {
        LLVM_DEBUG("Builtin IR: " << funcName);

        llvm::Value* startVal = args.size() > 0 ? args[0] : createTaggedInt(0);
        llvm::Value* endVal = args.size() > 1 ? args[1] : createTaggedInt(0);
        llvm::Value* handler = args.size() > 2 ? args[2] : createTaggedValueNull(TAG_NULL);
        
        llvm::Value* sepVal = createTaggedInt(1);
        if (auto it = namedArgs.find("sep"); it != namedArgs.end())
        {
            sepVal = it->second;
        }

        llvm::Function* func = Builder->GetInsertBlock()->getParent();

        auto* startPayload = extractPayload(startVal);
        auto* endPayload = extractPayload(endVal);
        auto* sepPayload = extractPayload(sepVal);
        
        auto* startInt = Builder->CreatePtrToInt(startPayload, llvm::Type::getInt64Ty(*TheContext), "start.int");
        auto* endInt = Builder->CreatePtrToInt(endPayload, llvm::Type::getInt64Ty(*TheContext), "end.int");
        auto* sepInt = Builder->CreatePtrToInt(sepPayload, llvm::Type::getInt64Ty(*TheContext), "sep.int");

        auto* isZeroSep = Builder->CreateICmpEQ(sepInt, llvm::ConstantInt::get(llvm::Type::getInt64Ty(*TheContext), 0), "is.zero.sep");
        auto* zeroSepBB = llvm::BasicBlock::Create(*TheContext, "range.zero.sep", func);
        auto* validSepBB = llvm::BasicBlock::Create(*TheContext, "range.valid.sep", func);
        auto* afterCheckBB = llvm::BasicBlock::Create(*TheContext, "range.after.check", func);

        Builder->CreateCondBr(isZeroSep, zeroSepBB, validSepBB);

        Builder->SetInsertPoint(zeroSepBB);
        {
            auto* printfType = llvm::FunctionType::get(llvm::Type::getInt32Ty(*TheContext), {getValueType()}, /*isVarArg=*/true);
            auto* printfFunc = getOrCreateExternalFunc("printf", printfType);
            Builder->CreateCall(printfFunc, {createGlobalStringPtr("The sep cannot be 0\\n")});
            Builder->CreateBr(afterCheckBB);
        }

        Builder->SetInsertPoint(validSepBB);
        {
            auto* condBB = llvm::BasicBlock::Create(*TheContext, "range.cond", func);
            auto* bodyBB = llvm::BasicBlock::Create(*TheContext, "range.body", func);
            auto* afterBB = llvm::BasicBlock::Create(*TheContext, "range.after", func);

            auto* isIncreasing = Builder->CreateICmpSGT(sepInt, llvm::ConstantInt::get(llvm::Type::getInt64Ty(*TheContext), 0), "is.increasing");

            auto* curAlloca = Builder->CreateAlloca(llvm::Type::getInt64Ty(*TheContext), nullptr, "range.cur");
            Builder->CreateStore(startInt, curAlloca);
            Builder->CreateBr(condBB);

            Builder->SetInsertPoint(condBB);
            {
                auto* cur = Builder->CreateLoad(llvm::Type::getInt64Ty(*TheContext), curAlloca, "cur");
                auto* incCond = Builder->CreateICmpSLT(cur, endInt, "range.inc.cond");
                auto* decCond = Builder->CreateICmpSGT(cur, endInt, "range.dec.cond");
                auto* cond = Builder->CreateSelect(isIncreasing, incCond, decCond, "range.cond");
                Builder->CreateCondBr(cond, bodyBB, afterBB);
            }

            Builder->SetInsertPoint(bodyBB);
            {
                auto* cur = Builder->CreateLoad(llvm::Type::getInt64Ty(*TheContext), curAlloca, "cur");
                
                auto* handlerTag = extractTag(handler);
                auto* isFunc = Builder->CreateICmpEQ(handlerTag, 
                    llvm::ConstantInt::get(llvm::Type::getInt64Ty(*TheContext), TAG_FUNCTION), "is.func");
                
                auto* callHandlerBB = llvm::BasicBlock::Create(*TheContext, "range.call.handler", func);
                auto* skipHandlerBB = llvm::BasicBlock::Create(*TheContext, "range.skip.handler", func);

                Builder->CreateCondBr(isFunc, callHandlerBB, skipHandlerBB);

                Builder->SetInsertPoint(callHandlerBB);
                {
                    auto* handlerPtr = extractPayload(handler);
                    std::vector<llvm::Type*> paramTypes(3, getValueType());
                    auto* handlerType = llvm::FunctionType::get(getValueType(), paramTypes, false);
                    
                    auto* isFirst = Builder->CreateICmpEQ(cur, startInt, "is.first");
                    auto* nextVal = Builder->CreateAdd(cur, sepInt, "next.val");
                    auto* incLastCond = Builder->CreateICmpSGE(nextVal, endInt, "last.inc.cond");
                    auto* decLastCond = Builder->CreateICmpSLE(nextVal, endInt, "last.dec.cond");
                    auto* isLast = Builder->CreateSelect(isIncreasing, incLastCond, decLastCond, "is.last");

                    std::vector<llvm::Value*> handlerArgs = {
                        createTaggedValue(TAG_INT, Builder->CreateIntToPtr(cur, getValueType())),
                        createTaggedBool(true),
                        createTaggedBool(false)
                    };
                    
                    Builder->CreateCall(handlerType, handlerPtr, handlerArgs, "handler.call");
                    Builder->CreateBr(skipHandlerBB);
                }

                Builder->SetInsertPoint(skipHandlerBB);
                
                auto* newCur = Builder->CreateAdd(cur, sepInt, "new.cur");
                Builder->CreateStore(newCur, curAlloca);
                Builder->CreateBr(condBB);
            }

            Builder->SetInsertPoint(afterBB);
            Builder->CreateBr(afterCheckBB);
        }

        Builder->SetInsertPoint(afterCheckBB);
        pushValue(createTaggedValueNull(TAG_NULL));
    };

    LLVM_DEBUG("Registered " << BuiltinIRGenerators.size() << " builtin IR generators");
    }

    bool LLVMCodeGenVisitor::tryEmitBuiltinIR(
        const std::string& funcName,
        const std::vector<llvm::Value*>& args,
        const std::map<std::string, llvm::Value*>& namedArgs)
    {
        // 对于已注册的内置函数 IR 生成器，始终尝试匹配
        // 不再要求函数名必须在 EncapsulatedFunctions 集合中
        // 因为标准库函数（如 sout）是通过 import 导入的，不会被标记为 encapsulated
        auto it = BuiltinIRGenerators.find(funcName);
        if (it != BuiltinIRGenerators.end())
        {
            it->second(*this, args, namedArgs, funcName);
            return true;
        }
        return false;
    }

    // ============================================================================
    // 模块导入与合并
    // ============================================================================

    bool LLVMCodeGenVisitor::compileImportedModule(const std::string& importPath)
    {
        // 检查循环导入
        if (ImportedModules.contains(importPath))
        {
            LLVM_DEBUG("Import: skipping already imported: " << importPath);
            return true;
        }
        ImportedModules.insert(importPath);

        LLVM_DEBUG("Import: compiling " << importPath);

        // 创建新的 LLVMCodeGenVisitor 编译导入的文件
        LLVMCodeGenVisitor importVisitor(ModuleName + ".import");
        importVisitor.enableDebug(DebugMode);

        if (!importVisitor.compile(importPath))
        {
            LLVM_DEBUG("Import: failed to compile " << importPath);
            return false;
        }

        // 合并导入模块的函数声明到当前模块
        mergeModuleDeclarations(importVisitor.getModule());

        LLVM_DEBUG("Import: successfully compiled " << importPath);
        return true;
    }

    void LLVMCodeGenVisitor::mergeModuleDeclarations(llvm::Module* sourceModule) const
    {
        if (!sourceModule) return;

        // 遍历源模块的所有函数
        for (auto& func : *sourceModule)
        {
            // 跳过已在当前模块中存在的函数
            if (TheModule->getFunction(func.getName()))
            {
                continue;
            }

            // 创建函数声明（不复制函数体）
            auto* decl = llvm::Function::Create(
                llvm::cast<llvm::FunctionType>(func.getValueType()),
                func.getLinkage(),
                func.getName(),
                TheModule.get());

            // 复制参数属性
            for (auto& arg : func.args())
            {
                auto* declArg = decl->getArg(arg.getArgNo());
                declArg->setName(arg.getName());
            }

            LLVM_DEBUG("  merged declaration: " << func.getName());
        }

        // 遍历源模块的所有全局变量
        for (auto& global : sourceModule->globals())
        {
            if (TheModule->getGlobalVariable(global.getName()))
            {
                continue;
            }

            auto* newGlobal = new llvm::GlobalVariable(
                *TheModule,
                global.getValueType(),
                global.isConstant(),
                global.getLinkage(),
                global.getInitializer(),
                global.getName());
            newGlobal->setAlignment(global.getAlign());
        }
    }

    // ============================================================================
    // 增强赋值运算符实现
    // ============================================================================

    void LLVMCodeGenVisitor::handleCompoundAssignment(InfixExpressionNode& node)
    {
        const auto& opStr = node.getOpToken().getValue();
        
        // 获取左操作数（必须是标识符）
        std::string varName;
        if (auto* ident = dynamic_cast<IdentifierNode*>(node.getLeftNode().get()))
        {
            varName = ident->getName();
        }
        else
        {
            LLVM_DEBUG("CompoundAssignment: left operand must be an identifier");
            pushValue(llvm::ConstantPointerNull::get(getValueType()));
            return;
        }

        // 计算右操作数
        node.getRightNode()->acceptVisitor(*this);
        auto* rhs = popValue();

        // 生成增强赋值 IR
        auto* result = emitCompoundAssign(varName, opStr, rhs);
        pushValue(result);
    }

    llvm::Value* LLVMCodeGenVisitor::emitCompoundAssign(
        const std::string& varName,
        const std::string& opStr,
        llvm::Value* rhs)
    {
        // 加载当前值
        auto* currentVal = loadVariable(varName);
        if (!currentVal)
        {
            LLVM_DEBUG("CompoundAssign: variable not found - " << varName);
            return rhs ? rhs : llvm::ConstantPointerNull::get(getValueType());
        }

        if (!rhs)
        {
            rhs = llvm::ConstantPointerNull::get(getValueType());
        }

        auto* int64Ty = llvm::Type::getInt64Ty(*TheContext);
        auto* doubleTy = llvm::Type::getDoubleTy(*TheContext);

        // 检查操作数类型
        auto* currentTag = extractTag(currentVal);
        auto* rhsTag = extractTag(rhs);
        auto* floatTagConst = llvm::ConstantInt::get(int64Ty, TAG_FLOAT);
        auto* isFloatCurrent = Builder->CreateICmpEQ(currentTag, floatTagConst, "is_float.current");
        auto* isFloatRhs = Builder->CreateICmpEQ(rhsTag, floatTagConst, "is_float.rhs");
        auto* isFloat = Builder->CreateOr(isFloatCurrent, isFloatRhs, "is_float");

        auto* currentFunc = Builder->GetInsertBlock()->getParent();
        auto* floatBB = llvm::BasicBlock::Create(*TheContext, "compound.float", currentFunc);
        auto* intBB = llvm::BasicBlock::Create(*TheContext, "compound.int", currentFunc);
        auto* mergeBB = llvm::BasicBlock::Create(*TheContext, "compound.merge", currentFunc);

        auto* resultAlloca = createEntryBlockAlloca(currentFunc, "compound.result", getValueType());
        Builder->CreateStore(currentVal, resultAlloca);

        Builder->CreateCondBr(isFloat, floatBB, intBB);

        // --- 浮点数运算 ---
        Builder->SetInsertPoint(floatBB);
        {
            auto* currentPayload = extractPayload(currentVal);
            auto* rhsPayload = extractPayload(rhs);
            auto* currentBits = Builder->CreatePtrToInt(currentPayload, int64Ty, "current.bits");
            auto* rhsBits = Builder->CreatePtrToInt(rhsPayload, int64Ty, "rhs.bits");

            auto* currentDouble = Builder->CreateSelect(
                isFloatCurrent,
                Builder->CreateBitCast(currentBits, doubleTy, "current.bitcast"),
                Builder->CreateSIToFP(currentBits, doubleTy, "current.sitofp"),
                "current.double");

            auto* rhsDouble = Builder->CreateSelect(
                isFloatRhs,
                Builder->CreateBitCast(rhsBits, doubleTy, "rhs.bitcast"),
                Builder->CreateSIToFP(rhsBits, doubleTy, "rhs.sitofp"),
                "rhs.double");

            llvm::Value* resultDouble = nullptr;
            if (opStr == "+=")
                resultDouble = Builder->CreateFAdd(currentDouble, rhsDouble, "compound.fadd");
            else if (opStr == "-=")
                resultDouble = Builder->CreateFSub(currentDouble, rhsDouble, "compound.fsub");
            else if (opStr == "*=")
                resultDouble = Builder->CreateFMul(currentDouble, rhsDouble, "compound.fmul");
            else if (opStr == "/=")
                resultDouble = Builder->CreateFDiv(currentDouble, rhsDouble, "compound.fdiv");
            else if (opStr == "%=")
                resultDouble = Builder->CreateFRem(currentDouble, rhsDouble, "compound.frem");
            else
                resultDouble = currentDouble;

            auto* resultBits = Builder->CreateBitCast(resultDouble, int64Ty, "result.bits");
            auto* resultPtr = Builder->CreateIntToPtr(resultBits, getValueType(), "result.ptr");
            auto* taggedResult = createTaggedValue(TAG_FLOAT, resultPtr);
            Builder->CreateStore(taggedResult, resultAlloca);
            Builder->CreateBr(mergeBB);
        }

        // --- 整数运算 ---
        Builder->SetInsertPoint(intBB);
        {
            auto* currentPayload = extractPayload(currentVal);
            auto* rhsPayload = extractPayload(rhs);
            auto* currentInt = Builder->CreatePtrToInt(currentPayload, int64Ty, "current.int");
            auto* rhsInt = Builder->CreatePtrToInt(rhsPayload, int64Ty, "rhs.int");

            llvm::Value* resultInt = nullptr;
            if (opStr == "+=")
                resultInt = Builder->CreateAdd(currentInt, rhsInt, "compound.add");
            else if (opStr == "-=")
                resultInt = Builder->CreateSub(currentInt, rhsInt, "compound.sub");
            else if (opStr == "*=")
                resultInt = Builder->CreateMul(currentInt, rhsInt, "compound.mul");
            else if (opStr == "/=")
                resultInt = Builder->CreateSDiv(currentInt, rhsInt, "compound.div");
            else if (opStr == "%=")
                resultInt = Builder->CreateSRem(currentInt, rhsInt, "compound.rem");
            else
                resultInt = currentInt;

            auto* resultPtr = Builder->CreateIntToPtr(resultInt, getValueType(), "result.ptr");
            auto* taggedResult = createTaggedValue(TAG_INT, resultPtr);
            Builder->CreateStore(taggedResult, resultAlloca);
            Builder->CreateBr(mergeBB);
        }

        // --- 合并 ---
        Builder->SetInsertPoint(mergeBB);
        auto* result = Builder->CreateLoad(getValueType(), resultAlloca, "compound.result");
        
        // 存储回变量
        storeVariable(varName, result);
        
        return result;
    }

} // namespace ast

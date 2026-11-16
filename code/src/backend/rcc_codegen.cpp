#include <algorithm>
// rcc_codegen.cpp —— AST -> x86-64 汇编
//
// 采用「朴素栈式」策略：每个表达式结果放 %rax（或 %xmm0），
// 二元运算用 push/pop 暂存。P0 优先正确性，不追求寄存器分配质量。
//
// 栈帧约定（Win64）：
//   进入函数时 rsp % 16 == 8（返回地址已压栈）
//   push %rbp          -> rsp % 16 == 0
//   mov %rsp, %rbp
//   sub $frameSize, %rsp   其中 frameSize 为 16 的倍数
//   因此所有 call 前 rsp 保持 16 对齐
//
#include "../../include/backend/rcc_codegen.h"
#include "../../include/backend/rcc_symbol_naming.h"
#include "../../include/rcc_base.h"
#include "../../include/analyzer/rcc_ast_components.h"
#include <cctype>
#include <cstring>
#include <functional>
#include <iostream>
#include <stdexcept>
namespace backend{
    namespace    {
        // 节点类型名 -> 运算符
        struct BinOpInfo        {
            const char* asmOp;
      // addq / subq / imulq ...
            const char* setCC;
      // setl / setg ...（比较类）
            bool isCompare;
        }
;
        // 从形如 "a: int" 的标签里取类型名。
        //
        // 注意：这个函数**只看裸名字**，`[...]` 里的内容它拿不到 ——
        // `getFullLabel()` 把 labelDesNodes 整个丢掉了。所以
        // `list[float]` 到这里只剩 "list"，元素类型必须由
        // CodeGen::typeFromLabelNode 从 LabelNode 直接读。
        // 这里只做「认不出名字时」的兜底。
        sema::TypePtr parseTypeName(const std::string& raw)        {
            std::string name;
            for (const char c : raw)            {
                if (std::isalnum(static_cast<unsigned char>(c)) || c == '_') name += c;
                else if (!name.empty()) break;
   // 取第一段
            }
            if (name.empty()) return sema::getUnknownType();
            if (name == "void")  return sema::getVoidType();
            if (name == "bool")  return sema::getBoolType();
            if (name == "char")  return sema::getCharType();
            if (name == "int")   return sema::getIntType();
            if (name == "float") return sema::getFloatType();
            if (name == "str")   return sema::getStrType();
            if (name == "ptr")   return sema::getPtrType();
            // P12/G13：`any` 作为显式标注。原仓库 builtInTypes 里有 any
            // （rcc_symbol.cpp:650），ds.rio:32 `fun copy(arg): any`、
            // math.rio:26 `fun pow(a, times: int): any` 都在用。
            // 映射成 unknown = 「不标注」，语义正好等价（checkAssignable /
            // sameValueFamily 对 unknown 一律放行）。
            if (name == "any")   return sema::getUnknownType();
            // P8：容器形参标注。没有元素类型信息时给一个「元素按 int 解释」的
            // 保守容器类型 —— 关键是让 `xs[i]` 知道左边是 list 而不是 int，
            // 否则会落到「Index access on a non-container value」。
            if (name == "list" || name == "series" || name == "array")            {
                return std::make_shared<sema::ListType>(sema::getIntType());
            }
            if (name == "dict" || name == "map")            {
                // Rio 的字典键以字符串为主，按 str 键构造（比较时按内容比）。
                return std::make_shared<sema::DictType>(sema::getStrType(), sema::getIntType());
            }
            // P10：`funi[params][ret]` —— 函数类型标注。
            // 裸名字到这里时元素类型已丢，只能给一个「参数未知」的占位，
            // 但至少让 `f: funi[...]` 不再报「Unknown type 'funi'」。
            if (name == "funi" || name == "fun")            {
                return std::make_shared<sema::FunctionType>(
                    std::vector<sema::TypePtr>{}, sema::getUnknownType());
            }
            return sema::getUnknownType();
        }
        // 从函数定义的 labelNodes 里取返回类型（`fun f(): int` 的 `int`）
        sema::TypePtr returnTypeOf(const std::shared_ptr<ast::FunctionDefinitionNode>& node)        {
            if (!node) return sema::getUnknownType();
            for (const auto& label : node->getLabelNodes())            {
                if (!label) continue;
                const auto t = parseTypeName(label->getFullLabel());
                if (t->getKind() != sema::TypeKind::TK_UNKNOWN) return t;
            }
            return sema::getUnknownType();
        }
        // 从形参 IdentifierNode 的 labels 里取类型（`a: int` 的 `int`）
        sema::TypePtr paramTypeOf(const std::shared_ptr<ast::ExpressionNode>& paramNode)        {
            if (const auto id = std::dynamic_pointer_cast<ast::IdentifierNode>(paramNode))            {
                for (const auto& label : id->getLabels())                {
                    if (!label) continue;
                    const auto t = parseTypeName(label->getFullLabel());
                    if (t->getKind() != sema::TypeKind::TK_UNKNOWN) return t;
                }
            }
            return sema::getUnknownType();
        }
        // 与 collectParamNames 并行，收集每个形参的静态类型（未知时返回 TK_UNKNOWN）
        void collectParamTypes(const std::shared_ptr<ast::ExpressionNode>& node,                               std::vector<sema::TypePtr>& out)        {
            if (!node) return;
            if (std::dynamic_pointer_cast<ast::IdentifierNode>(node))            {
                out.push_back(paramTypeOf(node));
                return;
            }
            if (const auto infix = std::dynamic_pointer_cast<ast::InfixExpressionNode>(node))            {
                if (infix->getOpToken().getType() == core::TokenType::TOKEN_COMMA)                {
                    collectParamTypes(infix->getLeftNode(), out);
                    collectParamTypes(infix->getRightNode(), out);
                    return;
                }
                // `x: float` 形态：左边是形参名，右边是类型名。
                // 匿名函数的形参类型标注 `(x: float) -> {...}` 就落在这里，
                // 命名函数的 `fun f(a: int)` 也可能走这条路径。
                if (infix->getOpToken().getType() == core::TokenType::TOKEN_COLON)                {
                    out.push_back(parseTypeName(infix->getRightNode()                                                    ? infix->getRightNode()->toString()                                                    : std::string{}));
                    return;
                }
                collectParamTypes(infix->getLeftNode(), out);
                return;
            }
            if (const auto block = std::dynamic_pointer_cast<ast::BlockRangerNode>(node))            {
                for (const auto& e : block->getBodyExpressions()) collectParamTypes(e, out);
                return;
            }
            if (const auto paren = std::dynamic_pointer_cast<ast::ParenRangerNode>(node))            {
                collectParamTypes(paren->getRangerNode(), out);
                return;
            }
            if (const auto list = std::dynamic_pointer_cast<ast::ListExpressionNode>(node))            {
                if (const auto body = std::dynamic_pointer_cast<ast::ExpressionNode>(list->getBodyNode()))                {
                    collectParamTypes(body, out);
                }
                return;
            }
            if (std::dynamic_pointer_cast<ast::ParameterNode>(node))            {
                const std::string text = node->toString();
                const auto colon = text.find(':');
                out.push_back(colon == std::string::npos                    ? sema::getUnknownType()                    : parseTypeName(text.substr(colon + 1)));
            }
        }
        // 收集形参默认值：AST 里 `b: int = 2` 是 AssignmentNode(target=b, value=2)。
        // 缺省实参的补齐在调用点做，所以这里只登记「名字 -> 默认值表达式」。
        void collectParamDefaults(const std::shared_ptr<ast::ExpressionNode>& node,
                                  std::map<std::string, std::shared_ptr<ast::ExpressionNode>>& out)
        {
            if (!node) return;
            if (const auto assign = std::dynamic_pointer_cast<ast::AssignmentNode>(node))
            {
                if (const auto target =
                        std::dynamic_pointer_cast<ast::IdentifierNode>(assign->getAssignPair().first))
                {
                    out[target->getName()] = assign->getAssignPair().second;
                }
                return;
            }
            if (const auto infix = std::dynamic_pointer_cast<ast::InfixExpressionNode>(node))
            {
                if (infix->getOpToken().getType() == core::TokenType::TOKEN_COMMA)
                {
                    collectParamDefaults(infix->getLeftNode(), out);
                    collectParamDefaults(infix->getRightNode(), out);
                    return;
                }
                collectParamDefaults(infix->getLeftNode(), out);
                return;
            }
            if (const auto block = std::dynamic_pointer_cast<ast::BlockRangerNode>(node))
            {
                for (const auto& e : block->getBodyExpressions()) collectParamDefaults(e, out);
                return;
            }
            if (const auto paren = std::dynamic_pointer_cast<ast::ParenRangerNode>(node))
            {
                collectParamDefaults(paren->getRangerNode(), out);
            }
        }
        // 递归收集形参名。Rio 的形参在 AST 里以多种嵌套形态出现：        //   IdentifierNode("a")                         单参数
        //   InfixExpressionNode(',', left, right)       逗号分隔的多个参数
        //   BlockRangerNode{...}                        括号包裹
        //   ParameterNode                               带类型/默认值的参数
        void collectParamNames(const std::shared_ptr<ast::ExpressionNode>& node,                               std::vector<std::string>& out)        {
            if (!node) return;
            if (const auto id = std::dynamic_pointer_cast<ast::IdentifierNode>(node))            {
                out.push_back(id->getName());
                return;
            }
            if (const auto infix = std::dynamic_pointer_cast<ast::InfixExpressionNode>(node))            {
                // 逗号连接：左右都是参数
                if (infix->getOpToken().getType() == core::TokenType::TOKEN_COMMA)                {
                    collectParamNames(infix->getLeftNode(), out);
                    collectParamNames(infix->getRightNode(), out);
                    return;
                }
                // 带默认值 a=1 之类的形态：取左边作为参数名
                collectParamNames(infix->getLeftNode(), out);
                return;
            }
            // `b: int = 2` 形参在 AST 里是 AssignmentNode，名字在 target 上
            if (const auto assign = std::dynamic_pointer_cast<ast::AssignmentNode>(node))            {
                collectParamNames(assign->getAssignPair().first, out);
                return;
            }
            if (const auto block = std::dynamic_pointer_cast<ast::BlockRangerNode>(node))            {
                for (const auto& e : block->getBodyExpressions())                {
                    collectParamNames(e, out);
                }
                return;
            }
            if (const auto paren = std::dynamic_pointer_cast<ast::ParenRangerNode>(node))            {
                collectParamNames(paren->getRangerNode(), out);
                return;
            }
            if (const auto list = std::dynamic_pointer_cast<ast::ListExpressionNode>(node))            {
                if (const auto body = std::dynamic_pointer_cast<ast::ExpressionNode>(list->getBodyNode()))                {
                    collectParamNames(body, out);
                }
                return;
            }
            // ParameterNode：从 toString 里抠出名字（形如 "a: any"）
            if (std::dynamic_pointer_cast<ast::ParameterNode>(node))            {
                const std::string text = node->toString();
                const auto colon = text.find(':');
                std::string name = colon == std::string::npos ? text : text.substr(0, colon);
                // 去掉可能的前后缀符号
                std::string cleaned;
                for (const char c : name)                {
                    if (std::isalnum(static_cast<unsigned char>(c)) || c == '_') cleaned += c;
                }
                if (!cleaned.empty()) out.push_back(cleaned);
            }
        }
        // 递归收集调用实参，结构与 collectParamNames 类似，但保留表达式
        void collectArgs(const std::shared_ptr<ast::ExpressionNode>& node,                         std::vector<std::shared_ptr<ast::ExpressionNode>>& out)        {
            if (!node) return;
            if (const auto paren = std::dynamic_pointer_cast<ast::ParenRangerNode>(node))            {
                collectArgs(paren->getRangerNode(), out);
                return;
            }
            if (const auto infix = std::dynamic_pointer_cast<ast::InfixExpressionNode>(node))            {
                if (infix->getOpToken().getType() == core::TokenType::TOKEN_COMMA)                {
                    collectArgs(infix->getLeftNode(), out);
                    collectArgs(infix->getRightNode(), out);
                    return;
                }
            }
            if (const auto block = std::dynamic_pointer_cast<ast::BlockRangerNode>(node))            {
                for (const auto& e : block->getBodyExpressions()) out.push_back(e);
                return;
            }
            out.push_back(node);
        }
        bool contains(const std::vector<std::string>& v, const std::string& s)        {
            for (const auto& e : v)            {
                if (e == s) return true;
            }
            return false;
        }
    }
    CodeGen::CodeGen(const AbiInfo& abi) : _abi(abi)    {
    }
    void CodeGen::fail(const std::string& msg)    {
        if (_errorMsg.empty()) _errorMsg = msg;
    }
    std::string CodeGen::posPrefix(const utils::Pos& pos)    {
        // 位置信息可能为空（合成节点、单测构造的 AST），此时不加前缀。
        if (pos.getLine() == 0) return std::string();
        std::string out;
        const std::string file = pos.getFilepath();
        if (!file.empty())        {
            // 只留文件名，诊断里出现长绝对路径既难读也会暴露构建机路径。
            out += utils::getFileFromPath(file);
            out += ":";
        }
        out += std::to_string(pos.getLine());
        out += ":";
        out += std::to_string(pos.getColumn());
        out += ": ";
        return out;
    }
    void CodeGen::failAt(const utils::Pos& pos, const std::string& msg)    {
        fail(posPrefix(pos) + msg);
    }
    bool CodeGen::sameValueFamily(const sema::TypePtr& a, const sema::TypePtr& b) const    {
        if (!a || !b) return true;
        if (a->getKind() == sema::TypeKind::TK_UNKNOWN ||
            b->getKind() == sema::TypeKind::TK_UNKNOWN) return true;
        if (a->getKind() == b->getKind()) return true;
        // 数值族内部互通（int/float/char/bool 之间允许隐式转换）
        const auto numeric = [](const sema::TypePtr& t) {
            const auto k = t->getKind();
            return k == sema::TypeKind::TK_INT || k == sema::TypeKind::TK_FLOAT ||
                   k == sema::TypeKind::TK_CHAR || k == sema::TypeKind::TK_BOOL;
        };
        if (numeric(a) && numeric(b)) return true;
        // P12/G12：`ptr` 与数值族互通。
        // ptr 本质就是 64 位整数，但它是**新架构自己引入**的类型（原仓库
        // builtInTypes 里没有 ptr），而 core_* 的 extern fun 不参与类型表，
        // 于是 `var p = core_alloc(32, 8)` 推断成 int，`ret p` 在 ptr 函数里
        // 报 "cannot assign int to ptr" —— 用户没有任何合法写法能过。
        // 与 P9「数值族互通、不设转换墙」的原则一致。
        const auto ptrLike = [](const sema::TypePtr& t) {
            return t->getKind() == sema::TypeKind::TK_PTR;
        };
        if ((ptrLike(a) && numeric(b)) || (ptrLike(b) && numeric(a))) return true;
        // 指针族互通：str/list/dict/class/ptr 都是「一个 8 字节指针」，
        // 运行时布局不同但寄存器语义一致，允许显式混用（rt 里就靠这个）。
        if (a->isPointerLike() && b->isPointerLike()) return true;
        // 函数值（闭包对象指针）也是 8 字节指针。函数类型之间不深究
        // 参数表是否一致 —— 那是调用点该报的错，不是赋值点。
        const auto funcOrPtr = [](const sema::TypePtr& t) {
            const auto k = t->getKind();
            return k == sema::TypeKind::TK_FUNC || t->isPointerLike();
        };
        if (funcOrPtr(a) && funcOrPtr(b)) return true;
        return false;
    }
    std::string CodeGen::checkAssignable(const sema::TypePtr& target,
                                         const sema::TypePtr& source) const    {
        // 宽松原则：只挡「值类别根本不同」的错配，不做数值族内部的挑刺。
        // 理由（docs/p9_plan.md §2）：Rio 目前没有显式转换语法，如果
        // int->float 也报错，用户没有任何合法写法能过。P9 的目标是
        // 把「静默生成垃圾」变成「明确拒绝」，不是加一道转换墙。
        if (!target || !source) return std::string();
        const auto tk = target->getKind();
        const auto sk = source->getKind();
        if (tk == sema::TypeKind::TK_UNKNOWN || sk == sema::TypeKind::TK_UNKNOWN) return std::string();
        // void 不能当值用
        if (sk == sema::TypeKind::TK_VOID)        {
            return "a void expression cannot be used as a value";
        }
        if (tk == sema::TypeKind::TK_VOID)        {
            return "cannot assign a value to a void-typed target";
        }
        if (sameValueFamily(target, source)) return std::string();
        return "cannot assign a value of type " + source->toString() +
               " to a target of type " + target->toString();
    }
    sema::TypePtr CodeGen::typeFromLabels(
        const std::vector<std::shared_ptr<ast::LabelNode>>& labels) const    {
        return typeFromLabelList(labels, _currentGenericParams);
    }
    // 单态化：把签名里的 GenericType 占位换成实参类型。
    sema::TypePtr CodeGen::substituteGenerics(const sema::TypePtr& t,
                                              const std::map<std::string, sema::TypePtr>& bindings)    {
        if (!t) return sema::getUnknownType();
        switch (t->getKind())        {
        case sema::TypeKind::TK_GENERIC:            {
            const auto name = std::static_pointer_cast<sema::GenericType>(t)->getName();
            const auto found = bindings.find(name);
            return found == bindings.end() ? t : found->second;
        }
        case sema::TypeKind::TK_LIST:            {
            const auto listType = std::static_pointer_cast<sema::ListType>(t);
            return std::make_shared<sema::ListType>(substituteGenerics(listType->getElemType(), bindings));
        }
        case sema::TypeKind::TK_DICT:            {
            const auto dictType = std::static_pointer_cast<sema::DictType>(t);
            return std::make_shared<sema::DictType>(
                substituteGenerics(dictType->getKeyType(), bindings),
                substituteGenerics(dictType->getValueType(), bindings));
        }
        case sema::TypeKind::TK_FUNC:            {
            const auto fnType = std::static_pointer_cast<sema::FunctionType>(t);
            std::vector<sema::TypePtr> params;
            for (const auto& p : fnType->getParams()) params.push_back(substituteGenerics(p, bindings));
            return std::make_shared<sema::FunctionType>(std::move(params),
                                                        substituteGenerics(fnType->getReturnType(), bindings));
        }
        default:
            return t;
        }
    }
    // §4.3 冻结的类型编码表。实现在 backend::naming（rcc_symbol_naming.cpp），
    // 这里只做转发：符号规则必须只有一份，`-s` 的导出器要复用同一套。
    std::string CodeGen::mangleType(const sema::TypePtr& t)    {
        return naming::mangleType(t);
    }
    std::string CodeGen::nodeText(const std::shared_ptr<ast::ExpressionNode>& node)    {
        if (!node) return "<null>";
        return node->toString();
    }
    std::string CodeGen::mangleInstance(const std::string& base,
                                        const std::vector<sema::TypePtr>& args)    {
        return naming::mangleSignature(base, args);
    }
    // ============================================================
    // P11：重载
    // ============================================================
    //
    // 符号策略（见 docs/p11_plan.md §2）：
    //   * 名字只有一个定义  -> 符号就是裸名。rt.o / core_win64.o 的 ABI
    //     一个字都不动，这是「P11 不改运行时」的硬前提。
    //   * 名字有多个定义    -> 全部用 `_R<len><name><形参编码>` 修饰，
    //     形状与 P10 冻结的实例名一致，只是编码的内容不同：
    //     P10 编的是**类型实参**，P11 编的是**形参类型**（未标注 = v）。
    //
    // 决议只发生在调用点，且只用实参的**静态类型** —— 单遍 codegen 没有
    // 独立的 sema pass，发射 call 指令的那一刻就必须定下符号。
    std::string CodeGen::mangleOverload(const std::string& base,
                                        const std::vector<sema::TypePtr>& paramTypes)    {
        return naming::mangleSignature(base, paramTypes);
    }
    CodeGen::FunctionSignature CodeGen::collectSignature(
        const std::shared_ptr<ast::FunctionDefinitionNode>& node)    {
        FunctionSignature sig;
        if (!node) return sig;
        // 函数名的提取规则必须与 emitFunctionDefinition 完全一致，
        // 否则预登记得到的签名与发射时算出的符号会对不上。
        if (const auto id = std::dynamic_pointer_cast<ast::IdentifierNode>(node->getCallNode()))        {
            sig.name = id->getName();
        }
 else if (const auto call = std::dynamic_pointer_cast<ast::FunctionCallNode>(node->getCallNode()))        {
            if (const auto id = std::dynamic_pointer_cast<ast::IdentifierNode>(call->getLeftNode()))            {
                sig.name = id->getName();
            }
            else if (const auto index = std::dynamic_pointer_cast<ast::IndexExpressionNode>(call->getLeftNode()))            {
                if (const auto id = std::dynamic_pointer_cast<ast::IdentifierNode>(index->getLeftNode()))                {
                    sig.name = id->getName();
                }
            }
        }
        if (sig.name.empty()) return sig;
        sig.symbol = sig.name;
        sig.def = node;
        sig.genericParams = collectGenericParams(node->getCallNode());
        // 形参名 / 标注原文 / 默认值三张表要同步收集，索引才对得上。
        std::vector<std::string> annotations;
        if (const auto callInfix = std::dynamic_pointer_cast<ast::InfixExpressionNode>(node->getCallNode()))        {
            if (const auto paren = std::dynamic_pointer_cast<ast::ParenRangerNode>(callInfix->getRightNode()))            {
                collectParamNames(paren->getRangerNode(), sig.paramNames);
                collectParamAnnotations(paren->getRangerNode(), annotations);
                collectParamDefaults(paren->getRangerNode(), sig.defaults);
            }
 else            {
                collectParamNames(callInfix->getRightNode(), sig.paramNames);
                collectParamAnnotations(callInfix->getRightNode(), annotations);
                collectParamDefaults(callInfix->getRightNode(), sig.defaults);
            }
        }
 else if (const auto call = std::dynamic_pointer_cast<ast::FunctionCallNode>(node->getCallNode()))        {
            // 泛型声明的 callNode 是 FunctionCallNode，形参表在右节点。
            const auto paramsNode = std::dynamic_pointer_cast<ast::ExpressionNode>(call->getRightNode());
            if (const auto paren = std::dynamic_pointer_cast<ast::ParenRangerNode>(paramsNode))            {
                collectParamNames(paren->getRangerNode(), sig.paramNames);
                collectParamAnnotations(paren->getRangerNode(), annotations);
                collectParamDefaults(paren->getRangerNode(), sig.defaults);
            }
 else if (paramsNode)            {
                collectParamNames(paramsNode, sig.paramNames);
                collectParamAnnotations(paramsNode, annotations);
                collectParamDefaults(paramsNode, sig.defaults);
            }
        }
 else        {
            collectParamNames(node->getCallNode(), sig.paramNames);
            collectParamAnnotations(node->getCallNode(), annotations);
            collectParamDefaults(node->getCallNode(), sig.defaults);
            // 无参形态下形参名列表可能把函数名本身也收进来了，去掉。
            if (!sig.paramNames.empty() && sig.paramNames.front() == sig.name)            {
                sig.paramNames.erase(sig.paramNames.begin());
                if (!annotations.empty()) annotations.erase(annotations.begin());
            }
        }
        if (annotations.size() > sig.paramNames.size()) annotations.resize(sig.paramNames.size());
        // 形参类型：**未标注**的留 TK_UNKNOWN，绝不回退 int ——
        // mangling 要靠它把 `fun f(x)` 与 `fun f(x: int)` 编成不同的符号。
        // 标注了但解析不出来（拼错类型名）也留 TK_UNKNOWN：真正的报错由
        // emitFunctionDefinition 的标注校验负责，这里只是不参与精确匹配。
        sig.annotationTexts = annotations;
        for (size_t i = 0; i < sig.paramNames.size(); ++i)        {
            const std::string text = (i < annotations.size()) ? annotations[i] : std::string();
            sig.paramTypes.push_back(text.empty() ? sema::getUnknownType() : resolveTypeName(text));
            sig.annotated.push_back(!text.empty());
        }
        // 返回类型：与 emitFunctionDefinition 同一套顺序
        // （声明标注 -> 类名再解析一次 -> 从函数体推断）。
        sig.retType = returnTypeOf(node);
        sig.retAnnotated = sig.retType && sig.retType->getKind() != sema::TypeKind::TK_UNKNOWN;
        if (!sig.retAnnotated)        {
            for (const auto& label : node->getLabelNodes())            {
                if (!label) continue;
                const auto t = resolveTypeName(label->getFullLabel());
                if (t && t->getKind() != sema::TypeKind::TK_UNKNOWN)                {
                    sig.retType = t;
                    sig.retAnnotated = true;
                    break;
                }
            }
        }
        if (!sig.retType || sig.retType->getKind() == sema::TypeKind::TK_UNKNOWN)        {
            const auto inferred = inferAnonReturnType(node->getBodyNode(), sig.paramNames, sig.paramTypes);
            if (!inferred || inferred->getKind() == sema::TypeKind::TK_UNKNOWN ||
                inferred->getKind() == sema::TypeKind::TK_VOID)            {
                sig.retType = sema::getIntType();
            }
 else            {
                sig.retType = inferred;
            }
        }
        // 函数体只有一个 encapsulated：实现由运行时提供，符号必须保持裸名。
        if (const auto bodyBlock = std::dynamic_pointer_cast<ast::BlockRangerNode>(node->getBodyNode()))        {
            bool onlyEncapsulated = !bodyBlock->getBodyExpressions().empty();
            for (const auto& e : bodyBlock->getBodyExpressions())            {
                if (!std::dynamic_pointer_cast<ast::EncapsulatedExpressionNode>(e))                {
                    onlyEncapsulated = false;
                    break;
                }
            }
            sig.encapsulated = onlyEncapsulated;
        }
 else if (std::dynamic_pointer_cast<ast::EncapsulatedExpressionNode>(node->getBodyNode()))        {
            sig.encapsulated = true;
        }
        return sig;
    }
    void CodeGen::registerOverloadSignatures(
        const std::vector<std::shared_ptr<ast::FunctionDefinitionNode>>& fns)    {
        std::vector<FunctionSignature> all;
        std::map<std::string, int> defCount;
        for (const auto& fn : fns)        {
            if (!fn) continue;
            auto sig = collectSignature(fn);
            if (sig.name.empty()) continue;
            // 泛型声明走 P10 自己的路径：模板本身不发射，也不会和重载集混在一起。
            if (!sig.genericParams.empty()) continue;
            // encapsulated 由 rt.o 提供实现，符号是裸名，不参与重载计数。
            if (sig.encapsulated) continue;
            // 编译器特化的名字不参与重载：
            //   main  -> 固定发射成 __rio_user_main
            //   sout / super -> 调用点直接特化，根本不看签名
            if (sig.name == "main" || sig.name == "sout" || sig.name == "super") continue;
            ++defCount[sig.name];
            all.push_back(std::move(sig));
        }
        for (const auto& sig : all)        {
            const auto countIt = defCount.find(sig.name);
            if (countIt == defCount.end() || countIt->second < 2) continue;
            auto entry = sig;
            entry.symbol = mangleOverload(sig.name, sig.paramTypes);
            _overloadedNames[sig.name] = true;
            // 候选集里**保留每一份定义**（哪怕符号撞车）：决议按符号去重，
            // 而发射阶段要靠 def 指针精确找到自己那一份 —— 折叠掉的话
            // 第二份定义就找不到签名，只能报内部错误，而真正该报的是
            // 「同一签名的重复定义」。
            _overloads[sig.name].push_back(entry);
            // 符号到签名的映射按先来后到：撞车时后一份不影响调用点。
            _signatures.emplace(entry.symbol, entry);
        }
    }
    int CodeGen::overloadScore(const sema::TypePtr& formal,
                               const sema::TypePtr& actual,
                               bool annotated) const    {
        // 未标注的形参是通配符 —— 这是 P9「标注权威 + 推断兜底」在重载上的延续：
        // `fun f(x)` 什么实参都收，只是优先级最低。
        if (!formal || formal->getKind() == sema::TypeKind::TK_UNKNOWN)        {
            // 标注了却解析不出来（拼错类型名）：比通配符更弱，但仍算候选，
            // 这样报出来的是「未知类型 Piont」而不是「没有匹配的重载」。
            return annotated ? 0 : 1;
        }
        if (!actual || actual->getKind() == sema::TypeKind::TK_UNKNOWN) return 0;
        if (actual->getKind() == sema::TypeKind::TK_VOID) return -1;
        if (formal->equals(actual)) return 100;
        if (!sameValueFamily(formal, actual)) return -1;
        // 数值族内部互通，但同族优先（`f(int)` 比 `f(float)` 更适合传 int）。
        if (formal->isNumeric() && actual->isNumeric())        {
            return (formal->getKind() == actual->getKind()) ? 90 : 50;
        }
        // 指针族之间（str/list/dict/class/ptr/func）只是「一个 8 字节指针」，
        // 允许互通但优先级很低，避免抢走精确匹配。
        return 30;
    }
    bool CodeGen::normalizeArgsFor(
        const std::vector<std::shared_ptr<ast::ExpressionNode>>& args,
        const std::vector<std::string>& names,
        const std::map<std::string, std::shared_ptr<ast::ExpressionNode>>& defaults,
        std::vector<std::shared_ptr<ast::ExpressionNode>>& out)    {
        out.clear();
        const auto namedTarget = [](const std::shared_ptr<ast::ExpressionNode>& a)            -> std::string {
            const auto assign = std::dynamic_pointer_cast<ast::AssignmentNode>(a);
            if (!assign) return std::string();
            const auto target = std::dynamic_pointer_cast<ast::IdentifierNode>(assign->getAssignPair().first);
            return target ? target->getName() : std::string();
        };
        if (names.empty())        {
            // 没有形参名可用：命名实参无法定位，直接判这个候选不合格。
            for (const auto& a : args)            {
                if (!namedTarget(a).empty()) return false;
                out.push_back(a);
            }
            return true;
        }
        std::vector<std::shared_ptr<ast::ExpressionNode>> slots(names.size(), nullptr);
        size_t positional = 0;
        for (const auto& a : args)        {
            std::shared_ptr<ast::ExpressionNode> value = a;
            const std::string named = namedTarget(a);
            if (!named.empty())            {
                const auto assign = std::dynamic_pointer_cast<ast::AssignmentNode>(a);
                value = assign->getAssignPair().second;
                size_t idx = names.size();
                for (size_t i = 0; i < names.size(); ++i)                {
                    if (names[i] == named) { idx = i; break; }
                }
                // 名字对不上任何形参，或者同一个形参被喂了两次
                if (idx == names.size()) return false;
                if (slots[idx]) return false;
                slots[idx] = value;
            }
 else            {
                while (positional < slots.size() && slots[positional]) ++positional;
                if (positional >= slots.size()) return false;
                slots[positional++] = value;
            }
        }
        // 空位用默认值补；补不上说明实参确实少了，这个候选不合格。
        for (size_t i = 0; i < slots.size(); ++i)        {
            if (slots[i]) continue;
            const auto found = defaults.find(names[i]);
            if (found == defaults.end()) return false;
            slots[i] = found->second;
        }
        out = std::move(slots);
        return true;
    }
    std::string CodeGen::resolveOverload(
        const std::string& name,
        const std::vector<std::shared_ptr<ast::ExpressionNode>>& args,
        bool* ambiguous) const    {
        if (ambiguous) *ambiguous = false;
        const auto found = _overloads.find(name);
        if (found == _overloads.end()) return std::string();
        std::string best;
        int bestScore = -1;
        bool tie = false;
        for (const auto& sig : found->second)        {
            std::vector<std::shared_ptr<ast::ExpressionNode>> flat;
            if (!normalizeArgsFor(args, sig.paramNames, sig.defaults, flat)) continue;
            if (flat.size() != sig.paramTypes.size()) continue;
            int score = 0;
            bool ok = true;
            for (size_t i = 0; i < flat.size(); ++i)            {
                const auto actual = inferType(flat[i]);
                const bool annotated = (i < sig.annotated.size()) ? sig.annotated[i] : false;
                const int s = overloadScore(sig.paramTypes[i], actual, annotated);
                if (s < 0) { ok = false; break; }
                score += s;
            }
            if (!ok) continue;
            if (score > bestScore)            {
                bestScore = score;
                best = sig.symbol;
                tie = false;
            }
 else if (score == bestScore && sig.symbol != best)            {
                // 同分但**符号相同**不算歧义：那是同一签名的重复定义，
                // 该由发射阶段的 _emittedSymbols 报重复定义，不是这里报歧义。
                tie = true;
            }
        }
        // 多个候选同分：不猜，交给调用点报歧义。
        if (tie)        {
            if (ambiguous) *ambiguous = true;
            return std::string();
        }
        return best;
    }
    void CodeGen::emitResolvedCall(
        const std::string& symbol,
        const std::string& displayName,
        const std::vector<std::shared_ptr<ast::ExpressionNode>>& args)    {
        const auto found = _signatures.find(symbol);
        if (found == _signatures.end())        {
            fail("Internal error: no signature registered for symbol '" + symbol + "'");
            return;
        }
        const auto& sig = found->second;
        std::vector<std::shared_ptr<ast::ExpressionNode>> flat;
        if (!normalizeArgsFor(args, sig.paramNames, sig.defaults, flat))        {
            fail("Internal error: cannot bind arguments to '" + displayName + "'");
            return;
        }
        // 落栈规则与 emitFunctionDefinition 一致：未标注的形参按 int 走整型寄存器。
        std::vector<sema::TypePtr> paramTypes = sig.paramTypes;
        for (auto& pt : paramTypes)        {
            if (!pt || pt->getKind() == sema::TypeKind::TK_UNKNOWN) pt = sema::getIntType();
        }
        emitArgsToRegisters(flat, displayName, paramTypes, 0, 0, &sig.annotated);
        if (hasFailed()) return;
        const size_t shadow = _abi.getShadowSpaceSize();
        _asm.subRsp(shadow);
        _asm.call(symbol);
        _asm.addRsp(shadow);
    }
    void CodeGen::failOverload(const std::string& name,
                               const std::vector<std::shared_ptr<ast::ExpressionNode>>& args,
                               const utils::Pos& pos,
                               bool ambiguous)    {
        // 候选签名用**原始标注文本**呈现：`fun f(x: Piont)` 要原样报出 "Piont"，
        // 不能拿解析失败后的占位类型糊弄用户。
        const auto describe = [](const FunctionSignature& s) {
            std::string out = "fun " + s.name + "(";
            for (size_t i = 0; i < s.paramNames.size(); ++i)            {
                if (i) out += ", ";
                out += s.paramNames[i];
                const std::string text = (i < s.annotationTexts.size()) ? s.annotationTexts[i] : std::string();
                if (!text.empty()) out += ": " + text;
            }
            out += ")";
            return out;
        };
        std::string actualTypes;
        for (size_t i = 0; i < args.size(); ++i)        {
            if (i) actualTypes += ", ";
            const auto t = inferType(args[i]);
            actualTypes += (t ? t->toString() : std::string("?"));
        }
        std::string candidates;
        if (const auto found = _overloads.find(name); found != _overloads.end())        {
            for (const auto& sig : found->second)            {
                if (!candidates.empty()) candidates += "; ";
                candidates += describe(sig);
            }
        }
        if (candidates.empty()) candidates = "<none>";
        if (ambiguous)        {
            failAt(pos, "Ambiguous call to overloaded function '" + name + "' with arguments (" +
                        actualTypes + "); candidates: " + candidates);
            return;
        }
        failAt(pos, "No matching overload for '" + name + "' with arguments (" + actualTypes +
                    "); candidates: " + candidates);
    }
    // ============================================================
    // 作用域
    // ============================================================
    void CodeGen::pushScope()    {
        _scopes.emplace_back();
    }
    void CodeGen::popScope()    {
        if (!_scopes.empty()) _scopes.pop_back();
    }
    CodeGen::VarSlot* CodeGen::lookupVar(const std::string& name)    {
        // 从最内层向外查找
        for (auto it = _scopes.rbegin(); it != _scopes.rend(); ++it)        {
            if (const auto found = it->find(name); found != it->end())            {
                return &found->second;
            }
        }
        // P12/G1a：局部作用域都没有，回退查全局槽。
        // 全局表独立于 _scopes —— emitFunctionBody 会 _scopes.clear()，
        // 全局变量必须活得比任何函数帧更久。
        if (const auto g = _globals.find(name); g != _globals.end())        {
            return &g->second;
        }
        return nullptr;
    }
    long long CodeGen::allocateSlot(const std::string& name, const sema::TypePtr& type, size_t size)    {
        // 每个槽至少 8 字节，保持 8 字节对齐
        // 每个槽至少 8 字节，保持 8 字节对齐。
        // 下界必须真的 >= 8：size 传 0（比如返回 void 的调用）时，
        // 如果 slotSize 也算成 0，_frameSize 不推进，两个变量就会拿到
        // 同一个栈偏移并互相覆盖。
        const size_t slotSize = size < 8 ? 8 : ((size + 7) & ~static_cast<size_t>(7));
        _frameSize += slotSize;
        // 关键：局部变量必须位于 rbp 之下，偏移取负值。
        // 写成正偏移会覆盖返回地址与调用者的栈帧。
        const long long offset = -static_cast<long long>(_frameSize);
        VarSlot slot;
        slot.type = type;
        slot.stackOffset = offset;
        if (_scopes.empty()) _scopes.emplace_back();
        _scopes.back()[name] = slot;
        return offset;
    }
    std::string CodeGen::addrOf(const VarSlot& slot)    {
        // P12/G1a：全局槽用 RIP 相对寻址，栈槽用 rbp 相对负偏移。
        if (slot.isGlobal()) return slot.globalLabel + "(%rip)";
        return std::to_string(slot.stackOffset) + "(%rbp)";
    }
    std::string CodeGen::allocateGlobal(const std::string& name, const sema::TypePtr& type, size_t size)    {
        // 全局槽必须至少 8 字节，保证对齐；.bss 天然零初始化，
        // 因此「顶层 var 的初始化语句」仍由 __rio_main 按源码顺序执行，
        // 这里只负责给它们一块固定存储。
        const size_t slotSize = size < 8 ? 8 : ((size + 7) & ~static_cast<size_t>(7));
        const std::string label = "_g_" + name;
        if (_globalSlots.insert({name, label}).second)        {
            _globalDecls.push_back({label, slotSize});
        }
        VarSlot slot;
        slot.type = type;
        slot.globalLabel = label;
        _globals[name] = slot;
        return label;
    }
    sema::TypePtr CodeGen::inferType(const std::shared_ptr<ast::ExpressionNode>& node) const    {
        if (!node) return sema::getVoidType();
        if (std::dynamic_pointer_cast<ast::IntegerLiteralNode>(node)) return sema::getIntType();
        if (std::dynamic_pointer_cast<ast::FloatLiteralNode>(node)) return sema::getFloatType();
        if (std::dynamic_pointer_cast<ast::StringLiteralNode>(node)) return sema::getStrType();
        if (std::dynamic_pointer_cast<ast::BooleanLiteralNode>(node)) return sema::getBoolType();
        if (std::dynamic_pointer_cast<ast::CharacterLiteralNode>(node)) return sema::getCharType();
        if (const auto id = std::dynamic_pointer_cast<ast::IdentifierNode>(node))        {
            if (const auto slot = const_cast<CodeGen*>(this)->lookupVar(id->getName()))            {
                return slot->type ? slot->type : sema::getIntType();
            }
            // 不是局部变量但是已知函数：`apply(add, 3)` 里的 add 是一个
            // 函数值（闭包指针），类型得是 FunctionType 而不是 int，
            // 否则传给 `f: funi[A][B]` 形参时会被可赋值性校验挡下来。
            const auto paramsIt = _functionParams.find(id->getName());
            const auto retIt = _functionReturns.find(id->getName());
            if (paramsIt != _functionParams.end() && retIt != _functionReturns.end())            {
                return std::make_shared<sema::FunctionType>(paramsIt->second, retIt->second);
            }
            return sema::getIntType();
        }
        if (const auto call = std::dynamic_pointer_cast<ast::FunctionCallNode>(node))        {
            // 被调函数名
            const auto callee = call->getLeftNode();
            // P6：方法调用 `p.sum()` 的返回类型从方法定义里取
            if (const auto member = std::dynamic_pointer_cast<ast::InfixExpressionNode>(callee))            {
                if (member->getOpToken().getType() == core::TokenType::TOKEN_DOT)                {
                    const auto mid = std::dynamic_pointer_cast<ast::IdentifierNode>(member->getRightNode());
                    // P8：`mod.fn(...)` / `mod.Cls(...)` —— 模块在加载期内联，
                    // 类型按普通全局函数 / 类名解析，不能落到「未知函数按 int」。
                    if (mid)
                    {
                        if (const auto modId =
                                std::dynamic_pointer_cast<ast::IdentifierNode>(member->getLeftNode());
                            modId && isModuleName(modId->getName()))
                        {
                            if (findClass(mid->getName()))
                                return std::make_shared<sema::ClassType>(mid->getName());
                            if (const auto found = _functionReturns.find(mid->getName());
                                found != _functionReturns.end())
                                return found->second;
                            return sema::getIntType();
                        }
                    }
                    const std::string owner = classNameOf(member->getLeftNode());
                    if (mid && !owner.empty())                    {
                        const auto methods = collectMethods(owner);
                        const auto found = methods.find(mid->getName());
                        if (found != methods.end() && found->second.def)                        {
                            std::vector<std::string> pn;
                            std::vector<sema::TypePtr> pt;
                            if (const auto inner = std::dynamic_pointer_cast<ast::FunctionCallNode>(found->second.def->getCallNode()))                            {
                                if (const auto argsNode = std::dynamic_pointer_cast<ast::ExpressionNode>(inner->getRightNode()))                                {
                                    collectParamNamesT(argsNode, pn);
                                    collectParamTypesT(argsNode, pt);
                                }
                            }
                            return methodReturnType(found->second.def, pn, pt, owner);
                        }
                    }
                    return sema::getIntType();
                }
            }
            if (const auto id = std::dynamic_pointer_cast<ast::IdentifierNode>(callee))            {
                // P6：`Point(10, 20)` 的结果类型就是这个类
                if (findClass(id->getName())) return std::make_shared<sema::ClassType>(id->getName());
                // P10：泛型调用的结果类型要在**编译期**就定下来（不能等到
                // 发射完再查表），否则 `var v = identity(1.5)` 的槽位会按 int
                // 分配，把 double 位模式当整数存。
                if (const auto t = inferGenericCallType(id->getName(), callee, call)) return t;
                // P11：重载名的返回类型只能靠决议后的签名给。重载名不进
                // _functionReturns（那是「一个名字一个签名」的表），所以必须
                // 在这里先决议一次 —— 否则 `var v = f(1.5)` 的槽位会按 int 分配，
                // 把 double 的位模式当整数存下来。
                if (_overloadedNames.find(id->getName()) != _overloadedNames.end())                {
                    std::vector<std::shared_ptr<ast::ExpressionNode>> callArgs;
                    if (const auto argsNode = std::dynamic_pointer_cast<ast::ExpressionNode>(call->getRightNode()))                    {
                        collectArgs(argsNode, callArgs);
                    }
                    const std::string sym = resolveOverload(id->getName(), callArgs, nullptr);
                    if (!sym.empty())                    {
                        if (const auto sigIt = _signatures.find(sym); sigIt != _signatures.end() &&
                                                                     sigIt->second.retType)
                            return sigIt->second.retType;
                    }
                    // 决议不出来时不要瞎猜：调用点会给出准确的诊断。
                    return sema::getIntType();
                }
                if (const auto found = _functionReturns.find(id->getName()); found != _functionReturns.end())                {
                    return found->second;
                }
                // 经由函数值调用：变量槽里存着 fun(...): T，结果类型就是 T。
                // 槽位类型是函数类型这一点，由 `var f = (..)->{..}` 的初始化
                // （inferType 对匿名函数返回 FunctionType）自动带来，不需要额外登记表。
                if (const auto slot = const_cast<CodeGen*>(this)->lookupVar(id->getName()))                {
                    if (slot->type && slot->type->getKind() == sema::TypeKind::TK_FUNC)                    {
                        const auto fnType = std::static_pointer_cast<sema::FunctionType>(slot->type);
                        if (fnType->getReturnType()) return fnType->getReturnType();
                    }
                }
            }
            // P10：`identity[int](42)` 的 callee 是 IndexExpressionNode。
            if (const auto index = std::dynamic_pointer_cast<ast::IndexExpressionNode>(callee))            {
                if (const auto id = std::dynamic_pointer_cast<ast::IdentifierNode>(index->getLeftNode()))                {
                    // `Box[int](1)` —— 泛型类构造调用的结果就是那个具体类
                    if (_genericClasses.find(id->getName()) != _genericClasses.end())                    {
                        const auto typeArgs = typeArgsOf(index);
                        auto* self = const_cast<CodeGen*>(this);
                        const std::string instName = self->ensureClassInstance(id->getName(), typeArgs);
                        if (!instName.empty()) return std::make_shared<sema::ClassType>(instName);
                    }
                    if (const auto t = inferGenericCallType(id->getName(), callee, call)) return t;
                }
            }
            // 未知函数的返回类型按 int 处理，**不能**按 void。
            // void 的 getSize() 是 0，会让 `var n = f(...)` 的槽位大小算成 0，
            // 于是 allocateSlot 不推进 _frameSize，n 和上一个局部变量/形参
            // 拿到同一个栈偏移，直接互相覆盖（rt_print_str 的 s 被 n 覆盖就是这么来的）。
            // 保守地按 int（8 字节）分配永远安全，最坏只是多占几个字节。
            return sema::getIntType();
        }
        if (const auto infix = std::dynamic_pointer_cast<ast::InfixExpressionNode>(node))        {
            // 复合赋值：类型与左值相同
            const auto t = infix->getOpToken().getType();
            // P6：成员读取的类型就是这个字段的静态类型
            if (t == core::TokenType::TOKEN_DOT)            {
                const auto fieldId = std::dynamic_pointer_cast<ast::IdentifierNode>(infix->getRightNode());
                const std::string owner = classNameOf(infix->getLeftNode());
                if (fieldId && !owner.empty())                {
                    const auto ft = findFieldType(owner, fieldId->getName());
                    if (ft) return ft;
                }
                return sema::getIntType();
            }
            if (t == core::TokenType::TOKEN_PLUS_ASSIGN || t == core::TokenType::TOKEN_MINUS_ASSIGN ||                t == core::TokenType::TOKEN_STAR_ASSIGN || t == core::TokenType::TOKEN_SLASH_ASSIGN ||                t == core::TokenType::TOKEN_MODULO_ASSIGN)            {
                return inferType(infix->getLeftNode());
            }
            // 逻辑运算返回 bool
            if (t == core::TokenType::TOKEN_AND || t == core::TokenType::TOKEN_OR)            {
                return sema::getBoolType();
            }
            const auto lt = inferType(infix->getLeftNode());
            const auto rt = inferType(infix->getRightNode());
            // P12/G3 + G4：`str + str` 得 str、`list + list` 得 list（拼接）。
            // 不这样写的话，`var s = "a" + "b"` 会把结果槽推成 int，
            // 后续 sout 走 rt_print_int，打出指针值。
            if (t == core::TokenType::TOKEN_PLUS && lt && rt)            {
                if (lt->getKind() == sema::TypeKind::TK_STR &&
                    rt->getKind() == sema::TypeKind::TK_STR)                {
                    return sema::getStrType();
                }
                if (lt->getKind() == sema::TypeKind::TK_LIST &&
                    rt->getKind() == sema::TypeKind::TK_LIST)                {
                    return lt;
                }
            }
            // 比较运算返回 bool（浮点比较也是 bool）
            if (t == core::TokenType::TOKEN_EQUAL || t == core::TokenType::TOKEN_NOT_EQUAL ||                t == core::TokenType::TOKEN_LESS || t == core::TokenType::TOKEN_LESS_EQUAL ||                t == core::TokenType::TOKEN_GREATER || t == core::TokenType::TOKEN_GREATER_EQUAL)            {
                return sema::getBoolType();
            }
            if (lt->isFloat() || rt->isFloat()) return sema::getFloatType();
            return sema::getIntType();
        }
        if (const auto unary = std::dynamic_pointer_cast<ast::UnaryExpressionNode>(node))        {
            if (unary->getOpToken().getType() == core::TokenType::TOKEN_NOT)            {
                return sema::getBoolType();
            }
            return inferType(unary->getRightNode());
        }
        // ---------- P5：容器 ----------
        // 索引访问必须先于通用 PostfixExpressionNode 判定 —— IndexExpressionNode
        // 是 PostfixExpressionNode 的子类，落到下面那条就会递归到 getNode()
        // 并错误地返回目标容器自身的类型（dict 而不是它的值类型）。
        if (const auto index = std::dynamic_pointer_cast<ast::IndexExpressionNode>(node))        {
            return inferIndexResultType(index);
        }
        if (const auto dict = std::dynamic_pointer_cast<ast::DictionaryExpressionNode>(node))        {
            return inferDictLiteralType(dict);
        }
        if (const auto list = std::dynamic_pointer_cast<ast::ListExpressionNode>(node))        {
            return inferListLiteralType(list);
        }
        if (const auto postfix = std::dynamic_pointer_cast<ast::PostfixExpressionNode>(node))        {
            return inferType(postfix->getNode());
        }
        if (std::dynamic_pointer_cast<ast::NullLiteralNode>(node)) return sema::getPtrType();
        if (const auto paren = std::dynamic_pointer_cast<ast::ParenRangerNode>(node))        {
            return inferType(paren->getRangerNode());
        }
        // 匿名函数表达式的类型：fun(参数类型...): 返回类型。
        // 参数类型从形参标签读（没标注就是 int），返回类型从函数体里第一个 ret 推。
        if (const auto anon = std::dynamic_pointer_cast<ast::AnonFunctionDefinitionNode>(node))        {
            std::vector<sema::TypePtr> params;
            if (anon->getParamNode()) collectParamTypes(anon->getParamNode(), params);
            for (auto& p : params)            {
                if (!p || p->getKind() == sema::TypeKind::TK_UNKNOWN) p = sema::getIntType();
            }
            auto ret = findReturnType(anon->getBodyNode());
            if (!ret || ret->getKind() == sema::TypeKind::TK_UNKNOWN) ret = sema::getIntType();
            return std::make_shared<sema::FunctionType>(params, ret);
        }
        return sema::getIntType();
    }
    // ============================================================
    // P5：容器类型推断
    // ============================================================
    //
    // 容器字面量的元素/值类型只在**编译期**有意义：它决定索引表达式的静态
    // 类型，进而决定 sout 选哪个 rt_print_*。运行时元素槽恒为 8 字节，
    // 所以异构字面量退化成 int 是安全的（只是打印时按 int 解释）。
    //
    // 为什么不用「和类型 / any」：那是 P6/P10 的课题。P5 只需要让
    //   var dic = {"name": "Rio", "age": 18}
    //   sout(dic["name"])   -> str
    //   sout(dic["age"])    -> int
    // 成立，因此字典额外记录每个**字符串字面量键**对应的值类型（DictType
    // 的键类型精化表），索引时按键取，取不到再退回统一的值类型。
    sema::TypePtr CodeGen::inferListLiteralType(const std::shared_ptr<ast::ExpressionNode>& node) const    {
        const auto list = std::dynamic_pointer_cast<ast::ListExpressionNode>(node);
        if (!list) return std::make_shared<sema::ListType>(sema::getIntType());
        std::vector<std::shared_ptr<ast::ExpressionNode>> elements;
        flattenListElements(list->getBodyNode(), elements);
        if (elements.empty()) return std::make_shared<sema::ListType>(sema::getIntType());
        // 元素类型必须统一：全同则取之，否则退化成 int。
        sema::TypePtr elem = inferType(elements.front());
        if (!elem || elem->getKind() == sema::TypeKind::TK_UNKNOWN ||
            elem->getKind() == sema::TypeKind::TK_VOID)        {
            elem = sema::getIntType();
        }
        for (size_t i = 1; i < elements.size(); ++i)        {
            const auto t = inferType(elements[i]);
            if (!t || !elem->equals(t))            {
                elem = sema::getIntType();
                break;
            }
        }
        return std::make_shared<sema::ListType>(elem);
    }
    sema::TypePtr CodeGen::inferDictLiteralType(const std::shared_ptr<ast::ExpressionNode>& node) const    {
        const auto dict = std::dynamic_pointer_cast<ast::DictionaryExpressionNode>(node);
        if (!dict) return std::make_shared<sema::DictType>(sema::getStrType(), sema::getIntType());
        std::vector<std::pair<std::shared_ptr<ast::ExpressionNode>,
                              std::shared_ptr<ast::ExpressionNode>>> pairs;
        flattenDictPairs(dict->getBodyNode(), pairs);
        if (pairs.empty())        {
            return std::make_shared<sema::DictType>(sema::getStrType(), sema::getIntType());
        }
        // 键类型：全部相同则取之，否则退化成 str（Rio 的字典几乎都是字符串键）
        sema::TypePtr keyType = inferType(pairs.front().first);
        if (!keyType || keyType->getKind() == sema::TypeKind::TK_UNKNOWN)        {
            keyType = sema::getStrType();
        }
        for (size_t i = 1; i < pairs.size(); ++i)        {
            const auto t = inferType(pairs[i].first);
            if (!t || !keyType->equals(t))            {
                keyType = sema::getStrType();
                break;
            }
        }
        // 值类型：全部相同则取之，否则退化成 int
        sema::TypePtr valType = inferType(pairs.front().second);
        if (!valType || valType->getKind() == sema::TypeKind::TK_UNKNOWN ||
            valType->getKind() == sema::TypeKind::TK_VOID)        {
            valType = sema::getIntType();
        }
        bool uniform = true;
        for (size_t i = 1; i < pairs.size(); ++i)        {
            const auto t = inferType(pairs[i].second);
            if (!t || !valType->equals(t))            {
                uniform = false;
                break;
            }
        }
        if (!uniform) valType = sema::getIntType();
        auto result = std::make_shared<sema::DictType>(keyType, valType);
        // 逐键登记：只对字符串字面量键生效，其余键查表时自然回退到 valType。
        std::map<std::string, sema::TypePtr> keyTypes;
        for (const auto& [k, v] : pairs)        {
            const auto lit = std::dynamic_pointer_cast<ast::StringLiteralNode>(k);
            if (!lit) continue;
            const auto vt = inferType(v);
            if (!vt || vt->getKind() == sema::TypeKind::TK_UNKNOWN ||
                vt->getKind() == sema::TypeKind::TK_VOID)            {
                continue;
            }
            keyTypes[lit->unescapedString()] = vt;
        }
        if (!keyTypes.empty()) result->setKeyTypes(std::move(keyTypes));
        return result;
    }
    sema::TypePtr CodeGen::inferIndexResultType(const std::shared_ptr<ast::ExpressionNode>& node) const    {
        const auto index = std::dynamic_pointer_cast<ast::IndexExpressionNode>(node);
        if (!index) return sema::getIntType();
        const auto targetType = inferType(index->getLeftNode());
        if (targetType && targetType->getKind() == sema::TypeKind::TK_LIST)        {
            const auto listType = std::static_pointer_cast<sema::ListType>(targetType);
            const auto elem = listType->getElemType();
            return (elem && elem->getKind() != sema::TypeKind::TK_UNKNOWN) ? elem : sema::getIntType();
        }
        // P12/G10：`s[i]` 返回 char（0..255）。
        if (targetType && targetType->getKind() == sema::TypeKind::TK_STR)        {
            return sema::getCharType();
        }
        if (targetType && targetType->getKind() == sema::TypeKind::TK_DICT)        {
            const auto dictType = std::static_pointer_cast<sema::DictType>(targetType);
            // 键是字符串字面量时走精化表，拿到那个键**自己**的值类型。
            const auto keyNode = unwrapIndexKey(index->getIndexNode());
            if (const auto lit = std::dynamic_pointer_cast<ast::StringLiteralNode>(keyNode))            {
                if (const auto refined = dictType->getKeyTypeOf(lit->unescapedString()))                {
                    return refined;
                }
            }
            const auto val = dictType->getValueType();
            return (val && val->getKind() != sema::TypeKind::TK_UNKNOWN) ? val : sema::getIntType();
        }
        // 目标不是容器（可能还没推出来）：按 int 兜底，保证槽位至少 8 字节。
        return sema::getIntType();
    }
    // 在函数体里找第一个 ret 的表达式，并推断它的类型。
    // 找不到（或推不出来）返回 TK_UNKNOWN，由调用方回退到 int。
    sema::TypePtr CodeGen::findReturnType(const std::shared_ptr<ast::ExpressionNode>& body) const    {
        if (!body) return sema::getUnknownType();
        // 裸表达式体：parser 会把它包成 ReturnExpressionNode
        if (const auto ret = std::dynamic_pointer_cast<ast::ReturnExpressionNode>(body))        {
            if (!ret->getReturnNode()) return sema::getVoidType();
            return inferType(ret->getReturnNode());
        }
        if (const auto stmt = std::dynamic_pointer_cast<ast::ExpressionStatementNode>(body))        {
            return findReturnType(stmt->getExpression());
        }
        if (const auto block = std::dynamic_pointer_cast<ast::BlockRangerNode>(body))        {
            for (const auto& e : block->getBodyExpressions())            {
                const auto t = findReturnType(e);
                if (t && t->getKind() != sema::TypeKind::TK_UNKNOWN) return t;
            }
            return sema::getUnknownType();
        }
        if (const auto cond = std::dynamic_pointer_cast<ast::ConditionNode>(body))        {
            for (const auto& branchExpr : cond->getBranchNodes())            {
                if (const auto branch = std::dynamic_pointer_cast<ast::BranchNode>(branchExpr))                {
                    const auto t = findReturnType(branch->getBodyNode());
                    if (t && t->getKind() != sema::TypeKind::TK_UNKNOWN) return t;
                }
            }
            return sema::getUnknownType();
        }
        if (const auto loop = std::dynamic_pointer_cast<ast::LoopNode>(body))        {
            return findReturnType(loop->getBodyNode());
        }
        return sema::getUnknownType();
    }
    // 与 findReturnType 同义，但先把形参放进一个影子作用域，
    // 让函数体里的形参引用（例如 `ret x * 2` 的 x）拿到正确类型，
    // 而不是误用外层同名变量。影子作用域里的槽位偏移全是 0 —— 这里只做类型推断，
    // 不发射任何指令，也不影响 _frameSize。
    sema::TypePtr CodeGen::inferAnonReturnType(const std::shared_ptr<ast::ExpressionNode>& body,
                                               const std::vector<std::string>& paramNames,
                                               const std::vector<sema::TypePtr>& paramTypes)    {
        pushScope();
        for (size_t i = 0; i < paramNames.size(); ++i)        {
            VarSlot slot;
            slot.type = (i < paramTypes.size() && paramTypes[i]) ? paramTypes[i] : sema::getIntType();
            slot.stackOffset = 0;
            _scopes.back()[paramNames[i]] = slot;
        }
        const auto ret = findReturnType(body);
        popScope();
        return ret;
    }
    // ============================================================
    // 入口
    // ============================================================
    std::string CodeGen::generate(const std::shared_ptr<ast::ProgramNode>& program, std::string& errorMsg)    {
        if (!program)        {
            errorMsg = "Empty program.";
            return {}
;
        }
        _errorMsg.clear();
        _asm.clear();
        _frameSize = 0;
        _scopes.clear();
        _floatPool.clear();
        _floatLiterals.clear();
        _pendingFunctions.clear();
        _pendingInstances.clear();
        _genericFunctions.clear();
        _genericInstances.clear();
        _genericClasses.clear();
        _genericClassInstances.clear();
        _genericClassEmitted.clear();
        _genericClassBindings.clear();
        _currentGenericParams.clear();
        _currentGenericBindings.clear();
        _closureObjects.clear();
        _namedClosures.clear();
        _anonCounter = 0;
        _tempCounter = 0;
        _classes.clear();
        _currentClass.clear();
        _functionParams.clear();
        _functionReturns.clear();
        // P11：重载表。_functionParamNames / _functionDefaults /
        // _functionParamAnnotated / _encapsulatedFunctions / _hasUserMain
        // 历史上就没有在这里清（同一 CodeGen 实例只 generate 一次），
        // 新表必须清，否则第二次 generate 会看到上一次的候选集。
        _overloads.clear();
        _overloadedNames.clear();
        _signatures.clear();
        _emittedSymbols.clear();
        // P12/G1a
        _globalSlots.clear();
        _globalDecls.clear();
        _topLevelVarNames.clear();
        _inTopLevelStatements = false;
        try        {
            emitPrelude();
            emitProgram(program);
            // P10：泛型类实例体。必须在 emitProgram 之后 —— 实例是随
            // 函数体 / 顶层语句的发射才被发现的，而它们各自都有独立的
            // 栈帧，只能在这里（帧已回填、作用域已弹出）单独发。
            emitGenericClassBodies();
            if (!_libraryMode)            {
                emitMainWrapper();
            }
            // 匿名函数的函数体在表达式位置被排队，等外层函数全部发完再吐出来。
            // 这一步必须在 emitStringLiterals 之前：匿名函数体里可能产生新的
            // 字符串/浮点字面量，那些池子还要在后面统一输出。
            emitPendingFunctions();
            // P10：泛型实例体。实例是在发射函数体时按调用点发现的，
            // 所以必须排在这里（不能就地发射，会搅乱正在用的栈帧）。
            emitPendingInstances();
            emitClosureObjects();
            _asm.emitStringLiterals();
            // P12/G1a：顶层 var 的全局槽。.bss 天然零初始化，
            // 真正的初值由 __rio_main 按源码顺序写入。
            if (!_globalDecls.empty())            {
                _asm.bss();
                for (const auto& [label, bytes] : _globalDecls)                {
                    _asm.align(8);
                    _asm.label(label);
                    _asm.inst(".zero", std::to_string(bytes));
                }
            }
            // 浮点常量池
            if (!_floatLiterals.empty())            {
                _asm.rodata();
                for (const auto& [label, value] : _floatLiterals)                {
                    long long bits = 0;
                    static_assert(sizeof(bits) == sizeof(value), "double must be 8 bytes");
                    std::memcpy(&bits, &value, sizeof(bits));
                    _asm.align(8);
                    _asm.label(label);
                    _asm.quad(bits);
                }
            }
        }
 catch (const std::exception& e)        {
            errorMsg = std::string("Code generation failed: ") + e.what();
            return {}
;
        }
        if (hasFailed())        {
            errorMsg = _errorMsg;
            return {}
;
        }
        return _asm.str();
    }
    void CodeGen::emitPrelude()    {
        _asm.header("Generated by RCC (Rio Compiler Collection)");
        _asm.header("Target: Windows x86-64, MS ABI, GNU as (AT&T syntax)");
        _asm.text();
        // 显式声明外部符号，避免链接器找不到
        for (const char* sym : {
"core_io_write", "core_proc_exit", "core_alloc",                                 "core_free", "core_mem_copy", "core_panic_raise",                                 "core_stack_probe"}
)        {
            (void)sym;
   // 由 core_win64.o 提供，无需在汇编里重复声明
        }
    }
    void CodeGen::emitFloatLiteral(const std::shared_ptr<ast::ExpressionNode>& node)    {
        const auto lit = std::dynamic_pointer_cast<ast::LiteralNode>(node);
        const std::string text = lit ? lit->literalString() : node->toString();
        double v = 0.0;
        try {
 v = std::stod(text);
 }
 catch (...) {
 v = 0.0;
 }
        // 浮点常量放在只读数据段，按标签从内存加载到 %xmm0。
        // 同一个值只发射一次，避免重复常量。
        std::string label;
        if (const auto found = _floatPool.find(v); found != _floatPool.end())        {
            label = found->second;
        }
 else        {
            label = _asm.newLabel("fconst");
            _floatPool[v] = label;
            _floatLiterals.emplace_back(label, v);
        }
        _asm.inst("movsd", label + "(%rip)", "%xmm0");
    }
    void CodeGen::emitProgram(const std::shared_ptr<ast::ProgramNode>& program)    {
        // 第一遍：按语句种类分流。注意这里**不立即**发射函数体：
        // 类的登记必须先完成，否则 `fun main` 里的 `Point(10, 20)` 会因为
        // 类还没登记而被当成普通函数调用（test_16 的类就写在 main 之前，
        // 但 AST 的语句顺序不保证这点）。
        std::vector<std::shared_ptr<ast::ExpressionNode>> topLevelStatements;
        std::vector<std::shared_ptr<ast::FunctionDefinitionNode>> functionNodes;
        std::vector<std::shared_ptr<ast::ClassDefinitionNode>> classNodes;
        for (const auto& stmt : program->getStatements())        {
            if (!stmt) continue;
            // 语句节点需要先取出其内部表达式
            const auto exprStmt = std::dynamic_pointer_cast<ast::ExpressionStatementNode>(stmt);
            if (!exprStmt) continue;
            const auto expr = exprStmt->getExpression();
            if (!expr) continue;
            if (const auto fn = std::dynamic_pointer_cast<ast::FunctionDefinitionNode>(expr))            {
                functionNodes.push_back(fn);
                continue;
            }
            if (const auto cls = std::dynamic_pointer_cast<ast::ClassDefinitionNode>(expr))            {
                classNodes.push_back(cls);
                continue;
            }
            topLevelStatements.push_back(expr);
        }
        // P12/G1a：顶层 `var` 必须放进全局静态槽，否则它只是 __rio_main 的
        // 局部变量，函数体里根本看不到（`math.rio` 的 `var pi: float const`
        // 就是这种顶层 var）。
        //
        // 做法：先扫出所有顶层 var 的名字并分配 .bss 槽，再把槽登记到
        // **最外层作用域**（_scopes.front()）。这样函数体 pushScope 之后
        // lookupVar 由内向外一定能命中它。
        for (const auto& expr : topLevelStatements)        {
            const auto varDef = std::dynamic_pointer_cast<ast::VariableDefinitionNode>(expr);
            if (!varDef) continue;
            for (const auto& def : varDef->getVarDefs())            {
                if (!def || !def->getNameNode()) continue;
                const std::string name = def->getNameNode()->getName();
                _topLevelVarNames.insert(name);
                // 类型：有初值就按初值推断，否则按标注（拿不到就 int）。
                sema::TypePtr varType;
                if (def->hasInitialValue() && def->getValueNode())                {
                    varType = inferType(def->getValueNode());
                }
                if (!varType) varType = sema::getIntType();
                allocateGlobal(name, varType, varType->getSize());
            }
        }
        // P6 阶段 A：登记所有类的名字、基类、字段、方法（不发射指令）。
        for (const auto& cls : classNodes)        {
            registerClass(cls);
        }
        // P11：重载签名必须在发射**任何**函数体之前登记完 —— 否则前向调用
        // （f1 里调用文件后面才定义的 f2）拿不到候选签名，决议就成了猜。
        // 必须排在 registerClass 之后：形参类型 `x: Point` 要靠类登记表解析。
        registerOverloadSignatures(functionNodes);
        // P6 阶段 B：递归算字段偏移与对象大小（基类先算）。
        // P10：泛型类模板不在这里布局 —— 它没有具体字段类型，布局在
        // ensureClassInstance 里对每个实例单独算。
        for (const auto& cls : classNodes)        {
            if (!cls) continue;
            const auto id = std::dynamic_pointer_cast<ast::IdentifierNode>(cls->getNameNode());
            if (!id) continue;
            computeLayout(id->getName());
        }
        // 函数体在类登记之后发射，这样构造函数/方法调用都能解析。
        for (const auto& fn : functionNodes)        {
            emitFunctionDefinition(fn);
        }
        // P6 阶段 C：把构造器与方法当作独立函数发射出去。
        for (const auto& cls : classNodes)        {
            if (!cls) continue;
            const auto id = std::dynamic_pointer_cast<ast::IdentifierNode>(cls->getNameNode());
            if (!id) continue;
            emitClassBodies(id->getName());
        }
        // P10：泛型类的实例体在 generate() 里统一补发 —— 实例是随着
        // 函数体 / 顶层语句的发射才被发现的，这里发不了。
        // 库模式：只发射函数定义，顶层语句（若有）不包装成 __rio_main。
        // rt / std 这类库不应该有顶层可执行语句，这里对它们直接忽略。
        if (_libraryMode)        {
            return;
        }
        // 第二遍：把顶层语句包进 main
        _functionName = "__rio_main";
        _asm.global("__rio_main");
        _asm.label("__rio_main");
        _asm.push("%rbp");
        _asm.mov("%rsp", "%rbp");
        // 帧大小必须覆盖所有局部变量。
        // 这里采用「先发射、后回填」：占位一行，等语句全部生成、
        // _frameSize 已知后再把真实的 subq 指令写回。
        //
        // 对齐要求：进入函数时 rsp%16==8；push %rbp 后 rsp%16==0；
        // 因此 subq 的量必须是 16 的倍数，才能保证 call 前 rsp%16==0。
        _returnLabel = _asm.newLabel("ret_main");
        // 用一个占位标签包住待回填的帧调整
        const std::string framePlaceholder = _asm.newLabel("frame");
        _asm.label(framePlaceholder);
        _inTopLevelStatements = true;
        for (const auto& expr : topLevelStatements)        {
            emitStatement(expr);
        }
        _inTopLevelStatements = false;
        _asm.label(_returnLabel);
        _asm.mov("%rbp", "%rsp");
        _asm.pop("%rbp");
        _asm.ret();
        // 现在 _frameSize 已经是实际用量，取整到 16 的倍数
        const size_t alignedFrame = ((_frameSize + 15) / 16) * 16;
        // 回填：把占位标签处替换成 subq
        _asm.replaceLabel(framePlaceholder,            "    subq    $" + std::to_string(alignedFrame) + ", %rsp\n");
        popScope();
    }
    void CodeGen::emitMainWrapper()    {
        // 提供 main 作为真正入口，调用 __rio_main 后退出。
        // 注意：进入 main 时 rsp%16==8（返回地址已压栈），
        // push %rbp 后 rsp%16==0，再 subq $32 仍是 16 的倍数，满足调用约定。
        _asm.global("main");
        _asm.label("main");
        _asm.push("%rbp");
        _asm.mov("%rsp", "%rbp");
        _asm.subRsp(32);
          // shadow space
        // 用户自己写了 fun main() 时，它只是一个普通函数，
        // 需要显式 `main()` 才会执行（legacy test_10_main 就是这样写的）。
        // 这里**不能**自动调用 __rio_user_main，否则用户 main 会跑两次。
        _asm.call("__rio_main");
        _asm.movImm(0, "%rax");
        _asm.mov("%rax", "%rcx");
        // 退出码
        _asm.call("core_proc_exit");
        // core_proc_exit 不会返回，下面是防御性代码
        _asm.addRsp(32);
        _asm.mov("%rbp", "%rsp");
        _asm.pop("%rbp");
        _asm.ret();
    }
    // ============================================================
    // 语句
    // ============================================================
    void CodeGen::emitStatement(const std::shared_ptr<ast::ExpressionNode>& node)    {
        if (!node) return;
        // 变量定义
        if (std::dynamic_pointer_cast<ast::VariableDefinitionNode>(node))        {
            emitVariableDefinition(node);
            return;
        }
        // return
        if (const auto ret = std::dynamic_pointer_cast<ast::ReturnExpressionNode>(node))        {
            emitReturn(node);
            return;
        }
        // 函数定义（嵌套）
        if (const auto fn = std::dynamic_pointer_cast<ast::FunctionDefinitionNode>(node))        {
            emitFunctionDefinition(fn);
            return;
        }
        // 赋值
        if (std::dynamic_pointer_cast<ast::AssignmentNode>(node))        {
            emitAssignment(node);
            return;
        }
        // 条件
        if (const auto cond = std::dynamic_pointer_cast<ast::ConditionNode>(node))        {
            emitIf(node);
            return;
        }
        // 循环
        if (std::dynamic_pointer_cast<ast::LoopNode>(node))        {
            emitLoop(node);
            return;
        }
        // break / continue
        if (std::dynamic_pointer_cast<ast::BreakExpressionNode>(node))        {
            emitBreak();
            return;
        }
        if (std::dynamic_pointer_cast<ast::ContinueExpressionNode>(node))        {
            emitContinue();
            return;
        }
        // pass / encapsulated
        if (std::dynamic_pointer_cast<ast::PassExpressionNode>(node) ||            std::dynamic_pointer_cast<ast::EncapsulatedExpressionNode>(node))        {
            return;
        }
        // ---------- P7：异常 ----------
        // try/throw 是语句，但也是 ExpressionNode，所以必须在
        // 「表达式语句：求值后丢弃」之前拦下来。
        if (std::dynamic_pointer_cast<ast::TryNode>(node))        {
            emitTry(node);
            return;
        }
        if (std::dynamic_pointer_cast<ast::ThrowNode>(node))        {
            emitThrow(node);
            return;
        }
        // 块
        if (const auto block = std::dynamic_pointer_cast<ast::BlockRangerNode>(node))        {
            emitBlock(node);
            return;
        }
        // 表达式语句：求值后丢弃
        emitExpression(node);
    }
    // ============================================================
    // 表达式
    // ============================================================
    void CodeGen::emitExpression(const std::shared_ptr<ast::ExpressionNode>& node)    {
        if (!node || hasFailed()) return;
        if (std::dynamic_pointer_cast<ast::IntegerLiteralNode>(node))        {
            emitIntegerLiteral(node);
            return;
        }
        if (std::dynamic_pointer_cast<ast::FloatLiteralNode>(node))        {
            emitFloatLiteral(node);
            return;
        }
        if (std::dynamic_pointer_cast<ast::StringLiteralNode>(node))        {
            emitStringLiteral(node);
            return;
        }
        if (std::dynamic_pointer_cast<ast::BooleanLiteralNode>(node))        {
            emitBoolLiteral(node);
            return;
        }
        if (std::dynamic_pointer_cast<ast::IdentifierNode>(node))        {
            emitIdentifier(node);
            return;
        }
        if (const auto call = std::dynamic_pointer_cast<ast::FunctionCallNode>(node))        {
            emitFunctionCall(node);
            return;
        }
        if (std::dynamic_pointer_cast<ast::AnonFunctionDefinitionNode>(node))        {
            emitAnonymousFunction(node);
            return;
        }
        if (std::dynamic_pointer_cast<ast::UnaryExpressionNode>(node))        {
            emitUnary(node);
            return;
        }
        // ---------- P5：容器 ----------
        // 索引访问必须排在通用 PostfixExpressionNode 之前：IndexExpressionNode
        // 是它的子类，落到下面那条会被 emitPostfix 当成 ++/-- 而报错。
        if (std::dynamic_pointer_cast<ast::IndexExpressionNode>(node))        {
            emitIndexExpression(node);
            return;
        }
        if (std::dynamic_pointer_cast<ast::DictionaryExpressionNode>(node))        {
            emitDictionaryExpression(node);
            return;
        }
        if (std::dynamic_pointer_cast<ast::ListExpressionNode>(node))        {
            emitListExpression(node);
            return;
        }
        if (std::dynamic_pointer_cast<ast::PostfixExpressionNode>(node))        {
            emitPostfix(node);
            return;
        }
        if (const auto infix = std::dynamic_pointer_cast<ast::InfixExpressionNode>(node))        {
            // 复合赋值走专用路径
            const auto t = infix->getOpToken().getType();
            // P6：成员读取 `obj.field`
            if (t == core::TokenType::TOKEN_DOT)            {
                emitAttribute(node);
                return;
            }
            if (t == core::TokenType::TOKEN_PLUS_ASSIGN || t == core::TokenType::TOKEN_MINUS_ASSIGN ||                t == core::TokenType::TOKEN_STAR_ASSIGN || t == core::TokenType::TOKEN_SLASH_ASSIGN ||                t == core::TokenType::TOKEN_MODULO_ASSIGN)            {
                emitCompoundAssign(node);
                return;
            }
            emitBinary(node);
            return;
        }
        if (std::dynamic_pointer_cast<ast::AssignmentNode>(node))        {
            emitAssignment(node);
            return;
        }
        if (const auto paren = std::dynamic_pointer_cast<ast::ParenRangerNode>(node))        {
            emitExpression(paren->getRangerNode());
            return;
        }
        if (std::dynamic_pointer_cast<ast::BlockRangerNode>(node))        {
            emitBlock(node);
            return;
        }
        if (std::dynamic_pointer_cast<ast::ConditionNode>(node))        {
            emitIf(node);
            return;
        }
        if (std::dynamic_pointer_cast<ast::LoopNode>(node))        {
            emitLoop(node);
            return;
        }
        if (std::dynamic_pointer_cast<ast::BreakExpressionNode>(node))        {
            emitBreak();
            return;
        }
        if (std::dynamic_pointer_cast<ast::ContinueExpressionNode>(node))        {
            emitContinue();
            return;
        }
        if (std::dynamic_pointer_cast<ast::PassExpressionNode>(node) ||            std::dynamic_pointer_cast<ast::EncapsulatedExpressionNode>(node))        {
            // pass / encapsulated 作为语句无副作用
            return;
        }
        // ---------- P7：异常 ----------
        if (std::dynamic_pointer_cast<ast::TryNode>(node))        {
            emitTry(node);
            return;
        }
        if (std::dynamic_pointer_cast<ast::ThrowNode>(node))        {
            emitThrow(node);
            return;
        }
        if (std::dynamic_pointer_cast<ast::ReturnExpressionNode>(node))        {
            emitReturn(node);
            return;
        }
        if (std::dynamic_pointer_cast<ast::NullLiteralNode>(node))        {
            _asm.movImm(0, "%rax");
            return;
        }
        // 前缀表达式：`-x` / `+x` / `!x`。
        // parser 把 `-x` 造成 PrefixExpressionNode（NodeType::UNARY），
        // 与 UnaryExpressionNode 是两类节点，必须单独处理，
        // 否则匿名函数体里写 `ret -x` 会直接报「不支持的表达式」。
        if (const auto prefix = std::dynamic_pointer_cast<ast::PrefixExpressionNode>(node))        {
            emitExpression(prefix->getNode());
            if (hasFailed()) return;
            const auto op = prefix->getMainToken().getType();
            if (op == core::TokenType::TOKEN_MINUS)            {
                if (inferType(prefix->getNode())->isFloat())                {
                    _asm.movImm(static_cast<long long>(0x8000000000000000ULL), "%rcx");
                    _asm.inst("movq", "%rcx", "%rdx");
                    _asm.inst("movq", "%xmm0", "%rax");
                    _asm.inst("xorq", "%rdx", "%rax");
                    _asm.inst("movq", "%rax", "%xmm0");
                }
 else                {
                    _asm.inst("negq", "%rax");
                }
                return;
            }
            if (op == core::TokenType::TOKEN_PLUS)            {
                return;
    // 一元正号无操作
            }
            if (op == core::TokenType::TOKEN_NOT)            {
                _asm.inst("testq", "%rax", "%rax");
                _asm.inst("sete", "%al");
                _asm.inst("movzbq", "%al", "%rax");
                return;
            }
            fail("Unsupported prefix operator: " + prefix->getMainToken().getValue());
            return;
        }
        if (const auto ch = std::dynamic_pointer_cast<ast::CharacterLiteralNode>(node))        {
            // 字符字面量的值：优先用 literalString()（形如 'a'），
            // 不能用 node->toString()——后者对字符节点返回的是调试用的
            // [Node(RANGER): ...] 形式，取第一个非引号字符会拿到 '['。
            std::string s = ch->literalString();
            if (s.empty()) s = node->toString();
            long long v = 0;
            for (const char c : s)            {
                if (c == '\'' || c == '"') continue;
                v = static_cast<unsigned char>(c);
                break;
            }
            _asm.movImm(v, "%rax");
            return;
        }
        fail("Unsupported expression in code generation: " + node->toString());
    }
    void CodeGen::emitIntegerLiteral(const std::shared_ptr<ast::ExpressionNode>& node)    {
        // IntegerLiteralNode 的 value 是 private，用 literalString() 取文本
        const auto lit = std::dynamic_pointer_cast<ast::LiteralNode>(node);
        const std::string text = lit ? lit->literalString() : node->toString();
        // 不能用 std::stoll：它在 i64 边界（9223372036854775807）上会抛
        // out_of_range，而那个值是合法的 int 字面量。
        // 这里自己按无符号累加，再按位解释为有符号。
        unsigned long long u = 0;
        bool negative = false;
        bool any = false;
        for (size_t i = 0; i < text.size(); ++i)        {
            char c = text[i];
            if (c == '-' && !any) {
 negative = true;
 continue;
 }
            if (c == '+' && !any) {
 continue;
 }
            if (c == '_' || c == '\'') continue;
          // 数字分隔符
            if (c < '0' || c > '9')            {
                if (any) break;
                            // 后缀之类，停止
                continue;
            }
            any = true;
            u = u * 10ULL + static_cast<unsigned long long>(c - '0');
        }
        long long v = static_cast<long long>(u);
        if (negative) v = static_cast<long long>(0ULL - u);
        _asm.movImm(v, "%rax");
    }
    void CodeGen::emitStringLiteral(const std::shared_ptr<ast::ExpressionNode>& node)    {
        const auto lit = std::dynamic_pointer_cast<ast::StringLiteralNode>(node);
        if (!lit)        {
            _asm.movImm(0, "%rax");
            return;
        }
        // unescapedString() 给出真实字节
        const std::string bytes = lit->unescapedString();
        // 统一用带长度头的布局：这样 str 值既能直接打印（快速路径读长度），
        // 也能存进变量后由 rt_print_str 在运行时读长度。
        const auto [label, len] = _asm.internStrValue(bytes);
        _asm.lea(label + "(%rip)", "%rax");
        (void)len;
    }
    void CodeGen::emitBoolLiteral(const std::shared_ptr<ast::ExpressionNode>& node)    {
        const auto lit = std::dynamic_pointer_cast<ast::LiteralNode>(node);
        const std::string text = lit ? lit->literalString() : "false";
        _asm.movImm(text == "true" ? 1 : 0, "%rax");
    }
    void CodeGen::emitIdentifier(const std::shared_ptr<ast::ExpressionNode>& node)    {
        const auto id = std::dynamic_pointer_cast<ast::IdentifierNode>(node);
        if (!id)        {
            _asm.movImm(0, "%rax");
            return;
        }
        const auto slot = lookupVar(id->getName());
        if (!slot)        {
            // 不是局部变量，但可能是已知函数：允许把函数名当函数值用
            // （`apply(add, 3)` 这种写法）。给它造一个静态闭包对象，
            // 表达式的值就是那个闭包对象的地址。
            if (_functionReturns.find(id->getName()) != _functionReturns.end())            {
                _asm.lea(closureForFunction(id->getName()) + "(%rip)", "%rax");
                return;
            }
            fail("Undefined variable: " + id->getName());
            return;
        }
        const std::string src = addrOf(*slot);
        // 按槽位类型选加载指令：浮点进 %xmm0，其余进 %rax
        if (slot->type && slot->type->isFloat())        {
            _asm.inst("movsd", src, "%xmm0");
            // 同时给出整数视图：Rio 允许把 float 赋给 int（截断），
            // 那种场景由 emitAssignment 负责转换；这里额外把位模式
            // 放进 %rax，方便指针/位操作路径直接使用。
            _asm.mov(src, "%rax");
        }
 else        {
            _asm.mov(src, "%rax");
        }
    }
    void CodeGen::emitBinary(const std::shared_ptr<ast::ExpressionNode>& node)    {
        const auto infix = std::dynamic_pointer_cast<ast::InfixExpressionNode>(node);
        if (!infix)        {
            fail("Not an infix expression: " + node->toString());
            return;
        }
        const auto op = infix->getOpToken().getType();
        // 逗号表达式：顺序求值，结果取最后一个
        if (op == core::TokenType::TOKEN_COMMA)        {
            emitExpression(infix->getLeftNode());
            emitExpression(infix->getRightNode());
            return;
        }
        // 逻辑运算必须短路求值，不能先算右边
        if (op == core::TokenType::TOKEN_AND || op == core::TokenType::TOKEN_OR)        {
            const std::string shortLabel = _asm.newLabel("logic_short");
            const std::string endLabel = _asm.newLabel("logic_end");
            emitExpression(infix->getLeftNode());
            _asm.inst("testq", "%rax", "%rax");
            // && : 左边为假 -> 结果 0，跳到结束
            // || : 左边为真 -> 结果 1，跳到结束
            _asm.inst(op == core::TokenType::TOKEN_AND ? "je" : "jne", shortLabel);
            emitExpression(infix->getRightNode());
            _asm.inst("testq", "%rax", "%rax");
            _asm.inst("setne", "%al");
            _asm.inst("movzbq", "%al", "%rax");
            _asm.jmp(endLabel);
            _asm.label(shortLabel);
            _asm.movImm(op == core::TokenType::TOKEN_AND ? 0 : 1, "%rax");
            _asm.label(endLabel);
            return;
        }
        // 浮点运算走 XMM
        const auto lt = inferType(infix->getLeftNode());
        const auto rt = inferType(infix->getRightNode());
        // P12/G3 + G4：`str + str` / `list + list` 是**拼接**，不是整数加法。
        // 之前这里无条件走 addq，于是 `"a" + "b"` 把两个指针相加，
        // 结果按 int 解释（原仓库 str.rio:111 的 `[""] + result + ""`、
        // lowlevel.rio:53 的 `i + ": "` 都依赖拼接）。
        if (op == core::TokenType::TOKEN_PLUS)        {
            const bool bothStr = lt && rt && lt->getKind() == sema::TypeKind::TK_STR &&
                                 rt->getKind() == sema::TypeKind::TK_STR;
            const bool bothList = lt && rt && lt->getKind() == sema::TypeKind::TK_LIST &&
                                  rt->getKind() == sema::TypeKind::TK_LIST;
            if (bothStr || bothList)            {
                // 两个实参都用栈槽传递，避免 push 造成 rsp 非对齐
                const std::string lhsSlot = allocateTemp(lt);
                emitExpression(infix->getLeftNode());
                if (hasFailed()) return;
                _asm.mov("%rax", lhsSlot);
                const std::string rhsSlot = allocateTemp(rt);
                emitExpression(infix->getRightNode());
                if (hasFailed()) return;
                _asm.mov("%rax", rhsSlot);
                if (bothStr)                {
                    emitStrConcat(lhsSlot, rhsSlot);
                }
                else                {
                    emitListConcat(lhsSlot, rhsSlot);
                }
                return;
            }
        }
        if (lt->isFloat() || rt->isFloat())        {
            emitFloatBinary(node, lt, rt);
            return;
        }
        // P12/G9：`str == str` / `str != str` 必须按**内容**比较。
        // 下面走的是 cmpq + sete，比的是两个指针；字符串字面量即使内容相同
        // 地址也不同，所以永远 ne（原仓库 str.rio:105 的 `if del == ""`、
        // datatype.rio:92 的 `typeOfArg1 == arg2[i]` 全都依赖内容比较）。
        if ((op == core::TokenType::TOKEN_EQUAL || op == core::TokenType::TOKEN_NOT_EQUAL) &&
            lt && rt && lt->getKind() == sema::TypeKind::TK_STR &&
            rt->getKind() == sema::TypeKind::TK_STR)        {
            const std::string lhsSlot = allocateTemp(lt);
            emitExpression(infix->getLeftNode());
            if (hasFailed()) return;
            _asm.mov("%rax", lhsSlot);
            const std::string rhsSlot = allocateTemp(rt);
            emitExpression(infix->getRightNode());
            if (hasFailed()) return;
            _asm.mov("%rax", rhsSlot);
            emitStrEq(lhsSlot, rhsSlot);
            if (op == core::TokenType::TOKEN_NOT_EQUAL)            {
                _asm.inst("xorq", "$1", "%rax");
            }
            return;
        }
        // 左值求值 -> push
        emitExpression(infix->getLeftNode());
        _asm.push("%rax");
        // 右值求值 -> %rax
        emitExpression(infix->getRightNode());
        // 恢复左值到 %rcx
        _asm.pop("%rcx");
        switch (op)        {
        case core::TokenType::TOKEN_PLUS:            _asm.add("%rcx", "%rax");
            break;
        case core::TokenType::TOKEN_MINUS:            // rax = left - right => 需要 rax=left, rcx=right
            _asm.mov("%rax", "%rdx");
            _asm.mov("%rcx", "%rax");
            _asm.sub("%rdx", "%rax");
            break;
        case core::TokenType::TOKEN_STAR:            _asm.imul("%rcx", "%rax");
            break;
        case core::TokenType::TOKEN_SLASH:            // rcx = 左（被除数），rax = 右（除数）
            // idivq 要求被除数在 rdx:rax、除数在另一个寄存器里。
            // 注意不能把除数留在 %rdx —— cqto 会把它覆盖掉（这正是除零崩溃的原因）。
            _asm.mov("%rax", "%r10");
        // 除数暂存到 r10
            _asm.mov("%rcx", "%rax");
        // 被除数 -> rax
            _asm.inst("cqto");
               // 符号扩展到 rdx:rax
            _asm.inst("idivq", "%r10");
      // rax = 商
            break;
        case core::TokenType::TOKEN_MODULO:            // 同除法，但结果取余数 %rdx
            _asm.mov("%rax", "%r10");
            _asm.mov("%rcx", "%rax");
            _asm.inst("cqto");
            _asm.inst("idivq", "%r10");
            _asm.mov("%rdx", "%rax");
            break;
        case core::TokenType::TOKEN_EQUAL:        case core::TokenType::TOKEN_NOT_EQUAL:        case core::TokenType::TOKEN_LESS:        case core::TokenType::TOKEN_LESS_EQUAL:        case core::TokenType::TOKEN_GREATER:        case core::TokenType::TOKEN_GREATER_EQUAL:        {
            _asm.cmp("%rax", "%rcx");
   // 比较 rcx 与 rax
            const char* cc = "e";
            switch (op)            {
            case core::TokenType::TOKEN_EQUAL:         cc = "e";
  break;
            case core::TokenType::TOKEN_NOT_EQUAL:     cc = "ne";
 break;
            case core::TokenType::TOKEN_LESS:          cc = "l";
  break;
            case core::TokenType::TOKEN_LESS_EQUAL:    cc = "le";
 break;
            case core::TokenType::TOKEN_GREATER:       cc = "g";
  break;
            case core::TokenType::TOKEN_GREATER_EQUAL: cc = "ge";
 break;
            default: break;
            }
            _asm.inst(std::string("set") + cc, "%al");
            _asm.inst("movzbq", "%al", "%rax");
            break;
        }
        default:            fail("Unsupported binary operator: " + infix->getOpToken().getValue());
            break;
        }
    }
    void CodeGen::emitFloatBinary(const std::shared_ptr<ast::ExpressionNode>& node,                                  const sema::TypePtr& lt, const sema::TypePtr& rt)    {
        const auto infix = std::dynamic_pointer_cast<ast::InfixExpressionNode>(node);
        const auto op = infix->getOpToken().getType();
        // 一边是整数、一边是浮点：把整数转成 double
        auto emitOperand = [&](const std::shared_ptr<ast::ExpressionNode>& e,                               const sema::TypePtr& t)        {
            emitExpression(e);
            if (!t->isFloat())            {
                // cvtsi2sdq %rax, %xmm0
                _asm.inst("cvtsi2sdq", "%rax", "%xmm0");
            }
        }
;
        // 约定：emitExpression 的结果在 %xmm0。
        // 二元运算需要把「左」放到 %xmm0、「右」放到 %xmm1，
        // 这样 `subsd %xmm1, %xmm0` 才是 左-右（AT&T 语法是 源,目标）。
        emitOperand(infix->getLeftNode(), lt);
        _asm.inst("movsd", "%xmm0", "%xmm2");
        // 左值先存到 xmm2
        emitOperand(infix->getRightNode(), rt);
      // 右值 -> xmm0
        _asm.inst("movsd", "%xmm0", "%xmm1");
        // 右值 -> xmm1
        _asm.inst("movsd", "%xmm2", "%xmm0");
        // 左值 -> xmm0
        switch (op)        {
        case core::TokenType::TOKEN_PLUS:  _asm.inst("addsd", "%xmm1", "%xmm0");
 break;
        case core::TokenType::TOKEN_MINUS: _asm.inst("subsd", "%xmm1", "%xmm0");
 break;
        case core::TokenType::TOKEN_STAR:  _asm.inst("mulsd", "%xmm1", "%xmm0");
 break;
        case core::TokenType::TOKEN_SLASH: _asm.inst("divsd", "%xmm1", "%xmm0");
 break;
        default:            // 浮点比较：xmm0 = 左, xmm1 = 右
            if (op == core::TokenType::TOKEN_EQUAL || op == core::TokenType::TOKEN_NOT_EQUAL ||                op == core::TokenType::TOKEN_LESS || op == core::TokenType::TOKEN_LESS_EQUAL ||                op == core::TokenType::TOKEN_GREATER || op == core::TokenType::TOKEN_GREATER_EQUAL)            {
                // ucomisd 只设置 CF/ZF/PF（无符号标志），不设置 SF/OF。
                // 所以浮点比较必须用「无符号」条件码 b/be/a/ae，
                // 用 l/le/g/ge 会永远得到错误结果（这是 -0.5 < 0.0 判断失败的原因）。
                //   ucomisd %xmm1, %xmm0   -> 比较 %xmm0 ? %xmm1
                //   小于   : CF=1        -> setb
                //   小于等于: CF=1 或 ZF=1 -> setbe
                //   大于   : CF=0 且 ZF=0 -> seta
                //   大于等于: CF=0        -> setae
                _asm.inst("ucomisd", "%xmm1", "%xmm0");
                const char* cc = "e";
                switch (op)                {
                case core::TokenType::TOKEN_EQUAL:         cc = "e";
  break;
                case core::TokenType::TOKEN_NOT_EQUAL:     cc = "ne";
 break;
                case core::TokenType::TOKEN_LESS:          cc = "b";
  break;
                case core::TokenType::TOKEN_LESS_EQUAL:    cc = "be";
 break;
                case core::TokenType::TOKEN_GREATER:       cc = "a";
  break;
                case core::TokenType::TOKEN_GREATER_EQUAL: cc = "ae";
 break;
                default: break;
                }
                _asm.inst(std::string("set") + cc, "%al");
                _asm.inst("movzbq", "%al", "%rax");
                return;
            }
            fail("Unsupported float binary operator: " + infix->getOpToken().getValue());
            break;
        }
    }
    void CodeGen::emitUnary(const std::shared_ptr<ast::ExpressionNode>& node)    {
        const auto unary = std::dynamic_pointer_cast<ast::UnaryExpressionNode>(node);
        if (!unary)        {
            fail("Not a unary expression: " + node->toString());
            return;
        }
        emitExpression(unary->getRightNode());
        const auto t = unary->getOpToken().getType();
        if (t == core::TokenType::TOKEN_NOT)        {
            // 逻辑非：0 -> 1，非 0 -> 0
            _asm.inst("testq", "%rax", "%rax");
            _asm.inst("sete", "%al");
            _asm.inst("movzbq", "%al", "%rax");
            return;
        }
        if (t == core::TokenType::TOKEN_MINUS)        {
            if (inferType(unary->getRightNode())->isFloat())            {
                // 取反：与 0.0 相减会引入 -0.0 问题，直接翻符号位更准确
                _asm.movImm(static_cast<long long>(0x8000000000000000ULL), "%rcx");
                _asm.inst("movq", "%rcx", "%rdx");
                _asm.inst("movq", "%xmm0", "%rax");
                _asm.inst("xorq", "%rdx", "%rax");
                _asm.inst("movq", "%rax", "%xmm0");
            }
 else            {
                _asm.inst("negq", "%rax");
            }
            return;
        }
        if (t == core::TokenType::TOKEN_PLUS)        {
            return;
   // 一元正号无操作
        }
        fail("Unsupported unary operator: " + unary->getOpToken().getValue());
    }
    void CodeGen::emitPostfix(const std::shared_ptr<ast::ExpressionNode>& node)    {
        const auto postfix = std::dynamic_pointer_cast<ast::PostfixExpressionNode>(node);
        if (!postfix)        {
            fail("Not a postfix expression: " + node->toString());
            return;
        }
        const auto id = std::dynamic_pointer_cast<ast::IdentifierNode>(postfix->getNode());
        if (!id)        {
            fail("Only simple variable ++/-- is supported: " + node->toString());
            return;
        }
        const auto slot = lookupVar(id->getName());
        if (!slot)        {
            fail("Undefined variable in ++/--: " + id->getName());
            return;
        }
        const std::string addr = addrOf(*slot);
        _asm.mov(addr, "%rax");
        if (postfix->getPostfixType() == ast::NodeType::INCREMENT)        {
            _asm.inst("addq", "$1", "%rax");
        }
 else        {
            _asm.inst("subq", "$1", "%rax");
        }
        _asm.mov("%rax", addr);
    }
    void CodeGen::emitCompoundAssign(const std::shared_ptr<ast::ExpressionNode>& node)    {
        const auto infix = std::dynamic_pointer_cast<ast::InfixExpressionNode>(node);
        if (!infix)        {
            fail("Not a compound assignment: " + node->toString());
            return;
        }
        const auto id = std::dynamic_pointer_cast<ast::IdentifierNode>(infix->getLeftNode());
        if (!id)        {
            // P12/G2：`a[i] += v` —— 元素复合赋值。
            // 降低成 `a[i] = a[i] <op> v` 再走 emitIndexAssign，避免为
            // 「读旧值 + 写回」再复制一份地址计算逻辑（dict 的写回还要
            // 处理扩容，重复实现只会引入不一致）。
            if (const auto idx = std::dynamic_pointer_cast<ast::IndexExpressionNode>(
                    infix->getLeftNode()))            {
                emitExpression(infix->getRightNode());
                if (hasFailed()) return;
                const std::string rhsSlot = allocateTemp(inferType(infix->getRightNode()));
                _asm.mov("%rax", rhsSlot);
                // 读旧值
                emitExpression(infix->getLeftNode());
                if (hasFailed()) return;
                // 按 op 合并：旧值在 %rax，右值从槽里取回
                const std::string contSlot = allocateTemp(inferType(idx->getLeftNode()));
                _asm.mov("%rax", contSlot);
                _asm.mov(rhsSlot, "%rcx");
                _asm.mov(contSlot, "%rax");
                switch (infix->getOpToken().getType())                {
                case core::TokenType::TOKEN_PLUS_ASSIGN:                    _asm.add("%rcx", "%rax");
                    break;
                case core::TokenType::TOKEN_MINUS_ASSIGN:                    _asm.sub("%rcx", "%rax");
                    break;
                case core::TokenType::TOKEN_STAR_ASSIGN:                    _asm.imul("%rcx", "%rax");
                    break;
                case core::TokenType::TOKEN_SLASH_ASSIGN:                    // %rax = 旧值，%rcx = 右值；除法是 old / rhs
                    _asm.mov("%rcx", "%r10");
                    _asm.inst("cqto");
                    _asm.inst("idivq", "%r10");
                    break;
                case core::TokenType::TOKEN_MODULO_ASSIGN:                    _asm.mov("%rcx", "%r10");
                    _asm.inst("cqto");
                    _asm.inst("idivq", "%r10");
                    _asm.mov("%rdx", "%rax");
                    break;
                default:                    fail("Unsupported compound assignment: " +
                         infix->getOpToken().getValue());
                    return;
                }
                if (!emitIndexAssignValue(idx, inferType(infix->getRightNode())))                {
                    failAt(infix->getPos(), "Index compound assignment on a non-container value: " +
                           infix->getLeftNode()->toString());
                }
                return;
            }
            fail("Only simple variable compound assignment is supported: " + node->toString());
            return;
        }
        const auto slot = lookupVar(id->getName());
        if (!slot)        {
            fail("Undefined variable in compound assignment: " + id->getName());
            return;
        }
        const std::string addr = addrOf(*slot);
        // P12/G4：`s += "x"` / `l += [v]` 与 `s = s + "x"` 同义，必须走拼接而不是
        // 整数加法 —— 否则两个指针相加，写回后一读就崩（G3/G4 只在 emitBinary
        // 的 TOKEN_PLUS 分支处理了拼接，复合赋值是另一条路径）。
        if (infix->getOpToken().getType() == core::TokenType::TOKEN_PLUS_ASSIGN &&
            slot->type &&
            (slot->type->getKind() == sema::TypeKind::TK_STR ||
             slot->type->getKind() == sema::TypeKind::TK_LIST))        {
            const std::string lhsSlot = allocateTemp(slot->type);
            _asm.mov(addr, "%rax");
            _asm.mov("%rax", lhsSlot);
            const auto rhsType = inferType(infix->getRightNode());
            // 右值必须同族：`s += 1` / `l += 1` 没有意义，早报错好过让
            // addq 把指针和整数加起来、写回一个野地址再在下一次读取时崩。
            if (!rhsType || rhsType->getKind() != slot->type->getKind())            {
                failAt(infix->getPos(),
                       std::string("Compound '+=' on ") +
                           (slot->type->getKind() == sema::TypeKind::TK_STR ? "str" : "list") +
                           " requires a " +
                           (slot->type->getKind() == sema::TypeKind::TK_STR ? "str" : "list") +
                           " right-hand side, got: " + infix->getRightNode()->toString());
                return;
            }
            const std::string rhsSlot = allocateTemp(rhsType);
            emitExpression(infix->getRightNode());
            if (hasFailed()) return;
            _asm.mov("%rax", rhsSlot);
            if (slot->type->getKind() == sema::TypeKind::TK_STR)            {
                emitStrConcat(lhsSlot, rhsSlot);
            }
            else            {
                emitListConcat(lhsSlot, rhsSlot);
            }
            _asm.mov("%rax", addr);
            return;
        }
        // 先把右值算出来暂存，再取左值
        emitExpression(infix->getRightNode());
        _asm.push("%rax");
        _asm.mov(addr, "%rcx");
        _asm.pop("%rax");
        switch (infix->getOpToken().getType())        {
        case core::TokenType::TOKEN_PLUS_ASSIGN:            _asm.add("%rcx", "%rax");
            break;
        case core::TokenType::TOKEN_MINUS_ASSIGN:            _asm.mov("%rax", "%rdx");
            _asm.mov("%rcx", "%rax");
            _asm.sub("%rdx", "%rax");
            break;
        case core::TokenType::TOKEN_STAR_ASSIGN:            _asm.imul("%rcx", "%rax");
            break;
        case core::TokenType::TOKEN_SLASH_ASSIGN:            // 除数不能留在 %rdx（cqto 会覆盖它）
            _asm.mov("%rax", "%r10");
            _asm.mov("%rcx", "%rax");
            _asm.inst("cqto");
            _asm.inst("idivq", "%r10");
            break;
        case core::TokenType::TOKEN_MODULO_ASSIGN:            _asm.mov("%rax", "%r10");
            _asm.mov("%rcx", "%rax");
            _asm.inst("cqto");
            _asm.inst("idivq", "%r10");
            _asm.mov("%rdx", "%rax");
            break;
        default:            fail("Unsupported compound assignment: " + infix->getOpToken().getValue());
            return;
        }
        _asm.mov("%rax", addr);
    }
    void CodeGen::emitAssignment(const std::shared_ptr<ast::ExpressionNode>& node)    {
        const auto assign = std::dynamic_pointer_cast<ast::AssignmentNode>(node);
        if (!assign)        {
            fail("Not an assignment: " + node->toString());
            return;
        }
        const auto [lhs, rhs] = assign->getAssignPair();
        // P6：`obj.field = v` —— 左值是成员访问，走专用路径。
        if (const auto member = std::dynamic_pointer_cast<ast::InfixExpressionNode>(lhs))        {
            if (member->getOpToken().getType() == core::TokenType::TOKEN_DOT)            {
                emitAttributeAssign(member, rhs);
                return;
            }
        }
        // P12/G2：`a[i] = v` / `d[k] = v`。值必须先算出来，再交给
        // emitIndexAssign（它会把值落临时槽，然后算元素地址写回）。
        if (const auto idx = std::dynamic_pointer_cast<ast::IndexExpressionNode>(lhs))        {
            emitExpression(rhs);
            if (hasFailed()) return;
            if (!emitIndexAssign(idx, rhs))            {
                failAt(assign->getPos(), "Index assignment on a non-container value: " +
                       lhs->toString());
            }
            return;
        }
        const auto id = std::dynamic_pointer_cast<ast::IdentifierNode>(lhs);
        if (!id)        {
            fail("Only simple variable assignment is supported: " + node->toString());
            return;
        }
        const auto slot = lookupVar(id->getName());
        if (!slot)        {
            fail("Undefined variable in assignment: " + id->getName());
            return;
        }
        emitExpression(rhs);
        if (hasFailed()) return;
        const std::string dest = addrOf(*slot);
        const auto rhsType = inferType(rhs);
        // P9：赋值是「把值塞进一个有静态类型的槽」，必须检查。
        // 之前 `var x = 1` 之后 `x = "str"` 会把字符串指针塞进整型槽，
        // 之后每次读 x 都拿到垃圾。
        if (slot->type)        {
            const auto why = checkAssignable(slot->type, rhsType);
            if (!why.empty())            {
                failAt(assign->getPos(),
                       "Assignment to '" + id->getName() + "': " + why);
                return;
            }
        }
        const bool destIsFloat = slot->type && slot->type->isFloat();
        const bool srcIsFloat = rhsType && rhsType->isFloat();
        if (destIsFloat && !srcIsFloat)        {
            // int -> float：值在 %rax，转换成 double 再存
            _asm.inst("cvtsi2sdq", "%rax", "%xmm0");
            _asm.inst("movsd", "%xmm0", dest);
        }
        else if (!destIsFloat && srcIsFloat)        {
            // float -> int：截断取整
            _asm.inst("cvttsd2siq", "%xmm0", "%rax");
            _asm.mov("%rax", dest);
        }
        else if (destIsFloat)        {
            _asm.inst("movsd", "%xmm0", dest);
        }
        else        {
            _asm.mov("%rax", dest);
        }
    }
    void CodeGen::emitVariableDefinition(const std::shared_ptr<ast::ExpressionNode>& node)    {
        const auto varDef = std::dynamic_pointer_cast<ast::VariableDefinitionNode>(node);
        if (!varDef)        {
            fail("Not a variable definition: " + node->toString());
            return;
        }
        for (const auto& def : varDef->getVarDefs())        {
            if (!def || !def->getNameNode()) continue;
            const std::string name = def->getNameNode()->getName();
            const bool hasInit = def->hasInitialValue() && def->getValueNode() != nullptr;
            const auto initType = hasInit ? inferType(def->getValueNode()) : sema::getUnknownType();
            // P9：显式标注成为权威类型。没标注时维持 P1 起的推断行为
            // （legacy 15 个用例全靠它，不能动）。
            sema::TypePtr type = sema::getIntType();
            if (def->hasLabels())            {
                const auto annotated = typeFromLabels(def->getLabelNodes());
                if (annotated && annotated->getKind() != sema::TypeKind::TK_UNKNOWN)                {
                    type = annotated;
                    // 标注与初始值不一致 -> 报错，而不是照标注静默存下去。
                    if (hasInit)                    {
                        const auto why = checkAssignable(type, initType);
                        if (!why.empty())                        {
                            failAt(def->getNameNode()->getPos(),
                                   "Variable '" + name + "' declared as " + type->toString() +
                                       ": " + why);
                            return;
                        }
                    }
                }
 else                {
                    // 写了标注却解析不出来（拼错 / 未定义的类名）：明确报错，
                    // 否则会退化成 int 槽位，后面所有字段访问都变成垃圾。
                    std::string raw;
                    for (const auto& label : def->getLabelNodes())                    {
                        if (label) { raw = label->getFullLabel(); break; }
                    }
                    // P12/G13：`var x: any` 等价于不标注，不是未知类型。
                    std::string firstWord;
                    for (const char c : raw)                    {
                        if (std::isalnum(static_cast<unsigned char>(c)) || c == '_') firstWord += c;
                        else if (!firstWord.empty()) break;
                    }
                    if (firstWord == "any")                    {
                        type = hasInit ? initType : sema::getUnknownType();
                        if (type && type->getKind() == sema::TypeKind::TK_UNKNOWN) type = sema::getIntType();
                    }
                    else                    {
                        failAt(def->getNameNode()->getPos(),
                               "Unknown type '" + raw + "' in declaration of '" + name + "'");
                        return;
                    }
                }
            }
 else if (hasInit)            {
                type = initType;
            }
            // P12/G1a：顶层 var 已经有全局槽了（emitProgram 里登记过），
            // 这里不能再去分配一个栈槽 —— 否则函数体看到的是全局那份，
            // 而初始化写进了栈上这份，全局永远是 0。
            std::string slot;
            if (_inTopLevelStatements && _topLevelVarNames.count(name) != 0)            {
                const auto found = _globalSlots.find(name);
                if (found == _globalSlots.end())                {
                    fail("Internal: top-level var '" + name + "' has no global slot");
                    return;
                }
                slot = found->second + "(%rip)";
                // 让全局槽的类型与本次推断保持一致
                if (auto* gslot = lookupVar(name)) gslot->type = type;
            }
 else            {
                const long long offset = allocateSlot(name, type, type->getSize());
                slot = std::to_string(offset) + "(%rbp)";
            }
            if (hasInit)            {
                emitExpression(def->getValueNode());
                if (hasFailed()) return;
                // 值在 %xmm0（浮点）或 %rax（其它），按目标类型选存储指令，
                // 并在需要时插入 int<->float 转换 —— 标注为权威后，
                // `var x: float = 1` 必须真的存成 1.0。
                const bool destIsFloat = type->isFloat();
                const bool srcIsFloat = initType && initType->isFloat();
                if (destIsFloat && !srcIsFloat)                {
                    _asm.inst("cvtsi2sdq", "%rax", "%xmm0");
                    _asm.inst("movsd", "%xmm0", slot);
                }
 else if (!destIsFloat && srcIsFloat)                {
                    _asm.inst("cvttsd2siq", "%xmm0", "%rax");
                    _asm.mov("%rax", slot);
                }
 else if (destIsFloat)                {
                    _asm.inst("movsd", "%xmm0", slot);
                }
 else                {
                    _asm.mov("%rax", slot);
                }
            }
 else            {
                if (type->isFloat())                {
                    _asm.inst("xorpd", "%xmm0", "%xmm0");
                    _asm.inst("movsd", "%xmm0", slot);
                }
 else                {
                    _asm.movImm(0, "%rax");
                    _asm.mov("%rax", slot);
                }
            }
        }
    }
    void CodeGen::emitReturn(const std::shared_ptr<ast::ExpressionNode>& node)    {
        const auto ret = std::dynamic_pointer_cast<ast::ReturnExpressionNode>(node);
        // P9：只有当返回类型是**显式标注**时才校验 ret 的值。
        // 推断出来的返回类型来自「函数体里第一个 ret」，拿它去约束
        // 其余分支只会自相矛盾；而标注是用户的承诺，可以查。
        if (ret && _functionReturnAnnotated && _functionReturnType)        {
            if (ret->getReturnNode())            {
                if (_functionReturnType->isVoid())                {
                    failAt(node->getPos(), "Function '" + _functionName +
                           "' is declared to return void and cannot return a value");
                    return;
                }
                const auto why = checkAssignable(_functionReturnType, inferType(ret->getReturnNode()));
                if (!why.empty())                {
                    failAt(node->getPos(), "Return value in '" + _functionName + "': " + why);
                    return;
                }
            }
 else            {
                // `ret` 无值，但函数声明了非 void 返回类型：调用方会拿
                // %rax 里的垃圾当结果用，属于明确的错。
                if (!_functionReturnType->isVoid())                {
                    failAt(node->getPos(), "Function '" + _functionName + "' is declared to return " +
                           _functionReturnType->toString() + " but 'ret' has no value");
                    return;
                }
            }
        }
        if (ret && ret->getReturnNode())        {
            emitExpression(ret->getReturnNode());
            if (hasFailed()) return;
            // P9：标注返回类型后，ret 的值必须按标注的类型落到返回寄存器。
            // `fun one(): float { ret 1 }` 之前直接把 %rax 里的 1 当 double
            // 用，调用方 sout 打印出垃圾 —— 必须在这里插转换。
            if (_functionReturnAnnotated && _functionReturnType)            {
                const auto vt = inferType(ret->getReturnNode());
                const bool destIsFloat = _functionReturnType->isFloat();
                const bool srcIsFloat = vt && vt->isFloat();
                if (destIsFloat && !srcIsFloat)                {
                    _asm.inst("cvtsi2sdq", "%rax", "%xmm0");
                }
 else if (!destIsFloat && srcIsFloat)                {
                    _asm.inst("cvttsd2siq", "%xmm0", "%rax");
                }
            }
        }
        // ret 跳出所有 try：把 handler 链全部摘干净再跳。
        // 不这么做，链上会留下指向本帧（马上被 ret 销毁）的节点，
        // 外层下一次 throw 就会跳进死帧。
        if (!_ehStack.empty())        {
            // core_eh_pop 用 %rax 返回 frame->prev，会把刚算好的返回值冲掉，
            // 所以先存进临时槽，展开完再装回来。浮点返回值在 %xmm0，
            // core_eh_pop 不碰它，但仍统一走一遍保存/恢复更稳。
            // 注意：上面可能已经按标注类型插入了转换，所以这里判定的
            // 必须是「转换之后的值类别」，也就是标注类型，而不是源表达式类型。
            const auto effective = (_functionReturnAnnotated && _functionReturnType)
                                       ? _functionReturnType
                                       : (ret && ret->getReturnNode() ? inferType(ret->getReturnNode()) : nullptr);
            const bool isFloat = effective && effective->isFloat();
            const std::string tmp = allocateTemp(isFloat ? sema::getFloatType() : sema::getIntType());
            if (isFloat)            {
                _asm.inst("movsd", "%xmm0", tmp);
            }
 else            {
                _asm.mov("%rax", tmp);
            }
            emitEhUnwindTo(0);
            if (isFloat)            {
                _asm.inst("movsd", tmp, "%xmm0");
            }
 else            {
                _asm.mov(tmp, "%rax");
            }
        }
        if (!_returnLabel.empty())        {
            _asm.jmp(_returnLabel);
        }
    }
    void CodeGen::emitIf(const std::shared_ptr<ast::ExpressionNode>& node)    {
        const auto cond = std::dynamic_pointer_cast<ast::ConditionNode>(node);
        if (!cond)        {
            fail("Not a condition node: " + node->toString());
            return;
        }
        const std::string endLabel = _asm.newLabel("cond_end");
        for (const auto& branchExpr : cond->getBranchNodes())        {
            const auto branch = std::dynamic_pointer_cast<ast::BranchNode>(branchExpr);
            if (!branch) continue;
            const std::string nextLabel = _asm.newLabel("cond_next");
            if (branch->getConditionNode())            {
                emitExpression(branch->getConditionNode());
                _asm.inst("testq", "%rax", "%rax");
                _asm.je(nextLabel);
            }
            pushScope();
            emitStatement(branch->getBodyNode());
            popScope();
            _asm.jmp(endLabel);
            _asm.label(nextLabel);
        }
        _asm.label(endLabel);
    }
    void CodeGen::emitBlock(const std::shared_ptr<ast::ExpressionNode>& node)    {
        const auto block = std::dynamic_pointer_cast<ast::BlockRangerNode>(node);
        if (!block)        {
            fail("Not a block: " + node->toString());
            return;
        }
        pushScope();
        for (const auto& expr : block->getBodyExpressions())        {
            emitStatement(expr);
        }
        popScope();
    }
    void CodeGen::emitLoop(const std::shared_ptr<ast::ExpressionNode>& node)    {
        const auto loop = std::dynamic_pointer_cast<ast::LoopNode>(node);
        if (!loop)        {
            fail("Not a loop node: " + node->toString());
            return;
        }
        const std::string condLabel = _asm.newLabel("loop_cond");
        const std::string bodyLabel = _asm.newLabel("loop_body");
        const std::string endLabel = _asm.newLabel("loop_end");
        // for 的 init 在循环外求值
        const auto forLoop = std::dynamic_pointer_cast<ast::ForLoopNode>(node);
        pushScope();
        if (forLoop && forLoop->getInitNode())        {
            emitStatement(forLoop->getInitNode());
        }
        _asm.label(condLabel);
        if (loop->getConditionNode())        {
            emitExpression(loop->getConditionNode());
            _asm.inst("testq", "%rax", "%rax");
            // until 与 while 条件相反
            if (loop->getLoopType() == core::LoopType::UNTIL)            {
                _asm.inst("jne", endLabel);
      // 条件为真 -> 退出
            }
 else            {
                _asm.je(endLabel);
               // 条件为假 -> 退出
            }
        }
        _asm.label(bodyLabel);
        // continue 的落点：
        //   while/until -> 回到条件判断
        //   for         -> 先跳到 update 段（否则 continue 会跳过 i = i + 1，死循环）
        const std::string continueLabel =
            (forLoop && forLoop->getUpdateNode())
                ? _asm.newLabel("loop_update")
                : condLabel;
        _loopStack.push_back({continueLabel, endLabel, _ehStack.size()});
        pushScope();
        if (loop->getBodyNode())        {
            if (const auto block = std::dynamic_pointer_cast<ast::BlockRangerNode>(loop->getBodyNode()))            {
                for (const auto& e : block->getBodyExpressions()) emitStatement(e);
            }
 else            {
                emitStatement(loop->getBodyNode());
            }
        }
        popScope();
        _loopStack.pop_back();
        // for 的 update 在每轮体末尾、回跳之前求值。
        // continue 也落在这里（见上面 continueLabel 的取法）。
        if (forLoop && forLoop->getUpdateNode())        {
            _asm.label(continueLabel);
            emitStatement(forLoop->getUpdateNode());
        }
        _asm.jmp(condLabel);
        _asm.label(endLabel);
        popScope();
    }
    void CodeGen::emitBreak()    {
        if (_loopStack.empty())        {
            fail("'break' outside of a loop");
            return;
        }
        // 只弹循环体内建立的帧；循环外的 try 帧要留着。
        emitEhUnwindTo(_loopStack.back().ehDepth);
        _asm.jmp(_loopStack.back().breakLabel);
    }
    void CodeGen::emitContinue()    {
        if (_loopStack.empty())        {
            fail("'continue' outside of a loop");
            return;
        }
        emitEhUnwindTo(_loopStack.back().ehDepth);
        _asm.jmp(_loopStack.back().continueLabel);
    }
    // ============================================================
    // P7：异常（方案 B —— 显式帧链，setjmp/longjmp 风格）
    // ============================================================
    //
    // 帧块（48 字节，全部通过 allocateSlot 走 %rbp 相对槽位）：
    //   +0   prev        core_eh_push 写入的上一帧（链指针）
    //   +8   saved rbp   进入 try 时的 %rbp
    //   +16  saved rsp   进入 try 时的 %rsp
    //   +24  resume      异常落地后跳转的标签地址
    //   +32  exc         异常值（core_eh_throw 写入，catch 读出）
    //   +40  pad         对齐填充
    //
    // 为什么把 rbp/rsp 存进栈槽而不是借 %rbx / %r12 之类：
    // 那些寄存器目前虽然没被 codegen 用到（grep 确认），但一旦将来引入
    // 寄存器分配就会冲突；栈槽是 %rbp 相对的，跨函数调用天然稳定，且
    // 由帧回填统一覆盖。
    //
    // 非局部退出不保证跑析构 —— 这是 docs/architecture_plan.md §8.2 里
    // 已接受的代价。但 **handler 链必须保持干净**：ret / break / continue
    // 跳出 try 时显式 core_eh_pop，否则链上留下指向已销毁栈帧的节点，
    // 下一次 throw 就会跳进死帧。
    void CodeGen::emitEhPop(long long frameOffset)    {
        _asm.lea(std::to_string(frameOffset) + "(%rbp)", "%rcx");
        _asm.subRsp(32);
        _asm.call("core_eh_pop");
        _asm.addRsp(32);
    }
    void CodeGen::emitEhUnwindTo(size_t depth)    {
        // 只发射弹出指令，**不动** _ehStack：调用点（ret / break / continue）
        // 都是终结性跳转，但它们之后的代码（比如 try 的正常收尾）仍会被
        // 发射出去，编译期帧栈必须保持与词法嵌套一致。
        if (_ehStack.size() <= depth) return;
        // 从内往外：每离开一层 try，先跑它的 finally，再摘链。
        // _emittingFinally 防重入：finally 体里若有 ret，不应再展开一次
        // 自己那层的 finally（会造成无限递归）。
        for (size_t i = _ehStack.size(); i > depth; --i)        {
            const EhFrame& frame = _ehStack[i - 1];
            if (frame.finallyBody && !_emittingFinally)            {
                _emittingFinally = true;
                pushScope();
                for (const auto& e : frame.finallyBody->getBodyExpressions()) emitStatement(e);
                popScope();
                _emittingFinally = false;
            }
            emitEhPop(frame.offset);
        }
    }
    void CodeGen::emitThrow(const std::shared_ptr<ast::ExpressionNode>& node)    {
        const auto thr = std::dynamic_pointer_cast<ast::ThrowNode>(node);
        if (!thr)        {
            fail("Not a throw node: " + node->toString());
            return;
        }
        // 先求值异常值，再交给 unwinder。throw 是终结性语句：
        // core_eh_throw 不会返回（要么长跳到 handler，要么 abort），
        // 所以后面不需要再发任何指令。
        if (thr->getThrowExpression())        {
            emitExpression(thr->getThrowExpression());
        }
 else        {
            _asm.movImm(0, "%rax");
        }
        _asm.mov("%rax", "%rcx");
        _asm.subRsp(32);
        _asm.call("core_eh_throw");
        _asm.addRsp(32);
        // 理论上不可达；留着让汇编器/调试器有个落点。
        _asm.inst("ud2");
    }
    void CodeGen::emitTry(const std::shared_ptr<ast::ExpressionNode>& node)    {
        const auto tryNode = std::dynamic_pointer_cast<ast::TryNode>(node);
        if (!tryNode)        {
            fail("Not a try node: " + node->toString());
            return;
        }
        const auto catchBodies = tryNode->getCatchBodies();
        const auto finallyBody = tryNode->getFinallyBody();
        const bool hasCatch = !catchBodies.empty();
        const bool hasFinally = static_cast<bool>(finallyBody);

        // 帧槽位：48 字节，由 allocateSlot 推进 _frameSize，
        // 这样 emitFunctionBody 末尾的帧回填会把它算进去。
        // allocateSlot 返回的是**帧块起始**的负偏移；块内字段依次 +0/+8/+16/+24/+32。
        const std::string frameName = "__p7eh_" + std::to_string(_ehCounter++);
        const long long frameOffset = allocateSlot(frameName, sema::getPtrType(), 48);
        // 拼一个 "N(%rbp)" 出来，N 可以是帧块基址或块内某个字段。
        const auto field = [frameOffset](long long delta)
        {
            return std::to_string(frameOffset + delta) + "(%rbp)";
        };

        const std::string resumeLabel = _asm.newLabel("eh_resume");
        const std::string endLabel = _asm.newLabel("eh_end");

        // ---- 建立帧 ----
        // prev 由 core_eh_push 填；rbp/rsp 保存当前值；resume 存标签地址。
        _asm.lea(field(0), "%rcx");
        _asm.subRsp(32);
        _asm.call("core_eh_push");
        _asm.addRsp(32);
        _asm.mov("%rbp", field(8));                 // saved rbp
        _asm.mov("%rsp", field(16));                // saved rsp
        _asm.lea(resumeLabel + "(%rip)", "%rax");
        _asm.mov("%rax", field(24));                // resume

        _ehStack.push_back({frameOffset, finallyBody});
        // 本 try 之外的活动帧数。两条路径（正常 / 异常落地）都要把编译期
        // 帧栈恢复到这一深度 —— 它们各自发一次 core_eh_pop，但 _ehStack
        // 只能被弹一次，否则后面嵌套的 try 会算错深度。
        const size_t outerDepth = _ehStack.size() - 1;

        // ---- try 体 ----
        pushScope();
        if (tryNode->getTryBody())        {
            for (const auto& e : tryNode->getTryBody()->getBodyExpressions()) emitStatement(e);
        }
        popScope();

        // 正常走完：先摘掉自己的帧，再跑 finally。
        emitEhPop(frameOffset);
        _ehStack.resize(outerDepth);
        if (hasFinally)        {
            pushScope();
            for (const auto& e : finallyBody->getBodyExpressions()) emitStatement(e);
            popScope();
        }
        _asm.jmp(endLabel);

        // ---- 异常落地 ----
        // 到这里时 core_eh_throw 已经把 rsp/rbp 恢复成进入 try 时的值，
        // 且链顶仍指向本帧（unwinder 弹出到本帧为止但不越过它）。
        _asm.label(resumeLabel);
        // 先摘链再进 handler：handler 里再 throw 时不该看到自己这一帧。
        emitEhPop(frameOffset);
        _ehStack.resize(outerDepth);
        // 异常值留在本帧的 exc 槽里（摘链不影响数据），读进 %rax。
        _asm.mov(field(32), "%rax");

        if (hasCatch)        {
            // Rio 的 catch 只有 `catch(e)` 一种形式，没有类型匹配，
            // 所以命中第一个分支即可（多写几个 catch 时后面的永远不生效）。
            const auto& first = catchBodies.front();
            pushScope();
            if (first.first)            {
                // 异常值在 P7 里是**字符串**（throw "msg" 是唯一的实际用法，
                // 也是 test_11 / test_12 的写法），所以 catch 变量按 str 绑定。
                // 局限：`throw 42` 这类非字符串值不会被静态拒绝，但 catch
                // 侧按 str 解释（rt_print_str 会读长度头）—— 见 docs/p7_plan.md。
                const long long slot = allocateSlot(first.first->getName(), sema::getStrType(), 8);
                _asm.mov("%rax", std::to_string(slot) + "(%rbp)");
            }
            for (const auto& e : first.second->getBodyExpressions()) emitStatement(e);
            popScope();
        }

        // finally 在正常路径和 catch 路径上都要跑。
        if (hasFinally)        {
            pushScope();
            for (const auto& e : finallyBody->getBodyExpressions()) emitStatement(e);
            popScope();
        }

        // 没有 catch：异常必须继续往外传。**finally 已经跑过了**，
        // 此时本帧已摘链，重新 throw 就是交给外层 handler。
        //
        // 注意异常值要**重新从帧槽读**：finally 体是任意代码，一定会把
        // %rax 冲掉，不能指望落地时读进 %rax 的值还活着。
        if (!hasCatch)        {
            _asm.mov(field(32), "%rcx");
            _asm.subRsp(32);
            _asm.call("core_eh_throw");
            _asm.addRsp(32);
            _asm.inst("ud2");
        }
        _asm.label(endLabel);
    }
    // ============================================================
    // P5：容器（list / dict）代码生成
    // ============================================================
    //
    // 运行时布局（与 docs/architecture_plan.md §4.2 的 VecHeader 一致）：
    //   ListHeader { i64 len, cap, rc, elem_size; u8 data[] }  32 字节头，data 从 +32 起
    //   DictHeader { i64 len, cap, rc; ptr keys; ptr vals }    40 字节头
    // 两者都是**单指针**（和 str 一样），元素槽恒为 8 字节。
    //
    // 这里刻意不做哈希、不做扩容：P5 只要求语义正确，查找用线性扫描。
    // 布局留好了 cap 字段，后续加哈希/append 时不用改 ABI。
    //
    // 中间值一律走**栈上的临时槽**（allocateTemp），不用 push/pop：
    // push 会让 rsp 变成 8 字节非对齐状态，中途一旦 call 就违反 MS ABI。
    void CodeGen::emitAlloc(size_t size)    {
        _asm.movImm(static_cast<long long>(size), "%rcx");
        _asm.movImm(8, "%rdx");
        _asm.subRsp(32);
        _asm.call("core_alloc");
        _asm.addRsp(32);
    }
    std::string CodeGen::allocateTemp(const sema::TypePtr& type)    {
        // 槽位名带前缀与递增计数，不会撞上用户标识符；
        // 走 allocateSlot 是为了让 _frameSize 正确推进，帧回填才会覆盖到它。
        const std::string name = "__p5tmp_" + std::to_string(_tempCounter++);
        const auto t = (type && type->getKind() != sema::TypeKind::TK_VOID)
                           ? type
                           : sema::getIntType();
        const long long offset = allocateSlot(name, t, 8);
        return std::to_string(offset) + "(%rbp)";
    }
    void CodeGen::flattenListElements(const std::shared_ptr<ast::ExpressionNode>& body,
                                      std::vector<std::shared_ptr<ast::ExpressionNode>>& out) const    {
        if (!body) return;
        // 逗号连接的中缀链（PARALLEL）：先按逗号劈开
        if (const auto infix = std::dynamic_pointer_cast<ast::InfixExpressionNode>(body))        {
            if (infix->getOpToken().getType() == core::TokenType::TOKEN_COMMA)            {
                flattenListElements(infix->getLeftNode(), out);
                flattenListElements(infix->getRightNode(), out);
                return;
            }
        }
        if (const auto paren = std::dynamic_pointer_cast<ast::ParenRangerNode>(body))        {
            flattenListElements(paren->getRangerNode(), out);
            return;
        }
        if (const auto block = std::dynamic_pointer_cast<ast::BlockRangerNode>(body))        {
            for (const auto& e : block->getBodyExpressions()) flattenListElements(e, out);
            return;
        }
        // 注意：DictionaryExpressionNode / ListExpressionNode 是 RangerNode 的子类，
        // 但**不是** BlockRangerNode，所以嵌套字面量会走到这里被当成一个元素。
        out.push_back(body);
    }
    void CodeGen::flattenDictPairs(const std::shared_ptr<ast::ExpressionNode>& body,
                                   std::vector<std::pair<std::shared_ptr<ast::ExpressionNode>,
                                                         std::shared_ptr<ast::ExpressionNode>>>& out) const    {
        if (!body) return;
        if (const auto infix = std::dynamic_pointer_cast<ast::InfixExpressionNode>(body))        {
            const auto t = infix->getOpToken().getType();
            // 逗号优先级低于冒号，所以先劈逗号，叶子才是「键: 值」
            if (t == core::TokenType::TOKEN_COMMA)            {
                flattenDictPairs(infix->getLeftNode(), out);
                flattenDictPairs(infix->getRightNode(), out);
                return;
            }
            if (t == core::TokenType::TOKEN_COLON)            {
                out.emplace_back(infix->getLeftNode(), infix->getRightNode());
                return;
            }
        }
        // 防御：某些优先级组合可能造出 PairExpressionNode
        if (const auto pair = std::dynamic_pointer_cast<ast::PairExpressionNode>(body))        {
            out.emplace_back(pair->getLeftNode(), pair->getRightNode());
            return;
        }
        if (const auto paren = std::dynamic_pointer_cast<ast::ParenRangerNode>(body))        {
            flattenDictPairs(paren->getRangerNode(), out);
            return;
        }
        if (const auto block = std::dynamic_pointer_cast<ast::BlockRangerNode>(body))        {
            for (const auto& e : block->getBodyExpressions()) flattenDictPairs(e, out);
        }
    }
    std::shared_ptr<ast::ExpressionNode>
    CodeGen::unwrapIndexKey(const std::shared_ptr<ast::ExpressionNode>& indexNode)    {
        if (!indexNode) return nullptr;
        // parser 造的是 IndexExpressionNode(token, left, BracketExpressionNode(body))，
        // 真正的键在方括号节点的 body 里。
        if (const auto bracket = std::dynamic_pointer_cast<ast::BracketExpressionNode>(indexNode))        {
            return bracket->getBodyNode();
        }
        return indexNode;
    }
    void CodeGen::emitListExpression(const std::shared_ptr<ast::ExpressionNode>& node)    {
        const auto list = std::dynamic_pointer_cast<ast::ListExpressionNode>(node);
        if (!list)        {
            fail("Not a list literal: " + node->toString());
            return;
        }
        std::vector<std::shared_ptr<ast::ExpressionNode>> elements;
        flattenListElements(list->getBodyNode(), elements);
        const size_t n = elements.size();
        // 先把对象整块分配好（头部 + n 个元素槽），再逐个求值填进去。
        // 顺序很重要：求值元素时可能再分配（嵌套字面量），
        // 若先求值后分配，中间结果就得跨分配保存。
        const std::string headerSlot = allocateTemp(inferListLiteralType(node));
        emitAlloc(32 + n * 8);
        _asm.mov("%rax", headerSlot);
        _asm.mov(headerSlot, "%rcx");
        _asm.movImm(static_cast<long long>(n), "%rax");
        _asm.mov("%rax", "0(%rcx)");
        // len
        _asm.mov("%rax", "8(%rcx)");
        // cap（暂不扩容，等于 len）
        _asm.movImm(1, "%rax");
        _asm.mov("%rax", "16(%rcx)");
        // rc
        _asm.movImm(8, "%rax");
        _asm.mov("%rax", "24(%rcx)");
        // elem_size
        for (size_t i = 0; i < n; ++i)        {
            emitExpression(elements[i]);
            if (hasFailed()) return;
            _asm.mov(headerSlot, "%rcx");
            const std::string dest = std::to_string(32 + i * 8) + "(%rcx)";
            const auto et = inferType(elements[i]);
            if (et && et->isFloat())            {
                // 浮点元素按位存 8 字节，读取时再按静态类型解释
                _asm.inst("movsd", "%xmm0", dest);
            }
  else            {
                _asm.mov("%rax", dest);
            }
        }
        _asm.mov(headerSlot, "%rax");
    }
    void CodeGen::emitDictionaryExpression(const std::shared_ptr<ast::ExpressionNode>& node)    {
        const auto dict = std::dynamic_pointer_cast<ast::DictionaryExpressionNode>(node);
        if (!dict)        {
            fail("Not a dictionary literal: " + node->toString());
            return;
        }
        std::vector<std::pair<std::shared_ptr<ast::ExpressionNode>,
                              std::shared_ptr<ast::ExpressionNode>>> pairs;
        flattenDictPairs(dict->getBodyNode(), pairs);
        const size_t n = pairs.size();
        const std::string headerSlot = allocateTemp(inferDictLiteralType(node));
        emitAlloc(40);
        _asm.mov("%rax", headerSlot);
        _asm.mov(headerSlot, "%rcx");
        _asm.movImm(static_cast<long long>(n), "%rax");
        _asm.mov("%rax", "0(%rcx)");
        // len
        _asm.mov("%rax", "8(%rcx)");
        // cap
        _asm.movImm(1, "%rax");
        _asm.mov("%rax", "16(%rcx)");
        // rc
        if (n == 0)        {
            // 空字典：keys/vals 都是 null，长度 0
            _asm.movImm(0, "%rax");
            _asm.mov("%rax", "24(%rcx)");
            _asm.mov("%rax", "32(%rcx)");
            _asm.mov(headerSlot, "%rax");
            return;
        }
        const std::string keysSlot = allocateTemp(sema::getPtrType());
        const std::string valsSlot = allocateTemp(sema::getPtrType());
        emitAlloc(n * 8);
        _asm.mov("%rax", keysSlot);
        emitAlloc(n * 8);
        _asm.mov("%rax", valsSlot);
        _asm.mov(headerSlot, "%rcx");
        _asm.mov(keysSlot, "%rax");
        _asm.mov("%rax", "24(%rcx)");
        _asm.mov(valsSlot, "%rax");
        _asm.mov("%rax", "32(%rcx)");
        for (size_t i = 0; i < n; ++i)        {
            const std::string idx = std::to_string(i * 8) + "(%rcx)";
            emitExpression(pairs[i].first);
            if (hasFailed()) return;
            _asm.mov(keysSlot, "%rcx");
            const auto kt = inferType(pairs[i].first);
            if (kt && kt->isFloat())            {
                _asm.inst("movsd", "%xmm0", idx);
            }
  else            {
                _asm.mov("%rax", idx);
            }
            emitExpression(pairs[i].second);
            if (hasFailed()) return;
            _asm.mov(valsSlot, "%rcx");
            const auto vt = inferType(pairs[i].second);
            if (vt && vt->isFloat())            {
                _asm.inst("movsd", "%xmm0", idx);
            }
  else            {
                _asm.mov("%rax", idx);
            }
        }
        _asm.mov(headerSlot, "%rax");
    }
    // ============================================================
    // P12/G2：元素写 `a[i] = v` / `d[k] = v`
    // ============================================================
    //
    // 为什么这是「恢复回归」而不是新特性：原仓库 v1.21.2 实测支持
    // `li[0] = 0` / `li[1] += 1` / `li += [4]`。原实现能成立是因为 RA VM
    // 的 `ITER_GET` 返回指向容器内元素的 shared_ptr —— 改临时变量就等于
    // 改容器。新架构把元素改成数据区内联的裸值，那条隐含通路没了，
    // 必须显式计算元素地址再写。
    //
    // 布局见本文件 P5 段落：ListHeader 的 data 从 +32 起、元素恒 8 字节；
    // DictHeader 是 {len,cap,rc,keys,vals}，keys/vals 各是独立数组。
    bool CodeGen::emitIndexAssign(const std::shared_ptr<ast::IndexExpressionNode>& target,
                                  const std::shared_ptr<ast::ExpressionNode>& value)    {
        // 值已经在 %rax（浮点则 %xmm0）。value 只用于取静态类型 —— 调用方
        // 必须已经**求过值**，这里不会重新求值（否则会重复副作用）。
        const auto valType = inferType(value);
        return emitIndexAssignValue(target, valType);
    }
    bool CodeGen::emitIndexAssignValue(const std::shared_ptr<ast::IndexExpressionNode>& target,
                                       const sema::TypePtr& valType)    {
        const auto cont = target->getLeftNode();
        const auto key = unwrapIndexKey(target->getIndexNode());
        if (!cont || !key) return false;
        const auto contType = inferType(cont);
        if (!contType) return false;
        // 值已经在 %rax（浮点则 %xmm0），先落到临时槽 —— 求容器地址的
        // 过程中会用到 caller-saved 寄存器。
        const std::string valSlot = allocateTemp(valType);
        const bool valIsFloat = valType && valType->isFloat();
        if (valIsFloat)        {
            _asm.inst("movsd", "%xmm0", valSlot);
        }
 else        {
            _asm.mov("%rax", valSlot);
        }
        if (contType->getKind() == sema::TypeKind::TK_LIST)        {
            const auto listType = std::static_pointer_cast<sema::ListType>(contType);
            const auto elemType = listType->getElemType();
            // Q4：元素类型必须匹配 elem_size（元素槽恒 8 字节）
            const auto why = checkAssignable(
                (elemType && elemType->getKind() != sema::TypeKind::TK_UNKNOWN)
                    ? elemType
                    : sema::getIntType(),
                valType);
            if (!why.empty())            {
                failAt(target->getPos(), "Assignment to list element: " + why);
                return true;
            }
            const std::string contSlot = allocateTemp(contType);
            emitExpression(cont);
            if (hasFailed()) return true;
            _asm.mov("%rax", contSlot);
            emitExpression(key);
            if (hasFailed()) return true;
            // 越界直接跳过（与原仓库抛 RangeError 不同，但新架构的 list 读
            // 已定为返回 0，写保持同一套「不越界写内存」的口径）。
            const std::string skipLabel = _asm.newLabel("idxset_skip");
            const std::string doneLabel = _asm.newLabel("idxset_done");
            _asm.mov(contSlot, "%rcx");
            _asm.cmp("(%rcx)", "%rax");
            _asm.inst("jae", skipLabel);
            _asm.mov(valSlot, "%rdx");
            _asm.inst("movq", "%rdx", "32(%rcx,%rax,8)");
            _asm.jmp(doneLabel);
            _asm.label(skipLabel);
            _asm.label(doneLabel);
            _asm.mov(valSlot, "%rax");
            if (valIsFloat) _asm.inst("movq", "%rax", "%xmm0");
            return true;
        }
        if (contType->getKind() == sema::TypeKind::TK_DICT)        {
            const auto dictType = std::static_pointer_cast<sema::DictType>(contType);
            const auto keyType = dictType->getKeyType();
            const bool keyIsStr = keyType && keyType->getKind() == sema::TypeKind::TK_STR;
            const std::string contSlot = allocateTemp(contType);
            emitExpression(cont);
            if (hasFailed()) return true;
            _asm.mov("%rax", contSlot);
            const std::string keySlot = allocateTemp(keyIsStr ? sema::getStrType() : sema::getIntType());
            emitExpression(key);
            if (hasFailed()) return true;
            _asm.mov("%rax", keySlot);
            const std::string iSlot = allocateTemp(sema::getIntType());
            _asm.movImm(0, "%rax");
            _asm.mov("%rax", iSlot);
            const std::string loopLabel = _asm.newLabel("idxset_loop");
            const std::string nextLabel = _asm.newLabel("idxset_next");
            const std::string foundLabel = _asm.newLabel("idxset_found");
            const std::string growLabel = _asm.newLabel("idxset_grow");
            const std::string doneLabel = _asm.newLabel("idxset_done");
            _asm.label(loopLabel);
            // 循环状态全放栈槽：core_mem_eq 会按 MS ABI 破坏 caller-saved 寄存器。
            _asm.mov(iSlot, "%rax");
            _asm.mov(contSlot, "%rcx");
            _asm.cmp("(%rcx)", "%rax");
            _asm.inst("jae", growLabel);
            _asm.inst("movq", "24(%rcx)", "%rdx");
            // keys[i]
            _asm.inst("movq", "(%rdx,%rax,8)", "%rdx");
            _asm.mov(keySlot, "%r8");
            if (keyIsStr)            {
                // 字符串键按**内容**比较：先比长度，再 mem_eq
                _asm.inst("movq", "(%rdx)", "%r9");
                // 已存键的长度
                _asm.inst("movq", "(%r8)", "%r10");
                // 查找键的长度
                _asm.cmp("%r10", "%r9");
                _asm.jne(nextLabel);
                // 真实字节从 +8 起（头 8 字节是长度）
                _asm.lea("8(%rdx)", "%rcx");
                _asm.lea("8(%r8)", "%rdx");
                _asm.mov("%r9", "%r8");
                _asm.subRsp(32);
                _asm.call("core_mem_eq");
                _asm.addRsp(32);
                _asm.inst("testl", "%eax", "%eax");
                _asm.je(nextLabel);
            }
 else            {
                _asm.cmp("%r8", "%rdx");
                _asm.jne(nextLabel);
            }
            _asm.jmp(foundLabel);
            _asm.label(nextLabel);
            _asm.mov(iSlot, "%rax");
            _asm.inst("addq", "$1", "%rax");
            _asm.mov("%rax", iSlot);
            _asm.jmp(loopLabel);
            // ---- 命中：写 vals[i] ----
            _asm.label(foundLabel);
            _asm.mov(contSlot, "%rcx");
            _asm.mov(iSlot, "%rax");
            _asm.inst("movq", "32(%rcx)", "%rdx");
            _asm.mov(valSlot, "%r8");
            _asm.inst("movq", "%r8", "(%rdx,%rax,8)");
            _asm.jmp(doneLabel);
            // ---- 未命中：Q3 定案「新增键值对」----
            // keys / vals 各重新分配成 len+1 个槽，拷回旧内容，追加新键值对。
            // 不做 cap 倍增：P5 的字典本来就是线性扫描、不做哈希，语义优先。
            _asm.label(growLabel);
            _asm.mov(contSlot, "%rcx");
            _asm.inst("movq", "(%rcx)", "%rax");
            // %rax = 旧 len
            _asm.mov("%rax", iSlot);
            // i = 旧 len（新元素下标）
            _asm.inst("addq", "$1", "%rax");
            _asm.mov(contSlot, "%rcx");
            _asm.mov("%rax", "(%rcx)");
            // header.len = len + 1
            _asm.mov("%rax", "8(%rcx)");
            // header.cap = len + 1
            // 新 keys 数组：(len+1) * 8 字节
            const std::string newKeysSlot = allocateTemp(sema::getPtrType());
            const std::string newValsSlot = allocateTemp(sema::getPtrType());
            _asm.movImm(0, "%rax");
            _asm.mov(iSlot, "%rax");
            _asm.inst("addq", "$1", "%rax");
            _asm.inst("leaq", "(,%rax,8)", "%rcx");
            _asm.subRsp(32);
            _asm.call("core_alloc");
            _asm.addRsp(32);
            _asm.mov("%rax", newKeysSlot);
            _asm.movImm(0, "%rax");
            _asm.mov(iSlot, "%rax");
            _asm.inst("addq", "$1", "%rax");
            _asm.inst("leaq", "(,%rax,8)", "%rcx");
            _asm.subRsp(32);
            _asm.call("core_alloc");
            _asm.addRsp(32);
            _asm.mov("%rax", newValsSlot);
            // 拷贝旧内容：长度 = 旧 len（存在 iSlot）
            const std::string jSlot = allocateTemp(sema::getIntType());
            const std::string copyLoop = _asm.newLabel("idxset_copy");
            const std::string copyDone = _asm.newLabel("idxset_copydone");
            _asm.movImm(0, "%rax");
            _asm.mov("%rax", jSlot);
            _asm.label(copyLoop);
            _asm.mov(jSlot, "%rax");
            _asm.mov(iSlot, "%rcx");
            _asm.cmp("%rcx", "%rax");
            _asm.inst("jae", copyDone);
            _asm.mov(contSlot, "%rcx");
            _asm.inst("movq", "24(%rcx)", "%rdx");
            _asm.inst("movq", "(%rdx,%rax,8)", "%rdx");
            _asm.mov(newKeysSlot, "%r8");
            _asm.inst("movq", "%rdx", "(%r8,%rax,8)");
            _asm.mov(contSlot, "%rcx");
            _asm.inst("movq", "32(%rcx)", "%rdx");
            _asm.inst("movq", "(%rdx,%rax,8)", "%rdx");
            _asm.mov(newValsSlot, "%r8");
            _asm.inst("movq", "%rdx", "(%r8,%rax,8)");
            _asm.mov(jSlot, "%rax");
            _asm.inst("addq", "$1", "%rax");
            _asm.mov("%rax", jSlot);
            _asm.jmp(copyLoop);
            _asm.label(copyDone);
            // header.keys / header.vals 指向新数组
            _asm.mov(contSlot, "%rcx");
            _asm.mov(newKeysSlot, "%rax");
            _asm.inst("movq", "%rax", "24(%rcx)");
            _asm.mov(newValsSlot, "%rax");
            _asm.inst("movq", "%rax", "32(%rcx)");
            // 追加到下标 = 旧 len
            _asm.mov(iSlot, "%rax");
            _asm.mov(contSlot, "%rcx");
            _asm.inst("movq", "24(%rcx)", "%rdx");
            _asm.mov(keySlot, "%r8");
            _asm.inst("movq", "%r8", "(%rdx,%rax,8)");
            _asm.inst("movq", "32(%rcx)", "%rdx");
            _asm.mov(valSlot, "%r8");
            _asm.inst("movq", "%r8", "(%rdx,%rax,8)");
            _asm.label(doneLabel);
            _asm.mov(valSlot, "%rax");
            if (valIsFloat) _asm.inst("movq", "%rax", "%xmm0");
            return true;
        }        return false;
    }
    // ============================================================
    // P12/G3 + G4：字符串 / 列表拼接
    // ============================================================
    //
    // `+` 在旧后端是 RA 虚拟机的动态加法；到 x86-64 后端变成裸 addq，
    // 而 str / list 都是**指针**，addq 出来的是垃圾（原仓库 str.rio:111 的
    // `[""] + result + ""`、lowlevel.rio:53 的 `i + ": "` 都依赖拼接）。
    //
    // 这里直接内联生成，不调 rt.rio 的辅助函数：core 只提供 mem_copy /
    // mem_set / mem_eq / alloc，没有「读 8 字节」这种入口，用 Rio 写不出来；
    // 为此改 core 的 ABI 不值得（core.rio 是三层之间的冻结契约）。
    void CodeGen::emitStrConcat(const std::string& lhsSlot, const std::string& rhsSlot)    {
        // 结果：新 str = { len_a+len_b, 字节A, 字节B }
        // 长度头在偏移 0，真实字节从 +8 起（与 rt_write_str 的约定一致）。
        const std::string laSlot = allocateTemp(sema::getIntType());
        const std::string lbSlot = allocateTemp(sema::getIntType());
        _asm.mov(lhsSlot, "%rax");
        _asm.inst("movq", "(%rax)", "%rax");
        // la
        _asm.mov("%rax", laSlot);
        _asm.mov(rhsSlot, "%rax");
        _asm.inst("movq", "(%rax)", "%rax");
        // lb
        _asm.mov("%rax", lbSlot);
        const std::string bufSlot = allocateTemp(sema::getStrType());
        _asm.mov(laSlot, "%rcx");
        _asm.mov(lbSlot, "%rax");
        _asm.inst("addq", "%rax", "%rcx");
        // total = la + lb
        _asm.inst("addq", "$8", "%rcx");
        // + 长度头
        _asm.movImm(8, "%rdx");
        _asm.subRsp(32);
        _asm.call("core_alloc");
        _asm.addRsp(32);
        _asm.mov("%rax", bufSlot);
        // 写长度头
        _asm.mov(laSlot, "%rcx");
        _asm.mov(lbSlot, "%rdx");
        _asm.inst("addq", "%rdx", "%rcx");
        _asm.mov("%rcx", "(%rax)");
        // mem_copy(buf+8, a+8, la)
        _asm.mov("%rax", "%rcx");
        _asm.inst("addq", "$8", "%rcx");
        _asm.mov(lhsSlot, "%rdx");
        _asm.inst("addq", "$8", "%rdx");
        _asm.mov(laSlot, "%r8");
        _asm.subRsp(32);
        _asm.call("core_mem_copy");
        _asm.addRsp(32);
        // mem_copy(buf+8+la, b+8, lb)
        _asm.mov(bufSlot, "%rcx");
        _asm.inst("addq", "$8", "%rcx");
        _asm.mov(laSlot, "%rax");
        _asm.inst("addq", "%rax", "%rcx");
        _asm.mov(rhsSlot, "%rdx");
        _asm.inst("addq", "$8", "%rdx");
        _asm.mov(lbSlot, "%r8");
        _asm.subRsp(32);
        _asm.call("core_mem_copy");
        _asm.addRsp(32);
        _asm.mov(bufSlot, "%rax");
    }
    void CodeGen::emitListConcat(const std::string& lhsSlot, const std::string& rhsSlot)    {
        // ListHeader { i64 len, cap, rc, elem_size; u8 data[] }，data 从 +32 起，
        // 元素槽恒 8 字节（见本文件 P5 段）。
        const std::string laSlot = allocateTemp(sema::getIntType());
        const std::string lbSlot = allocateTemp(sema::getIntType());
        _asm.mov(lhsSlot, "%rax");
        _asm.inst("movq", "(%rax)", "%rax");
        _asm.mov("%rax", laSlot);
        _asm.mov(rhsSlot, "%rax");
        _asm.inst("movq", "(%rax)", "%rax");
        _asm.mov("%rax", lbSlot);
        const std::string bufSlot = allocateTemp(sema::getPtrType());
        _asm.mov(laSlot, "%rcx");
        _asm.mov(lbSlot, "%rax");
        _asm.inst("addq", "%rax", "%rcx");
        // total
        _asm.inst("leaq", "32(,%rcx,8)", "%rcx");
        // 32 + total*8
        _asm.movImm(8, "%rdx");
        _asm.subRsp(32);
        _asm.call("core_alloc");
        _asm.addRsp(32);
        _asm.mov("%rax", bufSlot);
        // len / cap / rc / elem_size
        _asm.mov(laSlot, "%rcx");
        _asm.mov(lbSlot, "%rdx");
        _asm.inst("addq", "%rdx", "%rcx");
        _asm.mov("%rcx", "(%rax)");
        // len
        _asm.mov("%rcx", "8(%rax)");
        // cap
        _asm.movImm(1, "%rcx");
        _asm.mov("%rcx", "16(%rax)");
        // rc
        _asm.movImm(8, "%rcx");
        _asm.mov("%rcx", "24(%rax)");
        // elem_size
        // mem_copy(buf+32, a+32, la*8)
        _asm.mov(bufSlot, "%rcx");
        _asm.inst("addq", "$32", "%rcx");
        _asm.mov(lhsSlot, "%rdx");
        _asm.inst("addq", "$32", "%rdx");
        _asm.mov(laSlot, "%r8");
        _asm.inst("shlq", "$3", "%r8");
        _asm.subRsp(32);
        _asm.call("core_mem_copy");
        _asm.addRsp(32);
        // mem_copy(buf+32+la*8, b+32, lb*8)
        _asm.mov(bufSlot, "%rcx");
        _asm.inst("addq", "$32", "%rcx");
        _asm.mov(laSlot, "%rax");
        _asm.inst("leaq", "(%rcx,%rax,8)", "%rcx");
        _asm.mov(rhsSlot, "%rdx");
        _asm.inst("addq", "$32", "%rdx");
        _asm.mov(lbSlot, "%r8");
        _asm.inst("shlq", "$3", "%r8");
        _asm.subRsp(32);
        _asm.call("core_mem_copy");
        _asm.addRsp(32);
        _asm.mov(bufSlot, "%rax");
    }
    void CodeGen::emitStrEq(const std::string& lhsSlot, const std::string& rhsSlot)    {
        // 先比长度头，再比内容字节。两段都可能为 null（空串/未初始化），
        // core_mem_eq 对 n=0 直接返回 true，长度不等时提前退出即可。
        const std::string laSlot = allocateTemp(sema::getIntType());
        const std::string lbSlot = allocateTemp(sema::getIntType());
        _asm.mov(lhsSlot, "%rax");
        _asm.inst("movq", "(%rax)", "%rax");
        _asm.mov("%rax", laSlot);
        _asm.mov(rhsSlot, "%rax");
        _asm.inst("movq", "(%rax)", "%rax");
        _asm.mov("%rax", lbSlot);
        const std::string neLabel = _asm.newLabel("streq_ne");
        const std::string doneLabel = _asm.newLabel("streq_done");
        _asm.mov(laSlot, "%rcx");
        _asm.mov(lbSlot, "%rax");
        _asm.cmp("%rax", "%rcx");
        _asm.jne(neLabel);
        // 长度相等：比较内容（从 +8 起）
        _asm.mov(lhsSlot, "%rcx");
        _asm.inst("addq", "$8", "%rcx");
        _asm.mov(rhsSlot, "%rdx");
        _asm.inst("addq", "$8", "%rdx");
        _asm.mov(laSlot, "%r8");
        _asm.subRsp(32);
        _asm.call("core_mem_eq");
        _asm.addRsp(32);
        _asm.inst("movzbq", "%al", "%rax");
        _asm.jmp(doneLabel);
        _asm.label(neLabel);
        _asm.movImm(0, "%rax");
        _asm.label(doneLabel);
    }
    void CodeGen::emitIndexExpression(const std::shared_ptr<ast::ExpressionNode>& node)    {        const auto index = std::dynamic_pointer_cast<ast::IndexExpressionNode>(node);
        if (!index)        {
            fail("Not an index expression: " + node->toString());
            return;
        }
        const auto target = index->getLeftNode();
        const auto key = unwrapIndexKey(index->getIndexNode());
        if (!target || !key)        {
            fail("Malformed index expression: " + node->toString());
            return;
        }
        const auto targetType = inferType(target);
        const auto resultType = inferIndexResultType(index);
        // 容器指针先落到临时槽：求值键的过程中所有 caller-saved 寄存器都会被用掉
        const std::string contSlot = allocateTemp(targetType);
        emitExpression(target);
        if (hasFailed()) return;
        _asm.mov("%rax", contSlot);
        if (targetType && targetType->getKind() == sema::TypeKind::TK_LIST)        {
            emitExpression(key);
            if (hasFailed()) return;
            const std::string missLabel = _asm.newLabel("idx_miss");
            const std::string doneLabel = _asm.newLabel("idx_done");
            _asm.mov(contSlot, "%rcx");
            // 越界/负下标一律返回 0：cmpq 之后 jae 是**无符号**比较，
            // 负下标会变成巨大无符号数，同样被拦下。
            _asm.cmp("(%rcx)", "%rax");
            _asm.inst("jae", missLabel);
            _asm.inst("movq", "32(%rcx,%rax,8)", "%rax");
            _asm.jmp(doneLabel);
            _asm.label(missLabel);
            _asm.movImm(0, "%rax");
            _asm.label(doneLabel);
            if (resultType && resultType->isFloat())            {
                _asm.inst("movq", "%rax", "%xmm0");
            }
            return;
        }
        // P12/G10：`s[i]` 读取。str 的表示是「指向长度头的指针」，
        // 长度在偏移 0，真实字节从 +8 起。返回 char（0..255，int 族）。
        // 越界返回 0，与上面 list 分支同口径。
        // 注意：按**字节**索引，多字节 UTF-8 字符会被拆开 —— 与原仓库一致。
        if (targetType && targetType->getKind() == sema::TypeKind::TK_STR)        {
            emitExpression(key);
            if (hasFailed()) return;
            const std::string missLabel = _asm.newLabel("stridx_miss");
            const std::string doneLabel = _asm.newLabel("stridx_done");
            _asm.mov(contSlot, "%rcx");
            _asm.cmp("(%rcx)", "%rax");
            _asm.inst("jae", missLabel);
            _asm.inst("movzbq", "8(%rcx,%rax)", "%rax");
            _asm.jmp(doneLabel);
            _asm.label(missLabel);
            _asm.movImm(0, "%rax");
            _asm.label(doneLabel);
            return;
        }
        if (targetType && targetType->getKind() == sema::TypeKind::TK_DICT)        {
            const auto dictType = std::static_pointer_cast<sema::DictType>(targetType);
            const auto keyType = dictType->getKeyType();
            const bool keyIsStr = keyType && keyType->getKind() == sema::TypeKind::TK_STR;
            const std::string keySlot = allocateTemp(keyIsStr ? sema::getStrType() : sema::getIntType());
            const std::string iSlot = allocateTemp(sema::getIntType());
            emitExpression(key);
            if (hasFailed()) return;
            _asm.mov("%rax", keySlot);
            _asm.movImm(0, "%rax");
            _asm.mov("%rax", iSlot);
            const std::string loopLabel = _asm.newLabel("dict_loop");
            const std::string nextLabel = _asm.newLabel("dict_next");
            const std::string missLabel = _asm.newLabel("dict_miss");
            const std::string doneLabel = _asm.newLabel("dict_done");
            _asm.label(loopLabel);
            // 循环状态全放栈槽，每次迭代重新装载：
            // core_mem_eq 会按 MS ABI 破坏所有 caller-saved 寄存器。
            _asm.mov(iSlot, "%rax");
            _asm.mov(contSlot, "%rcx");
            _asm.cmp("(%rcx)", "%rax");
            _asm.inst("jae", missLabel);
            _asm.inst("movq", "24(%rcx)", "%rdx");
            // keys 数组基址
            _asm.inst("movq", "(%rdx,%rax,8)", "%rdx");
            // keys[i]
            _asm.mov(keySlot, "%r8");
            if (keyIsStr)            {
                // 字符串键按**内容**比较：先比长度，再 mem_eq
                _asm.inst("movq", "(%rdx)", "%r9");
                // 已存键的长度
                _asm.inst("movq", "(%r8)", "%r10");
                // 查找键的长度
                _asm.cmp("%r10", "%r9");
                _asm.jne(nextLabel);
                // 真实字节从 +8 起（头 8 字节是长度），不能直接传头指针：
                // 头指针指向的是 len 字段，比出来的永远是「长度相等」。
                _asm.lea("8(%rdx)", "%rcx");
                _asm.lea("8(%r8)", "%rdx");
                _asm.mov("%r9", "%r8");
                _asm.subRsp(32);
                _asm.call("core_mem_eq");
                _asm.addRsp(32);
                _asm.inst("testl", "%eax", "%eax");
                _asm.je(nextLabel);
            }
  else            {
                _asm.cmp("%r8", "%rdx");
                _asm.jne(nextLabel);
            }
            // 命中：取 vals[i]
            _asm.mov(contSlot, "%rcx");
            _asm.mov(iSlot, "%rax");
            _asm.inst("movq", "32(%rcx)", "%rdx");
            _asm.inst("movq", "(%rdx,%rax,8)", "%rax");
            _asm.jmp(doneLabel);
            _asm.label(nextLabel);
            _asm.mov(iSlot, "%rax");
            _asm.inst("addq", "$1", "%rax");
            _asm.mov("%rax", iSlot);
            _asm.jmp(loopLabel);
            _asm.label(missLabel);
            // 未命中返回 null（与 legacy 的 notFoundBB 行为一致）
            _asm.movImm(0, "%rax");
            _asm.label(doneLabel);
            if (resultType && resultType->isFloat())            {
                _asm.inst("movq", "%rax", "%xmm0");
            }
            return;
        }
        fail("Index access on a non-container value: " + node->toString());
    }
    void CodeGen::emitArgsToRegisters(const std::vector<std::shared_ptr<ast::ExpressionNode>>& args,
                                      const std::string& calleeDesc,
                                      const std::vector<sema::TypePtr>& paramTypes,
                                      const size_t intStart,
                                      const size_t floatStart,
                                      const std::vector<bool>* annotated)    {
        const auto& intRegs = _abi.getIntArgRegs();
        const auto& floatRegs = _abi.getFloatArgRegs();
        // 每个实参的目标值类别：有形参类型就按形参类型，否则按实参自身类型。
        // 这一步决定了走整型寄存器还是浮点寄存器 —— 必须与 callee 的
        // 形参落栈逻辑一致，否则 `(x) -> { ret x / 2.0 }` 这种
        // 形参未标注类型的函数会把 x 装进 %rcx，而被调方从 %xmm0 读。
        std::vector<bool> targetIsFloat(args.size(), false);
        for (size_t i = 0; i < args.size(); ++i)        {
            if (i < paramTypes.size() && paramTypes[i] &&
                paramTypes[i]->getKind() != sema::TypeKind::TK_UNKNOWN)            {
                targetIsFloat[i] = paramTypes[i]->isFloat();
            }
 else            {
                targetIsFloat[i] = inferType(args[i])->isFloat();
            }
        }
        // 整型与浮点各用一套寄存器序列（MS ABI 规则）。
        // intStart / floatStart 用于给隐藏的 this 之类的参数预留槽位：
        // 方法调用时 %rcx 已经被 this 占掉，用户实参要从 %rdx 起排。
        size_t intCount = intStart;
        size_t floatCount = floatStart;
        for (size_t i = 0; i < args.size(); ++i)        {
            if (targetIsFloat[i]) ++floatCount;
            else ++intCount;
        }
        if (intCount > intRegs.size() || floatCount > floatRegs.size())        {
            fail("More than 4 arguments are not supported yet: " + calleeDesc);
            return;
        }
        // 求值顺序从左到右，但寄存器分配要先算好，避免多个浮点实参
        // 在暂存时互相覆盖（全都经过 %xmm0）。
        std::vector<bool> isFloat(args.size(), false);
        std::vector<std::string> target(args.size());
        intCount = intStart;
        floatCount = floatStart;
        for (size_t i = 0; i < args.size(); ++i)        {
            isFloat[i] = inferType(args[i])->isFloat();
            target[i] = targetIsFloat[i] ? floatRegs[floatCount++] : intRegs[intCount++];
        }
        // P9：形参类型已知时校验实参 —— 这是「类型错误诊断」最要紧的一处，
        // 因为 `f("hello")` 传给 `x: int` 之前会静默把指针当整数用。
        for (size_t i = 0; i < args.size() && i < paramTypes.size(); ++i)        {
            if (!paramTypes[i] || paramTypes[i]->getKind() == sema::TypeKind::TK_UNKNOWN) continue;
            // 未标注的形参类型是「回退 int」，不是用户的承诺，不能拿来判错。
            // `fun apply(f, x)` 的 f 会被传闭包，按 int 校验就误报了。
            if (annotated && i < annotated->size() && !(*annotated)[i]) continue;
            const auto why = checkAssignable(paramTypes[i], inferType(args[i]));
            if (!why.empty())            {
                failAt(args[i] ? args[i]->getPos() : utils::getUnknownPos(),
                       "Argument " + std::to_string(i + 1) + " of '" + calleeDesc + "': " + why);
                return;
            }
        }
        // 先把所有实参求值并压栈暂存（栈上每项 8 字节），
        // 再从后往前弹出到各自的参数寄存器。
        for (size_t i = 0; i < args.size(); ++i)        {
            emitExpression(args[i]);
            if (hasFailed()) return;
            // 实参类型与形参类型不一致时按形参转换。
            if (isFloat[i] && !targetIsFloat[i])            {
                _asm.inst("cvttsd2siq", "%xmm0", "%rax");
            }
 else if (!isFloat[i] && targetIsFloat[i])            {
                _asm.inst("cvtsi2sdq", "%rax", "%xmm0");
            }
            if (targetIsFloat[i])            {
                _asm.subRsp(8);
                _asm.inst("movsd", "%xmm0", "(%rsp)");
            }
 else            {
                _asm.push("%rax");
            }
        }
        for (size_t i = args.size(); i-- > 0;)        {
            if (targetIsFloat[i])            {
                // 注意：这里不能先 movsd 到 %xmm0 再转到目标寄存器，
                // 因为后面的实参可能也要经过 %xmm0。直接读到目标寄存器。
                _asm.inst("movsd", "(%rsp)", target[i]);
                _asm.addRsp(8);
            }
 else            {
                _asm.pop(target[i]);
            }
        }
    }
    void CodeGen::emitIndirectCall(const std::shared_ptr<ast::ExpressionNode>& callee,
                                   const std::vector<std::shared_ptr<ast::ExpressionNode>>& args,
                                   const std::vector<sema::TypePtr>& paramTypes)    {
        // 函数值的表示：指向 16 字节闭包对象的指针
        //   +0  fnptr（代码地址）
        //   +8  env（捕获环境；P4 阶段无捕获，恒为 0）
        // 调用约定：env 走 %r10，目标地址走 %r11，其余与普通调用一致。
        //
        // 顺序很关键：先把闭包指针压栈保住，再布置实参（布置实参要用
        // 参数寄存器，任何一次求值都可能把它们冲掉），最后取回闭包指针。
        emitExpression(callee);
        if (hasFailed()) return;
        _asm.push("%rax");
        emitArgsToRegisters(args, nodeText(callee), paramTypes);
        if (hasFailed())        {
            _asm.addRsp(8);
            return;
        }
        // 取回闭包指针，再拆出 env 与 fnptr。
        // 必须先读 +8 再读 +0，否则 %rax 被覆盖后就没法再取 env。
        _asm.pop("%rax");
        _asm.mov("8(%rax)", "%r10");
        _asm.mov("(%rax)", "%r11");
        const size_t shadow = _abi.getShadowSpaceSize();
        _asm.subRsp(shadow);
        _asm.inst("call", "*%r11");
        _asm.addRsp(shadow);
    }
    void CodeGen::collectFreeVariables(const std::shared_ptr<ast::ExpressionNode>& node,
                                       std::vector<std::string>& bound,
                                       std::vector<std::string>& free)    {
        if (!node) return;
        // 嵌套匿名函数：它的自由变量在它自己的创建点解析，这里不深入。
        // 但它本身作为一个表达式，其函数体不贡献本层的自由变量。
        if (std::dynamic_pointer_cast<ast::AnonFunctionDefinitionNode>(node)) return;
        if (const auto id = std::dynamic_pointer_cast<ast::IdentifierNode>(node))        {
            const std::string name = id->getName();
            if (!contains(bound, name) && !contains(free, name))            {
                free.push_back(name);
            }
            return;
        }
        // var 定义：先收集初始化表达式，再把名字加入 bound
        if (const auto varDef = std::dynamic_pointer_cast<ast::VariableDefinitionNode>(node))        {
            for (const auto& def : varDef->getVarDefs())            {
                if (def->hasInitialValue()) collectFreeVariables(def->getValueNode(), bound, free);
                if (def->getNameNode()) bound.push_back(def->getNameNode()->getName());
            }
            return;
        }
        if (const auto assign = std::dynamic_pointer_cast<ast::AssignmentNode>(node))        {
            const auto [lhs, rhs] = assign->getAssignPair();
            collectFreeVariables(rhs, bound, free);
            // P8：赋值目标也可能是外部变量（`total = total + x`）。
            // 目标若是下标/成员访问，里面的基址同样是自由变量，
            // 所以这里统一走一遍 —— 只有裸标识符才会被 IdentifierNode 分支收下，
            // 而它已经进过 bound 的话不会重复登记。
            if (!std::dynamic_pointer_cast<ast::IdentifierNode>(lhs))            {
                collectFreeVariables(lhs, bound, free);
            }
 else            {
                // 裸标识符作为赋值目标：若它是外层变量，也算一次使用
                const auto name = std::dynamic_pointer_cast<ast::IdentifierNode>(lhs)->getName();
                if (!contains(bound, name) && !contains(free, name)) free.push_back(name);
            }
            return;
        }
        // ---------- P8：补上原先漏掉的分支 ----------
        // 没有这几个分支时，闭包里写 `f(x)` / `xs[i]` / `[a, b]` / `{k: v}`
        // 都不会登记任何自由变量，捕获列表就是空的，运行时读到垃圾。
        if (const auto call = std::dynamic_pointer_cast<ast::FunctionCallNode>(node))        {
            collectFreeVariables(call->getLeftNode(), bound, free);
            collectFreeVariables(call->getRightNode(), bound, free);
            return;
        }
        if (const auto index = std::dynamic_pointer_cast<ast::IndexExpressionNode>(node))        {
            // 必须排在 PostfixExpressionNode 之前：IndexExpressionNode 是它的子类
            collectFreeVariables(index->getLeftNode(), bound, free);
            collectFreeVariables(index->getIndexNode(), bound, free);
            return;
        }
        if (const auto dict = std::dynamic_pointer_cast<ast::DictionaryExpressionNode>(node))        {
            collectFreeVariables(dict->getBodyNode(), bound, free);
            return;
        }
        if (const auto list = std::dynamic_pointer_cast<ast::ListExpressionNode>(node))        {
            collectFreeVariables(list->getBodyNode(), bound, free);
            return;
        }
        if (const auto bracket = std::dynamic_pointer_cast<ast::BracketExpressionNode>(node))        {
            collectFreeVariables(bracket->getBodyNode(), bound, free);
            return;
        }
        if (const auto infix = std::dynamic_pointer_cast<ast::InfixExpressionNode>(node))        {
            collectFreeVariables(infix->getLeftNode(), bound, free);
            collectFreeVariables(infix->getRightNode(), bound, free);
            return;
        }
        if (const auto prefix = std::dynamic_pointer_cast<ast::PrefixExpressionNode>(node))        {
            collectFreeVariables(prefix->getNode(), bound, free);
            return;
        }
        if (const auto unary = std::dynamic_pointer_cast<ast::UnaryExpressionNode>(node))        {
            collectFreeVariables(unary->getRightNode(), bound, free);
            return;
        }
        if (const auto postfix = std::dynamic_pointer_cast<ast::PostfixExpressionNode>(node))        {
            collectFreeVariables(postfix->getNode(), bound, free);
            return;
        }
        if (const auto ret = std::dynamic_pointer_cast<ast::ReturnExpressionNode>(node))        {
            collectFreeVariables(ret->getReturnNode(), bound, free);
            return;
        }
        if (const auto stmt = std::dynamic_pointer_cast<ast::ExpressionStatementNode>(node))        {
            collectFreeVariables(stmt->getExpression(), bound, free);
            return;
        }
        if (const auto paren = std::dynamic_pointer_cast<ast::ParenRangerNode>(node))        {
            collectFreeVariables(paren->getRangerNode(), bound, free);
            return;
        }
        if (const auto block = std::dynamic_pointer_cast<ast::BlockRangerNode>(node))        {
            for (const auto& e : block->getBodyExpressions()) collectFreeVariables(e, bound, free);
            return;
        }
        if (const auto cond = std::dynamic_pointer_cast<ast::ConditionNode>(node))        {
            for (const auto& b : cond->getBranchNodes())            {
                if (const auto branch = std::dynamic_pointer_cast<ast::BranchNode>(b))                {
                    collectFreeVariables(branch->getConditionNode(), bound, free);
                    collectFreeVariables(branch->getBodyNode(), bound, free);
                }
            }
            return;
        }
        if (const auto forLoop = std::dynamic_pointer_cast<ast::ForLoopNode>(node))        {
            // for 的 init 里定义的变量属于循环体作用域，简化处理：按顺序收集
            collectFreeVariables(forLoop->getInitNode(), bound, free);
            collectFreeVariables(forLoop->getConditionNode(), bound, free);
            collectFreeVariables(forLoop->getUpdateNode(), bound, free);
            collectFreeVariables(forLoop->getBodyNode(), bound, free);
            return;
        }
        if (const auto loop = std::dynamic_pointer_cast<ast::LoopNode>(node))        {
            collectFreeVariables(loop->getConditionNode(), bound, free);
            collectFreeVariables(loop->getBodyNode(), bound, free);
            return;
        }
    }
    void CodeGen::emitAnonymousFunction(const std::shared_ptr<ast::ExpressionNode>& node)    {
        const auto anon = std::dynamic_pointer_cast<ast::AnonFunctionDefinitionNode>(node);
        if (!anon)        {
            fail("Not an anonymous function: " + node->toString());
            return;
        }
        // 形参：`(a, b) -> {...}` 的参数列表存在 paramNode 里，
        // 结构与命名函数的参数列表一致（ParenRangerNode 包着逗号表达式）。
        std::vector<std::string> paramNames;
        std::vector<sema::TypePtr> paramTypes;
        if (const auto paramNode = anon->getParamNode())        {
            collectParamNames(paramNode, paramNames);
            collectParamTypes(paramNode, paramTypes);
        }
        // 类型列表补齐到与名字列表等长。
        // 未标注的形参不能一律当 int：`(x) -> { ret x / 2.0 }` 里的 x
        // 如果按 int 处理，实参就会走 %rcx 而不是 %xmm0，结果直接错。
        // 这里先留 UNKNOWN，等下面按函数体「反推」一轮再定。
        paramTypes.resize(paramNames.size(), sema::getUnknownType());
        // 返回类型：从函数体里找第一个 ret 的表达式类型。
        // 这一步同时能反推未标注形参的类型（见 inferAnonReturnType）。
        auto retType = inferAnonReturnType(anon->getBodyNode(), paramNames, paramTypes);
        if (!retType || retType->getKind() == sema::TypeKind::TK_UNKNOWN) retType = sema::getIntType();
        // 反推之后仍未定型的形参才回退到 int。
        for (auto& pt : paramTypes)        {
            if (!pt || pt->getKind() == sema::TypeKind::TK_UNKNOWN) pt = sema::getIntType();
        }
        ++_anonCounter;
        const std::string fnLabel = "anon_func_" + std::to_string(_anonCounter);
        const std::string closureLabel = ".Lclosure" + std::to_string(_anonCounter);
        // 收集自由变量：函数体里用到、但不是形参、也不是它自己用 var 定义的
        // 名字。这些必须按值捕获进闭包的 env 块，否则函数体里访问不到。
        std::vector<std::string> bound = paramNames;
        std::vector<std::string> free;
        collectFreeVariables(anon->getBodyNode(), bound, free);
        // 只保留确实在当前作用域里存在的（函数名、全局等留给直接调用路径）。
        std::vector<std::string> capturedNames;
        std::vector<sema::TypePtr> capturedTypes;
        for (const auto& name : free)        {
            if (const auto slot = lookupVar(name))            {
                capturedNames.push_back(name);
                capturedTypes.push_back(slot->type ? slot->type : sema::getIntType());
            }
        }
        // 函数体不能就地发射（会打断外层函数的指令流），先排队。
        PendingFunction pending;
        pending.emitName = fnLabel;
        pending.paramNames = paramNames;
        pending.paramTypes = paramTypes;
        pending.retType = retType;
        pending.bodyNode = anon->getBodyNode();
        pending.capturedNames = capturedNames;
        pending.capturedTypes = capturedTypes;
        _pendingFunctions.push_back(pending);
        if (capturedNames.empty())        {
            // 无捕获：闭包对象是静态的 { fnptr, 0 }，表达式值就是它的地址。
            _closureObjects.push_back({closureLabel, fnLabel});
            _asm.lea(closureLabel + "(%rip)", "%rax");
            return;
        }
        // 有捕获：运行时分配一个 { fnptr, env } 块，再把捕获值写进 env。
        // 布局：[0]=fnptr, [8]=env 指针, [16..]=捕获值（每项 8 字节）。
        // 这样 env 也能被内层闭包继续捕获（多层闭包按值传递）。
        const size_t envBytes = capturedNames.size() * 8;
        _asm.movImm(static_cast<long long>(16 + envBytes), "%rcx");
        _asm.movImm(8, "%rdx");
        _asm.subRsp(32);
        _asm.call("core_alloc");
        _asm.addRsp(32);
        // 分配失败时按空指针处理，后续调用会崩在明显的地方，而不是静默错值。
        _asm.push("%rax");
        // [0] = fnptr
        _asm.lea(fnLabel + "(%rip)", "%rcx");
        _asm.inst("movq", "%rcx", "(%rax)");
        // [8] = env = 块内 16 字节处
        _asm.lea("16(%rax)", "%rcx");
        _asm.inst("movq", "%rcx", "8(%rax)");
        // 逐项拷贝捕获值
        for (size_t i = 0; i < capturedNames.size(); ++i)        {
            const auto slot = lookupVar(capturedNames[i]);
            if (!slot) continue;
            const std::string src = addrOf(*slot);
            const std::string dst = std::to_string(16 + i * 8) + "(%rax)";
            _asm.inst("movq", src, "%rcx");
            _asm.inst("movq", "%rcx", dst);
        }
        _asm.pop("%rax");
    }
    void CodeGen::emitPendingFunctions()    {
        // 匿名函数体里还可能再定义匿名函数，所以循环到队空为止。
        // 用下标遍历（期间 _pendingFunctions 可能增长/重分配），并拷贝当前项。
        size_t index = 0;
        while (index < _pendingFunctions.size())        {
            const PendingFunction pending = _pendingFunctions[index++];
            emitFunctionBody(pending.emitName, pending.paramNames, pending.paramTypes,
                             pending.retType, pending.bodyNode, false,
                             pending.capturedNames, pending.capturedTypes);
        }
        _pendingFunctions.clear();
    }
    void CodeGen::emitClosureObjects()    {
        if (_closureObjects.empty()) return;
        _asm.data();
        for (const auto& closure : _closureObjects)        {
            _asm.align(8);
            _asm.label(closure.label);
            _asm.quad(closure.fnLabel);
            _asm.quad(0LL);
        }
    }
    // 具名函数被当作函数值使用时，也要有一个闭包对象。
    // 同一个函数只生成一个（按函数名去重），这样 `apply(add, 3)` 反复出现
    // 也只多出一个 16 字节的常量。
    std::string CodeGen::closureForFunction(const std::string& fname)    {
        if (const auto found = _namedClosures.find(fname); found != _namedClosures.end())        {
            return found->second;
        }
        const std::string label = ".Lfnclosure_" + fname;
        _namedClosures[fname] = label;
        // 具名函数的发射符号：用户 main 是 __rio_user_main
        const std::string fnLabel = (fname == "main") ? "__rio_user_main" : fname;
        _closureObjects.push_back({label, fnLabel});
        return label;
    }
    void CodeGen::emitFunctionCall(const std::shared_ptr<ast::ExpressionNode>& node)    {
        const auto call = std::dynamic_pointer_cast<ast::FunctionCallNode>(node);
        if (!call)        {
            fail("Not a function call: " + node->toString());
            return;
        }
        const auto callee = call->getLeftNode();
        // 收集实参。Rio 的实参列表在 AST 里形如：
        //   ParenRangerNode{ InfixNode(comma, a, b) }  或  单个表达式
        std::vector<std::shared_ptr<ast::ExpressionNode>> args;
        if (const auto argsNode = std::dynamic_pointer_cast<ast::ExpressionNode>(call->getRightNode()))        {
            collectArgs(argsNode, args);
        }
        // 判定直接调用还是经由函数值间接调用：
        //   * 标识符且是 sout              -> sout 特化
        //   * 标识符且是当前作用域里的局部变量 -> 槽位里放着函数值，间接调用
        //   * 标识符且不是局部变量           -> 直接调用（含 extern/encapsulated，
        //                                      它们的签名可能不在本文件里，
        //                                      不能因为「不认识」就当成间接调用）
        //   * 被调者是任意其它表达式         -> 间接调用
        std::string fname;
        bool indirect = false;
        // P6：`p.sum()` —— 被调者是 `.` 中缀表达式，走方法调用。
        // P8 例外：`ds.size(x)` 里的 `ds` 是模块名，模块在加载期已被内联成
        // 普通全局函数，因此 `mod.fn(...)` 退化成直接调用 `fn(...)`。
        if (const auto member = std::dynamic_pointer_cast<ast::InfixExpressionNode>(callee))        {
            if (member->getOpToken().getType() == core::TokenType::TOKEN_DOT)            {
                if (const auto modId = std::dynamic_pointer_cast<ast::IdentifierNode>(member->getLeftNode());
                    modId && isModuleName(modId->getName()))                {
                    if (const auto fnId = std::dynamic_pointer_cast<ast::IdentifierNode>(member->getRightNode()))                    {
                        emitModuleFunctionCall(fnId->getName(), node, args);
                        return;
                    }
                }
                emitMethodCall(member, node, args);
                return;
            }
        }
        if (const auto calleeId = std::dynamic_pointer_cast<ast::IdentifierNode>(callee))        {
            const std::string n = calleeId->getName();
            // sout 特化
            if (n == "sout")            {
                emitSoutCall(node);
                return;
            }
            // P6：`super(...)` —— 用当前 this 调基类构造器
            if (n == "super")            {
                emitSuperCall(args);
                return;
            }
            // P6：`Point(10, 20)` —— 类名后接括号就是构造调用。
            // 必须排在「是不是局部变量」之前判断吗？不用：局部变量遮蔽类名的
            // 情况（`var Point = ...`）极不常见，而查找顺序上先看局部变量
            // 更符合直觉，所以这里保持 lookupVar 优先。
            if (lookupVar(n) == nullptr && findClass(n) != nullptr)            {
                emitConstructorCall(n, node, args);
                return;
            }
            if (lookupVar(n) != nullptr)            {
                // 局部变量遮蔽了同名函数，或者这个名字根本不是函数：
                // 一律当成「槽位里放着函数值」处理。
                indirect = true;
            }
 else            {
                // P10：裸名泛型调用 `identity(42)` —— 类型实参从实参类型推。
                if (tryEmitGenericCall(n, callee, node, args)) return;
                // P11：重载名在**这里**决议，符号由决议结果决定；
                // 唯一名字继续走下面的裸名路径（行为与 P10 完全一致）。
                if (_overloadedNames.find(n) != _overloadedNames.end())                {
                    bool ambiguous = false;
                    const std::string symbol = resolveOverload(n, args, &ambiguous);
                    if (symbol.empty())                    {
                        failOverload(n, args, node->getPos(), ambiguous);
                        return;
                    }
                    emitResolvedCall(symbol, n, args);
                    return;
                }
                fname = n;
            }
        }
 else if (const auto calleeIndex = std::dynamic_pointer_cast<ast::IndexExpressionNode>(callee))        {
            // P10：`identity[int](42)` —— 带显式类型实参的泛型调用。
            // callee 是 IndexExpressionNode(Identifier identity, Bracket(int))。
            std::string n;
            if (const auto id = std::dynamic_pointer_cast<ast::IdentifierNode>(calleeIndex->getLeftNode()))            {
                n = id->getName();
            }
            if (n.empty())            {
                indirect = true;
            }
            else if (_genericFunctions.find(n) != _genericFunctions.end())            {
                tryEmitGenericCall(n, callee, node, args);
                return;
            }
            else if (_genericClasses.find(n) != _genericClasses.end())            {
                // P10：`Box[int](1)` —— 泛型类构造调用。
                const auto typeArgs = typeArgsOf(calleeIndex);
                const std::string instName = ensureClassInstance(n, typeArgs);
                if (instName.empty())                {
                    failAt(node->getPos(), "Cannot instantiate generic class '" + n +
                                           "' with " + std::to_string(typeArgs.size()) +
                                           " type argument(s)");
                    return;
                }
                computeLayout(instName);
                emitConstructorCall(instName, node, args);
                return;
            }
            else            {
                // 不是泛型函数：`f[int]` 只可能是索引取值，交给索引路径
                indirect = true;
            }
        }
 else        {
            indirect = true;
        }
        if (indirect)        {
            // 间接调用：形参类型从函数值的 FunctionType 里取（有就按它走寄存器）
            std::vector<sema::TypePtr> paramTypes;
            if (const auto calleeId = std::dynamic_pointer_cast<ast::IdentifierNode>(callee))            {
                if (const auto slot = lookupVar(calleeId->getName());
                    slot && slot->type && slot->type->getKind() == sema::TypeKind::TK_FUNC)                {
                    paramTypes = std::static_pointer_cast<sema::FunctionType>(slot->type)->getParams();
                }
            }
            emitIndirectCall(callee, args, paramTypes);
            return;
        }
        // P8：命名实参与缺省实参在这里统一摊平。必须在取 paramTypes 之前做，
        // 否则实参个数和位置都对不上形参签名。
        const auto callArgs = normalizeCallArgs(args, fname);
        // 用户自定义的 main 在 codegen 里发射为 __rio_user_main
        const std::string emitName = (fname == "main") ? "__rio_user_main" : fname;
        // 有形参签名就按形参类型走寄存器（能处理隐式 int<->float 转换）
        std::vector<sema::TypePtr> paramTypes;
        if (const auto found = _functionParams.find(fname); found != _functionParams.end())        {
            paramTypes = found->second;
        }
        const std::vector<bool>* annotatedMask = nullptr;
        if (const auto found = _functionParamAnnotated.find(fname); found != _functionParamAnnotated.end())        {
            annotatedMask = &found->second;
        }
        emitArgsToRegisters(callArgs, fname, paramTypes, 0, 0, annotatedMask);
        if (hasFailed())        {
            return;
        }
        // 预留 shadow space 并调用。
        // 注意：MS ABI 要求 32 字节 shadow space，且调用点 %rsp 必须 16 字节对齐。
        const size_t shadow = _abi.getShadowSpaceSize();
        _asm.subRsp(shadow);
        _asm.call(emitName);
        _asm.addRsp(shadow);
    }
    bool CodeGen::isModuleName(const std::string& name) const    {
        for (const auto& m : _moduleNames)        {
            if (m == name) return true;
        }
        return false;
    }
    std::vector<std::shared_ptr<ast::ExpressionNode>> CodeGen::normalizeCallArgs(
        const std::vector<std::shared_ptr<ast::ExpressionNode>>& args,
        const std::string& fname) const
    {
        // 没有登记形参名：不认识这个函数，原样返回
        const auto namesIt = _functionParamNames.find(fname);
        if (namesIt == _functionParamNames.end() || namesIt->second.empty()) return args;
        const auto& names = namesIt->second;
        // 实参是否含命名形式 `name=value`：AST 里是 AssignmentNode(target=Identifier)
        bool anyNamed = false;
        for (const auto& a : args)
        {
            if (const auto assign = std::dynamic_pointer_cast<ast::AssignmentNode>(a))
            {
                if (std::dynamic_pointer_cast<ast::IdentifierNode>(assign->getAssignPair().first))
                {
                    anyNamed = true;
                    break;
                }
            }
        }
        // 既没有命名实参、实参个数也够，就不动它
        const auto defaultsIt = _functionDefaults.find(fname);
        const bool hasDefaults = defaultsIt != _functionDefaults.end();
        if (!anyNamed && (!hasDefaults || args.size() >= names.size())) return args;

        // 按形参位置摆放：命名实参按名字定位，位置实参按出现顺序填剩下的空位
        std::vector<std::shared_ptr<ast::ExpressionNode>> slots(names.size(), nullptr);
        size_t positional = 0;
        for (const auto& a : args)
        {
            std::string named;
            std::shared_ptr<ast::ExpressionNode> value = a;
            if (const auto assign = std::dynamic_pointer_cast<ast::AssignmentNode>(a))
            {
                if (const auto target =
                        std::dynamic_pointer_cast<ast::IdentifierNode>(assign->getAssignPair().first))
                {
                    named = target->getName();
                    value = assign->getAssignPair().second;
                }
            }
            if (!named.empty())
            {
                size_t idx = names.size();
                for (size_t i = 0; i < names.size(); ++i)
                {
                    if (names[i] == named)
                    {
                        idx = i;
                        break;
                    }
                }
                // 名字对不上任何形参：交给原来的路径报错，不在这里静默吞掉
                if (idx == names.size()) return args;
                slots[idx] = value;
            }
            else
            {
                while (positional < slots.size() && slots[positional]) ++positional;
                if (positional >= slots.size()) return args;
                slots[positional++] = value;
            }
        }
        // 空位用默认值补；没默认值说明实参确实少了，原样返回让原逻辑报错
        for (size_t i = 0; i < slots.size(); ++i)
        {
            if (slots[i]) continue;
            if (hasDefaults)
            {
                if (const auto found = defaultsIt->second.find(names[i]);
                    found != defaultsIt->second.end())
                {
                    slots[i] = found->second;
                    continue;
                }
            }
            return args;
        }
        return slots;
    }
    void CodeGen::emitModuleFunctionCall(const std::string& fname,
                                         const std::shared_ptr<ast::ExpressionNode>& callNode,
                                         const std::vector<std::shared_ptr<ast::ExpressionNode>>& args)    {
        // 模块在加载期已经被内联，`mod.fn(...)` 与 `fn(...)` 是同一个符号。
        // 这里只处理「直接调用模块里的全局函数」这一种情形：
        //   * 模块里的类由类名直接构造（`mod.Cls(...)` 会退化成构造调用）
        //   * 其余非函数成员（`mod.CONST`）不是调用，不该走到这里
        if (lookupVar(fname) == nullptr && findClass(fname) != nullptr)        {
            emitConstructorCall(fname, callNode, args);
            return;
        }
        // P11：模块里被内联进来的函数同样可能是重载集 —— `mod.f(1.5)`
        // 必须和 `f(1.5)` 决议到同一个符号，否则会调到「最后一个定义」上去。
        if (_overloadedNames.find(fname) != _overloadedNames.end())        {
            bool ambiguous = false;
            const std::string symbol = resolveOverload(fname, args, &ambiguous);
            if (symbol.empty())            {
                failOverload(fname, args, callNode ? callNode->getPos() : utils::getUnknownPos(), ambiguous);
                return;
            }
            emitResolvedCall(symbol, fname, args);
            return;
        }
        std::vector<sema::TypePtr> paramTypes;
        if (const auto found = _functionParams.find(fname); found != _functionParams.end())        {
            paramTypes = found->second;
        }
        emitArgsToRegisters(normalizeCallArgs(args, fname), fname, paramTypes);
        if (hasFailed())        {
            return;
        }
        const size_t shadow = _abi.getShadowSpaceSize();
        _asm.subRsp(shadow);
        _asm.call(fname);
        _asm.addRsp(shadow);
    }
    void CodeGen::emitRtPrintCall(const std::string& fnName, bool isFloat)    {
        // rt 打印函数的调用约定（Win64）：
        //   整型：值在 %rcx
        //   浮点：值在 %xmm0（即调用前已经就位）
        if (!isFloat)        {
            _asm.mov("%rax", "%rcx");
        }
        _asm.subRsp(32);
      // shadow space
        _asm.call(fnName);
        _asm.addRsp(32);
    }
    void CodeGen::emitSoutArg(const std::shared_ptr<ast::ExpressionNode>& arg)    {
        if (!arg) return;
        // null 字面量单独处理：没有类型信息可用
        if (std::dynamic_pointer_cast<ast::NullLiteralNode>(arg))        {
            _asm.call("rt_print_null");
            return;
        }
        const auto type = inferType(arg);
        // 字符串字面量走最快的路径：长度在编译期已知，直接 core_io_write。
        // 注意字面量现在带 8 字节长度头，真实字节从 +8 开始。
        if (const auto lit = std::dynamic_pointer_cast<ast::StringLiteralNode>(arg))        {
            const std::string text = lit->unescapedString();
            const auto [label, len] = _asm.internStrValue(text);
            _asm.movImm(1, "%rcx");
            _asm.lea(label + "+8(%rip)", "%rdx");
            _asm.movImm(static_cast<long long>(len), "%r8");
            _asm.subRsp(32);
            _asm.call("core_io_write");
            _asm.addRsp(32);
            return;
        }
        emitExpression(arg);
        if (type->isFloat())        {
            emitRtPrintCall("rt_print_float", true);
            return;
        }
        if (type->isInteger())        {
            emitRtPrintCall("rt_print_int", false);
            return;
        }
        if (type->getKind() == sema::TypeKind::TK_BOOL)        {
            emitRtPrintCall("rt_print_bool", false);
            return;
        }
        if (type->getKind() == sema::TypeKind::TK_CHAR)        {
            emitRtPrintCall("rt_print_char", false);
            return;
        }
        if (type->getKind() == sema::TypeKind::TK_STR)        {
            emitRtPrintCall("rt_print_str", false);
            return;
        }
        if (type->isPointerLike())        {
            emitRtPrintCall("rt_print_ptr", false);
            return;
        }
        fail("sout cannot print a value of type " + type->toString());
    }
    void CodeGen::emitSoutCall(const std::shared_ptr<ast::ExpressionNode>& node)    {
        const auto call = std::dynamic_pointer_cast<ast::FunctionCallNode>(node);
        if (!call) return;
        // 拆实参：关键字参数 end=... 要单独识别出来
        std::vector<std::shared_ptr<ast::ExpressionNode>> args;
        std::shared_ptr<ast::ExpressionNode> endArg;
        if (const auto argsNode = std::dynamic_pointer_cast<ast::ExpressionNode>(call->getRightNode()))        {
            collectArgs(argsNode, args);
        }
        std::vector<std::shared_ptr<ast::ExpressionNode>> positional;
        for (const auto& a : args)        {
            // end="x" 在 AST 里有两种形态：
            //   InfixExpressionNode('=' 或 ':', Identifier(end), 值)   —— 有些优先级组合
            //   AssignmentNode(Identifier(end), 值)                    —— 实测常见形态
            if (const auto assign = std::dynamic_pointer_cast<ast::AssignmentNode>(a))            {
                const auto [lhs, rhs] = assign->getAssignPair();
                if (const auto lid = std::dynamic_pointer_cast<ast::IdentifierNode>(lhs);                    lid && lid->getName() == "end")                {
                    endArg = rhs;
                    continue;
                }
            }
            if (const auto infix = std::dynamic_pointer_cast<ast::InfixExpressionNode>(a))            {
                const auto t = infix->getOpToken().getType();
                const auto lid = std::dynamic_pointer_cast<ast::IdentifierNode>(infix->getLeftNode());
                if (lid && lid->getName() == "end" &&                    (t == core::TokenType::TOKEN_ASSIGN || t == core::TokenType::TOKEN_COLON))                {
                    endArg = infix->getRightNode();
                    continue;
                }
            }
            positional.push_back(a);
        }
        if (positional.empty())        {
            fail("sout requires at least one argument");
            return;
        }
        for (const auto& arg : positional)        {
            emitSoutArg(arg);
        }
        // 结尾符：默认换行
        if (endArg)        {
            if (const auto lit = std::dynamic_pointer_cast<ast::StringLiteralNode>(endArg))            {
                const std::string text = lit->unescapedString();
                if (!text.empty())                {
                    const auto [label, len] = _asm.internString(text);
                    _asm.movImm(1, "%rcx");
                    _asm.lea(label + "(%rip)", "%rdx");
                    _asm.movImm(static_cast<long long>(len), "%r8");
                    _asm.subRsp(32);
                    _asm.call("core_io_write");
                    _asm.addRsp(32);
                }
            }
 else            {
                fail("sout's 'end' argument must be a string literal");
            }
        }
 else        {
            // 默认 end="\n"
            const auto [label, len] = _asm.internString("\n");
            _asm.movImm(1, "%rcx");
            _asm.lea(label + "(%rip)", "%rdx");
            _asm.movImm(static_cast<long long>(len), "%r8");
            _asm.subRsp(32);
            _asm.call("core_io_write");
            _asm.addRsp(32);
        }
    }
    // ============================================================
    // P6：类
    //
    // 对象布局见 code/include/sema/rcc_sema_types.h 的 ClassType 注释：
    // 单个指针指向扁平的 8 字节槽数组，字段 i 在 +8*i，基类字段排在前面。
    // 所以「把 Derived 当 Base 用」天然成立，super(...) 也可以直接把同一个
    // this 交给基类构造器，不需要任何偏移调整。
    // ============================================================
    CodeGen::ClassInfo* CodeGen::findClass(const std::string& name)    {
        const auto it = _classes.find(name);
        return it == _classes.end() ? nullptr : &it->second;
    }
    const CodeGen::ClassInfo* CodeGen::findClass(const std::string& name) const    {
        const auto it = _classes.find(name);
        return it == _classes.end() ? nullptr : &it->second;
    }
    sema::TypePtr CodeGen::classTypeOf(const std::string& name) const    {
        if (!findClass(name)) return nullptr;
        return std::make_shared<sema::ClassType>(name);
    }
    sema::TypePtr CodeGen::resolveTypeName(const std::string& raw) const    {
        // 先按内建类型名解析，认不出来再查类登记表 —— 这样 `x: Point`
        // 这种标注才会得到 ClassType 而不是 TK_UNKNOWN。
        const auto builtin = parseTypeName(raw);
        if (builtin && builtin->getKind() != sema::TypeKind::TK_UNKNOWN) return builtin;        std::string name;
        for (const char c : raw)        {
            if (std::isalnum(static_cast<unsigned char>(c)) || c == '_') name += c;
            else if (!name.empty()) break;
        }
        if (name.empty()) return sema::getUnknownType();
        if (findClass(name)) return std::make_shared<sema::ClassType>(name);
        return sema::getUnknownType();
    }
    // ============================================================
    // P10：类型标注的完整解析（含 `[...]`）
    // ============================================================
    void CodeGen::flattenTypeArgs(const std::shared_ptr<ast::ExpressionNode>& node,
                                  std::vector<std::shared_ptr<ast::ExpressionNode>>& out)    {
        if (!node) return;
        // `int, str` 在 AST 里是 InfixExpressionNode(',')
        if (const auto infix = std::dynamic_pointer_cast<ast::InfixExpressionNode>(node))        {
            if (infix->getOpToken().getType() == core::TokenType::TOKEN_COMMA)            {
                flattenTypeArgs(infix->getLeftNode(), out);
                flattenTypeArgs(infix->getRightNode(), out);
                return;
            }
        }
        // 括号包一层：`dict[(str, int)]` 之类
        if (const auto paren = std::dynamic_pointer_cast<ast::ParenRangerNode>(node))        {
            flattenTypeArgs(paren->getRangerNode(), out);
            return;
        }
        // 列表包一层：`list[[int]]` 之类
        if (const auto list = std::dynamic_pointer_cast<ast::ListExpressionNode>(node))        {
            flattenTypeArgs(list->getBodyNode(), out);
            return;
        }
        out.push_back(node);
    }
    sema::TypePtr CodeGen::typeFromTypeArgNode(const std::shared_ptr<ast::ExpressionNode>& node,
                                               const std::vector<std::string>& genericParams) const    {
        if (!node) return sema::getUnknownType();
        // 最常见的形态：裸标签 `float` / `int`
        if (const auto label = std::dynamic_pointer_cast<ast::LabelNode>(node))        {
            return typeFromLabelNode(label, genericParams);
        }
        // `Box[int]` / `list[int]` 这类带 `[...]` 的类型：AST 是
        //   IndexExpressionNode(target=Identifier(Box), index=BracketExpressionNode)
        if (const auto index = std::dynamic_pointer_cast<ast::IndexExpressionNode>(node))        {
            std::string base;
            if (const auto id = std::dynamic_pointer_cast<ast::IdentifierNode>(index->getLeftNode()))            {
                base = id->getName();
            }
            else if (const auto lbl = std::dynamic_pointer_cast<ast::LabelNode>(index->getLeftNode()))            {
                base = lbl->getFullLabel();
            }
            if (base.empty()) return sema::getUnknownType();
            // 收 `[...]` 里的类型实参
            std::vector<std::shared_ptr<ast::ExpressionNode>> argNodes;
            if (const auto bracket = std::dynamic_pointer_cast<ast::BracketExpressionNode>(index->getIndexNode()))            {
                flattenTypeArgs(bracket->getBodyNode(), argNodes);
            }
            std::vector<sema::TypePtr> args;
            for (const auto& a : argNodes) args.push_back(typeFromTypeArgNode(a, genericParams));
            if (base == "list" || base == "series" || base == "array")            {
                // 关键：`list[float]` 的元素类型必须真的来自标注。
                // P9 的缺陷就是这里一律给 int，导致 float 元素被当整数打印。
                const auto elem = args.empty() ? sema::getIntType() : args[0];
                return std::make_shared<sema::ListType>(
                    (elem && elem->getKind() != sema::TypeKind::TK_UNKNOWN) ? elem : sema::getIntType());
            }
            if (base == "dict" || base == "map")            {
                const auto key = args.size() > 0 ? args[0] : sema::getStrType();
                const auto val = args.size() > 1 ? args[1] : sema::getIntType();
                return std::make_shared<sema::DictType>(
                    (key && key->getKind() != sema::TypeKind::TK_UNKNOWN) ? key : sema::getStrType(),
                    (val && val->getKind() != sema::TypeKind::TK_UNKNOWN) ? val : sema::getIntType());
            }
            // 用户泛型类 `Box[int]`：按需单态化出一份具体类，
            // 类型指向 mangled 名（这样字段偏移/方法表都落在具体实例上）。
            // 注意：这里在 const 上下文里登记实例 —— 它是幂等的缓存填充
            // （同一组实参只会生成一次），不改变可观察的类型语义。
            if (_genericClasses.find(base) != _genericClasses.end())            {
                std::vector<sema::TypePtr> concrete;
                for (const auto& a : args) concrete.push_back(a);
                auto* self = const_cast<CodeGen*>(this);
                const std::string instName = self->ensureClassInstance(base, concrete);
                if (!instName.empty())                {
                    self->computeLayout(instName);
                    return std::make_shared<sema::ClassType>(instName);
                }
                return sema::getUnknownType();
            }
            if (findClass(base))            {
                return std::make_shared<sema::ClassType>(base);
            }
            return sema::getUnknownType();
        }
        if (const auto id = std::dynamic_pointer_cast<ast::IdentifierNode>(node))        {
            // 泛型形参出现在类型位置（`list[T]` 的 T）：先查泛型表，
            // 否则会被当成未知类型名而退化成 int。
            const std::string name = id->getName();
            for (const auto& g : genericParams)            {
                if (g == name)                {
                    if (const auto bound = _currentGenericBindings.find(g);
                        bound != _currentGenericBindings.end() && bound->second)
                        return bound->second;
                    return sema::getGenericType(g);
                }
            }
            return resolveTypeName(name);
        }
        return sema::getUnknownType();
    }
    sema::TypePtr CodeGen::typeFromLabelNode(const std::shared_ptr<ast::LabelNode>& label,
                                             const std::vector<std::string>& genericParams) const    {
        if (!label) return sema::getUnknownType();
        const std::string base = label->getFullLabel();
        // 泛型形参：声明阶段 T 是合法的「洞」，不是未知类型。
        // 实例化发射阶段则必须解析成**具体**类型 —— 槽位大小、走整型还是
        // 浮点寄存器、sout 选哪个 rt_print_* 全看它。
        for (const auto& g : genericParams)        {
            if (g == base)            {
                if (const auto bound = _currentGenericBindings.find(g);
                    bound != _currentGenericBindings.end() && bound->second)
                    return bound->second;
                return sema::getGenericType(g);
            }
        }
        // 裸名字（`[...]` 里的内容由下面的 IndexExpressionNode 路径处理）
        if (label->getLabelDesNodes().empty())        {
            return resolveTypeName(base);
        }
        // `funi[params][ret]`：labelDesNodes[0] = 参数表，[1] = 返回类型
        if (base == "funi" || base == "fun")        {
            const auto des = label->getLabelDesNodes();
            std::vector<sema::TypePtr> params;
            if (!des.empty() && des[0])            {
                std::vector<std::shared_ptr<ast::ExpressionNode>> paramNodes;
                flattenTypeArgs(des[0]->getBodyNode(), paramNodes);
                for (const auto& p : paramNodes) params.push_back(typeFromTypeArgNode(p, genericParams));
            }
            sema::TypePtr ret = sema::getVoidType();
            if (des.size() > 1 && des[1])            {
                ret = typeFromTypeArgNode(des[1]->getBodyNode(), genericParams);
            }
            return std::make_shared<sema::FunctionType>(std::move(params), std::move(ret));
        }
        // `list[float]` / `dict[str, float]` / `Box[int]`：等价于把它
        // 当成一个「名字 + 类型实参」的复合类型，和 IndexExpressionNode 同构处理。
        std::vector<sema::TypePtr> args;
        for (const auto& des : label->getLabelDesNodes())        {
            if (!des) continue;
            std::vector<std::shared_ptr<ast::ExpressionNode>> argNodes;
            flattenTypeArgs(des->getBodyNode(), argNodes);
            for (const auto& a : argNodes) args.push_back(typeFromTypeArgNode(a, genericParams));
        }
        if (base == "list" || base == "series" || base == "array")        {
            const auto elem = args.empty() ? sema::getIntType() : args[0];
            return std::make_shared<sema::ListType>(
                (elem && elem->getKind() != sema::TypeKind::TK_UNKNOWN) ? elem : sema::getIntType());
        }
        if (base == "dict" || base == "map")        {
            const auto key = args.size() > 0 ? args[0] : sema::getStrType();
            const auto val = args.size() > 1 ? args[1] : sema::getIntType();
            return std::make_shared<sema::DictType>(
                (key && key->getKind() != sema::TypeKind::TK_UNKNOWN) ? key : sema::getStrType(),
                (val && val->getKind() != sema::TypeKind::TK_UNKNOWN) ? val : sema::getIntType());
        }
        // `Box[int]`：泛型类按需单态化，类型指向 mangled 实例名。
        if (_genericClasses.find(base) != _genericClasses.end())        {
            auto* self = const_cast<CodeGen*>(this);
            const std::string instName = self->ensureClassInstance(base, args);
            if (!instName.empty())            {
                self->computeLayout(instName);
                return std::make_shared<sema::ClassType>(instName);
            }
            return sema::getUnknownType();
        }
        if (findClass(base)) return std::make_shared<sema::ClassType>(base);
        return sema::getUnknownType();
    }
    sema::TypePtr CodeGen::typeFromLabelList(
        const std::vector<std::shared_ptr<ast::LabelNode>>& labels,
        const std::vector<std::string>& genericParams) const    {
        for (const auto& label : labels)        {
            if (!label) continue;
            const auto t = typeFromLabelNode(label, genericParams);
            if (t && t->getKind() != sema::TypeKind::TK_UNKNOWN) return t;
        }
        return sema::getUnknownType();
    }
    void CodeGen::collectParamNamesT(const std::shared_ptr<ast::ExpressionNode>& node,
                                     std::vector<std::string>& out) const    {
        // 名字收集与类型无关，直接复用自由函数。
        collectParamNames(node, out);
    }
    void CodeGen::collectParamTypesT(const std::shared_ptr<ast::ExpressionNode>& node,
                                     std::vector<sema::TypePtr>& out) const    {
        // 与 collectParamTypes 同构，区别只在类型名要经过 resolveTypeName，
        // 才能把 `p: Point` 解析成 ClassType。
        if (!node) return;
        if (const auto id = std::dynamic_pointer_cast<ast::IdentifierNode>(node))        {
            out.push_back(typeFromLabelList(id->getLabels(), _currentGenericParams));
            return;
        }
        if (const auto infix = std::dynamic_pointer_cast<ast::InfixExpressionNode>(node))        {
            if (infix->getOpToken().getType() == core::TokenType::TOKEN_COMMA)            {
                collectParamTypesT(infix->getLeftNode(), out);
                collectParamTypesT(infix->getRightNode(), out);
                return;
            }
            if (infix->getOpToken().getType() == core::TokenType::TOKEN_COLON)            {
                out.push_back(typeFromTypeArgNode(infix->getRightNode(), _currentGenericParams));
                return;
            }
            collectParamTypesT(infix->getLeftNode(), out);
            return;
        }
        // `b: int = 2` 在 AST 里是 AssignmentNode(target=Identifier b, value=2)，
        // 形参类型挂在 target 的 label 上，所以递归进 target 取类型。
        if (const auto assign = std::dynamic_pointer_cast<ast::AssignmentNode>(node))        {
            collectParamTypesT(assign->getAssignPair().first, out);
            return;
        }
        if (const auto block = std::dynamic_pointer_cast<ast::BlockRangerNode>(node))        {
            for (const auto& e : block->getBodyExpressions()) collectParamTypesT(e, out);
            return;
        }
        if (const auto paren = std::dynamic_pointer_cast<ast::ParenRangerNode>(node))        {
            collectParamTypesT(paren->getRangerNode(), out);
            return;
        }
        if (const auto list = std::dynamic_pointer_cast<ast::ListExpressionNode>(node))        {
            if (const auto body = std::dynamic_pointer_cast<ast::ExpressionNode>(list->getBodyNode()))            {
                collectParamTypesT(body, out);
            }
            return;
        }
        if (std::dynamic_pointer_cast<ast::ParameterNode>(node))        {
            const std::string text = node->toString();
            const auto colon = text.find(':');
            out.push_back(colon == std::string::npos
                              ? sema::getUnknownType()
                              : resolveTypeName(text.substr(colon + 1)));
        }
    }
    void CodeGen::collectParamAnnotations(const std::shared_ptr<ast::ExpressionNode>& node,
                                          std::vector<std::string>& out) const    {
        // 与 collectParamTypesT 同构，但保留原始文本：空串 = 没标注。
        if (!node) return;
        if (const auto id = std::dynamic_pointer_cast<ast::IdentifierNode>(node))        {
            std::string text;
            for (const auto& label : id->getLabels())            {
                if (!label) continue;
                const auto full = label->getFullLabel();
                if (!full.empty()) { text = full; break; }
            }
            out.push_back(text);
            return;
        }
        if (const auto infix = std::dynamic_pointer_cast<ast::InfixExpressionNode>(node))        {
            if (infix->getOpToken().getType() == core::TokenType::TOKEN_COMMA)            {
                collectParamAnnotations(infix->getLeftNode(), out);
                collectParamAnnotations(infix->getRightNode(), out);
                return;
            }
            if (infix->getOpToken().getType() == core::TokenType::TOKEN_COLON)            {
                out.push_back(infix->getRightNode() ? infix->getRightNode()->toString() : std::string{});
                return;
            }
            collectParamAnnotations(infix->getLeftNode(), out);
            return;
        }
        if (const auto assign = std::dynamic_pointer_cast<ast::AssignmentNode>(node))        {
            collectParamAnnotations(assign->getAssignPair().first, out);
            return;
        }
        if (const auto block = std::dynamic_pointer_cast<ast::BlockRangerNode>(node))        {
            for (const auto& e : block->getBodyExpressions()) collectParamAnnotations(e, out);
            return;
        }
        if (const auto paren = std::dynamic_pointer_cast<ast::ParenRangerNode>(node))        {
            collectParamAnnotations(paren->getRangerNode(), out);
            return;
        }
        if (const auto list = std::dynamic_pointer_cast<ast::ListExpressionNode>(node))        {
            if (const auto body = std::dynamic_pointer_cast<ast::ExpressionNode>(list->getBodyNode()))            {
                collectParamAnnotations(body, out);
            }
            return;
        }
        if (std::dynamic_pointer_cast<ast::ParameterNode>(node))        {
            const std::string text = node->toString();
            const auto colon = text.find(':');
            out.push_back(colon == std::string::npos ? std::string{} : text.substr(colon + 1));
        }
    }
    // ============================================================
    // P10：泛型类实例化
    // ============================================================
    std::vector<sema::TypePtr> CodeGen::typeArgsOf(
        const std::shared_ptr<ast::ExpressionNode>& node) const    {
        std::vector<sema::TypePtr> out;
        if (!node) return out;
        // 形态一：IndexExpressionNode(Identifier Box, Bracket(...))
        if (const auto index = std::dynamic_pointer_cast<ast::IndexExpressionNode>(node))        {
            if (const auto bracket = std::dynamic_pointer_cast<ast::BracketExpressionNode>(index->getIndexNode()))            {
                std::vector<std::shared_ptr<ast::ExpressionNode>> nodes;
                flattenTypeArgs(bracket->getBodyNode(), nodes);
                for (const auto& n : nodes) out.push_back(typeFromTypeArgNode(n, _currentGenericParams));
            }
            return out;
        }
        // 形态二：LabelNode（类型标注里的 `Box[int]`）
        if (const auto label = std::dynamic_pointer_cast<ast::LabelNode>(node))        {
            for (const auto& des : label->getLabelDesNodes())            {
                if (!des) continue;
                std::vector<std::shared_ptr<ast::ExpressionNode>> nodes;
                flattenTypeArgs(des->getBodyNode(), nodes);
                for (const auto& n : nodes) out.push_back(typeFromTypeArgNode(n, _currentGenericParams));
            }
            return out;
        }
        return out;
    }
    std::string CodeGen::ensureClassInstance(const std::string& base,
                                             const std::vector<sema::TypePtr>& args)    {
        const auto tmpl = _genericClasses.find(base);
        if (tmpl == _genericClasses.end()) return std::string();
        const GenericClass& gc = tmpl->second;
        if (args.size() != gc.genericParams.size()) return std::string();
        const std::string mangled = mangleInstance(base, args);
        if (findClass(mangled)) return mangled;
        // 绑定：T -> int 之类
        std::map<std::string, sema::TypePtr> bindings;
        for (size_t i = 0; i < gc.genericParams.size(); ++i)        {
            bindings[gc.genericParams[i]] = args[i];
        }
        const auto savedParams = _currentGenericParams;
        const auto savedBindings = _currentGenericBindings;
        _currentGenericParams = gc.genericParams;
        _currentGenericBindings = bindings;
        // 用模板再登记一次：这一次类名是 mangled 名，字段类型里的 T
        // 会被解析成具体类型（靠 _currentGenericBindings）。
        const auto nameIndex = std::dynamic_pointer_cast<ast::IndexExpressionNode>(gc.def->getNameNode());
        if (!nameIndex)        {
            _currentGenericParams = savedParams;
            _currentGenericBindings = savedBindings;
            return std::string();
        }
        registerClassBody(gc.def, mangled);
        _genericClassInstances.push_back(mangled);
        _genericClassBindings[mangled] = {gc.genericParams, bindings};
        _currentGenericParams = savedParams;
        _currentGenericBindings = savedBindings;
        return mangled;
    }
    void CodeGen::registerClass(const std::shared_ptr<ast::ClassDefinitionNode>& node)    {
        if (!node) return;
        // P10：`class Box[T]` 的类名节点是 IndexExpressionNode(Identifier Box, Bracket([T]))。
        // 这种类**不**在这里登记为可构造的具体类 —— 它只是模板，
        // 真正登记发生在 `Box[int]` 被用到的时候（ensureClassInstance）。
        if (const auto nameIndex = std::dynamic_pointer_cast<ast::IndexExpressionNode>(node->getNameNode()))        {
            const auto id = std::dynamic_pointer_cast<ast::IdentifierNode>(nameIndex->getLeftNode());
            if (!id) return;
            GenericClass gc;
            gc.name = id->getName();
            gc.def = node;
            if (const auto bracket = std::dynamic_pointer_cast<ast::BracketExpressionNode>(nameIndex->getIndexNode()))            {
                std::vector<std::shared_ptr<ast::ExpressionNode>> nodes;
                flattenTypeArgs(bracket->getBodyNode(), nodes);
                for (const auto& n : nodes)                {
                    if (!n) continue;
                    if (const auto pid = std::dynamic_pointer_cast<ast::IdentifierNode>(n))
                        gc.genericParams.push_back(pid->getName());
                    else if (const auto lbl = std::dynamic_pointer_cast<ast::LabelNode>(n))
                        gc.genericParams.push_back(lbl->getFullLabel());
                }
            }
            if (!gc.genericParams.empty())            {
                _genericClasses[gc.name] = std::move(gc);
                return;
            }
        }
        const auto id = std::dynamic_pointer_cast<ast::IdentifierNode>(node->getNameNode());
        if (!id) return;
        registerClassBody(node, id->getName());
    }
    // 用给定的具体类名登记类体。模板实例化（`Box[int]`）也走这里，
    // 区别只在于类名是 mangled 名、且 _currentGenericBindings 已就位。
    void CodeGen::registerClassBody(const std::shared_ptr<ast::ClassDefinitionNode>& node,
                                    const std::string& className)    {
        ClassInfo info;
        info.name = className;
        // 继承：类名标识符上的标签就是基类名（`class Derived: Base`）
        std::shared_ptr<ast::IdentifierNode> id;
        if (const auto nameIndex = std::dynamic_pointer_cast<ast::IndexExpressionNode>(node->getNameNode()))        {
            id = std::dynamic_pointer_cast<ast::IdentifierNode>(nameIndex->getLeftNode());
        }
        else        {
            id = std::dynamic_pointer_cast<ast::IdentifierNode>(node->getNameNode());
        }
        if (id)        {
            for (const auto& label : id->getLabels())            {
                if (!label) continue;
                const std::string full = label->getFullLabel();
                if (!full.empty())                {
                    info.baseName = full;
                    break;
                }
            }
        }
        if (const auto body = std::dynamic_pointer_cast<ast::BlockRangerNode>(node->getBodyNode()))        {
            for (const auto& stmt : body->getBodyExpressions())            {
                if (!stmt) continue;
                // 字段：`var x: int, y: int` 一个节点里可以带多个定义
                if (const auto varDef = std::dynamic_pointer_cast<ast::VariableDefinitionNode>(stmt))                {
                    for (const auto& def : varDef->getVarDefs())                    {
                        if (!def || !def->getNameNode()) continue;
                        const std::string fname = def->getNameNode()->getName();
                        sema::TypePtr ftype = sema::getUnknownType();
                        if (def->hasLabels())                        {
                            ftype = typeFromLabelList(def->getLabelNodes(), _currentGenericParams);
                        }
                        if (!ftype || ftype->getKind() == sema::TypeKind::TK_UNKNOWN) ftype = sema::getIntType();
                        info.ownFieldNames.push_back(fname);
                        info.ownFieldTypes[fname] = ftype;
                        // P12/G11：收集实例字段初始化器，对象创建时写入。
                        // static 字段整体不支持，跳过即可（不影响普通字段）。
                        if (def->hasInitialValue() && def->getValueNode())                        {
                            info.ownFieldInits.emplace_back(fname, def->getValueNode());
                        }
                    }
                    continue;
                }
                // 构造器
                if (const auto ctor = std::dynamic_pointer_cast<ast::ConstructorDefinitionNode>(stmt))                {
                    info.hasCtor = true;
                    info.ctorParamNode = ctor->getParamNode();
                    info.ctorBodyNode = ctor->getBodyNode();
                    continue;
                }
                // 方法：形状与普通函数定义一致，名字在 callNode 里
                if (const auto fn = std::dynamic_pointer_cast<ast::FunctionDefinitionNode>(stmt))                {
                    std::string mname;
                    if (const auto cid = std::dynamic_pointer_cast<ast::IdentifierNode>(fn->getCallNode()))                    {
                        mname = cid->getName();
                    }
                    else if (const auto c = std::dynamic_pointer_cast<ast::FunctionCallNode>(fn->getCallNode()))                    {
                        if (const auto lid = std::dynamic_pointer_cast<ast::IdentifierNode>(c->getLeftNode()))                        {
                            mname = lid->getName();
                        }
                    }
                    if (!mname.empty())                    {
                        // P12/G5：方法名可以重载 —— 全部收进重载集，
                        // 单定义时 methods 仍指向那一份（保持裸名）。
                        info.methodOverloads[mname].push_back(fn);
                        info.methods[mname] = fn;
                    }
                }
            }
        }
        _classes[info.name] = info;
    }
    void CodeGen::computeLayout(const std::string& name)    {
        ClassInfo* cls = findClass(name);
        if (!cls) return;
        if (cls->layoutDone) return;
        // 先置位再递归，继承环（非法输入）不会导致无限递归。
        cls->layoutDone = true;
        long long offset = 0;
        if (!cls->baseName.empty())        {
            computeLayout(cls->baseName);
            if (const auto base = findClass(cls->baseName))            {
                cls->fieldOrder = base->fieldOrder;
                cls->fieldOffsets = base->fieldOffsets;
                cls->fieldTypes = base->fieldTypes;
                offset = base->size;
            }
        }
        for (const auto& fname : cls->ownFieldNames)        {
            cls->fieldOffsets[fname] = offset;
            cls->fieldTypes[fname] = cls->ownFieldTypes[fname];
            cls->fieldOrder.push_back(fname);
            offset += 8;
        }
        cls->size = offset;
    }
    std::map<std::string, CodeGen::MethodRef> CodeGen::collectMethods(const std::string& name) const    {
        std::map<std::string, MethodRef> out;
        // 沿继承链往上走，再从最顶层的基类往下覆盖，派生类同名方法胜出。
        std::vector<std::string> chain;
        std::string cur = name;
        size_t guard = 0;
        while (!cur.empty() && guard++ < 64)        {
            chain.push_back(cur);
            const auto cls = findClass(cur);
            if (!cls) break;
            cur = cls->baseName;
        }
        for (auto it = chain.rbegin(); it != chain.rend(); ++it)        {
            const auto cls = findClass(*it);
            if (!cls) continue;
            for (const auto& [mname, def] : cls->methods)            {
                out[mname] = MethodRef{*it, def};
            }
        }
        return out;
    }
    std::vector<CodeGen::MethodRef> CodeGen::collectMethodOverloads(const std::string& name,
                                                                    const std::string& method) const    {
        // P12/G5：收集「这个类 + 这个方法名」的全部定义（沿继承链）。
        // 派生类声明了同名方法就**整体遮蔽**基类的同名重载集 —— 与 Java/C++
        // 的「名字隐藏」一致，且不会出现「派生类一份 + 基类两份」的怪异组合。
        std::vector<std::string> chain;
        std::string cur = name;
        size_t guard = 0;
        while (!cur.empty() && guard++ < 64)        {
            chain.push_back(cur);
            const auto cls = findClass(cur);
            if (!cls) break;
            cur = cls->baseName;
        }
        for (const auto& clsName : chain)        {
            const auto cls = findClass(clsName);
            if (!cls) continue;
            const auto found = cls->methodOverloads.find(method);
            if (found == cls->methodOverloads.end() || found->second.empty()) continue;
            std::vector<MethodRef> out;
            for (const auto& def : found->second) out.push_back(MethodRef{clsName, def});
            return out;
        }
        return {};
    }
    long long CodeGen::findFieldOffset(const std::string& className, const std::string& field) const    {
        std::string cur = className;
        size_t guard = 0;
        while (!cur.empty() && guard++ < 64)        {
            const auto cls = findClass(cur);
            if (!cls) return -1;
            const auto it = cls->fieldOffsets.find(field);
            if (it != cls->fieldOffsets.end()) return it->second;
            cur = cls->baseName;
        }
        return -1;
    }
    sema::TypePtr CodeGen::findFieldType(const std::string& className, const std::string& field) const    {
        std::string cur = className;
        size_t guard = 0;
        while (!cur.empty() && guard++ < 64)        {
            const auto cls = findClass(cur);
            if (!cls) return nullptr;
            const auto it = cls->fieldTypes.find(field);
            if (it != cls->fieldTypes.end()) return it->second;
            cur = cls->baseName;
        }
        return nullptr;
    }
    std::string CodeGen::classNameOf(const std::shared_ptr<ast::ExpressionNode>& node) const    {
        if (!node) return {};
        // 注意：FunctionCallNode 是 InfixExpressionNode 的子类，必须排在前面
        if (const auto id = std::dynamic_pointer_cast<ast::IdentifierNode>(node))        {
            const auto slot = const_cast<CodeGen*>(this)->lookupVar(id->getName());
            if (slot && slot->type && slot->type->getKind() == sema::TypeKind::TK_CLASS)            {
                return std::static_pointer_cast<sema::ClassType>(slot->type)->getName();
            }
            return {};
        }
        if (const auto call = std::dynamic_pointer_cast<ast::FunctionCallNode>(node))        {
            // 构造调用 `Point(...)` 的结果就是 Point 对象
            if (const auto cid = std::dynamic_pointer_cast<ast::IdentifierNode>(call->getLeftNode()))            {
                if (findClass(cid->getName())) return cid->getName();
            }
            // P10：`Box[int](1)` —— callee 是 IndexExpressionNode
            if (const auto index = std::dynamic_pointer_cast<ast::IndexExpressionNode>(call->getLeftNode()))            {
                if (const auto cid = std::dynamic_pointer_cast<ast::IdentifierNode>(index->getLeftNode()))                {
                    if (_genericClasses.find(cid->getName()) != _genericClasses.end())                    {
                        const auto typeArgs = typeArgsOf(index);
                        auto* self = const_cast<CodeGen*>(this);
                        const std::string instName = self->ensureClassInstance(cid->getName(), typeArgs);
                        if (!instName.empty()) return instName;
                    }
                }
            }
            return {};
        }
        if (const auto infix = std::dynamic_pointer_cast<ast::InfixExpressionNode>(node))        {
            if (infix->getOpToken().getType() == core::TokenType::TOKEN_DOT)            {
                const auto fname = std::dynamic_pointer_cast<ast::IdentifierNode>(infix->getRightNode());
                if (!fname) return {};
                const std::string owner = classNameOf(infix->getLeftNode());
                if (owner.empty()) return {};
                const auto ft = findFieldType(owner, fname->getName());
                if (ft && ft->getKind() == sema::TypeKind::TK_CLASS)                {
                    return std::static_pointer_cast<sema::ClassType>(ft)->getName();
                }
            }
            return {};
        }
        if (const auto paren = std::dynamic_pointer_cast<ast::ParenRangerNode>(node))        {
            return classNameOf(paren->getRangerNode());
        }
        return {};
    }
    sema::TypePtr CodeGen::methodReturnType(const std::shared_ptr<ast::FunctionDefinitionNode>& def,
                                            const std::vector<std::string>& paramNames,
                                            const std::vector<sema::TypePtr>& paramTypes,
                                            const std::string& ownerClass) const    {
        if (!def) return sema::getIntType();
        // 声明标签优先（能解析出类名），否则从函数体推断，最后兜底 int。
        // P10：必须走 typeFromLabelList —— 泛型实例的返回标注是 `T`，
        // 裸 getFullLabel() 解析不出来，会静默退化成 int（float 返回值
        // 就被当成整数打印出位模式）。
        // 泛型实例的方法在**调用点**解析返回类型时，_currentGenericBindings
        // 早已恢复成空（调用点可能在任何地方），所以按 owner 类名把绑定
        // 重新装回去再解析。
        const auto savedParams = _currentGenericParams;
        const auto savedBindings = _currentGenericBindings;
        auto* self = const_cast<CodeGen*>(this);
        if (!ownerClass.empty())        {
            if (const auto b = _genericClassBindings.find(ownerClass);
                b != _genericClassBindings.end())            {
                self->_currentGenericParams = b->second.first;
                self->_currentGenericBindings = b->second.second;
            }
        }
        const auto annotated = typeFromLabelList(def->getLabelNodes(), _currentGenericParams);
        self->_currentGenericParams = savedParams;
        self->_currentGenericBindings = savedBindings;
        if (annotated && annotated->getKind() != sema::TypeKind::TK_UNKNOWN) return annotated;
        const auto inferred = const_cast<CodeGen*>(this)->inferAnonReturnType(def->getBodyNode(), paramNames, paramTypes);
        if (inferred && inferred->getKind() != sema::TypeKind::TK_UNKNOWN) return inferred;
        return sema::getIntType();
    }
    void CodeGen::emitGenericClassBodies()    {
        // 实例可能在发射别的实例体时被发现，所以循环到不再新增为止。
        // 已发射过的实例名记在 _genericClassEmitted 里避免重复。
        size_t cursor = 0;
        while (cursor < _genericClassInstances.size())        {
            if (hasFailed()) return;
            const std::string instName = _genericClassInstances[cursor++];
            if (_genericClassEmitted.find(instName) != _genericClassEmitted.end()) continue;
            _genericClassEmitted[instName] = true;
            if (!findClass(instName)) continue;
            const auto savedParams = _currentGenericParams;
            const auto savedBindings = _currentGenericBindings;
            // 恢复该实例的绑定，方法体里的 T 才能解析成具体类型。
            if (const auto b = _genericClassBindings.find(instName); b != _genericClassBindings.end())            {
                _currentGenericParams = b->second.first;
                _currentGenericBindings = b->second.second;
            }
            emitClassBodies(instName);
            _currentGenericParams = savedParams;
            _currentGenericBindings = savedBindings;
        }
    }
    void CodeGen::emitClassBodies(const std::string& name)    {
        ClassInfo* cls = findClass(name);
        if (!cls) return;
        const std::string savedClass = _currentClass;
        _currentClass = name;
        // 构造器：符号 <Class>.__init__，隐藏 this 在 %rcx，返回 this。
        if (cls->hasCtor)        {
            std::vector<std::string> paramNames;
            collectParamNamesT(cls->ctorParamNode, paramNames);
            std::vector<sema::TypePtr> paramTypes;
            collectParamTypesT(cls->ctorParamNode, paramTypes);
            while (paramTypes.size() < paramNames.size()) paramTypes.push_back(sema::getIntType());
            // P9：构造器形参同样记录「是否标注」，供调用点校验实参。
            std::vector<std::string> ctorAnn;
            collectParamAnnotations(cls->ctorParamNode, ctorAnn);
            std::vector<bool> ctorMask(paramTypes.size(), false);
            for (size_t i = 0; i < ctorMask.size() && i < ctorAnn.size(); ++i)
                ctorMask[i] = !ctorAnn[i].empty();
            _functionParamAnnotated[name + ".__init__"] = ctorMask;
            for (auto& t : paramTypes)            {
                if (!t || t->getKind() == sema::TypeKind::TK_UNKNOWN) t = sema::getIntType();
            }
            emitFunctionBody(name + ".__init__", paramNames, paramTypes,
                             classTypeOf(name), cls->ctorBodyNode, true,
                             {}, {}, "this", true);
        }
        // 方法：符号 <Class>.<method>，同样带隐藏 this。
        // P12/G5：同名方法可能有多个定义（重载）。≥2 个定义时按与全局函数
        // 相同的规则修饰成 `<Class>.<method>_R<len><形参编码>`，单定义保持裸名。
        for (const auto& [mname, defs] : cls->methodOverloads)        {
          for (const auto& def : defs)        {
            std::vector<std::string> paramNames;
            std::vector<sema::TypePtr> paramTypes;
            if (def && def->getCallNode())            {
                if (const auto call = std::dynamic_pointer_cast<ast::FunctionCallNode>(def->getCallNode()))                {
                    if (const auto argsNode = std::dynamic_pointer_cast<ast::ExpressionNode>(call->getRightNode()))                    {
                        collectParamNamesT(argsNode, paramNames);
                        collectParamTypesT(argsNode, paramTypes);
                    }
                }
            }
            while (paramTypes.size() < paramNames.size()) paramTypes.push_back(sema::getIntType());
            // P9：方法形参同样记录标注掩码（键与符号名一致）。
            std::vector<std::string> methAnn;
            if (def && def->getCallNode())            {
                if (const auto call = std::dynamic_pointer_cast<ast::FunctionCallNode>(def->getCallNode()))                {
                    if (const auto argsNode = std::dynamic_pointer_cast<ast::ExpressionNode>(call->getRightNode()))
                        collectParamAnnotations(argsNode, methAnn);
                }
            }
            std::vector<bool> methMask(paramTypes.size(), false);
            for (size_t i = 0; i < methMask.size() && i < methAnn.size(); ++i)
                methMask[i] = !methAnn[i].empty();
            _functionParamAnnotated[name + "." + mname] = methMask;
            for (auto& t : paramTypes)            {
                if (!t || t->getKind() == sema::TypeKind::TK_UNKNOWN) t = sema::getIntType();
            }
            const auto ret = methodReturnType(def, paramNames, paramTypes);
            // P12/G5：重载方法用修饰名发射；单定义保持裸名（与全局函数一致）。
            std::string methodSymbolName = name + "." + mname;
            if (defs.size() >= 2)            {
                methodSymbolName = mangleOverload(name + "." + mname, paramTypes);
            }
            emitFunctionBody(methodSymbolName, paramNames, paramTypes, ret,
                             def->getBodyNode(), true, {}, {}, "", true);
          }
        }
        _currentClass = savedClass;
    }
    void CodeGen::emitAttribute(const std::shared_ptr<ast::ExpressionNode>& node)    {
        const auto infix = std::dynamic_pointer_cast<ast::InfixExpressionNode>(node);
        if (!infix)        {
            fail("Not an attribute expression: " + node->toString());
            return;
        }
        const auto fieldId = std::dynamic_pointer_cast<ast::IdentifierNode>(infix->getRightNode());
        if (!fieldId)        {
            fail("Attribute name must be an identifier: " + node->toString());
            return;
        }
        const std::string owner = classNameOf(infix->getLeftNode());
        if (owner.empty())        {
            // P8：`mod.SOMETHING` 不是方法调用。模块在加载期已内联，
            // 顶层 `var` 会落进 __rio_main 的栈帧，因此模块级变量没法
            // 通过限定名读到 —— 这里给一条能看懂的诊断，而不是让它
            // 落到「推不出对象类型」这种泛泛的报错上。
            if (const auto modId = std::dynamic_pointer_cast<ast::IdentifierNode>(infix->getLeftNode());
                modId && isModuleName(modId->getName()))            {
                std::string memberName = nodeText(infix->getRightNode());
                if (const auto memberId =
                        std::dynamic_pointer_cast<ast::IdentifierNode>(infix->getRightNode()))
                {
                    memberName = memberId->getName();
                }
                fail("Module member access is not supported: '" + modId->getName() + "." +
                    memberName +
                    "'. Modules are inlined at compile time; expose module-level state through "
                    "a function instead.");
                return;
            }
            fail("Cannot determine the object type of attribute access: " + node->toString());
            return;
        }
        const long long offset = findFieldOffset(owner, fieldId->getName());
        if (offset < 0)        {
            fail("Class " + owner + " has no field named '" + fieldId->getName() + "'");
            return;
        }
        emitExpression(infix->getLeftNode());
        if (hasFailed()) return;
        // 左边求值后 %rax 就是对象指针。fieldOffsets 里存的已经是**字节**偏移
        // （computeLayout 里每个字段 +8），这里不能再乘 8。
        const std::string addr = std::to_string(offset) + "(%rax)";
        const auto ftype = findFieldType(owner, fieldId->getName());
        if (ftype && ftype->isFloat())        {
            _asm.inst("movsd", addr, "%xmm0");
        }
        _asm.mov(addr, "%rax");
    }
    void CodeGen::emitAttributeAssign(const std::shared_ptr<ast::InfixExpressionNode>& target,
                                      const std::shared_ptr<ast::ExpressionNode>& value)    {
        const auto fieldId = std::dynamic_pointer_cast<ast::IdentifierNode>(target->getRightNode());
        if (!fieldId)        {
            fail("Attribute name must be an identifier: " + target->toString());
            return;
        }
        const std::string owner = classNameOf(target->getLeftNode());
        if (owner.empty())        {
            fail("Cannot determine the object type of attribute assignment: " + target->toString());
            return;
        }
        const long long offset = findFieldOffset(owner, fieldId->getName());
        if (offset < 0)        {
            fail("Class " + owner + " has no field named '" + fieldId->getName() + "'");
            return;
        }
        const auto ftype = findFieldType(owner, fieldId->getName());
        const bool destIsFloat = ftype && ftype->isFloat();
        // 先算值并暂存到栈上，再算对象指针 —— 两边都可能包含调用，
        // 而调用会冲掉所有参数寄存器，所以顺序不能颠倒。
        emitExpression(value);
        if (hasFailed()) return;
        const auto vtype = inferType(value);
        const bool srcIsFloat = vtype && vtype->isFloat();
        if (destIsFloat && !srcIsFloat)        {
            _asm.inst("cvtsi2sdq", "%rax", "%xmm0");
        }
        else if (!destIsFloat && srcIsFloat)        {
            _asm.inst("cvttsd2siq", "%xmm0", "%rax");
        }
        if (destIsFloat)        {
            _asm.subRsp(8);
            _asm.inst("movsd", "%xmm0", "(%rsp)");
        }
        else        {
            _asm.push("%rax");
        }
        emitExpression(target->getLeftNode());
        if (hasFailed())        {
            _asm.addRsp(8);
            return;
        }
        _asm.mov("%rax", "%r10");
        const std::string addr = std::to_string(offset) + "(%r10)";
        if (destIsFloat)        {
            _asm.inst("movsd", "(%rsp)", "%xmm0");
            _asm.addRsp(8);
            _asm.inst("movsd", "%xmm0", addr);
        }
        else        {
            _asm.pop("%rax");
            _asm.mov("%rax", addr);
        }
    }
    void CodeGen::emitConstructorCall(const std::string& className,
                                      const std::shared_ptr<ast::ExpressionNode>& callNode,
                                      const std::vector<std::shared_ptr<ast::ExpressionNode>>& args)    {
        (void)callNode;
        ClassInfo* cls = findClass(className);
        if (!cls)        {
            fail("Unknown class: " + className);
            return;
        }
        // P10：泛型实例的构造器形参标注是模板里的 `a: A, b: B`。收集它们
        // 时必须用**目标实例**的绑定，否则 `Pair[B, A](...)` 在 `Pair[A, B]`
        // 的方法体里会按外层绑定解析，实参校验全错位。
        const auto savedParams = _currentGenericParams;
        const auto savedBindings = _currentGenericBindings;
        if (const auto b = _genericClassBindings.find(className); b != _genericClassBindings.end())        {
            _currentGenericParams = b->second.first;
            _currentGenericBindings = b->second.second;
        }
        const long long size = cls->size > 0 ? cls->size : 8;
        emitAlloc(static_cast<size_t>(size));
        if (hasFailed()) return;
        // 新对象先整体清零：没赋值的字段默认是 0。
        _asm.push("%rax");
        _asm.mov("%rax", "%rcx");
        _asm.movImm(0, "%rdx");
        _asm.movImm(size, "%r8");
        _asm.subRsp(32);
        _asm.call("core_mem_set");
        _asm.addRsp(32);
        _asm.pop("%rax");
        // P12/G11：写入实例字段初始化器（`var x: int = 42`）。
        // 必须在用户构造器**之前**、清零**之后**：ctor 里可以覆盖这些初值。
        // 顺序按声明顺序，与源码直觉一致。
        // 对象指针先落到栈槽 —— 求值初始化表达式会用到 %rax 和 caller-saved
        // 寄存器，指针留在寄存器里会被冲掉。
        if (!cls->ownFieldInits.empty() || !cls->baseName.empty())        {
            // 沿基类链收集：基类在前（与字段布局一致），派生类在后。
            // 只遍历自己会让 `class Derived: Base` 的实例丢掉基类字段初值。
            std::vector<const ClassInfo*> chain;
            for (const ClassInfo* c = cls; c != nullptr; )            {
                chain.push_back(c);
                c = c->baseName.empty() ? nullptr : findClass(c->baseName);
            }
            std::reverse(chain.begin(), chain.end());            const std::string selfSlot = allocateTemp(sema::getPtrType());
            _asm.mov("%rax", selfSlot);
            for (const ClassInfo* c : chain)            {
                for (const auto& [fname, initNode] : c->ownFieldInits)                {
                    // 偏移与类型必须用**最派生**类的完整布局查：
                    // 基类字段的偏移在派生类里可能因对齐而变化。
                    const auto offIt = cls->fieldOffsets.find(fname);
                    if (offIt == cls->fieldOffsets.end()) continue;
                    const long long off = offIt->second;
                    const auto ftype = cls->fieldTypes.count(fname) ? cls->fieldTypes[fname] : sema::getIntType();
                    emitExpression(initNode);
                    if (hasFailed()) return;
                    if (ftype && ftype->isFloat())                    {
                        _asm.mov(selfSlot, "%rcx");
                        _asm.inst("movsd", "%xmm0", std::to_string(off) + "(%rcx)");
                    }
                    else                    {
                        _asm.mov(selfSlot, "%rcx");
                        _asm.mov("%rax", std::to_string(off) + "(%rcx)");
                    }
                }
            }
            _asm.mov(selfSlot, "%rax");
        }
        std::vector<std::string> paramNames;
        collectParamNamesT(cls->ctorParamNode, paramNames);
        std::vector<sema::TypePtr> paramTypes;
        collectParamTypesT(cls->ctorParamNode, paramTypes);
        while (paramTypes.size() < paramNames.size()) paramTypes.push_back(sema::getIntType());
        // 对象指针占掉隐藏 this（%rcx），用户实参从 %rdx 起排。
        _asm.push("%rax");
        const std::vector<bool>* ctorMask = nullptr;
        if (const auto found = _functionParamAnnotated.find(className + ".__init__");
            found != _functionParamAnnotated.end())
            ctorMask = &found->second;
        emitArgsToRegisters(args, className, paramTypes, 1, 0, ctorMask);
        if (hasFailed())        {
            _asm.addRsp(8);
            _currentGenericParams = savedParams;
            _currentGenericBindings = savedBindings;
            return;
        }
        _asm.pop("%rcx");
        _currentGenericParams = savedParams;
        _currentGenericBindings = savedBindings;
        if (!cls->hasCtor)        {
            // 没有构造器：对象就是结果，字段保持全 0。
            _asm.mov("%rcx", "%rax");
            return;
        }
        _asm.subRsp(32);
        _asm.call(className + ".__init__");
        _asm.addRsp(32);
    }
    void CodeGen::emitMethodCall(const std::shared_ptr<ast::InfixExpressionNode>& member,
                                 const std::shared_ptr<ast::ExpressionNode>& callNode,
                                 const std::vector<std::shared_ptr<ast::ExpressionNode>>& args)    {
        (void)callNode;
        const auto methodId = std::dynamic_pointer_cast<ast::IdentifierNode>(member->getRightNode());
        if (!methodId)        {
            fail("Method name must be an identifier: " + member->toString());
            return;
        }
        const std::string owner = classNameOf(member->getLeftNode());
        if (owner.empty())        {
            fail("Cannot determine the object type of method call: " + member->toString());
            return;
        }
        const auto methods = collectMethods(owner);
        const auto it = methods.find(methodId->getName());
        if (it == methods.end())        {
            fail("Class " + owner + " has no method named '" + methodId->getName() + "'");
            return;
        }
        std::string symbol = it->second.owner + "." + methodId->getName();
        std::vector<std::string> paramNames;
        std::vector<sema::TypePtr> paramTypes;
        if (it->second.def && it->second.def->getCallNode())        {
            if (const auto call = std::dynamic_pointer_cast<ast::FunctionCallNode>(it->second.def->getCallNode()))            {
                if (const auto argsNode = std::dynamic_pointer_cast<ast::ExpressionNode>(call->getRightNode()))                {
                    collectParamNamesT(argsNode, paramNames);
                    collectParamTypesT(argsNode, paramTypes);
                }
            }
        }
        while (paramTypes.size() < paramNames.size()) paramTypes.push_back(sema::getIntType());
        // P12/G5：方法重载决议。
        // 判据与全局函数完全一致（P11）：唯一名字保持裸名，≥2 个定义才修饰。
        // 修饰名里带类名，避免两个类的方法互相撞符号。
        if (const auto overloads = collectMethodOverloads(owner, methodId->getName());
            overloads.size() >= 2)        {
            // 按实参类型打分，取最优；同分取先声明的（与全局重载一致）。
            std::vector<sema::TypePtr> actualTypes;
            for (const auto& a : args) actualTypes.push_back(inferType(a));
            int bestScore = -1;
            const MethodRef* best = nullptr;
            for (const auto& cand : overloads)            {
                std::vector<std::string> candNames;
                std::vector<sema::TypePtr> candTypes;
                if (cand.def && cand.def->getCallNode())                {
                    if (const auto call = std::dynamic_pointer_cast<ast::FunctionCallNode>(cand.def->getCallNode()))                    {
                        if (const auto argsNode = std::dynamic_pointer_cast<ast::ExpressionNode>(call->getRightNode()))                        {
                            collectParamNamesT(argsNode, candNames);
                            collectParamTypesT(argsNode, candTypes);
                        }
                    }
                }
                while (candTypes.size() < candNames.size()) candTypes.push_back(sema::getIntType());
                // 形参个数必须匹配（本实现不做可变参数 / 缺省实参的方法）
                if (candTypes.size() != actualTypes.size()) continue;
                int score = 0;
                bool ok = true;
                for (size_t i = 0; i < actualTypes.size(); ++i)                {
                    const int s = overloadScore(candTypes[i], actualTypes[i], true);
                    if (s < 0) { ok = false; break; }
                    score += s;
                }
                if (!ok) continue;
                if (score > bestScore)                {
                    bestScore = score;
                    best = &cand;
                }
            }
            if (!best)            {
                failAt(member->getPos(), "No matching overload for method '" + methodId->getName() +
                       "' of class " + owner);
                return;
            }
            // 用与全局函数同一套修饰规则生成符号（`_R<len><base><形参编码>`）。
            std::vector<sema::TypePtr> bestTypes;
            if (best->def && best->def->getCallNode())            {
                if (const auto call = std::dynamic_pointer_cast<ast::FunctionCallNode>(best->def->getCallNode()))                {
                    if (const auto argsNode = std::dynamic_pointer_cast<ast::ExpressionNode>(call->getRightNode()))
                        collectParamTypesT(argsNode, bestTypes);
                }
            }
            symbol = mangleOverload(best->owner + "." + methodId->getName(), bestTypes);
            // 形参类型/名字换成被选中那一份的
            paramNames.clear();
            paramTypes.clear();
            if (best->def && best->def->getCallNode())            {
                if (const auto call = std::dynamic_pointer_cast<ast::FunctionCallNode>(best->def->getCallNode()))                {
                    if (const auto argsNode = std::dynamic_pointer_cast<ast::ExpressionNode>(call->getRightNode()))                    {
                        collectParamNamesT(argsNode, paramNames);
                        collectParamTypesT(argsNode, paramTypes);
                    }
                }
            }
            while (paramTypes.size() < paramNames.size()) paramTypes.push_back(sema::getIntType());
        }
        // 接收者先求值压栈（实参求值可能包含调用），最后落到 %rcx。
        emitExpression(member->getLeftNode());
        if (hasFailed()) return;
        _asm.push("%rax");
        const std::vector<bool>* methMask = nullptr;
        if (const auto found = _functionParamAnnotated.find(symbol); found != _functionParamAnnotated.end())
            methMask = &found->second;
        emitArgsToRegisters(args, symbol, paramTypes, 1, 0, methMask);
        if (hasFailed())        {
            _asm.addRsp(8);
            return;
        }
        _asm.pop("%rcx");
        _asm.subRsp(32);
        _asm.call(symbol);
        _asm.addRsp(32);
    }
    void CodeGen::emitSuperCall(const std::vector<std::shared_ptr<ast::ExpressionNode>>& args)    {
        if (_currentClass.empty())        {
            fail("super() can only be used inside a class constructor or method");
            return;
        }
        const ClassInfo* cls = findClass(_currentClass);
        if (!cls || cls->baseName.empty())        {
            fail("super() used in class " + _currentClass + ", which has no base class");
            return;
        }
        const auto thisSlot = lookupVar("this");
        if (!thisSlot)        {
            fail("super() requires 'this'");
            return;
        }
        const std::string baseName = cls->baseName;
        const ClassInfo* base = findClass(baseName);
        if (!base)        {
            fail("Unknown base class: " + baseName);
            return;
        }
        std::vector<std::string> paramNames;
        collectParamNamesT(base->ctorParamNode, paramNames);
        std::vector<sema::TypePtr> paramTypes;
        collectParamTypesT(base->ctorParamNode, paramTypes);
        while (paramTypes.size() < paramNames.size()) paramTypes.push_back(sema::getIntType());
        // 复用当前 this：派生类对象的前缀与基类布局完全一致，
        // 所以直接把同一个指针交给基类构造器就行。
        _asm.mov(addrOf(*thisSlot), "%rax");
        _asm.push("%rax");
        const std::vector<bool>* superMask = nullptr;
        if (const auto found = _functionParamAnnotated.find(baseName + ".__init__");
            found != _functionParamAnnotated.end())
            superMask = &found->second;
        emitArgsToRegisters(args, baseName + ".__init__", paramTypes, 1, 0, superMask);
        if (hasFailed())        {
            _asm.addRsp(8);
            return;
        }
        _asm.pop("%rcx");
        if (!base->hasCtor) return;
        _asm.subRsp(32);
        _asm.call(baseName + ".__init__");
        _asm.addRsp(32);
    }
    // ============================================================
    // P10：泛型（单态化）
    // ============================================================
    std::vector<std::string> CodeGen::collectGenericParams(
        const std::shared_ptr<ast::ExpressionNode>& callNode)    {
        std::vector<std::string> out;
        if (!callNode) return out;
        // `fun f[T](...)` 的 callNode 是
        //   FunctionCallNode(left = IndexExpressionNode(Identifier f, Bracket(...)), right = Paren)
        const auto call = std::dynamic_pointer_cast<ast::FunctionCallNode>(callNode);
        if (!call) return out;
        const auto index = std::dynamic_pointer_cast<ast::IndexExpressionNode>(call->getLeftNode());
        if (!index) return out;
        const auto bracket = std::dynamic_pointer_cast<ast::BracketExpressionNode>(index->getIndexNode());
        if (!bracket) return out;
        std::vector<std::shared_ptr<ast::ExpressionNode>> nodes;
        flattenTypeArgs(bracket->getBodyNode(), nodes);
        for (const auto& n : nodes)        {
            if (!n) continue;
            if (const auto id = std::dynamic_pointer_cast<ast::IdentifierNode>(n))            {
                out.push_back(id->getName());
            }
            else if (const auto lbl = std::dynamic_pointer_cast<ast::LabelNode>(n))            {
                out.push_back(lbl->getFullLabel());
            }
        }
        return out;
    }
    std::vector<sema::TypePtr> CodeGen::explicitTypeArgs(
        const std::shared_ptr<ast::ExpressionNode>& callee) const    {
        std::vector<sema::TypePtr> out;
        // `f[int](...)` 的 callee 是 IndexExpressionNode(Identifier f, Bracket(int))
        const auto index = std::dynamic_pointer_cast<ast::IndexExpressionNode>(callee);
        if (!index) return out;
        const auto bracket = std::dynamic_pointer_cast<ast::BracketExpressionNode>(index->getIndexNode());
        if (!bracket) return out;
        std::vector<std::shared_ptr<ast::ExpressionNode>> nodes;
        flattenTypeArgs(bracket->getBodyNode(), nodes);
        for (const auto& n : nodes)        {
            if (!n) continue;
            out.push_back(typeFromTypeArgNode(n, {}));
        }
        return out;
    }
    bool CodeGen::deduceTypeArgs(const GenericFunction& generic,
                                 const std::vector<sema::TypePtr>& explicitArgs,
                                 const std::vector<std::shared_ptr<ast::ExpressionNode>>& args,
                                 std::map<std::string, sema::TypePtr>& bindings) const    {
        // 1) 显式实参优先：`identity[int](42)`
        //    显式给出的个数必须与泛型形参个数一致 —— `pair[int](1, "x")`
        //    少给一个 B 时不能靠推导悄悄补上，那样写错个数永远发现不了。
        if (!explicitArgs.empty() && explicitArgs.size() != generic.genericParams.size())        {
            return false;
        }
        for (size_t i = 0; i < generic.genericParams.size() && i < explicitArgs.size(); ++i)        {
            const auto& t = explicitArgs[i];
            if (t && t->getKind() != sema::TypeKind::TK_UNKNOWN)            {
                bindings[generic.genericParams[i]] = t;
            }
        }
        // 2) 剩下没绑定的，从形参类型与实参类型的对应关系里推。
        //    形参是 T        -> 绑定到实参类型
        //    形参是 list[T]  -> 绑定到实参的元素类型
        //    形参是 dict[K,V]-> 分别绑定
        for (size_t i = 0; i < generic.paramTypes.size() && i < args.size(); ++i)        {
            const auto& formal = generic.paramTypes[i];
            if (!formal) continue;
            auto actual = inferType(args[i]);
            // 实参是具名函数（`map2(addOne, 41)`）：inferType 对标识符只会
            // 返回变量槽类型，取不到函数签名。这里按已登记的签名补出来，
            // 否则 funi[A][B] 的 B 永远推不出来。
            if (formal->getKind() == sema::TypeKind::TK_FUNC)            {
                if (const auto fnId = std::dynamic_pointer_cast<ast::IdentifierNode>(args[i]))                {
                    const auto paramsIt = _functionParams.find(fnId->getName());
                    const auto retIt = _functionReturns.find(fnId->getName());
                    if (paramsIt != _functionParams.end() && retIt != _functionReturns.end())                    {
                        actual = std::make_shared<sema::FunctionType>(paramsIt->second, retIt->second);
                    }
                }
            }
            if (!actual || actual->getKind() == sema::TypeKind::TK_UNKNOWN) continue;
            std::function<void(const sema::TypePtr&, const sema::TypePtr&)> unify =
                [&](const sema::TypePtr& f, const sema::TypePtr& a)            {
                    if (!f || !a) return;
                    if (f->getKind() == sema::TypeKind::TK_GENERIC)                {
                        const auto name = std::static_pointer_cast<sema::GenericType>(f)->getName();
                        if (bindings.find(name) == bindings.end()) bindings[name] = a;
                        return;
                    }
                    if (f->getKind() == sema::TypeKind::TK_LIST &&
                        a->getKind() == sema::TypeKind::TK_LIST)                {
                        unify(std::static_pointer_cast<sema::ListType>(f)->getElemType(),
                              std::static_pointer_cast<sema::ListType>(a)->getElemType());
                        return;
                    }
                    if (f->getKind() == sema::TypeKind::TK_DICT &&
                        a->getKind() == sema::TypeKind::TK_DICT)                {
                        const auto fd = std::static_pointer_cast<sema::DictType>(f);
                        const auto ad = std::static_pointer_cast<sema::DictType>(a);
                        unify(fd->getKeyType(), ad->getKeyType());
                        unify(fd->getValueType(), ad->getValueType());
                        return;
                    }
                    // `funi[A][B]` 的形参里带泛型形参时，从实参函数类型推。
                    // 注意实参可能只是「参数个数对得上」的裸函数名，
                    // 参数类型本身也可能是泛型（泛型传泛型）。
                    if (f->getKind() == sema::TypeKind::TK_FUNC &&
                        a->getKind() == sema::TypeKind::TK_FUNC)                {
                        const auto ff = std::static_pointer_cast<sema::FunctionType>(f);
                        const auto af = std::static_pointer_cast<sema::FunctionType>(a);
                        const auto& fp = ff->getParams();
                        const auto& ap = af->getParams();
                        for (size_t k = 0; k < fp.size() && k < ap.size(); ++k) unify(fp[k], ap[k]);
                        unify(ff->getReturnType(), af->getReturnType());
                        return;
                    }
                };
            unify(formal, actual);
        }
        // 3) 还有没绑定的泛型形参：无法单态化，报错。
        for (const auto& g : generic.genericParams)        {
            if (bindings.find(g) == bindings.end()) return false;
        }
        return true;
    }
    std::string CodeGen::instantiateGeneric(const std::string& name,
                                            const std::map<std::string, sema::TypePtr>& bindings)    {
        const auto found = _genericFunctions.find(name);
        if (found == _genericFunctions.end()) return std::string();
        const GenericFunction& generic = found->second;
        std::vector<sema::TypePtr> orderedArgs;
        for (const auto& g : generic.genericParams)        {
            const auto b = bindings.find(g);
            if (b == bindings.end() || !b->second) return std::string();
            orderedArgs.push_back(b->second);
        }
        const std::string mangled = mangleInstance(name, orderedArgs);
        if (_genericInstances.find(mangled) != _genericInstances.end()) return mangled;
        _genericInstances[mangled] = true;
        // 具体化签名
        std::vector<sema::TypePtr> paramTypes;
        for (const auto& pt : generic.paramTypes)        {
            auto t = substituteGenerics(pt, bindings);
            if (!t || t->getKind() == sema::TypeKind::TK_UNKNOWN ||
                t->getKind() == sema::TypeKind::TK_GENERIC)
                t = sema::getIntType();
            paramTypes.push_back(t);
        }
        auto retType = substituteGenerics(generic.retType, bindings);
        if (!retType || retType->getKind() == sema::TypeKind::TK_UNKNOWN ||
            retType->getKind() == sema::TypeKind::TK_GENERIC)
            retType = sema::getIntType();
        // 登记到和普通函数同一张表里：调用点的寄存器分配、int<->float 转换、
        // 可赋值性校验、命名实参全都复用现成机制，不需要为泛型再开一条路径。
        _functionParams[mangled] = paramTypes;
        _functionParamNames[mangled] = generic.paramNames;
        _functionReturns[mangled] = retType;
        {
            std::vector<bool> mask(paramTypes.size(), true);
            _functionParamAnnotated[mangled] = mask;
        }
        PendingInstance inst;
        inst.emitName = mangled;
        inst.paramNames = generic.paramNames;
        inst.paramTypes = paramTypes;
        inst.retType = retType;
        inst.bodyNode = generic.def ? generic.def->getBodyNode() : nullptr;
        inst.bindings = bindings;
        inst.genericParams = generic.genericParams;
        _pendingInstances.push_back(std::move(inst));
        return mangled;
    }
    void CodeGen::emitPendingInstances()    {
        // 实例化可能发生在实例化函数体的发射过程中（泛型调泛型），
        // 所以边发边取，直到队列不再增长。
        size_t cursor = 0;
        while (cursor < _pendingInstances.size())        {
            if (hasFailed()) return;
            const PendingInstance inst = _pendingInstances[cursor++];
            const auto savedParams = _currentGenericParams;
            const auto savedBindings = _currentGenericBindings;
            _currentGenericParams = inst.genericParams;
            _currentGenericBindings = inst.bindings;
            emitFunctionBody(inst.emitName, inst.paramNames, inst.paramTypes, inst.retType,
                             inst.bodyNode, true, {}, {}, "", false, true);
            _currentGenericParams = savedParams;
            _currentGenericBindings = savedBindings;
        }
    }
    sema::TypePtr CodeGen::inferGenericCallType(const std::string& name,
                                                const std::shared_ptr<ast::ExpressionNode>& callee,
                                                const std::shared_ptr<ast::ExpressionNode>& callNode) const    {
        const auto found = _genericFunctions.find(name);
        if (found == _genericFunctions.end()) return nullptr;
        const GenericFunction& generic = found->second;
        // 实参列表
        std::vector<std::shared_ptr<ast::ExpressionNode>> args;
        if (const auto call = std::dynamic_pointer_cast<ast::FunctionCallNode>(callNode))        {
            if (const auto argsNode = std::dynamic_pointer_cast<ast::ExpressionNode>(call->getRightNode()))            {
                collectArgs(argsNode, args);
            }
        }
        std::map<std::string, sema::TypePtr> bindings;
        if (!deduceTypeArgs(generic, explicitTypeArgs(callee), args, bindings)) return nullptr;
        auto ret = substituteGenerics(generic.retType, bindings);
        if (!ret || ret->getKind() == sema::TypeKind::TK_UNKNOWN ||
            ret->getKind() == sema::TypeKind::TK_GENERIC)
            return nullptr;
        return ret;
    }
    bool CodeGen::tryEmitGenericCall(const std::string& name,
                                     const std::shared_ptr<ast::ExpressionNode>& callee,
                                     const std::shared_ptr<ast::ExpressionNode>& callNode,
                                     const std::vector<std::shared_ptr<ast::ExpressionNode>>& args)    {
        const auto found = _genericFunctions.find(name);
        if (found == _genericFunctions.end()) return false;
        const GenericFunction& generic = found->second;
        std::map<std::string, sema::TypePtr> bindings;
        const auto explicitArgs = explicitTypeArgs(callee);
        if (!deduceTypeArgs(generic, explicitArgs, args, bindings))        {
            // 个数不对与推不出来要分开报：前者是写错了类型实参列表，
            // 后者是实参里根本没有能推出 T 的信息。
            if (!explicitArgs.empty() && explicitArgs.size() != generic.genericParams.size())            {
                failAt(callNode ? callNode->getPos() : utils::Pos{},
                       "Generic function '" + name + "' takes " +
                       std::to_string(generic.genericParams.size()) +
                       " type argument(s), but " + std::to_string(explicitArgs.size()) +
                       " were supplied");
                return true;
            }
            std::string missing;
            for (const auto& g : generic.genericParams)            {
                if (bindings.find(g) == bindings.end())                {
                    if (!missing.empty()) missing += ", ";
                    missing += g;
                }
            }
            failAt(callNode ? callNode->getPos() : utils::Pos{},
                   "Cannot deduce type arguments for generic function '" + name +
                   "' (unspecified: " + missing + ")");
            return true;
        }
        const std::string mangled = instantiateGeneric(name, bindings);
        if (mangled.empty())        {
            failAt(callNode ? callNode->getPos() : utils::Pos{},
                   "Cannot instantiate generic function '" + name + "'");
            return true;
        }
        const auto instArgs = normalizeCallArgs(args, mangled);
        std::vector<sema::TypePtr> paramTypes;
        if (const auto p = _functionParams.find(mangled); p != _functionParams.end())        {
            paramTypes = p->second;
        }
        const std::vector<bool>* instMask = nullptr;
        if (const auto p = _functionParamAnnotated.find(mangled); p != _functionParamAnnotated.end())        {
            instMask = &p->second;
        }
        emitArgsToRegisters(instArgs, mangled, paramTypes, 0, 0, instMask);
        if (hasFailed()) return true;
        const size_t instShadow = _abi.getShadowSpaceSize();
        _asm.subRsp(instShadow);
        _asm.call(mangled);
        _asm.addRsp(instShadow);
        return true;
    }
    void CodeGen::emitFunctionDefinition(const std::shared_ptr<ast::FunctionDefinitionNode>& node)    {
        if (!node) return;
        // 函数名来自 callNode（IdentifierNode 或带参数的节点）
        std::string fname;
        if (const auto id = std::dynamic_pointer_cast<ast::IdentifierNode>(node->getCallNode()))        {
            fname = id->getName();
        }
 else if (const auto call = std::dynamic_pointer_cast<ast::FunctionCallNode>(node->getCallNode()))        {
            if (const auto id = std::dynamic_pointer_cast<ast::IdentifierNode>(call->getLeftNode()))            {
                fname = id->getName();
            }
            // P10：泛型声明的 callee 是 IndexExpressionNode(Identifier f, Bracket([T]))
            else if (const auto index = std::dynamic_pointer_cast<ast::IndexExpressionNode>(call->getLeftNode()))            {
                if (const auto id = std::dynamic_pointer_cast<ast::IdentifierNode>(index->getLeftNode()))                {
                    fname = id->getName();
                }
            }
        }
        if (fname.empty())        {
            fail("Cannot determine function name: " + node->toString());
            return;
        }
        // P10：泛型声明本身**不发射**，只登记模板。真正的代码在调用点
        // 按实参类型单态化生成（见 instantiateGeneric）。
        // 这样 `fun identity[T](x: T): T` 在没有任何调用时是零成本的。
        const auto genericParams = collectGenericParams(node->getCallNode());
        if (!genericParams.empty())        {
            GenericFunction generic;
            generic.name = fname;
            generic.genericParams = genericParams;
            generic.def = node;
            // 声明阶段收集签名：T 在这里解析成 GenericType 占位，不是报错。
            const auto savedParams = _currentGenericParams;
            _currentGenericParams = genericParams;
            // 泛型声明的 callNode 是 FunctionCallNode（不是 InfixExpressionNode），
            // 形参表在它的右节点里。
            if (const auto call = std::dynamic_pointer_cast<ast::FunctionCallNode>(node->getCallNode()))            {
                const auto paramsNode = std::dynamic_pointer_cast<ast::ExpressionNode>(call->getRightNode());
                if (const auto paren = std::dynamic_pointer_cast<ast::ParenRangerNode>(paramsNode))                {
                    collectParamNames(paren->getRangerNode(), generic.paramNames);
                    collectParamTypesT(paren->getRangerNode(), generic.paramTypes);
                }
                else if (paramsNode)                {
                    collectParamNames(paramsNode, generic.paramNames);
                    collectParamTypesT(paramsNode, generic.paramTypes);
                }
            }
            generic.retType = typeFromLabelList(node->getLabelNodes(), genericParams);
            _currentGenericParams = savedParams;
            while (generic.paramTypes.size() < generic.paramNames.size())
                generic.paramTypes.push_back(sema::getUnknownType());
            _genericFunctions[fname] = std::move(generic);
            return;
        }
        // 返回类型与形参类型的登记在收集完形参之后进行（见下方）。
        // 参数名：buildFunctionExpression 生成的 callNode 形如
        //   InfixExpressionNode(',', IdentifierNode("add"), <参数列表>)
        // 左节点是函数名，右节点才是参数列表。
        std::vector<std::string> paramNames;
        std::vector<sema::TypePtr> paramTypes;
        if (const auto callInfix = std::dynamic_pointer_cast<ast::InfixExpressionNode>(node->getCallNode()))        {
            // 参数列表形如 ParenRangerNode{ InfixNode(comma, a, b) }
            if (const auto paren = std::dynamic_pointer_cast<ast::ParenRangerNode>(callInfix->getRightNode()))            {
                collectParamNames(paren->getRangerNode(), paramNames);
                // 用能解析类名的那一版：`fun total(p: Point)` 的 p 必须是
                // ClassType，否则 p.sum() 找不到对象类型。
                collectParamTypesT(paren->getRangerNode(), paramTypes);
            }
 else            {
                collectParamNames(callInfix->getRightNode(), paramNames);
                collectParamTypesT(callInfix->getRightNode(), paramTypes);
            }
        }
 else        {
            // 无参形态：callNode 直接是 IdentifierNode
            collectParamNames(node->getCallNode(), paramNames);
            collectParamTypesT(node->getCallNode(), paramTypes);
            // 首项若是函数名本身，去掉
            if (!paramNames.empty() && paramNames.front() == fname)            {
                paramNames.erase(paramNames.begin());
                if (!paramTypes.empty()) paramTypes.erase(paramTypes.begin());
            }
        }
        // 登记形参类型：调用点据此决定实参走整型还是浮点寄存器，
        // 并在实参与形参类型不一致时插入转换指令。
        // 未标注类型的形参回退 int（Rio 的默认整型约定）。
        // P9：但「标注了却解析不出来」是错误，不能也回退成 int ——
        // `fun f(x: Piont)` 静默当 int 会让 f 内部所有字段访问变成垃圾。
        std::vector<std::string> annotations;
        {
            if (const auto callInfix = std::dynamic_pointer_cast<ast::InfixExpressionNode>(node->getCallNode()))            {
                if (const auto paren = std::dynamic_pointer_cast<ast::ParenRangerNode>(callInfix->getRightNode()))
                    collectParamAnnotations(paren->getRangerNode(), annotations);
                else
                    collectParamAnnotations(callInfix->getRightNode(), annotations);
            }
 else            {
                collectParamAnnotations(node->getCallNode(), annotations);
                if (!annotations.empty() && !paramNames.empty() &&
                    annotations.front().empty() && paramNames.front() == fname)
                    annotations.erase(annotations.begin());
            }
            for (size_t i = 0; i < annotations.size(); ++i)            {
                if (annotations[i].empty()) continue;
                // P12/G13：`any` 是合法标注（等价于「不标注」），不是未知类型。
                const std::string firstWord = [&] {
                    std::string w;
                    for (const char c : annotations[i])                    {
                        if (std::isalnum(static_cast<unsigned char>(c)) || c == '_') w += c;
                        else if (!w.empty()) break;
                    }
                    return w;
                }();
                if (firstWord == "any") continue;
                const auto t = resolveTypeName(annotations[i]);
                if (!t || t->getKind() == sema::TypeKind::TK_UNKNOWN)                {
                    const std::string pname = (i < paramNames.size()) ? paramNames[i] : std::to_string(i);
                    failAt(node->getPos(), "Unknown type '" + annotations[i] +
                           "' of parameter '" + pname + "' in function '" + fname + "'");
                    return;
                }
            }
            std::vector<bool> annotatedMask(paramTypes.size(), false);
            for (size_t i = 0; i < annotatedMask.size() && i < annotations.size(); ++i)
                annotatedMask[i] = !annotations[i].empty();
            _functionParamAnnotated[fname] = annotatedMask;
        }
        for (size_t i = 0; i < paramTypes.size(); ++i)        {
            auto& pt = paramTypes[i];
            if (!pt || pt->getKind() == sema::TypeKind::TK_UNKNOWN)            {
                // P12/G13：标注为 `any` 的形参必须**保持** unknown。
                // 补成 int 会让 `copy("hi")` 报 "cannot assign str to int"，
                // 而 any 的语义就是「什么都收」。
                const bool isAny = i < annotations.size() && [&] {
                    std::string w;
                    for (const char c : annotations[i])                    {
                        if (std::isalnum(static_cast<unsigned char>(c)) || c == '_') w += c;
                        else if (!w.empty()) break;
                    }
                    return w == "any";
                }();
                if (!isAny) pt = sema::getIntType();
            }
        }
        _functionParams[fname] = paramTypes;
        // P8：登记形参名与默认值，供调用点处理命名实参 / 缺省实参。
        _functionParamNames[fname] = paramNames;
        {
            std::map<std::string, std::shared_ptr<ast::ExpressionNode>> defaults;
            if (const auto callInfix =
                    std::dynamic_pointer_cast<ast::InfixExpressionNode>(node->getCallNode()))
            {
                if (const auto paren =
                        std::dynamic_pointer_cast<ast::ParenRangerNode>(callInfix->getRightNode()))
                    collectParamDefaults(paren->getRangerNode(), defaults);
                else
                    collectParamDefaults(callInfix->getRightNode(), defaults);
            }
            if (!defaults.empty()) _functionDefaults[fname] = defaults;
        }
        // 登记返回类型：优先用声明的 `: type`，否则从函数体的 ret 表达式推断。
        // 必须推断而不是一律当 int：`fun make() { ret [1, 2] }` 的调用结果
        // 得是 list 类型，`make()[0]` 才知道按列表下标取值。
        // 位置在形参类型收齐之后：推断要在影子作用域里放形参槽。
        auto retType = returnTypeOf(node);
        // P9：标注了返回类型就记为「权威」，后面 ret 要按它校验。
        bool retAnnotated = retType && retType->getKind() != sema::TypeKind::TK_UNKNOWN;
        // 声明的返回类型认不出来时（多半是类名，parseTypeName 只认内建类型），
        // 再按类登记表解析一次 —— `fun make(): Point` 必须得到 ClassType。
        if (retType->getKind() == sema::TypeKind::TK_UNKNOWN)        {
            for (const auto& label : node->getLabelNodes())            {
                if (!label) continue;
                const auto t = resolveTypeName(label->getFullLabel());
                if (t && t->getKind() != sema::TypeKind::TK_UNKNOWN)                {
                    retType = t;
                    retAnnotated = true;
                    break;
                }
            }
        }
        // P9：写了返回类型标注却解析不出来（拼错类型名）——明确报错。
        if (!retAnnotated && !node->getLabelNodes().empty())        {
            std::string raw;
            for (const auto& label : node->getLabelNodes())            {
                if (label) { raw = label->getFullLabel(); break; }
            }
            if (!raw.empty())            {
                // P12/G13：`any` 是合法返回标注，等价于不标注 ——
                // 让下面的 inferAnonReturnType 去推断真实类型。
                std::string firstWord;
                for (const char c : raw)                {
                    if (std::isalnum(static_cast<unsigned char>(c)) || c == '_') firstWord += c;
                    else if (!firstWord.empty()) break;
                }
                if (firstWord != "any")                {
                    failAt(node->getPos(), "Unknown return type '" + raw + "' of function '" + fname + "'");
                    return;
                }
            }
        }
        if (retType->getKind() == sema::TypeKind::TK_UNKNOWN)        {
            retType = inferAnonReturnType(node->getBodyNode(), paramNames, paramTypes);
            if (!retType || retType->getKind() == sema::TypeKind::TK_UNKNOWN ||
                retType->getKind() == sema::TypeKind::TK_VOID)            {
                retType = sema::getIntType();
            }
        }
        _functionReturns[fname] = retType;
        // 函数体若只有一个 encapsulated 语句，表示「由运行时提供实现」：
        // 只登记签名，不发射函数体，调用点照常生成 call，由链接器去 rt 里找。
        // 这样 legacy 测试开头那个
        //     fun sout(*args, end="\n"): void { encapsulated }
        // 桩就不会再卡在 Node(PREFIX) 上。
        if (const auto bodyBlock = std::dynamic_pointer_cast<ast::BlockRangerNode>(node->getBodyNode()))        {
            bool onlyEncapsulated = !bodyBlock->getBodyExpressions().empty();
            for (const auto& e : bodyBlock->getBodyExpressions())            {
                if (!std::dynamic_pointer_cast<ast::EncapsulatedExpressionNode>(e))                {
                    onlyEncapsulated = false;
                    break;
                }
            }
            if (onlyEncapsulated)            {
                _encapsulatedFunctions[fname] = true;
                return;
            }
        }
        else if (std::dynamic_pointer_cast<ast::EncapsulatedExpressionNode>(node->getBodyNode()))        {
            _encapsulatedFunctions[fname] = true;
            return;
        }
        // P11：重载名的发射符号在预登记表里已经定好了（形参类型要参与修饰，
        // 而这里 paramTypes 已经被「未标注回退 int」改过，不能再用它算一遍）。
        // 用 def 指针精确配对，保证与 registerOverloadSignatures 里那一份一致。
        // 注意查的是 _overloads（**保留每一份定义**），不是 _signatures
        // （符号 -> 签名，同签名的重复定义在里面只剩先来的那一份）。
        std::string emitSymbol;
        if (_overloadedNames.find(fname) != _overloadedNames.end())        {
            if (const auto bucket = _overloads.find(fname); bucket != _overloads.end())            {
                for (const auto& entry : bucket->second)                {
                    if (entry.def == node)                    {
                        emitSymbol = entry.symbol;
                        break;
                    }
                }
            }
            if (emitSymbol.empty())            {
                failAt(node->getPos(), "Internal error: no overload signature registered for '" + fname + "'");
                return;
            }
            // 同一签名的重复定义必须在这里挡住：否则汇编器报
            // "symbol already defined"，用户看不出是哪两个函数撞了。
            if (_emittedSymbols.find(emitSymbol) != _emittedSymbols.end())            {
                failAt(node->getPos(), "Duplicate definition of overloaded function '" + fname +
                                       "' with the same parameter types (symbol " + emitSymbol + ")");
                return;
            }
            _emittedSymbols[emitSymbol] = true;
        }
        // 用户可能自己写了 `fun main()`。它的汇编符号必须与我们的入口 `main` 区分开，
        // 否则汇编器会报 "symbol 'main' is already defined"。
        // 约定：用户 main 发射为 __rio_user_main，由 wrapper 调用。
        const bool isUserMain = (fname == "main");
        if (isUserMain) _hasUserMain = true;
        const std::string emitName = !emitSymbol.empty()
                                         ? emitSymbol
                                         : (isUserMain ? "__rio_user_main" : fname);
        emitFunctionBody(emitName, paramNames, paramTypes, retType, node->getBodyNode(), true,
                         {}, {}, "", false, retAnnotated);
    }
    void CodeGen::emitFunctionBody(const std::string& emitName,
                                   const std::vector<std::string>& paramNames,
                                   const std::vector<sema::TypePtr>& paramTypes,
                                   const sema::TypePtr& retType,
                                   const std::shared_ptr<ast::ExpressionNode>& bodyNode,
                                   bool makeGlobal,
                                   const std::vector<std::string>& capturedNames,
                                   const std::vector<sema::TypePtr>& capturedTypes,
                                   const std::string& defaultReturnVar,
                                   bool hasThis,
                                   bool retTypeAnnotated)    {
        const std::string savedFunction = _functionName;
        const size_t savedFrame = _frameSize;
        const std::string savedReturnLabel = _returnLabel;
        const sema::TypePtr savedReturnType = _functionReturnType;
        const bool savedReturnAnnotated = _functionReturnAnnotated;
        // P7：try 帧栈与循环栈都是函数局部的，跨函数必须清空，
        // 否则嵌套发射（方法体里再发匿名函数）会把外层帧带进来。
        const std::vector<EhFrame> savedEhStack = _ehStack;
        const std::vector<LoopContext> savedLoopStack = _loopStack;
        _functionName = emitName;
        _functionReturnType = retType;
        _functionReturnAnnotated = retTypeAnnotated;
        _frameSize = 0;
        _ehStack.clear();
        _loopStack.clear();
        _scopes.clear();
        pushScope();
        if (makeGlobal) _asm.global(emitName);
        _asm.label(emitName);
        _asm.push("%rbp");
        _asm.mov("%rsp", "%rbp");
        // 栈帧大小在函数体生成完才知道，这里先占位，末尾回填。
        const std::string framePlaceholder = _asm.newLabel("frame_" + emitName);
        _asm.label(framePlaceholder);
        // 形参分别按整型/浮点各用一套寄存器（MS ABI）
        const auto& intRegs = _abi.getIntArgRegs();
        const auto& floatRegs = _abi.getFloatArgRegs();
        size_t intIndex = 0;
        size_t floatIndex = 0;
        // P6：构造器/方法有隐藏的 this 参数，占掉第一个整型寄存器（%rcx），
        // 用户形参整体后移一格。this 存成一个普通局部槽位，这样函数体里
        // `this.x` 走的是和普通变量完全一样的取址路径。
        if (hasThis)        {
            sema::TypePtr thisType = _currentClass.empty()
                                         ? sema::getIntType()
                                         : resolveTypeName(_currentClass);
            if (!thisType) thisType = sema::getIntType();
            const long long offset = allocateSlot("this", thisType, thisType->getSize());
            _asm.mov("%rcx", std::to_string(offset) + "(%rbp)");
            intIndex = 1;
        }
        for (size_t i = 0; i < paramNames.size(); ++i)        {
            auto pt = (i < paramTypes.size()) ? paramTypes[i] : sema::getIntType();
            if (!pt || pt->getKind() == sema::TypeKind::TK_UNKNOWN) pt = sema::getIntType();
            const long long offset = allocateSlot(paramNames[i], pt, pt->getSize());
            const std::string slot = std::to_string(offset) + "(%rbp)";
            if (pt->isFloat())            {
                if (floatIndex >= floatRegs.size()) continue;
                _asm.inst("movsd", floatRegs[floatIndex++], slot);
            }
 else            {
                if (intIndex >= intRegs.size()) continue;
                _asm.mov(intRegs[intIndex++], slot);
            }
        }
        _returnLabel = _asm.newLabel("ret_" + emitName);
        // 闭包捕获：%r10 指向 env 块，逐项拷进本地槽位。
        // 槽位按普通局部变量分配，函数体随后照常按名字访问它们。
        for (size_t i = 0; i < capturedNames.size(); ++i)        {
            auto ct = (i < capturedTypes.size()) ? capturedTypes[i] : sema::getIntType();
            if (!ct || ct->getKind() == sema::TypeKind::TK_UNKNOWN) ct = sema::getIntType();
            const long long offset = allocateSlot(capturedNames[i], ct, ct->getSize());
            const std::string slot = std::to_string(offset) + "(%rbp)";
            _asm.inst("movq", std::to_string(i * 8) + "(%r10)", "%rax");
            _asm.inst("movq", "%rax", slot);
        }
        // 函数体
        if (bodyNode)        {
            if (const auto block = std::dynamic_pointer_cast<ast::BlockRangerNode>(bodyNode))            {
                for (const auto& e : block->getBodyExpressions())                {
                    emitStatement(e);
                }
            }
 else            {
                emitStatement(bodyNode);
            }
        }
        // 默认返回：构造器返回 this（调用方拿它当 new 的结果），
        // 其余函数默认返回 0（浮点函数返回 0.0）。
        const VarSlot* defaultSlot = defaultReturnVar.empty() ? nullptr : lookupVar(defaultReturnVar);
        if (defaultSlot)        {
            _asm.mov(addrOf(*defaultSlot), "%rax");
        }
        else if (retType && retType->isFloat())        {
            _asm.inst("xorpd", "%xmm0", "%xmm0");
        }
        else        {
            _asm.movImm(0, "%rax");
        }
        _asm.label(_returnLabel);
        _asm.mov("%rbp", "%rsp");
        _asm.pop("%rbp");
        _asm.ret();
        // 回填栈帧大小。
        // 两个约束：
        //   1. 至少 32 字节 —— MS ABI 规定被调函数可以使用调用者预留的
        //      shadow space，而本函数自己也可能调用别人，必须有这 32 字节。
        //      没有它，`subq $0, %rsp` 的函数一调用子函数就会踩坏自己的栈。
        //   2. 取整到 16 的倍数 —— push %rbp 之后 %rsp 已是 16 字节对齐，
        //      帧大小保持 16 的倍数，调用点就仍然对齐。
        const size_t shadowSpace = _abi.getShadowSpaceSize();
        const size_t frameNeeded = _frameSize > shadowSpace ? _frameSize : shadowSpace;
        const size_t alignedFrame = ((frameNeeded + 15) / 16) * 16;
        _asm.replaceLabel(framePlaceholder,            "    subq    $" + std::to_string(alignedFrame) + ", %rsp\n");
        _functionName = savedFunction;
        _frameSize = savedFrame;
        _returnLabel = savedReturnLabel;
        _functionReturnType = savedReturnType;
        _functionReturnAnnotated = savedReturnAnnotated;
        _ehStack = savedEhStack;
        _loopStack = savedLoopStack;
        _scopes.clear();
        pushScope();
    }
}


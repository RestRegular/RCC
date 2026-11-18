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
        // 生命周期标签（life circle label）不是类型。
        //
        // `rcc_base.cpp` 的 DESCRIBE_LABELS 把标签分四类，其中：
        //   // life circle label
        //   "static", "global",
        // 它们与类型标签同为 TOKEN_LABEL、同样出现在标签区，但语义是
        // 「这个字段/变量归谁的生命周期」而不是「它是什么类型」。
        // 原仓库就有的 `var _number: int static = 0`（tests/test.rio）
        // 正是这种写法 —— P0 重写时把 static 字段的解析整个丢了，
        // 于是它被当成类型名，报 "unknown type 'static'"。
        bool isLifecycleLabel(const std::string& name)        {
            return name == "static" || name == "global";
        }
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
            // P13-2/L3-A（决议 §1.1）：Pool 是标准库普通类型，不做关键字。
            if (name == "Pool")  return sema::getPoolType();
            // P12/G13：`any` 作为显式标注。原仓库 builtInTypes 里有 any
            // （rcc_symbol.cpp:650），ds.rio:32 `fun copy(arg): any`、
            // math.rio:26 `fun pow(a, times: int): any` 都在用。
            // 映射成 unknown = 「不标注」，语义正好等价（checkAssignable /
            // sameValueFamily 对 unknown 一律放行）。
            if (name == "any")   return sema::getUnknownType();
            // P8：容器形参标注。没有元素类型信息时给一个「元素未知」的容器
            // 类型 —— 关键是让 `xs[i]` 知道左边是 list 而不是 int，
            // 否则会落到「Index access on a non-container value」。
            //
            // 元素用 **nullptr** 表示「标注里根本没写元素类型」（裸 `list`），
            // 显式写 `any` 才是 UNKNOWN。两者在重载符号里必须分开：
            //   list        -> ListType(nullptr)      -> `Lv`
            //   list[any]   -> ListType(UNKNOWN)      -> `LA`
            //   list[int]   -> ListType(int)          -> `Li`
            // 混用会让 `fun f(x: list)` 与 `fun f(x: list[int])` 拼出同一符号，
            // 直接报 "Duplicate definition"（2026-11-17 实测）。
            // 索引结果的静态类型仍按 int 兜底（见 inferIndexResultType）。
            if (name == "list" || name == "series" || name == "array")            {
                return std::make_shared<sema::ListType>(nullptr);
            }
            // dict：**键与值都设为 nullptr** = 都没写，什么 dict 都收。
            //
            // 键不能钉成 str（P13 修复，2026-11-17）：dict 的键可以是
            // int / float / char（实测全部可用）。钉成 str 会让
            // `fun f(d: dict)` 拒绝 `dict[int, str]` 实参 —— 打分时
            // 「形参键 str ≠ 实参键 int」直接判不合格。
            // 「字符串键按内容比较」那件事由**索引表达式**按实际 dict 的键类型
            // 决定（见 emitIndexExpression 的 keyIsStr），与形参标注无关。
            if (name == "dict" || name == "map")            {
                return std::make_shared<sema::DictType>(nullptr, nullptr);
            }
            // P10：`func[params][ret]` —— 函数类型标注。
            // 裸名字到这里时元素类型已丢，只能给一个「参数未知」的占位，
            // 但至少让 `f: func[...]` 不再报「Unknown type 'func'」。
            //
            // 2026-11-18：`func` 已全部改名为 `func`（用户裁决），不再保留
            // 旧名 —— 与 `fun` 关键字区分开，`func` 是类型名。
            if (name == "func")            {
                return std::make_shared<sema::FunctionType>(
                    std::vector<sema::TypePtr>{}, sema::getUnknownType());
            }
            return sema::getUnknownType();
        }
        // 从函数定义的 labelNodes 里取返回类型（`fun f(): int` 的 `int`）
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
        // 扫描形参表里有没有变长形参 `*args` / `**kwargs`。
        // 它们在 AST 里是 UnaryExpressionNode(TOKEN_STAR / TOKEN_DOUBLE_STAR)，
        // collectParamNames 不解包（`*args` 里的 args 不是普通形参），
        // 所以这里单独走一遍，只记「有没有」，不碰名字。
        void collectVariadicFlags(const std::shared_ptr<ast::ExpressionNode>& node,
                                  bool& hasPositional,
                                  bool& hasKeyword)        {
            if (!node) return;
            if (const auto unary = std::dynamic_pointer_cast<ast::UnaryExpressionNode>(node))            {
                const auto t = unary->getOpToken().getType();
                if (t == core::TokenType::TOKEN_DOUBLE_STAR) hasKeyword = true;
                else if (t == core::TokenType::TOKEN_STAR) hasPositional = true;
                return;
            }
            if (const auto infix = std::dynamic_pointer_cast<ast::InfixExpressionNode>(node))            {
                collectVariadicFlags(infix->getLeftNode(), hasPositional, hasKeyword);
                collectVariadicFlags(infix->getRightNode(), hasPositional, hasKeyword);
                return;
            }
            if (const auto block = std::dynamic_pointer_cast<ast::BlockRangerNode>(node))            {
                for (const auto& e : block->getBodyExpressions())
                    collectVariadicFlags(e, hasPositional, hasKeyword);
                return;
            }
            if (const auto paren = std::dynamic_pointer_cast<ast::ParenRangerNode>(node))            {
                collectVariadicFlags(paren->getRangerNode(), hasPositional, hasKeyword);
                return;
            }
            if (const auto list = std::dynamic_pointer_cast<ast::ListExpressionNode>(node))            {
                if (const auto body = std::dynamic_pointer_cast<ast::ExpressionNode>(list->getBodyNode()))
                    collectVariadicFlags(body, hasPositional, hasKeyword);
                return;
            }
            if (const auto assign = std::dynamic_pointer_cast<ast::AssignmentNode>(node))            {
                collectVariadicFlags(assign->getAssignPair().first, hasPositional, hasKeyword);
            }
        }
        bool contains(const std::vector<std::string>& v, const std::string& s)        {
            for (const auto& e : v)            {
                if (e == s) return true;
            }
            return false;
        }
        // ============================================================
        // core 种子层的返回值签名表（P12 §4 方案 2 的最小实现）
        //
        // 为什么需要：`core.rio` 里的 `extern fun` **不参与新后端的类型表**
        // （它只是三层契约文档 + 旧前端用），所以 inferType 对任何 `core_*`
        // 调用都落到「未知函数按 int」。于是
        //     var c: str = core_clone_str(s)
        //     var ys: list[int] = core_clone_list(xs)
        // 在**返回值类型就是它自己**的场合被可赋值性检查挡下
        // （"cannot assign a value of type int to a target of type str"）。
        // 这是唯一挡得住的场合，因为 str / list 与 int 不同族；ptr 与数值族
        // 互通（P12/G12 的裁决），所以 `var p: ptr = core_alloc(...)` 一直是通的。
        //
        // 只登记**返回值非 int** 的那几个（void 的也不能按 int：`var n = f()`
        // 的槽位大小会算错，见 inferType 末尾的说明）。表里没有的名字仍按
        // 原来的 int 兜底，行为不变。
        //
        // 这张表必须与 lib/core/core.rio 的 extern 声明同步 —— 只影响**静态
        // 类型推断**，不改变任何 ABI：符号名、寄存器约定、栈布局都不动。
        // ============================================================
        sema::TypePtr coreReturnType(const std::string& name)        {
            // 指针族（ptr）：分配器入口
            if (name == "core_alloc" || name == "core_realloc") return sema::getPtrType();
            // 容器族：clone 出来的就是原容器
            if (name == "core_clone_str") return sema::getStrType();
            if (name == "core_clone_list") return std::make_shared<sema::ListType>(nullptr);
            // 标量 / void
            if (name == "core_mem_eq") return sema::getBoolType();
            if (name == "core_str_len" || name == "core_io_read" || name == "core_io_write" ||
                name == "core_time_ns" || name == "core_dbg_allocations" ||
                name == "core_dbg_frees" || name == "core_dbg_double_frees" ||
                name == "core_dbg_poison_bytes")
                return sema::getIntType();
            if (name == "core_mem_copy" || name == "core_mem_move" || name == "core_mem_set" ||
                name == "core_free" || name == "core_list_append" || name == "core_io_flush" ||
                name == "core_io_write_byte" || name == "core_proc_exit" ||
                name == "core_proc_abort" || name == "core_panic_raise" ||
                name == "core_stack_probe" || name == "core_eh_pop" || name == "core_eh_throw" ||
                name == "core_dbg_reset" || name == "core_dbg_bump_alloc" ||
                name == "core_dbg_bump_free" || name == "core_sys_breakpoint")
                return sema::getVoidType();
            return nullptr;   // 不在表里：交给调用点原来的 int 兜底
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
    bool CodeGen::isEmptyContainerLiteral(const std::shared_ptr<ast::ExpressionNode>& node)    {
        if (!node) return false;
        // 带括号的 `([])`：剥掉括号再看
        if (const auto paren = std::dynamic_pointer_cast<ast::ParenRangerNode>(node))        {
            return isEmptyContainerLiteral(paren->getRangerNode());
        }
        // 空列表 / 空字典的 body 是 nullptr。非空字面量的 body 一定非空，
        // 所以只看 body 就够了（不需要摊平元素，那是发射阶段的事，
        // 而这里必须能在 const 上下文里用）。
        if (const auto list = std::dynamic_pointer_cast<ast::ListExpressionNode>(node))        {
            return list->getBodyNode() == nullptr;
        }
        if (const auto dict = std::dynamic_pointer_cast<ast::DictionaryExpressionNode>(node))        {
            return dict->getBodyNode() == nullptr;
        }
        return false;
    }
    bool CodeGen::sameValueFamily(const sema::TypePtr& a, const sema::TypePtr& b) const    {        if (!a || !b) return true;
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
        // ---------- P13-2/L3：内存模型特殊类型（决议 §4） ----------
        // Player（裸类名 = ClassType）与 Body[Player] 是同一类型（别名）；
        // Content/Ref/Mut/Take 是不同角色，不能与 Body 或彼此混用。
        const auto roleOf = [](const sema::TypePtr& t) -> int {
            if (!t || t->getKind() != sema::TypeKind::TK_MEMORY) return -1;
            return static_cast<int>(std::dynamic_pointer_cast<sema::MemoryType>(t)->getRole());
        };
        const auto isBodyMemory = [&](const sema::TypePtr& t) {
            return t && t->getKind() == sema::TypeKind::TK_MEMORY &&
                   roleOf(t) == static_cast<int>(sema::MemoryRole::BODY);
        };
        const auto isBareBody = [&](const sema::TypePtr& t) {
            // 裸类名（ClassType）是 Body 的别名；泛型/其他类型不算。
            return t && t->getKind() == sema::TypeKind::TK_CLASS;
        };
        if (roleOf(target) >= 0 || roleOf(source) >= 0)        {
            const int tr = roleOf(target), sr = roleOf(source);
            // 两侧都显式标注角色：角色必须相同，内层再递归比。
            if (tr >= 0 && sr >= 0)            {
                if (tr != sr) return "cannot assign " + source->toString() +
                                    " to " + target->toString() + ": different memory roles";
                const auto ti = std::dynamic_pointer_cast<sema::MemoryType>(target)->getInnerType();
                const auto si = std::dynamic_pointer_cast<sema::MemoryType>(source)->getInnerType();
                return (!ti || !si || ti->equals(si)) ? std::string()
                       : "inner type mismatch: " + source->toString() + " vs " + target->toString();
            }
            // 一侧显式 Body、另一侧裸类名：别名，放行（决议 §4）。
            if ((isBodyMemory(target) && isBareBody(source)) ||
                (isBodyMemory(source) && isBareBody(target))) return std::string();
            // P13-2/L3（决议 §7.3 + §6.3）：调用点 f(bob) 由形参类型决定语义 ——
            // Ref[T]/Mut[T] 形参：Body 实参隐式取引用（底层同一指针）；
            // Content[T]/Take[T] 形参：Body 实参隐式 take（落定，源→E）。
            // 都要求 inner 类型与实参类型一致。
            if (tr >= 0 && sr < 0 && sk != sema::TypeKind::TK_PTR)            {
                const auto role = static_cast<sema::MemoryRole>(tr);
                if (role == sema::MemoryRole::REF || role == sema::MemoryRole::MUT ||
                    role == sema::MemoryRole::CONTENT || role == sema::MemoryRole::TAKE)                {
                    const auto inner = std::dynamic_pointer_cast<sema::MemoryType>(target)->getInnerType();
                    if (!inner || inner->equals(source)) return std::string();
                    return "cannot pass " + source->toString() + " as " + target->toString() +
                           ": inner type mismatch";
                }
            }
            // Content/Ref/Mut/Take 与裸类名混用：拒绝。
            if (sk != sema::TypeKind::TK_PTR)            {
                return "cannot assign " + source->toString() + " to " + target->toString() +
                       ": memory role requires explicit type annotation";
            }
            // null 字面量（sema 记为 ptr）是任何引用型内存类型的合法零值
            // （决议 Q-A：引用类型 null = 0 指针，与有效对象地址不同）。
            return std::string();
        }
        // ---------- 容器：必须比元素类型（P13 缺陷修复，2026-11-17） ----------
        //
        // 原先这里直接落到 sameValueFamily，而它对 list/dict 只看「都是容器」，
        // 于是 `fun f(xs: list[str])` 接受 `[1,2,3]`、`dict[str,int]` 接受
        // `{"a": "b"}` —— 编译通过，运行时把 int 当 str 指针解引用，
        // 直接段错误（用户 test.rio 实测 exit -1073741819）。
        // 这是「静默生成垃圾」，正是 P9 要挡的那类错。
        //
        // 判据：
        //   * 容器种类不同（str / list / dict）       -> 拒绝
        //   * 任一侧元素类型是 nullptr（裸容器标注）  -> 放行（不约束）
        //   * 任一侧是 UNKNOWN（显式 any）            -> 放行（any 就是不约束）
        //   * 两侧都具体 -> 逐层递归比较，不一致就拒绝
        // str 没有类型实参，不需要比。
        if (tk == sema::TypeKind::TK_LIST && sk == sema::TypeKind::TK_LIST)        {
            const auto tl = std::static_pointer_cast<sema::ListType>(target)->getElemType();
            const auto sl = std::static_pointer_cast<sema::ListType>(source)->getElemType();
            const auto loose = [](const sema::TypePtr& t) {
                return !t || t->getKind() == sema::TypeKind::TK_UNKNOWN;
            };
            if (loose(tl) || loose(sl)) return std::string();
            // 空列表字面量 `[]` 的元素类型是「推不出来」，inferListLiteralType
            // 会兜底成 int。那**不是**用户的承诺，不能让
            // `var r: list[str] = []` 报错。靠 target 与 source 的
            // 「元素是否来自字面量空表」区分不了，所以在调用点侧
            // （emitArgsToRegisters / emitVariableDefinition）用
            // isUntypedEmptyLiteral 提前放行 —— 见那里的说明。
            // 这里只处理两侧都确定的场合。
            if (!tl->equals(sl))            {
                return "element type mismatch: list[" + tl->toString() + "] vs list[" +
                       sl->toString() + "]";
            }
            return std::string();
        }
        if (tk == sema::TypeKind::TK_DICT && sk == sema::TypeKind::TK_DICT)        {
            const auto td = std::static_pointer_cast<sema::DictType>(target);
            const auto sd = std::static_pointer_cast<sema::DictType>(source);
            const auto loose = [](const sema::TypePtr& t) {
                return !t || t->getKind() == sema::TypeKind::TK_UNKNOWN;
            };
            if (!loose(td->getKeyType()) && !loose(sd->getKeyType()))            {
                if (!td->getKeyType()->equals(sd->getKeyType()))                {
                    return "key type mismatch: dict[" + td->getKeyType()->toString() +
                           ", ...] vs dict[" + sd->getKeyType()->toString() + ", ...]";
                }
            }
            if (!loose(td->getValueType()) && !loose(sd->getValueType()))            {
                if (!td->getValueType()->equals(sd->getValueType()))                {
                    return "value type mismatch: dict[..., " + td->getValueType()->toString() +
                           "] vs dict[..., " + sd->getValueType()->toString() + "]";
                }
            }
            return std::string();
        }
        // 一侧是容器、另一侧不是：交给 sameValueFamily 判（它会拒绝，
        // 因为非指针族与容器跨族）。
        // ---------- 类：只有同一类或基类方向可赋值 ----------
        //
        // sameValueFamily 里「指针族互通」是为了让 rt 的位级中转（`var s: str = p`、
        // `_dsLoadI64` 的伪造 str 头）能过，那条放宽针对的是 str/list/dict/ptr。
        // 但 ClassType 也 isPointerLike，于是 `var a: Enemy = Player("x")` 静默
        // 通过 —— 编译期认了，运行时按 Enemy 的字段布局去读 Player 对象，
        // 字段错位。
        //
        // Rio **有**继承（`class Derived: Base`，基类字段排在前面，见
        // computeLayout），所以判据是「source 是 target 本身或它的派生类」，
        // 即沿 baseName 链往上找得到 target。反方向（把 Base 当 Derived）
        // 会少字段，必须拒绝。
        if (tk == sema::TypeKind::TK_CLASS && sk == sema::TypeKind::TK_CLASS)        {
            const auto tn = std::static_pointer_cast<sema::ClassType>(target)->getName();
            const auto sn = std::static_pointer_cast<sema::ClassType>(source)->getName();
            if (!tn.empty() && !sn.empty() && tn != sn)            {
                bool isDerived = false;
                std::string cur = sn;
                // 防御成环（非法继承链）时不要死循环。
                for (size_t guard = 0; guard < 64 && !cur.empty(); ++guard)                {
                    const auto info = findClass(cur);
                    if (!info || info->baseName.empty()) break;
                    cur = info->baseName;
                    if (cur == tn) { isDerived = true; break; }
                }
                if (!isDerived)                {
                    return "cannot assign a value of type " + sn + " to a target of type " + tn +
                           " (not the same class or a derived class)";
                }
            }
            return std::string();
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
        // 泛型实例用 `_G` 前缀（2026-11-17 用户批准的 ABI 变更）。
        // 之前与重载共用 `_R`，导致 `toStr[T](list[T])` 实例化成 list[int]
        // 时与 `toStr(arg: int)` 撞成同一个符号 `_R5toStri`。
        return naming::mangleGenericInstance(base, args);
    }
    // ============================================================
    // P13-1：销毁（drop glue）
    // ============================================================
    //
    // 设计见 docs/p13_plan.md 的 D4 / D11 / D14 / D16：
    //   * 只有三种堆类型需要释放：str / list / dict。标量、ptr、class 实例、
    //     any、函数值一律不释放（D14：引用语义，释放必双重释放）。
    //   * list/dict 的释放要按**元素类型**递归：list[str] 先逐元素释放每个
    //     str 的堆块，再释放 data 块，最后释放 header。这就是「drop glue」。
    //   * 每个类型对应一个 mangled 名的函数，在实例缓存里去重，出口只 call。
    //
    // 为什么 drop 函数收**槽位地址**而不是值：出口处要把槽位写成 null
    // （调试钩子 D17 靠它检测重复释放），按值传就拿不到槽位了。
    bool CodeGen::needsDrop(const sema::TypePtr& type) const    {
        if (!type) return false;
        switch (type->getKind())        {
        case sema::TypeKind::TK_STR:
        case sema::TypeKind::TK_LIST:
        case sema::TypeKind::TK_DICT:
            return true;
        // P13-2/L3：内存模型类型需要进 drop 表以跟踪 F/X 状态（移动语义），
        // 实际资源释放由 L3 主体的 fill/release 负责，这里的 drop glue
        // 只把槽写 null（见 emitDropGlueBody 默认分支）。
        case sema::TypeKind::TK_MEMORY:
            return true;
        // D14：范围外类型一律不释放。
        //   TK_CLASS   —— 引用语义，可能有别名
        //   TK_FUNC    —— 闭包对象，env 归属未定
        //   TK_PTR     —— 裸指针，所有权不在语言层
        //   TK_INT/FLOAT/BOOL/CHAR/VOID —— 无资源
        default:
            return false;
        }
    }
    std::string CodeGen::ensureDropGlue(const sema::TypePtr& type)    {
        if (!needsDrop(type)) return std::string();
        const std::string mangle = mangleType(type);
        if (const auto found = _dropGlue.find(mangle); found != _dropGlue.end())        {
            return found->second;
        }
        const std::string fnName = "__rio_drop_" + mangle;
        // 先登记再生成，递归类型（若将来允许）不会无限展开。
        _dropGlue[mangle] = fnName;
        _pendingDropGlue.emplace_back(fnName, type);
        return fnName;
    }
    void CodeGen::emitDropGlueBody(const std::string& fnName, const sema::TypePtr& type)    {
        // 签名：void fn(ptr slot) —— slot 是变量的**栈槽地址**。
        // 约定：读出值 -> 若为 null 直接返回 -> 释放 -> 把槽写成 null。
        //
        // ============ P13-1 的关键取舍：不递归释放元素 ============
        //
        // 只释放容器**自己**的内存（data/keys/vals 块 + header），**不**逐元素
        // 释放。原因是 P13-1 还没有任何机制能判断「元素归不归这个容器所有」：
        //
        //   * `dictKeys(d)` 返回的 list[str] 里装的是 d 内部数组的**键指针**，
        //     d 还活着，元素不是这个 list 的；
        //   * `ds.copy(xs)` 是浅拷贝，同样共享元素；
        //   * `xs += [otherContainerElement]` 也是把别人的指针塞进来。
        //
        // 逐元素释放会把上面这些全部变成 use-after-free —— 实测
        // `dumpToStr(dict)` 直接崩在堆上。判定「元素所有权」需要 P13-4 的
        // 子引用路径与成员集合追踪，按 D14 的口径登记为**已知泄漏**：
        // list[str] / dict[K,str] 的元素堆数据 P13-1 不回收。
        //
        // 这与 D14 对 `list[Foo]` 的处置完全一致（那里写的是「只释放缓冲区，
        // 不释放元素」），这里把同一口径推广到全部元素类型。
        //
        // **不发 .globl**：drop glue 是编译器在每个翻译单元内部生成的私有
        // 辅助函数，不是库导出的 API。修前这里发了 `.globl __rio_drop_<mangle>`，
        // 而 `release/lib/rt/rt.o` 会被链接进**每一个**程序 —— 只要用户程序里
        // 也出现 str / list / dict 变量（几乎必然），两边就各定义一份同名全局
        // 符号，链接直接失败：
        //
        //   multiple definition of `__rio_drop_K';
        //   ...rt.o: first defined here / user.o: first defined here
        //
        // 2026-11-18 实测：`var s: str = "hi"` 这样的最小程序都编不过。
        // 早先没暴露是因为 `rt.o` 是 P13-1 之前的陈旧产物（不含 drop glue），
        // 重建 rt 层之后才浮出来 —— 属既有缺陷，与本次改动无关。
        //
        // 去掉 .globl 后每个 TU 各有一份**局部**符号，互不干扰；
        // 它们是纯函数，重复存在没有语义代价。
        _asm.label(fnName);
        _asm.push("%rbp");
        _asm.mov("%rsp", "%rbp");
        _asm.subRsp(32);
        // %rcx = slot，保存到栈上（-8(%rbp)）
        _asm.mov("%rcx", "-8(%rbp)");
        // 值 -> %rax
        _asm.mov("-8(%rbp)", "%rcx");
        _asm.inst("movq", "(%rcx)", "%rax");
        const std::string nullLabel = _asm.newLabel("drop_null");
        const std::string doneLabel = _asm.newLabel("drop_done");
        _asm.inst("testq", "%rax", "%rax");
        _asm.je(nullLabel);

        const auto kind = type->getKind();
        if (kind == sema::TypeKind::TK_STR)        {
            // str 只需释放头（内容是内联在头后面的）。
            // %rax 就是槽里存的值（header 指针），从第 580 行读出来还没被动过。
            _asm.mov("%rax", "%rcx");
            _asm.subRsp(32);
            _asm.call("core_free");
            _asm.addRsp(32);
            _asm.jmp(doneLabel);
        }
 else if (kind == sema::TypeKind::TK_LIST)        {
            // list：释放 data 块，再释放 header（元素不递归，见上面的说明）。
            //
            // 层次要分清：slot 是**变量槽**，(%slot) 才是 header 指针；
            // header 的字段才是 24/16/0 这些偏移。少一层间接是自由的野地址，
            // 实测直接堆损坏（0xC0000374）。
            _asm.mov("-8(%rbp)", "%rcx");
            _asm.inst("movq", "(%rcx)", "%rcx");
            // %rcx = header
            _asm.inst("movq", "24(%rcx)", "%rcx");
            // header->data
            _asm.subRsp(32);
            _asm.call("core_free");
            _asm.addRsp(32);
            _asm.mov("-8(%rbp)", "%rcx");
            _asm.inst("movq", "(%rcx)", "%rcx");
            // header
            _asm.subRsp(32);
            _asm.call("core_free");
            _asm.addRsp(32);
            _asm.jmp(doneLabel);
        }
 else if (kind == sema::TypeKind::TK_DICT)        {
            // dict：释放 keys 块、vals 块、header（键/值元素不递归）
            _asm.mov("-8(%rbp)", "%rcx");
            _asm.inst("movq", "(%rcx)", "%rcx");
            _asm.inst("movq", "16(%rcx)", "%rcx");
            // header->keys
            _asm.subRsp(32);
            _asm.call("core_free");
            _asm.addRsp(32);
            _asm.mov("-8(%rbp)", "%rcx");
            _asm.inst("movq", "(%rcx)", "%rcx");
            _asm.inst("movq", "24(%rcx)", "%rcx");
            // header->vals
            _asm.subRsp(32);
            _asm.call("core_free");
            _asm.addRsp(32);
            _asm.mov("-8(%rbp)", "%rcx");
            _asm.inst("movq", "(%rcx)", "%rcx");
            // header
            _asm.subRsp(32);
            _asm.call("core_free");
            _asm.addRsp(32);
        }
        _asm.label(nullLabel);
        // 把槽位写成 null（D17：调试钩子据此检测重复释放）
        _asm.mov("-8(%rbp)", "%rcx");
        _asm.movImm(0, "%rax");
        _asm.mov("%rax", "(%rcx)");
        _asm.label(doneLabel);
        _asm.mov("%rbp", "%rsp");
        _asm.pop("%rbp");
        _asm.ret();
    }
    void CodeGen::registerDropVar(const std::string& name, const std::string& addr,
                                  const sema::TypePtr& type)    {
        if (!needsDrop(type)) return;
        if (_scopeDrops.empty()) _scopeDrops.emplace_back();
        _scopeDrops.back().push_back({name, addr, type, false, false});
    }
    void CodeGen::registerMoveVar(const std::string& name)    {
        if (_moveStates.empty()) _moveStates.emplace_back();
        // 内层遮蔽外层：只在最内层登记，isVarMoved 由内向外查。
        _moveStates.back()[name] = BodyState::FULL;
    }
    std::string CodeGen::poolNewReceiverName(const std::shared_ptr<ast::ExpressionNode>& node)    {
        // `pool.new[T](args)`：FunctionCallNode(callee=Infix(DOT,
        // left=Identifier(pool), right=IndexExpression(new, [T])))。
        const auto call = std::dynamic_pointer_cast<ast::FunctionCallNode>(node);
        if (!call) return std::string();
        const auto member = std::dynamic_pointer_cast<ast::InfixExpressionNode>(call->getLeftNode());
        if (!member || member->getOpToken().getType() != core::TokenType::TOKEN_DOT) return std::string();
        const auto idx = std::dynamic_pointer_cast<ast::IndexExpressionNode>(member->getRightNode());
        if (!idx) return std::string();
        const auto mid = std::dynamic_pointer_cast<ast::IdentifierNode>(idx->getLeftNode());
        if (!mid || mid->getName() != "new") return std::string();
        const auto recv = std::dynamic_pointer_cast<ast::IdentifierNode>(member->getLeftNode());
        return recv ? recv->getName() : std::string();
    }
    std::string CodeGen::findBirthPlaceLabel(
        const std::vector<std::shared_ptr<ast::LabelNode>>& labels) const    {
        // 决议 §2.2：出生地标签是 Pool 类型的变量名。标签区顺序不敏感，
        // 判定顺序「内置类型 → Pool → 自定义 → 出生地」（Q-A）。
        // 这里只认「能解析为 Pool 类型局部变量」的标签。
        for (const auto& label : labels)        {
            if (!label) continue;
            const auto name = label->getFullLabel();
            if (name.empty()) continue;
            const auto slot = const_cast<CodeGen*>(this)->lookupVar(name);
            if (slot && slot->type && slot->type->getKind() == sema::TypeKind::TK_POOL)
                return name;
        }
        return std::string();
    }
    std::vector<std::string> CodeGen::findAllBirthPlaceLabels(
        const std::vector<std::shared_ptr<ast::LabelNode>>& labels) const    {
        // 决议 §2.2：标签区**最多一个**出生地标签。
        // findBirthPlaceLabel 只返回第一个，无法发现第二个，实测
        // `var a: Player p1 p2 = Player("x")` 静默通过。这里全量收集。
        std::vector<std::string> found;
        for (const auto& label : labels)        {
            if (!label) continue;
            const auto name = label->getFullLabel();
            if (name.empty()) continue;
            const auto slot = const_cast<CodeGen*>(this)->lookupVar(name);
            if (slot && slot->type && slot->type->getKind() == sema::TypeKind::TK_POOL)
                found.push_back(name);
        }
        return found;
    }
    void CodeGen::registerBorrow(const std::string& name, const sema::TypePtr& type,
                                 const std::string& addr)    {
        if (!needsDrop(type)) return;
        if (_scopeDrops.empty()) _scopeDrops.emplace_back();
        // 借用也进登记表（这样 isBorrowed 能找到它），但 moved=true 表示
        // 「出口不销毁」—— 借用本来就不拥有资源。
        //
        // addr 必须记下来：借用被重新赋值（`ys = <新值>`）时会升格成拥有者，
        // 那时出口要拿这个地址去释放。形参的槽位地址在 emitFunctionBody 里
        // 算完之后才登记，所以此时能传进来。
        _scopeDrops.back().push_back({name, addr, type, true, true});
    }
    bool CodeGen::isBorrowed(const std::string& name) const    {
        for (auto it = _scopeDrops.rbegin(); it != _scopeDrops.rend(); ++it)        {
            for (const auto& e : *it)            {
                if (e.name == name) return e.borrowed;
            }
        }
        return false;
    }
    void CodeGen::collectEnsuresItems(const std::shared_ptr<ast::ExpressionNode>& node,
                                      std::optional<BodyState> inherited,
                                      std::vector<std::pair<std::string, std::optional<BodyState>>>& out)    {
        // P13-2/L3-H（决议 §9.2）：`ensures` 的展平不能直接用 flattenTypeArgs。
        //
        // 原因：`!` / `?` / `+` 是前缀运算符，而 parser 让前缀的优先级**低于**逗号，
        // 所以 `ensures[!t, !t]` 被解析成 `!(t, !t)` —— 整个 `t, !t` 是 `!` 的操作数。
        // 直接展平只会看到一个节点，名字解析为空后被静默跳过，于是整条子句
        // 既不登记也不报错。实测（修前）：`ensures[!t, !t]` 与 `ensures[?t, ?t]`
        // 都 exit=0，而 `ensures[!t]` 单写却能正确报错。
        //
        // 正确做法：把「前缀标记」当作**继承状态**往下传，逗号每一项各自继承，
        // 这样 `!(t, !t)` 会被还原成 `(!t, !t)`。
        if (!node) return;
        if (const auto infix = std::dynamic_pointer_cast<ast::InfixExpressionNode>(node))        {
            if (infix->getOpToken().getType() == core::TokenType::TOKEN_COMMA)            {
                collectEnsuresItems(infix->getLeftNode(), inherited, out);
                collectEnsuresItems(infix->getRightNode(), inherited, out);
                return;
            }
        }
        if (const auto paren = std::dynamic_pointer_cast<ast::ParenRangerNode>(node))        {
            collectEnsuresItems(paren->getRangerNode(), inherited, out);
            return;
        }
        if (const auto list = std::dynamic_pointer_cast<ast::ListExpressionNode>(node))        {
            collectEnsuresItems(list->getBodyNode(), inherited, out);
            return;
        }
        if (const auto unary = std::dynamic_pointer_cast<ast::UnaryExpressionNode>(node))        {
            const auto op = unary->getOpToken().getType();
            const auto st = (op == core::TokenType::TOKEN_NOT)
                                ? BodyState::EMPTY :
                                (op == core::TokenType::TOKEN_QUESTION ? BodyState::UNKNOWN : BodyState::FULL);
            collectEnsuresItems(unary->getRightNode(), st, out);
            return;
        }
        if (const auto prefix = std::dynamic_pointer_cast<ast::PrefixExpressionNode>(node))        {
            const auto op = prefix->getMainToken().getType();
            const auto st = (op == core::TokenType::TOKEN_NOT)
                                ? BodyState::EMPTY :
                                (op == core::TokenType::TOKEN_QUESTION ? BodyState::UNKNOWN : BodyState::FULL);
            collectEnsuresItems(prefix->getNode(), st, out);
            return;
        }
        std::string name;
        if (const auto id = std::dynamic_pointer_cast<ast::IdentifierNode>(node)) name = id->getName();
        else if (const auto lbl = std::dynamic_pointer_cast<ast::LabelNode>(node)) name = lbl->getFullLabel();
        if (name.empty()) return;
        out.emplace_back(name, inherited);
    }
    void CodeGen::validateEnsuresClauses(const std::vector<std::string>& paramNames,
                                         const std::vector<sema::TypePtr>& paramTypes)    {
        _ensuresMarks.clear();
        for (const auto& clause : _currentSignatureClauses)        {
            if (!clause || clause->getFullLabel() != "ensures") continue;
            const auto des = clause->getLabelDesNodes();
            if (des.empty() || !des[0] || !des[0]->getBodyNode()) continue;
            std::vector<std::pair<std::string, std::optional<BodyState>>> items;
            collectEnsuresItems(des[0]->getBodyNode(), std::nullopt, items);
            for (const auto& [name, explicitState] : items)            {
                // 决议 §9.2：每条形参只能有一个状态。修前 `ensures[!t, !t]` 静默通过
                // —— 前缀标记吞掉了逗号，整条子句根本没被登记。
                if (_ensuresMarks.find(name) != _ensuresMarks.end())                {
                    failAt(clause->getMainToken().getPos(),
                           "L3: ensures lists parameter '" + name +
                           "' more than once; each parameter may have only one state "
                           "(P13-2 / §9.2)");
                    return;
                }
                // 决议 §9.2：只对 Take[T] 形参生效，列其他类型报错。
                size_t idx = paramNames.size();
                for (size_t i = 0; i < paramNames.size(); ++i)
                    if (paramNames[i] == name) { idx = i; break; }
                const bool isTake = idx < paramTypes.size() && paramTypes[idx] &&
                    paramTypes[idx]->getKind() == sema::TypeKind::TK_MEMORY &&
                    std::dynamic_pointer_cast<sema::MemoryType>(paramTypes[idx])->getRole() ==
                        sema::MemoryRole::TAKE;
                if (!isTake)                {
                    failAt(clause->getMainToken().getPos(),
                           "L3: ensures['" + name + "'] requires a Take[T] parameter "
                           "(P13-2 / §9.2)");
                    return;
                }
                // 裸名 = Full（决议 §9.2）
                _ensuresMarks[name] = explicitState.value_or(BodyState::FULL);
            }
        }
        _currentSignatureClauses.clear(); // 消费后清空，避免泄漏到后续函数体
    }
    std::string CodeGen::clauseItemRoot(
        const std::shared_ptr<ast::ExpressionNode>& item)    {
        // P13-2/L3-F（决议 §9.3/§9.4）：取 `writes[p.name]` / `from[p]` 的根标识符。
        // 支持 p / p.name / p[i] / &p.name 这几种写法。
        if (!item) return std::string();
        if (const auto id = std::dynamic_pointer_cast<ast::IdentifierNode>(item)) return id->getName();
        if (const auto lbl = std::dynamic_pointer_cast<ast::LabelNode>(item)) return lbl->getFullLabel();
        if (const auto ref = std::dynamic_pointer_cast<ast::RefExpressionNode>(item))
            return clauseItemRoot(ref->getOperand());
        if (const auto inf = std::dynamic_pointer_cast<ast::InfixExpressionNode>(item))
            return clauseItemRoot(inf->getLeftNode());
        if (const auto idx = std::dynamic_pointer_cast<ast::IndexExpressionNode>(item))
            return clauseItemRoot(idx->getLeftNode());
        if (const auto unary = std::dynamic_pointer_cast<ast::UnaryExpressionNode>(item))
            return clauseItemRoot(unary->getRightNode());
        if (const auto prefix = std::dynamic_pointer_cast<ast::PrefixExpressionNode>(item))
            return clauseItemRoot(prefix->getNode());
        return std::string();
    }
    void CodeGen::validateWritesFromClauses(const std::vector<std::string>& paramNames)    {
        // P13-2/L3-F（决议 §9.3/§9.4）：writes / from 的每一项都必须是形参的路径。
        // 修前这两个子句完全没有校验 —— 实测 `writes[q.name]`（q 不是形参）、
        // `from[zz.name]`（zz 根本不存在）都静默通过。
        _fromRoots.clear();
        for (const auto& clause : _currentSignatureClauses)        {
            if (!clause) continue;
            const auto label = clause->getFullLabel();
            if (label != "writes" && label != "from") continue;
            const auto des = clause->getLabelDesNodes();
            if (des.empty() || !des[0] || !des[0]->getBodyNode()) continue;
            std::vector<std::shared_ptr<ast::ExpressionNode>> items;
            flattenTypeArgs(des[0]->getBodyNode(), items);
            for (const auto& item : items)            {
                const auto root = clauseItemRoot(item);
                if (root.empty())                {
                    failAt(clause->getMainToken().getPos(),
                           "L3: '" + label + "' clause entries must be parameter paths (P13-2 / §9.3)");
                    return;
                }
                if (!isFunctionParam(root))                {
                    failAt(clause->getMainToken().getPos(),
                           "L3: '" + label + "[" + root + "]' refers to '" + root +
                           "', which is not a parameter of this function (P13-2 / §9.3)");
                    return;
                }
                if (label == "from") _fromRoots.insert(root);
            }
        }
    }
    void CodeGen::checkEnsuresAtExit(const utils::Pos& pos)    {
        for (const auto& [name, expected] : _ensuresMarks)        {
            const auto actual = getBodyState(name);
            // 决议 §9.2：? = 不确定，任何状态都满足；其余须精确匹配。
            if (expected == BodyState::UNKNOWN) continue;
            if (actual == BodyState::UNKNOWN)            {
                failAt(pos, "L3: Take parameter '" + name +
                       "' has unknown state at function exit; ensures requires a definite state");
                return;
            }
            if (actual != expected)            {
                failAt(pos, "L3: Take parameter '" + name + "' is " + bodyStateName(name) +
                       " at exit, but ensures requires " +
                       (expected == BodyState::FULL ? "Full" : "Empty"));
                return;
            }
        }
    }
    bool CodeGen::isFunctionParam(const std::string& name) const    {
        // P13-2/L3-I（决议 §9.3/§9.4）：方法里的 `this` 等价于一个形参 ——
        // `writes[this.x]` 与 `from[this.x]` 是决议明文允许的写法，
        // 但它既不在形参表里，也不是普通局部变量。修前把它当普通局部：
        //   fun refSelf(): Ref[str] from[this.name] { ret &this.name }
        //   → 误报「cannot return a reference to local 'this'」
        //   fun hit(): int writes[this.hp] { this.hp = this.hp - 1 ... }
        //   → 被当成「不是形参的路径」
        if (name == "this" && !_currentClass.empty()) return true;
        const auto it = _functionParamNames.find(_functionName);
        if (it == _functionParamNames.end()) return false;
        for (const auto& p : it->second) if (p == name) return true;
        return false;
    }
    CodeGen::BodyState CodeGen::getBodyState(const std::string& name) const    {
        for (auto it = _moveStates.rbegin(); it != _moveStates.rend(); ++it)        {
            if (const auto found = it->find(name); found != it->end()) return found->second;
        }
        return BodyState::FULL; // 未登记（非 Body 变量）视作满，不阻挡操作
    }
    void CodeGen::setBodyState(const std::string& name, BodyState state,
                               const utils::Pos& pos, const std::string& action)    {
        for (auto it = _moveStates.rbegin(); it != _moveStates.rend(); ++it)        {
            if (const auto found = it->find(name); found != it->end())            {
                found->second = state;
                return;
            }
        }
        (void)pos; (void)action; // 未登记变量：静默忽略（非 Body 变量）
    }
    CodeGen::BodyState CodeGen::joinBodyStates(BodyState a, BodyState b)    {
        // 决议 §8.1：合流逐项取格上界。相同状态保持不变（X ⊔ X = X）。
        if (a == b) return a;
        // H 是 F 的子态（take 挂起期间内容位置没动，只是当前语句不能用），
        // 所以 H ⊔ F = F —— 一条路径把它留在挂起、另一条回到满，取满即可。
        if ((a == BodyState::HELD && b == BodyState::FULL) ||
            (a == BodyState::FULL && b == BodyState::HELD))
            return BodyState::FULL;
        // 其余不同组合（F ⊔ E / F ⊔ X / E ⊔ X …）一律降为 ?：
        // 编译期不能再假定是哪一种，后续只能走 get() 之类的安全入口。
        return BodyState::UNKNOWN;
    }
    CodeGen::MoveStateSnapshot
    CodeGen::joinMoveStateSnapshots(const std::vector<MoveStateSnapshot>& paths) const    {
        if (paths.empty()) return _moveStates;
        // 深度取最小：更深层的 map 属于分支/循环体内的局部量，popScope 后
        // 已随作用域消失，不能把它们带到外面来。
        size_t depth = _moveStates.size();
        for (const auto& p : paths) depth = std::min(depth, p.size());
        MoveStateSnapshot acc(paths.front().begin(), paths.front().begin() + depth);
        for (size_t p = 1; p < paths.size(); ++p)        {
            const auto& other = paths[p];
            for (size_t i = 0; i < depth; ++i)            {
                for (auto& entry : acc[i])                {
                    const auto it = other[i].find(entry.first);
                    // 该路径没碰过这项 = 它保持入口状态，与入口一致，保持原状。
                    if (it == other[i].end()) continue;
                    entry.second = joinBodyStates(entry.second, it->second);
                }
                for (const auto& entry : other[i])                {
                    if (acc[i].find(entry.first) == acc[i].end())
                        acc[i][entry.first] = entry.second;
                }
            }
        }
        return acc;
    }
    void CodeGen::refreshOpaqueAfterMerge(const std::vector<MoveStateSnapshot>& paths)    {
        // 决议 §9.2 的 Take 形参入口态 `?` 是「编译器不做承诺」；一旦某条路径
        // 把它推到了确定状态（take→E / fill→F），合流出来的 `?` 就变成真正的
        // 「不确定」了，不再享有无条件放行。
        auto stateIn = [](const MoveStateSnapshot& snap, const std::string& name) -> const BodyState* {
            for (auto it = snap.rbegin(); it != snap.rend(); ++it)            {
                const auto found = it->find(name);
                if (found != it->end()) return &found->second;
            }
            return nullptr;
        };
        for (auto oit = _opaqueMoveVars.begin(); oit != _opaqueMoveVars.end();)        {
            bool narrowed = false;
            for (const auto& p : paths)            {
                const auto* st = stateIn(p, *oit);
                if (st && *st != BodyState::UNKNOWN)                {
                    narrowed = true;
                    break;
                }
            }
            if (narrowed) oit = _opaqueMoveVars.erase(oit);
            else          ++oit;
        }
    }
    void CodeGen::joinMoveStatePaths(const std::vector<MoveStateSnapshot>& paths)    {
        if (paths.empty()) return;
        _moveStates = joinMoveStateSnapshots(paths);
    }
    bool CodeGen::isAlwaysTrueCondition(const std::shared_ptr<ast::ExpressionNode>& cond)    {
        // parser 把 `else` 造成「条件 = BooleanLiteralNode(true)」的分支
        // （buildConditionExpression），所以字面 true 就是无条件 else。
        const auto lit = std::dynamic_pointer_cast<ast::BooleanLiteralNode>(cond);
        return lit && lit->literalString() == "true";
    }
    bool CodeGen::statementDiverges(const std::shared_ptr<ast::ExpressionNode>& node)    {
        // 判断一条语句之后是否**不可能**再往下走（ret / throw / break / continue）。
        // 分支出入口做状态合流时，发散的分支不会到达汇合点，不能参与合流，
        // 否则 `if c { return } else { bob.clear() }` 之后会把 bob 误判成 ?。
        if (!node) return false;
        if (std::dynamic_pointer_cast<ast::ReturnExpressionNode>(node)) return true;
        if (std::dynamic_pointer_cast<ast::ThrowNode>(node)) return true;
        if (std::dynamic_pointer_cast<ast::BreakExpressionNode>(node)) return true;
        if (std::dynamic_pointer_cast<ast::ContinueExpressionNode>(node)) return true;
        if (const auto block = std::dynamic_pointer_cast<ast::BlockRangerNode>(node))        {
            const auto& body = block->getBodyExpressions();
            return !body.empty() && statementDiverges(body.back());
        }
        if (const auto cond = std::dynamic_pointer_cast<ast::ConditionNode>(node))        {
            // if/elif 链：只有存在「无条件 else」且每个分支都发散，整体才发散。
            bool hasElse = false;
            for (const auto& branchExpr : cond->getBranchNodes())            {
                const auto branch = std::dynamic_pointer_cast<ast::BranchNode>(branchExpr);
                if (!branch) continue;
                if (isAlwaysTrueCondition(branch->getConditionNode())) hasElse = true;
                if (!statementDiverges(branch->getBodyNode())) return false;
            }
            return hasElse;
        }
        return false;
    }
    bool CodeGen::isVarMoved(const std::string& name) const    {
        // P13-2/L3：先查独立的移动状态表（含 ClassType/Body 别名等
        // D14 不进 drop 表但参与 take/移动的变量）。
        for (auto it = _moveStates.rbegin(); it != _moveStates.rend(); ++it)        {
            if (const auto found = it->find(name); found != it->end())
                return found->second == BodyState::MOVED;
        }
        // 再查 P13-1 的 drop 表（str/list/dict/Content 等资源拥有者）。
        // 由内向外找第一个同名项（内层遮蔽外层）。
        // 借用的 moved 字段恒为 true（借用不销毁），但那是「不用管生命周期」
        // 而不是「已被移走」，所以只看非借用项。
        for (auto it = _scopeDrops.rbegin(); it != _scopeDrops.rend(); ++it)        {
            for (const auto& e : *it)            {
                if (e.name != name) continue;
                if (e.borrowed) return false;
                return e.moved;
            }
        }
        return false;
    }
    void CodeGen::beginPendingTake(const std::string& name, const utils::Pos& pos)    {
        if (_pendingTakes.count(name))        {
            failAt(pos, "L3: '" + name +
                   "' is already taken in this statement (pending_take is exclusive)");
            return;
        }
        _pendingTakes.insert(name);
        setBodyState(name, BodyState::HELD, pos, "taken");
    }
    std::string CodeGen::exprPathText(const std::shared_ptr<ast::ExpressionNode>& node)    {
        // P13-2/L3-F（决议 §4.2）：把表达式还原成路径文本 ——
        // `p` -> "p"，`p.name` -> "p.name"，`p.a.b` -> "p.a.b"。
        // 只认标识符与 `.` 链；下标、调用、字面量一律返回空串（无法精确追踪）。
        if (!node) return std::string();
        if (const auto id = std::dynamic_pointer_cast<ast::IdentifierNode>(node)) return id->getName();
        if (const auto inf = std::dynamic_pointer_cast<ast::InfixExpressionNode>(node))        {
            if (inf->getOpToken().getType() != core::TokenType::TOKEN_DOT) return std::string();
            const auto fname = std::dynamic_pointer_cast<ast::IdentifierNode>(inf->getRightNode());
            if (!fname) return std::string();
            const std::string base = exprPathText(inf->getLeftNode());
            if (base.empty()) return std::string();
            return base + "." + fname->getName();
        }
        if (const auto paren = std::dynamic_pointer_cast<ast::ParenRangerNode>(node))
            return exprPathText(paren->getRangerNode());
        return std::string();
    }
    bool CodeGen::pathIsPrefixOf(const std::string& p, const std::string& q)    {
        // P13-2/L3-F（规范 §4.1）：按**路径段**比较，不是按字符 ——
        // "p.n" 不能算 "p.name" 的前缀。
        if (p.empty() || q.empty()) return false;
        if (p == q) return true;
        if (q.size() <= p.size()) return false;
        if (q.compare(0, p.size(), p) != 0) return false;
        // 下一字符必须是路径分隔符（'.' 或 '['），否则是同一个名字的更长拼写
        return q[p.size()] == '.' || q[p.size()] == '[';
    }
    bool CodeGen::checkWriteConflicts(const std::string& path, const utils::Pos& pos,
                                      const std::string& what)    {
        // P13-2/L3-F（决议 §4.1 + §4.2）：修改路径 P 与活跃子引用路径 Q
        // 冲突，当且仅当 P 是 Q 的前缀（含相等）。
        //   p.name = "y"  vs  &p.name  → 相等，冲突
        //   p.take()      vs  &p.name  → "p" 是 "p.name" 前缀，冲突
        //   p.score = 99  vs  &p.name  → 不相交，放行
        if (path.empty()) return true; // 无法解析：不在这里报错（调用点自己保守）
        for (const auto& ref : _activeRefs)        {
            // 只比子引用（壳子引用 refVar 为空、且路径就是 owner 的，属于
            // 「壳子引用」——规范 §4.2 明确壳子引用不受影响）。
            if (ref.path.empty()) continue;
            if (ref.path == ref.owner) continue; // 壳子引用
            if (pathIsPrefixOf(path, ref.path))            {
                failAt(pos, "L3: " + what + " '" + path + "' conflicts with the live " +
                       "sub-reference '" + ref.path + "' (P13-2 / §4.1)");
                return false;
            }
        }
        return true;
    }
    std::string CodeGen::resolveEffectPath(const std::string& root, const std::string& suffix,
                                           const std::vector<std::string>& paramNames,
                                           const std::vector<std::shared_ptr<ast::ExpressionNode>>& args) const    {
        // P13-2/L3-F（决议 §4.2）：把形参名映射回调用点的实参路径。
        // 实参不是可追踪的路径（字面量、构造调用……）时返回空串 —— 保守，
        // 调用方遇到空串就跳过检查（宁可不报也不误报）。
        for (size_t i = 0; i < paramNames.size(); ++i)        {
            if (paramNames[i] != root) continue;
            if (i >= args.size()) return std::string();
            const std::string base = exprPathText(args[i]);
            if (base.empty()) return std::string();
            return base + suffix;
        }
        return std::string();
    }
    void CodeGen::registerSignatureEffects(
        const std::string& emitName,
        const std::shared_ptr<ast::FunctionDefinitionNode>& def,
        const std::vector<std::string>& paramNames)    {
        // P13-2/L3-F（决议 §9.3/§9.4）：把签名里的 writes / from 解析成效果表。
        // 必须在发射任何函数体之前登记完，否则前向调用（f1 里调文件后面
        // 才定义的 f2）在调用点拿不到效果。
        _writeEffects.erase(emitName);
        _fromEffects.erase(emitName);
        if (!def) return;
        for (const auto& clause : def->getSignatureClauses())        {
            if (!clause) continue;
            const auto label = clause->getFullLabel();
            if (label != "writes" && label != "from") continue;
            const auto des = clause->getLabelDesNodes();
            if (des.empty() || !des[0] || !des[0]->getBodyNode()) continue;
            std::vector<std::shared_ptr<ast::ExpressionNode>> items;
            flattenTypeArgs(des[0]->getBodyNode(), items);
            for (const auto& item : items)            {
                // 用 exprPathText 拿完整路径（"p.name"），再拆成 root + suffix
                std::string full = exprPathText(item);
                if (full.empty())                {
                    // `&p.name` 这类带 & 的写法：剥掉取引用再看
                    if (const auto ref = std::dynamic_pointer_cast<ast::RefExpressionNode>(item))
                        full = exprPathText(ref->getOperand());
                }
                if (full.empty()) continue;
                const auto dot = full.find('.');
                WriteEffect eff;
                if (dot == std::string::npos)                {
                    eff.root = full;
                    eff.suffix.clear();
                }
  else                {
                    eff.root = full.substr(0, dot);
                    eff.suffix = full.substr(dot);
                }
                bool isParam = false;
                for (const auto& p : paramNames) if (p == eff.root) { isParam = true; break; }
                if (!isParam) continue; // 非法项由 validateWritesFromClauses 报错
                if (label == "writes") _writeEffects[emitName].push_back(eff);
                else _fromEffects[emitName].push_back(eff);
            }
        }
    }
    void CodeGen::registerActiveRef(const std::string& path, const std::string& owner,
                                    const std::string& refVar, const utils::Pos& pos,
                                    const std::string& indexExpr)    {
        // 决议 §4.1：路径穿透引用展开 ——
        // path 是完整路径（如 "p.name"）。若根 owner（p）本身是已登记的
        // 引用变量（如 &q.x 得到的 p），把前缀替换成该引用的 path，
        // 得到 "q.x.name"（精确登记）。
        std::string full = path;
        for (const auto& existing : _activeRefs)            {
            if (!existing.refVar.empty() && existing.refVar == owner &&
                path.rfind(owner, 0) == 0)                {
                full = existing.path + path.substr(owner.size());
                break;
            }
        }
        _activeRefs.push_back({full, owner, refVar, true, indexExpr}); // justBound：绑定语句不清除
        (void)pos;
    }
    void CodeGen::bindLastActiveRef(const std::string& refVar)    {
        if (!_activeRefs.empty())        {
            _activeRefs.back().refVar = refVar;
            _activeRefs.back().justBound = true; // 标记绑定语句，drain 时豁免一次
        }
    }
    static bool nodeUsesVarImpl(const std::shared_ptr<ast::ExpressionNode>& node,
                                const std::string& name)    {
        if (!node) return false;
        if (const auto id = std::dynamic_pointer_cast<ast::IdentifierNode>(node))
            return id->getName() == name;
        if (const auto inf = std::dynamic_pointer_cast<ast::InfixExpressionNode>(node))
            return nodeUsesVarImpl(inf->getLeftNode(), name) || nodeUsesVarImpl(inf->getRightNode(), name);
        if (const auto call = std::dynamic_pointer_cast<ast::FunctionCallNode>(node))
            return nodeUsesVarImpl(call->getLeftNode(), name) || nodeUsesVarImpl(call->getRightNode(), name);
        if (const auto block = std::dynamic_pointer_cast<ast::BlockRangerNode>(node))        {
            for (const auto& e : block->getBodyExpressions()) if (nodeUsesVarImpl(e, name)) return true;
            return false;
        }
        if (const auto asn = std::dynamic_pointer_cast<ast::AssignmentNode>(node))        {
            const auto p = asn->getAssignPair();
            return nodeUsesVarImpl(p.first, name) || nodeUsesVarImpl(p.second, name);
        }
        if (const auto vd = std::dynamic_pointer_cast<ast::VariableDefinitionNode>(node))        {
            for (const auto& def : vd->getVarDefs())            {
                if (def && def->hasInitialValue() && nodeUsesVarImpl(def->getValueNode(), name)) return true;
            }
            return false;
        }
        if (const auto ref = std::dynamic_pointer_cast<ast::RefExpressionNode>(node))
            return nodeUsesVarImpl(ref->getOperand(), name);
        return false;
    }
    void CodeGen::drainActiveRefs(const std::shared_ptr<ast::ExpressionNode>& stmt)    {
        // 决议 §4.2：NLL —— 引用活跃到最后使用。语句里用到某引用变量，
        // 则它继续活跃；没用到且本语句不是其绑定语句，则视为最后使用已过。
        for (auto it = _activeRefs.begin(); it != _activeRefs.end(); )        {
            if (it->justBound)            {
                it->justBound = false; // 绑定语句豁免，下一次 drain 才开始 NLL
                ++it; continue;
            }
            if (it->refVar.empty()) { ++it; continue; } // 未绑定变量名的暂保守保留
            if (!nodeUsesVarImpl(stmt, it->refVar)) it = _activeRefs.erase(it);
            else ++it;
        }
    }
    bool CodeGen::hasActiveRefOn(const std::string& owner,
                                 const std::string& indexExpr) const    {
        // 决议 §4.3：不等事实 i≠j 作为事实表一维，消费时放宽冲突。
        // 只有两个不同的**整数字面量**下标才能证明不相等。
        const auto isIntLiteral = [](const std::string& s) {
            if (s.empty()) return false;
            size_t i = (s[0] == '-' || s[0] == '+') ? 1 : 0;
            if (i >= s.size()) return false;
            for (; i < s.size(); ++i) if (!std::isdigit(static_cast<unsigned char>(s[i]))) return false;
            return true;
        };
        for (const auto& ref : _activeRefs)        {
            if (ref.owner != owner) continue;
            if (ref.indexExpr.empty() || indexExpr.empty()) return true; // 静态路径，保守冲突
            if (!isIntLiteral(ref.indexExpr) || !isIntLiteral(indexExpr)) return true;
            if (ref.indexExpr == indexExpr) return true; // 同一下标，冲突
            // 不同字面量：i≠j 成立，此条不冲突，继续看其它引用
        }
        return false;
    }
    bool CodeGen::hasActiveRefOn(const std::string& owner) const    {
        for (const auto& ref : _activeRefs)
            if (ref.owner == owner) return true;
        return false;
    }
    bool CodeGen::checkNotPending(const std::string& name, const utils::Pos& pos,
                                  const std::string& action)    {
        if (_pendingTakes.count(name))        {
            failAt(pos, "L3: '" + name + "' is pending take and cannot be " +
                   action + " (pending_take is exclusive)");
            return false;
        }
        return true;
    }
    void CodeGen::settlePendingTake(const std::string& name)    {
        _pendingTakes.erase(name);
        setBodyState(name, BodyState::EMPTY, utils::Pos{}, "settled");
    }
    void CodeGen::drainPendingTakes()    {
        // 决议 §3.1：未落定的回填 F；已落定的已在 settlePendingTake 里转 E。
        for (const auto& name : _pendingTakes)
            setBodyState(name, BodyState::FULL, utils::Pos{}, "restored");
        _pendingTakes.clear();
    }
    bool CodeGen::isVarUnavailable(const std::string& name) const    {
        for (auto it = _moveStates.rbegin(); it != _moveStates.rend(); ++it)        {
            if (const auto found = it->find(name); found != it->end())            {
                // 决议 §3.4：HELD（挂起）期间允许通过引用读/直接读写字段，
                // 内容位置没动。EMPTY（空壳）/MOVED 不可直接读。
                if (found->second == BodyState::EMPTY || found->second == BodyState::MOVED)
                    return true;
                // UNKNOWN（?）：只有「因控制流合流而不确定」的才拒绝；
                // Take 形参入口那种不透明 `?`（§9.2）仍然允许直接访问。
                if (found->second == BodyState::UNKNOWN && isMergedUnknown(name))
                    return true;
                return false;
            }
        }
        return isVarMoved(name); // 回退 drop 表（str/list 等 moved）
    }
    std::string CodeGen::bodyStateName(const std::string& name) const    {
        for (auto it = _moveStates.rbegin(); it != _moveStates.rend(); ++it)        {
            if (const auto found = it->find(name); found != it->end())            {
                switch (found->second)                            {
                case BodyState::FULL:  return "Full";
                case BodyState::EMPTY: return "Empty";
                case BodyState::HELD:  return "Held";
                case BodyState::MOVED: return "Moved";
                case BodyState::UNKNOWN: return "?";
                }
            }
        }
        return "Full";
    }
    bool CodeGen::checkBorrowNotEscaping(const std::string& name, const utils::Pos& pos,
                                         const std::string& action) const    {
        if (!isBorrowed(name)) return true;
        const_cast<CodeGen*>(this)->failAt(
            pos, "'" + name + "' is a borrowed value and cannot be " + action +
                     " (P13-1 / D9 rule C: only an owner can be moved)");
        return false;
    }
    bool CodeGen::isDropOwned(const std::string& name) const    {
        // 只有「在登记表里 **且不是借用**」才算拥有者。早先这里漏了 borrowed
        // 判断，于是 `var ys = xs` 之后的 `ys = xs` 会先按持有者去 drop ys
        // 的旧值 —— 而 ys 只是 xs 的壳子别名，两个槽指向同一块内容，
        // 释放它等于提前释放 xs 的缓冲区，作用域末 xs 再释放一次就崩。
        for (auto it = _scopeDrops.rbegin(); it != _scopeDrops.rend(); ++it)        {
            for (const auto& e : *it)            {
                if (e.name != name) continue;
                return !e.borrowed;
            }
        }
        return false;
    }
    void CodeGen::setDropAlias(const std::string& name, const bool borrowed)    {
        // 从内向外找第一个同名项：内层遮蔽外层，改的是内层那个。
        for (auto it = _scopeDrops.rbegin(); it != _scopeDrops.rend(); ++it)        {
            for (auto e = it->rbegin(); e != it->rend(); ++e)            {
                if (e->name != name) continue;
                e->borrowed = borrowed;
                // **两个字段必须一起改**：emitScopeDrops 跳过登记项的依据是
                // moved，而不变量本身不看 borrowed。只把 borrowed 置真会留下
                // moved==false，作用域末照样销毁一次 => 与别名源重复释放。
                // 借用项在 registerBorrow 里就是 moved==true，语义一致。
                e->moved = borrowed;
                return;
            }
        }
    }
    bool CodeGen::markVarMoved(const std::string& name, const utils::Pos& pos,
                               const std::string& action)    {
        // P13-2/L3：先更新独立的移动状态表（Body/ClassType 等）。
        for (size_t s = _moveStates.size(); s > 0; --s)        {
            if (const auto found = _moveStates[s - 1].find(name);
                found != _moveStates[s - 1].end())            {
                for (const auto& frame : _ehStack)                {
                    if (!frame.inTryBody) continue;
                    if (s - 1 >= frame.tryBodyScope) continue;
                    failAt(pos, "'" + name + "' is declared outside the current 'try' body "
                                "and cannot be " + action + " inside it");
                    return false;
                }
                found->second = BodyState::MOVED;
                return true;
            }
        }
        // 再更新 P13-1 的 drop 表（原有逻辑）。
        // 从内向外找第一个同名项：内层遮蔽外层，被移动的是内层那个。
        for (size_t s = _scopeDrops.size(); s > 0; --s)        {
            auto& entries = _scopeDrops[s - 1];
            for (auto e = entries.rbegin(); e != entries.rend(); ++e)            {
                if (e->name != name) continue;
                // P13-1（D13）：移动外层拥有者时，若当前正处在 try 体内，
                // 抛出会让 longjmp 直接跳过该拥有者的作用域出口 —— 它既不被
                // 销毁（泄漏）又已经被移走（接收方也拿不到），两种情况都比
                // 「拒绝编译」糟。P13-1 没有跨异常边界的状态格，只能保守禁止。
                // 判据：登记下标 < 该 try 体入口的作用域深度 = 比 try 活得久。
                for (const auto& frame : _ehStack)                {
                    if (!frame.inTryBody) continue;
                    if (s - 1 >= frame.tryBodyScope) continue;
                    failAt( pos,
                            "'" + name + "' is an owner declared outside the current 'try' body "
                                          "and cannot be " + action + " inside it (P13-1 / D13: an "
                                          "exception would skip its destruction, turning the move into "
                                          "a leak or a double free)");
                    return false;
                }
                e->moved = true;
                return true;
            }
        }
        return true;
    }
    void CodeGen::emitScopeDrops(const size_t scopeIndex, const bool nullOut)    {
        if (scopeIndex >= _scopeDrops.size()) return;
        const auto& entries = _scopeDrops[scopeIndex];
        // D4：声明的**逆序**销毁
        for (auto it = entries.rbegin(); it != entries.rend(); ++it)        {
            if (it->moved) continue;
            const std::string dropFn = ensureDropGlue(it->type);
            if (dropFn.empty()) continue;
            // 空地址意味着登记时没给出槽位（借用升格成拥有者却没记地址）。
            // 那会生成 `leaq , %rcx` 这种非法汇编，报错文本还是 Assembler
            // failed，完全看不出根因。在生成期就拦下来。
            if (it->addr.empty())            {
                fail("internal: no slot address registered for '" + it->name +
                     "' (it was promoted from borrowed to owned without an address)");
                return;
            }
            _asm.lea(it->addr, "%rcx");
            _asm.subRsp(32);
            _asm.call(dropFn);
            _asm.addRsp(32);
            (void)nullOut;   // drop 函数自己会把槽写成 null（D17）
        }
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
        //
        // P13 缺陷修复（2026-11-17 实测）：形参类型必须与
        // emitFunctionDefinition **走同一条解析路径**。原先这里只拿标注文本
        // 调 resolveTypeName，而它收的是裸名字 —— `getFullLabel()` 把 `[...]`
        // 整个丢掉，`list[str]` 到这里只剩 "list"，于是
        // `fun f(x: list[str])` 与 `fun f(x: list[int])` 拼出同一个符号
        // `_R1fLi`，直接报 "Duplicate definition"。
        // collectParamTypesT 是发射函数体时用的那一套（走 LabelNode /
        // IndexExpressionNode，方括号里的类型实参拿得全），复用它才能保证
        // 「预登记的签名」与「发射时算出的符号」是同一个。
        sig.annotationTexts = annotations;
        {
            const auto savedGenerics = _currentGenericParams;
            _currentGenericParams = sig.genericParams;
            std::vector<sema::TypePtr> resolved;
            if (const auto callInfix = std::dynamic_pointer_cast<ast::InfixExpressionNode>(node->getCallNode()))            {
                if (const auto paren = std::dynamic_pointer_cast<ast::ParenRangerNode>(callInfix->getRightNode()))                {
                    collectParamTypesT(paren->getRangerNode(), resolved);
                }
  else                {
                    collectParamTypesT(callInfix->getRightNode(), resolved);
                }
            }
  else if (const auto call = std::dynamic_pointer_cast<ast::FunctionCallNode>(node->getCallNode()))            {
                const auto paramsNode = std::dynamic_pointer_cast<ast::ExpressionNode>(call->getRightNode());
                if (const auto paren = std::dynamic_pointer_cast<ast::ParenRangerNode>(paramsNode))                {
                    collectParamTypesT(paren->getRangerNode(), resolved);
                }
  else if (paramsNode)                {
                    collectParamTypesT(paramsNode, resolved);
                }
            }
  else            {
                collectParamTypesT(node->getCallNode(), resolved);
            }
            _currentGenericParams = savedGenerics;
            sig.paramTypes = std::move(resolved);
        }
        // 位置对齐：三张表（名字 / 标注 / 类型）长度必须一致。
        while (sig.paramTypes.size() < sig.paramNames.size())        {
            sig.paramTypes.push_back(sema::getUnknownType());
        }
        if (sig.paramTypes.size() > sig.paramNames.size())        {
            sig.paramTypes.resize(sig.paramNames.size());
        }
        for (size_t i = 0; i < sig.paramNames.size(); ++i)        {
            const std::string text = (i < annotations.size()) ? annotations[i] : std::string();
            sig.annotated.push_back(!text.empty());
        }
        // 返回类型：与 emitFunctionDefinition 同一套顺序
        // （声明标注 -> 类名再解析一次 -> 从函数体推断）。
        sig.retType = returnTypeFromNode(node);
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
            // P13-2/L3-F（决议 §5.3）：泛型定义处不写实例级 writes[p.field]
            // （T 的字段未知，只能用 writes[p] 表示整个参数的上界）。
            if (!sig.genericParams.empty())            {
                for (const auto& clause : fn->getSignatureClauses())                {
                    if (!clause || clause->getFullLabel() != "writes") continue;
                    const auto des = clause->getLabelDesNodes();
                    if (des.empty() || !des[0] || !des[0]->getBodyNode()) continue;
                    std::vector<std::shared_ptr<ast::ExpressionNode>> items;
                    flattenTypeArgs(des[0]->getBodyNode(), items);
                    for (const auto& item : items)                    {
                        if (const auto inf = std::dynamic_pointer_cast<ast::InfixExpressionNode>(item);
                            inf && inf->getOpToken().getType() == core::TokenType::TOKEN_DOT)                        {
                            failAt(clause->getMainToken().getPos(),
                                   "L3: generic definition must not use instance-level writes[p.field] "
                                   "(P13-2 / §5.3; use writes[p])");
                            return;
                        }
                    }
                }
            }
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
        // P13-2/L3-F（决议 §4.2）：**非重载**函数的 writes / from 也要登记。
        // 必须在这里（发射任何函数体之前）做完，否则前向调用拿不到效果。
        // 重载的在下面按 mangling 符号登记，这里跳过它们。
        for (const auto& sig : all)        {
            const auto countIt = defCount.find(sig.name);
            if (countIt != defCount.end() && countIt->second >= 2) continue;
            const std::string emitName = (sig.name == "main") ? "__rio_user_main" : sig.name;
            registerSignatureEffects(emitName, sig.def, sig.paramNames);
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
            // P13-2/L3-F（决议 §4.2）：登记这份重载的 writes / from 效果。
            // 键必须是**发射符号**（已 mangling），调用点 resolveOverload
            // 拿到的就是它。
            registerSignatureEffects(entry.symbol, sig.def, sig.paramNames);
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
        // ---------- 泛型体内的实参：形参在单态化前是占位符 ----------
        //
        // 在 `fun f[T](xs: list[T])` 体内调用重载集时，`xs[i]` 的静态类型是
        // GenericType("T")。它**尚未**知道具体是中国，任何具体形参都可能
        // （也可能不）合适 —— 不能像之前那样落到「指针族兜底 30 分」，
        // 那会让 `toStr(int)` / `toStr(str)` / `toStr(char)` 全部平分，
        // 决议随机取胜者（实测 `dumpToStr(["a"])` 把 str 当 int 打印）。
        //
        // 正确做法：给**所有**候选同一个中性分（比通配符高、比任何精确匹配低），
        // 这样同分 → 报歧义而不是静默选错？不 —— 那会让整个泛型库没法用。
        //
        // 现实可用的口径：单态化后类型才确定，所以这里必须让决议**推迟**。
        // 但当前架构是单遍 codegen，没有延迟决议的能力。折中：按「形参是标量
        // 还是指针族」给出中性分，同时把「形参是 str」排在同分候选的**后面**
        // 由调用方实例化的实际类型决定 —— 见 genericParamOverloadHint。
        if (actual->getKind() == sema::TypeKind::TK_GENERIC)        {
            // 占位符实参：所有候选都得同一个分，避免按「指针族 30」随意选。
            // 让步规则：形参不是容器/指针族的（纯标量）略高，因为泛型体里
            // 标量是最常见的实参形态；指针族（str/list/dict）靠后。
            // 真正精确的决议发生在**单态化之后**的第二次发射里
            // （_currentGenericBindings 已就位，inferType 会返回具体类型）。
            return formal->isPointerLike() ? 10 : 20;
        }
        if (formal->equals(actual)) return 100;
        // ---------- 容器之间的亲和度（P13 缺陷修复，2026-11-17） ----------
        //
        // 只看「都是 8 字节指针」会让 str / list / dict 彼此顶替：
        //   fun test(arg: str) / test(arg: list) / test(arg: dict)
        //   test([1,2]) 三者各得 30 分 → 直接报 Ambiguous（用户实测的第一个症状）。
        // 但这份代码**根本不该**接受 str 或 dict 实参，它们之间没有转换语义。
        // 所以：容器族之间一律 -1（不合格），只有同族才继续比元素类型。
        //
        // 同族内部按「元素类型写得多具体」分档 —— 越具体的写法越贴合：
        //   list[int] 接 list[int]  -> 100（上面的 equals 已命中）
        //   list[int] 接 list       -> 60（裸形参接具体实参，能用但不够贴）
        //   list      接 list[int]  -> 70（裸形参什么 list 都收）
        //   list[any] 接任何 list   -> 70（any 也是「不约束」，与裸形参同级）
        // 裸容器的元素是 nullptr、显式 any 是 UNKNOWN，两者都当「不约束」。
        const auto isContainer = [](const sema::TypePtr& t) {
            const auto k = t->getKind();
            return k == sema::TypeKind::TK_STR || k == sema::TypeKind::TK_LIST ||
                   k == sema::TypeKind::TK_DICT;
        };
        // 只有**两侧都是容器**时才进这个分支：容器之间互不兼容。
        // 容器与 class / ptr / func 之间仍按 P11 §3.3 的「指针族互通 30」
        // 处理（那条规则原样保留，下面照旧 return 30）。
        if (isContainer(formal) && isContainer(actual))        {
            if (formal->getKind() != actual->getKind()) return -1;
            // str 没有类型实参，走到这里说明两者都是 str 但内容不等 —— 不可能，
            // 保守按同族处理。
            if (formal->getKind() == sema::TypeKind::TK_STR) return 90;
            if (formal->getKind() == sema::TypeKind::TK_LIST)            {
                const auto feat = std::static_pointer_cast<sema::ListType>(formal)->getElemType();
                const auto aeat = std::static_pointer_cast<sema::ListType>(actual)->getElemType();
                const bool formalGeneric = !feat || feat->getKind() == sema::TypeKind::TK_UNKNOWN;
                const bool actualGeneric = !aeat || aeat->getKind() == sema::TypeKind::TK_UNKNOWN;
                // 形参不约束：什么 list 都收，但不如精确匹配贴。
                if (formalGeneric) return 70;
                // 形参具体、实参不约束：实参这边没给出信息，按「能装下」处理。
                if (actualGeneric) return 60;
                // 两侧都具体但元素不等：不做元素间的数值提升，
                // 保守判不合格 —— 需要转换请显式写一个更宽的形参。
                return -1;
            }
            if (formal->getKind() == sema::TypeKind::TK_DICT)            {
                const auto fd = std::static_pointer_cast<sema::DictType>(formal);
                const auto ad = std::static_pointer_cast<sema::DictType>(actual);
                const auto generic = [](const sema::TypePtr& t) {
                    return !t || t->getKind() == sema::TypeKind::TK_UNKNOWN;
                };
                const bool fkGeneric = generic(fd->getKeyType());
                const bool fvGeneric = generic(fd->getValueType());
                const bool akGeneric = generic(ad->getKeyType());
                const bool avGeneric = generic(ad->getValueType());
                // 形参两侧都不约束（裸 `dict` / `dict[any, any]`）：什么都收。
                if (fkGeneric && fvGeneric) return 70;
                // 实参两侧都不约束：实参没给出信息，按「能装下」处理。
                if (akGeneric && avGeneric) return 60;
                // 逐侧比较：形参那一侧不约束就跳过，约束了就必须精确相等。
                // 键尤其不能猜 —— 键参与查找语义（str 键按内容比、int 键按值比）。
                if (!fkGeneric)                {
                    if (akGeneric) return 60;
                    if (!fd->getKeyType()->equals(ad->getKeyType())) return -1;
                }
                if (!fvGeneric)                {
                    if (avGeneric) return 60;
                    if (!fd->getValueType()->equals(ad->getValueType())) return -1;
                }
                // 形参比实参更宽松（至少有一侧不约束，且没有不匹配）：
                // 比精确匹配低一档，但确实能用。
                return 90;
            }
        }
        if (!sameValueFamily(formal, actual)) return -1;
        // 数值族内部互通，按亲和度梯度打分。
        //
        // 原实现是「int/float 用 90/50，其余落到指针族兜底 30」，而 isNumeric()
        // 只认 int/float，于是 bool/char 实参面对 int/char 两个候选时**各得 30
        // 分判平局** → `charToInt(true)` 报 Ambiguous。数值族需要按「拓宽无损 /
        // 收窄有损 / 兄弟无语义依据」区分，不能让它们同分。
        //
        // 分档依据（用户 2026-11-16 批准）：
        //   精确             100  （上面已返回）
        //   同族拓宽/无损     90   bool/char → int、int → float
        //   收窄（有损）      40   int → bool/char、float → int
        //   兄弟（无语义依据） 20   bool ↔ char
        // 非数值族之间仍是 30（指针族互通），刻意低于数值族的任何一档：
        // 数值类型之间至少有可定义的转换语义，指针族互通只是「都是 8 字节」。
        const auto numericKind = [](const sema::TypePtr& t) {
            const auto k = t->getKind();
            return k == sema::TypeKind::TK_INT || k == sema::TypeKind::TK_FLOAT ||
                   k == sema::TypeKind::TK_CHAR || k == sema::TypeKind::TK_BOOL;
        };
        if (numericKind(formal) && numericKind(actual))        {
            const auto f = formal->getKind();
            const auto a = actual->getKind();
            // 同族（int→int 已在 equals 命中，这里覆盖 float→float 等）
            if (f == a) return 90;
            // 拓宽到 int：bool/char 底层的值域完全落在 int 内，无损。
            if (f == sema::TypeKind::TK_INT &&
                (a == sema::TypeKind::TK_BOOL || a == sema::TypeKind::TK_CHAR)) return 90;
            // int → float：数值转换，保持 P11 既有口径。
            if (f == sema::TypeKind::TK_FLOAT && a == sema::TypeKind::TK_INT) return 50;
            // float → int：截断丢小数，有损。
            if (f == sema::TypeKind::TK_INT && a == sema::TypeKind::TK_FLOAT) return 40;
            // int → bool/char：收窄，值域可能装不下（65 不是合法 char 语义上的
            // 字符、2 不是合法 bool）。
            if ((f == sema::TypeKind::TK_BOOL || f == sema::TypeKind::TK_CHAR) &&
                a == sema::TypeKind::TK_INT) return 40;
            // float → bool/char：既收窄又跨族。
            if ((f == sema::TypeKind::TK_BOOL || f == sema::TypeKind::TK_CHAR) &&
                a == sema::TypeKind::TK_FLOAT) return 20;
            // bool ↔ char：两者互不相干，没有任何语义依据可依。
            if ((f == sema::TypeKind::TK_BOOL && a == sema::TypeKind::TK_CHAR) ||
                (f == sema::TypeKind::TK_CHAR && a == sema::TypeKind::TK_BOOL)) return 20;
            // 兜底：数值族内未列举的组合按收窄处理。
            return 40;
        }
        // 指针族之间（str/list/dict/class/ptr/func）只是「一个 8 字节指针」，
        // 允许互通但优先级很低，避免抢走精确匹配。
        //
        // 注意（P13）：容器与容器之间**不走**这条兜底 —— 上面已单独处理
        // （str/list/dict 语义互不相同，按「指针」互换会选中错误的函数体）。
        // 这条留给「容器 vs class/ptr/func」以及其余指针族组合。
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
    bool CodeGen::applyWriteEffects(const std::string& emitName, const std::string& displayName,
                                    const std::vector<std::shared_ptr<ast::ExpressionNode>>& args,
                                    const utils::Pos& pos)    {
        // P13-2/L3-F（决议 §4.2）：调用点按被调函数的 writes 效果更新活跃集合。
        // 修前这里从不读 writes，于是「&p.name 仍活跃时调用 writes[p.name] 的
        // 函数」静默通过 —— 决议明文要求的效果系统整块缺失。
        // 保守口径（决议 §9.3/§9.4）：没写 writes 的用户函数按函数体推断记录；
        // 这里做不到跨函数推断，退化为「不检查」而不是乱报。
        const auto effIt = _writeEffects.find(emitName);
        if (effIt == _writeEffects.end() || effIt->second.empty()) return true;
        const auto namesIt = _functionParamNames.find(displayName);
        const std::vector<std::string> emptyNames;
        const auto& pnames = (namesIt != _functionParamNames.end()) ? namesIt->second : emptyNames;
        for (const auto& eff : effIt->second)        {
            const std::string path = resolveEffectPath(eff.root, eff.suffix, pnames, args);
            if (path.empty()) continue; // 实参不是可追踪路径：保守跳过
            if (!checkWriteConflicts(path, pos, "call writing")) return false;
        }
        return true;
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
        const size_t callArea = emitArgsToRegisters(flat, displayName, paramTypes, 0, &sig.annotated);
        if (hasFailed()) return;
        if (!applyWriteEffects(symbol, displayName, flat, utils::getUnknownPos())) return;
        _asm.call(symbol);
        _asm.addRsp(callArea);
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
        _scopeDrops.emplace_back();
        _moveStates.emplace_back();
    }
    void CodeGen::popScope()    {
        // P13-1（D4）：作用域末发 drop 调用，顺序为声明逆序。
        // 注意 popScope 只在「词法出口」被调用；ret / break / continue 另有
        // 各自的处理（见 emitReturn / emitBreak / emitContinue）。
        if (!_scopes.empty())        {
            emitScopeDrops(_scopeDrops.size() - 1, true);
            _scopes.pop_back();
        }
        if (!_scopeDrops.empty()) _scopeDrops.pop_back();
        if (!_moveStates.empty()) _moveStates.pop_back();
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
        // P13-2/L3（决议 §7.2/§4.1）：`&bob` -> Ref[inner]；
        // `&p.name` -> Ref[字段类型]（p 可能是 Ref 变量，需穿透查类）。
        if (const auto refExpr = std::dynamic_pointer_cast<ast::RefExpressionNode>(node))        {
            sema::TypePtr inner;
            if (const auto dot = std::dynamic_pointer_cast<ast::InfixExpressionNode>(refExpr->getOperand());
                dot && dot->getOpToken().getType() == core::TokenType::TOKEN_DOT)            {
                const auto field = std::dynamic_pointer_cast<ast::IdentifierNode>(dot->getRightNode());
                const auto root = std::dynamic_pointer_cast<ast::IdentifierNode>(dot->getLeftNode());
                const std::string cls = root ? classNameFromVariable(root->getName())
                                             : classNameOf(dot->getLeftNode());
                if (field && !cls.empty()) inner = findFieldType(cls, field->getName());
            }
            if (!inner) inner = inferType(refExpr->getOperand());
            return std::make_shared<sema::MemoryType>(sema::MemoryRole::REF, inner);
        }
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
            // 否则传给 `f: func[A][B]` 形参时会被可赋值性校验挡下来。
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
                    // P13-2/L3-B（决议 §1.4）：`pool.new[T](args)` 的形态是
                    // Infix(DOT, pool, IndexExpression(new, [T]))。
                    // 返回类型就是 T（构造器选择，必须与构造返回类型一致）。
                    if (const auto idx = std::dynamic_pointer_cast<ast::IndexExpressionNode>(member->getRightNode()))                {
                        if (const auto mid = std::dynamic_pointer_cast<ast::IdentifierNode>(idx->getLeftNode());
                            mid && mid->getName() == "new")                        {
                            if (const auto bracket = std::dynamic_pointer_cast<ast::BracketExpressionNode>(idx->getIndexNode()))                            {
                                std::vector<std::shared_ptr<ast::ExpressionNode>> argNodes;
                                flattenTypeArgs(bracket->getBodyNode(), argNodes);
                                if (!argNodes.empty()) return typeFromTypeArgNode(argNodes[0], _currentGenericParams);
                            }
                        }
                    }
                    const auto mid = std::dynamic_pointer_cast<ast::IdentifierNode>(member->getRightNode());
                    // P8：`mod.fn(...)` / `mod.Cls(...)` —— 模块在加载期内联，
                    // 类型按普通全局函数 / 类名解析，不能落到「未知函数按 int」。
                    if (mid)
                    {
                        // P13-2/L3-E（决议 §4.3）：list.get_mut(i) → Mut[元素类型]
                        if (mid->getName() == "get_mut")                        {
                            if (const auto lv = std::dynamic_pointer_cast<ast::IdentifierNode>(member->getLeftNode()))                            {
                                if (const auto s = const_cast<CodeGen*>(this)->lookupVar(lv->getName());
                                    s && s->type && s->type->getKind() == sema::TypeKind::TK_LIST)                                {
                                    const auto elem = std::static_pointer_cast<sema::ListType>(s->type)->getElemType();
                                    return std::make_shared<sema::MemoryType>(
                                        sema::MemoryRole::MUT, elem ? elem : sema::getIntType());
                                }
                            }
                        }
                        // P13-2/L3-A：Pool 静态方法返回 Pool 类型
                        if (const auto ownerId = std::dynamic_pointer_cast<ast::IdentifierNode>(member->getLeftNode());
                            ownerId && ownerId->getName() == "Pool" &&
                            (mid->getName() == "fixed" || mid->getName() == "boxed" ||
                             mid->getName() == "chunked"))                        {
                            return sema::getPoolType();
                        }
                        if (const auto modId =
                                std::dynamic_pointer_cast<ast::IdentifierNode>(member->getLeftNode());
                            modId && isModuleName(modId->getName()))
                        {
                            if (findClass(mid->getName()))
                                return std::make_shared<sema::ClassType>(mid->getName());
                            // P12/G14：`mod.gen(...)` 的返回类型必须按泛型决议算，
                            // 否则 `sout(math.max(2.5, 1.5))` 会按 int 解释 double
                            // 的位模式（打印出 4612811918334230528）。
                            if (const auto t = inferGenericCallType(mid->getName(), member, node))                                return t;
                            if (const auto found = _functionReturns.find(mid->getName());
                                found != _functionReturns.end())
                                return found->second;
                            return sema::getIntType();
                        }
                    }
                    const std::string owner = classNameOf(member->getLeftNode());
                    if (mid && !owner.empty())                    {
                        // P13-2/L3（决议 §7.1）：Body 内建 ref()/mut() ——
                        // p.ref() ≡ &p → Ref[Owner]，p.mut() → Mut[Owner]。
                        if (mid->getName() == "ref" || mid->getName() == "mut" ||
                            mid->getName() == "take")                        {
                            const auto role = mid->getName() == "ref"
                                ? sema::MemoryRole::REF :
                                mid->getName() == "mut" ? sema::MemoryRole::MUT
                                                        : sema::MemoryRole::CONTENT;
                            return std::make_shared<sema::MemoryType>(
                                role, std::make_shared<sema::ClassType>(owner));
                        }
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
                // core 种子层：extern fun 不进 _functionReturns，返回值类型
                // 只能由这张静态表给（否则 clone 出来的 str/list 会被当成 int）。
                if (const auto coreRet = coreReturnType(id->getName())) return coreRet;
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
        // P13-2/L3-N（决议 §12.1）：裸块出现在右值位置时是「块表达式」，
        // 它的类型由出口值决定 —— 优先取块内 `ret value` 的类型（显式出口），
        // 没有就取末表达式的类型。不这样处理会一路落到下面的 int 兜底，
        // `var s: str = { ret "x" }` 的值槽就会被开成 int。
        if (const auto block = std::dynamic_pointer_cast<ast::BlockRangerNode>(node))        {
            for (const auto& e : block->getBodyExpressions())            {
                const auto stmt = std::dynamic_pointer_cast<ast::ExpressionStatementNode>(e);
                const auto ret = std::dynamic_pointer_cast<ast::ReturnExpressionNode>(
                    stmt ? stmt->getExpression() : e);
                if (ret && !ret->exitsFunction() && ret->getReturnNode())                {
                    const auto t = inferType(ret->getReturnNode());
                    if (t && t->getKind() != sema::TypeKind::TK_UNKNOWN) return t;
                }
            }
            return inferBlockValueType(node);
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
            // P13-1：drop glue 函数体。它们是在作用域出口 / ret 处
            // 按类型发现并排队的，这里统一发射（同样的理由：不能就地发）。
            // 注意 emitDropGlueBody 里可能再发现新类型（嵌套容器），
            // 所以循环到队空。
            while (!_pendingDropGlue.empty())            {
                const auto [fn, ty] = _pendingDropGlue.back();
                _pendingDropGlue.pop_back();
                emitDropGlueBody(fn, ty);
            }
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
        // P13-2/L0：类声明层面的"去伪"校验（重名 / 保留名），必须在
        // registerClass 之前做，否则重复类会被静默覆盖、类名冲突静默生效。
        if (validateL0ClassDeclarations(classNodes)) return;
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
        if (hasFailed()) return;
        // P13-2/L0-2 + L0-4：类字段类型 + 多类型标签冲突校验
        // （此时类表已齐，前向引用不成问题）。
        if (validateL0FieldTypes(classNodes)) return;
        if (validateL0LabelConflicts(program)) return;
        if (validateL3TakeUsage(program)) return;
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
        // 生命周期标签 `static` 的字段：为每个分配一块 .bss 全局槽。
        // 它们不属于实例布局，因此必须在 computeLayout 之外单独处理。
        emitStaticFieldSlots();
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
        // static 字段的初始化器要在**任何用户代码之前**执行一次。
        emitStaticFieldInitCode();
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
        // P13-1（D10）：临时值的销毁点是**语句末**。这里记下进入本语句时的
        // 累积长度做标记，语句发射完把标记之后新增的临时值逆序销毁。
        //
        // 为什么放在这一层而不是各分支内部：语句的种类太多（调用、赋值、
        // 表达式语句……），每一种都可能有临时值；统一在唯一的出口收尾，
        // 才不会有哪个分支漏掉。嵌套语句进入时会再取一次标记，天然正确。
        const size_t mark = _stmtTemps.size();
        // P13-2/L3-D（决议 §3.3）：独立 take 语句（既不落定也不使用）
        // 编译警告、不阻断。走 stderr 诊断格式，不新增机制。
        if (const auto call = std::dynamic_pointer_cast<ast::FunctionCallNode>(node))            {
            if (takeReceiverName(node) != "")                {
                const auto pos = node->getPos();
                fprintf(stderr, "%swarning: 'take()' as a standalone statement has no effect "
                                "(result is discarded)\n", posPrefix(pos).c_str());
            }
        }
        emitStatementInner(node);
        drainStmtTemps(mark);
        // P13-2/L3-D（决议 §3.1）：语句末结算 pending_take ——
        // 未落定的回填 F，已落定的已在落点转 E。
        drainPendingTakes();
        // P13-2/L3-E（决议 §4.2）：语句末 NLL —— 移除最后使用已过的子引用。
        drainActiveRefs(node);
    }
    void CodeGen::drainStmtTemps(const size_t mark)    {
        if (mark >= _stmtTemps.size()) return;
        // 逆序销毁（与作用域末一致），然后截断。
        for (size_t i = _stmtTemps.size(); i > mark; --i)        {
            const auto& tmp = _stmtTemps[i - 1];
            const std::string dropFn = ensureDropGlue(tmp.type);
            if (dropFn.empty()) continue;
            // 声明语句那条路径可能已经把槽位自己释放并清空了，这里读到的就是
            // null —— 而 drop glue 对 null 是静默返回的（D5），重复调用无害。
            _asm.lea(tmp.addr, "%rcx");
            _asm.subRsp(32);
            _asm.call(dropFn);
            _asm.addRsp(32);
        }
        _stmtTemps.resize(mark);
    }
    bool CodeGen::isFreshValue(const std::shared_ptr<ast::ExpressionNode>& node) const    {
        if (!node) return false;
        // 已有变量 / 元素 / 字段 = 别人已经管着它的生命周期 -> 不是新值。
        if (std::dynamic_pointer_cast<ast::IdentifierNode>(node)) return false;
        if (std::dynamic_pointer_cast<ast::IndexExpressionNode>(node)) return false;
        if (const auto inf = std::dynamic_pointer_cast<ast::InfixExpressionNode>(node))        {
            if (inf->getOpToken().getType() == core::TokenType::TOKEN_DOT) return false;
        }
        return needsDrop(inferType(node));
    }
    void CodeGen::emitStatementInner(const std::shared_ptr<ast::ExpressionNode>& node)    {
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
            emitBreak(node);
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
        // P13-2/L3（决议 §7.2/§4.1）：`&bob` 壳子引用透传对象指针；
        // `&p.name` 子引用要算出**字段槽地址**（对象指针 + 字段偏移），
        // 不是读字段的值。
        if (const auto refExpr = std::dynamic_pointer_cast<ast::RefExpressionNode>(node))        {
            const auto operand = refExpr->getOperand();
            if (const auto dot = std::dynamic_pointer_cast<ast::InfixExpressionNode>(operand);
                dot && dot->getOpToken().getType() == core::TokenType::TOKEN_DOT)            {
                const auto field = std::dynamic_pointer_cast<ast::IdentifierNode>(dot->getRightNode());
                const auto dotRoot = std::dynamic_pointer_cast<ast::IdentifierNode>(dot->getLeftNode());
                // 决议 §4.1：根节点可能是 Ref/Mut 引用变量（穿透），
                // 此时 classNameOf 拿不到类名，用 classNameFromVariable 取内嵌类。
                const std::string owner = dotRoot
                    ? classNameFromVariable(dotRoot->getName()) : classNameOf(dot->getLeftNode());
                if (!field || owner.empty())                {
                    failAt(node->getPos(), "L3: cannot resolve field path for sub-reference");
                    return;
                }
                const long long offset = findFieldOffset(owner, field->getName());
                // 决议 §4.1：ownerName 是根变量名（p 或引用变量 q），
                // path 完整路径；穿透展开在 registerActiveRef 内部处理。
                const std::string ownerName = dotRoot ? dotRoot->getName() : owner;
                // 决议 §4.1：path 用完整路径 "p.name"（穿透展开在
                // registerActiveRef 内部按 refVar 替换前缀）。
                registerActiveRef(ownerName + "." + field->getName(), ownerName,
                                  "", node->getPos()); // refVar 由 var 绑定处补全
                emitExpression(dot->getLeftNode()); // 对象指针 -> %rax
                if (hasFailed()) return;
                if (offset != 0) _asm.inst("addq", std::to_string(offset) + ", %rax");
                return;
            }
            emitExpression(operand);
            return;
        }
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
        // 走到 emitExpression 说明这个块在**值位置**（右值 / 实参 / 返回表达式），
        // 按块表达式发射：它自己就是可返回值作用域（决议 §12.1 / §13.1）。
        // 语句位置的块由 emitStatement 单独拦下（asValue=false），不受影响。
        if (std::dynamic_pointer_cast<ast::BlockRangerNode>(node))        {
            emitBlock(node, true);
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
            emitBreak(node);
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
        // P13-1（D9 H/X 两档）：已移动的值不能再用。
        //
        // H = 槽里有值，X = 已被整体移走（ret 出去 / 塞进容器）。
        // 这条检查是必须的：`ret xs` 已经把 xs 交给调用方，之后再用
        // `xs[0]` 会读到一块随时被别人释放的内存。实测这个用例之前
        // 能编译通过并「碰巧」打印出正确值，正因为 P13-1 之前根本没释放。
        // P13-2/L3-C：EMPTY（空壳）/HELD（挂起）/MOVED 都不能直接读值。
        // E 状态的壳要先用 fill 装回内容（决议 §6.1/§10.1）。
        if (isVarUnavailable(id->getName()))        {
            const auto st = bodyStateName(id->getName());
            if (st == "?")            {
                // 决议 §3.4：? 状态无法判定内容是否还在，直接读值不安全，
                // 提示改用 get()（会走运行时判空/兜底）。
                failAt(node->getPos(), "'" + id->getName() +
                       "' is in ? state (control flow merged to an unknown body state) and "
                       "cannot be used directly (P13-2: use get() instead)");
                return;
            }
            failAt(node->getPos(), "'" + id->getName() +
                   "' is in " + st + " state and cannot be used directly "
                   "(P13-2: fill it first if Empty)");
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
        // `str < str` / `<=` / `>` / `>=` 必须按**字典序**比较。
        // 没有这个分支时它们落到下面的 cmpq，比的是两个指针 —— 结果取决于
        // 字面量在堆上的相对地址。泛型单态化成 str 后尤其明显：
        // `lt("bb","aa")` 返回 true。
        if ((op == core::TokenType::TOKEN_LESS || op == core::TokenType::TOKEN_LESS_EQUAL ||
             op == core::TokenType::TOKEN_GREATER || op == core::TokenType::TOKEN_GREATER_EQUAL) &&
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
            emitStrOrder(lhsSlot, rhsSlot, static_cast<int>(op));
            return;
        }
        // 左值求值 -> 暂存槽（不能用 push：右值求值里可能有调用，
        // push 造成的 8 字节偏移会让那个调用的 %rsp 失去 16 字节对齐）
        emitExpression(infix->getLeftNode());
        const std::string lhsSlot = allocateTemp(sema::getIntType());
        _asm.mov("%rax", lhsSlot);
        // 右值求值 -> %rax
        emitExpression(infix->getRightNode());
        // 恢复左值到 %rcx
        _asm.mov(lhsSlot, "%rcx");
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
        //
        // **左值必须存进栈槽，不能留在 %xmm2**（P13 缺陷修复，2026-11-17）：
        // %xmm2 是 caller-saved，右值一旦是**函数调用**就会被被调方踩掉，
        // 于是 `fi(1) + fi(2)` 算成 4.0（左值被右值覆盖，等于 2+2）。
        // 变量相加 `a + b` 没有调用，所以一直是好的 —— 这类缺陷只在
        // 「两侧都是调用」时现形，很容易漏。
        emitOperand(infix->getLeftNode(), lt);
        const std::string lhsFloatSlot = allocateTemp(sema::getFloatType());
        _asm.inst("movsd", "%xmm0", lhsFloatSlot);
        emitOperand(infix->getRightNode(), rt);
        _asm.inst("movsd", "%xmm0", "%xmm1");
        // 右值 -> xmm1
        _asm.inst("movsd", lhsFloatSlot, "%xmm0");
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
            // P13-1（D9 规则 C）：把**拥有者**放进容器即**移动** —— 源变量
            // 变成 X，作用域末不能再释放它（否则容器里留下悬空元素）。
            //
            // `xs += [v]` 是「元素入容器」的标准写法（std 里 split / map /
            // filter / concat 全是这么写的），所以这里必须识别方括号字面量
            // 里的标识符并标记移动。`xs += otherList` 是整体拼接，两个操作数
            // 都只被读，不发生移动。
            if (slot->type->getKind() == sema::TypeKind::TK_LIST)            {
                const auto markElementMoved = [&](const std::shared_ptr<ast::ExpressionNode>& e) {
                    if (const auto eid = std::dynamic_pointer_cast<ast::IdentifierNode>(e))                    {
                        // D18 负向用例③：**存储借用** —— 借用不能存进容器。
                        // 容器可能活得比借用源更久（`stash(param)` 返回后，
                        // 形参指向的东西已经随调用方的作用域销毁），
                        // 存进去就是让容器持有悬空指针。
                        if (!checkBorrowNotEscaping(eid->getName(), e->getPos(), "stored into a container"))                        {
                            return;
                        }
                        markVarMoved(eid->getName(), e->getPos(), "stored into a container");
                    }
                };
                const auto rhsNode = infix->getRightNode();
                if (const auto lit = std::dynamic_pointer_cast<ast::ListExpressionNode>(rhsNode))                {
                    std::vector<std::shared_ptr<ast::ExpressionNode>> elems;
                    flattenListElements(lit->getBodyNode(), elems);
                    for (const auto& e : elems) markElementMoved(e);
                }
            }
            if (slot->type->getKind() == sema::TypeKind::TK_STR)            {
                emitStrConcat(lhsSlot, rhsSlot);
            }
            else            {
                emitListConcat(lhsSlot, rhsSlot);
            }
            _asm.mov("%rax", addr);
            return;
        }
        // 先把右值算出来暂存进槽位，再取左值。
        // 不能用 push：右值求值里可能有调用，push 的 8 字节偏移
        // 会让那个调用的 %rsp 失去 16 字节对齐。
        emitExpression(infix->getRightNode());
        const std::string rhsSlot = allocateTemp(sema::getIntType());
        _asm.mov("%rax", rhsSlot);
        _asm.mov(addr, "%rcx");
        _asm.mov(rhsSlot, "%rax");
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
        // P13-1（D9 规则 C + 规范 §11A.1）：重新赋值分两步，顺序不能反。
        //
        // 第一步：若接收方**当前拥有**内容，先释放旧内容。否则旧值直接泄漏；
        // 而若先写后释放，释放的就成了刚写进去的新值（use-after-return 级）。
        // 顺序：右值已在 %rax/%xmm0 -> 存进临时槽保命 -> 释放旧值 -> 装回 -> 写入。
        //
        // 第二步：确定接收方**赋值后的身份**，这是原先漏掉、导致双重释放的一步。
        // 规范 §11A.1 把 `x = y` 定为**壳子赋值**：两个槽共享同一块内容，
        // 只有一方是拥有者。所以：
        //     x = <已有变量/元素/字段>  ->  x 降级为借用（别名），出口不销毁
        //     x = <新值>                ->  x 升格为拥有者，出口销毁
        // 只做第一步的话，`b = a` 之后 a、b 都还是拥有者且指向同一块，
        // 作用域末各 drop 一次 => 0xC0000374 堆损坏（实测）。
        const std::string dropFn = needsDrop(slot->type) ? ensureDropGlue(slot->type) : std::string();
        const bool ownsSlot = !dropFn.empty() && isDropOwned(id->getName());
        std::string savedSlot;
        if (ownsSlot)        {
            const bool srcFloat2 = rhsType && rhsType->isFloat();
            savedSlot = allocateTemp(srcFloat2 ? sema::getFloatType() : sema::getIntType());
            if (srcFloat2) _asm.inst("movsd", "%xmm0", savedSlot);
            else           _asm.mov("%rax", savedSlot);
            _asm.lea(dest, "%rcx");
            _asm.subRsp(32);
            _asm.call(dropFn);
            _asm.addRsp(32);
            // drop 已把槽写成 null，重新装回右值
            if (srcFloat2) _asm.inst("movsd", savedSlot, "%xmm0");
            else           _asm.mov(savedSlot, "%rax");
        }
        if (!dropFn.empty())        {
            // 右值是「已有变量 / 元素 / 字段」= 已经有人管着它的生命周期，
            // 接收方只是又多了一个壳子别名 -> 借用。否则是新值 -> 拥有者。
            //
            // 与 emitVariableDefinition 的 isBorrowInit 同一口径，且同样要求
            // **同类型**：`var r: str = p`（p 是 ptr）是位模式转换，堆块所有权
            // 经 ptr 中转交给 r，r 是新的所有者，不是别名。
            bool rhsIsAlias = false;
            const auto rhsSlot = [&]() -> const CodeGen::VarSlot*                {
                const auto rid = std::dynamic_pointer_cast<ast::IdentifierNode>(rhs);
                if (!rid) return nullptr;
                return const_cast<CodeGen*>(this)->lookupVar(rid->getName());
            }();
            if (rhsSlot)            {
                rhsIsAlias = !(rhsSlot->type && slot->type) || rhsSlot->type->equals(slot->type);
            }
            else if (std::dynamic_pointer_cast<ast::IndexExpressionNode>(rhs))            {
                rhsIsAlias = true;
            }
            else if (const auto inf = std::dynamic_pointer_cast<ast::InfixExpressionNode>(rhs))            {
                if (inf->getOpToken().getType() == core::TokenType::TOKEN_DOT) rhsIsAlias = true;
            }
            setDropAlias(id->getName(), rhsIsAlias);
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
                    // 空容器字面量例外：`[]` / `{}` 没有元素类型信息，
                    // 标注才是权威（`var r: list[str] = []` 必须合法）。
                    if (hasInit && !isEmptyContainerLiteral(def->getValueNode()))                    {
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
            // P13-1（D9 规则 C）：只有**拥有者**才登记销毁。
            //
            //   初始化表达式是「新值」（字面量 / 函数调用结果 / clone()）
            //     -> 拥有者，作用域末销毁
            //   初始化表达式是「已有的变量或元素」（`var ys = xs`、`xs[i]`）
            //     -> **借用**，不销毁（壳子已经有人管了）
            //
            // 这条判据是必须的：std 里 `var s: str = scratch` 这种「把一个
            // 已存在的指针包装成别的静态类型」的写法到处都是（`_dsLoadI64`
            // 的伪造 str 头就是），把它当拥有者会在作用域末把**别人还在用**的
            // 缓冲区 free 掉 —— 实测 ds 的 dictKeys/dictValues/dictRemove
            // 全部因此拿到已释放内存。
            //
            // 顶层 var 走全局槽，生命周期是整个进程，P13-1 不释放
            // （全局槽没有静态已知的「退出点」，留 P13-2 的出生地概念定稿后处理）。
            const bool isBorrowInit = [&] {
                if (!hasInit) return false;
                const auto init = def->getValueNode();
                // P13-2/L3-E（决议 §4.2）：绑定子引用变量名，供 NLL 追踪。
                if (std::dynamic_pointer_cast<ast::RefExpressionNode>(init))
                    bindLastActiveRef(name);
                // P13-2/L3-E（决议 §4.3）：`var a = xs.get_mut(i)` 同样是子引用绑定。
                // 修前 get_mut 登记时 refVar 留空，而 drainActiveRefs 对空 refVar
                // 一律保守保留，于是登记项永不淘汰 —— 后面再来一次 get_mut(0)
                // 就假报"conflicting mutable references"，哪怕第一次早已最后使用。
                if (const auto gc = std::dynamic_pointer_cast<ast::FunctionCallNode>(init))                {
                    const auto gm = std::dynamic_pointer_cast<ast::InfixExpressionNode>(gc->getLeftNode());
                    if (gm && gm->getOpToken().getType() == core::TokenType::TOKEN_DOT)                    {
                        const auto gmid = std::dynamic_pointer_cast<ast::IdentifierNode>(gm->getRightNode());
                        if (gmid && gmid->getName() == "get_mut") bindLastActiveRef(name);
                    }
                }
                // P13-2/L3-C（决议 §6.1/§6.2 位置 1）：`var c = bob.take()` 落定 ——
                // bob（Body 壳）→E 空壳（可 clear/fill 复用），c 是新 Content 拥有者。
                if (const std::string taken = takeReceiverName(init); !taken.empty())                {
                    settlePendingTake(taken); // 挂起→落定 E
                    return false; // 新拥有者
                }
                if (const auto initId = std::dynamic_pointer_cast<ast::IdentifierNode>(init))                {
                    // P13-2/L3（决议 §11）：Content 变量赋值即移动 ——
                    // `var a = b`（b 是 Content[T]）时 b→X，a 是新拥有者，
                    // 不按借用处理。必须是 Content 才特判：str/ptr 的
                    // `var s: str = scratch` 位复用写法依赖借用行为，不能动。
                    if (const auto srcSlot = const_cast<CodeGen*>(this)->lookupVar(initId->getName());
                        srcSlot && srcSlot->type && srcSlot->type->getKind() == sema::TypeKind::TK_MEMORY &&
                        std::dynamic_pointer_cast<sema::MemoryType>(srcSlot->type)->getRole() ==
                            sema::MemoryRole::CONTENT)                {
                        if (!markVarMoved(initId->getName(), initId->getPos(), "moved into '" + name + "'")) return false;
                        return false; // 新拥有者，作用域末销毁
                    }
                    // 只有**同类型**的 `var b = a` 才是借用（两个槽引用同一条
                    // 堆对象）。`var r: str = p`（p 是 ptr）是**位模式转换**：
                    // 堆块的所有权经 ptr 中转交给 r，r 是新的所有者 ——
                    // std 里到处是这种写法（_strSlice / _dsLoadI64 的伪造头）。
                    const auto srcSlot = const_cast<CodeGen*>(this)->lookupVar(initId->getName());
                    if (srcSlot && srcSlot->type && type)                    {
                        return srcSlot->type->equals(type);
                    }
                    // 类型取不到（形参未标注等）时保守按借用处理
                    return true;
                }
                if (std::dynamic_pointer_cast<ast::IndexExpressionNode>(init)) return true;
                // `obj.field` 是 InfixExpressionNode(DOT)
                if (const auto inf = std::dynamic_pointer_cast<ast::InfixExpressionNode>(init))                {
                    if (inf->getOpToken().getType() == core::TokenType::TOKEN_DOT) return true;
                }
                return false;
            }();
            const bool isGlobalSlot = _inTopLevelStatements && _topLevelVarNames.count(name) != 0;
            if (!isGlobalSlot)            {
                if (isBorrowInit)                {
                    registerBorrow(name, type, slot);
                }
 else                {
                    registerDropVar(name, slot, type);
                }
            }
            // P13-2/L3：内存模型类型（含 Body 别名 ClassType）进独立移动表，
            // 以跟踪 F/X 状态（D14 下 ClassType 不进 drop 表，但仍可被 take）。
            //
            // 注意这段**必须**在 isGlobalSlot 判断之外。顶层 `var` 走全局槽，
            // 修前整段（移动状态登记 + 出生地记录 + pool.new 一致性）都写在
            // `if (!isGlobalSlot)` 里面，于是顶层的 Body 变量根本没进移动状态表：
            //   var bob: Body[Player] pool = Player("bob")
            //   var c: Content[Player] pool = bob.take()   // bob 已是 E
            //   sout(bob.name)                             // 修前 exit=0，读空壳
            // 同一段代码放进 fun 里就会正确报 Empty 态错误 —— 行为不一致，
            // 而顶层恰恰是最容易被当成"能跑就行"的地方。
            {
                const bool isMemoryModelVar =
                    (type && type->getKind() == sema::TypeKind::TK_MEMORY) ||
                    (type && type->getKind() == sema::TypeKind::TK_CLASS);
                if (isMemoryModelVar) registerMoveVar(name);
                // P13-2/L3-A（决议 §2）：记录出生地标签（Pool 变量名）。
                // P13-2/L3-A/B（决议 §2/§1.4）：出生地记录与一致性检查。
                // 标签区出生地、pool.new 绑定二者只能指定一次且必须一致。
                const auto birth = def->hasLabels()
                    ? findBirthPlaceLabel(def->getLabelNodes()) : std::string();
                // 决议 §2.2：标签区**最多一个**出生地标签。
                if (def->hasLabels())                {
                    const auto allBirths = findAllBirthPlaceLabels(def->getLabelNodes());
                    if (allBirths.size() > 1)                    {
                        failAt(def->getNameNode()->getPos(),
                               "L3: variable '" + name + "' has " +
                               std::to_string(allBirths.size()) +
                               " birth place labels ('" + allBirths[0] + "', '" + allBirths[1] +
                               "'); at most one birth place label is allowed (P13-2 / §2.2)");
                        return;
                    }
                    // 决议 §2.2：「出生地标签必须是一个 Pool 类型的变量名」。
                    //
                    // 修前标签区里既不解析成类型、又不是 Pool 变量的名字被**静默
                    // 丢弃** —— 出生地靠 findBirthPlaceLabel 的 lookupVar 认，
                    // 认不出就当没写。于是 `var p: Player nosuchpool = Player("a")`
                    // 通过编译，pool 名字拼错也不报，运行时落进默认池。
                    //
                    // 判据顺序与 §2.2/Q-A 一致：能解析成具体类型（含泛型形参）的
                    // 是类型标签，跳过；剩下的必须能解析成 Pool 类型变量。
                    for (const auto& label : def->getLabelNodes())                    {
                        if (!label) continue;
                        const auto full = label->getFullLabel();
                        if (full.empty() || full == "any") continue;
                        bool isGenericParam = false;
                        for (const auto& g : _currentGenericParams)                        {
                            if (g == full) { isGenericParam = true; break; }
                        }
                        if (isGenericParam) continue;
                        const auto t = typeFromLabelNode(label, _currentGenericParams);
                        if (t && t->getKind() != sema::TypeKind::TK_UNKNOWN) continue;
                        const auto slot = const_cast<CodeGen*>(this)->lookupVar(full);
                        if (slot && slot->type &&
                            slot->type->getKind() == sema::TypeKind::TK_POOL) continue;
                        failAt(def->getNameNode()->getPos(),
                               "L3: unknown label '" + full + "' in declaration of '" + name +
                               "'; a birth place label must name a Pool variable (P13-2 / §2.2)");
                        return;
                    }
                }
                const auto newPool = hasInit
                    ? poolNewReceiverName(def->getValueNode()) : std::string();
                if (!birth.empty() && !newPool.empty() && birth != newPool)                {
                    failAt(def->getNameNode()->getPos(),
                           "L3: birth place '" + birth + "' conflicts with pool.new on '" +
                           newPool + "' (specify the birth place only once)");
                    return;
                }
                if (!birth.empty()) _birthPlaces[name] = birth;
                else if (!newPool.empty()) _birthPlaces[name] = newPool;
                // 决议 §1.4：`pool.new[T](...)` 的结果类型就是 T，
                // 必须与声明的变量类型一致。修前完全没校验，
                // `var a: Enemy = pl.new[Player]("x")` 静默通过。
                if (hasInit)                {
                    if (const auto newType = poolNewTypeArg(def->getValueNode()))                    {
                        if (type && !type->equals(newType))                        {
                            failAt(def->getNameNode()->getPos(),
                                   "L3: pool.new[" + newType->toString() + "] result cannot initialize '" +
                                   name + "' of type " + type->toString() + " (P13-2 / §1.4)");
                            return;
                        }
                    }
                }
            }
        }
    }
    sema::TypePtr CodeGen::poolNewTypeArg(const std::shared_ptr<ast::ExpressionNode>& node) const {        // P13-2/L3-B（决议 §1.4）：取 `pool.new[T](...)` 里的 T。
        // 与 poolNewReceiverName 共用 AST 形状判断。
        const auto call = std::dynamic_pointer_cast<ast::FunctionCallNode>(node);
        if (!call) return nullptr;
        const auto member = std::dynamic_pointer_cast<ast::InfixExpressionNode>(call->getLeftNode());
        if (!member || member->getOpToken().getType() != core::TokenType::TOKEN_DOT) return nullptr;
        const auto idx = std::dynamic_pointer_cast<ast::IndexExpressionNode>(member->getRightNode());
        if (!idx) return nullptr;
        const auto mid = std::dynamic_pointer_cast<ast::IdentifierNode>(idx->getLeftNode());
        if (!mid || mid->getName() != "new") return nullptr;
        const auto bracket = std::dynamic_pointer_cast<ast::BracketExpressionNode>(idx->getIndexNode());
        if (!bracket) return nullptr;
        std::vector<std::shared_ptr<ast::ExpressionNode>> typeArgs;
        flattenTypeArgs(bracket->getBodyNode(), typeArgs);
        if (typeArgs.empty()) return nullptr;
        return typeFromTypeArgNode(typeArgs[0], _currentGenericParams);
    }
    std::string CodeGen::takeReceiverName(const std::shared_ptr<ast::ExpressionNode>& node)    {        // 识别 `X.take()`：FunctionCallNode(callee=InfixExpressionNode(DOT,
        // left=IdentifierNode(X), right=IdentifierNode("take")))。
        const auto call = std::dynamic_pointer_cast<ast::FunctionCallNode>(node);
        if (!call) return std::string();
        const auto member = std::dynamic_pointer_cast<ast::InfixExpressionNode>(call->getLeftNode());
        if (!member || member->getOpToken().getType() != core::TokenType::TOKEN_DOT) return std::string();
        const auto method = std::dynamic_pointer_cast<ast::IdentifierNode>(member->getRightNode());
        if (!method || method->getName() != "take") return std::string();
        const auto recv = std::dynamic_pointer_cast<ast::IdentifierNode>(member->getLeftNode());
        return recv ? recv->getName() : std::string();
    }
    sema::TypePtr CodeGen::inferBlockValueType(const std::shared_ptr<ast::ExpressionNode>& body) const {
        const auto block = std::dynamic_pointer_cast<ast::BlockRangerNode>(body);
        if (!block) return inferType(body);
        const auto& exprs = block->getBodyExpressions();
        if (exprs.empty()) return sema::getVoidType();
        return inferType(exprs.back());
    }
    bool CodeGen::bodyHasRetValue(const std::shared_ptr<ast::ExpressionNode>& body) const {
        if (!body) return false;
        if (const auto ret = std::dynamic_pointer_cast<ast::ReturnExpressionNode>(body))        {
            return !ret->exitsFunction() && ret->getReturnNode() != nullptr;
        }
        if (const auto block = std::dynamic_pointer_cast<ast::BlockRangerNode>(body))        {
            for (const auto& e : block->getBodyExpressions())
                if (bodyHasRetValue(e)) return true;
            return false;
        }
        if (const auto cond = std::dynamic_pointer_cast<ast::ConditionNode>(body))        {
            for (const auto& b : cond->getBranchNodes()) if (bodyHasRetValue(b)) return true;
            return false;
        }
        if (const auto branch = std::dynamic_pointer_cast<ast::BranchNode>(body))
            return bodyHasRetValue(branch->getBodyNode());
        if (const auto loop = std::dynamic_pointer_cast<ast::LoopNode>(body))
            return bodyHasRetValue(loop->getBodyNode());
        if (const auto stmt = std::dynamic_pointer_cast<ast::ExpressionStatementNode>(body))
            return bodyHasRetValue(stmt->getExpression());
        return false;
    }
    void CodeGen::emitReturn(const std::shared_ptr<ast::ExpressionNode>& node)    {
        const auto ret = std::dynamic_pointer_cast<ast::ReturnExpressionNode>(node);
        // P13-2/L1+L2+L3-N：`return` 只退函数；`ret` 退最近可返回值作用域 ——
        // 比较 _loopStack / _ifStack / _blockStack 栈顶的 enterSeq，选后进入（更近）的；
        // 三者都空则退函数（保持兼容）。
        if (ret && !ret->exitsFunction())        {
            // 三个栈里取 enterSeq 最大的那个；同等最近时按 块 < if < 循环 的
            // 嵌套方向处理（外层先入栈，内层 enterSeq 更大，实际不会相等）。
            enum class Nearest { FUNCTION, BLOCK, IF, LOOP };
            Nearest nearest = Nearest::FUNCTION;
            size_t best = 0;
            const auto consider = [&](const size_t seq, const Nearest kind)                {
                if (seq > best)                    {
                    best = seq;
                    nearest = kind;
                }
            };
            if (!_blockStack.empty()) consider(_blockStack.back().enterSeq, Nearest::BLOCK);
            if (!_ifStack.empty()) consider(_ifStack.back().enterSeq, Nearest::IF);
            if (!_loopStack.empty()) consider(_loopStack.back().enterSeq, Nearest::LOOP);
            switch (nearest)            {
                case Nearest::BLOCK:
                    emitRetFromBlock(node);
                    return;
                case Nearest::IF:
                    emitRetFromIf(node);
                    return;
                case Nearest::LOOP:
                    emitRetFromLoop(node);
                    return;
                case Nearest::FUNCTION:
                default:
                    break;
            }
        }
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
            // P13-2/L3-D（决议 §6.2 落定位置 3）：`return bob.take()` 落定到 result，
            // bob→E。修前这里没有结算，take 在语句末被 drainPendingTakes 回填成 F，
            // 于是 `fun f(t: Take[Player]): Content[Player] ensures[!t] { return t.take() }`
            // 报 "Take parameter 't' is Held at exit, but ensures requires Empty" ——
            // 决议 §6.2 明文列出的四种落定位置之一实际不可用。
            if (const std::string taken = takeReceiverName(ret->getReturnNode()); !taken.empty())
                settlePendingTake(taken);
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
        // P13-1（D4 + D9 规则 C）：ret 之前先在**当前作用域链**上发 drop 调用。
        //
        // 顺序必须是这样：先算返回值（已在 %rax / %xmm0），再销毁其余变量，
        // 最后跳转。所以先把返回值存进临时槽 —— 销毁调用会冲掉寄存器。
        //
        // 「拥有者整体 ret」= 移动（D9）：被返回的那个变量**不能**再销毁，
        // 否则返回给调用者的是一块已释放内存。这里靠 markVarMoved 把它从
        // 销毁表里摘掉（只摘最内层同名项，即刚被返回的那个）。
        {
            const auto retExpr = ret ? ret->getReturnNode() : nullptr;
            // 返回值需要跨销毁调用保存：按**转换后**的值类别决定寄存器
            const auto effType = (_functionReturnAnnotated && _functionReturnType)
                                     ? _functionReturnType
                                     : (retExpr ? inferType(retExpr) : nullptr);
            const bool retIsFloat = effType && effType->isFloat();
            const bool retHasValue = retExpr && !(effType && effType->isVoid());
            std::string retSlot;
            if (retHasValue)            {
                retSlot = allocateTemp(effType);
                if (retIsFloat) _asm.inst("movsd", "%xmm0", retSlot);
                else            _asm.mov("%rax", retSlot);
            }
            // 移动语义：如果返回值就是本层的某个变量，它整体转移给调用方。
            //
            // D18 负向用例②：**返回借用的形参必须编译报错**。
            // 用户 2026-11-17 明确裁决：**严格报错，不为了能跑而造近似**。
            // 借用不拥有资源，把它 ret 出去等于把「别人管的生命周期」
            // 当成本函数的成果交出去。
            //
            // 已知后果（用户已确认接受）：std 里 `fun toStr(arg: str): str
            // { ret arg }` 这类恒等函数会被这条规则拦下，标准库需要改写 ——
            // 但**本阶段不改**，等内存规范（P13-2 起的 Body/Content 与
            // 能力类型）真正落地之后再改，避免用临时近似污染 std。
            if (const auto id = std::dynamic_pointer_cast<ast::IdentifierNode>(retExpr))            {
                if (!checkBorrowNotEscaping(id->getName(), node->getPos(), "returned"))                {
                    return;
                }
                if (!markVarMoved(id->getName(), node->getPos(), "returned")) return;
            }
            // P13-2/L3-F（决议 §5.2）：返回引用不得指向本函数 local。
            // &localVar / localVar.field（引用根是局部变量且非形参）报错。
            if (const auto refExprNode = std::dynamic_pointer_cast<ast::RefExpressionNode>(retExpr))            {
                const auto root = [&]() -> std::string {
                    if (const auto id = std::dynamic_pointer_cast<ast::IdentifierNode>(refExprNode->getOperand()))
                        return id->getName();
                    if (const auto dot = std::dynamic_pointer_cast<ast::InfixExpressionNode>(refExprNode->getOperand()))                    {
                        if (const auto id = std::dynamic_pointer_cast<ast::IdentifierNode>(dot->getLeftNode()))
                            return id->getName();
                    }
                    return std::string();
                }();
                if (!root.empty() && !isFunctionParam(root))                {
                    failAt(node->getPos(), "L3: cannot return a reference to local '" + root +
                           "' (P13-2 / from: references must not point to function locals)");
                    return;
                }
                // 决议 §9.4：返回引用的来源必须 ⊆ 声明的 from 集合。
                // 修前只做了「不得返回 local」这一半，`from[p]` 里 ret &q
                // （q 是另一个形参）静默通过。
                if (!root.empty() && !_fromRoots.empty() && !_fromRoots.count(root))                {
                    failAt(node->getPos(), "L3: returned reference points to '" + root +
                           "', which is not listed in this function's from clause (P13-2 / §9.4)");
                    return;
                }
            }
            // 逆序销毁本层所有未移动的可销毁变量（含所有外层作用域的？
            // 不 —— ret 会跳出整条链，但外层变量在**外层的出口**销毁。
            // 这里 ret 是终结性跳转，不会再回到外层的词法出口，
            // 所以必须把链上每一层都销毁，从内到外。）
            for (size_t i = _scopeDrops.size(); i > 0; --i)            {
                emitScopeDrops(i - 1, true);
            }
            if (retHasValue)            {
                if (retIsFloat) _asm.inst("movsd", retSlot, "%xmm0");
                else            _asm.mov(retSlot, "%rax");
            }
        }
        if (!_returnLabel.empty())        {
            // P13-2/L3-F：ret 一个 Take 形参 = 内容转移出去，形参→EMPTY；
            // 然后逐出口校验 ensures。
            if (ret && ret->getReturnNode())            {
                if (const auto id = std::dynamic_pointer_cast<ast::IdentifierNode>(ret->getReturnNode()))
                    if (_ensuresMarks.count(id->getName()))
                        setBodyState(id->getName(), BodyState::EMPTY, node->getPos(), "returned");
            }
            checkEnsuresAtExit(node->getPos());
            _asm.jmp(_returnLabel);
        }
    }
    void CodeGen::emitRetFromLoop(const std::shared_ptr<ast::ExpressionNode>& node)    {
        const auto ret = std::dynamic_pointer_cast<ast::ReturnExpressionNode>(node);
        // P13-2/L1 / 决议 §12.1、§13.1：`ret` 在循环里退最近作用域，
        // 等价 `break value`。行为对齐 emitBreak：
        //   1. 先算值并存临时槽（drop 调用会冲寄存器）
        //   2. 只销毁「循环体这一层」的拥有者（不是全部 scope 链）
        //   3. 只弹循环体内建立的 try 帧（不 unwind 到函数顶）
        //   4. 跳循环的 breakLabel
        // 与 break 的差别：break 不允许带值（L0-5 已设为报错），
        // 而 `ret value` 在循环里是合法的循环出口值（L2 的类型标注/合流再消费）。
        const auto& ctx = _loopStack.back();
        std::string valueSlot;
        bool valueIsFloat = false;
        bool hasValue = false;
        if (ret && ret->getReturnNode())        {
            emitExpression(ret->getReturnNode());
            if (hasFailed()) return;
            const auto vt = inferType(ret->getReturnNode());
            valueIsFloat = vt && vt->isFloat();
            hasValue = !(vt && vt->isVoid());
            if (hasValue)            {
                valueSlot = allocateTemp(valueIsFloat ? sema::getFloatType() : sema::getIntType());
                if (valueIsFloat) _asm.inst("movsd", "%xmm0", valueSlot);
                else              _asm.mov("%rax", valueSlot);
            }
        }
        // 只销毁循环体层及其子块（口径同 emitBreak）
        for (size_t i = _scopeDrops.size(); i > ctx.dropDepth; --i)        {
            emitScopeDrops(i - 1, true);
        }
        // 只弹循环体内建立的 try 帧（口径同 emitBreak）
        emitEhUnwindTo(ctx.ehDepth);
        if (hasValue)        {
            if (valueIsFloat) _asm.inst("movsd", valueSlot, "%xmm0");
            else              _asm.mov(valueSlot, "%rax");
            // P13-2/L2：ret value 在循环里等价 break value，写入循环结果槽。
            if (!ctx.valueSlot.empty())            {
                if (valueIsFloat) _asm.inst("movsd", "%xmm0", ctx.valueSlot);
                else              _asm.mov("%rax", ctx.valueSlot);
            }
        }
        // P13-2/L3-G（决议 §13.1）：`ret` 在循环里等价 `break value`，
        // 因此它也是循环的一类出口，状态必须和 emitBreak 一样参与合流。
        if (!_loopExitStates.empty())
            _loopExitStates.back().push_back(snapshotMoveStates());
        _asm.jmp(ctx.breakLabel);
    }
    void CodeGen::emitRetFromIf(const std::shared_ptr<ast::ExpressionNode>& node)    {
        const auto ret = std::dynamic_pointer_cast<ast::ReturnExpressionNode>(node);
        const auto& ctx = _ifStack.back();
        std::string valueSlot;
        bool valueIsFloat = false;
        if (ret && ret->getReturnNode())        {
            const auto vt = inferType(ret->getReturnNode());
            if (ctx.annotatedType)            {
                const auto why = checkAssignable(ctx.annotatedType, vt);
                if (!why.empty())                {
                    failAt(ret->getReturnNode()->getPos(),
                           "L2: ret value does not match if result type: " + why);
                    return;
                }
            }
            emitExpression(ret->getReturnNode());
            if (hasFailed()) return;
            valueIsFloat = vt && vt->isFloat();
            valueSlot = allocateTemp(valueIsFloat ? sema::getFloatType() : sema::getIntType());
            if (valueIsFloat) _asm.inst("movsd", "%xmm0", valueSlot);
            else              _asm.mov("%rax", valueSlot);
        }
        // 只销毁 if 分支这一层的拥有者（口径同循环 break）
        for (size_t i = _scopeDrops.size(); i > ctx.dropDepth; --i)        {
            emitScopeDrops(i - 1, true);
        }
        emitEhUnwindTo(ctx.ehDepth);
        if (!valueSlot.empty())        {
            if (valueIsFloat) _asm.inst("movsd", valueSlot, "%xmm0");
            else              _asm.mov(valueSlot, "%rax");
            if (valueIsFloat) _asm.inst("movsd", "%xmm0", ctx.valueSlot);
            else              _asm.mov("%rax", ctx.valueSlot);
        }
        _asm.jmp(ctx.endLabel);
    }
    void CodeGen::emitRetFromBlock(const std::shared_ptr<ast::ExpressionNode>& node)    {
        const auto ret = std::dynamic_pointer_cast<ast::ReturnExpressionNode>(node);
        const auto& ctx = _blockStack.back();
        // 决议 §13.1/§13.2：`ret` 退最近的可返回值作用域，带出的值必须匹配
        // 该作用域的类型。块表达式的类型来自 `: T` 标注或出口值推断，
        // 有标注时这里就是权威校验点。
        if (ret && ret->getReturnNode() && ctx.annotatedType)        {
            const auto why = checkAssignable(ctx.annotatedType, inferType(ret->getReturnNode()));
            if (!why.empty())            {
                failAt(ret->getReturnNode()->getPos(),
                       "L3: ret value does not match block result type: " + why);
                return;
            }
        }
        std::string valueSlot;
        bool valueIsFloat = false;
        if (ret && ret->getReturnNode())        {
            const auto vt = inferType(ret->getReturnNode());
            valueIsFloat = vt && vt->isFloat();
            emitExpression(ret->getReturnNode());
            if (hasFailed()) return;
            if (!(vt && vt->isVoid()))            {
                valueSlot = allocateTemp(valueIsFloat ? sema::getFloatType() : sema::getIntType());
                if (valueIsFloat) _asm.inst("movsd", "%xmm0", valueSlot);
                else              _asm.mov("%rax", valueSlot);
            }
        }
        // 只销毁块这一层的拥有者（口径同 if 分支 / 循环 break）：
        // 块外的变量由外层出口负责，块内的必须在这里销毁 ——
        // ret 是终结性跳转，走不到 popScope 的词法出口。
        for (size_t i = _scopeDrops.size(); i > ctx.dropDepth; --i)        {
            emitScopeDrops(i - 1, true);
        }
        // 只弹块内建立的 try 帧
        emitEhUnwindTo(ctx.ehDepth);
        if (!valueSlot.empty())        {
            if (valueIsFloat) _asm.inst("movsd", valueSlot, "%xmm0");
            else              _asm.mov(valueSlot, "%rax");
            if (!ctx.valueSlot.empty())            {
                if (valueIsFloat) _asm.inst("movsd", "%xmm0", ctx.valueSlot);
                else              _asm.mov("%rax", ctx.valueSlot);
            }
        }
        _asm.jmp(ctx.endLabel);
    }
    void CodeGen::emitIf(const std::shared_ptr<ast::ExpressionNode>& node)    {
        const auto cond = std::dynamic_pointer_cast<ast::ConditionNode>(node);
        if (!cond)        {
            fail("Not a condition node: " + node->toString());
            return;
        }
        const std::string endLabel = _asm.newLabel("cond_end");
        // P13-2/L2(Q-C)：写了 `: T` 就是权威校验——每个正常出口必须产出 T。
        // 标注挂在 ConditionNode 上（取第一个分支的 `: T`）。
        sema::TypePtr annotatedType;
        std::string ifValueSlot;
        if (!cond->getResultLabels().empty())        {
            annotatedType = typeFromLabelList(cond->getResultLabels(), _currentGenericParams);
            if (!annotatedType || annotatedType->getKind() == sema::TypeKind::TK_UNKNOWN)            {
                failAt(node->getPos(), "L2: unknown result type '" +
                       cond->getResultLabels().front()->getFullLabel() + "' in if annotation");
                return;
            }
            ifValueSlot = allocateTemp(annotatedType);
            if (annotatedType->isFloat()) _asm.inst("pxor", "%xmm0", "%xmm0");
            else                          _asm.movImm(0, "%rax");
            if (annotatedType->isFloat()) _asm.inst("movsd", "%xmm0", ifValueSlot);
            else                          _asm.mov("%rax", ifValueSlot);
        }
        // P13-2/L3-G（决议 §8.1）：分支出入口做状态快照 + 格上界合流。
        // 每条分支都从入口状态出发（兄弟分支互不污染），到得了汇合点的
        // 分支各留一份出口快照，最后逐项取上界写回。
        const MoveStateSnapshot entryStates = snapshotMoveStates();
        std::vector<MoveStateSnapshot> exitPaths;
        bool hasUnconditionalElse = false;
        for (const auto& branchExpr : cond->getBranchNodes())        {
            const auto branch = std::dynamic_pointer_cast<ast::BranchNode>(branchExpr);
            if (!branch) continue;
            const std::string nextLabel = _asm.newLabel("cond_next");
            // else 分支在 AST 里不是「无条件的」，而是带一个合成的
            // BooleanLiteralNode(true) 条件（见 buildConditionExpression）。
            // 只认「条件为字面 true」当无条件 else：有它就没有 fall-through 路径。
            if (!isAlwaysTrueCondition(branch->getConditionNode()))            {
                emitExpression(branch->getConditionNode());
                _asm.inst("testq", "%rax", "%rax");
                _asm.je(nextLabel);
            }
 else            {
                hasUnconditionalElse = true;
            }
            if (annotatedType && !bodyHasRetValue(branch->getBodyNode()))            {
                const auto valueType = inferBlockValueType(branch->getBodyNode());
                if (!valueType || valueType->isVoid())                {
                    failAt(branch->getBodyNode() ? branch->getBodyNode()->getPos() : node->getPos(),
                           "L2: if branch has no value but result type " +
                           annotatedType->toString() + " is annotated");
                    return;
                }
                const auto why = checkAssignable(annotatedType, valueType);
                if (!why.empty())                {
                    failAt(branch->getBodyNode()->getPos(),
                           "L2: if branch value does not match annotated result type: " + why);
                    return;
                }
            }
            // 分支体一律从入口状态出发：不能看见兄弟分支留下的改动。
            // 必须在 pushScope 之前恢复，否则栈深度对不上。
            restoreMoveStates(entryStates);
            pushScope();
            if (!ifValueSlot.empty())            {
                _ifStack.push_back({endLabel, ifValueSlot, annotatedType,
                                    _scopeDrops.size(), _ehStack.size(), ++_scopeEnterSeq});
            }
            emitStatement(branch->getBodyNode());
            if (!ifValueSlot.empty()) _ifStack.pop_back();
            popScope();
            // 分支体执行后的状态（popScope 已丢弃分支局部那一层，
            // 深度与入口一致，改的都是外层 map）。
            // 发散的分支（ret/throw/break/continue）到不了汇合点，不参与合流。
            if (!statementDiverges(branch->getBodyNode()))
                exitPaths.push_back(snapshotMoveStates());
            // 分支末值写入结果槽（ret value 出口已由 emitRetFromIf 写入）
            if (!ifValueSlot.empty() && !bodyHasRetValue(branch->getBodyNode()))            {
                if (annotatedType->isFloat()) _asm.inst("movsd", "%xmm0", ifValueSlot);
                else                          _asm.mov("%rax", ifValueSlot);
            }
            _asm.jmp(endLabel);
            _asm.label(nextLabel);
        }
        _asm.label(endLabel);
        if (!ifValueSlot.empty())        {
            if (annotatedType->isFloat()) _asm.inst("movsd", ifValueSlot, "%xmm0");
            else                          _asm.mov(ifValueSlot, "%rax");
        }
        // 没有无条件 else 时，条件全假会直接落到汇合点，那条路径保持入口状态。
        if (!hasUnconditionalElse) exitPaths.push_back(entryStates);
        if (!exitPaths.empty())        {
            joinMoveStatePaths(exitPaths);
            // 某条路径把不透明的 Take 形参推到了确定状态 → 合流后的 ? 是真的不确定。
            refreshOpaqueAfterMerge(exitPaths);
        }
 else                    restoreMoveStates(entryStates); // 全部分支发散：维持入口
    }
    void CodeGen::emitBlock(const std::shared_ptr<ast::ExpressionNode>& node, const bool asValue)    {
        const auto block = std::dynamic_pointer_cast<ast::BlockRangerNode>(node);
        if (!block)        {
            fail("Not a block: " + node->toString());
            return;
        }
        // P13-2/L3-N（决议 §12.1 / §13.1 / §15.1）：
        //   右值位置的裸块 = 「块表达式」，是**可返回值作用域** ——
        //     块内 `ret value` 退这个块并带出值；
        //   语句位置的裸块**不是**可返回值作用域（§15.1 明写「ret / break /
        //     continue 在裸块内按外层上下文生效」），块内 `ret` 直接退外层。
        // 所以只有 asValue 时才建立 BlockContext，语句块路径与 L1 完全一致。
        std::string valueSlot;
        sema::TypePtr blockType;
        std::string endLabel;
        if (asValue)        {
            blockType = inferType(node);
            if (blockType && !blockType->isVoid())            {
                valueSlot = allocateTemp(blockType);
                endLabel = _asm.newLabel("block_end");
                // 零初始化：块内可能出现不带值的 `ret`（提前退出），
                // 那种出口不写值槽，出口读到的必须是零值而不是栈上垃圾。
                if (blockType->isFloat())                {
                    _asm.inst("xorpd", "%xmm0", "%xmm0");
                    _asm.inst("movsd", "%xmm0", valueSlot);
                }
  else                {
                    _asm.movImm(0, "%rax");
                    _asm.mov("%rax", valueSlot);
                }
            }
        }
        // dropDepth 取**pushScope 之前**的深度：块自己的这一层变量在 `ret`
        // 出口也必须销毁（ret 是终结性跳转，走不到 popScope 的词法出口）。
        // emitIf 那边取的是 pushScope 之后的深度，因为它的分支体是一个嵌套块，
        // 差一层正好对上。
        const size_t dropDepthBefore = _scopeDrops.size();
        pushScope();
        if (!valueSlot.empty())        {
            _blockStack.push_back({endLabel, valueSlot, blockType, dropDepthBefore,
                                   _ehStack.size(), ++_scopeEnterSeq});
        }
        const auto bodyExprs = block->getBodyExpressions();
        for (size_t i = 0; i < bodyExprs.size(); ++i)        {
            const auto& expr = bodyExprs[i];
            // 末表达式本身是块时，它也在值位置（本块的值就是它的值），
            // 所以按块表达式发射 —— 这样它内部的 `ret` 退的是它自己，
            // 符合 §13.1「ret 退最近一层」。其余一律按语句发射。
            if (!valueSlot.empty() && i + 1 == bodyExprs.size() &&
                std::dynamic_pointer_cast<ast::BlockRangerNode>(expr))            {
                emitBlock(expr, true);
            }
  else            {
                emitStatement(expr);
            }
        }
        // 末值落槽必须发生在 popScope **之前**：popScope 发出的销毁调用会
        // 冲掉 %rax / %xmm0。块内若已有 `ret value` 出口，那段代码是不可达的
        // （ret 已经 jmp 到 endLabel），留着不影响运行。
        if (!valueSlot.empty() && !hasFailed())        {
            if (blockType->isFloat()) _asm.inst("movsd", "%xmm0", valueSlot);
            else                      _asm.mov("%rax", valueSlot);
        }
        if (!valueSlot.empty()) _blockStack.pop_back();
        popScope();
        if (!valueSlot.empty())        {
            _asm.label(endLabel);
            if (blockType->isFloat()) _asm.inst("movsd", valueSlot, "%xmm0");
            else                      _asm.mov(valueSlot, "%rax");
        }
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
        // P13-2/L2(Q-C)：写了 `: T` 就是权威校验——循环体末值必须匹配 T。
        // 同时分配结果值槽：自然结束写入 T 的零值，break/ret value 写入携带值，
        // 循环出口统一从槽读出（决议 §12.2）。
        std::string loopValueSlot;
        sema::TypePtr loopAnnotatedType;
        if (!loop->getResultLabels().empty())        {
            const auto annotated = typeFromLabelList(loop->getResultLabels(), _currentGenericParams);
            if (!annotated || annotated->getKind() == sema::TypeKind::TK_UNKNOWN)            {
                failAt(node->getPos(), "L2: unknown result type '" +
                       loop->getResultLabels().front()->getFullLabel() + "' in loop annotation");
                return;
            }
            const auto valueType = inferBlockValueType(loop->getBodyNode());
            if (!valueType || valueType->isVoid())            {
                failAt(loop->getBodyNode() ? loop->getBodyNode()->getPos() : node->getPos(),
                       "L2: loop body has no value but result type " +
                       annotated->toString() + " is annotated");
                return;
            }
            const auto why = checkAssignable(annotated, valueType);
            if (!why.empty())            {
                failAt(loop->getBodyNode()->getPos(),
                       "L2: loop body value does not match annotated result type: " + why);
                return;
            }
            loopAnnotatedType = annotated;
            // 自然结束取 T 的零值（决议 §12.2）：标注循环先写零值到槽。
            loopValueSlot = allocateTemp(annotated);
            if (annotated->isFloat()) _asm.inst("pxor", "%xmm0", "%xmm0");
            else                      _asm.movImm(0, "%rax");
            if (annotated->isFloat()) _asm.inst("movsd", "%xmm0", loopValueSlot);
            else                      _asm.mov("%rax", loopValueSlot);
        }
        _loopStack.push_back({continueLabel, endLabel, _ehStack.size(), _scopeDrops.size(),
                              loopValueSlot, loopAnnotatedType, ++_scopeEnterSeq});
        // P13-2/L3-G（决议 §8.1/§14）：循环状态不动点。
        // 进入前状态 S；沿循环体推演得到回边状态 B；循环后 = S ⊔ B ⊔ break 出口。
        // 循环体可能执行 0 次，所以「没进过循环体」这条路径也必须参与合流。
        const MoveStateSnapshot entryStates = snapshotMoveStates();
        _loopExitStates.emplace_back();
        _loopBackStates.emplace_back();
        pushScope();
        if (loop->getBodyNode())        {
            if (const auto block = std::dynamic_pointer_cast<ast::BlockRangerNode>(loop->getBodyNode()))            {
                for (const auto& e : block->getBodyExpressions()) emitStatement(e);
            }
 else            {
                emitStatement(loop->getBodyNode());
            }
        }
        // 回边状态：循环体正常走完（没被 break/ret 打断）时的状态。
        if (!statementDiverges(loop->getBodyNode()))
            _loopBackStates.back().push_back(snapshotMoveStates());
        popScope();
        _loopStack.pop_back();
        // for 的 update 在每轮体末尾、回跳之前求值。
        // continue 也落在这里（见上面 continueLabel 的取法）。
        if (forLoop && forLoop->getUpdateNode())        {
            _asm.label(continueLabel);
            emitStatement(forLoop->getUpdateNode());
            // update 每轮都执行，它的出口也是一条回边路径。
            _loopBackStates.back().push_back(snapshotMoveStates());
        }
        _asm.jmp(condLabel);
        _asm.label(endLabel);
        // 决议 §8.1/§14：循环后状态 = 进入前状态 ⊔ 回边状态 ⊔ break 出口状态。
        // 循环体可能一次都不执行，所以入口状态本身就是一条到得了出口的路径；
        // 任一维不一致就降为 ?（循环后只能走 get() 之类的安全入口）。
        std::vector<MoveStateSnapshot> exitPaths = _loopBackStates.back();
        for (const auto& p : _loopExitStates.back()) exitPaths.push_back(p);
        _loopExitStates.pop_back();
        _loopBackStates.pop_back();
        exitPaths.push_back(entryStates);
        joinMoveStatePaths(exitPaths);
        refreshOpaqueAfterMerge(exitPaths);
        if (!loopValueSlot.empty())        {
            if (loopAnnotatedType && loopAnnotatedType->isFloat())
                _asm.inst("movsd", loopValueSlot, "%xmm0");
            else
                _asm.mov(loopValueSlot, "%rax");
        }
        popScope();
    }
    void CodeGen::emitBreak(const std::shared_ptr<ast::ExpressionNode>& node)    {
        if (_loopStack.empty())        {
            fail("'break' outside of a loop");
            return;
        }
        const auto& ctx = _loopStack.back();
        // P13-2/L2(Q-C)：`break value` 只有在带 `: T` 标注的循环里才合法
        // （值写入结果槽）；无标注循环里带值是静默丢弃，明确报错。
        const auto brk = std::dynamic_pointer_cast<ast::BreakExpressionNode>(node);
        const auto breakValue = brk ? brk->getValueNode() : nullptr;
        if (breakValue && ctx.valueSlot.empty())        {
            failAt(node ? node->getPos() : utils::Pos{},
                   "L2: 'break value' requires the loop to have a ': T' annotation");
            return;
        }
        std::string valueSlot;
        bool valueIsFloat = false;
        if (breakValue)        {
            // 先算值；类型必须匹配标注（决议 §13.2）
            const auto vt = inferType(breakValue);
            if (!checkAssignable(ctx.annotatedType, vt).empty())            {
                failAt(breakValue->getPos(),
                       "L2: break value does not match annotated result type: " +
                       checkAssignable(ctx.annotatedType, vt));
                return;
            }
            emitExpression(breakValue);
            if (hasFailed()) return;
            valueIsFloat = vt && vt->isFloat();
            valueSlot = allocateTemp(valueIsFloat ? sema::getFloatType() : sema::getIntType());
            if (valueIsFloat) _asm.inst("movsd", "%xmm0", valueSlot);
            else              _asm.mov("%rax", valueSlot);
        }
        // P13-1（D4）：break 是终结性跳转，跳出后**不会**再走到循环体的词法
        // 出口，所以循环体内登记的拥有者必须在跳转前销毁，否则每 break 一次
        // 就泄漏一批。顺序：先销毁（会冲掉寄存器，但 break 不带走任何值），
        // 再弹 try 帧，最后跳。
        //
        // 只销毁 dropDepth 之后（= 循环体这一层及其被子块）的登记项：
        // 循环外层的变量由外层出口负责，不能在这里提前释放。
        for (size_t i = _scopeDrops.size(); i > ctx.dropDepth; --i)        {
            emitScopeDrops(i - 1, true);
        }
        // 只弹循环体内建立的帧；循环外的 try 帧要留着。
        emitEhUnwindTo(ctx.ehDepth);
        if (!valueSlot.empty())        {
            if (valueIsFloat) _asm.inst("movsd", valueSlot, "%xmm0");
            else              _asm.mov(valueSlot, "%rax");
            if (valueIsFloat) _asm.inst("movsd", "%xmm0", ctx.valueSlot);
            else              _asm.mov("%rax", ctx.valueSlot);
        }
        // P13-2/L3-G（决议 §8.1）：break 是循环的另一类出口，它带来的状态
        // 与「条件为假退出」不同，必须单独记一条路径参与循环后的合流。
        if (!_loopExitStates.empty())
            _loopExitStates.back().push_back(snapshotMoveStates());
        _asm.jmp(ctx.breakLabel);
    }
    void CodeGen::emitContinue()    {
        if (_loopStack.empty())        {
            fail("'continue' outside of a loop");
            return;
        }
        // P13-1（D4）：与 break 同理。continue 回到条件判断（for 则是 update 段），
        // 循环体这一层的变量每轮都是新的，不在跳转前销毁就会每轮泄漏一批。
        for (size_t i = _scopeDrops.size(); i > _loopStack.back().dropDepth; --i)        {
            emitScopeDrops(i - 1, true);
        }
        emitEhUnwindTo(_loopStack.back().ehDepth);
        // P13-2/L3-G（决议 §8.1）：continue 也是一条回边，它的状态要参与
        // 循环不动点的合流，否则「只在 continue 之前的路径上改动」会被漏掉。
        if (!_loopBackStates.empty())
            _loopBackStates.back().push_back(snapshotMoveStates());
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
        // P13-2/L3-I：catch 体节点留到后面判发散用（catch 里 ret/throw 时
        // 这条路径到不了汇合点，不能参与状态合流）。
        std::shared_ptr<ast::ExpressionNode> catchBodyForDivergence;

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

        // P13-1（D13）：记下 try 体的作用域下标，并把帧标成「在体内」。
        //
        // 下标要算准：pushScope 之后 try 体那一层的下标恰好等于当前的
        // _scopeDrops.size()（push 前）。判据是「登记下标 < 这个值」= 比 try
        // 活得久。早先写成 +1，导致 try 体内**自己新声明**的拥有者也被误判
        // 成外层的（实测 d13_inner_ok 假报错）。
        const size_t tryBodyScope = _scopeDrops.size();
        _ehStack.push_back({frameOffset, finallyBody, tryBodyScope, true});
        // 本 try 之外的活动帧数。两条路径（正常 / 异常落地）都要把编译期
        // 帧栈恢复到这一深度 —— 它们各自发一次 core_eh_pop，但 _ehStack
        // 只能被弹一次，否则后面嵌套的 try 会算错深度。
        const size_t outerDepth = _ehStack.size() - 1;

        // ---- try 体 ----
        pushScope();
        // P13-2/L3-I（决议 §9.2）：catch 之后 Take 形参状态回到 `?`，不沿用承诺。
        // 记下 try 体的**入口**快照，异常落地时拿它复原再推成 `?`。
        const auto tryEntryStates = snapshotMoveStates();
        if (tryNode->getTryBody())        {
            for (const auto& e : tryNode->getTryBody()->getBodyExpressions()) emitStatement(e);
        }
        popScope();
        // 正常走完这条路径的状态，要在 finally / catch 之前先存下来 ——
        // 后面两条路径共用一份编译期状态表，不隔离就会互相污染。
        const auto normalExitStates = snapshotMoveStates();

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
            // P13-2/L3-I（决议 §9.2）：catch 之后 Take 形参状态回到 `?`。
            //
            // 语义：`ensures` 是「正常返回时我保证成这样」的承诺。异常路径上
            // try 体可能在任何一条语句处中断 —— 它后面那些 take / clear 有没有
            // 执行过是**不确定**的，所以不能在 catch 里沿用 try 体走完后的状态。
            // 实测（修前）：try 里 take、catch 后 ret，`ensures[!t]` 被沿用而通过。
            //
            // 做法：先用 try 入口快照把状态复原（抹掉 try 体里可能只执行了一半的
            // 改动），再把**入口时就已经是 Take 形参**的那些变量统一推成 `?`。
            restoreMoveStates(tryEntryStates);
            for (auto& [name, state] : _moveStates.back())            {
                if (_ensuresMarks.count(name) == 0) continue; // 只管 ensures 里的 Take 形参
                state = BodyState::UNKNOWN;
                // 入口 `?` 本来就是不透明的，此时不能被当成「合流出来的 ?」拒绝。
                _opaqueMoveVars.insert(name);
            }
            // 决议 §7.3 / §9.2：`?` 对 Take 形参是不透明的 —— 字段/方法/take 仍合法。
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
            catchBodyForDivergence = first.second;
        }

        // finally 在正常路径和 catch 路径上都要跑。
        if (hasFinally)        {
            pushScope();
            for (const auto& e : finallyBody->getBodyExpressions()) emitStatement(e);
            popScope();
        }

        // P13-2/L3-I（决议 §9.2）：把「正常走完」和「异常落地」两条路径的状态合流。
        //
        // 两条路径共用同一份编译期状态表，catch 里为 §9.2 推的 `?` 会直接
        // 盖掉正常路径上已经确定的状态 —— 实测（只做 catch 复位时）：
        //   try { var c = t.take() } catch (e: str) { throw "rethrow" }
        //   ret 1
        // 正常路径上 t 明明是 E，却因为 catch 分支写了 ? 而误报
        // 「unknown state at function exit」。
        //
        // 合流规则与 L3-G 的分支合流一致：任一出口发散则该出口不参与；
        // 只剩一条路径时直接用它的状态，不会平白升成 ?。
        if (hasCatch)        {
            std::vector<MoveStateSnapshot> exits;
            // 异常路径：catch 体走完之后的状态（已含 catch 里的 take 等改动）。
            if (!statementDiverges(catchBodyForDivergence))
                exits.push_back(snapshotMoveStates());
            // 正常路径：try 体走完后、进 finally 之前存下的那份。
            exits.push_back(normalExitStates);
            if (!exits.empty())            {
                joinMoveStatePaths(exits);
                refreshOpaqueAfterMerge(exits);
            }
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
    std::string CodeGen::dictLiteralKeyText(const std::shared_ptr<ast::ExpressionNode>& keyNode)    {
        if (!keyNode) return std::string();
        // 剥掉括号：`{"a": 1, ("a"): 2}` 是同一个键
        if (const auto paren = std::dynamic_pointer_cast<ast::ParenRangerNode>(keyNode))        {
            return dictLiteralKeyText(paren->getRangerNode());
        }
        if (const auto lit = std::dynamic_pointer_cast<ast::StringLiteralNode>(keyNode))        {
            // 按**内容**判等，不是按节点的 toString（那样引号/转义形态会干扰）
            return "s:" + lit->unescapedString();
        }
        if (const auto lit = std::dynamic_pointer_cast<ast::IntegerLiteralNode>(keyNode))        {
            return "i:" + lit->literalString();
        }
        if (const auto lit = std::dynamic_pointer_cast<ast::CharacterLiteralNode>(keyNode))        {
            return "c:" + lit->literalString();
        }
        if (const auto lit = std::dynamic_pointer_cast<ast::BooleanLiteralNode>(keyNode))        {
            return "b:" + lit->literalString();
        }
        if (std::dynamic_pointer_cast<ast::NullLiteralNode>(keyNode))        {
            return "n:";
        }
        return std::string();
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
        // ABI v2：header 与 data **分离**（docs/p13_plan.md D12 裁决 A）。
        //   ListHeader { len@0, cap@8, elem_size@16, ptr data@24 }
        // 为什么要分离：原地追加扩容时会 realloc data；若 data 内联在
        // header 之后，realloc 会让调用者的变量、其他拷贝、形参里的指针
        // 全部悬空（单指针布局无法同时更新所有别名）。分离后 header 地址
        // 永不变，扩容只改写 header 里那个指针。
        const std::string headerSlot = allocateTemp(inferListLiteralType(node));
        const std::string dataSlot = allocateTemp(sema::getPtrType());
        emitAlloc(kListHeaderSize);
        _asm.mov("%rax", headerSlot);
        // data 块单独分配（n==0 时不分配，留 null；null+0 参与 mem_copy 安全）
        if (n == 0)            {
            _asm.movImm(0, "%rax");
        }
 else            {
            emitAlloc(n * 8);
        }
        _asm.mov("%rax", dataSlot);
        _asm.mov(headerSlot, "%rcx");
        _asm.movImm(static_cast<long long>(n), "%rax");
        _asm.mov("%rax", "0(%rcx)");
        // len
        _asm.mov("%rax", "8(%rcx)");
        // cap（暂不扩容，等于 len）
        _asm.movImm(8, "%rax");
        _asm.mov("%rax", "16(%rcx)");
        // elem_size（元素槽恒 8 字节）
        _asm.mov(dataSlot, "%rax");
        _asm.mov("%rax", "24(%rcx)");
        // data 指针
        for (size_t i = 0; i < n; ++i)        {
            emitExpression(elements[i]);
            if (hasFailed()) return;
            // P13-1（D9 规则 C）：把借用塞进容器 = 存储借用，必须报错。
            // `var r: list[str] = [param]` 与 `r += [param]` 同样危险：
            // 容器活得比借用源久时，里面留下的是悬空指针。
            if (const auto eid = std::dynamic_pointer_cast<ast::IdentifierNode>(elements[i]))            {
                if (!checkBorrowNotEscaping(eid->getName(), elements[i]->getPos(),
                                            "stored into a container"))                {
                    return;
                }
                if (!markVarMoved(eid->getName(), elements[i]->getPos(),
                                  "stored into a container")) return;
            }
            _asm.mov(dataSlot, "%rcx");
            const std::string dest = std::to_string(i * 8) + "(%rcx)";
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
        // ---------- 字面量重复键去重（P13 缺陷修复，2026-11-17） ----------
        //
        // `{"a": 1, "a": 2}` 原先会把两条都写进 keys/vals 数组，得到
        // 长度 2、并且 `d["a"]` 命中**先出现**的 1。字典语义要求
        // 「后写的覆盖先写的」，结果应是长度 1、值 2（Python/JS/C++/Java 一致）。
        //
        // 只对**编译期可判定的字面量键**去重（str / int / char / bool / null）：
        //   * str 键按内容比（与运行时查找一致，字面量相同内容指向不同常量池项）
        //   * 标量键按值比
        // 键是变量或表达式时保持原样 —— 那种情况要看运行期，属于 P13 之后的
        // 「字典插入语义」范畴，不是字面量构造能解决的。
        //
        // 保留**后出现**的那条（覆盖），并保持首次出现的位置，这样
        // len 与遍历顺序都符合直觉。
        {
            std::vector<std::pair<std::shared_ptr<ast::ExpressionNode>,
                                  std::shared_ptr<ast::ExpressionNode>>> unique;
            for (const auto& kv : pairs)            {
                const std::string key = dictLiteralKeyText(kv.first);
                bool replaced = false;
                if (!key.empty())                {
                    for (auto& u : unique)                    {
                        if (dictLiteralKeyText(u.first) == key)                        {
                            // 后出现的覆盖值，位置不变
                            u.second = kv.second;
                            replaced = true;
                            break;
                        }
                    }
                }
                if (!replaced) unique.push_back(kv);
            }
            pairs.swap(unique);
        }
        const size_t n = pairs.size();
        // ABI v2：DictHeader { len@0, cap@8, ptr keys@16, ptr vals@24 }
        // keys / vals 本来就是独立数组，这里只是把 rc 去掉、偏移前移
        // （见 docs/p13_plan.md D12）。
        const std::string headerSlot = allocateTemp(inferDictLiteralType(node));
        emitAlloc(kDictHeaderSize);
        _asm.mov("%rax", headerSlot);
        _asm.mov(headerSlot, "%rcx");
        _asm.movImm(static_cast<long long>(n), "%rax");
        _asm.mov("%rax", "0(%rcx)");
        // len
        _asm.mov("%rax", "8(%rcx)");
        // cap
        if (n == 0)        {
            // 空字典：keys/vals 都是 null，长度 0
            _asm.movImm(0, "%rax");
            _asm.mov("%rax", "16(%rcx)");
            _asm.mov("%rax", "24(%rcx)");
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
        _asm.mov("%rax", "16(%rcx)");
        // header.keys
        _asm.mov(valsSlot, "%rax");
        _asm.mov("%rax", "24(%rcx)");
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
            // ABI v2：data 是 header+24 处的独立指针
            _asm.inst("movq", "24(%rcx)", "%rcx");
            _asm.mov(valSlot, "%rdx");
            _asm.inst("movq", "%rdx", "(%rcx,%rax,8)");
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
            // 同 emitIndexExpression：键的求值类型优先看**键表达式自身**，
            // 裸 `dict` 的标注里没有键类型信息。
            const auto keyExprType0 = inferType(key);
            const bool keyIsStr = (keyExprType0 && keyExprType0->getKind() == sema::TypeKind::TK_STR) ||
                                  (keyType && keyType->getKind() == sema::TypeKind::TK_STR);
            const std::string contSlot = allocateTemp(contType);
            emitExpression(cont);
            if (hasFailed()) return true;
            _asm.mov("%rax", contSlot);
            // 同 emitIndexExpression：float 键在 %xmm0，不能直接读 %rax。
            const auto keyExprType = inferType(key);
            const bool keyIsFloat = keyExprType && keyExprType->isFloat();
            const std::string keySlot = allocateTemp(keyIsStr ? sema::getStrType() : sema::getIntType());
            emitExpression(key);
            if (hasFailed()) return true;
            if (keyIsFloat)            {
                _asm.inst("movq", "%xmm0", "%rax");
            }
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
            _asm.inst("movq", "16(%rcx)", "%rdx");
            // keys[i]（ABI v2：keys 指针在 header+16）
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
                _asm.lea("16(%rdx)", "%rcx");
                _asm.lea("16(%r8)", "%rdx");
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
            _asm.inst("movq", "24(%rcx)", "%rdx");
            // vals 指针在 header+24（ABI v2）
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
            _asm.inst("movq", "16(%rcx)", "%rdx");
            // keys 指针在 header+16（ABI v2）
            _asm.inst("movq", "(%rdx,%rax,8)", "%rdx");
            _asm.mov(newKeysSlot, "%r8");
            _asm.inst("movq", "%rdx", "(%r8,%rax,8)");
            _asm.mov(contSlot, "%rcx");
            _asm.inst("movq", "24(%rcx)", "%rdx");
            // vals 指针在 header+24（ABI v2）
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
            _asm.inst("movq", "%rax", "16(%rcx)");
            _asm.mov(newValsSlot, "%rax");
            _asm.inst("movq", "%rax", "24(%rcx)");
            // 追加到下标 = 旧 len
            _asm.mov(iSlot, "%rax");
            _asm.mov(contSlot, "%rcx");
            _asm.inst("movq", "16(%rcx)", "%rdx");
            _asm.mov(keySlot, "%r8");
            _asm.inst("movq", "%r8", "(%rdx,%rax,8)");
            _asm.inst("movq", "24(%rcx)", "%rdx");
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
    // 读容器长度头到 dstSlot，null 视为 0。
    // 见头文件注释：null 指针裸解引用偏移 0 会段错误。
    void CodeGen::emitContainerLenTo(const std::string& containerSlot,
                                     const std::string& dstSlot)    {
        const std::string nullLabel = _asm.newLabel("len_null");
        _asm.mov(containerSlot, "%rax");
        _asm.movImm(0, "%rcx");
        _asm.cmp("%rcx", "%rax");
        _asm.je(nullLabel);
        _asm.inst("movq", "(%rax)", "%rcx");
        _asm.label(nullLabel);
        _asm.mov("%rcx", dstSlot);
    }
    void CodeGen::emitStrConcat(const std::string& lhsSlot, const std::string& rhsSlot)    {
        // 结果：新 str = { len_a+len_b, cap, 字节A, 字节B }
        // ABI v2：len@0, cap@8, 内容@16（见头文件 kStrData）。
        // 两侧都可能是 null（空串），长度按 0 算；null+16 参与 mem_copy 时
        // n=0，core_mem_copy 直接返回，不会解引用。
        const std::string laSlot = allocateTemp(sema::getIntType());
        const std::string lbSlot = allocateTemp(sema::getIntType());
        emitContainerLenTo(lhsSlot, laSlot);
        emitContainerLenTo(rhsSlot, lbSlot);
        const std::string bufSlot = allocateTemp(sema::getStrType());
        _asm.mov(laSlot, "%rcx");
        _asm.mov(lbSlot, "%rax");
        _asm.inst("addq", "%rax", "%rcx");
        // total = la + lb
        _asm.inst("addq", "$16", "%rcx");
        // + StrHeader（len + cap）
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
        // cap：拼接结果是**堆上新建**的，可释放（cap = 总字节数，非 0）
        _asm.mov("%rcx", "8(%rax)");
        // mem_copy(buf+16, a+16, la)
        _asm.mov("%rax", "%rcx");
        _asm.inst("addq", "$16", "%rcx");
        _asm.mov(lhsSlot, "%rdx");
        _asm.inst("addq", "$16", "%rdx");
        _asm.mov(laSlot, "%r8");
        _asm.subRsp(32);
        _asm.call("core_mem_copy");
        _asm.addRsp(32);
        // mem_copy(buf+16+la, b+16, lb)
        _asm.mov(bufSlot, "%rcx");
        _asm.inst("addq", "$16", "%rcx");
        _asm.mov(laSlot, "%rax");
        _asm.inst("addq", "%rax", "%rcx");
        _asm.mov(rhsSlot, "%rdx");
        _asm.inst("addq", "$16", "%rdx");
        _asm.mov(lbSlot, "%r8");
        _asm.subRsp(32);
        _asm.call("core_mem_copy");
        _asm.addRsp(32);
        _asm.mov(bufSlot, "%rax");
    }
    void CodeGen::emitListConcat(const std::string& lhsSlot, const std::string& rhsSlot)    {
        // ABI v2：ListHeader { len@0, cap@8, elem_size@16, ptr data@24 }
        // header 与 data 分开分配；data 块大小 = total*8（元素槽恒 8 字节）。
        // 两侧都可能是 null（空列表），长度按 0 算，见 emitContainerLenTo。
        const std::string laSlot = allocateTemp(sema::getIntType());
        const std::string lbSlot = allocateTemp(sema::getIntType());
        emitContainerLenTo(lhsSlot, laSlot);
        emitContainerLenTo(rhsSlot, lbSlot);
        const std::string headerSlot2 = allocateTemp(sema::getPtrType());
        const std::string dataSlot = allocateTemp(sema::getPtrType());
        // 1) header
        emitAlloc(kListHeaderSize);
        _asm.mov("%rax", headerSlot2);
        // 2) data 块（total*8；total==0 时不分配，留 null）
        _asm.mov(laSlot, "%rcx");
        _asm.mov(lbSlot, "%rax");
        _asm.inst("addq", "%rax", "%rcx");
        const std::string zeroData = _asm.newLabel("lcat_nodata");
        const std::string dataDone = _asm.newLabel("lcat_datadone");
        _asm.inst("testq", "%rcx", "%rcx");
        _asm.je(zeroData);
        _asm.inst("leaq", "(,%rcx,8)", "%rcx");
        _asm.movImm(8, "%rdx");
        _asm.subRsp(32);
        _asm.call("core_alloc");
        _asm.addRsp(32);
        _asm.jmp(dataDone);
        _asm.label(zeroData);
        _asm.movImm(0, "%rax");
        _asm.label(dataDone);
        _asm.mov("%rax", dataSlot);
        // 3) 写 header：len / cap / elem_size / data
        _asm.mov(headerSlot2, "%rcx");
        _asm.mov(laSlot, "%rax");
        _asm.mov(lbSlot, "%rdx");
        _asm.inst("addq", "%rdx", "%rax");
        _asm.mov("%rax", "(%rcx)");
        // len = la + lb
        _asm.mov("%rax", "8(%rcx)");
        // cap = len
        _asm.movImm(8, "%rax");
        _asm.mov("%rax", "16(%rcx)");
        // elem_size
        _asm.mov(dataSlot, "%rax");
        _asm.mov("%rax", "24(%rcx)");
        // data 指针
        // 4) mem_copy(data, a.data, la*8)
        //    注意：la==0 时 a 可能是 null（空列表）或 data==null（空 data 块），
        //    此时**不能**解引用。core_mem_copy 对 n=0 会直接返回，但读
        //    `24(%rdx)` 这一步本身就会崩（null+24）。
        const std::string skipA = _asm.newLabel("lcat_skip_a");
        const std::string afterA = _asm.newLabel("lcat_after_a");
        _asm.mov(laSlot, "%rax");
        _asm.inst("testq", "%rax", "%rax");
        _asm.je(skipA);
        _asm.mov(dataSlot, "%rcx");
        _asm.mov(lhsSlot, "%rdx");
        _asm.inst("movq", "24(%rdx)", "%rdx");
        _asm.mov(laSlot, "%r8");
        _asm.inst("shlq", "$3", "%r8");
        _asm.subRsp(32);
        _asm.call("core_mem_copy");
        _asm.addRsp(32);
        _asm.label(skipA);
        _asm.label(afterA);
        // 5) mem_copy(data + la*8, b.data, lb*8)
        const std::string skipB = _asm.newLabel("lcat_skip_b");
        _asm.mov(lbSlot, "%rax");
        _asm.inst("testq", "%rax", "%rax");
        _asm.je(skipB);
        _asm.mov(dataSlot, "%rcx");
        _asm.mov(laSlot, "%rax");
        _asm.inst("leaq", "(%rcx,%rax,8)", "%rcx");
        _asm.mov(rhsSlot, "%rdx");
        _asm.inst("movq", "24(%rdx)", "%rdx");
        _asm.mov(lbSlot, "%r8");
        _asm.inst("shlq", "$3", "%r8");
        _asm.subRsp(32);
        _asm.call("core_mem_copy");
        _asm.addRsp(32);
        _asm.label(skipB);
        _asm.mov(headerSlot2, "%rax");
    }
    void CodeGen::emitStrEq(const std::string& lhsSlot, const std::string& rhsSlot)    {        // 先比长度头，再比内容字节。两段都可能为 null（空串/未初始化），
        // core_mem_eq 对 n=0 直接返回 true，长度不等时提前退出即可。
        const std::string laSlot = allocateTemp(sema::getIntType());
        const std::string lbSlot = allocateTemp(sema::getIntType());
        emitContainerLenTo(lhsSlot, laSlot);
        emitContainerLenTo(rhsSlot, lbSlot);
        const std::string neLabel = _asm.newLabel("streq_ne");
        const std::string doneLabel = _asm.newLabel("streq_done");
        _asm.mov(laSlot, "%rcx");
        _asm.mov(lbSlot, "%rax");
        _asm.cmp("%rax", "%rcx");
        _asm.jne(neLabel);
        // 长度相等：比较内容（从 +8 起）
        _asm.mov(lhsSlot, "%rcx");
        _asm.inst("addq", "$16", "%rcx");
        _asm.mov(rhsSlot, "%rdx");
        _asm.inst("addq", "$16", "%rdx");
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
    // str 的**大小**比较（< <= > >=），按字典序。
    //
    // 之前这四个运算符没有 str 特例，直接落到 cmpq 比两个指针 —— 结果取决于
    // 字面量在堆上的相对地址，与内容无关。表现是泛型 `fun lt[T](a: T, b: T)`
    // 单态化成 str 之后 `lt("bb","aa")` 返回 true（N2/N7/N8 实测）。
    // `==` / `!=` 早在 G9 就按内容比了（emitStrEq），但顺序比较一直漏着。
    //
    // 语义：逐字节比共同前缀（按**无符号**字节，与 ASCII/UTF-8 一致），
    // 首个不同字节决定大小；前缀全同则短的小（"ab" < "abc"）。
    // 结果按 op 写入 %rax（0/1）。
    void CodeGen::emitStrOrder(const std::string& lhsSlot, const std::string& rhsSlot,
                               int op)    {
        // 长度头在 +0，内容从 +8 起
        const std::string laSlot = allocateTemp(sema::getIntType());
        const std::string lbSlot = allocateTemp(sema::getIntType());
        emitContainerLenTo(lhsSlot, laSlot);
        emitContainerLenTo(rhsSlot, lbSlot);
        // 取共同前缀长度 min(la, lb)
        const std::string minSlot = allocateTemp(sema::getIntType());
        _asm.mov(laSlot, "%rax");
        _asm.mov(lbSlot, "%rcx");
        _asm.cmp("%rcx", "%rax");
        const std::string useA = _asm.newLabel("strord_use_a");
        const std::string minDone = _asm.newLabel("strord_min_done");
        _asm.inst("jle", useA);
        _asm.mov("%rcx", "%rax");
        _asm.label(useA);
        _asm.mov("%rax", minSlot);
        _asm.label(minDone);
        // 逐字节比较：%r8 = 循环计数，%r9 = lhs 内容基址，%r10 = rhs 内容基址
        const std::string loopLabel = _asm.newLabel("strord_loop");
        const std::string diffLabel = _asm.newLabel("strord_diff");
        const std::string prefixEq = _asm.newLabel("strord_prefix_eq");
        _asm.mov(lhsSlot, "%r9");
        _asm.inst("addq", "$16", "%r9");
        _asm.mov(rhsSlot, "%r10");
        _asm.inst("addq", "$16", "%r10");
        _asm.movImm(0, "%r8");
        _asm.label(loopLabel);
        _asm.mov(minSlot, "%rax");
        _asm.cmp("%rax", "%r8");
        _asm.inst("jge", prefixEq);
        // 取两个字节（无符号）
        _asm.inst("movzbq", "(%r9,%r8,1)", "%rax");
        _asm.inst("movzbq", "(%r10,%r8,1)", "%rcx");
        _asm.cmp("%rcx", "%rax");
        _asm.jne(diffLabel);
        _asm.inst("addq", "$1", "%r8");
        _asm.jmp(loopLabel);
        _asm.label(diffLabel);
        // %rax = lhs 字节，%rcx = rhs 字节，两者不等
        // 用 setb / seta 直接由标志位得出「lhs < rhs」/「lhs > rhs」
        {
            const char* cc = "b";   // 默认「lhs < rhs」
            switch (static_cast<core::TokenType>(op))            {
            case core::TokenType::TOKEN_LESS:          cc = "b";  break;
            case core::TokenType::TOKEN_LESS_EQUAL:    cc = "be"; break;
            case core::TokenType::TOKEN_GREATER:       cc = "a";  break;
            case core::TokenType::TOKEN_GREATER_EQUAL: cc = "ae"; break;
            default: break;
            }
            _asm.inst(std::string("set") + cc, "%al");
            _asm.inst("movzbq", "%al", "%rax");
        }
        const std::string doneLabel = _asm.newLabel("strord_done");
        _asm.jmp(doneLabel);
        // 共同前缀全同：按长度定大小
        _asm.label(prefixEq);
        _asm.mov(laSlot, "%rcx");
        _asm.mov(lbSlot, "%rax");
        _asm.cmp("%rax", "%rcx");   // 比较 la ? lb
        {
            const char* cc = "l";
            switch (static_cast<core::TokenType>(op))            {
            case core::TokenType::TOKEN_LESS:          cc = "l";  break;
            case core::TokenType::TOKEN_LESS_EQUAL:    cc = "le"; break;
            case core::TokenType::TOKEN_GREATER:       cc = "g";  break;
            case core::TokenType::TOKEN_GREATER_EQUAL: cc = "ge"; break;
            default: break;
            }
            _asm.inst(std::string("set") + cc, "%al");
            _asm.inst("movzbq", "%al", "%rax");
        }
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
            // ABI v2：data 是 header+24 处的独立指针
            _asm.inst("movq", "24(%rcx)", "%rcx");
            _asm.inst("movq", "(%rcx,%rax,8)", "%rax");
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
            _asm.inst("movzbq", "16(%rcx,%rax)", "%rax");
            _asm.jmp(doneLabel);
            _asm.label(missLabel);
            _asm.movImm(0, "%rax");
            _asm.label(doneLabel);
            return;
        }
        if (targetType && targetType->getKind() == sema::TypeKind::TK_DICT)        {
            const auto dictType = std::static_pointer_cast<sema::DictType>(targetType);
            const auto keyType = dictType->getKeyType();
            // 键的求值类型：**优先看键表达式自身**，而不是 dict 标注的键类型。
            //
            // 为什么：裸 `dict` 的键类型是 nullptr（「没写」），如果只看标注，
            // `var d: dict = {"a": 1}` 的 `d["a"]` 会被当成按指针比较、
            // 查不到键（实测 d["a"] 返回 0）。而键表达式是字符串字面量，
            // 它自己的类型是 str，这就够判断「该按内容比」了。
            // 标注了键类型时两者一致；标注更宽时以表达式为准才是对的。
            const auto keyExprType = inferType(key);
            const bool keyIsStr = (keyExprType && keyExprType->getKind() == sema::TypeKind::TK_STR) ||
                                  (keyType && keyType->getKind() == sema::TypeKind::TK_STR);
            // 键的类型决定怎么求值：float 键在 %xmm0，其余在 %rax。
            // 原先无条件读 %rax，于是 `d[1.5]` 这种 float **字面量**键
            // 拿到的是 %rax 里的垃圾（实测命中失败、返回 0），
            // 而 `d[k]`（k 是 float 变量）碰巧能用 —— 变量路径会把位模式
            // 同时放进 %rax，字面量路径只写 %xmm0。
            const bool keyIsFloat = keyExprType && keyExprType->isFloat();
            const std::string keySlot = allocateTemp(keyIsStr ? sema::getStrType() : sema::getIntType());
            const std::string iSlot = allocateTemp(sema::getIntType());
            emitExpression(key);
            if (hasFailed()) return;
            if (keyIsFloat)            {
                _asm.inst("movq", "%xmm0", "%rax");
            }
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
            _asm.inst("movq", "16(%rcx)", "%rdx");
            // keys 数组基址（ABI v2：header+16）
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
                _asm.lea("16(%rdx)", "%rcx");
                _asm.lea("16(%r8)", "%rdx");
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
            _asm.inst("movq", "24(%rcx)", "%rdx");
            // ABI v2：vals 指针在 header+24
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
    size_t CodeGen::argSlotOffset(const size_t pos) const    {
        // 调用区布局（相对调用点 %rsp）：
        //   [0, 32)          shadow space，第 pos 个寄存器参数暂存在 pos*8
        //   [32, 32+8*n)     第 n 个溢出参数的栈槽（n = pos - 4）
        const size_t registerSlots = _abi.getIntArgRegs().size();
        return pos < registerSlots
                   ? pos * 8
                   : _abi.getShadowSpaceSize() + (pos - registerSlots) * 8;
    }
    size_t CodeGen::emitArgsToRegisters(const std::vector<std::shared_ptr<ast::ExpressionNode>>& args,
                                        const std::string& calleeDesc,
                                        const std::vector<sema::TypePtr>& paramTypes,
                                        const size_t argStart,
                                        const std::vector<bool>* annotated)    {
        const auto& intRegs = _abi.getIntArgRegs();
        const auto& floatRegs = _abi.getFloatArgRegs();
        const size_t registerSlots = intRegs.size() < floatRegs.size() ? intRegs.size() : floatRegs.size();
        const size_t shadow = _abi.getShadowSpaceSize();
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
        // MS ABI 的寄存器分配是**按整体参数位置**的（gcc 15.2.0 实测）：
        // 第 pos 个参数用 intRegs[pos] 或 floatRegs[pos]，两类共用同一个
        // 位置下标；pos >= 4 的一律进栈，栈槽序号 = pos - 4。
        // 所以这里不能再分别数「第几个整型」「第几个浮点」。
        std::vector<bool> isFloat(args.size(), false);
        std::vector<std::string> targetReg(args.size()); // 栈参数留空
        std::vector<size_t> targetSlot(args.size());     // 寄存器参数用 kNoSlot
        const size_t kNoSlot = static_cast<size_t>(-1);
        size_t overflow = 0;
        for (size_t i = 0; i < args.size(); ++i)        {
            isFloat[i] = inferType(args[i])->isFloat();
            const size_t pos = argStart + i;
            if (pos < registerSlots)            {
                targetReg[i] = targetIsFloat[i] ? floatRegs[pos] : intRegs[pos];
                targetSlot[i] = kNoSlot;
            }
 else            {
                targetSlot[i] = pos - registerSlots;
                ++overflow;
            }
        }
        // P9：形参类型已知时校验实参 —— 这是「类型错误诊断」最要紧的一处，
        // 因为 `f("hello")` 传给 `x: int` 之前会静默把指针当整数用。
        for (size_t i = 0; i < args.size() && i < paramTypes.size(); ++i)        {
            if (!paramTypes[i] || paramTypes[i]->getKind() == sema::TypeKind::TK_UNKNOWN) continue;
            // 未标注的形参类型是「回退 int」，不是用户的承诺，不能拿来判错。
            // `fun apply(f, x)` 的 f 会被传闭包，按 int 校验就误报了。
            if (annotated && i < annotated->size() && !(*annotated)[i]) continue;
            // 空容器字面量（`[]` / `{}`）不携带元素类型信息：
            // inferListLiteralType 会把空列表兜底成 list[int]，那**不是**用户的
            // 承诺。`f([])` 传给 `xs: list[str]` 必须放行，否则
            // `var r: list[str] = []` 这类标准库写法全部报错（实测 str.rio:346）。
            if (isEmptyContainerLiteral(args[i])) continue;
            const auto why = checkAssignable(paramTypes[i], inferType(args[i]));
            if (!why.empty())            {
                failAt(args[i] ? args[i]->getPos() : utils::getUnknownPos(),
                       "Argument " + std::to_string(i + 1) + " of '" + calleeDesc + "': " + why);
                return 0;
            }
        }
        // 调用区 = shadow space + 溢出栈槽，取整到 16 的倍数保持调用点对齐。
        // 必须在**求值实参之前**预留：实参表达式里可能含调用，那些调用会
        // 从更低的 %rsp 起用自己的调用区，不会踩到这里。
        const size_t callArea = ((shadow + overflow * 8) + 15) / 16 * 16;
        _asm.subRsp(callArea);
        // 求值顺序从左到右。寄存器参数先落到 shadow space 里的对应格子暂存
        // （位置 pos 用 pos*8，正好占满 32 字节 shadow space），
        // 溢出参数直接写自己的栈槽（shadow + (pos-4)*8）。
        // 全部求值完再统一装寄存器 —— 这样后面实参的求值不会冲掉前面
        // 装好的 %rcx/%xmm0。
        for (size_t i = 0; i < args.size(); ++i)        {
            emitExpression(args[i]);
            if (hasFailed()) return 0;
            // 实参类型与形参类型不一致时按形参转换。
            if (isFloat[i] && !targetIsFloat[i])            {
                _asm.inst("cvttsd2siq", "%xmm0", "%rax");
            }
 else if (!isFloat[i] && targetIsFloat[i])            {
                _asm.inst("cvtsi2sdq", "%rax", "%xmm0");
            }
            const std::string addr = std::to_string(argSlotOffset(argStart + i)) + "(%rsp)";
            if (targetIsFloat[i])            {
                _asm.inst("movsd", "%xmm0", addr);
            }
 else            {
                _asm.mov("%rax", addr);
            }
            // P13-1（D10）：实参是**新值**（堆上新分配的：拼接、容器字面量、
            // 返回堆对象的调用……）时，它的出生地就是当前语句的匿名 local，
            // 销毁点是语句末。调用返回后没有任何变量接住它，不登记就是纯泄漏
            // （实测 `take("a"+"b")` 早先 alloc=1 / free=0）。
            //
            // 为什么要额外占一个专属槽，而不能直接登记 shadow space 那几条
            // 实参格子：那块空间是**调用约定借用**的，调用一返回就不再属于
            // 我们；而被调方还可能往里写东西。专属槽是自己的栈空间，调用
            // 前后都有效，语句末销毁时读到的仍是那个指针。
            //
            // 借用实参（形参、已有变量的元素）不是新值，不登记 —— 释放它
            // 等于释放调用方的内存。
            if (isFreshValue(args[i]))            {
                const auto at = inferType(args[i]);
                const std::string keep = allocateTemp(at);
                if (targetIsFloat[i]) _asm.inst("movsd", "%xmm0", keep);
                else                  _asm.mov("%rax", keep);
                _stmtTemps.push_back({keep, at});
            }
            // P13-2/L3（决议 §6.3）：Content[T] 形参接受 Body 实参时隐式 take，
            // 落定即生效（源→E）。移动状态表（_moveStates）已覆盖 ClassType
            // Body 变量，标记在这里才真正有效。
            if (i < paramTypes.size() && paramTypes[i] &&
                paramTypes[i]->getKind() == sema::TypeKind::TK_MEMORY)            {
                const auto mt = std::dynamic_pointer_cast<sema::MemoryType>(paramTypes[i]);
                if (mt && (mt->getRole() == sema::MemoryRole::CONTENT ||
                           mt->getRole() == sema::MemoryRole::TAKE))                {
                    if (const auto id = std::dynamic_pointer_cast<ast::IdentifierNode>(args[i]))                    {
                        const auto srcSlot = lookupVar(id->getName());
                        if (srcSlot && srcSlot->type &&
                            (srcSlot->type->getKind() == sema::TypeKind::TK_CLASS ||
                             (srcSlot->type->getKind() == sema::TypeKind::TK_MEMORY &&
                              std::dynamic_pointer_cast<sema::MemoryType>(srcSlot->type)->getRole() ==
                                  sema::MemoryRole::BODY)))                        {
                                settlePendingTake(id->getName()); // 隐式 take→落定 E
                        }
                    }
                    // P13-2/L3（决议 §6.2 位置 4）：实参是 `bob.take()` 时，
                    // Content 形参位置是落点，接收者 bob→E。
                    if (const std::string taken = takeReceiverName(args[i]); !taken.empty())                {
                        settlePendingTake(taken); // 显式 take()→落定 E
                    }
                }
            }
        }
        for (size_t i = 0; i < args.size(); ++i)        {
            if (targetSlot[i] != kNoSlot) continue;
            const std::string addr = std::to_string(argSlotOffset(argStart + i)) + "(%rsp)";
            if (targetIsFloat[i])            {
                _asm.inst("movsd", addr, targetReg[i]);
            }
 else            {
                _asm.mov(addr, targetReg[i]);
            }
        }
        return callArea;
    }
    void CodeGen::emitIndirectCall(const std::shared_ptr<ast::ExpressionNode>& callee,
                                   const std::vector<std::shared_ptr<ast::ExpressionNode>>& args,
                                   const std::vector<sema::TypePtr>& paramTypes)    {
        // 函数值的表示：指向 16 字节闭包对象的指针
        //   +0  fnptr（代码地址）
        //   +8  env（捕获环境；P4 阶段无捕获，恒为 0）
        // 调用约定：env 走 %r10，目标地址走 %r11，其余与普通调用一致。
        //
        // 顺序很关键：先把闭包指针存进栈槽保住，再布置实参（布置实参要用
        // 参数寄存器，任何一次求值都可能把它们冲掉），最后取回闭包指针。
        // 不能用 push 暂存：push 会让 %rsp 偏离 16 字节对齐，调用点就错了。
        emitExpression(callee);
        if (hasFailed()) return;
        const std::string closureSlot = allocateTemp(sema::getPtrType());
        _asm.mov("%rax", closureSlot);
        const size_t callArea = emitArgsToRegisters(args, nodeText(callee), paramTypes);
        if (hasFailed()) return;
        // 取回闭包指针，再拆出 env 与 fnptr。
        // 必须先读 +8 再读 +0，否则 %rax 被覆盖后就没法再取 env。
        _asm.mov(closureSlot, "%rax");
        _asm.mov("8(%rax)", "%r10");
        _asm.mov("(%rax)", "%r11");
        _asm.inst("call", "*%r11");
        _asm.addRsp(callArea);
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
                // P13-2/L3-A（决议 §1.1）：Pool 标准库类型的静态方法
                // Pool.fixed(n) / Pool.boxed() / Pool.chunked(block: n)。
                // A 阶段先返回最小 Pool 句柄（分配 8 字节头），真实分配器留待 B 阶段。
                if (const auto ownerId = std::dynamic_pointer_cast<ast::IdentifierNode>(member->getLeftNode());
                    ownerId && ownerId->getName() == "Pool")            {
                    if (const auto fnId = std::dynamic_pointer_cast<ast::IdentifierNode>(member->getRightNode()))                    {
                        const auto& m = fnId->getName();
                        if (m == "fixed" || m == "boxed" || m == "chunked")                        {
                            // 求值实参（fixed 容量 / chunked block）后丢弃，A 阶段不存容量。
                            // 命名参数 `block: 64` 在 AST 里是 InfixExpressionNode(op=COLON)，
                            // 必须只求值右侧 value，不能当变量读左侧名字。
                            for (const auto& a : args)                            {
                                if (const auto inf = std::dynamic_pointer_cast<ast::InfixExpressionNode>(a);
                                    inf && inf->getOpToken().getType() == core::TokenType::TOKEN_COLON)
                                    emitExpression(inf->getRightNode());
                                else
                                    emitExpression(a);
                            }
                            // 最小句柄：分配 8 字节（core_alloc(8, 8) -> ptr）。
                            _asm.movImm(8, "%rcx");
                            _asm.movImm(8, "%rdx");
                            _asm.subRsp(32);
                            _asm.call("core_alloc");
                            _asm.addRsp(32);
                            return;
                        }
                    }
                }
                // P13-2/L3-B（决议 §1.4）：`pool.new[T](args)` —— 方法名带类型实参。
                // AST: Infix(DOT, pool, IndexExpression(new, [T]))。
                // B 阶段先复用 T 的构造器（pool 句柄暂不参与分配），
                // 真实池内分配待后续语义阶段。
                if (const auto idx = std::dynamic_pointer_cast<ast::IndexExpressionNode>(member->getRightNode()))                {
                    if (const auto mid = std::dynamic_pointer_cast<ast::IdentifierNode>(idx->getLeftNode());
                        mid && mid->getName() == "new")                    {
                        // 决议 §1.4：接收者必须是 Pool 类型的变量。
                        // 修前只认了 `X.new[T]` 的形状，没查 X 是不是 Pool，
                        // 于是 `var n: Player = ...; n.new[Player]("y")` 静默通过。
                        const auto recv = std::dynamic_pointer_cast<ast::IdentifierNode>(member->getLeftNode());
                        if (!recv)                        {
                            failAt(node->getPos(), "L3: pool.new[T] requires a Pool variable receiver");
                            return;
                        }
                        const auto recvSlot = lookupVar(recv->getName());
                        if (!recvSlot || !recvSlot->type ||
                            recvSlot->type->getKind() != sema::TypeKind::TK_POOL)                        {
                            failAt(node->getPos(), "L3: '" + recv->getName() +
                                   "' is not a Pool; pool.new[T] requires a Pool receiver (P13-2 / §1.4)");
                            return;
                        }
                        if (const auto bracket = std::dynamic_pointer_cast<ast::BracketExpressionNode>(idx->getIndexNode()))                        {
                            std::vector<std::shared_ptr<ast::ExpressionNode>> typeArgs;
                            flattenTypeArgs(bracket->getBodyNode(), typeArgs);
                            if (!typeArgs.empty())                            {
                                const auto t = typeFromTypeArgNode(typeArgs[0], _currentGenericParams);
                                if (t && t->getKind() == sema::TypeKind::TK_CLASS)                                {
                                    emitConstructorCall(t->toString(), node, args);
                                    return;
                                }
                                failAt(node->getPos(), "L3: pool.new[T] requires a class type T");
                                return;
                            }
                        }
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
        // 个数校验必须排在摊平**之前**：摊平会把缺省实参补进来，
        // 之后就再也看不出「用户到底少传了几个」。
        checkCallArity(fname, args, node->getPos(), fname);
        if (hasFailed()) return;
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
        const size_t callArea = emitArgsToRegisters(callArgs, fname, paramTypes, 0, annotatedMask);
        if (hasFailed())        {
            return;
        }
        if (!applyWriteEffects(emitName, fname, callArgs, node->getPos())) return;
        // emitArgsToRegisters 已经预留好调用区（shadow space + 溢出栈槽），
        // 且保证 %rsp 16 字节对齐，这里只负责调用与回收。
        _asm.call(emitName);
        _asm.addRsp(callArea);
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
    void CodeGen::checkCallArity(const std::string& fname,
                                 const std::vector<std::shared_ptr<ast::ExpressionNode>>& args,
                                 const utils::Pos& pos,
                                 const std::string& displayName)    {
        const auto sigIt = _functionParams.find(fname);
        if (sigIt == _functionParams.end()) return;   // 不认识的名字：交给别的路径
        const size_t declared = sigIt->second.size();
        // 变长形参不占 paramTypes 的位置，多余实参算被它吃掉。
        const auto varIt = _functionVariadic.find(fname);
        const bool hasPositional = varIt != _functionVariadic.end() && varIt->second.positional;
        const bool hasKeyword = varIt != _functionVariadic.end() && varIt->second.keyword;
        // 命名实参 `name=value` 不算位置实参，个数校验只数位置实参。
        size_t positional = 0;
        for (const auto& a : args)        {
            const auto assign = std::dynamic_pointer_cast<ast::AssignmentNode>(a);
            if (assign && std::dynamic_pointer_cast<ast::IdentifierNode>(assign->getAssignPair().first))
                continue;
            ++positional;
        }
        // 上界：`*args` 能吃掉任意多个；`**kwargs` 只吃命名实参，不吃位置实参。
        if (!hasPositional && positional > declared)        {
            failAt(pos, "Function '" + displayName + "' takes " + std::to_string(declared) +
                        " argument(s) but " + std::to_string(positional) + " were given");
            return;
        }
        // 下界：缺省实参补上之后还少，就是实打实少传。
        // 有 `*args` 也只能兜住 0 个，不能兜住「少传了必填形参」。
        const auto defaultsIt = _functionDefaults.find(fname);
        size_t required = declared;
        if (defaultsIt != _functionDefaults.end())        {
            const auto namesIt = _functionParamNames.find(fname);
            if (namesIt != _functionParamNames.end())            {
                required = 0;
                for (size_t i = 0; i < declared; ++i)                {
                    const bool hasDefault =
                        i < namesIt->second.size() &&
                        defaultsIt->second.find(namesIt->second[i]) != defaultsIt->second.end();
                    if (!hasDefault) ++required;
                }
            }
        }
        // 命名实参可能已经把某些必填形参填上了，只统计「一个都没填」的必填位。
        if (required > positional)        {
            size_t filledByName = 0;
            const auto namesIt = _functionParamNames.find(fname);
            if (namesIt != _functionParamNames.end())            {
                for (const auto& a : args)                {
                    const auto assign = std::dynamic_pointer_cast<ast::AssignmentNode>(a);
                    if (!assign) continue;
                    const auto target = std::dynamic_pointer_cast<ast::IdentifierNode>(assign->getAssignPair().first);
                    if (!target) continue;
                    const auto& names = namesIt->second;
                    for (size_t i = 0; i < names.size(); ++i)                    {
                        if (names[i] != target->getName()) continue;
                        const bool hasDefault =
                            defaultsIt != _functionDefaults.end() &&
                            defaultsIt->second.find(names[i]) != defaultsIt->second.end();
                        if (!hasDefault) ++filledByName;
                        break;
                    }
                }
            }
            if (required > positional + filledByName)            {
                failAt(pos, "Function '" + displayName + "' requires at least " +
                            std::to_string(required) + " argument(s) but " +
                            std::to_string(positional + filledByName) + " were given");
                return;
            }
        }
        (void)hasKeyword;
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
        // P12/G14：模块里的泛型函数 —— `math.max(1, 2)` 的符号在调用点
        // 才单态化出来，直接发裸名 `max` 会链接失败（定义处根本没发射这个名字）。
        // 必须和裸名调用走同一条决议路径。
        if (_genericFunctions.find(fname) != _genericFunctions.end())        {
            // callee 传 nullptr：`mod.fn(...)` 的限定名不携带显式类型实参
            // （`mod.fn[int](...)` 是另一种 AST 形态，另行处理）。
            if (tryEmitGenericCall(fname, nullptr, callNode, args)) return;
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
        const size_t callArea = emitArgsToRegisters(normalizeCallArgs(args, fname), fname, paramTypes);
        if (hasFailed())        {
            return;
        }
        _asm.call(fname);
        _asm.addRsp(callArea);
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
            _asm.lea(label + "+16(%rip)", "%rdx");
            _asm.movImm(static_cast<long long>(len), "%r8");
            _asm.subRsp(32);
            _asm.call("core_io_write");
            _asm.addRsp(32);
            return;
        }
        emitExpression(arg);
        // P13-1（D10）：sout 是**特化发射**的（直接 core_io_write / rt_print_*），
        // 不走 emitArgsToRegisters，所以临时值的登记必须在这里也写一遍。
        // `sout("a" + "b")` 是 str 临时值最常见的来源之一，漏了它这条规则
        // 等于没做（实测 t_temp_stmt 仍是 alloc=1 / free=0）。
        //
        // 顺序：值已在 %rax/%xmm0，先存进专属槽保住，再从槽里装回给打印用。
        if (isFreshValue(arg))        {
            const std::string keep = allocateTemp(type);
            if (type->isFloat()) _asm.inst("movsd", "%xmm0", keep);
            else                 _asm.mov("%rax", keep);
            _stmtTemps.push_back({keep, type});
            if (type->isFloat()) _asm.inst("movsd", keep, "%xmm0");
            else                 _asm.mov(keep, "%rax");
        }
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
            // P13-2/L3：嵌套的类型实参形态 `list[Body[Player]]` 等走这里。
            if (base == "Body" || base == "Content" || base == "Ref" ||
                base == "Mut" || base == "Take")            {
                sema::MemoryRole role =
                    base == "Body" ? sema::MemoryRole::BODY :
                    base == "Content" ? sema::MemoryRole::CONTENT :
                    base == "Ref" ? sema::MemoryRole::REF :
                    base == "Mut" ? sema::MemoryRole::MUT : sema::MemoryRole::TAKE;
                const auto inner = args.empty() ? sema::getUnknownType() : args[0];
                return std::make_shared<sema::MemoryType>(role, inner);
            }
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
    sema::TypePtr CodeGen::returnTypeFromNode(const std::shared_ptr<ast::FunctionDefinitionNode>& node,
                                              const std::vector<std::string>& genericParams) const    {
        if (!node) return sema::getUnknownType();
        for (const auto& label : node->getLabelNodes())        {
            if (!label) continue;
            const auto t = typeFromLabelNode(label, genericParams);
            if (t && t->getKind() != sema::TypeKind::TK_UNKNOWN) return t;
        }
        return sema::getUnknownType();
    }
    sema::TypePtr CodeGen::typeFromLabelNode(const std::shared_ptr<ast::LabelNode>& label,
                                             const std::vector<std::string>& genericParams) const    {        if (!label) return sema::getUnknownType();
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
        // `func[params][ret]`：labelDesNodes[0] = 参数表，[1] = 返回类型
        // （2026-11-18 起 `func` 改名 `func`，旧名不再接受。）
        if (base == "func")        {
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
        // P13-2/L3：五个内存模型特殊类型 Body[T]/Content[T]/Ref[T]/Mut[T]/Take[T]。
        // 决议 §4：裸类名 Player 仍是 Body[Player] 的别名（走 ClassType 路径），
        // 只有显式写 Body[...] 才得到 MemoryType。
        if (base == "Body" || base == "Content" || base == "Ref" ||
            base == "Mut" || base == "Take")        {
            sema::MemoryRole role;
            if      (base == "Body")    role = sema::MemoryRole::BODY;
            else if (base == "Content") role = sema::MemoryRole::CONTENT;
            else if (base == "Ref")     role = sema::MemoryRole::REF;
            else if (base == "Mut")     role = sema::MemoryRole::MUT;
            else                        role = sema::MemoryRole::TAKE;
            const auto inner = args.empty() ? sema::getUnknownType() : args[0];
            return std::make_shared<sema::MemoryType>(role, inner);
        }
        if (base == "list" || base == "series" || base == "array")        {
            // 元素类型**如实**保留：写 `list[any]` 就是「元素是 any」，
            // 不能兜底成 int —— 否则 `list[any]` 与 `list[int]` 会拼出同一个
            // 重载符号 `_R1tLi`，报 "Duplicate definition"（2026-11-17 实测）。
            // 只有真的没写元素类型（`list` 裸名）才是 nullptr，
            // 那条路径走 resolveTypeName，不会到这里。
            const auto elem = args.empty() ? sema::getUnknownType() : args[0];
            return std::make_shared<sema::ListType>(elem ? elem : sema::getUnknownType());
        }
        if (base == "dict" || base == "map")        {
            // 裸 `dict`：**键与值都不约束**（都用 nullptr 表示「没写」）。
            //
            // 键不能钉成 str（P13 修复，2026-11-17）：dict 的键可以是
            // int / float / char（实测全部可用），把裸 `dict` 的键当成 str
            // 会让 `fun f(d: dict)` 拒绝 `dict[int, str]` 实参
            // —— 打分时「形参键 str ≠ 实参键 int」直接判不合格。
            // 至于「字符串键要按内容比较」那件事，由**索引表达式**那边
            // 按实际 dict 的键类型决定（见 emitIndexExpression 的 keyIsStr），
            // 与形参标注无关。
            const auto key = args.size() > 0 ? args[0] : nullptr;
            const auto val = args.size() > 1 ? args[1] : nullptr;
            return std::make_shared<sema::DictType>(key, val);
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
    // ============================================================
    // P13-2 / L0：语义层"去伪"校验
    // 这些写法当前静默接受但语义无效，必须在后端明确报错。
    // ============================================================
    bool CodeGen::validateL0ClassDeclarations(
        const std::vector<std::shared_ptr<ast::ClassDefinitionNode>>& classNodes)    {
        std::unordered_set<std::string> seen;
        for (const auto& node : classNodes)        {
            if (!node) continue;
            // 泛型类 `class Box[T]` 的名字节点是 IndexExpressionNode，
            // 取其基名做检查。
            std::string name;
            utils::Pos namePos;
            bool hasNamePos = false;
            if (const auto index = std::dynamic_pointer_cast<ast::IndexExpressionNode>(node->getNameNode()))            {
                if (const auto id = std::dynamic_pointer_cast<ast::IdentifierNode>(index->getLeftNode()))                {
                    name = id->getName();
                    namePos = id->getMainToken().getPos();
                    hasNamePos = true;
                }
            } else if (const auto id = std::dynamic_pointer_cast<ast::IdentifierNode>(node->getNameNode()))            {
                name = id->getName();
                namePos = id->getMainToken().getPos();
                hasNamePos = true;
            } else if (const auto label = std::dynamic_pointer_cast<ast::LabelNode>(node->getNameNode()))            {
                // 内置类型名（int/str/...）词法上是 TOKEN_LABEL，
                // buildClassExpression 拿到的是 LabelNode 而非 IdentifierNode。
                name = label->getFullLabel();
                namePos = label->getMainToken().getPos();
                hasNamePos = true;
            }
            if (name.empty()) continue;
            const auto pos = hasNamePos ? namePos : node->getPos();
            // L0-6：类名不得与内置类型名 / 预声明类型名 Pool 冲突（Q-A 派生）。
            const bool builtin = base::TYPES.contains(name) || name == "Pool";
            if (builtin)            {
                failAt(pos,
                       "L0-6: class name '" + name +
                       "' conflicts with a built-in / predefined type; rename it");
                continue;
            }
            // L0-7：重复类定义
            if (!seen.insert(name).second)            {
                failAt(pos,
                       "L0-7: duplicate class definition '" + name + "'");
            }
        }
        return hasFailed();
    }
    bool CodeGen::validateL0FieldTypes(
        const std::vector<std::shared_ptr<ast::ClassDefinitionNode>>& classNodes)    {
        for (const auto& node : classNodes)        {
            if (!node) continue;
            const auto body = std::dynamic_pointer_cast<ast::BlockRangerNode>(node->getBodyNode());
            if (!body) continue;
            // 泛型类的形参（`class Box[T]` 里的 T）在字段类型里是合法的「洞」，
            // 不是未知类型。
            //
            // 修前这里直接用了 `_currentGenericParams` —— 那是**实例化发射**阶段
            // 才被赋值的成员，而本校验发生在 registerClass 之前，恒为空。于是
            // 泛型类的字段 `var v: T` 被当成未知类型，报
            // "L0-2: unknown type 'T' of field 'v'"（2026-11-18 实测，
            // 直接导致 test_p10_generics 的 6 条泛型类用例全挂）。
            // 正确做法是按**这个类自己**的形参列表解析。
            std::vector<std::string> classGenerics;
            if (const auto nameIndex =
                    std::dynamic_pointer_cast<ast::IndexExpressionNode>(node->getNameNode()))            {
                if (const auto bracket =
                        std::dynamic_pointer_cast<ast::BracketExpressionNode>(nameIndex->getIndexNode()))                {
                    std::vector<std::shared_ptr<ast::ExpressionNode>> nodes;
                    flattenTypeArgs(bracket->getBodyNode(), nodes);
                    for (const auto& n : nodes)                    {
                        if (!n) continue;
                        if (const auto pid = std::dynamic_pointer_cast<ast::IdentifierNode>(n))
                            classGenerics.push_back(pid->getName());
                        else if (const auto lbl = std::dynamic_pointer_cast<ast::LabelNode>(n))
                            classGenerics.push_back(lbl->getFullLabel());
                    }
                }
            }
            for (const auto& stmt : body->getBodyExpressions())            {
                const auto varDef = std::dynamic_pointer_cast<ast::VariableDefinitionNode>(stmt);
                if (!varDef) continue;
                for (const auto& def : varDef->getVarDefs())                {
                    if (!def || !def->hasLabels()) continue;
                    for (const auto& label : def->getLabelNodes())                    {
                        if (!label) continue;
                        const auto full = label->getFullLabel();
                        if (full.empty()) continue;
                        // `any` 是合法的通配符标注。
                        if (full == "any") continue;
                        // 生命周期标签不是类型。
                        //
                        // `rcc_base.cpp` 的 DESCRIBE_LABELS 把标签分成四类，
                        // 其中 "life circle label" 就是 static / global：
                        //   // life circle label
                        //   "static", "global",
                        // 它们与类型标签一样是 TOKEN_LABEL、一样进标签区，
                        // 但语义是「这个字段/变量的生命周期归属」，不是类型。
                        //
                        // 修前这里把标签区**每一个**标签都当类型解析，于是原仓库
                        // 就有的 `var _number: int static = 0`（tests/test.rio）
                        // 报 "L0-2: unknown type 'static' of field '_number'" ——
                        // P0 重写把 static 字段的解析整个丢了。
                        if (isLifecycleLabel(full)) continue;
                        const auto t = typeFromLabelNode(label, classGenerics);
                        if (!t || t->getKind() == sema::TypeKind::TK_UNKNOWN)                        {
                            failAt(def->getNameNode()->getPos(),
                                   "L0-2: unknown type '" + full + "' of field '" +
                                   def->getNameNode()->getName() + "'");
                            return true;
                        }
                    }
                }
            }
        }
        return hasFailed();
    }
    bool CodeGen::validateL0LabelConflicts(const std::shared_ptr<ast::ProgramNode>& program)    {
        // L0-4：标签区里若出现两个以上**能解析成具体类型**的标签，报错。
        // 决议 §2.3：标签区最多一个类型标签（出生地标签 L1 才引入）。
        // 这里只检查"两个都是已知类型"的硬冲突，避免误伤未来的出生地标签。
        std::vector<std::shared_ptr<ast::ExpressionNode>> worklist;
        for (const auto& stmt : program->getStatements())        {
            if (const auto es = std::dynamic_pointer_cast<ast::ExpressionStatementNode>(stmt))
                worklist.push_back(es->getExpression());
        }
        while (!worklist.empty())        {
            auto node = worklist.back();
            worklist.pop_back();
            if (!node) continue;
            if (const auto block = std::dynamic_pointer_cast<ast::BlockRangerNode>(node))            {
                for (const auto& e : block->getBodyExpressions()) worklist.push_back(e);
                continue;
            }
            if (const auto varDef = std::dynamic_pointer_cast<ast::VariableDefinitionNode>(node))            {
                for (const auto& def : varDef->getVarDefs())                {
                    if (!def || !def->hasLabels()) continue;
                    sema::TypePtr firstType;
                    std::string firstName;
                    for (const auto& label : def->getLabelNodes())                    {
                        if (!label) continue;
                        const auto full = label->getFullLabel();
                        if (full.empty() || full == "any") continue;
                        auto t = typeFromLabelNode(label, _currentGenericParams);
                        if (!t || t->getKind() == sema::TypeKind::TK_UNKNOWN) continue;
                        if (firstType && !t->equals(firstType))                        {
                            failAt(def->getNameNode()->getPos(),
                                   "L0-4: conflicting type labels '" + firstName + "' and '" +
                                   full + "' in declaration of '" + def->getNameNode()->getName() + "'");
                            return true;
                        }
                        firstType = t;
                        firstName = full;
                    }
                }
                continue;
            }
            // 递归进常见的容器节点，覆盖函数体/分支里的 var 定义。
            if (const auto stmt = std::dynamic_pointer_cast<ast::ExpressionStatementNode>(node))            {
                worklist.push_back(stmt->getExpression());
            } else if (const auto fn = std::dynamic_pointer_cast<ast::FunctionDefinitionNode>(node))            {
                worklist.push_back(fn->getBodyNode());
            } else if (const auto cls = std::dynamic_pointer_cast<ast::ClassDefinitionNode>(node))            {
                worklist.push_back(cls->getBodyNode());
            } else if (const auto cond = std::dynamic_pointer_cast<ast::ConditionNode>(node))            {
                for (const auto& b : cond->getBranchNodes()) worklist.push_back(b);
            } else if (const auto branch = std::dynamic_pointer_cast<ast::BranchNode>(node))            {
                worklist.push_back(branch->getBodyNode());
            } else if (const auto loop = std::dynamic_pointer_cast<ast::LoopNode>(node))            {
                worklist.push_back(loop->getBodyNode());
            }
        }
        return hasFailed();
    }
    bool CodeGen::validateL3TakeUsage(const std::shared_ptr<ast::ProgramNode>& program)    {
        // 决议 §7.3：Take[T] 是二等值，只作为函数形参类型出现；
        // `var x: Take[T]` / `-> Take[T]` / 字段 `Take[T]` 均编译错误。
        const auto isTake = [&](const std::shared_ptr<ast::LabelNode>& label) {
            if (!label) return false;
            const auto t = typeFromLabelNode(label, _currentGenericParams);
            return t && t->getKind() == sema::TypeKind::TK_MEMORY &&
                   std::dynamic_pointer_cast<sema::MemoryType>(t)->getRole() == sema::MemoryRole::TAKE;
        };
        const auto checkLabels = [&](const std::vector<std::shared_ptr<ast::LabelNode>>& labels,
                                     const utils::Pos& pos, const std::string& where) {
            for (const auto& label : labels)            {
                if (isTake(label))                {
                    failAt(pos, "L3: Take[T] cannot be used as " + where +
                                "; it is only valid as a function parameter type");
                    return true;
                }
            }
            return false;
        };
        std::vector<std::shared_ptr<ast::ExpressionNode>> worklist;
        for (const auto& stmt : program->getStatements())        {
            if (const auto es = std::dynamic_pointer_cast<ast::ExpressionStatementNode>(stmt))
                worklist.push_back(es->getExpression());
        }
        while (!worklist.empty())        {
            auto node = worklist.back();
            worklist.pop_back();
            if (!node) continue;
            if (const auto varDef = std::dynamic_pointer_cast<ast::VariableDefinitionNode>(node))            {
                for (const auto& def : varDef->getVarDefs())                {
                    if (!def || !def->getNameNode()) continue;
                    if (checkLabels(def->getLabelNodes(), def->getNameNode()->getPos(),
                                    "the type of variable '" + def->getNameNode()->getName() + "'")) return true;
                }
                continue;
            }
            if (const auto fn = std::dynamic_pointer_cast<ast::FunctionDefinitionNode>(node))            {
                if (checkLabels(fn->getLabelNodes(), node->getPos(),
                                "a function return type")) return true;
                worklist.push_back(fn->getBodyNode());
                continue;
            }
            if (const auto cls = std::dynamic_pointer_cast<ast::ClassDefinitionNode>(node))            {
                worklist.push_back(cls->getBodyNode());
                continue;
            }
            if (const auto block = std::dynamic_pointer_cast<ast::BlockRangerNode>(node))            {
                for (const auto& e : block->getBodyExpressions()) worklist.push_back(e);
                continue;
            }
            if (const auto cond = std::dynamic_pointer_cast<ast::ConditionNode>(node))            {
                for (const auto& b : cond->getBranchNodes()) worklist.push_back(b);
                continue;
            }
            if (const auto branch = std::dynamic_pointer_cast<ast::BranchNode>(node))            {
                worklist.push_back(branch->getBodyNode());
                continue;
            }
            if (const auto loop = std::dynamic_pointer_cast<ast::LoopNode>(node))            {
                worklist.push_back(loop->getBodyNode());
                continue;
            }
            if (const auto stmt = std::dynamic_pointer_cast<ast::ExpressionStatementNode>(node))            {
                worklist.push_back(stmt->getExpression());
            }
        }
        return hasFailed();
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
                        // 生命周期标签 `static`：全类共享一份，不进实例布局。
                        // 原仓库 test.rio 的 `var _number: int static = 0` 靠它
                        // 做「构造计数」，语义是类级变量而非每实例字段。
                        const bool isStaticField = [&]()                            {
                            if (!def->hasLabels()) return false;
                            for (const auto& label : def->getLabelNodes())                            {
                                if (!label) continue;
                                if (label->getFullLabel() == "static") return true;
                            }
                            return false;
                        }();
                        if (isStaticField)                        {
                            info.staticFieldTypes[fname] = ftype;
                            info.staticFieldSymbols[fname] = info.name + "." + fname + ".static";
                            // 初始化器交给 emitStaticFieldInitCode 发进 .bss + 启动代码，
                            // **不能**进 ownFieldInits —— 那份是每实例初始化，
                            // 进了就会在每次构造时把共享计数重置回 0。
                            if (def->hasInitialValue() && def->getValueNode())                            {
                                info.staticFieldInits.emplace_back(fname, def->getValueNode());
                            }
                            continue;
                        }
                        info.ownFieldNames.push_back(fname);
                        info.ownFieldTypes[fname] = ftype;
                        // P12/G11：收集实例字段初始化器，对象创建时写入。
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
    std::string CodeGen::findStaticFieldSymbol(const std::string& className,
                                               const std::string& field) const    {
        // 沿基类链找：`static` 字段属于声明它的那个类，子类共用同一份。
        std::string cur = className;
        size_t guard = 0;
        while (!cur.empty() && guard++ < 64)        {
            const auto cls = findClass(cur);
            if (!cls) return std::string();
            const auto it = cls->staticFieldSymbols.find(field);
            if (it != cls->staticFieldSymbols.end()) return it->second;
            cur = cls->baseName;
        }
        return std::string();
    }
    sema::TypePtr CodeGen::findStaticFieldType(const std::string& className,
                                               const std::string& field) const    {
        std::string cur = className;
        size_t guard = 0;
        while (!cur.empty() && guard++ < 64)        {
            const auto cls = findClass(cur);
            if (!cls) return nullptr;
            const auto it = cls->staticFieldTypes.find(field);
            if (it != cls->staticFieldTypes.end()) return it->second;
            cur = cls->baseName;
        }
        return nullptr;
    }
    std::string CodeGen::classNameFromVariable(const std::string& name) const    {
        const auto slot = const_cast<CodeGen*>(this)->lookupVar(name);
        if (!slot || !slot->type) return std::string();
        if (slot->type->getKind() == sema::TypeKind::TK_CLASS)
            return std::static_pointer_cast<sema::ClassType>(slot->type)->getName();
        // Ref/Mut/Take/Body 等 MemoryType：取内嵌类型（决议 §4.1 穿透引用）
        if (slot->type->getKind() == sema::TypeKind::TK_MEMORY)        {
            const auto inner = std::dynamic_pointer_cast<sema::MemoryType>(slot->type)->getInnerType();
            if (inner && inner->getKind() == sema::TypeKind::TK_CLASS)
                return std::static_pointer_cast<sema::ClassType>(inner)->getName();
        }
        return std::string();
    }
    std::string CodeGen::classNameOf(const std::shared_ptr<ast::ExpressionNode>& node) const    {
        if (!node) return {};
        // 注意：FunctionCallNode 是 InfixExpressionNode 的子类，必须排在前面
        if (const auto id = std::dynamic_pointer_cast<ast::IdentifierNode>(node))        {
            // P13-2/L3（决议 §4）：Ref/Mut/Take/Content/Body 形参与变量都要能
            // 拿到**内层类名** —— 它们与 Body 是同一个对象指针，字段访问与方法
            // 调用都作用在内层对象上。
            // 修前这里只认 TK_CLASS，于是 `fun f(t: Take[Player]) { t.clear() }`
            // 报 "Cannot determine the object type of method call"，
            // 连 t.name 这种字段读也失败 —— 导致 ensures[!t] 除了 throw 出口
            // 之外无解（无法通过方法消费来满足 Empty）。
            return classNameFromVariable(id->getName());
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
                // P13-2：字段本身是 Ref/Mut/Content/Body 时同样取内层类名，
                // 这样 `p.owner.name` 这种链式穿透也能解析。
                if (ft && ft->getKind() == sema::TypeKind::TK_MEMORY)                {
                    const auto inner = std::dynamic_pointer_cast<sema::MemoryType>(ft)->getInnerType();
                    if (inner && inner->getKind() == sema::TypeKind::TK_CLASS)
                        return std::static_pointer_cast<sema::ClassType>(inner)->getName();
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
                                            const std::string& ownerClass,
                                            bool* outAnnotated) const    {
        if (outAnnotated) *outAnnotated = false;
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
        if (annotated && annotated->getKind() != sema::TypeKind::TK_UNKNOWN)        {
            // 返回类型来自显式 `: T` 标注 —— 这是用户的承诺，ret 要按它校验。
            if (outAnnotated) *outAnnotated = true;
            return annotated;
        }
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
    void CodeGen::emitStaticFieldSlots()    {
        // 生命周期标签 `static` 的字段：全类共享一份，不在实例布局里。
        //
        // 原仓库 tests/test.rio：
        //     class ID {
        //         var _number: int static = 0, num: int
        //         ctor () { this._number = this._number + 1  this.num = this._number }
        //     }
        // 靠 `_number` 递增给每个实例发号。修前 static 被当普通实例字段，
        // 每个实例各自从 0 开始，于是构造三个 ID 全是 1 1 1。
        //
        // 这里只发 .bss 槽（天然零初始化）。初始化器 `= 0` 这类**非零**初值
        // 在 __rio_main 里按源码顺序执行（见 emitStaticFieldInitCode）。
        for (auto& [cname, info] : _classes)        {
            for (const auto& [fname, sym] : info.staticFieldSymbols)            {
                const auto typeIt = info.staticFieldTypes.find(fname);
                const auto ftype = typeIt != info.staticFieldTypes.end()
                    ? typeIt->second : sema::getIntType();
                const size_t raw = ftype ? ftype->getSize() : 8;
                const size_t slotSize = raw < 8 ? 8 : ((raw + 7) & ~static_cast<size_t>(7));
                _globalDecls.push_back({sym, slotSize});
            }
        }
    }
    void CodeGen::emitStaticFieldInitCode()    {
        // static 字段的初始化器在用户代码之前跑一次。
        //
        // 只处理有非零初值的：.bss 已经把它们清成 0，`= 0` 再写一遍没意义，
        // 而且表达式里可能引用别的东西（顺序/依赖都不确定）。这里保守地
        // 只发**字面量初值**，其它留给用户显式赋值 —— 静默跳过比乱序执行安全。
        for (auto& [cname, info] : _classes)        {
            for (const auto& [fname, initNode] : info.staticFieldInits)            {
                if (!initNode) continue;
                const auto symIt = info.staticFieldSymbols.find(fname);
                if (symIt == info.staticFieldSymbols.end()) continue;
                // 只认整数字面量（含正负号）。浮点/字符串/表达式初值一律跳过：
                // .bss 已是 0，静默跳过比乱序执行安全，用户可显式赋值。
                const std::string text = nodeText(initNode);
                if (text.empty()) continue;
                const char c0 = text[0];
                if (!std::isdigit(static_cast<unsigned char>(c0)) && c0 != '-' && c0 != '+') continue;
                long long intVal = 0;
                try                {
                    intVal = std::stoll(text);
                }
                catch (...)                {
                    continue;
                }
                if (intVal == 0) continue; // .bss 已是 0
                const auto typeIt = info.staticFieldTypes.find(fname);
                const auto ftype = typeIt != info.staticFieldTypes.end()
                    ? typeIt->second : sema::getIntType();
                if (ftype && ftype->isFloat())                {
                    _asm.movImm(intVal, "%rax");
                    _asm.inst("cvtsi2sdq", "%rax", "%xmm0");
                    _asm.inst("movsd", "%xmm0", symIt->second + "(%rip)");
                }
 else                {
                    _asm.movImm(intVal, "%rax");
                    _asm.mov("%rax", symIt->second + "(%rip)");
                }
            }
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
            bool retAnnotated = false;
            const auto ret = methodReturnType(def, paramNames, paramTypes, "", &retAnnotated);
            // P12/G5：重载方法用修饰名发射；单定义保持裸名（与全局函数一致）。
            std::string methodSymbolName = name + "." + mname;
            if (defs.size() >= 2)            {
                methodSymbolName = mangleOverload(name + "." + mname, paramTypes);
            }
            // P13-2/L3-I（决议 §9.2/§9.3/§9.4）：方法上的 writes / from / ensures
            // 子句必须像普通函数一样进校验。
            //
            // 修前这里既不登记 _functionParamNames，也不设 _currentSignatureClauses，
            // 于是三件事同时失效：
            //   ① validateWritesFromClauses 遍历的是空表 → 方法上的 writes 整片跳过，
            //      `fun bad(): int writes[zzz.name]` 静默通过（普通函数会报）；
            //   ② isFunctionParam 查不到形参名（键是符号名，方法从没登记过）→
            //      `writes[o.name]` 里 o 明明是形参也报「not a parameter」；
            //   ③ validateEnsuresClauses 同样是空表 → 方法上的 ensures 不生效。
            // 登记用**发射名**做键，与 _functionParams / _functionParamAnnotated 一致。
            _functionParamNames[methodSymbolName] = paramNames;
            _currentSignatureClauses = def ? def->getSignatureClauses() : std::vector<std::shared_ptr<ast::LabelNode>>{};
            emitFunctionBody(methodSymbolName, paramNames, paramTypes, ret,
                             def->getBodyNode(), true, {}, {}, "", true, retAnnotated);
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
        // 生命周期标签 `static` 的字段不在实例里，读它等于读一个全局槽，
        // 而且**不能**对左操作数求值 —— 那是对象指针，与共享槽无关。
        if (const std::string sym = findStaticFieldSymbol(owner, fieldId->getName());
            !sym.empty())        {
            const auto ftype = findStaticFieldType(owner, fieldId->getName());
            if (ftype && ftype->isFloat())            {
                _asm.inst("movsd", sym + "(%rip)", "%xmm0");
            }
            _asm.mov(sym + "(%rip)", "%rax");
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
        // 生命周期标签 `static` 的字段写的是全局槽，与对象指针无关。
        if (const std::string sym = findStaticFieldSymbol(owner, fieldId->getName());
            !sym.empty())        {
            const auto ftype = findStaticFieldType(owner, fieldId->getName());
            const bool destIsFloat = ftype && ftype->isFloat();
            emitExpression(value);
            if (hasFailed()) return;
            const auto vtype = inferType(value);
            const bool srcIsFloat = vtype && vtype->isFloat();
            if (destIsFloat && !srcIsFloat)            {
                _asm.inst("cvtsi2sdq", "%rax", "%xmm0");
            }
 else if (!destIsFloat && srcIsFloat)            {
                _asm.inst("cvttsd2siq", "%xmm0", "%rax");
            }
            if (destIsFloat)            {
                _asm.inst("movsd", "%xmm0", sym + "(%rip)");
            }
 else            {
                _asm.mov("%rax", sym + "(%rip)");
            }
            return;
        }
        const long long offset = findFieldOffset(owner, fieldId->getName());
        if (offset < 0)        {
            fail("Class " + owner + " has no field named '" + fieldId->getName() + "'");
            return;
        }
        const auto ftype = findFieldType(owner, fieldId->getName());
        const bool destIsFloat = ftype && ftype->isFloat();
        // P13-2/L3-F（决议 §4.1）：字段赋值是「替换 / 重分配」，与指向该字段
        // （或其子路径）的活跃子引用冲突。
        // 修前这里完全不查，于是 `var r = &p.name; p.name = "y"` 静默通过。
        if (const std::string targetPath = exprPathText(target); !targetPath.empty())        {
            if (!checkWriteConflicts(targetPath, target->getPos(), "field assignment to")) return;
        }
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
        // 暂存到栈槽（不能用 push：算对象指针时可能有调用，
        // push 的 8 字节偏移会让那个调用的 %rsp 失去 16 字节对齐）。
        const std::string valueSlot = allocateTemp(destIsFloat ? sema::getFloatType() : sema::getIntType());
        if (destIsFloat)        {
            _asm.inst("movsd", "%xmm0", valueSlot);
        }
        else        {
            _asm.mov("%rax", valueSlot);
        }
        emitExpression(target->getLeftNode());
        if (hasFailed())        {
            return;
        }
        _asm.mov("%rax", "%r10");
        const std::string addr = std::to_string(offset) + "(%r10)";
        if (destIsFloat)        {
            _asm.inst("movsd", valueSlot, "%xmm0");
            _asm.inst("movsd", "%xmm0", addr);
        }
        else        {
            _asm.mov(valueSlot, "%rax");
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
        // 对象指针全程放栈槽：下面的 core_mem_set 调用、字段初始化器求值、
        // 构造器实参求值都会冲掉 caller-saved 寄存器。
        // 不用 push 暂存：push 的 8 字节偏移会让调用点的 %rsp 失去 16 字节对齐。
        const std::string objectSlot = allocateTemp(sema::getPtrType());
        _asm.mov("%rax", objectSlot);
        // 新对象先整体清零：没赋值的字段默认是 0。
        _asm.mov("%rax", "%rcx");
        _asm.movImm(0, "%rdx");
        _asm.movImm(size, "%r8");
        _asm.subRsp(32);
        _asm.call("core_mem_set");
        _asm.addRsp(32);
        _asm.mov(objectSlot, "%rax");
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
        // 对象指针占掉隐藏 this（位置 0），用户实参从位置 1 起排。
        // 不能用 push 暂存 this：push 会让 %rsp 偏离 16 字节对齐。
        const std::string selfArgSlot = allocateTemp(sema::getPtrType());
        _asm.mov("%rax", selfArgSlot);
        const std::vector<bool>* ctorMask = nullptr;
        if (const auto found = _functionParamAnnotated.find(className + ".__init__");
            found != _functionParamAnnotated.end())
            ctorMask = &found->second;
        const size_t ctorArea = emitArgsToRegisters(args, className, paramTypes, 1, ctorMask);
        if (hasFailed())        {
            _currentGenericParams = savedParams;
            _currentGenericBindings = savedBindings;
            return;
        }
        _asm.mov(selfArgSlot, "%rcx");
        _currentGenericParams = savedParams;
        _currentGenericBindings = savedBindings;
        if (!cls->hasCtor)        {
            // 没有构造器：对象就是结果，字段保持全 0。
            // 参数已经求值过了，预留的调用区要还回去。
            _asm.addRsp(ctorArea);
            _asm.mov(selfArgSlot, "%rax");
            return;
        }
        _asm.call(className + ".__init__");
        _asm.addRsp(ctorArea);
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
        // P13-2/L3-E（决议 §4.3）：list 内建 get_mut(i) ——
        // 返回元素槽的 Mut 视图（元素地址），登记 xs[*]+i 路径。
        // receiver 是 list（内置类型，classNameOf 为空），须在 owner 检查前处理。
        if (methodId->getName() == "get_mut")        {
            const auto listVar = std::dynamic_pointer_cast<ast::IdentifierNode>(member->getLeftNode());
            if (!listVar || args.size() != 1)            {
                failAt(member->getPos(), "L3: get_mut(i) requires a list variable and one index");
                return;
            }
            const auto listType = lookupVar(listVar->getName())
                ? lookupVar(listVar->getName())->type : nullptr;
            if (!listType || listType->getKind() != sema::TypeKind::TK_LIST)            {
                failAt(member->getPos(), "L3: get_mut(i) requires a list receiver");
                return;
            }
            // 决议 §4.3：下标文本必须是纯字面量（"0"/"1"）才能证明 i≠j；
            // args[0]->toString() 带节点格式，不能用于不等事实判定。
            const auto indexTextForFact = [](const std::shared_ptr<ast::ExpressionNode>& n) {
                if (const auto lit = std::dynamic_pointer_cast<ast::IntegerLiteralNode>(n))
                    return lit->literalString();
                return std::string(); // 非整数字面量：保守（空串→冲突）
            };
            const auto idxText = indexTextForFact(args[0]);
            // 决议 §4.3：同一 owner 上同一下标的另一个 get_mut 冲突；
            // 不同字面量下标（i≠j）不冲突。
            if (hasActiveRefOn(listVar->getName(), idxText))            {
                failAt(member->getPos(), "L3: conflicting mutable references to '" +
                       listVar->getName() + "' at the same index");
                return;
            }
            // 登记动态路径 xs[*] + 下标文本
            registerActiveRef(listVar->getName() + "[*]", listVar->getName(), "",
                              member->getPos(), idxText);
            // 元素地址：header(rax) -> data(+24) -> data + i*8
            emitExpression(listVar);
            if (hasFailed()) return;
            _asm.inst("movq", "24(%rax)", "%rax");   // data 指针
            emitExpression(args[0]);
            if (hasFailed()) return;
            _asm.inst("movq", "%rax", "%rcx");       // i
            _asm.inst("imulq", "$8, %rcx");
            _asm.inst("addq", "%rcx", "%rax");       // 元素槽地址
            return;
        }
        // P13-2/L3-C（决议 §2.3）：Pool 内建 release(x) ——
        // 只对自定义 Pool 出生地的 Body 可用；x 须 E 态；跨池报错。
        // 必须在 owner 检查之前：receiver 是 Pool 类型变量，classNameOf 为空。
        if (methodId->getName() == "release")        {
            const auto poolVar = std::dynamic_pointer_cast<ast::IdentifierNode>(member->getLeftNode());
            if (!poolVar)            {
                failAt(member->getPos(), "L3: release() requires a Pool receiver");
                return;
            }
            if (args.size() != 1)            {
                failAt(member->getPos(), "L3: release(x) takes exactly one argument");
                return;
            }
            const auto x = std::dynamic_pointer_cast<ast::IdentifierNode>(args[0]);
            if (!x)            {
                failAt(args[0]->getPos(), "L3: release(x) requires a Body variable");
                return;
            }
            if (!checkNotPending(x->getName(), args[0]->getPos(), "released")) return;
            if (hasActiveRefOn(x->getName()))            {
                failAt(args[0]->getPos(), "L3: '" + x->getName() +
                       "' has active sub-references; cannot release");
                return;
            }
            const auto found = _birthPlaces.find(x->getName());
            if (found == _birthPlaces.end() || found->second.empty())            {
                failAt(args[0]->getPos(), "L3: release target '" + x->getName() +
                       "' has no Pool birth place (default-birth Body cannot be released)");
                return;
            }
            if (found->second != poolVar->getName())            {
                failAt(args[0]->getPos(), "L3: cross-pool release: '" + x->getName() +
                       "' was born in '" + found->second + "', not '" + poolVar->getName() + "'");
                return;
            }
            if (isMergedUnknown(x->getName()))            {
                // 决议 §3.4/§10.3：? 状态连「是不是空壳」都不确定，release
                // 会凭空丢掉一块可能还有内容的堆内存，必须拒绝。
                failAt(args[0]->getPos(), "L3: release target '" + x->getName() +
                       "' is in ? state (control flow merged to an unknown body state); "
                       "cannot release");
                return;
            }
            if (getBodyState(x->getName()) != BodyState::EMPTY)            {
                failAt(args[0]->getPos(), "L3: release target '" + x->getName() +
                       "' is not Empty (clear it first)");
                return;
            }
            _birthPlaces.erase(found);
            setBodyState(x->getName(), BodyState::MOVED, args[0]->getPos(), "released");
            return;
        }
        const std::string owner = classNameOf(member->getLeftNode());
        if (owner.empty())        {
            fail("Cannot determine the object type of method call: " + member->toString());
            return;
        }
        // P13-2/L3（决议 §7.1）：Body 内建 ref()/mut() 是 & 的语法糖，
        // 底层与 Body 同为对象指针，直接透传左值；不接受实参。
        if (methodId->getName() == "ref" || methodId->getName() == "mut")        {
            if (!args.empty())            {
                failAt(member->getPos(), "L3: '" + methodId->getName() +
                       "()' takes no arguments");
                return;
            }
            // P13-2/L3-F（决议 §4.1/§4.2）：`mut()` 不是写，但它要求「没有
            // 与之重叠的活跃引用」—— 与 §4.2「同一 body 的独占」一致。
            // 修前 `&p.name` 活跃时 `p.mut()` 静默通过。
            if (methodId->getName() == "mut")            {
                if (const auto recv = std::dynamic_pointer_cast<ast::IdentifierNode>(member->getLeftNode()))                {
                    if (!checkWriteConflicts(recv->getName(), member->getPos(),
                                             "mut() on")) return;
                }
            }
            emitExpression(member->getLeftNode());
            return;
        }
        // P13-2/L3-D（决议 §3.1）：take() 先挂起（pending_take + HELD），
        // 值就是接收者指针（内容位置不动）；落定/回填由 emitStatement 末结算。
        if (methodId->getName() == "take")        {
            if (!args.empty())            {
                failAt(member->getPos(), "L3: take() takes no arguments");
                return;
            }
            const auto recv = std::dynamic_pointer_cast<ast::IdentifierNode>(member->getLeftNode());
            if (!recv)            {
                failAt(member->getPos(), "L3: take() requires a Body variable receiver");
                return;
            }
            // 决议 §6.1 / §10：take 只能从 **F（Full）** 取内容。
            // E（已 clear / 已 take 走）与 X（已移动）都是空壳，取不出东西。
            // 修前只查了同一语句内的 pending_take，跨语句的状态迁移没人管，
            // 于是 `p.take()` 连着写两次、或 clear() 之后再 take 都静默通过。
            {
                const auto st = getBodyState(recv->getName());
                if (st == BodyState::EMPTY)                {
                    failAt(member->getPos(), "L3: take target '" + recv->getName() +
                           "' is Empty; there is no content to take (P13-2 / §6.1)");
                    return;
                }
                if (st == BodyState::MOVED)                {
                    failAt(member->getPos(), "L3: take target '" + recv->getName() +
                           "' has been moved and cannot be taken again (P13-2 / §6.1)");
                    return;
                }
                if (st == BodyState::UNKNOWN && isMergedUnknown(recv->getName()))                {
                    // 决议 §6.1：take 只从 F 取。? 表示控制流合流后无法确定
                    // 是否还有内容，此时 take 可能取到空壳，必须拒绝。
                    failAt(member->getPos(), "L3: take target '" + recv->getName() +
                           "' is in ? state (control flow merged to an unknown body state); "
                           "cannot take");
                    return;
                }
            }
            beginPendingTake(recv->getName(), member->getPos());
            // P13-2/L3-F（规范 §4.2）：take 等价于对**根路径**的替换，与任何
            // 活跃子引用冲突。修前 `var r = &p.name; p.take()` 静默通过。
            if (!checkWriteConflicts(recv->getName(), member->getPos(), "take on")) return;
            emitExpression(member->getLeftNode());
            return;
        }
        // P13-2/L3-C（决议 §2.1）：Body 内建 fill(c) / clear()。
        // 接收者必须是 Body 变量（IdentifierNode），才能查/改事实表状态。
        if (methodId->getName() == "fill" || methodId->getName() == "clear")        {
            const auto recv = std::dynamic_pointer_cast<ast::IdentifierNode>(member->getLeftNode());
            if (!recv)            {
                failAt(member->getPos(), "L3: '" + methodId->getName() +
                       "()' requires a Body variable receiver");
                return;
            }
            const auto& name = recv->getName();
            if (!checkNotPending(name, member->getPos(), methodId->getName() + "()")) return;
            if (hasActiveRefOn(name))            {
                failAt(member->getPos(), "L3: '" + name +
                       "' has active sub-references; cannot " + methodId->getName() + "()");
                return;
            }
            if (methodId->getName() == "clear")            {
                if (!args.empty())                {
                    failAt(member->getPos(), "L3: clear() takes no arguments");
                    return;
                }
                const auto st = getBodyState(name);
                if (st == BodyState::UNKNOWN && isMergedUnknown(name))                {
                    // 决议 §10.2：clear() 前置是「目标确定 F」。? 不满足确定，
                    // 此时 clear 可能销毁不了东西，也可能销毁一块不该动的内存。
                    failAt(member->getPos(), "L3: clear target '" + name +
                           "' is in ? state (control flow merged to an unknown body state); "
                           "cannot clear");
                    return;
                }
                if (st == BodyState::FULL)                    setBodyState(name, BodyState::EMPTY, member->getPos(), "cleared");
                // 决议 §2.2：已 E 时 no-op 幂等；H/X 状态非法操作（D 阶段细化）
                return;
            }
            // fill(c)：前置目标 E，后置 F；c（Content 变量）→ X。
            if (args.size() != 1)            {
                failAt(member->getPos(), "L3: fill(c) takes exactly one argument");
                return;
            }
            const auto st = getBodyState(name);
            if (st == BodyState::UNKNOWN && isMergedUnknown(name))            {
                // 决议 §10.1：fill 前置是「目标确定 E」。? 不确定，拒绝。
                failAt(member->getPos(), "L3: fill target '" + name +
                       "' is in ? state (control flow merged to an unknown body state); "
                       "cannot fill");
                return;
            }
            if (st != BodyState::EMPTY)            {
                failAt(member->getPos(), "L3: fill target '" + name +
                       "' is not Empty (call clear() first)");
                return;
            }
            if (const auto src = std::dynamic_pointer_cast<ast::IdentifierNode>(args[0]))            {
                const auto srcState = getBodyState(src->getName());
                if (srcState == BodyState::MOVED || srcState == BodyState::HELD ||
                    srcState == BodyState::UNKNOWN)                {
                    failAt(args[0]->getPos(), "L3: fill source '" + src->getName() +
                           "' is not available");
                    return;
                }
                setBodyState(src->getName(), BodyState::MOVED, args[0]->getPos(), "filled into '" + name + "'");
            }
            setBodyState(name, BodyState::FULL, member->getPos(), "filled");
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
        // 接收者先求值存进栈槽（实参求值可能包含调用），最后落到 %rcx。
        // 不能用 push：push 会破坏调用点的 16 字节对齐。
        emitExpression(member->getLeftNode());
        if (hasFailed()) return;
        const std::string receiverSlot = allocateTemp(sema::getPtrType());
        _asm.mov("%rax", receiverSlot);
        const std::vector<bool>* methMask = nullptr;
        if (const auto found = _functionParamAnnotated.find(symbol); found != _functionParamAnnotated.end())
            methMask = &found->second;
        const size_t methArea = emitArgsToRegisters(args, symbol, paramTypes, 1, methMask);
        if (hasFailed())        {
            return;
        }
        _asm.mov(receiverSlot, "%rcx");
        _asm.call(symbol);
        _asm.addRsp(methArea);
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
        // 不能 push 暂存：push 会破坏调用点的 16 字节对齐。
        _asm.mov(addrOf(*thisSlot), "%rax");
        const std::string superSelfSlot = allocateTemp(sema::getPtrType());
        _asm.mov("%rax", superSelfSlot);
        const std::vector<bool>* superMask = nullptr;
        if (const auto found = _functionParamAnnotated.find(baseName + ".__init__");
            found != _functionParamAnnotated.end())
            superMask = &found->second;
        const size_t superArea = emitArgsToRegisters(args, baseName + ".__init__", paramTypes, 1, superMask);
        if (hasFailed())        {
            return;
        }
        if (!base->hasCtor)        {
            _asm.addRsp(superArea);
            return;
        }
        _asm.mov(superSelfSlot, "%rcx");
        _asm.call(baseName + ".__init__");
        _asm.addRsp(superArea);
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
            // P12/G14：`null` 字面量不能用来决定泛型形参。
            // inferType 把 `null` 当 ptr，于是 `orVal(null, 10)` 的 T 会绑成
            // ptr，返回值按指针打印成 `0x000000000000000a`。null 本来就可以
            // 赋给任何指针族类型，让它参与推导只会污染结果 —— 跳过，
            // 由同一调用里其它实参（或显式类型实参）决定 T。
            if (std::dynamic_pointer_cast<ast::NullLiteralNode>(args[i])) continue;
            auto actual = inferType(args[i]);
            // 实参是具名函数（`map2(addOne, 41)`）：inferType 对标识符只会
            // 返回变量槽类型，取不到函数签名。这里按已登记的签名补出来，
            // 否则 func[A][B] 的 B 永远推不出来。
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
                    // `func[A][B]` 的形参里带泛型形参时，从实参函数类型推。
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
        // 同名可能有 `f[T]` / `f[T,K]` 两份：挑「泛型形参全部绑定上」的那一份。
        // 交错处（两份都能绑上）不可能出现 —— 绑定集来自 deduceTypeArgs，
        // 它只填该份声明自己的 genericParams。
        const GenericFunction* chosen = nullptr;
        for (const auto& g : found->second)        {
            bool complete = true;
            for (const auto& gp : g.genericParams)            {
                if (bindings.find(gp) == bindings.end() || !bindings.at(gp)) { complete = false; break; }
            }
            if (!complete) continue;
            // 还要保证绑定集里没有这一份声明不认识的泛型形参（否则是另一份的）。
            bool foreign = false;
            for (const auto& [key, _] : bindings)            {
                bool known = false;
                for (const auto& gp : g.genericParams)                {
                    if (gp == key) { known = true; break; }
                }
                if (!known) { foreign = true; break; }
            }
            if (foreign) continue;
            chosen = &g;
            break;
        }
        if (!chosen) return std::string();
        const GenericFunction& generic = *chosen;
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
        // 缺省实参表也要按 mangled 名登记一份：调用点拿 mangled 名去查
        // _functionDefaults 补空位（见 normalizeCallArgs）。漏了这一步，
        // 泛型调用少传的实参补不上，寄存器里留着上一个调用的残值。
        if (!generic.defaults.empty()) _functionDefaults[mangled] = generic.defaults;
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
    std::vector<const CodeGen::GenericFunction*> CodeGen::genericCandidates(
        const std::string& name, const size_t explicitCount) const    {
        std::vector<const GenericFunction*> out;
        const auto found = _genericFunctions.find(name);
        if (found == _genericFunctions.end()) return out;
        for (const auto& g : found->second)        {
            // 显式给了类型实参：个数必须对得上（这是硬约束，
            // 库里的 pair[A,B] 不能用 pair[A] 去调）。
            if (explicitCount > 0 && g.genericParams.size() != explicitCount) continue;
            out.push_back(&g);
        }
        return out;
    }
    bool CodeGen::genericAcceptsArgCount(const GenericFunction& g, const size_t argc) const    {        // 实参个数必须落在「必填形参个数 .. 全部形参个数」之间。
        // 有默认值的形参可以不传 —— 这一点原先漏了，导致
        // `fun pad[T](x: T, n: int = 42)` 的 `pad(1)` 被当成「个数不符」
        // 而报 Cannot deduce（P13 回归，2026-11-17）。
        const size_t total = g.paramNames.size();
        size_t required = total;
        for (size_t i = 0; i < total; ++i)        {
            const std::string& pname = g.paramNames[i];
            if (g.defaults.find(pname) != g.defaults.end())            {
                required = i;
                break;
            }
        }
        return argc >= required && argc <= total;
    }
    // 这份泛型声明的形参类型与实参是否相容（只看「容器种类」这一层）。
    //
    // 为什么必须有这一步（P13 缺陷修复，2026-11-17）：候选选择原先只按
    // 「显式类型实参个数 + 实参个数 + 能否推导」过滤，完全不管实参类型。
    // 于是 `dumpToStr[str](d)`（d 是 dict）会选中 list 版的
    // `fun dumpToStr[T](c: list[T])` —— 类型实参个数 1 恰好对上，
    // 推导也「成功」（把 str 绑给 T），编译通过，运行时把 dict 当 list
    // 解引用，直接段错误（实测 exit -1073741819）。
    //
    // 判据只认「根本不可转换」的情形：容器 vs 非容器、容器种类不同。
    // 元素类型留给实例化后的 emitArgsToRegisters 去校验（那里有完整的
    // checkAssignable），这里判太细会把合法调用也挡掉。
    bool CodeGen::genericArgsShapeMatch(const GenericFunction& g,
                                        const std::vector<std::shared_ptr<ast::ExpressionNode>>& args) const    {
        const size_t n = args.size() < g.paramTypes.size() ? args.size() : g.paramTypes.size();
        for (size_t i = 0; i < n; ++i)        {
            const auto& formal = g.paramTypes[i];
            if (!formal) continue;
            // 形参本身是泛型占位符或未知：什么实参都收
            const auto fk = formal->getKind();
            if (fk == sema::TypeKind::TK_GENERIC || fk == sema::TypeKind::TK_UNKNOWN) continue;
            auto actual = inferType(args[i]);
            if (!actual) continue;
            const auto ak = actual->getKind();
            if (ak == sema::TypeKind::TK_UNKNOWN) continue;
            const auto isContainerKind = [](const sema::TypeKind k) {
                return k == sema::TypeKind::TK_STR || k == sema::TypeKind::TK_LIST ||
                       k == sema::TypeKind::TK_DICT;
            };
            if (isContainerKind(fk) || isContainerKind(ak))            {
                // 容器与非容器之间、或不同种类的容器之间：不可转换
                if (fk != ak) return false;
            }
        }
        return true;
    }
    sema::TypePtr CodeGen::inferGenericCallType(const std::string& name,
                                                const std::shared_ptr<ast::ExpressionNode>& callee,
                                                const std::shared_ptr<ast::ExpressionNode>& callNode) const    {
        // 实参列表
        std::vector<std::shared_ptr<ast::ExpressionNode>> args;
        if (const auto call = std::dynamic_pointer_cast<ast::FunctionCallNode>(callNode))        {
            if (const auto argsNode = std::dynamic_pointer_cast<ast::ExpressionNode>(call->getRightNode()))            {
                collectArgs(argsNode, args);
            }
        }
        const auto explicitArgs = explicitTypeArgs(callee);
        // 同名泛型可能有多份（`f[T]` / `f[T,K]`）：逐个试，第一个能推出
        // 全部泛型形参、且能绑定实参的候选就是答案。
        // 约束按「显式类型实参个数」先过滤，再从实参推导。
        for (const auto* generic : genericCandidates(name, explicitArgs.size()))        {
            if (!genericAcceptsArgCount(*generic, args.size())) continue;
            if (!genericArgsShapeMatch(*generic, args)) continue;
            std::map<std::string, sema::TypePtr> bindings;
            if (!deduceTypeArgs(*generic, explicitArgs, args, bindings)) continue;
            auto ret = substituteGenerics(generic->retType, bindings);
            if (!ret || ret->getKind() == sema::TypeKind::TK_UNKNOWN ||
                ret->getKind() == sema::TypeKind::TK_GENERIC)
                continue;
            return ret;
        }
        return nullptr;
    }
    bool CodeGen::tryEmitGenericCall(const std::string& name,
                                     const std::shared_ptr<ast::ExpressionNode>& callee,
                                     const std::shared_ptr<ast::ExpressionNode>& callNode,
                                     const std::vector<std::shared_ptr<ast::ExpressionNode>>& args)    {
        const auto found = _genericFunctions.find(name);
        if (found == _genericFunctions.end()) return false;
        const auto explicitArgs = explicitTypeArgs(callee);
        // 同名泛型可能有多份（`f[T]` / `f[T,K]`）。按实参个数 + 能否推出
        // 全部泛型形参来挑；挑不出唯一一份时按下面的规则给诊断。
        const GenericFunction* chosen = nullptr;
        std::map<std::string, sema::TypePtr> bindings;
        for (const auto* generic : genericCandidates(name, explicitArgs.size()))        {
            if (!genericAcceptsArgCount(*generic, args.size())) continue;
            if (!genericArgsShapeMatch(*generic, args)) continue;
            std::map<std::string, sema::TypePtr> candidateBindings;
            // 显式类型实参必须能覆盖这一份声明的**全部**泛型形参。
            // 否则当作不匹配 —— 这一步保证了
            // `dumpToStr[int]([])` 只会考虑 `[T]` 那份（1 个类型实参），
            // 不会被 `[T, K]` 那份错误的报「takes 2」。
            if (!explicitArgs.empty() && explicitArgs.size() != generic->genericParams.size()) continue;
            if (!deduceTypeArgs(*generic, explicitArgs, args, candidateBindings)) continue;
            chosen = generic;
            bindings = std::move(candidateBindings);
            break;
        }
        if (!chosen)        {
            // 挑不出候选。**先看这个名字下有没有「非泛型」的重载** ——
            // 有的话必须把机会交回去（返回 false 让调用方走重载决议），
            // 不能在这里直接报「推不出类型实参」。
            //
            // 典型形状（ds.rio 的 dumpToStr）：
            //     fun dumpToStr(container: list[any]): str        // 非泛型
            //     fun dumpToStr[T, K](container: dict[T, K]): str // 泛型
            // `dumpToStr([])` 本该命中非泛型那份，但空列表推不出 T/K，
            // 若在这里报错，用户会看到一个完全无关的「Cannot deduce T, K」。
            //
            // 判据：名字下有**非泛型**定义就交回去。注意不能用
            // _overloadedNames —— 泛型声明在 registerOverloadSignatures 里
            // 被跳过了，那张表只反映「非泛型定义是否 >= 2 份」。
            if (_functionReturns.find(name) != _functionReturns.end())            {
                return false;
            }
            // 挑不出候选：**只在显式类型实参个数与某一份完全对不上时**才报
            // "takes N"，否则报「推不出来」。判据要与上面循环一致 ——
            // 要有一份的泛型形参个数 == explicitArgs.size()，才轮到说
            // 「个数对但推不出」；一份都不匹配时才说「个数都不对」。
            const auto& all = found->second;
            // 先看有没有「类型实参个数对得上」的候选。
            const GenericFunction* ArityMatch = nullptr;
            for (const auto& g : all)            {
                if (g.genericParams.size() == explicitArgs.size()) { ArityMatch = &g; break; }
            }
            // 类型实参个数对得上、但**形参形状不符**（例如
            // `dumpToStr[str](d)` 里 d 是 dict，而个数 1 的那份形参是 list）：
            // 报「形参类型不匹配」比「推不出 T」准确得多 —— 后者会让用户
            // 以为自己忘了写类型实参（2026-11-17 实测）。
            if (ArityMatch && !genericArgsShapeMatch(*ArityMatch, args))            {
                std::string expected;
                for (size_t i = 0; i < ArityMatch->paramTypes.size(); ++i)                {
                    if (ArityMatch->paramTypes[i])                    {
                        if (!expected.empty()) expected += ", ";
                        expected += ArityMatch->paramTypes[i]->toString();
                    }
                }
                std::string actual;
                for (const auto& a : args)                {
                    const auto t = inferType(a);
                    if (!actual.empty()) actual += ", ";
                    actual += (t ? t->toString() : std::string("?"));
                }
                failAt(callNode ? callNode->getPos() : utils::Pos{},
                       "No matching generic overload for '" + name + "' with arguments (" +
                       actual + "); the candidate taking " +
                       std::to_string(ArityMatch->genericParams.size()) +
                       " type argument(s) expects (" + expected + ")");
                return true;
            }
            if (!explicitArgs.empty() && !ArityMatch && !all.empty())            {
                // 只有一份声明时用最直接的措辞（与 P10 起沿用的诊断一致）：
                // `pair[int](1, "x")` 写错个数，直接说它要几个。
                if (all.size() == 1)                {
                    failAt(callNode ? callNode->getPos() : utils::Pos{},
                           "Generic function '" + name + "' takes " +
                           std::to_string(all.front().genericParams.size()) +
                           " type argument(s), but " + std::to_string(explicitArgs.size()) +
                           " were supplied");
                    return true;
                }
                failAt(callNode ? callNode->getPos() : utils::Pos{},
                       "Generic function '" + name + "' has no overload taking " +
                       std::to_string(explicitArgs.size()) + " type argument(s); candidates take " +
                       [&all] {
                           std::string s;
                           for (const auto& g : all)                           {
                               if (!s.empty()) s += ", ";
                               s += std::to_string(g.genericParams.size());
                           }
                           return s;
                       }() + " type argument(s)");
                return true;
            }
            const GenericFunction* diagnostic = ArityMatch ? ArityMatch
                                                           : (all.empty() ? nullptr : &all.front());
            if (diagnostic)            {
                std::string missing;
                for (const auto& g : diagnostic->genericParams)                {
                    if (bindings.find(g) == bindings.end())                {
                        if (!missing.empty()) missing += ", ";
                        missing += g;
                    }
                }
                if (!missing.empty())                {
                    failAt(callNode ? callNode->getPos() : utils::Pos{},
                           "Cannot deduce type arguments for generic function '" + name +
                           "' (unspecified: " + missing + ")");
                    return true;
                }
            }
            // 走到这里说明是「没有一份能接受这个实参个数」——交回普通调用路径，
            // 由它报「未定义函数」或交给重载决议。
            return false;
        }
        const GenericFunction& generic = *chosen;
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
        const size_t instArea = emitArgsToRegisters(instArgs, mangled, paramTypes, 0, instMask);
        if (hasFailed()) return true;
        _asm.call(mangled);
        _asm.addRsp(instArea);
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
                    collectParamDefaults(paren->getRangerNode(), generic.defaults);
                }
                else if (paramsNode)                {
                    collectParamNames(paramsNode, generic.paramNames);
                    collectParamTypesT(paramsNode, generic.paramTypes);
                    collectParamDefaults(paramsNode, generic.defaults);
                }
            }
            generic.retType = typeFromLabelList(node->getLabelNodes(), genericParams);
            _currentGenericParams = savedParams;
            while (generic.paramTypes.size() < generic.paramNames.size())
                generic.paramTypes.push_back(sema::getUnknownType());
            // 同名不同泛型形参个数是合法的重载形状，按名字收成一组，
            // 不能覆盖（P13 缺陷修复，2026-11-17）。
            _genericFunctions[fname].push_back(std::move(generic));
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
                // P13-2/L3：五个特殊类型名带 `[...]`（如 Take[Player]），
                // resolveTypeName 只认裸名；paramTypes 那条路径已由
                // typeFromTypeArgNode 正确解析，这里跳过字符串校验即可。
                if (firstWord == "Body" || firstWord == "Content" ||
                    firstWord == "Ref" || firstWord == "Mut" || firstWord == "Take") continue;
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
        // 变长形参的存在性：`*args` 允许调用点传任意多个位置实参，
        // `**kwargs` 允许任意多个命名实参。它们不占 paramTypes 的位置，
        // 所以「收不收多余实参」只能单独登记。
        {
            bool hasPositional = false, hasKeyword = false;
            if (const auto callInfix =
                    std::dynamic_pointer_cast<ast::InfixExpressionNode>(node->getCallNode()))            {
                if (const auto paren =
                        std::dynamic_pointer_cast<ast::ParenRangerNode>(callInfix->getRightNode()))
                    collectVariadicFlags(paren->getRangerNode(), hasPositional, hasKeyword);
                else
                    collectVariadicFlags(callInfix->getRightNode(), hasPositional, hasKeyword);
            }
            else            {
                collectVariadicFlags(node->getCallNode(), hasPositional, hasKeyword);
            }
            VariadicInfo info;
            info.fixedCount = paramNames.size();
            info.positional = hasPositional;
            info.keyword = hasKeyword;
            if (hasPositional || hasKeyword) _functionVariadic[fname] = info;
            else _functionVariadic.erase(fname);
        }
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
        auto retType = returnTypeFromNode(node);
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
        _currentSignatureClauses = node->getSignatureClauses();
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
        // P13-2：内存模型的三张事实表（移动状态 / 活跃子引用 / 出生地）
        // 以及语句级的 pending_take，都是**函数局部**的。
        // 与 _scopes / _scopeDrops 同理：函数体是独立栈帧，形参同名也只是
        // 巧合，绝不能继承上一个函数（或外层函数）的登记项。
        // 实测（修前）：
        //   fun a(): void { var p: Player = Player("x"); var r = &p.name }
        //   fun b(p: Player): void { p.clear() }
        //   → 假报错 "'p' has active sub-references"
        //   fun a(): void { var pl: Pool = Pool.fixed(4); var q: Player pl = ... }
        //   fun b(q: Player): void { pl.release(q) }
        //   → 假通过（出生地串了函数）
        // 自然结束（走到末尾）的函数不会经过 popScope 链，泄漏最明显。
        const auto savedMoveStates = _moveStates;
        const auto savedActiveRefs = _activeRefs;
        const auto savedBirthPlaces = _birthPlaces;
        const auto savedPendingTakes = _pendingTakes;
        // P13-2/L3-G：不透明 `?` 集合同样是函数局部的（形参同名也只是巧合）。
        const auto savedOpaqueMoveVars = _opaqueMoveVars;
        // P13-1：销毁登记表与 _scopes 是**同生共死**的一对，跨函数必须一起清。
        // 只清 _scopes 而不清 _scopeDrops 会让内层函数对**外层帧**的槽位发
        // drop（实测：mk() 帧只有 64 字节，却对 -176(%rbp) 调 core_free，
        // 直接崩在堆上）。函数体是独立帧，不需要继承外层登记表，所以直接清。
        _functionName = emitName;
        _functionReturnType = retType;
        _functionReturnAnnotated = retTypeAnnotated;
        // P13-2/L3-F（决议 §9.3/§9.4）：writes / from 的每一项必须是形参路径。
        // 必须排在 validateEnsuresClauses **之前** —— 后者结束时会
        // _currentSignatureClauses.clear()，之后再读就是空表。
        validateWritesFromClauses(paramNames);
        if (hasFailed()) return;
        // P13-2/L3-F（决议 §9.2）：ensures 语法校验（只对 Take 形参生效）
        validateEnsuresClauses(paramNames, paramTypes);
        if (hasFailed()) return;
        _frameSize = 0;
        _ehStack.clear();
        _loopStack.clear();
        // P13-2/L3-G：循环状态合流用的出口/回边路径表也随函数体清空，
        // 否则上一个函数里未闭合的循环会污染下一个函数。
        _loopExitStates.clear();
        _loopBackStates.clear();
        _opaqueMoveVars.clear();
        _scopes.clear();
        _scopeDrops.clear();
        // P13-2：三张内存模型事实表 + 挂起集合随函数体一起清空。
        _moveStates.clear();
        _activeRefs.clear();
        _birthPlaces.clear();
        _pendingTakes.clear();
        pushScope();
        if (makeGlobal) _asm.global(emitName);
        _asm.label(emitName);
        _asm.push("%rbp");
        _asm.mov("%rsp", "%rbp");
        // 栈帧大小在函数体生成完才知道，这里先占位，末尾回填。
        const std::string framePlaceholder = _asm.newLabel("frame_" + emitName);
        _asm.label(framePlaceholder);
        // 形参按**整体位置**接收（MS ABI，gcc 实测）：第 pos 个形参用
        // intRegs[pos] 或 floatRegs[pos]，两类共用同一个位置下标；
        // pos >= 4 的形参在调用者的栈上，序号 k = pos - 4，
        // 入口处位于 getStackArgOffset(k)，push %rbp 之后整体 +8。
        const auto& intRegs = _abi.getIntArgRegs();
        const auto& floatRegs = _abi.getFloatArgRegs();
        const size_t registerSlots = intRegs.size() < floatRegs.size() ? intRegs.size() : floatRegs.size();
        size_t argPos = 0;
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
            argPos = 1;
        }
        for (size_t i = 0; i < paramNames.size(); ++i)        {
            auto pt = (i < paramTypes.size()) ? paramTypes[i] : sema::getIntType();
            if (!pt || pt->getKind() == sema::TypeKind::TK_UNKNOWN) pt = sema::getIntType();
            const long long offset = allocateSlot(paramNames[i], pt, pt->getSize());
            const std::string slot = std::to_string(offset) + "(%rbp)";
            // P13-1（D9 规则 C）：形参一律**借用** —— 被调函数不释放它，
            // 调用方负责。登记成借用后，`ret 形参` / 把它塞进容器都会被
            // D18 的负向检查拦下。
            //
            // 槽位地址必须传：形参被重新赋值成新值时会升格成拥有者，
            // 那时出口要拿这个地址去释放（忘了传会生成 `leaq , %rcx`）。
            registerBorrow(paramNames[i], pt, slot);
            // P13-2/L3-F（决议 §9.2）：Take 形参进入函数时状态为 `?`（不确定）。
            if (pt && pt->getKind() == sema::TypeKind::TK_MEMORY &&
                std::dynamic_pointer_cast<sema::MemoryType>(pt)->getRole() == sema::MemoryRole::TAKE)            {
                registerMoveVar(paramNames[i]);
                setBodyState(paramNames[i], BodyState::UNKNOWN, utils::Pos{}, "Take parameter");
                // 决议 §9.2：这个 `?` 是「编译器不做承诺」，不是合流不确定。
                // 登记进不透明集合，读字段/调方法/take 仍然合法（§7.3）。
                _opaqueMoveVars.insert(paramNames[i]);
            }
            const size_t pos = argPos + i;
            if (pos < registerSlots)            {
                if (pt->isFloat())                {
                    _asm.inst("movsd", floatRegs[pos], slot);
                }
 else                {
                    _asm.mov(intRegs[pos], slot);
                }
            }
 else            {
                // 栈参数：getStackArgOffset 以入口 %rsp 为基准，
                // 此时 %rbp = 入口 %rsp - 8，所以再 +8。
                const std::string from = std::to_string(_abi.getStackArgOffset(pos - registerSlots) + 8) + "(%rbp)";
                if (pt->isFloat())                {
                    _asm.inst("movsd", from, "%xmm0");
                    _asm.inst("movsd", "%xmm0", slot);
                }
 else                {
                    _asm.mov(from, "%rax");
                    _asm.mov("%rax", slot);
                }
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
        // P13-1（D4）：函数体末尾（隐式返回）也要销毁本帧的局部变量。
        //
        // 为什么必须在这里做：函数体是一个**语句序列**而不是 BlockRanger，
        // 所以它没有自己的 popScope —— 只在 popScope 里发 drop 的话，
        // 函数体这一层永远不销毁。实测 `fun f() { var s = "a"+"b" }`
        // 的 s 一路泄漏（core_dbg_frees() 一直是 0）。
        //
        // 放在「设置默认返回值」**之前**：drop 会冲掉 %rax/%xmm0，而默认值
        // 是常量（0 / 0.0 / this 槽地址），drop 之后再装正好。
        // 显式 ret 不走这里 —— 它跳到 _returnLabel 前已自己发过 drop。
        for (size_t i = _scopeDrops.size(); i > 0; --i)        {
            emitScopeDrops(i - 1, true);
        }
        if (defaultSlot)        {
            _asm.mov(addrOf(*defaultSlot), "%rax");
        }
        else if (retType && retType->isFloat())        {
            _asm.inst("xorpd", "%xmm0", "%xmm0");
        }
        else        {
            _asm.movImm(0, "%rax");
        }
        // P13-2/L3-H（决议 §9.2）：`throw` 出口不检查 ensures。
        // 这里的末尾检查只对应「走到函数末尾」这一个正常出口；若函数体的最后
        // 一条语句是发散的（throw / return / 全分支发散且有 else 的 if），
        // 就根本没有这条出口，不能拿它去校验 ensures。
        // 实测（修前）：`fun f(t: Take[P]): int ensures[!t] { throw "x" }`
        // 只有 throw 一个出口，却被末尾检查误报「unknown state at function exit」。
        // 显式 ret 出口由 emitReturn 自己逐出口校验，不受影响。
        if (!statementDiverges(bodyNode))
            checkEnsuresAtExit(bodyNode ? bodyNode->getPos() : utils::Pos{});
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
        _ensuresMarks.clear();
        _returnLabel = savedReturnLabel;
        _functionReturnType = savedReturnType;
        _functionReturnAnnotated = savedReturnAnnotated;
        _ehStack = savedEhStack;
        _loopStack = savedLoopStack;
        // P13-1：与上面的 clear 对称 —— 恢复外层作用域的销毁登记表。
        // 不清回的话，外层变量会被内层"继承"并在错误的位置销毁。
        _scopes.clear();
        _scopeDrops.clear();
        // P13-2：三张事实表同理回滚到进入本函数前的状态。
        _moveStates = savedMoveStates;
        _activeRefs = savedActiveRefs;
        _birthPlaces = savedBirthPlaces;
        _pendingTakes = savedPendingTakes;
        _opaqueMoveVars = savedOpaqueMoveVars;
        _loopExitStates.clear();
        _loopBackStates.clear();
        pushScope();          // 与外层入口时的状态对齐（_scopes 与 _scopeDrops 各一层）
    }
}


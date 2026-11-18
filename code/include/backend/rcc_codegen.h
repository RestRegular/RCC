//
// rcc_codegen.h —— 从 AST 直接生成 x86-64 汇编
//
// P1 阶段支持：
//   字面量（整数/浮点/字符串/布尔/字符/null）、算术、比较、逻辑
//   var 定义、赋值、复合赋值、++/--
//   fun 定义与调用、ret、递归
//   if/elif/else、while/until/for、break/continue
//   sout（按静态类型分派到 rt 的打印函数）
//
// 未支持的节点会明确报错，而不是静默生成错误代码。
//

#ifndef RCC_CODEGEN_H
#define RCC_CODEGEN_H

#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "rcc_asm_emitter.h"
#include "rcc_abi.h"
#include "../sema/rcc_sema_types.h"
#include "../lib/rcc_utils.h"

namespace ast
{
    class LabelNode;
    class ProgramNode;
    class ExpressionNode;
    class FunctionDefinitionNode;
    class ClassDefinitionNode;
    // 决议 A（2026-11-18）：值语义 struct 的 AST 节点。
    class StructDefinitionNode;
    class InfixExpressionNode;
    class IndexExpressionNode;
    class BlockRangerNode;
}

namespace backend
{
    class CodeGen
    {
    public:
        explicit CodeGen(const AbiInfo& abi);

        // 编译为库（不发射 main 入口，也不把顶层语句包进 __rio_main）。
        // 用于编译 lib/rt / lib/std 这类要链接进别人程序的目标文件。
        void setLibraryMode(bool on) { _libraryMode = on; }

        // P8：告诉后端哪些名字是「模块名」。模块在加载期已经被内联成
        // 普通全局函数，所以 `ds.size(x)` 这种限定调用要退化成 `size(x)`。
        void setModuleNames(const std::vector<std::string>& names,
                            const std::vector<std::string>& aliases = {})
        {
            _moduleNames = names;
            // 别名与模块名同等对待：`import(u="utilx")` 之后 `u.twice(21)`
            // 里的 u 也要被 isModuleName 认出来。
            _moduleNames.insert(_moduleNames.end(), aliases.begin(), aliases.end());
        }

        // 决议 C（2026-11-18 用户裁决）：条件销毁出口的诊断级别。
        //
        // 默认：需要「条件销毁」的出口是**编译错误**（不是警告、不是静默）。
        // 放行需要**双重条件**同时成立：
        //   1. 命令行带 --allow-cond-destroy（即本开关为 true，启用指令扫描）；
        //   2. 文件级注释指令 `// @allow-cond-destroy` 存在。
        // 缺一仍报错。这样「放行」既是代码里的显式人为决定，也是构建配置
        // 层面的显式决定，不会因为粘贴代码而悄悄生效。
        void setAllowCondDestroy(bool on) { _allowCondDestroy = on; }
        // 文件级放行指令是否已被扫描到（由前端/驱动在读取源文件后告知）。
        void setCondDestroyDirectiveSeen(bool seen) { _condDestroyDirectiveSeen = seen; }

        // 生成完整汇编文本。失败时 errorMsg 非空。
        std::string generate(const std::shared_ptr<ast::ProgramNode>& program, std::string& errorMsg);

    private:
        // ---------- 顶层 ----------
        void emitProgram(const std::shared_ptr<ast::ProgramNode>& program);
        void emitPrelude();
        void emitMainWrapper();

        // ---------- 语句/表达式 ----------
        // 结果留在 %rax（整型）或 %xmm0（浮点）
        void emitExpression(const std::shared_ptr<ast::ExpressionNode>& node);
        void emitStatement(const std::shared_ptr<ast::ExpressionNode>& node);

        void emitIntegerLiteral(const std::shared_ptr<ast::ExpressionNode>& node);
        void emitFloatLiteral(const std::shared_ptr<ast::ExpressionNode>& node);
        void emitStringLiteral(const std::shared_ptr<ast::ExpressionNode>& node);
        void emitBoolLiteral(const std::shared_ptr<ast::ExpressionNode>& node);
        void emitNullLiteral();
        void emitIdentifier(const std::shared_ptr<ast::ExpressionNode>& node);
        void emitBinary(const std::shared_ptr<ast::ExpressionNode>& node);
        void emitFloatBinary(const std::shared_ptr<ast::ExpressionNode>& node,
                             const sema::TypePtr& lt, const sema::TypePtr& rt);
        void emitUnary(const std::shared_ptr<ast::ExpressionNode>& node);
        void emitPostfix(const std::shared_ptr<ast::ExpressionNode>& node);
        void emitCompoundAssign(const std::shared_ptr<ast::ExpressionNode>& node);
        void emitAssignment(const std::shared_ptr<ast::ExpressionNode>& node);
        void emitVariableDefinition(const std::shared_ptr<ast::ExpressionNode>& node);
        void emitFunctionDefinition(const std::shared_ptr<ast::FunctionDefinitionNode>& node);
        void emitFunctionCall(const std::shared_ptr<ast::ExpressionNode>& node);
        // P8：`mod.fn(args)` —— 模块已在加载期内联，直接按全局函数 fn 调用。
        void emitModuleFunctionCall(const std::string& fname,
                                    const std::shared_ptr<ast::ExpressionNode>& callNode,
                                    const std::vector<std::shared_ptr<ast::ExpressionNode>>& args);
        [[nodiscard]] bool isModuleName(const std::string& name) const;

        // ---------- P4：函数值 / 匿名函数 / 间接调用 ----------
        // 匿名函数表达式：把函数体排进待发射队列，当前表达式的值取闭包对象地址。
        void emitAnonymousFunction(const std::shared_ptr<ast::ExpressionNode>& node);
        // 间接调用：闭包指针 -> 取 { fnptr, env }，env 走 %r10，fnptr 走 call *%r11。
        // paramTypes 非空时按它决定实参走整型还是浮点寄存器。
        void emitIndirectCall(const std::shared_ptr<ast::ExpressionNode>& callee,
                              const std::vector<std::shared_ptr<ast::ExpressionNode>>& args,
                              const std::vector<sema::TypePtr>& paramTypes = {});
        // 发射一个函数的完整帧（序言 + 形参落栈 + 函数体 + 尾声 + 栈帧回填）。
        // 命名函数与匿名函数共用这一份逻辑。
        // capturedNames/capturedTypes 非空时，函数按「闭包体」发射：
        // 序言里把 %r10 指向的 env 块逐项拷进本地槽位，函数体随后按普通局部变量访问。
        void emitFunctionBody(const std::string& emitName,
                              const std::vector<std::string>& paramNames,
                              const std::vector<sema::TypePtr>& paramTypes,
                              const sema::TypePtr& retType,
                              const std::shared_ptr<ast::ExpressionNode>& bodyNode,
                              bool makeGlobal,
                              const std::vector<std::string>& capturedNames = {},
                              const std::vector<sema::TypePtr>& capturedTypes = {},
                              const std::string& defaultReturnVar = "",
                              bool hasThis = false,
                              bool retTypeAnnotated = false);
        // 把所有排队的匿名函数体发射出来（发射过程中可能继续入队，循环到队空）。
        void emitPendingFunctions();
        // 发射闭包对象（无捕获时是静态的 { fnptr, env } 常量）。
        void emitClosureObjects();
        // 具名函数被当作函数值使用时的闭包对象标签（按函数名去重）。
        std::string closureForFunction(const std::string& fname);
        // 在匿名函数体里收集自由变量名（不含形参、不含它自己定义的局部变量、
        // 不含函数名）。嵌套匿名函数的函数体不深入 —— 它的捕获在它自己的
        // 创建点解析，与外层无关。
        void collectFreeVariables(const std::shared_ptr<ast::ExpressionNode>& node,
                                  std::vector<std::string>& bound,
                                  std::vector<std::string>& free);
        // 第 pos 个参数在调用区（以调用点 %rsp 为基准）里的字节偏移。
        // 寄存器参数落在 shadow space 内（pos*8，0..24），
        // 溢出参数落在 shadow space 之后（shadow + (pos-4)*8）。
        size_t argSlotOffset(size_t pos) const;
        // 决议 B：容器内建 get() —— 返回 `T?`，越界/未命中给 null（不 panic）。
        void emitListGetOrNull(const std::shared_ptr<ast::ExpressionNode>& receiver,
                               const std::shared_ptr<ast::ExpressionNode>& indexExpr,
                               const sema::TypePtr& elemType);
        void emitDictGetOrNull(const std::shared_ptr<ast::ExpressionNode>& receiver,
                               const std::shared_ptr<ast::ExpressionNode>& keyExpr,
                               const sema::TypePtr& valType);
        // 把实参求值并装进 ABI 参数寄存器，超出的写进调用者预留的栈槽，
        // 最后**自己**把整块调用区（shadow space + 溢出槽）从 %rsp 上预留出来。
        // 返回值是预留的字节数，调用点负责 `call` 之后 `addRsp(该值)`。
        //
        // argStart 是第一个实参的**整体参数位置**：方法/构造器的隐藏 this
        // 占掉位置 0，所以用户实参从 1 起排。
        // MS ABI 实测规则（gcc 15.2.0 交叉验证）：寄存器分配是**按整体位置**
        // 的，第 pos 个参数用 intRegs[pos] 或 floatRegs[pos]，两类共用同一个
        // 位置下标；pos >= 4 的参数一律进栈，栈槽序号 = pos - 4。
        size_t emitArgsToRegisters(const std::vector<std::shared_ptr<ast::ExpressionNode>>& args,
                                   const std::string& calleeDesc,
                                   const std::vector<sema::TypePtr>& paramTypes = {},
                                   size_t argStart = 0,
                                   const std::vector<bool>* annotated = nullptr);
        void emitReturn(const std::shared_ptr<ast::ExpressionNode>& node);
        // P13-2/L1：`ret` 在循环里退最近作用域（等价 break），与退函数的 return 分开。
        void emitRetFromLoop(const std::shared_ptr<ast::ExpressionNode>& node);
        // P13-2/L2：`ret` 在带 : T 标注的 if 分支里退该 if 并带出值。
        void emitRetFromIf(const std::shared_ptr<ast::ExpressionNode>& node);
        // P13-2/L3-N：`ret` 在块表达式里退该块并带出值。
        void emitRetFromBlock(const std::shared_ptr<ast::ExpressionNode>& node);
        void emitIf(const std::shared_ptr<ast::ExpressionNode>& node);
        // asValue=true 表示这个块是「块表达式」（出现在右值位置）。
        // 决议 §13.1 的「块表达式」是可返回值作用域，§15.1 的裸块语句不是 ——
        // 后者块内 `ret` 按外层上下文生效，所以两种位置必须走不同路径。
        void emitBlock(const std::shared_ptr<ast::ExpressionNode>& node, bool asValue = false);
        void emitLoop(const std::shared_ptr<ast::ExpressionNode>& node);
        void emitBreak(const std::shared_ptr<ast::ExpressionNode>& node = nullptr);
        void emitContinue();

        // ---------- P7：异常（方案 B 显式帧链） ----------
        //
        // 帧布局（40 字节，一块连续栈槽，由 allocateSlot 分配）：
        //   +0   prev        core_eh_push 写入的上一帧（链指针）
        //   +8   saved rbp   进入 try 时的 %rbp
        //   +16  saved rsp   进入 try 时的 %rsp
        //   +24  resume      handler 命中后要跳转的标签地址
        //   +32  exc         异常值（core_eh_throw 写入，catch 读出）
        //
        // 为什么把 rbp/rsp 存进帧而不是用 %rbx/%r12 之类：
        // 这些寄存器虽然目前没被 codegen 使用，但一旦将来引入寄存器
        // 分配就会冲突；栈槽是 %rbp 相对的，跨函数调用天然稳定。
        //
        // 代价（docs/architecture_plan.md §8.2 已接受）：非局部退出
        // 不保证执行析构。但 **handler 链必须保持干净** —— 所以 ret /
        // break / continue 跳出 try 时会显式 core_eh_pop，否则链上会
        // 留下指向已销毁栈帧的节点，下一次 throw 就会跳进死帧。
        void emitTry(const std::shared_ptr<ast::ExpressionNode>& node);
        void emitThrow(const std::shared_ptr<ast::ExpressionNode>& node);
        // 发射 core_eh_pop(frameOffset)。
        void emitEhPop(long long frameOffset);
        // 把 handler 链回退到 depth（弹出 _ehStack 里 index >= depth 的帧）。
        void emitEhUnwindTo(size_t depth);

        // ---------- P5：容器（str / list / dict） ----------
        //
        // **ABI v2**（P13-1，2026-11-17）：三个容器统一成「header 与 data 分离」，
        // 且移除已废弃的 rc 字段。理由与裁决见 docs/p13_plan.md 的 D11 / D12：
        //
        //   * D12=A：header 地址必须**永不改变**，data 可以 realloc。
        //     单指针布局（header+data 同块）下，原地追加一旦扩容就会让
        //     调用者的变量、其他拷贝、形参里的指针全部悬空。
        //   * D11=A：`core_free` 对 `cap==0` 静默返回，用来标记「不可释放」。
        //     str 字面量指向静态只读头、不拷贝，必须靠 cap=0 识别。
        //     旧 str 布局根本没有 cap 字段，所以这条裁决要求 str 加 cap。
        //   * rc 字段从 G 轮起就废弃（内存模型非目标：引用计数），
        //     与上面的改造合并成**一次** ABI 变更，不拆两次。
        //
        // 统一后的布局（所有偏移都是字节，字段 8 字节对齐）：
        //
        //   StrHeader  { i64 len@0, i64 cap@8,  u8 data@16 }
        //   ListHeader { i64 len@0, i64 cap@8,  i64 elem_size@16, ptr data@24 }
        //   DictHeader { i64 len@0, i64 cap@8,  ptr keys@16,       ptr vals@24 }
        //
        // **len 一律留在 +0** 是刻意的：`core_str_len` 以及 std 里
        // `size()`/`isEmpty()` 对 str/list/dict 通用，全靠长度在偏移 0。
        //
        // list/dict 的 data/keys/vals 都是**独立分配的堆块**，header 只持有指针。
        // 扩容 = realloc data 块 + 改写 header 里的指针，header 地址不动。
        //
        // 字典仍是 keys / vals 两块连续数组 + 线性扫描（不做哈希）；
        // 元素/键/值槽恒为 8 字节。
        static constexpr long long kHeaderLen = 0;
        static constexpr long long kHeaderCap = 8;

        // str
        static constexpr long long kStrData = 16;
        static constexpr size_t kStrHeaderSize = 16;

        // list
        static constexpr long long kListElemSize = 16;
        static constexpr long long kListData = 24;
        static constexpr size_t kListHeaderSize = 32;

        // dict
        static constexpr long long kDictKeys = 16;
        static constexpr long long kDictVals = 24;
        static constexpr size_t kDictHeaderSize = 32;

        // ---------- P13-1：销毁（drop glue） ----------
        //
        // 裁决依据（docs/p13_plan.md）：
        //   * D16：按**元素类型**生成 drop glue 函数，出口只发一次 call，
        //     不把整段销毁逻辑内联到每个出口。嵌套类型（list[list[str]]）
        //     各自对应一个 mangled 名的 drop 函数，在实例缓存里去重。
        //   * D4：出口 = break / continue / ret / 作用域末，各发一份；
        //     销毁顺序定为**声明的逆序**。
        //   * D14：范围外类型（class 实例 / any / 闭包 env）P13-1 一律**不释放**
        //     （它们是引用语义，直接释放必然双重释放）；`list[Foo]` 只释放缓冲区。
        //   * D11：`core_free` 对 cap==0 静默返回，所以 str 字面量天然安全。
        //
        // drop 函数的签名统一是 `void drop_<mangle>(ptr p)`（p 是**槽位地址**，
        // 不是值），因为 list/dict 需要写回 null 给调试钩子用。

        // 类型是否有需要释放的资源。标量（int/float/bool/char/ptr）返回 false。
        [[nodiscard]] bool needsDrop(const sema::TypePtr& type) const;
        // 取（必要时生成）某个类型的 drop glue 函数名。type 不可释放时返回空串。
        std::string ensureDropGlue(const sema::TypePtr& type);
        // 发射一个 drop 函数体（按 type 的具体形状生成）。
        void emitDropGlueBody(const std::string& fnName, const sema::TypePtr& type);
        // 在**作用域出口**给该作用域里登记的所有需销毁变量发 drop 调用，
        // 顺序为声明逆序。scopeIndex 是 _scopes 的下标。
        void emitScopeDrops(size_t scopeIndex, bool nullOut);
        // 记录「这个作用域里哪些变量需要销毁」，popScope 时据此发调用。
        // addr 是该变量的槽位寻址串（`-8(%rbp)` 或 `_g_x(%rip)`）。
        void registerDropVar(const std::string& name, const std::string& addr,
                             const sema::TypePtr& type,
                             const utils::Pos& pos = utils::Pos{});
        // P13-2/L3：独立的移动状态表（F/E/X），与 _scopeDrops 平行但只登记
        // 内存模型类型变量（含 Body 别名 ClassType）。D14 下 ClassType 不进
        // drop 表，但决议 §6/§11 要求这类变量也能 take/移动，故单独跟踪。
        void registerMoveVar(const std::string& name);
        // P13-2/L3-A：在标签区里找出生地标签（能解析为 Pool 类型局部变量的名字），
        // 返回空串表示未指定（默认当前块）。
        [[nodiscard]] std::string findBirthPlaceLabel(
            const std::vector<std::shared_ptr<ast::LabelNode>>& labels) const;
        // P13-2/L3-A（决议 §2.2）：全量收集出生地标签，用于「最多一个」校验。
        [[nodiscard]] std::vector<std::string> findAllBirthPlaceLabels(
            const std::vector<std::shared_ptr<ast::LabelNode>>& labels) const;
        // P13-2/L3-B（决议 §1.4）：取 `pool.new[T](...)` 的类型实参 T（无则 nullptr）。
        [[nodiscard]] sema::TypePtr poolNewTypeArg(
            const std::shared_ptr<ast::ExpressionNode>& node) const;
        // D9 规则 C：登记一个**借用**变量（形参、或初始化自已有变量/元素）。
        // 借用不销毁，但记录在案以便检查「借用逃逸」（ret / 入容器 / 存外层）。
        // addr 同样要记下来：借用被重新赋值后会**升格成拥有者**，那时出口
        // 需要拿它的槽位地址去释放（见 setDropAlias 的说明）。
        void registerBorrow(const std::string& name, const sema::TypePtr& type,
                            const std::string& addr = std::string());
        // 这个变量是否是借用（沿作用域链由内向外找）。
        [[nodiscard]] bool isBorrowed(const std::string& name) const;
        // D9 的 H/X 两档：这个变量是否已被整体移走（X）。被移动后再使用
        // 必须编译报错 —— 那等于读一块随时会被别人释放的内存。
        [[nodiscard]] bool isVarMoved(const std::string& name) const;
        // P13-2/L3-C：变量是否处于不可直接读取状态（EMPTY/MOVED/HELD）。
        [[nodiscard]] bool isVarUnavailable(const std::string& name) const;
        [[nodiscard]] std::string bodyStateName(const std::string& name) const;
        // 借用逃逸检查：借用不能 ret、不能入容器、不能存进外层作用域变量。
        // 违反时报错并返回 false。在哪一层作用域要看 fromScope。
        bool checkBorrowNotEscaping(const std::string& name, const utils::Pos& pos,
                                    const std::string& action) const;
        // 把某个变量标记为已移动（D9 规则 C）：出口不再为它发 drop。
        //
        // D13（P13-1 保守规则）：**try 体内禁止移动外层拥有者**。原因见
        // docs/p13_plan.md D13——异常路径不保证销毁（D1=A），一旦把外层拥有者
        // 移进 try 体内的容器再抛出，接收方随栈帧一起被 longjmp 跳过，
        // 「移动」承诺的「总有一个人负责销毁」就落空了。P13-1 没有状态格
        // 来追踪跨异常边界的 H/X，只能保守禁止；P13-2 有了 F/E 状态格再放宽。
        // 违反时报错并返回 false。
        bool markVarMoved(const std::string& name, const utils::Pos& pos,
                          const std::string& action);
        // 这个变量是否**拥有**资源（= 在登记表里且不是借用）。
        // 借用（形参、模块内临时、壳子别名）不拥有资源，重新赋值时不需要
        // 先释放旧值 —— 释放了就变成释放别人的内存。
        [[nodiscard]] bool isDropOwned(const std::string& name) const;
        // 这个表达式是不是「新值」（堆上新分配出来的东西）。
        //
        // D9 规则 C 的判据，两处共用：新值 -> 拥有者；已有变量/元素/字段 ->
        // 借用（别名）。用 D10 的说法，新值就是「出生地为当前语句的匿名
        // local」，语句末必须销毁。
        [[nodiscard]] bool isFreshValue(const std::shared_ptr<ast::ExpressionNode>& node) const;

        // ---------- P13-1：D10 语句级临时值 ----------
        //
        // 规范/裁决：临时值 = 出生地为当前语句的匿名 local，销毁点 = 语句末。
        //
        // 为什么必须做：`sout("a" + "b")` 与 `take("a" + "b")` 里的拼接结果
        // 没有任何变量接住它，早先这里 alloc=1 / free=0，纯泄漏。而 str 的
        // 大部分分配恰恰来自临时值，不做这条 P13-1 对 str 基本没效果。
        //
        // 实现方式：临时实参先落进一个**专属栈槽**（不是 shadow space ——
        // 那块是调用约定借用的、调用返回后就不属于我们了），登记到
        // _stmtTemps；emitStatement 在语句末尾统一逆序销毁。
        struct StmtTemp
        {
            std::string addr;      // 栈槽地址
            sema::TypePtr type;
        };
        // 当前语句里累积的临时值。emitStatement 用「进入时的长度」做标记，
        // 结束时把标记之后的全部销毁并截断 —— 这样嵌套语句天然正确。
        std::vector<StmtTemp> _stmtTemps;
        // 把 [mark, size) 区间的临时值逆序销毁并截断到 mark。
        void drainStmtTemps(size_t mark);
        // 语句发射的实体（emitStatement 是它的包装，负责收尾销毁）。
        void emitStatementInner(const std::shared_ptr<ast::ExpressionNode>& node);
        // 把一个变量的「拥有 / 借用」身份改写掉（D9 规则 C 的状态转移）。
        //
        // 为什么需要它：规范 §11A.1 把 `x = y` 定为**壳子赋值**——两个槽共享
        // 同一块内容，只有一方是拥有者。所以赋值既要释放 x 的旧内容，又要
        // 把 x 从拥有者降级成借用；反过来 `x = <新值>` 要把它升回拥有者。
        // 只改登记表里的标记，不发任何代码。
        void setDropAlias(const std::string& name, bool borrowed);

        // 每个作用域里按声明顺序登记的「可能需要销毁」的变量。
        struct DropEntry
        {
            std::string name;
            // 登记时就把**槽位地址**定下来。不能只在销毁时按名字 lookupVar：
            // 内层同名变量会遮蔽外层，按名字查会销毁错的那个。
            std::string addr;
            sema::TypePtr type;
            bool moved = false;   // 已被整体移走（ret / 入容器），出口不销毁
            // D9 规则 C：借用（初始化表达式是已有变量/元素，或来自形参）
            // 不拥有资源，不能 ret、不能入容器、不能存进外层变量。
            bool borrowed = false;
            // 决议 C：登记点的位置，用于「条件销毁」诊断报出准确坐标。
            // 借用项不参与该诊断（borrowed=true 直接跳过），留空即可。
            utils::Pos pos;
        };
        // 与 _scopes 同长、同生命周期。popScope 时同步弹出。
        std::vector<std::vector<DropEntry>> _scopeDrops;
        // P13-2/L3：移动状态表（由内向外每层一个 map：name -> moved）。
        // P13-2/L3-C（决议 §2.2）：Body 的四态编译期事实表状态。
        // F=FULL 有内容；E=EMPTY 空壳；H=HELD take 挂起（D 阶段）；
        // X=MOVED 已移走。不是运行时元数据，无 ABI 变更。
        enum class BodyState { FULL, EMPTY, HELD, MOVED, UNKNOWN };
        // 与 _scopeDrops 同步 push/pop，但独立跟踪 ClassType（Body 别名）等
        // D14 不进 drop 表、却需要 F/X 移动语义的变量。
        std::vector<std::map<std::string, BodyState>> _moveStates;
        // 读取/设置变量的 Body 状态（由内向外找第一个同名项）。
        [[nodiscard]] BodyState getBodyState(const std::string& name) const;
        void setBodyState(const std::string& name, BodyState state,
                          const utils::Pos& pos, const std::string& action);
        // ---- P13-2/L3-G（决议 §8.1）：控制流状态合流 ----
        //
        // 事实表是**按作用域分层**的，而 setBodyState 改的是「由内向外第一个
        // 同名项」——对分支/循环体里修改的外层变量，写的就是外层那张 map。
        // 于是兄弟分支互相污染、分支出口漏报/误报。修法不是靠作用域遮蔽，
        // 而是在分支/循环的出口显式做「快照 + 格上界合并」。
        //
        // 快照必须**同时**带上 `_scopeDrops` 的 moved 位，不能只有 `_moveStates`：
        // 「已移走」这个事实在两处都有记录（P13-1 的 drop 表 + P13-2 的状态表），
        // 只回滚后者会让两者脱钩。实测（2026-11-18）：
        //   fun f(del: str): list[str] {
        //       var result: list[str] = []
        //       if core_str_len(del) == 0 { result += ["x"] ret result }   // 发散分支
        //       result += ["y"]
        //       ret result            // ← 报 'result' is in Full state ...
        //   }
        // 发散分支里的 `ret result` 把 drop 表的 moved 置真，分支出口只回滚了
        // 状态表，于是状态是 Full、drop 表却说已移走 —— isVarUnavailable 回退
        // 到 drop 表判定为「不可用」，报出来的状态名却是 Full，自相矛盾。
        struct MoveStateSnapshot {
            std::vector<std::map<std::string, BodyState>> states;
            // 与 _scopeDrops 同形：每层每项的 (moved, borrowed) 两位。
            // borrowed 也必须进快照：`x = <新值>` 会把借用升格成拥有者
            // （setDropAlias 同时改这两位），不跟着回滚就会让「入口是借用、
            // 循环体里升格成拥有者」的两条路径在汇合点脱钩。
            struct DropBits {
                bool moved = false;
                bool borrowed = false;
            };
            std::vector<std::vector<DropBits>> drops;
        };
        // 状态格上界：相同保持不变；F ⊔ E、F ⊔ X、E ⊔ X 一律 ?；
        // 与 ? 合流仍是 ?。H 是 F 的子态（内容位置没动），H ⊔ F = F。
        [[nodiscard]] static BodyState joinBodyStates(BodyState a, BodyState b);
        [[nodiscard]] MoveStateSnapshot snapshotMoveStates() const;
        void restoreMoveStates(const MoveStateSnapshot& snap);
        // 把若干条路径的出口快照按格上界逐层合并后写回 _moveStates。
        void joinMoveStatePaths(const std::vector<MoveStateSnapshot>& paths);
        // 决议 C：出口处的「条件销毁」诊断。凡该作用域登记了销毁项、而它的
        // 壳子状态在出口是 `?`（各条控制流路径不一致），就意味着这个所有者
        // 可能在块结束时仍持有内容 —— 唯一所有权不允许，默认是编译错误。
        // 放行需同时满足：命令行 --allow-cond-destroy 且文件里有
        // `// @allow-cond-destroy` 指令（两者缺一仍报错）。
        void checkCondDestroyAtExit(const size_t scopeIndex);
        // 命令行开关：启用「扫描文件级放行指令」。
        bool _allowCondDestroy = false;
        // 文件级放行指令是否真的被扫描到。
        bool _condDestroyDirectiveSeen = false;
        // 只算合并结果、不写回（循环要先把回边/出口各自合并再取上界）。
        // 深度取所有路径的最小值：更深层是分支/循环体内的局部量，不参与。
        [[nodiscard]] MoveStateSnapshot
        joinMoveStateSnapshots(const std::vector<MoveStateSnapshot>& paths) const;
        // 该语句之后是否不可能再往下走（ret/throw/break/continue 及其块/if 包装）。
        // 发散的分支到不了汇合点，不参与状态合流。
        [[nodiscard]] static bool statementDiverges(const std::shared_ptr<ast::ExpressionNode>& node);
        // 该分支条件是否是 parser 给 `else` 合成的字面 true（= 无条件 else）。
        [[nodiscard]] static bool isAlwaysTrueCondition(const std::shared_ptr<ast::ExpressionNode>& cond);
        // 循环的出口（break）/回边（continue）路径快照，与 _loopStack 同生命周期。
        // 决议 §8.1：循环后状态 = 进入前状态 ⊔ 回边状态 ⊔ break 出口状态。
        std::vector<std::vector<MoveStateSnapshot>> _loopExitStates;
        std::vector<std::vector<MoveStateSnapshot>> _loopBackStates;
        // 入口状态被故意记成 `?` 的变量名（目前只有 Take 形参，决议 §9.2）。
        // 这种 `?` 的含义是「编译器不做承诺」，不是「控制流合流后不确定」——
        // 函数体里对 Take 形参读字段/调方法/take 都是合法的（§7.3 独占取内容）。
        // 真正因合流而变成 `?` 的变量不进这个集合，会被 §3.4 拒绝。
        std::set<std::string> _opaqueMoveVars;
        // 该变量当前是否是「因控制流合流而不确定」的 `?`（要按 §3.4 拒绝直接访问）。
        // Take 形参入口那种不透明 `?` 不算，除非它已经被推到过确定状态。
        [[nodiscard]] bool isMergedUnknown(const std::string& name) const
        {
            return getBodyState(name) == BodyState::UNKNOWN && _opaqueMoveVars.find(name) == _opaqueMoveVars.end();
        }
        // 合流之后重新判定「不透明」：某条路径上被改动过的，就变成真正的 `?`。
        void refreshOpaqueAfterMerge(const std::vector<MoveStateSnapshot>& paths);
        // P13-2/L3-A（决议 §2）：变量 -> 出生地 Pool 变量名（空=当前块默认出生地）。
        // A 阶段只记录不检查生命周期；完整 R1/R2 检查待后续。
        std::map<std::string, std::string> _birthPlaces;
        // P13-2/L3-D（决议 §3.1）：当前语句内被 take 挂起的 Body 变量名集合。
        // 不是事务、不是运行时元数据；emitStatement 末统一结算。
        std::set<std::string> _pendingTakes;
        // P13-2/L3-E（决议 §4）：活跃子引用事实表。每项是一条路径及其
        // 根对象变量名（owner）。当前实现是作用域级保守近似（owner 有
        // 活跃引用即视为冲突），精确 NLL「活跃到最后使用」待后续。
        struct ActiveRef {
            std::string path; std::string owner; std::string refVar;
            bool justBound = false;
            // P13-2/L3-E（决议 §4.3）：动态下标表达式文本（get_mut(i) 的 i）。
            // 空 = 静态精确路径（如 &p.name）。
            std::string indexExpr;
            // P13-2/L3-D（决议 §4.2）：区间视图的边界文本（split_at / split_from）。
            // 空 = 非区间视图。hiText 为 "len" 表示无上界（split_from 的尾段）。
            // 只有两段能**证明**不相交时才允许共存，见 rangesDisjoint。
            std::string loText, hiText;
        };
        std::vector<ActiveRef> _activeRefs;
        // P13-2/L3-F（决议 §4.2 / §9.3）：函数写效果表。
        // 决议要求「调用时按 writes / from 更新活跃集合」—— 修前 writes 只在
        // 泛型定义处做形态校验，调用点从不读它，于是下面这些全部静默通过：
        //   fun w(p: Player) writes[p.name] { p.name = "y" }
        //   var r = &p.name; w(p)          // r 仍活跃，p.name 被替换 → 应报错
        //   var r = &p.name; p.name = "y"  // 同上（本地写，§4.1 前缀冲突）
        //   var r = &p.name; p.take()      // take ≡ 根替换 → 应报错
        // root 是形参名，suffix 是它之后的路径（""=整个形参，".name"=该字段）。
        struct WriteEffect {
            std::string root;
            std::string suffix;
        };
        // 键是**发射名**（重载已 mangling），与 _functionParamNames 同键。
        std::map<std::string, std::vector<WriteEffect>> _writeEffects;
        // 决议 §9.4：函数返回引用的来源表（from 子句解析结果，按发射名）。
        // 空向量 = 函数返回引用但没写 from → 保守视为可能来自任一引用形参。
        std::map<std::string, std::vector<WriteEffect>> _fromEffects;
        // 登记函数签名里的 writes / from 效果（发射函数体前由签名预登记调用）。
        void registerSignatureEffects(const std::string& emitName,
                                      const std::shared_ptr<ast::FunctionDefinitionNode>& def,
                                      const std::vector<std::string>& paramNames);
        // 把形参名 + 路径后缀解析成调用点的实际路径（"p.name" / "a" / ""=无法解析）。
        [[nodiscard]] std::string resolveEffectPath(const std::string& root,
                                                    const std::string& suffix,
                                                    const std::vector<std::string>& paramNames,
                                                    const std::vector<std::shared_ptr<ast::ExpressionNode>>& args) const;
        // 取表达式的路径文本（Identifier / DOT 链）；无法解析返回空串。
        [[nodiscard]] static std::string exprPathText(const std::shared_ptr<ast::ExpressionNode>& node);
        // 决议 §4.1：P 是 Q 的前缀（按路径段，含相等）即冲突。
        [[nodiscard]] static bool pathIsPrefixOf(const std::string& p, const std::string& q);
        // 检查「写路径 P」是否与某个活跃子引用冲突；冲突则报错。
        [[nodiscard]] bool checkWriteConflicts(const std::string& path,
                                               const utils::Pos& pos,
                                               const std::string& what);
        // 调用点应用被调函数（发射名 emitName、显示名 displayName）的 writes 效果。
        [[nodiscard]] bool applyWriteEffects(const std::string& emitName,
                                             const std::string& displayName,
                                             const std::vector<std::shared_ptr<ast::ExpressionNode>>& args,
                                             const utils::Pos& pos);
        // P13-2/L3-D（决议 §4.2）：内置列表增长例程（listAppend 等）会移动
        // 元素缓冲区，与活跃的元素子引用冲突。泛型调用不走 applyWriteEffects，
        // 故在调用入口按名字单独拦一道。
        [[nodiscard]] bool checkGrowthEffects(const std::string& name,
                                              const std::vector<std::shared_ptr<ast::ExpressionNode>>& args,
                                              const utils::Pos& pos);
        // 登记子引用；检查 owner 是否有活跃子引用（冲突时报错）。
        void registerActiveRef(const std::string& path, const std::string& owner,
                               const std::string& refVar, const utils::Pos& pos,
                               const std::string& indexExpr = "",
                               const std::string& loText = "",
                               const std::string& hiText = "");
        // P13-2/L3-D（决议 §4.2）：两段区间视图是否**可证明**不相交。
        // 只有 [0,k) 与 [k,len) 这种「同一下界文本相接」才算证明；其余保守冲突。
        [[nodiscard]] static bool rangesDisjoint(const std::string& loA, const std::string& hiA,
                                                 const std::string& loB, const std::string& hiB);
        // P13-2/L3-D：区间视图登记前的冲突检查（能证明不相交则放行）。
        [[nodiscard]] bool hasConflictingRangeRef(const std::string& owner,
                                                  const std::string& loText,
                                                  const std::string& hiText) const;
        // P13-2/L3-E：把最后登记的子引用的 refVar 补成绑定变量名
        // （var r = &p.name 里的 r），供 NLL 追踪最后使用。
        void bindLastActiveRef(const std::string& refVar);
        // NLL：语句发射后移除「最后使用」已过的引用（本语句用到则保留到下一语句）。
        void drainActiveRefs(const std::shared_ptr<ast::ExpressionNode>& stmt);
        [[nodiscard]] bool hasActiveRefOn(const std::string& owner) const;
        // P13-2/L3-E（决议 §4.3）：带动态下标的冲突检查。
        // 两个不同的整数字面量下标（如 0 与 1）证明 i≠j，不冲突；
        // 相同下标或非字面量保守视为冲突。
        [[nodiscard]] bool hasActiveRefOn(const std::string& owner,
                                          const std::string& indexExpr) const;
        // take 挂起一个 Body（加入 pending 并置 HELD）；重复 take 报错。
        void beginPendingTake(const std::string& name, const utils::Pos& pos);
        // P13-2/L3-D（决议 §6.4）：挂起期间禁止 fill/clear/release。
        [[nodiscard]] bool checkNotPending(const std::string& name,
                                           const utils::Pos& pos, const std::string& action);
        // 落定：从 pending 移除并置 E（EMPTY）。
        void settlePendingTake(const std::string& name);
        // 结算当前语句的 pending_take：未落定的回填 F（决议 §3.1）。
        void drainPendingTakes();
        // drop glue 实例缓存：mangled 类型名 -> 函数标签
        std::map<std::string, std::string> _dropGlue;
        // 待发射的 drop 函数（类型 + 标签），函数体在 emitProgram 末尾统一发射
        std::vector<std::pair<std::string, sema::TypePtr>> _pendingDropGlue;
        // 正在生成的 drop 函数名（防止递归类型无限展开）
        std::set<std::string> _dropInProgress;

        // 列表字面量 `[a, b, c]`
        void emitListExpression(const std::shared_ptr<ast::ExpressionNode>& node);
        // 字典字面量 `{k: v, ...}`
        void emitDictionaryExpression(const std::shared_ptr<ast::ExpressionNode>& node);
        // 索引访问 `x[i]` / `d[k]`
        // P12/G3 + G4：`str + str` / `list + list` 的拼接（emitBinary 分派过来）。
        // 两个参数是栈槽地址串，结果放 %rax。
        void emitStrConcat(const std::string& lhsSlot, const std::string& rhsSlot);
        void emitListConcat(const std::string& lhsSlot, const std::string& rhsSlot);
        // 读取容器（str / list / dict）长度头到临时槽，**null 视为长度 0**。
        //
        // 为什么不能直接 `movq (%rax), %rax`：Rio 里 `var s: str` 未初始化、
        // 以及 `ds.top([])` 的空列表分支拿到的都是 null 指针。str/list 的长度头
        // 就在偏移 0，裸解引用 null 直接段错误（P12-3-D 实测：`"" + null`、
        // `null == ""`、`null < "a"` 全部 exit -1073741819）。core_str_len 早就
        // 把 null 定义成空串（返回 0），codegen 内联的这几个运算符必须一致。
        void emitContainerLenTo(const std::string& containerSlot, const std::string& dstSlot);
        // P12/G9：`str == str` 的内容比较（emitBinary 分派过来）。结果放 %rax（0/1）。
        void emitStrEq(const std::string& lhsSlot, const std::string& rhsSlot);
        // `str < str` / `<=` / `>` / `>=` 的字典序比较（emitBinary 分派过来）。
        // 之前这四个运算符直接 cmpq 比指针，结果与内容无关。
        // 第三个参数是 core::TokenType 的整数值（不在此处引入 rcc_core.h，
        // 保持后端头文件对前端 token 定义的解耦）。
        void emitStrOrder(const std::string& lhsSlot, const std::string& rhsSlot, int op);
        void emitIndexExpression(const std::shared_ptr<ast::ExpressionNode>& node);

        // 把容器字面量的体拆成元素列表。
        // 体是逗号连接的中缀链（PARALLEL），空字面量的体是 nullptr。
        void flattenListElements(const std::shared_ptr<ast::ExpressionNode>& body,
                                 std::vector<std::shared_ptr<ast::ExpressionNode>>& out) const;
        // 把字典字面量的体拆成 (键, 值) 对。键值对是冒号中缀（PAIR）。
        void flattenDictPairs(const std::shared_ptr<ast::ExpressionNode>& body,
                              std::vector<std::pair<std::shared_ptr<ast::ExpressionNode>,
                                                    std::shared_ptr<ast::ExpressionNode>>>& out) const;
        // 字典**字面量键**的编译期比较键（空串 = 判不出来，调用方应保留原样）。
        //
        // 用于 `{"a":1, "a":2}` 的去重：返回能唯一刻画这个键的字符串，
        // 使得「内容相同的两个 str 字面量」得到同一个值（运行时查找按内容比，
        // 但两个同内容字面量在常量池里是不同指针，不能比指针）。
        //   字符串字面量 -> "s:" + 内容
        //   整数字面量   -> "i:" + 十进制
        //   字符字面量   -> "c:" + 字节值
        //   布尔字面量   -> "b:" + 0/1
        // null          -> "n:"
        // 其余（变量、调用、复杂表达式）返回空串 —— 无法静态去重。
        [[nodiscard]] static std::string dictLiteralKeyText(
            const std::shared_ptr<ast::ExpressionNode>& keyNode);
        // 索引表达式里的键：IndexExpressionNode 的 indexNode 是 BracketExpressionNode，
        // 真正的键在它的 body 里，需要剥掉这层壳。
        [[nodiscard]] static std::shared_ptr<ast::ExpressionNode>
        unwrapIndexKey(const std::shared_ptr<ast::ExpressionNode>& indexNode);

        // 分配堆对象：core_alloc(size, 8)，结果留在 %rax。
        void emitAlloc(size_t size);
        // 借一个临时栈槽保存中间值（跨表达式求值时保护指针/键）。
        // 槽位名唯一，不会和用户变量冲突；返回形如 "-16(%rbp)" 的寻址串。
        [[nodiscard]] std::string allocateTemp(const sema::TypePtr& type);

        // sout 特化
        void emitSoutCall(const std::shared_ptr<ast::ExpressionNode>& node);
        // sout 的单个实参：按静态类型分派到 rt_print_*
        void emitSoutArg(const std::shared_ptr<ast::ExpressionNode>& arg);
        // 调用一个 rt 打印函数：把值放在约定的位置后 call
        void emitRtPrintCall(const std::string& fnName, bool isFloat);

        // ---------- 作用域 ----------
        struct VarSlot
        {
            sema::TypePtr type;
            long long stackOffset = 0;   // 相对 %rbp 的偏移（负值）
            // P12/G1a：顶层 `var` 不再占栈槽，而是放进 .bss 的全局槽。
            // 非空时 stackOffset 无意义，取地址一律走 addrOf()。
            std::string globalLabel;
            [[nodiscard]] bool isGlobal() const { return !globalLabel.empty(); }
        };

        void pushScope();
        void popScope();
        [[nodiscard]] VarSlot* lookupVar(const std::string& name);
        long long allocateSlot(const std::string& name, const sema::TypePtr& type, size_t size);
        // P12/G1a：全局槽。返回 .bss 标签名。
        std::string allocateGlobal(const std::string& name, const sema::TypePtr& type, size_t size);
        // 把一个槽变成可直接嵌进指令的操作数地址（`-8(%rbp)` 或 `_g_x(%rip)`）。
        [[nodiscard]] static std::string addrOf(const VarSlot& slot);

        // ---------- 辅助 ----------
        [[nodiscard]] sema::TypePtr inferType(const std::shared_ptr<ast::ExpressionNode>& node) const;
        // P13-2/L2(Q-C)：取块/分支最后一条语句的推断类型（末值语义）。
        // P13-2/L3(决议 §6)：识别 `X.take()` 形态，返回接收者变量名（空=不是）。
        [[nodiscard]] static std::string takeReceiverName(const std::shared_ptr<ast::ExpressionNode>& node);
        // P13-2/L3-B：识别 `pool.new[T](...)` 形态，返回池变量名（空=不是）。
        [[nodiscard]] static std::string poolNewReceiverName(const std::shared_ptr<ast::ExpressionNode>& node);
        [[nodiscard]] sema::TypePtr inferBlockValueType(const std::shared_ptr<ast::ExpressionNode>& body) const;
        // P13-2/L2：块内是否含 `ret value`（非 return），用于值出口校验分流。
        [[nodiscard]] bool bodyHasRetValue(const std::shared_ptr<ast::ExpressionNode>& body) const;
        // P5：容器字面量与索引访问的静态类型推断。
        // 元素/值类型只影响编译期分派（sout 选 rt_print_*、槽位大小），
        // 运行时槽位恒为 8 字节，所以异构字面量可以安全地退化成 int。
        [[nodiscard]] sema::TypePtr inferListLiteralType(const std::shared_ptr<ast::ExpressionNode>& node) const;
        [[nodiscard]] sema::TypePtr inferDictLiteralType(const std::shared_ptr<ast::ExpressionNode>& node) const;
        [[nodiscard]] sema::TypePtr inferIndexResultType(const std::shared_ptr<ast::ExpressionNode>& node) const;
        // 推断匿名函数的返回类型：临时把形参放进一个影子作用域，
        // 再从函数体里找第一个 ret 的表达式类型。
        [[nodiscard]] sema::TypePtr inferAnonReturnType(const std::shared_ptr<ast::ExpressionNode>& body,
                                                        const std::vector<std::string>& paramNames,
                                                        const std::vector<sema::TypePtr>& paramTypes);
        // 在函数体里递归找 ret 的表达式类型（找不到返回 TK_UNKNOWN）
        [[nodiscard]] sema::TypePtr findReturnType(const std::shared_ptr<ast::ExpressionNode>& body) const;
        // P13-2/L3-F：当前正在发射的函数的签名子句（emitFunctionBody 读取）。
        std::vector<std::shared_ptr<ast::LabelNode>> _currentSignatureClauses;
        // P13-2/L3-F：解析后的 ensures 标记（形参名 -> 期望状态）。
        std::map<std::string, BodyState> _ensuresMarks;
        // 出口检查（决议 §9.2：逐出口验证，throw 不检查）。
        void checkEnsuresAtExit(const utils::Pos& pos);
        // P13-2/L3-F（决议 §9.2）：ensures 语法校验——只能列 Take[T] 形参。
        // P13-2/L3-H：把前缀标记（`!` / `?` / `+`）当继承状态逐项收集。
        // parser 让前缀优先级低于逗号，`ensures[!t, !t]` 实际是 `!(t, !t)`，
        // 不能直接 flattenTypeArgs（那样只会看到一个节点，整条子句被静默丢弃）。
        void collectEnsuresItems(const std::shared_ptr<ast::ExpressionNode>& node,
                                 std::optional<BodyState> inherited,
                                 std::vector<std::pair<std::string, std::optional<BodyState>>>& out);
        void validateEnsuresClauses(const std::vector<std::string>& paramNames,
                                    const std::vector<sema::TypePtr>& paramTypes);
        // P13-2/L3-F（决议 §9.3/§9.4）：writes / from 子句的每一项必须是形参路径。
        void validateWritesFromClauses(const std::vector<std::string>& paramNames);
        // 取子句项（p / p.name / p[i] / &p.name）的根标识符。
        [[nodiscard]] static std::string clauseItemRoot(
            const std::shared_ptr<ast::ExpressionNode>& item);
        // 本函数 `from` 声明允许的引用来源根（决议 §9.4：返回引用 ⊆ from）。
        std::set<std::string> _fromRoots;
        // P13-2/L3-F（决议 §5.2）：判断名字是否当前函数形参。
        [[nodiscard]] bool isFunctionParam(const std::string& name) const;
        [[nodiscard]] std::string currentFunctionName() const { return _functionName; }
        void fail(const std::string& msg);
        // P9：带位置信息的诊断。pos 为空时退化成 fail(msg)。
        // 位置前缀形如 `file.rio:12:5: `（不带绝对路径，用 briefString）。
        void failAt(const utils::Pos& pos, const std::string& msg);
        // P9：把节点位置拼成诊断前缀。拿不到有效位置时返回空串。
        [[nodiscard]] static std::string posPrefix(const utils::Pos& pos);
        // P9：类型兼容性判定（宽松，只挡真正的错配）。
        // 返回空串表示可赋值；否则返回一句人类可读的原因。
        [[nodiscard]] std::string checkAssignable(const sema::TypePtr& target,
                                                  const sema::TypePtr& source) const;
        // P9：两个类型是否「同一族」，用于 ret / 实参等只关心值类别的场合。
        [[nodiscard]] bool sameValueFamily(const sema::TypePtr& a, const sema::TypePtr& b) const;
        // 表达式是否是**空容器字面量**（`[]` / `{}`，包括带括号的 `([])`）。
        //
        // 这类字面量不携带元素类型信息：inferListLiteralType 把空列表兜底成
        // list[int]，那是「推不出来」而不是用户的承诺。类型校验遇到它必须
        // 放行，否则 `var r: list[str] = []`、`f([])` 这类合法写法会被误报。
        [[nodiscard]] static bool isEmptyContainerLiteral(
            const std::shared_ptr<ast::ExpressionNode>& node);
        // P9：判断标注里是否真的写了类型（`var x` 与 `var x: int` 的区别）。
        // 注解解析不出来时返回 TK_UNKNOWN，由调用方决定是报错还是忽略。
        [[nodiscard]] sema::TypePtr typeFromLabels(const std::vector<std::shared_ptr<ast::LabelNode>>& labels) const;
        [[nodiscard]] bool hasFailed() const { return !_errorMsg.empty(); }

        // 从 AST 节点提取文本
        [[nodiscard]] static std::string nodeText(const std::shared_ptr<ast::ExpressionNode>& node);

        const AbiInfo& _abi;
        AsmEmitter _asm;
        std::string _errorMsg;
        std::string _functionName;
        // 库模式：不发射 main / __rio_main
        bool _libraryMode = false;
        // P8：模块名集合。`mod.fn(...)` 里的 mod 命中这里时，
        // 调用退化成直接调用 fn —— 模块已在加载期内联。
        std::vector<std::string> _moduleNames;

        // 作用域栈
        std::vector<std::map<std::string, VarSlot>> _scopes;
        // 当前帧已分配的局部变量字节数
        size_t _frameSize = 0;
        // P12/G1a：顶层 `var` 的全局槽。name -> .bss 标签。
        std::map<std::string, std::string> _globalSlots;
        // 全局槽的类型表。不能塞进 _scopes：emitFunctionBody 每次都会
        // _scopes.clear()，全局变量必须活得比函数帧更久。
        std::map<std::string, VarSlot> _globals;
        // 需要在 .bss 里声明的 (标签, 字节数)，按首次登记顺序。
        std::vector<std::pair<std::string, size_t>> _globalDecls;
        // 顶层 var 的名字集合 —— emitProgram 用它把顶层 `var x = 1` 从
        // 「局部变量定义」改判成「写全局槽」。
        // 光看名字不够：函数体里的同名局部 `var x` 必须仍然分配栈槽并
        // 遮蔽全局，所以还要配合 _inTopLevelStatements 判断。
        std::set<std::string> _topLevelVarNames;
        // 正在发射 __rio_main 的顶层语句（而不是任何函数体）。
        bool _inTopLevelStatements = false;
        // 当前函数的返回标签
        std::string _returnLabel;
        // P9：当前函数的返回类型 + 是否来自显式标注。
        // 只有显式标注时才校验 ret（推断类型来自 ret 自身，自证无意义）。
        sema::TypePtr _functionReturnType;
        bool _functionReturnAnnotated = false;
        // 决议 A：当前函数返回 struct 时，调用方传入的返回缓冲区。
        // 存的是**装着指针的栈槽**（`-N(%rbp)`），与 emitStructAddrOf 的
        // 约定一致。非 struct 返回时为空。
        //
        // 为什么必须由调用方提供：struct 的值是一块内存，如果被调方在自己
        // 的帧里造一个临时槽再返回它的地址，ret 之后那块栈马上被调用方
        // 的下一层调用覆盖 —— 返回的 struct 会读到垃圾。
        std::string _structRetBufSlot;
        // 当前函数是否把第一个整型寄存器让给了隐藏的返回缓冲区指针。
        bool _hasStructRetBuf = false;
        // 循环栈：{ continue 目标标签, break 目标标签 }
        struct LoopContext
        {
            std::string continueLabel;
            std::string breakLabel;
            // 进入循环体时的 try 帧深度：break/continue 只需要弹出
            // 循环体内部建立的帧，循环外面的帧要留着。
            size_t ehDepth = 0;
            // P13-1（D4）：进入循环体时的销毁登记深度。break / continue 是
            // 终结性跳转，跳出后**不会**再走到循环体的词法出口，所以必须在
            // 跳转前把这一区间里登记的拥有者销毁。只销毁「循环体这一层」
            // 的变量 —— 循环外层的变量由外层出口负责。
            size_t dropDepth = 0;
            // P13-2/L2(Q-C)：带 `: T` 标注的循环，break/ret value 存入此槽，
            // 循环出口统一从槽读出；空串表示无标注（break 不许带值）。
            std::string valueSlot;
            std::shared_ptr<sema::Type> annotatedType;
            // P13-2/L2：进入作用域的全局序号，用于与 _ifStack 比较谁更近。
            size_t enterSeq = 0;
        };
        std::vector<LoopContext> _loopStack;
        // P13-2/L2(Q-C)：带 `: T` 标注的 if 分支，ret value 存入值槽，
        // if 出口统一从槽读出。结构与 LoopContext 对齐，共用 enterSeq 判最近。
        struct IfContext
        {
            std::string endLabel;
            std::string valueSlot;
            std::shared_ptr<sema::Type> annotatedType;
            size_t dropDepth = 0;
            size_t ehDepth = 0;
            size_t enterSeq = 0;
        };
        std::vector<IfContext> _ifStack;
        // P13-2/L3-N：块表达式（右值位置的裸块）的上下文。
        // 决议 §12.1 把「裸块 { ... }」列为可返回值作用域，§13.1 的 `ret`
        // 退「最近的可返回值作用域」。语句位置的裸块不算（§15.1：块内
        // ret/break/continue 按外层上下文生效），所以只有值位置的块入栈。
        struct BlockContext
        {
            std::string endLabel;
            std::string valueSlot;
            std::shared_ptr<sema::Type> annotatedType;
            size_t dropDepth = 0;
            size_t ehDepth = 0;
            size_t enterSeq = 0;
        };
        std::vector<BlockContext> _blockStack;
        size_t _scopeEnterSeq = 0; // 单调递增，用于判定最近的可返回值作用域

        // 已知函数签名：名字 -> 返回类型
        std::map<std::string, sema::TypePtr> _functionReturns;
        // 已知函数形参类型：名字 -> 每个形参的静态类型。
        // 调用点用它决定实参走 %rcx 还是 %xmm0，并在必要时插入转换。
        std::map<std::string, std::vector<sema::TypePtr>> _functionParams;
        // P9：每个形参是否写了类型标注（按位置）。
        // 未标注的形参在 _functionParams 里是回退出来的 int，
        // 调用点不能用它校验实参，否则 `fun apply(f, x)` 会被误报。
        std::map<std::string, std::vector<bool>> _functionParamAnnotated;
        // P8：形参名（按位置）与默认值（名字 -> 默认值表达式）。
        // 命名实参要按名字映射回位置，缺省实参要在调用点补上默认值，
        // 这两件事都离不开它们。
        std::map<std::string, std::vector<std::string>> _functionParamNames;
        std::map<std::string, std::map<std::string, std::shared_ptr<ast::ExpressionNode>>>
            _functionDefaults;
        // 把命名实参按形参位置重排，并在末尾补齐缺省实参的默认值表达式。
        // 没有命名实参、也没有缺省时原样返回（既有调用路径完全不变）。
        [[nodiscard]] std::vector<std::shared_ptr<ast::ExpressionNode>>
        normalizeCallArgs(const std::vector<std::shared_ptr<ast::ExpressionNode>>& args,
                          const std::string& fname) const;
        // 变长形参（`*args` / `**kwargs`）的存在性。
        // 它们不进入 paramNames / paramTypes，所以「这个函数收不收多余实参」
        // 必须单独登记，否则实参个数校验无从判断。
        struct VariadicInfo
        {
            size_t fixedCount = 0;      // 非变长形参的个数
            bool positional = false;    // 有 `*args`
            bool keyword = false;       // 有 `**kwargs`
        };
        std::map<std::string, VariadicInfo> _functionVariadic;
        // 调用点实参个数校验。只对已登记签名的函数生效；不认识的名字直接放过。
        // 变长形参只是「接受多余实参」，**不等于**实现了收集语义 —— 收集
        // 仍是待设计项（见 docs/p12_plan.md G1b）。
        void checkCallArity(const std::string& fname,
                            const std::vector<std::shared_ptr<ast::ExpressionNode>>& args,
                            const utils::Pos& pos,
                            const std::string& displayName);
        // 已登记但实现为 encapsulated 的函数（不发射函数体）
        std::map<std::string, bool> _encapsulatedFunctions;
        // 用户是否自己写了 fun main()（发射为 __rio_user_main）
        bool _hasUserMain = false;
        // 浮点字面量池：值 -> 标签
        std::map<double, std::string> _floatPool;
        std::vector<std::pair<std::string, double>> _floatLiterals;

        // ---------- P4 状态 ----------
        // 待发射的匿名函数体。匿名函数出现在表达式位置，但函数体必须
        // 放在函数外发射，所以先排队，等外层函数发完再统一吐出来。
        struct PendingFunction
        {
            std::string emitName;
            std::vector<std::string> paramNames;
            std::vector<sema::TypePtr> paramTypes;
            sema::TypePtr retType;
            std::shared_ptr<ast::ExpressionNode> bodyNode;
            // 闭包捕获的变量（按值拷贝，序言里从 env 块装进本地槽位）
            std::vector<std::string> capturedNames;
            std::vector<sema::TypePtr> capturedTypes;
        };
        std::vector<PendingFunction> _pendingFunctions;
        // 静态闭包对象：{ fnptr, env=0 }，放在 .data 里，值就是它的地址。
        struct ClosureObject
        {
            std::string label;
            std::string fnLabel;
        };
        std::vector<ClosureObject> _closureObjects;
        // 匿名函数命名计数：anon_func_1, anon_func_2 ...
        size_t _anonCounter = 0;
        // 具名函数名 -> 它的闭包对象标签（函数名当函数值用）
        std::map<std::string, std::string> _namedClosures;

        // ---------- P5 状态 ----------
        // 临时槽位计数：allocateTemp 用它造出不会与用户变量重名的槽位名。
        size_t _tempCounter = 0;

        // ---------- P6：类 ----------
        // 对象布局：扁平的 8 字节槽数组，第 i 个字段在 +8*i；基类字段排在
        // 派生类字段之前，所以派生类对象天然可以当基类用（前缀兼容）。
        // 对象没有头部（不记 rc/类型），因为 P6 不做 GC。
        struct ClassInfo
        {
            std::string name;
            std::string baseName;                        // 空 = 无基类
            // 本类自己声明的字段（登记顺序）
            std::vector<std::string> ownFieldNames;
            std::map<std::string, sema::TypePtr> ownFieldTypes;
            // P12/G11：实例字段的初始化器（`var x: int = 42`），按声明顺序。
            // 原仓库 datatype.rio:50 `var TP_INT: str static = type(0)`、
            // ds.rio:492 `compareHandler: func[...] private = null` 都靠它。
            // static 字段不进这里（见下面的 staticFields）。
            std::vector<std::pair<std::string, std::shared_ptr<ast::ExpressionNode>>> ownFieldInits;
            // 生命周期标签 `static` 修饰的字段：**不属于实例布局**，全类共享一份。
            //
            // 原仓库 tests/test.rio 的 `var _number: int static = 0` 就是这种：
            // 每个 ID 实例构造时 `this._number += 1`，靠共享槽递增，
            // 于是各实例拿到 1、2、3…… 修前它被当成普通实例字段，
            // 每个实例各自从 0 开始，输出恒为 1 1 1。
            //
            // 名字 -> 类型；初始化器在 registerClass 阶段单独发进全局槽。
            std::map<std::string, sema::TypePtr> staticFieldTypes;
            // static 字段的全局槽符号（`<Class>.<field>` -> asm label）
            std::map<std::string, std::string> staticFieldSymbols;
            // static 字段的初始化器（在程序启动、任何用户代码之前跑一次）
            std::vector<std::pair<std::string, std::shared_ptr<ast::ExpressionNode>>> staticFieldInits;
            // computeLayout 填出来的完整布局（含继承来的字段，基类在前）
            std::vector<std::string> fieldOrder;
            std::map<std::string, long long> fieldOffsets;
            std::map<std::string, sema::TypePtr> fieldTypes;
            long long size = 0;                          // 对象字节数
            bool layoutDone = false;
            // 构造器（可缺省）
            bool hasCtor = false;
            std::shared_ptr<ast::ExpressionNode> ctorParamNode;
            std::shared_ptr<ast::ExpressionNode> ctorBodyNode;
            // 方法名 -> 定义
            std::map<std::string, std::shared_ptr<ast::FunctionDefinitionNode>> methods;
            // P12/G5：方法重载集。`A.m(x: int)` 与 `A.m(x: str)` 都能保留。
            // 单定义的方法**保持裸名**（与全局函数同一判据），只有 ≥2 个定义
            // 的方法才修饰成 `<Class>.m_R<len><形参编码>`。
            std::map<std::string, std::vector<std::shared_ptr<ast::FunctionDefinitionNode>>> methodOverloads;
        };
        // 方法解析结果：定义 + 真正声明它的类（发射符号要用 owner）
        struct MethodRef
        {
            std::string owner;
            std::shared_ptr<ast::FunctionDefinitionNode> def;
        };

        // ---------- 决议 A（2026-11-18）：值语义 struct ----------
        //
        // 与 class 的根本区别：struct **不是指针**，它的值就是那几块字节本身。
        // 因此布局是「按声明顺序把字段内联排开」，没有头部、没有 8 字节对齐
        // 到槽的浪费，`var b = a` 是**逐字节复制**（两个独立对象），而 class
        // 的 `var b = a` 只是复制指针（两个壳子共享同一对象）。
        //
        // 为什么字段限定为纯标量（int/float/bool/char）：struct 的复制是
        // 浅拷贝。若允许 str（指针+长度）或 list/dict/class，复制后两份都
        // 指向同一块堆内存，谁负责释放就成了悬案 —— 要么双重释放，要么
        // 泄漏。纯标量没有堆资源，浅拷贝天然安全，所以也不需要登记 drop
        // （needsDrop 对 struct 恒为 false）。
        struct StructInfo
        {
            std::string name;
            // 字段按**声明顺序**排列；偏移是「前面所有字段大小之和」。
            std::vector<std::string> fieldNames;
            std::map<std::string, long long> fieldOffsets;
            std::map<std::string, sema::TypePtr> fieldTypes;
            // 字段默认值（`var x: int = 1`），按声明顺序收集；构造时先按位置
            // 写构造器实参，没覆盖到的字段用默认值，再没有就零初始化。
            std::vector<std::pair<std::string, std::shared_ptr<ast::ExpressionNode>>> fieldDefaults;
            long long size = 0;                          // 对象字节数
            bool layoutDone = false;
        };
        // 阶段 A：登记 struct 名与字段（不发射指令）。
        void registerStruct(const std::shared_ptr<ast::StructDefinitionNode>& node);
        // 阶段 B：算字段偏移与总大小，顺带校验「字段只能是纯标量」。
        void computeStructLayout(const std::string& name);
        StructInfo* findStruct(const std::string& name);
        const StructInfo* findStruct(const std::string& name) const;
        // struct 字段偏移；找不到返回 -1。
        long long findStructFieldOffset(const std::string& structName,
                                        const std::string& field) const;
        // 把 `Pt` 这样的名字解析成 StructType（不是 struct 就返回 nullptr）。
        sema::TypePtr structTypeOf(const std::string& name) const;

        // 阶段 A：只登记类名、基类、字段、方法，不发射任何指令。
        void registerClass(const std::shared_ptr<ast::ClassDefinitionNode>& node);
        // 生命周期标签 `static` 的字段：分配 .bss 全局槽。
        // 必须在 computeLayout 之后调用（那时类表已齐，能沿基类链查）。
        void emitStaticFieldSlots();
        // static 字段的初始化器：在用户代码之前跑一次（只发字面量初值）。
        void emitStaticFieldInitCode();
        // P13-2/L0：语义层"去伪"校验（在 emitProgram 的分流阶段调用）。
        // 这些检查都是"当前静默接受但语义无效"的写法，必须变成编译错误。
        // 返回 true 表示已登记一条错误（hasFailed() 为真），调用方应尽早停止。
        bool validateL0ClassDeclarations(
            const std::vector<std::shared_ptr<ast::ClassDefinitionNode>>& classNodes);
        bool validateL0FieldTypes(
            const std::vector<std::shared_ptr<ast::ClassDefinitionNode>>& classNodes);
        bool validateL0LabelConflicts(const std::shared_ptr<ast::ProgramNode>& program);
        // P13-2/L3(决议 §7.3)：Take[T] 只能作函数形参，不可作变量/字段/返回类型。
        bool validateL3TakeUsage(const std::shared_ptr<ast::ProgramNode>& program);
        // 阶段 B：递归算出字段偏移与对象大小（基类先算）。
        void computeLayout(const std::string& name);
        // 阶段 C：把构造器与方法作为独立函数发射出去。
        void emitClassBodies(const std::string& name);
        // 成员访问 `obj.field`（读）。
        void emitAttribute(const std::shared_ptr<ast::ExpressionNode>& node);
        // 成员写 `obj.field = v`（emitAssignment 分派过来）。
        void emitAttributeAssign(const std::shared_ptr<ast::InfixExpressionNode>& target,
                                 const std::shared_ptr<ast::ExpressionNode>& value);
        // P12/G2：元素写 `a[i] = v` / `d[k] = v`（emitAssignment 分派过来）。
        // 值已经在 %rax（或浮点 %xmm0）。返回 false 表示左值不是容器索引。
        bool emitIndexAssign(const std::shared_ptr<ast::IndexExpressionNode>& target,
                             const std::shared_ptr<ast::ExpressionNode>& value);
        // 同上，但值的静态类型由调用方给出 —— 用于复合赋值等「值已经算好、
        // 但手上没有对应 AST 节点」的场景，避免重新求值。
        bool emitIndexAssignValue(const std::shared_ptr<ast::IndexExpressionNode>& target,
                                  const sema::TypePtr& valType);
        // 构造调用 `Point(10, 20)`：分配并清零对象，再调 <Class>.__init__。
        void emitConstructorCall(const std::string& className,
                                 const std::shared_ptr<ast::ExpressionNode>& callNode,
                                 const std::vector<std::shared_ptr<ast::ExpressionNode>>& args);
        // ── 决议 A：值语义 struct 的发射 ────────────────────────────────────
        // `Pt(1, 2)`：按字段声明顺序位置初始化。结果不是堆指针，而是
        // **一块栈上的值**；因为寄存器装不下多字宽的值，这里把它落在
        // 一个临时槽里，并把槽地址留在 %rax（调用方负责拷走）。
        void emitStructConstructor(const std::string& structName,
                                   const std::shared_ptr<ast::ExpressionNode>& callNode,
                                   const std::vector<std::shared_ptr<ast::ExpressionNode>>& args);
        // 把 srcAddr 处的 struct 值逐字节拷到 dstAddr（都用 core_mem_copy）。
        // 地址由调用方算好放进这两个寄存器/槽位表达式里。
        void emitStructCopy(const std::string& dstAddr, const std::string& srcAddr, long long size);
        // 决议 A：**所有**调用点都必须走这两个函数，否则返回 struct 的调用
        // 会退化成「被调方自己造缓冲区」——那是 ret 之后就失效的栈内存。
        //
        // prepareStructRetBuf：返回 struct 时在调用方帧里分配返回缓冲区，
        // 把它的地址存进一个栈槽并返回该槽（空串 = 返回类型不是 struct）。
        // 非 struct 返回时 argStart 置 0；是 struct 时置 1（缓冲区占掉
        // 第一个整型寄存器位置，用户实参整体后移一格）。
        std::string prepareStructRetBuf(const sema::TypePtr& retType, size_t& argStart);
        // 调用之后把缓冲区地址重新装进 %rax：struct 表达式的结果是**值的地址**。
        void finishStructRetCall(const std::string& bufSlot);
        // 表达式求值后拿它的**地址**（struct 值语义专用）。
        // 变量 → 槽位地址；构造调用 → 临时槽地址；字段 → 基址+偏移。
        // 返回空串表示推不出来（调用方负责报错）。
        std::string emitStructAddrOf(const std::shared_ptr<ast::ExpressionNode>& node);
        // 从表达式推出 struct 名（推不出来返回空串）。
        std::string structNameOf(const std::shared_ptr<ast::ExpressionNode>& node) const;
        // 方法调用 `p.sum()` / `p.scale(3)`。
        void emitMethodCall(const std::shared_ptr<ast::InfixExpressionNode>& member,
                            const std::shared_ptr<ast::ExpressionNode>& callNode,
                            const std::vector<std::shared_ptr<ast::ExpressionNode>>& args);
        // `super(...)`：用当前 this 调用基类构造器。
        void emitSuperCall(const std::vector<std::shared_ptr<ast::ExpressionNode>>& args);
        // 类登记表
        ClassInfo* findClass(const std::string& name);
        const ClassInfo* findClass(const std::string& name) const;
        // 某个类（含基类）里所有可见方法。
        std::map<std::string, MethodRef> collectMethods(const std::string& name) const;
        // P12/G5：收集「这个类 + 这个方法名」的全部定义（沿继承链，派生类整体遮蔽）。
        std::vector<MethodRef> collectMethodOverloads(const std::string& name,
                                                       const std::string& method) const;
        // 从表达式推出它的类名（推不出来返回空串）。
        std::string classNameOf(const std::shared_ptr<ast::ExpressionNode>& node) const;
        // P13-2/L3-E（决议 §4.1）：从变量（含 Ref/Mut 引用）解析类类型名，
        // 用于穿透引用的子引用字段查找。
        [[nodiscard]] std::string classNameFromVariable(const std::string& name) const;
        // 在类层次里找字段偏移，找不到返回 -1。
        long long findFieldOffset(const std::string& className, const std::string& field) const;
        // 在类层次里找字段类型，找不到返回 nullptr。
        sema::TypePtr findFieldType(const std::string& className, const std::string& field) const;
        // 生命周期标签 static 的字段：返回全局槽的 asm 符号，沿基类链找。
        // 返回空串表示该字段不是 static。
        std::string findStaticFieldSymbol(const std::string& className,
                                          const std::string& field) const;
        sema::TypePtr findStaticFieldType(const std::string& className,
                                          const std::string& field) const;
        // 把 `Point` 这样的名字解析成 ClassType（不是类就返回 nullptr）。
        sema::TypePtr classTypeOf(const std::string& name) const;
        // 把 `a: Point` 这样的标签文本解析成类型（含类名）。
        sema::TypePtr resolveTypeName(const std::string& raw) const;

        // ---------- P10：泛型 ----------
        //
        // 类型标注里 `[...]` 的内容**只**存在于 LabelNode::labelDesNodes，
        // getFullLabel() 会把它整个丢掉（`list[float]` 只剩 "list"）。
        // 所以类型解析必须走 LabelNode 而不是字符串往返 —— 这也是 P9
        // 「`var xs: list[float]` 打印出 double 位模式」的根因。
        //
        // genericParams 非空时，出现在其中的名字解析成 GenericType 占位
        // （声明阶段），而不是「未知类型」报错。
        sema::TypePtr typeFromLabelNode(const std::shared_ptr<ast::LabelNode>& label,
                                        const std::vector<std::string>& genericParams = {}) const;
        // 决议 B：解析 `T?` 里的那个 T（不做可空包装，避免递归套娃）。
        sema::TypePtr typeFromLabelNodeNonNull(const std::shared_ptr<ast::LabelNode>& label,
                                               const std::vector<std::string>& genericParams) const;
        // 类型位置上的表达式转成类型：`float`、`Box[int]`、`list[int]`。
        sema::TypePtr typeFromTypeArgNode(const std::shared_ptr<ast::ExpressionNode>& node,
                                          const std::vector<std::string>& genericParams) const;
        // 函数定义节点上的返回类型标注 -> 类型。
        //
        // 为什么不用匿名命名空间里的 returnTypeOf()：它只调 parseTypeName，
        // 而 parseTypeName 只看裸名字、拿不到 `[...]`（`list[str]` 会退化成
        // 「元素按 int 解释」的 list）。后果是 `fun mk(): list[str]` 登记进
        // _functionReturns 的类型丢了元素类型，于是 `mk()[0]` 的静态类型
        // 变成 int —— sout 按 int 打印指针，输出一串数字。
        // 走 LabelNode 才能保留元素类型（P9/P10 同一套理由）。
        sema::TypePtr returnTypeFromNode(const std::shared_ptr<ast::FunctionDefinitionNode>& node,
                                         const std::vector<std::string>& genericParams = {}) const;
        // 把逗号分隔的类型实参表达式摊平（`func[int, str][bool]` 的第一段）。
        static void flattenTypeArgs(const std::shared_ptr<ast::ExpressionNode>& node,
                                    std::vector<std::shared_ptr<ast::ExpressionNode>>& out);
        // 单态化：把签名里的 GenericType 换成实参类型（未绑定的保持原样）。
        static sema::TypePtr substituteGenerics(const sema::TypePtr& t,
                                                const std::map<std::string, sema::TypePtr>& bindings);
        // §4.3 冻结的名字修饰：`_R<len><name><encoded-params>`。
        static std::string mangleType(const sema::TypePtr& t);
        static std::string mangleInstance(const std::string& base,
                                          const std::vector<sema::TypePtr>& args);
        // 泛型函数声明（未实例化）。实例按需单态化后放进 _pendingFunctions。
        struct GenericFunction
        {
            std::string name;
            std::vector<std::string> genericParams;
            std::shared_ptr<ast::FunctionDefinitionNode> def;
            // 声明时的「原始」形参类型 / 返回类型（含 GenericType 占位）。
            std::vector<sema::TypePtr> paramTypes;
            sema::TypePtr retType;
            std::vector<std::string> paramNames;
            // 每个形参是否写了类型标注（按位置）。决议要靠它区分
            // 「裸容器形参」与「写了标注的形参」。
            std::vector<bool> paramAnnotated;
            // 形参默认值：名字 -> 默认值表达式。
            // 必须在声明阶段就收下来：泛型声明本身不发射，形参表只在这里出现一次，
            // 实例化时拿不到 AST 就补不出缺省实参（曾因此让 f(1) 读到栈垃圾）。
            std::map<std::string, std::shared_ptr<ast::ExpressionNode>> defaults;
        };
        // 名字 -> 该名字下的**全部**泛型声明。
        //
        // 为什么是 vector 而不是单个 GenericFunction（P13 缺陷修复，2026-11-17）：
        // 同名不同泛型形参个数的声明是合法的重载形状，典型写法
        //     fun dumpToStr[T](c: list[T]): str
        //     fun dumpToStr[T, K](c: dict[T, K]): str
        // 原先用 map<string, GenericFunction> + operator[] 登记，后一份**直接
        // 覆盖**前一份且没有任何诊断，调用 dumpToStr[int](...) 时报的是
        // "takes 2 type argument(s), but 1 were supplied" —— 用户看不出是
        // 声明被吞了。现在按名字收成一组，调用点按「显式类型实参个数」
        // 优先、其次按「实参类型能推出哪一份」来选择。
        std::map<std::string, std::vector<GenericFunction>> _genericFunctions;
        // 按显式类型实参个数挑候选；个数为 0 时返回全部。
        std::vector<const GenericFunction*> genericCandidates(
            const std::string& name, size_t explicitCount) const;
        // 这份泛型声明能否接受 argc 个实参（必填 <= argc <= 全部）。
        [[nodiscard]] bool genericAcceptsArgCount(const GenericFunction& g, size_t argc) const;
        // 这份泛型声明的形参类型与实参是否「形状相容」（容器种类一层）。
        // 用于挡住 `dumpToStr[str](dict)` 这种「类型实参个数恰好对上、
        // 但实参根本不是那个容器」的候选 —— 否则会静默选中错的重载并在
        // 运行时段错误。元素级差异留给 emitArgsToRegisters 的 checkAssignable。
        [[nodiscard]] bool genericArgsShapeMatch(
            const GenericFunction& g,
            const std::vector<std::shared_ptr<ast::ExpressionNode>>& args) const;
        // 已实例化的签名：mangled 名 -> true（避免重复单态化）。
        std::map<std::string, bool> _genericInstances;
        // 待发射的实例化函数体。
        //
        // 为什么必须排队而不能就地发射：实例化是在**调用点**被发现的，
        // 那时正在发射某个函数体，rsp/frame/label 全在用。就地发射会把
        // 两个函数的帧搅在一起。所以实例化只登记签名 + 排队，
        // 等 emitPendingInstances 在末尾统一发射（和闭包同一套路）。
        struct PendingInstance
        {
            std::string emitName;
            std::vector<std::string> paramNames;
            std::vector<sema::TypePtr> paramTypes;
            sema::TypePtr retType;
            std::shared_ptr<ast::ExpressionNode> bodyNode;
            std::map<std::string, sema::TypePtr> bindings;
            std::vector<std::string> genericParams;
        };
        std::vector<PendingInstance> _pendingInstances;
        void emitPendingInstances();
        // 从泛型函数的声明里取出泛型形参名（`[T]` / `[A, B]`）。
        static std::vector<std::string> collectGenericParams(
            const std::shared_ptr<ast::ExpressionNode>& callNode);
        // 从 `f[T](...)` 的 callee 里取出显式类型实参（`f[int]` 的 int）。
        std::vector<sema::TypePtr> explicitTypeArgs(
            const std::shared_ptr<ast::ExpressionNode>& callee) const;
        // 按实参类型推导类型实参（显式实参优先，缺的从实参补）。
        bool deduceTypeArgs(const GenericFunction& generic,
                            const std::vector<sema::TypePtr>& explicitArgs,
                            const std::vector<std::shared_ptr<ast::ExpressionNode>>& args,
                            std::map<std::string, sema::TypePtr>& bindings) const;
        // 实例化一个泛型函数，返回 mangled 名（失败返回空串）。
        std::string instantiateGeneric(const std::string& name,
                                       const std::map<std::string, sema::TypePtr>& bindings);
        // 发射一次泛型调用（`f[int](x)` 或裸名 `f(x)`）。返回 false 表示
        // 这个名字不是泛型函数，调用方继续走普通路径。
        bool tryEmitGenericCall(const std::string& name,
                                const std::shared_ptr<ast::ExpressionNode>& callee,
                                const std::shared_ptr<ast::ExpressionNode>& callNode,
                                const std::vector<std::shared_ptr<ast::ExpressionNode>>& args);
        // 泛型调用的静态结果类型（推导失败返回 nullptr）。
        // 必须在发射前就知道，槽位大小 / sout 选型都依赖它。
        sema::TypePtr inferGenericCallType(const std::string& name,
                                           const std::shared_ptr<ast::ExpressionNode>& callee,
                                           const std::shared_ptr<ast::ExpressionNode>& callNode) const;
        // 从表达式里的标注取出类型（无标注返回 unknown）。
        sema::TypePtr typeFromLabelList(
            const std::vector<std::shared_ptr<ast::LabelNode>>& labels,
            const std::vector<std::string>& genericParams = {}) const;
        // 当前正在实例化的泛型形参（空 = 不在泛型函数体里）。
        // 函数体里的 `var y: T = x` 要靠它把 T 解析成占位而不是报未知类型。
        std::vector<std::string> _currentGenericParams;
        // 当前实例化绑定的实参类型：T -> int。发射实例函数体时，
        // 标注里的 T 必须解析成**具体**类型（槽位大小、float 与否都靠它）。
        std::map<std::string, sema::TypePtr> _currentGenericBindings;

        // ---------- P10：泛型类 ----------
        // `class Box[T]` 只是模板，不登记成可构造的类；`Box[int]` 出现时
        // 才用这个模板生成一份具体布局（名字带 mangled 后缀）。
        struct GenericClass
        {
            std::string name;
            std::vector<std::string> genericParams;
            std::shared_ptr<ast::ClassDefinitionNode> def;
        };
        std::map<std::string, GenericClass> _genericClasses;
        // 已实例化出来的具体类名（按发现顺序）与已发射标记。
        std::vector<std::string> _genericClassInstances;
        std::map<std::string, bool> _genericClassEmitted;
        // 每个实例的绑定，发射方法体时要恢复。
        std::map<std::string, std::pair<std::vector<std::string>,
                                        std::map<std::string, sema::TypePtr>>> _genericClassBindings;
        // 确保 `Box[int]` 这个具体类已登记，返回它的 mangled 名。
        std::string ensureClassInstance(const std::string& base,
                                        const std::vector<sema::TypePtr>& args);
        // 把类型实参列表解析出来（`Box[int, str]` 的 [int, str]）。
        std::vector<sema::TypePtr> typeArgsOf(const std::shared_ptr<ast::ExpressionNode>& node) const;
        // 用给定具体类名登记类体（模板实例化复用同一条路径）。
        void registerClassBody(const std::shared_ptr<ast::ClassDefinitionNode>& node,
                               const std::string& className);
        // 发射所有已实例化泛型类的构造器与方法体。
        void emitGenericClassBodies();

        // 形参/返回类型的收集变体：能把类名解析成 ClassType。
        void collectParamNamesT(const std::shared_ptr<ast::ExpressionNode>& node,
                                std::vector<std::string>& out) const;
        void collectParamTypesT(const std::shared_ptr<ast::ExpressionNode>& node,
                                std::vector<sema::TypePtr>& out) const;
        // P9：收集每个形参的「原始标注文本」，未标注的项为空串。
        // 用来区分 `fun f(x)`（未标注，允许推断）与
        // `fun f(x: Piont)`（标注了但解析不出来，必须报错）。
        void collectParamAnnotations(const std::shared_ptr<ast::ExpressionNode>& node,
                                     std::vector<std::string>& out) const;
        // 方法/构造器的返回类型：声明的标签优先，否则从函数体推断。
        // outAnnotated 非空时回填「返回类型是否来自**显式标注**」——
        // emitFunctionBody 的 retTypeAnnotated 参数靠它决定要不要校验
        // `ret` 的值类型（与顶层函数 emitFunctionDefinition 同一判据）。
        sema::TypePtr methodReturnType(const std::shared_ptr<ast::FunctionDefinitionNode>& def,
                                       const std::vector<std::string>& paramNames,
                                       const std::vector<sema::TypePtr>& paramTypes,
                                       const std::string& ownerClass = std::string(),
                                       bool* outAnnotated = nullptr) const;
        // 当前正在发射的方法/构造器所属的类名（空 = 不在类里）。
        std::string _currentClass;
        // 类登记表
        std::map<std::string, ClassInfo> _classes;
        // 决议 A：值语义 struct 登记表
        std::map<std::string, StructInfo> _structs;

        // ---------- P11：重载 ----------
        //
        // 一个名字可以有多个签名。符号规则（沿用 P10 冻结的 mangling 形状）：
        //   * 名字只有一个定义 -> 符号就是裸名，rt.o / core 的 ABI 完全不变
        //   * 名字有多个定义   -> 全部用 `_R<len><name><encoded-params>` 修饰
        // 决议发生在**调用点**，只看实参的静态类型 —— 单遍 codegen 没有独立
        // sema pass，所以决议必须能在发射调用指令的当下完成。
        //
        // 与 P10 的关系：泛型实例也用 `_R<len><name>...` 这个名字槽位，但那里
        // 编码的是**类型实参**，这里编码的是**形参类型**。一个名字不会同时
        // 是泛型又是重载集（泛型声明不参与重载计数，见 registerOverloadSignatures）。
        struct FunctionSignature
        {
            std::string name;
            // 发射用的汇编符号。唯一名字时等于 name。
            std::string symbol;
            std::vector<std::string> paramNames;
            // 形参类型。未标注的形参在这里是 TK_UNKNOWN（不是回退的 int）：
            // mangling 要靠它把 `fun f(x)` 与 `fun f(x: int)` 区分开。
            std::vector<sema::TypePtr> paramTypes;
            // 每个形参是否写了类型标注。未标注的形参在决议里是通配符。
            std::vector<bool> annotated;
            // 形参标注的**原始文本**（空串 = 没标注）。诊断要用它：
            // `fun f(x: Piont)` 必须报出 "Piont" 而不是解析失败的占位。
            std::vector<std::string> annotationTexts;
            std::map<std::string, std::shared_ptr<ast::ExpressionNode>> defaults;
            sema::TypePtr retType;
            bool retAnnotated = false;
            // 非空 = 泛型声明（走 P10 路径，不参与重载）。
            std::vector<std::string> genericParams;
            // 函数体只有一个 encapsulated：由运行时提供实现，符号必须是裸名
            // （rt.o 里就是这么定义的），所以不参与重载计数、不加修饰。
            bool encapsulated = false;
            std::shared_ptr<ast::FunctionDefinitionNode> def;
        };
        // 纯收集：名字、形参、标注、默认值、返回类型。不做登记、不报错，
        // 因此预登记与正式发射可以各调一次而结果一致。
        FunctionSignature collectSignature(const std::shared_ptr<ast::FunctionDefinitionNode>& node);
        // 名字 -> 候选签名。只对「有多个定义」的名字填充。
        std::map<std::string, std::vector<FunctionSignature>> _overloads;
        std::map<std::string, bool> _overloadedNames;
        // 符号 -> 签名。同样只覆盖重载名：唯一名字继续用裸名 + 原来的
        // _functionParams/_functionReturns 那几张表，行为完全不变。
        //
        // 为什么必须**预登记**：函数体是按文件顺序发射的，f1 里调用文件
        // 后面才定义的 f2 时，f2 的签名还没登记进 _functionParams。
        // 重载决议需要候选签名才能比较，所以要在发射任何函数体之前
        // 先把签名收集一遍（见 registerOverloadSignatures）。
        std::map<std::string, FunctionSignature> _signatures;
        // 已发射过的汇编符号（同签名的重复定义要给出明确诊断，
        // 而不是让汇编器报 symbol already defined）。
        std::map<std::string, bool> _emittedSymbols;
        // 预登记：必须在发射**任何**函数体之前跑一遍，否则前向调用
        // （f1 的函数体里调文件后面才定义的 f2）解析不出候选签名。
        void registerOverloadSignatures(
            const std::vector<std::shared_ptr<ast::FunctionDefinitionNode>>& fns);
        // 重载名的符号修饰。编码的是**形参类型**（未标注形参编成 v）。
        static std::string mangleOverload(const std::string& base,
                                          const std::vector<sema::TypePtr>& paramTypes);
        // 按实参静态类型选签名，返回符号；空串 = 没有候选（ambiguous 置位表示
        // 多个候选同分）。
        [[nodiscard]] std::string resolveOverload(
            const std::string& name,
            const std::vector<std::shared_ptr<ast::ExpressionNode>>& args,
            bool* ambiguous = nullptr) const;
        // 单个形参的匹配分：-1 不匹配，数值越大越精确。
        [[nodiscard]] int overloadScore(const sema::TypePtr& formal,
                                        const sema::TypePtr& actual,
                                        bool annotated) const;
        // 按给定签名摊平命名实参 / 补齐默认值。装不下返回 false。
        static bool normalizeArgsFor(
            const std::vector<std::shared_ptr<ast::ExpressionNode>>& args,
            const std::vector<std::string>& names,
            const std::map<std::string, std::shared_ptr<ast::ExpressionNode>>& defaults,
            std::vector<std::shared_ptr<ast::ExpressionNode>>& out);
        // 已决议到具体符号的调用：摊平实参 + 按签名走寄存器 + call。
        void emitResolvedCall(const std::string& symbol,
                              const std::string& displayName,
                              const std::vector<std::shared_ptr<ast::ExpressionNode>>& args);
        // 重载决议失败的诊断（无候选 / 歧义）。
        void failOverload(const std::string& name,
                          const std::vector<std::shared_ptr<ast::ExpressionNode>>& args,
                          const utils::Pos& pos,
                          bool ambiguous);

        // ---------- P7：异常 ----------
        // 当前函数里处于活动状态的 try 帧，栈底 = 最外层 try。
        // 只记帧块相对 %rbp 的偏移：ret / break / continue 提前离开
        // try 时必须发射配对的 core_eh_pop，否则链上会留下指向已销毁
        // 栈帧的节点，下一次 throw 就会跳进死帧。
        struct EhFrame
        {
            long long offset = 0;
            // 该 try 的 finally 体（可能为空）。ret / break / continue 提前
            // 离开时必须把它内联发射一遍 —— 否则 finally 被静默跳过。
            std::shared_ptr<ast::BlockRangerNode> finallyBody;
            // P13-1（D13）：进入该 try 的 **body** 时的作用域深度。
            //
            // 「移动外层拥有者」的判据就是这个：要移动的变量登记在
            // _scopeDrops 的下标 < tryBodyScope 里，说明它比 try 活得久，
            // 一旦抛出就会被 longjmp 跳过它的销毁 —— 必须禁止。
            // 循环里 break/continue 用的 dropDepth 是同一套口径。
            size_t tryBodyScope = 0;
            // 是否正在发射 try 体（false = 正在发射 handler / finally，
            // 那两处不受此禁令约束）。
            bool inTryBody = false;
        };
        std::vector<EhFrame> _ehStack;
        // 正在内联发射 finally 体时置位，防止 finally 里的 ret / break
        // 又触发一次 finally 展开（无限递归）。
        bool _emittingFinally = false;
        // try 帧槽位命名计数
        size_t _ehCounter = 0;
    };
}

#endif //RCC_CODEGEN_H

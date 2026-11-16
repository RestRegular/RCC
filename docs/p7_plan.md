# P7 阶段记录：异常（try / catch / finally / throw）（对应路线图 P7）

> 前置：P6 已验收通过（P0 5/5、P1.2 13/13、P4 函数值 10/10、P5 容器 14/14、
> P6 类 15/15、legacy 13/17）。
> 本文件只描述本阶段的范围、设计、阻塞点与验收标准，不改动 `architecture_plan.md` 的总体路线。

---

## 一、本阶段要解决什么

`architecture_plan.md` §9.1 把 P7 定义为「异常：try/catch/finally/throw（方案 B）」，
验收标准是 `test_11_throw` 与 `test_12_trycatch` 通过。

P6 结束后这两个用例都红着，报错是：

```
Error: Code generation failed: Unsupported expression in code generation: [Node(THROW)...
Error: Code generation failed: Unsupported expression in code generation: [Node(TRY)...
```

parser 早就把 `try` / `throw` 解析成 `TryNode` / `ThrowNode` 了（`rcc_prefix_builder.cpp`
的 `buildTryExpression` / `buildThrowExpression`），缺的纯粹是 codegen 这条路。

但**只把节点编译出来是不够的**：`test_11` / `test_12` 在运行时其实一次都不抛
（`test_11` 的 `safeDivide(10, 3)` 走的是 `b != 0` 分支，`test_12` 两个 try 体都不抛）。
如果照 legacy LLVM 版那样「try 就直接跑、catch 跳过、throw 就 exit(1)」，
这两个用例也会变绿，但异常根本没实现 —— 阶段验收会变成假的。
所以本阶段按 §8.2 的方案 B 做一个**真的 unwinder**，并用一组自己写的用例
（含真实的跨函数抛出、未捕获终止）来证明它确实在工作。

---

## 二、方案选择：为什么是 B

`architecture_plan.md` §8.2 的三选一：

| 方案 | 说明 | 工作量 |
|---|---|---|
| A. 表驱动 unwind（Rust/Itanium 风格） | 需要 `.pdata`/`.xdata` 与自定义 unwinder | 极大 |
| B. 显式帧链（setjmp/longjmp 风格） | `core` 维护 handler 栈，`throw` 时跳转 | 小，`core.eh_*` 已列出 |
| C. 改为 Result 返回 | 抛弃异常语义 | 改语言 |

选 **B**。理由：

1. `lib/core/core.rio` 在 P0 就把 `eh_push` / `eh_pop` / `eh_throw` 三个函数写进了
   ABI v1（第 56–62 行），`core_win64.s` 里 `core_eh_push` / `core_eh_pop` 也早已可用，
   `core_eh_throw` 是个 `jmp core_proc_abort` 的桩。方案 B 是唯一**不需要新增 ABI**的路径。
2. 方案 A 要写 DWARF/SEH 的 unwind 表，在 Windows x64 上还要和 `.pdata` 对齐，
   与本项目「先跑通语义、再谈性能」的取向不符。
3. 方案 C 要改语言，不是编译器阶段能决定的。

**接受的代价**（§8.2 原文）：非局部退出不保证执行析构。P7 阶段本来也没有析构
（P6 §七.5 明确记了「没有析构，对象永不释放」），所以这个代价当前为零。
将来引入引用计数或 RAII 时，需要把「沿途帧的清理」补进 unwinder —— 届时
帧布局已经留好了 `exc` 槽，可以再挂一个 cleanup 链。

---

## 三、帧布局（codegen 与 core 之间的约定）

handler 帧是一块 **48 字节**的栈槽，由 `emitTry` 通过 `allocateSlot` 分配
（因此 `_frameSize` 会把它算进去，函数末尾的帧回填自动覆盖）：

| 偏移 | 字段 | 谁写 |
|---|---|---|
| `+0` | `prev` —— 上一帧（链指针） | `core_eh_push` |
| `+8` | `saved rbp` —— 进入 try 时的 `%rbp` | `emitTry` |
| `+16` | `saved rsp` —— 进入 try 时的 `%rsp` | `emitTry` |
| `+24` | `resume` —— 异常落地后跳转的标签地址 | `emitTry` |
| `+32` | `exc` —— 异常值 | `core_eh_throw` 写，catch 读 |
| `+40` | pad | — |

**为什么把 `%rbp` / `%rsp` 存进栈槽，而不是借 `%rbx` / `%r12` 之类的 callee-saved 寄存器？**
那些寄存器目前确实没被 codegen 用到（grep 确认过），但一旦将来引入寄存器分配
就会冲突。栈槽是 `%rbp` 相对的，跨函数调用天然稳定，且由帧回填统一管理，
不会和任何寄存器约定打架。

**为什么 `+40` 是 pad？** `allocateSlot` 会把大小取整到 8 的倍数，48 正好是
16 的倍数，方便将来挂 cleanup 指针（`+40` 就是留给它的位置）。

---

## 四、控制流

`emitTry` 生成的骨架（以 `try { A } catch(e) { B } finally { C }` 为例）：

```
    leaq   frame, %rcx
    call   core_eh_push          # 压帧，prev = 旧栈顶
    movq   %rbp, frame+8
    movq   %rsp, frame+16
    leaq   .Lresume(%rip), %rax
    movq   %rax, frame+24
    <A>
    call   core_eh_pop(frame)    # 正常路径：先摘链
    <C>                          # 再跑 finally
    jmp    .Lend
.Lresume:                        # 异常落地
    call   core_eh_pop(frame)    # 先摘链，catch 里再 throw 不会自己接自己
    movq   frame+32, %rax        # 取异常值
    <e = %rax>                   # 绑定 catch 变量
    <B>
    <C>                          # catch 路径也要跑 finally
.Lend:
```

三条硬规则：

1. **正常路径和异常落地路径各自发一次 `core_eh_pop`**，但编译期的 `_ehStack`
   只能弹一次 —— 否则后面嵌套的 try 会算错深度。实现里用 `outerDepth`
   在两条路径上 `_ehStack.resize(outerDepth)`。
2. **`ret` / `break` / `continue` 跳出 try 时必须发配对的 `core_eh_pop`**
   （`emitEhUnwindTo`），否则链上留下指向已销毁栈帧的节点，下一次 throw
   就会跳进死帧。`ret` 走 `emitEhUnwindTo(0)`（全部摘掉），`break`/`continue`
   走 `emitEhUnwindTo(_loopStack.back().ehDepth)`（只摘循环体内的帧）。
   **同时必须把沿途每层的 `finally` 体内联发射一遍** —— 否则 `ret` 会静默
   跳过 `finally`（实测踩到：`try { ret 1 } finally { sout("f") }` 只输出 `1`）。
   所以 `EhFrame` 里存了 `finallyBody`，`emitEhUnwindTo` 从内往外逐层
   「先跑 finally、再摘链」；`_emittingFinally` 置位防重入（finally 体里
   若还有 `ret`，不应再展开自己那层）。
3. **`emitEhUnwindTo` 只发指令、不动 `_ehStack`** —— 调用点是终结性跳转，
   但它们之后的代码（比如 try 的正常收尾）仍会被发射，编译期帧栈必须
   保持与词法嵌套一致。

`ret` 还有一个细节：`core_eh_pop` 用 `%rax` 返回 `frame->prev`，会把刚算好的
返回值冲掉。所以 `emitReturn` 在展开前先把返回值存进临时槽，展开后再装回来
（浮点值走 `%xmm0`，同样处理）。

---

## 五、unwinder（`core_eh_throw`）

`lib/core/core_win64.s` 里原本是 `jmp core_proc_abort` 的桩，P7 换成真的：

```asm
core_eh_throw:
    movq    core_eh_top(%rip), %rax
    testq   %rax, %rax
    je      .Leh_uncaught
    movq    %rcx, 32(%rax)       # frame->exc = exc
    movq    8(%rax), %rbp        # 恢复 %rbp
    movq    16(%rax), %rcx       # 取 saved rsp
    movq    24(%rax), %rdx       # 取 resume
    movq    %rcx, %rsp           # 恢复 %rsp（最后一步）
    jmp     *%rdx
.Leh_uncaught:
    jmp     core_proc_abort      # 退出码 3
```

三个关键点：

1. **链顶停在目标帧上，不弹出它** —— 目标帧的 handler 落地后自己
   `core_eh_pop`。这样 catch 体里再 throw 不会看到自己那一帧（否则自己接自己，死循环）。
2. **`%rsp` 必须在读完帧字段之后才恢复** —— 目标帧可能就在当前 `%rsp` 之上，
   先改 `%rsp` 会踩掉还没读的字段。
3. **未捕获退化为 `core_proc_abort`（退出码 3）**，与 P0 骨架的既有约定一致。

`core_eh_top` 是 `.data` 里的单个 `.quad`，即**单线程**的。P7 不做线程，够了；
将来要多线程时改成 TLS 变量即可，ABI 签名不用动。

---

## 六、AST 形态（实测）

| 写法 | 节点 |
|---|---|
| `try { ... } catch(e) { ... }` | `TryNode`，`getTryBody()` = `BlockRangerNode` |
| `catch(e)` | `getCatchBodies()` → `vector<pair<IdentifierNode, BlockRangerNode>>` |
| `finally { ... }` | `getFinallyBody()` = `BlockRangerNode`（可空） |
| `throw expr` | `ThrowNode`，`getThrowExpression()` |

**`catch(e)` 没有类型过滤** —— parser 只接受 `catch (identifier)` 这一种形式
（`buildTryExpression` 里 `RANGER → IDENTIFIER`，否则 `recordSyntaxError("Invalid catch item.")`）。
所以「多个 catch 按类型匹配」在语法层面就不存在，codegen 取第一个分支即可；
多写几个 catch 时后面的永远不生效。

---

## 七、catch 变量的类型

异常值在 P7 里**按字符串处理**：`throw "msg"` 是唯一的实际用法，也是
`test_11` / `test_12` 的写法，`sout(e)` 走 `rt_print_str`（读 `+0` 的长度头）。

局限：`throw 42` 这类非字符串值不会被静态拒绝，但 catch 侧仍按 `str` 解释，
`rt_print_str` 会把整数当指针读，结果未定义。**正确做法**是引入一个
`Any` / `Variant` 标签值（`architecture_plan.md` §8.3 讨论的「去 any」问题），
那是后续阶段的事，P7 不展开。

---

## 八、实现清单

`code/include/backend/rcc_codegen.h`：

- `struct EhFrame { long long offset; std::shared_ptr<ast::BlockRangerNode> finallyBody; };`
  —— 偏移用于摘链，`finallyBody` 用于提前退出时内联跑 finally；
- `std::vector<EhFrame> _ehStack;` —— 栈底 = 最外层 try；
- `bool _emittingFinally` —— 防 finally 重入；
- `size_t _ehCounter` —— 帧槽命名计数；
- `LoopContext` 加 `size_t ehDepth` —— break/continue 只需要弹出循环体内的帧；
- 声明 `emitTry` / `emitThrow` / `emitEhPop` / `emitEhUnwindTo`。

`code/src/backend/rcc_codegen.cpp`：

- `emitStatement` / `emitExpression` 都加 `TryNode` / `ThrowNode` 分派；
- 新增 §四的四个方法，插在 `emitContinue` 之后；
- `emitReturn` 加返回值保存 + `emitEhUnwindTo(0)`；
- `emitBreak` / `emitContinue` 加 `emitEhUnwindTo(ehDepth)`；
- `emitEhUnwindTo` 在摘链前逐层内联发射 `finallyBody`（`_emittingFinally` 防重入）；
- `emitLoop` 在 push `LoopContext` 时记 `_ehStack.size()`；
- `emitFunctionBody` 进入时保存并清空 `_ehStack` / `_loopStack`，退出时恢复
  （try 帧栈是函数局部的，嵌套发射匿名函数时不能把外层帧带进来）。

`lib/core/core_win64.s`：

- 替换 `core_eh_throw` 的桩（§五）。

---

## 九、已知限制

1. **非局部退出不保证跑析构** —— §8.2 已接受的代价；当前没有析构，代价为零；
2. **单线程** —— `core_eh_top` 是全局 `.quad`，不是 TLS；
3. **catch 无类型匹配** —— 语法层面只有 `catch(e)`，多个 catch 只取第一个；
4. **异常值只支持字符串** —— 见 §七；
5. **`finally` 里 `ret` / `break` 的行为** —— 会覆盖掉正在传播的异常，
   P7 不做静态拒绝（`test_11` / `test_12` 都没有这种写法）；
6. **没有 `throw` 表达式的类型检查** —— `throw` 是语句，没有类型。

---

## 十、验收

新增 `test_scripts/test_p7_exceptions.ps1`，17 个用例覆盖：

throw/catch 基本流程、catch 变量取字符串值、throw 之后的语句不执行、
无异常时 catch 不执行、finally 在正常路径执行、finally 在 catch 之后执行、
只有 finally 时异常继续外传、跨一层函数传播、跨两层函数传播、
嵌套 try 内层捕获、嵌套 try 内层不捕获则冒到外层、循环内 try + break、
循环内 try + continue、try 内 ret 提前返回、ret 时 finally 仍执行、
break 时 finally 仍执行、未捕获 throw 以退出码 3 终止。

最后一条是本阶段「真的实现了」的关键证据：它要求 unwinder 在链空时
真的终止进程，而不是编译通过就算数。倒数三条则锁住「提前退出也要跑
finally」这条容易漏掉的语义。

结果：

```
P0 5/5   P1.2 13/13   P4 10/10   P5 14/14   P6 15/15   P7 17/17
legacy 15/17（test_11、test_12 转绿）
```

剩余 2 个 legacy 失败用例与本阶段无关：

- `test_14_iter` / `test_15_forinrange` —— `import` 与 `ds.forInRange`（函数调用，非 for-in 语法），P8。

---

## 十一、实测踩到的坑

1. **`%rax` 在 `finally` 之后不能用** —— 重新抛出时如果读 `%rax` 里的异常值，
   `finally` 体的任意代码都已经把它冲掉了。必须重新从帧槽 `+32` 读。
   `finally_no_catch_reraises` 用例抓到这个问题（表现为外层 catch 到了野指针，
   进程 `0xC0000005` 崩溃）。
2. **正常路径和异常路径的 `_ehStack` 只能弹一次** —— 两条路径都发
   `core_eh_pop`，但如果都 `pop_back()`，后面的嵌套 try 深度就错了。
   用 `resize(outerDepth)` 而不是 `pop_back()`。
3. **`emitEhUnwindTo` 不能修改 `_ehStack`** —— 它是「发射指令」，不是「维护编译期状态」。
   混在一起会让 try 正常收尾时重复弹栈。
4. **`core_eh_pop` 会毁掉 `%rax`** —— `ret` 里要保存返回值，见 §四。
5. **`fun main()` 不会自动执行** —— 它被发射成 `__rio_user_main`，
   `emitMainWrapper` 明确不自动调用（否则 legacy `test_10_main` 会跑两次）。
   写探针时要么用顶层语句，要么显式 `main()`。
6. **提前退出必须内联跑 finally** —— 只发 `core_eh_pop` 而不跑 finally，
   会让 `ret` / `break` / `continue` 静默跳过 finally 体。
   `ret_runs_finally` / `break_runs_finally` 两个用例锁住这条。

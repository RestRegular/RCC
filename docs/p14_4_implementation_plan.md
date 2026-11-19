# P14-4 实施方案：`Locked<T>` / `lock` / `Atomic<T>` 显式同步

> **本文性质**：实施方案（2026-11-20）。依据已裁决的 P14 全部设计（`docs/p14_plan.md` §7，Q1–Q20）、
> 规范 §10.6（`docs/memory-model-spec-full.md`），三个实现层接口空白已于 2026-11-20 由用户裁决闭合（§4），
> 以及 codegraph 对当前代码库的实测（§1）。
> 遵守硬规则 `1b1ccafb`：本文用到的设计全部已获批准；出现的每个语法/类型/原语都在裁决清单内。
> 前置：P14-1a（运行时地基）、P14-1b（scope/spawn）、P14-3（par_each）均已完成并全量回归通过。

---

## 0. 阶段定位

- 阶段路线（`p14_plan.md §2`）：P14-4 → P14-5 Chan → P14-6 验收。
- 本阶段交付 §10.6 的显式同步三件：`Locked<T>`（互斥 + 二等值 guard）、`lock c as g {}`（语句形态）、
  `Atomic<T>`（i32/i64 顺序一致原子）。
- 竞争排除论证中本阶段负责的一条：**共享可变状态只经 `Locked`，访问只在 `lock` 块内，二等值不逃逸**
  （`p14_plan.md §4.2` 第 5 条）。
- 每完成一个子阶段汇报进度后继续（沿用 1a/1b/3 自主推进方式）。

---

## 1. 关键事实（codegraph 实测，2026-11-19/20，逐条带出处）

### 1.1 词法层无 `lock` / `as`
- `TokenType`（`code/include/rcc_core.h:64-155`）有 `TOKEN_SCOPE`/`TOKEN_SPAWN`，无 `TOKEN_LOCK`/`TOKEN_AS`。
- `tokenMap`（`code/src/rcc_core.cpp:30-107`）与 `getTokenTypeName`（:250+）需三处同步新增。
- `lock` 语句可仿 `scope` 的 prefix builder：`buildScopeExpression`（`rcc_prefix_builder.cpp:941-976`）
  读标识符 → `TOKEN_LBRACE` → `buildRangerExpression` → 构造节点，并在 `rcc_parser.cpp:96` 的分发表登记。

### 1.2 类型系统无 Locked / Atomic / i32
- `TypeKind`（`code/include/sema/rcc_sema_types.h:21-41`）有 `TK_POOL`/`TK_LINK`/`TK_MEMORY`，无同步类型。
  新增 `TK_LOCKED`/`TK_ATOMIC`/`TK_I32` 与对应 Type 子类（带 inner TypePtr）与既有 `PoolType`/`LinkType` 同构。
- 类型标注解析：`parseTypeName`（`rcc_codegen.cpp:77`，裸名）与 `typeFromTypeArgNode`（:11651，
  bracketed `[...]`）。后者的 `Ref`/`Mut`/`Take`/`Body`/`Content` 分支（:11686-11695）是 `Locked[T]`/
  `Atomic[T]` 的模板；`ResourceType`/`MemoryRole` 机制见 `rcc_sema_types.h:453-497`。
- **Rio 的 `int` 是 64 位**（P12 i64 字面量实测，`docs/p12_plan.md:1532-1560`），无 `i32`/`i64` 类型名。
  按裁决 §3-B：`i64` 作为 `int` 的别名解析，`i32` 为新增 32 位内建类型（仅低 32 位参与原子宽度）。

### 1.3 方法派发落点
- `emitFunctionCall`（`rcc_codegen.cpp:10388`）DOT 分支依次处理：模块名(:10438) → `Pool.fixed/boxed/chunked`
  (:10450-10482) → `pool.new[T]`(:10487+) → 兜底 `emitMethodCall`。
- 类内建能力方法 `ref()/mut()/take()` 在 `inferType`(:3713-3719) 与 `emitMethodCall`(:14748+) 处理。
- `Locked.new` / `Atomic.new` 落 `emitFunctionCall` DOT 静态分支（仿 Pool）；实例方法
  （`share`/原子方法）按接收者静态类型 `TK_LOCKED`/`TK_ATOMIC` 特化；`inferType` 镜像返回类型。
- 三点登记法（沿用 P13/P14-3）：core 汇编 `.globl` + `core.rio` extern + `coreReturnType`（:469）非 int 返回。

### 1.4 运行时同步原语现状：零基础
- core 已有自旋锁先例：`core_io_spin`（`core_win64.s:773`）、`core_dbg_spin`(:571)、`core_eh_tls_spin`(:1064)，
  均用 `xchgl` + `pause` 自旋；`core_io_write`(:777) 演示「xchgl 加锁 / 所有 ret 出口解锁」。
- **无** lock/unlock 通用原语、**无**任何原子指令（grep 确认 `code/`、`lib/` 无 `lock xadd`/`cmpxchg`）。
- 本阶段新增：自旋锁 acquire/release + 原子 load/store/fetch_add/sub/and/or/xor/swap/cas（i32/i64 两套宽度）。

### 1.5 异常与跳转的解锁路径（Q6「不 poison，unwind 自动解锁」）
- `emitEhUnwindTo`（`rcc_codegen.cpp:7614`）逐层跑 `EhFrame::finallyBody` 再 `emitEhPop`；
  `emitTry`(:7641) 建帧。`ret`/`break`/`continue` 也经 `emitEhUnwindTo` 退出作用域。
- `lock` 块的解锁以 **try/finally 语义**实现：块正常结束、以及 `ret`/`break`/`continue`/`throw`
  逃出块时，都要走到解锁。实现期以「注入 finally 清理」为首选（见 §3.3 / §6 风险 1）。
- 注意 Q9/B5：任务内异常直接 abort，故任务线程内 lock 持有中 abort 不需解锁（进程结束）。

### 1.6 二等值 guard 与并行检查（Q6 / Q20）
- 二等值语义可复用既有移动状态表 `_moveStates` / 能力路径机制（`rcc_codegen.h:509-534`，
  `rcc_codegen.cpp:1944,2123`）。guard `g` 绑定为 `Mut[T]` 局部，登记「二等值」标记。
- Q20=A：内部可变只在 `Locked`/`Atomic`/`Chan` 三种类型上开例外，写死列举；`Ref[Locked[T]]`/
  `Ref[Atomic[T]]` 在 spawn/par_each 能力检查中按只读计（既有 Ref→R 已覆盖，无需大改）。

---

## 2. 语法与语义（全部已裁决，见 §7 / 记忆 `7abfbb11`）

```
// Locked：构造 + 共享句柄 + 加锁访问
struct Stats { var hits: int }
var counter: Locked[Stats] = Locked.new(Stats(0))
var shared = counter.share()          // Ref[Locked[Stats]]，可传给 spawn 任务
fun bump(c: Ref[Locked[Stats]]) {
    lock c as g {                     // g: Mut[Stats]，二等值，仅块内有效
        g.hits = g.hits + 1
    }
}

// Atomic：构造 + 全套顺序一致原子方法
var n: Atomic[i64] = Atomic.new(0)
n.store(10)
var x = n.fetch_add(1)               // x = 旧值 10
var ok = n.cas(11, 20)               // ok = 读到的旧值（调用方比较）
var m: Atomic[i32] = Atomic.new[int](0)   // i32 宽度
```

- `Locked.new(init)`：`init` 类型推断 T；对象布局 `[+0: 4B lock word][+8: T 值槽]`；返回 `Locked[T]` 句柄（指针）。
- `share()`：`Locked[T]` → `Ref[Locked[T]]`，运行时同一指针，类型层标记「可共享」；不新造引用语义。
- `lock c as g {}`：`c` 求值为 Locked 指针 → 加自旋锁 → `g` 绑定值槽地址（类型 `Mut[T]`）→ 跑块 → 所有出口解锁。
  `g` 是二等值：不能存储到变量、不能返回、子引用不能逃出块、不能作为 spawn 实参。
- 同作用域内对同一把锁重入 → 编译期错误（Q6=A）；不 poison。
- `Atomic.new[T](init)` / 无类型参数时按 init 推断宽度；`Atomic[i32]`/`Atomic[i64]`（i64≡int）。
  方法：`load()` / `store(v)` / `fetch_add(v)` / `fetch_sub(v)` / `fetch_and(v)` / `fetch_or(v)` /
  `fetch_xor(v)` / `swap(v)` / `cas(old,new)`；全部顺序一致；`fetch_*`/`swap`/`cas` 返回**旧值**。
- `Atomic` 永不 `take()`（Q7）；`Ref[Atomic[T]]` 并行检查视只读。

---

## 3. 分层实施（子阶段）

### 3.1 子阶段 4-0：类型与词法基础
- `rcc_sema_types.h`：TypeKind 加 `TK_I32`/`TK_LOCKED`/`TK_ATOMIC`；新增 `I32Type`（8B 槽、整数族）、
  `LockedType(inner)` / `AtomicType(inner)`（指针族，`getInnerType()`）。
- `rcc_core.h`/`rcc_core.cpp`：`TOKEN_LOCK`/`TOKEN_AS` + tokenMap + getTokenTypeName。
- `parseTypeName`：`i32`→I32Type、`i64`→int（别名）；`typeFromTypeArgNode`：`Locked[...]`/`Atomic[...]`
  → LockedType/AtomicType，Atomic inner 只接受 i32/i64/int（否则明确诊断）。
- 验收：`var c: Locked[Stats]` / `var n: Atomic[i64]` / `var m: Atomic[i32]` 能 parse + 类型标注不报错。

### 3.2 子阶段 4-1：运行时 core 原语
- `core_win64.s`：
  - `core_lock_acquire(ptr)` / `core_lock_release(ptr)`：xchgl 自旋锁（仿 core_io_spin），
    release 用 `xorl %eax,%eax; xchgl %eax,(%rcx)` + 屏障语义。
  - 原子族（i64 用 q / i32 用 l）：
    `core_atomic_load/store/fetch_add/swap/cas`（直接指令：mov / xchg / lock xadd / xchg / lock cmpxchg），
    `core_atomic_fetch_op(ptr,val,op)`（cmpxchg 循环实现 and/or/xor；fetch_sub 由 codegen 取负后走 fetch_add）。
    全部 `.globl` + Win64 影子空间/对齐。
- `core.rio`：对应 extern 声明（指针/返回值按 int 处理，i64 值走 int）。
- `coreReturnType`：`core_lock_acquire/release`、`core_atomic_store` → void；其余原子 → int（i64 值）；
  i32 load 结果在 %eax，codegen 显式 `movslq` 符号扩展。
- 验收：C/asm 探针单线程 + 双线程各加 N 次，结果精确（无丢失更新）。

### 3.3 子阶段 4-2：AST / parser / `lock` 语句
- `rcc_ast_components.h/.cpp`：新增 `LockNode final : ExpressionNode`（`Token` + `lockExpr`(ExpressionNodePtr)
  + `guardName`(IdentifierNodePtr) + `body`(BlockRangerNodePtr)）；`NodeType::LOCK` + dump/遍历补分支。
- `buildLockExpression`（仿 `buildScopeExpression`）：`lock` → 表达式（`c`）→ `as` → 标识符（`g`）→ `{ body }`。
  `as` 用 `TOKEN_AS`；诊断明确（缺 as / 缺标识符 / 缺块）。parser 分发表 `TOKEN_LOCK` 登记。
- codegen `emitLock`：
  1. 求值 `c` → Locked 指针存栈槽；取 lockword 地址（+0）与值槽地址（+8）；
  2. `core_lock_acquire(lockword)`；
  3. 为 `g` 分配局部槽存值槽地址，绑定 `Mut[T]`（T 从 `c` 的 `Locked[T]`/`Ref[Locked[T]]` inner 取）；
     登记二等值 guard + 当前活动锁集合（重入检测）；
  4. `pushScope` 后发射 body；
  5. 块末 / 各出口 `core_lock_release(lockword)`（unwind 见 §3.4）；`popScope` 前解锁（Q17 顺序一致）。
- 验收：`lock c as g { g.hits = g.hits + 1 }` 可编译运行，单线程结果正确。

### 3.4 子阶段 4-3：unwind / 二等值静态检查
- **解锁覆盖**：用 try/finally 语义把 release 注入所有退出路径——正常落块末、`ret`（`emitReturn`）、
  `break`/`continue`（`emitBreak`/`emitContinue`）、`throw`（`emitEhUnwindTo`）。
  实现策略（按优先级）：① 若能把 release 注册成 `EhFrame::finallyBody` 则复用现有 unwind；
  ② 否则在 lock 块入口压一个编译期「锁清理栈」，在四个跳转发射点查栈并前置 release。
  **本子阶段必须验证**：`lock` 块内 `throw` 被外层 `try/catch` 捕获后，再次 `lock` 同一个 `c` 不死锁（证明已解锁）。
- **二等值检查**（复用 `_moveStates` / 能力路径）：
  - guard `g` 作为赋值右值/函数实参/`ret` 值 → 报「lock guard is a two-value and cannot escape the block」；
  - `&g.x` 赋给块内变量后带出块 → 拒绝（子引用逃逸）；
  - 同锁重入 → 报「reentrant lock on the same Locked value」；
  - `lock` 接收者非 `Locked[T]`/`Ref[Locked[T]]` → 报类型错误。
- **并行检查**：`share()` 结果按 `Ref[Locked[T]]`（只读能力）参与 spawn/par_each 检查；
  把 guard 作为 spawn 实参 → 因「二值不能逃逸」被拒。
- 验收：负面用例（guard 逃逸 / 子引用逃逸 / 重入 / 非 Locked 接收者 / guard 作 spawn 实参）全部编译期拒绝。

### 3.5 子阶段 4-4：codegen — Locked.new / share / Atomic 方法
- `emitFunctionCall` DOT 静态分支：
  - `Locked.new(init)`：分配 16B 对象，init 写入 +8，返回指针（`Locked[T]`）；
  - `Atomic.new[T](init)` / `Atomic.new(init)`：按 T 宽度分配 8B（i32 用低 4B），store init。
- `emitMethodCall` / DOT 特化按接收者静态类型：
  - `share()` on `Locked[T]` → 返回同指针，类型 `Ref[Locked[T]]`；
  - 原子方法 on `Ref[Atomic[T]]` → 取对象地址作 ptr 实参调对应 core 原语；i32 结果 `movslq` 扩展；
    `fetch_sub` = 取负 + `fetch_add`；`fetch_and/or/xor` = `core_atomic_fetch_op` + op 码。
- `inferType` 镜像：`Locked.new`→LockedType、`share()`→Ref[Locked]、原子 load/fetch/swap/cas→T 宽度类型、
  store→void。
- 验收：§2 全部正向写法可编译运行，结果正确。

### 3.6 子阶段 4-5：测试套件 + 全量回归
- 新套件 `tests/agent/p14-4-locked-atomic`：
  - 正向：lock 互斥计数（多线程各加 N 次无丢失）、share 跨任务、Atomic i64 计数、Atomic i32、
    全套原子方法、lock 内 throw 后解锁（§3.4 验证）。
  - 拒绝：guard 逃逸、子引用逃逸、重入、非 Locked 接收者、guard 作 spawn 实参、atomic.take()。
  - 压力：多任务多线程重复加减，结果恒定。
- 全量 `tests/agent` 回归无回归；探针归档 `docs/p14_4_probes/`。

---

## 4. 设计空白裁决记录（2026-11-20，用户拍板，全部闭合）

### 4-A. Locked 的共享引用接口
**裁决**：新增专用内建方法 `locked.share() -> Ref[Locked[T]]`（规范 §10.6 写法）。
不复用通用 `ref()`：复用会让「引用数据」（现有 `ref()` 语义）与「引用锁包装器」（此处语义）反转，
构成绕过锁的 footgun，且与规范写法不一致；实现成本与复用几乎相同。

### 4-B. Atomic 类型落地
**裁决**：新增 `i32` 内建 32 位类型 + `Atomic[i32]`/`Atomic[i64]`；`i64` 对齐现有 `int`（64 位），
顺序一致（Q7=A）。原子操作按宽度分别发射 32/64 位 lock 前缀指令。

### 4-C. 原子操作接口
**裁决**：全套 `load/store/fetch_add/fetch_sub/fetch_and/fetch_or/fetch_xor/swap/cas`，均顺序一致；
`cas(old,new)` 返回**读到的旧值**（调用方自行比较），`fetch_*`/`swap` 返回旧值。

---

## 5. 验收标准

1. `lock c as g { ... }` 编译运行，多线程互斥更新无丢失；`g` 为二等值，逃逸/重入编译期拒绝。
2. `share()` 传 `Ref[Locked[T]]` 给 spawn/par_each 任务，能力检查按只读放行。
3. `Atomic[i64]`/`Atomic[i32]` 全套方法顺序一致；cas/fetch_* 返回旧值；并发计数无丢失。
4. unwind：`lock` 块内 `throw` 被捕获后同一锁可再次 acquire（证明自动解锁，Q6 不 poison）。
5. Q20 例外清单写死三种内部可变类型；`Ref[Locked]`/`Ref[Atomic]` 并行检查视只读。
6. 新套件全绿 + 全量 `tests/agent` 无回归。

---

## 6. 风险

1. **unwind 解锁是最大难点**：Rio 的异常是显式帧链 + `longjmp` 风格，`ret`/`break`/`continue`/`throw`
   十条出口都要覆盖。首选复用 `EhFrame::finallyBody`（§3.4）；若不可行，退化为编译期锁清理栈，
   并在文档如实标注覆盖范围。**必须**有「throw 后同锁可再 acquire」的验证用例兜底。
2. **i32 宽度与 ABI**：Rio 无 32 位类型先例，`i32` 值进入 64 位表达式需 `movslq` 符号扩展；
   槽分配仍按 8B，仅原子存储用低 4B。需在测试里覆盖负数（验证符号扩展）与边界值。
3. **二等值检查精度**：guard 的「子引用逃逸」检查依赖能力路径机制，保守实现可能误拒合法写法；
   首版宁可拒绝并给明确诊断，也不放过逃逸（安全优先，符合 Q4 严格度）。
4. **重入检测范围**：只检同一词法作用域内对同一 Locked 的嵌套 `lock`；跨函数调用链的重入不检测
   （与 Q13「跨函数未覆盖」口径一致），文档如实标注。
5. **core 原语 clobber**：仿 P14-3 踩坑（记忆 `873af480`），`core_lock_*`/`core_atomic_*` 的 `%rax`
   结果在后续索引计算前先存专属栈槽，避免被覆盖。
6. **构建工具链路径**：已迁移至 `D:\soft\Clion\bin\mingw`（记忆 `44feb632`），
   测试 PATH 需含 `bin` 与 `x86_64-w64-mingw32\bin`；旧路径已失效。

---

## 7. 完成记录（2026-11-20）

### 7.1 交付

| 子阶段 | 交付 | 文件 |
|---|---|---|
| 4-0 | `TK_I32`/`TK_LOCKED`/`TK_ATOMIC` 类型 + `TOKEN_LOCK`/`TOKEN_AS` + `LockNode` AST/parser + `i32`/`i64` 与 `Locked[...]`/`Atomic[...]` 标注解析 | rcc_sema_types.h、rcc_core.h/.cpp、rcc_ast.h/rcc_ast_components.h/.cpp、rcc_ast_dump.cpp、rcc_parser.h/.cpp、rcc_prefix_builder.cpp、rcc_codegen.cpp |
| 4-1 | core 自旋锁 `core_lock_acquire/release` + 原子 14 原语（load/store/fetch_add/fetch_op/swap/cas，i32/i64）+ 三点登记（core_win64.s/core.rio/coreReturnType） | lib/core/core_win64.s、core.rio、rcc_codegen.cpp |
| 4-2 | `lock c as g {}` 语句：`emitLock`（求值 Locked 句柄 → lockword 加锁 → g 绑定 Mut[T] 值槽 → 块体 → 解锁）+ 解锁在 popScope 前 | rcc_codegen.cpp/.h |
| 4-3 | unwind 解锁：throw/ret 逃出 lock 块前按 `_activeLocks` 栈内到外 `core_lock_release`；重入编译期拒绝；guard 二等值逃逸（存储/返回/实参/子引用）检查 | rcc_codegen.cpp/.h |
| 4-4 | `Locked.new(init)`（struct 内联字节拷贝）/ `share()` → Ref[Locked[T]]；`Atomic.new[T](init)` + 全套原子方法派发 + inferType 镜像 | rcc_codegen.cpp |
| 4-5 | 新套件 `tests/agent/p14-4-locked-atomic` 9 用例 + 全量回归 | tests/agent/p14-4-locked-atomic/ |

### 7.2 关键实现决定

- **struct 内联归属**：`Locked.new(Stats(0))` 的 init 值在 %rax 里是**指向 struct 的指针**；对象值槽必须内联 struct 字节（`emitStructCopy`），guard `g: Mut[Stats]` 指向 obj+8 才能正确字段访问。曾因存指针导致读垃圾（实测），用 init 表达式的实际类型判 struct 修之。
- **i32 显式标注**：原子 i32 无法从 int 字面量推断，`Atomic.new[i32](v)` 的 `[T]` 标注必须优先于推断；且 `pool.new[T]` 的三处通用判断（inferType / poolNewTypeArg / poolNewReceiverName）必须排除 Locked/Atomic 接收者，否则 `Atomic.new[i32]` 被误判为 pool.new 导致类型错误。
- **unwind 解锁范围**：Q6 要求「unwind 自动解锁」，Rio 的异常是 setjmp/longjmp 风格（throw 直接长跳，跳过普通控制流）。首版在 **throw / 函数 ret** 两条出口发射解锁（均在 lock 块内可达）；`break`/`continue` 逃出 lock 块的解锁**未覆盖**（见 §7.4）。
- **guard 二等值口径**：禁止「guard 本身作存储/返回/实参」与「`&g`/`&g.x` 子引用逃逸」；通过 guard 读标量字段（`g.hits`）是块内合法访问，不拦（避免误拒所有正常用法）。
- **clobber 防护**：core 原语只改 caller-saved 寄存器；`emitLock`/原子方法在调用前把 receiver 地址存入专属栈槽，避免后续计算覆盖（P14-3 同类踩坑）。

### 7.3 验证

- 新套件 `tests/agent/p14-4-locked-atomic` **9/9**（正向 5：lock 计数 12 / share 4 任务 ×100=400 / Atomic i64 全套 / Atomic i32 负数与符号扩展 / throw unwind 后再锁=2；拒绝 4：重入 / guard 实参逃逸 / 子引用逃逸 / 非 Locked 接收者）。
- share+spawn 互斥：4 线程各加 100 次，5 次运行恒为 400，无丢失更新。
- 全量 `tests/agent` 36 套件 **1006 项全通过 0 失败**（742 标准格式 + 44 p12-3c-ds + 220 std-full；原基线 997 + 新增 9）。
- core 原语 C 层探针 `cases/temp-test/p14_4_core_probe.c` 全覆盖（i32/i64 全套方法 + 自旋锁），ALL OK。
- 探针归档 `cases/temp-test/p14_4_*`。

### 7.4 已知限制 / 后续

- **`break`/`continue` 逃出 lock 块的解锁未覆盖**：首版只对 throw/ret 发射解锁；lock 块内若有跳出循环的 break/continue，锁可能遗留。登记为 P14-4 后续项（需与循环 drop-depth 协同）。
- **跨函数重入不检测**：只检同一词法作用域内对同一 Locked 的嵌套 lock（与 Q13「跨函数未覆盖」口径一致）。
- **i32 类型仅用于原子**：`i32` 作为独立内建类型加入，但普通算术/打印路径未全面适配 32 位语义（当前仅保证原子操作宽度与符号扩展正确）。
- 后续阶段：P14-5 Chan、P14-6 验收（粒子系统）。

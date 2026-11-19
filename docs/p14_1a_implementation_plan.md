# P14-1a 实施方案：运行时前置（线程 spike + Q14 线程安全地基）

> **本文性质**：实施方案草案（2026-11-19），待用户确认后开工。
> 依据：`docs/p14_plan.md` §7 全部裁决（Q1–Q20 闭合）、`docs/p14_design_audit.md`、
> `memory-model-spec-full.md` §10，以及 codegraph 对当前代码库的实测。
> 遵守硬规则 `1b1ccafb`：本阶段**不引入任何 Rio 语法/类型/原语**，只搭建运行时地基，
> 所有语言层设计（`scope`/`spawn`/`par_each`/`Locked`/`Chan`）留 P14-1b 及之后。
> 立项依据还有 Claude 的阶段建议：「P14-1 拆成 1a 运行时前置 + 1b scope/spawn」。

---

## 0. 目标与边界

**做**：
1. 确认目标 ABI 与运行时链接方式（已实测，见 §1）；
2. 最小线程 spike：在 core 层提供「创建线程 + join」两个原语，用一段荧光笔式的最小样例证明端到端可用；
3. Q14 = C 混合方案的运行时地基：全局堆加锁（支持跨线程 free）+ TLS 状态（出生地栈/异常指针/errno/全局 drop 表）+ stdout 输出锁；
4. 全局可变状态审计：逐项列出生成代码触及的全局状态，决定各自的线程安全处理；
5. 一套可复用的并发测试脚手架（负面/压力用例框架，TSAN 不可用）。

**不做**（本阶段不碰，登记后续）：
- `scope` / `spawn` / `par_each` / `Locked` / `Atomic` / `Chan` 的任何语法、类型、语义、检查器；
- Q17 的「join 先于局部 drop」序列（属 P14-1b）；
- Q9 任务内异常 abort 的语义落地（属 P14-1b；本阶段只保证异常机制本身多线程不破坏）；
- Q16 并发 Pool 类型标记、Q7 Atomic intrinsic（属 P14-3/P14-4）；
- 线程局部缓存优化（Claude 明确不属 P14）。

---

## 1. 关键事实（codegraph 实测，2026-11-19）

### 1.1 目标 ABI 与链接（Claude Q15 前置「目标 OS/ABI 确认」）
- **平台**：Windows x64，MS x64 ABI。`code/include/backend/rcc_abi.h` 已实测确认寄存器分配按整体参数位置（rcx/rdx/r8/r9 与 xmm0-3 共用位置）、pos>=4 进栈、32 字节 shadow space、16 字节对齐、RAX/XMM0 返回。已有 `AbiInfo` 抽象，将来加 Linux/SysV 只需新增实现。
- **链接**：`code/src/backend/rcc_toolchain.cpp` 用 MinGW `g++.exe` 链接，**借用它提供的 CRT 与链接脚本**（`ToolchainConfig.linker` 注释）。→ 线程 API 直接用 **Win32 `CreateThread` / `WaitForSingleObject`** 即可，无需自己实现 TLS 或系统调用层；MinGW CRT 提供 `malloc/free` 与线程本地存储。
- 结论：**Q1 的「每任务一个 OS 线程」在 Win32 上 1:1 直接可行**，不需要 pthread/协程。

### 1.2 分配器（Q14 的「全局堆一把锁」落地点）
- `lib/core/core.rio` 冻结 ABI v1：`alloc/realloc/free`（`core_alloc/core_realloc/core_free`），`align` 为 2 的幂；签名变更须提升 ABI 版本并同步 `core_win64.s`。
- `core_win64.s` 实现全部 core 例程（`.globl` 导出），`rcc_codegen.cpp:469` 的 `coreReturnType()` 登记非 int 返回（alloc/realloc/pool_new/pool_alloc → ptr）。
- 三点登记法（沿用 P13 惯例）：① `core_win64.s` 汇编实现 + `.globl`；② `core.rio` extern 声明；③ `coreReturnType()` 非 int 返回登记。
- 现状：分配器是全局的，无锁 → 多线程同时 alloc/free 会破坏堆结构。Q14=C 要求给全局堆加一把锁，且**必须允许跨线程 free**（take 进任务的值由任务线程析构）。

### 1.3 异常机制（Q9/E 项的约束）
- `emitThrow`（`rcc_codegen.cpp:7614`）调用 `core_eh_throw`，长跳到 handler 或 abort；`emitEhUnwindTo`（:7589）跑 finally + `core_eh_pop` 摘链；`emitTry`（:7640）建 try 帧。
- 异常对象归属已裁决「推迟，维持 D1 异常即泄漏」（记忆 `8ff5d19d`，E 项闭合）→ 与 Q9「任务内异常直接 abort」兼容，P14-1a 不需要设计异常对象销毁。
- 注意：handler 链（`core_eh_pop`/栈上的 EhFrame）当前是**全局/栈帧级**的，多线程下各线程的 try 帧不能互相干扰 → 需确认链头是否要放 TLS（见 §3 审计项 A3）。

### 1.4 作用域与 drop（Q17 的落地点，留 P14-1b）
- `pushScope/popScope`（:3392/:3398）只维护编译期符号表 `_scopes`、drop 表 `_scopeDrops`、移动状态 `_moveStates`；`emitScopeDrops`（:2570）按声明逆序发 drop 调用。当前与线程无关。
- 本阶段不动这些，仅在 §5 的 spike 里验证「任务线程释放主线程分配的对象」这一物理能力。

---

## 2. 阶段目标拆分（P14-1a 内部顺序）

| 子阶段 | 内容 | 验收物 | 状态 |
|---|---|---|---|
| **1a-0** | 目标 ABI 与工具链确认（§1）+ 全局状态审计（§3） | 审计表 + 结论 | ✅ 完成（2026-11-19） |
| **1a-1** | core 层加线程原语：`core_thread_create` / `core_thread_join`（Win32 CreateThread/WaitForSingleObject） | 三点登记 + 汇编实现 + C 层探针 | ✅ 完成（2026-11-19） |
| **1a-2** | Q14 全局堆加锁 + 跨线程 free 验证 | 分配器改造 + 压力用例 |
| **1a-3** | TLS 状态：出生地栈、异常指针、errno、全局 drop 表按 §3 结论迁移 | TLS 访问原语 + 对照用例 |
| **1a-4** | stdout 输出锁 | core_io_write 系列加锁 + 并发输出用例 |
| **1a-5** | 最小端到端 spike：一个 Rio 样例，主线程 alloc 一个对象、子线程写入并 free、join 后主线程继续 | 通过用例 + 回归登记 |

> 顺序理由：先确认 ABI/状态（1a-0），再建线程能力（1a-1），然后逐个解决线程安全（1a-2/3/4），最后端到端验证（1a-5）。
> 每完成一个子阶段停下向用户汇报（沿用 P13-3 的「逐 Q 汇报」流程记忆 `b7ac86a3`）。

---

## 3. 全局可变状态审计（1a-0 已完成，2026-11-19 实测）

> 逐项对照 `core_win64.s` / `core.rio` / codegraph 源码确认。**第 3 点（分配器）已由实测定案，见 3.1。**
> **进度**：1a-0 ✅ → 1a-1 ✅ → 1a-2 ✅（见 3.2）→ 1a-3 ✅（见 3.3）→ 1a-4 ✅（见 3.4）→ 1a-5 ✅（端到端 Rio spike + 1000 次压力 + 全量回归，见 §9）。**P14-1a 全部完成（2026-11-19）。**

| 状态 | 实测现状（源码位置） | Q14=C 的处理 | 结论 |
|---|---|---|---|
| 全局堆 | `core_alloc`/`core_free`/`core_realloc` 转发 `GetProcessHeap` 的 `HeapAlloc/HeapFree`（`core_win64.s:140/257/190`） | **不重写堆**；实测证明底层线程安全，见 §3.1 | ✅ 无需加堆锁 |
| 调试计数/ring | `core_dbg_*` 全是 `.bss` 全局 8 字节 + 128 字节 recent ring（`:600-617`），`addq $1, ...(%rip)` 非原子 | **加一把全局锁**保护所有 `core_dbg_*` 读写（含 alloc/free 内的 bump 与 ring 扫描） | ⚠ 本阶段要改 |
| 异常 handler 链 | `core_eh_top` 是**全局变量**（`core_eh_push/pop/throw` 直接读写 `:941-972`），单链头 | **迁 TLS**：每线程独立 `core_eh_top`，否则多线程 throw/push/pop 串台 | ⚠ 本阶段要改（硬需求） |
| 出生地栈 | codegraph 确认：`pushScope/popScope` 只维护编译期 `_scopes/_scopeDrops/_moveStates`（`rcc_codegen.cpp:3392`），**无运行时出生地栈** | 无需迁移（Claude 设想的运行时出生地栈不存在） | ✅ 无需处理 |
| drop 标志表 | 无全局表；drop 是编译期登记（`emitScopeDrops :2570`），运行时只有 drop glue 调用 | 无需迁移 | ✅ 无需处理 |
| errno / 错误状态 | 生成代码不直接使用 errno；MinGW CRT 的 errno 已是 TLS | 确认即可 | ✅ 无需处理 |
| stdout / io_write | `core_io_write`（`:682`）等未加锁 | 加全局输出锁，每次 write 持锁（按 Q14=C） | ⚠ 本阶段要改 |

### 3.1 第 3 点决定（用户授权我实测后决定）：分配器不重写，降级为「确认底层安全 + 保护 RCC 自有全局状态」

**实测**（`cases/temp-test/p14_1a_heap_probe.c`，2026-11-19）：8 线程 × 20 万轮 × 2 次 alloc/free = **320 万次**并发分配释放，`failures = 0`，`HeapValidate = OK`。

**结论**：Windows `GetProcessHeap` 返回的进程堆在未设 `HEAP_NO_SERIALIZE` 时由系统串行化，`HeapAlloc/HeapFree` **本身线程安全**，且天然支持跨线程 free（同一进程堆）——正好满足 Q14=C「支持跨线程释放」的核心诉求。因此**不重写堆、不在 alloc/free 外层再加一把堆锁**（重复且无收益，且 Q14=C 的本意是满足约束的最小实现）。

**真正需要加锁的是 RCC 自己叠在堆之上的全局调试状态**：`core_dbg_alloc_count` / `free_count` / `live_bytes` / `double_free_count` / `poison_bytes_total` / `recent[16]` / `recent_idx`。这些是非原子 .bss 读写，多线程下计数会丢、ring 会撕裂。方案：加一把 `CRITICAL_SECTION` 或 `SRWLOCK`（共享/独占：计数用独占），在所有 `core_dbg_*` 读写处加解锁（含 `core_alloc` 成功 bump、`core_free` 的 dup 扫描与 bump、`core_dbg_reset` 及各 getter）。

> 注：调试钩子只在 `-debug-alloc` 构建里被引用（`core_win64.s:153-154` 注释），发布构建计数恒 0 也无害。但压力测试要用计数证明正确性，故本阶段必须锁好。

**对 §2 子阶段的影响**：1a-2 从「给堆加锁」改为「给调试全局状态加锁 + 验证跨线程 free」；改动面显著缩小，风险更低。

### 3.2 1a-2 完成记录（2026-11-19）

- **计数原子化**：`core_dbg_bump_alloc/free`、core_alloc/free 内的 `alloc_count/free_count/poison_bytes_total/double_free_count` 全改 `lock addq`（x64 lock 前缀原子加）。
- **recent ring 改 TLS**（关键修正）：初版只加全局自旋锁，压力测试暴露 **11/402000 次误判重复释放**——根因是「全局地址 ring + 地址复用」存在 ABA 式竞态：线程 A free 写 ring 与线程 B alloc 清 ring 交错，导致仍存活的地址留在 ring 被误判，命中后**跳过真正的 HeapFree → 内存泄漏**。加锁保护不了跨 alloc/free 的时序。按 Q14=C「线程状态归线程」把 ring 改为 **每线程独立 136 字节 TLS buffer**（`TlsAlloc` 惰性初始化 + `HeapAlloc` + `TlsSetValue`，分配后必须清零 idx@+128，否则 ring_add 越界崩溃——实测踩过）。每线程只看自己释放过的地址，跨线程地址复用不再互相污染。
- **core_alloc/free 栈帧重写**：core_alloc 改用 `push rbx` + `sub $48` 保存 size 与分配指针（不再借 callee-saved rsi）；core_free 的 dup 扫描/ring 写入改调 TLS helper。所有 call 点保持 rsp≡0、shadow 已留。
- **验证**：`cases/temp-test/p14_1a_dbg_probe.c`，8 线程 × 5 万轮 churn + 2000 次跨线程 free，共 40.2 万次 alloc/free：`allocs=402000 frees=402000 dup=0 failures=0`，RESULT PASS。单线程 20 万次基线 dup=0。构建全绿。

### 3.3 1a-3 完成记录（2026-11-19）
- **core_eh_top 迁 TLS**：原 `.data` 全局单链头改为每线程独立 8 字节 TLS slot（`core_eh_get_slot`：TlsAlloc 惰性初始化 + HeapAlloc(8) + 清零 + TlsSetValue）。`core_eh_push/pop/throw` 全部改经本线程 slot 读写链头，跨线程 throw 不再串台。
- **踩坑**：①程序化替换时漏写 `.globl core_eh_push/core_eh_pop`，链接报 undefined reference（p7 套件 16 用例链接失败）；②`core_eh_throw` 首版在 `popq %rbx` 后才写 `frame->exc`，而 rbx 保存的 exc 已被 pop 覆盖 → 异常值丢失（8 用例值断言失败，如实际 111/222）。修法：在恢复栈前用 rbx（仍为 exc）先写 `frame->exc`，并单列 `.Lthrow_uncaught` 分支保证栈平衡。
- **验证**：`p7-exceptions` 回归 17/17 全通过（throw/catch、嵌套、跨函数、finally、重抛、uncaught abort）；`cases/temp-test/p14_1a_eh_probe.c` 8 线程 × 2 万次 push/pop 隔离探针 failures=0 PASS。构建全绿。

### 3.4 1a-4 完成记录（2026-11-19）
- **全局输出锁**：`.data` 加 `core_io_spin`（4 字节自旋锁）；`core_io_write` 与 `core_io_write_byte` 在 WriteFile 前用 `xchgl` 加锁、所有 ret 出口解锁。xchg 只改 eax，不破坏 fd(ecx)/ch(dl)；解锁用 edx（此时已无用）。锁粒度 = 单次 write，符合 Q14=C「stdout 全局一把输出锁，写一次持锁一次」。
- **验证**：`cases/temp-test/p14_1a_io_probe.c` 8 线程各写 500 行 64 字符同字符行，重定向后校验 4000 行**零交错**、write 全部成功。回归 p12-3b-io-str 25/25、p7-exceptions 17/17、p13-memory 28/28 全通过。构建全绿。

### 1a-1 完成记录（2026-11-19）
- **三点登记**：① `core_win64.s` 加 `core_thread_create`（CreateThread，6 参栈布局：rcx/rdx/r8/r9 + rsp+32 flags + rsp+40 threadid，3 push+sub48 保持 16 字节对齐）与 `core_thread_join`（WaitForSingleObject(INFINITE)+CloseHandle，null 直接返回）；② `core.rio` 加 extern `core_thread_create(fn:ptr,arg:ptr):ptr` / `core_thread_join(handle:ptr):void`；③ `coreReturnType()` 登记 create→ptr、join→void。
- **构建验证**：`mingw32-make -C build -j10` 全绿（汇编 core_win64.s、编译 codegen.cpp、链接 RCC、自宿主 rt.rio 全通过）。
- **C 层探针**（`cases/temp-test/p14_1a_thread_probe.c`）：链接 release core_win64.o，主线程传 42 → 子线程读到 42 并改写 999 → join → 主线程读到 999，`child_ran=1`，RESULT PASS。证明 Win64 栈布局、参数传递、线程入口 ABI 全部正确。

---

## 4. core 层线程原语设计（1a-1）

### 4.1 新增原语（三点登记法）
| 原语 | 签名（core.rio extern） | 语义 | ABI |
|---|---|---|---|
| `core_thread_create` | `extern fun thread_create(fn: ptr, arg: ptr): ptr` | 创建 OS 线程，入口 `fn(arg)`，返回线程句柄 | 返回 ptr；参数 fn/arg 为指针 |
| `core_thread_join` | `extern fun thread_join(handle: ptr): void` | 等待线程结束并回收句柄 | void |

- **实现位置**：`lib/core/core_win64.s`（Win32 `CreateThread`/`WaitForSingleObject`/`CloseHandle`）。
- **三点登记**：① 汇编 `.globl` + 实现；② `core.rio` 加 extern 签名；③ `rcc_codegen.cpp:coreReturnType()` 加 `core_thread_create → ptr`、`core_thread_join → void`。
- **ABI 版本**：core.rio 头部注明「ABI_VERSION = 1」为注释，本次新增原语不改既有签名/布局 → 是否需要提升 ABI 版本须确认（建议提升并注明，因新增了种子层契约面）。

### 4.2 约束
- 借用 `rep movsb`/`rep stosb` 例程的自存自恢复 `%rdi/%rsi` 铁律（记忆 `85812347`）；新例程若用到这些寄存器同样遵守。
- 3 push + `sub $N` 保持 16 字节对齐；跨 core 调用活值放栈槽（`p13_7_implementation_plan.md` §1 的 ABI 铁律）。
- 线程入口 `fn` 的 ABI：与 Rio 函数一致（Win64 MS ABI），arg 通过 `%rcx` 传入。需确认子线程入口的栈/帧初始化与主线程一致（1a-5 spike 验证）。

---

## 5. 端到端 spike（1a-5）

```
// 伪 Rio，仅示意；本阶段不引入 scope/spawn 语法，spike 可直接调用 core 原语
fn child(p: ptr) {
    let n: int = ... // 从 p 读
    ...             // 写
    core_free(p)    // 子线程释放主线程分配的对象 —— 验证跨线程 free
}
fn main() {
    let p: ptr = core_alloc(16, 8) in local   // 主线程分配
    ...                                       // 主线程填充
    let h: ptr = core_thread_create(child, p) in local
    core_thread_join(h)                        // join
    ...                                       // 主线程继续，确认无崩溃/无泄漏误报
}
```

**验收**：
1. 编译链接通过（`rcc -build` 走完整链）；
2. 运行无 crash、无 double-free、无堆损坏；
3. `core_dbg_allocations`/`core_dbg_frees` 计数平衡（分配=释放）；
4. 重复跑 N 次（如 1000 次）压力测试稳定。

---

## 6. 测试脚手架

- 位置：`tests/agent/p14-1a-thread`（新建，参照现有 `tests/agent/p13-*` 结构）。
- 内容：① 最小线程 spike 正向用例；② 并发 alloc/free 压力用例（多线程反复分配释放）；③ 全局状态审计中每项的针对性用例；④ 编译失败/运行崩溃类负面用例框架（为 P14-2 的检查器负面用例预铺）。
- 注意：自制编译器无法用现成 TSAN，压力测试靠重复次数 + 计数平衡 + 显式锁的正确性推理。

---

## 7. 风险与待确认

1. **MinGW CRT 的线程安全程度**：`g++` 链接的 CRT 自带的 malloc/free 是否本身已线程安全？若已安全，Q14=C 的「全局堆加锁」可能只需确认而非重造。**实施第一步先实测**：多线程直接调 alloc/free 是否崩溃。若 CRT 已安全，则本阶段的分配器改造降级为「确认 + 计数加锁」，减少改动面。
2. **handler 链是否全局**：需读 `core_win64.s` 的 `core_eh_throw`/`core_eh_pop` 实现确认链头位置；若全局，必须迁 TLS，否则多线程 throw 会串台。
3. **ABI 版本提升**：新增 core 原语是否算「签名变更」需确认；建议提升并同步 core.rio 头注释。
4. **FPU/SIMD 状态**：Win32 线程创建时 FPU 状态是默认初始化的（CreateThread 保证），需确认浮点参数跨线程传递正确（1a-5 可加一个 float 参数用例）。

---

## 8. 确认清单（请用户确认后开工）

1. 本阶段严格限定为运行时地基、**不引入任何语言语法**——是否同意？
2. 1a 内部拆成 6 个子阶段、每完成一个停下汇报——是否沿用 P13-3 的「逐 Q 汇报」节奏？
3. ~~Q14=C 的分配器加锁是否降级~~ **已由我实测决定（用户授权）**：8 线程 320 万次 alloc/free 零失败、HeapValidate OK → GetProcessHeap 本身线程安全且支持跨线程 free，**不重写堆、不加堆锁**；改为保护 RCC 自有的 `core_dbg_*` 全局状态（见 §3.1）。
4. 线程 API 选 Win32 CreateThread（而非 pthread/协程）——基于已确认的 Windows x64 + MinGW CRT，**用户已同意**。

### 用户确认记录（2026-11-19）
1. ✅ 本阶段严格限定运行时地基、不引入语言语法。
2. ✅ 拆 6 个子阶段、每个停下汇报（沿用 P13-3 逐 Q 汇报节奏）。
3. ✅ 第 3 点交由我实测决定，结论见 §3.1。
4. ✅ 线程 API 选 Win32 CreateThread。

> 流程变更（2026-11-19）：用户后续要求「直接设定目标，期间不用再确认」，故从 1a-1 起不再逐子阶段停下，改为每完成一个汇报进度后继续，直至全阶段完成。

---

## 9. 完成记录（2026-11-19）

### 9.1 交付清单

| 子阶段 | 交付 | 验证 |
|---|---|---|
| 1a-0 | 全局状态审计（§3）+ 分配器线程安全实测（§3.1） | 8 线程 320 万次 alloc/free 零失败 |
| 1a-1 | `core_thread_create` / `core_thread_join` 三点登记 | C 探针：跨线程读改写 PASS |
| 1a-2 | 计数 `lock addq` + recent ring 改 TLS（§3.2） | 8 线程 40.2 万次：allocs=frees=402000 dup=0 |
| 1a-3 | `core_eh_top` 改 TLS（§3.3） | p7-exceptions 17/17 + 隔离探针 16 万次 |
| 1a-4 | `core_io_write`/`write_byte` 全局输出锁（§3.4） | 4000 行零交错 |
| 1a-5 | 端到端 Rio spike + 正式测试套件 `tests/agent/p14-1a-thread` | 1000 次线程压力 + 全量回归 981 项 |

### 9.2 线程入口设计决定（1a-5 实测暴露）

- **问题**：Rio 的具名函数值 emit 成闭包对象（`.Lfnclosure_child: .quad child`），`core_thread_create(child, p)` 收到的 fn 是闭包对象地址而非裸函数入口；直接跳过去执行会崩溃（e2e 首跑 exit 0xC0000005）。
- **决定**：`core_thread_create` 在汇编里对 fn 解引用一次（`movq (%r12),%r12`）取真实入口。这与 Q3 裁决「首版只允许 spawn 具名顶层函数」一致——无捕获闭包对象首字即函数地址。**带捕获闭包不作为线程入口**（其闭包对象布局不同），属后续设计，须另行裁决。
- 此项是运行时对新后端「函数值表示」的适配，非新增语言语法；core.rio 的 `core_thread_create(fn: ptr,...)` 签名不变。

### 9.3 关键踩坑汇总

1. **1a-2**：全局地址 ring 在并发地址复用下有 ABA 竞态（误判重复释放→跳过真正 HeapFree→泄漏），加锁不解决，必须改 TLS；TLS buffer 分配后需清零 idx@+128，否则 ring_add 越界崩溃。
2. **1a-3**：程序化替换漏写 `.globl` 导致链接失败；`core_eh_throw` 写 `frame->exc` 必须在 `popq %rbx` 前（否则 exc 被覆盖→异常值丢失，8 用例值断言失败）。
3. **1a-5**：Rio 函数值=闭包对象，线程入口需解包一次（见 §9.2）。

### 9.4 全量回归

`tests/agent` 全部 32 个套件：**717（主统计）+ 44（p12-3c-ds）+ 220（std-full）= 981 项全通过，0 失败**。core 层改动未破坏任何既有功能。

### 9.5 状态

**P14-1a 完成**。运行时地基已就绪：线程原语、跨线程安全的分配/释放、TLS 异常链、输出锁全部验证通过。下一步 P14-1b（scope/spawn 语言层）需在 P14 各裁决基础上另写实施方案。

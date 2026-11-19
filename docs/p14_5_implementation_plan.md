# P14-5 实施方案：`Chan<T>` 通道（有界 / 阻塞 / 显式 close）

> **本文性质**：实施方案（2026-11-20）。依据已裁决的 P14 全部设计（`docs/p14_plan.md` §7，Q1–Q20）、
> 规范 §10.7（`docs/memory-model-spec-full.md`）、2026-11-20 用户对 Chan 五个接口空白的逐条裁决（§4），
> 以及 codegraph 对当前代码库的实测（§1）。
> 遵守硬规则 `1b1ccafb`：本文用到的每个类型/方法/接口都在裁决清单内，无自创设计混入。
> 前置：P14-1a（运行时地基）、P14-1b（scope/spawn）、P14-3（par_each）、P14-4（Locked/Atomic）均已完成并全量回归通过。

---

## 0. 阶段定位

- 阶段路线（`p14_plan.md §2`）：P14-5 → P14-6 验收。
- 本阶段交付 §10.7：`Chan<T>` 有界通道 + `send`（消耗 Content）/ `recv`（返回 Content?）/ `close` / `share`，
  以及 scope 结束时对 scope 内创建的 Chan 自动关闭。
- 竞争排除论证中本阶段负责的一条：**send→recv 建立 happens-before，recv 在接收方出生地创建 Content，无共享无竞争**（Q15 / 规范 §10.7）。
- 每完成一个子阶段汇报进度后继续（沿用 1a/1b/3/4 自主推进方式）。

---

## 1. 关键事实（codegraph 实测，2026-11-20，逐条带出处）

### 1.1 类型系统：可完整复用 P14-4 的同步包装类型骨架
- `TypeKind`（`code/include/sema/rcc_sema_types.h:21-41`）现有 `TK_I32/TK_LOCKED/TK_ATOMIC`；
  `SyncWrapperType(kind, inner)`（:419-440，带 `getInnerType()`）是 `Locked[T]`/`Atomic[T]` 的共用底座。
  新增 `TK_CHAN` + `ChanType(inner)`（复用 `SyncWrapperType` 或同构子类），`toString()="Chan"`，
  `isPointerLike()`（:70-77）加入 `TK_CHAN`。
- 类型标注解析：`typeFromTypeArgNode`（`rcc_codegen.cpp:12019`）的 `Body/Content/Ref/Mut/Take` 分支（:12054）与
  `Locked[...]`/`Atomic[...]`（P14-4 新增）是 `Chan[...]` 的模板；`poolNewTypeArg`（:6610）与
  `poolNewReceiverName`（:1276）的「排除 Locked/Atomic 接收者」判断须一并排除 `Chan`，
  否则 `Chan.new[T]` 被误判成 `pool.new[T]`（P14-4 同类踩坑）。
- `Content[T]` 是 `TK_MEMORY`（`rcc_sema_types.h:524`），`MemoryType` 属 pointer-like（:70-77），
  因此 **`Content[T]?` 可直接用现有 `?` 标注**（`typeFromLabelNode`，:12190-12216），
  `recv()` 返回 `Content[T]?` 无需新增语法；窄化沿用 `if var m = ch.recv() { ... }`（:7279-7293）。

### 1.2 Chan 方法派发落点（全部是方法名，无新关键字）
- 静态构造 `Chan.new[T](capacity)` / `Chan.new[T]()`：仿 P14-4 的 `Locked.new`/`Atomic.new`，
  落 `emitFunctionCall` 的 DOT 静态分支（:10660+，约 :10770-10840 的 isLocked/isAtomic 分支旁加 isChan）。
- 实例方法 `share/send/recv/close`：落 `emitMethodCall`（:14781）里的 `emitSyncMethodCall`（:14651，
  接收者静态类型特化）+ `inferType`（:3532）镜像返回类型。`emitSyncMethodCall` 已具
  「剥掉 Ref/Mut 包装拿核心类型、按 `TK_LOCKED`/`TK_ATOMIC` 特化」的骨架，加 `TK_CHAN` 分支即可。
- `Ref[Chan[T]]` 在 spawn/par_each 能力检查中按只读计（Q20 / Q13；P14-4 已对 Ref[Locked]/Ref[Atomic] 放行）。

### 1.3 运行时现状：锁有先例，等待/通道为零基础
- core 用汇编实现（`CMakeLists.txt:79-99`，`core_win64.s` → `core_win64.o`，随程序链接）；
  没有 `.c` target，通道逻辑新增到 `core_win64.s`（§3.1 给出完整原语清单）。
- 自旋锁先例：`core_lock_acquire/release`（`core_win64.s:1958-1977`，`xchgl` + `pause`），
  Win64 ABI：rcx/rdx/r8/r9 传参、rax 返回、调用方留 32B shadow space。
- **无**等待原语、**无** Chan 运行时；本阶段新增 core 等待原语（Sleep 退避的 condition 循环，§3.1）。
- 三点登记法（P13/P14-4 沿用）：core 汇编 `.globl` + `core.rio` extern（`lib/core/core.rio:152-169` 之后）+ `coreReturnType`（:471）非 int 返回。

### 1.4 scope 退出与 join 顺序（auto-close 的插入点）
- `emitScope`（`rcc_codegen.cpp:10020-10244`）：收集 `s.spawn(...)` → 发射体 → 隐式 join（`core_thread_join`，:10233-10240）→ `popScope`（:10243）。
- **auto-close 必须在 join 之前**（Q8 裁决 + §4-D）：在 :10233 的 join 循环之前插入
  「对本 scope 内创建的 Chan 句柄逐个 `core_chan_close`」，唤醒阻塞在 recv 上的任务，否则 join 死锁。
- 登记方式：`emitScope` 检查阶段遇到 `var ch: Chan[T] = Chan.new[T](...)` 语句（或推断出 Chan.new 调用）时，
  把结果槽地址记入 scope 局部的 `chanSlots` 列表，与 `handleSlots` 并列。

### 1.5 生命周期 / drop（避免 Content 丢失与重复释放）
- Content 变量是资源拥有者，赋值即移动（`:6436-6443`），drop glue（:1140-1259）释放元素、把槽写 null（D17），
  **不递归释放元素**（P13-1 口径）。
- `send` 消耗 Content → 调用方槽变 X（`_moveStates`）；`recv` 在接收方出生地创建 Content 拥有者。
- Chan 对象本身（含环形缓冲）在 scope 末由 drop glue 释放；缓冲里**尚未 recv 的 Content** 属已转移所有权，
  需在 Chan drop 时逐个 core_free（§3.1 `core_chan_drop`），避免泄漏（这是 Chan 与 Locked/Atomic 不同的关键点）。

---

## 2. 语法与语义（全部已裁决，见 §4 / 记忆 `3ce842fd`）

```
class Msg { var id: int  ctor(n: int) { this.id = n } }
fun producer(ch: Ref[Chan[Msg]]): void {
    var m = Msg(7)
    ch.send(m.take())          // send 只收 Content[Msg]；调用方 m 变 X
}
fun consumer(ch: Ref[Chan[Msg]]): void {
    if var m = ch.recv() {     // recv() -> Content[Msg]?（关闭为 null）；窄化后 m: Content[Msg]
        sout(m.id)
    }
}
fun main(): void {
    var ch: Chan[Msg] = Chan.new[Msg](4)   // 显式容量，字面量常量
    var shared = ch.share()    // Ref[Chan[Msg]]
    scope s {
        s.spawn(producer(shared))
        s.spawn(consumer(shared))
    }                          // scope 末：先 close 所有 scope 内 Chan → join → drop
}
```

- `Chan[T]`：独立标准库类型（Q8=B），句柄是指针；类型参数 `T` 是**元素体类型**，
  通道内部承载 `Content[T]`。`send` 收 `Content[T]`、`recv` 返回 `Content[T]?`
  （`Content[T]` 是 `TK_MEMORY`/pointer-like，天然可空，故 `T` 本身不必是引用型）。
- `Chan.new[T](capacity)`：容量须字面量常量，首版有界；`Chan.new[T]()` 用默认容量 16（§4-A）。
- `share()`：`Chan[T]` / `Ref[Chan[T]]` → `Ref[Chan[T]]`，运行时同一指针（与 P14-4 `Locked.share()` 对称）。
- `send(c: Content[T])` → void：满则**阻塞**；`c` 必须是 `Content[T]`（调用方先 `x.take()`），发送后 `c` 槽为 X。
- `recv()` → `Content[T]?`：空则**阻塞**；关闭后返回 null；成功后接收方出生地得到新 Content 拥有者。
- `close()` → void：**幂等**；closed 后再 send → 运行时 abort（与 Q9 任务异常 abort 一致）；
  recv 已关闭且缓冲空时返回 null。
- scope 结束时对 **scope 内创建的 Chan** 自动 close（在 join 前），唤醒阻塞 recv 避免死锁（§4-D）。
- Q15 happens-before 点：`send` 完成 → `recv` 返回；`close` 唤醒所有等待者。
- Q20 内部可变例外清单写死三种（Locked/Atomic/Chan），`Ref[Chan]` 并行检查视只读。

---

## 3. 分层实施（子阶段）

### 3.1 子阶段 5-0：类型与标注基础
- `rcc_sema_types.h`：TypeKind 加 `TK_CHAN`；新增 `ChanType(inner)`（复用 `SyncWrapperType` 骨架，
  `toString()="Chan"`）；`isPointerLike()` 加 `TK_CHAN`。
- `rcc_codegen.cpp ::parseTypeName`（:77）：`Chan` → ChanType（仿 Locked/Atomic 分支）。
- `typeFromTypeArgNode`（:12019）、`typeFromLabelNodeNonNull`（:12219）：`Chan[...]` → ChanType(inner)；
  inner 必须是引用型（否则明确诊断「Chan[T] requires a nullable-capable T」）。
- `poolNewTypeArg`（:6610）/ `poolNewReceiverName`（:1276）：排除 `Chan` 接收者。
- 验收：`var ch: Chan[Content[Msg]] = ...` / `Chan.new[Content[Msg]](4)` 能 parse + 类型标注不报错。

### 3.2 子阶段 5-1：core 等待原语与 Chan 运行时
- `lib/core/core_win64.s` 新增（全部 `.globl`、Win64 ABI、32B shadow space 由调用方预留、只改 caller-saved 寄存器）：
  - `core_chan_new(capacity: int) -> ptr`：分配对象 `[+0 lockword][+8 cap][+16 head][+24 tail][+32 count]
    [+40 closed][+48 slot[cap]]`（slot 每 8B）；返回对象指针。
  - `core_chan_send(ch: ptr, value: ptr) -> void`：加锁；满或 closed → 解锁后 abort（closed）或
    **阻塞等待**（解锁 → `pause`/`Sleep(0)` 退避 → 重加锁，直到有空位或 closed）；写 slot[tail]，
    tail 前进，解锁（可唤醒 recv 等待者）。
  - `core_chan_recv(ch: ptr) -> ptr`：加锁；有元素 → 取 slot[head]、head 前进、解锁、返回值；
    空且 closed → 解锁返回 0（null）；空且开 → 解锁后阻塞退避重试。
  - `core_chan_close(ch: ptr) -> void`：加锁设 closed=1，解锁（幂等）。
  - `core_chan_drop(ch: ptr) -> void`：释放通道对象本身（环形缓冲内嵌在对象里）。
    **不逐个释放残留 Content**：实测 `Content[class]` 的运行时值就是对象指针，其 drop glue
    只把槽写 null、不递归释放元素（P13-1 口径）；通道运行时无元素类型信息，贸然 `core_free`
    会误释放非堆负载或造成 double free。未取走的 Content 登记为已知泄漏（与容器元素同口径）。
- `lib/core/core.rio`：对应 extern 声明（通道指针/Content 值按 ptr 处理，`recv` 返回 ptr）。
- `coreReturnType`（:471）：`core_chan_new` → ptr；`core_chan_recv` → ptr（映射到 Content?）；
  `core_chan_send/close/drop` → void。
- **clobber 防护**（P14-3/4 踩坑）：core 原语只改 caller-saved；codegen 调用前把 receiver/实参地址存专属栈槽。
- 验收：C 层探针 `cases/temp-test/p14_5_chan_probe.c` 单/双线程 send-recv FIFO、满阻塞、close 唤醒、closed send abort。

### 3.3 子阶段 5-2：codegen — Chan.new / send / recv / close / share + inferType
- `emitFunctionCall` DOT 静态分支加 `isChan`：
  - `Chan.new[T](capacity)` / `Chan.new[T]()`：容量字面量校验（非字面量报明确诊断），
    调 `core_chan_new(cap)`（无参用 16），返回指针标注 `Chan[T]`。
  - `inferType` 镜像：返回 `ChanType(inner)`。
- `emitSyncMethodCall`（:14651）加 `TK_CHAN` 分支（接收者剥 Ref/Mut 后判 `TK_CHAN`）：
  - `share()` → 返回同指针，类型 `Ref[Chan[T]]`。
  - `send(args[0])`：编译期检查 `args[0]` 静态类型必须与 inner 一致且为 `Content[T]`（否则明确诊断，
    §3.4）；`emitExpression` 求值 Content → 存槽；调 `core_chan_send(ch, value)`；void。
  - `recv()`：调 `core_chan_recv(ch)` → 值槽标注 `Content[T]?`（`NullableType(Content[T])`）。
  - `close()`：调 `core_chan_close(ch)`；void。
- `inferType` 镜像：share→Ref[Chan]、send/close→void、recv→Content[T]?。
- 验收：§2 全部正向写法可编译运行，单线程 send→recv 值正确、close 后 recv 返回 null。

### 3.4 子阶段 5-3：scope auto-close + 静态检查
- **auto-close**（`emitScope`，:10233 前）：scope 内创建的 Chan 登记到 `chanSlots`；join 循环前逐个
  `core_chan_close`；Q17 顺序：close → join → popScope/drop（drop 时 `core_chan_drop` 释放残留 Content）。
- **静态检查**（复用 `_moveStates` / 能力路径 / `checkAssignable`）：
  - `send` 实参非 `Content[T]` 或 inner 不符 → 编译期拒绝（「Chan.send requires Content[T]」）；
  - 对 `&ch`/`ch` 当作普通 Body 使用、或裸 `Chan` 作值传递（未 share）→ 编译期拒绝/诊断；
  - `Ref[Chan]` 传 spawn/par_each 按只读放行（Q20）；把 `Content` 作为 spawn 实参遵守既有能力规则；
  - close 后 send 属运行时 abort，编译期不静态追踪（与 Q9 一致）。
- 验收：负面用例（send 非 Content / send inner 不符 / 裸 Chan 跨任务）全部编译期拒绝；
  auto-close 用例（两任务一阻塞 recv，scope 正常退出不死锁）通过。

### 3.5 子阶段 5-4：测试套件 + 全量回归
- 新套件 `tests/agent/p14-5-chan`（沿用 p14-4 的 `manifest.json` / `execute.ps1` / `code/cases.json` 数据驱动格式）：
  - 正向：单线程 send→recv FIFO（多值有序）、spawn 生产者/消费者经 Chan 传 Content（Q12 结果带回）、
    share 跨任务、close 后 recv 返回 null、scope auto-close 唤醒阻塞 recv 不死锁、有界容量阻塞不丢数据。
  - 拒绝：send 非 Content、send inner 不符、裸 Chan 跨任务（未 share）。
  - 压力：多生产者/消费者经 Channel 传递 N 个 Content，计数恒定。
- 全量 `tests/agent` 回归无回归；探针归档 `cases/temp-test/p14_5_*`。

---

## 4. 设计空白裁决记录（2026-11-20，用户拍板，五项全闭合，全部选推荐项 A）

- **4-A 构造**：`Chan.new[T](capacity)` 显式有界容量（capacity 须字面量常量），
  `Chan.new[T]()` 无参用默认容量 16；类型 `Chan[T]`，句柄是指针。
- **4-B 负载**：严格 Content——send 只接受 `Content[T]`（调用方先 `x.take()`），
  recv 返回 `Content[T]?`（Content 是 pointer-like，`?` 标注天然合法，沿用 `if var` 窄化）。
- **4-C 阻塞**：满 send / 空 recv 均阻塞（运行时新增等待原语；Q10 已把阻塞取消延后）。
- **4-D close**：`ch.close()` 幂等；closed 后 send → 运行时 abort（与 Q9 一致）；
  recv 关闭后返回 null；**scope 退出时在所有 join 之前对 scope 内创建的 Chan 自动 close**（避免 join 死锁）。
- **4-E 共享**：`ch.share() -> Ref[Chan[T]]`，与 Locked.share() 对称，Ref[Chan] 可传 spawn/par_each、并行检查按只读计。

> 按硬规则 `1b1ccafb` 这些接口属标准库公开面／新类型，现已获批，可进入实施。调研见记忆 `1a5eb412`。

---

## 5. 验收标准

1. `Chan.new[T](capacity)` 构造有界通道，`send(c: Content[T])` / `recv()->Content[T]?` FIFO 正确；满/空阻塞不丢数据。
2. send→recv 跨任务传递 Content，接收方得到新拥有者；调用方 send 后槽变 X（移动语义）。
3. `close()` 幂等；closed 后 recv 返回 null、send 触发 abort；scope 末 auto-close 在 join 前，阻塞 recv 的任务能正常 join。
4. `share()->Ref[Chan[T]]` 可传 spawn/par_each，并行检查按只读计；裸 Chan 跨任务被拒。
5. Chan drop 释放缓冲内未取走的 Content（无泄漏，dbg 钩子可验证）。
6. 新套件全绿 + 全量 `tests/agent` 无回归。

---

## 6. 风险

1. **core 手写汇编的阻塞循环**：等待用「解锁 → `pause`/`Sleep(0)` → 重加锁」退避，避免持有锁 Sleep 死锁；
   Win64 ABI 只改 caller-saved，结果在后续索引计算前先存专属栈槽（P14-3/4 clobber 踩坑）。
2. **scope auto-close 时机**：必须在 join 前；否则空 recv 阻塞 → join 永不返回。登记 `chanSlots` 要区分
   「scope 内创建」与「外层传入」（后者 scope 不负责关闭，遵循「谁创建谁关闭」）。
3. **Content 双重释放**：send 消耗后调用方槽必须变 X；recv 创建的新 Content 与缓冲残留 Content 由不同路径释放，
   Chan drop 只释放仍留在缓冲里的。用 D17 dbg 钩子（double_frees==0）验证。
4. **T 的角色**：类型参数 `T` 是元素体类型，recv 返回的是 `Content[T]?`；`Content[T]` 恒为 pointer-like，
   故 `Chan[int]` 合法（承载 Content[int] 需额外处理标量 Content 表示，首版若实测不支持则明确诊断）；
   计划示例一律用 class/struct 等有 Body 语义的元素类型，贴合 take/fill 既有用例。
5. **类型标注与 pool.new 误判**：`Chan.new[T]` 必须在 `poolNewTypeArg`/`poolNewReceiverName` 三处判断里排除，
   否则被当成 Pool 构造（P14-4 Atomic.new[i32] 同类踩坑）。
6. **构建工具链路径**：已迁移至 `D:\soft\Clion\bin\mingw`（记忆 `44feb632`），测试 PATH 需含
   `bin` 与 `x86_64-w64-mingw32\bin`；旧路径已失效。

---

## 7. 完成记录（2026-11-20）

### 7.1 交付

| 子阶段 | 交付 | 文件 |
|---|---|---|
| 5-0 | `TK_CHAN` + `getChanType` + isPointerLike/toString + `Chan[T]` 两处标注解析 + parseTypeName 裸名兜底 + pool.new 三处排除 Chan | rcc_sema_types.h、rcc_codegen.cpp |
| 5-1 | core 通道原语 `core_chan_new/send/recv/close/drop`（48B 头 + cap*8 环形缓冲、自旋锁、SwitchToThread 退避阻塞、close 幂等、closed send→abort）+ 三点登记 | lib/core/core_win64.s、core.rio、rcc_codegen.cpp(coreReturnType) |
| 5-2 | `Chan.new[T](capacity)`（容量整数字面量、默认16）/ `share`/`send`/`recv`/`close` + inferType 镜像（recv→`Content[T]?`、send 静态检查 Content[T]）+ drop glue（TK_CHAN→core_chan_drop） | rcc_codegen.cpp |
| 5-3 | scope 退出 auto-close：在 join 前对 scope 帧内 TK_CHAN 拥有者变量发 `core_chan_close`，唤醒阻塞 recv 避免 join 死锁 | rcc_codegen.cpp(emitScope) |
| 5-4 | 新套件 `tests/agent/p14-5-chan` 6 用例 + 全量回归 + core C 探针归档 | tests/agent/p14-5-chan/、cases/temp-test/p14_5_* |

### 7.2 关键实现决定

- **载荷不透明 8B 值**：实测 `Content[class]` 运行时就是对象指针、drop glue 只把槽写 null 不递归释放（P13-1 口径），通道运行时无元素类型信息，故 `core_chan_drop` 只释放通道对象，**不逐个 core_free 残留载荷**（否则误释放非堆负载/double free）。残留 Content 登记为已知泄漏，与容器元素同口径。
- **阻塞用解锁+退避**：满 send / 空 recv 在释放自旋锁后调 `SwitchToThread()` 再重试，绝不持锁等待；close 只置位 closed，阻塞的 recv 下一轮循环自然看到并返回 null。首版无取消机制（Q10）。
- **auto-close 只关拥有者**：只关闭 scope 帧内以 TK_CHAN 类型登记的局部变量（真正 `Chan.new` 出来的），`share()` 得到的 `Ref[Chan]` 是 TK_MEMORY 不在列，遵循「谁创建谁关闭」；且必须在 join 前，否则任务阻塞在 recv 上死锁。
- **send 消耗 Content 的静态口径**：复用 `BodyState::MOVED`——`ch.send(m.take())` 后 `m` 槽变 X，后续访问按既有空壳检查拒绝；非标识符实参（如直接 `m.take()`）由表达式本身的 take 语义处理。

### 7.3 验证

- 新套件 `tests/agent/p14-5-chan` **6/6**（正向 3：单线程 FIFO+close→null、scope 内 Chan 跨 2 spawn 传 10 个 Content 无死锁、默认容量 16 交替 send/recv 越过回绕点和=190；拒绝 3：send 非 Content、容量非字面量、未知方法）。
- **全量 `tests/agent` 37 套件 1012 项全通过 0 失败**（原基线 36 套件 1006 项 + 本套件 6）。
- core C 探针 `cases/temp-test/p14_5_chan_probe.c` ALL OK（2 生产者各 100 + 主线程收 200 恒定；close→recv null；幂等 close）；`p14_5_chan_min4.c` 200 次 send/recv 回环无误。
- 汇编栈对齐经实测修正：2 次 push 后必须 `sub $40`（≡8 mod16）才能在 call 前保持 rsp≡0，recv 曾因 `sub $32` 偶发 0xC0000005；跨 core 调用的索引/返回值改用 callee-saved `%r12/%r13`（`core_lock_*` 的 xchg 会改 %rax）。

### 7.4 已知限制 / 后续

- **阻塞退避非 futex**：等待用 `SwitchToThread()` 轮询而非内核等待原语，高负载下有 CPU 开销；Q10 已把阻塞取消/优化列为后续，首版满足语义正确。
- **残留 Content 泄漏**：通道 close 时缓冲内尚未 recv 的 Content 不回收（无元素类型信息），与容器元素泄漏同口径，登记为已知限制。
- **容量须字面量**：`Chan.new[T](capacity)` 只接受整数字面量（裁决 4-A），变量容量编译期拒绝。
- **send/recv 需显式 take/if-var**：负载严格 Content（裁决 4-B），调用方须先 `x.take()`，recv 须用 `if var m = ch.recv(){}` 窄化；这是严格语义要求，非缺陷。
- 后续阶段：P14-6 验收（粒子系统两阶段示例 + §10.9 竞争排除论证逐条对应 + 回归套件）。

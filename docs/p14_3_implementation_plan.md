# P14-3 实施方案：`par_each` 数据并行原语

> **本文性质**：实施方案（2026-11-19）。依据已裁决的 P14 全部设计（`docs/p14_plan.md` §7，Q1–Q20）、
> 规范 §10.4/§10.8（`docs/memory-model-spec-full.md`），以及 codegraph 对当前代码库的实测。
> 遵守硬规则 `1b1ccafb`：本文用到的设计全部已获批准；三个设计空白已于 2026-11-19 由用户裁决闭合（§4）。
> 前置：P14-1a（运行时地基）、P14-1b（scope/spawn）均已完成并全量回归通过。

---

## 0. 阶段定位与下一阶段确认

- P14-2（效果集并行组合检查，Q4）的**核心已随 P14-1b 的 1b-3 落地**：spawn 间能力冲突检查、
  借出变量冻结、跨层拒绝、循环内拒绝（`rcc_codegen.cpp emitScope` 检查阶段；记忆 `673e8315`）。
  本方案不再重复实现 P14-2。
- 两个前序方案的"下一步"均明确指向 **P14-3 par_each**
  （`p14_1b_implementation_plan.md §7.4`、`p14_1a_implementation_plan.md §9`）。
- 阶段路线（`p14_plan.md §2`）：P14-3 → P14-4 Locked/Atomic → P14-5 Chan → P14-6 验收。

---

## 1. 关键事实（codegraph 实测，2026-11-19，逐条带出处）

### 1.1 Rio 闭包是真实存在的一等公民（无需新造闭包语法）
- 匿名函数语法 `(params) -> { body }`，AST `AnonFunctionDefinitionNode`，
  parser 在 `rcc_prefix_builder.cpp:510-543`（`(` 后 `:` 读类型标签 → `->` indicator → body）
  与 `rcc_infix_builder.cpp:65-69`（简写 `-> expr` = `ret expr`）。
- 闭包值 = 16 字节闭包对象指针 `{ +0 fnptr, +8 env }`。
  - 具名函数作值：`closureForFunction`（`rcc_codegen.cpp:9883-9895`）生成静态 `.Lfnclosure_<f> {fn,0}`；
  - 无捕获匿名：`emitAnonymousFunction :9552-9557` 同走静态对象；
  - 带捕获匿名：`:9559-9594` 运行时 `core_alloc(16+8n)`，env = block+16，逐项写捕获变量**地址**
    （2026-11-18 裁决 A：按引用捕获）。
- 闭包调用走 `emitIndirectCall :9261-9323`：闭包指针暂存槽 → env→`%r10`(+8)、fnptr→`%r11`(+0)、
  `call *%r11`（:9318-9321）；callee 序言从 `i*8(%r10)` 装入 boxed 本地槽（:15884-15901）。
- 闭包按引用捕获时 **Take 形参禁止被捕获**（`:9524-9535`，P13-3/§8.5 rule 2）。

### 1.2 线程入口 ABI 与现有 trampoline（par_each 不能直接复用）
- `core_thread_create`（`lib/core/core_win64.s:1836-1859`）：`movq (%r12),%r12`（:1845）**只解包一次取 fnptr**，
  参数只有 arg→`%rcx`，**完全不设置 `%r10`(env)**。具名闭包 env=0 无碍；带捕获闭包进线程后
  按引用捕获读 `i*8(%r10)` 拿到垃圾 → 这就是 1a-5「带捕获闭包不作线程入口」的根因。
- P14-1b 的 trampoline：`_pendingTrampolines`（`rcc_codegen.h:1027-1033`，{label,targetFn,argCount}），
  `emitPendingTrampolines`（`rcc_codegen.cpp:9614-9634`）：`rcx`=argblock 指针，逐槽 0/8/16/24(%rax)
  →`rcx/rdx/r8/r9`，`sub rsp 40; call targetFn`——**对符号直接 call，不装载 env，最多 4 参**。
- 结论：par_each 闭包带 env，需新增「从 argblock 取闭包指针 + 装 `%r10` + `call *%r11`」的
  **带 env 间接调用 trampoline**（实现细节，非新设计）。

### 1.3 Pool 运行时：无元素类型、无枚举原语
- `PoolType final : Type`（`rcc_sema_types.h:371-385`）**不携带元素类型**，无 `getElemType()`；
  元素类型只能在 `pool.new[T]` 处取（`poolNewTypeArg()` `rcc_codegen.cpp:6504-6527`）。
- `lib/core/core.rio:145-147` 仅三个 extern：`core_pool_new/alloc/release`。
- 池头 48B（`core_win64.s:1211-1354`）：偏移 0=mode(1 fixed/2 boxed/3 chunked)、8=cap、
  16=elem_size(惰性)、24=used_in_chunk(游标)、32=current_chunk、40=**total**。
  chunk=`[prev_ptr][slots...]`，槽地址 = `current_chunk+8+used*elem_size`，永不搬家。
- **没有 `core_pool_len/get/at/iter`**：total 有值但无原语暴露；无成员位图（p13_6 方案的位图未实现）。
- 现有迭代：无 `for x in pool`（`emitLoop :7355-7382` 仅 C 风格）；Link 遍历靠全局边表
  （`core_link_*`）；`get()` 特化仅接 LIST/DICT（`:13886-13905`），Pool 会报错。
  → par_each 必须新增枚举原语（建议 `core_pool_count` + `core_pool_slot_at`）。

### 1.4 方法特化派发落点
- `emitFunctionCall :9982` DOT 分支：模块名(:10032) → `Pool.fixed/boxed/chunked`(:10044-10076)
  → `pool.new[T]`(:10077-10122) → 兜底 `emitMethodCall`(:10123)。
- `emitMethodCall :13798`：clone(:13836-13874)、get(:13886-13905)、release(:14049-14139)、Link(:14140+)。
- par_each 落点：`emitFunctionCall` DOT 分支 :10122 之前；`inferType :3664` 镜像返回类型；
  core 原语按三点登记法（core_win64.s `.globl` + core.rio `extern` + `coreReturnType :469`）。

### 1.5 已批准的 par_each 语义（来自规范 §10.4，p14_plan §0 列为可复用）
- 闭包体只访问 `p` 及子路径（写），以及显式捕获的 `Ref`（只读；只读数据不能是同类型 T 的其他槽位，
  除非整个 par_each 内无任何写）；
- 不对其他 slot 用 `take`，不调 Link 原语，不改 pool 结构；
- 每槽恰好一次、元素互不相交 → 无需逐对证明；
- Q18=A：自带 join（阻塞返回）、成员集合**快照**语义、禁 Link 遍历；
- Q5=C：首版不含 par_zip，用 par_each + 只读捕获另一类型 Pool + 槽位 id 查找（接口见 §4-B）；
- Q19：`Take<Pool>` 是笔误 → `Mut<Pool>`，不改结构、只写元素。

### 1.6 已裁决但属"实现空白"的能力
- Q13（`:163`）：任务含调用链不得访问可变全局、不得调 extern，需跨函数触碰全局/extern 汇总分析。
  P14-1b 明确排除（`rcc_codegen.cpp:3267`「做不到跨函数推断，退化为不检查」、`:9863`）。
  **用户裁决（§4-C）**：首版只检查闭包体直接触碰，跨函数部分在文档如实标注"未覆盖"，后续补。
- Q1（`:151`）：par_each 内部固定 worker 池，但**「池可后加不改语义」** →
  首版用语义等价的串行 / 1:1 线程实现，worker 池后加（实现取舍，非新设计）。
- 成员集合机制（重数格 + 快照）已有基础设施：`rcc_codegen.h:521-529`、`.cpp:1965-2042`；
  快照落地（`copy` 原语）在 par_each 入口调用。

---

## 2. Rio 语法映射（已裁决，见 §4-A）

规范示例是 Rust 伪代码；映射为 Rio 真实闭包语法（复用现有 `(params) -> { body }`，不新造关键字）：

```
// 对池每个元素并行写（ Mut 能力）
pl.par_each((p: Mut[Player]) -> {
    p.hp = p.hp + 1
})
// 只读捕获另一类型 Pool + 槽位 id 查找（Q5=C 的 par_zip 替代写法，接口见 §4-B）
pl.par_each((p: Ref[Particle]) -> {
    var f = forces.at(p.id)     // at：§4-B 裁决的查找接口
    p.vel = p.vel + f.value * cfg.dt
})
```

- 元素能力由闭包**首参的类型标注**给出：`Ref[T]`=只读、`Mut[T]`=写、`Take[T]`=拒绝（§1.5）。
- 元素类型 `T` 从闭包首参类型标注解析（`typeFromTypeArgNode :11202`，已支持 `Ref/Mut/Take` 前缀 :11239-11243）。

> **实现注意**：anon 函数 param 的类型标注当前 parser 只收 `TOKEN_LABEL`/`TOKEN_IDENTIFIER`
> （`rcc_prefix_builder.cpp:514-519`），`(p: Mut[Player])` 中的 `[...]` 可能解析失败 →
> 需扩展该路径以接受 bracketed type args（实现细节，验证后确定）。

---

## 3. 分层实施（子阶段）

| 子阶段 | 内容 | 验收 |
|---|---|---|
| 3-0 | **Pool 枚举原语**：`core_pool_count(pool)` / `core_pool_slot_at(pool,index)`；沿 chunk 链 + elem_size + total 逐槽取址；三点登记 | C 层探针：N 槽遍历计数正确、槽地址稳定 |
| 3-1 | **闭包元素类型解析 + par_each 派发骨架**：闭包首参标注→元素角色/类型；`emitFunctionCall` 加 `pool.par_each` 分支；`inferType` 镜像返回 void | `pl.par_each((p: Mut[T])->{})` 能 parse+codegen（先空体） |
| 3-2 | **带 env 间接调用 trampoline**：argblock 传闭包对象指针 → 装 `%r10` → `call *%r11`；参数=槽指针(+捕获 env 随闭包) | 单线程顺序调用闭包 N 次结果正确 |
| 3-3 | **静态检查器**：§1.5 闭包体验证（首参角色、禁 take/Link/结构修改、捕获只读 Ref 非同类型 T）；复用能力路径机制；成员集合快照 | 负面用例编译期拒绝 |
| 3-4 | **并行执行 + 隐式 join**：首版串行/1:1（Q1 允许池后加）；join 后实例级事实刷新（首版保守）；`pool.at` 接口落地 | 正向用例结果正确；压力测试 |
| 3-5 | **回归套件 + 全量回归**：`tests/agent/p14-3-par-each` 正向×N + 拒绝×N；全量 tests/agent | 新套件全绿 + 既有 726 项无回归 |

> 每完成一个子阶段汇报进度后继续（沿用 1a/1b 自主推进方式）。

---

## 4. 设计空白裁决记录（2026-11-19，用户拍板，全部闭合）

### 4-A. par_each 语法
**裁决**：复用 Rio 现有闭包语法 —— `pl.par_each((p: Mut[T]) -> { ... })`；不新增关键字、
不支持专用语句形态。（备选"专用关键字"、"任意函数值实参"未采纳。）

### 4-B. 槽位 id 查 Pool 接口
**裁决**：新增公开方法 `pool.at(id) -> Ref[T]?`；`id` 为槽位稳定 id（池不搬家故稳定），
找不到返回 null。可空性按 Rio 现有 `null` 语义落地（`?` 为规范表述，Rio 未实现 `T?` 语法）。
（备选"扩展 pool.get()"未采纳。）

### 4-C. Q13 分析范围
**裁决**：首版只检查闭包体**直接**触碰全局/extern；跨函数（调用链）部分**不实现**，
并在文档/测试如实标注"Q13 跨函数部分未覆盖"，后续阶段补。不阻塞 P14-3 开工。

---

## 5. 验收标准

1. 正向：`pl.par_each((p: Mut[T])->{...})` 编译运行，每槽恰好一次，结果正确；
2. 拒绝：闭包首参非 Ref/Mut、对他人 take、调 Link、改 pool 结构 → 编译期报错；
3. 元素互不相交无需逐对证明；捕获只读 Ref 为同类型 T 的其他槽位且有写 → 拒绝；
4. 快照语义：迭代中 add 的新槽本轮不被访问；
5. Q5 替代：能只读捕获另一类型 Pool 并用 `pool.at(id)` 查找（§4-B）；
6. 回归套件全通过，全量 tests/agent 无回归。

---

## 6. 风险

1. **带 env 闭包的线程入口 ABI**：`%r10` 需在 trampoline 中正确装载，且捕获变量的堆装箱
   （`boxCapturedVar`）与任务生命周期要对齐，否则悬垂 —— 实现期重点验证。
2. **Q13 跨函数部分未覆盖**：首版只查闭包体直接触碰，文档中如实标注，不得声称满足 Q13 全量。
3. **anon 闭包 bracketed 类型标注解析**：现有 parser 路径可能不支持 `(p: Mut[T])`，需实测。
4. **元素类型来源**：PoolType 不携带元素类型，首版依赖闭包首参标注，类型不一致时需明确诊断。
5. **`core_pool_slot_at` 的越界语义**：index 越界应返回 0/null 而非越界读 —— 实现期定明确行为。

---

## 7. 完成记录（2026-11-19）

### 7.1 交付

| 子阶段 | 交付 | 文件 |
|---|---|---|
| 3-0 | Pool 枚举原语 `core_pool_count` / `core_pool_slot_at`（沿 chunk 链 + elem_size + total；越界返回 0）+ extern + 三点登记 | lib/core/core_win64.s、core.rio、rcc_codegen.cpp |
| 3-1 | 闭包元素类型解析 `anonParamTypes`（走完整 label 路径，支持 `Mut[T]`/`Ref[T]`）+ `emitFunctionCall` 的 `pool.par_each` / `pool.at` 分支 + `inferType` 镜像 | rcc_codegen.cpp/.h |
| 3-2 | 带 env 间接调用 trampoline：`PendingTrampoline::viaClosure`，`pareach_tramp_*` 从 argblock 取 `{closure, slot}` → env(%r10)/fnptr(%r11) → `call *%r11` | rcc_codegen.cpp/.h |
| 3-3 | 静态检查器：Take 元素拒绝、结构修改/Link/他人 take 拒绝、**捕获变量写拒绝**（§10.4 只读捕获）；`scanBody` 递归扫描闭包体 | rcc_codegen.cpp emitParEach |
| 3-4 | **并行执行**：每元素一个 OS 线程（Q1 的 1:1），堆上 argblock/handle 数组，块末隐式 join（Q18=A），join 后释放；`pool.at(id)` 落地 | rcc_codegen.cpp emitParEach |
| 3-5 | 回归套件 `tests/agent/p14-3-par-each` 7 用例 + 全量回归 | tests/agent/p14-3-par-each/ |

### 7.2 关键实现决定

- **真正并行（非串行占位）**：虽 Q1 允许「池可后加不改语义」，本阶段直接实现 1:1 OS 线程并行。
  复用 1a 的 `core_thread_create`（解包闭包 fnptr）+ 1b 的 `_pendingTrampolines` 基础设施，
  新增 `viaClosure` 形态装载 env 后间接调用。argblock/handle 数组因元素数运行时可知而用堆分配
  （`core_alloc`），join 后 `core_free`（Q17 顺序）。
- **捕获写必须拒绝**：并行执行下，闭包写捕获变量是真数据竞争。`scanBody` 用 `collectFreeVariables`
  求捕获集合，写目标根若在捕获集合中（且非元素参数）即编译期拒绝。
- **clobber 修复**：`core_pool_slot_at` / `core_thread_create` 的 `%rax` 结果在索引计算前先存入
  专属栈槽，否则被 `imulq`/`add` 覆盖导致崩溃（实测 0xC0000005，已修）。
- **Q13 跨函数部分未覆盖**（按 §4-C 裁决）：只查闭包体直接触碰，未做跨函数调用链的
  全局/extern 汇总分析 —— 后续阶段补，本阶段不声称满足 Q13 全量。
- **成员集合快照**：Par_each 首版假定迭代期间不改池结构（静态检查器已禁结构修改），
  故 count 在循环开始时取一次即可等价于快照；迭代中 add 不可达（闭包体内禁 add）。

### 7.3 验证

- 新套件 `tests/agent/p14-3-par-each` 7/7（正向 4：mut 写/捕获 Ref+pool.at/跨块 200 元素压力/只读；
  负面 3：捕获写/Take 元素/结构修改）。
- 200 元素跨块并行压力：5 次运行 sum 恒为 200，无竞态。
- 全量 `tests/agent` 35 套件 997 项全通过 0 失败（原 990 + 新增 7）。
- 探针归档 `docs/p14_3_probes/`。

### 7.4 已知限制 / 后续

- **Q13 跨函数分析未实现**（§4-C 裁决，登记后续）。
- **固定 worker 池未实现**：当前每元素一个 OS 线程（Q1 允许的 1:1 形态）；元素很多时线程数线性增长，
  固定 worker 池是后续优化（Q1「池可后加不改语义」）。
- **捕获同类型 T 的另一槽位只读检查**未细化（§1.5 规则）：首版只做「捕获写拒绝」，
  未区分「只读捕获同类型 T 的其他槽位且有写」的情形，登记后续。
- 后续阶段：P14-4 Locked/Atomic、P14-5 Chan、P14-6 验收（粒子系统）。

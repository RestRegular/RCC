# P14-1b 实施方案：`scope` / `spawn` 结构化并发（语言层首版）

> **本文性质**：实施方案（2026-11-19），依据已裁决的 P14 全部设计（`docs/p14_plan.md` §7，Q1–Q20）
> 与新增 B1–B5 裁决（记忆 `eadfc100`），以及 codegraph 对当前代码库的实测。
> 遵守硬规则 `1b1ccafb`：本文实现的设计全部已获批准；**出现的每个语法/类型/原语都在裁决清单内**，
> 不自行纳入新设计。运行时地基（1a）已完成，见 `docs/p14_1a_implementation_plan.md`。
> **实施状态（2026-11-19）**：五个子阶段全部完成并验证——新套件 7/7、全量回归 726 项全绿。
> Q4=A 的父线程冻结、循环内 spawn 明确拒绝均已补齐（见 §7 完成记录）。

---

## 0. 范围与不做什么

**做**（语言层首版，Q1–Q20 + B1–B5 已批准范围内）：
1. 新增两个关键字 `scope` / `spawn`；
2. `scope s { ... }` 语句：块绑定一个 scope 句柄 `s`（二等值），块末隐式 join；
3. `s.spawn(f(args))` 特化方法调用：`f` 限具名顶层函数（Q3），实参遵循普通调用规则（含 `.take()`，B2）；
4. 实参能力检查（Q4=A / B3=A）：由实参能力推导效果，同一 slot 被两方以冲突能力借用即编译期报错；不做跨函数效果推断；
5. 嵌套规则（B4）：`s` 只代表本 scope，不能跨层；内层任务只能挂内 scope；
6. 任务内异常 → 进程级 abort（Q9 / B5，复用 `core_proc_abort`）；
7. join 先于 scope 体内局部 drop（Q17）；
8. 回归套件 `tests/agent/p14-1b-scope-spawn`：正向 + 冲突拒绝负面用例。

**不做**（登记后续，不属本阶段）：
- par_each / par_zip（P14-3）、Locked/Atomic（P14-4）、Chan（P14-5）；
- 跨函数效果推断（Q4=B，已排除）、detached 任务、阻塞取消、异步协程；
- 带捕获闭包作线程入口（B2 之外；1a-5 已明确首版只具名顶层函数）；
- 运行时全局状态改造（1a 已完成）。

---

## 1. 关键事实（codegraph 实测，2026-11-19）

- **词法层无 scope/spawn**：`TokenType` 枚举（`code/include/rcc_core.h:64-153`）无 `TOKEN_SCOPE`/`TOKEN_SPAWN`；`tokenMap`（`rcc_core.cpp:30-106`）无映射；`getTokenTypeName`（:248）无 case。需三处同步新增。
- **parser 分发**：`Parser::buildExpression`（`rcc_parser.cpp:333`）经 `prefixExpressionBuilders` 按 token 类型分发；`buildStatement` 只是 `buildExpressionStatement` 的薄壳（:456-465）。带块语句（try/loop）都靠 prefix builder + `BlockRangerNode`。
- **AST 模板**：`TryNode`（`rcc_ast_components.h:696-714`）持 `tryBody/catchBodies/finallyBody`（均 `BlockRangerNode`），可作 `ScopeNode` 模板（`ScopeNode{ name(IdentifierNode), body(BlockRangerNode), spawnCalls }`）。
- **codegen 模板**：`emitTry`（`rcc_codegen.cpp:7641`）/ `emitLoop`（:7332）用 `pushScope/popScope` + 标签 + `emitEhUnwindTo`；`popScope`（:3398）按逆序发 drop（Q17 要 join 先于此）。
- **方法调用特化**：`emitFunctionCall`（:9712）对 DOT 成员调用按接收者类型做特化分派（见 `Pool.fixed`、`clone`、`get` 等分支）；`emitMethodCall`（:13528）处理类方法。`s.spawn(...)` 走 DOT 路径，按「接收者是 scope 句柄」特化。
- **函数值表示**：具名函数值 emit 成闭包对象 `.Lfnclosure_f: .quad f`；1a-5 已让 `core_thread_create` 解包一次取真实入口，`s.spawn(f(...))` 直接复用该机制。
- **取参规则**：`collectArgs`（:9724 附近）把实参列表解析成 `vector<ExpressionNodePtr>`；命名参数在 AST 里是 `Infix(op=COLON, name, value)`。
- **能力语义**：`take` 在 Rio 里是 `.take()` 方法（`pl.take()`，P13-3）；`Ref/Mut/Take` 是 sema 类型（`rcc_sema_types.h`）。效果检查复用既有实例级路径/不等事实机制（P13-4/5）。

---

## 2. 语法设计（全部已批准：B1=A / B2=A / Q2 / Q3）

```
scope s {
    s.spawn(work(a.take(), cfg))   // B1: s.spawn(f(args))；B2: take 用现有 .take()
    s.spawn(work(b.take(), cfg))
    log(cfg.name)                  // 父线程后续代码也是并行的一方
}                                  // 块末隐式 join（Q17：join 先于局部 drop）
```

- `scope s { ... }`：`s` 是本 scope 句柄变量名（标识符），块是 `BlockRangerNode`。
- `s.spawn(f(args))`：`s` 必须是当前最内层 scope 句柄（B4，不跨层）；`f` 必须是具名顶层函数标识符（Q3，非闭包/非方法）；`args` 是普通实参列表，`.take()` 按 P13-3 语义。

---

## 3. 分层实施

### 3.1 词法与 AST（1b-1）
- `code/include/rcc_core.h`：TokenType 加 `TOKEN_SCOPE`、`TOKEN_SPAWN`（放在 TOKEN_STRUCT 附近）。
- `code/src/rcc_core.cpp`：`tokenMap` 加 `{"scope",...}`、`{"spawn",...}`；`getTokenTypeName` 加两 case。
- `code/include/analyzer/rcc_ast_components.h` + `.cpp`：新增 `ScopeNode final : ExpressionNode`：
  - 字段：`std::shared_ptr<IdentifierNode> _name`（scope 句柄名）、`std::shared_ptr<BlockRangerNode> _body`；
  - `NodeType::SCOPE`（在 NodeType 枚举加值）；`getRealType()` 返回 SCOPE；getter。
- AST dump / symbol export 按需补 SCOPE 分支（避免遍历时漏节点）。

### 3.2 parser（1b-2）
- `buildScopeStatement`（新 prefix builder）绑定 `TOKEN_SCOPE`：
  - `scope` 后读标识符（句柄名）→ 期望 `TOKEN_LBRACE` → `buildRangerExpression` 建 body → 构造 `ScopeNode`。
  - 句柄名重复/缺失、块不匹配 → `failAt` 明确诊断。
- `spawn` 不单独做 prefix builder——它是 `s.spawn(...)` 的 DOT 成员调用，走 codegen 特化（3.3）。parser 无需识别 `TOKEN_SPAWN` 为表达式起点；但关键字需登记（避免被当普通标识符）。

### 3.3 sema/检查器（1b-3，Q4=A / B3 / B4）
在 codegen 发射期做（本项目是单遍 codegen，无独立 sema pass）：
- **scope 栈**：新增 `_scopeStack`（vector<ScopeFrame>），记录每个活动 scope 句柄名、其槽位、以及各任务借入 slot 的「能力登记表」。
- **spawn 检查**（Q4=A）：对 `s.spawn(f(args))`：
  1. `s` 必须是 `_scopeStack` 最内层句柄（B4），否则报错；
  2. `f` 必须是具名顶层函数（查函数表，非闭包/方法），否则报错（Q3）；
  3. 逐实参按能力推导效果：`Ref`→只读、`Mut`/`.take()`（Take）→写；记录到当前 scope 的登记表；
  4. 冲突判定：新任务的写路径与「已登记的其他方（父线程 + 先前任务）」的读/写路径重叠 → 编译期报错（复用实例级路径 + 不等事实；判断不了退回类型级拒绝）。父线程从 spawn 到 join 的后续代码作为一方参与（Q4 明确）。
- **借出变量冻结**（Q4=A 的「借用延伸」）：被任务借走（尤其 Take/Mut）的 slot 在 scope 内对父线程标记冻结，父线程在 join 前读写冲突路径报错。
- **循环内 spawn**：按 Q4 裁决，循环体内带 Mut/Take 的 spawn 首版直接拒绝（暂不证明跨迭代不相交）。

### 3.4 codegen（1b-4，Q17 / Q9 / Q1a）
- **emitScope**：
  1. 为句柄 `s` 分配一个栈槽（存一个 scope id，运行时可仅占位；join 用 core_thread_join 的句柄列表）；
  2. 收集块内所有 `s.spawn(...)` 调用点，按源码顺序发射；每个 spawn：
     - 在**父线程**求值实参（含 `.take()` 的消耗动作），把每个参数存进临时槽（跨调用存活）；
     - 生成任务 trampoline：一个内部函数，接收打包的参数、按普通调用约定调用 `f`；
     - `call core_thread_create(trampoline, argblock)`，返回的 HANDLE 存入本 scope 的句柄数组（栈上）。
  3. 发射 scope 块其余语句（父线程代码）。
  4. **隐式 join**（Q17）：块末、在 `popScope` 发 drop **之前**，对本 scope 每个 HANDLE 调 `core_thread_join`；join 后按 Q10.2 把任务触及 slot 的事实回到 `?`。
- **任务 trampoline 与异常**：任务是 OS 线程入口；任务体若 throw → 现有 `core_eh_throw` 在本线程 EH 链（1a-3 TLS）无 handler → 走 `.Leh_uncaught` → `core_proc_abort`（B5，进程级 abort）。无需额外代码，天然满足。
- **join 句柄存储**：栈上数组，容量 = 块内 spawn 点数（编译期已知）；超出/动态数量首版不支持（配合「循环内 spawn 拒绝」）。

### 3.5 寄存器与 ABI 注意
- 实参求值后必须落栈槽（跨 `core_thread_create` 调用存活，caller-saved 会被破坏）——沿用 emitCall 的 `emitArgsToRegisters` + 临时槽策略。
- `core_thread_create` 已解包闭包对象取真实入口；trampoline 作为函数值仍需 emit 成 `.Lfnclosure_*` 形态（或直接传裸符号 + 调整 1a 的解包策略——二选一，实施时按现有闭包发射惯例选最小改动）。
- 所有 `subRsp/addRsp` 保持 16 字节对齐、32 字节 shadow（ABI 铁律）。

---

## 4. 子阶段顺序

| 子阶段 | 内容 | 验收 |
|---|---|---|
| 1b-1 | 词法 + AST（关键字、ScopeNode） | 能 parse 出 ScopeNode，dump 正常 |
| 1b-2 | parser（buildScopeStatement） | `scope s {...}` 语法树正确 |
| 1b-3 | 检查器（scope 栈、能力冲突、冻结、B4、循环拒绝） | 负面用例编译期拒绝 |
| 1b-4 | codegen（emitScope、spawn、trampoline、join、Q17 顺序） | 正向用例跑通 |
| 1b-5 | 回归套件 + 全量回归 | 新套件通过 + 既有全绿 |

每完成一个子阶段汇报进度后继续（沿用 1a 的自主推进方式）。

---

## 5. 验收标准

1. 正向：`scope s { s.spawn(f(a.take(), cfg)); ... }` 编译运行，父线程后续代码与任务并发，块末 join 后结果正确；
2. 冲突拒绝：同一 slot 两方写、或一方写另一方读 → 编译期报错（负面用例）；
3. 嵌套：内层 scope 句柄不能 spawn 到外层（B4 拒绝）；
4. 循环内 Mut/Take spawn → 编译期拒绝（Q4）；
5. join 顺序：scope 内局部变量的 drop 发生在 join 之后（Q17）；
6. 回归套件 `tests/agent/p14-1b-scope-spawn` 全通过，全量 tests/agent 无回归。

---

## 6. 风险与待确认（实施中若触发，回来找用户，不自行决定）

1. **trampoline 参数打包的 ABI**：多参数/不同类型参数如何打包进 argblock——若现有 `emitArgsToRegisters` 无法直接复用，可能需要新的打包约定（属实现细节，但若涉及公开语义回来确认）。
2. **「父线程冻结」与既有借用检查的交互**：Q4=A 的借出冻结如何与 P13 的活跃子引用/状态格叠加——若出现规范未覆盖的冲突场景，列出来裁决。
3. **spawn 句柄数组的容量**：编译期固定；若发现需要动态数量（如条件 spawn），首版报错并登记后续。

---

## 7. 完成记录（2026-11-19）

### 7.1 交付

| 子阶段 | 交付 | 文件 |
|---|---|---|
| 1b-1 | 关键字 `scope`/`spawn`（TokenType/tokenMap/getTokenTypeName）+ NodeType::SCOPE + ScopeNode AST + dump | rcc_core.h/.cpp、rcc_ast.h/.cpp、rcc_ast_components.h/.cpp、rcc_ast_dump.cpp |
| 1b-2 | parser：`buildScopeExpression`（scope s {...}）+ `buildSpawnExpression`（spawn 作方法名标识符）+ 两处 map 注册 | rcc_parser.h/.cpp、rcc_prefix_builder.cpp |
| 1b-3 | 检查器：spawn 间能力冲突（Q4=A/B3）、父线程冻结、B4 跨层拒绝、循环内 spawn 明确拒绝 | rcc_codegen.cpp emitScope |
| 1b-4 | codegen：emitScope + spawn 特化 + trampoline（`_pendingTrampolines`）+ core_thread_create/join + Q17 join 先于 drop + B5 任务异常 abort | rcc_codegen.h/.cpp |
| 1b-5 | 回归套件 `tests/agent/p14-1b-scope-spawn`（7 用例）+ 全量回归 | tests/agent/p14-1b-scope-spawn/ |

### 7.2 关键实现决定

- **trampoline ABI**（方案 §6 风险点 1 的落地）：编译器生成 `spawn_tramp_<tag>_<i>`，入口 rcx=argblock（连续 8 字节槽，每实参一槽），取参到 rcx/rdx/r8/r9（首版 ≤4 参），sub $40（32 shadow + 8 对齐）后 call 目标函数。argblock/handle 槽用 `allocateSlot` 分配，跨 core_thread_create 调用存活。tag 用 `++_anonCounter` 保证多 scope 不重名。
- **父线程冻结**（方案 §6 风险点 2 的落地）：检查阶段收集被任务 take/mut（写能力）的路径，递归扫描 scope body 非 spawn 语句的赋值目标（`AssignmentNode`），与冻结路径 mayAlias 即报错。测试 `neg_parent_freeze` 验证。
- **循环内 spawn**：合法 spawn 全部由 emitScope 受控处理；任何落到 emitMethodCall 的 `.spawn` 都是非法的（循环/条件/嵌套/scope 外），给明确诊断"spawn must be a direct statement of a scope body"。测试 `neg_loop_spawn` 验证。
- **闭包对象复用**：trampoline 作为函数值走现有 `.Lfnclosure_*` 闭包对象机制（`core_thread_create` 在 1a-5 已适配解包一次）。

### 7.3 验证

- 正向：`pos_spawn_two`（两任务+join，allocs=frees=2）、`pos_spawn_one`（单任务，1/1）。
- 负面：`neg_take_conflict`（两 take 同 slot→parallel conflict）、`neg_cross_scope`（跨层→innermost scope handle）、`neg_task_throw_abort`（退出码 3）、`neg_loop_spawn`（循环内明确拒绝）、`neg_parent_freeze`（父线程写冻结 slot）。
- 套件 7/7 通过；全量 tests/agent 726 项全绿（另 p12-3c-ds 44、std-full 220 与之前一致）。

### 7.4 状态与后续

**P14-1b 完成**。语言层首版 scope/spawn 已落地，结构化并发的最小闭环（创建任务→并行→隐式 join→静态拒绝竞争）打通。
后续阶段（P14-3 par_each、P14-4 Locked/Atomic、P14-5 Chan）另行写方案；本阶段未覆盖的项：跨函数效果推断（Q4=B 已排除）、动态数量 spawn、带捕获闭包作任务入口、join 后实例级事实刷新的完整实现（首版检查器保守）。

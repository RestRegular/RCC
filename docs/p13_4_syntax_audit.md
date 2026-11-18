# P13-4 前置审计：子引用与局部排他（规范 §4）

> **本文性质**：审计 + 裁决记录，**不是实施方案，未改任何 codegen**。
> 遵守 RCC 硬规则（`1b1ccafb`）：任何语言设计 / 语义变更必须先向用户提出并获批。
> 审计阶段：①把规范 §4 的主张对照 `release\RCC.exe` 实测；②标出与现有语义的冲突；③列出必须先裁决的问题。
> **裁决状态（2026-11-19）：Q1–Q4 已全部逐条裁决并实施完成，见 §3 各项末尾的裁决记录。**
> 实施结果见 §4 与 `tests/agent/p13-4-subref`（9/9）。
>
> 审计基线：`release\RCC.exe`（2026-11-19 01:46:58，2650845 字节），HEAD `5c107e2`（P13-3 已提交）。
> 每个结论均有可复现探针（`build\p13_4_probe\*.rio`，`run.ps1` 批量编译）。
> 前置依据：`docs/p13_plan.md` §9.2、`docs/pending_designs.md` §D、`docs/memory-model-spec-full.md` §4/§4.3。

---

## 0. 结论摘要

P13-4 的规划定义是「**路径登记 + 前缀冲突判据**（P 是 Q 的前缀才冲突），验收 §4.1 冲突表、§4.3 类型级效果」。
实测结论：**单层路径判据已完整落地，但深路径登记、原地改值语义、§4.3 类型级效果三处仍有缺口**，且与 P13-5/P13-6 的边界存在歧义。

| 规范主张 | 实测 | 判定 |
|---|---|---|
| §4.1 单层路径前缀冲突（`p.name="y"` vs `&p.name`） | 正确报 `conflicts with the live sub-reference` | ✅ 已落地 |
| §4.1 不相交路径放行（`p.score=99` vs `&p.name`） | 编译通过 | ✅ 已落地 |
| §4.2 `take` 与任何子引用冲突、壳子引用不受影响 | 均符合 | ✅ 已落地 |
| §4.2 区间视图 `split_at`/`split_from` 共存 + 增长冲突 + NLL | 回归 6 例全过 | ✅ 已落地 |
| **§4.1 多级字段路径**（`p.team.captain.name`） | **深路径未登记，所有冲突静默放行** | ❌ **缺口 Q1** |
| **§4.1 原地改值 vs 替换的分界**（标量字段 `p.hp=20`） | 标量赋值一律按「替换」判冲突，无原地/替换之分 | ⚠️ **分叉 Q2** |
| **§4.3 类型级效果 `(类型.字段)`** | 只支持实例级 `writes[b.field]`；类型级写法无法解析 | ❌ **缺口 Q3** |
| `dict.clone()`（plan 记为 P13-4 缺口） | 明确报错 `unsupported receiver type`；依赖成员集合（P13-6） | ⚠️ **边界 Q4** |

---

## 1. 逐项实测对照（探针表）

| 探针 | 写法 | 实测结果 | §4 应有判定 |
|---|---|---|---|
| `b_disjoint_field` | `p.score=99` 同时 `&p.name` 活跃 | ✅ 编译通过 | ✓ 不相交，放行 |
| `g_scalar_assign_disjoint` | `p.hp=20` 同时 `&p.name` 活跃 | ✅ 编译通过 | ✓ 不相交，放行 |
| `c_replace_conflict` | `p.name="Bob"` 同时 `&p.name` 活跃 | ❌ `L3: field assignment to 'p.name' conflicts with the live sub-reference 'p.name' (§4.1)` | ✓ 相等路径，冲突 |
| `d_take_conflict` | `p.take()` 同时 `&p.name` 活跃 | ❌ `L3: take on 'p' conflicts with the live sub-reference 'p.name' (§4.1)` | ✓ 根是前缀，冲突 |
| `f_shell_ref_survives` | `p.take()` 同时壳子引用 `&p` 活跃 | ✅ 编译通过 | ✓ 壳子引用不是子引用，不受影响 |
| `h_ref_to_int_inplace` | `p.hp=20` 同时 `&p.hp` 活跃 | ❌ 报 §4.1 冲突 | ⚠️ 标量赋值被算作替换（见 Q2） |
| `e_nested_prefix` | 写 `p.team.captain.name` 同时 `&p.team` 活跃 | ✅ 编译通过 | ✓ P 不是 Q 的前缀，放行（正确） |
| `k_ancestor_write_conflict` | 写 `p.team` 同时 `&p.team.captain.name` 活跃 | ✅ 编译通过 | ❌ **应冲突**（P 是 Q 前缀）→ 深路径未登记 |
| `p_deep_exact_conflict` | 写 `p.team.captain.name` 同时 `&p.team.captain.name` | ✅ 编译通过 | ❌ **应冲突**（路径相等）→ 深路径未登记 |
| `q_deep_mid_conflict` | 写 `p.team.captain` 同时 `&p.team.captain.name` | ✅ 编译通过 | ❌ **应冲突**（P 是 Q 前缀）→ 深路径未登记 |
| `r_shallow_ref_deep_write` | 写 `p.team` 同时 `&p.team.captain` | ✅ 编译通过 | ❌ **应冲突** → 深路径未登记 |
| `i_writes_type_effect` | `writes[(Button.listeners)]` | ❌ `'writes[Button]' refers to 'Button', which is not a parameter` | ❌ 类型级效果未实现 |
| `s_writes_class_qualified` | `writes[Button.listeners]` | ❌ 同上 | ❌ 类型级效果未实现 |
| `l_writes_param_path` | `writes[b.listeners]`（实例级，形参） | ✅ 编译通过 | ✓ 实例级效果可用 |
| `m_write_via_stored_ref` | while 循环 + 含 `writes` 效果的函数调用 | ✅ 编译通过 | §4.3 迭代拦截无 for-in 落点（已知限制） |
| `a_inplace` | `p.hp += 10` | ❌ `Only simple variable compound assignment is supported` | 语言通用限制，非 §4（见 Q2 注） |
| `j_dict_clone` | `d.clone()`（dict） | ❌ `unsupported receiver type` | 见 Q4 |

**根因（深路径）**：`code/src/backend/rcc_codegen.cpp` 的 `emitExpression` 中 `RefExpressionNode` 分支（约 4382–4410 行）只处理**一层** DOT：
`dot->getRightNode()` 取末字段、`dot->getLeftNode()` 取根，仅当左节点是 `IdentifierNode` 时按 `ownerName + "." + field` 登记。
多级链 `p.team.captain.name` 的左节点是嵌套 DOT，`dotRoot` 为空、`owner` 退化成类名，登记路径既不是 `p.team.captain.name` 也不是其前缀 → `checkWriteConflicts` 完全漏检。

**已复用的地基**（无需重做）：`exprPathText`（任意深度 DOT 还原路径文本）、`pathIsPrefixOf`（按段比较）、`registerActiveRef`（路径穿透展开）、`checkWriteConflicts`（前缀判据主体）、`drainActiveRefs`（NLL）、区间视图 `rangesDisjoint` / `hasConflictingRangeRef`、`checkGrowthEffects`、`writes`/`from`/`ensures` 实例级登记。

---

## 2. 与现有语义 / 规划的冲突点

1. **深路径登记 vs 已有「结构体字段内子引用不带路径」取舍**：规范 §4.2 末句「结构体字段里存的子引用不带路径，统一按整个内容登记」是**有意取舍**。多级 `&p.team.captain.name` 要精确登记，必须确认这条取舍只对「存进字段里的引用值」成立，而不排除「对多级字段路径取引用」。（Q1）
2. **§4.1 表格的「原地改值」**：规范只给了 `p.hp += 10`（改不同字段）作例子，未明确「写标量字段本身」算原地还是替换。当前实现把一切字段赋值当替换；放宽需要类型系统知道字段是不是值类型，属语义变更。（Q2）
3. **§4.3 类型级效果 `(类型.字段)` vs 已有实例级 `writes[b.field]`**：两者是不同的效果表达，前者是「该类型任意实例的该字段」，由函数体推断、接口方法须声明。规范把它当作默认记法，当前实现完全没有。（Q3）
4. **规划自相矛盾处**：`p13_plan.md` §9.1/§9.2 把 `dict.clone()`、容器元素递归释放列为 P13-4 缺口，但同文又说这两项依赖「成员集合追踪」，而成员集合明确是 P13-6 的内容。边界必须澄清，否则 P13-4 会吞掉 P13-6 的工作。（Q4）

---

## 3. 待裁决问题

> 以下问题互不依赖，可逐条答复；每条给出推荐选项。答复后我才据此写实施方案并动 codegen。

### Q1 — 多级字段路径登记（§4.1 核心）

`&p.team.captain.name` 这类多级路径目前不登记，导致写 `p.team` / `p.team.captain` / `p.team.captain.name` 全部静默放行。

- **A（推荐）**：纳入 P13-4。修 `emitExpression` 的 `RefExpressionNode` 分支，用 `exprPathText` 还原任意深度的 DOT 链作为登记路径（如 `p.team.captain.name`），owner 取根变量名。这样 §4.1 冲突表里的 `p.team.captain` 例子真正生效。
- **B**：保守退化——把多级子引用统一登记到根 `p`（整内容），则任何对 `p` 子路径的写都冲突。实现最省，但会挡掉合法代码（如持有深引用时写 `p.score`），且违背 §4.1「P 是 Q 前缀才冲突」的精确判据。

**裁决（2026-11-19）：A —— 纳入 P13-4，用 `exprPathText` 精确登记多级路径。**

### Q2 — 原地改值 vs 替换的分界（§4.1 表格）

当前所有字段赋值都按「替换」判冲突：`p.hp=20` 与 `&p.hp` 冲突（`h_ref_to_int_inplace`）。规范表格区分「原地改值（否）」与「替换/重分配（是）」，但未明确标量字段赋值的归类。

- **A（推荐）**：保持现状（标量字段赋值也算替换 → 冲突）。理由：安全、语义简单，规范未要求放宽；且 `p.hp += 10` 这类复合赋值本身不被编译器支持（独立语言限制，不在 P13-4 处理）。
- **B**：放宽——按字段静态类型区分：值类型（int/float/bool/值语义 struct）字段赋值视为原地改值，不使子引用失效；引用类型（str/list/dict/class）字段赋值仍算替换。需要类型系统配合，改动面较大。

**裁决（2026-11-19）：用户自定义 —— 实现字段复合赋值（`p.hp += 10`），冲突行为保持现状（等同 A）。**
字段赋值仍按替换判冲突；本阶段补上「简单变量的复合赋值已支持，但字段/下标复合赋值不支持」的缺口（`a_inplace` 探针报错）。复合赋值走与简单赋值相同的路径冲突检查。

### Q3 — §4.3 类型级效果 `(类型.字段)`

当前只支持实例级 `writes[b.listeners]`；规范 §4.3 还要求类型级效果 `(类型.字段)`（该类型任意实例的字段），由编译器从函数体推断、接口方法须声明，并在「迭代容器期间调用函数」时据此拒绝。Rio 无 `for-in`，迭代拦截无落点。

- **A（推荐）**：本阶段只补「类型级效果的登记 + 从函数体推断」（不改语法，沿用 `writes[...]` 标注，新增类型级条目的解析），「迭代拦截」因无 for-in 登记为后续项不实施。
- **B**：整个 §4.3 推迟到 P13-5（签名标注阶段），P13-4 只做 §4.1 路径冲突。理由：§4.3 与签名标注强耦合，且主要消费者（迭代）无语法落点，单独做价值有限。

**裁决（2026-11-19）：A —— 本阶段补类型级效果登记 + 从函数体推断，迭代拦截登记为后续项。**

### Q4 — `dict.clone()` / 容器递归释放的归属

`p13_plan.md` 把这两项记为 P13-4 缺口，但同文指出它们依赖「成员集合追踪」（P13-6）。

- **A（推荐）**：明确从 P13-4 移除，归 P13-6。理由：深拷贝要安全释放元素必须知道「元素归不归这个容器所有」，这正是成员集合追踪的职责；P13-4 专注路径冲突判据。
- **B**：本阶段做有限版 `dict.clone()`——只深拷贝容器外壳 + 键，值按 D14 口径浅拷贝（与 `list.copy` 对齐），不做递归释放。能补全 API 面，但留下已知的不完整语义。

**裁决（2026-11-19）：A —— dict.clone() / 容器递归释放归 P13-6，从 P13-4 移除。**

---

## 4. 裁决后实施范围（P13-4）

| 项 | 裁决 | 实施动作 |
|---|---|---|
| Q1 多级路径 | A | 修 `emitExpression` 的 `RefExpressionNode` 分支，用 `exprPathText` 还原任意深度 DOT 登记路径；根变量取最左标识符 |
| Q2 复合赋值 | 自定义（保持现状 + 实现字段复合赋值） | 补字段/下标复合赋值（`p.hp += 10`），复用简单赋值的路径冲突检查；标量赋值仍按替换判冲突 |
| Q3 类型级效果 | A | 实现「从函数体推断写效果」（规范 §4.3「默认由编译器从函数体推断」）：`inferWriteEffectsFromBody` 扫描形参路径的字段赋值/复合赋值/take/mut，在 `registerSignatureEffects` 合并进效果表，调用点按 `resolveEffectPath` + `checkWriteConflicts` 拦。**未发明 `(类型.字段)` 的 Rio 拼写**（规范只给了元记号，无约定语法，按硬规则不自行设计，登记为待提案） |
| Q4 dict.clone() | A | 归 P13-6，从 P13-4 移除；更新 `p13_plan.md` 相关表述 |

**实施文件**：`code/src/backend/rcc_codegen.cpp`（Q1 `RefExpressionNode` 分支、Q2 `emitCompoundAssign` DOT 分支、Q3 `inferWriteEffectsFromBody` + `registerSignatureEffects`）、`code/include/backend/rcc_codegen.h`（新声明）、`tests/agent/p13-verify-fixes/code/cases.json`（I5 从 compileOnly 改为 expectFailure —— Q3 推断后该场景应报冲突）、`docs/p13_plan.md`、`tests/agent/p13-4-subref/`（新增 9 用例）。

## 5. 后续待设计项（本阶段登记，不实施）

- **`writes[(类型.字段)]` 的 Rio 拼写**（Q3 引出）：规范 §4.3 的 `(类型.字段)` 只是元记号，没有约定 Rio 写法；按硬规则不自行发明语法，须单独提案获批后再补「手写类型级效果标注」（本阶段只做函数体推断）。
- `sout(Ref[T])` 打印引用本身而非所指值（P13-3 Q5 已登记，需新增 API，单独提案）。
- 「迭代期间按类型级效果拒绝」依赖 for-in 语法，待后续阶段。
- `dict.clone()` / 容器元素递归释放（Q4，归 P13-6）。

## 6. 验收（2026-11-19）

- `tests/agent/p13-4-subref`：9/9 全过（Q1 深路径 4 例、Q2 复合赋值 3 例、Q3 推断 2 例）。
- 全量回归 13 套全绿：p13-2-regression 153、p13-verify-fixes 43、p13-3-nullable 20、p13-3-capability 19、p13-memory 28、p10-generics 34、p11-overload 24、p12-3c-ds 44、std-full 220、p12-std-skeleton 31、p5-containers 15、p6-classes 15、p2-functions 13。
- 工作区 unrelated 未提交改动未触碰；本轮仅改 codegen 两文件、cases.json、p13_plan.md + 新增两文档与一套件。

# P13-7 实施方案：`Link` 成对引用（规范 §7）

> **本文性质**：实施方案，**依据已裁决的全部项**（见 `p13_7_syntax_audit.md` §6），未改任何 codegen。
> 遵守 RCC 硬规则（`1b1ccafb`）：本方案实现的是规范 §7 **已冻结**的设计 + 用户本轮裁决；新增的东西只有 `Link` 这一语言原语（规范已设计、本轮已批准落地形态），不自创语法。
> 裁决：Q1=A（三段方括号）、Q2=A（新建 LinkType）、Q3=A（只 elem 入 mangle）、Q4=A（struct 定义期全量对偶校验+跨类）、Q5=B（基数单向宽松）、Q6=A（运行时同池检查+panic）、Q10=A（完整 §7.5）、Q11=A（扩展 Pool 槽位图）。
> 告知项维持：跨池禁止 / 三方拆多个 / 不开放 trust / `sout(Ref[T])` 不碰。

---

## 0. 范围与不做什么

**做**：
1. sema 新类型 `TK_LINK` + `LinkType(elem, peer, card)`，三段方括号 `Link[T][peer][card]` 解析与 mangle（`K` + elem）。
2. struct 定义收集期对偶声明全量校验（peer 字段存在 + 指回 + 类型一致 + 基数相容表 B，支持跨类）。
3. 五个原语 `add`/`remove`/`clear`/`set`/`unlink_all` 成对修改运行时（core 层）+ 同池 panic（Q6）。
4. 状态/事实：Pool 槽侧位图 `links_nonempty`（Q11）+ 编译期 `LinkSet` 事实表（空/非空/?）。
5. §7.5 效果追踪（实例级优先、退回类型级、`a≠b` 五来源、事实失效）（Q10）；`take`/`release` 的 Link 判据接入。
6. 回归套件 `tests/agent/p13-7-link`，覆盖 §7 全部 + §11.2 含环图示例。

**不做**（登记为后续项）：跨池开禁（§12 未决 8）、三方/多元 Link（§12 未决 9）、用户 trust 语法（§12 未决 10）、`sout(Ref[T])` 输出引用指向值（须单独提案）、成员集合完整化与 `dict.clone()`（P13-6 Q2/Q4 挂账）。

---

## 1. 关键约束（来自事实调研）

- **`link` 已是词法关键字**（`rcc_core.cpp:52` 小写，TOKEN_LINK）；**`Link` 是普通标识符**（大写，不在关键字表）——类型名走标识符路径，无需动 lexer。
- **parser 已支持多段 `[...]`**：`LabelNode::labelDesNodes`（`rcc_prefix_builder.cpp:436-462`），`func[A][B]` 是先例；`Link[T][peer][card]` 会解析成 `labelDesNodes[0..2]`。
- **字符串参数取法**：`labelDesNodes[i]->getBodyNode()` 是 `IdentifierNode`，直接 `getName()` 拿 `peer`/`card` 字符串，**不能**走 `typeFromTypeArgNode`（会把标识符查成 UNKNOWN Type）。
- **三点登记法**（`p13_6_implementation_plan.md:26`）：core 例程 = `core_win64.s` 实现 + `.globl` → `core.rio` extern → `coreReturnType()` 非 int 返回登记。构建链无需改。
- **ABI 铁律**（`core_win64.s:21-25`）：借 `%rsi/%rdi` 当临时必须 push/pop；跨 core 调用活值放栈槽；3 push + `sub $N` 保持 16 字节对齐；`rep movsb`/`rep stosb` 例程须自存自恢复 `%rdi/%rsi`（记忆 `85812347`）。
- **池头布局**（Q11 扩展点）：现 48 字节 6 字段（`core_win64.s:1003-1013`），侧位图每槽 2 位中 `links_nonempty` 位已规划未用（`p13_6_implementation_plan.md:24`）。

---

## 2. sema：`TK_LINK` + `LinkType`（Q1/Q2/Q3）

### 2.1 类型定义（`code/include/sema/rcc_sema_types.h`）

- `TypeKind` 枚举（`:21-40`）在 `TK_POOL` 后加 `TK_LINK`。
- 新增基数枚举：
  ```cpp
  enum class LinkCardinality { OPT, ONE, MANY };
  ```
- 新增 `LinkType final : public Type`：
  - 字段：`TypePtr _elem`、`std::string _peer`、`LinkCardinality _card`；
  - `isPointerLike()`（`:63-69`）加 `TK_LINK`；
  - `getSize()` 走默认 8（指针）；
  - `toString()`：`Link[<elem>][<peer>][<card>]`；
  - `equals()`：按 **Q3** 只比 elem（`_elem->equals(o->_elem)`），peer/card 不参与。

### 2.2 解析（`code/src/backend/rcc_codegen.cpp`）

- `parseTypeName()`（`:77-153`）：`base == "Link"` 时若 `labelDesNodes` 为空 → 报错「Link 需要三个参数」；否则读 `des[0]` 得 elem（`typeFromTypeArgNode`）、`des[1]`/`des[2]` 的 `IdentifierNode::getName()` 得 peer/card 字符串，card 解析（`one`/`opt`/`many`→枚举，其它→报错），构造 `LinkType`。
- `typeFromLabelNodeNonNull()`（`:10499-10646`）：五个 Memory 角色分支（`:10558`）旁加 `Link` 分支，逻辑同上；注意此处 `args` 已经把三段都 `typeFromTypeArgNode` 成类型了——Link 分支要在 `:10545` 通用 args 循环**之前**拦截，直接读 `labelDesNodes` 原始节点取字符串。
- `typeFromTypeArgNode()`（`:10316-10451`）：`IndexExpressionNode` 路径里 `Link` 形态要在更早的位置识别（同 `:10545` 的问题），或直接让 Link 声明只走 label 路径（字段/变量标注都是 LabelNode）。

### 2.3 mangle（`code/src/backend/rcc_symbol_naming.cpp`）

- `mangleType()` switch（`:80-99`）加 `TK_LINK`：返回 `"K" + mangleType(link->getElemType())`（Q3：peer/card 不入符号）。

---

## 3. 对偶声明校验（Q4/Q5，struct 定义收集期）

- **时机**：class/struct 定义收集完成、字段表建好之后，统一遍历所有声明了 Link 字段的类型做校验（避免在类型解析中途 peer 类型还没建好）。
- **校验项**（Q4=A）：对每个 `Link[T][peer][card]` 字段 F（属于类型 C）：
  1. C 存在名为 `peer` 的字段 F'，且 F' 也是 Link 类型；
  2. F'.peer == F 所在字段名（互相指向，支持自环：两个字段路径不相交时合法，规范 §7.2）；
  3. F.elem 与 F'.elem 类型一致（`equals()`）；
  4. F.card 与 F'.card 满足基数相容表（Q5=B）；
  5. elem 类型与 C（或跨类时 F' 所属类）一致——规范 §7.1「字段类型一致」；跨类 Link（Q4 确认）允许 F' 属于另一个类。
- **基数相容表（Q5=B，单向宽松）**：
  ```
  many ↔ (many | one | opt)   // many 端对端任意
  opt  ↔ (opt  | many)        // opt 端对端 opt 或 many
  one  ↔ (one  | opt)         // one 端对端 one 或 opt
  ```
  相容是对称判定：任一端按自己的基数检查对端是否落在允许集合内，两端都满足才算相容。兼容规范自带 `parent:opt ↔ children:many` 范例。
- **诊断**：违反任一项在字段声明处 `failAt` 报错，带字段名与对端字段名。

---

## 4. 运行时原语与同池检查（Q6）

### 4.1 core 层（`lib/core/core_win64.s` + `core.rio` + `coreReturnType()`）

用**邻接表**表达每个槽的 Link 集合。槽句柄即对象地址（8 字节），每对象每 Link 字段一个动态数组（head/count/cap 的 24 字节头，仿 List）。因 Pool 无类型参数、Link 字段在编译期可知字段偏移，采用**按 (slot, fieldIndex) 索引**的全局侧表而非把集合塞进对象：

| 例程 | 签名 | 语义 |
|---|---|---|
| `core_link_add` | `(pool:ptr, a:ptr, aField:i64, b:ptr, bField:i64) -> void` | 同池校验（Q6，失败 panic `cross-pool link`）；a、b Full 校验；两端各插入对端指针（去重）；置 a、b 槽 `links_nonempty` 位 |
| `core_link_remove` | `(pool:ptr, a:ptr, aField:i64, b:ptr, bField:i64) -> void` | 两端各删对端指针；集合空时清 `links_nonempty` 位 |
| `core_link_clear` | `(pool:ptr, a:ptr, aField:i64, bField:i64) -> void` | 遍历 a 的集合，对每个邻居调 remove 对偶边 |
| `core_link_unlink_all` | `(pool:ptr, a:ptr) -> void` | 遍历 a 的所有 Link 字段（字段数由 codegen 传入或按 sidecar 表），逐字段 clear；清所有 `links_nonempty` 位 |
| `core_link_is_empty` | `(pool:ptr, a:ptr) -> i64` | 返回 a 是否无任何 Link 边（读侧位图），供 take/release 判据或调试 |

- **同池判定**（Q6=A）：`a`、`b` 的槽地址落在池 `[data, data+used*elem_size)` 范围内即同池；否则 panic（退出码 3，诊断 `cross-pool link`）。
- **Full 校验**：读槽位图 `full` 位，非 Full panic（规范 §7.2「a、b 须确定 Full」）。
- **链接集合**：单一全局 side table 按 `(slotAddr, fieldIndex)` 哈希定位，避免把集合塞进对象布局；字段索引用字段声明顺序编号。
- 所有例程遵守 ABI 铁律（§1）。

### 4.2 codegen 发射（`rcc_codegen.cpp`）

- **方法调用识别**：参照 `ref()/mut()/take()` 的 receiver 形状识别（`takeReceiverName` `:6316-6330`），新增 `linkMethodKind()` 判断 `a.<LinkField>.<op>(args)`：
  - 从 receiver 解析出 a 表达式 + 字段名 F（需确认 F 是 Link 字段，从类型表查 `LinkType`）；
  - `op ∈ {add, remove, clear, set}`，或 `a.unlink_all()`（无字段名，遍历所有 Link 字段）。
- **参数**：`add(b)`/`remove(b)` 的 b 必须是同元素类型的 Body/Ref（Full 引用）；`set(p)`（仅 opt/one）先对旧对端发 remove 再 add 新对端（`a.L.set(p)` ≡ remove-old + add-new，规范 §7.2）；`clear()`/`unlink_all()` 无参。
- **发射**：把 receiver 求值的槽地址、字段索引、对端地址/字段索引放进 `%rcx..%r8`，调用对应 `core_link_*`；返回 void。
- **读 Link 内容**：`a.L.get()` 返回集合的 `LinkType.elem` 引用（Full，遍历/读取无需 `get()`——规范 §7.4）；`one`/`opt` 的 `get()` 返回 `T?`（单元素或 null）。实现为读 side table 的首个元素。

---

## 5. 状态与事实（Q11/Q10）

### 5.1 Pool 槽位图 `links_nonempty`（Q11）

- 在 P13-6 的每槽 2 位侧位图（`full` + `links_nonempty`）上，`core_link_add/remove/clear/unlink_all` 维护 `links_nonempty` 位。
- `core_link_is_empty` 读该位供判据使用。

### 5.2 编译期 `LinkSet` 事实表（§7.4「Link 是否确定为空」）

- `CodeGen` 新增事实表：`map<slotExpr, LinkEmptyFact>`，取值 `EMPTY` / `NONEMPTY` / `UNKNOWN`（对应状态格 §8.1）。
- 原语调用后更新：add/set→NONEMPTY；clear/unlink_all→EMPTY。
- **事实失效**（规范 §7.5）：跨函数边界、可能被别处加边的调用（类型级效果含该 Link 字段 add）→ 回到 UNKNOWN。

### 5.3 §7.5 效果追踪（Q10=A）

- 新增 `LinkEffect` 表（类型级 `(类.字段)` + 实例级 `(a.L, b.L')`），在函数调用点登记带 Link-add 效果。
- **实例级优先**：迭代 `u.fans` 期间，Link 原语调用与迭代冲突**当且仅当**对端可能是 `u`；能证明 `u ≠ 对端`（`a≠b` 五来源）则放行，否则退回类型级拒绝。
- **`a≠b` 五来源**：(1) 同池两次 add 返回 slot 互异；(2) 新建 Body 与已有 slot 不同；(3) `if a.is(b){}else{a≠b}`；(4) one/opt 对端非空时与别的对端不同；(5) snapshot 迭代变量默认不推出 `≠`，须先 `is` 排除。
- **保守规则**：`unlink_all` 与 `take`/`release` 之间不允许夹杂带 Link-add 类型级效果的调用（否则事实回 UNKNOWN，判据拒绝）。
- **并行接入**（§10.3/§10.4）：`L` 视为对两端字段的写并入 `W_X`；`par_each` 闭包体内禁调 Link 原语（P13-8 不做并发，但效果表先留接口）。

---

## 6. take / release 判据接入

- `release`（`rcc_codegen.cpp:12944-13022`）：在第 10 步成员判据后（`:13018`，注释已预留）、擦出生地前，插入「x 不在任何 Link 中」检查：查 `LinkSet[x]` 为 EMPTY 且 side table 中 x 无任何入边/出边；否则 `failAt`「cannot release: still in a link」。
- `take`（`:13052-13105`）：在 Full-only 状态检查块后（`:13097`）、`beginPendingTake` 前，插入「所有 Link 字段为空」检查：查 `LinkSet[x]` 为 EMPTY；否则 `failAt`「must unlink_all before take」。
- 原语本身（§7.2）的 Full/同池检查在运行时（§4），编译期这两处是额外的静态防御（符合 P13-6 Q3「不放宽状态规则，成员判据作防御性机械检查」的口径）。

---

## 7. 测试矩阵（`tests/agent/p13-7-link`）

| 用例 | 覆盖 | 期望 |
|---|---|---|
| `pos_add_bidirectional` | `a.follows.add(b)` 后两端互见 | 输出对偶读取 |
| `pos_remove_clear` | remove/clear 双向一致 | 两端 cleared |
| `pos_set_opt_one` | `n.parent.set(p)` 自动从旧对端移除 | 正确改指 |
| `pos_unlink_all` | 清空所有 Link 字段 | 全清 |
| `pos_traverse_no_get` | 遍历 Link 元素无需 `get()` | 直接打印 Full 元素 |
| `pos_cycle_graph` | §11.2 含环图（a→b→c→a） | 成环、正常运行、销毁点唯一 |
| `pos_self_loop` | 自环合法 | 正常 |
| `neg_single_side` | 用户拿不到单边接口（编译器不暴露） | 编译失败或无法构造单边 |
| `neg_cross_pool` | 跨池 add（Q6） | 运行时 panic，`stderrContains: cross-pool link`，exitCode 3 |
| `neg_take_linked` | 未 unlink_all 就 take（§7.4） | `expectFailure`，`mustMatch: unlink_all` |
| `neg_release_linked` | 在 Link 中 release（§7.4） | `expectFailure`，`mustMatch: link` |
| `neg_peer_mismatch` | 对偶不互指（Q4） | `expectFailure`，`mustMatch: peer` |
| `neg_card_incompatible` | 基数不相容（如 one↔many，Q5） | `expectFailure`，`mustMatch: cardinality` |
| `neg_cross_class_ok` | 跨类 Link（Q4=A 支持） | 编译通过 |
| `neg_iter_conflict` | 迭代中改邻居且可能是自身（§7.5） | `expectFailure` |
| `pos_fact_invalidation` | unlink_all 与 take 之间插 Link-add 调用（§7.5） | `expectFailure` |
| `pos_snapshot_modify` | `for f in u.fans.snapshot()` + `is` 排除 → remove | 正常 |

> 覆盖 §7 全部 + §11.2 含环图示例（验收条款）。

---

## 8. 实施步骤（顺序）

1. **sema 类型**：`TK_LINK`/`LinkType`/`LinkCardinality` + 解析 + mangle（§2）。编译通过，`Link[...]` 不再报 unknown type。
2. **对偶校验**：struct 收集后遍历校验（§3）；编译期能拒绝 peer/基数错误。
3. **运行时原语**：core 层五个例程 + 三点登记 + side table（§4.1）；同池 panic。
4. **codegen 发射**：方法识别 + 调用发射 + get 读取（§4.2）。
5. **状态/事实**：侧位图维护 + LinkSet + §7.5 效果表（§5）。
6. **判据接入**：take/release（§6）。
7. **回归套件**：§7 用例 + 全量 14 套回归（§7）。
8. **提交**：仅提交本阶段新增/修改文件，不混入工作区 unrelated 改动（记忆 `b7ac86a3`）；按项目规则发布/推送（先 memory_search 发布流程，见 `ed6be693`）。

---

## 9. 验收标准

- `tests/agent/p13-7-link`：全部用例通过（正向输出匹配、负向诊断命中）。
- 全量既有回归 14 套全绿（与 P13-6 基线一致）。
- §7 全部条款 + §11.2 含环图示例有对应用例或诊断。
- 文档 `p13_plan.md` / `pending_designs.md` 的 P13-7 状态行同步更新为「已完成」。

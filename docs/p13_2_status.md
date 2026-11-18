# P13-2 实施总结（2026-11-18）

> 本文记录 P13-2 内存模型语法的实施状态，基于本会话 37 个提交
> （其中 32 个动 `code/`，其余为文档与测试脚本）。
> 所有变更均来自已批准的设计文档 `memory-model-syntax-resolutions.md`
> 与 `p13_2_syntax_resolutions.md`，未自行设计语法。

## 已完成（按依赖顺序）

| 阶段 | 提交 | 内容 |
|---|---|---|
| L0 去伪 | 16c5cf6 | 7 项静默错误改编译报错 |
| L1 地基 | 2bfe1d2 / 84d4a91 | ret/return 拆 token 与退最近作用域；裸块识别（Q-D） |
| L2 结构语义 | c3cdcdb / fa5f3ce / b8d16d3 / 9b5e542 | if/while `: T` 标注校验；break/ret value 值合流；if 内 ret value |
| L3 地基 | 3620e94 / 24ea573 | 五类型进 sema（Body/Content/Ref/Mut/Take）；Take 位置约束 |
| L3 移动 | bda8c72 / 9ac487c / 680689a / 62b17bf / 60d4cf6 / 719be5e | Content 移动；移动状态表；Content 形参 take；& 前缀；ref/mut；take 落定；Body→Ref/Mut 形参隐式引用 |
| A Pool | 0122e10 / 85ab087 | Pool 类型骨架 + 标签区出生地识别 |
| B pool.new | c2c462e | `pool.new[T](...)` 语法形态 + 出生地一致性检查 |
| C 四态 | 9fa9ae6 / 01bfcbe | E/F/H/X 事实表；fill/clear/release 内建方法；take 状态修正（Body→E） |
| D 挂起态 | f49e008 | pending_take + 独占检查 + 独立 take 警告 |
| E 子引用 | 07c8ba0 / 5e85a3d / 9b0017d / 5a800ec / 645ad6b | 子引用类型+地址；路径表+冲突；NLL；穿透展开；get_mut+不等事实 |
| F 诊断 | ba1f99b / 9ebc47b / a58b1cd / 798f526 / 0c4c1ff / 000629a | 子句进 AST；from 检查；ensures 语法校验；Take 形参调用；ensures 逐出口；writes 泛型校验；实施总结 + dump 子句参数 |

**验证**：`tests/p13_2_regression.ps1`，60 条用例全 PASS；每次提交后回归。
另有 `tests/_probe/verify_fixes.ps1`（25 条），覆盖审阅中发现的实现缺陷及其对照用例。

## 剩余项与状态

### A. 设计文档明确暂缓（不属当前可实施范围）

| 项 | 依据 | 说明 |
|---|---|---|
| Maybe / if-let / match（§6.1） | Q3 机制待用户提供设计 | 可空类型方案已定为「只有引用类型可空 + 对象包装基本类型」，但 `if let` / `match` 的语法未定 |
| 条件销毁属性语法（§5.4） | 归 Q4 条件销毁，待设计 | 决议只写「静默生效，无提示语法」，属性形态未定 |
| 值语义 struct（§6.2） | struct 声明语法待用户提供 | 实测 `struct Pt { ... }` → `Undefined variable: struct` |
| split(k)（§4.3） | 决议只写「返回两段 Mut 视图」 | 两段如何绑定（元组/数组/两次调用）未定；实测 `xs.split(1)` → "Cannot determine the object type of method call" |

### B. 决议已定但未实现（真实缺口）

> **分支 / 循环的状态合流已不在本表**：2026-11-18 已实现，语义与实测见下文
> 「L3-G」一节。

| 项 | 决议依据 | 实测表现 |
|---|---|---|
| `Link[T][peer][cardinality]` | §7.1、§9.3 的 `release` 前置含 Link 检查 | 类型不存在：`unknown type 'Link' of field` |
| `clone()`（深拷贝） | §11「拷贝用显式 `clone()`（深拷贝，P13-1 起）」 | `Class Player has no method named 'clone'` |
| 缺省 `writes` 的函数体推断 | §9.3「用户写的函数：缺省 `writes` 按函数体推断记」 | 退化为「没写就不检查」；用户已明确**暂不补** |
| `writes[this.x]` 的方法内校验 | §9.3「`writes[this.x]` 允许（方法内写自身字段）」 | 方法上写 `writes[zzz.name]` 不报错（普通函数会报）—— 方法路径整体没进校验 |
| `throw` 出口不检查 `ensures` | §9.2「`throw` 出口不检查 `ensures`；catch 之后 `Take` 形参状态回到 `?`」 | `fun f(t: Take[P]): int ensures[!t] { throw "x" }` 报「unknown state at function exit」 |
| try/catch 后 `Take` 状态回 `?` | §9.2 | try 里 take、catch 后 `ret`，承诺被沿用（不报错） |
| `ensures` 重复列同一形参 | §9.2 每条形参只能有一个状态 | `ensures[!t, !t]` 不报错 |

### C. 实现缺陷（与决议不符，属 bug）

| 缺陷 | 根因 | 实测 |
|---|---|---|
| §6.4 独占未检查 | 只有「同一语句二次 take」查了 pending_take；`mut()` 之间、`Ref` 与 `Mut` 之间没有重数检查 | `var m1 = p.mut() var m2 = p.mut()` 通过；`var m = p.mut() var r = &p` 通过 |
| 未声明的出生地标签不报错 | 标签只做「最多一个」检查，没查是否对应真实 Pool 变量 | `var p: Player nosuchpool = Player("a")` 通过 |
| 类赋值类型检查弱 | `var a: Enemy = Player("x")` 无继承关系也通过 | 通过（P13-1 遗留，非 P13-2 引入） |

## 判断

- objective 中「L0→L1→L2→L3→L4」各阶段的**主体**均已落地：
  L0 去伪、L1 语法地基、L2 结构语义、L3 内存模型主体、L4 诊断（from/ensures/writes 语法层）。
- L4 的「可选类型」部分、split 绑定形态、条件销毁被设计文档明确推迟，
  不属当前可实施范围（硬规则：语言设计层变更必须先获批）。
- **writes 调用点效果（决议 §4.2）曾整块缺失，现已实现。**
  早先的判断（「NLL 已在引用最后使用后将其移除，writes 要影响的活跃引用
  本就不存在」）经实测证伪：`_activeRefs` 的清理只发生在 `drainActiveRefs`
  被调用的语句边界上，而冲突检查原本只有 `get_mut` / `release` / `clear`
  三处会查 `hasActiveRefOn`，`take` / `mut` / 字段赋值 / 调用 `writes` 函数
  一律不查。实测（修前）：
  - `writes[p.name]` 的函数在 `&p.name` 仍活跃时被调用 → 编译通过（应报错）
  - `p.name = "y"` 时 `&p.name` 仍活跃 → 编译通过（违反 §4.1 前缀冲突）
  - `&p.name` 活跃时 `p.take()` / `p.mut()` / 连续两次 `p.mut()` → 均编译通过

  现已按决议补齐：函数效果表（`_writeEffects` / `_fromEffects`，键为发射符号，
  重载按 mangling 区分）在发射任何函数体之前由 `registerSignatureEffects`
  登记完，调用点由 `applyWriteEffects` 把 `writes` 路径映射回实参路径，
  再按 §4.1「P 是 Q 的前缀（按路径段，含相等）」判冲突。本地写路径
  （字段赋值 / `take()` / `mut()`）走同一个 `checkWriteConflicts`。
- 唯一未实现的可选项 split(k) 因决议未写两段 Mut 的绑定形态而暂缓，
  不自行设计。

## 审阅后的修复（2026-11-18）

对本文件前一轮结论的复核发现若干「实现与已批准决议不符」的缺陷，
均属内部一致性 bug（不涉语言设计变更），已修复：

| 缺陷 | 根因 | 修复 |
|---|---|---|
| 内存模型事实表跨函数泄漏 | `emitFunctionBody` 只清 `_ehStack/_loopStack/_scopes/_scopeDrops`，不清 `_moveStates/_activeRefs/_birthPlaces/_pendingTakes`；走到函数末尾自然结束的路径不经过 popScope，泄漏最明显 | 进入函数体时清空、退出时回滚 |
| `take()` 无状态前置 | 只查同一语句内的 `pending_take`，跨语句的 E/X 态没人管 | 增加 E（Empty）/ X（MOVED）前置拒绝 |
| `pool.new[T]` 结果类型不绑定 | 只认了 AST 形状，没校验 T 与声明类型、也没校验接收者是 Pool | 校验 `T` 与变量类型一致 + 接收者必须是 `Pool` |
| `ensures[+a]` 被整体静默丢弃 | `+a` 走 `PrefixExpressionNode`，而校验只匹配了 `UnaryExpressionNode`（`!` / `?`） | 补 `PrefixExpressionNode` 分支 |
| `get_mut` 的登记项永不淘汰 | 登记时 `refVar` 留空，而 `drainActiveRefs` 对空 `refVar` 一律保守保留 | `var a = xs.get_mut(i)` 时绑定 `refVar` |
| `writes` / `from` 子句不校验参数 | 两个子句此前完全没有校验 | 每项必须是形参路径；`from` 另校验「返回引用来源 ⊆ 声明集」（§9.4） |
| 多出生地标签不报错 | `findBirthPlaceLabel` 只返回第一个，发现不了第二个 | 全量收集后校验「最多一个」（§2.2） |
| 内存模型形参无法访问字段/方法 | `classNameOf` 的 Identifier 分支只认 `TK_CLASS`，`Take[Player]` 这类拿不到内层类名，`t.clear()` / `t.name` 全报 "Cannot determine the object type" | 改走 `classNameFromVariable`（已处理 MemoryType inner）；`.field` 分支同样补 MemoryType 穿透 |
| `return bob.take()` 不落定 | §6.2 列的四种落定位置，第 3 种（`return`）没结算，take 在语句末被回填成 F，`ensures[!t]` 因此不可达 | `emitReturn` 里补 `settlePendingTake` |
| §4.2 writes 调用点效果缺失 | `writes` 只在泛型定义处做形态校验，调用点从不读 | 效果表 + 调用点映射 + §4.1 前缀冲突检查 |
| take / mut / 字段赋值不查活跃子引用 | 冲突检查只在 `get_mut` / `release` / `clear` 三处 | 三处本地写路径统一走 `checkWriteConflicts` |

验证：`tests/p13_2_regression.ps1` 75 条全 PASS（新增 L3-G 15 条）；
`tests/_probe/verify_fixes.ps1` 43 条全 PASS。

## L3-G：分支 / 循环的状态合流（2026-11-18 实现）

### 语义（依据 §8.1 格上界、§14 循环不动点）

状态格 `F ⊔ E = ?`、`X ⊔ X = X`、`HELD ⊔ FULL = FULL`（HELD 是 FULL 的子态）。

| 位置 | 合流路径 |
|---|---|
| `if` / `elif` / `else` | 每个分支出口的状态快照 + （无 `else` 时）入口状态 |
| `while` / `for` | 入口状态 ⊔ 回边状态（含 `continue` 与 for-update）⊔ `break`/`ret` 出口状态 |
| 发散的分支 | `return` / `throw` / `break` / `continue` 到不了汇合点，**不参与**合流 |

`?` 态在 §3.4 下禁止直接读，§6.1 / §10.2 / §10.3 下禁止 `take` / `fill` / `clear` / `release`。

### 实现

- `_moveStates` 仍是作用域分层 map 栈，但 `emitIf` / `emitLoop` 现在显式做
  快照与合并：`snapshotMoveStates` / `restoreMoveStates` /
  `joinMoveStateSnapshots` / `joinMoveStatePaths`。
- 合并按 `depth = min(所有路径的层数)` 对齐，逐层按格上界合并；只在某条路径
  出现的条目取该条目的值（其余路径视为「未改动」），从而保证分支内声明的
  同名局部变量不会把状态泄漏到外层同名变量。
- `statementDiverges` 判定发散语句（含块尾递归、if 全分支发散且有 `else`）。
- 循环的出口/回边路径用 `_loopExitStates` / `_loopBackStates` 两个栈收集，
  随函数体进出清空。

### 一个必须区分的细节：`Take` 形参的入口 `?` 不是合流 `?`

§9.2 规定 `Take[T]` 形参进入函数时状态是 `?`（编译器不做承诺），此时
`t.name` / `t.greet()` / `t.take()` 都是合法的（§7.3 独占取内容）。
这与「控制流合流后不确定」的 `?` 语义完全不同，不能共用一条拒绝规则。

实现上用 `_opaqueMoveVars` 记录这类变量，所有 `?` 拒绝统一走
`isMergedUnknown(name)`（= 状态是 `UNKNOWN` 且不在不透明集合里）。一旦某条
路径把它推到过确定状态，`refreshOpaqueAfterMerge` 就把它移出集合 —— 此后
合流出来的 `?` 是真正的不确定。

### 实测（`tests/p13_2_regression.ps1` L3-G 15 条）

| 用例 | 修前 | 修后 |
|---|---|---|
| 互斥分支各 take 一次 | 误报 Empty | 通过 |
| 分支只一侧 take，之后读 / take | 漏报 | 报 `? state` |
| 两侧都 clear 后 fill | 误报 | 通过 |
| 嵌套 if / 三分支全 clear | 误报 | 通过 |
| 缺 else（有 fall-through） | 漏报 | 报 `? state` |
| 分支内同名局部变量 | 污染外层 | 不污染 |
| 循环体 clear 后读 | 报 Empty（本应 `?`） | 报 `? state` |
| 循环体 clear+fill（回边回到 F） | 漏报 | 通过 |
| break 出口参与合流（F⊔E=?） | 出口被丢 | 报 `? state` |
| `Take` 形参入口 `?` 读字段 / take | — | 通过 |

## 未实现项清单（2026-11-18 复核，逐项实测）

> 本节是上文「剩余项与状态」的补充说明。所有结论均由 `tests/_probe/fix/`
> 下的最小用例实测得出，不是从文档推断。

**分支/循环的状态合流已在 2026-11-18 实现**（见上文「L3-G」一节）。
当时的根因分析保留在此供溯源：`_moveStates` 是「按作用域分层的 map 栈」，
`getBodyState` 从最内层向外回退查找第一个同名项 —— 这意味着**内层作用域写的
状态会泄漏到外层和兄弟分支**，因为 `popScope()` 只是把内层的 map 丢掉，外层
那份从头到尾没被更新过。正确做法是在 `emitIf` / `emitLoop` 的分支出入口做
状态快照与上界合并，而不是靠作用域栈的遮蔽 —— 这正是最终采用的方案。

**其余缺口的性质**：`Link` / `clone()` 是**类型与方法整体不存在**（不是
语义错误，是用不了）；`writes[this.x]` 的方法路径没进校验是**校验覆盖
不全**；`throw` 出口和 try/catch 是**出口枚举不全**（只枚举了正常出口）。
这些都属实现缺口，不涉语言设计变更。
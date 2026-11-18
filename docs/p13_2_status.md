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
| G 合流 | 111（L3-G） | 分支/循环的移动状态快照 + 格上界合流 |
| H ensures | L3-H | throw 出口不检查 ensures；重复项逐项继承前缀标记 |
| I 方法子句 | L3-I | 方法上的 writes/from/ensures 进校验；try/catch 后 Take 回 `?` |
| J 出生地 | L3-J | 未声明出生地标签报错；类赋值方向校验 |
| K/L 顶层与标签 | L3-K / L3-L | 顶层 var 进移动状态表；`static`/`global` 生命周期标签 |
| M 回归修复 | 59359cf | 泛型类字段 `T`；drop glue 不发 `.globl` |
| N 块表达式 | L3-N | 右值位置的裸块作可返回值作用域（`var a: int = { ret 10 }`） |
| O 方法 ret | L3-O | 类方法漏传 `retTypeAnnotated`，`ret` 类型校验与返回值转换整段被跳过 |
| P std 重写 | 29f6e07 | 标准库（str/io/ops/math/ds）按内存模型规范重写完成；期间修掉四个编译器缺陷，见「C-2」一节 |
| Q clone() | 29f6e07 | `x.clone()` / `clone(x)` 按静态类型派发（str/list/值类型；dict 待 P13-4） |

**验证**：`tests/p13_2_regression.ps1`，**126 条用例全 PASS**；每次提交后回归。
另有 `tests/_probe/verify_fixes.ps1`（43 条），覆盖审阅中发现的实现缺陷及其对照用例。

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
> **`throw` 出口不检查 `ensures`、`ensures` 重复项已不在本表**：2026-11-18
> 已实现，见下文「L3-H」一节。
> **方法上的 `writes`/`from`/`ensures` 校验、try/catch 后 `Take` 回 `?` 已不在本表**：
> 2026-11-18 已实现，见下文「L3-I」一节。
> **未声明的出生地标签、类赋值方向已不在本表**：2026-11-18 已实现，
> 见下文「L3-J」一节。
> **块表达式作右值已不在本表**：2026-11-18 已实现，见下文「L3-N」一节。

| 项 | 决议依据 | 实测表现 |
|---|---|---|
| `Link[T][peer][cardinality]` | §7.1、§9.3 的 `release` 前置含 Link 检查 | 类型不存在：`unknown type 'Link' of field` |
| 缺省 `writes` 的函数体推断 | §9.3「用户写的函数：缺省 `writes` 按函数体推断记」 | 退化为「没写就不检查」；用户已明确**暂不补** |

> **`clone()` 已不在本表**：2026-11-18 已实现。方法形式 `x.clone()` 与自由
> 函数形式 `clone(x)` 都可用，按接收者的**静态类型**派发：
> `str` → `core_clone_str`、`list` → `core_clone_list`、
> `int`/`float`/`bool`/`char`/`ptr` → 原样返回（值语义，不分配）。
> 派发在 `emitMethodCall` / 自由函数调用两处，类型推断在 `inferType`
> 的 DOT 分支与裸名调用分支各补一条（否则结果槽会按 int 分配，
> `var t: str = clone(s)` 赋值处报类型不符）。
> `dict` 暂不支持：P13-4 之前没有 dict 的 clone 原语，显式报错而不是静默浅拷贝。
>
> 这是 std 里「泛型形参要返回」的唯一合法手段（D9 规则 C 不放宽）：
> `ops.rio` 的 `ternary` / `orLazy` 已改为 `ret clone(...)`。



### C. 实现缺陷（与决议不符，属 bug）

> **本表已清空**：原列的两条（「未声明的出生地标签不报错」「类赋值类型检查弱」）
> 已于 2026-11-18 修复，见下文「L3-J」一节。修后实测：
> `var p: Player nosuchpool = Player("a")` 报 `unknown label 'nosuchpool'`；
> `var a: Enemy = Player("x")` 报 `not the same class or a derived class`。
> 其余回归修复见「L3-M」一节。

### C-2. 2026-11-18 标准库重写期间修掉的编译器缺陷

std 按内存模型规范重写时暴露出来的四个编译器缺陷，全部已修：

1. **`return` 在循环里返回的是循环值而非函数值**
   `fun f(): int { while i < 3 { if i == 1 { return 42 } ... } ret -1 }`
   打印 `-1`，期望 `42`。根因在
   `builder/rcc_prefix_builder.cpp` 的 `buildReturnExpression()`：
   `exitsFunction` 用 `currentToken()` 求值，而那时代码已把表达式解析完，
   游标早就不在 `return` 上 —— 每个带值的 `return` 都被当成 `ret`。
   改用 `returnToken.getType()` 后，`ret`（退出最近的带值作用域，
   循环里等价于 `break v`）与 `return`（退出函数）各归其位。
   这一个修复连带解决了 `find`/`replace`/`split`/`trim` 整片 std 错误。

2. **`ptr` 包装成 `str`/`list` 时所有权判定错误**
   两个方向都踩过：只看类型相等时，`_dsLoadI64` 把形参 `scratch` 当拥有者，
   函数退出 `core_free` 掉**调用方**的缓冲区（`dictValues` 崩
   `exit=-1073741819`）；反过来让 ptr 一律转移所有权时，
   `toStr[T,K](dict)` 里 `var slot: str = scratch` 之后 `slot = scratch`
   会把同一个堆块先 free 再写回，循环第二轮读 use-after-free。
   最终口径回到 D14 的字面意思：**ptr 永远不承担所有权**，
   经 ptr 包装出来的一律是借用视图。因此 `_strNew` 的返回类型从
   `ptr` 改成 `str`（分配点直接产出拥有者），`io.sin()` 的缓冲区同理；
   确实需要把借用转成独立值的 `_dsSlotToStr` 用 `clone`。

3. **控制流合流时 drop 表的 `moved` 取了「与」**
   注释原写「代价只是某条路径拥有却没销毁 = 漏一次释放」，实际相反：
   `split` 里 `var front: str = subStr(...)` 之后
   `if ... { result += [front] }` 只有一条路径移动，合流后
   `moved = true && false = false`，循环体出口照样 drop `front` ——
   而它已经作为元素进了容器。实测 `split("a,b", ",")` 返回 `["", ""]`，
   元素槽指向已释放的头。改成取「或」（漏放可容忍、重复释放不可，
   与 D17 的既有取舍一致）后正常。

4. **`ret` 形参被误当成「ptr 形参」放行**
   为修 (2) 给 `registerBorrow` 加了 ptr 登记后，
   `fun asInt(p: ptr): int { ret p }` 被借用检查拦下。
   ptr 没有语言层所有权，借用语义对它不适用（`ret p` 只是返回位模式），
   已在 `checkBorrowNotEscaping` 里显式放行 ptr。

修后实测：`test_scripts/test_all.ps1` 25 套件全过（718 用例通过，
仅 `test_legacy_all` 两个 G6 已延期的用例失败）；
`tests/p13_2_regression.ps1` 126/126；`tests/_probe/verify_fixes.ps1` 43/43。

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

验证：`tests/p13_2_regression.ps1` 100 条全 PASS（L3-G 15 + L3-H 10 + L3-I 9 + L3-J 6 条）；
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

## L3-H：ensures 的出口枚举与重复项（2026-11-18 实现）

### 1. `throw` 出口不检查 `ensures`（§9.2）

决议 §9.2 明文：「**每条正常出口**（`ret`、走到末尾）都必须满足 `ensures`；
**`throw` 出口不检查 `ensures`**」。

修前的实现只在两个地方调用 `checkEnsuresAtExit`：`emitReturn`（显式 `ret`）
和函数体末尾。后者对应「走到末尾」这个正常出口 —— 但当函数体最后一条语句
本身是发散的（`throw` / `return` / 全分支发散且有 `else` 的 `if`）时，
根本不存在这条出口，却仍被拿去做校验：

```rio
fun f(t: Take[Player]): int ensures[!t] { throw "x" }   // 只有 throw 一个出口
```

修前报 `Take parameter 't' has unknown state at function exit`，修后通过。

修法是复用 L3-G 已有的 `statementDiverges`：只在函数体**可能**走到末尾时
才做末尾校验。`if (c) { throw } ret 1` 这种「有 throw 但也有正常出口」的
仍然必须报错（`ret 1` 那条出口上 `t` 未被消费），这条也加了对照用例。

### 2. `ensures` 重复列同一形参（§9.2）

§9.2：每条形参只能有一个状态。修前 `ensures[!t, !t]` 静默通过。

根因不在「没写查重」，而在**整条子句根本没被解析出来**：`!` / `?` / `+`
是前缀运算符，而 parser 让前缀优先级**低于**逗号，所以 `ensures[!t, !t]`
被解析成 `!(t, !t)` —— 整个 `t, !t` 是 `!` 的操作数。原来的展平直接用
`flattenTypeArgs`，只会看到一个 `UnaryExpressionNode`，其操作数是逗号表达式
而非标识符，名字解析为空后被 `continue` 跳过，于是整条子句既不登记也不报错。

对比实测（修前）：

| 写法 | 修前 | 原因 |
|---|---|---|
| `ensures[!t]` | 正确报错 | 单项，名字能解析 |
| `ensures[!t, !t]` | **exit=0** | `!(t, !t)`，名字解析为空被跳过 |
| `ensures[?t, ?t]` | **exit=0** | 同上 |
| `ensures[t, t]` | 报「more than once」 | 无前缀，逗号正常展平 |

修法：新增 `collectEnsuresItems`，把前缀标记当作**继承状态**往下递归传递，
逗号每一项各自继承 —— 这样 `!(t, !t)` 会被还原成 `(!t, !t)`，重复检测
才有机会看到两项。

### 实测（`tests/p13_2_regression.ps1` L3-H 10 条）

| 用例 | 修前 | 修后 |
|---|---|---|
| 只有 `throw`，`ensures[!t]` / `[+t]` | 误报 unknown state | 通过 |
| `throw` 之后仍有 `ret` 出口 | 报错 | 报错（正确，不能整体豁免） |
| `throw` 之后 `ret` 前已消费 | — | 通过 |
| `ensures[!t, !t]` | exit=0 静默通过 | 报 more than once |
| `ensures[?t, ?t]` | exit=0 静默通过 | 报 more than once |
| `ensures[t, t]` | 报 more than once | 报 more than once |
| `ensures[!t, +t]`（不同状态） | exit=0 静默通过 | 报 more than once |
| 单写 `!t` 未消费 / 单写 `?t` | 正确 | 正确 |

## L3-I：方法上的子句校验 + try/catch 的 Take 复位（2026-11-18 实现）

### 1. 方法上的 `writes` / `from` / `ensures` 从未进校验

`emitClassBodies` 发射方法体时只调了 `emitFunctionBody`，既没登记
`_functionParamNames`，也没设 `_currentSignatureClauses`。后果是三件事同时失效：

| 现象 | 根因 |
|---|---|
| `fun bad(): int writes[zzz.name]` 静默通过（顶层函数会报） | `validateWritesFromClauses` 遍历空表 |
| `fun cmp(o: P): bool writes[o.name]` 误报「o 不是形参」 | `isFunctionParam` 的键是符号名，方法从没登记过 |
| 方法上的 `ensures` 完全不生效 | `validateEnsuresClauses` 同样是空表 |

修法：在 `emitClassBodies` 里用**发射名**做键登记形参名（与 `_functionParams`
/ `_functionParamAnnotated` 一致），并把 `def->getSignatureClauses()` 装进
`_currentSignatureClauses` 后再调 `emitFunctionBody`。

同时 `isFunctionParam` 要认得 `this`：决议 §9.3 明文允许 `writes[this.x]`
（方法内写自身字段），§9.4 的 `from[this.x]` 同理。修前把 `this` 当普通局部：

```rio
fun refSelf(): Ref[str] from[this.name] { ret &this.name }
// 误报：cannot return a reference to local 'this'
```

现在 `this` 在方法体内等价于一个形参（判据：`_currentClass` 非空）。

### 2. try/catch 后 `Take` 形参状态回到 `?`（§9.2）

决议：`ensures` 是「正常返回时我保证成这样」的承诺；异常路径上 try 体可能在
任何一条语句处中断，后面那些 take / clear 有没有执行过是**不确定**的，
所以 catch 里不能沿用 try 体走完后的状态。

修前 try 里 take、catch 后 `ret`，`ensures[!t]` 被沿用而通过。

修法：进 catch 前先用 **try 入口快照**复原状态（抹掉可能只执行了一半的改动），
再把 `ensures` 里那些 Take 形参统一推成 `?`，并加入 `_opaqueMoveVars`
（`?` 对 Take 形参是不透明的，字段 / 方法 / `take()` 仍合法 —— 与 L3-G 一致）。

### 3. 两条路径必须合流（否则正常路径被污染）

只做 catch 复位会引入新 bug：正常路径和异常路径共用同一份编译期状态表，
catch 里为 §9.2 写的 `?` 会直接盖掉正常路径上已经确定的状态。

```rio
fun f(t: Take[Player]): int ensures[!t] {
 try { var c = t.take() } catch (e: str) { throw "rethrow" }
 ret 1                                   // 正常路径上 t 明明是 E
}
```

实测（只做 catch 复位时）：误报「unknown state at function exit」。

修法：正常路径的状态在进 finally / catch **之前**先快照一份，末尾按 L3-G
同一套规则合流（发散路径不参与；只剩一条路径时直接用它的状态，不平白升成 `?`）。

### 实测（`tests/p13_2_regression.ps1` L3-I 9 条）

| 用例 | 修前 | 修后 |
|---|---|---|
| 方法 `writes[zzz.name]`（非形参） | exit=0 静默通过 | 报 not a parameter |
| 方法 `writes[this.hp]` | 报 not a parameter | 通过 |
| 方法 `writes[o.name]`（o 是形参） | 报 not a parameter | 通过 |
| 方法 `from[this.name]` + `ret &this.name` | 误报 reference to local 'this' | 通过 |
| 方法 `ret &this.name` 不在 from 集合 | 误报 local 'this' | 报 not listed |
| catch 后 `ret` 不沿用 try 的 take | exit=0（承诺被沿用） | 报 unknown state |
| 正常路径不受 catch 的 `?` 污染 | — | 通过 |
| catch 里 `?` 态仍可 take / 读字段 | — | 通过 |
## L3-J：出生地标签校验 + 类赋值方向（2026-11-18 实现）

### 1. 未声明的出生地标签被静默丢弃

决议 §2.2：「出生地标签必须是一个 Pool 类型的变量名」。但标签区里既解析不成
类型、又不是 Pool 变量的名字，修前被**直接忽略** —— 出生地靠
`findBirthPlaceLabel` 的 `lookupVar` 认，认不出就当没写：

```rio
var p: Player nosuchpool = Player("a")   // 修前 exit=0，落进默认池
```

pool 名字拼错、或从没声明过，编译器都不吭声，运行时悄悄用默认池，
事后排查毫无线索。

修法：在 `emitVariableDefinition` 的标签区检查里补一条 —— 按 §2.2 / Q-A 的
判定顺序（内置类型 → 泛型形参 → 自定义类 → Pool 变量）逐个过，能解析成
具体类型的是类型标签，剩下的必须能解析成 Pool 类型变量，否则报
`unknown label '<name>' ... must name a Pool variable`。

### 2. 类赋值方向

`checkAssignable` 对 `TK_CLASS` 落到 `sameValueFamily`，而后者有一条
「指针族互通」的放宽（为了让 rt 的位级中转 `var s: str = p`、
`_dsLoadI64` 的伪造 str 头能过）。`ClassType` 也 `isPointerLike`，
于是这条放宽把类赋值一起放过了：

```rio
class Player { ... }
class Enemy  { ... }
var a: Enemy = Player("x")   // 修前 exit=0
```

编译期认了，运行时按 `Enemy` 的字段布局去读 `Player` 对象 —— 字段错位。

修法：`TK_CLASS` × `TK_CLASS` 单独判。**Rio 有继承**（`class Derived: Base`，
基类字段排在前面，见 `computeLayout`），所以判据是「source 是 target 本身
或它的派生类」，沿 `baseName` 链往上找；反方向（把 `Base` 当 `Derived`）
会少字段，拒绝。带 64 层防御，非法继承链成环时不会死循环。

注意 `tests/test_17_class_inheritance` 就是继承用例，第一版判据写成
「不同类名一律拒绝」会误伤它 —— 这是本次改动唯一需要小心的地方。

### 实测（`tests/p13_2_regression.ps1` L3-J 6 条）

| 用例 | 修前 | 修后 |
|---|---|---|
| 未声明的出生地标签 `nosuchpool` | exit=0 静默丢弃 | 报 unknown label |
| 声明过的 Pool 作出生地 | 通过 | 通过 |
| 普通类型标签 | 通过 | 通过 |
| 无关类之间赋值 `Enemy = Player` | exit=0 | 报 not the same class |
| 派生类赋给基类 `Base = Derived` | 通过 | 通过 |
| 基类赋给派生类 `Derived = Base` | exit=0 | 报 not the same class |

## L3-K：顶层 `var` 的内存模型状态（2026-11-18 修复）

### 缺陷：顶层 var 从不进移动状态表

`emitVariableDefinition` 里有一段 `if (!isGlobalSlot) { ... }`，把
`registerMoveVar`、`registerBorrow` / `registerDropVar`、出生地校验、
`_birthPlaces` 记录**全部**包在里面。顶层 `var` 走全局槽
（`_inTopLevelStatements && _topLevelVarNames.count(name)`），于是整段被跳过：

```
var bob: Body[Player] pool = Player("bob")
var c: Content[Player] pool = bob.take()   // bob 实际已是 E，但表里没登记
sout(bob.name)                             // exit=0 —— 读空壳，静默通过
```

同一段代码放进 `fun` 里就会正确报 `'bob' is in Empty state`。
更糟的是 `fill` 方向也反了：`bob.fill(c)` 报
`L3: fill target 'bob' is not Empty` —— 说明状态表里**根本没有** `bob`
这一项，而不是记错了状态。

实测对照（`tests/_probe/fix/m1`..`m4`）：

| 写法 | 修前 | 修后 |
|---|---|---|
| `fun` 内 take 后读 | exit=1 报 Empty state | exit=1（不变） |
| 顶层 take 后读 | **exit=0 静默通过** | exit=1 报 Empty state |
| 顶层 take 后 `sout(bob)` | **exit=0** | exit=1 |
| 顶层 take 后 `bob.fill(c)` | **exit=1 报 not Empty**（反向错） | exit=0 |

### 修法

把移动状态登记 + 出生地记录移出 `!isGlobalSlot` 分支（全局槽只影响
`registerDropVar` / `registerBorrow` —— 那两项管的是栈帧清理，顶层 var
本来就不该进）。改后 `isGlobalSlot` 只包住这两项。

### 实测（`tests/p13_2_regression.ps1` L3-K 4 条）

全部通过，含「函数内同写法」与「顶层未 take 时读」两条对照。

## L3-L：生命周期标签 `static` 字段（2026-11-18 修复）

### 缺陷一：`static` 被当成类型名

`rcc_base.cpp` 的 `DESCRIBE_LABELS` 把标签分成四类，其中：

```cpp
// life circle label
"static", "global",
```

它们和类型标签一样是 `TOKEN_LABEL`、一样进标签区，但语义是
「这个字段/变量归谁的生命周期」。`validateL0FieldTypes` 修前把标签区
**每一个**标签都当类型解析，于是原仓库就有的写法：

```rio
class ID {
    var _number: int static = 0, num: int
```

报 `L0-2: unknown type 'static' of field '_number'` —— P0 重写把
static 字段的解析整个丢了。修法：加 `isLifecycleLabel()` 白名单，
`static` / `global` 跳过类型解析（与 `any` 同等对待）。

### 缺陷二：`static` 字段被当普通实例字段

`ClassInfo` 里原先的注释写着「static 字段整体不支持，跳过即可」，
但代码实际把它推进了 `ownFieldNames` / `fieldOffsets`，即**每个实例
各有一份**。原仓库 `test.rio` 的用法是靠它给实例发号：

```rio
ctor () { this._number = this._number + 1  this.num = this._number }
```

修前构造三个 `ID` 输出 `1 1 1`（各自从 0 开始），修后 `1 2 3`。

### 修法

- `ClassInfo` 新增 `staticFieldTypes` / `staticFieldSymbols` /
  `staticFieldInits`，static 字段**不进** `fieldOrder` / `fieldOffsets`。
- `emitStaticFieldSlots()` 为每个 static 字段发一块 `.bss` 全局槽，
  符号名 `<Class>.<field>.static`。
- `emitStaticFieldInitCode()` 在 `__rio_main` 最开头、任何用户代码之前
  执行一次初始化器（只认整数字面量，`.bss` 已零初始化，其余保守跳过）。
- `emitAttribute` / `emitAttributeAssign` 遇到 static 字段直接读写
  RIP 相对全局槽，**不**对左操作数求值（那是对象指针，与共享槽无关）。
- 查找沿基类链走，子类与基类共用同一份。

### 实测（`tests/p13_2_regression.ps1` L3-L 4 条）

| 用例 | 修前 | 修后 |
|---|---|---|
| `var n: int static = 0` | 报 unknown type 'static' | 通过 |
| `var n: int global = 0` | 报 unknown type 'global' | 通过 |
| 普通字段（对照） | 通过 | 通过 |
| 未知类型标签 `zzz`（对照） | 报错 | 报错（不变） |

运行时验证（`tests/_probe/fix/s8_static_shared.rio`，三个 `ID` 实例）：
修前 `1 1 1`，修后 `1 2 3`。

## 修正：`Mut` 的多重别名不是缺口（2026-11-18 复核）

本表原先列过一条「§6.4 独占未检查：`mut()` 之间、`Ref` 与 `Mut` 之间没有
重数检查」。复核决议原文后**这条判断是错的，已从缺口表删除**，依据如下：

- spec §3.2 的能力表里，只有 `Take<T>` 写了「独占，同时最多一个」；
  `Mut<T>` 的限制是「受局部排他约束（**第 4 节**）」—— 第 4 节讲的是
  子引用**路径前缀**冲突（`p.name` 被替换时 `&p.name` 失效），
  不是「同一时刻只能有一个 Mut」。
- 决议 §6.4「独占与挂起」全文只约束 `take`：同一语句二次 take、挂起期间
  禁 fill/clear/release、挂起期间不得传给 `Take[T]` 形参。没有一条要求
  两个 `mut()` 互斥。
- 附录 A 的对照表写「`Take` 独占 + 局部排他」，同样把独占归给 `Take`。

所以 `var m1 = p.mut() var m2 = p.mut()` 通过编译是**符合决议**的，
不是缺陷。把 `Mut` 改成 Rust 式 `&mut` 独占属于新增语言语义，
按硬规则 1b1ccafb 必须先向用户提出并获批，不能顺手塞进 P13-2。

已实现且验证过的是 §6.4 的第一条（take 与其余实参中的 Ref/Mut）：

```rio
fun f(t: Take[Player], m: Mut[Player]): int { ret 1 }
var p = Player("a")
var r = f(p, p.mut())   // 报 'p' is in Empty state —— take 已把它移走
```
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

## L3-M：两处回归修复（2026-11-18，提交 59359cf）

### 1. 泛型类字段的 `T` 被报成未知类型

`validateL0FieldTypes` 用 `_currentGenericParams` 解析字段标签，但那是
**实例化发射**阶段才被赋值的成员，而本校验发生在 `registerClass` 之前，
恒为空。于是：

```rio
class Box[T] { var v: T  ctor(val: T) { this.v = val } }
// 报 L0-2: unknown type 'T' of field 'v'
```

这是 L0-2 去伪校验（16c5cf6）引入的回归 —— `test_p10_generics` 的 6 条
泛型类用例全挂。修法：按**这个类自己**的形参列表（`class Box[T]` 的 `[...]`）
解析，而不是全局成员。

### 2. drop glue 发 `.globl` 导致与 rt.o 重复定义

`__rio_drop_<mangle>` 是编译器在每个翻译单元内部生成的**私有**辅助函数，
却发了 `.globl`；而 `release/lib/rt/rt.o` 会链接进**每一个**程序。只要用户
程序里出现 str / list / dict 变量（几乎必然），两边各定义一份同名全局符号：

```
ld: ...rt.o:(.text+0xc5e): multiple definition of `__rio_drop_K';
    ...user.o:(.text+0x36f): first defined here
```

最小复现：`var s: str = "hi"` 编不过。**早先没暴露**是因为 `rt.o` 是 P13-1
之前的陈旧产物（不含 drop glue）；重建 rt 层之后才浮出来，属既有缺陷。

修法：不发 `.globl`，各 TU 各持一份局部符号 —— drop glue 是纯函数，重复
存在没有语义代价。修后 `test_p13_memory` 由 12/28 回到 **28/28**，
`test_p12_g9g10_str` / `test_p12_g3g4_concat` 等全部恢复。

### 验证

| 套件 | 结果 |
|---|---|
| `tests/p13_2_regression.ps1` | 108 / 108 PASS |
| `tests/_probe/verify_fixes.ps1` | 43 / 43 PASS |
| `test_scripts/test_p13_memory.ps1` | 28 / 28 PASS |
| `test_scripts/test_p12_g9g10_str.ps1` | 16 / 16 PASS |
| `test_scripts/test_p10_generics.ps1` | 31 / 34（余 3 条见下） |
| 全量 `test_all.ps1` | 25 套件 / 19 通过 |

`test_p10_generics` 剩下的 3 条（`pos_identity_str` / `pos_pair_two_params` /
`pos_generic_default_str`）与 `test_p11_overload` 的 3 条是**同一个**既有
问题：`fun identity[T](x: T): T { ret x }` 被 P13-1 的 D9 严格借用规则判成
「'x' is a borrowed value and cannot be returned」。这是用户 2026-11-17 明确
裁决的口径（**严格报错、不造近似**），不是回归，也不是本次改动引入。

## L3-N：块表达式作右值（2026-11-18 实现）

### 缺口

决议 §12.1 把「裸块 `{ ... }`」列为**可返回值作用域**，§13.1 规定 `ret`
退**最近的可返回值作用域**。但修前 `emitBlock` 只有
`pushScope → emitStatement × N → popScope`，既没有值槽也没有上下文栈；
`emitReturn` 只在 `_loopStack` / `_ifStack` 里找最近的栈顶，于是块内 `ret`
一路落到「退函数」分支，被 void 返回类型挡下：

```rio
fun main(): void {
    var a: int = { ret 10 }
    sout(a)
}
// 报：Function '__rio_user_main' is declared to return void and cannot return a value
```

`if` 和循环都实现了这条语义，只有裸块漏了 —— 属实现缺口，不是设计待定。

### 关键设计判断：块表达式 ≠ 语句块

决议 §15.1 同时写明：「`ret` / `break` / `continue` 在裸块内**按外层上下文生效**」。
两条看似冲突，实际分工是：

| 位置 | 是不是可返回值作用域 | 块内 `ret` 退到哪 |
|---|---|---|
| 右值位置（`var a = { ... }`、实参、返回表达式） | **是**（§12.1「块表达式」） | 退这个块，带出值 |
| 语句位置（裸块单独成句） | **不是**（§15.1） | 退外层函数 / 循环 / if |

所以 `emitBlock` 新增 `asValue` 参数，只有 `emitExpression`（值位置）传
`true`，`emitStatement`（语句位置）保持 `false` —— 语句块路径与 L1 完全一致，
零回归风险。

### 实现

- `BlockContext { endLabel, valueSlot, annotatedType, dropDepth, ehDepth, enterSeq }`
  与 `IfContext` 同构；`_blockStack` 参与 `emitReturn` 的「取 enterSeq 最大者」
  比较，三者统一排序，不再用两两 `if` 判断。
- `emitRetFromBlock`：算值 → 存临时槽 → 只销毁块这一层（`dropDepth` 取
  **pushScope 之前**的深度，块自己的局部变量必须在 ret 出口销毁，因为 ret
  是终结性跳转、走不到词法出口）→ `emitEhUnwindTo` → 写值槽 → `jmp endLabel`。
- `emitBlock(asValue=true)`：按推断类型开值槽并**零初始化**（块内可能有
  不带值的提前出口，出口不能读到栈上垃圾）→ 末值落槽**在 popScope 之前**
  （popScope 的销毁调用会冲掉 `%rax`/`%xmm0`）→ `endLabel` 处读回寄存器。
- `inferType` 补上块表达式分支：优先取块内 `ret value` 的类型，没有就取末
  表达式类型。不补的话会一路落到 `int` 兜底，`var s: str = { ret "x" }`
  的值槽会被开成 int。

### 实测（`tests/p13_2_regression.ps1` L3-N 13 条）

| 写法 | 结果 |
|---|---|
| `var a: int = { ret 10 }` | 10 |
| `var a: int = { 10 }` | 10 |
| `var a: int = { var t = 3; t * 4 }` | 12 |
| `var s: str = { ret "hi" }` | hi |
| `var f: float = { ret 1.5 }` | 1.5 |
| `var a: int = { var b: int = { ret 3 }; ret b + 1 }` | 4 |
| `twice({ 21 })`（实参位置） | 42 |
| `var a: int = { if (1>0) { ret 5 }; 6 }` | 5 |
| 循环体内 `var v: int = { ret i }` | 累加正确 |
| `var a: int = { ret "str" }` | 报 `cannot assign`（类型不符） |
| `{ ret }`（语句块） | 仍退函数（§15.1 对照） |
| `{ sout(1) }; sout(2)`（语句块） | 1 后 2，不受影响 |

**回归**：`p13_2_regression` 108 → **121/121**（新增 13 条全过）；
`verify_fixes` 43/43；全量 `test_all.ps1` 25 套件 19 通过 / **556 通过
108 失败**（较修前 553/111 多 3 条通过，失败套件仍是同一批 6 个既有项）。

## L3-O：类方法的 `ret` 类型校验被整段跳过（2026-11-18 修复）

### 现象

同一个表达式 `"" + 1`，在类里静默通过、在顶层报错：

```rio
class Object {
    fun toStr(): str {
        ret "" + 1        // 编译通过，运行时输出空串
    }
}

fun toStr(): str {
    ret "" + 1            // 报 cannot assign a value of type int to a target of type str
}
```

### 根因：类方法调用点漏传了第 11 个实参

`emitFunctionBody` 有 11 个参数，最后一个是 `retTypeAnnotated`：

```cpp
void emitFunctionBody(..., bool hasThis = false, bool retTypeAnnotated = false);
```

它决定 `_functionReturnAnnotated`，而后者控制着 `emitReturn` 里的**两段逻辑**：

1. `ret` 的值类型是否匹配返回标注（不匹配就报错）；
2. `ret` 的值要不要插**返回值类型转换**。

对照各调用点：

| 调用点 | 实参数 | 状态 |
|---|---|---|
| 顶层函数 | 11（`false, retAnnotated`） | ✅ |
| 泛型实例 | 11（`false, true`） | ✅ |
| **类方法** | **10** | ❌ 漏传，`retTypeAnnotated` 用默认 `false` |
| 构造器 | 10（`"this", true`） | 构造器返回类型固定为类本身，可接受 |
| 匿名函数 | 8 | 无标注返回类型，可接受 |

于是 `_functionReturnAnnotated = false`，`emitReturn` 里
`if (ret && _functionReturnAnnotated && _functionReturnType)` 整块不执行。

### 影响不止类型校验

第 2 段逻辑被跳过更要紧。`fun one(): float { ret 1 }` 在类里会把 `%rax`
里的整数 `1` **直接当 double** 交给调用方 —— 修前实测打印出垃圾，修后打印
`1.0`。这与源码注释里记录的是同一类问题：

> `fun one(): float { ret 1 }` 之前直接把 %rax 里的 1 当 double 用，
> 调用方 sout 打印出垃圾 —— 必须在这里插转换。

### 修法

`methodReturnType` 原本把「声明标注」和「函数体推断」合并成一个返回值，
无法区分。加一个可选出参 `bool* outAnnotated` 回填标注状态，类方法调用点
按顶层函数同一判据传给 `emitFunctionBody`：

```cpp
bool retAnnotated = false;
const auto ret = methodReturnType(def, paramNames, paramTypes, "", &retAnnotated);
emitFunctionBody(methodSymbolName, paramNames, paramTypes, ret,
                 def->getBodyNode(), true, {}, {}, "", true, retAnnotated);
```

出参默认 `nullptr`，`inferType` 那个调用点（只需要类型）不用改。

### 实测（`tests/p13_2_regression.ps1` L3-O 5 条）

| 用例 | 修前 | 修后 |
|---|---|---|
| 类方法 `fun toStr(): str { ret "" + 1 }` | exit=0（静默，输出空串） | exit=1，报 `cannot assign` |
| 类方法 `fun one(): float { ret 1 }` | 输出垃圾值 | 输出 `1.0` |
| 类方法 `fun f(): void { ret 1 }` | exit=0 | exit=1，报 `declared to return void` |
| 类方法正常返回（对照） | 通过 | 通过 |
| 顶层同名写法（对照） | 报错 | 报错（两边终于一致） |

**回归**：`p13_2_regression` 121 → **126/126**；`verify_fixes` 43/43；
全量 `test_all.ps1` **556 通过 / 108 失败**，与修前持平，无回归。

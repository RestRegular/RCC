# Rio 内存模型 · P13-2 实施清单对齐决议（讨论稿）

> 本文记录 2026-11-18 针对 `p13_2_design_doc_checklist.md` 中 #1–#5 的语法/语义层决议。
> 与 `memory-model-syntax-resolutions.md` 同族，作为其补充。
> 已确认项直接实施；暂定/推迟项注明去向。

---

## 目录

1. Pool 类型与出生地（checklist #1）
2. fill / clear / release（checklist #2）
3. take 挂起态与独占检查（checklist #3）
4. 子引用 / NLL / 局部排他（checklist #4）
5. 签名标注真实校验（checklist #5）
6. 暂缓项（checklist #6 / #7）

---

## 1. Pool 类型与出生地（checklist #1，已确认）

### 1.1 Pool 是标准库普通类型

- Pool 是 Rio 标准库普通类型，**不做关键字**，不内建。
- 三种分配策略走静态方法：
  ```rio
  var p1: Pool = Pool.fixed(100)
  var p2: Pool = Pool.boxed()
  var p3: Pool = Pool.chunked(block: 64)
  ```
- `fixed(100)` 位置参数；`chunked(block: 64)` 命名参数；`boxed()` 无参。均返回 `Pool`。

### 1.2 fixed 满的失败语义

- `fixed` 满时**运行时 abort**，不运行销毁，pool 内已登记对象泄漏（已知代价，文档标注）。
- **不引入 `T?` 返回**；`T?` 机制整段留到 P13-2。
- `pool.new[T](...)` 返回类型为 `T`（非可空），满时 abort。
- 可选静态检查（容量字面量 + 登记点可枚举时编译期检查总数 ≤ 容量）第一版**可选、可砍**，只留运行时 abort。
- “设置准确/富余容量”是编程建议，不强制。

### 1.3 登记方式：砍掉 pool.add

- **砍掉 `pool.add(x)`**。理由：`x` 在调用前必须先有出生地，`add` 等于“先出生再搬家”，违反 §2.3“Body 出生后地址永不改变”。
- 登记只靠两条路：
    1. **声明式标签**：`var a: Player pool = Player("b")`
    2. **`pool.new[T](...)`**：`var a: Player = pool.new[Player]("b")`

### 1.4 pool.new 的语义与绑定

- `pool.new[T](args)` = 用 T 的构造器在 pool 里构造 Body；`[T]` 是构造器选择，必须与构造返回类型一致。
- **允许 var 绑定 `pool.new` 结果**：`var a: Player = pool.new[Player]("b")` 合法，a 出生在 pool。
- **出生地只能指定一次**：标签区写了出生地 + 表达式用 `pool.new` 且不一致 → 编译错误，要求二选一；都没写则默认当前块。
- 不提供 `pool.new(Content)` 形态（装已有 Content 用标签区或 `Body(x)`）。
- **退路**：若 `pool.new[T](args)` 实现困难或无法实现，可砍去；砍去后“非 var 位置构造”退回“先标签区声明再使用”或“临时 Body + 语句级匿名 local”。

### 1.5 临时 Body 的出生地（修正 §5.3）

- **允许临时 Body 有语句级匿名 local 出生地**，用于“构造后立即使用、语句末销毁”。
- 修正 §5.3 “Body 无匿名 local” → “Body 无**命名**匿名 local，但有**语句级临时** local”。
- 例：`foo(Player("b"))` 里临时 Body 出生在当前语句匿名 local，语句末销毁。

### 1.6 Pool 双角色与 AST

- 作为容器：靠声明式标签 + `pool.new[T](...)`（`pool.add` 已砍）。
- 作为被管理对象：`var p2: Pool pool1 = Pool.fixed(...)`。
- 出生地标签 AST 表示（labelNodes 内加字段 vs 独立字段）是**实现细节**，划给实施侧；判定顺序“内置→Pool→自定义→出生地”已定。

---

## 2. fill / clear / release（checklist #2，已确认）

### 2.1 调用形态

- **方法形态**：
  ```rio
  p.fill(c)
  p.clear()
  pool.release(x)
  ```
- `release` 的 `x` 是**槽位引用**（`&bob` 或 Body 变量）。
- 与 `bob.take()` / `bob.ref()` / `bob.mut()` 对称。

### 2.2 四态状态是编译期事实表

- E/F/H/X 四态是**编译期事实表**，不是运行时元数据。
- 当前 `_moveStates` 的 bool（F/X 两态）是临时实现，P13-1 改为事实表。
- **不是 ABI 变更**，是编译器内部表示变更。

### 2.3 release 的池绑定

- **默认出生地（当前块）的 Body 不能 `release`**：槽位由块结束自动回收；语法上无法表示“默认出生地”作为 `release` 目标，天然挡住。
- **`release` 只对自定义 Pool 出生地的 Body 可用**，且 `x` 的出生地必须是**该 pool**。
- **跨池 `pool2.release(x)`（x 出生在 pool1）编译错误**。
- **`clear()` 与出生地无关**：任何 Body 都能 `clear`，销毁内容不回收壳。
- 程序员不得提前销毁默认出生地的槽位；`clear` / `fill` / `take` 仍可用（它们不回收壳）。

### 2.4 状态冲突检查

- 依赖 #4 子引用路径登记，交叉依赖，语义层已对齐（状态与子引用在同一事实表）。

---

## 3. take 挂起态与独占检查（checklist #3，已确认）

### 3.1 挂起态表示

- 挂起态 = 事实表的 **`pending_take` 集合**（语句作用域临时事实）。
- 语句末清空：已落定的转 E，未落定的回填 F。
- **不是事务、不是运行时元数据**。

### 3.2 “同一语句”的定义

- “同一语句” = **同一个 AST 语句节点**；分号 / 换行分隔算不同语句。

### 3.3 独立 take 语句的警告

- `bob.take()` 作为独立语句（既不落定也不使用）→ **编译警告，不阻断编译**。
- 走现有诊断机制（file:line:col + message），无新机制。

### 3.4 异常路径

- **`throw` 时挂起态不回填**，body 状态变 `?`，与“异常即泄漏”（§9.1）一致。

---

## 4. 子引用 / NLL / 局部排他（checklist #4，已确认）

### 4.1 路径穿透引用展开

- 子引用路径**穿透引用展开**：引用按其来源（`from`）展开。
- 例：`&q.x` 得到的引用 `p`，`p.name` 路径为 `q.x.name`，精确登记。
- 跨函数时 `from` 必须写全；缺省 `from` 或无法追踪时退 `⊤`（保守）。

### 4.2 NLL 边界

- **过程内 NLL**（活跃到最后使用）+ **跨函数用签名标注保守**。
- 调用时按 `writes` / `from` 更新活跃集合；无标注则类型级保守（§4.3）。

### 4.3 split / get_mut / 不等事实

- `split_at(k)` / `split_from(k)` / `get_mut(i)` 返回时登记路径。
- `get_mut(i)` 带 `xs.items[*]` + 下标 i；`split_at` / `split_from` 带区间
  `xs.items[0..k)` / `xs.items[k..len)`，两段可共存。
- 不等事实 `i≠j` 作为事实表一维，消费时放宽冲突。

### 4.4 实现细节

- 路径的 AST / 事实表编码（`p.name`、`p.team.captain`、`xs[i]`、`s[0..k)` 统一编码）是**实现细节**，划给实施侧。

---

## 5. 签名标注真实校验（checklist #5，已确认）

### 5.1 ensures

- `+a` / `?a` / `!a` 标记的事实表表示与逐出口验证是**实现细节**，语义已定。
- 每条正常出口必须满足 `ensures`；`throw` 出口不检查。

### 5.2 from

- “返回引用不得指向本函数 local”检查依赖 #4 路径登记，已闭环。

### 5.3 writes 在泛型下的合流

- **泛型定义处 `writes[p]` 表示整个参数**（上界）。
- 实例化时以实例推断为准；定义处 `writes[p]` 是上界。
- **泛型定义处不写实例级 `writes[p.field]`**（T 的字段未知）。

### 5.4 异常出口交互

- `throw` 不检查 `ensures`、`catch` 后回 `?` —— 暂不展开，归 Q4 条件销毁。

---

## 6. 暂缓项（checklist #6 / #7）

### 6.1 Maybe / if-let / match 窄化（#6）

- 整段推迟 P13-2，等待用户设计文档。
- 已知约束：只有引用类型可空（裁决 388a73e0）；`get()` 倾向返回 `T?`，未最终确认。
- 本条不展开。

### 6.2 值语义 struct（#7）

- 等待用户提供 struct 声明语法设计。
- 待定点：声明语法、与 class 的区别（值 vs 引用）、字段布局、赋值语义、与 Body/Content 状态格的关系。
- 本条不展开。

---

## 7. 与现有决议文档的关系

- 本文是 `memory-model-syntax-resolutions.md` 的补充，不重复其已确认条目。
- 冲突时以本文 + `memory-model-syntax-resolutions.md` 为准，`memory-model-spec-full.md` 待同步修订。
- #1–#5 语法/语义层闭环；AST 编码、静态检查实现、事实表落地属实施侧。
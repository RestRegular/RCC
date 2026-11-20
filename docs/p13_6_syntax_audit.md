# P13-6 前置审计：Pool 与成员集合追踪（规范 §6）

> **本文性质**：审计 + 裁决记录，**不是实施方案，未改任何 codegen**。
> 遵守 RCC 硬规则（`1b1ccafb`）：任何语言设计 / 语义变更必须先向用户提出并获批。
> 审计方式：两个子代理分头调研（Pool 实现、成员集合/release）+ 实机探针。
> **裁决状态（2026-11-19）：Q1–Q4 已全部逐条裁决并实施完成：Q1=A（三档真实分配器）、Q2=A（成员集合最小集）、Q3=A（不放宽状态规则）、Q4=A（dict.clone 本阶段不做）。** 实施结果见 §6 与 `tests/agent/p13-6-pool`（7/7）。
>
> 审计基线：HEAD 基线 P13-4/P13-5 刚完成（未提交），探针 `build\p13_6_probe\*.rio`。
> 前置依据：`docs/p13_plan.md` §P13-6、`docs/memory-model-spec-full.md` §6、
> `docs/p13_2_syntax_resolutions.md` §1/§2、`docs/pending_designs.md` §7。

---

## 0. 结论摘要：plan 文档的「基本未实现」已过时

`p13_plan.md` 写「P13-6 只有 `Pool` 类型名识别、成员集合零命中」。实测更精确：

| 规范 §6 项 | 实测状态 | 判定 |
|---|---|---|
| Pool 作为真实类型 | `TK_POOL` / `PoolType` / `getPoolType()` / mangle `Q` 全在 | ✅ 已实现（文档过时） |
| 出生地标签区（`var a: Player pl = ...`）→ `_birthPlaces` | 登记 + 校验 + 最多一个出生地 | ✅ 已实现（P13-2） |
| `pool.new[T](...)` | 静态检查完整，复用普通构造器 | 🟡 部分（无池内分配） |
| `Pool.fixed/boxed/chunked` | 最小 8 字节句柄 stub，丢弃容量参数 | 🟡 部分（无真实分配器） |
| `pool.release(x)` | 9 步编译期检查（活跃引用/E 态/同池/pending/? 态） | 🟡 部分（成员集合重数没查） |
| `core_pool_*` 运行时例程 | 零命中；`core_win64.s` 无 pool 例程 | ❌ 未实现 |
| 成员集合追踪（重数 `1`/`many`、`*@L`、`⊤`、原语效果） | `cardinality`/`membership`/`重数` code 里零命中 | ❌ 未实现 |
| §6.4 的 Link 条款 | `Link` 类型不存在（P13-7 零代码） | ❌ 依赖 P13-7 |

**一句话**：Pool 的**语法和编译期检查**基本齐了，**运行时分配器**和**成员集合追踪**两大块是真空。
而「成员集合」才是 P13-6 的技术核心（也是 Q4 从 P13-4 移过来的那块）。

---

## 1. 逐项实测

### 1.1 Pool 真实类型（已实现，文档过时）

- `code/include/sema/rcc_sema_types.h:37` `TK_POOL`；`:369-383` `PoolType` + `getPoolType()`；`:63-69` 归入引用型家族。
- `code/src/backend/rcc_symbol_naming.cpp:86-87` mangle `Q`。
- `code/src/backend/rcc_codegen.cpp:104-106` `if (name == "Pool") return getPoolType()`（这是"名字→真实类型"入口，不是"只有字符串识别"）。
- `docs/p13_2_syntax_resolutions.md:100-107`：Pool 是标准库普通类型、不做关键字、**不带类型参数**（通用 Pool 管多种对象）。

### 1.2 构造器 / new / release（部分实现）

| 原语 | 位置 | 现状 |
|---|---|---|
| `Pool.fixed(n)` / `boxed()` / `chunked(block:n)` | `rcc_codegen.cpp:9398-9429` | 求值实参后丢弃，只 `core_alloc(8,8)` 返回 8 字节句柄；注释自述"A 阶段先返回最小句柄，真实分配器留待 B 阶段" |
| `pool.new[T](...)` | `rcc_codegen.cpp:9430-9474` | 检查 receiver 是 Pool、T 是 class、结果类型一致；实际调普通 `emitConstructorCall`，池内分配未做 |
| `pool.release(x)` | `rcc_codegen.cpp:12803-12865` | 9 步检查：receiver 是 Pool / 1 参 / x 是 Body / 非 pending / 无活跃子引用 / 有 Pool 出生地 / 同池 / 非 `?` 态 / 是 Empty 态；成功仅擦出生地 + 置 MOVED，壳回收靠 scope drop |
| 裸名 `release(x)` 拦截 | `rcc_codegen.cpp:9519-9542` | 用户裁决 A：编译期报"不是函数，请写方法形态"，用户自定义同名函数不误伤 |

**release 的 9 步里没有一步查"成员集合重数=0"**（规范 §6.4 的第一要素）。

### 1.3 成员集合追踪（零实现）

- grep `cardinality` / `membership` / `重数` / `*@L`：`code/` 零命中；唯一"成员集合"是 `rcc_codegen.cpp:957-970` 的"已知泄漏"注释。
- P13-4 的 `_activeRefs`（`rcc_codegen.h:499-510`）表达的是**子引用路径**（`p.name`、`xs.items[*]`、`xs.items[lo..hi)`），`hasActiveRefOn(owner)` 只按 owner 名匹配，**不能表达**成员槽位的 `1`/`many`/`⊤` 重数，也没有 `snapshot`/`extend` 效果。
- 规范 §6.3 要求的完整追踪（原语效果表、嵌套展平、闭包捕获、函数边界推断）均为文档层要求，无代码。

### 1.4 实机探针：容器存引用的真实行为

| 探针 | 写法 | 结果 | 说明 |
|---|---|---|---|
| `a_list_ref` | `var team = [&a]` | ✅ 编译通过 | `list[Ref[T]]` 能构造 |
| `b_list_body` | `var team = [a]`（a 是 Pool 出生的 Body） | ✅ 编译通过 | 能存，但见下 |
| `c_release_live_member` | 存 a 后 `a.clear(); pool.release(a)` | ❌ `not Empty` | 存进容器后 a 不再是 Empty |
| `e_release_no_sout` | 存 a 后 clear + release | ❌ `not Empty` | 同上，与 sout 无关 |
| `f_clear_then_store` | 先 `a.clear()` 再 `[a]` | ❌ `'a' is in Empty state and cannot be used directly` | Empty 态变量不能直接存 |

**关键语义事实**：当前 move 规则下，把 Body 存进容器会使其离开 Empty 态；规范 §11.1 示例里
`team.clear()` 后再 `pool.release(bob)` 的那条路径，在当前编译器里走不通（bob 进 team 后无法回到可 release 的 Empty 态）。
这意味着"成员集合判据"的实战入口比规范假设的窄，实施前必须裁决其触发范围。

---

## 2. 与已有裁决 / 语义的冲突点

1. **Pool 三档分配器是不是语言设计新增？** 规范 §6.1 给了三档语义，P13-2 裁决"Pool 是标准库普通类型、不做关键字"，但三档构造器和 `pool.new[T]` 的**真实运行时分配**从未实现。按硬规则，新增/重新设计标准库公开 API 面须先裁决；这里 API 名字已在规范中冻结，但**实现档位与默认档**需确认。
2. **成员集合追踪的完整度**：规范 §6.3 要求重数格 + 一整套原语效果 + 函数边界推断，接近一个完整的数据流分析。是否一次性做全，还是先做能支撑 §6.4 release 判据的最小子集（"哪些活跃容器含 x"），需要裁决。
3. **release 成员判据的触发范围**：探针显示"存进容器后 release"当前被状态规则挡住。若成员集合判据只对"能走到 release 的路径"生效，实际可能**零触发**（没有可观测行为）。要裁决是维护完整机械判据（哪怕当前路径少），还是先补状态规则让规范示例跑通。
4. **Q4 归属的连带项**：`dict.clone()` / 容器元素递归释放已由 P13-4/Q4 归到 P13-6。它们同样依赖"元素归不归容器所有"的成员集合信息，是否与 P13-6 一起做，需明确。
5. **Link 条款**：§6.4 要求"x 含 Link 则 Link 状态为空且 x 不在任何 Link 中"，Link 是 P13-7 的内容（零代码）。release 的这一条必须明确推迟到 P13-7。

---

## 3. 待裁决问题

> 以下问题互不依赖，可逐条答复；每条给出推荐选项。答复后我才据此写实施方案并动 codegen / 运行时。

### Q1 — Pool 三档分配器的实现范围

- **A（推荐）**：本阶段实现 `Pool.fixed` / `Pool.boxed` / `Pool.chunked` 的**真实运行时分配器**（core 层 `core_pool_*` + codegen 接通），默认 `chunked`；`pool.new[T]` 在池内分配槽位，槽地址稳定（符合 §6.1）。`Pool` 仍不带类型参数、仍是普通类型，API 名沿用规范冻结名，不算新增设计，只是把 stub 做实。
- **B**：只做最小可观察集——`Pool.fixed(n)` 一档真实分配（一整块 + 位图），`boxed`/`chunked` 保持 stub 行为但按 fixed 实现（语义等价，参数忽略）。省一半运行时工作，三档对外行为暂不分化。
- **C**：Pool 分配器本阶段全部保持 stub，P13-6 只做成员集合追踪（Q2）。理由：当前测试与 std 都不依赖真实池内分配，成员集合才是核心。

**裁决（2026-11-19）：A —— 三档真实分配器，默认 chunked，API 名沿用规范冻结名。**

### Q2 — 成员集合追踪的完整度

- **A（推荐）**：做**能支撑 release 判据的最小集合追踪**：按路径索引、"x 是否在活跃容器成员集合中"的判定 + 重数 `1`/`many`/`⊤` 饱和 + `push`/`remove_all`/`clear`/`snapshot` 四种原语效果；`*@L`/循环动态加入/嵌套展平/闭包捕获登记为后续项。
- **B**：完整实现 §6.3 全部规则（原语效果表 9 项 + 嵌套传递闭包 + 函数边界推断）。工作量大，且多数规则当前无消费者（无 for-in、无 Link）。
- **C**：完全不做成员集合，release 只保留现有 9 步检查；把 §6.4 的成员条款登记为已知缺口。

**裁决（2026-11-19）：A —— 最小集，服务 release 判据：路径索引 + 成员重数 1/many/⊤ 饱和 + push/remove_all/clear/snapshot 四原语效果；其余登记为后续项。**

### Q3 — release 成员判据的触发范围（探针发现的语义分叉）

探针证实：当前"把 Body 存进容器 → clear → release"路径被状态规则挡住（`not Empty` / `Empty state cannot be used`），规范 §11.1 的 `team.clear(); pool.release(bob)` 走不通。

- **A（推荐）**：本阶段**不放宽状态规则**，成员集合判据作为"防御性机械检查"接进 release（路径若可达则拦截），不为了让规范示例跑通而改 move/状态语义；规范 §11.1 示例的通路登记为后续项（涉及状态语义变更，单独提案）。
- **B**：顺带放宽状态规则，让"存进容器后 clear 再 release"跑通，使 §11.1 示例成立，成员集合判据有实际触发场景。属语义变更，需在本条一并裁决授权。

**裁决（2026-11-19）：A —— 不放宽状态规则；成员判据作防御性机械检查接进 release，§11.1 通路登记为后续项。**

### Q4 — `dict.clone()` / 容器递归释放的连带处理

P13-4/Q4 已把这两项归到 P13-6，它们同样依赖成员集合信息。

- **A（推荐）**：本阶段不做。成员集合最小实现（Q2=A）只服务 release 判据；dict 深拷贝/递归释放留待成员集合完整化（Q2=B 的范围）后单独阶段。
- **B**：本阶段顺带做 `dict.clone()` 深拷贝 + 容器递归释放。需要 Q2=B 的完整成员集合打底，工作量显著增加。

**裁决（2026-11-19）：A —— dict.clone() / 容器递归释放本阶段不做，留待成员集合完整化后单独阶段。**

---

## 4. 裁决后实施范围（P13-6）

| 项 | 裁决 | 实施动作 |
|---|---|---|
| Q1 Pool 分配器 | A | core 层新增 `core_pool_*`（fixed 整块 / boxed 逐个 / chunked 分块，各带 2 位/槽侧位图），codegen 把三档构造器从 stub 接成真实分配；`pool.new[T]` 槽地址稳定；默认 chunked；API 名沿用规范，Pool 仍不带类型参数 |
| Q2 成员集合 | A | 新增按路径索引的成员集合表（重数 1/many/⊤ 饱和）+ push/remove_all/clear/snapshot 四原语效果；release 判据接入"成员重数=0" |
| Q3 release 触发 | A | 不放宽状态规则；成员判据作防御性机械检查；§11.1 通路登记后续项 |
| Q4 dict.clone | A | 本阶段不做，留待成员集合完整化后 |

**注意（Q1+A 的语义边界）**：Pool 不带类型参数、仍为标准库普通类型、API 名已在规范冻结——实现的是既有设计的**运行时部分**，非新增语言设计；不触碰 parser/关键字。

---

## 5. 后续待设计项（本阶段登记，不实施）

- Link 相关的 release 条款（"x 不在任何 Link 中"）——依赖 P13-7。
- 规范 §11.1 示例的 release 通路（Q3 裁决引出，涉及状态语义变更，单独提案）。
- 三档 Pool 的并发 `chunked(concurrent:true)`（规范 §10.5）——依赖 P13-8。
- ~~snapshot() 成员登记~~ ✅ 已完成（2026-11-20，见 §6）。
- ~~成员集合活跃性（§6.3 死变量不计入）~~ ✅ 已完成（2026-11-20）：`MemberSetEntry` 加 `scopeDepth`，容器出作用域后集合不再计入 release 判据；用例 `pos_q2_dead_scope`。
- ~~循环动态加入（§6.3 循环中加入 → many/*@L）~~ ✅ 已完成（2026-11-20）：`memberAdd` 内 `_loopStack` 非空时按 MANY 饱和（单遍 codegen 只扫到一次 push，但每轮都执行）；用例 `neg_q2_loop_many`（诊断 mult=many）。
- ~~嵌套展平（§6.3 容器的容器 = 内层集合的并）~~ ✅ 已完成（2026-11-20）：`memberBlocksRelease` 改为沿「成员→持有它的容器→更外层容器」做传递闭包（`memberReachesLive` + visited 防环），内层容器出作用域后仍可经活跃外层容器传递持有；用例 `neg_q2_nested_flatten`（诊断指 groups）。
- ~~`dict.clone()`（P13-6 Q4）~~ ✅ 已完成（2026-11-20）：新增 `core_clone_dict`（复制 keys/vals 两个 8 字节槽数组 + 新 header，D14 浅拷贝口径，与 core_clone_list 对称），codegen 方法/自由函数两形态 + coreReturnType 三点登记；用例 `L3-dict-clone`（改克隆体不影响源：1/99/2）。
- 成员集合剩余完整化（Q2 的 B 范围：*@L 精确槽位 / 闭包捕获 / 函数边界推断）与容器元素递归释放（Q4 剩余，涉及所有权语义，与浅拷贝冲突需另裁）。

---

## 6. 实施与验收（2026-11-19）

**落地实现**（与方案的偏差在此说明）：

- **Q1 三档分配器**：`core_win64.s` 新增 `core_pool_new` / `core_pool_alloc` /
  `core_pool_release` 三个例程；三档统一用块链 + 48 字节固定池头，槽位**惰性分配**
  （Pool 无类型参数，`elem_size` 只有首个 `pool.new[T]` 时才知道——这是相对方案 §2.2
  的实现简化，对外语义不变：容量固定、地址稳定）。`core_return_type` 登记返回类型，
  `core.rio` 加 extern；三档构造器接 `core_pool_new`，`emitConstructorCall` 加
  `poolVarName` 参数让 `pool.new[T]` 在池槽分配；fixed 满时**运行时 panic**
  （方案里写的编译期 fail 不可行——满否是运行时状态），退出码 3 + 清晰诊断。
- **Q2 成员集合**：`CodeGen` 新增 `MemberMult` 格（ZERO/ONE/MANY/TOP）与
  `_memberSets` 表；在 `checkGrowthEffects` 登记 `listAppend`/`listInsert`/`listPush`
  （+1 饱和）、`listRemove`（归零）、`listClear`（清空）；`release` 加第 10 步查
  "成员重数=0"，诊断带容器名与重数（符合 §11.4）。
  - **`snapshot()` 效果（clone）已登记（2026-11-20 完整化）**：在 `emitVariableDefinition`
    的绑定点（`isBorrowInit` 判定之后）回填——识别 `x.clone()` / `clone(x)` / 同类型容器
    赋值三种形态，调 `memberSnapshot(src, dst)` 把源集合拷到新变量。此前"调用点拿不到目标
    变量名"的障碍按 `bindLastActiveRef` 同样的绑定点回填思路解决。验证：源容器 clear 后
    快照仍拦 release（报 `container 'team2'`）、空容器快照不误伤；用例
    `neg_q2_snapshot_release` / `pos_q2_snapshot_empty`，套件 9/9。
  - `remove_all` 精确语义用 `listRemove` 近似归零（当前 std 无 `remove_all` API）。

**验收**：

- `tests/agent/p13-6-pool`：7/7（Q1 三档 incl. fixed-full panic、Q2 release 拦截/many 饱和/正向）。
- 全量 14 套回归全绿：p13-2-regression 153、p13-verify-fixes 43、p13-3-nullable 20、
  p13-3-capability 19、p13-4-subref 9、p13-memory 28、p10-generics 34、p11-overload 24、
  p12-3c-ds 44、std-full 220、p12-std-skeleton 31、p5-containers 15、p6-classes 15、
  p2-functions 13。
- 工作区 unrelated 未提交改动未触碰；本轮改 `core_win64.s` / `core.rio` / codegen 两文件 /
  两文档（本审计 + 方案）/ `p13_plan.md` + 新增 `p13-6-pool` 套件；未 commit/push。

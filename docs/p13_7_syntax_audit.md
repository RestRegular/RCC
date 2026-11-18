# P13-7 前置审计：`Link` 成对引用（规范 §7）

> **本文性质**：审计 + 裁决记录，**不是实施方案，未改任何 codegen**。
> 遵守 RCC 硬规则（`1b1ccafb`）：任何语言设计 / 语义变更必须先向用户提出并获批。
> 审计方式：两个子代理分头调研（规范/测试惯例、类型系统/落地路径）+ 实机探针。
> **裁决状态（2026-11-19）：待逐条裁决（见 §3）**。
>
> 审计基线：HEAD `5c107e2`（P13-3 已提交），工作区有 P13-4/5/6 未提交改动（本轮不触碰、不混入提交）。
> 探针：`build\p13_7_probe\*.rio`，编译器 `release\RCC.exe`。
> 前置依据：`docs/p13_plan.md` §P13-7、`docs/memory-model-spec-full.md` §7、
> `docs/memory-model-syntax-resolutions.md`、`docs/pending_designs.md` §6、
> 记忆 `3edc03d7`（P13 真实进度盘点）、`7520a182`（基数语义裁决 A）。

---

## 0. 结论摘要：规范设计齐备，代码零实现

| 规范 §7 项 | 实测状态 | 判定 |
|---|---|---|
| `Link` 词法 token | `TOKEN_LINK` 在 `rcc_core.cpp:52` / `rcc_core.h:108`；小写 `link` 是关键字，大写 `Link` 是普通标识符 | ✅ 已有 |
| 类型系统表示 | `TypeKind`（`rcc_sema_types.h:21-40`）无 `TK_LINK`；无 `LinkType` | ❌ 零实现 |
| 声明语法解析 | parser 已支持多段 `[...]`（`func[A][B]` 先例）、`dict[K,V]` 单括号多参；但 `[标识符]` 会被降级为 UNKNOWN Type，字符串身份丢失 | 🟡 底层够用、缺 Link 专用分支 |
| 五个原语 `add/remove/clear/set/unlink_all` | 全仓零命中 | ❌ 零实现 |
| 对偶一致性 / 基数校验 | 零实现 | ❌ 零实现 |
| `links_nonempty` 槽状态位（§6.5 规划） | 池头 48 字节只有 6 字段，全库无 bitmap 代码 | ❌ 未实现（P13-6 也未做） |
| release 的 Link 条款 | `rcc_codegen.cpp:13005` 注释自述「Link 条款依赖 P13-7，本阶段不查」 | ❌ 明确推迟到本阶段 |
| take 的 Link 条款 | `x.take()` 检查块（`:13067-13097`）无 Link 判据 | ❌ 未实现 |
| §11.2 含环图示例 | 依赖 `add` 原语 | ❌ 依赖本阶段 |

**一句话**：`Link` 是 P13 里唯一**零代码**阶段；parser/AST 的多段方括号基础设施（`LabelNode::labelDesNodes`）已经现成，缺口集中在 sema 新类型 + 原语发射 + 不变量状态，以及一大批规范细节需要先裁决。

### 实机探针（一手证据）

| 探针 | 写法 | 结果 |
|---|---|---|
| `p1b_local_link` | `var l: Link[int][int][one] = null` | `Unknown type 'Link' in declaration of 'l'` |
| `p2b_field_link` | `var f: Link[int][g][many]`（类字段） | `L0-2: unknown type 'Link' of field 'f'` |
| `p3_link_stmt` | `link a`（小写关键字作语句） | parser 报「找不到 TOKEN_LINK 的前缀构造函数」 |
| `p4_comma_link` | `Link[User, fans, many]`（逗号单括号） | `unknown type 'Link' of field` |

---

## 1. 逐项审计（规范 vs 现状）

### 1.1 声明语法：两种写法并存，必须定一种（风险点 R1）

- 规范 §7.1（`memory-model-spec-full.md:330-340`）与 `pending_designs.md:269` 写**逗号单括号**：`Link<User, fans, many>`。
- `p13_plan.md:241` 与阶段任务书写**三段方括号**：`Link[int][int][one]`。
- 语法决议已确认统一方括号（`memory-model-syntax-resolutions.md:33/407`）：`Link[T][peer][cardinality]`，且 `set` 仅 `opt`/`one`（`:387`）。
- parser 两种都能吃下：单括号多参走 `flattenTypeArgs()`（`rcc_codegen.cpp:10288-10315`，`dict[K,V]` 先例），多段方括号走 `LabelNode::labelDesNodes`（`func[A][B]` 先例，`rcc_prefix_builder.cpp:436-462`）。
- **关键风险**：`[fans]` / `[one]` 里的标识符经 `typeFromTypeArgNode()`（`rcc_codegen.cpp:10433-10449`）会被 `resolveTypeName` 查成 UNKNOWN Type，**字符串身份丢失**。必须直接从 `label->getLabelDesNodes()[i]->getBodyNode()` 取 `IdentifierNode::getName()` 原样保存。

### 1.2 类型表示：需新增 `LinkType`（风险点 R2）

- `TypeKind`（`rcc_sema_types.h:21-40`）有 `TK_MEMORY`/`TK_POOL`，无 `TK_LINK`。
- `MemoryType`（`:400-435`）只有 `(role, inner)` 两个字段，装不下 `dual` + `cardinality` 两个字符串参数。
- 参照 `PoolType`（`:369-383`）：新建 `LinkType(TypePtr elem, string peer, Cardinality)` 子类最干净；`isPointerLike()`（`:63-69`）、mangle `switch`（`rcc_symbol_naming.cpp`）、`parseTypeName`（`:77-153`）都要登记，否则静默 int 兜底。

### 1.3 原语与运行时：照搬 Pool 的「三点登记法」（风险点 R3）

- 五个原语全仓零命中；`core_win64.s` 无 Link 例程。
- P13-6 的 `core_pool_*` 已建立可复用链路（`lib/core/core_win64.s:1000-1116` + `core.rio:136-139` extern + `coreReturnType()` 登记，见 `p13_6_implementation_plan.md:26`）。
- 但 `links_nonempty` 状态位（§6.5）**未实现**——池头只有 mode/cap/elem_size/used/current/total 六字段。Link 状态是新工作。

### 1.4 不变量与状态追踪：可借用成员集合表，但需独立扩展（风险点 R4）

- P13-6 新增 `MemberMult`（ZERO/ONE/MANY/TOP 饱和格）与 `_memberSets` 表（`rcc_codegen.h:511-530`，实现 `:1784-1862`），可作「Link 是否为空」雏形。
- 但缺三类信息：对偶字段名、Full 状态、实例级 vs 类型级效果区分（§7.5）；且该表是函数局部事实（`rcc_codegen.cpp:14396` 每函数清空），Link 不变量需跨函数存活。建议独立 `LinkSet` 表。

### 1.5 对 take/release 的影响：插入点明确（低风险）

- `release`（`rcc_codegen.cpp:12944-13022`）：第 10 步成员判据后（`:13018`）、擦出生地前插入「不在任何 Link 中」检查；注释 `:13005` 已预留。
- `take`（`:13052-13105`）：状态检查块后（`:13097`）、`beginPendingTake` 前插入「Link 必空」检查。
- 已有 `checkTakeArgOverlap`（`:9390-9437`）与 `WriteEffect` 表（`rcc_codegen.h:539`）可作 §7.5 迭代冲突判据的起点。

---

## 2. 已裁决 / 已确认事项（不再提问，直接实现）

| 项 | 来源 | 结论 |
|---|---|---|
| 基数语义 | `pending_designs.md:269` / 记忆 `7520a182`（2026-11-18 裁决 A） | `opt` 至多一个、`one` **连接后**恰好一个、`many` 任意多个；`one` 不要求构造即连接，首次 `set` 前为空（`a.spouse.get()` 返回 `T?`）；`one` 与 `opt` 差别在连接后的对称强制（自动建反向边、不可单方面断开，只能 `set` 新对端或成对 `unlink_all()`） |
| 声明语法形态 | `memory-model-syntax-resolutions.md:33/407` | 方括号 `Link[T][peer][cardinality]`（规范 §7.1 的尖括号写法视为过时） |
| `set` 适用范围 | `memory-model-syntax-resolutions.md:387` | `set` 仅 `opt`/`one`，`many` 无 `set` |
| 自动空初始化、方法调用形式 | `memory-model-syntax-resolutions.md:387` | 确认 |
| `sout(Ref[T])` 打印 | `pending_designs.md:268`（用户裁决） | 保持底层引用输出格式（打印地址），不改；「输出引用指向值」是独立提案，本阶段不碰 |

---

## 3. 待裁决问题清单（逐条向用户汇报，按 Q1→Q12 顺序）

> 依 P13-3/P13-6 惯例：一次一个、给解释与推荐、确认后再进下一项。
> 【必问】= 阻塞实施；【告知】= 仅备案。

### Q1【必问·语法分叉】声明写成哪种形态？
- **背景**：规范 §7.1/pending_designs 用逗号单括号 `Link[T, peer, card]`，p13_plan/任务书用三段方括号 `Link[T][peer][card]`；语法决议已确认方括号形态。
- **A**：三段方括号 `Link[T][peer][card]`（遵循语法决议 L33/L407，与 `func[A][B]` 一致）。
- **B**：逗号单括号 `Link[T, peer, card]`（遵循规范 §7.1 原文，与 `dict[K,V]` 一致）。
- **推荐 A**：语法决议是更晚的正式裁决且明确要求方括号统一；三段形态在 AST 里天然分离三个参数，不易与类型参数混淆。

### Q2【必问·类型表示】`Link` 的字符串参数怎么存？
- **背景**：`peer`（对端字段名）、`cardinality`（opt/one/many）是字符串/枚举，不是类型；`MemoryType` 只有 `(role, inner)` 装不下。
- **A**：新增 `LinkType(TypePtr elem, string peer, Cardinality card)` 子类（推荐）。
- **B**：扩展 `MemoryType` 加两个字段（污染所有能力类型）。
- **C**：peer/card 拼进类型名字符串（mangle 脆弱，无法做对偶校验）。
- **推荐 A**：与 `PoolType` 先例一致，职责单一，`equals()`/对偶校验/mangle 都清晰。

### Q3【必问·ABI】`Link` 的 mangle 编码用什么字符？dual/card 是否入符号？
- **背景**：`Pool=Q`、`Memory=Y<role><inner>`（`rcc_symbol_naming.cpp:86-96`）；历史上有「编码改动需用户批准」先例。
- **A**：新字符（如 `K`）+ 只把 elem 类型入符号，peer/card **不**入（同类签名一致，减少重载爆炸）（推荐）。
- **B**：`K` + peer/card 全部入符号（签名最精确，但同名不同基数会被当成不同类型，重载/赋值更严格）。
- **推荐 A**：peer/card 是对偶关系的元数据，不影响函数的调用签名形状；校验在 struct 定义层做，无需进 ABI。

### Q4【必问·对偶校验】对偶声明检查放在哪、查多严？
- **背景**：§7.1 要求「编译器检查对偶声明互相指向、字段类型一致」；语法决议 L384 已倾向跨类 Link + 跨类检查但未最终确认；L385 基数相容表悬置。
- **A**：struct 定义收集阶段做全量校验：peer 字段存在 + 指回本字段 + 两端 elem 类型一致 + 基数相容表；支持跨类（推荐）。
- **B**：仅校验 peer 字段存在且指回，类型/基数留到首次 `add` 时运行时检查。
- **C**：只做最小校验（peer 字符串非空），其余信任程序员。
- **推荐 A**：对偶一致性是本原语的核心不变量，编译期拦截最安全；跨类按既有倾向确认。基数相容表的具体规则见 Q5。

### Q5【必问·基数相容表】哪些基数配对合法？
- **背景**：语法决议 L385 悬置「基数相容表」；规范只说「相容（不必相等）」；规范自带 `parent: opt ↔ children: many` 范例（§7.1 L338-339）。
- **A**：对称严格：opt↔opt / one↔one / many↔many（会挡住规范自带的 opt↔many 范例，不推荐）。
- **B**：单向宽松：many 端对端任意；opt 端对端 opt/many；one 端对端 one/opt（推荐，兼容 `parent:opt ↔ children:many` 且保住 one 的对称强制）。
- **C**：只查 peer 互指，基数完全不查。
- **推荐 B**：既符合「不必相等」又保住 one 语义；A 与规范范例冲突，C 使基数失去意义。

### Q6【必问·同 Pool 检查】跨池 Link 怎么拦？
- **背景**：§7.6 禁止跨池 Link；语法决议 L386 悬置检查时机（运行时 vs trust 块）。
- **A**：`add`/`set` 时运行时检查两端同池，否则 panic（推荐，与 `pool is full` panic 惯例一致，诊断清晰）。
- **B**：依赖 Link 内部 trust 块检查（扩大 trust 面，与 §12 未决 10 的收紧方向冲突）。
- **C**：编译期类型级证明（需 Pool 实例进类型系统，超出本阶段）。
- **推荐 A**：实现最直接、错误定位清晰。

### Q7【告知·跨池开禁】§12 未决 8：本阶段维持禁止跨池 Link。
- 推荐维持禁止（跨池使对端 Full 不变量与单一销毁点论证复杂化），留独立提案。

### Q8【告知·三方关系】§12 未决 9：维持拆成多个 Link，不引入 n 元 Link 新语法。

### Q9【告知·trust 治理】§12 未决 10：本阶段仅编译器内建实现原语，不向用户开放 trust 语法（守住「用户拿不到单边接口」不变量）。

### Q10【必问·§7.5 实现深度】类型级/实例级效果与事实失效追踪做到什么程度？
- **背景**：§7.5 要求实例级优先判断、冲突退回类型级拒绝、`a≠b` 五来源、Link 事实失效、unlink_all 与 take/release 之间禁插 Link-add 效果调用；验收写明「§7 全部」。P13-6 刚做过「最小集 vs 完整」同类裁决。
- **A**：完整实现 §7.5（推荐，符合验收「§7 全部」，冲突判断是内存安全所需）。
- **B**：最小集（只做实例级冲突 + 保守退回类型级，不做 `a≠b` 五来源全量 / snapshot 精细判断），同步修订验收条款。
- **推荐 A**：验收条款是权威口径；若用户要求缩范围必须同步改验收，避免文档自相矛盾。

### Q11【必问·运行时/状态位】`links_nonempty` 槽状态位与 Pool 头布局是否扩展？
- **背景**：§6.5 规划每槽 2 位含 `links_nonempty`，P13-6 未实现；§7.4 要求追踪「Link 是否确定为空」。
- **A**：本阶段扩展 Pool 槽位图加 `links_nonempty`，原语发射时维护，供 take/release 判据读取（推荐，忠实于规范）。
- **B**：不碰 Pool 头，用独立的编译期 `LinkSet` 事实表维护「Link 是否为空」（更轻，但运行时无物理位）。
- **推荐 A**：与 §6.5 规划一致，且池槽侧位图已有基础设施，扩展成本低；B 的纯编译期追踪难以覆盖别名/跨函数场景。

### Q12【告知·Ref[str] 小缺陷】维持已裁决的「打印地址」，本阶段不碰。
- 记忆 `3edc03d7` 把它标为小缺陷，但 `pending_designs.md:268` 明确「按裁决保持底层引用输出格式」；静默改为打印内容会违反该裁决，「输出引用指向值」须单独提案。本阶段不实施。

---

## 4. 落地路径预览（待裁决后写入实施方案）

1. **sema 层**：`TypeKind::TK_LINK` + `LinkType`（Q2），`parseTypeName`/`typeFromLabelNodeNonNull` 加 `Link` 分支，直接读 `labelDesNodes` 保留 peer/card 字符串（Q1）；mangle 登记（Q3）。
2. **struct 定义收集**：对偶声明全量校验（Q4），基数相容表（Q5）。
3. **运行时**：`core_link_add/remove/clear/unlink_all` 下沉 `core_win64.s`，三点登记法（Q6/Q11）；原语发射含同池 panic。
4. **状态/事实**：Pool 槽 `links_nonempty` 位 + 编译期 `LinkSet` 事实表（Q11），§7.5 效果追踪（Q10）。
5. **判据接入**：`release`（`:13018`）/ `take`（`:13097`）插入 Link 检查。
6. **验收**：`tests/agent/p13-7-link` 回归套件，覆盖 §7 全部 + §11.2 含环图示例。

---

## 5. 后续待设计项（本阶段登记，不实施）

- 跨池 Link 开禁（§12 未决 8）、三方关系一般化（§12 未决 9）、trust 块用户语法治理（§12 未决 10）。
- `sout(Ref[T])` 输出引用指向值的新 API（Q12 引出，须单独提案）。
- 成员集合完整化（Q2 的 B 范围）与 `dict.clone()`/递归释放（P13-6 Q4）仍挂账。

---

## 6. 裁决记录（2026-11-19，逐条讨论确认）

> 节奏：Q1/Q4/Q5/Q10/Q11 批量确认；Q2/Q3/Q6 单独逐条裁决（Q3/Q6 各因「没看懂/要解释加提问」重述后确认）。
> 全部 8 个必问项已闭合，可作为实施方案的权威依据。

| 项 | 主题 | 裁决 | 核心口径 |
|---|---|---|---|
| Q1 | 声明语法形态 | **A** | 三段方括号 `Link[T][peer][card]`；遵循语法决议 L33/L407，与 `func[A][B]` 一致；规范 §7.1 尖括号写法过时 |
| Q2 | 字符串参数存储 | **A** | 新建 `LinkType(TypePtr elem, string peer, Cardinality card)` 子类，与 `PoolType` 先例一致；不动 `MemoryType` |
| Q3 | mangle 编码 | **A** | 新字符 `K` + 只 elem 元素类型入符号；peer/card 不进签名，对偶校验在 struct 定义层做，避免重载爆炸 |
| Q4 | 对偶校验时机/范围 | **A** | struct 定义收集阶段做全量校验：peer 字段存在 + 指回本字段 + 两端 elem 类型一致 + 基数相容表；支持跨类 |
| Q5 | 基数相容表 | **B** | 单向宽松：many 端对端任意；opt 端对端 opt/many；one 端对端 one/opt。兼容规范自带 `parent:opt ↔ children:many` 范例，保住 one 对称强制 |
| Q6 | 同 Pool 检查 | **A** | `add`/`set` 时运行时检查两端同池，跨池 panic（与 `pool is full` panic 惯例一致，退出码 3 + 诊断）；编译期不做 |
| Q10 | §7.5 效果追踪深度 | **A** | 完整实现 §7.5：实例级优先 + 冲突退回类型级拒绝、`a≠b` 五来源、Link 事实失效、unlink_all 与 take/release 之间禁插 Link-add 效果调用；符合验收「§7 全部」 |
| Q11 | links_nonempty 状态位 | **A** | 扩展 Pool 槽位图加 `links_nonempty`，原语发射时维护，供 take/release 判据读取；忠实于 §6.5 规划 |

**告知项（按推荐维持现状，无需实施动作）**：Q7 跨池维持禁止（留独立提案）、Q8 三方关系拆多个 Link、Q9 不向用户开放 trust 语法、Q12 `sout(Ref[T])` 保持打印地址不动（「输出引用指向值」须单独提案）。

---

## 7. 实施与验收（2026-11-19）

**落地实现**（与方案的偏差在此说明）：

- **Q1/Q2/Q3 sema**：`TypeKind::TK_LINK` + `LinkType(elem,peer,card)`（`rcc_sema_types.h`）；
  `typeFromLabelNodeNonNull` 在读 `labelDesNodes` 前拦截 `Link`，peer/card 直接取
  `IdentifierNode::getName()`（不走 `typeFromTypeArgNode`，避免字符串身份降级为 UNKNOWN）；
  mangle `K + elem`（`rcc_symbol_naming.cpp`）。新增 `_declaredClassNames` 集合解决
  类体解析阶段自引用 elem 尚未进 `_classes` 的问题。探针证：`_R1fKN1U` / `_R1fKN1W`
  —— peer/card 确实不入符号。
- **Q4/Q5 对偶校验**：`validateLinkDeclarations()` 在所有类/struct 登记后遍历 Link 字段，
  校验 peer 存在且互指、elem 类型一致、基数相容表 B；支持跨类。探针证：
  `parent:opt ↔ children:many` 放行、`ghost` peer 与 `one↔many` 均带诊断拒绝。
- **Q6 运行时**：`lib/core/core_win64.s` 新增 `core_link_add/remove/clear/unlink_all/
  is_empty/count/has`，用全局对称边表（每边 `(a,fieldA)-(b,fieldB)` 只存一条，
  读端同时匹配两端），单次 `add` 即建立对偶关系（用户拿不到单边接口）。同池检查
  `core_link_slot_in_pool` 沿池块链验证两端槽地址，跨池 `core_panic_raise("cross-pool link")`。
  三点登记法：`core_win64.s` + `core.rio` extern + `coreReturnType()`。
- **Q11 状态**：用编译期 `_linkEmptyFacts`（EMPTY/NONEMPTY/UNKNOWN）实现 §7.4 的
  「Link 是否确定为空」状态格；`unlink_all`→EMPTY、`add/set`→NONEMPTY，跨调用保守 UNKNOWN。
  **偏差**：未落物理 `links_nonempty` 槽位图（方案 §5.1），以编译期事实表替代——副作用是
  重数语义走编译期而非运行时。登记为后续项。
- **§7.5 效果追踪**：顺序场景的保守规则已闭合（add 后事实变 NONEMPTY，
  后续 take/release 被拒）。**偏差**：实例级迭代冲突 / `a≠b` 五来源 / snapshot 迭代修改
  未实现——当前编译器无 `for-in` over Link，属无消费者的基础设施，登记为后续项
  （README §本次未覆盖）。
- **take/release 判据**：`release` 成员判据后插入「不在任何 Link 中」检查，
  `take` Full 检查后插入「所有 Link 字段为空」检查。

**验收**：

- `tests/agent/p13-7-link`：11/11（成对原语、基数范例、含环图、自环、peer/基数/take/set 负向、跨池 panic）。
- 全量 15 套回归全绿（p13-2-regression 153、p13-verify-fixes 43、p13-3-nullable 20、
  p13-3-capability 19、p13-4-subref 9、p13-memory 28、p10-generics 34、p11-overload 24、
  p12-3c-ds、std-full、p12-std-skeleton 31、p5-containers 15、p6-classes 15、p2-functions 13、
  p13-6-pool 7），无回归。
- 工作区 unrelated 未提交改动未触碰；本阶段仅改 `rcc_sema_types.h` / `rcc_codegen.h` /
  `rcc_codegen.cpp` / `rcc_symbol_naming.cpp` / `core_win64.s` / `core.rio` +
  新增 `docs/p13_7_*.md` 与 `tests/agent/p13-7-link/`。

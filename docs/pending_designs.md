# 设计裁决记录（原「待设计项清单」）

> 原生成时间：2026-11-18；改写时间：2026-11-18。
> 性质：**已裁决**。A–E 五项由用户于 2026-11-18 逐项讨论确认，逐条结论见各节「用户裁决」小节。
> 这些均为获批的语言设计层变更，符合项目硬规则 `1b1ccafb`，可直接进入实施。
> 裁决总索引：项目记忆 `aa49d97c`；分项记忆 A `855f44d5` / B `abaedf5c` / C `ba2107d4` / D `8632c4db` / E `8ff5d19d`。

---

## 0. 结论：五项全部裁决完毕

| 编号 | 议题 | 原优先级 | 裁决状态 | 实现状态 |
|---|---|---|---|---|
| **A** | 值语义 `struct` | 最高（挡着 D19，且挡着 B） |   已裁决 |   已实现 |
| **B** | `Maybe` / `if var` 窄化 | 最高（与 A 同批） |   已裁决 |   已实现 |
| **C** | 条件销毁的诊断级别 | 中 |   已裁决 |   已实现 |
| **D** | `split(k)` 两段视图的绑定形态 | 中 |   已裁决 |   已实现 |
| **E** | 异常对象的出生地与归属 | 低 |   已裁决（推迟） | ⬜ 推迟 |

原文档提到的另有 4 项属**后续阶段**或**长期悬置**，仍列在 §6 供全貌了解，不阻塞当前。

**已全部闭环、不需要再出文档的**（见 §7）：Pool 类型、出生地标签区、`fill`/`clear`/`release`、四态状态格、take 挂起态、子引用/NLL、签名标注校验。决议在 `docs/p13_2_syntax_resolutions.md`，已实现并跑通。

---

## A. 值语义 `struct`（已裁决）

### 为什么它是最高优先级

规范 §6.2 要求值语义 `struct`，而 Rio 现在**只有引用语义的 `class`**。这不是"少一个语法糖"，是**语义空洞**：

```rio
class Player { var hp: int }
var a: Player = Player(100)
var b: Player = a        // 指针拷贝！a 和 b 指向同一对象
b.hp = 50
sout(a.hp)               // 50 —— 但内存模型要求这里必须是 100
```

`Content` 的唯一所有权前提是"赋值即转移"，而 `class` 的赋值是共享指针。规范 §12 未决问题第 14 条已登记这个缺口，D19 裁决要求"正面引入值语义 struct 解决"。

**D19 的原话**：不采用"先做精简 P13-2、把用户对象推到后续子阶段"的方案，P13-2 一次做完整版。

### 规范怎么写的 / 现状实测

`docs/memory-model-spec-full.md` §6.2 只有一句"值语义 struct"，**没有任何语法**。实测 `struct Pt { var x: int }`：

```
Error: Code generation failed: Undefined variable: struct
```

### 用户裁决（2026-11-18）

1. **声明语法**：独立关键字 `struct`，与 `class` 并列；**不支持继承**（不能 extends、不能当基类）。

2. **字段范围**：第一版**只允许标量字段**（int/float/bool/char）；**禁止 ptr、str、list、dict、class、Body、Content 字段**。比原提案更严——原提案曾把 `ptr` 列入允许，用户删掉了 `ptr`。理由：`str` 是"指针+长度"的头，值拷贝只浅复制指针 → 共享堆数据 → double-free/悬垂；ptr 同理。内存按字段顺序内联展开。

3. **赋值语义**：struct 是值类型，`var b = a` **逐字节拷贝**，a 仍可用、两者独立；**不进 Content 状态格、不参与移动/消耗**（与 int 同级）。此拷贝不受 Content 唯一所有权约束——struct 只含纯值、无堆资源所有权，复制不会制造共享所有权。`class` 赋值保持现状（指针拷贝），不做 breaking change。

4. **引用**：`&s`（s 为 struct 变量）得 `Ref[Pt]`，指向变量位置；`&s.x` 是子引用展开到字段。

5. **struct vs class 的边界**：靠**语义选择**，不加强制约束（不强制"只能装值类型字段"之外的显式标注）。含引用字段的 struct 等后续有元素级 clone 再做。

**出口意义**：解决 class 指针拷贝与 Content 唯一所有权的冲突（D19），需要值语义的场合改用 struct。

**实现状态（2026-11-18 已完成）**：词法/语法/AST/代码生成全链路打通。结构体值在寄存器里始终是**地址**，赋值与传参走 `core_mem_copy` 语义的逐字节拷贝，返回走「调用方分配返回缓冲、首参传地址」的 ABI（避免返回指向已销毁栈帧的指针）。字段仅允许标量（int/float/bool/char），非标量字段与继承/方法在 `registerStruct` 阶段报错。回归套件待补。

---

## B. `Maybe` / `if var` 窄化（已裁决）

### 规范怎么写的 / 现状实测

§3.4 / §8.2 写的是 `get()?.name`、`if let Some(p)`、`match`，但**语法全是示意**（规范第 4 行自己标注"语法为示意，未最终确定"）。

```rio
class P { var n: str }
var a: P? = P("x")
if let b = a { sout(b.n) }
```
```
Error: Compilation failed: syntax errors.
```

`if let` / `match` 两个关键字都不存在（`T?` 类型本身 P12 已有；但另有记忆 `a87037d8` 说 `T?` 语法"完全不能写"——这处文档矛盾留待实现时核对源码，不影响设计结论）。

### 已有裁决的约束（不重定）

- 裁决 `388a73e0`：**只有引用类型可空** + 对象包装基本类型
- 裁决 `a87037d8`：零值方案（用 null 表示"无"）**不可行**，已证伪
- 决议 §16.2：倾向 `get()` 返回 `T?`，**不引入 Maybe** —— 现由本次裁决确认为最终结论

### 用户裁决（2026-11-18）

1. **不引入 `Maybe` 类型，复用 `T?`**。`get()` 越界**不 panic**（与 §16.2「get() 返回 T?」一致）；`[i]` 索引**保持现有零值行为**不变。（用户否决了 agent 提议的"get() 越界 panic"方案。）

   > **2026-11-18 修正（用户裁决 A）**：原句写「`get()` 越界返回 `T?` 的 null」，这对**值类型元素**是错的 —— 值类型没有「没有」这个状态，用 0 冒充 null 会让 `list[int]` 里真实的 `0` 静默变成「越界」。现改为按元素静态类型分化，见下文「补充裁决」。


2. **窄化语法**：
   ```
   if var p = r.get() { ... } else { ... }
   ```
   **复用 `var` 关键字做绑定，不引入 `let` 关键字、不用 `if let`**。p 非 null 进 then 块并绑定为 T；null 走 else 或跳过。

3. **`if var` 边界**：
   - 绑定变量 p 的作用域**仅限 then 块**；else 里不可见
   - else 可省略
   - `=` 右边**必须是 `T?`**（类型约束）

4. **`match`**：**P13-2 不做 match**，只做 `if var`。

   > **2026-11-18 追补（用户裁决 A）**：从「P13-2 不做」升级为**从待办清单彻底删除**，不再是「留后续阶段」的悬空项。理由：Rio 当前**没有 `enum`**（实测 `Undefined variable: enum`）、没有代数数据类型，`match` 缺少可匹配的对象；而 `if var` + `if/else` 已能表达现有场景。真正需要 `match` 的时机是引入 `enum` / sum type 时，那时一并设计才有意义。若将来要做，须另立提案，并先明确「匹配什么」（`T?` 的 null/非 null？整数？字符串？）。

5. **`if [a]({...})` 通用绑定糖**：~~留后续阶段，P13-2 不做。~~

   > **2026-11-18 已按用户裁决 A 删除**：该条目的语法形态**从未被定义过** —— `[a]` 读作列表字面量、`({...})` 读作括号包块，二者拼在一起不构成任何已知语法，文档里也找不到它的设计意图与来源。而它想表达的东西（另一种绑定语法）已被本小节第 2 条裁决覆盖：「复用 `var` 关键字做绑定，不引入 `let` 关键字、不用 `if let`」。规范 §3.4 / §8.2 里残留的 Rust 形态 `if let Some(p) = r.get()` 同样已被 `if var` 取代。**留着它是记录一个已被取代的方案，应删而非延后。**

### 补充裁决：值类型容器的 `get()`（2026-11-18，用户选方案 A）

**问题**：`get()` 对任意元素类型的容器开放，而裁决「只有引用类型可空」禁止 `int?`。两条裁决在此冲突，实现默默选了第三条路 —— 内部照旧构造 `int?`，运行时用 0 当 null。实测后果（修前）：

| 容器与下标 | 元素实际值 | 期望 | 修前实际 |
|---|---|---|---|
| `list[int] [0, 5]` → `get(0)` | `0` | `0` | **走 else** |
| `list[bool] [true, false]` → `get(1)` | `false` | `false` | **走 else** |
| `list[float] [0.0, 1.5]` → `get(0)` | `0.0` | `0.0` | **走 else** |
| `list[str] ["", "x"]` → `get(0)` | `""` | `""` | `""`   |

`str`/`list`/`dict`/类/`ptr` 全对（空指针才是 null，`""` 是有效对象），`int`/`bool`/`float` 三种全错，且**不崩不报错**——静默走 else 分支。这正是裁决 `a87037d8` 判定「零值当 null 不可行」的同一坑，被 `get()` 原样搬到了容器返回值上。

**裁决**：

1. **`get()` 保留**，但静态返回类型按**元素类型**分化：

   | 元素类型 | `get()` 的静态返回类型 | 越界行为 | `if var` |
   |---|---|---|---|
   | 引用型（`str`/`list`/`dict`/类/`ptr`/`Ref`/`Mut`/`Take`/`Pool`） | `T?` | 给 null |   允许 |
   | 值类型（`int`/`float`/`bool`/`char`/`struct`） | `T`（非可空） | 给零值，与 `[i]` **完全一致** | ❌ 编译期报错 |

   值类型元素的 `get()` 不再提供「有没有」的信息；要判断请用 `size()`。

2. **实现约束（用户明确追加，比修行为更重要）**：不能只修 `if var` 的运行时判零，**编译器内部类型系统必须一致**：
   - `list[int].get()` 的静态返回类型**就是 `int`**，不许暗中构造 `int?`；
   - `list[str].get()` 的静态返回类型才是 `str?`；
   - `if var` 必须**依据静态类型**决定是否允许，**不是**在运行时检查寄存器是不是零；
   - `sout` 等其它类型检查也必须看到正确的静态返回类型。

   否则只是把探针里的一个错误绕过去，内部矛盾仍在。

**实现状态（2026-11-18 已完成）**：新增唯一事实源 `CodeGen::isNullableCapable`（判据：只有指针族有「没有」这个状态）与 `CodeGen::containerGetReturnType`。`inferType` 的 `get()` 分支、`T?` 标注解析（`typeFromLabelNode`）、`if var` 的允许性判断三处全部改走它们，不再各自写一份等价判断。运行时**一行没改** —— `get()` 的越界分支本来就给 0，值类型的 0 就是零值、引用型的 0 就是 null，所以静态类型改对之后运行时自然正确。`emitListGetOrNull`/`emitDictGetOrNull` 更名为 `emitListGet`/`emitDictGet`（原名字暗示必然返回 null，已不准确）。回归套件 `tests/agent/p13-3-nullable` 由 12 例扩到 **16 例**，其中 `pos_value_zero_is_value` 专门盯「合法零值不能被当成没有」。


**实现状态（2026-11-18 已完成）**：`T?` 只在引用型上允许（值类型在标注解析阶段直接报错）；`T` 可隐式提升到 `T?`，反方向必须用 `if var` 取出；`if var p = expr` 的 `p` 只活在 then 块（`popScope` 后消失）；`[i]` 保持越界读零值的原语义。容器 `get(i)`/`get(k)` 的返回类型按元素类型分化（见下文「补充裁决」）：引用型元素给 `T?`（越界 null，**不 panic**），值类型元素给非可空 `T`（越界零值）。编译器不做隐式空检查，安全性由「必须先取出」的规则保证。回归套件 `tests/agent/p13-3-nullable`（16 例）。

---

## C. 条件销毁的诊断级别（已裁决、已实现）

### 背景

§8.4 第 3 点原写：分支出口状态不一致时插条件销毁（flag 位图），默认**警告**，模块级 `#[strict_destroy]` 升级为**错误**。

条件销毁的**代码生成**已能工作（实测 `f4_cond_run` 正常输出），但诊断层原本没实现（既不警告也不报错）。

关键坑：`#[strict_destroy]` 是示意写法，Rio **没有属性语法**，`#` 连注释都不是（实测当代码解析，报 `Undefined variable`），该写法落不了地。

### 用户裁决（2026-11-18）

用户**翻转**了 agent 的"默认静默 + 编译开关"提案：

1. **语言默认：需要条件销毁的出口是编译错误**（不是静默、不是警告）——凡分支出口状态不一致、需插条件销毁处，默认直接编译报错。

2. **放行机制 = 文件级注释指令** `// @allow-cond-destroy`，编译器**扫描注释**识别；不引入属性语法、不引入关键字、不影响 Rio 语法（复用现有 `//` 注释）。

3. **命令行开关控制"读不读这个注释"**：启用 = 扫描并识别该指令；禁用 = 忽略指令，文件里的 `// @allow-cond-destroy` 失效（等于没写）。

4. **放行需双重条件**：文件声明了指令 **且** 命令行启用了注释扫描，才把条件销毁出口从错误放行；缺一仍报错。

5. **改写规范 §8.4**：原写"默认警告 + `#[strict_destroy]` 升级为错误"，现改为"默认错误 + 文件级注释指令放行 + 命令行开关控制指令是否生效"。

### 实现状态（2026-11-18 已完成）

判据落在 `CodeGen::checkCondDestroyAtExit(scopeIndex)`：遍历该作用域登记的销毁项，跳过 `moved`/`borrowed`，只对「壳子状态是 `?` **且** 是控制流合流产生的 `?`」（`isMergedUnknown`，排除 Take 形参入口那种编译器不做承诺的 `?`）报错。调用点有两个 —— `emitScopeDrops` 与 `popScope` —— 覆盖全部 8 个销毁点，不会漏。

放行需 `_allowCondDestroy && _condDestroyDirectiveSeen` 同时成立，缺一仍报错，且诊断文本会明确说**缺的是哪一半**（"the command line switch -allow-cond-destroy is not enabled" / "the file has no `// @allow-cond-destroy` directive"）。

- 命令行：`-allow-cond-destroy`（RCC 的 flag 是**单横线**，与 `--path`/`--output` 这类取值选项不同）。
- 指令扫描：`rcc_main.cpp` 的 `scanCondDestroyDirective()` 在**原始文本**上做，不看 lexer —— 因为 `rcc_lexer.cpp` 只跟踪注释类型、**丢弃注释正文**，改 lexer 会破坏既有契约。只扫主文件，不扫被 import 的模块。
- 代码生成：`emitScopeDrops` 对每个非 `moved` 项都发一次 drop 调用，看起来是"无条件销毁"，但 **drop glue 自身先判空**（`emitDropGlueBody` 入口就是 `movq (%rcx),%rax` / `testq %rax,%rax` / `je null`），空壳直接返回。所以放行之后「分支里已 clear、出口处壳子是空的」不会重复释放 —— 效果上等价于条件销毁，不需要额外的 flag 位图。2026-11-18 实测：`L3-cond-destroy-release` 里 `c` 在 `cond=true` 分支被 clear，两条路径各跑一次都正常退出。

回归：`tests/agent/p13-2-regression` 的 `L3-cond-destroy`（默认必须报错）、`L3-cond-destroy-no-switch`（有指令无开关仍报错）、`L3-cond-destroy-release`（指令 + 开关齐 → 编译并运行，需 `flags: ["-allow-cond-destroy"]`）。

---

## D. `split(k)` 两段视图的绑定形态（已裁决、已实现）

### 背景

§4.3 只写"返回两段 Mut 视图"，**没说怎么绑定**。

现状实测：
```rio
var xs: list[int] = [1,2,3]
var parts = xs.split(1)
```
```
Error: Cannot determine the object type of method call
```
方法存在性判定不了，因为返回类型未定。关键约束：**Rio 现在没有元组**。

底层依据（记忆 `f9f6c2b8` 容器内存模型）：`list` = 一个 Body + 一块堆内存；元素是子引用（缓冲区扩容会搬动元素）；访问路径 `xs.items` / `xs.items[*]`；`push`/`insert`/`remove`/`clear` 路径登记在 `xs.items`，是所有元素子引用的前缀。

### 用户裁决（2026-11-18）

1. **两段返回 = 调两次**，零新语法：
   ```
   var a = xs.split_at(1)    // 前段 [0, k)
   var b = xs.split_from(1)  // 后段 [k, len)
   ```
   各返回单个 Mut 视图，避开"元组不存在"的大特性。

2. **越界语义 = panic**（k<0 或 k>len），与索引一致；不采用 clamp。

3. **两段视图可共存**：区间不重叠（前 `[0,k)` / 后 `[k,len)`），各自可改自己区间的元素。

4. **视图存活期对原列表**：`push`/`insert`/`remove`/`clear`（路径 `xs.items`）与视图冲突 → 编译报错（扩容搬动元素会让视图失效）；视图不能 take/整体搬动列表。

5. **子引用路径需支持区间** `xs.items[lo..hi)`（现有只有 `items`/`items[*]` 两档）——这是"两段共存"的前提，用户接受该实现量。

> 实施提示：若区间子引用实现量超预期，可与用户再权衡"保两段共存"还是"退化成两段互斥（保守登记在 `xs.items`）"。

### 实现状态（2026-11-18 已完成，两段共存方案）

按裁决原样实现，**没有退化成互斥**。落地要点：

- **语法零新增**：`xs.split_at(k)` / `xs.split_from(k)` 是 `emitMethodCall` 里的内建方法分支（与 `get_mut(i)` 同级），不走用户方法解析，所以不需要在标准库里定义 `split_at`/`split_from`。返回类型由 `inferType` 的方法调用分支给出 `Mut[elem]`。
- **视图值 = 该段首元素槽地址**：`split_at` 取 `data`，`split_from` 取 `data + k*8`。`b[i]` 于是就是 `*(view + i*8)`，读写在 `emitIndexExpression` / `emitIndexAssignValue` 里各加一个 `TK_MEMORY + MUT` 分支。视图本身不带长度 —— 边界由建视图时的检查保证，元素下标不再重复检查。
- **越界 panic**：`k<0 || k>len` 走 `core_panic_raise`，消息用 `internStrValue` 物化（cap==0 的静态头，`core_free` 静默忽略，不会漏放）。实测退出码 3，stderr 打出 `split_at index out of range` / `split_from index out of range`。
- **区间路径**：登记成 `xs.items[0..k)` / `xs.items[k..len)`。`pathIsPrefixOf` 按路径段比较（`[` 与 `.` 都是段边界），所以 `xs.items` 是它们的共同前缀 —— 增长路径天然命中。
- **共存判据** `rangesDisjoint`：只认"同一下界文本相接"（`hiA == loB` 或反向），即 `[0,k)` 与 `[k,len)`。其余形状（符号边界、同侧两段、交叉）一律保守冲突。边界文本取整数字面量或标识符名，其余留空 → 必然冲突。
- **增长冲突**：`listAppend`/`listRemove` 是泛型，泛型调用不经过 `applyWriteEffects`（效果表按实例 mangling 登记，调用点拿的是未 mangling 的名字），所以在 `emitFunctionCall` 入口加了 `checkGrowthEffects`，按被调者名字拦一道，写路径是第一个实参的 `.items`。裸名与 `ds.listAppend(...)` 两种写法都覆盖。
- **`get_mut` 路径下移**：从 `xs[*]` 改为 `xs.items[*]`，与区间视图、增长路径统一到同一层，否则增长冲突打不到元素视图。

回归：`tests/agent/p13-2-regression` 的 `L3-split-coexist` / `L3-split-write` / `L3-split-panic-at` / `L3-split-panic-from` / `L3-split-overlap` / `L3-split-growth` / `L3-get_mut-growth`。

---

## E. 异常对象的出生地与归属（已裁决：推迟）

### 背景

§9.2 第 4 条："P13-1 不释放"，长期归属待定（规范 §12 未决问题第 15 条）。D1 裁决"异常即泄漏"——`throw` 创建的对象永远泄漏，在 P13-1/P13-2 是已知且可接受的代价，不阻塞任何工作。

### 用户裁决（2026-11-18）

**推迟**。异常对象的长期归属（catch 块销毁 / 进程级池 / 维持泄漏）不在 P13-2 定；P13-2 及之前继续按 D1「异常即泄漏」处理。规范 §12 未决第 15 条、§9.2 第 4 条保持悬挂。

---

## 6. 后续阶段的待设计项（不阻塞当前）

这些属于 P13-3 之后，列出来只为一次看清全貌。

| 项 | 阶段 | 状态 |
|---|---|---|
| 能力类型 `Ref`/`Mut`/`Take` 的具体语法 | P13-3 | **部分已实施**：`&`、`ref()`/`mut()`/`take()`、隐式转换已工作。P13-3 的"降级规则"（只能 Take→Mut→Ref）与"Take 是二等值"约束需要单独提案 |
| `Link[T, dual, 基数]` 成对引用 | P13-7 | 类型完全不存在（`unknown type 'Link'`）。规范 §7 有完整设计。**基数语义已裁决（2026-11-18，选 A）**：`opt` 至多一个、`one` **连接后**恰好一个、`many` 任意多个；`one` 约束的是「已连接时的对偶一致性」，**不要求创建即连接** —— 字段创建后、首次 `set` 之前是空状态（`a.spouse.get()` 返回 `T?`），此时不违反 `one`。`one` 与 `opt` 的差别不在连接数，而在**连接后的对称强制**：`set` 自动建立反向边，且任一方不能单方面断开（只能 `set` 到新对端，或成对 `unlink_all()`）。否决「构造时必须连接」：互相引用的两个对象将无法同时构造。规范 §7.1 已同步此条。落地形态仍待实施 |
| `spawn` / 结构化并发 | P13-8 或 P14 | 规范 §10 整章，建议不在 P13 承诺 |
| 跨池 Link | 规范 §12 未决 8 | 目前禁止，是否需要开 |
| 三方关系是否一般化为多元 Link | 规范 §12 未决 9 | 需拆成多个 Link 够不够 |
| `trust` 块的治理 | 规范 §12 未决 10 | 用户代码是否允许写、如何审计 |
| 内存序 / 取消机制 / 异步协程 | 规范 §12 未决 2/4/5 | 完全未设计 |

---

## 7. 已经闭环、不需要再出文档的（避免重复劳动）

以下原先在 `p13_2_design_doc_checklist.md` 的 #1–#5，**已由 `docs/p13_2_syntax_resolutions.md` 裁决并全部实现**：

| 项 | 裁决要点 | 实测状态 |
|---|---|---|
| Pool 类型 | 标准库普通类型、不做关键字；`fixed`/`boxed`/`chunked` 三种静态方法 |   `Pool.fixed(100)` / `.boxed(100)` / `.chunked(100)` 全部编译运行 |
| 出生地标签区 | `var p: Player pool = ...`；砍掉 `pool.add`（违反"出生后地址不变"） |   标签区写法工作，`pool.add` 确实不存在 |
| `pool.new[T]` | 构造器选择，返回 T，满时 abort |   `pool.new[Player]("a")` 工作 |
| `fill`/`clear` | 方法形态，与 take/ref/mut 对称 |   `b.clear()` / `b.fill(Box())` 工作 |
| `pool.release(x)` | 方法形态，x 须先 clear（须为 E 态） |   存在；未 clear 时报 `release target 'p' is not Empty` |
| 四态状态格 | E/F/H/X 是**编译期事实表**，不是运行时元数据（**不是 ABI 变更**） |   已实现 |
| take 挂起态 | `pending_take` 集合；同一语句 = 同一 AST 节点；独立 take 警告不阻断 |   独立 `take()` 出 warning；挂起态检查工作 |
| 子引用 / NLL | 路径穿透引用展开；过程内 NLL + 跨函数保守 |   `var r: Ref[str] = &p.name` 工作 |
| 签名标注校验 | `ensures` 逐出口、`from` 子集、`writes` 泛型上界 |   147 条回归全过 |

**另外两处实现与决议不一致，已记为待修缺陷（不涉设计，不需要裁决）**：

1. `release(x)` 全局函数形态：决议 §6.4 标题写 `release(x)`，但 §2.1 明确方法形态 `pool.release(x)`。实测全局 `release(p)` 能过类型检查但**链接失败**（`undefined reference to 'release'`）。

   > **2026-11-18 已按用户裁决 A 处理**：删掉这个伪符号，改为在 `emitFunctionCall` 的裸名分支拦截 —— 未定义过的 `release(...)` 一律编译期报错 `release(x) is not a function; release is a Pool primitive, write it in method form: pool.release(x) (decision L3)`，不再拖到链接阶段。诊断依据是「`release` 不在 `_functionParams` / `_functionReturns` / `_genericFunctions` / `_overloadedNames` 任何一张表里」，所以用户自己写的 `fun release(x: int)` 不受影响（保留名只拦「没定义过」的情况）。不实现全局形态：那要从出生地反推 Pool，等于打开隐式推断 Pool 的口子，与「跨池 release 必须显式」的既有裁决方向相反，且默认出生地的 Body 根本没有 Pool 可反推。回归：`L3-release-bare-name`（编译期报错）、`L3-release-user-func`（同名函数不被误伤）。
2. `pool.add_reusing(...)`：spec §6.2「槽位回收」提到"或 `pool.add_reusing(...)` 优先填空槽"（原文把位置写成 §3.1，实为 §6.2），但语法层完全没有。

   > **2026-11-18 已按用户裁决 A 处理**：判定为**分配器内部策略**，不是用户可见原语，已从 spec 删掉这个假 API。依据：池的槽位复用对外不可观察（用户只持有 Body 句柄，看不到槽地址，也看不到迭代顺序差异），所以它没有成为用户原语的资格。否决"实现为用户可见原语"：那需要先定义它与出生地操作的语义差异（地址稳定性？迭代顺序？），而这两点 spec 都没说，凭空造语义违反项目硬规则。
   >
   > **同轮连带发现（比 add_reusing 严重得多）**：spec 里 `pool.add(...)` 出现了 8 处（§2.2 示例、§3.5、§8.3、§11 示例三连），而 `pool.add` **已被既有裁决砍掉** —— 它违反"出生后地址不变"，由 `pool.new[T](...)` + 出生地标签区取代（见上表「出生地标签区」行）。实测 `p.add(Player("a"))` 直接报 `Cannot determine the object type of method call`，`var a: Player = p.add(...)` 报 `cannot assign a value of type int`。也就是说 spec 通篇用的是一个已被否决的语法。本轮已把 spec 的 8 处全部改为 `pool.new[T](...)`，§6.1 的「满了 `add` 返回 `Maybe`」改为「满了 `pool.new[T]` 中止（abort）」，与 `pending_designs.md` 上表记录的实现状态对齐。

3. **间接调用（闭包 / 函数值）返回 struct**：`var f = () -> { ret Pt(1, 2) }; var p: Pt = f()` —— 编译能过，运行期拿到的是错误值。

   > **2026-11-18 已按用户裁决 A 处理（编译期显式拒绝）**：struct 返回需要调用方传缓冲区地址（`__p5_retbuf` 占第一个整型寄存器），被调用方拷进去再在 `%rax` 返回缓冲区地址。直接调用 / 方法 / 泛型 / 重载四条路径都已修好，但**间接调用**没接上 —— 它的目标地址在 `%r11`、env 在 `%r10`，`%rax` 是闭包指针本身的来源，没有位置再摆 retbuf，于是返回值被当整数用（`%rax` 里其实是别的地址），属**静默错误值**，正是本项目一直在清理的那类问题。
   >
   > 裁决理由：①`func[][Pt]` 这种类型在实践中用途有限（闭包通常返回引用类型或标量）；②先明确拒绝比留一个运行期崩溃的洞好；③将来有真实需求时再实现，那时已知具体用法，可以设计得更准。**否决「保持现状」** —— 静默出错不可接受。
   >
   > 实现：`emitIndirectCall` 入口按「变量槽类型 → `inferType` 回退」取被调用者的 `FunctionType`，返回类型为 `TK_STRUCT` 时报 `indirect calls cannot return a struct value ('Pt'); ... call it directly, or return a reference/pointer instead`。回归：`L3-indirect-struct-ret`（编译期报错）、`L3-indirect-scalar-ret`（闭包返回标量不受影响，确认没有误伤间接调用本身）。

4. **容器元素为 struct**（`list[Pt]` / `dict[str, Pt]`）：整个不可用，且比上一条更危险。

   > **2026-11-18 已按用户裁决 A 处理（编译期显式拒绝）**：实测 `var xs: list[Pt] = [Pt(1,2)]` 后，`xs.size()` / `xs[0].x` / `xs.get(0).x` **全部**报 `Cannot determine the object type of attribute access`（容器里 struct 的元素布局、`[]` 取值、字段访问都没接）；而 `var p: Pt = xs[99]` 越界时**静默返回垃圾地址**、不报错 —— 比间接调用 struct 返回更危险，因为那条至少会因类型不匹配报错。
   >
   > 裁决理由：①静默错误值不可接受；②`list[Pt]` 在实践中可以用 `list[Ref[Pt]]` 或对象包装替代；③将来有真实需求再实现，那时要定 struct 在堆缓冲区的布局、逐元素拷贝语义、`get()` 返回什么。**否决「仅记录为已知缺口」**。
   >
   > 实现：在两个类型解析入口各加一道校验 —— `typeFromLabelNodeNonNull`（变量/形参标注 `var xs: list[Pt]`、`fun f(xs: list[Pt])`）与 `typeFromTypeArgNode`（泛型实参、`func` 签名里的类型位置），命中 `TK_STRUCT` 时报错并指引 `list[Ref<Pt>]` 或对象包装；dict 的键和值同样拦。回归 +4：`L3-list-struct-elem` / `L3-dict-struct-elem` / `L3-param-struct-elem`（三条必须报错）、`L3-container-scalar-elem`（非 struct 元素全部正常，确认没误伤）。

**2026-11-18 补记**：另有三处 struct 返回 ABI 缺陷（泛型 / 重载 / 方法三条调用
路径没给返回缓冲区留位置，其中方法路径会把 struct 值写进接收者对象本身）
已修复，详见 `docs/p13_2_status.md`「L3-P」一节。同轮补了 9 条 struct 回归用例
（`tests/agent/p13-2-regression/code/L3-struct-*.rio`）—— 此前整个回归套件里
没有任何 `struct` 声明。

---

## 8. 建议的推进顺序

```
A + B 的设计文档（  已出，本文）
        ↓
实现 struct + if-var（  2026-11-18 已完成，提交 9f5333e）
        ↓
C（条件销毁诊断）——   2026-11-18 已完成
        ↓
D（split 两段绑定）——   2026-11-18 已完成（含子引用区间档）
        ↓
P13-2 完整版收尾，进 P13-3
```

当前 P13-2 主体（L0–L3 全部 + std 重写）已可交付，测试 718/720 通过。A–E 是"完整版"与"主体版"的差额，现已**全部裁决并实施完毕**（E 项按裁决推迟）。

---

## 9. 一句话总结

**A–E 五项设计全部裁决并实施完毕**：值语义 `struct`（标量字段、值拷贝、不进状态格）+ `if var` 窄化（复用 `T?`、不做 match）解除 P13-2 完整版的两大阻塞；条件销毁默认报错、由文件级注释指令配合命令行开关放行；`split` 用 `split_at(k)` / `split_from(k)` 两次调用返回不重叠的两段视图；异常对象归属推迟。

# 设计裁决记录（原「待设计项清单」）

> 原生成时间：2026-11-18；改写时间：2026-11-18。
> 性质：**已裁决**。A–E 五项由用户于 2026-11-18 逐项讨论确认，逐条结论见各节「用户裁决」小节。
> 这些均为获批的语言设计层变更，符合项目硬规则 `1b1ccafb`，可直接进入实施。
> 裁决总索引：项目记忆 `aa49d97c`；分项记忆 A `855f44d5` / B `abaedf5c` / C `ba2107d4` / D `8632c4db` / E `8ff5d19d`。

---

## 0. 结论：五项全部裁决完毕

| 编号 | 议题 | 原优先级 | 裁决状态 | 实现状态 |
|---|---|---|---|---|
| **A** | 值语义 `struct` | 最高（挡着 D19，且挡着 B） | ✅ 已裁决 | ✅ 已实现 |
| **B** | `Maybe` / `if var` 窄化 | 最高（与 A 同批） | ✅ 已裁决 | ✅ 已实现 |
| **C** | 条件销毁的诊断级别 | 中 | ✅ 已裁决 | ✅ 已实现 |
| **D** | `split(k)` 两段视图的绑定形态 | 中 | ✅ 已裁决 | ✅ 已实现 |
| **E** | 异常对象的出生地与归属 | 低 | ✅ 已裁决（推迟） | ⬜ 推迟 |

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

1. **不引入 `Maybe` 类型，复用 `T?`**。`get()` 越界**返回 `T?` 的 null，不 panic**（与 §16.2「get() 返回 T?」一致）；`[i]` 索引**保持现有零值行为**不变。（用户否决了 agent 提议的"get() 越界 panic"方案。）

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

5. **`if [a]({...})` 通用绑定糖**：留后续阶段，P13-2 不做。

**实现状态（2026-11-18 已完成）**：`T?` 只在引用型上允许（值类型在标注解析阶段直接报错）；`T` 可隐式提升到 `T?`，反方向必须用 `if var` 取出；`if var p = expr` 的 `p` 只活在 then 块（`popScope` 后消失）；容器内建 `get(i)`/`get(k)` 返回 `T?`，越界/未命中给 null 而**不 panic**，`[i]` 保持越界读零值的原语义。编译器不做隐式空检查，安全性由「必须先取出」的规则保证。回归套件 `tests/agent/p13-3-nullable`（12 例）。

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
| `Link[T, dual, 基数]` 成对引用 | P13-7 | 类型完全不存在（`unknown type 'Link'`）。规范 §7 有完整设计，但**基数语义**（`1`/`many`/`*@L`）需要确认落地形态 |
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
| Pool 类型 | 标准库普通类型、不做关键字；`fixed`/`boxed`/`chunked` 三种静态方法 | ✅ `Pool.fixed(100)` / `.boxed(100)` / `.chunked(100)` 全部编译运行 |
| 出生地标签区 | `var p: Player pool = ...`；砍掉 `pool.add`（违反"出生后地址不变"） | ✅ 标签区写法工作，`pool.add` 确实不存在 |
| `pool.new[T]` | 构造器选择，返回 T，满时 abort | ✅ `pool.new[Player]("a")` 工作 |
| `fill`/`clear` | 方法形态，与 take/ref/mut 对称 | ✅ `b.clear()` / `b.fill(Box())` 工作 |
| `pool.release(x)` | 方法形态，x 须先 clear（须为 E 态） | ✅ 存在；未 clear 时报 `release target 'p' is not Empty` |
| 四态状态格 | E/F/H/X 是**编译期事实表**，不是运行时元数据（**不是 ABI 变更**） | ✅ 已实现 |
| take 挂起态 | `pending_take` 集合；同一语句 = 同一 AST 节点；独立 take 警告不阻断 | ✅ 独立 `take()` 出 warning；挂起态检查工作 |
| 子引用 / NLL | 路径穿透引用展开；过程内 NLL + 跨函数保守 | ✅ `var r: Ref[str] = &p.name` 工作 |
| 签名标注校验 | `ensures` 逐出口、`from` 子集、`writes` 泛型上界 | ✅ 145 条回归全过 |

**另外两处实现与决议不一致，已记为待修缺陷（不涉设计，不需要裁决）**：

1. `release(x)` 全局函数形态：决议 §6.4 标题写 `release(x)`，但 §2.1 明确方法形态 `pool.release(x)`。实测全局 `release(p)` 能过类型检查但**链接失败**（`undefined reference to 'release'`）。应删掉这个伪符号或补上实现。
2. `pool.add_reusing(...)`：§3.1 提到"或 `pool.add_reusing(...)` 优先填空槽"，但语法层完全没有。需要确认这是**必做**还是**可选优化**。

**2026-11-18 补记**：另有三处 struct 返回 ABI 缺陷（泛型 / 重载 / 方法三条调用
路径没给返回缓冲区留位置，其中方法路径会把 struct 值写进接收者对象本身）
已修复，详见 `docs/p13_2_status.md`「L3-P」一节。同轮补了 9 条 struct 回归用例
（`tests/agent/p13-2-regression/code/L3-struct-*.rio`）—— 此前整个回归套件里
没有任何 `struct` 声明。

---

## 8. 建议的推进顺序

```
A + B 的设计文档（✅ 已出，本文）
        ↓
实现 struct + if-var（✅ 2026-11-18 已完成，提交 9f5333e）
        ↓
C（条件销毁诊断）—— ✅ 2026-11-18 已完成
        ↓
D（split 两段绑定）—— ✅ 2026-11-18 已完成（含子引用区间档）
        ↓
P13-2 完整版收尾，进 P13-3
```

当前 P13-2 主体（L0–L3 全部 + std 重写）已可交付，测试 718/720 通过。A–E 是"完整版"与"主体版"的差额，现已**全部裁决并实施完毕**（E 项按裁决推迟）。

---

## 9. 一句话总结

**A–E 五项设计全部裁决并实施完毕**：值语义 `struct`（标量字段、值拷贝、不进状态格）+ `if var` 窄化（复用 `T?`、不做 match）解除 P13-2 完整版的两大阻塞；条件销毁默认报错、由文件级注释指令配合命令行开关放行；`split` 用 `split_at(k)` / `split_from(k)` 两次调用返回不重叠的两段视图；异常对象归属推迟。

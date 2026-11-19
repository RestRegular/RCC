# P13-2 前置审计：`memory-model-syntax-resolutions.md` 逐条对照编译器实测

> **本文性质**：审计记录，不是设计方案。
> 只做三件事：①把决议文档的每条主张对照 `release\RCC.exe` 实测；②标出与**现有语法**正面冲突的地方；③列出必须先裁决的问题。
> 本文**不包含任何自创语法设计**，所有待定项均标为「待裁决」。
>
> 审计基线：`release\RCC.exe`（2026-11-17 15:39:48，29879815 字节），HEAD `5db8171`。
> 每个结论都有可复现的探针（见文末「探针复现方式」）。

---

## 0. 结论摘要

决议文档共 16 节。逐条实测后分四类：

| 类别 | 数量 | 含义 |
|---|---|---|
| ✅ 现状已支持 | 6 | 可以直接用，无需改动 |
| ⚠️ 静默接受但无实现 | 7 | **最危险**：写了不报错，但没有语义，用户以为生效了 |
| ❌ 与现有语法冲突 | 5 | 必须改现有语法或改决议，二选一 |
| 🆕 全新能力（无任何基础） | 6 | 从零实现，工作量占 P13-2 大头 |

**两个必须优先知道的事实**：

1. **决议文档假设的四个基础构件，编译器里全都不存在**：类型标注块 `: T`、裸块作用域 `{ }`、`&` 取引用、以及 `Body`/`Content`/`Ref`/`Mut`/`Take` 五个类型名。文档 §1 说「遵循 Rio 现有风格，不另起一套语法体系」，但实测下来这几处是**新起体系**，不是复用。
2. **`ret` 和 `return` 现在是同一个 token**（`rcc_core.cpp:41` 与 `:54` 都映射到 `TOKEN_RETURN`）。决议 §13.1 要求二者分工（`ret` 退最近可返回值作用域 / `return` 只退函数），**在词法层就无法区分**，必须先拆 token。

---

## 1. 逐节对照

### §1 总体语法风格 — ❌ 前提不成立

主张：类型统一方括号 `Ref[T]` / `Mut[T]` / `Take[T]` / `Body[T]` / `Content[T]` / `Link[T][peer][cardinality]`。

| 探针 | 结果 |
|---|---|
| `var p: Body[Player] pool = Player("bob")` | ❌ `Unknown type 'Body' in declaration of 'p'` |
| `fun eat(x: Content[P])` | ❌ `Unknown type 'Content' of parameter 'x'` |
| `var r: Ref[P] = null` | ❌ `Unknown type 'Ref' in declaration of 'r'` |
| `var l: Link[U][fans][many] = null`（局部） | ❌ `Unknown type 'Link' in declaration of 'l'` |
| `class H { var l: Link[U][fans][many] }`（字段） | ⚠️ 编译通过（因为**类字段类型根本不校验**，见 §7 静默项） |

补充：`ctor` 和 `link` **已经是词法关键字**（`rcc_core.cpp:49` `TOKEN_CTOR`、`:51` `TOKEN_LINK`），但 `TOKEN_LINK` **没有注册到任何 parser 分发表**，所以 `link` 开头的表达式会直接报「找不到前缀构造函数」。`TOKEN_CTOR` 注册在 prefix map（`rcc_parser.cpp:73` → `buildConstructorExpression`）。

**裁决点**：五个类型名是「新增语言特性」还是「标准库类型」？`Pool` 文档 §3.4 明确说是标准库普通类型，但 `Body`/`Content`/`Ref`/`Mut`/`Take` 是编译器特殊语义类型，必须进 sema。

---

### §2 出生地 — ⚠️ 部分可用，但消歧机制与文档不符

| 主张 | 实测 | 判定 |
|---|---|---|
| 出生地写在 `var` 标签区，与类型标签空格分隔 | `var x: int pool = 42` 编译通过并打印 42 | ✅ 语法位**已存在** |
| 标签区顺序不敏感 | `var x: pool int = 42` 同样通过 | ✅ 已支持 |
| 消歧靠符号表（是 Pool 变量即出生地） | **否**。`typeFromLabelList`（`rcc_codegen.cpp:5686`）取**第一个**能解析成具体类型的标签，其余**静默丢弃** | ❌ 机制不符 |
| 只给出生地不给类型：`var c: pool = ...` | ❌ `Unknown type 'pool' in declaration of 'c'` | ❌ 与 §2.3 直接冲突 |
| 删除 `local` / `outer` 关键字 | `local`/`outer` **根本不是关键字**（词法表里没有），无从删除 | ✅ 无需动作 |

关键实测（证明是「首个命中」而不是符号表消歧）：

```
var x: str int = "hi"    → 通过，x 是 str（打印 hi）  ← 第一个标签赢
var x: int str = 5       → 通过，x 是 int（打印 5）   ← 第一个标签赢，str 被丢弃
var x: int pool = 42     → 通过（pool 被静默丢弃）
var x: pool int = 42     → 通过（pool 被静默丢弃）
var x: pool = 42         → 报错 Unknown type 'pool'   ← 没有类型标签兜底时暴露
```

**冲突点**：文档要求「靠符号表消歧」，现状是「靠位置取首个」。这意味着 `var p: Body[Player] pool = ...` 里的 `pool` 会被无条件丢弃——**出生地永远不生效**，而且不报错。这是 §2 全部语义的地基，必须先改。

---

### §3 Pool 与出生地 — 🆕 无基础

| 探针 | 结果 |
|---|---|
| `var pool: Pool = Pool.fixed(100)` | ❌ `Unknown type 'Pool'` |
| `Pool.fixed(2)` 直接调用 | ❌ `Cannot determine the object type of method call` |
| 任何 `ClassName.method()` 形式 | ❌ 语法错误（见 §5.2） |

Pool 类型、`fixed`/`boxed`/`chunked` 三种策略、R1/R2a/R2b 生命周期检查，全部为零。

---

### §4 类型名与 Body / Content — 🆕 无基础

| 主张 | 实测 | 判定 |
|---|---|---|
| `Player` 与 `Body[Player]` 同一类型（别名） | sema 无类型别名机制 | 🆕 |
| `Content` 必须显式写 | `Content` 类型不存在 | 🆕 |
| `var c: Player = b.take()` 必须编译错误 | 双标签类型不匹配**现在不报错**（`var x: str int = 5` 通过） | ❌ 无法保证 |

**注意**：最后一条是连带风险。只要「多个类型标签」仍然靠首个命中、不校验冲突，那么「Body vs Content 不匹配要报错」这个承诺就无法兑现。

---

### §5 `ctor` 与构造 — ⚠️ 一个静默错误 + 一个语法缺失

#### 5.1 「`fun ctor(...)` 必须编译报错」 — ⚠️ 实测**不报错，而且行为错**

```
class A {
    var v: int
    fun ctor(n: int) { this.v = n }
}
var a = A(7)
sout(a.v)     → 打印 0（不是 7）
```

编译退出码 0。`fun ctor` 被当成**普通方法**登记，`A(7)` 完全不调用它，字段保持零值。用户看到的是一次静默的语义失效——比报错危险得多。

#### 5.2 `Player.ctor("b")` 成员访问形式 — ❌ 语法错误

```
var c = P.ctor("bob")
→ Unexpected token: [<TOKEN_NEWLINE>] @ 7:26
→ Expected token type: TOKEN_LBRACE
```

原因：`TOKEN_CTOR` 只注册在 **prefix map**，`.` 之后不会走 prefix 分发。所以 `P.ctor(...)` 被解析成 `P` 后面跟一个 `ctor` 前缀表达式，然后期待 `{`。

同样地 `c.ctor("x")` 也是同一处报错（8:16）。

**这与 §5.2 的「`Player.ctor(...)` 是成员访问，走普通成员访问语法」直接冲突**——它现在连 parse 都过不去。

#### 5.3 `Body(x)` — ❌ 链接错误

`Body(x)` 能过 parse，最后在链接期报未定义符号（被当普通函数调用）。

#### 5.4 「解析走省略 `fun` 的函数定义，不是关键字特判」 — ❌ 与实现相反

实现就是关键字特判：`ctor` 是 `TOKEN_CTOR`，直接分发给 `buildConstructorExpression`。文档的措辞（「不是关键字特判」）与代码不符，需要在文档里改成描述现状，或改实现。

---

### §6 `take` 语义 — 🆕 无基础

| 探针 | 结果 |
|---|---|
| `p.take()` | ❌ `Class P has no method named 'take'` |
| `p.ref()` / `p.mut()` | ❌ 同上 |
| `var t = bob.take()` | ❌ 同上 |

Body 默认方法 `ref()`/`mut()`/`take()`、挂起态、落定四位置、语句末回填——全部为零，且挂起态需要在事实表里新增一个维度。

---

### §7 引用与能力 — ❌ `&` 两侧都缺

主张：`&` 前缀 = 取引用，`&` 中缀 = 位与，「按位置区分」。

实测：**两个位置都没有**。

| 探针 | 结果 |
|---|---|
| `&x` 前缀 | ❌ `failure to find the prefix expression construction function ... Error token type: TOKEN_BIT_AND` |
| `a & b` 中缀 | ❌ 同样不在 infix map |
| `&=` | ❌ 不在 infix map |
| `a \| b`、`a ^ b`、`a << 1`、`~a` | ❌ 全部不在 map（`TOKEN_BIT_OR` / `TOKEN_BIT_XOR` / `TOKEN_LESS` / `TOKEN_TILDE`） |

也就是说 Rio 目前**完全没有位运算**。文档 §7.2 说「`&` 作中缀 = 位与」，但位与本身不存在，所以「按位置区分」实际上是在**给一个不存在的运算让路**。

另外 `&Player ≡ Ref[Player]`（类型简写）需要类型系统支持 `&` 前缀类型语法，`fun f(r: &P)` 实测是语法错误。

**裁决点**：位运算是否属于 P13 范围？如果不属于，那么「`&` 中缀 = 位与」这条可以从文档删掉，`&` 就纯粹是取引用——反而更简单、无歧义。

---

### §8 子引用与局部排他 — 🆕 无基础

| 探针 | 结果 |
|---|---|
| `xs.split(2)` | ❌ `Cannot determine the object type of method call` |
| `xs.get_mut(0)` | ❌ 同上 |
| `xs.clone()` | ❌ 同上 |

根因：`list`/`dict` 是**内置类型，没有方法表**，任何 `.method()` 都落到「无法确定对象类型」。所以 `split`/`get_mut`/`clone` 三个方法需要先在 list 上开方法分发通道。

> **2026-11-18 已全部解决**：三个方法都在 `emitMethodCall` 里开了内建分支。`split` 最终定为 `split_at(k)` / `split_from(k)` 两次调用（Rio 没有元组），各返回一个 `Mut` 视图，路径 `xs.items[lo..hi)`，两段可共存，越界 panic。见 `pending_designs.md` §D。

NLL、路径登记、不等事实——全部为零。

---

### §9 签名标注 — ⚠️ **静默丢弃，且比文档更宽松**

这是本次审计里最危险的一条。

| 探针 | 结果 |
|---|---|
| `fun f(a: int): int from[a] { ... }` | ⚠️ 编译通过、运行正确，**但符号表里没有子句记录** |
| `writes[a]` / `ensures[a]` | ⚠️ 同上 |
| **`garbage[a, b]`**（完全瞎写的名字） | ⚠️ **同样编译通过** |
| `from[]`（空方括号） | ⚠️ **通过**（文档 §9.1 明令禁止） |
| `} from[a]`（写在函数体后） | ❌ 语法错误（文档 §9.1 要求「位置自由，} 后也可」） |

符号表 dump 证实子句被完全丢弃：

```
$ RCC.exe -s --p _q2.rio
Function (3)
  f(a: int): int     _q2.rio:2:1  asm=f      ← 没有 from 信息
  main(): void
  sout(*args: any, end: any = "\n"): void
```

**判定**：`from`/`writes`/`ensures` 目前是**纯装饰**，parser 吃掉不报错。任何用户现在写 `from[p.name]` 都会以为生效了，实际毫无作用。

**必须先做**：让未知子句名报错（`garbage` 不能过），再实现真实语义。否则 P13-2 会在一个「已经能写但不生效」的假地基上开工。

---

### §10 `fill` / `clear` / `release` — 🆕 无基础

| 探针 | 结果 |
|---|---|
| `p.fill(null)` | ❌ `Class P has no method named 'fill'` |
| `p.release()` | ❌ 同上 |

Body 的 F/E/H/X 状态机、drop glue、销毁插桩——P13-1 只做了「作用域末释放堆内存」（`core_dbg_allocations`/`core_dbg_frees` 计数器），**没有对象状态格**。

---

### §11 Content 赋值即移动 — 🆕 无基础

依赖 Content 类型（不存在）+ 移动标记（不存在）+ `clone()`（不存在，见 §8）。现状 `var a = b` 是浅拷贝，无任何所有权跟踪。

---

### §12 控制流与可返回值作用域 — ❌ **与现状正面冲突（最重要）**

#### 12.1 裸块 `{ ... }` 是作用域 — ❌ 实际被解析成字典字面量

`{` 在 prefix map 里固定分发给 `buildDictionaryExpression`（`rcc_parser.cpp:48`）。实测后果：

```
fun main(): void {
    {
        sout(1)      ← 从未执行
    }
    sout(2)
}
输出：2          ← 只有 2，sout(1) 消失了
```

```
var t = 0
{
    t = 5        ← 赋值没发生
}
sout(t)          → 0
```

```
var x = { 7 }
sout(x)          → 0x00000240c5b5aff0   ← 打印的是字典指针，不是 7
```

**冲突**：文档 §15.1 说「裸 `{ ... }` 块可作为独立语句，是局部作用域」，§12.1 说「裸块是可返回值作用域」。现状 `{ }` **既是字典字面量**，两者不可能同时成立。必须二选一（或引入区分规则）。

#### 12.2 `if (cond): T { ... }` 类型标注 — ❌ 语法错误，但 `if` 已经是值表达式

| 探针 | 结果 |
|---|---|
| `if (1 > 0): int { ret 7 } else { ret 8 }` | ❌ `Unexpected token: [<TOKEN_LABEL> "int"]`，`Expected token type: TOKEN_LBRACE` |
| `var x = if 1 > 0 { 7 } else { 8 }` | ✅ **通过，x = 7** |
| `var x = if 0 > 1 { 100 } else { 200 }` | ✅ 通过，x = 200 |
| `var x: int = if 1 > 0 { 7 } else { 8 }` | ✅ 通过 |
| `var x = if 1 > 0 { 7 }`（无 else，条件假） | ✅ 通过，x = 0（不是报错） |
| `var x = if 1 > 0 { 7 } else { "no" }` | ✅ 通过，x = 7（类型不匹配**不报错**） |

**这是本次审计最重要的发现**：`if` **已经是**可返回值表达式，机制是「分支块的最后一条表达式值」。文档 §12.2 却要求引入 `: T` 显式标注，并说「显式 `: T` 不是强制要求」。

于是 P13-2 面临一个必须裁决的问题：**两套值机制并存还是合并？**
- 现状隐式机制已经能跑，但**完全不校验**（无 else 返回 0、类型不匹配静默取首分支、void 分支也当值）。
- 文档的 `: T` 机制能校验，但与隐式机制重叠。

如果不裁决就动手，结果会是「两套机制、行为不一致」，比现在更糟。

#### 12.3 循环体 `ret value` — ❌ 与现状语义相反

文档 §12.1：「循环体块不是独立可返回值作用域，循环体内 `ret value` 直接退循环，等价 `break value`」。

实测：

```
fun f(): int {
    var i = 0
    while i < 5 {
        i += 1
        if i == 2 { ret 99 }     ← 退的是整个函数
    }
    ret 0
}
sout(f())    → 99
```

`ret` 在循环里**退的是函数**，不是循环。要改成文档语义，等于**改变现有 `ret` 的行为**——属于语言设计变更，必须获批。

#### 12.4 `while` 作表达式 — ⚠️ 能跑但语义可疑

| 探针 | 结果 |
|---|---|
| `var x = while i < 3 { i += 1 }` | 通过，x = 0 |
| `var x = while i < 3 { i += 1; break 42 }` | 通过，x = **1**（不是 42） |

`break 42` 的值被丢弃，`x` 拿到的是循环体最后一条表达式的值。文档 §12.2 要求「带类型标注的循环自然结束时值为 T 的零值」「`break value` 参与类型合流」——现状是第三套行为（取最后表达式值）。同一处又是三套语义打架。

#### 12.5 `for` 语法 — ⚠️ 与文档假设完全不同

实际语法是**花括号包三段式**：

```rio
for ({var i = 0; i < 3; i += 1}) {
    sout(i)
}
```

`for x in xs { }`、`for (var i = 0; i < 3; i += 1) { }` 都是语法错误。另有 `until cond { }`（等价 while）。

---

### §13 带值退出 — ❌ 词法层无法实现

#### 13.1 `ret` 与 `return` 是同一个 token

```
rcc_core.cpp:41   {"return", TokenType::TOKEN_RETURN}
rcc_core.cpp:54   {"ret",    TokenType::TOKEN_RETURN}
```

两者映射到**完全相同的 token**，parser 和 codegen 根本看不到区别。文档 §13.1 的三关键字分工表（`ret` 退最近可返回值作用域 / `return` 只退函数）**在当前词法层不可实现**，必须先拆成两个 token。这是 P13-2 的第一个硬前置。

#### 13.2 `break <value>` — ⚠️ 静默丢弃

| 探针 | 结果 |
|---|---|
| `while ... { if i == 2 { break 42 } }` | 通过，`break` 正常跳出，但 **42 被丢弃** |
| `break "abc"` | 通过，字符串也丢弃 |
| `break undefinedVariableXYZ` | ❌ `Undefined variable` ← 说明值**确实被解析和求值**了，只是结果扔掉 |

即：`break` 带值现在能写、能编译、能跑，但值无声消失。又是一个「写了以为生效」的陷阱。

---

### §14 循环与不动点 — ✅ 每轮新建已工作

`while` 体内 `var t = i * 10` 实测输出 `0 | 10 | 20`，每轮独立创建正常。P13-1 的循环内释放已有验收用例覆盖。

不动点/状态合流：🆕 无基础。

---

### §15 块作用域与并发 — ❌ 裸块问题见 §12.1；并发推迟

并发整段推迟，无异议。

---

### §16 暂定 / 推迟项

| 项 | 实测 |
|---|---|
| `Link[T][peer][cardinality]` 声明语法 | ⚠️ 类字段位置能编译（因字段类型不校验），局部变量位置报 `Unknown type 'Link'` |
| `int?` 可空类型 | ❌ 语法错误（`TOKEN_QUESTION` 存在于词法表，但**不在任何 parser 分发表**） |
| `Maybe` | 🆕 不存在 |
| `get()` | 🆕 不存在 |

文档 §16.2 倾向「`get()` 返回 `T?`，复用 Rio 现有可空类型」——但**实测 `int?` 根本不能写**，Rio 现在没有可空类型。这条倾向的前提不成立，Q3 的设计空间比文档假设的大。

---

## 2. 静默接受但无实现的清单（P0 级风险）

以下写法**现在都能编译通过、不报错、行为错误或无行为**。这类比「报错」危险得多，因为用户在写代码时得不到任何反馈：

| # | 写法 | 实际行为 |
|---|---|---|
| 1 | `fun f(): int from[a]` / `writes[...]` / `ensures[...]` | 子句被吃掉，无记录无校验 |
| 2 | `fun f(): int garbage[a, b]` | 同上，**任意标识符都接受** |
| 3 | `from[]` 空方括号 | 接受（文档明令禁止） |
| 4 | `var x: int pool = 42` | `pool` 被静默丢弃，出生地永不生效 |
| 5 | `break 42` | 值被求值后丢弃 |
| 6 | `class H { var f: Bogus }` | 类字段类型**完全不校验**（`Bogus`/`Body[H]`/`str` 都能过） |
| 7 | `fun ctor(n: int) { ... }` | 登记成普通方法，`A(...)` 不调用它，字段保持零值 |
| 8 | `if` 分支类型不匹配 / 缺 else | 静默取首分支 / 取零值，无校验 |

建议 P13-2 开工前先把 1–7 全部改成**明确报错**，把「静默」变「显式未实现」。这样后续每条语法落地时都能立刻验证，而不是在一堆假通过上叠加。

---

## 3. 与现有语法正面冲突的 5 处（必须二选一）

| # | 决议文档主张 | 现状 | 冲突性质 |
|---|---|---|---|
| C1 | `ret` 退最近可返回值作用域，`return` 只退函数 | 二者同 token，且 `ret` 在循环里退函数 | 需拆 token + 改变现有 `ret` 行为 |
| C2 | 裸 `{ }` 是作用域 / 可返回值作用域 | `{ }` 是字典字面量，作语句时内容不执行 | 二者互斥，必须定区分规则 |
| C3 | `if (c): T { }` 显式类型标注 | `if` 已靠「最后表达式值」隐式返回值，`: T` 是语法错误 | 两套值机制，需合并或分工 |
| C4 | `Player.ctor(...)` 走成员访问 | `ctor` 只在 prefix map，`.` 后不解析 | 需给 `ctor` 加中缀/成员分发 |
| C5 | `&` 前缀取引用 / 中缀位与 | 位运算整体不存在（`&`/`\|`/`^`/`<<`/`~` 全缺） | 位运算是否属 P13 范围待定 |

另有 3 处「文档措辞与实现不符」（不阻塞，但文档要改）：
- §5.1「解析走省略 `fun` 的函数定义，不是关键字特判」→ 实现就是关键字特判。
- §2.2「删除 `local`/`outer` 关键字」→ 二者从来不是关键字。
- §9.1「子句写在函数体 `}` 后亦可」→ 实测语法错误。

---

## 4. 必须先裁决的问题（不裁决就无法开工）

按阻塞程度排序：

**Q-A（阻塞 P13-2 全部）出生地与类型标签如何在 AST 层区分？**
现状 `typeFromLabelList` 取首个可解析标签、其余丢弃。文档要求靠符号表消歧。可选：AST 拆两个字段 / 保留一个列表但带标记 / 其它。→ 这决定 `var p: Body[Player] pool` 能否工作。

**Q-B（阻塞 §12/§13）`ret` / `return` 拆 token 与语义变更**
拆成两个 token 是纯实现动作（可批准），但「`ret` 在循环里退循环而非退函数」**改变现有行为**，属语言设计变更，需单独批准。

**Q-C（阻塞 §12）`if`/`while` 的值语义走哪套？**
现状隐式「最后表达式值」已在跑但不校验；文档要 `: T` 显式标注。三选一：保留隐式并补校验 / 只认 `: T` / 两者并存（`if` 用隐式、需要校验时写 `: T`）。**同一处现在已有三套行为（if 取分支末值、while 取末值、break value 丢弃），不裁决会变成四套。**

**Q-D（阻塞 §15）裸块 `{ }` 与字典字面量如何区分？**
可选方向（都需批准）：`{` 后首个 token 是否 `}` 或 `key:` 来区分 / 引入显式块关键字 / 裸块不作为语句（放弃 §15.1）。→ 这决定「裸块是局部作用域」这条能否成立。

**Q-E（阻塞 §7/§8）位运算是否纳入 P13？**
若纳入，`&` 的中缀/前缀歧义要设计优先级；若不纳入，建议从文档删除「`&` 中缀 = 位与」，`&` 纯作取引用，无歧义。

**Q-F（原 Q2/Q3/Q4，文档未覆盖的部分）**
- Q2 出生地语法：文档已给「标签区」形态，但依赖 Q-A。
- Q3 Maybe/窄化：文档 §16.2 的倾向（`get()` 返回 `T?`）前提不成立——**`int?` 现在不能写**，Rio 无可空类型。
- Q4 条件销毁诊断属性语法：决议文档**完全未覆盖**，仍是空白。

---

## 5. 建议的实施阶梯（仅排序，不含设计）

按依赖关系，P13-2 的开工顺序应当是「先拆掉假地基，再往上盖」：

**L0 去伪（不涉及语言设计，纯 bug 修复，可立即做）**
1. 未知签名子句报错（`garbage[a]` 不能过）
2. 类字段类型校验补上（`var f: Bogus` 报错）
3. `fun ctor` 报错
4. 多类型标签冲突报错（`var x: str int` 报错）
5. `break <value>` 报错（在实现前先明确不支持）

→ 做完这步，所有「静默」变「显式报错」，后续每条语法都能真验证。

**L1 语法地基（需要 Q-A/Q-B/Q-E 裁决）**
6. 类型标签与出生地标签在 AST 分离
7. `ret` / `return` 拆 token
8. 五个特殊类型名进 sema（Body/Content/Ref/Mut/Take）
9. `&` 前缀取引用（依赖 Q-E 结论）

**L2 结构语义（需要 Q-C/Q-D 裁决）**
10. 裸块作用域（依赖 Q-D）
11. 块表达式 / `: T` 标注（依赖 Q-C）
12. `break value` / `ret value` 类型匹配

**L3 内存模型主体（依赖 Q2/Q3/Q4）**
13. Body 状态格 F/E/H/X + 出生地 + 销毁插桩
14. Content / 移动 / `take` / `fill` / `clear` / `release`
15. Pool + `Link` + 生命周期树检查
16. 子引用 + NLL + 局部排他

**L4 诊断**
17. `from`/`writes`/`ensures` 真实校验
18. Maybe / 窄化（Q3）

---

## 6. 附：探针复现方式

所有探针用同一套最小骨架，避免 BOM 污染（`Set-Content -Encoding utf8` 会写 BOM 导致 valid 代码报假语法错误）：

```powershell
$stub = 'fun sout(*args, end="\n"): void { encapsulated }'
$enc  = New-Object System.Text.UTF8Encoding($false)
[System.IO.File]::WriteAllText("$PWD\tests\_x.rio", "$stub`n<被测代码>", $enc)
& release\RCC.exe -build --path tests\_x.rio --output tests\_x.exe
# 退出码 0 = 通过；随后运行 exe 看输出
```

签名子句是否被记录，用符号表 dump 判定（比看输出可靠）：

```powershell
& release\RCC.exe -s --p tests\_x.rio
```

本次审计产生的全部临时探针已清理，`tests\` 只剩 `test_closure.rio`（用户手写）与 `build_and_run.ps1`。

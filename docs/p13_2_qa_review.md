# P13-2 前置问题裁决的实测复核

> **本文性质**：对用户 Q-A～Q-F 六条裁决的**可行性复核**，不是设计提案。
> 每条给出：裁决内容 → 现状实测 → 影响面 → 结论（可行 / 已闭合 / 需新增检查）。
> 本文不含自创语法设计。凡需新设计的点均标「待裁决」。

复核基线：`release\RCC.exe`（2026-11-17 15:39:48），HEAD `5db8171`。
前置文档：[p13_2_syntax_audit.md](p13_2_syntax_audit.md)。

---

## 0. 总表（全部已裁决闭合）

| 裁决 | 内容 | 可行性 | 结论 |
|---|---|---|---|
| Q-A | 类型标签分内置/自定义；**不允许用户自定义同名 Pool** | ✅ 已决 | 判定顺序可闭合（见 §1） |
| Q-B | `ret` = 退最近作用域，循环里退循环 | ✅ 可行 | 标准库 **6 处**需改写；`ret`/`return` 需拆 token |
| Q-C | 写 `: T` 必须有返回值且类型匹配，否则报错；不写则自动推断 | ✅ 已决 | 优先级问题解决（见 §3） |
| Q-D | 按结构区分；空 `{}` 默认空字典；空字典可作表达式，空裸块不能赋值 | ✅ 已决 | 歧义闭合（见 §4） |
| Q-E | 位运算暂不管，保留提示项 | ✅ 已决 | 无 |
| Q-F | **只有引用类型可空**；基础类型用对象包装 | ✅ 已决 | 零值方案已废弃（见 §5） |

**P13-2 的六个前置问题全部闭合，可以进入实施。**

---

## 1. Q-A：类型标签分内置/自定义 — ⚠️ 清单未闭合

### 裁决内容
类型标签分为内置类型和自定义类型，`pool` 只是一个标识符；用户同时指出「AST 还不能区分自定义类型和出生地」。

### 现状实测

`typeFromLabelList`（`rcc_codegen.cpp:5686`）的判定逻辑：

```cpp
for (const auto& label : labels) {
    const auto t = typeFromLabelNode(label, genericParams);
    if (t && t->getKind() != sema::TypeKind::TK_UNKNOWN) return t;   // 首个命中就返回
}
return sema::getUnknownType();
```

后果（实测）：

| 写法 | 结果 |
|---|---|
| `var x: str int = "hi"` | ✅ 通过，x 是 str（**第二个标签静默丢弃**） |
| `var x: int str = 5` | ✅ 通过，x 是 int（同上） |
| `var x: int pool = 42` | ✅ 通过，pool 丢弃 |
| `var x: pool int = 42` | ✅ 通过，pool 丢弃 |
| `var x: pool = 42` | ❌ `Unknown type 'pool'` ← 无内置类型兜底时暴露 |

### 阻塞点：内置类型清单必须闭合

`resolveTypeName`（`rcc_codegen.cpp:49-85`）目前认这些名字：

```
void / bool / int / float / str / ptr / any / list(series,array) / dict(map)
```

`pool` 不在其中。用户说「pool 只是一个标识符」，但**判定一个标识符是「自定义类型」还是「出生地」需要一个确定的规则**，而这个规则依赖内置清单是否闭合。

关键问题：**自定义类型名与 Pool 变量名可以重名吗？**

```rio
class Pool { var n: int }        // 用户定义了一个叫 Pool 的类
fun main(): void {
    var mypool = Pool()
    var p: Body[Player] mypool = ...   // mypool 是出生地还是类型？
}
```

现状的 `typeFromLabelList` 遇到 `mypool` 会先问「这是不是类型」，如果不是类型才可能是别的——但**它根本不会走到「不是类型」的分支**，因为首个命不中就继续找下一个，全不中就报 Unknown type。

### 结论（已裁决闭合）

**用户第二轮裁决**：
1. 不允许用户自定义同名的 `Pool` 类型。
2. 可空类型只有引用类型可空，基础类型用**对象包装**表达。

**现状实测**：这两条目前**都没有任何检查**。

| 探针 | 结果 |
|---|---|
| `class Pool { var n: int }` | ✅ 编译通过并运行（**应报错**） |
| `class int { var n: int }` + `var x: int = 5` | ✅ 通过，`int` 仍解析成内置 int（**类定义被静默忽略**） |
| `class str { ... }` + `var s: str = "hi"` | ✅ 通过，`str` 仍是内置 str |
| `class list { ... }` + `var xs: list[int] = [1]` | ✅ 通过，`list` 仍是内置 list |
| `class Pool { }` 定义两次 | ✅ 通过（重复定义也没拦） |
| `class IntBox { var v: int }` + `var b: IntBox = null` | ✅ 通过，`b == null` 可判定（**对象包装方案可行**） |

**关键发现**：`resolveTypeName`（`rcc_codegen.cpp:49-85`）的内置类型名判定**优先于用户类**，所以 `class int` 定义后 `var x: int` 仍然解析成内置 int——用户类被**静默忽略**，连重复定义都不报。这是又一处「静默失效」。

**由此得到的标签区判定顺序（可直接实施，无需再裁决）**：

```
1. 内置类型名  void/bool/char/int/float/str/ptr/any/list/dict/func
2. 预声明类型名  Pool（用户不可自定义同名）
3. 用户自定义类型名（class 定义，且不得与 1、2 重名 → 新增检查）
4. 出生地变量名（必须是 Pool 类型的变量）
```

第 3 步需要**新增**「类名不得与内置/预声明类型名冲突」的检查（现状完全没有）。

**对象包装方案的可行性已验证**：

```rio
class IntBox { var v: int }
var b: IntBox = null
if b == null { ... } else { sout(b.v) }     → 输出 1，判定正确
```

因为 `IntBox` 是引用类型（指针），`null` = 0 指针，与任何有效对象地址不同——零值方案在值类型上的歧义问题**不再存在**。

---

## 2. Q-B：`ret` 退最近作用域 — ✅ 可行，但有 6 处存量要改

### 裁决内容
`ret` 通用表示退出最近的作用域；在循环里就是退循环。

### 现状实测

| 探针 | 结果 |
|---|---|
| `fun f(): int { while i<5 { i+=1; if i==2 { ret 99 } }; ret 0 }` | ✅ 通过，返回 **99**（退的是函数） |
| `fun f(): int { while i<5 { i+=1; if i==2 { return 99 } }; ret 0 }` | ✅ 通过，返回 99 |
| `while ... { if i==2 { break 42 } }` | 通过，跳出正常，**42 丢弃** |

**两个 token 是同一个**：
```
rcc_core.cpp:41   {"return", TokenType::TOKEN_RETURN}
rcc_core.cpp:54   {"ret",    TokenType::TOKEN_RETURN}
```

### 影响面（已精确量化）

标准库 `lib/` 下 **6 处 `ret` 位于循环内**，全部依赖「`ret` 退出函数」：

| 文件 | 行 | 代码 | 依赖语义 |
|---|---|---|---|
| `ds.rio` | 128 | `ret i`（`_dsKeyIndexOfStr` 循环内） | 找到即返回下标 |
| `ds.rio` | 142 | `ret i`（`_dsKeyIndexOfScalar`） | 同上 |
| `ds.rio` | 510 | `ret true`（`contains`） | 找到即返回 true |
| `ds.rio` | 526 | `ret i`（`indexOf`） | 找到即返回下标 |
| `str.rio` | 346 | `ret i`（`find`） | 找到即返回下标 |
| `str.rio` | 470 | `ret true`（`_strHasByte`） | 找到即返回 true |

典型形态（`ds.rio:116-133`）：

```rio
fun _dsKeyIndexOfStr(d: dict, key: str): int {
    var i: int = 0
    while i < n {
        if ks == key {
            ret i          ← 依赖「退出整个函数」
        }
        i += 1
    }
    ret -1
}
```

若 `ret` 改为退循环，这 6 处全部变成**跳出循环后继续执行 `ret -1`**，函数永远返回 -1。`contains`/`indexOf`/`find` 三个公开 API 会直接功能失效。

### 结论
**裁决可行**，但必须连带做两件事：
1. `ret` / `return` 拆成两个 token（纯实现动作）。
2. 这 6 处改成 `return`（因为它们的意图明确是「退出函数」）。

改法无歧义（意图清楚），不需要额外裁决。但要计入 P13-2 工作量，且改动后必须重跑 `test_p12_3c_ds.ps1` / `test_p12_3b_io_str.ps1` / `test_std.ps1`。

**注意**：用户裁决说「`ret` 通用表示退出最近的作用域」。那么函数体本身也是作用域，所以函数顶层 `ret` 仍退函数——这与决议 §13.1「函数体顶层 `ret` 退函数，不是特例」一致。只有「`ret` 在循环/裸块里」的行为改变。

---

## 3. Q-C：隐式末值 + `: T` 并存 — ⚠️ 需补充冲突规则

### 裁决内容
两套机制并存。

### 现状实测

隐式机制**已经在工作**：

| 探针 | 结果 |
|---|---|
| `var x = if 1>0 { 7 } else { 8 }` | ✅ x = 7 |
| `var x = if 0>1 { 100 } else { 200 }` | ✅ x = 200 |
| `var x = if 1>0 { 7 }`（无 else，条件假） | ✅ x = **0**（不报错） |
| `var x = if 1>0 { 7 } else { "no" }` | ✅ x = 7（**类型不匹配不报错**） |
| `var x: int = if 1>0 { 7 } else { 8 }` | ✅ x = 7 |
| `var x = while i<3 { i += 1 }` | ✅ x = 0 |
| `var x = while i<3 { i+=1; break 42 }` | ✅ x = **1**（42 丢弃，取末表达式值） |

显式 `: T` 机制**完全不存在**：

| 探针 | 结果 |
|---|---|
| `if (1>0): int { ret 7 } else { ret 8 }` | ❌ `Unexpected token: [<TOKEN_LABEL> "int"]` |
| `while i<3 { }: int` | ❌ 语法错误 |
| `for ({...}): int` | ❌ 语法错误 |

### 阻塞点：并存需要优先级规则

「并存」在实现上必须先回答：**同时出现两种信号时，谁赢？**

```rio
// 情况 1：标注与末值一致
var x: int = if (c): int { 7 } else { 8 }        // 无争议

// 情况 2：标注存在，但分支末值与标注不符
var x = if (c): int { "hello" } else { 8 }       // 报错还是取末值？

// 情况 3：标注存在，分支用了 ret value 而非末值
var x = if (c): int { ret 7 } else { ret 8 }     // ret 的值 vs 末值？

// 情况 4：标注存在，但某个分支无值（void）
var x = if (c): int { sout(1) } else { 8 }       // 报错？
```

另外还有 `break value` 与末值的关系（实测 `while { break 42 }` 取的是末表达式值 1，不是 42）。

### 结论（已裁决闭合）

**用户第二轮裁决**：写了 `: T` 表示**必须**有返回值，没有返回值或类型不一致就**编译报错**；没写 `: T` 则根据代码自动推断。

这条把「并存谁优先」的问题彻底解决了——不是并存竞争，而是**两条互斥路径**：

| 写法 | 语义 | 现状 |
|---|---|---|
| 写了 `: T` | 权威校验：所有正常出口必须产出 `T`，否则报错 | ❌ `: T` 语法不存在 |
| 不写 `: T` | 自动推断：取分支/循环体末表达式值 | ✅ 已工作，但**完全不校验** |

**现状隐式推断的三个已知漏洞**（实施时必须一并修）：

| 探针 | 现状 | 裁决后应改为 |
|---|---|---|
| `var x = if 1>0 { 7 }`（无 else，条件假） | x = 0，静默 | 保持（推断路径不强制） |
| `var x = if 1>0 { 7 } else { "no" }` | x = 7，类型不匹配不报错 | 保持推断但**应报类型冲突** |
| `var x = while i<3 { i+=1; break 42 }` | x = 1（42 丢弃） | `break 42` 应参与类型合流 |

**语法落点实测**：`if (1 > 0) : int { }` 报 `Unexpected token TOKEN_LABEL`，但 `if 1 > 0 : int { }`（无括号）能过 parse，最后在 codegen 报 `Unsupported expression in code generation: [Node(LABEL)]`。说明 `:` 作为 infix 运算符（映射到 `NodeType::PAIR`，见 `rcc_infix_builder.cpp`）已经能把 `int` 吃进去，只是 codegen 没处理——**实现落点明确，改动量小**。

---

## 4. Q-D：按结构区分裸块与字典 — ✅ 可行，但有 1 处真歧义

### 裁决内容
字典必须是键值对（键值用冒号分隔、键值对之间用逗号分隔）；裸块是若干语句，单行用分号分隔语句。

### 现状实测

字典字面量的**真实语法**（`buildDictionaryExpression`，`rcc_prefix_builder.cpp:474-490`）：

```cpp
next(); skipCurrentNewLineToken();
if (currentTokenIs(TOKEN_RBRACE)) return DictionaryExpressionNode(...nullptr);   // {}
auto bodyNode = buildExpression(currentTokenPrecedence());                        // 整个 body 当一个表达式
next(); skipCurrentNewLineToken();
if (!currentTokenIs(TOKEN_RBRACE)) { recordUnexpectedTokenTypeError(...); }
```

关键：**整个 `{ }` 内部被当成一个表达式解析**，靠 `:` 和 `,` 是 infix 运算符自然形成 `key: value, key: value` 的结构。

| 写法 | 结果 |
|---|---|
| `{ "a": 1 }` | ✅ 字典 |
| `{ 1: 2 }` | ✅ 字典（键可以是 int） |
| `{ "a": 1, "b": 2 }` | ✅ 字典 |
| `{}` | ✅ 空字典 |
| `{ "a": {"b": 1} }` | ✅ 嵌套 |
| `{a: 1}`（裸名键） | ❌ `Undefined variable: a`（键当变量求值） |
| `{ var x = 5 }` | ✅ **通过**（被当字典，body 是 `var` 表达式） |
| `{ 1+1: 3 }` | ✅ 通过（表达式键） |
| `{ ret 7 }` | ✅ 通过（当字典） |

### 按 Q-D 规则推导的区分表

| `{` 后首个 token | 判定 | 理由 |
|---|---|---|
| `}` | 空字典 | Q-D 未覆盖，需定（见下） |
| 形如 `expr :` | 字典 | 有冒号分隔键值 |
| `var` / `ret` / `sout(...)` 等语句开头 | 裸块 | 无键值对结构 |

### 唯一真歧义已裁决

**用户第二轮裁决**：默认空的为空字典；**空字典可以单独作为表达式存在，但空裸块不能用来赋值**。

实测印证：

| 写法 | 结果 | 裁决后 |
|---|---|---|
| `var d = {}` | ✅ 通过，是空字典 | ✅ 合法（空字典作表达式） |
| `var x = { }` | ✅ 通过，返回字典指针 `0x...` | ✅ 合法（同上，就是空字典） |
| 空裸块作右值 | 现状不存在（`{ }` 一律字典） | ❌ 应报错 |

因为「空裸块不能用来赋值」，所以 `var x = { }` 里的 `{ }` 只能是空字典——歧义消失，不需要额外的区分规则。

### 另一处需注意：分号在 `{ }` 内目前不可用

| 写法 | 结果 |
|---|---|
| `fun main(): void { var a = 1; var b = 2; sout(a+b) }` | ✅ 通过，输出 3 |
| `{ sout(1); sout(2) }` | ❌ 语法错误 |
| `{ var x: int = 5; sout(x) }` | ❌ 语法错误 |

**分号作为语句分隔符在函数体里已经能用**（因为函数体走 block 解析），但在 `{ }` 里不行——因为 `{ }` 被当字典，内部走 `buildExpression` 而非语句列表。

这恰好说明：**只要把裸块正确识别出来、让它走语句列表解析，分号就自然可用**。Q-D 的「单行用分号分隔语句」不需要新实现，复用现有分号逻辑即可。

### 结论
**可行**。建议明确两条（不必再走裁决）：
1. `{}` 一律是空字典，裸块至少一条语句。
2. 裸块识别后复用现有 block 解析路径，分号自动可用。

---

## 5. Q-F：新增可空类型，基础类型空值用零值 — ❌ 不可行

这是六条里唯一**有硬阻塞**的。用户要求我判断可行性，结论是不可行，理由如下。

### 裁决内容
新增可空类型功能，基础类型的空值使用零值。

### 实测证据：零值与合法值无法区分

| 类型 | 零值 | `x == null` 判定 | 问题 |
|---|---|---|---|
| `int` | 0 | **true** | **`0` 无法与「空」区分** |
| `bool` | false | **true** | **`false` 无法与「空」区分** |
| `float` | 0.0 | true | `0.0` 无法与「空」区分 |
| `str` | `""` | false（`""` 是有效对象） | 空串 ≠ null，但无法表示「没有字符串」 |
| `list` | `[]` | false | 空列表 ≠ null |
| `dict` | `{}` | false | 空字典 ≠ null |

实测代码与输出：

```rio
var z: int = 0
if z == null { sout(1) } else { sout(2) }      → 输出 1   ← 0 被当成 null
var b: bool = false
if b == null { sout(1) } else { sout(2) }      → 输出 1   ← false 被当成 null
var e: str = ""
if e == null { sout(1) } else { sout(2) }      → 输出 2   ← 空串不是 null
var xs: list[int] = []
if xs == null { sout(1) } else { sout(2) }     → 输出 2   ← 空列表不是 null
```

### 根本矛盾

「零值表示空」这个方案在**引用类型上成立**（null 指针 = 0，与任何有效对象地址不同），但在**值类型上必然失效**：

- `int` 的 0 是合法值（计数、下标、金额……）
- `bool` 的 false 是合法值（「没有」和「否」是两件事）
- 一旦规定 `0 == null`，那么 `var n: int = 0` 之后 `n` 就处于「空」状态，**所有算术都会遇到空值**，而语言又要求「无隐式运行时检查」（决议 §D5 的原则）。

### 三种可行替代（都需要裁决，我不自行选择）

**方案 1：只有引用类型可空（最小改动）**
- `int`/`bool`/`float` 不可空，`str`/`list`/`dict`/类/ptr 可空（用 null 指针）
- 优点：符合零值语义，无需额外存储
- 缺点：基础类型不能用 `T?`，`get()` 返回 `int?` 这类需求无法满足

**方案 2：所有类型可空，用独立标志位（真正区分）**
- `int?` 内部是 `{值, 是否有值}` 两字
- 优点：语义正确，`0` 和「空」能区分
- 缺点：所有可空类型体积翻倍/额外 8 字节；涉及 ABI 变更（P13-1 已因 ABI 变更提过审批）

**方案 3：Maybe 用标签联合（tagged union）**
- `Maybe[int]` = `{tag, value}`，tag 区分 None/Some
- 优点：语义最清晰，与决议 §16.2 的 `Maybe` 讨论一致
- 缺点：实现量最大

### 补充：`T?` 语法目前完全不能写

`TOKEN_QUESTION` 存在于词法表（`rcc_core.cpp:75`），但**不在任何 parser 分发表**（prefix/infix/postfix 三张表都没有它）。实测：

```
var x: int? = null        → 语法错误
fun g(): int? { ret 1 }   → 语法错误
```

所以 Q-F 不是「改造现有可空类型」，而是**从零新增**，语法、类型系统、codegen 全都要新做。

### 结论（已裁决闭合，零值方案废弃）

**用户第二轮裁决**：只有引用类型可空，基础类型用**对象包装**表达。

这直接绕开了零值方案的死结：

| 类型 | 可空 | 表示方式 |
|---|---|---|
| `str` / `list` / `dict` / 类 / `ptr` | ✅ 可空 | null 指针（= 0） |
| `int` / `bool` / `float` / `char` | ❌ 不可空 | 需要「可能没有」时用对象包装 |

对象包装的用法（已实测可行）：

```rio
class IntBox {
    var v: int
    ctor(x: int) { this.v = x }
}
var b: IntBox = null
if b == null { sout("没有") } else { sout(b.v) }    → 判定正确
```

因为 `IntBox` 是引用类型，`null` = 0 指针，与任何有效对象地址不同，**`0` 和「空」不再冲突**。

**仍需实现的两点**：
1. `T?` 语法本身（`TOKEN_QUESTION` 在词法表但不在任何 parser 分发表，需新增 prefix/postfix 分发）。
2. 引用类型的 null 判定与窄化（`if b == null { } else { b.v }` 后者能安全访问字段——这属于 Q3 的窄化机制）。

**注意**：`char` 类型也在内置清单里（`rcc_codegen.cpp:51`），但实测 `var c: char = "a"` 报类型不匹配（字符串字面量不是 char）。char 的正确字面量写法需另行确认，不属本轮范围。

---

## 6. 修订后的开工顺序

原审计的 L0（去伪）不受任何裁决影响，可以立即做。之后的顺序按本轮裁决更新：

**L0 去伪（可立即做，纯 bug 修复，共 7 项）**
1. 未知签名子句报错（`garbage[a]` 不能过）
2. 类字段类型校验（`var f: Bogus` 报错）
3. `fun ctor` 报错
4. 多类型标签冲突报错（`var x: str int` 报错）
5. `break <value>` 报错
6. 类名不得与内置/预声明类型名冲突（`class int` / `class Pool` 报错） ← Q-A 派生
7. 重复类定义报错（`class Pool` 定义两次） ← Q-A 派生

**L1 语法地基**
8. `ret` / `return` 拆 token ← Q-B 已批
9. 标准库 6 处 `ret`-in-loop 改 `return` ← Q-B 连带
10. `ret` 退最近作用域语义 ← Q-B 已批
11. 裸块识别 + 走语句列表（复用分号） ← Q-D 已批
12. 标签区判定顺序（内置 → Pool → 自定义 → 出生地） ← Q-A 已批
13. 五个特殊类型名进 sema（Body/Content/Ref/Mut/Take）

**L2 结构语义**
14. `: T` 标注（必须返回值 + 类型匹配，否则报错） ← Q-C 已批
15. `break value` / `ret value` 类型匹配

**L3 内存模型主体**
16. Body 状态格 + 出生地 + 销毁插桩
17. Content / 移动 / `take` / `fill` / `clear` / `release`
18. Pool + `Link` + 生命周期树
19. 子引用 + NLL

**L4 诊断与可选类型**
20. `from`/`writes`/`ensures` 真实校验
21. `T?` 语法 + 引用类型可空 + 窄化 ← Q-F 已批（仅引用类型）
22. 对象包装惯用法文档化（`class IntBox`） ← Q-F 派生

---

## 7. 裁决状态

**Q-A～Q-F 全部闭合，无遗留待裁决项。** 可以直接进入实施。

实施时需额外注意的新增检查项（由本轮裁决派生，不需再确认）：

| 检查 | 现状 | 依据 |
|---|---|---|
| 类名不得与内置类型名冲突（`class int` 应报错） | ❌ 静默忽略用户类 | Q-A「不允许自定义同名 Pool」 |
| 类名不得与预声明类型名 `Pool` 冲突 | ❌ 通过 | 同上 |
| 空裸块不得作右值 | 现状不存在 | Q-D |
| `: T` 必须有返回值且类型匹配 | ❌ 语法不存在 | Q-C |
| `T?` 语法 | ❌ 语法不存在 | Q-F |

**L0（去伪）的清单因此从 5 项增加到 7 项**——加上「类名与内置/预声明类型冲突」和「重复类定义」。这 7 项都是纯 bug 修复，不涉及语言设计，可立即开工。

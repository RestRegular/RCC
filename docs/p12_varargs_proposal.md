# 变长参数（`*args` / `**kwargs`）设计提案

> **状态：设计全部获批（Q1–Q7 + Q7-a/b/c），本次实施（2026-11-19）。**
> 用户 2026-11-16 裁决：
> - **Q1 = A 同质打包**（`*args` 收集成 `list[T]`，所有实参必须可赋给 `T`）
> - **Q2 = 强制标注，不豁免**（含 `sout` 这类编译器特化函数）
> - **Q8 = P13 之后做**，P13 打地基优先
>
> 用户 2026-11-19 补齐裁决：
> - **Q4 = A**：泛型首参定 `T`，其余按 `checkAssignable` 严格校验；
>   `pair(1, 2.5)` 报错，需显式 `pair[float](1, 2.5)`
> - **Q5 = A**：`**kwargs` 键固定 `str`（收集成 `dict[str, T]`）
> - **Q6 = A**：空调用允许，给空 `list[T]`（非 `null`、非报错）
> - **Q7 = 纳入本次范围**，并拆成三项子规则（Q7-a/b/c，全部选 A，见 §8.1）
>
> 本文其余部分保留为设计依据与待办清单。
>
> 提出时间：2026-11-16（P12-3 期间）
> 触发原因：用户要求「`lowlevel.ri` 不必实现（旧架构遗留），但 `*args` / `**kwargs`
> 本身要考虑清楚如何实现」，并点名四个必答问题：
> ①变长参数是否说明类型必须保持一致？②是否需要声明内部参数的类型？
> ③是否支持泛型？④（隐含）如何实现。

---

## 0. 结论速览

| 问题 | 结论 | 依据 |
|---|---|---|
| 变长参数类型必须一致吗 | **不必**，但新架构下**做不到异构** | 见 §3（元素槽恒 8 字节、无类型标签） |
| 需要声明内部参数类型吗 | **需要**，且应为**强制** | 不声明就按 int 推断，传 float/str 得到垃圾（§4） |
| 支持泛型吗 | **支持，但语义受限** | 空调用推不出 T；`T` 只能绑到单一具体类型（§5） |
| 实现方式 | 调用点打包成 `list[T]` / `dict[str,T]`，与 C 可变参数不同 | 原仓库就是这么做的（§2） |

**一句话**：变长参数在 Rio 里是「**同质**打包」，不是 C 的 `va_list` 异构透传。
这是新架构（元素内联、无装箱、无类型标签）的必然结果，**不是可以绕过的实现细节**。

---

## 1. 现状（实测，2026-11-16）

### 1.1 语法已通，语义全无

```rio
fun f(*args, **kwargs): void { sout("body") }
f(1, 2)          # ✅ 编译、运行，打印 body
```

形参表**解析**是通的（AST 里是 `UnaryExpressionNode(TOKEN_STAR / TOKEN_DOUBLE_STAR)`），
但函数体里引用 `args` 直接报错：

```
Error: Code generation failed: Undefined variable: args
```

**根因**：`collectParamNames` 不解包 `UnaryExpressionNode`，所以 `args` 这个**名字**
从未被登记成变量；更根本的是，调用点也没有任何「把变长实参收集成列表」的代码。

### 1.2 已核实的相关事实

| # | 事实 | 验证方式 |
|---|---|---|
| F1 | 形参表里 `*args` / `**kwargs` 能被解析，位置任意 | `-dump-ast` |
| F2 | `*args: int` 这种**带标注**的写法能过编译，但标注被完全忽略 | 探针 `va7` |
| F3 | 函数体里 `args` 不是变量（`Undefined variable: args`） | 探针 `va1` |
| F4 | 调用点不打包：实参按位置塞进寄存器，多余的**静默丢弃** | 探针 `va2`（修 arity 前） |
| F5 | 最多 4 个实参（寄存器上限），第 5 个报 `More than 4 arguments are not supported yet` | 探针 `ar5` |
| F6 | `*args` 后的关键字形参**带默认值**可用；调用点**按名字传**会报 `Undefined variable in assignment: end` | 探针 `kw2` / `kw3` |
| F7 | `list` / `dict` 元素槽**恒 8 字节**，存**内联裸值**，**无类型标签** | 源码 `rcc_codegen.cpp` |
| F8 | `list[any]` 存不了异构值：`[1, 2.5, "x"]` 读出 `1, 4612811918334230528, 1406...` | 探针 `any1` |
| F9 | `dict[str, any]` 同样存不了异构值 | 探针 `dic3` |
| F10 | `fun g[T](*args: T)` 在调用点报 `Cannot deduce type arguments ... (unspecified: T)` | 探针 `va8` |
| F11 | 闭包**按值**捕获，且写回无效（G6 已知） | 探针 `cl3` / `cl4` |
| F12 | 调用点展开 `f(*xs)` 不支持（被 arity 校验拒绝） | 探针 `sp1` |

**F8/F9 是最关键的发现**：新架构去掉了装箱，容器元素是内联裸值，
所以 `list` 和 `dict` 都是**同质**的。`any` 只是「不做静态类型检查」，
**不是**「运行期能存任何类型」——元素槽仍然只有 8 字节，放不下类型信息。

---

## 2. 原仓库是怎么做的（源码依据）

`rcc_function_call_compiler.cpp`（RioKit，已删除前版本）：

- `PARAM_VAR_LEN_POSITIONAL`（`*args`）：把剩余位置实参逐个收集进 `items`，
  然后 `TP_SET(list)` + `ITER_APND(items)` —— **打包成一个 list**。
- `PARAM_VAR_LEN_KEYWORD`（`**kwargs`）：把命名实参收集进 `items` —— **打包成一个 dict**。
- 类型检查**整段被注释掉了**：

  ```cpp
  // if (!checkTypeMatch(param, argItem)) { ... }
  ```

  注释里还留着一句提示：*"You can try using the `any` type to set the parameter types more loosely."*

**结论**：原仓库的 `*args` 就是**打包成 list**，不是 C 的 `va_list`。
它能接受异构实参，是因为 RA 时代的容器元素是 `shared_ptr<RVM_Data>` **装箱对象**
（带运行期类型标签）。新架构把装箱去掉了（这是 P5 的核心设计决定），
所以「异构打包」这条能力**在原理上就不成立了**，不是实现偷懒。

---

## 3. 问题①：变长参数的类型必须一致吗

**答案：语法上不必，但实现上必然一致——即只能收同类型实参。**

理由链（全部有实测依据）：

1. `*args` 要打包成容器 → 容器元素槽恒 8 字节（F7）
2. 8 字节放不下「值 + 类型标签」→ 无法在运行期区分 int / float / str（F8/F9）
3. 因此打包出的容器只能是**同质**的 → 所有实参必须是同一类型

### 3.1 三个可选方向

| 方向 | 做法 | 代价 | 我的评价 |
|---|---|---|---|
| **A. 同质打包** | `*args: T` 收集成 `list[T]`，所有实参必须可赋给 `T` | 异构调用报编译期错误 | ✅ **推荐**：与现有容器语义一致，零新增运行期机制 |
| B. 异构打包（恢复装箱） | 为变长参数专门引入带标签的 `ArgValue` 结构 | 新增运行期类型标签、与「去装箱」核心决定冲突、ABI 变更 | ❌ 不建议：等于为变长参数单独把装箱加回来 |
| C. 不做收集，只做透传 | `*args` 只能原样转给另一个 `*args` 函数（如 `sout`） | 函数体里仍拿不到 `args` | 部分可行，但无法满足「函数体里遍历参数」这个基本诉求 |

### 3.2 推荐 A 的具体语义

```rio
fun sum(*nums: int): int {
    var total: int = 0
    var i: int = 0
    while i < ds.size(nums) { total += nums[i]; i += 1 }
    ret total
}

sum(1, 2, 3)      # ✅ → 6，nums 是 list[int]
sum(1, "x")       # ❌ 编译期错误：cannot pass str as int
sum()             # ✅ → 0，nums 是空 list[int]
```

- `nums` 在函数体里的静态类型是 **`list[T]`**，可直接索引、`ds.size`、遍历。
- 实参逐个按 `T` 做可赋值性校验，**与普通形参同一套规则**（复用 `checkAssignable`）。
- 空调用得到**空列表**，不是 `null`——避免「忘了判空」。

---

## 4. 问题②：需要声明内部参数类型吗

**答案：需要，且建议设为强制（`*args` 必须带类型标注）。**

### 4.1 为什么必须

新后端对**未标注**的形参一律按 `int` 推断（`_functionParams` 回退逻辑）。
如果不强制标注：

```rio
fun show(*xs): void { sout(xs[0]) }
show(2.5)         # xs 被当成 list[int]，读出 4612811918334230528（double 位模式）
```

这是**静默错误**——不报错、结果垃圾。强制标注能让这类错误变成编译期诊断。

### 4.2 三个可选策略

| 策略 | 说明 | 优点 | 缺点 |
|---|---|---|---|
| **A. 强制标注** | `*args: T` 必须写 `: T`，否则编译错误 | 消灭静默错误；与 Rio「显式优先」一致 | 旧代码 `fun sout(*args, ...)` 需补标注 |
| B. 省略即 `any` | 不写标注等价于 `list[any]` | 兼容旧写法 | `list[any]` 存不了异构值（F8），是**假承诺** |
| C. 省略即推断 | 从调用点反推 `T` | 写法最省 | 跨函数前向调用时推不出；多处调用类型冲突无解 |

**推荐 A**，理由：B 会给出一个「看着能用、实际全是垃圾」的类型，比报错更糟；
C 在多调用点场景下无解（`f(1)` 和 `f("x")` 同时存在时 `T` 是什么？）。

### 4.3 对现有代码的影响

测试脚本里 16 处 `fun sout(*args, end="\n")` 桩都是**无标注**的。
用户已裁决「**强制标注，不豁免**」，所以这些桩在 P13 实施时需要补标注。

但 `sout` 本身是**编译器特化**（见 §4.4），它根本不会走到 `*args` 打包路径，
所以**补标注只是为了通过新规则，不影响 `sout` 的实际行为**。

#### 4.4 `sout` 到底是什么（2026-11-16 查证）

用户提问：「`sout` 的定义和声明到底在哪儿？它属于纯内置的函数吗？」

**答案：`sout` 是编译器硬编码的特化，不是纯内置函数，也不属于任何模块。**

证据链：

| 事实 | 验证 |
|---|---|
| 不写任何 `import` 也能用 `sout("hello")` | 探针 `si1` → 打印 `hello` |
| `print(...)` 不 import 则**链接失败** | 探针 `si4` → `ld returned 1 exit status` |
| `sout` 在 codegen 里被**按名字拦截** | `rcc_codegen.cpp` `emitCall`：`if (n == "sout") { emitSoutCall(node); return; }` |
| `emitSoutCall` 按每个实参的**静态类型**分别调 `rt_print_int/float/str/...` | 同上 |
| 仓库里 `fun sout(*args, end="\n")` 有 4 处声明（`io.rio` / `ds.rio`×2 / `stdio.rio`） | grep |

所以：

- **`sout` 的"声明"是 `emitCall` 里的一个字符串比较**，不在任何 `.rio` 文件里；
- `io.rio` / `ds.rio` / `stdio.rio` 里的那些 `fun sout(...)` 是**同名的普通函数声明**，
  它们**永远不会被调用**（`emitCall` 在解析后第一时间就拦截了 `sout` 这个名字）；
- 它们的存在价值只是让「`import io` 之后 `io.sout(x)` 能通过模块解析」，
  但实际执行仍然走特化路径（`emitModuleFunctionCall` 里也会落到同一个特化）。

**这与「纯内置函数」不同**：原架构的纯内置（`rcc_builtin_core.cpp` 里注册的那批）
是通过**符号表注册**、有正式的 `ParameterSymbol` 和签名，能被 `prog.entry` 之类
的机制内省；`sout` 只是一个 codegen 的 `if` 分支，没有符号表条目，不可内省。

⚠️ **这是架构债**：一个"函数"的实现散落在 codegen 的字符串比较里，
既不在 `lib/std/` 也不在 `lib/core/`，无法被文档、符号导出（`-s`）、
或类型系统看到。P12-3-B（`io` 模块）应该把它收编成一个明确的机制
（要么正式登记为内建、要么让 `io.sout` 真正走普通函数 + `rt` 实现）。

### 4.5 附带发现：`import` 处理也有问题（用户要求记录，暂不处理）

用户指示：「`import` 相关处理有问题，你记录一下就行，暂时不管这个问题」。

已知现象：

| 现象 | 探针 |
|---|---|
| `sout(sin(""))` 不 import 也能编译，打印 `337660345088`（垃圾值） | `si3` |
| 说明未定义函数被当作返回 `int` 的符号，链接期可能碰巧命中同名 C 符号 | 同上 |
| `print(...)` 不 import 则链接失败（`ld returned 1 exit status`），错误信息不含"未定义函数" | `si4` |

**问题定性**：调用一个**未 import、未定义**的函数时，codegen 不报「未知函数」，
而是按「未知函数返回 int」处理（`inferType` 末尾的保守回退），于是错误被推迟到
链接期，且错误信息对用户毫无指向性。

**暂不处理的理由**（用户裁决）：P12 范围已定，这个问题需要先决定
「未声明函数是错误还是隐式外部符号」，属语言设计，不在本阶段。

记录在此，P13 处理。


---

## 5. 问题③：支持泛型吗

**答案：支持，但有两个明确限制。**

### 5.1 限制一：空调用推不出类型实参

```rio
fun f[T](*xs: T): void { }
f()          # ❌ Cannot deduce type arguments for generic function 'f' (unspecified: T)
```

实测（探针 `va8`）已确认。这是**合理的**：没有实参就没有类型信息。
解决办法是**显式类型实参**：`f[int]()`。

但当前 `f[int]()` 这种写法在**变长**场景下尚未验证（`explicitTypeArgs` 只处理
普通泛型调用），需要实现时一并确认。

### 5.2 限制二：`T` 只能绑到单一具体类型

```rio
fun pair[T](*xs: T): void { }
pair(1, 2)        # ✅ T = int
pair(1, 2.5)      # ❓ T 取谁？int 还是 float？需定义规则
```

这与 §3 的「同质」要求是同一件事的两面。**建议规则**：
按普通泛型推导（第一个实参决定 `T`），后续实参按 `T` 做可赋值性校验，
不满足就报错。即 `pair(1, 2.5)` → 报错（int 收不下 float 的精度）。

⚠️ 但 `int` → `float` 的**拓宽**是否允许（`pair(1, 2.5)` → `T = float`）需要裁决，
见 §8 的 Q4。

### 5.3 泛型 + 变长的组合价值

它让 `math.max` 这类函数可以接受任意个参数：

```rio
fun maxOf[T](*xs: T): T { ... }
maxOf(1, 5, 3)          # → 5
maxOf(1.5, 2.5)         # → 2.5
```

但注意：`maxOf("a", "b")` 也会通过（str 支持 `>=`），这是泛型的正常表现。

---

## 6. 问题④：如何实现（技术方案）

### 6.1 调用点：打包

在 `emitArgsToRegisters` 之前插入打包步骤：

```
对每个变长形参：
  1. 收集对应的实参表达式（位置实参 → *args；命名实参 → **kwargs）
  2. 生成一个 list/dict 字面量节点（复用现有 emitListLiteral / emitDictLiteral）
  3. 把这个字面量当作**单个实参**传给形参
```

**关键约束**：打包出的容器**占一个寄存器**，所以 `fun f(a: int, *xs: int)`
在 ABI 层只有 **2 个**参数（`a` 和 `xs` 列表指针），不是 N+1 个。
这顺带**解决 F5 的 4 实参上限**——`f(1,2,3,4,5,6,7,8)` 打包后只占 2 个寄存器。

### 6.2 被调方：绑定名字

`collectParamNames` 需要解包 `UnaryExpressionNode`：

```cpp
if (const auto unary = std::dynamic_pointer_cast<ast::UnaryExpressionNode>(node)) {
    // *args / **kwargs 里的标识符是形参名，必须登记，
    // 否则函数体里引用 args 会报 Undefined variable
    if (auto id = std::dynamic_pointer_cast<ast::IdentifierNode>(unary->getRightNode()))
        out.push_back(id->getName());
    return;
}
```

形参类型登记为 `list[T]` / `dict[str, T]`，函数体里就能正常索引/遍历。

### 6.3 需要改的地方（工作量评估）

| 文件 | 改动 | 规模 |
|---|---|---|
| `rcc_codegen.cpp` `collectParamNames` | 解包 unary，登记名字 | ~8 行 |
| `rcc_codegen.cpp` `emitFunctionDefinition` | 变长形参类型登记为 `list[T]` | ~20 行 |
| `rcc_codegen.cpp` `emitArgsToRegisters` 或新函数 | 调用点打包 | ~60 行 |
| `rcc_codegen.cpp` `checkCallArity` | 已做（接受多余实参），需补「打包后只占 1 个槽」 | ~10 行 |
| `rcc_codegen.cpp` 泛型推导 | 变长形参的 `T` 推导 | ~30 行 |
| 文档 + 测试 | | ~200 行 |

**总计约 330 行**，其中核心逻辑 ~130 行。这是**一天左右**的工作量，不是一周。

### 6.4 与 G1b（argv 通道）的关系

实现变长参数**不等于**实现 `prog.entry` 的 argv 通道。两者的关系是：

- 变长参数解决「**函数怎么收**可变数量参数」（本文档）
- G1b 解决「**argv 怎么变成**参数」（`docs/p12_plan.md` §4.2.4，Q1–Q9 未裁决）

变长参数是 G1b 的**前置**（有了它，argv 才有地方放），但不替代 G1b 的设计裁决。

---

## 7. 对 `lowlevel.ri` 的影响（用户已裁决：不实现）

用户明确「`lowlevel.ri` 也是旧架构下的遗留库函数，没必要保留」。
所以变长参数**不需要**为了 `ri` 而实现。但变长参数本身仍有独立价值：

- `io.print(*args, sep=...)` —— 需要遍历参数（P12-3-B 会话 5）
- `math.maxOf(*xs)` 这类聚合函数
- 用户自己的 `fun log(*msgs: str)`

---

## 8. 裁决结果（2026-11-16）

| # | 问题 | 裁决 | 说明 |
|---|---|---|---|
| **Q1** | 打包语义 | ✅ **A 同质打包** | `*args` → `list[T]`，实参必须可赋给 `T` |
| **Q2** | 是否强制标注 | ✅ **强制，不豁免** | 含 `sout` 这类特化函数；16 处测试桩 P13 补标注 |
| **Q3** | 特化函数是否豁免 | ✅ **不豁免**（随 Q2） | 特化函数不走打包路径，补标注只是为过规则 |
| **Q4** | 泛型类型统一规则 | ✅ **A** | 首个实参定 `T`，其余按 `checkAssignable` 严格校验；`pair(1,2.5)` 报错 |
| **Q5** | `**kwargs` 键类型 | ✅ **A** | 固定 `str`，收集成 `dict[str, T]` |
| **Q6** | 空调用是否允许 | ✅ **A** | 允许，得到空 `list[T]`（非 `null`、非报错） |
| **Q7** | 调用点展开 `f(*xs)` | ✅ **纳入本次**，见 Q7-a/b/c | 用户 2026-11-19 明确「是本次任务的一部分，需要搞清楚」 |
| **Q8** | 实施时机 | ✅ **P13 之后，本次** | P13 先打地基优先，现已完成 |

#### Q7 调用点展开的三项子规则（2026-11-19 全部裁决为 A）

| # | 规则 | 裁决 |
|---|---|---|
| **Q7-a** | 展开目标 | **A**：`*xs` 对变长 callee 直接传 list（零成本）；对固定形参 callee 生成运行时长度检查（不符 panic）+ 逐元素 `xs[i]` 填寄存器 |
| **Q7-b** | 位置与数量 | **A**：`*xs` 只能在位置实参末尾，`**d` 只能在最后，各至多一个；中间穿插或多个展开编译报错 |
| **Q7-c** | `**kwargs` 展开 | **A**：`**d` 仅对有 `**kwargs` 的变长 callee 生效（直接传 dict）；展开到固定命名形参编译报错 |

**关键事实（探针实测 2026-11-19）**：`f(*xs)` 在 AST 里是 `UnaryExpressionNode(TOKEN_STAR, xs)`，
`collectArgs` 把它当**单个实参**不展开，所以 `g(*xs)` 现报「需要 3 个但给了 1 个」。
展开后的实参个数运行时才已知，而 MS x64 ABI 的寄存器位置编译期固定 —— 固定形参
展开必须生成运行时长度检查（Q7-a），这是该规则成立的实质原因。

### 8.2 实施清单（原 §8.1，2026-11-19 更新）

1. `collectParamNames` 解包 `UnaryExpressionNode`，登记 `*args` / `**kwargs` 的形参名
2. `emitFunctionDefinition` 把变长形参类型登记为 `list[T]` / `dict[str, T]`
3. 调用点打包（新函数或 `emitArgsToRegisters` 前置步骤）
4. `checkCallArity` 补「打包后变长形参只占 1 个寄存器槽」
5. 泛型变长的 `T` 推导
6. 强制标注校验：`*args` 无标注 → 编译期错误
7. 修 F6：`*args` 后关键字形参按名字传参报 `Undefined variable in assignment`
8. 16 处测试桩补标注
9. 文档更新 + 新增测试脚本
10. **Q7 调用点展开**（本次新增）：`collectArgs` 展开 `UnaryExpressionNode(TOKEN_STAR/DOUBLE_STAR)`；变长 callee 直接传容器；固定形参 callee 生成运行时长度检查 + 逐元素填寄存器；Q7-b 位置限制、Q7-c `**kwargs` 限制的诊断

> **现状核对（2026-11-19）**：清单第 1 项的一半已由旧代码完成 —— `collectVariadicFlags`
> （`rcc_codegen.cpp:367`）+ `VariadicInfo`/`_functionVariadic`（`rcc_codegen.h:963`）+
> `checkCallArity` 多余实参放行（`:10025`），但 `collectParamNames` 不解包 `*args`（`:269`），
> 函数体里 `args` 仍是未定义变量。其余项全未做。

### 8.3 顺带记录的架构债（不属 P12 范围）

- **`sout` 的实现散落在 codegen 字符串比较里**（§4.4），不在 `lib/std/`，
  无法被 `-s` 符号导出、文档或类型系统看到。P12-3-B 应重新设计 `io` 模块。
- **调用未定义函数不报错**（§4.5），错误被推迟到链接期且信息无指向性。
  需先决定「未声明函数是错误还是隐式外部符号」，属语言设计，P13 处理。

---

## 9. 如果不做，代价是什么

诚实记录，避免把「不做」说得比实际好：

- 函数体里**永远拿不到**变长实参（`args` 未定义）；
- 多余实参只能静默丢弃（**已在本次会话修为编译期报错**，这是改善）；
- `io.print(*args, sep=...)` 无法按 `print` 的语义实现，只能用编译器特化绕过；
- `prog.entry` 的 argv 通道继续无解（但那是 G1b 的独立裁决）。

---

## 10. 参考：本次会话已修的相关缺陷

| 缺陷 | 状态 |
|---|---|
| 实参过多/过少**静默通过** | ✅ 已修（编译期报错） |
| `*args` 后的关键字形参按名字传参报 `Undefined variable in assignment` | ⏳ 未修（F6，需与本文档一并设计） |
| 模块限定名调泛型链接失败 | ✅ 已修（`emitModuleFunctionCall`） |
| 模块限定泛型返回类型错 | ✅ 已修（`inferType`） |
| `null` 污染泛型推导 | ✅ 已修（`deduceTypeArgs`） |

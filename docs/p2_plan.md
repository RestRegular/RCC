# P2 阶段计划：函数值 / 匿名函数 / 闭包（对应路线图 P4）

> 前置：P1 已验收通过（`test_scripts/test_p0.ps1` 5/5、`test_scripts/test_p1_control.ps1` 13/13）。
> 本文件只描述本阶段的范围、设计、阻塞点与验收标准，不改动 `architecture_plan.md` 的总体路线。

---

## 一、本阶段要解决什么

`architecture_plan.md` §9.1 把 P4 定义为「函数：定义、递归、匿名函数、闭包、默认参数、命名参数」，
验收标准写作「test_6 / test_7 / test_9 / test_10 通过」。

P1 结束后，test_6 / test_9 / test_10 已经通过，**唯一还红的是 test_7_anonymous**。
所以本阶段真正要补的是三件事：

1. **匿名函数表达式** `(a, b) -> { ... }` 的 codegen；
2. **函数值**（一等公民）：`var f = (x) -> {...}`、把函数当实参传给高阶函数；
3. **间接调用** `f(...)`：被调者不是具名函数，而是槽位里的函数值。

闭包捕获（`(x) -> { ret x + n }` 里的 `n`）也一并做掉 —— 不做的话
`tests/test_15_forinrange` 的 lambda 形态（P8）还得回头重做一遍。

---

## 二、AST 形态（实测）

匿名函数有两种写法，都由 parser 造成 `AnonFunctionDefinitionNode`：

| 写法 | 构造点 | 说明 |
|---|---|---|
| `(a, b) -> { ret a + b }` | `rcc_prefix_builder.cpp:340`（`buildRangerExpression` 里的 `->` 检测） | 函数体是 `BlockRangerNode` |
| `(x) -> x * 2` | `rcc_infix_builder.cpp:64` | 裸表达式体，parser 自动包成 `ReturnExpressionNode` |

节点接口：`getParamNode()`（`ParenRangerNode` 包着逗号表达式）、`getBodyNode()`。
形参列表的结构与具名函数完全一致，所以 `collectParamNames` / `collectParamTypes` 可以直接复用。

---

## 三、设计

### 3.1 函数值的表示

`architecture_plan.md` 第 253 行约定 `func` 是 `{ fnptr, env }`，16 字节、按引用传递。

本阶段的落地方式是**闭包对象指针**：表达式的值就是指向一个 16 字节块的指针。

```
[0]  fnptr   —— 代码地址
[8]  env     —— 捕获环境块指针（无捕获时为 0）
```

- **无捕获**的匿名函数：闭包对象是静态常量，放 `.data`，`lea label(%rip), %rax` 一次搞定。
- **有捕获**的匿名函数：运行时用 `core_alloc` 分配 `16 + 8*捕获数` 字节，
  填 `fnptr`，把 `env` 指向块内 `16(%rax)`，再逐项把捕获值拷进去。
- **具名函数当函数值用**（`apply(add, 3)`）：按函数名去重生成一个静态闭包对象，
  标签 `.Lfnclosure_<name>`。

### 3.2 间接调用

```asm
    # 被调表达式求值 -> 闭包指针在 %rax
    pushq   %rax
    <装实参到参数寄存器>
    popq    %rax
    movq    8(%rax), %r10      # env
    movq    (%rax), %r11       # fnptr
    subq    $32, %rsp          # shadow space
    call    *%r11
    addq    $32, %rsp
```

`env` 走 `%r10`（调用者保存寄存器，不属于参数寄存器，不占参数位）。

**直接调用 / 间接调用的判定规则**（`emitFunctionCall`）：

| 被调者 | 判定 |
|---|---|
| 标识符 `sout` | `sout` 特化路径 |
| 标识符且是当前作用域里的局部变量 | **间接**（槽位里放着函数值，局部变量遮蔽同名函数） |
| 标识符且不是局部变量 | **直接**（含 `encapsulated` / extern，签名可能不在本文件里） |
| 任意其它表达式 | **间接** |

第三条曾经写成「必须是已知函数才直接调用」，结果构建期自举编译 `lib/rt/rt.rio` 时
报 `Undefined variable: core_io_write_byte` —— rt 里那些 `encapsulated` 外部函数
并没有在本文件登记。判据必须是「是不是局部变量」，不是「认不认识这个名字」。

### 3.3 函数体的延后发射

匿名函数出现在**表达式位置**，但函数体不能就地发射（会打断外层函数的指令流）。
所以 `emitAnonymousFunction` 只做三件事：

1. 收集形参、推断返回类型、收集自由变量；
2. 把函数体压进 `_pendingFunctions` 队列，表达式值取闭包对象地址；
3. 等 `emitProgram()` 把具名函数全发完，再由 `emitPendingFunctions()` 统一吐出。

队列用**下标遍历**而不是迭代器，因为匿名函数体里还可能再定义匿名函数，
遍历过程中 `_pendingFunctions` 会增长（vector 会重分配）。

`emitClosureObjects()` 排在 `emitPendingFunctions()` 之后、`emitStringLiterals()` 之前 ——
匿名函数体里可能产生新的字符串/浮点字面量，那些池子还要在后面统一输出。

### 3.4 类型推断

`inferType` 对 `AnonFunctionDefinitionNode` 返回 `FunctionType(参数类型, 返回类型)`。
这一条让 `var f = (x) -> {...}` 的槽位类型自动成为函数类型，
调用点就能从槽位类型里拿到**返回值类型**和**形参类型**，不需要额外的登记表。

未标注类型的形参不能一律当 int：`(x) -> { ret x / 2.0 }` 里的 `x`
如果按 int 处理，实参就会走 `%rcx` 而不是 `%xmm0`，结果直接错（实测输出 `0.5` 而不是 `2.5`）。
做法是：

1. 形参类型先留 `TK_UNKNOWN`；
2. `inferAnonReturnType` 把形参以 UNKNOWN 绑进影子作用域，从函数体找第一个 `ret`，
   推断返回类型时顺带把形参类型定下来；
3. 仍为 UNKNOWN 的才回退 int。

### 3.5 闭包捕获（按值）

`collectFreeVariables` 递归遍历函数体，收集「用到但不是形参、也不是自己 `var` 定义的」名字。
嵌套匿名函数的函数体**不深入** —— 它的捕获在它自己的创建点解析。

捕获是**按值拷贝**：

- 创建点：把当前槽位的值拷进 `env` 块的第 `i` 项；
- 被调方序言：`%r10` 指向 `env`，逐项拷进本地槽位，函数体随后按普通局部变量访问。

按值而不是按引用，是为了让 `makeAdder(5)` 返回后即使栈帧已经销毁，
闭包里的 `n` 依然是 `5`。代价是闭包内对捕获变量的修改不影响外层 ——
这一限制记录在 §六。

---

## 四、实测阻塞点与修法

| 现象 | 根因 | 修法 |
|---|---|---|
| `Unsupported expression: Node(ANON_FUNCTION_DEFINITION)` | `emitExpression` 没有这个分支 | 加 `emitAnonymousFunction` 分派 |
| `Unsupported expression: Node(PREFIX)` | `-x` 是 `PrefixExpressionNode`，与 `UnaryExpressionNode` 是**两类节点** | 加 `PREFIX` 分支，复用 `UnaryExpressionNode` 的取反逻辑 |
| `Undefined variable: core_io_write_byte`（构建期自举） | 直接调用判定写成「必须是已知函数」，把 rt 的 encapsulated 外部函数当成间接调用 | 改成「不是局部变量就按直接调用」 |
| `anon_return_float` 输出 `0.5`（应为 `2.5`） | 未标注类型的形参一律按 int，实参走错寄存器 | `inferAnonReturnType` 反推形参类型 |
| `Undefined variable: n`（闭包捕获） | 只做无捕获闭包 | `collectFreeVariables` + env 块 |

---

## 五、风险与缓解

| 风险 | 影响 | 缓解 |
|---|---|---|
| 闭包对象用 `core_alloc` 分配，但没有任何释放路径 | 长循环里创建闭包会漏内存 | 记录在案；引用计数统一在 P5 的值表示层做，本阶段不做半套 |
| 捕获按值，闭包内改捕获变量不影响外层 | 与某些语言语义不同 | 文档明确写出；需要写回时用显式可变容器 |
| `%r10` 传递 env 是本项目的自定义约定 | 与 C ABI 不可互操作 | 闭包只在 Rio 内部传递；跨 C 边界的只有具名函数 |
| 形参超过 4 个仍不支持 | 高阶函数参数多时受限 | 报错信息明确（`More than 4 arguments are not supported yet`） |
| 返回类型只取「第一个 ret」 | 分支返回不同类型的函数会判错 | 当前测试都是同类型返回；完整流分析推到后续阶段 |

---

## 六、已知限制

1. **闭包捕获是按值**：`(x) -> { n = n + 1 }` 不会改到外层的 `n`。
2. **闭包对象不释放**：每个捕获闭包泄漏 `16 + 8*捕获数` 字节。
3. **捕获只认当前作用域的局部变量**：全局变量、具名函数不进入 `env`，
   而是走直接调用/直接寻址路径。
4. **形参上限 4 个**（整型与浮点各自 4 个）。
5. **返回类型取第一个 `ret`**，不做完整控制流分析。

---

## 七、验收结果（2026-11-15）

P2 完成，四套测试同时通过：

| 套件 | 脚本 | 结果 |
|---|---|---|
| P0 回归 | `test_scripts/test_p0.ps1` | **5 / 5** |
| P1.2 控制流 | `test_scripts/test_p1_control.ps1` | **13 / 13** |
| P2 函数值（本阶段） | `test_scripts/test_p2_functions.ps1` | **10 / 10** |
| legacy 全量 | `test_scripts/test_legacy_all.ps1` | **10 / 17** |
| legacy test_7_anonymous | `tests/test_7_anonymous/test.rio` | **通过**（`8 / 20 / 25`） |

legacy 测试分布从 P1 的 9/17 变为 **10/17**，新转绿的是 test_7_anonymous。

完整 17 个 legacy 测试的分布（`test_scripts/test_legacy_all.ps1` 逐字节比对 `expected.txt`）：

| 测试 | 状态 | 阻塞点 | 归属 |
|---|---|---|---|
| test_1_builtins | 通过 | — | — |
| test_2_variables | 通过 | — | — |
| test_3_literals | 通过 | — | — |
| test_4_comparisons | 通过 | — | — |
| test_5_conditions | 通过 | — | — |
| test_6_functions | 通过 | — | — |
| test_7_anonymous | **通过（本阶段新转绿）** | — | — |
| test_8_collections | 失败 | `Node(RANGER)` —— dict/list 字面量 | P5 |
| test_9_integration | 通过 | — | — |
| test_10_main | 通过 | — | — |
| test_11_throw | 失败 | `Node(THROW)` | P7 |
| test_12_trycatch | 失败 | `Node(TRY)` | P7 |
| test_13_compound_assign | 通过 | — | — |
| test_14_iter | 失败 | `Undefined variable: import` | P8 |
| test_15_forinrange | 失败 | `Undefined variable: import` | P8 |
| test_16_class_basic | 失败 | `Undefined variable: x`（成员访问） | P6 |
| test_17_class_inheritance | 失败 | `Undefined variable: val`（成员访问） | P6 |

`test_scripts/test_p2_functions.ps1` 覆盖的形态（10 个用例）：

| 用例 | 覆盖点 |
|---|---|
| `anon_basic` | 两个匿名函数绑定到 `var` 并调用 |
| `anon_as_arg` | 匿名函数直接当实参传给高阶函数 |
| `named_as_value` | 具名函数名当函数值传递（`.Lfnclosure_*`） |
| `anon_bare_expr` | 裸表达式体（无大括号） |
| `anon_return_float` | 返回浮点的匿名函数，形参类型反推 |
| `anon_if_body` | 匿名函数体里带 `if`，且用到前缀 `-x` |
| `anon_loop_body` | 匿名函数体里带 `while` |
| `anon_nested` | 嵌套匿名函数 + 闭包捕获 |
| `anon_inline_call` | 匿名函数表达式就地调用 `((a,b) -> {...})(6,7)` |
| `anon_multi_args` | 4 个参数，全走整型寄存器 |

补充两点实测结论：

1. **`-x` 是 `PrefixExpressionNode` 而不是 `UnaryExpressionNode`**。
   parser 的 `buildPrefixExpression` 造的是前者，`buildUnaryExpression` 造的是后者，
   两者在 `emitExpression` 里必须各有一个分支。
2. **`inferType` 对函数值变量返回 `FunctionType`**，调用点从槽位类型里同时拿到
   返回类型和形参类型。这是本阶段能正确处理 `(x) -> { ret x / 2.0 }` 的关键 ——
   不需要额外的「变量名 -> 函数签名」登记表。

---

## 八、下一步

按 §9.2 依赖链，P2 之后是 P3（运算符与类型转换）→ P4（本轮已完成）→ P5（容器）。

本轮实际推进的是路线图里的 **P4**，按 §9.1 的验收标准（test_6/7/9/10 通过）已全部满足。
剩余 7 个 legacy 测试的归属见 §七 的表格：test_8→P5，test_11/12→P7，
test_14/15→P8，test_16/17→P6。

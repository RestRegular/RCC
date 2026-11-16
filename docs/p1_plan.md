# P1 阶段计划：值表示层

> 前置：P0 已验收通过（`test_scripts/test_p0.ps1` 5/5）。
> 本文件只描述 P1 的范围、阻塞点与验收标准，不改动 `architecture_plan.md` 的总体路线。

---

## 一、P1 要解决什么

`architecture_plan.md` §9.1 给 P1 的定义是「值表示：`rt.rio` 的分配器、`str`/`list` 底层、`sout` 打印」，
验收标准写作「test_1、test_3 通过」。

实测之后，这个定义需要收敛。原因是 **P0 的 codegen 覆盖面比预期窄**，17 个 legacy 测试在
到达「值表示」之前就被更基础的缺失卡住了。P1 的实际目标是：

> 让 17 个 legacy 测试里**不依赖匿名函数、容器、类、异常、模块**的那 9 个全部转绿。

具体是 test_1 / 2 / 3 / 4 / 5 / 6 / 9 / 10 / 13。

---

## 二、当前阻塞点实测（2026-11-15）

用 `release/RCC.exe -build` 逐个跑 `tests/test_*/test.rio`：

| 测试 | 阻塞点 | 归属 |
|---|---|---|
| test_1_builtins | `sout` 只支持字符串字面量 | P1.3 |
| test_2_variables | `sout` 同上 | P1.3 |
| test_3_literals | `sout` 同上 | P1.3 |
| test_4_comparisons | `&&` `\|\|` `!` 未实现 + `sout` | P1.1 + P1.3 |
| test_5_conditions | 只差 `sout` 没有自动换行 | P1.3 |
| test_6_functions | 递归可跑，`sout` 挡住 | P1.3 |
| test_7_anonymous | 匿名函数 / 闭包 | P4 |
| test_8_collections | dict 字面量与索引 | P5 |
| test_9_integration | 递归可跑，`sout` 挡住 | P1.3 |
| test_10_main | 用户 `fun main()` 与生成的 `main` 符号冲突 | P1.1 |
| test_11_throw | `throw` | P7 |
| test_12_trycatch | `try/catch/finally` | P7 |
| test_13_compound_assign | `+= -= *= /=` 未实现 + `sout` 多参数 | P1.1 + P1.3 |
| test_14_iter | `import ds` + 迭代器 | P8 |
| test_15_forinrange | `ds.forInRange(...)` 函数调用（**不是** for-in 语法，原仓库无此语法） | P8 |
| test_16_class_basic | class | P6 |
| test_17_class_inheritance | class + 继承 | P6 |

**所有 17 个测试都先撞在同一个点上**：每个 legacy 测试开头都有

```rio
fun sout(*args, end="\n"): void {
    encapsulated
}
```

而 codegen 不认识 `encapsulated`（报 `Node(PREFIX)` 不支持）。这一条必须先修，否则看不到真实阻塞点。

---

## 三、P1 拆成三个可独立验收的子阶段

### P1.1 表达式与控制流补齐（不含 rt）

目标：把 codegen 的表达式覆盖补到「普通程序写得出来」的程度。

| 项 | 现状 | 做法 |
|---|---|---|
| `encapsulated` 函数体 | `Node(PREFIX)` 报错 | 视为「外部实现」：登记签名、**不发射函数体**，调用点照常生成 `call` |
| 用户 `fun main()` | 与 wrapper 的 `main` 冲突 | wrapper 改名，用户 `main` 改发射为 `__rio_user_main`，由 wrapper 调用 |
| `%` | `Unsupported binary operator` | `cqto` + `idivq`，取 `%rdx` |
| `&&` `\|\|` | 同上 | 短路求值（`testq` + 跳转），不做纯算术 |
| 一元 `-` | 已可用（`Node(UNARY)` 走前缀路径） | 复核 |
| 一元 `!` | `Node(UNARY)` 报错 | `testq %rax,%rax` + `sete` |
| `++` / `--` | `Node(POSTFIX)` 报错 | 读槽 → 加减 1 → 写回，结果留 `%rax` |
| `+= -= *= /= %=` | `Unsupported binary operator` | 展开为「读槽 → 运算 → 写槽」 |
| `while` / `until` | `Node(LOOP)` 报错 | 条件回跳 + `_loopStack` 维护 |
| `for (init; cond; update)` | 同上 | 三段式展开 |
| `break` / `continue` | 未实现 | 从 `_loopStack` 取标签 |

验收：`test_4`、`test_10`、`test_13` 能编译并生成可执行文件（输出内容仍受 sout 限制，属 P1.3）。

### P1.2 循环验收用例

`test_5_conditions` 已覆盖 if/elif/else，循环没有现成用例。新增
`test_scripts/test_p1_control.ps1`，覆盖：while、until、C 形式 for、break、continue、嵌套循环。

验收：脚本全绿。

### P1.3 rt 值表示与 `sout` 全类型打印

这是 P1 的正文。

**顺序修正（2026-11-15 决策）**：`architecture_plan.md` §7.1 要求「rt 从第一天起就用 Rio 写」，
用户确认**不做汇编 bootstrap**，直接等控制流可用后用 Rio 写 rt。

而 P1.1 本身就包含 `while`/`until`/`for`/`break`/`continue`——也就是 P3 的控制流内容。
因此实际执行顺序调整为：

```
P1.1（表达式 + 控制流）──► P1.2（循环验收）──► rt 用 Rio 写（原 P3 的 rt 部分）──► P1.3（sout 全类型）
```

好处：控制流一落地，rt 立刻可以用 Rio 写，不需要任何汇编过渡层，
也不会出现「写完汇编 rt 又要扔掉重写」的双份工作量。
`architecture_plan.md` §9.1 里 P3 的控制流项事实上并入 P1.1，P3 只剩「rt 落地」。

| 项 | 做法 |
|---|---|
| `rt_print_int` | i64 → 十进制 ASCII，写 `core_io_write`；负数、0、i64 最小值都要对 |
| `rt_print_float` | f64 → 十进制，先支持 `%g` 风格的最短表示（3.14 → `3.14`） |
| `rt_print_bool` | 输出 `true` / `false` |
| `rt_print_str` | `StrHeader*` → 取 `len` + `data`，写 `core_io_write` |
| `sout` 多参数 | 逐参数按**静态类型**分派到对应 `rt_print_*`，无分隔符 |
| `sout` 的 `end=` | 默认追加 `"\n"`；显式 `end="x"` 时追加 `x` |
| `sout(null)` | 输出 `null` |
| rt 源码 | `lib/rt/rt.rio`，**纯 Rio**，用 P1.1 的循环实现十进制转换 |
| rt 产物 | `release/lib/rt/rt.o`，由 RCC 自身编译 `rt.rio` 得到 |
| 链接列表 | `toolchain.link({objPath, coreObject, rtObject}, ...)` |

**rt 必须用 Rio 写，且由 RCC 自己编译**。这意味着 P1.3 之前要先验证
「RCC 能编译一个不含 `encapsulated`、只用循环和算术的 Rio 文件并产出可用 `.o`」——
这一步本身就是最有价值的自举前置验证。

**静态类型分派**：`sout` 的实参类型在编译期可判定（字面量类型、变量槽类型、函数返回类型、
二元运算结果类型），不需要运行时 tag。这是 §4.2「静态化后每个类型有原生表示，不再有 tag」的直接结果。

验收：`test_1 / 2 / 3 / 4 / 5 / 6 / 9 / 10 / 13` 全部与 `expected.txt` 逐字节一致。

---

## 四、P1 完成后的状态

- 17 个 legacy 测试中 **9 个转绿**（1、2、3、4、5、6、9、10、13）。
- 剩余 8 个的阻塞点全部落在 P4–P8，与计划一致：
  test_7 → P4，test_8 → P5，test_11/12 → P7，test_14/15 → P8，test_16/17 → P6。
- `rt` 层有第一版实现：`lib/rt/rt.rio`，**纯 Rio**，由 RCC 自身编译成 `release/lib/rt/rt.o`。

---

## 五、风险

| 风险 | 影响 | 缓解 |
|---|---|---|
| `sout` 的 `end=` 是关键字参数，AST 形态未确认 | P1.3 可能卡在参数解析 | 先 dump AST 确认，再写 codegen |
| 浮点格式化精度 | test_3 只要求 `3.14` | 先做最短表示，`%g` 级别的完整实现推到 P5 |
| `encapsulated` 跳过函数体后，若用户程序**没有**对应 rt 实现会链接失败 | 报错信息不够友好 | 链接失败时提示「该函数声明为 encapsulated，但未链接对应实现」 |
| rt 用 Rio 写，而 Rio 版 rt 依赖的循环刚在 P1.1 落地 | 循环若有 bug，rt 直接受影响 | P1.2 先用脚本把循环验透，再写 rt |
| rt 由 RCC 自己编译，等于第一次「自举前置」验证 | 编译器缺陷会在 rt 上暴露 | 这是好事：早暴露早修 |

---

## 六、执行顺序

```
P1.1（表达式/控制流/encapsulated/main）──► P1.2（循环验收脚本）──► rt.rio（Rio 实现）──► P1.3（sout 全类型）
```

P1.1 先做，因为它把「看不到真实阻塞点」这个障碍清掉；
rt.rio 排在 sout 之前，因为 sout 的类型分派需要 rt 的打印函数存在。

---

## 七、验收结果（2026-11-15）

P1 全部完成，三套测试同时通过：

| 套件 | 脚本 | 结果 |
|---|---|---|
| P0 回归 | `test_scripts/test_p0.ps1` | **5 / 5** |
| P1.2 控制流 | `test_scripts/test_p1_control.ps1` | **13 / 13** |
| P1.1 legacy | 9 个目标测试逐字节比对 `expected.txt` | **9 / 9** |

17 个 legacy 测试的完整分布：

- **通过 9 个**：test_1 / 2 / 3 / 4 / 5 / 6 / 9 / 10 / 13
- **未通过 8 个**，阻碍点与 §四 的规划一致：
  - test_7_anonymous —— 匿名函数（P5）
  - test_8_collections —— list/dict 字面量（P3）
  - test_11_throw、test_12_trycatch —— 异常（P4）
  - test_14_iter、test_15_forinrange —— `import` + lambda（P5）
  - test_16_class_basic、test_17_class_inheritance —— 类与成员访问（P6）

P1.2 覆盖的控制流形态（`test_scripts/test_p1_control.ps1` 13 个用例）：
`while` / `until` / C 风格 `for` / `break` / `continue` / 嵌套循环里的
break 与 continue 只作用于内层 / `elif` 链 / 浮点循环变量。

补充两点实测结论：

1. **C 风格 for 的语法**是 `for ({init; cond; update}) { body }` ——
   三段表达式用分号隔开、整体再包一层大括号，不是 C 的 `for (init; cond; update)`。
2. **`continue` 在 for 里必须跳到 update 段**，不能直接跳条件判断，
   否则会跳过 `i = i + 1` 造成死循环。实现上 `LoopContext.continueLabel`
   对 for 取 update 标签，对 while/until 仍取条件标签。

### 浮点打印口径

`sout` 的浮点输出取「最短形式但保留类型痕迹」：

- `3.14` → `3.14`（去掉尾部 0，不输出 `3.140000`）
- `2.5` → `2.5`
- `1.0` → `1.0`、`0.0` → `0.0`（整数形态的 float 补 `.0`，
  否则 `sout(1.0)` 与 `sout(1)` 输出无法区分）

legacy 的 expected 里只有 `3.14` 一种形态，`.0` 是 P1 新定的口径，
完整 `%g` 级最短表示（含正确舍入）留到 P5。

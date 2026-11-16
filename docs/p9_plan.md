# P9 —— 静态化（类型标注权威化 + 类型错误诊断）

> 目标：把「类型标注」从装饰变成契约 —— 写了标注，编译器就按它分配槽位、
> 按它插入转换、并在不匹配时报错；没写标注的地方维持推断，不破坏既有代码。
> 验收：`test_scripts/test_p9_types.ps1`（20 个用例）全绿 + legacy 全量不回退
> （仍为 15/17，`test_14_iter` / `test_15_forinrange` 属 P12 范围）。

## 1. 为什么是「标注权威 + 推断兜底」，而不是「注解必填」

`architecture_plan.md` §9.1 对 P9 的描述是「移除 `any`；类型注解必填 + 推导」。
真正动手时「注解必填」这一条必须降级，理由是硬事实而不是偏好：

- legacy 17 个测试里，用户写的 `fun` 形参、局部 `var`、闭包回调参数**全都没有标注**。
  `test_2_variables` 是 `var x = 100`，`test_6_functions` 是 `fun add(a, b)`，
  `test_7_anonymous` 是 `(item) -> {...}`。
- `lib/rt/rt.rio` 由 CMake 在构建期用 `RCC -build -emit-lib` 自己编译（CMakeLists.txt:123–147），
  所以任何「未标注即报错」的规则会直接把构建本身弄挂。

如果 P9 强推「注解必填」，代价是同时重写 17 个 legacy 用例 + `rt.rio` + `core.rio`，
那是 P12 的工作量，而且会让「P9 让语言更安全」变成「P9 让所有代码编不过」。

所以 P9 的实际边界是：**写了标注就当真，没写就继续推断**。
这不是妥协，是让已有的标注第一次真正生效 —— 在此之前，
`var x: float = 1` 会被推断成 int 槽，标注完全被忽略（`emitVariableDefinition`
压根没读 `VarDefData::hasLabels()`）。

## 2. 允许的隐式转换（"值类别" 判据）

Rio 目前没有强制转换语法（`(float)x` 之类）。如果 int→float 也报错，
用户没有任何合法写法能表达「把这个整数当浮点用」，只能被迫改标注。
所以判据是**值类别**（value family）而不是精确类型：

| 族 | 成员 | 互通 |
|---|---|---|
| 数值 | `int` `float` `char` `bool` | 全部互通，按需插 `cvtsi2sdq` / `cvttsd2siq` |
| 指针 | `str` `list` `dict` `class` `ptr` | 全部互通（都是一个 8 字节指针） |
| 其它 | `void` | 不能当值用 |

`void` 双向禁止：`void` 表达式不能当值（`var x = f()` 其中 f 返回 void），
`void` 目标不能接收值（`fun g(): void { ret 1 }`）。

`ptr` 与 `str` 必须互通：`rt.rio` 里 `rt_write_str("false", 5)` 把字符串字面量
传给 `ptr` 形参，`s + 8` 在 `ptr` 上做指针算术。这是既有代码的既定用法。

**已知的宽松点**：`var b: bool = 1` 是允许的，运行时打印 `1` 而不是 `true`。
真正的修法是给 bool 加规范化（`!= 0`），但那会改变 `test_4_comparisons` 之外的
既有输出语义，属于 P10 之后的事。P9 只保证「bool 槽不会被塞进一个指针」。

## 3. 实现结构

全部改动集中在 `code/src/backend/rcc_codegen.cpp`（后端没有独立 sema pass，
类型系统就活在 codegen 里）。新增 5 个成员：

| 成员 | 作用 |
|---|---|
| `posPrefix(pos)` | 生成 `file:line:col: ` 前缀。**不用 `Pos::getFilePosStr()`** —— 那个函数硬编码 `:1:1`（rcc_utils.cpp:1207） |
| `failAt(pos, msg)` | `fail(posPrefix(pos) + msg)`，保持「首个错误胜出」的既有语义 |
| `sameValueFamily(a, b)` | §2 的族判据；任一侧 UNKNOWN 时放行（推断不出来就不该判错） |
| `checkAssignable(target, source)` | 返回空串 = 可赋值；否则返回人话理由。同时挡 `void` |
| `typeFromLabels(labels)` | 从 `LabelNode` 列表解析出类型，解析不出返回 UNKNOWN |

以及 `collectParamAnnotations()`（与 `collectParamTypesT` 同构，但保留原始标注文本），
用来区分两种「类型是 UNKNOWN」：

- `fun f(x)` —— 没写标注，回退 int，**不校验**实参
- `fun f(x: Piont)` —— 写了但拼错，**必须报错**

这个区分靠 `_functionParamAnnotated[fname]`（按位置的 bool 掩码）落地。
少了它，`fun apply(f, x)` 会被按 `int` 校验，而 legacy 传的是闭包，直接误报
（第一次改完就是这么挂掉 `test_7_anonymous` 的）。

## 4. 五处检查点

| 位置 | 检查 | 反例 |
|---|---|---|
| `emitVariableDefinition` | 标注 vs 初始值 | `var x: int = "hello"` |
| 同上 | 标注能否解析 | `var x: Piont = 1` |
| `emitAssignment` | 槽位类型 vs 右值 | `var x = 1` 后 `x = "str"` |
| `emitArgsToRegisters` | 形参标注 vs 实参（仅已标注的形参） | `fun f(x: int)` 调用 `f("hi")` |
| `emitReturn` | 返回标注 vs ret 值（仅已标注的返回类型） | `fun g(): str { ret 42 }` |

「仅已标注」这条约束很关键：**推断出来的类型不能拿来判错**。
推断返回类型来自「函数体里第一个 ret」，用它去约束其余分支只会自相矛盾
（`fun f(x) { if x { ret 1 } ret 2.5 }` 是合法代码）。

## 5. 标注生效带来的代码生成修正

光「读标注」不够 —— 读进来之后，存储/返回必须真的按标注类型走，否则
`var x: float = 1` 会把 int 的位模式当 double 解释。三处补了转换：

1. `emitVariableDefinition`：初始化时按目标类型选 `movsd` / `mov`，
   跨族插 `cvtsi2sdq` / `cvttsd2siq`
2. `emitReturn`：ret 值按标注的返回类型落寄存器
   （`fun one(): float { ret 1 }` 之前会把 `%rax=1` 当 double 打印出垃圾）
3. `emitReturn` 的 try 帧展开：保存/恢复返回值时用的 `isFloat` 判定
   必须改成「转换后的类型」，否则转换完又被按 int 存进临时槽

## 6. 与 P8 之前行为的对照

| 写法 | P9 之前 | P9 之后 |
|---|---|---|
| `var x: float = 1` | int 槽，打印 `1` | float 槽，打印 `1.0` |
| `fun f(x: int): int` 调用 `f("hi")` | 打印垃圾指针 | `arg.rio:3:22: Argument 1 of 'f': cannot assign a value of type str to a target of type int` |
| `fun g(): str { ret 42 }` | 打印空串 | `Return value in 'g': cannot assign a value of type int to a target of type str` |
| `var x: int = "hello"` | 打印 `hello` | `Variable 'x' declared as int: cannot assign ...` |
| `var x: Piont = 1` | 静默当 int | `Unknown type 'Piont' in declaration of 'x'` |
| `fun f(a: Piont)` | 静默当 int | `Unknown type 'Piont' of parameter 'a' in function 'f'` |

## 7. 已知限制

- **只检查，不推断跨函数**：`fun f()` 不标注返回类型时，返回类型靠「第一个 ret
  表达式」推断，不做全函数体的类型统一（没有控制流汇合分析）。
- **重载不存在**（P11）：同名函数只有一份签名，`_functionParams` 按名字索引。
- **泛型不存在**（P10）：`list` 一律是 `list[int]`，`dict` 一律是 `dict[str,int]`
  （`parseTypeName` 的硬编码）。所以 `var l: list = [1,2]` 能过，
  但 `var l: list = 5` 会被挡。
- **bool 不做规范化**：见 §2 末尾。
- **`_functionParamAnnotated` 只覆盖具名函数/方法/构造器**，匿名函数的形参
  目前一律按推断走（`emitAnonFunction` 路径没有标注掩码），
  因为 `(a: int) -> {...}` 这种写法在 legacy 里不存在，也无测试覆盖。

## 8. 验收

```powershell
cmake --build build
powershell -NoProfile -ExecutionPolicy Bypass -File test_scripts/test_p9_types.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File test_scripts/test_legacy_all.ps1
```

- `test_p9_types.ps1`：20 个用例（10 正向验证标注真的生效，10 反向验证诊断）
- `test_legacy_all.ps1`：仍为 15/17，不回退
- `test_p0/p1/p2/p5/p6/p7/p8`：全部 0 失败

`rt.rio` 的自举（构建期 `-build -emit-lib`）是隐含的最强回归：
它是一份 204 行、带完整标注、且被真实链接进每个 exe 的 Rio 程序。
它编不过，整个构建就挂。

# P13-2 回归

P13-2 已实施成果回归：语法/类型/所有权/生命周期各层的编译期行为，以及决议 A（值语义 struct）的运行期行为（143 条）。

- 类型: regression
- 用例数: 143
- 入口: `execute.ps1`（转接 `tests/agent/_shared/driver.ps1`）

## compileOnly

负向用例带 `compileOnly: true`：只验编译成功/失败，不运行 exe，用 `mustMatchLiteral` 指定诊断子串。

`L3-struct-*` 这 9 条是**运行期**用例（不带 `compileOnly`），用 `expect` 比对 stdout。它们覆盖决议 A 的调用约定：

| 用例 | 覆盖点 |
| --- | --- |
| `L3-struct-value` | 赋值即逐字节拷贝，两份互不影响 |
| `L3-struct-default` | `Pt()` 零值 + 字段默认值 `var y: int = 5` |
| `L3-struct-param` | struct 形参按值传递（被调方改的是副本） |
| `L3-struct-ret` | 普通函数返回 struct（调用方提供返回缓冲区） |
| `L3-struct-ret-generic` | 泛型实例返回 struct |
| `L3-struct-ret-overload` | 重载决议出的那一份返回 struct |
| `L3-struct-ret-method` | 方法返回 struct，且不破坏接收者对象 |
| `L3-struct-ret-method-spill` | 方法返回 struct 且实参跨越寄存器/栈边界 |
| `L3-struct-scalar-fields` | `int`/`float`/`bool`/`char` 四种标量字段 |

后四条是 2026-11-18 修掉的三处 ABI 缺陷的回归：泛型路径、重载路径、方法路径都没有给返回缓冲区留位置（方法路径还让 `this` 和 `__p5_retbuf` 撞同一个寄存器，`ret p` 会把 struct 值写进接收者对象本身）。

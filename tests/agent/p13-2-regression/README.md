# P13-2 回归

P13-2 已实施成果回归：语法/类型/所有权/生命周期各层的编译期行为（126 条，只验编译成功或失败 + 诊断片段）

- 类型: regression
- 用例数: 126
- 入口: `execute.ps1`（转接 `tests/agent/_shared/driver.ps1`）

## compileOnly

每个用例带 `compileOnly: true`：只验编译成功/失败，不运行 exe。负向用例用 `mustMatchLiteral` 指定诊断子串。

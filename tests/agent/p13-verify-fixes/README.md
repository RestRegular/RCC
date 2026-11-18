# P13 审阅缺陷修复验证

P13-2 审阅发现的 A-J 类缺陷修复验证（43 条，只验编译成功或失败）

- 类型: regression
- 用例数: 43
- 入口: `execute.ps1`（转接 `tests/agent/_shared/driver.ps1`）

## compileOnly

每个用例带 `compileOnly: true`：只验编译成功/失败，不运行 exe。负向用例用 `mustMatchLiteral` 指定诊断子串。

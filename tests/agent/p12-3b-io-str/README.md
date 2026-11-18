# P12-3-B io/str

io 与 str 模块的分层验收（compile / api / behavior）

- 类型: regression
- 用例数: 25
- 入口: `execute.ps1`（**自带判定逻辑**，不走数据驱动驱动）

## 为什么不是数据驱动

本套件断言的是**结构性质**而非单次 stdout：
符号表六格式里该有的节点在不在、std 模块公开函数是否被全部覆盖、
`lib/std` 与 `release/Lib/builtin` 是否逐字节一致。
这些断言数据化后只会变成一堆藏在 JSON 里的字段名，可读性反而更差，
所以保留原生 PowerShell 判定。

## 运行

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tests\agent\p12-3b-io-str\execute.ps1
```

依赖 `release\RCC.exe`：先 `cmake --build cmake-build-debug --target RCC`。
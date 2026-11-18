# P0 编译链路

Rio 源码 -> 汇编 -> 目标文件 -> 可执行 -> 运行，整条链路（不依赖 LLVM，只用 GNU as/ld）

- 类型: smoke
- 用例数: 5
- 入口: `execute.ps1`（**自带判定逻辑**，不走数据驱动驱动）

## 为什么不是数据驱动

本套件断言的是**结构性质**而非单次 stdout：
符号表六格式里该有的节点在不在、std 模块公开函数是否被全部覆盖、
`lib/std` 与 `release/Lib/builtin` 是否逐字节一致。
这些断言数据化后只会变成一堆藏在 JSON 里的字段名，可读性反而更差，
所以保留原生 PowerShell 判定。

## 运行

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tests\agent\p0-pipeline\execute.ps1
```

依赖 `release\RCC.exe`：先 `cmake --build cmake-build-debug --target RCC`。
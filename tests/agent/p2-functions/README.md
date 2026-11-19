# P2 函数

函数定义、调用、默认参数、递归、返回值

- 类型: regression
- 用例数: 10
- 入口: `execute.ps1`（转接 `tests/_shared/driver.ps1`）

## 结构

```
code/cases.json   用例清单（id / 源文件 / 期望）
code/<id>.rio     每个用例一个可直接编译的源文件
```

## 用例字段

| 字段 | 含义 |
| --- | --- |
| `id` | 用例名，唯一 |
| `file` | 单文件用例的源码路径（相对套件根） |
| `main` + `files` | 多文件用例：`main` 是编译入口，`files` 是全部源文件 |
| `expect` | 期望 stdout（CRLF 归一后比对，尾部空白忽略） |
| `exitCode` | 期望运行退出码，默认 0 |
| `expectFailure` | `true` 表示必须**编译失败** |
| `mustMatch` | 期望失败的诊断片段；`mustMatchRegex` 为 `true` 时按正则，否则按通配符包含 |

## 运行

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tests\agent\p2-functions\execute.ps1
```

依赖 `release\RCC.exe`：先 `cmake --build cmake-build-debug --target RCC`。
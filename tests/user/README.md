# tests/user —— 用户自由区（B 类）

**Agent 不会主动改动这里的任何东西。**

这是给你自己用的实验区：随手写复现、反复试、想怎么放就怎么放。
满意之后再 `pack` 成 `tests/cases/` 下的正式测试集。

## 目录内容

| 文件 | 用途 |
|---|---|
| `pack.ps1` | 把这里的复现打包成 `cases/<name>` |
| `run-all.ps1` | 跑测试集的运行器（扫描式，不需要维护清单） |
| 其它 | 你的，随便放 |

## 典型流程

```powershell
# 1. 在这里写你的复现（main.rio + 说明，随便）
# 2. 打包成正式测试集
./pack.ps1 -Name "issue-003-login-timeout" -Type repro -Description "登录超时导致500"

# 3. 补全 cases/<name>/execute.ps1 与 expected/output.txt 后验证
../cases/issue-003-login-timeout/execute.ps1
../validate.ps1
```

## 运行器

```powershell
./run-all.ps1                  # 跑 agent + cases
./run-all.ps1 -Area agent      # 只跑 agent
./run-all.ps1 -Filter p13      # 只跑名字匹配的
./run-all.ps1 -StopOnFail      # 首个失败就停
./run-all.ps1 -KeepLog         # 保留完整日志到 build/test_logs
```

运行器**扫描目录**找测试集（有 `execute.ps1` 的目录），
不再像旧 `test_all.ps1` 那样维护一张硬编码套件清单 ——
新增测试集不用改运行器。
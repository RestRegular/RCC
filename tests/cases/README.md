# tests/cases —— 用户复现用例（A 类）

用户报告的可复现问题放在这里。每个子目录是一个标准测试集：

```
cases/<name>/
├── manifest.json
├── README.md
├── code/
├── execute.ps1
├── input/
└── expected/
```

## 怎么进来

**不要手写**，用 `tests/user/pack.ps1` 从 `tests/user/` 打包过来：

```powershell
./tests/user/pack.ps1 -Name "issue-003-login-timeout" -Type repro -Description "登录超时导致500"
```

## 校验

`tests/validate.ps1` 会检查本目录下每个测试集是否符合契约
（manifest 字段、entry 存在、timeoutSec 合法等）。
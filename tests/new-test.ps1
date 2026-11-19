# ============================================================================
#  new-test.ps1 —— 测试集模板生成器（唯一入口）
#
#  两种用法：
#    参数式（给 Agent 用，配合 -NonInteractive 缺参直接失败）
#      ./new-test.ps1 -Name "issue-003-login-timeout" -Type repro -Target user -Description "登录超时导致500"
#
#    引导式（给人用，缺哪个问哪个）
#      ./new-test.ps1
#
#  生成的骨架（**数据驱动**，判定逻辑不写进测试集）：
#    <Target>/<Name>/
#      manifest.json      元数据
#      README.md          人读说明
#      code/cases.json    用例清单（数据）
#      code/smoke.rio     示例用例源文件
#      input/             输入数据（可选，driver 不用）
#      expected/          output.txt + exit-code.txt（套件级汇总）
#      execute.ps1        薄壳：dot-source tests/_shared/driver.ps1
#
#  判定逻辑只有一份：tests/_shared/driver.ps1。
#  agent / cases / user 三区的测试集格式完全相同，user 区生成的也是数据驱动。
#
#  注意：**不生成 setup.ps1**。环境准备逻辑并入 driver 的构建阶段。
# ============================================================================

param(
    [string]$Name,
    [ValidateSet("repro", "regression", "smoke")][string]$Type,
    [ValidateSet("cases", "agent", "user")][string]$Target,
    [string]$Description,
    [int]$TimeoutSec = 60,
    [switch]$Force,           # 已存在时覆盖
    [switch]$NonInteractive   # 强制失败而非引导（Agent 调用）
)

$ErrorActionPreference = "Stop"
$root = $PSScriptRoot

# RCC 的词法分析器不接受 BOM，所有文本一律无 BOM 落盘。
$utf8NoBom = New-Object System.Text.UTF8Encoding($false)

function Write-Text($path, $text) {
    [System.IO.File]::WriteAllText($path, $text, $utf8NoBom)
}

# ---------------------------------------------------------------------------
#  1. 缺参数时进入引导模式
# ---------------------------------------------------------------------------
if (-not $Name -or -not $Type -or -not $Target) {
    if ($NonInteractive) {
        throw "缺少必要参数（-Name / -Type / -Target）；-NonInteractive 下不做引导"
    }
    if (-not $Name)        { $Name        = Read-Host "测试集名称" }
    if (-not $Type)        {
        do {
            $Type = Read-Host "类型 (repro/regression/smoke)"
        } until ($Type -in @('repro', 'regression', 'smoke'))
    }
    if (-not $Target)      {
        do {
            $Target = Read-Host "目标目录 (cases/agent/user)"
        } until ($Target -in @('cases', 'agent', 'user'))
    }
    if (-not $Description) { $Description = Read-Host "描述（可空）" }
}

# 引导模式下 Read-Host 也可能给出非法值，这里统一再校验一次。
if ($Type -notin @('repro', 'regression', 'smoke')) {
    throw "类型非法：'$Type'（只能是 repro / regression / smoke）"
}
if ($Target -notin @('cases', 'agent', 'user')) {
    throw "目标目录非法：'$Target'（只能是 cases / agent / user）"
}
if ($TimeoutSec -le 0) { throw "超时必须是正整数秒：$TimeoutSec" }

# ---------------------------------------------------------------------------
#  2. 校验名称合法性（防路径穿越、非法字符）
# ---------------------------------------------------------------------------
if ($Name -notmatch '^[A-Za-z0-9][A-Za-z0-9._-]{0,63}$') {
    throw "名称只能包含字母数字 . _ -，且不以符号开头（收到：'$Name'）"
}
# 正则已排除 / \ 与 ..，但 `a..b` 这类仍可能被某些工具误读，再挡一道。
if ($Name.Contains('..')) {
    throw "名称不允许包含连续点号：'$Name'"
}

# ---------------------------------------------------------------------------
#  3. 目标路径
# ---------------------------------------------------------------------------
$dst = Join-Path (Join-Path $root $Target) $Name
if ((Test-Path $dst) -and -not $Force) {
    throw "已存在: $dst（用 -Force 覆盖）"
}
New-Item -ItemType Directory -Force -Path $dst | Out-Null

# ---------------------------------------------------------------------------
#  4. 生成目录骨架
# ---------------------------------------------------------------------------
"code", "input", "expected" | ForEach-Object {
    New-Item -ItemType Directory -Force -Path (Join-Path $dst $_) | Out-Null
}

# ---------------------------------------------------------------------------
#  5. 写 manifest.json
# ---------------------------------------------------------------------------
$manifest = [ordered]@{
    name        = $Name
    version     = "1.0"
    createdAt   = (Get-Date).ToUniversalTime().ToString("o")
    author      = if ($env:USERNAME) { $env:USERNAME } else { "agent" }
    type        = $Type
    description = $Description
    entry       = "execute.ps1"
    timeoutSec  = $TimeoutSec
    caseCount   = 1
    expected    = [ordered]@{
        exitCode   = 0
        outputFile = "expected/output.txt"
        matchMode  = "contains"
    }
}
Write-Text (Join-Path $dst "manifest.json") ($manifest | ConvertTo-Json -Depth 5)

# ---------------------------------------------------------------------------
#  6. 写 execute.ps1 薄壳（只做转接，判定逻辑在 tests/_shared/driver.ps1）
# ---------------------------------------------------------------------------
# $PSScriptRoot = tests/<Target>/<Name>；上溯两级到 tests/，再进 _shared/。
$executeTemplate = @'
# 数据驱动套件：判定逻辑只有一份，在 tests/_shared/driver.ps1
# 本文件只做转接，请不要在这里写判定代码（否则各套件语义会漂移）。
param([string]$WorkDir = $PSScriptRoot)
$ErrorActionPreference = "Stop"

. (Join-Path (Split-Path (Split-Path $PSScriptRoot -Parent) -Parent) '_shared\driver.ps1')
Invoke-RioSuite -WorkDir $WorkDir
'@
Write-Text (Join-Path $dst "execute.ps1") $executeTemplate

# ---------------------------------------------------------------------------
#  7. 写 code/cases.json + 示例用例（数据驱动的「数据」部分）
# ---------------------------------------------------------------------------
$caseId = 'smoke'
$sampleRio = @"
fun main(): void {
    sout("hello from $Name")
}
main()
"@
Write-Text (Join-Path $dst "code\$caseId.rio") $sampleRio

$cases = [ordered]@{
    cases = @(
        [ordered]@{
            id     = $caseId
            file   = "code/$caseId.rio"
            expect = "hello from $Name"
        }
    )
}
Write-Text (Join-Path $dst "code\cases.json") ($cases | ConvertTo-Json -Depth 5)

# ---------------------------------------------------------------------------
#  8. 写 README.md
# ---------------------------------------------------------------------------
$readme = @"
# $Name

- 类型: $Type
- 描述: $Description
- 用例数: 1
- 入口: ``execute.ps1``（转接 ``tests/_shared/driver.ps1``）
- 超时: ${TimeoutSec}s

## 结构

``````
code/cases.json   用例清单（id / 源文件 / 期望）
code/smoke.rio    示例用例（每个用例一个可直接编译的源文件）
input/            输入数据（driver 当前不用，保留占位）
expected/         套件级汇总（由 driver 打印）
``````

## 用例字段

| 字段 | 含义 |
| --- | --- |
| ``id`` | 用例名，唯一 |
| ``file`` | 单文件用例的源码路径（相对套件根） |
| ``main`` + ``files`` | 多文件用例：``main`` 是编译入口，``files`` 是全部源文件 |
| ``expect`` | 期望 stdout（CRLF 归一后比对，尾部空白忽略） |
| ``matchMode`` | ``equals``（默认）或 ``ignoreWhitespace`` |
| ``exitCode`` | 期望运行退出码，默认 0 |
| ``expectFailure`` | ``true`` 表示必须**编译失败** |
| ``mustMatch`` | 期望失败的诊断片段（配 ``mustMatchLiteral`` / ``mustMatchRegex``） |
| ``compileOnly`` | 只验编译通过，不运行 |

## 怎么加用例

1. 在 ``code/`` 下写一个 ``<id>.rio`` 源文件。
2. 在 ``code/cases.json`` 的 ``cases`` 数组里加一项，填 ``id`` / ``file`` / ``expect``。
3. 跑 ``./execute.ps1``，driver 会自动编译、运行、逐例比对并打印汇总。

判定逻辑只在 ``tests/_shared/driver.ps1`` 这一份，三区（agent / cases / user）共用。
"@
Write-Text (Join-Path $dst "README.md") $readme

# ---------------------------------------------------------------------------
#  9. 写 expected/ 占位与 input/ 占位
# ---------------------------------------------------------------------------
Write-Text (Join-Path $dst "expected/output.txt") "结果：通过 1，失败 0"
Write-Text (Join-Path $dst "expected/exit-code.txt") "0"
Write-Text (Join-Path $dst "expected/README.txt") @"
本套件为**数据驱动**，逐用例期望值在 ``code/cases.json``。

整体约定：
- stdout：驱动打印 ``结果：通过 N，失败 0``
- 退出码：0 表示全部用例通过，1 表示有失败（失败明细与临时目录会一并打印）
"@
Write-Text (Join-Path $dst "input/.gitkeep") ""

Write-Host "✅ 已创建: $dst"

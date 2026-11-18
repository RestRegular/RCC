# ============================================================================
#  new-test.ps1 —— 测试集模板生成器（唯一入口）
#
#  两种用法：
#    参数式（给 Agent 用，配合 -NonInteractive 缺参直接失败）
#      ./new-test.ps1 -Name "issue-003-login-timeout" -Type repro -Target cases -Description "登录超时导致500"
#
#    引导式（给人用，缺哪个问哪个）
#      ./new-test.ps1
#
#  生成的骨架：
#    <Target>/<Name>/
#      manifest.json   元数据
#      README.md       人读说明
#      code/           被测代码 / 复现脚本
#      input/          输入数据（可选）
#      expected/       期望结果（output.txt + exit-code.txt）
#      execute.ps1     入口：构建 + 编译 + 运行（三阶段合一）
#
#  注意：**不生成 setup.ps1**。环境准备逻辑并入 execute.ps1 的「构建/编译」阶段。
# ============================================================================

param(
    [string]$Name,
    [ValidateSet("repro", "regression", "smoke")][string]$Type,
    [ValidateSet("cases", "agent")][string]$Target,
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
            $Target = Read-Host "目标目录 (cases/agent)"
        } until ($Target -in @('cases', 'agent'))
    }
    if (-not $Description) { $Description = Read-Host "描述（可空）" }
}

# 引导模式下 Read-Host 也可能给出非法值，这里统一再校验一次。
if ($Type -notin @('repro', 'regression', 'smoke')) {
    throw "类型非法：'$Type'（只能是 repro / regression / smoke）"
}
if ($Target -notin @('cases', 'agent')) {
    throw "目标目录非法：'$Target'（只能是 cases / agent）"
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
    expected    = [ordered]@{
        exitCode   = 0
        outputFile = "expected/output.txt"
        matchMode  = "contains"
    }
}
Write-Text (Join-Path $dst "manifest.json") ($manifest | ConvertTo-Json -Depth 5)

# ---------------------------------------------------------------------------
#  6. 写 execute.ps1 模板（构建 → 运行 → 判定 三阶段合一）
# ---------------------------------------------------------------------------
$executeTemplate = @'
param([string]$WorkDir = $PSScriptRoot)
$ErrorActionPreference = "Stop"
Set-Location $WorkDir

# === 1. 构建/编译（幂等：重复跑不报错）===
# 例：dotnet build ./code/xxx.csproj -c Release
# 例：gcc ./code/main.c -o ./code/main.exe
# 例：& "$repo\release\RCC.exe" -build --path ./code/main.rio --output ./code/main.exe

# === 2. 运行（读 input/，用 env 传参）===
# 例：./code/main.exe < ./input/data.txt

# === 3. 结果判定（stdout 供 expected/output.txt 比对，退出码表达成败）===
Write-Output "TODO: 输出实际结果"
exit 0
'@
Write-Text (Join-Path $dst "execute.ps1") $executeTemplate

# ---------------------------------------------------------------------------
#  7. 写 README.md
# ---------------------------------------------------------------------------
$readme = @"
# $Name

- 类型: $Type
- 描述: $Description
- 入口: execute.ps1
- 超时: ${TimeoutSec}s

## 约定
- code/     : 被测代码，execute.ps1 负责构建与运行
- input/    : 输入数据
- expected/ : 期望输出 / 退出码

## 三阶段契约
execute.ps1 一个入口跑完 构建 → 运行 → 判定：
- 构建幂等，重复执行不报错
- stdout 与 expected/output.txt 比对
- 退出码 0 表示通过
"@
Write-Text (Join-Path $dst "README.md") $readme

# ---------------------------------------------------------------------------
#  8. 写占位文件
# ---------------------------------------------------------------------------
Write-Text (Join-Path $dst "code/.gitkeep") ""
Write-Text (Join-Path $dst "input/.gitkeep") ""
Write-Text (Join-Path $dst "expected/output.txt") ""
Write-Text (Join-Path $dst "expected/exit-code.txt") "0"

Write-Host "✅ 已创建: $dst"

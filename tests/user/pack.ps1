# ============================================================================
#  pack.ps1 —— 把 tests/user/<Name>/ 原样提交为 tests/cases/<Name>/
#
#  用法：
#    ./pack.ps1 -Name "issue-003-login-timeout"                    # 从 user/<Name>/ 打包
#    ./pack.ps1 -Name "issue-003" -Dir "D:\work\my-repro"          # 从外部目录打包
#    ./pack.ps1 -Name "issue-003" -Force                           # 覆盖已有 cases/<Name>/
#
#  前提：源目录必须已经是标准测试集结构：
#    <src>/
#      execute.ps1        (必需)
#      code/              (必需，至少一个文件)
#      input/             (可选)
#      expected/          (必需，至少 output.txt 或 exit-code.txt)
#      manifest.json      (可选；没有则用本脚本参数生成)
#      README.md          (可选；没有则自动生成)
#
#  做的事：校验结构 → 拷到 cases/<Name>/ → 补全 manifest/README → 提示下一步
# ============================================================================

param(
    [Parameter(Mandatory = $true)][string]$Name,
    [ValidateSet("repro", "regression", "smoke")][string]$Type,
    [string]$Description = "",
    [string]$Dir,                 # 源目录；默认 = tests/user/<Name>
    [int]$TimeoutSec = 60,
    [switch]$Force
)

$ErrorActionPreference = "Stop"
$root      = $PSScriptRoot                    # tests/user
$testsRoot = Split-Path $root -Parent         # tests

# --- 源目录 ----------------------------------------------------------------
if (-not $Dir) { $Dir = Join-Path $root $Name }
if (-not (Test-Path $Dir)) { throw "源目录不存在: $Dir" }
$Dir = (Resolve-Path $Dir).Path

# --- 名字校验（和 new-test.ps1 保持一致） ---------------------------------
if ($Name -notmatch '^[A-Za-z0-9][A-Za-z0-9._-]{0,63}$') {
    throw "名称只能包含字母数字 . _ -，且不以符号开头（收到：'$Name'）"
}
if ($Name.Contains('..')) { throw "名称不允许包含连续点号：'$Name'" }

# --- 目标目录 --------------------------------------------------------------
$casesRoot = Join-Path $testsRoot "cases"
$dst       = Join-Path $casesRoot $Name
if ((Test-Path $dst) -and -not $Force) {
    throw "已存在: $dst（用 -Force 覆盖）"
}

# ---------------------------------------------------------------------------
#  1. 校验源目录结构（缺什么直接报错，不做猜测）
# ---------------------------------------------------------------------------
$execSrc = Join-Path $Dir "execute.ps1"
if (-not (Test-Path $execSrc)) { throw "缺少 execute.ps1: $Dir" }

$codeSrc = Join-Path $Dir "code"
if (-not (Test-Path $codeSrc)) { throw "缺少 code/ 目录: $Dir" }
$codeFiles = Get-ChildItem $codeSrc -Recurse -File -Force |
             Where-Object { $_.Name -ne '.gitkeep' }
if (-not $codeFiles) { throw "code/ 是空的（除 .gitkeep 外没有文件）: $codeSrc" }

$expSrc = Join-Path $Dir "expected"
if (-not (Test-Path $expSrc)) { throw "缺少 expected/ 目录: $Dir" }
$hasOutput   = Test-Path (Join-Path $expSrc "output.txt")
$hasExitCode = Test-Path (Join-Path $expSrc "exit-code.txt")
if (-not $hasOutput -and -not $hasExitCode) {
    throw "expected/ 里至少要有 output.txt 或 exit-code.txt: $expSrc"
}

# ---------------------------------------------------------------------------
#  2. 拷到 cases/<Name>/（整目录搬，不挑拣）
# ---------------------------------------------------------------------------
New-Item -ItemType Directory -Force -Path $dst | Out-Null
Get-ChildItem $Dir -Force | ForEach-Object {
    Copy-Item $_.FullName -Destination $dst -Recurse -Force
}
Write-Host "→ 已拷贝: $Dir → $dst"

# ---------------------------------------------------------------------------
#  3. 补全 manifest.json（若源没有，或强制刷新）
# ---------------------------------------------------------------------------
$manifestPath = Join-Path $dst "manifest.json"
if (-not (Test-Path $manifestPath) -or $Force) {
    $utf8NoBom = New-Object System.Text.UTF8Encoding($false)
    $manifest = [ordered]@{
        name        = $Name
        version     = "1.0"
        createdAt   = (Get-Date).ToUniversalTime().ToString("o")
        author      = if ($env:USERNAME) { $env:USERNAME } else { "user" }
        type        = if ($Type) { $Type } else { "repro" }
        description = $Description
        entry       = "execute.ps1"
        timeoutSec  = $TimeoutSec
        expected    = [ordered]@{
            exitCode   = if ($hasExitCode) { [int](Get-Content (Join-Path $expSrc "exit-code.txt") -Raw).Trim() } else { 0 }
            outputFile = if ($hasOutput) { "expected/output.txt" } else { $null }
            matchMode  = "contains"
        }
    }
    [System.IO.File]::WriteAllText($manifestPath, ($manifest | ConvertTo-Json -Depth 5), $utf8NoBom)
    Write-Host "→ 已写入 manifest.json"
} else {
    Write-Host "→ 沿用源目录的 manifest.json"
}

# ---------------------------------------------------------------------------
#  4. 补全 README.md（若源没有）
# ---------------------------------------------------------------------------
$readmePath = Join-Path $dst "README.md"
if (-not (Test-Path $readmePath)) {
    $readme = @"
# $Name

- 类型: $Type
- 描述: $Description
- 入口: execute.ps1
- 超时: ${TimeoutSec}s
- 打包时间: $((Get-Date).ToString('yyyy-MM-dd HH:mm:ss'))

## 来源
从 ``$Dir`` 打包而来。

## 三阶段契约
execute.ps1 一个入口跑完 构建 → 运行 → 判定：
- 构建幂等，重复执行不报错
- stdout 与 expected/output.txt 比对
- 退出码 0 表示通过
"@
    $utf8NoBom = New-Object System.Text.UTF8Encoding($false)
    [System.IO.File]::WriteAllText($readmePath, $readme, $utf8NoBom)
    Write-Host "→ 已写入 README.md"
} else {
    Write-Host "→ 沿用源目录的 README.md"
}

# ---------------------------------------------------------------------------
#  5. 提示
# ---------------------------------------------------------------------------
Write-Host ""
Write-Host "✅ 已提交: $dst" -ForegroundColor Green
Write-Host "   下一步："
Write-Host "     1) 跑一次验证:  cd $dst; ./execute.ps1"
Write-Host "     2) 全局校验:    $testsRoot\validate.ps1"
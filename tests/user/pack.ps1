# ============================================================================
#  pack.ps1 —— 把用户自己写好的复现打包成正式测试集（tests/user → tests/cases）
#
#  用法：
#    ./pack.ps1 -Name "issue-003-login-timeout" -Type repro -Description "登录超时导致500"
#    ./pack.ps1 -Name "issue-003" -Type repro -Dir "D:\work\my-repro"
#
#  做的事情：
#    1. 调用 tests/new-test.ps1 在 cases/<Name>/ 生成标准骨架
#    2. 把用户目录里的东西拷进 cases/<Name>/code/
#       （如果用户目录自带 execute.ps1，就用用户的，不用模板）
#    3. 提示还需要补什么才能被 validate.ps1 认可
#
#  设计意图：tests/user/ 是用户的自由区，Agent 不进来乱动。
#  用户在这里随便写、随便试，满意了再 pack 成 cases/ 下的正式测试集。
# ============================================================================

param(
    [Parameter(Mandatory = $true)][string]$Name,
    [Parameter(Mandatory = $true)][ValidateSet("repro", "regression", "smoke")][string]$Type,
    [string]$Description = "",
    [string]$Dir,                 # 要打包的用户目录；默认 = pack.ps1 所在目录
    [int]$TimeoutSec = 60,
    [switch]$Force
)

$ErrorActionPreference = "Stop"
$root = $PSScriptRoot                        # tests/user
$testsRoot = Split-Path $root -Parent        # tests
$utf8NoBom = New-Object System.Text.UTF8Encoding($false)

if (-not $Dir) { $Dir = $root }
if (-not (Test-Path $Dir)) { throw "目录不存在: $Dir" }
$Dir = (Resolve-Path $Dir).Path

$newTest = Join-Path $testsRoot "new-test.ps1"
if (-not (Test-Path $newTest)) { throw "找不到模板生成器: $newTest" }

# ---------------------------------------------------------------------------
#  1. 先让 new-test.ps1 生成标准骨架（它负责校验名字、写 manifest/README）
# ---------------------------------------------------------------------------
$newTestArgs = @{
    Name           = $Name
    Type           = $Type
    Target         = "cases"
    Description    = $Description
    TimeoutSec     = $TimeoutSec
    NonInteractive = $true
}
if ($Force) { $newTestArgs['Force'] = $true }

Write-Host "→ 生成骨架 tests/cases/$Name ..."
# 注意：& 调用 .ps1 不会设置 $LASTEXITCODE（那只对原生命令生效），
# 失败时 new-test.ps1 自己 throw，异常会直接冒泡上来。
& $newTest @newTestArgs

$dst = Join-Path (Join-Path $testsRoot "cases") $Name
$codeDst = Join-Path $dst "code"

# ---------------------------------------------------------------------------
#  2. 拷用户内容进 code/（跳过 pack.ps1 自身与垃圾文件）
# ---------------------------------------------------------------------------
$skipNames = @('pack.ps1', 'run-all.ps1', 'execute.ps1', 'manifest.json', 'README.md')
$skipDirs  = @('expected', 'input', 'code', '.git', 'node_modules', 'build', 'out')

$copied = 0
Get-ChildItem -Path $Dir -Force | ForEach-Object {
    if ($_.PSIsContainer) {
        if ($skipDirs -contains $_.Name) { return }
        Copy-Item $_.FullName -Destination $codeDst -Recurse -Force
        $copied++
    } else {
        if ($skipNames -contains $_.Name) { return }
        if ($_.Extension -in @('.exe', '.o', '.obj', '.pdb', '.ilk')) { return }
        Copy-Item $_.FullName -Destination $codeDst -Force
        $copied++
    }
}
Write-Host "→ 拷入 code/: $copied 项"

# ---------------------------------------------------------------------------
#  3. 用户自带 execute.ps1 时，优先用用户的（模板会被覆盖）
# ---------------------------------------------------------------------------
$userExec = Join-Path $Dir "execute.ps1"
if (Test-Path $userExec) {
    Copy-Item $userExec -Destination (Join-Path $dst "execute.ps1") -Force
    Write-Host "→ 使用用户自带的 execute.ps1"
} else {
    Write-Host "→ 使用模板 execute.ps1（记得把 code/ 里的文件名填进去）"
}

# ---------------------------------------------------------------------------
#  4. 若用户带了自己的 expected/，一并搬过去
# ---------------------------------------------------------------------------
$userExp = Join-Path $Dir "expected"
if (Test-Path $userExp) {
    Copy-Item (Join-Path $userExp '*') -Destination (Join-Path $dst "expected") -Recurse -Force -ErrorAction SilentlyContinue
    Write-Host "→ 合并用户 expected/"
}

# ---------------------------------------------------------------------------
#  5. 提示后续
# ---------------------------------------------------------------------------
Write-Host ""
Write-Host "✅ 已打包: $dst" -ForegroundColor Green
Write-Host "   下一步："
Write-Host "     1) 检查 execute.ps1 里的构建/运行命令是否指向 code/ 下的真实文件"
Write-Host "     2) 把期望输出写进 expected/output.txt"
Write-Host "     3) 跑一次验证:  ./execute.ps1"
Write-Host "     4) 全局校验:    ../validate.ps1"

#
# test_p0.ps1 —— P0 阶段验收测试
#
# 验证整条链路：Rio 源码 -> 汇编 -> 目标文件 -> 可执行文件 -> 运行。
# 不依赖 LLVM，只用 GNU as / ld（由 RCC 内部的 detectToolchain 定位）。
#
# 用法（在本机 Windows PowerShell 5.1 下）：
#     powershell -ExecutionPolicy Bypass -File test_scripts/test_p0.ps1
# 若已装 PowerShell 7，也可：
#     pwsh -File test_scripts/test_p0.ps1
#

$ErrorActionPreference = 'Stop'

$repoRoot = Split-Path -Parent $PSScriptRoot
$rcc = Join-Path $repoRoot 'release/RCC.exe'

if (-not (Test-Path $rcc)) {
    Write-Host "错误：找不到编译器 $rcc" -ForegroundColor Red
    exit 1
}

# 临时目录放在仓库内：受限环境下系统 %TEMP% 可能不可写
$workDir = Join-Path $repoRoot 'build/p0_test'
if (Test-Path $workDir) { Remove-Item -Recurse -Force $workDir }
New-Item -ItemType Directory -Force -Path $workDir | Out-Null

$utf8NoBom = New-Object System.Text.UTF8Encoding($false)
$pass = 0
$fail = 0

function Invoke-Case {
    param(
        [string]$Name,
        [string]$Source,
        [string]$ExpectStdout
    )

    $rioPath = Join-Path $workDir "$Name.rio"
    # Rio 的 lexer 不支持 BOM，必须无 BOM 写入
    [System.IO.File]::WriteAllText($rioPath, $Source + "`n", $utf8NoBom)

    $exePath = Join-Path $workDir "$Name.exe"
    $stdoutFile = Join-Path $workDir "$Name.out"
    $stderrFile = Join-Path $workDir "$Name.err"

    $build = Start-Process -FilePath $rcc `
        -ArgumentList '-build', '--path', $rioPath `
        -NoNewWindow -Wait -PassThru `
        -RedirectStandardOutput $stdoutFile -RedirectStandardError $stderrFile

    if ($build.ExitCode -ne 0) {
        Write-Host "  编译失败：$Name" -ForegroundColor Red
        Get-Content $stderrFile -Raw | ForEach-Object { Write-Host "    $_" -ForegroundColor DarkRed }
        $script:fail++
        return
    }

    if (-not (Test-Path $exePath)) {
        Write-Host "  未生成可执行文件：$Name" -ForegroundColor Red
        $script:fail++
        return
    }

    $runOut = Join-Path $workDir "$Name.run"
    $run = Start-Process -FilePath $exePath `
        -NoNewWindow -Wait -PassThru `
        -RedirectStandardOutput $runOut

    if ($run.ExitCode -ne 0) {
        Write-Host "  运行退出码非零：$Name (0x{0:X})" -f $run.ExitCode -ForegroundColor Red
        $script:fail++
        return
    }

    $actual = (Get-Content $runOut -Raw -ErrorAction SilentlyContinue)
    if ($null -eq $actual) { $actual = '' }
    $actual = $actual.TrimEnd("`r", "`n")

    if ($actual -ne $ExpectStdout) {
        Write-Host "  输出不符：$Name" -ForegroundColor Red
        Write-Host "    期望：[$ExpectStdout]" -ForegroundColor DarkGray
        Write-Host "    实际：[$actual]" -ForegroundColor DarkGray
        $script:fail++
        return
    }

    Write-Host "  通过：$Name" -ForegroundColor Green
    $script:pass++
}

Write-Host "P0 验收测试" -ForegroundColor Cyan
Write-Host "编译器：$rcc"
Write-Host "工作目录：$workDir"
Write-Host ""

# 1. 最小程序：只输出一个字符串
Invoke-Case -Name 'hello' -Source @'
sout("hello from rio")
'@ -ExpectStdout 'hello from rio'

# 2. 整数算术与变量
Invoke-Case -Name 'arith' -Source @'
var a = 10
var b = 32
var c = a + b
sout("arith ok")
'@ -ExpectStdout 'arith ok'

# 3. 函数定义与调用（验证 Win64 参数传递与栈帧）
Invoke-Case -Name 'func' -Source @'
fun add(x, y) {
    ret x + y
}

var r = add(20, 22)
sout("func ok")
'@ -ExpectStdout 'func ok'

# 4. 多段输出（验证字符串字面量池）
#    sout 默认追加 "\n"；要精确控制输出时用 end="" 关掉
Invoke-Case -Name 'multi' -Source @'
sout("line one", end="")
sout("\n", end="")
sout("line two", end="")
sout("\n", end="")
'@ -ExpectStdout @'
line one
line two
'@

# 5. 乘法与比较
Invoke-Case -Name 'ops' -Source @'
var a = 6
var b = 7
var c = a * b
sout("ops ok")
'@ -ExpectStdout 'ops ok'

Write-Host ""
Write-Host "结果：通过 $pass，失败 $fail" -ForegroundColor $(if ($fail -eq 0) { 'Green' } else { 'Red' })

if ($fail -eq 0) {
    Write-Host "P0 验收通过" -ForegroundColor Green
    exit 0
}

exit 1

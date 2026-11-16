# ============================================================================
#  RCC 改造 P1.3（阶段 P4）—— 函数值 / 匿名函数 / 间接调用验收
#
#  覆盖：匿名函数表达式、var 绑定函数值、函数值当实参、具名函数当函数值、
#        嵌套匿名函数、匿名函数里的 if / 循环、返回浮点的匿名函数
#  每个用例的 .rio 源码与期望输出都内联在本脚本里，跑完即删临时目录。
#
#  用法：powershell -NoProfile -ExecutionPolicy Bypass -File test_scripts/test_p2_functions.ps1
# ============================================================================

$ErrorActionPreference = 'Stop'

$repo = Split-Path -Parent $PSScriptRoot
$rcc = Join-Path $repo 'release\RCC.exe'
$work = Join-Path $repo 'build\p2_func_verify'

if (-not (Test-Path $rcc)) {
    Write-Host "找不到编译器：$rcc" -ForegroundColor Red
    Write-Host "请先构建：cmake --build cmake-build-debug -- -j8"
    exit 1
}

if (Test-Path $work) { Remove-Item $work -Recurse -Force }
New-Item -ItemType Directory -Force -Path $work | Out-Null

$utf8NoBom = New-Object System.Text.UTF8Encoding($false)

# 每个用例都需要 sout 的封装声明（由 rt 提供实现）
$soutStub = 'fun sout(*args, end="\n"): void { encapsulated }'

# ---------------------------------------------------------------------------
#  用例表：名称 -> @{ Src = 源码；Expect = 期望的完整输出 }
# ---------------------------------------------------------------------------
$cases = [ordered]@{

    'anon_basic' = @{
        Src = @"
$soutStub
var add = (a, b) -> { ret a + b }
sout(add(3, 5))
var double = (x) -> { ret x * 2 }
sout(double(10))
"@
        Expect = "8`n20"
    }

    'anon_as_arg' = @{
        # 匿名函数直接当实参传给高阶函数
        Src = @"
$soutStub
fun apply(f, x) { ret f(x) }
sout(apply((n) -> { ret n * n }, 5))
"@
        Expect = "25"
    }

    'named_as_value' = @{
        # 具名函数名当函数值传给高阶函数
        Src = @"
$soutStub
fun add(a, b) { ret a + b }
fun apply2(f, x, y) { ret f(x, y) }
sout(apply2(add, 3, 5))
"@
        Expect = "8"
    }

    'anon_bare_expr' = @{
        # 裸表达式体（没有大括号，parser 会自动包成 ret）
        Src = @"
$soutStub
var triple = (x) -> x * 3
sout(triple(7))
"@
        Expect = "21"
    }

    'anon_return_float' = @{
        # 返回浮点的匿名函数：结果类型要正确推成 float
        Src = @"
$soutStub
var half = (x) -> { ret x / 2.0 }
sout(half(5.0))
"@
        Expect = "2.5"
    }

    'anon_if_body' = @{
        # 匿名函数体里带 if
        Src = @"
$soutStub
var abs = (x) -> {
    if x < 0 {
        ret -x
    }
    ret x
}
sout(abs(-42))
sout(abs(42))
"@
        Expect = "42`n42"
    }

    'anon_loop_body' = @{
        # 匿名函数体里带循环
        Src = @"
$soutStub
var sumTo = (n) -> {
    var s = 0
    var i = 1
    while i <= n {
        s = s + i
        i = i + 1
    }
    ret s
}
sout(sumTo(10))
"@
        Expect = "55"
    }

    'anon_nested' = @{
        # 匿名函数里再定义匿名函数
        Src = @"
$soutStub
var makeAdder = (n) -> {
    var adder = (x) -> { ret x + n }
    ret adder(10)
}
sout(makeAdder(5))
"@
        Expect = "15"
    }

    'anon_inline_call' = @{
        # 匿名函数表达式就地调用
        Src = @"
$soutStub
sout(((a, b) -> { ret a * b })(6, 7))
"@
        Expect = "42"
    }

    'anon_multi_args' = @{
        # 4 个参数，全走整型寄存器
        Src = @"
$soutStub
var quad = (a, b, c, d) -> { ret a + b * 2 + c * 3 + d * 4 }
sout(quad(1, 2, 3, 4))
"@
        Expect = "30"
    }
}

# ---------------------------------------------------------------------------
#  执行
# ---------------------------------------------------------------------------
$pass = 0
$fail = 0
$failedNames = @()

foreach ($name in $cases.Keys) {
    $case = $cases[$name]
    $rioPath = Join-Path $work "$name.rio"
    [System.IO.File]::WriteAllText($rioPath, $case.Src, $utf8NoBom)

    $exePath = Join-Path $work $name
    $buildOut = & $rcc -build --path $rioPath --output $exePath 2>&1 | Out-String

    if ($buildOut -match '(?m)^\s*Error|ParserError|CodeGenError|LinkError') {
        Write-Host ("  [失败] {0} —— 编译失败" -f $name) -ForegroundColor Red
        $firstErr = ($buildOut -split "`n" | Where-Object { $_ -match 'Error|Info' } | Select-Object -First 2) -join ' | '
        Write-Host ("         {0}" -f $firstErr.Trim()) -ForegroundColor DarkGray
        $fail++
        $failedNames += $name
        continue
    }

    if (-not (Test-Path "$exePath.exe")) {
        Write-Host ("  [失败] {0} —— 没有产出 exe" -f $name) -ForegroundColor Red
        $fail++
        $failedNames += $name
        continue
    }

    $actual = (& "$exePath.exe" 2>&1 | Out-String).Trim()
    # 子进程输出是 CRLF，内联期望用 LF；统一成 LF 再比
    $actual = $actual -replace "`r`n", "`n"
    $expect = ($case.Expect.Trim()) -replace "`r`n", "`n"

    if ($actual -eq $expect) {
        Write-Host ("  [通过] {0}" -f $name) -ForegroundColor Green
        $pass++
    } else {
        Write-Host ("  [失败] {0} —— 输出不符" -f $name) -ForegroundColor Red
        Write-Host ("         期望：{0}" -f ($expect -replace "`n", ' / ')) -ForegroundColor DarkGray
        Write-Host ("         实际：{0}" -f ($actual -replace "`n", ' / ')) -ForegroundColor DarkGray
        $fail++
        $failedNames += $name
    }
}

# ---------------------------------------------------------------------------
#  汇总
# ---------------------------------------------------------------------------
Write-Host ""
Write-Host ("结果：通过 {0}，失败 {1}" -f $pass, $fail)

if ($fail -eq 0) {
    Write-Host "P4 函数值验收通过" -ForegroundColor Green
    Remove-Item $work -Recurse -Force -ErrorAction SilentlyContinue
    exit 0
} else {
    Write-Host ("失败用例：{0}" -f ($failedNames -join ', ')) -ForegroundColor Red
    Write-Host ("临时目录保留在：{0}" -f $work) -ForegroundColor Yellow
    exit 1
}

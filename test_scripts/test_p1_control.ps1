# ============================================================================
#  RCC 改造 P1.2 —— 控制流验收
#
#  覆盖：while / until / C 风格 for / break / continue / 嵌套循环 / elif 链
#  每个用例的 .rio 源码与期望输出都内联在本脚本里，跑完即删临时目录。
#
#  用法：powershell -NoProfile -ExecutionPolicy Bypass -File test_scripts/test_p1_control.ps1
# ============================================================================

$ErrorActionPreference = 'Stop'

$repo = Split-Path -Parent $PSScriptRoot
$rcc = Join-Path $repo 'release\RCC.exe'
$work = Join-Path $repo 'build\p1_ctrl_verify'

if (-not (Test-Path $rcc)) {
    Write-Host "找不到编译器：$rcc" -ForegroundColor Red
    Write-Host "请先构建：cmake --build cmake-build-debug -- -j8"
    exit 1
}

if (Test-Path $work) { Remove-Item $work -Recurse -Force }
New-Item -ItemType Directory -Force -Path $work | Out-Null

$utf8NoBom = New-Object System.Text.UTF8Encoding($false)

# ---------------------------------------------------------------------------
#  用例表：名称 -> @{ Src = 源码；Expect = 期望的完整输出 }
# ---------------------------------------------------------------------------
$cases = [ordered]@{

    'while' = @{
        Src = @'
var i = 0
while i < 3 {
    sout(i)
    i = i + 1
}
'@
        Expect = "0`n1`n2"
    }

    'until' = @{
        Src = @'
var n = 0
until n >= 3 {
    sout(n)
    n = n + 1
}
'@
        Expect = "0`n1`n2"
    }

    'for_c_style' = @{
        # Rio 的 C 风格 for 把三段表达式用分号隔开，整体再包一层大括号
        Src = @'
for ({var k = 0; k < 4; k = k + 1}) {
    sout(k)
}
'@
        Expect = "0`n1`n2`n3"
    }

    'for_with_step' = @{
        Src = @'
for ({var k = 10; k > 0; k = k - 4}) {
    sout(k)
}
'@
        Expect = "10`n6`n2"
    }

    'break' = @{
        Src = @'
var j = 0
while j < 10 {
    j = j + 1
    if j == 3 {
        break
    }
    sout(j)
}
'@
        Expect = "1`n2"
    }

    'continue' = @{
        Src = @'
var k = 0
while k < 5 {
    k = k + 1
    if k == 2 {
        continue
    }
    sout(k)
}
'@
        Expect = "1`n3`n4`n5"
    }

    'nested_continue' = @{
        # 内层 continue 只跳过内层，外层照常跑完
        Src = @'
var total = 0
var i = 0
while i < 3 {
    var j = 0
    while j < 3 {
        if j == 1 {
            j = j + 1
            continue
        }
        total = total + 1
        j = j + 1
    }
    i = i + 1
}
sout(total)
'@
        Expect = "6"
    }

    'nested_break' = @{
        # 内层 break 只跳内层：每轮外层都数 2 个
        Src = @'
var s = 0
var a = 0
while a < 5 {
    a = a + 1
    var b = 0
    while b < 5 {
        b = b + 1
        if b == 3 {
            break
        }
        s = s + 1
    }
}
sout(s)
'@
        Expect = "10"
    }

    'break_in_for' = @{
        Src = @'
for ({var k = 0; k < 100; k = k + 1}) {
    if k == 4 {
        break
    }
    sout(k)
}
'@
        Expect = "0`n1`n2`n3"
    }

    'continue_in_for' = @{
        Src = @'
for ({var k = 0; k < 5; k = k + 1}) {
    if k == 2 {
        continue
    }
    sout(k)
}
'@
        Expect = "0`n1`n3`n4"
    }

    'elif_chain' = @{
        Src = @'
var x = 42
if x > 100 {
    sout("A")
} elif x > 50 {
    sout("B")
} elif x > 10 {
    sout("C")
} else {
    sout("D")
}
'@
        Expect = "C"
    }

    'while_with_float' = @{
        # 浮点循环变量：条件用 ucomisd，必须走无符号条件码
        Src = @'
var f = 0.0
while f < 1.5 {
    sout(f)
    f = f + 0.5
}
'@
        Expect = "0.0`n0.5`n1.0"
    }

    'loop_accumulate' = @{
        Src = @'
var sum = 0
for ({var i = 1; i <= 10; i = i + 1}) {
    sum = sum + i
}
sout("sum 1..10: ", sum)
'@
        Expect = "sum 1..10: 55"
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
    Write-Host "P1.2 控制流验收通过" -ForegroundColor Green
    Remove-Item $work -Recurse -Force -ErrorAction SilentlyContinue
    exit 0
} else {
    Write-Host ("失败用例：{0}" -f ($failedNames -join ', ')) -ForegroundColor Red
    Write-Host ("临时目录保留在：{0}" -f $work) -ForegroundColor Yellow
    exit 1
}

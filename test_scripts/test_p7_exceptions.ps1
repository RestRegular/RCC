# ============================================================================
#  RCC 改造 阶段 P7 —— 异常（try / catch / finally / throw）验收
#
#  覆盖：catch 变量绑定与取值、finally 在正常/异常两条路径上都执行、
#        嵌套 try（内层不捕获则向外传播）、跨函数传播、循环内 try + break、
#        try 内 ret 提前返回（帧链必须摘干净）、未捕获 throw 以退出码 3 中止、
#        throw 之后的语句不执行、无 catch 只有 finally 时异常继续外传
#
#  每个用例的 .rio 源码与期望输出都内联在本脚本里，跑完即删临时目录。
#
#  用法：powershell -NoProfile -ExecutionPolicy Bypass -File test_scripts/test_p7_exceptions.ps1
# ============================================================================

$ErrorActionPreference = 'Stop'

$repo = Split-Path -Parent $PSScriptRoot
$rcc = Join-Path $repo 'release\RCC.exe'
$work = Join-Path $repo 'build\p7_exception_verify'

if (-not (Test-Path $rcc)) {
    Write-Host "找不到编译器：$rcc" -ForegroundColor Red
    Write-Host "请先构建：ninja -C build"
    exit 1
}

if (Test-Path $work) { Remove-Item $work -Recurse -Force }
New-Item -ItemType Directory -Force -Path $work | Out-Null

$utf8NoBom = New-Object System.Text.UTF8Encoding($false)

# 每个用例都需要 sout 的封装声明（由 rt 提供实现）
$soutStub = 'fun sout(*args, end="\n"): void { encapsulated }'

# ---------------------------------------------------------------------------
#  用例表：名称 -> @{ Src = 源码；Expect = 期望输出；Exit = 期望退出码（可选，默认 0） }
# ---------------------------------------------------------------------------
$cases = [ordered]@{

    'throw_catch_basic' = @{
        # 最小用例：try 里 throw，catch 接住，后面的语句继续执行
        Src = @"
$soutStub
sout("before")
try {
    throw "boom"
} catch(e) {
    sout("caught")
}
sout("after")
"@
        Expect = "before`ncaught`nafter"
    }

    'catch_value_str' = @{
        # catch 变量绑定的是抛出的那个字符串，能正常打印
        Src = @"
$soutStub
try {
    throw "the message"
} catch(e) {
    sout(e)
}
"@
        Expect = "the message"
    }

    'throw_skips_rest' = @{
        # throw 之后的语句在 try 体内不执行
        Src = @"
$soutStub
try {
    sout("in try")
    throw "x"
    sout("NOT REACHED")
} catch(e) {
    sout("caught")
}
"@
        Expect = "in try`ncaught"
    }

    'no_throw_no_catch' = @{
        # try 体不抛异常时 catch 不执行，try 正常走完
        Src = @"
$soutStub
try {
    sout("try ok")
} catch(e) {
    sout("NOT REACHED")
}
sout("end")
"@
        Expect = "try ok`nend"
    }

    'finally_normal_path' = @{
        # finally 在没有异常时也要执行
        Src = @"
$soutStub
try {
    sout("try")
} finally {
    sout("finally")
}
sout("end")
"@
        Expect = "try`nfinally`nend"
    }

    'finally_after_catch' = @{
        # 异常被 catch 接住后，finally 仍要在 catch 之后执行
        Src = @"
$soutStub
try {
    throw "e1"
} catch(e) {
    sout("catch")
} finally {
    sout("finally")
}
sout("end")
"@
        Expect = "catch`nfinally`nend"
    }

    'finally_no_catch_reraises' = @{
        # 只有 finally 没有 catch：finally 跑完后异常继续向外传，外层接住
        Src = @"
$soutStub
try {
    try {
        throw "deep"
    } finally {
        sout("inner finally")
    }
    sout("NOT REACHED")
} catch(e) {
    sout("outer caught")
    sout(e)
}
"@
        Expect = "inner finally`nouter caught`ndeep"
    }

    'cross_function' = @{
        # 异常跨函数边界传播：在 boom 里抛，main 顶层的 try 接住
        Src = @"
$soutStub
fun boom() {
    throw "from boom"
}
try {
    boom()
    sout("NOT REACHED")
} catch(e) {
    sout(e)
}
sout("end")
"@
        Expect = "from boom`nend"
    }

    'cross_two_frames' = @{
        # 跨两层函数传播
        Src = @"
$soutStub
fun lvl3() {
    throw "three deep"
}
fun lvl2() {
    lvl3()
    sout("NOT REACHED 2")
}
fun lvl1() {
    lvl2()
    sout("NOT REACHED 1")
}
try {
    lvl1()
} catch(e) {
    sout(e)
}
"@
        Expect = "three deep"
    }

    'nested_inner_handles' = @{
        # 内层 try 自己接住异常，外层不该被触发
        Src = @"
$soutStub
try {
    try {
        throw "inner"
    } catch(ei) {
        sout("inner got")
        sout(ei)
    }
    sout("after inner")
} catch(eo) {
    sout("NOT REACHED outer")
}
"@
        Expect = "inner got`ninner`nafter inner"
    }

    'nested_outer_handles' = @{
        # 内层 try 没有 catch（也没有 finally），异常冒到外层
        Src = @"
$soutStub
try {
    try {
        throw "pass through"
    }
    sout("NOT REACHED 2")
} catch(e) {
    sout("outer got")
    sout(e)
}
"@
        Expect = "outer got`npass through"
    }

    'loop_try_break' = @{
        # 循环体内 try + break：帧链必须摘干净，循环后的 try 仍要正常工作
        Src = @"
$soutStub
var i = 0
while i < 5 {
    try {
        if i == 2 { break }
        sout(i)
    } catch(le) {
        sout("NOT REACHED")
    }
    i = i + 1
}
sout("loop done")
try {
    throw "post loop"
} catch(pe) {
    sout(pe)
}
"@
        Expect = "0`n1`nloop done`npost loop"
    }

    'loop_try_continue' = @{
        # 循环体内 try + continue
        Src = @"
$soutStub
var i = 0
while i < 4 {
    i = i + 1
    try {
        if i == 2 { continue }
        sout(i)
    } catch(ce) {
        sout("NOT REACHED")
    }
}
sout("done")
"@
        Expect = "1`n3`n4`ndone"
    }

    'ret_inside_try' = @{
        # try 内部 ret 提前返回：帧链要摘干净，且之后仍能正常抛/接
        Src = @"
$soutStub
fun pick(n) {
    try {
        if n == 1 {
            ret 111
        }
    } catch(r) {
        sout("NOT REACHED")
    }
    ret 222
}
sout(pick(1))
sout(pick(0))
try {
    throw "after ret"
} catch(e) {
    sout(e)
}
"@
        Expect = "111`n222`nafter ret"
    }

    'ret_runs_finally' = @{
        # try 里 ret 提前返回时，finally 必须仍然执行
        Src = @"
$soutStub
fun f() {
    try {
        ret 1
    } finally {
        sout("finally in f")
    }
}
sout(f())
sout("done")
"@
        Expect = "finally in f`n1`ndone"
    }

    'break_runs_finally' = @{
        # 循环里 break 跳出 try 时，finally 也要执行
        Src = @"
$soutStub
var i = 0
while i < 3 {
    try {
        if i == 1 { break }
        sout(i)
    } finally {
        sout("fin")
    }
    i = i + 1
}
sout("done")
"@
        Expect = "0`nfin`nfin`ndone"
    }

    'uncaught_aborts' = @{
        # 未捕获的 throw 终止进程，退出码 3，且之后的语句不执行
        Src = @"
$soutStub
sout("before")
throw "uncaught"
sout("NOT REACHED")
"@
        Expect = "before"
        Exit = 3
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

    $actual = (& "$exePath.exe" 2>&1 | Out-String)
    $exitCode = $LASTEXITCODE
    # 子进程输出是 CRLF，内联期望用 LF；统一成 LF 再比
    $actual = ($actual.Trim()) -replace "`r`n", "`n"
    $expect = ($case.Expect.Trim()) -replace "`r`n", "`n"
    $expectExit = if ($case.ContainsKey('Exit')) { $case.Exit } else { 0 }

    $ok = ($actual -eq $expect) -and ($exitCode -eq $expectExit)
    if ($ok) {
        Write-Host ("  [通过] {0}" -f $name) -ForegroundColor Green
        $pass++
    } else {
        Write-Host ("  [失败] {0} —— 输出或退出码不符" -f $name) -ForegroundColor Red
        if ($actual -ne $expect) {
            Write-Host ("         期望输出：{0}" -f ($expect -replace "`n", ' / ')) -ForegroundColor DarkGray
            Write-Host ("         实际输出：{0}" -f ($actual -replace "`n", ' / ')) -ForegroundColor DarkGray
        }
        if ($exitCode -ne $expectExit) {
            Write-Host ("         期望退出码 {0}，实际 {1}" -f $expectExit, $exitCode) -ForegroundColor DarkGray
        }
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
    Write-Host "P7 异常验收通过" -ForegroundColor Green
    Remove-Item $work -Recurse -Force -ErrorAction SilentlyContinue
    exit 0
} else {
    Write-Host ("失败用例：{0}" -f ($failedNames -join ', ')) -ForegroundColor Red
    Write-Host ("临时目录保留在：{0}" -f $work) -ForegroundColor Yellow
    exit 1
}

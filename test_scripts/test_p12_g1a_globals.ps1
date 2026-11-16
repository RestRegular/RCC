# ============================================================================
#  P12-2 / G1a —— 顶层 `var` 变成全局静态槽
#
#  问题：顶层 `var` 原本被当作 __rio_main 的局部变量发射，函数体里看不到，
#        报 `Undefined variable: g`。原仓库有 ScopeType::GLOBAL，
#        `math.rio` 的 `var pi: float const` 就是顶层 var。
#
#  修法：emitProgram 先扫一遍顶层 var，为每个在 .bss 分配 `_g_<name>` 槽，
#        登记进独立的 _globals 表（不能放 _scopes —— emitFunctionBody 会
#        _scopes.clear()）；lookupVar 未命中局部作用域时回退查 _globals。
#        顶层 var 的初始化语句仍在 __rio_main 里按源码顺序执行。
#
#  用法：powershell -NoProfile -ExecutionPolicy Bypass -File test_scripts/test_p12_g1a_globals.ps1
# ============================================================================

$ErrorActionPreference = 'Stop'

$repo = Split-Path -Parent $PSScriptRoot
$rcc  = Join-Path $repo 'release\RCC.exe'
$work = Join-Path $repo 'build\p12_g1a_verify'

if (-not (Test-Path $rcc)) {
    Write-Host "找不到编译器：$rcc" -ForegroundColor Red
    exit 1
}

if (Test-Path $work) { Remove-Item $work -Recurse -Force }
New-Item -ItemType Directory -Force -Path $work | Out-Null

$utf8NoBom = New-Object System.Text.UTF8Encoding($false)
$soutStub = 'fun sout(*args, end="\n"): void { encapsulated }'

$cases = [ordered]@{

    # 最基本的：函数体里读顶层 var
    'read_in_function' = @{
        Src = @"
$soutStub
var g: int = 7
fun readG(): int { ret g }
sout(readG())
"@
        Expect = '7'
    }

    # 函数体里**写**顶层 var，改动要对后续可见
    'write_in_function' = @{
        Src = @"
$soutStub
var g: int = 1
fun bump(): void { g = g + 1 }
bump()
bump()
sout(g)
"@
        Expect = '3'
    }

    # 顶层语句里赋值，函数体里能读到新值
    'assign_at_top' = @{
        Src = @"
$soutStub
var g: int = 1
fun readG(): int { ret g }
g = 100
sout(readG())
"@
        Expect = '100'
    }

    # 三种类型都要能放全局槽（float 走 xmm0 路径，str 是指针）
    'float_global' = @{
        Src = @"
$soutStub
var pi: float = 3.5
fun readPi(): float { ret pi }
sout(readPi())
"@
        Expect = '3.5'
    }

    'str_global' = @{
        Src = @"
$soutStub
var msg: str = "hi"
fun readMsg(): str { ret msg }
sout(readMsg())
"@
        Expect = 'hi'
    }

    # 多个顶层 var 不能互相覆盖（每个都得有独立的 .bss 槽）
    'multiple_globals' = @{
        Src = @"
$soutStub
var a: int = 1
var b: int = 2
var c: int = 3
fun sum3(): int { ret a + b + c }
sout(sum3())
sout(a)
sout(b)
sout(c)
"@
        Expect = "6`n1`n2`n3"
    }

    # 全局与同名局部：局部必须遮蔽全局（栈槽优先），且不能污染全局
    'local_shadows_global' = @{
        Src = @"
$soutStub
var x: int = 10
fun f(): int {
    var x: int = 99
    ret x
}
sout(f())
sout(x)
"@
        Expect = "99`n10"
    }

    # 全局参与表达式与函数实参
    'global_as_arg' = @{
        Src = @"
$soutStub
var base: int = 40
fun add(a: int, b: int): int { ret a + b }
sout(add(base, 2))
"@
        Expect = '42'
    }
}

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
        $firstErr = ($buildOut -split "`n" | Where-Object { $_ -match 'Error' } | Select-Object -First 1) -join ''
        if ($firstErr) { Write-Host ("         {0}" -f $firstErr.Trim()) -ForegroundColor DarkGray }
        $fail++; $failedNames += $name
        continue
    }

    if (-not (Test-Path "$exePath.exe")) {
        Write-Host ("  [失败] {0} —— 没有产出 exe" -f $name) -ForegroundColor Red
        $fail++; $failedNames += $name
        continue
    }

    $actual = (& "$exePath.exe" 2>&1 | Out-String).Trim() -replace "`r`n", "`n"
    $expect = ($case.Expect.Trim()) -replace "`r`n", "`n"

    if ($actual -eq $expect) {
        Write-Host ("  [通过] {0}" -f $name) -ForegroundColor Green
        $pass++
    } else {
        Write-Host ("  [失败] {0} —— 输出不符" -f $name) -ForegroundColor Red
        Write-Host ("         期望：{0}" -f ($expect -replace "`n", ' / ')) -ForegroundColor DarkGray
        Write-Host ("         实际：{0}" -f ($actual -replace "`n", ' / ')) -ForegroundColor DarkGray
        $fail++; $failedNames += $name
    }
}

Write-Host ""
Write-Host "============================================================" -ForegroundColor Cyan
Write-Host "结果：通过 $pass，失败 $fail" -ForegroundColor $(if ($fail -eq 0) { 'Green' } else { 'Red' })

if ($fail -gt 0) {
    Write-Host "失败用例：$($failedNames -join ', ')" -ForegroundColor Red
    exit 1
}

Write-Host "P12-2 / G1a 顶层 var 全局槽验收通过" -ForegroundColor Green
exit 0

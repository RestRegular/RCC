# ============================================================================
#  P12-2 / G9 + G10 —— str 内容比较 与 str 下标读取
#
#  ---- G9：`str == str` 必须按内容比较 ----
#  问题：emitBinary 的 TOKEN_EQUAL 走 cmpq + sete，比的是**指针**。
#        字符串字面量即使内容相同地址也不同，所以永远 ne。
#  原仓库依据：str.rio:105 `if del == ""`、datatype.rio:92
#        `typeOfArg1 == arg2[i]`、ds.rio 里大量 `if xxx == "..."` ——
#        全部依赖内容比较。
#  修法：两侧同为 TK_STR 时调 emitStrEq（先比长度头，再 core_mem_eq）。
#
#  ---- G10：`s[i]` 读取 ----
#  问题：emitIndexExpression 只处理 TK_LIST / TK_DICT，str 报
#        `Index access on a non-container value`。
#  原仓库依据：visitIndexExpressionNode 明确把 str 归入 iterable；
#        str.rio:24 `ret arg[0]`、str.rio:52 都在用。v1.21.2 实测 s[1] -> e。
#  修法：增加 TK_STR 分支，返回 char（0..255），越界返回 0。
#
#  ⚠️ `s[i] = c` **必须编译期报错**（见 G2 的反向用例）：原仓库的 str 写入
#     是静默失效的 bug（String::getDataAt 返回副本），不得复制。
#     str 是不可变字节序列，允许写会别名污染所有持有者。
#
#  ⚠️ UTF-8：`s[i]` 按**字节**索引，多字节字符会被拆开 —— 与原仓库一致。
#
#  用法：powershell -NoProfile -ExecutionPolicy Bypass -File test_scripts/test_p12_g9g10_str.ps1
# ============================================================================

$ErrorActionPreference = 'Stop'

$repo = Split-Path -Parent $PSScriptRoot
$rcc  = Join-Path $repo 'release\RCC.exe'
$work = Join-Path $repo 'build\p12_g9g10_verify'

if (-not (Test-Path $rcc)) {
    Write-Host "找不到编译器：$rcc" -ForegroundColor Red
    exit 1
}

if (Test-Path $work) { Remove-Item $work -Recurse -Force }
New-Item -ItemType Directory -Force -Path $work | Out-Null

$utf8NoBom = New-Object System.Text.UTF8Encoding($false)
$soutStub = 'fun sout(*args, end="\n"): void { encapsulated }'

$cases = [ordered]@{

    # ---- G9 内容比较 ----

    # 探针 ff3/ff6 的原始症状：两个内容相同的变量必须相等
    'eq_variables' = @{
        Src = @"
$soutStub
var a: str = "ab"
var c: str = "ab"
if a == c { sout("eq") } else { sout("ne") }
"@
        Expect = 'eq'
    }

    'ne_variables' = @{
        Src = @"
$soutStub
var a: str = "ab"
var c: str = "ac"
if a == c { sout("eq") } else { sout("ne") }
"@
        Expect = 'ne'
    }

    # 变量与字面量比较（字面量地址必然不同，这是最容易踩的）
    'eq_variable_literal' = @{
        Src = @"
$soutStub
var a: str = "ab"
if a == "ab" { sout("eq") } else { sout("ne") }
"@
        Expect = 'eq'
    }

    # 长度不同必须不等（不能只比前 n 字节）
    'ne_prefix' = @{
        Src = @"
$soutStub
var a: str = "ab"
if a == "abc" { sout("bad") } else { sout("ok") }
"@
        Expect = 'ok'
    }

    # 原仓库 str.rio:105 的写法：与空串比较
    'eq_empty' = @{
        Src = @"
$soutStub
var e: str = ""
if e == "" { sout("empty") } else { sout("no") }
"@
        Expect = 'empty'
    }

    'ne_empty' = @{
        Src = @"
$soutStub
var a: str = "x"
if a == "" { sout("bad") } else { sout("ok") }
"@
        Expect = 'ok'
    }

    # != 必须是 == 的逻辑取反
    'not_equal_operator' = @{
        Src = @"
$soutStub
var a: str = "ab"
var b: str = "ab"
if a != b { sout("bad") } else { sout("ok") }
if a != "zz" { sout("ok2") } else { sout("bad2") }
"@
        Expect = "ok`nok2"
    }

    # 拼接结果与字面量比较（G3 + G9 联用）
    'eq_concat_result' = @{
        Src = @"
$soutStub
var a: str = "ab"
var c: str = a + "cd"
if c == "abcd" { sout("eq") } else { sout("ne") }
"@
        Expect = 'eq'
    }

    # 中文按字节比较
    'eq_multibyte' = @{
        Src = @"
$soutStub
var a: str = "你好"
if a == "你好" { sout("eq") } else { sout("ne") }
"@
        Expect = 'eq'
    }

    # ---- G10 str 下标读取 ----

    'index_read' = @{
        Src = @"
$soutStub
var s: str = "hello"
sout(s[0])
sout(s[1])
sout(s[4])
"@
        Expect = "h`ne`no"
    }

    'index_read_literal' = @{
        Src = @"
$soutStub
sout("abc"[1])
"@
        Expect = 'b'
    }

    # 下标是变量
    'index_read_variable' = @{
        Src = @"
$soutStub
var s: str = "hello"
var i: int = 2
sout(s[i])
"@
        Expect = 'l'
    }

    # 越界返回 0（与 list 读同口径），不能崩。
    # 注意：s[i] 的静态类型是 char，sout 按**字符**打印 —— 直接 sout(s[99])
    # 输出的是 NUL 字节而不是 "0"。要观察数值必须提升成 int（+ 0）。
    'index_read_out_of_range' = @{
        Src = @"
$soutStub
var s: str = "ab"
sout(s[99] + 0)
"@
        Expect = '0'
    }

    # 返回值参与算术（char 是 int 族）：'A' = 65
    'index_read_as_int' = @{
        Src = @"
$soutStub
var s: str = "A"
sout(s[0] + 1)
"@
        Expect = '66'
    }

    # 循环遍历字符串的字节（原仓库 str.rio 的典型用法）
    'index_read_in_loop' = @{
        Src = @"
$soutStub
var s: str = "abc"
var i: int = 0
while i < 3 {
    sout(s[i])
    i += 1
}
"@
        Expect = "a`nb`nc"
    }

    # 空串下标：越界返回 0（同上，需提升成 int 才看得到数值）
    'index_read_empty' = @{
        Src = @"
$soutStub
var e: str = ""
sout(e[0] + 0)
"@
        Expect = '0'
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

    if ($buildOut -match 'Error|ParserError|CodeGenError|LinkError') {
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

Write-Host "P12-2 / G9 + G10 str 比较与下标验收通过" -ForegroundColor Green
exit 0

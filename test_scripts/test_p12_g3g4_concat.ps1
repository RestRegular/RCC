# ============================================================================
#  P12-2 / G3 + G4 —— 字符串拼接与列表拼接
#
#  问题：`emitBinary` 的 `+` 无条件走 addq（整数加法）。str / list 都是**指针**，
#        加出来是垃圾：
#          "ab" + "cd"  -> 报 cannot assign int to str，或打出指针值
#          [1,2] + [3,4] -> Index access on a non-container value
#
#  原仓库依据（证明这是**回归**不是新特性）：
#    str.rio:111   `[""] + result + ""`     列表拼接
#    lowlevel.rio:53 `var riString: str = i + ": "`  字符串拼接
#
#  修法：
#    ① inferType 对 `str + str` 返回 str、`list + list` 返回 list ——
#       否则结果槽被推成 int，后续 sout 走 rt_print_int 打出指针值。
#    ② emitBinary 在 TOKEN_PLUS 处判断两侧类型族，分派到内联的
#       emitStrConcat / emitListConcat。
#
#  为什么内联而不调 rt.rio 的辅助函数：core 只提供 mem_copy / mem_set /
#  mem_eq / alloc，没有「读 8 字节」的入口，用 Rio 写不出来；为此改 core.rio
#  的冻结 ABI 不值得。
#
#  用法：powershell -NoProfile -ExecutionPolicy Bypass -File test_scripts/test_p12_g3g4_concat.ps1
# ============================================================================

$ErrorActionPreference = 'Stop'

$repo = Split-Path -Parent $PSScriptRoot
$rcc  = Join-Path $repo 'release\RCC.exe'
$work = Join-Path $repo 'build\p12_g3g4_verify'

if (-not (Test-Path $rcc)) {
    Write-Host "找不到编译器：$rcc" -ForegroundColor Red
    exit 1
}

if (Test-Path $work) { Remove-Item $work -Recurse -Force }
New-Item -ItemType Directory -Force -Path $work | Out-Null

$utf8NoBom = New-Object System.Text.UTF8Encoding($false)
$soutStub = 'fun sout(*args, end="\n"): void { encapsulated }'

$cases = [ordered]@{

    # ---- G3 字符串拼接 ----

    'str_concat_literals' = @{
        Src = @"
$soutStub
sout("x" + "y")
"@
        Expect = 'xy'
    }

    'str_concat_variables' = @{
        Src = @"
$soutStub
var a: str = "ab"
var b: str = "cd"
sout(a + b)
"@
        Expect = 'abcd'
    }

    # 原仓库 str.rio 的写法：从空串开始累积
    'str_concat_accumulate' = @{
        Src = @"
$soutStub
var r: str = ""
r = r + "hi"
r = r + "!"
sout(r)
"@
        Expect = 'hi!'
    }

    # 三连拼接（左结合，验证中间结果也是 str）
    'str_concat_chain' = @{
        Src = @"
$soutStub
sout("a" + "b" + "c")
"@
        Expect = 'abc'
    }

    # 一侧是空串：长度头 + 0 字节，不能崩
    'str_concat_empty_left' = @{
        Src = @"
$soutStub
var e: str = ""
sout(e + "tail")
"@
        Expect = 'tail'
    }

    'str_concat_empty_right' = @{
        Src = @"
$soutStub
var e: str = ""
sout("head" + e)
"@
        Expect = 'head'
    }

    # 拼接结果参与比较（内容相等）
    'str_concat_then_compare' = @{
        Src = @"
$soutStub
var a: str = "ab"
var c: str = a + "cd"
sout(c)
"@
        Expect = 'abcd'
    }

    # 中文（多字节）拼接：按字节复制，不能按字符
    'str_concat_multibyte' = @{
        Src = @"
$soutStub
sout("你好" + "世界")
"@
        Expect = '你好世界'
    }

    # ---- G4 列表拼接 ----

    'list_concat_literals' = @{
        Src = @"
$soutStub
var l = [1, 2] + [3, 4]
sout(l[0])
sout(l[1])
sout(l[2])
sout(l[3])
"@
        Expect = "1`n2`n3`n4"
    }

    'list_concat_variables' = @{
        Src = @"
$soutStub
var a = [1, 2]
var b = [3, 4]
var c = a + b
sout(c[0])
sout(c[1])
sout(c[2])
sout(c[3])
"@
        Expect = "1`n2`n3`n4"
    }

    # 空列表参与拼接
    'list_concat_empty_left' = @{
        Src = @"
$soutStub
var e = []
var l = e + [7]
sout(l[0])
"@
        Expect = '7'
    }

    'list_concat_empty_right' = @{
        Src = @"
$soutStub
var e = []
var l = [7] + e
sout(l[0])
"@
        Expect = '7'
    }

    # 拼接结果的长度必须正确（读越界应返回 0，说明长度没算错）
    'list_concat_length' = @{
        Src = @"
$soutStub
var a = [1, 2, 3]
var b = [4, 5]
var c = a + b
sout(c[4])
sout(c[5])
"@
        Expect = "5`n0"
    }

    # 拼接后的列表可继续被索引赋值（G2 与 G4 联用）
    'list_concat_then_set' = @{
        Src = @"
$soutStub
var c = [1, 2] + [3, 4]
c[0] = 99
sout(c[0])
sout(c[3])
"@
        Expect = "99`n4"
    }

    # 原列表不能被拼接改动（拼接是新建，不是原地追加）
    'list_concat_no_mutation' = @{
        Src = @"
$soutStub
var a = [1, 2]
var b = [3, 4]
var c = a + b
sout(a[0])
sout(a[1])
sout(b[0])
sout(b[1])
"@
        Expect = "1`n2`n3`n4"
    }

    # 三连拼接
    'list_concat_chain' = @{
        Src = @"
$soutStub
var l = [1] + [2] + [3]
sout(l[0])
sout(l[1])
sout(l[2])
"@
        Expect = "1`n2`n3"
    }

    # 复合赋值形态：`s += "x"` 必须与 `s = s + "x"` 同义（P12/G4 补充）
    'str_compound_append' = @{
        Src = @"
$soutStub
var s: str = "a"
s += "b"
s += "c"
sout(s)
"@
        Expect = 'abc'
    }

    'str_compound_append_empty' = @{
        Src = @"
$soutStub
var s: str = ""
s += "x"
sout(s)
"@
        Expect = 'x'
    }

    'list_compound_append' = @{
        Src = @"
$soutStub
var l = [1]
l += [2]
sout(l[0])
sout(l[1])
"@
        Expect = "1`n2"
    }

    'list_compound_append_chain' = @{
        Src = @"
$soutStub
var l = [1]
l += [2]
l += [3]
sout(l[2])
"@
        Expect = '3'
    }

    # 复合赋值必须是拼接，不能是「指针相加」——原仓库实测可用
    'str_compound_no_pointer_add' = @{
        Src = @"
$soutStub
var s: str = "ab"
s += "cd"
sout(core_str_len(s))
"@
        Expect = '4'
    }

    # 字符串元素列表的拼接（元素是指针，按 8 字节复制）
    'list_concat_str_elements' = @{
        Src = @"
$soutStub
var l = ["a", "b"] + ["c"]
sout(l[0])
sout(l[1])
sout(l[2])
"@
        Expect = "a`nb`nc"
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

Write-Host "P12-2 / G3 + G4 拼接验收通过" -ForegroundColor Green
exit 0

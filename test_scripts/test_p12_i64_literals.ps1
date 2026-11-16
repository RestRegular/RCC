# ============================================================================
#  P12 / 任务 2 —— 整数字面量放宽到 int64
#
#  ---- 现象（改前实测）----
#  2147483647              -> 2147483647          （通过）
#  2147483648              -> Error: Overflow error: 2147483648
#  9223372036854775807     -> Error: Overflow error: 9223372036854775807
#  -9223372036854775808    -> Error: Overflow error: -9223372036854775808
#
#  ---- 根因（三处 32 位宽度，全是遗留）----
#  1) utils::stringToInt 返回 int，且用 numeric_limits<int>::max() 判溢出
#     —— 字面量在词法/解析期就被判越界，根本到不了 codegen。
#  2) ast::IntegerLiteralNode::value 是 int（AST 侧再截一次）。
#  3) utils::Number::int_value 是 int（RJson 等共用同一类型）。
#  注：emitIntegerLiteral 本身早就按 unsigned long long 自己累加，不受影响；
#      它之所以「看着支持 i64」，是因为上面三处先把它拦下了。
#
#  ---- 修法 ----
#  stringToInt -> long long；IntegerLiteralNode::value -> long long；
#  Number::int_value -> long long（并保留 int 重载避免 Number(5) 二义）。
#  溢出判定改成按**绝对值**：负数上限 2^63（-9223372036854775808 合法），
#  正数上限 2^63-1；用无符号累加、末位按符号折回，所以 i64 两端都精确，
#  超范围继续报 Overflow error（不静默截断、不退化成浮点）。
#
#  ---- 顺带修掉 ----
#  lib/rt/rt.rio 的 rt_print_int 里有条历史注释写着「字面量是 32 位，
#  写不出 i64 最小值，所以不做特判」，于是 rt_print_int(-9223372036854775808)
#  的 0-v 溢出、打印出垃圾（实测 "("）。字面量放开后补上特判。
#
#  用法：powershell -NoProfile -ExecutionPolicy Bypass -File test_scripts/test_p12_i64_literals.ps1
# ============================================================================

$ErrorActionPreference = 'Stop'

$repo = Split-Path -Parent $PSScriptRoot
$rcc  = Join-Path $repo 'release\RCC.exe'
$work = Join-Path $repo 'build\p12_i64_verify'

if (-not (Test-Path $rcc)) {
    Write-Host "找不到编译器：$rcc" -ForegroundColor Red
    exit 1
}

if (Test-Path $work) { Remove-Item $work -Recurse -Force }
New-Item -ItemType Directory -Force -Path $work | Out-Null

$utf8NoBom = New-Object System.Text.UTF8Encoding($false)
$soutStub = 'fun sout(*args, end="\n"): void { encapsulated }'

$I32_MAX = '2147483647'
$I32_MAXP1 = '2147483648'
$U32_MAX = '4294967295'
$U32_MAXP1 = '4294967296'
$I64_MAX = '9223372036854775807'
$I64_MIN = '-9223372036854775808'

$cases = [ordered]@{

    # ---- 32 位边界：回归保护，不能为了放 i64 把老边界弄坏 ----

    'i32_max' = @{ Src = "$soutStub`nsout($I32_MAX)"; Expect = $I32_MAX }

    'i32_max_plus_1' = @{ Src = "$soutStub`nsout($I32_MAXP1)"; Expect = $I32_MAXP1 }

    'u32_max' = @{ Src = "$soutStub`nsout($U32_MAX)"; Expect = $U32_MAX }

    'u32_max_plus_1' = @{ Src = "$soutStub`nsout($U32_MAXP1)"; Expect = $U32_MAXP1 }

    # ---- i64 两端 ----

    'i64_max' = @{ Src = "$soutStub`nsout($I64_MAX)"; Expect = $I64_MAX }

    'i64_min' = @{ Src = "$soutStub`nsout($I64_MIN)"; Expect = $I64_MIN }

    # 中等量级（1e14 量级，旧版同样报错）
    'medium_1e14' = @{ Src = "$soutStub`nsout(123456789012345)"; Expect = '123456789012345' }

    # ---- 字面量不只是能打印，还得能参与运算 ----

    'i64_arith_add' = @{
        Src = "$soutStub`nvar a = $I64_MAX`nvar b = a - 1`nsout(b)"
        Expect = '9223372036854775806'
    }

    'i64_arith_mul' = @{
        Src = "$soutStub`nsout(2147483648 * 2)"
        Expect = '4294967296'
    }

    'i64_arith_div' = @{
        Src = "$soutStub`nsout(4294967296 / 2)"
        Expect = '2147483648'
    }

    'i64_arith_mod' = @{
        Src = "$soutStub`nsout($I64_MAX % 1000)"
        Expect = '807'
    }

    # i64 最小值加 1：验证最小值不是「先取正再取负」硬凑出来的假值
    'i64_min_plus_one' = @{
        Src = "$soutStub`nsout($I64_MIN + 1)"
        Expect = '-9223372036854775807'
    }

    # 用 0 减去 i64 最大值再减 1：运行时算出来的最小值，走的是普通减法路径
    'i64_min_computed' = @{
        Src = "$soutStub`nvar m = $I64_MAX`nsout(0 - m - 1)"
        Expect = $I64_MIN
    }

    # ---- 比较 ----

    'i64_compare_gt' = @{
        Src = "$soutStub`nsout(2147483648 > 2147483647)"
        Expect = 'true'
    }

    'i64_compare_neg' = @{
        Src = "$soutStub`nsout($I64_MIN < 0)"
        Expect = 'true'
    }

    'i64_compare_eq' = @{
        Src = "$soutStub`nsout($I64_MAX == $I64_MAX)"
        Expect = 'true'
    }

    # 最小值不能被误判成 0 / 正数
    'i64_min_sign' = @{
        Src = "$soutStub`nvar v = $I64_MIN`nif v < 0 {`n    sout(`"negative`")`n} else {`n    sout(`"non-negative`")`n}"
        Expect = 'negative'
    }

    # ---- 字面量要能穿过各种值传递路径 ----

    'i64_as_param' = @{
        Src = "$soutStub`nfun echo(v: int): int { ret v }`nsout(echo($I64_MAX))"
        Expect = $I64_MAX
    }

    'i64_in_list' = @{
        Src = "$soutStub`nvar l = [2147483648, $I64_MAX]`nsout(l[0])`nsout(l[1])"
        Expect = "2147483648`n$I64_MAX"
    }

    'i64_negative_var' = @{
        Src = "$soutStub`nvar x = -123456789012345`nsout(x)`nsout(0 - x)"
        Expect = "-123456789012345`n123456789012345"
    }

    # 负号被词法器吸进 token 的形态（-2147483648 是经典的 32 位边界坑）
    'i64_negative_lexed' = @{
        Src = "$soutStub`nvar a = -2147483648`nsout(a)"
        Expect = '-2147483648'
    }

    # ---- 浮点路径不受影响（回归保护）----

    'float_still_works' = @{
        Src = "$soutStub`nsout(3.14)`nsout(1.5 + 2.25)"
        Expect = "3.14`n3.75"
    }

    # ---- 反向用例：超出 i64 必须报错，不能静默截断或退化成浮点 ----

    'overflow_i64_max_plus_1' = @{
        Src = "$soutStub`nsout(9223372036854775808)"
        MustMatch = 'Overflow error'
    }

    'overflow_i64_min_minus_1' = @{
        Src = "$soutStub`nsout(-9223372036854775809)"
        MustMatch = 'Overflow error'
    }

    'overflow_huge' = @{
        Src = "$soutStub`nsout(99999999999999999999)"
        MustMatch = 'Overflow error'
    }
}

$pass = 0
$fail = 0
$failedNames = @()

foreach ($name in $cases.Keys) {
    $case = $cases[$name]
    $rioPath = Join-Path $work "$name.rio"
    [System.IO.File]::WriteAllText($rioPath, $case.Src, $utf8NoBom)

    # 预期失败的命令会往 stderr 写诊断，Stop 会把它当终止错误抛出。
    $prev = $ErrorActionPreference
    $ErrorActionPreference = 'Continue'
    try {
        $buildOut = & $rcc -build --path $rioPath --output (Join-Path $work $name) 2>&1 | Out-String
    } finally {
        $ErrorActionPreference = $prev
    }

    # 反向用例：必须编译失败，且诊断匹配
    if ($case.ContainsKey('MustMatch')) {
        if ($buildOut -match 'Error' -and $buildOut -match $case.MustMatch) {
            Write-Host ("  [通过] {0}" -f $name) -ForegroundColor Green
            $pass++
        } else {
            Write-Host ("  [失败] {0} —— 本应报 '{1}' 却没有" -f $name, $case.MustMatch) -ForegroundColor Red
            $fail++; $failedNames += $name
        }
        continue
    }

    if ($buildOut -match 'Error|ParserError|CodeGenError|LinkError') {
        Write-Host ("  [失败] {0} —— 编译失败" -f $name) -ForegroundColor Red
        $firstErr = ($buildOut -split "`n" | Where-Object { $_ -match 'Error' } | Select-Object -First 1) -join ''
        if ($firstErr) { Write-Host ("         {0}" -f $firstErr.Trim()) -ForegroundColor DarkGray }
        $fail++; $failedNames += $name
        continue
    }

    $exePath = Join-Path $work "$name.exe"
    if (-not (Test-Path $exePath)) {
        Write-Host ("  [失败] {0} —— 没有产出 exe" -f $name) -ForegroundColor Red
        $fail++; $failedNames += $name
        continue
    }

    $actual = (& $exePath 2>&1 | Out-String).Trim() -replace "`r`n", "`n"
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

Write-Host "P12 任务 2（int64 字面量）验收通过" -ForegroundColor Green
exit 0

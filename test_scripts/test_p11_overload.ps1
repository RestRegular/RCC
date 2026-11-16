# ============================================================================
#  P11 —— 重载 + mangling 验收测试
#
#  设计前提（P11 的核心决策，见 docs/p11_plan.md）：
#    符号策略是**按定义个数分流的** ——
#      * 名字只有一个定义  -> 符号就是裸名（rt.o / core_win64.o 的 ABI 一字不动）
#      * 名字有多个定义    -> 全部修饰成 _R<len><name><形参编码>
#    mangling 编的是**形参类型**（未标注编成 v），与 P10 编类型实参的形状一致
#    但内容不同：`fun f(x: int)` -> _R1fi，`fun f(x)` -> _R1fv。
#
#  用例分三类：
#    正向 —— 编译通过、运行输出符合预期（验证真的决议到了正确的那一份）
#    决议 —— 隐式转换 / 默认值 / 命名实参参与决议（验证打分与绑定）
#    反向 —— 编译必须失败，且诊断信息里必须出现指定片段
#
#  用法：powershell -NoProfile -ExecutionPolicy Bypass -File test_scripts/test_p11_overload.ps1
# ============================================================================

$ErrorActionPreference = 'Stop'

$repo = Split-Path -Parent $PSScriptRoot
$rcc = Join-Path $repo 'release\RCC.exe'
$work = Join-Path $repo 'build\p11_overload_verify'

if (-not (Test-Path $rcc)) {
    Write-Host "找不到编译器：$rcc" -ForegroundColor Red
    Write-Host "请先构建：cmake --build build" -ForegroundColor Yellow
    exit 1
}

if (Test-Path $work) { Remove-Item $work -Recurse -Force }
New-Item -ItemType Directory -Force -Path $work | Out-Null

$utf8NoBom = New-Object System.Text.UTF8Encoding($false)
# 每个用例都以这个桩开头，模拟运行时提供的 sout。
$soutStub = "fun sout(*args, end=`"\n`"): void { encapsulated }`n"

function Write-Source {
    param([string]$Name, [string]$Body)
    $path = Join-Path $work "$Name.rio"
    [System.IO.File]::WriteAllText($path, $soutStub + $Body, $utf8NoBom)
    return $path
}

# 编译并运行，返回 @{ Ok; Output; ExitCode }。
function Invoke-Case {
    param([string]$Name, [string]$Body, [switch]$ExpectFailure)
    $src = Write-Source -Name $Name -Body $Body
    $exe = Join-Path $work "$Name.exe"
    if (Test-Path $exe) { Remove-Item $exe -Force }

    # PowerShell 5.1 在 Stop 下会把原生命令的 stderr 当终止错误，
    # 而 RCC 的诊断正是写 stderr 的，这里必须临时放宽。
    $saved = $ErrorActionPreference
    $ErrorActionPreference = 'Continue'
    try {
        $buildOut = & $rcc -build --path $src --output $exe 2>&1 | Out-String
        $code = $LASTEXITCODE
    } finally {
        $ErrorActionPreference = $saved
    }

    if ($ExpectFailure) {
        return @{ Ok = ($code -ne 0 -or $buildOut -match 'Code generation failed'); Output = $buildOut; ExitCode = $code }
    }
    if ($code -ne 0 -or -not (Test-Path $exe)) {
        return @{ Ok = $false; Output = $buildOut; ExitCode = $code }
    }
    $runOut = (& $exe 2>&1 | Out-String).Trim() -replace "`r`n", "`n"
    return @{ Ok = $true; Output = $runOut; ExitCode = $code }
}

$pass = 0
$fail = 0
$failed = @()

# ---------------------------------------------------------------------------
# 正向用例：按形参类型决议到不同的实现
# ---------------------------------------------------------------------------
$positive = [ordered]@{
    # 三种标量族各一份实现，按实参类型各走各的。
    'pos_three_families' = @{
        Body   = "fun show(x: int): void { sout(`"int`") }`nfun show(x: float): void { sout(`"float`") }`nfun show(x: str): void { sout(`"str`") }`nfun main(): void {`n    show(42)`n    show(2.5)`n    show(`"hi`")`n}`nmain()`n"
        Expect = "int`nfloat`nstr"
    }
    # 参数个数不同（arity 决议）。
    'pos_arity' = @{
        Body   = "fun f(a: int): int { ret a }`nfun f(a: int, b: int): int { ret a + b }`nfun main(): void {`n    sout(f(7))`n    sout(f(3, 4))`n}`nmain()`n"
        Expect = "7`n7"
    }
    # 返回类型跟着决议走：float 版本返回 double，不能按 int 打印。
    'pos_return_type' = @{
        Body   = "fun pick(x: int): int { ret x }`nfun pick(x: float): float { ret x * 2.0 }`nfun main(): void {`n    var a = pick(3)`n    var b = pick(1.25)`n    sout(a)`n    sout(b)`n}`nmain()`n"
        Expect = "3`n2.5"
    }
    # 未标注形参是通配符，但精确匹配优先（`f(int)` 抢过 `f(x)`）。
    'pos_annotated_wins' = @{
        Body   = "fun f(x: int): void { sout(`"exact`") }`nfun f(x): void { sout(`"wild`") }`nfun main(): void {`n    f(1)`n    f(`"s`")`n}`nmain()`n"
        Expect = "exact`nwild"
    }
    # 指针族之间可互通，但精确的 str 版本优先。
    'pos_pointer_prefers_exact' = @{
        Body   = "fun g(x: str): void { sout(`"str`") }`nfun g(x: list): void { sout(`"list`") }`nfun main(): void {`n    g(`"hello`")`n    g([1, 2])`n}`nmain()`n"
        Expect = "str`nlist"
    }
    # 类类型参与决议。
    'pos_class_param' = @{
        Body   = "class Point {`n    var x: int`n    ctor(v: int) { this.x = v }`n}`nfun show(p: Point): void { sout(`"point `", p.x) }`nfun show(n: int): void { sout(`"int `", n) }`nfun main(): void {`n    var p = Point(9)`n    show(p)`n    show(5)`n}`nmain()`n"
        Expect = "point 9`nint 5"
    }
    # 重载函数之间互相调用（决议发生在函数体里，不是只在 main 里）。
    'pos_nested_call' = @{
        Body   = "fun h(x: int): int { ret x + 1 }`nfun h(x: str): str { ret x }`nfun twice(x: int): int { ret h(h(x)) }`nfun main(): void {`n    sout(twice(40))`n}`nmain()`n"
        Expect = '42'
    }
    # 单一名字（非重载）必须保持裸名符号，行为与 P10 完全一致。
    'pos_single_stays_bare' = @{
        Body   = "fun only(x: int): int { ret x * 3 }`nfun main(): void {`n    sout(only(14))`n}`nmain()`n"
        Expect = '42'
    }
    # 两个重载 + 一个非重载，混合场景下各走各的符号。
    'pos_mixed' = @{
        Body   = "fun dup(x: int): void { sout(`"dup-int`") }`nfun dup(x: float): void { sout(`"dup-float`") }`nfun solo(x: int): void { sout(`"solo`") }`nfun main(): void {`n    dup(1)`n    dup(1.5)`n    solo(2)`n}`nmain()`n"
        Expect = "dup-int`ndup-float`nsolo"
    }
}

# ---------------------------------------------------------------------------
# 决议用例：隐式转换、默认值、命名实参
# ---------------------------------------------------------------------------
$resolution = [ordered]@{
    # int 实参可以喂给 float 形参（数值族互通），走 float 那一份。
    'res_numeric_widen' = @{
        Body   = "fun f(x: int): void { sout(`"int`") }`nfun f(x: float): void { sout(`"float`") }`nfun main(): void {`n    f(1)`n    f(2.5)`n}`nmain()`n"
        Expect = "int`nfloat"
    }
    # 默认值参与候选过滤：只给一个实参也能匹配两形参的那一份。
    'res_default_arg' = @{
        Body   = "fun f(a: int, b: int = 10): int { ret a + b }`nfun f(a: str): str { ret a }`nfun main(): void {`n    sout(f(5))`n    sout(f(`"x`"))`n}`nmain()`n"
        Expect = "15`nx"
    }
    # 命名实参参与绑定：b= 指定到第二个形参上。
    'res_named_arg' = @{
        Body   = "fun f(a: int, b: int): int { ret a * 100 + b }`nfun f(s: str): str { ret s }`nfun main(): void {`n    sout(f(b=2, a=1))`n    sout(f(`"z`"))`n}`nmain()`n"
        Expect = "102`nz"
    }
    # 三个重载里选唯一最佳（同族优先）。
    'res_best_of_three' = @{
        Body   = "fun k(x: int): void { sout(`"i`") }`nfun k(x: float): void { sout(`"f`") }`nfun k(x: bool): void { sout(`"b`") }`nfun main(): void {`n    k(1)`n    k(1.5)`n    k(true)`n}`nmain()`n"
        Expect = "i`nf`nb"
    }
}

# ---------------------------------------------------------------------------
# 反向用例：必须报错，且诊断里要出现指定片段
# ---------------------------------------------------------------------------
$negative = [ordered]@{
    # 没有任何候选能吃下这个实参。
    'neg_no_match' = @{
        Body   = "fun f(x: int): void { sout(x) }`nfun f(x: float): void { sout(x) }`nfun main(): void {`n    f(`"str`")`n}`nmain()`n"
        Expect = "No matching overload for 'f'"
    }
    # 实参个数对不上任何一份（少给且无默认值）。
    'neg_arity_mismatch' = @{
        Body   = "fun f(a: int): int { ret a }`nfun f(a: int, b: int): int { ret a + b }`nfun main(): void {`n    f(1, 2, 3)`n}`nmain()`n"
        Expect = "No matching overload for 'f'"
    }
    # 两个候选同分：不猜，明确报歧义。
    # 用 str / list 两份指针族签名 + 一个类实例实参 —— 指针族互通让它俩
    # 都得 30 分，谁也不算更好，这正是「该报歧义」的形状。
    'neg_ambiguous' = @{
        Body   = "class P {`n    var x: int`n    ctor(v: int) { this.x = v }`n}`nfun f(x: str): void { sout(`"str`") }`nfun f(x: list): void { sout(`"list`") }`nfun main(): void {`n    var p = P(1)`n    f(p)`n}`nmain()`n"
        Expect = "Ambiguous call to overloaded function 'f'"
    }
    # 同一签名写两遍：符号相同，必须报重复定义而不是让汇编器去炸。
    # 期望片段刻意避开引号：控制台折行会在 `'` 与 `f'` 之间插一个换行，
    # 压平空白后变成 `' f'`，带引号的期望片段永远匹配不上。
    'neg_duplicate_signature' = @{
        Body   = "fun f(x: int): int { ret x }`nfun f(x: int): int { ret x + 1 }`nfun main(): void {`n    sout(f(1))`n}`nmain()`n"
        Expect = "Duplicate definition of overloaded function"
    }
    # 没有任何候选能吃下这组实参时，诊断必须把候选签名（含标注原文）列出来，
    # 否则用户看不出到底有哪几份可选。
    'neg_candidate_listing' = @{
        Body   = "fun f(a: int, b: int): void { sout(a) }`nfun f(a: str, b: str): void { sout(a) }`nfun main(): void {`n    f(1, `"x`")`n}`nmain()`n"
        Expect = "candidates: fun f(a: int, b: int); fun f(a: str, b: str)"
    }
}

# ---------------------------------------------------------------------------
# 执行
# ---------------------------------------------------------------------------
function Invoke-Suite {
    param([string]$Title, $Cases, [bool]$ExpectFailure)
    Write-Host ""
    Write-Host $Title -ForegroundColor Cyan
    foreach ($name in $Cases.Keys) {
        $case = $Cases[$name]
        $r = Invoke-Case -Name $name -Body $case.Body -ExpectFailure:$ExpectFailure
        if ($ExpectFailure) {
            if (-not $r.Ok) {
                Write-Host ("  [失败] {0} —— 本应报错却编译通过了" -f $name) -ForegroundColor Red
                $script:fail++; $script:failed += $name
                continue
            }
            # 诊断可能被控制台宽度折行（RCC 的前缀本身就占了大半行），
            # 匹配前先把连续空白压成单个空格，否则长诊断永远匹配不上。
            $flat = ($r.Output -replace '\s+', ' ')
            if ($flat -like "*$($case.Expect)*") {
                Write-Host ("  [通过] {0}" -f $name) -ForegroundColor Green
                $script:pass++
            } else {
                Write-Host ("  [失败] {0} —— 诊断信息不含期望片段" -f $name) -ForegroundColor Red
                Write-Host ("         期望包含：{0}" -f $case.Expect) -ForegroundColor DarkGray
                $first = ($r.Output -split "`n" | Where-Object { $_ -match 'Error' } | Select-Object -First 1)
                if ($first) { Write-Host ("         实际：{0}" -f $first.Trim()) -ForegroundColor DarkGray }
                $script:fail++; $script:failed += $name
            }
            continue
        }
        if (-not $r.Ok) {
            Write-Host ("  [失败] {0} —— 编译失败" -f $name) -ForegroundColor Red
            $first = ($r.Output -split "`n" | Where-Object { $_ -match 'Error' } | Select-Object -First 1)
            if ($first) { Write-Host ("         {0}" -f $first.Trim()) -ForegroundColor DarkGray }
            $script:fail++; $script:failed += $name
            continue
        }
        if ($r.Output -eq $case.Expect) {
            Write-Host ("  [通过] {0}" -f $name) -ForegroundColor Green
            $script:pass++
        } else {
            Write-Host ("  [失败] {0} —— 输出不符" -f $name) -ForegroundColor Red
            Write-Host ("         期望：{0}" -f ($case.Expect -replace "`n", ' / ')) -ForegroundColor DarkGray
            Write-Host ("         实际：{0}" -f ($r.Output -replace "`n", ' / ')) -ForegroundColor DarkGray
            $script:fail++; $script:failed += $name
        }
    }
}

Invoke-Suite -Title '正向用例（按形参类型决议）' -Cases $positive -ExpectFailure $false
Invoke-Suite -Title '决议用例（隐式转换 / 默认值 / 命名实参）' -Cases $resolution -ExpectFailure $false
Invoke-Suite -Title '反向用例（必须报错）' -Cases $negative -ExpectFailure $true

Write-Host ""
Write-Host ("结果：通过 {0}，失败 {1}" -f $pass, $fail)
if ($fail -gt 0) {
    Write-Host ("失败用例：{0}" -f ($failed -join ', ')) -ForegroundColor Yellow
    exit 1
}

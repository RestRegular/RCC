# ============================================================================
#  P9 —— 静态化（类型标注权威化 + 类型错误诊断）验收测试
#
#  两类用例：
#    正向 —— 编译通过、运行输出符合预期（验证标注真的影响了代码生成）
#    反向 —— 编译必须失败，且诊断信息里必须出现指定的片段（验证能报错、
#            且报在正确的文件/行/列上）
#
#  用法：powershell -NoProfile -ExecutionPolicy Bypass -File test_scripts/test_p9_types.ps1
# ============================================================================

$ErrorActionPreference = 'Stop'

$repo = Split-Path -Parent $PSScriptRoot
$rcc = Join-Path $repo 'release\RCC.exe'
$work = Join-Path $repo 'build\p9_types_verify'

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
# 正向用例：标注必须真正生效
# ---------------------------------------------------------------------------
$positive = [ordered]@{
    # 标注为权威类型：int 字面量存进 float 槽必须变成 1.0，而不是 1。
    'pos_float_annot_int_init' = @{
        Body   = "fun main(): void {`n    var x: float = 1`n    sout(x)`n}`nmain()`n"
        Expect = '1.0'
    }
    # 未标注的 float 局部变量维持推断行为（legacy 依赖这条）。
    'pos_float_infer' = @{
        Body   = "fun main(): void {`n    var x = 1.5`n    sout(x)`n}`nmain()`n"
        Expect = '1.5'
    }
    # 形参标注 float，实参传 int 要在调用点转成 double。
    'pos_param_float_from_int' = @{
        Body   = "fun half(a: float): float { ret a / 2.0 }`nfun main(): void {`n    sout(half(3))`n}`nmain()`n"
        Expect = '1.5'
    }
    # 标注 int 的形参 + int 实参。
    'pos_param_int' = @{
        Body   = "fun twice(n: int): int { ret n * 2 }`nfun main(): void {`n    var b: int = 7`n    sout(twice(b))`n}`nmain()`n"
        Expect = '14'
    }
    # float 槽 + int 右值的复合赋值。
    'pos_float_compound' = @{
        Body   = "fun main(): void {`n    var f: float = 1.5`n    f = f + 1`n    sout(f)`n}`nmain()`n"
        Expect = '2.5'
    }
    # 显式标注 str。
    'pos_str_annot' = @{
        Body   = "fun main(): void {`n    var s: str = `"hi`"`n    sout(s)`n}`nmain()`n"
        Expect = 'hi'
    }
    # 显式标注 bool。
    'pos_bool_annot' = @{
        Body   = "fun main(): void {`n    var b: bool = true`n    sout(b)`n}`nmain()`n"
        Expect = 'true'
    }
    # 标注 float 的返回值，函数体 ret 一个 int 表达式：允许（数值族）。
    'pos_ret_float_from_int' = @{
        Body   = "fun one(): float { ret 1 }`nfun main(): void {`n    sout(one())`n}`nmain()`n"
        Expect = '1.0'
    }
    # 未标注的形参照旧可被传闭包（legacy test_7_anonymous 的写法）。
    # apply 收到的闭包是 add(a, b)，这里传一个单参闭包，返回 5*2。
    'pos_unannotated_param_closure' = @{
        Body   = "fun apply(f, x) { ret f(x) }`nfun main(): void {`n    var dbl = (a) -> { ret a * 2 }`n    sout(apply(dbl, 5))`n}`nmain()`n"
        Expect = '10'
    }
    # 未标注的变量照旧按初始值推断（legacy 全量依赖）。
    'pos_unannotated_infer' = @{
        Body   = "fun main(): void {`n    var x = 100`n    var z = x + 1`n    sout(z)`n}`nmain()`n"
        Expect = '101'
    }
}

foreach ($name in $positive.Keys) {
    $case = $positive[$name]
    $r = Invoke-Case -Name $name -Body $case.Body
    if (-not $r.Ok) {
        Write-Host ("  [失败] {0} —— 编译失败" -f $name) -ForegroundColor Red
        $first = ($r.Output -split "`n" | Where-Object { $_ -match 'Error' } | Select-Object -First 1)
        if ($first) { Write-Host ("         {0}" -f $first.Trim()) -ForegroundColor DarkGray }
        $fail++; $failed += $name
        continue
    }
    if ($r.Output -eq $case.Expect) {
        Write-Host ("  [通过] {0}" -f $name) -ForegroundColor Green
        $pass++
    } else {
        Write-Host ("  [失败] {0} —— 输出不符" -f $name) -ForegroundColor Red
        Write-Host ("         期望：{0}" -f ($case.Expect -replace "`n", ' / ')) -ForegroundColor DarkGray
        Write-Host ("         实际：{0}" -f ($r.Output -replace "`n", ' / ')) -ForegroundColor DarkGray
        $fail++; $failed += $name
    }
}

# ---------------------------------------------------------------------------
# 反向用例：必须报错，且诊断里要出现指定片段
# ---------------------------------------------------------------------------
$negative = [ordered]@{
    # 变量标注与初始值冲突
    'neg_var_annot_mismatch' = @{
        Body   = "fun main(): void {`n    var x: int = `"hello`"`n}`nmain()`n"
        Expect = "Variable 'x' declared as int"
    }
    # 实参类型与形参标注冲突
    'neg_arg_mismatch' = @{
        Body   = "fun f(x: int): int { ret x }`nfun main(): void {`n    f(`"hello`")`n}`nmain()`n"
        Expect = "Argument 1 of 'f'"
    }
    # 返回类型标注与 ret 值冲突
    'neg_ret_mismatch' = @{
        Body   = "fun g(): str { ret 42 }`nfun main(): void {`n    sout(g())`n}`nmain()`n"
        Expect = "Return value in 'g'"
    }
    # 赋值时把 str 塞进 int 槽
    'neg_assign_mismatch' = @{
        Body   = "fun main(): void {`n    var x = 1`n    x = `"str`"`n}`nmain()`n"
        Expect = "Assignment to 'x'"
    }
    # 变量声明里的类型名拼错
    'neg_unknown_var_type' = @{
        Body   = "fun main(): void {`n    var x: Piont = 1`n}`nmain()`n"
        Expect = "Unknown type 'Piont' in declaration of 'x'"
    }
    # 形参类型名拼错
    'neg_unknown_param_type' = @{
        Body   = "fun f(a: Piont): int { ret 1 }`nfun main(): void {`n    f(1)`n}`nmain()`n"
        Expect = "Unknown type 'Piont' of parameter 'a'"
    }
    # 返回类型名拼错
    'neg_unknown_ret_type' = @{
        Body   = "fun g(): Piont { ret 1 }`nfun main(): void {`n    g()`n}`nmain()`n"
        Expect = "Unknown return type 'Piont'"
    }
    # void 函数里 ret 一个值
    'neg_void_ret_value' = @{
        Body   = "fun g(): void { ret 1 }`nfun main(): void {`n    g()`n}`nmain()`n"
        Expect = "declared to return void"
    }
    # bool 标注 + str 初始值
    'neg_bool_from_str' = @{
        Body   = "fun main(): void {`n    var b: bool = `"yes`"`n}`nmain()`n"
        Expect = "declared as bool"
    }
    # list 标注 + int 初始值
    'neg_list_from_int' = @{
        Body   = "fun main(): void {`n    var l: list = 5`n}`nmain()`n"
        Expect = "declared as list"
    }
}

foreach ($name in $negative.Keys) {
    $case = $negative[$name]
    $r = Invoke-Case -Name $name -Body $case.Body -ExpectFailure
    if (-not $r.Ok) {
        Write-Host ("  [失败] {0} —— 本应报错却编译通过了" -f $name) -ForegroundColor Red
        $fail++; $failed += $name
        continue
    }
    if ($r.Output -like "*$($case.Expect)*") {
        Write-Host ("  [通过] {0}" -f $name) -ForegroundColor Green
        $pass++
    } else {
        Write-Host ("  [失败] {0} —— 诊断信息不含期望片段" -f $name) -ForegroundColor Red
        Write-Host ("         期望包含：{0}" -f $case.Expect) -ForegroundColor DarkGray
        $first = ($r.Output -split "`n" | Where-Object { $_ -match 'Error' } | Select-Object -First 1)
        if ($first) { Write-Host ("         实际：{0}" -f $first.Trim()) -ForegroundColor DarkGray }
        $fail++; $failed += $name
    }
}

Write-Host ""
Write-Host ("结果：通过 {0}，失败 {1}" -f $pass, $fail)
if ($fail -gt 0) {
    Write-Host ("失败用例：{0}" -f ($failed -join ', ')) -ForegroundColor Yellow
    exit 1
}

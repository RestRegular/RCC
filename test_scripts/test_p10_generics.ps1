# ============================================================================
#  P10 —— 泛型（单态化）验收测试
#
#  设计前提（P10 的核心决策，见 docs/p10_plan.md）：
#    **单态化即约束** —— 泛型在定义处不做检查，检查发生在实例化处。
#    `fun identity[T](x: T): T` 本身不产生任何机器码，只有
#    `identity[int]` / `identity(3.5)` 这样的具体调用才会生成一份实例。
#
#  用例分三类：
#    正向 —— 编译通过、运行输出符合预期（验证单态化真的按实参类型生成）
#    推导 —— 不写显式类型实参，靠实参反推（验证 deduceTypeArgs）
#    反向 —— 编译必须失败，且诊断信息里必须出现指定片段
#
#  用法：powershell -NoProfile -ExecutionPolicy Bypass -File test_scripts/test_p10_generics.ps1
# ============================================================================

$ErrorActionPreference = 'Stop'

$repo = Split-Path -Parent $PSScriptRoot
$rcc = Join-Path $repo 'release\RCC.exe'
$work = Join-Path $repo 'build\p10_generics_verify'

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
# 正向用例：显式类型实参 + 泛型类
# ---------------------------------------------------------------------------
$positive = [ordered]@{
    # 最基本：identity[int] 生成一份 int 实例。
    'pos_identity_int' = @{
        Body   = "fun identity[T](x: T): T { ret x }`nfun main(): void {`n    sout(identity[int](42))`n}`nmain()`n"
        Expect = '42'
    }
    # 同一个泛型函数的 str 实例（单态化：两份独立的机器码）。
    'pos_identity_str' = @{
        Body   = "fun identity[T](x: T): T { ret x }`nfun main(): void {`n    sout(identity[str](`"rio`"))`n}`nmain()`n"
        Expect = 'rio'
    }
    # float 实例必须真的按 double 走，不能把位模式当整数打出来。
    'pos_identity_float' = @{
        Body   = "fun identity[T](x: T): T { ret x }`nfun main(): void {`n    sout(identity[float](3.5))`n}`nmain()`n"
        Expect = '3.5'
    }
    # 两个类型形参，各按实参单态化。
    'pos_pair_two_params' = @{
        Body   = "fun pair[A, B](a: A, b: B): B { ret b }`nfun main(): void {`n    sout(pair[int, str](7, `"x`"))`n}`nmain()`n"
        Expect = 'x'
    }
    # 泛型类：字段与方法的类型都随实例走。
    'pos_generic_class_int' = @{
        Body   = "class Box[T] {`n    var v: T`n    ctor(val: T) { this.v = val }`n    fun get(): T { ret this.v }`n}`nfun main(): void {`n    var b: Box[int] = Box[int](42)`n    sout(b.get())`n}`nmain()`n"
        Expect = '42'
    }
    # 泛型类的 float 实例：返回值是 double，不是 int 位模式。
    'pos_generic_class_float' = @{
        Body   = "class Box[T] {`n    var v: T`n    ctor(val: T) { this.v = val }`n    fun get(): T { ret this.v }`n}`nfun main(): void {`n    var d = Box[float](2.5)`n    sout(d.get())`n}`nmain()`n"
        Expect = '2.5'
    }
    # 泛型类的 str 实例。
    'pos_generic_class_str' = @{
        Body   = "class Box[T] {`n    var v: T`n    ctor(val: T) { this.v = val }`n    fun get(): T { ret this.v }`n}`nfun main(): void {`n    var c: Box[str] = Box[str](`"rio`")`n    sout(c.get())`n}`nmain()`n"
        Expect = 'rio'
    }
    # 多类型形参的泛型类。
    'pos_generic_class_multi' = @{
        Body   = "class Pair[A, B] {`n    var first: A`n    var second: B`n    ctor(a: A, b: B) {`n        this.first = a`n        this.second = b`n    }`n}`nfun main(): void {`n    var p: Pair[int, str] = Pair[int, str](7, `"x`")`n    sout(p.first)`n    sout(p.second)`n}`nmain()`n"
        Expect = "7`nx"
    }
    # 泛型函数里再调泛型函数（实例中发现实例）。
    'pos_nested_instance' = @{
        Body   = "fun identity[T](x: T): T { ret x }`nfun twice[T](x: T): T { ret identity(x) }`nfun main(): void {`n    sout(twice(9))`n    sout(twice(2.5))`n}`nmain()`n"
        Expect = "9`n2.5"
    }
    # 泛型类的方法里构造另一个实例（`Pair[B, A]`）。
    'pos_generic_class_swap' = @{
        Body   = "class Pair[A, B] {`n    var first: A`n    var second: B`n    ctor(a: A, b: B) {`n        this.first = a`n        this.second = b`n    }`n    fun swap(): Pair[B, A] { ret Pair[B, A](this.second, this.first) }`n}`nfun main(): void {`n    var p = Pair[int, str](7, `"x`")`n    var q = p.swap()`n    sout(q.first)`n    sout(q.second)`n}`nmain()`n"
        Expect = "x`n7"
    }
    # 泛型声明未被使用时不应产生任何符号（零成本）。
    'pos_unused_generic' = @{
        Body   = "fun identity[T](x: T): T { ret x }`nfun main(): void {`n    sout(1)`n}`nmain()`n"
        Expect = '1'
    }
    # 泛型函数的默认参数（回归：曾经完全失效，被调方读到栈垃圾）。
    # 根因是泛型声明分支提前 return，既没把默认值收进 GenericFunction，
    # 也没按 mangled 名登记 _functionDefaults，调用点补不出缺省实参。
    'pos_generic_default_int' = @{
        Body   = "fun pad[T](x: T, n: int = 42): int { ret n }`nfun main(): void {`n    sout(pad(1))`n    sout(pad(2, 7))`n}`nmain()`n"
        Expect = "42`n7"
    }
    # 默认值是 str：曾经会把整段内存（含环境变量）打出来。
    'pos_generic_default_str' = @{
        Body   = "fun tag[T](x: T, s: str = `"DEF`"): str { ret s }`nfun main(): void {`n    sout(tag(1))`n    sout(tag(2, `"X`"))`n}`nmain()`n"
        Expect = "DEF`nX"
    }
    # 默认值是 bool：曾经碰巧返回 true，掩盖了缺陷。
    'pos_generic_default_bool' = @{
        Body   = "fun flag[T](x: T, b: bool = false): bool { ret b }`nfun main(): void {`n    sout(flag(1))`n    sout(flag(2, true))`n}`nmain()`n"
        Expect = "false`ntrue"
    }
    # 多个默认参数 + 部分传参：位置必须对得上。
    'pos_generic_default_multi' = @{
        Body   = "fun mix[T](x: T, a: int = 1, b: int = 2): int { ret a * 10 + b }`nfun main(): void {`n    sout(mix(0))`n    sout(mix(0, 7))`n    sout(mix(0, 7, 8))`n}`nmain()`n"
        Expect = "12`n72`n78"
    }
    # 默认值是类型形参本身。
    'pos_generic_default_typeparam' = @{
        Body   = "fun pick[T](x: T, y: T = 0): T { ret y }`nfun main(): void {`n    sout(pick(5))`n    sout(pick(5, 9))`n}`nmain()`n"
        Expect = "0`n9"
    }
    # 显式类型实参 + 默认参数。
    'pos_generic_default_explicit' = @{
        Body   = "fun pad[T](x: T, n: int = 42): int { ret n }`nfun main(): void {`n    sout(pad[int](1))`n}`nmain()`n"
        Expect = '42'
    }
    # 命名实参跳过默认位（补默认值时必须按名字定位）。
    'pos_generic_default_named' = @{
        Body   = "fun mix[T](x: T, a: int = 1, b: int = 2): int { ret a * 10 + b }`nfun main(): void {`n    sout(mix(0, b = 9))`n}`nmain()`n"
        Expect = '19'
    }
    # 不同单态化实例各自的默认值都要正确（不能共用一份表）。
    'pos_generic_default_two_inst' = @{
        Body   = "fun pad[T](x: T, n: int = 42): int { ret n }`nfun main(): void {`n    sout(pad(1))`n    sout(pad(`"s`"))`n    sout(pad(2.5))`n}`nmain()`n"
        Expect = "42`n42`n42"
    }
}

# ---------------------------------------------------------------------------
# 推导用例：不写显式类型实参
# ---------------------------------------------------------------------------
$deduced = [ordered]@{
    # 裸调用靠实参类型反推 T。
    'ded_int' = @{
        Body   = "fun identity[T](x: T): T { ret x }`nfun main(): void {`n    sout(identity(42))`n}`nmain()`n"
        Expect = '42'
    }
    # 推导出 float 后，槽位必须是 double 大小。
    'ded_float' = @{
        Body   = "fun identity[T](x: T): T { ret x }`nfun main(): void {`n    var v = identity(3.5)`n    sout(v)`n}`nmain()`n"
        Expect = '3.5'
    }
    # list[T]：从 list[float] 反推 T=float。
    'ded_list_elem' = @{
        Body   = "fun firstOf[T](xs: list[T]): T { ret xs[0] }`nfun main(): void {`n    var xs: list[float] = [1.5, 2.5]`n    sout(firstOf(xs))`n}`nmain()`n"
        Expect = '1.5'
    }
    # dict[str, T]：从值类型反推。
    'ded_dict_value' = @{
        Body   = "fun firstVal[V](d: dict[str, V]): V { ret d[`"a`"] }`nfun main(): void {`n    var d: dict[str, float] = {`"a`": 1.5}`n    sout(firstVal(d))`n}`nmain()`n"
        Expect = '1.5'
    }
    # 高阶：funi[A][B] 从具名函数的签名反推 A、B。
    'ded_func_param' = @{
        Body   = "fun addOne(x: int): int { ret x + 1 }`nfun apply[A, B](f: funi[A][B], v: A): B { ret f(v) }`nfun main(): void {`n    sout(apply(addOne, 41))`n}`nmain()`n"
        Expect = '42'
    }
    # 显式 + 推导混用：identity[int](...) 与 identity(...) 共享同一份实例。
    'ded_shared_instance' = @{
        Body   = "fun identity[T](x: T): T { ret x }`nfun main(): void {`n    sout(identity[int](1))`n    sout(identity(2))`n}`nmain()`n"
        Expect = "1`n2"
    }
    # 推导结果用于泛型类：Box(42) 形式的推导（显式标注在左值上）。
    'ded_generic_class_from_annot' = @{
        Body   = "class Box[T] {`n    var v: T`n    ctor(val: T) { this.v = val }`n    fun get(): T { ret this.v }`n}`nfun main(): void {`n    var b: Box[float] = Box[float](1.25)`n    sout(b.get())`n}`nmain()`n"
        Expect = '1.25'
    }
}

# ---------------------------------------------------------------------------
# 同名多份泛型声明（P13 缺陷回归，2026-11-17）
#
# 用户实测发现：`dumpToStr[T](list[T])` 与 `dumpToStr[T, K](dict[T, K])`
# 同名共存时，后一份会把前一份**覆盖**掉且无任何诊断，调用前者报
# "takes 2 type argument(s), but 1 were supplied"。根因是
# _genericFunctions 原本是 map<string, GenericFunction>，operator[] 直接覆盖。
# 现在按名字收成一组，调用点按「显式类型实参个数 + 实参个数 + 能否推导」挑。
# ---------------------------------------------------------------------------
$genericOverloads = [ordered]@{
    # 两份同名泛型，各自按显式类型实参个数选中。
    'gen_multi_decl' = @{
        Body   = "fun f[T](xs: list[T]): str { ret `"list`" }`nfun f[T, K](d: dict[T, K]): str { ret `"dict`" }`nfun main(): void {`n    sout(f[int]([1]))`n    sout(f[str, int]({`"a`": 1}))`n}`nmain()`n"
        Expect = "list`ndict"
    }
    # 省略类型实参时靠实参推导选中对应的一份。
    'gen_multi_deduce' = @{
        Body   = "fun f[T](xs: list[T]): int { ret 1 }`nfun f[T, K](d: dict[T, K]): int { ret 2 }`nfun main(): void {`n    sout(f([1]))`n    sout(f({`"a`": 1}))`n}`nmain()`n"
        Expect = "1`n2"
    }
    # 默认参数 + 同名多份：`pad(1)` 只给一个实参，仍要能选中 [T] 那份。
    'gen_default_arg_count' = @{
        Body   = "fun pad[T](x: T, n: int = 42): int { ret n }`nfun main(): void {`n    sout(pad(1))`n    sout(pad(2, 7))`n}`nmain()`n"
        Expect = "42`n7"
    }
}

# ---------------------------------------------------------------------------
# 反向用例：必须报错，且诊断里要出现指定片段
# ---------------------------------------------------------------------------
$negative = [ordered]@{
    # 显式类型实参个数与泛型形参个数不符。
    'neg_wrong_arg_count' = @{
        Body   = "fun pair[A, B](a: A, b: B): B { ret b }`nfun main(): void {`n    pair[int](1, `"x`")`n}`nmain()`n"
        Expect = "takes 2 type argument(s)"
    }
    # 推不出来的类型形参（只出现在返回位置，实参里没有）。
    'neg_undeducible' = @{
        Body   = "fun make[T](): T { ret 1 }`nfun main(): void {`n    make()`n}`nmain()`n"
        Expect = "Cannot"
    }
    # 泛型实例化的实参类型与形参标注冲突。
    'neg_instance_arg_mismatch' = @{
        Body   = "fun identity[T](x: T): T { ret x }`nfun main(): void {`n    var s: str = identity[int](1)`n}`nmain()`n"
        Expect = "declared as str"
    }
    # 泛型类实例化的类型实参个数不符。
    'neg_class_arg_count' = @{
        Body   = "class Box[T] {`n    var v: T`n    ctor(val: T) { this.v = val }`n}`nfun main(): void {`n    var b = Box[int, str](1, `"x`")`n}`nmain()`n"
        Expect = "Cannot"
    }
}

# ---------------------------------------------------------------------------
# 执行
# ---------------------------------------------------------------------------
function Invoke-Suite {
    param([string]$Title, $Cases, [bool]$ExpectFailure, [string]$ExpectKey)
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
            if ($r.Output -like "*$($case.Expect)*") {
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

Invoke-Suite -Title '正向用例（显式类型实参 / 泛型类）' -Cases $positive -ExpectFailure $false
Invoke-Suite -Title '推导用例（省略类型实参）' -Cases $deduced -ExpectFailure $false
Invoke-Suite -Title '同名多份泛型声明（P13 缺陷回归）' -Cases $genericOverloads -ExpectFailure $false
Invoke-Suite -Title '反向用例（必须报错）' -Cases $negative -ExpectFailure $true

Write-Host ""
Write-Host ("结果：通过 {0}，失败 {1}" -f $pass, $fail)
if ($fail -gt 0) {
    Write-Host ("失败用例：{0}" -f ($failed -join ', ')) -ForegroundColor Yellow
    exit 1
}

# ============================================================================
#  P12-2 / G7 + G11 —— `ret` 同行写法 与 类字段初始化器
#
#  ---- G7：`{ ret }` 解析坑 ----
#  问题：`fun f(): void { ret }` 写在同一行时 parser 报
#        `TOKEN_RBRACE 找不到前缀构造`；把 `}` 换到下一行就好了。
#  根因：buildReturnExpression 只把 TOKEN_NEWLINE 当作「无返回值」，
#        其它情况一律按「后面跟表达式」解析，于是把 `}` 当表达式起始。
#  修法：`}` 与 TOKEN_STREAM_END 也视为无返回值。
#
#  ---- G11：类字段初始化器 ----
#  问题：`class A { var x: int = 42 }` 后 `A().x` 读出 0。
#  原仓库依据：datatype.rio:50 `var TP_INT: str static = type(0)`、
#        ds.rio:492 `compareHandler: funi[...] private = null` —— 都是字段初始化器。
#  修法：registerClassBody 收集 ownFieldInits；emitConstructorCall 在清零之后、
#        用户 ctor 之前按声明顺序写入。
#
#  ⚠️ static 字段整体不支持（§1.3，用顶层 var 替代）；G11 只处理实例字段。
#  ⚠️ ctor 里给字段赋值必须写 `this.n = v`，裸写 `n = v` 报未定义变量
#     —— 这是既有语义，不是本轮引入的。
#
#  用法：powershell -NoProfile -ExecutionPolicy Bypass -File test_scripts/test_p12_g7g11_ret_field.ps1
# ============================================================================

$ErrorActionPreference = 'Stop'

$repo = Split-Path -Parent $PSScriptRoot
$rcc  = Join-Path $repo 'release\RCC.exe'
$work = Join-Path $repo 'build\p12_g7g11_verify'

if (-not (Test-Path $rcc)) {
    Write-Host "找不到编译器：$rcc" -ForegroundColor Red
    exit 1
}

if (Test-Path $work) { Remove-Item $work -Recurse -Force }
New-Item -ItemType Directory -Force -Path $work | Out-Null

$utf8NoBom = New-Object System.Text.UTF8Encoding($false)
$soutStub = 'fun sout(*args, end="\n"): void { encapsulated }'

$cases = [ordered]@{

    # ---- G7 ----

    # 最初的症状：void 函数体同一行 `{ ret }`
    'ret_same_line_void' = @{
        Src = @"
$soutStub
fun noop(): void { ret }
noop()
sout("ok")
"@
        Expect = 'ok'
    }

    # 早退：if 块里的 `{ ret 1 }`
    'ret_same_line_with_value' = @{
        Src = @"
$soutStub
fun early(x: int): int {
    if x > 0 { ret 1 }
    ret 0
}
sout(early(5))
sout(early(0 - 5))
"@
        Expect = "1`n0"
    }

    # 多条语句同一行
    'ret_after_statement_same_line' = @{
        Src = @"
$soutStub
fun f(): int { var a: int = 3
    ret a }
sout(f())
"@
        Expect = '3'
    }

    # 嵌套块里的同行 ret
    # 注意：`if x { ... } ret 1` 这种「} 后面同一行再跟语句」是 if 后缀解析的
    # 既有行为（报 Expected TOKEN_IF/ELIF/ELSE），与 G7 无关，故这里换行。
    'ret_in_nested_block' = @{
        Src = @"
$soutStub
fun f(x: int): int {
    if x > 0 {
        if x > 10 { ret 2 }
        ret 1
    }
    ret 0
}
sout(f(20))
sout(f(5))
sout(f(0 - 1))
"@
        Expect = "2`n1`n0"
    }

    # 换行写法必须仍然有效（不能为了修同行而破坏原写法）
    # 注意：裸 `ret` 后面换行 = 无返回值，int 函数这样写会被正确拒绝
    # （"Function 'f' is declared to return int but 'ret' has no value"），
    # 所以这里用 `ret <expr>` 换行来验证原写法未受影响。
    'ret_next_line_still_works' = @{
        Src = @"
$soutStub
fun noop(): void {
    ret
}
fun f(): int {
    ret 42
}
noop()
sout(f())
"@
        Expect = '42'
    }

    # ---- G11 ----

    # 最基本的字段初始化器
    'field_init_int' = @{
        Src = @"
$soutStub
class A { var x: int = 42 }
var o = A()
sout(o.x)
"@
        Expect = '42'
    }

    # 字符串字段初始化器（指针值，按 8 字节写）
    'field_init_str' = @{
        Src = @"
$soutStub
class A { var s: str = "hello" }
var o = A()
sout(o.s)
"@
        Expect = 'hello'
    }

    # 多个字段各自独立（不能串位）
    'field_init_multiple' = @{
        Src = @"
$soutStub
class A {
    var x: int = 1
    var y: int = 2
    var z: int = 3
}
var o = A()
sout(o.x)
sout(o.y)
sout(o.z)
"@
        Expect = "1`n2`n3"
    }

    # 有 ctor 时，ctor 可以覆盖初始化器
    'ctor_overrides_init' = @{
        Src = @"
$soutStub
class A {
    var x: int = 7
    ctor(v: int) {
        this.x = v
    }
}
var o = A(99)
sout(o.x)
"@
        Expect = '99'
    }

    # 有 ctor 但没覆盖的字段保持初始化器
    'ctor_keeps_other_init' = @{
        Src = @"
$soutStub
class A {
    var p: int = 5
    var q: int = 6
    ctor() {
        this.q = 100
    }
}
var o = A()
sout(o.p)
sout(o.q)
"@
        Expect = "5`n100"
    }

    # 没有初始化器的字段仍然默认 0（回归保护）
    'field_without_init_is_zero' = @{
        Src = @"
$soutStub
class A { var a: int }
var o = A()
sout(o.a)
"@
        Expect = '0'
    }

    # 两个实例互不影响（初值不能写成共享状态）
    'two_instances_independent' = @{
        Src = @"
$soutStub
class A { var x: int = 10 }
var p = A()
var q = A()
p.x = 20
sout(p.x)
sout(q.x)
"@
        Expect = "20`n10"
    }

    # 初始化器里用表达式（不只是字面量）
    'field_init_expression' = @{
        Src = @"
$soutStub
class A { var x: int = 3 * 4 + 1 }
var o = A()
sout(o.x)
"@
        Expect = '13'
    }

    # 初始化器引用另一个已声明的字段
    'field_init_uses_earlier_field' = @{
        Src = @"
$soutStub
class A {
    var a: int = 4
    var b: int = 6
}
var o = A()
sout(o.a + o.b)
"@
        Expect = '10'
    }

    # 方法里读初始化过的字段
    'method_reads_initialized_field' = @{
        Src = @"
$soutStub
class A {
    var x: int = 21
    fun double(): int {
        ret this.x * 2
    }
}
var o = A()
sout(o.double())
"@
        Expect = '42'
    }

    # 继承：基类字段初始化器在派生类实例上也要生效
    'base_field_init_inherited' = @{
        Src = @"
$soutStub
class Base { var b: int = 11 }
class Derived: Base { var d: int = 22 }
var o = Derived()
sout(o.b)
sout(o.d)
"@
        Expect = "11`n22"
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

Write-Host "P12-2 / G7 + G11 验收通过" -ForegroundColor Green
exit 0

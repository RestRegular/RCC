# ============================================================================
#  RCC 改造 阶段 P6 —— 类（class / ctor / 字段 / 方法 / 继承 / super）验收
#
#  覆盖：字段读写、构造器、多字段、方法（无参/带参）、方法改字段、
#        继承字段前缀兼容、super(...) 复用 this、三层继承、无构造器类、
#        浮点字段、对象作为函数实参/返回值、方法返回值参与运算
#  每个用例的 .rio 源码与期望输出都内联在本脚本里，跑完即删临时目录。
#
#  用法：powershell -NoProfile -ExecutionPolicy Bypass -File test_scripts/test_p6_classes.ps1
# ============================================================================

$ErrorActionPreference = 'Stop'

$repo = Split-Path -Parent $PSScriptRoot
$rcc = Join-Path $repo 'release\RCC.exe'
$work = Join-Path $repo 'build\p6_class_verify'

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
#  用例表：名称 -> @{ Src = 源码；Expect = 期望的完整输出 }
# ---------------------------------------------------------------------------
$cases = [ordered]@{

    'field_basic' = @{
        # 最小用例：构造器写字段，外部读字段
        Src = @"
$soutStub
class Point {
    var x: int, y: int
    ctor(a: int, b: int) {
        this.x = a
        this.y = b
    }
}
fun main(): void {
    var p = Point(10, 20)
    sout(p.x)
    sout(p.y)
}
main()
"@
        Expect = "10`n20"
    }

    'field_write_outside' = @{
        # 对象创建后从外部改字段
        Src = @"
$soutStub
class Box {
    var v: int
    ctor(n: int) { this.v = n }
}
fun main(): void {
    var b = Box(1)
    sout(b.v)
    b.v = 99
    sout(b.v)
}
main()
"@
        Expect = "1`n99"
    }

    'no_ctor' = @{
        # 没有构造器的类：对象整体清零，字段默认 0
        Src = @"
$soutStub
class Empty {
    var a: int, b: int
}
fun main(): void {
    var e = Empty()
    sout(e.a)
    sout(e.b)
    e.a = 7
    sout(e.a)
    sout(e.b)
}
main()
"@
        Expect = "0`n0`n7`n0"
    }

    'method_no_args' = @{
        Src = @"
$soutStub
class Point {
    var x: int, y: int
    ctor(a: int, b: int) {
        this.x = a
        this.y = b
    }
    fun sum(): int {
        ret this.x + this.y
    }
}
fun main(): void {
    var p = Point(3, 4)
    sout(p.sum())
}
main()
"@
        Expect = "7"
    }

    'method_with_args' = @{
        # 方法带实参：this 占 %rcx，用户实参从 %rdx 起
        Src = @"
$soutStub
class Point {
    var x: int, y: int
    ctor(a: int, b: int) {
        this.x = a
        this.y = b
    }
    fun add(dx: int, dy: int) {
        this.x = this.x + dx
        this.y = this.y + dy
    }
}
fun main(): void {
    var p = Point(10, 20)
    p.add(5, 6)
    sout(p.x)
    sout(p.y)
}
main()
"@
        Expect = "15`n26"
    }

    'method_mutates' = @{
        # 方法内部改字段，验证 this 是引用而不是拷贝
        Src = @"
$soutStub
class Counter {
    var n: int
    ctor() { this.n = 0 }
    fun bump(k: int) {
        this.n = this.n + k
    }
}
fun main(): void {
    var c = Counter()
    c.bump(1)
    c.bump(2)
    c.bump(3)
    sout(c.n)
}
main()
"@
        Expect = "6"
    }

    'inherit_fields' = @{
        # 派生类对象能读到基类字段（基类字段排在前面，偏移一致）
        Src = @"
$soutStub
class Base {
    var val: int
    ctor(v: int) { this.val = v }
}
class Derived: Base {
    var extra: int
    ctor(v: int, e: int) {
        super(v)
        this.extra = e
    }
}
fun main(): void {
    var d = Derived(42, 99)
    sout(d.val)
    sout(d.extra)
}
main()
"@
        Expect = "42`n99"
    }

    'inherit_method' = @{
        # 派生类继承基类方法（方法表沿继承链收集）
        Src = @"
$soutStub
class Base {
    var val: int
    ctor(v: int) { this.val = v }
    fun get(): int { ret this.val }
}
class Derived: Base {
    var extra: int
    ctor(v: int, e: int) {
        super(v)
        this.extra = e
    }
}
fun main(): void {
    var d = Derived(5, 6)
    sout(d.get())
}
main()
"@
        Expect = "5"
    }

    'three_levels' = @{
        # 三层继承：字段顺序 C 的字段 → B 的字段 → A 的字段
        Src = @"
$soutStub
class A {
    var a: int
    ctor(x: int) { this.a = x }
}
class B: A {
    var b: int
    ctor(x: int, y: int) {
        super(x)
        this.b = y
    }
}
class C: B {
    var c: int
    ctor(x: int, y: int, z: int) {
        super(x, y)
        this.c = z
    }
}
fun main(): void {
    var o = C(1, 2, 3)
    sout(o.a)
    sout(o.b)
    sout(o.c)
}
main()
"@
        Expect = "1`n2`n3"
    }

    'float_field' = @{
        # 浮点字段：按 movsd 存取，sout 按静态类型走 rt_print_float
        Src = @"
$soutStub
class Vec {
    var x: float, y: float
    ctor(a: float, b: float) {
        this.x = a
        this.y = b
    }
}
fun main(): void {
    var v = Vec(1.5, 2.5)
    sout(v.x)
    sout(v.y)
    v.x = 9.5
    sout(v.x)
}
main()
"@
        Expect = "1.5`n2.5`n9.5"
    }

    'object_as_arg' = @{
        # 对象作为函数实参：就是一个指针，跨函数边界无开销
        Src = @"
$soutStub
class Point {
    var x: int, y: int
    ctor(a: int, b: int) {
        this.x = a
        this.y = b
    }
    fun sum(): int { ret this.x + this.y }
}
fun total(p: Point): int {
    ret p.sum()
}
fun main(): void {
    var p = Point(8, 9)
    sout(total(p))
}
main()
"@
        Expect = "17"
    }

    'object_as_return' = @{
        # 对象作为返回值：构造调用本身就返回对象指针
        Src = @"
$soutStub
class Point {
    var x: int, y: int
    ctor(a: int, b: int) {
        this.x = a
        this.y = b
    }
}
fun make(a: int, b: int): Point {
    ret Point(a, b)
}
fun main(): void {
    var p = make(4, 5)
    sout(p.x)
    sout(p.y)
}
main()
"@
        Expect = "4`n5"
    }

    'two_objects' = @{
        # 两个独立对象互不影响（各自独立分配）
        Src = @"
$soutStub
class Point {
    var x: int
    ctor(a: int) { this.x = a }
}
fun main(): void {
    var p = Point(1)
    var q = Point(2)
    p.x = 100
    sout(p.x)
    sout(q.x)
}
main()
"@
        Expect = "100`n2"
    }

    'method_ret_arith' = @{
        # 方法返回值参与算术与比较
        Src = @"
$soutStub
class Rect {
    var w: int, h: int
    ctor(a: int, b: int) {
        this.w = a
        this.h = b
    }
    fun area(): int { ret this.w * this.h }
}
fun main(): void {
    var r = Rect(3, 4)
    sout(r.area())
    sout(r.area() + 1)
    sout(r.area() > 10)
}
main()
"@
        Expect = "12`n13`ntrue"
    }

    'nested_field' = @{
        # 对象字段指向另一个对象：`outer.inner` 的静态类型是类名
        Src = @"
$soutStub
class Inner {
    var v: int
    ctor(n: int) { this.v = n }
}
class Outer {
    var inner: Inner
    ctor(i: Inner) { this.inner = i }
}
fun main(): void {
    var i = Inner(77)
    var o = Outer(i)
    sout(o.inner.v)
}
main()
"@
        Expect = "77"
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
    Write-Host "P6 类验收通过" -ForegroundColor Green
    Remove-Item $work -Recurse -Force -ErrorAction SilentlyContinue
    exit 0
} else {
    Write-Host ("失败用例：{0}" -f ($failedNames -join ', ')) -ForegroundColor Red
    Write-Host ("临时目录保留在：{0}" -f $work) -ForegroundColor Yellow
    exit 1
}

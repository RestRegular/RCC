# ============================================================================
#  P12-2 / G5 —— 方法重载
#
#  问题：registerClassBody 一个方法名只留一份定义，`A.m(x: int)` 与
#        `A.m(x: str)` 后者覆盖前者。
#
#  修法：把 P11 在**全局函数**上的做法平移到方法上 ——
#    · 新增 ClassInfo::methodOverloads，同名方法的全部定义都保留
#    · 判据与全局函数完全一致：**单定义保持裸名**（`A.m`），
#      ≥2 个定义才修饰成 `<Class>.m_R<len><形参编码>`
#    · emitMethodCall 按实参类型打分决议（复用 overloadScore）
#    · 修饰名里带类名，避免两个类的方法互相撞符号
#    · 派生类声明同名方法时**整体遮蔽**基类同名重载集（名字隐藏，
#      与 Java/C++ 一致；不会出现「派生类一份 + 基类两份」的怪异组合）
#
#  ⚠️ 本实现不做可变参数 / 缺省实参的方法重载：候选的形参个数必须与
#     实参个数相等，否则不参与决议。
#
#  用法：powershell -NoProfile -ExecutionPolicy Bypass -File test_scripts/test_p12_g5_method_overload.ps1
# ============================================================================

$ErrorActionPreference = 'Stop'

$repo = Split-Path -Parent $PSScriptRoot
$rcc  = Join-Path $repo 'release\RCC.exe'
$work = Join-Path $repo 'build\p12_g5_verify'

if (-not (Test-Path $rcc)) {
    Write-Host "找不到编译器：$rcc" -ForegroundColor Red
    exit 1
}

if (Test-Path $work) { Remove-Item $work -Recurse -Force }
New-Item -ItemType Directory -Force -Path $work | Out-Null

$utf8NoBom = New-Object System.Text.UTF8Encoding($false)
$soutStub = 'fun sout(*args, end="\n"): void { encapsulated }'

$cases = [ordered]@{

    # 最核心的用例：同名方法两份定义，按实参类型各走各的
    'overload_int_vs_str' = @{
        Src = @"
$soutStub
class A {
    fun m(x: int): int {
        ret x * 2
    }
    fun m(x: str): int {
        ret core_str_len(x)
    }
}
var o = A()
sout(o.m(21))
sout(o.m("hello"))
"@
        Expect = "42`n5"
    }

    # 三个重载
    'overload_three' = @{
        Src = @"
$soutStub
class A {
    fun m(x: int): int { ret 1 }
    fun m(x: str): int { ret 2 }
    fun m(x: list): int { ret 3 }
}
var o = A()
sout(o.m(0))
sout(o.m("s"))
sout(o.m([9]))
"@
        Expect = "1`n2`n3"
    }

    # 单定义的方法必须保持可用（回归保护：不能为了重载破坏普通方法）
    'single_method_still_works' = @{
        Src = @"
$soutStub
class A {
    fun only(x: int): int {
        ret x + 1
    }
}
var o = A()
sout(o.only(9))
"@
        Expect = '10'
    }

    # 同一个类里既有重载方法又有普通方法
    'mixed_overload_and_plain' = @{
        Src = @"
$soutStub
class A {
    fun m(x: int): int { ret x + 100 }
    fun m(x: str): int { ret core_str_len(x) + 200 }
    fun plain(x: int): int { ret x * 3 }
}
var o = A()
sout(o.m(1))
sout(o.m("ab"))
sout(o.plain(4))
"@
        Expect = "101`n202`n12"
    }

    # 两个类各有同名方法，符号不能互相撞
    'two_classes_same_method_name' = @{
        Src = @"
$soutStub
class A {
    fun m(x: int): int { ret 10 }
    fun m(x: str): int { ret 11 }
}
class B {
    fun m(x: int): int { ret 20 }
    fun m(x: str): int { ret 21 }
}
var a = A()
var b = B()
sout(a.m(0))
sout(a.m("s"))
sout(b.m(0))
sout(b.m("s"))
"@
        Expect = "10`n11`n20`n21"
    }

    # 重载方法内部可以调用另一个重载
    'overload_calls_overload' = @{
        Src = @"
$soutStub
class A {
    fun m(x: int): int { ret x * 2 }
    fun m(x: str): int { ret this.m(core_str_len(x)) }
}
var o = A()
sout(o.m("abc"))
"@
        Expect = '6'
    }

    # 重载方法访问字段
    'overload_uses_field' = @{
        Src = @"
$soutStub
class A {
    var base: int = 5
    fun m(x: int): int { ret this.base + x }
    fun m(x: str): int { ret this.base + core_str_len(x) }
}
var o = A()
sout(o.m(1))
sout(o.m("abc"))
"@
        Expect = "6`n8"
    }

    # 浮点重载与整数重载并存（数值族内部按同族优先）
    'overload_int_vs_float' = @{
        Src = @"
$soutStub
class A {
    fun m(x: int): int { ret 1 }
    fun m(x: float): int { ret 2 }
}
var o = A()
sout(o.m(3))
var f: float = 1.5
sout(o.m(f))
"@
        Expect = "1`n2"
    }

    # 派生类遮蔽基类同名方法（单定义 → 单定义）
    'derived_shadows_base' = @{
        Src = @"
$soutStub
class Base {
    fun m(x: int): int { ret 1 }
}
class Derived: Base {
    fun m(x: int): int { ret 2 }
}
var d = Derived()
sout(d.m(0))
"@
        Expect = '2'
    }

    # 派生类重载集遮蔽基类重载集
    'derived_overload_shadows_base' = @{
        Src = @"
$soutStub
class Base {
    fun m(x: int): int { ret 1 }
    fun m(x: str): int { ret 2 }
}
class Derived: Base {
    fun m(x: int): int { ret 10 }
    fun m(x: str): int { ret 20 }
}
var d = Derived()
sout(d.m(0))
sout(d.m("s"))
"@
        Expect = "10`n20"
    }

    # 派生类没声明的方法从基类继承（回归保护）
    'derived_inherits_method' = @{
        Src = @"
$soutStub
class Base {
    fun hello(x: int): int { ret x + 7 }
}
class Derived: Base {
    fun other(x: int): int { ret x }
}
var d = Derived()
sout(d.hello(3))
"@
        Expect = '10'
    }
}

$pass = 0
$fail = 0
$failedNames = @()

foreach ($name in $cases.Keys) {
    $case = $cases[$name]
    $rioPath = Join-Path $work "$name.rio"
    [System.IO.File]::WriteAllText($rioPath, $case.Src, $utf8NoBom)

    $prev = $ErrorActionPreference
    $ErrorActionPreference = 'Continue'
    try {
        $buildOut = & $rcc -build --path $rioPath --output (Join-Path $work $name) 2>&1 | Out-String
    } finally {
        $ErrorActionPreference = $prev
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

Write-Host "P12-2 / G5 方法重载验收通过" -ForegroundColor Green
exit 0

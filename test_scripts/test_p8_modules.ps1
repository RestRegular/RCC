# ============================================================================
#  RCC 改造 阶段 P8 —— 模块 / import 验收
#
#  覆盖：单模块内联、多模块、嵌套 import（依赖先于使用方）、菱形依赖去重、
#        模块内类与方法、import(...) 别名形式、限定调用 mod.fn(...)、
#        模块内互相调用、跨模块全局函数直接调用、
#        找不到模块报错、循环 import 报错、语法错误定位
#
#  说明：本阶段不使用 tests/test_14_iter、tests/test_15_forinrange 作为门槛，
#        它们依赖旧 LLVM 时代的 release/Lib/builtin/ds.rio（encapsulated +
#        RA 指令），与新后端无关。P8 的模块用例全部是脚本内现写的小 .rio 文件。
#
#  用法：powershell -NoProfile -ExecutionPolicy Bypass -File test_scripts/test_p8_modules.ps1
# ============================================================================

$ErrorActionPreference = 'Stop'

$repo = Split-Path -Parent $PSScriptRoot
$rcc = Join-Path $repo 'release\RCC.exe'
$work = Join-Path $repo 'build\p8_modules_verify'

if (-not (Test-Path $rcc)) {
    Write-Host "找不到编译器：$rcc" -ForegroundColor Red
    Write-Host "请先构建：ninja -C build"
    exit 1
}

if (Test-Path $work) { Remove-Item $work -Recurse -Force }
New-Item -ItemType Directory -Force -Path $work | Out-Null

$utf8NoBom = New-Object System.Text.UTF8Encoding($false)

$soutStub = 'fun sout(*args, end="\n"): void { encapsulated }'

# ---------------------------------------------------------------------------
#  辅助函数
# ---------------------------------------------------------------------------
function Write-Source([string]$path, [string]$src) {
    $dir = Split-Path -Parent $path
    if (-not (Test-Path $dir)) { New-Item -ItemType Directory -Force -Path $dir | Out-Null }
    [System.IO.File]::WriteAllText($path, $src, $utf8NoBom)
}

# 编译 + 运行，返回 @{ Ok = $true/$false; Output = 输出或错误信息 }
function Invoke-Case([string]$name, [string]$mainPath) {
    $out = Join-Path $work "$name.exe"
    # 负向用例要求编译失败，而失败时 RCC 会往 stderr 写错误信息。
    # 脚本顶部是 ErrorActionPreference=Stop，原生命令的 stderr 会被当成
    # 终止性错误抛出，所以这里临时放开，自己看退出码判断。
    $prev = $ErrorActionPreference
    $ErrorActionPreference = 'Continue'
    try {
        $log = & $rcc -build --path $mainPath --output $out 2>&1 | Out-String
        $code = $LASTEXITCODE
    } finally {
        $ErrorActionPreference = $prev
    }
    if ($code -ne 0 -or -not (Test-Path $out)) {
        return @{ Ok = $false; Output = $log.Trim() }
    }
    $run = & $out 2>&1 | Out-String
    return @{ Ok = $true; Output = ($run.TrimEnd() -replace "`r`n", "`n") }
}

# ---------------------------------------------------------------------------
#  用例表
# ---------------------------------------------------------------------------
$cases = [ordered]@{}

# ---- 1. 单模块内联 ---------------------------------------------------------
$cases['single_module'] = @{
    Files = @{
        'mathx.rio' = @"
$soutStub
fun add(a: int, b: int): int { ret a + b }
fun mul(a: int, b: int): int { ret a * b }
"@
        'main.rio' = @"
import mathx
fun main() {
    sout(add(1, 2))
    sout(mul(6, 7))
}
main()
"@
    }
    Main = 'main.rio'
    Expect = "3`n42"
}

# ---- 2. 限定调用 mod.fn(...) ----------------------------------------------
$cases['qualified_call'] = @{
    Files = @{
        'mathx.rio' = @"
$soutStub
fun add(a: int, b: int): int { ret a + b }
fun mul(a: int, b: int): int { ret a * b }
"@
        'main.rio' = @"
import mathx
fun main() {
    sout(mathx.add(10, 20))
    sout(mathx.mul(3, 4))
}
main()
"@
    }
    Main = 'main.rio'
    Expect = "30`n12"
}

# ---- 3. 嵌套 import：algeb 依赖 mathx，main 依赖两者 ------------------------
$cases['nested_import'] = @{
    Files = @{
        'mathx.rio' = @"
$soutStub
fun add(a: int, b: int): int { ret a + b }
fun mul(a: int, b: int): int { ret a * b }
"@
        'algeb.rio' = @"
$soutStub
import mathx
fun square(a: int): int { ret mathx.mul(a, a) }
fun cube(a: int): int { ret mathx.mul(mathx.mul(a, a), a) }
"@
        'main.rio' = @"
import mathx
import algeb
fun main() {
    sout(mathx.add(1, 2))
    sout(algeb.square(5))
    sout(algeb.cube(3))
    sout(mul(6, 7))
}
main()
"@
    }
    Main = 'main.rio'
    # 25 = square(5)，27 = cube(3)，42 = mul(6,7) 直接调用
    Expect = "3`n25`n27`n42"
}

# ---- 4. 菱形依赖去重：main -> a, b；a -> base；b -> base -------------------
$cases['diamond_dedupe'] = @{
    Files = @{
        'base.rio' = @"
$soutStub
fun twice(x: int): int { ret x * 2 }
"@
        'moda.rio' = @"
$soutStub
import base
fun quad(x: int): int { ret twice(twice(x)) }
"@
        'modb.rio' = @"
$soutStub
import base
fun oct(x: int): int { ret twice(quad(x)) }
"@
        'main.rio' = @"
import moda
import modb
import base
fun main() {
    sout(quad(1))
    sout(oct(1))
    sout(twice(21))
}
main()
"@
    }
    Main = 'main.rio'
    # base 只被内联一次，重复内联会撞上「符号重复定义」或调用错乱
    Expect = "4`n8`n42"
}

# ---- 5. import(...) 别名形式 ----------------------------------------------
$cases['alias_import'] = @{
    Files = @{
        'mathx.rio' = @"
$soutStub
fun add(a: int, b: int): int { ret a + b }
"@
        'utilx.rio' = @"
$soutStub
fun twice(a: int): int { ret a * 2 }
"@
        'main.rio' = @"
import(u="utilx", m="mathx")
fun main() {
    sout(u.twice(21))
    sout(m.add(2, 3))
}
main()
"@
    }
    Main = 'main.rio'
    Expect = "42`n5"
}

# ---- 6. 模块内定义类，跨模块构造与方法调用 --------------------------------
$cases['module_class'] = @{
    Files = @{
        'shapes.rio' = @"
$soutStub
class Point {
    var x: int, y: int
    ctor(a: int, b: int) {
        this.x = a
        this.y = b
    }
    fun sum(): int { ret this.x + this.y }
}
"@
        'main.rio' = @"
import shapes
fun main() {
    var p = Point(3, 4)
    sout(p.sum())
    var q = shapes.Point(10, 20)
    sout(q.sum())
}
main()
"@
    }
    Main = 'main.rio'
    Expect = "7`n30"
}

# ---- 7. 模块导出 export(...) 不参与代码生成 --------------------------------
$cases['export_ignored'] = @{
    Files = @{
        'utilx.rio' = @"
$soutStub
fun twice(a: int): int { ret a * 2 }
class Box {
    var v: int = 0
    fun get(): int { ret this.v }
}
export(twice, Box)
"@
        'main.rio' = @"
import utilx
fun main() {
    sout(twice(5))
    var b = Box()
    sout(b.get())
}
main()
"@
    }
    Main = 'main.rio'
    # export(...) 若被当普通调用发射，会变成对未定义符号 export 的调用
    Expect = "10`n0"
}

# ---- 8. 模块里的函数互相调用（内联后同处一个翻译单元） --------------------
$cases['intra_module_calls'] = @{
    Files = @{
        'chain.rio' = @"
$soutStub
fun step1(x: int): int { ret x + 1 }
fun step2(x: int): int { ret step1(x) * 2 }
fun step3(x: int): int { ret step2(x) + 3 }
"@
        'main.rio' = @"
import chain
fun main() { sout(step3(4)) }
main()
"@
    }
    Main = 'main.rio'
    # (4+1)*2+3 = 13
    Expect = "13"
}

# ---- 9. 主文件先定义、模块后使用（顺序无关） ------------------------------
$cases['importer_before_module'] = @{
    Files = @{
        'late.rio' = @"
$soutStub
fun helper(): int { ret 99 }
"@
        'main.rio' = @"
fun main() { sout(helper()) }
import late
main()
"@
    }
    Main = 'main.rio'
    Expect = "99"
}

# ---- 10. 缺省实参与命名实参（P8 一并补齐的调用点能力） --------------------
$cases['default_and_named_args'] = @{
    Files = @{
        'main.rio' = @"
$soutStub
fun g(a: int, b: int = 2): int { ret a + b }
fun main() {
    sout(g(1))
    sout(g(1, 10))
    sout(g(b=5, a=1))
}
main()
"@
    }
    Main = 'main.rio'
    Expect = "3`n11`n6"
}

# ---------------------------------------------------------------------------
#  负向用例：必须编译失败，且 stderr 里出现指定片段
# ---------------------------------------------------------------------------
$negative = [ordered]@{}

$negative['missing_module'] = @{
    Files = @{
        'main.rio' = @"
$soutStub
import nosuchmodule
fun main() { sout(1) }
main()
"@
    }
    Main = 'main.rio'
    Expect = "Cannot find imported module 'nosuchmodule'"
}

$negative['cyclic_import'] = @{
    Files = @{
        'cyca.rio' = @"
$soutStub
import cycb
fun ca(): int { ret 1 }
"@
        'cycb.rio' = @"
$soutStub
import cyca
fun cb(): int { ret 2 }
"@
        'main.rio' = @"
$soutStub
import cyca
fun main() { sout(ca()) }
main()
"@
    }
    Main = 'main.rio'
    Expect = "Recursive import detected"
}

$negative['module_syntax_error'] = @{
    Files = @{
        'broken.rio' = @"
$soutStub
fun bad(a: int: int { ret a }
"@
        'main.rio' = @"
$soutStub
import broken
fun main() { sout(1) }
main()
"@
    }
    Main = 'main.rio'
    Expect = 'broken.rio'
}

# ---------------------------------------------------------------------------
#  执行
# ---------------------------------------------------------------------------
$pass = 0
$fail = 0
$failedNames = @()

Write-Host ""
Write-Host "== P8 正向用例 ==" -ForegroundColor Cyan

foreach ($name in $cases.Keys) {
    $case = $cases[$name]
    $dir = Join-Path $work $name
    New-Item -ItemType Directory -Force -Path $dir | Out-Null
    foreach ($f in $case.Files.Keys) {
        Write-Source (Join-Path $dir $f) $case.Files[$f]
    }
    $mainPath = Join-Path $dir $case.Main
    $result = Invoke-Case $name $mainPath
    if (-not $result.Ok) {
        Write-Host ("  [失败] {0} —— 编译失败" -f $name) -ForegroundColor Red
        Write-Host ("         {0}" -f (($result.Output -split "`n" | Select-Object -First 3) -join ' / ')) -ForegroundColor DarkGray
        $fail++
        $failedNames += $name
        continue
    }
    $expect = $case.Expect
    if ($result.Output -eq $expect) {
        Write-Host ("  [通过] {0}" -f $name) -ForegroundColor Green
        $pass++
    } else {
        Write-Host ("  [失败] {0} —— 输出不符" -f $name) -ForegroundColor Red
        Write-Host ("         期望：{0}" -f ($expect -replace "`n", ' / ')) -ForegroundColor DarkGray
        Write-Host ("         实际：{0}" -f ($result.Output -replace "`n", ' / ')) -ForegroundColor DarkGray
        $fail++
        $failedNames += $name
    }
}

Write-Host ""
Write-Host "== P8 负向用例（必须编译失败）==" -ForegroundColor Cyan

foreach ($name in $negative.Keys) {
    $case = $negative[$name]
    $dir = Join-Path $work $name
    New-Item -ItemType Directory -Force -Path $dir | Out-Null
    foreach ($f in $case.Files.Keys) {
        Write-Source (Join-Path $dir $f) $case.Files[$f]
    }
    $mainPath = Join-Path $dir $case.Main
    $result = Invoke-Case $name $mainPath
    if ($result.Ok) {
        Write-Host ("  [失败] {0} —— 本应编译失败却成功了" -f $name) -ForegroundColor Red
        $fail++
        $failedNames += $name
        continue
    }
    if ($result.Output -like "*$($case.Expect)*") {
        Write-Host ("  [通过] {0}" -f $name) -ForegroundColor Green
        $pass++
    } else {
        Write-Host ("  [失败] {0} —— 错误信息不含期望片段" -f $name) -ForegroundColor Red
        Write-Host ("         期望含：{0}" -f $case.Expect) -ForegroundColor DarkGray
        Write-Host ("         实际：{0}" -f (($result.Output -split "`n" | Select-Object -First 3) -join ' / ')) -ForegroundColor DarkGray
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
    Write-Host "P8 模块验收通过" -ForegroundColor Green
    Remove-Item $work -Recurse -Force -ErrorAction SilentlyContinue
    exit 0
} else {
    Write-Host ("失败用例：{0}" -f ($failedNames -join ', ')) -ForegroundColor Red
    Write-Host ("临时目录保留在：{0}" -f $work) -ForegroundColor Yellow
    exit 1
}

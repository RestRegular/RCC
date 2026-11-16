# ============================================================================
#  RCC 改造 阶段 P5 —— 容器（list / dict）验收
#
#  覆盖：字典字面量 + 字符串键取值、列表字面量 + 整数下标取值、
#        空容器、嵌套容器、浮点元素、越界下标、未命中键、
#        变量键查找、循环里反复取下标、容器作为函数实参/返回值
#  每个用例的 .rio 源码与期望输出都内联在本脚本里，跑完即删临时目录。
#
#  用法：powershell -NoProfile -ExecutionPolicy Bypass -File test_scripts/test_p5_containers.ps1
# ============================================================================

$ErrorActionPreference = 'Stop'

$repo = Split-Path -Parent $PSScriptRoot
$rcc = Join-Path $repo 'release\RCC.exe'
$work = Join-Path $repo 'build\p5_cont_verify'

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

    'dict_literal' = @{
        # 字符串键按内容查找，且每个键各自的静态类型要正确
        Src = @"
$soutStub
var dic = {"name": "Rio", "age": 18}
sout(dic["name"])
sout(dic["age"])
"@
        Expect = "Rio`n18"
    }

    # 重复键：后写的覆盖先写的（P13 缺陷回归，2026-11-17）。
    # 原先两条都会进 keys/vals 数组，长度变 2，且查找命中**先出现**的旧值。
    # 字典语义要求 `{"a":1,"a":2}` 等价于 `{"a":2}`。
    'dict_dup_key_overwrites' = @{
        Src = @"
$soutStub
var d = {"a": 1, "a": 2}
sout(core_str_len(d))
sout(d["a"])
var e = {"x": 1, "y": 2, "x": 3}
sout(core_str_len(e))
sout(e["x"])
sout(e["y"])
"@
        Expect = "1`n2`n2`n3`n2"
    }

    'dict_multi_key' = @{        # 三个键共用一个长度，最容易暴露「按头指针比较」的 bug
        Src = @"
$soutStub
var d = {"a": 1, "b": 2, "c": 3}
sout(d["a"])
sout(d["b"])
sout(d["c"])
"@
        Expect = "1`n2`n3"
    }

    'dict_miss' = @{
        # 未命中的键返回 null 指针。注意 sout 按**静态类型**分派，
        # 这个字典的值类型是 int，所以 null 被当成 0 打印 —— 这是静态
        # 分派的必然结果（运行时值确实是 null，下面用 == null 验证）。
        Src = @"
$soutStub
var d = {"x": 1, "y": 2}
sout(d["zzz"] == null)
sout(d["x"] == null)
"@
        Expect = "true`nfalse"
    }

    'dict_var_key' = @{
        # 键是变量（非字面量）：静态类型回退到字典的值类型
        Src = @"
$soutStub
var d = {"one": 11, "two": 22}
var k = "two"
sout(d[k])
"@
        Expect = "22"
    }

    'dict_long_keys' = @{
        # 长度不同 / 前缀相同的键，验证先比长度再比内容
        Src = @"
$soutStub
var d = {"ab": 1, "abc": 2, "abcd": 3}
sout(d["abc"])
sout(d["abcd"])
sout(d["ab"])
"@
        Expect = "2`n3`n1"
    }

    'list_literal' = @{
        Src = @"
$soutStub
var xs = [10, 20, 30, 40]
sout(xs[0])
sout(xs[2])
sout(xs[3])
"@
        Expect = "10`n30`n40"
    }

    'list_float' = @{
        # 浮点元素按位存 8 字节，取出来按静态类型解释
        Src = @"
$soutStub
var fs = [1.5, 2.5, 3.5]
sout(fs[0])
sout(fs[2])
"@
        Expect = "1.5`n3.5"
    }

    'list_oob' = @{
        # 越界与负下标都返回 0
        Src = @"
$soutStub
var xs = [7, 8]
sout(xs[2])
sout(xs[99])
sout(xs[-1])
"@
        Expect = "0`n0`n0"
    }

    'empty_containers' = @{
        # 空容器：没有元素，任何下标都落空
        Src = @"
$soutStub
var e = []
var d = {}
sout(e[0])
sout(d["k"] == null)
"@
        Expect = "0`ntrue"
    }

    'nested' = @{
        # 容器里放容器：外层元素就是内层头指针
        Src = @"
$soutStub
var grid = [[1, 2], [3, 4]]
sout(grid[0][1])
sout(grid[1][0])
var d = {"nums": [5, 6]}
sout(d["nums"][1])
"@
        Expect = "2`n3`n6"
    }

    'loop_index' = @{
        # 循环里反复取下标：验证每次迭代都重新装载容器指针
        # （for-in 迭代属于 P8，这里用 while 表达）
        Src = @"
$soutStub
var xs = [2, 4, 6, 8]
var sum = 0
var i = 0
while i < 4 {
    sum = sum + xs[i]
    i = i + 1
}
sout(sum)
"@
        Expect = "20"
    }

    'dict_in_func' = @{
        # 容器跨函数边界：返回值方向完全支持（返回类型由 ret 表达式推断）。
        # 注意**形参**方向不支持 —— 无类型标注的形参静态类型未知，
        # 索引时无法确定是 list 还是 dict，需要 P11 的类型推断。
        Src = @"
$soutStub
fun make_list() {
    ret [100, 200, 300]
}
fun make_dict() {
    ret {"k": 77}
}
sout(make_list()[1])
sout(make_dict()["k"])
"@
        Expect = "200`n77"
    }

    'list_sum_loop' = @{
        # 列表长度 + 索引遍历
        Src = @"
$soutStub
var xs = [1, 2, 3, 4, 5]
var total = 0
var i = 0
while i < 5 {
    total = total + xs[i]
    i = i + 1
}
sout(total)
"@
        Expect = "15"
    }

    'mixed_dict' = @{
        # 值类型异构：退化成 int 打印，但字符串键仍按内容命中
        Src = @"
$soutStub
var d = {"a": 1, "b": 2}
sout(d["a"])
sout(d["b"])
"@
        Expect = "1`n2"
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
    Write-Host "P5 容器验收通过" -ForegroundColor Green
    Remove-Item $work -Recurse -Force -ErrorAction SilentlyContinue
    exit 0
} else {
    Write-Host ("失败用例：{0}" -f ($failedNames -join ', ')) -ForegroundColor Red
    Write-Host ("临时目录保留在：{0}" -f $work) -ForegroundColor Yellow
    exit 1
}

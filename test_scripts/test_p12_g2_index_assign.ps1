# ============================================================================
#  P12-2 / G2 —— 索引赋值 `a[i] = v` / `d[k] = v`
#
#  ⚠️ 这是**恢复功能回归**，不是新语言特性。原仓库 v1.21.2 实测支持：
#       li[0] = 0    ✅
#       li[1] += 1   ✅
#       li += [4]    ✅（归 G4）
#  新架构（P0–P11 重写）把它丢了，报 `Only simple variable assignment is supported`。
#
#  根因：原实现靠 RA VM 的 ITER_GET 返回**指向容器元素的 shared_ptr** ——
#  改临时变量就等于改容器。新架构元素是数据区内联的裸值，那条隐含通路没了，
#  必须显式计算元素地址再写。
#
#  定案（docs/p12_plan.md §G2.4）：
#    Q1 越界     静态可算报错 / 动态越界跳过（与 list 读返回 0 同口径）
#    Q2 s[i] = c ❌ 编译期报错（原仓库是静默失效 bug，见 G10）
#    Q3 d[k]=v   键不存在则新增
#    Q4 元素类型必须匹配（走 checkAssignable）
#    Q5 a[i] += v ✅ 支持
#
#  用法：powershell -NoProfile -ExecutionPolicy Bypass -File test_scripts/test_p12_g2_index_assign.ps1
# ============================================================================

$ErrorActionPreference = 'Stop'

$repo = Split-Path -Parent $PSScriptRoot
$rcc  = Join-Path $repo 'release\RCC.exe'
$work = Join-Path $repo 'build\p12_g2_verify'

if (-not (Test-Path $rcc)) {
    Write-Host "找不到编译器：$rcc" -ForegroundColor Red
    exit 1
}

if (Test-Path $work) { Remove-Item $work -Recurse -Force }
New-Item -ItemType Directory -Force -Path $work | Out-Null

$utf8NoBom = New-Object System.Text.UTF8Encoding($false)
$soutStub = 'fun sout(*args, end="\n"): void { encapsulated }'

$cases = [ordered]@{

    # 原仓库实测的第一个用例
    'list_set_first' = @{
        Src = @"
$soutStub
var li = [1, 2, 3]
li[0] = 0
sout(li[0])
sout(li[1])
sout(li[2])
"@
        Expect = "0`n2`n3"
    }

    # 改中间元素不能波及两侧（元素地址算错的典型症状）
    'list_set_middle' = @{
        Src = @"
$soutStub
var li = [1, 2, 3]
li[1] = 99
sout(li[0])
sout(li[1])
sout(li[2])
"@
        Expect = "1`n99`n3"
    }

    # 原仓库实测的复合赋值
    'list_compound_plus' = @{
        Src = @"
$soutStub
var li = [1, 2, 3]
li[1] += 10
sout(li[0])
sout(li[1])
sout(li[2])
"@
        Expect = "1`n12`n3"
    }

    'list_compound_minus' = @{
        Src = @"
$soutStub
var li = [10, 20, 30]
li[2] -= 5
sout(li[2])
"@
        Expect = '25'
    }

    'list_compound_star' = @{
        Src = @"
$soutStub
var li = [2, 3]
li[0] *= 7
sout(li[0])
"@
        Expect = '14'
    }

    # 下标是变量 / 表达式
    'list_set_variable_index' = @{
        Src = @"
$soutStub
var li = [0, 0, 0]
var i: int = 2
li[i] = 7
li[i - 1] = 5
sout(li[0])
sout(li[1])
sout(li[2])
"@
        Expect = "0`n5`n7"
    }

    # 循环里反复写：最容易暴露临时槽被复用的问题
    'list_set_in_loop' = @{
        Src = @"
$soutStub
var li = [0, 0, 0, 0]
var i: int = 0
while i < 4 {
    li[i] = i * i
    i += 1
}
sout(li[0])
sout(li[1])
sout(li[2])
sout(li[3])
"@
        Expect = "0`n1`n4`n9"
    }

    # 越界写必须**跳过**，不能写坏内存，也不能影响别的元素
    'list_set_out_of_range' = @{
        Src = @"
$soutStub
var li = [1, 2]
li[99] = 123
sout(li[0])
sout(li[1])
"@
        Expect = "1`n2"
    }

    # 负下标按无符号比较被拦下（与 list 读一致）
    'list_set_negative_index' = @{
        Src = @"
$soutStub
var li = [1, 2]
li[0 - 1] = 123
sout(li[0])
sout(li[1])
"@
        Expect = "1`n2"
    }

    # Q3：dict 覆盖已有键
    'dict_overwrite' = @{
        Src = @"
$soutStub
var d = {"a": 1}
d["a"] = 9
sout(d["a"])
"@
        Expect = '9'
    }

    # Q3：dict 新增键，且不能破坏已有键
    'dict_insert_new' = @{
        Src = @"
$soutStub
var d = {"a": 1}
d["b"] = 2
sout(d["a"])
sout(d["b"])
"@
        Expect = "1`n2"
    }

    # 连续插入多个键（扩容路径连跑多次）
    'dict_insert_many' = @{
        Src = @"
$soutStub
var d = {"a": 1}
d["b"] = 2
d["c"] = 3
d["d"] = 4
sout(d["a"])
sout(d["b"])
sout(d["c"])
sout(d["d"])
"@
        Expect = "1`n2`n3`n4"
    }

    # 往空字典里插第一个键
    'dict_insert_into_empty' = @{
        Src = @"
$soutStub
var d = {}
d["x"] = 42
sout(d["x"])
"@
        Expect = '42'
    }

    # 先插入再覆盖同一个键（走两条不同路径）
    'dict_insert_then_overwrite' = @{
        Src = @"
$soutStub
var d = {"a": 1}
d["z"] = 5
d["z"] = 6
sout(d["a"])
sout(d["z"])
"@
        Expect = "1`n6"
    }

    # 整数键的字典
    'dict_int_key' = @{
        Src = @"
$soutStub
var d = {1: 10}
d[2] = 20
d[1] = 11
sout(d[1])
sout(d[2])
"@
        Expect = "11`n20"
    }

    # 写进去的值参与后续运算（确认写的是真值不是地址）
    'written_value_is_usable' = @{
        Src = @"
$soutStub
var li = [0, 0]
li[0] = 6
li[1] = 7
sout(li[0] * li[1])
"@
        Expect = '42'
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

    if ($buildOut -match '(?m)^\s*Error|ParserError|CodeGenError|LinkError') {
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

# ---- 反向用例：必须编译失败 ----
$negatives = [ordered]@{
    # Q2：str 元素写必须编译期报错（原仓库是静默失效 bug，不得复制）
    'str_index_assign_rejected' = @{
        Src = @"
$soutStub
var s: str = "hello"
s[1] = "X"
sout(s)
"@
        MustMatch = 'Assignment to list element|Index assignment'
    }
}

foreach ($name in $negatives.Keys) {
    $case = $negatives[$name]
    $rioPath = Join-Path $work "$name.rio"
    [System.IO.File]::WriteAllText($rioPath, $case.Src, $utf8NoBom)
    # 预期失败的命令会往 stderr 写诊断；ErrorActionPreference=Stop 会把它
    # 当终止错误抛出，这里临时降级。
    $prev = $ErrorActionPreference
    $ErrorActionPreference = 'Continue'
    try {
        $buildOut = & $rcc -build --path $rioPath --output (Join-Path $work $name) 2>&1 | Out-String
    } finally {
        $ErrorActionPreference = $prev
    }
    $failedAsExpected = ($buildOut -match 'Error|CodeGenError|ParserError')
    if ($failedAsExpected) {
        # Q2 定案要求 str 元素写是**编译期**错误，而不是运行期静默失效
        if ($buildOut -match 'Index assignment on a non-container value') {
            Write-Host ("  [通过] {0}" -f $name) -ForegroundColor Green
            $pass++
        } else {
            Write-Host ("  [失败] {0} —— 报错了但诊断不符" -f $name) -ForegroundColor Red
            $fail++; $failedNames += $name
        }
    } else {
        Write-Host ("  [失败] {0} —— 本应编译失败却通过了" -f $name) -ForegroundColor Red
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

Write-Host "P12-2 / G2 索引赋值验收通过" -ForegroundColor Green
exit 0

# ============================================================================
#  P12-2 / G12 + G13 —— `ptr` ↔ `int` 互通 与 `any` 显式标注
#
#  ---- G12：`ptr` 返回值标注 ----
#  现象（探针 oo2 / qq3 / rr2）：
#      fun cp(arg: list): ptr {
#        var p = core_alloc(32, 8)     // core_* 无类型签名，推断为 int
#        ret p                          // cannot assign int to ptr
#      }
#  根因：core_* 的 extern fun 不参与新后端的类型表，`core_alloc` 推断返回 int；
#        而 ptr 是**新架构自己引入**的类型（原仓库 builtInTypes 里没有 ptr），
#        sameValueFamily 没把 ptr 与数值族归为一类。
#  修法（方案 1）：sameValueFamily 放行 ptr ↔ 数值族。
#        ptr 本质就是 64 位整数，与 P9「数值族互通、不设转换墙」的原则一致。
#        代价是失去「把整数误当指针」的静态检查 —— 新架构本来就没有 ptr 的
#        强类型传统，可接受。
#
#  ---- G13：`any` 显式标注 ----
#  现象：`fun f(a: any): int` → `Unknown type 'any'`（省略标注则完全正常）。
#  原仓库依据：builtInTypes 里有 any（rcc_symbol.cpp:650），
#        ds.rio:32 `fun copy(arg): any`、math.rio:26 `fun pow(a, times: int): any`。
#  修法：parseTypeName / resolveTypeName / 形参登记 / 返回标注校验四处
#        都认 `any`，语义映射成 unknown（= 「不标注」）。
#
#  ⚠️ `any` 作为**返回**标注时，真实返回类型仍由函数体推断
#     （inferAnonReturnType）。`fun copy(arg: any): any { ret arg }` 对
#     `copy("hi")` 会推断成 int —— 这是返回类型推断的固有限制，不是 `any`
#     标注本身的问题。数值场景（math.rio 的 pow）不受影响。
#
#  用法：powershell -NoProfile -ExecutionPolicy Bypass -File test_scripts/test_p12_g12g13_ptr_any.ps1
# ============================================================================

$ErrorActionPreference = 'Stop'

$repo = Split-Path -Parent $PSScriptRoot
$rcc  = Join-Path $repo 'release\RCC.exe'
$work = Join-Path $repo 'build\p12_g12g13_verify'

if (-not (Test-Path $rcc)) {
    Write-Host "找不到编译器：$rcc" -ForegroundColor Red
    exit 1
}

if (Test-Path $work) { Remove-Item $work -Recurse -Force }
New-Item -ItemType Directory -Force -Path $work | Out-Null

$utf8NoBom = New-Object System.Text.UTF8Encoding($false)
$soutStub = 'fun sout(*args, end="\n"): void { encapsulated }'

$cases = [ordered]@{

    # ---- G12 ----

    # 探针 oo2 的原始症状：core_alloc 的结果直接 ret 出 ptr 函数
    'ptr_return_from_core_alloc' = @{
        Src = @"
$soutStub
fun cp(): ptr {
    var p = core_alloc(32, 8)
    ret p
}
var buf = cp()
sout(buf != null)
"@
        Expect = 'true'
    }

    # ptr 形参接收，再当 int 用
    'ptr_param_as_int' = @{
        Src = @"
$soutStub
fun asInt(p: ptr): int {
    ret p
}
var q: ptr = core_alloc(16, 8)
sout(asInt(q) > 0)
"@
        Expect = 'true'
    }

    # int 形参接收 ptr 值
    'ptr_to_int_param' = @{
        Src = @"
$soutStub
fun fromInt(v: int): ptr {
    ret v
}
var q: ptr = core_alloc(16, 8)
var back: ptr = fromInt(q)
sout(back == q)
"@
        Expect = 'true'
    }

    # ptr 与 int 混合算术（裸指针算术是 core/rt 的日常写法）
    'ptr_arithmetic' = @{
        Src = @"
$soutStub
var q: ptr = core_alloc(64, 8)
core_mem_set(q, 0, 64)
var base: int = q
var plus8: int = q + 8
sout(plus8 - base)
"@
        Expect = '8'
    }

    # var 声明成 ptr，赋 core_alloc 的返回值
    'var_declared_ptr' = @{
        Src = @"
$soutStub
var q: ptr = core_alloc(16, 8)
core_mem_set(q, 0, 16)
sout(core_str_len(q))
"@
        Expect = '0'
    }

    # 写内存后按 ptr 读回（core_str_len 读第一个 8 字节）
    'ptr_write_then_read' = @{
        Src = @"
$soutStub
var q: ptr = core_alloc(16, 8)
core_mem_set(q, 0, 16)
core_mem_set(q, 5, 1)
sout(core_str_len(q))
"@
        Expect = '5'
    }

    # ptr 函数返回 null
    'ptr_return_null' = @{
        Src = @"
$soutStub
fun nothing(): ptr {
    ret null
}
var p = nothing()
sout(p == null)
"@
        Expect = 'true'
    }

    # ---- G13 ----

    # 形参 any：接收 int
    'any_param_int' = @{
        Src = @"
$soutStub
fun identity(a: any): int {
    ret a
}
sout(identity(7))
"@
        Expect = '7'
    }

    # 形参 any：接收 str（any 必须什么都收）
    'any_param_str' = @{
        Src = @"
$soutStub
fun firstByte(a: any): int {
    ret core_str_len(a)
}
sout(firstByte("hello"))
"@
        Expect = '5'
    }

    # 形参 any：接收 list
    'any_param_list' = @{
        Src = @"
$soutStub
fun len2(a: any): int {
    ret core_str_len(a)
}
var l = [1, 2, 3]
sout(len2(l))
"@
        Expect = '3'
    }

    # 返回标注 any（数值场景，原仓库 math.rio:26 的形态）
    'any_return_numeric' = @{
        Src = @"
$soutStub
fun pow2(a: int, times: int): any {
    var r: int = 1
    var i: int = 0
    while i < times {
        r = r * a
        i += 1
    }
    ret r
}
sout(pow2(3, 4))
"@
        Expect = '81'
    }

    # 形参与返回都是 any（原仓库 ds.rio:32 的形态）
    'any_param_and_return' = @{
        Src = @"
$soutStub
fun copy(arg: any): any {
    ret arg
}
sout(copy(42))
"@
        Expect = '42'
    }

    # var 声明成 any 等价于不标注
    'var_annotated_any' = @{
        Src = @"
$soutStub
var x: any = 5
sout(x)
var y: any = "text"
sout(y)
"@
        Expect = "5`ntext"
    }

    # 无标注写法必须仍然有效（回归保护：不能为了加 any 破坏默认行为）
    'unannotated_still_works' = @{
        Src = @"
$soutStub
fun ident(a): int {
    ret a
}
sout(ident(9))
"@
        Expect = '9'
    }

    # 拼错的类型名必须仍然报错（不能因为加了 any 就放行一切）
    'typo_still_rejected' = @{
        Src = @"
$soutStub
fun f(a: piont): int {
    ret a
}
sout(f(1))
"@
        MustMatch = "Unknown type"
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

Write-Host "P12-2 / G12 + G13 验收通过" -ForegroundColor Green
exit 0

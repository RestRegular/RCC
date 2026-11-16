# test_p12_3c_ds.ps1 —— ds 模块三层验收（compile / api / behavior）
$ErrorActionPreference = 'Continue'
$root = Split-Path -Parent $PSScriptRoot
$rcc = Join-Path $root "release\RCC.exe"
$outDir = Join-Path $root "build\test_p12_3c"
$utf8 = New-Object System.Text.UTF8Encoding($false)

if (-not (Test-Path $outDir)) { New-Item -ItemType Directory -Path $outDir -Force | Out-Null }

$script:pass = 0
$script:fail = 0
$script:failures = @()

function Invoke-Case {
    param([string]$Name, [string]$Body, [string]$Expect)
    $p = Join-Path $outDir "$Name.rio"
    [System.IO.File]::WriteAllText($p, $Body, $utf8)
    $exe = Join-Path $outDir "$Name.exe"
    if (Test-Path $exe) { Remove-Item $exe -Force }

    $co = & $rcc -build --path $p --output $exe 2>&1 | Out-String
    if ($LASTEXITCODE -ne 0) {
        $script:fail += 1
        $err = (($co -split "`n") | Where-Object { $_ -match 'Error' } | Select-Object -First 1)
        $msg = if ($err) { ($err.Trim() -replace '\s+', ' ') } else { 'compile failed' }
        $script:failures += "$Name [COMPILE] $msg"
        Write-Host ("  [FAIL] {0,-32} COMPILE: {1}" -f $Name, $msg.Substring(0,[Math]::Min(90,$msg.Length))) -ForegroundColor Red
        return
    }
    if (-not (Test-Path $exe)) {
        $script:fail += 1
        $script:failures += "$Name [LINK] no exe"
        Write-Host ("  [FAIL] {0,-32} LINK: no exe" -f $Name) -ForegroundColor Red
        return
    }

    $run = & $exe 2>&1 | Out-String
    $actual = ($run.Trim() -replace "`r`n", "/") -replace "\s+", ""
    $want = ($Expect.Trim() -replace "\s+", "")
    if ($actual -eq $want) {
        $script:pass += 1
        Write-Host ("  [ OK ] {0,-32} {1}" -f $Name, $actual) -ForegroundColor Green
    }
    else {
        $script:fail += 1
        $script:failures += "$Name expect=[$want] actual=[$actual]"
        Write-Host ("  [FAIL] {0,-32} expect=[{1}] actual=[{2}]" -f $Name, $want, $actual) -ForegroundColor Red
    }
}

$imp = "import(ds=`"ds`", sm=`"str`")`n"

Write-Host ""
Write-Host "=========== 第一层：compile ===========" -ForegroundColor Cyan
Invoke-Case "c01_compile_only" ($imp + "var xs: list[int] = [1]`nsout(core_str_len(xs))") "1"

Write-Host ""
Write-Host "=========== 第二层：API（22 个导出名）===========" -ForegroundColor Cyan

# isEmpty
Invoke-Case "a01_isEmpty" ($imp + @'
sout(isEmpty([]))
sout(isEmpty([1]))
'@) "true/false"

# top
Invoke-Case "a02_top" ($imp + @'
sout(top([1, 2, 3]))
sout(top(["a", "b"]))
'@) "3/b"

# copy
Invoke-Case "a03_copy" ($imp + @'
var a: list[int] = [1, 2, 3]
var b: list[int] = copy(a)
sout(core_str_len(b))
sout(b[0])
sout(b[2])
'@) "3/1/3"

# repStr
Invoke-Case "a04_repStr" ($imp + @'
sout(repStr("ab", 3))
sout(core_str_len(repStr("x", 0)))
sout(core_str_len(repStr("x", -2)))
'@) "ababab/0/0"

# repList
Invoke-Case "a05_repList" ($imp + @'
var a: list[int] = repList(7, 3)
sout(core_str_len(a))
sout(a[0])
sout(a[2])
'@) "3/7/7"

# listAppend
Invoke-Case "a06_listAppend" ($imp + @'
var a: list[int] = [1, 2]
var b: list[int] = listAppend(a, 3)
sout(core_str_len(a))
sout(core_str_len(b))
sout(b[2])
'@) "2/3/3"

# listRemove
Invoke-Case "a07_listRemove" ($imp + @'
var a: list[int] = [10, 20, 30]
listRemove(a, 1)
sout(core_str_len(a))
sout(a[0])
sout(a[1])
'@) "2/10/30"

# dictRemove
Invoke-Case "a08_dictRemove" ($imp + @'
var d: dict = {"aa": 11, "bb": 22, "cc": 33}
dictRemove(d, "bb")
sout(core_str_len(d))
sout(d["aa"])
sout(d["cc"])
'@) "2/11/33"

# dictKeys
Invoke-Case "a09_dictKeys" ($imp + @'
var d: dict = {"zz": 1, "aa": 2}
var ks: list[str] = dictKeys(d)
sout(core_str_len(ks))
sout(ks[0])
sout(ks[1])
'@) "2/zz/aa"

# dictValues
Invoke-Case "a10_dictValues" ($imp + @'
var d: dict = {"aa": 11, "bb": 22}
var vs: list = dictValues(d)
sout(core_str_len(vs))
sout(vs[0])
sout(vs[1])
'@) "2/11/22"

# contains
Invoke-Case "a11_contains" ($imp + @'
sout(contains([1, 2, 3], 2))
sout(contains([1, 2, 3], 9))
sout(contains(["a", "bb"], "bb"))
'@) "true/false/true"

# indexOf
Invoke-Case "a12_indexOf" ($imp + @'
sout(indexOf([1, 2, 3], 3))
sout(indexOf([1, 2, 3], 9))
'@) "2/-1"

# map
Invoke-Case "a13_map" ($imp + @'
var a: list[int] = map([1, 2, 3], (v: int) -> { ret v * 2 })
sout(core_str_len(a))
sout(a[0])
sout(a[2])
'@) "3/2/6"

# filter
Invoke-Case "a14_filter" ($imp + @'
var a: list[int] = filter([1, 2, 3, 4], (v: int) -> { ret v % 2 == 0 })
sout(core_str_len(a))
sout(a[0])
sout(a[1])
'@) "2/2/4"

# reverse
Invoke-Case "a15_reverse" ($imp + @'
var a: list[int] = reverse([1, 2, 3])
sout(a[0])
sout(a[2])
'@) "3/1"

# slice
Invoke-Case "a16_slice" ($imp + @'
var a: list[int] = slice([1, 2, 3, 4], 1, 3)
sout(core_str_len(a))
sout(a[0])
sout(a[1])
'@) "2/2/3"

# concat
Invoke-Case "a17_concat" ($imp + @'
var a: list[int] = concat([1, 2], [3])
sout(core_str_len(a))
sout(a[2])
'@) "3/3"

# sum
Invoke-Case "a18_sum" ($imp + @'
sout(sum([1, 2, 3]))
sout(sum([]))
'@) "6/0"

# sort
Invoke-Case "a19_sort" ($imp + @'
var a: list[int] = sort([3, 1, 2])
sout(a[0])
sout(a[1])
sout(a[2])
var b: list[int] = sort([3, 1, 2], true)
sout(b[0])
sout(b[2])
'@) "1/2/3/3/1"

# join
Invoke-Case "a20_join" ($imp + @'
sout(join([1, 2, 3], "-"))
sout(join(["a", "b"]))
sout(core_str_len(join([], "-")))
'@) "1-2-3/ab/0"

# repeat
Invoke-Case "a21_repeat" ($imp + @'
repeat(3, (i: int) -> { sout(i) })
'@) "0/1/2"

# forInRange
Invoke-Case "a22_forInRange" ($imp + @'
forInRange(0, 3, (i: int) -> { sout(i) })
forInRange(0, 6, (i: int) -> { sout(i) }, 2)
'@) "0/1/2/0/2/4"

Write-Host ""
Write-Host "=========== 第三层：behavior 边界 ===========" -ForegroundColor Cyan

Invoke-Case "b01_top_empty" ($imp + @'
sout(top([]))
sout(top([9]))
'@) "0/9"

Invoke-Case "b02_sort_empty_single" ($imp + @'
var a: list[int] = sort([])
sout(core_str_len(a))
var b: list[int] = sort([7])
sout(b[0])
'@) "0/7"

Invoke-Case "b03_sort_str" ($imp + @'
var a: list[str] = sort(["cc", "aa", "bb"])
sout(a[0])
sout(a[2])
'@) "aa/cc"

Invoke-Case "b04_sort_float" ($imp + @'
var a: list[float] = sort([3.5, 1.25, 2.0])
sout(a[0])
sout(a[2])
'@) "1.25/3.5"

Invoke-Case "b05_slice_bounds" ($imp + @'
var a: list[int] = slice([1, 2, 3], -1, 99)
sout(core_str_len(a))
var b: list[int] = slice([1, 2, 3], 2, 1)
sout(core_str_len(b))
'@) "3/0"

Invoke-Case "b06_listRemove_edges" ($imp + @'
var a: list[int] = [1, 2, 3]
listRemove(a, 0)
sout(a[0])
var b: list[int] = [1, 2, 3]
listRemove(b, 2)
sout(core_str_len(b))
var c: list[int] = [5]
listRemove(c, 0)
sout(core_str_len(c))
'@) "2/2/0"

Invoke-Case "b07_listRemove_oob" ($imp + @'
var a: list[int] = [1, 2]
listRemove(a, 9)
listRemove(a, -1)
sout(core_str_len(a))
'@) "2"

Invoke-Case "b08_dictRemove_missing" ($imp + @'
var d: dict = {"aa": 1}
dictRemove(d, "zz")
sout(core_str_len(d))
sout(d["aa"])
'@) "1/1"

Invoke-Case "b09_map_filter_empty" ($imp + @'
var a: list[int] = map([], (v: int) -> { ret v })
sout(core_str_len(a))
var b: list[int] = filter([], (v: int) -> { ret true })
sout(core_str_len(b))
'@) "0/0"

Invoke-Case "b10_contains_empty" ($imp + @'
sout(contains([], 1))
sout(indexOf([], 1))
'@) "false/-1"

Invoke-Case "b11_reverse_empty" ($imp + @'
var a: list[int] = reverse([])
sout(core_str_len(a))
'@) "0"

Invoke-Case "b12_forInRange_zero_step" ($imp + @'
forInRange(0, 5, (i: int) -> { sout(i) }, 0)
sout("done")
'@) "done"

Invoke-Case "b13_forInRange_neg" ($imp + @'
forInRange(3, 0, (i: int) -> { sout(i) }, -1)
'@) "3/2/1"

Invoke-Case "b14_concat_empty" ($imp + @'
var a: list[int] = concat([], [1])
sout(core_str_len(a))
var b: list[int] = concat([1], [])
sout(core_str_len(b))
'@) "1/1"

Invoke-Case "b15_dictKeys_empty" ($imp + @'
var d: dict = {}
var ks: list = dictKeys(d)
sout(core_str_len(ks))
'@) "0"

Invoke-Case "b16_copy_independent" ($imp + @'
var a: list[int] = [1, 2, 3]
var b: list[int] = copy(a)
b[0] = 99
sout(a[0])
sout(b[0])
'@) "1/99"

Invoke-Case "b17_join_single" ($imp + @'
sout(join([42], "-"))
'@) "42"

Write-Host ""
Write-Host "================================================" -ForegroundColor Cyan
$total = $script:pass + $script:fail
Write-Host ("  总计 {0} 项：通过 {1}，失败 {2}" -f $total, $script:pass, $script:fail) -ForegroundColor $(if ($script:fail -eq 0) { "Green" } else { "Red" })
if ($script:fail -gt 0) {
    Write-Host ""
    Write-Host "失败清单：" -ForegroundColor Red
    $script:failures | ForEach-Object { Write-Host "  - $_" -ForegroundColor Red }
    exit 1
}
exit 0

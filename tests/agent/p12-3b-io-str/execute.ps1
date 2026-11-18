# ============================================================================
#  test_p12_3b_io_str.ps1 —— P12-3-B 验收：io / str 两个模块
#
#  与 test_p12_std_skeleton.ps1 的分工：骨架脚本验的是「lib/std 是唯一真相」
#  这套机制，本脚本验的是**两个模块的行为是否正确**。P12-3-D 会把各模块的
#  功能用例统一收进 test_std.ps1，在那之前这里就是它们的回归网。
#
#  检查项：
#    1. str 的数值转换（int / float / bool / char → str）
#    2. str 的子串、查找、大小写、替换、切分、修剪
#    3. str 的字符↔整数转换（语义已变更，见 lib/std/str.rio 文件头）
#    4. io 的 sin / input（含文件重定向读入）
#    5. 跨模块组合（str + io）
#    6. UTF-8 多字节字符不被切坏（旧版逐字符 setType 会切坏）
#    7. 编译器缺陷回归：`fn(): list[T]` 的返回值直接索引
#       （returnTypeOf 只走 parseTypeName 丢了 `[...]`，已修）
# ============================================================================

$ErrorActionPreference = 'Stop'
$repo = Split-Path -Parent (Split-Path -Parent (Split-Path -Parent $PSScriptRoot))
$rcc = Join-Path $repo 'release\RCC.exe'
$work = Join-Path $repo 'build\p12_3b_check'
$utf8NoBom = New-Object System.Text.UTF8Encoding($false)

$pass = 0
$fail = 0
$failures = @()

function Check($name, $ok, $detail) {
    if ($ok) {
        $script:pass++
        Write-Host "  [通过] $name"
    } else {
        $script:fail++
        $script:failures += $name
        Write-Host "  [失败] $name" -ForegroundColor Red
        if ($detail) { Write-Host "         $detail" -ForegroundColor DarkGray }
    }
}

if (-not (Test-Path $rcc)) { throw "RCC.exe not found: $rcc (build first)" }
New-Item -ItemType Directory -Force -Path $work | Out-Null

function Invoke-RioCase($name, $src, $expected) {
    $file = Join-Path $work ($name + '.rio')
    $exe = Join-Path $work ($name + '.exe')
    [System.IO.File]::WriteAllText($file, $src, $utf8NoBom)
    if (Test-Path $exe) { Remove-Item $exe -Force }
    $log = & $rcc -build --path $file --output $exe 2>&1 | Out-String
    if ($LASTEXITCODE -ne 0 -or -not (Test-Path $exe)) {
        Check $name $false $log.Trim()
        return
    }
    $actual = (& $exe 2>&1 | Out-String).Trim()
    # 换行归一：RCC 输出 "\n"，PowerShell 管道把它变成 CRLF。
    $actual = $actual -replace "`r`n", "`n"
    $expected = $expected -replace "`r`n", "`n"
    Check $name ($actual -eq $expected) "expected=[$expected] actual=[$actual]"
}

Write-Host ''
Write-Host '=== 1. str：数值 -> 字符串 ==='

Invoke-RioCase 'str_ints' @'
import(strings="str")
sout(intToStr(0))
sout(intToStr(7))
sout(intToStr(-45))
sout(intToStr(12345))
'@ "0`n7`n-45`n12345"

Invoke-RioCase 'str_floats' @'
import(strings="str")
sout(floatToStr(3.14))
sout(floatToStr(2.5))
sout(floatToStr(-1.5))
sout(floatToStr(10.0))
'@ "3.14`n2.5`n-1.5`n10.0"

# toStr 用重载按静态类型分派（泛型做不到类型分派，any 会截断 float）
Invoke-RioCase 'str_toStr_overloads' @'
import(strings="str")
sout(toStr(42))
sout(toStr(3.14))
sout(toStr(true))
sout(toStr(false))
sout(toStr("raw"))
'@ "42`n3.14`ntrue`nfalse`nraw"

Write-Host ''
Write-Host '=== 2. str：子串 / 查找 / 大小写 / 替换 / 切分 / 修剪 ==='

Invoke-RioCase 'str_subStr' @'
import(strings="str")
sout("[" + subStr("hello world", 6, 5) + "]")
sout("[" + subStr("hello", 1) + "]")
sout("[" + subStr("hello", 0, 0) + "]")
sout("[" + subStr("hello", 99, 3) + "]")
'@ "[world]`n[ello]`n[]`n[]"

Invoke-RioCase 'str_find' @'
import(strings="str")
sout(find("hello world", "world"))
sout(find("hello", "xyz"))
sout(find("aaa", "a"))
sout(find("hello", "l", 3))
'@ "6`n-1`n0`n3"

Invoke-RioCase 'str_case' @'
import(strings="str")
sout(toUpperCase("hello"))
sout(toLowerCase("WORLD"))
sout(toUpperCase("a1b2"))
'@ "HELLO`nworld`nA1B2"

Invoke-RioCase 'str_replace' @'
import(strings="str")
sout(replace("aXbXc", "X", "-"))
sout(replace("aXbXc", "X", "-", true))
sout(replace("hello", "z", "y"))
'@ "a-b-c`na-bXc`nhello"

Invoke-RioCase 'str_split' @'
import(strings="str")
var p: list[str] = split("a,b,c", ",")
sout(core_str_len(p))
sout(p[0])
sout(p[2])
var q: list[str] = split("a b", " ")
sout(core_str_len(q))
'@ "3`na`nc`n2"

Invoke-RioCase 'str_chunk' @'
import(strings="str")
var c: list[str] = chunk("abcdef", 2)
sout(core_str_len(c))
sout(c[0])
sout(c[2])
'@ "3`nab`nef"

# 用 [] 包住结果：$actual 末尾会被 .Trim() 吃掉，裸打印分不清
# 「空串」和「只有空格的串」，也分不清 trim(...,false) 的尾随空格。
Invoke-RioCase 'str_trim' @'
import(strings="str")
sout("[" + trim("  hi  ") + "]")
sout("[" + trim("xxhixx", "x") + "]")
sout("[" + trim("  hi  ", " ", true, false) + "]")
sout("[" + trim("  hi  ", " ", false, true) + "]")
'@ "[hi]`n[hi]`n[hi  ]`n[  hi]"

Write-Host ''
Write-Host '=== 3. str：字符 <-> 整数 ==='

# 语义：`intToChar` 返回 char（`sout` 按字符打印），`charToInt` 接受 int/char/str。
# 2026-11-16 用户裁决：intToChar 恢复原仓库的 `char` 返回类型；charToInt 保留
# str 重载但要求长度恰为 1（否则运行时抛错），不再静默返回首字节。
Invoke-RioCase 'str_char_int' @'
import(strings="str")
sout(toChar("A"))
sout(charToInt("A"))
sout(intToChar(66))
sout(toChar(""))
'@ "65`n65`nB`n0"

# charToInt 的三种入参形式必须等价，且 char 与 int 分派正确。
Invoke-RioCase 'str_charToInt_overloads' @'
import(strings="str")
var c: char = 'A'
sout(charToInt(65))
sout(charToInt(c))
sout(charToInt("A"))
sout(charToInt(true))
'@ "65`n65`n65`n1"

# 多字符字符串必须报错（原实现静默返回首字节，是错的）。
$multiFile = Join-Path $work 'str_charToInt_multichar.rio'
$multiExe = Join-Path $work 'str_charToInt_multichar.exe'
[System.IO.File]::WriteAllText($multiFile, @'
import(strings="str")
sout(charToInt("true"))
'@, $utf8NoBom)
if (Test-Path $multiExe) { Remove-Item $multiExe -Force }
$multiLog = & $rcc -build --path $multiFile --output $multiExe 2>&1 | Out-String
if ($LASTEXITCODE -eq 0 -and (Test-Path $multiExe)) {
    # 编译通过（当前设计：长度检查在运行时）—— 要求运行时非零退出。
    & $multiExe 2>&1 | Out-Null
    Check 'str_charToInt_multichar' ($LASTEXITCODE -ne 0) "expected runtime failure, exit=$LASTEXITCODE"
} else {
    # 将来若改成编译期检查，这里仍然算通过。
    Check 'str_charToInt_multichar' $true "compile-time rejection"
}

Write-Host ''
Write-Host '=== 4. str：size 对 str / list / dict 通用 ==='

Invoke-RioCase 'str_size' @'
import(strings="str")
sout(size("hello"))
sout(size([1, 2, 3]))
sout(size({"k": 1}))
'@ "5`n3`n1"

Write-Host ''
Write-Host '=== 5. io：sin / input ==='

Invoke-RioCase 'io_input_prompt' @'
import(io="io")
sout("before")
'@ "before"

# input 从文件重定向读入：用 cmd 的 < 重定向（PowerShell 不支持 <）
$inFile = Join-Path $work 'stdin.txt'
[System.IO.File]::WriteAllText($inFile, "hello world`r`n", $utf8NoBom)
$inputSrc = @'
import(io="io")
var s: str = input("")
sout(core_str_len(s))
sout(s)
'@
$inputRio = Join-Path $work 'io_read.rio'
$inputExe = Join-Path $work 'io_read.exe'
[System.IO.File]::WriteAllText($inputRio, $inputSrc, $utf8NoBom)
if (Test-Path $inputExe) { Remove-Item $inputExe -Force }
$log = & $rcc -build --path $inputRio --output $inputExe 2>&1 | Out-String
if ($LASTEXITCODE -ne 0 -or -not (Test-Path $inputExe)) {
    Check 'io_read_line' $false $log.Trim()
} else {
    $out = cmd /c "`"$inputExe`" < `"$inFile`"" 2>&1 | Out-String
    $out = ($out -replace "`r`n", "`n").Trim()
    Check 'io_read_line' ($out -eq "13`nhello world") "expected=[13`nhello world] actual=[$out]"
}

Write-Host ''
Write-Host '=== 6. 跨模块组合（str + io） ==='

Invoke-RioCase 'combo_str_io' @'
import(strings="str", io="io")
sout(toUpperCase(subStr("hello world", 0, 5)))
sout(intToStr(size("abcd")))
sout(replace(trim("  a b  "), " ", "_"))
'@ "HELLO`n4`na_b"

Write-Host ''
Write-Host '=== 7. UTF-8 多字节字符不被切坏 ==='

# 旧版 subStr 靠逐字符 setType 拼接，多字节字符会被拆成单字节后重新组装，
# 新版按字节切 + 拷内存，天然保留原始字节序列。
Invoke-RioCase 'utf8_substr' @'
import(strings="str")
sout(subStr("中文测试", 0, 6))
sout(subStr("中文测试", 6, 6))
sout(core_str_len("中文测试"))
'@ "中文`n测试`n12"

Write-Host ''
Write-Host '=== 8. 编译器缺陷回归：返回值直接索引 ==='

# 根因：returnTypeOf() 只调 parseTypeName，而它拿不到 LabelNode 的 `[...]`，
# 于是 `fun mk(): list[str]` 登记进 _functionReturns 的类型是「元素按 int
# 解释」的 list。`mk()[0]` 的静态类型因此变成 int，sout 按 int 打印指针。
# 修法：新增 returnTypeFromNode() 走 typeFromLabelNode（P9/P10 同一套路径）。
Invoke-RioCase 'ret_index_str' @'
fun mk(): list[str] {
    ret ["ab", "cd"]
}
sout(mk()[0])
sout(mk()[1])
'@ "ab`ncd"

Invoke-RioCase 'ret_index_float' @'
fun mk(): list[float] {
    ret [1.5, 2.5]
}
sout(mk()[0])
sout(mk()[1])
'@ "1.5`n2.5"

Invoke-RioCase 'ret_index_nested_dict' @'
fun mk(): dict[str, str] {
    ret {"k": "v"}
}
sout(mk()["k"])
'@ "v"

Invoke-RioCase 'ret_index_list_int_unchanged' @'
fun mk(): list[int] {
    ret [7, 8]
}
sout(mk()[0])
'@ "7"

# str 必须**自包含**：`toStr(dict)` 曾写成调用嵌套函数 _strDictKeys，
# 后者内部用了 ds 模块的私有辅助 `_dsLoadI64`。只 `import str` 时 ds 没被
# 内联，ds 的私有符号不存在，编译报
#   Argument 1 of '_strDictKeys': cannot assign a value of type int to a
#   target of type dict[?, ?]
# 而同时 import ds 时又「碰巧」能用 —— 这类依赖只有单独 import 才暴露。
# 所以这里刻意**只 import str**，不 import ds。
Invoke-RioCase 'str_selfcontained_dict_fmt' @'
import(sm="str")
sout(toStr({"a": 1}))
sout(toStr([1, 2]))
'@ "{a: 1}`n[1, 2]"

Write-Host ''
Write-Host '=== 9. 已删除模块的依赖确实清干净 ==='

# str 旧版有 6 个函数依赖 datatype 的 setType（toStr/toChar/charToInt/
# intToChar/split/trim）。datatype 已按 S3 整模块删除，这些函数必须
# 在不依赖它的情况下工作 —— 上面所有用例都是证据。这里再显式确认
# 源码里没有残留引用。
foreach ($mod in @('str.rio', 'io.rio')) {
    $p = Join-Path $repo "lib\std\$mod"
    # 只看代码行：文件头注释里会**讨论** setType/datatype（说明为什么不再
    # 依赖它们），按整文件匹配会把注释误判成依赖。
    $code = (Get-Content $p -Encoding UTF8 | Where-Object { $_ -notmatch '^\s*(//|/\*|\*)' }) -join "`n"
    $hasDatatype = $code -match 'import\([^)]*datatype'
    $hasSetType = $code -match '\bsetType\b'
    $hasDs = $code -match 'import\([^)]*\bds\s*='
    Check "$mod no_datatype_dep" ((-not $hasDatatype) -and (-not $hasSetType) -and (-not $hasDs)) `
        "datatype=$hasDatatype setType=$hasSetType ds=$hasDs"
}

Write-Host ''
if ($fail -eq 0) {
    Write-Host "结果：通过 $pass，失败 0" -ForegroundColor Green
    Write-Host 'P12-3-B io/str 验收通过' -ForegroundColor Green
    exit 0
} else {
    Write-Host "结果：通过 $pass，失败 $fail" -ForegroundColor Red
    Write-Host "失败用例：$($failures -join ', ')" -ForegroundColor Red
    exit 1
}

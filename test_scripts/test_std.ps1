# ============================================================================
#  test_std.ps1 —— P12-3-D 标准库三层验收（compile / api / behavior）
#
#  设计依据：docs/p12_plan.md §5.6。
#
#  「编译通过 ≠ 实现正确」，所以分三层，任一层非全绿该模块即判定未完成：
#
#    第一层 compile  —— lib/std/*.rio 逐个 -emit-lib 编译；且验「唯一真相」
#                       机制（release/Lib/builtin 是逐字节拷贝、未被 git 跟踪、
#                       UTF-8 无 BOM、不残留 encapsulated / 已删模块依赖）。
#    第二层 api      —— 每个模块的**每个公开函数**都要有调用点，且 -s 导出的
#                       签名与基线表逐条一致（名字 / 参数个数顺序 / 默认参数 /
#                       返回类型）；已删 API 必须确实不存在。
#    第三层 behavior —— 每个公开函数至少 1 个行为用例（脚本自己校验覆盖率，
#                       漏了会失败），并覆盖正常 / 边界 / 空 / 非法 / 内存五类。
#
#  用法：
#    powershell -NoProfile -ExecutionPolicy Bypass -File test_scripts\test_std.ps1
#    powershell ... -File test_scripts\test_std.ps1 -Layer compile
#    powershell ... -File test_scripts\test_std.ps1 -Layer api
#    powershell ... -File test_scripts\test_std.ps1 -Layer behavior
#
#  退出码：三层全绿 0，任一失败 1。
#
#  与 test_p12_3b_io_str.ps1 / test_p12_3c_ds.ps1 的关系：那两个是 P12-3-B/C
#  的阶段性回归网，本脚本是它们的超集与收口（覆盖全部 6 个模块、加 API 层与
#  覆盖率校验）。三者都保留：前者验「当时怎么修的」，本脚本验「现在是否完整」。
# ============================================================================

param(
    [ValidateSet('compile', 'api', 'behavior', 'all')]
    [string]$Layer = 'all'
)

$ErrorActionPreference = 'Continue'
$repo = Split-Path -Parent $PSScriptRoot
$rcc = Join-Path $repo 'release\RCC.exe'
$stdDir = Join-Path $repo 'lib\std'
$builtinDir = Join-Path $repo 'release\Lib\builtin'
$work = Join-Path $repo 'build\std_check'
$utf8NoBom = New-Object System.Text.UTF8Encoding($false)

if (-not (Test-Path $rcc)) { throw "RCC.exe not found: $rcc (build first: mingw32-make -C build -j 10)" }
New-Item -ItemType Directory -Force -Path $work | Out-Null

$modules = @('ds', 'str', 'math', 'ops', 'io', 'prog')

# ============================================================================
#  基线表：每个模块的公开函数名（用于覆盖率校验）与导出签名（用于 API diff）
#
#  签名来自 `RCC -s --p lib/std/<m>.rio --fmt text -exp` 的实际输出，也就是
#  编译器眼中的真身。改标准库签名必须同步改这张表 —— 这是刻意的：签名变更
#  属于 API 变更，应当被人看见，而不是被测试自动跟随。
# ============================================================================

$expectedFns = [ordered]@{
    ds   = @('concat', 'contains', 'copy', 'dictKeys', 'dictRemove', 'dictValues',
             'dumpToStr', 'filter', 'forInRange', 'indexOf', 'isEmpty', 'join',
             'listAppend', 'listRemove', 'map', 'repList', 'repStr', 'repeat',
             'reverse', 'slice', 'sort', 'sum', 'toFloat', 'top')
    str  = @('charToInt', 'chunk', 'find', 'floatToStr', 'intToChar', 'intToStr',
             'replace', 'size', 'split', 'subStr', 'toChar', 'toLowerCase',
             'toStr', 'toUpperCase', 'trim')
    math = @('abs', 'max', 'min', 'opp', 'pow', 'e', 'pi')
    ops  = @('orLazy', 'orVal', 'ternary', 'ternaryLazy')
    io   = @('input', 'sin')
    prog = @('breakpoint', 'entry')
}

$expectedSigs = @{
    ds   = @(
        'concat[T](a: list, b: list): list'
        'contains[T](xs: list, t: T): bool'
        'copy[T](xs: list): list'
        'dictKeys(d: dict): list'
        'dictRemove(d: dict, key: int): void'
        'dictRemove(d: dict, key: str): void'
        'dictValues(d: dict): list'
        'dumpToStr[T, K](container: dict): str'
        'dumpToStr[T](container: list): str'
        'filter[T](xs: list, h: funi): list'
        'forInRange(start: int, end: int, h: funi, sep: int = 1): void'
        'indexOf[T](xs: list, t: T): int'
        'isEmpty(arg: dict): bool'
        'isEmpty(arg: list): bool'
        'isEmpty(arg: str): bool'
        'join[T](xs: list, sep: str = ""): str'
        'listAppend[T](xs: list, item: T): list'
        'listRemove[T](xs: list, index: int): void'
        'map[T](xs: list, h: funi): list'
        'repList[T](x: T, times: int): list'
        'repStr(s: str, times: int): str'
        'repeat(times: int, h: funi): void'
        'reverse[T](xs: list): list'
        'slice[T](xs: list, start: int, end: int): list'
        'sort[T](xs: list, reverse: bool = false): list'
        'sum(xs: list): int'
        'toFloat(arg: bool): float'
        'toFloat(arg: int): float'
        'top[T](xs: list): T'
    )
    str  = @(
        'charToInt(c: int): int'
        'charToInt(c: char): int'
        'charToInt(s: str): int'
        'chunk(content: str, num: int): list'
        'find(content: str, part: str, startIndex: int = 0): int'
        'floatToStr(v: float): str'
        'intToChar(i: int): char'
        'intToStr(v: int): str'
        'replace(content: str, old: str, new: str, once: bool = false): str'
        'size(arg: any): int'
        'split(content: str, del: str, keepDel: bool = false, keepEmpty: bool = false): list'
        'subStr(content: str, startIndex: int = 0, length: int = -1): str'
        'toChar(arg: str): int'
        'toChar(arg: int): int'
        'toLowerCase(data: str): str'
        'toStr(arg: int): str'
        'toStr(arg: float): str'
        'toStr(arg: bool): str'
        'toStr(arg: str): str'
        'toStr(arg: char): str'
        'toStr[T, K](arg: dict): str'
        'toStr[T](arg: list): str'
        'toUpperCase(data: str): str'
        'trim(content: str, extraChars: str = " ", onLeft: bool = true, onRight: bool = true): str'
    )
    math = @(
        'abs[T](arg: T): T'
        'max[T](a: T, b: T): T'
        'min[T](a: T, b: T): T'
        'opp[T](arg: T): T'
        'pow[T](a: T, times: int): T'
        'e: float'
        'pi: float'
    )
    ops  = @(
        'orLazy[T](front: T, back: funi): T'
        'orVal[T](fallback: T, spare: T): T'
        'ternary[T](condition: bool, trueValue: T, falseValue: T): T'
        'ternaryLazy[T](condition: bool, trueFuni: funi, falseFuni: funi): T'
    )
    io   = @(
        'input(prompt: str = ""): str'
        'sin(): str'
    )
    prog = @(
        'breakpoint(): void'
        'entry(arg: any, *args: any): void'
    )
}

# ============================================================================
#  记账
# ============================================================================

$script:layerPass = @{}
$script:layerFail = @{}
$script:layerNote = @{}
foreach ($l in @('compile', 'api', 'behavior')) {
    $script:layerPass[$l] = @{}
    $script:layerFail[$l] = @{}
    $script:layerNote[$l] = @{}
    foreach ($m in $modules) {
        $script:layerPass[$l][$m] = 0
        $script:layerFail[$l][$m] = 0
        $script:layerNote[$l][$m] = ''
    }
}
$script:failures = @()
$script:covered = @{}      # behavior 层：已覆盖的 <module>.<fn>
foreach ($m in $modules) { foreach ($fn in $expectedFns[$m]) { $script:covered["$m.$fn"] = $false } }

function Record($layerName, $moduleName, $ok, $name, $detail) {
    if ($ok) {
        $script:layerPass[$layerName][$moduleName]++
    } else {
        $script:layerFail[$layerName][$moduleName]++
        $script:failures += "[$layerName/$moduleName] $name :: $detail"
    }
    $tag = if ($ok) { '[通过]' } else { '[失败]' }
    $color = if ($ok) { 'Green' } else { 'Red' }
    Write-Host ("  {0} {1,-46}" -f $tag, $name) -ForegroundColor $color
    if (-not $ok -and $detail) { Write-Host "         $detail" -ForegroundColor DarkGray }
}

# 跑一个 Rio 程序并比对输出。返回 $true/$false，同时记账。
function Invoke-RioCase {
    param(
        [string]$LayerName,
        [string]$Module,
        [string]$Name,
        [string]$Src,
        [string]$Expect,
        [string[]]$Covers = @()
    )
    foreach ($c in $Covers) { if ($script:covered.ContainsKey($c)) { $script:covered[$c] = $true } }

    $file = Join-Path $work "$Name.rio"
    $exe = Join-Path $work "$Name.exe"
    [System.IO.File]::WriteAllText($file, $Src, $utf8NoBom)
    if (Test-Path $exe) { Remove-Item $exe -Force }

    $log = & $rcc -build --path $file --output $exe 2>&1 | Out-String
    if ($LASTEXITCODE -ne 0 -or -not (Test-Path $exe)) {
        $err = (($log -split "`n") | Where-Object { $_ -match 'Error' } | Select-Object -First 1)
        $msg = if ($err) { ($err.Trim() -replace '\s+', ' ') } else { 'compile failed' }
        Record $LayerName $Module $false $Name "COMPILE: $msg"
        return $false
    }

    $actual = (& $exe 2>&1 | Out-String)
    $code = $LASTEXITCODE
    # 换行归一：RCC 输出 "\n"，PowerShell 管道把它变成 CRLF。
    $actual = ($actual.Trim() -replace "`r`n", "`n")
    $want = ($Expect.Trim() -replace "`r`n", "`n")
    $ok = ($actual -eq $want) -and ($code -eq 0)
    $detail = if ($ok) { '' } elseif ($code -ne 0) { "exit=$code expected=[$want] actual=[$actual]" }
              else { "expected=[$want] actual=[$actual]" }
    Record $LayerName $Module $ok $Name $detail
    return $ok
}

function Invoke-RioCompiles {
    param(
        [string]$LayerName,
        [string]$Module,
        [string]$Name,
        [string]$Src,
        [string[]]$Covers = @()
    )
    foreach ($c in $Covers) { if ($script:covered.ContainsKey($c)) { $script:covered[$c] = $true } }
    $file = Join-Path $work "$Name.rio"
    $exe = Join-Path $work "$Name.exe"
    [System.IO.File]::WriteAllText($file, $Src, $utf8NoBom)
    if (Test-Path $exe) { Remove-Item $exe -Force }
    $log = & $rcc -build --path $file --output $exe 2>&1 | Out-String
    $ok = ($LASTEXITCODE -eq 0) -and (Test-Path $exe)
    $msg = if ($ok) { '' } else { (($log -split "`n") | Where-Object { $_ -match 'Error' } | Select-Object -First 1) }
    Record $LayerName $Module $ok $Name ($msg -replace '\s+', ' ')
    return $ok
}

# 期望编译失败（用于「已删 API 确实不存在」）
function Invoke-RioFails {
    param(
        [string]$LayerName,
        [string]$Module,
        [string]$Name,
        [string]$Src,
        [string]$Match
    )
    $file = Join-Path $work "$Name.rio"
    $exe = Join-Path $work "$Name.exe"
    [System.IO.File]::WriteAllText($file, $Src, $utf8NoBom)
    if (Test-Path $exe) { Remove-Item $exe -Force }
    $log = & $rcc -build --path $file --output $exe 2>&1 | Out-String
    $failed = ($LASTEXITCODE -ne 0) -or (-not (Test-Path $exe))
    $matched = [string]::IsNullOrEmpty($Match) -or ($log -match [regex]::Escape($Match))
    Record $LayerName $Module ($failed -and $matched) $Name `
        "expected failure matching '$Match'; got: $(($log -replace '\s+', ' ').Trim())"
}

# ============================================================================
#  第一层：compile
# ============================================================================

function Invoke-CompileLayer {
    Write-Host ''
    Write-Host '================ 第一层：compile ================' -ForegroundColor Cyan

    $srcFiles = Get-ChildItem $stdDir -Filter *.rio | Sort-Object Name
    if ($srcFiles.Count -eq 0) { throw "no modules in $stdDir" }

    foreach ($f in $srcFiles) {
        $m = $f.BaseName
        if ($modules -notcontains $m) { continue }
        $obj = Join-Path $work "$m.o"
        if (Test-Path $obj) { Remove-Item $obj -Force }
        $log = & $rcc -build -emit-lib --path $f.FullName --output $obj 2>&1 | Out-String
        $ok = ($LASTEXITCODE -eq 0) -and (Test-Path $obj)
        $msg = if ($ok) { '' } else { (($log -split "`n") | Where-Object { $_ -match 'Error' } | Select-Object -First 1) }
        Record 'compile' $m $ok "emit-lib $($f.Name)" ($msg -replace '\s+', ' ')
    }

    # CMake 拷贝一致性：lib/std 是唯一真相，release/Lib/builtin 必须是它的副本。
    foreach ($m in $modules) {
        $src = Join-Path $stdDir "$m.rio"
        $dst = Join-Path $builtinDir "$m.rio"
        if (-not (Test-Path $dst)) {
            Record 'compile' $m $false "copied $m.rio" "missing: $dst"
            continue
        }
        $h1 = (Get-FileHash $src -Algorithm SHA256).Hash
        $h2 = (Get-FileHash $dst -Algorithm SHA256).Hash
        Record 'compile' $m ($h1 -eq $h2) "copied $m.rio" 'content drift between lib/std and release/Lib/builtin'
    }

    # 构建产物不被 git 跟踪
    Push-Location $repo
    foreach ($m in $modules) {
        $tracked = (& git ls-files "release/Lib/builtin/$m.rio" 2>&1 | Out-String).Trim()
        Record 'compile' $m ([string]::IsNullOrEmpty($tracked)) "untracked $m.rio" "still tracked"
    }
    Pop-Location

    # UTF-8 无 BOM（旧 math.rio 是 GB18030，会让词法分析器炸）
    foreach ($m in $modules) {
        $p = Join-Path $stdDir "$m.rio"
        $bytes = [System.IO.File]::ReadAllBytes($p)
        $hasBom = ($bytes.Length -ge 3 -and $bytes[0] -eq 0xEF -and $bytes[1] -eq 0xBB -and $bytes[2] -eq 0xBF)
        $validUtf8 = $true
        try {
            $strict = New-Object System.Text.UTF8Encoding($false, $true)
            [void]$strict.GetString($bytes)
        } catch { $validUtf8 = $false }
        Record 'compile' $m ((-not $hasBom) -and $validUtf8) "utf8 $m.rio" "BOM=$hasBom validUtf8=$validUtf8"
    }

    # 不残留 encapsulated 声明，也不 import 已删除的模块。
    # 注意只查代码行：文件头注释会**讨论**这些名字（解释为什么不再依赖），
    # 按整文件匹配会把注释误判成依赖。
    $deletedModules = @('datatype', 'lowlevel', '_pure_builtin')
    foreach ($m in $modules) {
        $p = Join-Path $stdDir "$m.rio"
        $code = (Get-Content $p -Encoding UTF8 | Where-Object { $_ -notmatch '^\s*(//|/\*|\*)' }) -join "`n"
        $enc = $code -match '\bencapsulated\b'
        Record 'compile' $m (-not $enc) "no encapsulated $m.rio" 'encapsulated declaration found'

        $bad = @()
        foreach ($dm in $deletedModules) {
            if ($code -match "import\([^)]*\b$([regex]::Escape($dm))\s*=") { $bad += $dm }
        }
        Record 'compile' $m ($bad.Count -eq 0) "no deleted dep $m.rio" "imports deleted module(s): $($bad -join ',')"
    }

    # 已删模块的 import 必须干净失败（不是诡异错误）
    Invoke-RioFails 'compile' 'prog' 'import_lowlevel_gone' `
        "import(lowlevel=`"lowlevel`")`nsout(1)`n" 'Cannot find imported module'
}

# ============================================================================
#  第二层：api
# ============================================================================

# 每个模块一个「把每个公开函数都调一遍」的文件。默认参数要**省略写法**也出现，
# 因为「省略能编译且等价于显式传默认值」是 §5.6 的硬要求。
$apiSources = @{
    ds   = @'
import(ds="ds")
var l: list[int] = [1, 2, 3]
var e: list[int] = []
var d: dict = {"a": 1, "b": 2}
sout(isEmpty(l))
sout(isEmpty(""))
sout(isEmpty({}))
sout(top(l))
sout(top(e))
var c: list[int] = copy(l)
sout(core_str_len(c))
sout(repStr("a", 2))
var rl: list[int] = repList(1, 2)
sout(core_str_len(rl))
var la: list[int] = listAppend(l, 4)
sout(core_str_len(la))
var lr: list[int] = [1, 2, 3]
listRemove(lr, 0)
sout(core_str_len(lr))
dictRemove(d, "a")
sout(core_str_len(d))
var ks: list[str] = dictKeys(d)
sout(core_str_len(ks))
var vs: list = dictValues(d)
sout(core_str_len(vs))
sout(contains(l, 2))
sout(indexOf(l, 2))
var mp: list[int] = map(l, (v: int) -> { ret v })
sout(core_str_len(mp))
var fl: list[int] = filter(l, (v: int) -> { ret v > 0 })
sout(core_str_len(fl))
var rv: list[int] = reverse(l)
sout(core_str_len(rv))
var sl: list[int] = slice(l, 0, 1)
sout(core_str_len(sl))
var cc: list[int] = concat(l, l)
sout(core_str_len(cc))
sout(sum(l))
var so: list[int] = sort(l)
sout(core_str_len(so))
var so2: list[int] = sort(l, true)
sout(core_str_len(so2))
sout(join(l, "-"))
sout(join(l))
repeat(2, (i: int) -> { })
forInRange(0, 2, (i: int) -> { })
forInRange(0, 4, (i: int) -> { }, 2)
sout(dumpToStr(l))
var typedDict: dict[str, int] = {"a": 1}
sout(dumpToStr(typedDict))
sout(dumpToStr({"k": 1.5}))
sout(toFloat(true))
sout(toFloat(3))
'@
    str  = @'
import(sm="str")
sout(intToStr(1))
sout(floatToStr(1.5))
sout(toStr(1))
sout(toStr(1.5))
sout(toStr(true))
sout(toStr("s"))
sout(toStr('A'))
sout(toChar("A"))
sout(toChar(65))
sout(charToInt(65))
sout(charToInt('A'))
sout(charToInt("A"))
sout(intToChar(65))
sout("[" + subStr("hello") + "]")
sout("[" + subStr("hello", 1) + "]")
sout("[" + subStr("hello", 1, 2) + "]")
sout(find("hello", "l"))
sout(find("hello", "l", 3))
sout(toLowerCase("AB"))
sout(toUpperCase("ab"))
sout(replace("aa", "a", "b"))
sout(replace("aa", "a", "b", true))
var sp: list[str] = split("a,b", ",")
sout(core_str_len(sp))
var sp2: list[str] = split("a,b", ",", true)
sout(core_str_len(sp2))
var sp3: list[str] = split("a,b", ",", false, true)
sout(core_str_len(sp3))
var ch: list[str] = chunk("abcdef", 2)
sout(core_str_len(ch))
sout("[" + trim("  a  ") + "]")
sout("[" + trim("xxaxx", "x") + "]")
sout("[" + trim("  a  ", " ", true, false) + "]")
sout("[" + trim("  a  ", " ", false, true) + "]")
sout(size("abc"))
'@
    math = @'
import(m="math")
var a: int = abs(-1)
var b: float = abs(-1.5)
var c: int = max(1, 2)
var d: float = max(1.5, 2.5)
var e: int = min(1, 2)
var f: float = min(1.5, 2.5)
var g: int = opp(1)
var h: float = opp(1.5)
var i: int = pow(2, 3)
var j: float = pow(1.5, 2)
var k: float = pi
var l: float = e
sout(a + b)
sout(c + d)
sout(e + f)
sout(g + h)
sout(i + j)
sout(k)
sout(l)
'@
    ops  = @'
import(o="ops")
var a: int = ternary(true, 1, 2)
var b: str = ternary(false, "a", "b")
var c: int = ternaryLazy(true, () -> { ret 1 }, () -> { ret 2 })
var d: int = orVal(null, 10)
var e: int = orLazy(null, () -> { ret 7 })
sout(a)
sout(b)
sout(c)
sout(d)
sout(e)
'@
    io   = @'
import(io="io")
var a: str = sin()
var b: str = input()
var c: str = input("prompt: ")
sout(core_str_len(a) >= 0)
sout(core_str_len(b) >= 0)
sout(core_str_len(c) >= 0)
'@
    prog = @'
import(prog="prog")
breakpoint()
entry(0)
sout("ok")
'@
}

function Invoke-ApiLayer {
    Write-Host ''
    Write-Host '================ 第二层：api ================' -ForegroundColor Cyan

    $report = @()
    $report += 'P12-3-D 标准库 API 验收报告'
    $report += ('生成时间：' + (Get-Date -Format 'yyyy-MM-dd HH:mm:ss'))
    $report += ''

    foreach ($m in $modules) {
        $path = Join-Path $stdDir "$m.rio"

        # --- 2.1 导出签名与基线表逐条 diff ---
        $dump = & $rcc -s --p $path --fmt text -exp 2>&1 | Out-String
        $actual = @()
        foreach ($line in ($dump -split "`n")) {
            if ($line -match '\[exported\]') {
                if ($line -match '^\s+(.*?)\s{2,}\S+\.rio:\d+:\d+') { $actual += ($matches[1].Trim() -replace '\s+', ' ') }
            }
        }
        $want = @($expectedSigs[$m])
        $missing = @($want | Where-Object { $actual -notcontains $_ })
        $extra = @($actual | Where-Object { $want -notcontains $_ })
        $sigOk = ($missing.Count -eq 0) -and ($extra.Count -eq 0)
        $detail = ''
        if ($missing.Count -gt 0) { $detail += "missing: $($missing -join ' | ') " }
        if ($extra.Count -gt 0) { $detail += "unexpected: $($extra -join ' | ')" }
        Record 'api' $m $sigOk "signatures $m ($($want.Count) exported)" $detail

        $report += "[$m] 导出签名 $($actual.Count)/$($want.Count) 一致=$sigOk"
        foreach ($w in $want) {
            $hit = $actual -contains $w
            $report += ('    ' + $(if ($hit) { 'OK  ' } else { 'MISS' }) + ' ' + $w)
        }
        if ($extra.Count -gt 0) { foreach ($x in $extra) { $report += "    EXTRA $x" } }

        # --- 2.2 每个公开函数都有调用点（含默认参数省略写法） ---
        $src = $apiSources[$m]
        $file = Join-Path $work "api_$m.rio"
        $exe = Join-Path $work "api_$m.exe"
        [System.IO.File]::WriteAllText($file, $src, $utf8NoBom)
        if (Test-Path $exe) { Remove-Item $exe -Force }
        $log = & $rcc -build --path $file --output $exe 2>&1 | Out-String
        $ok = ($LASTEXITCODE -eq 0) -and (Test-Path $exe)
        $msg = if ($ok) { '' } else { (($log -split "`n") | Where-Object { $_ -match 'Error' } | Select-Object -First 1) }
        Record 'api' $m $ok "call-sites $m" ($msg -replace '\s+', ' ')
        $report += "    调用点文件 api_$m.rio 编译=$ok"

        # --- 2.3 每个公开函数名确实出现在调用点文件里 ---
        # math 的 pi / e 是**变量**不是函数，不能要求带括号，否则永远失败。
        # 判据：基线签名里带 `(` 的是函数，要求出现调用括号；不带的是变量，
        # 只要求名字作为独立标识符出现。
        foreach ($fn in $expectedFns[$m]) {
            $isFn = @($expectedSigs[$m] | Where-Object { $_ -match ('^' + [regex]::Escape($fn) + '\s*[\[(]') }).Count -gt 0
            if ($isFn) {
                $called = $src -match ("(?<![\w.])" + [regex]::Escape($fn) + "\s*[\[(]")
            } else {
                $called = $src -match ("(?<![\w.])" + [regex]::Escape($fn) + "(?![\w.])")
            }
            Record 'api' $m $called "covered $m.$fn" 'no call site in api source'
            if (-not $called) { $report += "    NOCALL $fn" }
        }
        $report += ''
    }

    # --- 2.4 已删 API 确实不存在 ---
    #
    # 判定口径很重要：模块是编译期内联的，且**嵌套 import 会把被导入模块的
    # 符号平铺进同一命名空间**（现有语义，实测：import inner（inner 里 import
    # str）之后用户能直接调 toUpperCase）。ds 依赖 str 的 toStr，所以
    # 「import ds 后调 size 失败」这种断言在加入该依赖后就失效了 —— size 会
    # 从 str 泄漏进来。
    #
    # 因此这里改成验**导出表**：模块自己 export 了哪些名字，才是它的公开 API。
    # 删掉的函数既不该在导出表里，也不该能从模块名限定调用到。
    $deletedFromDs = @('size', 'joins', 'iter', 'iterRemove', 'iterContains', 'unpack')
    $dsDump = & $rcc -s --p (Join-Path $stdDir 'ds.rio') --fmt text -exp 2>&1 | Out-String
    foreach ($gone in $deletedFromDs) {
        $present = $dsDump -match ('(?m)^\s+' + [regex]::Escape($gone) + '\s*[\[(]')
        Record 'api' 'ds' (-not $present) "deleted ds.$gone" 'still exported by ds.rio'
    }

    # 已删模块必须干净失败（前端错误，不是链接期 undefined reference）
    Invoke-RioFails 'api' 'prog' 'gone_datatype' `
        "import(datatype=`"datatype`")`nsout(1)`n" 'Cannot find imported module'
    Invoke-RioFails 'api' 'prog' 'gone_lowlevel' `
        "import(lowlevel=`"lowlevel`")`nsout(1)`n" 'Cannot find imported module'
    # 真正不存在的函数：RCC 不报前端错，而是生成外部符号引用，链接期炸。
    # 这不是理想行为（应在前端报错），但它是**当前实际行为**，登记为已知。
    Invoke-RioFails 'api' 'ds' 'gone_ds_joins' `
        "import(ds=`"ds`")`nsout(joins(`"a`", `"b`"))`n" 'undefined reference'

    $reportPath = Join-Path $work 'api_report.txt'
    [System.IO.File]::WriteAllLines($reportPath, $report, $utf8NoBom)
    Write-Host "  [报告] $reportPath" -ForegroundColor DarkGray
}

# ============================================================================
#  第三层：behavior
# ============================================================================

function Invoke-BehaviorLayer {
    Write-Host ''
    Write-Host '================ 第三层：behavior ================' -ForegroundColor Cyan

    $dsImp = "import(ds=`"ds`")`n"
    $stImp = "import(sm=`"str`")`n"
    $allImp = "import(ds=`"ds`", sm=`"str`", m=`"math`", o=`"ops`", io=`"io`", prog=`"prog`")`n"

    # ---------------- ds：正常 ----------------
    Invoke-RioCase 'behavior' 'ds' 'b_ds_isEmpty' ($dsImp + @'
sout(isEmpty([]))
sout(isEmpty([1]))
sout(isEmpty(""))
sout(isEmpty("x"))
sout(isEmpty({}))
sout(isEmpty({"k": 1}))
'@) "true`nfalse`ntrue`nfalse`ntrue`nfalse" @('ds.isEmpty')

    Invoke-RioCase 'behavior' 'ds' 'b_ds_top' ($dsImp + @'
sout(top([1, 2, 3]))
sout(top(["a", "b"]))
sout(top([9]))
'@) "3`nb`n9" @('ds.top')

    # 边界 + 内存：空列表的零值必须是**类型零值**而不是堆上的野指针。
    # P12-3-D 实测：旧实现 `var zero: T = xs[0]` 对 T=str 返回野指针，
    # `"[" + top(es) + "]"` 段错误 exit -1073741819。修法是改成未初始化的 `var zero: T`。
    Invoke-RioCase 'behavior' 'ds' 'b_ds_top_empty_typed' ($dsImp + @'
var ei: list[int] = []
var ef: list[float] = []
var eb: list[bool] = []
var es: list[str] = []
sout(top(ei))
sout(top(ef))
sout(top(eb))
sout("[" + top(es) + "]")
sout(core_str_len(top(es)))
'@) "0`n0.0`nfalse`n[]`n0" @('ds.top')

    Invoke-RioCase 'behavior' 'ds' 'b_ds_copy' ($dsImp + @'
var a: list[int] = [1, 2, 3]
var b: list[int] = copy(a)
b[0] = 99
sout(a[0])
sout(b[0])
sout(core_str_len(b))
'@) "1`n99`n3" @('ds.copy')

    Invoke-RioCase 'behavior' 'ds' 'b_ds_repStr' ($dsImp + @'
sout(repStr("ab", 3))
sout("[" + repStr("x", 0) + "]")
sout("[" + repStr("x", -2) + "]")
sout("[" + repStr("", 5) + "]")
'@) "ababab`n[]`n[]`n[]" @('ds.repStr')

    Invoke-RioCase 'behavior' 'ds' 'b_ds_repList' ($dsImp + @'
var a: list[int] = repList(7, 3)
sout(core_str_len(a))
sout(a[0])
sout(a[2])
var b: list[int] = repList(7, 0)
sout(core_str_len(b))
var c: list[int] = repList(7, -1)
sout(core_str_len(c))
'@) "3`n7`n7`n0`n0" @('ds.repList')

    # 内存：listAppend 返回**新**列表，原列表不能被改（索引写有越界检查，无法原地追加）。
    Invoke-RioCase 'behavior' 'ds' 'b_ds_listAppend' ($dsImp + @'
var a: list[int] = [1, 2]
var b: list[int] = listAppend(a, 3)
sout(core_str_len(a))
sout(core_str_len(b))
sout(b[2])
var e: list[int] = []
var f: list[int] = listAppend(e, 1)
sout(core_str_len(f))
'@) "2`n3`n3`n1" @('ds.listAppend')

    Invoke-RioCase 'behavior' 'ds' 'b_ds_listRemove' ($dsImp + @'
var a: list[int] = [10, 20, 30]
listRemove(a, 1)
sout(core_str_len(a))
sout(a[0])
sout(a[1])
var b: list[int] = [1, 2, 3]
listRemove(b, 0)
sout(b[0])
var c: list[int] = [1, 2, 3]
listRemove(c, 2)
sout(core_str_len(c))
var d: list[int] = [5]
listRemove(d, 0)
sout(core_str_len(d))
'@) "2`n10`n30`n2`n2`n0" @('ds.listRemove')

    # 非法输入：越界下标必须被忽略，不能破坏列表。
    Invoke-RioCase 'behavior' 'ds' 'b_ds_listRemove_oob' ($dsImp + @'
var a: list[int] = [1, 2]
listRemove(a, 9)
listRemove(a, -1)
sout(core_str_len(a))
sout(a[0])
sout(a[1])
'@) "2`n1`n2" @('ds.listRemove')

    Invoke-RioCase 'behavior' 'ds' 'b_ds_dictRemove' ($dsImp + @'
var d: dict = {"aa": 11, "bb": 22, "cc": 33}
dictRemove(d, "bb")
sout(core_str_len(d))
sout(d["aa"])
sout(d["cc"])
'@) "2`n11`n33" @('ds.dictRemove')

    Invoke-RioCase 'behavior' 'ds' 'b_ds_dictRemove_missing' ($dsImp + @'
var d: dict = {"aa": 1}
dictRemove(d, "zz")
sout(core_str_len(d))
sout(d["aa"])
'@) "1`n1" @('ds.dictRemove')

    Invoke-RioCase 'behavior' 'ds' 'b_ds_dictKeys' ($dsImp + @'
var d: dict = {"zz": 1, "aa": 2}
var ks: list[str] = dictKeys(d)
sout(core_str_len(ks))
sout(ks[0])
sout(ks[1])
var e: dict = {}
sout(core_str_len(dictKeys(e)))
'@) "2`nzz`naa`n0" @('ds.dictKeys')

    Invoke-RioCase 'behavior' 'ds' 'b_ds_dictValues' ($dsImp + @'
var d: dict = {"aa": 11, "bb": 22}
var vs: list = dictValues(d)
sout(core_str_len(vs))
sout(vs[0])
sout(vs[1])
var e: dict = {}
sout(core_str_len(dictValues(e)))
'@) "2`n11`n22`n0" @('ds.dictValues')

    Invoke-RioCase 'behavior' 'ds' 'b_ds_contains' ($dsImp + @'
sout(contains([1, 2, 3], 2))
sout(contains([1, 2, 3], 9))
sout(contains(["a", "bb"], "bb"))
sout(contains([], 1))
sout(contains([1.5, 2.5], 2.5))
'@) "true`nfalse`ntrue`nfalse`ntrue" @('ds.contains')

    Invoke-RioCase 'behavior' 'ds' 'b_ds_indexOf' ($dsImp + @'
sout(indexOf([1, 2, 3], 3))
sout(indexOf([1, 2, 3], 9))
sout(indexOf([], 1))
sout(indexOf(["a", "b", "a"], "a"))
'@) "2`n-1`n-1`n0" @('ds.indexOf')

    Invoke-RioCase 'behavior' 'ds' 'b_ds_map' ($dsImp + @'
var a: list[int] = map([1, 2, 3], (v: int) -> { ret v * 2 })
sout(core_str_len(a))
sout(a[0])
sout(a[2])
var e: list[int] = map([], (v: int) -> { ret v })
sout(core_str_len(e))
'@) "3`n2`n6`n0" @('ds.map')

    Invoke-RioCase 'behavior' 'ds' 'b_ds_filter' ($dsImp + @'
var a: list[int] = filter([1, 2, 3, 4], (v: int) -> { ret v % 2 == 0 })
sout(core_str_len(a))
sout(a[0])
sout(a[1])
var e: list[int] = filter([], (v: int) -> { ret true })
sout(core_str_len(e))
var n: list[int] = filter([1, 3], (v: int) -> { ret false })
sout(core_str_len(n))
'@) "2`n2`n4`n0`n0" @('ds.filter')

    Invoke-RioCase 'behavior' 'ds' 'b_ds_reverse' ($dsImp + @'
var a: list[int] = reverse([1, 2, 3])
sout(a[0])
sout(a[1])
sout(a[2])
var e: list[int] = reverse([])
sout(core_str_len(e))
var s: list[int] = reverse([7])
sout(s[0])
'@) "3`n2`n1`n0`n7" @('ds.reverse')

    Invoke-RioCase 'behavior' 'ds' 'b_ds_slice' ($dsImp + @'
var a: list[int] = slice([1, 2, 3, 4], 1, 3)
sout(core_str_len(a))
sout(a[0])
sout(a[1])
var b: list[int] = slice([1, 2, 3], -1, 99)
sout(core_str_len(b))
var c: list[int] = slice([1, 2, 3], 2, 1)
sout(core_str_len(c))
var d: list[int] = slice([], 0, 1)
sout(core_str_len(d))
'@) "2`n2`n3`n3`n0`n0" @('ds.slice')

    Invoke-RioCase 'behavior' 'ds' 'b_ds_concat' ($dsImp + @'
var a: list[int] = concat([1, 2], [3])
sout(core_str_len(a))
sout(a[0])
sout(a[2])
var b: list[int] = concat([], [1])
sout(core_str_len(b))
var c: list[int] = concat([1], [])
sout(core_str_len(c))
var d: list[int] = concat([], [])
sout(core_str_len(d))
'@) "3`n1`n3`n1`n1`n0" @('ds.concat')

    Invoke-RioCase 'behavior' 'ds' 'b_ds_sum' ($dsImp + @'
sout(sum([1, 2, 3]))
sout(sum([]))
sout(sum([-5, 5]))
sout(sum([42]))
'@) "6`n0`n0`n42" @('ds.sum')

    Invoke-RioCase 'behavior' 'ds' 'b_ds_sort' ($dsImp + @'
var a: list[int] = sort([3, 1, 2])
sout(a[0])
sout(a[1])
sout(a[2])
var b: list[int] = sort([3, 1, 2], true)
sout(b[0])
sout(b[2])
var e: list[int] = sort([])
sout(core_str_len(e))
var s: list[int] = sort([7])
sout(s[0])
'@) "1`n2`n3`n3`n1`n0`n7" @('ds.sort')

    Invoke-RioCase 'behavior' 'ds' 'b_ds_sort_typed' ($dsImp + @'
var a: list[str] = sort(["cc", "aa", "bb"])
sout(a[0])
sout(a[2])
var b: list[float] = sort([3.5, 1.25, 2.0])
sout(b[0])
sout(b[2])
'@) "aa`ncc`n1.25`n3.5" @('ds.sort')

    Invoke-RioCase 'behavior' 'ds' 'b_ds_join' ($dsImp + @'
sout(join([1, 2, 3], "-"))
sout(join(["a", "b"]))
sout("[" + join([], "-") + "]")
sout(join([42], "-"))
sout(join(["x"], "-"))
'@) "1-2-3`nab`n[]`n42`nx" @('ds.join')

    # dumpToStr：容器转字符串表示（list / dict 两个泛型重载）。
    # 只覆盖**标量元素**：`toStr` 目前没有 list/dict 重载（它只处理
    # int/float/bool/str/char），所以 `dumpToStr([[1,2]])` 这种嵌套 list
    # 会因 `toStr(list[int])` 无匹配而编译失败 —— 那是 toStr 的功能缺口，
    # 不是 dumpToStr 的缺陷。空字面量同理（推不出 T）。
    Invoke-RioCase 'behavior' 'ds' 'b_ds_dumpToStr' ($allImp + @'
sout(dumpToStr([1, 2, 3]))
sout(dumpToStr(["a", "b"]))
sout(dumpToStr({"1": 2.5}))
sout(dumpToStr({"k": 1}))
'@) "[1, 2, 3]`n[a, b]`n{1: 2.5}`n{k: 1}" @('ds.dumpToStr')

    # toFloat：bool / int 两个重载。
    Invoke-RioCase 'behavior' 'ds' 'b_ds_toFloat' ($allImp + @'
sout(toFloat(true))
sout(toFloat(false))
sout(toFloat(3))
sout(toFloat(0))
sout(toFloat(true) + toFloat(2))
'@) "1.0`n0.0`n3.0`n0.0`n3.0" @('ds.toFloat')

    Invoke-RioCase 'behavior' 'ds' 'b_ds_repeat' ($dsImp + @'
repeat(3, (i: int) -> { sout(i) })
repeat(0, (i: int) -> { sout("never") })
'@) "0`n1`n2" @('ds.repeat')

    Invoke-RioCase 'behavior' 'ds' 'b_ds_forInRange' ($dsImp + @'
forInRange(0, 3, (i: int) -> { sout(i) })
forInRange(0, 6, (i: int) -> { sout(i) }, 2)
'@) "0`n1`n2`n0`n2`n4" @('ds.forInRange')

    Invoke-RioCase 'behavior' 'ds' 'b_ds_forInRange_edges' ($dsImp + @'
forInRange(0, 5, (i: int) -> { sout(i) }, 0)
sout("zero-step-ok")
forInRange(3, 0, (i: int) -> { sout(i) }, -1)
forInRange(2, 2, (i: int) -> { sout("never") })
sout("empty-ok")
'@) "zero-step-ok`n3`n2`n1`nempty-ok" @('ds.forInRange')

    # ---------------- str：正常 ----------------
    Invoke-RioCase 'behavior' 'str' 'b_str_intToStr' ($stImp + @'
sout(intToStr(0))
sout(intToStr(7))
sout(intToStr(-45))
sout(intToStr(12345))
'@) "0`n7`n-45`n12345" @('str.intToStr')

    Invoke-RioCase 'behavior' 'str' 'b_str_floatToStr' ($stImp + @'
sout(floatToStr(3.14))
sout(floatToStr(2.5))
sout(floatToStr(-1.5))
sout(floatToStr(10.0))
'@) "3.14`n2.5`n-1.5`n10.0" @('str.floatToStr')

    Invoke-RioCase 'behavior' 'str' 'b_str_toStr' ($stImp + @'
sout(toStr(42))
sout(toStr(3.14))
sout(toStr(true))
sout(toStr(false))
sout(toStr("raw"))
sout(toStr('A'))
'@) "42`n3.14`ntrue`nfalse`nraw`n65" @('str.toStr')

    Invoke-RioCase 'behavior' 'str' 'b_str_toChar_charToInt' ($stImp + @'
sout(toChar("A"))
sout(toChar(65))
sout(toChar(""))
sout(charToInt(65))
sout(charToInt('A'))
sout(charToInt("A"))
sout(intToChar(66))
sout(intToChar(65))
'@) "65`n65`n0`n65`n65`n65`nB`nA" @('str.toChar', 'str.charToInt', 'str.intToChar')

    # 非法输入：charToInt 对多字符字符串必须报错（旧实现静默返回首字节）。
    $multiSrc = $stImp + "sout(charToInt(`"true`"))`n"
    $multiFile = Join-Path $work 'b_str_charToInt_multichar.rio'
    $multiExe = Join-Path $work 'b_str_charToInt_multichar.exe'
    [System.IO.File]::WriteAllText($multiFile, $multiSrc, $utf8NoBom)
    if (Test-Path $multiExe) { Remove-Item $multiExe -Force }
    $multiLog = & $rcc -build --path $multiFile --output $multiExe 2>&1 | Out-String
    $script:covered['str.charToInt'] = $true
    if ($LASTEXITCODE -eq 0 -and (Test-Path $multiExe)) {
        & $multiExe 2>&1 | Out-Null
        Record 'behavior' 'str' ($LASTEXITCODE -ne 0) 'b_str_charToInt_multichar' `
            "expected runtime failure, exit=$LASTEXITCODE"
    } else {
        Record 'behavior' 'str' $true 'b_str_charToInt_multichar' 'compile-time rejection'
    }

    Invoke-RioCase 'behavior' 'str' 'b_str_subStr' ($stImp + @'
sout("[" + subStr("hello world", 6, 5) + "]")
sout("[" + subStr("hello", 1) + "]")
sout("[" + subStr("hello", 0, 0) + "]")
sout("[" + subStr("hello", 99, 3) + "]")
sout("[" + subStr("", 0, 3) + "]")
sout("[" + subStr("hello", -1, 2) + "]")
'@) "[world]`n[ello]`n[]`n[]`n[]`n[he]" @('str.subStr')

    Invoke-RioCase 'behavior' 'str' 'b_str_find' ($stImp + @'
sout(find("hello world", "world"))
sout(find("hello", "xyz"))
sout(find("aaa", "a"))
sout(find("hello", "l", 3))
sout(find("", "a"))
sout(find("abc", ""))
'@) "6`n-1`n0`n3`n-1`n0" @('str.find')

    Invoke-RioCase 'behavior' 'str' 'b_str_case' ($stImp + @'
sout(toUpperCase("hello"))
sout(toLowerCase("WORLD"))
sout(toUpperCase("a1b2"))
sout("[" + toUpperCase("") + "]")
'@) "HELLO`nworld`nA1B2`n[]" @('str.toUpperCase', 'str.toLowerCase')

    Invoke-RioCase 'behavior' 'str' 'b_str_replace' ($stImp + @'
sout(replace("aXbXc", "X", "-"))
sout(replace("aXbXc", "X", "-", true))
sout(replace("hello", "z", "y"))
sout("[" + replace("", "a", "b") + "]")
sout(replace("aaa", "a", "b", true))
'@) "a-b-c`na-bXc`nhello`n[]`nbaa" @('str.replace')

    Invoke-RioCase 'behavior' 'str' 'b_str_split' ($stImp + @'
var p: list[str] = split("a,b,c", ",")
sout(core_str_len(p))
sout(p[0])
sout(p[2])
var q: list[str] = split("a b", " ")
sout(core_str_len(q))
var k: list[str] = split("a,b", ",", true)
sout(core_str_len(k))
sout(k[1])
var e: list[str] = split("", ",")
sout(core_str_len(e))
'@) "3`na`nc`n2`n3`n,`n0" @('str.split')

    Invoke-RioCase 'behavior' 'str' 'b_str_chunk' ($stImp + @'
var c: list[str] = chunk("abcdef", 2)
sout(core_str_len(c))
sout(c[0])
sout(c[2])
var g: list[str] = chunk("abcdefg", 2)
sout(core_str_len(g))
sout(g[3])
var e: list[str] = chunk("", 3)
sout(core_str_len(e))
'@) "3`nab`nef`n4`ng`n0" @('str.chunk')

    # 用 [] 包住结果：$actual 末尾会被 .Trim() 吃掉，裸打印分不清
    # 「空串」和「只有空格的串」，也分不清 trim(...,false) 的尾随空格。
    Invoke-RioCase 'behavior' 'str' 'b_str_trim' ($stImp + @'
sout("[" + trim("  hi  ") + "]")
sout("[" + trim("xxhixx", "x") + "]")
sout("[" + trim("  hi  ", " ", true, false) + "]")
sout("[" + trim("  hi  ", " ", false, true) + "]")
sout("[" + trim("xxhixx", "") + "]")
sout("[" + trim("") + "]")
'@) "[hi]`n[hi]`n[hi  ]`n[  hi]`n[xxhixx]`n[]" @('str.trim')

    Invoke-RioCase 'behavior' 'str' 'b_str_size' ($stImp + @'
sout(size("hello"))
sout(size([1, 2, 3]))
sout(size({"k": 1}))
sout(size(""))
sout(size([]))
sout(size({}))
'@) "5`n3`n1`n0`n0`n0" @('str.size')

    # UTF-8：按字节切，多字节字符不被切坏（旧版逐字符 setType 会拆坏）
    Invoke-RioCase 'behavior' 'str' 'b_str_utf8' ($stImp + @'
sout(subStr("中文测试", 0, 6))
sout(subStr("中文测试", 6, 6))
sout(core_str_len("中文测试"))
sout(toUpperCase("abc"))
'@) "中文`n测试`n12`nABC" @('str.subStr')

    # ---------------- math ----------------
    Invoke-RioCase 'behavior' 'math' 'b_math_consts' (@'
import(m="math")
sout(pi)
sout(e)
'@) "3.141593`n2.718281" @('math.pi', 'math.e')

    Invoke-RioCase 'behavior' 'math' 'b_math_maxmin' (@'
import(m="math")
sout(max(5, 3))
sout(max(1.5, 2.5))
sout(min(3, 4))
sout(min(1.5, 2.5))
sout(max(-1, -9))
'@) "5`n2.5`n3`n1.5`n-1" @('math.max', 'math.min')

    Invoke-RioCase 'behavior' 'math' 'b_math_pow' (@'
import(m="math")
sout(pow(2, 10))
sout(pow(1.5, 3))
sout(pow(2, 0))
sout(pow(5, 1))
'@) "1024`n3.375`n1`n5" @('math.pow')

    Invoke-RioCase 'behavior' 'math' 'b_math_abs_opp' (@'
import(m="math")
sout(abs(-7))
sout(abs(7))
sout(abs(-1.5))
sout(opp(3))
sout(opp(-2.5))
sout(opp(0))
'@) "7`n7`n1.5`n-3`n2.5`n0" @('math.abs', 'math.opp')

    # ---------------- ops ----------------
    Invoke-RioCase 'behavior' 'ops' 'b_ops_ternary' (@'
import(o="ops")
sout(ternary(true, "a", "b"))
sout(ternary(false, "a", "b"))
sout(ternary(1 < 2, 10, 20))
sout(ternaryLazy(true, () -> { ret 1 }, () -> { ret 2 }))
sout(ternaryLazy(false, () -> { ret 1 }, () -> { ret 2 }))
'@) "a`nb`n10`n1`n2" @('ops.ternary', 'ops.ternaryLazy')

    Invoke-RioCase 'behavior' 'ops' 'b_ops_or' (@'
import(o="ops")
sout(orVal(null, 10))
sout(orVal(5, 10))
sout(orLazy(null, () -> { ret 7 }))
sout(orLazy(3, () -> { ret 7 }))
'@) "10`n5`n7`n3" @('ops.orVal', 'ops.orLazy')

    # ---------------- io ----------------
    Invoke-RioCase 'behavior' 'io' 'b_io_sin' (@'
import(io="io")
var s: str = sin()
sout(core_str_len(s) >= 0)
sout("sin-ok")
'@) "true`nsin-ok" @('io.sin')

    # input 从文件重定向读入（PowerShell 不支持 <，走 cmd /c）
    $inFile = Join-Path $work 'b_io_stdin.txt'
    [System.IO.File]::WriteAllText($inFile, "hello world`r`n", $utf8NoBom)
    $inSrc = @'
import(io="io")
var s: str = input("")
sout(core_str_len(s))
sout(s)
'@
    $inRio = Join-Path $work 'b_io_input.rio'
    $inExe = Join-Path $work 'b_io_input.exe'
    [System.IO.File]::WriteAllText($inRio, $inSrc, $utf8NoBom)
    if (Test-Path $inExe) { Remove-Item $inExe -Force }
    $inLog = & $rcc -build --path $inRio --output $inExe 2>&1 | Out-String
    $script:covered['io.input'] = $true
    if ($LASTEXITCODE -ne 0 -or -not (Test-Path $inExe)) {
        Record 'behavior' 'io' $false 'b_io_input' ($inLog -replace '\s+', ' ')
    } else {
        $out = cmd /c "`"$inExe`" < `"$inFile`"" 2>&1 | Out-String
        $out = ($out -replace "`r`n", "`n").Trim()
        Record 'behavior' 'io' ($out -eq "13`nhello world") 'b_io_input' `
            "expected=[13`nhello world] actual=[$out]"
    }

    # ---------------- prog ----------------
    Invoke-RioCase 'behavior' 'prog' 'b_prog_entry_breakpoint' (@'
import(prog="prog")
breakpoint()
entry(0)
entry(0, 1, 2)
sout("alive")
'@) "alive" @('prog.breakpoint', 'prog.entry')

    # ---------------- 跨模块 ----------------
    Invoke-RioCase 'behavior' 'ds' 'b_combo_all_modules' ($allImp + @'
var xs: list[int] = sort([3, 1, 2])
sout(join(xs, "-"))
sout(toUpperCase(ternary(size(xs) == 3, "yes", "no")))
sout(intToStr(max(1, 2)))
var parts: list[str] = split("a,b,c", ",")
sout(join(parts, "+"))
sout(contains(parts, "b"))
sout(pow(sum(xs), 2))
breakpoint()
'@) "1-2-3`nYES`n2`na+b+c`ntrue`n36" @('ds.sort', 'ds.join', 'str.size', 'str.split', 'str.toUpperCase', 'str.intToStr', 'math.max', 'math.pow', 'ds.sum', 'ds.contains', 'ops.ternary', 'prog.breakpoint')

    # ---------------- 覆盖率自检 ----------------
    Write-Host ''
    Write-Host '  --- 行为层覆盖率自检（每个公开函数至少 1 个用例）---' -ForegroundColor DarkCyan
    foreach ($m in $modules) {
        foreach ($fn in $expectedFns[$m]) {
            $key = "$m.$fn"
            $hit = $script:covered[$key]
            Record 'behavior' $m $hit "coverage $key" 'no behavior case covers this function'
        }
    }
}

# ============================================================================
#  执行 + 汇总
# ============================================================================

$runCompile = ($Layer -eq 'all' -or $Layer -eq 'compile')
$runApi = ($Layer -eq 'all' -or $Layer -eq 'api')
$runBehavior = ($Layer -eq 'all' -or $Layer -eq 'behavior')

if ($runCompile) { Invoke-CompileLayer }
if ($runApi) { Invoke-ApiLayer }
if ($runBehavior) { Invoke-BehaviorLayer }

Write-Host ''
Write-Host '================================================================' -ForegroundColor Cyan
Write-Host '  标准库三层验收总表（P12-3-D / §5.6）' -ForegroundColor Cyan
Write-Host '================================================================' -ForegroundColor Cyan

$hdr = "  {0,-6} {1,-16} {2,-16} {3,-16} {4}" -f '模块', 'compile', 'api', 'behavior', '结论'
Write-Host $hdr
Write-Host ('  ' + ('-' * 76))

$anyFail = $false
$colTotals = @{ compile = @(0, 0); api = @(0, 0); behavior = @(0, 0) }

foreach ($m in $modules) {
    $cells = @()
    $moduleOk = $true
    foreach ($l in @('compile', 'api', 'behavior')) {
        $p = $script:layerPass[$l][$m]
        $f = $script:layerFail[$l][$m]
        $isRun = ($Layer -eq 'all' -or $Layer -eq $l)
        if (-not $isRun) {
            $cells += '-'
            continue
        }
        $colTotals[$l][0] += $p
        $colTotals[$l][1] += $f
        if ($f -gt 0) { $moduleOk = $false }
        $cells += $(if ($f -eq 0) { "PASS $p/$($p + $f)" } else { "FAIL $p/$($p + $f)" })
    }
    if (-not $moduleOk) { $anyFail = $true }
    $verdict = if ($moduleOk) { '完成' } else { '未完成' }
    $color = if ($moduleOk) { 'Green' } else { 'Red' }
    Write-Host ("  {0,-6} {1,-16} {2,-16} {3,-16} {4}" -f $m, $cells[0], $cells[1], $cells[2], $verdict) -ForegroundColor $color
}

Write-Host ('  ' + ('-' * 76))
$totP = $colTotals.compile[0] + $colTotals.api[0] + $colTotals.behavior[0]
$totF = $colTotals.compile[1] + $colTotals.api[1] + $colTotals.behavior[1]
Write-Host ("  合计：通过 {0}，失败 {1}" -f $totP, $totF) -ForegroundColor $(if ($totF -eq 0) { 'Green' } else { 'Red' })

if ($anyFail) {
    Write-Host ''
    Write-Host '失败清单：' -ForegroundColor Red
    $script:failures | ForEach-Object { Write-Host "  - $_" -ForegroundColor Red }
    Write-Host ''
    Write-Host 'P12-3-D 标准库三层验收未通过' -ForegroundColor Red
    exit 1
}

Write-Host ''
Write-Host 'P12-3-D 标准库三层验收通过' -ForegroundColor Green
exit 0

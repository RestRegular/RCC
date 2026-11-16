# ============================================================================
#  test_p12_std_skeleton.ps1 —— P12-3-A std 层骨架验收
#
#  验的是「lib/std/ 是唯一真相、release/Lib/builtin/ 是构建产物」这套机制
#  本身，不是各模块的功能细节（那些在 test_std.ps1，P12-3-D 才建）。
#
#  检查项：
#    1. lib/std/ 下的每个模块都能被 -emit-lib 编译（语法没漂）
#    2. CMake 拷贝确实发生：release/Lib/builtin/ 里内容与 lib/std/ 逐字节一致
#    3. release/Lib/builtin/ 的产物没有被 git 跟踪（防止再次签进仓库）
#    4. 每个模块都能被用户代码 import 并调用（真实使用路径）
#    5. math 模块的 UTF-8 无 BOM（旧版是 GB18030，会让词法分析器炸）
#
#  为什么值得单独测：stdlib 是纯 Rio 源码，编译器不编译它就发现不了它的
#  语法错误。CMake 只做拷贝，一旦 lib/std/ 里写错了，要到用户 import 时才炸。
# ============================================================================

$ErrorActionPreference = 'Stop'
$repo = Split-Path -Parent $PSScriptRoot
$rcc = Join-Path $repo 'release\RCC.exe'
$stdDir = Join-Path $repo 'lib\std'
$outDir = Join-Path $repo 'release\Lib\builtin'
$work = Join-Path $repo 'build\p12_std_check'
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

Write-Host ''
Write-Host '=== 1. lib/std 源码可独立编译（-emit-lib） ==='

$modules = Get-ChildItem $stdDir -Filter *.rio | Sort-Object Name
if ($modules.Count -eq 0) { throw "No modules found in $stdDir" }

foreach ($m in $modules) {
    $out = Join-Path $work ($m.BaseName + '.o')
    $log = & $rcc -build -emit-lib --path $m.FullName --output $out 2>&1 | Out-String
    $ok = ($LASTEXITCODE -eq 0) -and (Test-Path $out)
    Check "compile $($m.Name)" $ok ($log.Trim())
}

Write-Host ''
Write-Host '=== 2. CMake 拷贝一致性（lib/std -> release/Lib/builtin） ==='

foreach ($m in $modules) {
    $dst = Join-Path $outDir $m.Name
    if (-not (Test-Path $dst)) {
        Check "copied $($m.Name)" $false "missing in release/Lib/builtin/"
        continue
    }
    $srcHash = (Get-FileHash $m.FullName -Algorithm SHA256).Hash
    $dstHash = (Get-FileHash $dst -Algorithm SHA256).Hash
    Check "copied $($m.Name)" ($srcHash -eq $dstHash) "content drift: source and build copy differ"
}

Write-Host ''
Write-Host '=== 3. 构建产物未被 git 跟踪 ==='

foreach ($name in @('math.rio', 'ops.rio', 'prog.rio', 'str.rio', 'io.rio')) {
    Push-Location $repo
    $tracked = (& git ls-files "release/Lib/builtin/$name" 2>&1 | Out-String).Trim()
    Pop-Location
    Check "untracked $name" ([string]::IsNullOrEmpty($tracked)) "still tracked: release/Lib/builtin/$name"
}

Write-Host ''
Write-Host '=== 4. UTF-8 无 BOM（旧 math.rio 是 GB18030） ==='

foreach ($m in $modules) {
    $bytes = [System.IO.File]::ReadAllBytes($m.FullName)
    $hasBom = ($bytes.Length -ge 3 -and $bytes[0] -eq 0xEF -and $bytes[1] -eq 0xBB -and $bytes[2] -eq 0xBF)
    # GB18030 的双字节序列首字节 >= 0x81，UTF-8 中文首字节是 0xE4-0xE9。
    # 这里做的是"能被严格 UTF-8 解码"这个更强也更有意义的检查。
    $validUtf8 = $true
    try {
        $strict = New-Object System.Text.UTF8Encoding($false, $true)
        [void]$strict.GetString($bytes)
    } catch {
        $validUtf8 = $false
    }
    Check "utf8 $($m.Name)" ((-not $hasBom) -and $validUtf8) "BOM=$hasBom validUtf8=$validUtf8"
}

Write-Host ''
Write-Host '=== 5. 真实 import 使用路径 ==='

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
    # 换行归一：RCC 输出 "\n"，PowerShell 管道把它变成 CRLF。不归一的话
    # 每个多行用例都会因为不可见字符失败。
    $actual = $actual -replace "`r`n", "`n"
    $expected = $expected -replace "`r`n", "`n"
    Check $name ($actual -eq $expected) "expected=[$expected] actual=[$actual]"
}

Invoke-RioCase 'use_math' @'
import(math="math")
sout(max(5, 3))
sout(min(1.5, 2.5))
sout(pow(2, 10))
sout(abs(-7))
sout(opp(3))
'@ "5`n1.5`n1024`n7`n-3"

Invoke-RioCase 'use_math_qualified' @'
import(math="math")
sout(math.max(5, 3))
sout(math.pow(1.5, 3))
'@ "5`n3.375"

Invoke-RioCase 'use_ops' @'
import(ops="ops")
sout(ternary(true, "a", "b"))
sout(orVal(null, 10))
sout(orLazy(null, () -> { ret 7 }))
'@ "a`n10`n7"

Invoke-RioCase 'use_prog' @'
import(prog="prog")
breakpoint()
sout("alive")
'@ "alive"

# max(1,2)=2、min(3,4)=3，所以 2>3 为假 —— 期望 "no"。
Invoke-RioCase 'use_all_three' @'
import(math="math", ops="ops", prog="prog")
sout(ternary(max(1, 2) > min(3, 4), "yes", "no"))
breakpoint()
'@ "no"

Write-Host ''
Write-Host '=== 6. 旧模块确实已不在（P12-3-A 删除清单） ==='

foreach ($gone in @('_pure_builtin.rio', 'lowlevel.rio')) {
    $p = Join-Path $outDir $gone
    Check "removed $gone" (-not (Test-Path $p)) "still present: $p"
}

# lowlevel 的 ri 依赖 *args 收集（G1b 待设计），整模块已按用户裁决不保留。
# 但要确认 import lowlevel 现在给出的是"找不到模块"而不是诡异错误。
# 这里预期 RCC 失败：stderr 会被 PowerShell 包成 NativeCommandError，
# 必须用 2>&1 + ErrorAction 抑制，否则脚本会以非 0 退出。
$ll = Join-Path $work 'use_lowlevel_gone.rio'
[System.IO.File]::WriteAllText($ll, "import(lowlevel=`"lowlevel`")`nsout(1)`n", $utf8NoBom)
# $ErrorActionPreference='Stop' 会把原生命令的 stderr 变成终止性错误，
# 而这里**预期**失败，必须临时放宽。
$savedEap = $ErrorActionPreference
$ErrorActionPreference = 'Continue'
$llOut = & $rcc -build --path $ll --output (Join-Path $work 'use_lowlevel_gone.exe') 2>&1 | Out-String
$ErrorActionPreference = $savedEap
$llOk = ($llOut -match 'Cannot find imported module')
Check 'lowlevel_import_fails_cleanly' $llOk "expected 'Cannot find imported module', got: $($llOut.Trim())"

Write-Host ''
if ($fail -eq 0) {
    Write-Host "结果：通过 $pass，失败 0" -ForegroundColor Green
    Write-Host 'P12-3-A std 层骨架验收通过' -ForegroundColor Green
    exit 0
} else {
    Write-Host "结果：通过 $pass，失败 $fail" -ForegroundColor Red
    Write-Host "失败用例：$($failures -join ', ')" -ForegroundColor Red
    exit 1
}

# ============================================================================
#  RCC legacy 测试总览
#
#  逐个编译 tests/test_*/test.rio，运行后与同目录 expected.txt 逐字节比对。
#  用于快速看清「当前 17 个 legacy 测试过了几个」。
#
#  用法：powershell -NoProfile -ExecutionPolicy Bypass -File test_scripts/test_legacy_all.ps1
# ============================================================================

$ErrorActionPreference = 'Stop'

$repo = Split-Path -Parent $PSScriptRoot
$rcc = Join-Path $repo 'release\RCC.exe'
$work = Join-Path $repo 'build\legacy_verify'

if (-not (Test-Path $rcc)) {
    Write-Host "找不到编译器：$rcc" -ForegroundColor Red
    exit 1
}

if (Test-Path $work) { Remove-Item $work -Recurse -Force }
New-Item -ItemType Directory -Force -Path $work | Out-Null

$dirs = Get-ChildItem (Join-Path $repo 'tests') -Directory |
        Where-Object { $_.Name -match '^test_\d+_' } |
        Sort-Object { [int]($_.Name -replace '^test_(\d+)_.*$', '$1') }

$pass = 0
$fail = 0
$failed = @()

foreach ($dir in $dirs) {
    $name = $dir.Name
    $rio = Join-Path $dir.FullName 'test.rio'
    $exp = Join-Path $dir.FullName 'expected.txt'
    if (-not (Test-Path $rio)) { continue }

    $out = Join-Path $work $name
    # PowerShell 5.1 在 $ErrorActionPreference='Stop' 下会把原生命令的 stderr
    # 当成终止错误，而 RCC 的编译失败信息正是写 stderr 的。
    # 调用期间临时放宽，让失败信息能正常进入 $buildOut 走统一判定。
    $savedEap = $ErrorActionPreference
    $ErrorActionPreference = 'Continue'
    $buildOut = & $rcc -build --path $rio --output $out 2>&1 | Out-String
    $ErrorActionPreference = $savedEap

    if ($buildOut -match '(?m)^\s*Error|ParserError|CodeGenError|LinkError') {
        $first = ($buildOut -split "`n" | Where-Object { $_ -match 'Error' } | Select-Object -First 1)
        Write-Host ("  [失败] {0} —— 编译失败" -f $name) -ForegroundColor Red
        Write-Host ("         {0}" -f $first.Trim()) -ForegroundColor DarkGray
        $fail++; $failed += $name
        continue
    }
    if (-not (Test-Path "$out.exe")) {
        $first = ($buildOut -split "`n" | Where-Object { $_ -match '\S' } | Select-Object -First 1)
        Write-Host ("  [失败] {0} —— 没有产出 exe" -f $name) -ForegroundColor Red
        if ($first) { Write-Host ("         {0}" -f $first.Trim()) -ForegroundColor DarkGray }
        $fail++; $failed += $name
        continue
    }

    $actual = (& "$out.exe" 2>&1 | Out-String).Trim() -replace "`r`n", "`n"
    if (-not (Test-Path $exp)) {
        Write-Host ("  [无期望] {0} => {1}" -f $name, ($actual -replace "`n", ' / ')) -ForegroundColor Yellow
        continue
    }
    $expect = (Get-Content $exp -Raw).Trim() -replace "`r`n", "`n"

    if ($actual -eq $expect) {
        Write-Host ("  [通过] {0}" -f $name) -ForegroundColor Green
        $pass++
    } else {
        Write-Host ("  [失败] {0} —— 输出不符" -f $name) -ForegroundColor Red
        Write-Host ("         期望：{0}" -f ($expect -replace "`n", ' / ')) -ForegroundColor DarkGray
        Write-Host ("         实际：{0}" -f ($actual -replace "`n", ' / ')) -ForegroundColor DarkGray
        $fail++; $failed += $name
    }
}

Write-Host ""
Write-Host ("结果：通过 {0}，失败 {1}" -f $pass, $fail)
if ($fail -gt 0) {
    Write-Host ("失败用例：{0}" -f ($failed -join ', ')) -ForegroundColor Yellow
}

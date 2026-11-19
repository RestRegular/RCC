# ============================================================================
#  run-all.ps1 —— 跑测试集（从旧的 test_scripts/ 运行器迁移而来）
#
#  旧运行器靠一张**硬编码套件清单**（25 行 @{Name=...}）来调度，
#  每加一个套件就要改运行器 —— 这是旧体系最大的维护负担。
#  新契约下每个测试集自带 manifest.json 与 execute.ps1，
#  运行器只需**扫描目录**即可，不再需要清单。
#
#  用法：
#    powershell -NoProfile -ExecutionPolicy Bypass -File tests\user\run-all.ps1
#    powershell ... run-all.ps1 -Area agent          # 只跑 agent
#    powershell ... run-all.ps1 -Area cases          # 只跑用户复现用例
#    powershell ... run-all.ps1 -Filter p13          # 只跑名字匹配的
#    powershell ... run-all.ps1 -StopOnFail          # 首个失败就停
#    powershell ... run-all.ps1 -KeepLog             # 保留每套完整日志
#
#  退出码：全绿 0，任一失败 1。
# ============================================================================

param(
    [ValidateSet('agent', 'cases', 'all')][string]$Area = 'all',
    [string]$Filter = '',
    [switch]$StopOnFail,
    [switch]$KeepLog
)

$ErrorActionPreference = 'Continue'
$here = $PSScriptRoot
$testsRoot = Split-Path $here -Parent          # tests/
$repo = Split-Path $testsRoot -Parent          # 仓库根
$rcc = Join-Path $repo 'release\RCC.exe'
$logDir = Join-Path $repo 'build\test_logs'
$utf8NoBom = New-Object System.Text.UTF8Encoding($false)

if (-not (Test-Path $rcc)) {
    Write-Host "找不到编译器：$rcc" -ForegroundColor Red
    Write-Host "请先构建：mingw32-make -C build -j 10" -ForegroundColor Yellow
    exit 1
}

if ($KeepLog) {
    if (Test-Path $logDir) { Remove-Item $logDir -Recurse -Force }
    New-Item -ItemType Directory -Force -Path $logDir | Out-Null
}

# ---------------------------------------------------------------------------
#  扫描测试集：只有「有 execute.ps1」的目录才算
#  - 下划线开头的目录是内部共享目录（如 _shared），跳过
#  - tests/user 是用户自由区，永远不参与批量运行
# ---------------------------------------------------------------------------
$areas = switch ($Area) {
    'agent' { @('agent') }
    'cases' { @('cases') }
    default { @('agent', 'cases') }
}

$suites = @()
foreach ($a in $areas) {
    $dir = Join-Path $testsRoot $a
    if (-not (Test-Path $dir)) { continue }
    Get-ChildItem $dir -Directory |
        Where-Object { -not $_.Name.StartsWith('_') } |
        Where-Object { Test-Path (Join-Path $_.FullName 'execute.ps1') } |
        Sort-Object Name |
        ForEach-Object {
            $suites += [pscustomobject]@{
                Name  = $_.Name
                Area  = $a
                Path  = (Join-Path $_.FullName 'execute.ps1')
                Dir   = $_.FullName
            }
        }
}

if ($Filter) { $suites = @($suites | Where-Object { $_.Name -match $Filter }) }

if ($suites.Count -eq 0) {
    Write-Host "没有找到测试集（Area=$Area, Filter='$Filter'）" -ForegroundColor Yellow
    exit 1
}

Write-Host ''
Write-Host '================================================================' -ForegroundColor Cyan
Write-Host '  RCC 测试集' -ForegroundColor Cyan
Write-Host '================================================================' -ForegroundColor Cyan
Write-Host ("  区域：{0}   测试集：{1} 个" -f $Area, $suites.Count)
Write-Host ''

# ---------------------------------------------------------------------------
#  执行
# ---------------------------------------------------------------------------
$results = @()
$idx = 0

foreach ($s in $suites) {
    $idx++
    Write-Host ("  [{0}/{1}] {2}/{3}" -f $idx, $suites.Count, $s.Area, $s.Name) -ForegroundColor Gray

    $sw = [System.Diagnostics.Stopwatch]::StartNew()
    $out = & powershell -NoProfile -ExecutionPolicy Bypass -File $s.Path 2>&1 | Out-String
    $code = $LASTEXITCODE
    $sw.Stop()

    if ($KeepLog) {
        [System.IO.File]::WriteAllText((Join-Path $logDir "$($s.Area)__$($s.Name).log"), $out, $utf8NoBom)
    }

    # 各套件汇总措辞不完全统一，按宽容差匹配：
    #   「结果：通过 N，失败 M」/「总计 N 项：通过 N，失败 M」/「合计：通过 N，失败 M」
    $pass = 0; $fail = 0
    $m = [regex]::Match($out, '通过\s*(\d+)\s*[，,]\s*失败\s*(\d+)')
    if ($m.Success) { $pass = [int]$m.Groups[1].Value; $fail = [int]$m.Groups[2].Value }

    $ok = ($code -eq 0)
    $tag = if ($ok) { 'PASS' } else { 'FAIL' }
    $color = if ($ok) { 'Green' } else { 'Red' }
    Write-Host ("           {0}  exit={1}  {2:N1}s  通过 {3}，失败 {4}" -f $tag, $code, $sw.Elapsed.TotalSeconds, $pass, $fail) -ForegroundColor $color

    if (-not $ok) {
        $tail = ($out -split "`n" | Where-Object { $_ -match '\[失败\]' } | Select-Object -First 6)
        foreach ($t in $tail) { Write-Host ("             " + $t.TrimEnd()) -ForegroundColor DarkRed }
    }

    $results += [pscustomobject]@{ Name = $s.Name; Area = $s.Area; Exit = $code; Pass = $pass; Fail = $fail }

    if (-not $ok -and $StopOnFail) {
        Write-Host ''
        Write-Host '  -StopOnFail：中止后续测试集' -ForegroundColor Yellow
        break
    }
}

# ---------------------------------------------------------------------------
#  汇总
# ---------------------------------------------------------------------------
Write-Host ''
Write-Host '================================================================' -ForegroundColor Cyan
Write-Host '  汇总' -ForegroundColor Cyan
Write-Host '================================================================' -ForegroundColor Cyan
Write-Host ("  {0,-30} {1,-10} {2}" -f '测试集', '结果', '用例')
Write-Host ('  ' + ('-' * 60))

$totalPass = 0; $totalFail = 0; $failedSuites = @()

foreach ($r in $results) {
    $ok = ($r.Exit -eq 0)
    if (-not $ok) { $failedSuites += "$($r.Area)/$($r.Name)" }
    $totalPass += $r.Pass
    $totalFail += $r.Fail
    $cases = if ($r.Pass + $r.Fail -gt 0) { "$($r.Pass)/$($r.Pass + $r.Fail)" } else { '-' }
    $tag = if ($ok) { 'PASS' } else { "FAIL($($r.Exit))" }
    $color = if ($ok) { 'Green' } else { 'Red' }
    Write-Host ("  {0,-30} {1,-10} {2}" -f "$($r.Area)/$($r.Name)", $tag, $cases) -ForegroundColor $color
}

Write-Host ('  ' + ('-' * 60))
Write-Host ("  测试集：{0} 个，通过 {1}，失败 {2}" -f $results.Count, ($results.Count - $failedSuites.Count), $failedSuites.Count) `
    -ForegroundColor $(if ($failedSuites.Count -eq 0) { 'Green' } else { 'Red' })
Write-Host ("  用例：通过 {0}，失败 {1}" -f $totalPass, $totalFail) `
    -ForegroundColor $(if ($failedSuites.Count -eq 0) { 'Green' } else { 'Red' })

if ($failedSuites.Count -gt 0) {
    Write-Host ''
    Write-Host "  失败测试集：$($failedSuites -join ', ')" -ForegroundColor Red
    if ($KeepLog) { Write-Host "  完整日志：$logDir" -ForegroundColor Yellow }
    else { Write-Host '  重跑时加 -KeepLog 可保留完整日志' -ForegroundColor Yellow }
    exit 1
}

Write-Host ''
Write-Host '  全部通过' -ForegroundColor Green
exit 0

# ============================================================================
#  test_all.ps1 —— 一条命令跑完全部测试（P12-4）
#
#  设计目标：**只有一套判定逻辑**。旧 scripts/test.bat 用 `fc /W` 比对
#  expected.txt（会吞掉空白差异），而 test_legacy_all.ps1 是逐字节比对 ——
#  同一份输出可能在一个脚本里过、在另一个里挂。现在全部收敛到这里。
#
#  每个套件是一个独立的 .ps1，自己判定并返回退出码。本脚本只负责：
#    1. 按依赖顺序跑（先编译层，再功能层，最后标准库与 legacy）
#    2. 收集退出码与「通过/失败」计数
#    3. 汇总成一张表；任一失败 exit 1
#
#  用法：
#    powershell -NoProfile -ExecutionPolicy Bypass -File test_scripts\test_all.ps1
#    powershell ... -File test_scripts\test_all.ps1 -Quick
#    powershell ... -File test_scripts\test_all.ps1 -Filter std
#    powershell ... -File test_scripts\test_all.ps1 -StopOnFail
#
#  退出码：全绿 0，任一失败 1。
# ============================================================================

param(
    [switch]$Quick,          # 跳过 legacy 与慢套件
    [string]$Filter = '',    # 只跑名字匹配该正则的套件
    [switch]$StopOnFail,     # 首个失败就停
    [switch]$KeepLog         # 保留每个套件的完整日志到 build\test_logs
)

$ErrorActionPreference = 'Continue'
$repo = Split-Path -Parent $PSScriptRoot
$rcc = Join-Path $repo 'release\RCC.exe'
$logDir = Join-Path $repo 'build\test_logs'

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
#  套件清单
#
#  顺序有讲究：先跑「编译器自身是否正确」（p0/p1/...），再跑「标准库是否正确」，
#  最后跑 legacy。这样一旦失败，最先失败的那个通常就是根因。
#
#  Slow = 参与 -Quick 过滤；Legacy = 单独标注（15/17 是 G6 已知延期）。
# ---------------------------------------------------------------------------

$suites = @(
    @{ Name = 'test_p0';                   Slow = $false; Legacy = $false; Desc = 'P0 骨架与 CLI' }
    @{ Name = 'test_p1_control';           Slow = $false; Legacy = $false; Desc = 'P1 控制流' }
    @{ Name = 'test_p2_functions';         Slow = $false; Legacy = $false; Desc = 'P4 函数值' }
    @{ Name = 'test_p5_containers';        Slow = $false; Legacy = $false; Desc = 'P5 容器' }
    @{ Name = 'test_p6_classes';           Slow = $false; Legacy = $false; Desc = 'P6 类' }
    @{ Name = 'test_p7_exceptions';        Slow = $false; Legacy = $false; Desc = 'P7 异常' }
    @{ Name = 'test_p8_modules';           Slow = $false; Legacy = $false; Desc = 'P8 模块' }
    @{ Name = 'test_p9_types';             Slow = $false; Legacy = $false; Desc = 'P9 类型' }
    @{ Name = 'test_p10_generics';         Slow = $false; Legacy = $false; Desc = 'P10 泛型' }
    @{ Name = 'test_p11_overload';         Slow = $false; Legacy = $false; Desc = 'P11 重载' }
    @{ Name = 'test_p12_i64_literals';     Slow = $false; Legacy = $false; Desc = 'P12 int64 字面量' }
    @{ Name = 'test_p12_g1a_globals';      Slow = $false; Legacy = $false; Desc = 'P12 G1a 全局槽' }
    @{ Name = 'test_p12_g2_index_assign';  Slow = $false; Legacy = $false; Desc = 'P12 G2 索引赋值' }
    @{ Name = 'test_p12_g3g4_concat';      Slow = $false; Legacy = $false; Desc = 'P12 G3+G4 拼接' }
    @{ Name = 'test_p12_g5_method_overload'; Slow = $false; Legacy = $false; Desc = 'P12 G5 方法重载' }
    @{ Name = 'test_p12_g7g11_ret_field';  Slow = $false; Legacy = $false; Desc = 'P12 G7+G11' }
    @{ Name = 'test_p12_g9g10_str';        Slow = $false; Legacy = $false; Desc = 'P12 G9+G10 str' }
    @{ Name = 'test_p12_g12g13_ptr_any';   Slow = $false; Legacy = $false; Desc = 'P12 G12+G13 ptr/any' }
    @{ Name = 'test_symbol_export';        Slow = $false; Legacy = $false; Desc = 'P12-1 符号表导出' }
    @{ Name = 'test_p12_std_skeleton';     Slow = $false; Legacy = $false; Desc = 'P12-3-A std 骨架' }
    @{ Name = 'test_p12_3b_io_str';        Slow = $false; Legacy = $false; Desc = 'P12-3-B io/str' }
    @{ Name = 'test_p12_3c_ds';            Slow = $false; Legacy = $false; Desc = 'P12-3-C ds' }
    @{ Name = 'test_std';                  Slow = $true;  Legacy = $false; Desc = 'P12-3-D 三层验收' }
    @{ Name = 'test_legacy_all';           Slow = $true;  Legacy = $true;  Desc = 'legacy 17 套（G6 延期 2 项）' }
)

if ($Quick) { $suites = @($suites | Where-Object { -not $_.Slow }) }
if ($Filter) { $suites = @($suites | Where-Object { $_.Name -match $Filter }) }

if ($suites.Count -eq 0) {
    Write-Host "没有匹配的套件（Filter='$Filter'）" -ForegroundColor Yellow
    exit 1
}

# ---------------------------------------------------------------------------
#  执行
# ---------------------------------------------------------------------------

Write-Host ''
Write-Host '================================================================' -ForegroundColor Cyan
Write-Host '  RCC 全量测试' -ForegroundColor Cyan
Write-Host '================================================================' -ForegroundColor Cyan
Write-Host ("  套件数：{0}{1}" -f $suites.Count, $(if ($Quick) { '（Quick 模式）' } else { '' }))
Write-Host ''

$results = @()
$idx = 0

foreach ($s in $suites) {
    $idx++
    $path = Join-Path $PSScriptRoot "$($s.Name).ps1"
    if (-not (Test-Path $path)) {
        Write-Host ("  [{0}/{1}] {2,-28} SKIP（脚本不存在）" -f $idx, $suites.Count, $s.Name) -ForegroundColor Yellow
        $results += [pscustomobject]@{ Name = $s.Name; Desc = $s.Desc; Exit = 127; Pass = 0; Fail = 0; Note = 'missing' }
        continue
    }

    Write-Host ("  [{0}/{1}] {2,-28} {3}" -f $idx, $suites.Count, $s.Name, $s.Desc) -ForegroundColor Gray

    $sw = [System.Diagnostics.Stopwatch]::StartNew()
    $out = & powershell -NoProfile -ExecutionPolicy Bypass -File $path 2>&1 | Out-String
    $code = $LASTEXITCODE
    $sw.Stop()

    if ($KeepLog) {
        [System.IO.File]::WriteAllText((Join-Path $logDir "$($s.Name).log"), $out,
            (New-Object System.Text.UTF8Encoding($false)))
    }

    # 从输出尾部解析「通过 N，失败 M」；各脚本措辞不完全统一，多写几个模式。
    $pass = 0; $fail = 0
    $m = [regex]::Match($out, '通过\s*(\d+)[，,]\s*失败\s*(\d+)')
    if (-not $m.Success) { $m = [regex]::Match($out, '总计\s*(\d+)\s*项[：:]\s*通过\s*(\d+)[，,]\s*失败\s*(\d+)') }
    if ($m.Success) {
        if ($m.Groups.Count -ge 3 -and $m.Groups[2].Value -ne '') {
            $pass = [int]$m.Groups[1].Value; $fail = [int]$m.Groups[2].Value
        }
    }

    # legacy 的判定特例：15/17 是 G6 延期，不是回归。脚本自己已按此返回 0，
    # 这里只在汇总里标注，不改判。
    $note = ''
    if ($s.Legacy) {
        $note = 'G6 延期 2 项'
        $lm = [regex]::Match($out, '失败用例[：:]\s*(.+)')
        if ($lm.Success) { $note = 'G6 延期：' + $lm.Groups[1].Value.Trim() }
    }

    $ok = ($code -eq 0)
    $color = if ($ok) { 'Green' } else { 'Red' }
    $tag = if ($ok) { 'PASS' } else { 'FAIL' }
    Write-Host ("           {0}  exit={1}  {2:N1}s  {3}" -f $tag, $code, $sw.Elapsed.TotalSeconds, $note) -ForegroundColor $color

    if (-not $ok) {
        # 只打印失败套件的尾部，完整日志用 -KeepLog 取
        $tail = ($out -split "`n" | Where-Object { $_ -match '\[失败\]|失败|Error' } | Select-Object -First 6)
        foreach ($t in $tail) { Write-Host ("             " + $t.TrimEnd()) -ForegroundColor DarkRed }
    }

    $results += [pscustomobject]@{ Name = $s.Name; Desc = $s.Desc; Exit = $code; Pass = $pass; Fail = $fail; Note = $note }

    if (-not $ok -and $StopOnFail) {
        Write-Host ''
        Write-Host '  -StopOnFail：中止后续套件' -ForegroundColor Yellow
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
Write-Host ("  {0,-30} {1,-8} {2,-10} {3}" -f '套件', '结果', '用例', '备注')
Write-Host ('  ' + ('-' * 74))

$totalPass = 0; $totalFail = 0; $failedSuites = @()

foreach ($r in $results) {
    $ok = ($r.Exit -eq 0)
    if (-not $ok) { $failedSuites += $r.Name }
    $totalPass += $r.Pass
    $totalFail += $r.Fail
    $cases = if ($r.Pass + $r.Fail -gt 0) { "$($r.Pass)/$($r.Pass + $r.Fail)" } else { '-' }
    $tag = if ($ok) { 'PASS' } else { "FAIL($($r.Exit))" }
    $color = if ($ok) { 'Green' } else { 'Red' }
    Write-Host ("  {0,-30} {1,-8} {2,-10} {3}" -f $r.Name, $tag, $cases, $r.Note) -ForegroundColor $color
}

Write-Host ('  ' + ('-' * 74))
Write-Host ("  套件：{0} 个，通过 {1}，失败 {2}" -f $results.Count, ($results.Count - $failedSuites.Count), $failedSuites.Count) `
    -ForegroundColor $(if ($failedSuites.Count -eq 0) { 'Green' } else { 'Red' })
Write-Host ("  用例：通过 {0}，失败 {1}" -f $totalPass, $totalFail) `
    -ForegroundColor $(if ($failedSuites.Count -eq 0) { 'Green' } else { 'Red' })

if ($failedSuites.Count -gt 0) {
    Write-Host ''
    Write-Host "  失败套件：$($failedSuites -join ', ')" -ForegroundColor Red
    if ($KeepLog) { Write-Host "  完整日志：$logDir" -ForegroundColor Yellow }
    else { Write-Host '  重跑时加 -KeepLog 可保留每个套件的完整日志' -ForegroundColor Yellow }
    exit 1
}

Write-Host ''
Write-Host '  全部通过' -ForegroundColor Green
exit 0

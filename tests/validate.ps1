# ============================================================================
#  validate.ps1 —— 校验 tests/cases 与 tests/agent 下所有测试集是否符合契约
#
#  契约（每个测试集目录必须满足）：
#    manifest.json   存在、是合法 JSON、含 name/version/entry/timeoutSec/expected
#    execute.ps1     存在
#    code/           存在
#    README.md       存在
#    expected/       存在（output.txt / exit-code.txt 可选）
#    manifest.name   与目录名一致
#    manifest.entry  指向的文件确实存在
#
#  只校验 cases/ 与 agent/；user/ 是用户自由区，不校验。
#  以 `_` 开头的目录视为内部共享目录（如 agent/_shared），跳过。
#
#  用法：
#    powershell -NoProfile -ExecutionPolicy Bypass -File tests\validate.ps1
#  退出码：全部合规 0，否则 1。
# ============================================================================

$ErrorActionPreference = 'Stop'
$root = $PSScriptRoot

$script:errors = 0
$script:warns = 0
$script:setCount = 0

function Fail($set, $msg) {
    $script:errors++
    Write-Host ("  [错误] {0}: {1}" -f $set, $msg) -ForegroundColor Red
}

function Warn($set, $msg) {
    $script:warns++
    Write-Host ("  [警告] {0}: {1}" -f $set, $msg) -ForegroundColor Yellow
}

foreach ($group in @('cases', 'agent')) {
    $dir = Join-Path $root $group
    Write-Host ''
    Write-Host ("==== {0}/ ====" -f $group) -ForegroundColor Cyan

    if (-not (Test-Path $dir)) {
        Write-Host "  （目录不存在，跳过）" -ForegroundColor DarkGray
        continue
    }

    $sets = @(Get-ChildItem $dir -Directory | Where-Object { -not $_.Name.StartsWith('_') })
    if ($sets.Count -eq 0) {
        Write-Host "  （暂无测试集）" -ForegroundColor DarkGray
        continue
    }

    foreach ($s in $sets) {
        $script:setCount++
        $name = $s.Name
        $ok = $true

        $manifestPath = Join-Path $s.FullName 'manifest.json'
        $entryPath    = Join-Path $s.FullName 'execute.ps1'
        $codePath     = Join-Path $s.FullName 'code'
        $readmePath   = Join-Path $s.FullName 'README.md'
        $expectPath   = Join-Path $s.FullName 'expected'

        if (-not (Test-Path $manifestPath)) {
            Fail $name '缺 manifest.json'; $ok = $false
        } else {
            $m = $null
            try {
                $m = Get-Content $manifestPath -Raw -Encoding UTF8 | ConvertFrom-Json
            } catch {
                Fail $name ('manifest.json 不是合法 JSON: ' + $_.Exception.Message); $ok = $false
            }
            if ($m) {
                foreach ($field in @('name', 'version', 'entry', 'timeoutSec', 'expected')) {
                    if ($null -eq $m.$field) { Fail $name "manifest.json 缺字段 '$field'"; $ok = $false }
                }
                if ($m.name -and $m.name -ne $name) {
                    Fail $name ("manifest.name='{0}' 与目录名不一致" -f $m.name); $ok = $false
                }
                if ($m.entry -and -not (Test-Path (Join-Path $s.FullName $m.entry))) {
                    Fail $name ("manifest.entry='{0}' 指向的文件不存在" -f $m.entry); $ok = $false
                }
                if ($null -ne $m.timeoutSec -and [int]$m.timeoutSec -le 0) {
                    Fail $name ("manifest.timeoutSec 必须是正整数（{0}）" -f $m.timeoutSec); $ok = $false
                }
                if ($m.type -and ($m.type -notin @('repro', 'regression', 'smoke'))) {
                    Warn $name ("manifest.type='{0}' 不在 repro/regression/smoke 之内" -f $m.type)
                }
            }
        }

        if (-not (Test-Path $entryPath)) { Fail $name '缺 execute.ps1'; $ok = $false }
        if (-not (Test-Path $codePath))  { Fail $name '缺 code/'; $ok = $false }
        if (-not (Test-Path $readmePath)) { Warn $name '缺 README.md' }
        if (-not (Test-Path $expectPath)) { Warn $name '缺 expected/' }

        if ($ok) {
            Write-Host ("  [通过] {0}" -f $name) -ForegroundColor Green
        }
    }
}

Write-Host ''
Write-Host '============================================================' -ForegroundColor Cyan
Write-Host ("校验完成：测试集 {0} 个，错误 {1}，警告 {2}" -f $script:setCount, $script:errors, $script:warns) `
    -ForegroundColor $(if ($script:errors -eq 0) { 'Green' } else { 'Red' })

if ($script:errors -gt 0) { exit 1 }
exit 0

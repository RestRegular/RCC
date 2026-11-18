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
#  用例清单一致性（仅对带 code/cases.json 的套件）：
#    manifest.caseCount 必须等于 cases.json 里 cases 的实际条数
#    每条 case 的 id 必须唯一
#    每条 case 引用的源文件必须存在（单文件用 file，多文件用 main + files）
#    code/ 下的 .rio 必须都被 cases.json 引用（防止加了用例却没登记）
#
#  为什么要校验 caseCount：这个字段没有任何脚本读取，纯粹是给人看的数字，
#  于是极易忘记同步 —— 实测多次出现「加了用例但 manifest 数字没改」。
#  它一旦和实际不符，文档里的「N 条用例全过」就是假的。
#
#  只校验 cases/ 与 agent/；user/ 是用户自由区，不校验。
#  以 `_` 开头的目录视为内部共享目录（如 _shared），跳过。
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

        # ---------- 用例清单一致性（只对带 code/cases.json 的套件） ----------
        # 旧式套件（用例直接写在 execute.ps1 里、code/ 只有 .gitkeep）没有
        # cases.json，无法静态核对条数，跳过 —— 它们由 run-all 解析
        # 「结果：通过 N」那行来统计，不在本脚本职责内。
        $casesPath = Join-Path $codePath 'cases.json'
        if (Test-Path $casesPath) {
            $spec = $null
            try {
                $spec = Get-Content $casesPath -Raw -Encoding UTF8 | ConvertFrom-Json
            } catch {
                Fail $name ('code/cases.json 不是合法 JSON: ' + $_.Exception.Message); $ok = $false
            }
            if ($spec) {
                $cases = @($spec.cases)
                $actualCount = $cases.Count

                # ① caseCount 与实际条数一致
                if ($null -eq $m) {
                    # manifest 本身缺失/损坏，前面已报错，这里不重复
                } elseif ($null -eq $m.caseCount) {
                    Fail $name ("manifest.json 缺字段 'caseCount'（cases.json 实际 {0} 条）" -f $actualCount); $ok = $false
                } elseif ([int]$m.caseCount -ne $actualCount) {
                    Fail $name ("manifest.caseCount={0} 与 cases.json 实际 {1} 条不一致（改了用例记得同步数字）" -f $m.caseCount, $actualCount)
                    $ok = $false
                }

                # ② id 唯一
                $dups = @($cases | ForEach-Object { $_.id } | Group-Object | Where-Object { $_.Count -gt 1 })
                foreach ($d in $dups) {
                    Fail $name ("cases.json 里 id '{0}' 重复 {1} 次" -f $d.Name, $d.Count); $ok = $false
                }

                # ③ 每条 case 引用的源文件存在，并收集被引用的文件名
                $referenced = @{}
                foreach ($c in $cases) {
                    $refs = @()
                    if ($c.file) {
                        $refs += $c.file
                    } else {
                        if ($c.main) { $refs += $c.main }
                        if ($c.files) { $refs += @($c.files) }
                    }
                    if ($refs.Count -eq 0) {
                        Fail $name ("case '{0}' 既没有 file 也没有 main/files 字段" -f $c.id); $ok = $false
                        continue
                    }
                    foreach ($r in $refs) {
                        # 用例里的路径是相对套件目录、用 / 分隔
                        $abs = Join-Path $s.FullName ($r -replace '/', '\')
                        if (-not (Test-Path $abs)) {
                            Fail $name ("case '{0}' 引用的文件不存在: {1}" -f $c.id, $r); $ok = $false
                        }
                        $referenced[(Split-Path $r -Leaf)] = $true
                    }
                }

                # ④ code/ 下的 .rio 都被引用（防止加了用例文件却没登记进 cases.json）
                $orphans = @(Get-ChildItem $codePath -Filter *.rio -File -Recurse |
                    Where-Object { -not $referenced.ContainsKey($_.Name) })
                foreach ($o in $orphans) {
                    $rel = $o.FullName.Substring($s.FullName.Length + 1) -replace '\\', '/'
                    Warn $name ("code 下的 '{0}' 没有被 cases.json 引用（孤儿用例）" -f $rel)
                }
            }
        }

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

# ============================================================================
#  driver.ps1 —— tests/agent 下数据驱动测试集的**唯一判定逻辑**
#
#  为什么有这个东西：26 个套件若各自内联一份「编译→运行→比对」的代码，
#  判定语义迟早漂移（旧系统就吃过这个亏：`fc /W` 与逐字节比对对同一份输出
#  给出不同结论）。所以判定只有这一份，套件只提供**数据**（code/cases.json
#  + code/*.rio），execute.ps1 只做三行转接。
#
#  目录约定：
#    tests/agent/_shared/driver.ps1     ← 本文件（下划线开头 = 内部目录，validate 跳过）
#    tests/agent/<suite>/execute.ps1    ← 薄壳，dot-source 本文件
#    tests/agent/<suite>/code/cases.json ← 用例清单
#    tests/agent/<suite>/code/*.rio      ← 每个用例一个可直接编译的源文件
#
#  cases.json 结构：
#    {
#      "cases": [
#        { "id": "while", "file": "code/while.rio", "expect": "0\n1\n2" },
#        { "id": "neg_x", "file": "code/neg_x.rio",
#          "expectFailure": true, "mustMatch": "borrowed" },
#        { "id": "multi", "main": "code/multi/main.rio",
#          "files": ["code/multi/main.rio", "code/multi/mathx.rio"],
#          "expect": "3", "exitCode": 0 },
#        { "id": "ds01", "file": "code/ds01.rio",
#          "expect": "true/false", "matchMode": "ignoreWhitespace" }
#      ]
#    }
#
#  用例字段：
#    id            必填，唯一
#    file          单文件用例的源（相对套件根）
#    main + files  多文件用例：main 是编译入口，files 是全部需要落盘的源
#    expect        期望 stdout（equals 模式下比对，尾部空白归一）
#    matchMode     equals（默认）| ignoreWhitespace（去掉全部空白后比对）
#    exitCode      期望运行退出码，默认 0
#    flags         额外编译器开关数组（例如 @('-allow-cond-destroy')）
#    expectFailure true = 必须**编译失败**；此时 expect 无意义
#    mustMatch     expectFailure 时要求的诊断片段（正则）
# ============================================================================

$script:RioUtf8 = New-Object System.Text.UTF8Encoding($false)

function Write-RioSuiteFail($id, $reason) {
    Write-Host ("  [失败] {0} —— {1}" -f $id, $reason) -ForegroundColor Red
}

function Write-RioSuitePass($id) {
    Write-Host ("  [通过] {0}" -f $id) -ForegroundColor Green
}

# 归一化 stdout：CRLF -> LF，去掉尾部空白。RCC 输出 "\n"，PowerShell 管道
# 会把它变成 CRLF；不归一的话每个多行用例都会因为不可见字符失败。
function Convert-RioStdout($text) {
    if ($null -eq $text) { return '' }
    return ($text -replace "`r`n", "`n").Trim()
}

function Invoke-RioBuild($rcc, $mainPath, $exePath, $extraFlags) {
    # 预期会失败的命令会往 stderr 写诊断；ErrorActionPreference=Stop 会把它
    # 当成终止错误抛出，所以调用期间必须临时降级。
    $saved = $ErrorActionPreference
    $ErrorActionPreference = 'Continue'
    try {
        # 必须给 -Width：默认宽度（控制台 120 列）会把长诊断**从词中间硬折行**，
        # 例如 'instance-level' 被劈成 'instance-l' + 'evel'。这样期望片段就
        # 永远匹配不上，而且是否折行取决于临时目录路径长度 —— 极难定位。
        # 给一个大宽度，捕获到的就是未折行的原始文本。
        #
        # $extraFlags 供用例传单横线开关（例如 -allow-cond-destroy）。放在
        # --path 之前：取值选项的实参不能和开关混在一起。
        $flagArgs = @()
        if ($extraFlags) { $flagArgs = @($extraFlags) }
        $out = & $rcc @flagArgs -build --path $mainPath --output $exePath 2>&1 | Out-String -Width 10000
        $code = $LASTEXITCODE
    } finally {
        $ErrorActionPreference = $saved
    }
    return @{ Text = $out; Code = $code; Flat = (Convert-RioFlat $out) }
}

# 诊断文本的「折行无关」形态。
# 主防线是 Invoke-RioBuild 里的 -Width 10000（从源头避免折行）；
# 这里再做一层兜底：折叠空白，并修复「连字符断词」残留
# （'instance-l evel' -> 'instance-level'），以防某些环境下仍被折行。
function Convert-RioFlat($text) {
    if ($null -eq $text) { return '' }
    $s = $text -replace '\s+', ' '
    $s = $s -replace '(?<=[A-Za-z])- (?=[a-z])', '-'
    return $s.Trim()
}

function Invoke-RioSuite {
    param([string]$WorkDir = $PSScriptRoot)

    $suiteDir = (Resolve-Path $WorkDir).Path
    $suiteName = Split-Path $suiteDir -Leaf
    # tests/agent/<suite> -> tests/agent -> tests -> <repo>
    $repo = Split-Path (Split-Path (Split-Path $suiteDir -Parent) -Parent) -Parent
    $rcc = Join-Path $repo 'release\RCC.exe'

    if (-not (Test-Path $rcc)) {
        Write-Host "找不到编译器：$rcc" -ForegroundColor Red
        Write-Host "请先构建：cmake --build cmake-build-debug --target RCC" -ForegroundColor Yellow
        exit 1
    }

    $casesPath = Join-Path $suiteDir 'code\cases.json'
    if (-not (Test-Path $casesPath)) {
        Write-Host "缺少用例清单：$casesPath" -ForegroundColor Red
        exit 1
    }
    $spec = Get-Content $casesPath -Raw -Encoding UTF8 | ConvertFrom-Json
    $cases = @($spec.cases)
    if ($cases.Count -eq 0) {
        Write-Host "用例清单为空：$casesPath" -ForegroundColor Red
        exit 1
    }

    # 临时目录放在仓库内：受限环境下系统 %TEMP% 可能不可写
    $work = Join-Path $repo ("build\tests\{0}" -f $suiteName)
    if (Test-Path $work) { Remove-Item $work -Recurse -Force }
    New-Item -ItemType Directory -Force -Path $work | Out-Null

    $pass = 0
    $fail = 0
    $failedIds = @()

    foreach ($case in $cases) {
        $id = $case.id
        if (-not $id) { Write-RioSuiteFail '(未命名)' '用例缺少 id'; $fail++; continue }

        # ---- 落盘源码（多文件用例各自建子目录，避免同名互覆盖）----
        $mainPath = $null
        if ($case.files) {
            $caseDir = Join-Path $work $id
            New-Item -ItemType Directory -Force -Path $caseDir | Out-Null
            foreach ($rel in @($case.files)) {
                $src = Join-Path $suiteDir $rel
                if (-not (Test-Path $src)) {
                    Write-RioSuiteFail $id "源文件缺失：$rel"
                    $fail++; $failedIds += $id; $mainPath = $null; break
                }
                $dstName = Split-Path $rel -Leaf
                Copy-Item $src (Join-Path $caseDir $dstName) -Force
            }
            if (-not $mainPath -and $case.main) {
                $mainPath = Join-Path $caseDir (Split-Path $case.main -Leaf)
            }
        } else {
            $src = Join-Path $suiteDir $case.file
            if (-not (Test-Path $src)) {
                Write-RioSuiteFail $id "源文件缺失：$($case.file)"
                $fail++; $failedIds += $id; continue
            }
            $mainPath = Join-Path $work ("{0}.rio" -f $id)
            Copy-Item $src $mainPath -Force
        }
        if (-not $mainPath) { continue }

        $exePath = Join-Path $work $id
        $build = Invoke-RioBuild $rcc $mainPath $exePath $case.flags
        $buildOk = ($build.Code -eq 0) -and (Test-Path "$exePath.exe")

        # ---- 负向用例：必须编译失败，且诊断命中指定片段 ----
        if ($case.expectFailure) {
            if ($buildOk) {
                Write-RioSuiteFail $id '本应编译失败，却编译成功了'
                $fail++; $failedIds += $id; continue
            }
            if ($case.mustMatch) {
                # 老套件里片段匹配有三种语义，必须原样保留：
                #   mustMatchLiteral -> $out.Contains(...)  字面子串（p13-2 回归）
                #   mustMatchRegex   -> -match 片段        正则（g2/g12g13/i64）
                #   否则             -> -like "*片段*"     通配符（p8/p9/p10/p11）
                # 三种都在「折行无关」文本上匹配：stderr 会被控制台宽度硬折行，
                # 直接匹配原始文本会因路径长度不同而随机失败。
                $hit = if ($case.mustMatchLiteral) {
                    $build.Flat.Contains($case.mustMatch)
                } elseif ($case.mustMatchRegex) {
                    $build.Flat -match $case.mustMatch
                } else {
                    $build.Flat -like ("*" + $case.mustMatch + "*")
                }
                if (-not $hit) {
                    $first = ($build.Text -split "`n" | Where-Object { $_ -match 'Error' } | Select-Object -First 1)
                    Write-RioSuiteFail $id ("报错了但诊断不含 '{0}'： {1}" -f $case.mustMatch, ("$first").Trim())
                    $fail++; $failedIds += $id; continue
                }
            }
            Write-RioSuitePass $id
            $pass++
            continue
        }

        # ---- 正向用例：必须编译成功 ----
        if (-not $buildOk) {
            $first = ($build.Text -split "`n" | Where-Object { $_ -match 'Error' } | Select-Object -First 1)
            $msg = if ($first) { ($first.Trim() -replace '\s+', ' ') } else { '编译失败（无诊断）' }
            Write-RioSuiteFail $id "编译失败：$msg"
            $fail++; $failedIds += $id; continue
        }

        # compileOnly：只验编译通过，不运行。用于「编译期行为」套件
        # （如 P13-2 回归：验的是类型检查/诊断，跑起来反而引入无关噪声）。
        if ($case.compileOnly) {
            Write-RioSuitePass $id
            $pass++
            continue
        }

        # ---- 运行 ----
        $saved = $ErrorActionPreference
        $ErrorActionPreference = 'Continue'
        try {
            $runOut = (& "$exePath.exe" 2>&1 | Out-String)
            $runCode = $LASTEXITCODE
        } finally {
            $ErrorActionPreference = $saved
        }

        $expectExit = if ($null -ne $case.exitCode) { [int]$case.exitCode } else { 0 }
        if ($runCode -ne $expectExit) {
            Write-RioSuiteFail $id ("退出码 {0}，期望 {1}" -f $runCode, $expectExit)
            $fail++; $failedIds += $id; continue
        }

        $actual = Convert-RioStdout $runOut
        $expect = Convert-RioStdout $case.expect
        if ($case.matchMode -eq 'ignoreWhitespace') {
            $actual = $actual -replace '\s+', ''
            $expect = $expect -replace '\s+', ''
        }

        # stderrContains：panic 之类的诊断走 stderr，PowerShell 的 2>&1 会把它
        # 连同调用栈噪声一起并进 stdout，没法用 expect 做等值比对。这里只要求
        # 原始输出里出现该片段（字面子串），用于「必须 panic 且消息正确」的用例。
        if ($case.stderrContains) {
            if (-not ($runOut -like ("*" + $case.stderrContains + "*"))) {
                Write-RioSuiteFail $id ("输出不含 '{0}'" -f $case.stderrContains)
                Write-Host ("         实际：{0}" -f (($runOut -replace "`n", ' / '))) -ForegroundColor DarkGray
                $fail++; $failedIds += $id; continue
            }
            Write-RioSuitePass $id
            $pass++
            continue
        }

        if ($actual -eq $expect) {
            Write-RioSuitePass $id
            $pass++
        } else {
            Write-RioSuiteFail $id '输出不符'
            Write-Host ("         期望：{0}" -f ($expect -replace "`n", ' / ')) -ForegroundColor DarkGray
            Write-Host ("         实际：{0}" -f ($actual -replace "`n", ' / ')) -ForegroundColor DarkGray
            $fail++; $failedIds += $id
        }
    }

    Write-Host ''
    Write-Host ("结果：通过 {0}，失败 {1}" -f $pass, $fail) `
        -ForegroundColor $(if ($fail -eq 0) { 'Green' } else { 'Red' })

    if ($fail -gt 0) {
        Write-Host ("失败用例：{0}" -f ($failedIds -join ', ')) -ForegroundColor Red
        Write-Host ("临时目录保留在：{0}" -f $work) -ForegroundColor Yellow
        exit 1
    }

    Remove-Item $work -Recurse -Force -ErrorAction SilentlyContinue
    exit 0
}

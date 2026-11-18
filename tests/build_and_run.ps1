# build-run.ps1
# 用法:
#   .\build-run.ps1                                   # 全流程（默认）
#   .\build-run.ps1 -Source .\tests\other.rio          # 指定源文件
#   .\build-run.ps1 -SkipCompiler                      # 跳过 RCC 编译器重编
#   .\build-run.ps1 -SkipBuild                         # 只重编编译器并跑旧 exe
#   .\build-run.ps1 -SkipRun                           # 只编译不运行
#   .\build-run.ps1 -SkipCompiler -SkipBuild           # 只跑现成 exe
#   .\build-run.ps1 -SkipCompiler -SkipBuild -SkipRun  # 空转（用于校验参数）
#
# 流程（各阶段均可跳过）:
#   0. 增量构建 RCC 编译器本体
#   1. 用 RCC.exe 把 .rio 源码编译成 .exe
#   2. 运行生成的 .exe，并把它的退出码透传给调用方

param(
    [string]$Source = ".\tests\test.rio",

    # ── 阶段开关（默认全部执行）────────────────────────────────
    [switch]$SkipCompiler,   # 跳过第 0 步：不构建 RCC 编译器
    [switch]$SkipBuild,      # 跳过第 1 步：不编译 .rio 源文件
    [switch]$SkipRun         # 跳过第 2 步：不运行生成的 exe
)

$ErrorActionPreference = "Stop"

# ── 路径常量（集中放置，方便以后改）────────────────────────────
$CMakeExe   = "D:\soft\Clion\CLion 2026.2.3.1\bin\cmake\win\x64\bin\cmake.exe"
$CMakeBuild = "D:\DeepseekHarnessProjects\RCC\cmake-build-debug"
$RccExe     = ".\release\RCC.exe"

# 汇总哪些阶段被跳过，便于一眼看清本次执行了哪些步骤
$skipped = @()
if ($SkipCompiler) { $skipped += "compiler" }
if ($SkipBuild)    { $skipped += "build"    }
if ($SkipRun)      { $skipped += "run"      }
if ($skipped.Count -gt 0) {
    Write-Host ("==> Skipping: {0}" -f ($skipped -join ", ")) -ForegroundColor Yellow
}

# ─────────────────────────────────────────────────────────────
# 0. 增量构建 RCC 编译器本体
# ─────────────────────────────────────────────────────────────
if (-not $SkipCompiler) {
    Write-Host "==> Building RCC compiler (incremental) ..." -ForegroundColor Cyan
    & $CMakeExe --build $CMakeBuild --target RCC -j 10
    if ($LASTEXITCODE -ne 0) {
        Write-Host "RCC build failed (exit code $LASTEXITCODE)" -ForegroundColor Red
        exit $LASTEXITCODE
    }
} else {
    Write-Host "==> [skip] Building RCC compiler" -ForegroundColor DarkGray
}

# ─────────────────────────────────────────────────────────────
# 1. 用 RCC 编译 .rio 源文件
# ─────────────────────────────────────────────────────────────
if (-not $SkipBuild) {
    Write-Host "==> Building $Source ..." -ForegroundColor Cyan
    & $RccExe build --p $Source
    if ($LASTEXITCODE -ne 0) {
        Write-Host "Build failed (exit code $LASTEXITCODE)" -ForegroundColor Red
        exit $LASTEXITCODE
    }
} else {
    Write-Host "==> [skip] Building $Source" -ForegroundColor DarkGray
}

# ─────────────────────────────────────────────────────────────
# 2. 推导可执行文件路径并运行
# ─────────────────────────────────────────────────────────────
if (-not $SkipRun) {
    $exePath = [System.IO.Path]::ChangeExtension($Source, ".exe")

    if (-not (Test-Path $exePath)) {
        Write-Host "Executable not found: $exePath" -ForegroundColor Red
        exit 1
    }

    Write-Host "==> Running $exePath ..." -ForegroundColor Cyan
    & $exePath
    exit $LASTEXITCODE
} else {
    Write-Host "==> [skip] Running exe" -ForegroundColor DarkGray
}
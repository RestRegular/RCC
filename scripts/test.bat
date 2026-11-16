@echo off
REM ============================================================================
REM  test.bat —— 一条命令跑完全部测试
REM
REM  P12-4 重写：旧版是 bat 里遍历 tests\test_* 目录、逐个 call run.bat、
REM  用 `fc /W` 比对 expected.txt。问题：
REM    1. `fc /W` 会**吞掉空白差异**，与 test_legacy_all.ps1 的逐字节比对
REM       语义不一致 —— 同一份输出在两个脚本里判定结果可能不同。
REM    2. bat 处理中文路径、引号、CRLF 都很脆，失败时信息也不足。
REM
REM  新版：本文件退化成薄壳，逻辑全在 test_scripts\test_all.ps1，只有一套
REM  判定逻辑。这样 run.bat / test.bat 只负责「找到工具」，不负责判定。
REM
REM  用法：
REM    scripts\test.bat              跑全部（含 legacy）
REM    scripts\test.bat -Quick       跳过 legacy 与慢套件
REM    scripts\test.bat -Filter std  只跑名字含 std 的套件
REM ============================================================================
setlocal

set "HERE=%~dp0"
set "PS1=%HERE%..\test_scripts\test_all.ps1"

if not exist "%PS1%" (
    echo ERROR: test runner not found: %PS1%
    exit /b 1
)

where powershell >nul 2>&1
if errorlevel 1 (
    echo ERROR: powershell not found in PATH
    exit /b 1
)

powershell -NoProfile -ExecutionPolicy Bypass -File "%PS1%" %*
exit /b %errorlevel%

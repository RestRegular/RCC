@echo off
REM ============================================================================
REM  test.bat —— 一条命令跑完全部测试集
REM
REM  本文件只是薄壳，真正的调度逻辑在 tests\user\run-all.ps1。
REM  运行器是**扫描式**的：找到 tests\{agent,cases} 下每个带 execute.ps1 的目录
REM  就执行，不再需要维护硬编码套件清单。
REM
REM  用法：
REM    scripts\test.bat                     跑 agent + cases
REM    scripts\test.bat -Area agent         只跑 agent
REM    scripts\test.bat -Area cases         只跑用户复现用例
REM    scripts\test.bat -Filter p13         只跑名字匹配的测试集
REM    scripts\test.bat -StopOnFail         首个失败就停
REM    scripts\test.bat -KeepLog            保留每套完整日志到 build\test_logs
REM ============================================================================
setlocal

set "HERE=%~dp0"
set "PS1=%HERE%..\tests\user\run-all.ps1"

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

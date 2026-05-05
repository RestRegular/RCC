@echo off
cls
echo ^>^> test.bat %*
setlocal enabledelayedexpansion

REM 检查参数数量
if "%~2"=="" (
    echo 用法: %~nx0 ^<要扫描的目录^> ^<输出日志目录^>
    echo 示例: %~nx0 C:\input D:\output
    exit /b 1
)

REM 获取要扫描的绝对路径
set "scan_dir=%~f1"
if not exist "%scan_dir%" (
    echo 错误: 目录 "%scan_dir%" 不存在
    exit /b 1
)

REM 获取输出目录的绝对路径
set "out_dir=%~f2"
if not exist "%out_dir%" (
    mkdir "%out_dir%" 2>nul
    if errorlevel 1 (
        echo 错误: 无法创建输出目录 "%out_dir%"
        exit /b 1
    )
)

REM 获取脚本所在目录
set "script_dir=%~dp0"
set "runner=%script_dir%run.bat"

if not exist "%runner%" (
    echo 错误: 未找到 "%runner%"
    exit /b 1
)

echo 扫描目录: %scan_dir%
echo 输出目录: %out_dir%
echo 使用执行器: %runner%
echo.

REM 递归查找所有 .rio 文件
for /R "%scan_dir%" %%F in (*.rio) do (
    set "fullpath=%%F"
    set "filename=%%~nF"
    set "logfile=%out_dir%!filename!.output"

    echo 处理: !fullpath!
    echo 输出: !logfile!

    REM 执行 run.bat -i <rio文件>，并将输出重定向到日志文件
    call "%runner%" -i "!fullpath!" > "!logfile!" 2>&1

    if errorlevel 1 (
        echo 警告: 处理 !fullpath! 时出错
    ) else (
        echo 完成: !fullpath!
    )
    echo.
)

echo 所有处理完成。
exit /b 0
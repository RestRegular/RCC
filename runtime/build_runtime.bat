@echo off
REM ============================================================================
REM RCC Runtime Library - Build & Run Script (Windows)
REM ============================================================================
REM Usage:
REM   build_runtime.bat                       - 仅编译运行时
REM   build_runtime.bat run tests\irs\test.ll - 编译运行时并运行 IR
REM
REM 自动检测 LLVM clang，回退到 g++/gcc
REM ============================================================================

set "ACTION=%~1"
set "IR_FILE=%~2"

set "CC="
set "CLANG="

REM 优先检测 PATH 中的 clang
where clang >nul 2>&1
if %errorlevel% equ 0 (
    set "CC=clang"
    set "CLANG=clang"
    goto :compile
)

REM 检测常见 LLVM 安装路径
for %%P in (
    "D:\repositories\llvm-project\build\bin\clang.exe"
    "C:\LLVM\bin\clang.exe"
    "D:\LLVM\bin\clang.exe"
) do (
    if exist %%P (
        set "CC=g++"
        set "CLANG=%%~P"
        goto :compile
    )
)

REM 回退到 g++/gcc
where g++ >nul 2>&1
if %errorlevel% equ 0 (
    set "CC=g++"
    goto :compile
)
where gcc >nul 2>&1
if %errorlevel% equ 0 (
    set "CC=gcc"
    goto :compile
)

echo [RCC] ERROR: No C compiler found.
exit /b 1

:compile
echo [RCC] Compiling runtime with %CC%...
%CC% -c -O2 runtime\rcc_runtime.c -o runtime\rcc_runtime.o
if %errorlevel% neq 0 (
    echo [RCC] ERROR: runtime compilation failed
    exit /b 1
)
echo [RCC] Runtime built: runtime\rcc_runtime.o

if not "%ACTION%"=="run" goto :done
if "%IR_FILE%"=="" (
    echo [RCC] Usage: build_runtime.bat run ^<ir_file^>
    exit /b 1
)
if not exist "%IR_FILE%" (
    echo [RCC] ERROR: file not found: %IR_FILE%
    exit /b 1
)

:run
echo [RCC] Compiling %IR_FILE% with %CLANG%...
%CLANG% -O2 "%IR_FILE%" runtime\rcc_runtime.o -o runtime\rcc_output.exe
if %errorlevel% neq 0 (
    echo [RCC] ERROR: clang compilation failed
    exit /b 1
)

echo [RCC] Running...
runtime\rcc_output.exe
del runtime\rcc_output.exe 2>nul

:done
echo.
echo [RCC] Manual usage:
echo [RCC]   %%CLANG%% -O2 test.ll runtime\rcc_runtime.o -o test.exe ^&^& test.exe

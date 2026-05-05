@echo off
REM ============================================================================
REM RCC Runtime Library - Build Script (Windows)
REM ============================================================================
REM Usage:
REM   build_runtime.bat              - 编译运行时库
REM   build_runtime.bat run test.ll  - 编译并运行 IR 文件
REM
REM Prerequisites:
REM   - LLVM clang in PATH (推荐)
REM   - 或 g++/gcc in PATH
REM ============================================================================

set "ACTION=%~1"
set "IR_FILE=%~2"

echo [RCC] Building runtime library...

set "CC="
set "LLVM_CLANG="

REM 优先使用 LLVM 自带的 clang（与 lli 兼容）
where clang >nul 2>&1
if %errorlevel% equ 0 (
    set "CC=clang"
    set "LLVM_CLANG=clang"
    goto :compile
)

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

echo [RCC] ERROR: No C compiler found. Install clang or g++.
exit /b 1

:compile
echo [RCC] Using %CC%...
%CC% -c -O2 runtime\rcc_runtime.c -o runtime\rcc_runtime.o
if %errorlevel% neq 0 (
    echo [RCC] ERROR: compilation failed
    exit /b 1
)
echo [RCC] Runtime object built: runtime\rcc_runtime.o

if "%ACTION%"=="run" goto :run
goto :done

:run
if "%IR_FILE%"=="" (
    echo [RCC] Usage: build_runtime.bat run ^<ir_file^>
    exit /b 1
)

if not exist "%IR_FILE%" (
    echo [RCC] ERROR: file not found: %IR_FILE%
    exit /b 1
)

echo [RCC] Running %IR_FILE%...

REM 方式 1: 使用 clang 直接编译 IR + 运行时为可执行文件（推荐）
if not "%LLVM_CLANG%"=="" (
    echo [RCC] Compiling with clang...
    %LLVM_CLANG% -O2 "%IR_FILE%" runtime\rcc_runtime.o -o runtime\rcc_output.exe
    if %errorlevel% equ 0 (
        echo [RCC] Running...
        runtime\rcc_output.exe
        del runtime\rcc_output.exe
        goto :done
    )
    echo [RCC] clang compilation failed, trying lli...
)

REM 方式 2: 使用 lli 加载 .o（部分版本支持）
where lli >nul 2>&1
if %errorlevel% equ 0 (
    echo [RCC] Trying lli with -load...
    lli -load=runtime\rcc_runtime.o "%IR_FILE%"
    if %errorlevel% equ 0 goto :done
)

echo [RCC] ERROR: Failed to run. Make sure LLVM clang is in PATH.
exit /b 1

:done
echo.
echo [RCC] Done.
echo [RCC] Manual usage:
echo [RCC]   clang -O2 test.ll runtime\rcc_runtime.o -o test.exe ^&^& test.exe

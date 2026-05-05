@echo off
REM ============================================================================
REM RCC Runtime Library - Build & Run Script (Windows)
REM ============================================================================
REM Usage:
REM   build_runtime.bat                       - 仅编译运行时
REM   build_runtime.bat run tests\irs\test.ll - 编译运行时并运行 IR
REM
REM 策略: llc(IR->obj) + g++(obj链接) 避免 clang 链接器问题
REM ============================================================================

set "ACTION=%~1"
set "IR_FILE=%~2"

set "CC="
set "LLC="

REM 检测 C 编译器
where g++ >nul 2>&1
if %errorlevel% equ 0 (
    set "CC=g++"
    goto :find_llc
)
where gcc >nul 2>&1
if %errorlevel% equ 0 (
    set "CC=gcc"
    goto :find_llc
)
echo [RCC] ERROR: g++/gcc not found.
exit /b 1

:find_llc
REM 检测 llc
where llc >nul 2>&1
if %errorlevel% equ 0 (
    set "LLC=llc"
    goto :compile
)
for %%P in (
    "D:\repositories\llvm-project\build\bin\llc.exe"
    "C:\LLVM\bin\llc.exe"
) do (
    if exist %%P (
        set "LLC=%%~P"
        goto :compile
    )
)
echo [RCC] ERROR: llc not found.
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
echo [RCC] Compiling IR with %LLC%...
%LLC% -filetype=obj -O2 "%IR_FILE%" -o runtime\rcc_ir.obj
if %errorlevel% neq 0 (
    echo [RCC] ERROR: llc compilation failed
    exit /b 1
)

echo [RCC] Linking with %CC%...
%CC% -O2 runtime\rcc_ir.obj runtime\rcc_runtime.o -o runtime\rcc_output.exe
if %errorlevel% neq 0 (
    echo [RCC] ERROR: linking failed
    del runtime\rcc_ir.obj 2>nul
    exit /b 1
)

echo [RCC] Running...
runtime\rcc_output.exe
del runtime\rcc_ir.obj 2>nul
del runtime\rcc_output.exe 2>nul

:done
echo.
echo [RCC] Manual usage:
echo [RCC]   llc -filetype=obj -O2 test.ll -o test.obj
echo [RCC]   g++ -O2 test.obj runtime\rcc_runtime.o -o test.exe
echo [RCC]   test.exe

@echo off
REM ============================================================================
REM RCC Runtime Library - Build Script (Windows)
REM ============================================================================
REM Usage: build_runtime.bat
REM
REM Prerequisites:
REM   - clang, g++, or MSVC cl.exe in PATH
REM ============================================================================

echo [RCC] Building runtime library...

where clang >nul 2>&1
if %errorlevel% equ 0 (
    echo [RCC] Using clang...
    clang -c -O2 runtime\rcc_runtime.c -o runtime\rcc_runtime.o
    if %errorlevel% neq 0 (
        echo [RCC] ERROR: clang compilation failed
        exit /b 1
    )
    echo [RCC] Runtime library built: runtime\rcc_runtime.o
    goto :done
)

where g++ >nul 2>&1
if %errorlevel% equ 0 (
    echo [RCC] Using g++...
    g++ -c -O2 runtime\rcc_runtime.c -o runtime\rcc_runtime.o
    if %errorlevel% neq 0 (
        echo [RCC] ERROR: g++ compilation failed
        exit /b 1
    )
    echo [RCC] Runtime library built: runtime\rcc_runtime.o
    goto :done
)

where gcc >nul 2>&1
if %errorlevel% equ 0 (
    echo [RCC] Using gcc...
    gcc -c -O2 runtime\rcc_runtime.c -o runtime\rcc_runtime.o
    if %errorlevel% neq 0 (
        echo [RCC] ERROR: gcc compilation failed
        exit /b 1
    )
    echo [RCC] Runtime library built: runtime\rcc_runtime.o
    goto :done
)

where cl >nul 2>&1
if %errorlevel% equ 0 (
    echo [RCC] Using MSVC cl...
    cl /c /O2 runtime\rcc_runtime.c /Fo:runtime\rcc_runtime.obj
    if %errorlevel% neq 0 (
        echo [RCC] ERROR: cl compilation failed
        exit /b 1
    )
    echo [RCC] Runtime library built: runtime\rcc_runtime.obj
    goto :done
)

echo [RCC] ERROR: No C/C++ compiler found. Install clang, g++, or MSVC.
exit /b 1

:done
echo [RCC] Usage: lli -extra-archive=runtime\rcc_runtime.o tests\irs\test.ll

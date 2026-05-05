@echo off
REM ============================================================================
REM RCC Runtime Library - Build Script (Windows/MSVC)
REM ============================================================================
REM Usage: build_runtime.bat
REM
REM Prerequisites:
REM   - LLVM installed (clang in PATH)
REM   - Or MSVC cl.exe in PATH
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
    echo [RCC] Usage: lli -extra-archive=runtime\rcc_runtime.o tests\irs\test.ll
) else (
    where cl >nul 2>&1
    if %errorlevel% equ 0 (
        echo [RCC] Using MSVC cl...
        cl /c /O2 runtime\rcc_runtime.c /Fo:runtime\rcc_runtime.obj
        if %errorlevel% neq 0 (
            echo [RCC] ERROR: cl compilation failed
            exit /b 1
        )
        REM Convert .obj to .o for lli compatibility (or use lib.exe)
        echo [RCC] Runtime library built: runtime\rcc_runtime.obj
        echo [RCC] Note: lli may need .o format. Use clang if available.
    ) else (
        echo [RCC] ERROR: No C compiler found. Install clang or MSVC.
        exit /b 1
    )
)

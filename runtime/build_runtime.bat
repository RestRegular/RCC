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

set "CC="
set "AR="

where clang >nul 2>&1
if %errorlevel% equ 0 (
    set "CC=clang"
    set "AR=llvm-ar"
    goto :compile
)

where g++ >nul 2>&1
if %errorlevel% equ 0 (
    set "CC=g++"
    set "AR=gcc-ar"
    goto :compile
)

where gcc >nul 2>&1
if %errorlevel% equ 0 (
    set "CC=gcc"
    set "AR=gcc-ar"
    goto :compile
)

where cl >nul 2>&1
if %errorlevel% equ 0 (
    echo [RCC] Using MSVC cl...
    cl /c /O2 runtime\rcc_runtime.c /Fo:runtime\rcc_runtime.obj
    if %errorlevel% neq 0 (
        echo [RCC] ERROR: cl compilation failed
        exit /b 1
    )
    REM MSVC: use lib.exe to create .lib archive
    where lib >nul 2>&1
    if %errorlevel% equ 0 (
        lib /OUT:runtime\rcc_runtime.lib runtime\rcc_runtime.obj
        echo [RCC] Runtime library built: runtime\rcc_runtime.lib
        echo [RCC] Usage: lli -extra-archive=runtime\rcc_runtime.lib tests\irs\test.ll
    ) else (
        echo [RCC] Runtime library built: runtime\rcc_runtime.obj
        echo [RCC] WARNING: lib.exe not found, cannot create archive for lli
    )
    goto :done
)

echo [RCC] ERROR: No C/C++ compiler found. Install clang, g++, or MSVC.
exit /b 1

:compile
echo [RCC] Using %CC%...
%CC% -c -O2 runtime\rcc_runtime.c -o runtime\rcc_runtime.o
if %errorlevel% neq 0 (
    echo [RCC] ERROR: compilation failed
    exit /b 1
)

REM Create static archive (.a) for lli -extra-archive
where %AR% >nul 2>&1
if %errorlevel% equ 0 (
    echo [RCC] Creating archive with %AR%...
    %AR% rcs runtime\rcc_runtime.a runtime\rcc_runtime.o
    if %errorlevel% neq 0 (
        echo [RCC] WARNING: archive creation failed, using .o directly
    ) else (
        echo [RCC] Runtime library built: runtime\rcc_runtime.a
    )
) else (
    where ar >nul 2>&1
    if %errorlevel% equ 0 (
        echo [RCC] Creating archive with ar...
        ar rcs runtime\rcc_runtime.a runtime\rcc_runtime.o
        echo [RCC] Runtime library built: runtime\rcc_runtime.a
    ) else (
        echo [RCC] Runtime library built: runtime\rcc_runtime.o
        echo [RCC] WARNING: ar not found, .o may not work with lli -extra-archive
    )
)

:done
echo [RCC] Usage: lli -extra-archive=runtime\rcc_runtime.a tests\irs\test.ll

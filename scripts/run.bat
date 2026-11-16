@echo off
REM ============================================================================
REM  run.bat —— 编译并运行单个 Rio 程序
REM
REM  P12-4 重写：旧版是 LLVM 时代的五步流水线
REM    RCC -ir -> llc -> g++ -> 运行
REM  其中 llc.exe 与 cmake-build-debug 两个路径早已不存在，整条链是死的。
REM  新后端直接发 x86-64 汇编，as / ld 由 RCC 自己调，所以只需一步。
REM
REM  用法：
REM    scripts\run.bat                          编译并运行 tests\test_1_builtins\test.rio
REM    scripts\run.bat -i tests\test_6_functions 指定目录
REM    scripts\run.bat -i tests\test_1_builtins --no-run     只编译不运行
REM    scripts\run.bat -i tests\test_1_builtins --skip-cmake 不重新构建编译器
REM    scripts\run.bat -h
REM ============================================================================
setlocal enabledelayedexpansion

REM 从脚本自身位置推导仓库根，避免写死 D:\ClionProjects\... 这类绝对路径
set "REPO=%~dp0.."
pushd "%REPO%" || (echo ERROR: cannot enter "%REPO%" & exit /b 1)
set "REPO=%CD%"
set "RCC_EXE=%REPO%\release\RCC.exe"
set "BUILD_DIR=%REPO%\build"

set "TEST_DIR=tests\test_1_builtins"
set "RUN_EXE=true"
set "SKIP_CMAKE=false"

if "%~1"=="" goto :after_parse
if "%~1"=="/?" goto :help
if "%~1"=="-h" goto :help
if "%~1"=="--help" goto :help

:parse_args
if "%~1"=="" goto :after_parse
if /i "%~1"=="-i" (
    set "TEST_DIR=%~2"
    shift
    shift
    goto :parse_args
)
if /i "%~1"=="--no-run" (
    set "RUN_EXE=false"
    shift
    goto :parse_args
)
if /i "%~1"=="--skip-cmake" (
    set "SKIP_CMAKE=true"
    shift
    goto :parse_args
)
echo Unknown option: %~1
goto :help

:after_parse

REM 测试目录可能是相对仓库根的，也可能是绝对路径
if exist "%TEST_DIR%\" (
    set "TDIR=%TEST_DIR%"
) else if exist "%REPO%\%TEST_DIR%\" (
    set "TDIR=%REPO%\%TEST_DIR%"
) else (
    echo ERROR: test directory not found: %TEST_DIR%
    popd
    exit /b 1
)
pushd "%TDIR%" || (echo ERROR: cannot enter "%TDIR%" & popd & exit /b 1)
set "TDIR=%CD%"
popd

set "INPUT_RIO=%TDIR%\test.rio"
set "OUTPUT_EXE=%TDIR%\test.exe"
set "ACTUAL_TXT=%TDIR%\actual.txt"

echo ========================================
echo RCC Build Script
echo ========================================
echo Repo:             %REPO%
echo Test directory:   %TDIR%
echo Input file:       %INPUT_RIO%
echo Output exe:       %OUTPUT_EXE%
echo Run after build:  %RUN_EXE%
echo Skip CMake build: %SKIP_CMAKE%
echo ========================================
echo.

REM 步骤 0：构建编译器（唯一需要的外部工具是 mingw32-make）
if /i "%SKIP_CMAKE%"=="false" (
    echo [0/2] Building RCC compiler...
    where mingw32-make >nul 2>&1
    if errorlevel 1 (
        echo WARNING: mingw32-make not found in PATH, skipping build
        echo          Use --skip-cmake to silence this, or add MinGW bin to PATH
    ) else (
        mingw32-make -C "%BUILD_DIR%" -j 10
        if errorlevel 1 (
            echo ERROR: build failed
            popd
            exit /b 1
        )
    )
    echo.
) else (
    echo [0/2] Skipping compiler build --skip-cmake
    echo.
)

if not exist "%RCC_EXE%" (
    echo ERROR: RCC compiler not found: %RCC_EXE%
    echo Build it first: mingw32-make -C build -j 10
    popd
    exit /b 1
)

if not exist "%INPUT_RIO%" (
    echo ERROR: input file not found: %INPUT_RIO%
    popd
    exit /b 1
)

REM 步骤 1：一步到位编译（RCC 内部依次调 as 汇编、g++ 链接）
echo [1/2] Compiling %INPUT_RIO% ...
"%RCC_EXE%" -build --path "%INPUT_RIO%" --output "%OUTPUT_EXE%"
if errorlevel 1 (
    echo ERROR: compile failed
    popd
    exit /b 1
)
echo SUCCESS: %OUTPUT_EXE%
echo.

REM 步骤 2：运行并落盘 actual.txt
if /i "%RUN_EXE%"=="true" (
    echo [2/2] Running executable...
    echo ========================================
    "%OUTPUT_EXE%" > "%ACTUAL_TXT%" 2>&1
    set "RUN_CODE=!errorlevel!"
    type "%ACTUAL_TXT%"
    echo ========================================
    echo Output saved to %ACTUAL_TXT%
    echo.
    if not "!RUN_CODE!"=="0" (
        echo ERROR: program exited with code !RUN_CODE!
        popd
        exit /b !RUN_CODE!
    )
) else (
    echo [2/2] Skipping execution --no-run
)

echo.
echo ========================================
echo Build completed successfully!
echo ========================================
popd
exit /b 0

:help
echo Usage: %~nx0 [options]
echo.
echo Options:
echo   -i ^<test_dir^>    Directory containing test.rio (default: tests\test_1_builtins)
echo   --no-run           Skip running the executable after building
echo   --skip-cmake       Skip rebuilding the compiler (use existing release\RCC.exe)
echo   -h, --help, /?     Show this help message
echo.
echo Examples:
echo   %~nx0
echo   %~nx0 -i tests\test_6_functions
echo   %~nx0 -i tests\test_9_integration --no-run
echo   %~nx0 -i tests\test_1_builtins --skip-cmake
echo.
popd
exit /b 0

@echo off
cls
echo ^>^> run.bat %*
setlocal enabledelayedexpansion

REM 设置默认路径变量
set RCC_EXE=.\release\RCC.exe
set LLC_EXE=D:\repositories\llvm-project\build\bin\llc.exe
set GXX_EXE=g++.exe
set CMAKE_EXE=D:\soft\Clion\CLion-2024.2.3\bin\cmake\win\x64\bin\cmake.exe
set CMAKE_BUILD_DIR=D:\ClionProjects\RCC\cmake-build-debug
set CMAKE_TARGET=RCC

REM 默认测试目录
set TEST_DIR=.\tests\test_1_builtins
set RUN_EXE=true
set SKIP_CMAKE=false

REM 显示帮助信息
if "%1"=="/?" goto :help
if "%1"=="-h" goto :help
if "%1"=="--help" goto :help

REM 解析命令行参数
:parse_args
if "%1"=="" goto :after_parse
if "%1"=="-i" (
    set TEST_DIR=%2
    shift
    shift
    goto :parse_args
)
if "%1"=="--no-run" (
    set RUN_EXE=false
    shift
    goto :parse_args
)
if "%1"=="--skip-cmake" (
    set SKIP_CMAKE=true
    shift
    goto :parse_args
)
echo Unknown option: %1
goto :help

:after_parse

REM 从测试目录派生输入输出路径
set INPUT_RIO=%TEST_DIR%\test.rio
set INPUT_LL=%TEST_DIR%\test.ll
set INPUT_OBJ=%TEST_DIR%\test.obj
set OUTPUT_EXE=%TEST_DIR%\test.exe
set ACTUAL_TXT=%TEST_DIR%\actual.txt

echo ========================================
echo RCC Build Script
echo ========================================
echo Test directory: %TEST_DIR%
echo Input file:     %INPUT_RIO%
echo LLVM IR file:   %INPUT_LL%
echo Object file:    %INPUT_OBJ%
echo Output exe:     %OUTPUT_EXE%
echo Actual output:  %ACTUAL_TXT%
echo Run after build: %RUN_EXE%
echo Skip CMake build: %SKIP_CMAKE%
echo ========================================
echo.

REM 步骤0: CMake 构建 RCC 编译器
if "%SKIP_CMAKE%"=="false" (
    echo [0/5] Building RCC compiler with CMake...
    echo Build directory: %CMAKE_BUILD_DIR%
    echo Target: %CMAKE_TARGET%
    echo Using 10 parallel jobs

    if not exist "%CMAKE_BUILD_DIR%" (
        echo WARNING: Build directory not found: %CMAKE_BUILD_DIR%
        echo Please ensure CMake has been configured first
        echo Run: cmake -B %CMAKE_BUILD_DIR% -G "Ninja" or similar
        echo.
        set /p continue="Continue anyway? (y/n): "
        if /i not "!continue!"=="y" exit /b 1
    )

    %CMAKE_EXE% --build %CMAKE_BUILD_DIR% --target %CMAKE_TARGET% -j 10
    if %errorlevel% neq 0 (
        echo ERROR: CMake build failed!
        exit /b %errorlevel%
    )
    echo SUCCESS: RCC compiler built successfully
    echo.
) else (
    echo [0/5] Skipping CMake build (--skip-cmake specified)
    echo.
)

REM 检查 RCC 编译器是否存在
if not exist "%RCC_EXE%" (
    echo ERROR: RCC compiler not found: %RCC_EXE%
    echo Please build the compiler first or use --skip-cmake if already built
    exit /b 1
)

REM 检查输入文件是否存在
if not exist "%INPUT_RIO%" (
    echo ERROR: Input file not found: %INPUT_RIO%
    exit /b 1
)

REM 步骤1: 生成LLVM IR
echo [1/5] Generating LLVM IR from %INPUT_RIO%...
%RCC_EXE% -ir --p "%INPUT_RIO%" --ir-o "%INPUT_LL%" 2>&1
if %errorlevel% neq 0 (
    echo ERROR: LLVM IR generation failed!
    exit /b %errorlevel%
)
echo SUCCESS: LLVM IR generated to %INPUT_LL%
echo.

REM 步骤2: 生成目标文件
echo [2/5] Generating object file from %INPUT_LL%...
%LLC_EXE% -filetype=obj -O2 "%INPUT_LL%" -o "%INPUT_OBJ%"
if %errorlevel% neq 0 (
    echo ERROR: Object file generation failed!
    exit /b %errorlevel%
)
echo SUCCESS: Object file generated to %INPUT_OBJ%
echo.

REM 步骤3: 链接生成可执行文件
echo [3/5] Linking to generate executable...
%GXX_EXE% -O2 "%INPUT_OBJ%" -o "%OUTPUT_EXE%"
if %errorlevel% neq 0 (
    echo ERROR: Linking failed!
    exit /b %errorlevel%
)
echo SUCCESS: Executable generated to %OUTPUT_EXE%
echo.

REM 步骤4: 运行可执行文件并将输出保存到 actual.txt
if "%RUN_EXE%"=="true" (
    echo [4/5] Running executable...
    echo ========================================
    "%OUTPUT_EXE%" > "%ACTUAL_TXT%" 2>&1
    echo ========================================
    echo.
    echo Output saved to %ACTUAL_TXT%
    echo Execution completed!
) else (
    echo [4/5] Skipping execution (--no-run specified)
)

echo.
echo ========================================
echo Build completed successfully!
echo ========================================
goto :eof

:help
echo Usage: %~nx0 [options]
echo.
echo Options:
echo   -i ^<test_dir^>    Test directory containing test.rio (default: .\tests\test_1_builtins)
echo   --no-run           Skip running the executable after building
echo   --skip-cmake       Skip CMake build step (use existing RCC.exe)
echo   -h, --help, /?     Show this help message
echo.
echo Examples:
echo   %~nx0                                      Build test_1_builtins with default settings
echo   %~nx0 -i .\tests\test_6_functions          Build and run test_6_functions
echo   %~nx0 -i .\tests\test_9_integration --no-run   Build only, don't run
echo   %~nx0 -i .\tests\test_1_builtins --skip-cmake   Skip rebuilding RCC compiler
echo.
goto :eof

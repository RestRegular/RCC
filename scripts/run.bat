@echo off
setlocal enabledelayedexpansion

REM 设置默认路径变量
set RCC_EXE=.\release\RCC.exe
set LLC_EXE=D:\repositories\llvm-project\build\bin\llc.exe
set GXX_EXE=g++.exe

REM 默认输入输出文件
set INPUT_RIO=.\tests\test_builtins.rio
set INPUT_LL=.\tests\irs\test.ll
set INPUT_OBJ=.\tests\irs\test.obj
set OUTPUT_EXE=.\tests\exes\test.exe
set RUN_EXE=true

REM 显示帮助信息
if "%1"=="/?" goto :help
if "%1"=="-h" goto :help
if "%1"=="--help" goto :help

REM 解析命令行参数
:parse_args
if "%1"=="" goto :after_parse
if "%1"=="-i" (
    set INPUT_RIO=%2
    shift
    shift
    goto :parse_args
)
if "%1"=="-o" (
    set OUTPUT_EXE=%2
    shift
    shift
    goto :parse_args
)
if "%1"=="--no-run" (
    set RUN_EXE=false
    shift
    goto :parse_args
)
if "%1"=="-ir-o" (
    set INPUT_LL=%2
    shift
    shift
    goto :parse_args
)
echo Unknown option: %1
goto :help

:after_parse

REM 从输入文件名自动生成输出文件名（如果未通过 -o 指定）
if "%OUTPUT_EXE%"==".\tests\exes\test.exe" (
    if not "%INPUT_RIO%"==".\tests\test_builtins.rio" (
        REM 提取文件名（不含扩展名）
        for %%f in ("%INPUT_RIO%") do set BASENAME=%%~nf
        set OUTPUT_EXE=.\tests\exes\!BASENAME!.exe
        set INPUT_LL=.\tests\irs\!BASENAME!.ll
        set INPUT_OBJ=.\tests\irs\!BASENAME!.obj
    )
)

echo ========================================
echo RCC Build Script
echo ========================================
echo Input file:   %INPUT_RIO%
echo Output exe:   %OUTPUT_EXE%
echo LLVM IR file: %INPUT_LL%
echo Object file:  %INPUT_OBJ%
echo Run after build: %RUN_EXE%
echo ========================================
echo.

REM 检查输入文件是否存在
if not exist "%INPUT_RIO%" (
    echo ERROR: Input file not found: %INPUT_RIO%
    exit /b 1
)

REM 创建输出目录（如果不存在）
for %%i in ("%INPUT_LL%") do if not exist "%%~dpi" mkdir "%%~dpi"
for %%i in ("%OUTPUT_EXE%") do if not exist "%%~dpi" mkdir "%%~dpi"

REM 步骤1: 生成LLVM IR
echo [1/4] Generating LLVM IR from %INPUT_RIO%...
%RCC_EXE% -ir --p "%INPUT_RIO%" --ir-o "%INPUT_LL%"
if %errorlevel% neq 0 (
    echo ERROR: LLVM IR generation failed!
    exit /b %errorlevel%
)
echo SUCCESS: LLVM IR generated to %INPUT_LL%
echo.

REM 步骤2: 生成目标文件
echo [2/4] Generating object file from %INPUT_LL%...
%LLC_EXE% -filetype=obj -O2 "%INPUT_LL%" -o "%INPUT_OBJ%"
if %errorlevel% neq 0 (
    echo ERROR: Object file generation failed!
    exit /b %errorlevel%
)
echo SUCCESS: Object file generated to %INPUT_OBJ%
echo.

REM 步骤3: 链接生成可执行文件
echo [3/4] Linking to generate executable...
%GXX_EXE% -O2 "%INPUT_OBJ%" -o "%OUTPUT_EXE%"
if %errorlevel% neq 0 (
    echo ERROR: Linking failed!
    exit /b %errorlevel%
)
echo SUCCESS: Executable generated to %OUTPUT_EXE%
echo.

REM 步骤4: 运行可执行文件
if "%RUN_EXE%"=="true" (
    echo [4/4] Running executable...
    echo ========================================
    "%OUTPUT_EXE%"
    echo ========================================
    echo.
    echo Execution completed!
) else (
    echo [4/4] Skipping execution (--no-run specified)
)

echo.
echo Build completed successfully!
goto :eof

:help
echo Usage: %~nx0 [options]
echo.
echo Options:
echo   -i ^<file^>        Input .rio file (default: .\tests\test_builtins.rio)
echo   -o ^<file^>        Output executable file (default: auto-generated from input name)
echo   -ir-o ^<file^>     Output LLVM IR file (default: auto-generated from input name)
echo   --no-run           Skip running the executable after building
echo   -h, --help, /?     Show this help message
echo.
echo Examples:
echo   %~nx0                                      Build with default settings
echo   %~nx0 -i myfile.rio                        Build myfile.rio
echo   %~nx0 -i myfile.rio -o myprogram.exe       Build with custom output name
echo   %~nx0 -i myfile.rio --no-run               Build only, don't run
echo.
goto :eof
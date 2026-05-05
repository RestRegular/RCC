@echo off
cls
echo ========================================
echo RCC Test Runner
echo ========================================

setlocal enabledelayedexpansion

REM 获取脚本所在目录
set "script_dir=%~dp0"
set "runner=%script_dir%run.bat"

if not exist "%runner%" (
    echo ERROR: runner not found: %runner%
    exit /b 1
)

REM 设置测试根目录
set "tests_root=%script_dir%..\tests"

REM 初始化计数器
set PASS_COUNT=0
set FAIL_COUNT=0
set TOTAL_COUNT=0

echo Tests root: %tests_root%
echo Runner:     %runner%
echo ========================================
echo.

REM 遍历所有 test_* 目录
for /D %%D in ("%tests_root%\test_*") do (
    set "test_dir=%%D"
    set "test_name=%%~nxD"
    set "expected=%%D\expected.txt"
    set "actual=%%D\actual.txt"

    REM 检查 expected.txt 是否存在
    if not exist "!expected!" (
        echo [SKIP] !test_name! - expected.txt not found
        echo.
    ) else (
        set /a TOTAL_COUNT+=1

        echo [RUN]  !test_name!...

        REM 运行 run.bat --skip-cmake
        call "%runner%" -i "!test_dir!" --skip-cmake > "!test_dir!\build.log" 2>&1

        if errorlevel 1 (
            echo [FAIL] !test_name! - build/run error
            type "!test_dir!\build.log"
            set /a FAIL_COUNT+=1
        ) else (
            REM 检查 actual.txt 是否生成
            if not exist "!actual!" (
                echo [FAIL] !test_name! - actual.txt not generated
                set /a FAIL_COUNT+=1
            ) else (
                REM 比较 actual.txt 和 expected.txt
                fc /W "!expected!" "!actual!" > nul 2>&1
                if errorlevel 1 (
                    echo [FAIL] !test_name! - output mismatch
                    echo --- Expected ---
                    type "!expected!"
                    echo --- Actual ---
                    type "!actual!"
                    echo ----------------
                    set /a FAIL_COUNT+=1
                ) else (
                    echo [PASS] !test_name!
                    set /a PASS_COUNT+=1
                )
            )
        )
        echo.
    )
)

REM 输出汇总
echo ========================================
echo Test Summary
echo ========================================
echo Total: %TOTAL_COUNT%
echo Pass:  %PASS_COUNT%
echo Fail:  %FAIL_COUNT%
echo ========================================

if %FAIL_COUNT% gtr 0 (
    echo RESULT: FAILED
    exit /b 1
) else (
    echo RESULT: ALL PASSED
    exit /b 0
)

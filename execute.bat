@echo off
cls
color 0A
set CMAKE_PATH=D:\soft\Clion\CLion-2024.2.3\bin\cmake\win\x64\bin\cmake.exe
set COMMAND_OUTPUT_PREFIX="^>^>^>"
set COMMAND_INPUT_PREFIX="^<^<^< "

set "BLUE_COLOR=\033[34m"
set "RESET_COLOR=\033[0m"

set PROJECT_TARGET=RCC

set PROJECT_DIR=D:\ClionProjects\%PROJECT_TARGET%
set BUILD_DIR=%PROJECT_DIR%\cmake-build-debug
set EXECUTABLE_PATH=%BUILD_DIR%\%PROJECT_TARGET%.exe
echo %COMMAND_OUTPUT_PREFIX:"=% Building project...
"%CMAKE_PATH%" --build %BUILD_DIR% --target %PROJECT_TARGET% -j 10
color 07
cls
python .\colorful_echo.py %COMMAND_OUTPUT_PREFIX:"=% %EXECUTABLE_PATH% %*
%EXECUTABLE_PATH% %*

setlocal

set RVM_COMMAND=RVM --t D:\ClionProjects\RVM\test\test.ra

<nul set /p=%COMMAND_INPUT_PREFIX:"=%
set /p PARAMS=

cls
python .\colorful_echo.py %COMMAND_OUTPUT_PREFIX:"=% %RVM_COMMAND% %PARAMS%
RVM --t D:\ClionProjects\RVM\test\test.ra %PARAMS%

endlocal
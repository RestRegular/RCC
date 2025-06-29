@echo off
cls
color 0A
set CMAKE_PATH=D:\soft\Clion\CLion-2024.2.3\bin\cmake\win\x64\bin\cmake.exe
set COMMAND_PREFIX="^>^>^>"

set PROJECT_TARGET=RCC

set PROJECT_DIR=D:\ClionProjects\%PROJECT_TARGET%
set BUILD_DIR=%PROJECT_DIR%\cmake-build-debug
set EXECUTABLE_PATH=%BUILD_DIR%\%PROJECT_TARGET%.exe
echo %COMMAND_PREFIX:"=% Building project...
"%CMAKE_PATH%" --build %BUILD_DIR% --target %PROJECT_TARGET% -j 10
color 07
cls
echo %COMMAND_PREFIX:"=% %EXECUTABLE_PATH% %*
%EXECUTABLE_PATH% %*
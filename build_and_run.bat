@REM E:\CodeLve\build_and_run.bat
@echo off
echo CodeLve Build and Run Script
echo ===========================

REM Create build directory if it doesn't exist
if not exist build mkdir build

REM Navigate to build directory
cd build

REM Configure with CMake
echo Configuring with CMake...
cmake -G "Visual Studio 17 2022" -A x64 ..
if %ERRORLEVEL% NEQ 0 (
    echo CMake configuration failed.
    cd ..
    pause
    exit /b %ERRORLEVEL%
)

REM Build the project
echo Building CodeLve...
cmake --build . --config Release
if %ERRORLEVEL% NEQ 0 (
    echo Build failed.
    cd ..
    pause
    exit /b %ERRORLEVEL%
)

REM Run the application
echo Starting CodeLve...
start "" "bin\Release\CodeLve.exe"

REM Return to root directory
cd ..
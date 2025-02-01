@echo off
REM Build script for CodeLve

echo CodeLve Build Script
echo ====================

REM Create build directory if it doesn't exist
if not exist build mkdir build

REM Navigate to build directory
cd build

REM Configure with CMake
echo Configuring with CMake...
cmake -G "Visual Studio 17 2022" -A x64 ..

REM Build the project
echo Building CodeLve...
cmake --build . --config Release

REM Return to root directory
cd ..

echo Build completed!
echo Executable location: build\bin\Release\CodeLve.exe

pause
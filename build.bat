@echo off
REM Modified Build script for CodeLve with filtered output

echo CodeLve Build Script - Filtered Output
echo =====================================

REM Create build directory if it doesn't exist
if not exist build mkdir build

REM Navigate to build directory
cd build

REM Configure with CMake
echo Configuring with CMake...
cmake -G "Visual Studio 17 2022" -A x64 .. > cmake_config.log 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo CMake configuration failed. See cmake_config.log for details.
    cd ..
    exit /b %ERRORLEVEL%
)

REM Build the project with filtered output
echo Building CodeLve...
cmake --build . --config Release > build_full.log 2>&1

REM Extract just unique error types to a summary
echo Extracting error summary...
findstr /C:"error C" build_full.log | findstr /V /C:"note:" | sort /UNIQUE > build_errors.log

REM Show summary of errors (count per error type)
echo.
echo Error Summary:
for /F "tokens=2 delims=C" %%G in (build_errors.log) do (
    findstr /C:"error C%%G" build_full.log | find /C "error C%%G"
    findstr /C:"error C%%G" build_full.log | findstr /N /C:"error C%%G" | findstr /B "1:" 
)

REM Return to root directory
cd ..

echo.
echo Build logs are available in the build directory:
echo - build\cmake_config.log - CMake configuration log
echo - build\build_full.log - Complete build output with all errors
echo - build\build_errors.log - Unique error codes

if exist build\bin\Release\CodeLve.exe (
    echo.
    echo Build completed with warnings/errors.
    echo Executable location: build\bin\Release\CodeLve.exe
) else (
    echo.
    echo Build failed. Check logs for details.
)

pause
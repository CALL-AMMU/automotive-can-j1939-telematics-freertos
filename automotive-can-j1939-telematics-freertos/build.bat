@echo off
echo =========================================================
echo   Compiling Automotive CAN (J1939) Telematics Gateway
echo =========================================================
gcc -std=c99 -Wall -Wextra -Iinclude src/j1939.c src/telematics_gateway.c src/main.c -o can_telematics.exe
if %ERRORLEVEL% EQU 0 (
    echo.
    echo Compilation successful! Executable created: can_telematics.exe
    echo Running real-time telemetry simulation...
    echo.
    can_telematics.exe
) else (
    echo.
    echo Compilation failed!
)
pause

@echo off
echo =========================================================
echo   Pushing Automotive CAN Telematics Gateway to GitHub
echo =========================================================
echo.
cd /d "D:\Amit_Advanced_Projects\automotive-can-j1939-telematics-freertos"

echo Checking Git status...
git status
echo.

echo Staging all project files...
git add .
git commit -m "feat: initial release of Automotive CAN J1939 Telematics Gateway & BMS Monitor"
echo.

echo Pushing to GitHub (https://github.com/CALL-AMMU/automotive-can-j1939-telematics-freertos)...
echo [NOTE: Please ensure you have created the repository 'automotive-can-j1939-telematics-freertos' on github.com/new]
git push -u origin main

echo.
echo =========================================================
echo   Done!
echo =========================================================
pause

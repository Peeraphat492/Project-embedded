@echo off
echo ========================================
echo  Smart Room Reservation Backend
echo ========================================
echo.

cd /d "%~dp0"

echo [1/4] Checking current directory...
echo Current directory: %cd%
echo.

echo [2/4] Testing Node.js...
node --version
if %errorlevel% neq 0 (
    echo ERROR: Node.js not found!
    echo Please install Node.js from https://nodejs.org/
    echo Make sure to restart your computer after installation
    pause
    exit /b 1
)

echo [3/4] Testing npm...
npm --version
if %errorlevel% neq 0 (
    echo ERROR: npm not found!
    echo Please reinstall Node.js
    pause
    exit /b 1
)

echo [4/4] Starting server...
echo Server will be available at: http://localhost:3000
echo Health check: http://localhost:3000/api/health
echo.
echo Press Ctrl+C to stop the server
echo ========================================
echo.

npm start

echo.
echo Server stopped. Press any key to exit...
pause > nul
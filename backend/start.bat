@echo off
echo Starting Smart Room Reservation Backend...
echo.

cd /d "%~dp0"

echo Checking Node.js...
node --version >nul 2>&1
if %errorlevel% neq 0 (
    echo Error: Node.js is not installed or not in PATH
    echo Please install Node.js from https://nodejs.org/
    pause
    exit /b 1
)

echo Installing dependencies...
if not exist "node_modules" (
    npm install
    if %errorlevel% neq 0 (
        echo Error: Failed to install dependencies
        pause
        exit /b 1
    )
)

echo.
echo Starting server...
echo Backend will be available at http://localhost:3000
echo API endpoints at http://localhost:3000/api
echo.
echo Press Ctrl+C to stop the server
echo.

npm start
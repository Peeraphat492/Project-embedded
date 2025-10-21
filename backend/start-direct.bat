@echo off
echo Starting Backend with Node directly...
echo.

cd /d "%~dp0"

echo Testing Node.js...
node --version
if %errorlevel% neq 0 (
    echo Node.js not found!
    pause
    exit
)

echo Starting server directly with Node.js...
node server.js

pause
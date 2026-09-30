@echo off
setlocal
cd /d "%~dp0"
where g++ >nul 2>nul
if errorlevel 1 (
  echo g++ was not found. Add your MinGW bin folder to PATH first.
  exit /b 1
)
g++ -std=c++17 -O2 -Wall -Wextra -municode -mwindows -static main.cpp calculatorengine.cpp -o Calculator.exe -lgdi32 -luser32
if errorlevel 1 exit /b 1
echo Build succeeded. Run .\Calculator.exe

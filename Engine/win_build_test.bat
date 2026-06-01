@echo off

if exist "out" ( rmdir /s /q "out" )

cmake -B out/build/Debug -G Ninja -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_C_COMPILER=clang

if %errorlevel% equ 0 (
    cmake --build out/build/Debug
)

if %errorlevel% equ 0 (
    start cmd /K "cd /d "%~dp0out\build\Debug" && ByteSeal.exe"
)
else(
    pause
)

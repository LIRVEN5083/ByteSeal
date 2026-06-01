@echo off

if exist "out" ( rmdir /s /q "out" )

cmake -B out/build/Debug -G Ninja -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_C_COMPILER=clang && cmake --build out/build/Debug

if %errorlevel% neq 0 (
    pause
)
	

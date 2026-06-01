@echo off
cmake -B out/build/Debug -G Ninja -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_C_COMPILER=clang && cmake --build out/build/Debug

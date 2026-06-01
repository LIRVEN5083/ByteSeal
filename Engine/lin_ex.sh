set -e

rm -rf out

CC=clang CXX=clang++ cmake -B out/build/Debug -G "Ninja"

cmake --build out/build/Debug

cd out/build/Debug
./ByteSeal

read -p

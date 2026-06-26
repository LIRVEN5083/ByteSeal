set -e

rm -rf out

CC=clang CXX=clang++ cmake -B out -G "Ninja"

cmake --build out

cd out
./ByteSeal

read -p

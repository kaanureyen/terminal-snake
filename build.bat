@echo off
echo "Creating build folder"
mkdir build
echo "Compiling ./src/snake.c to ./build/snake.exe"
gcc ./src/snake.c -o ./build/snake.exe -lncurses -DNCURSES_STATIC -Wall -Wpedantic -Wextra -O3 -DNDEBUG
echo "Running ./build/snake.exe"
start build\snake.exe
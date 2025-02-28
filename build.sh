if [ -d build ]; then
    echo "./build/ found, deleting"
    rm -r ./build
fi

echo "./build/ is being created"
mkdir build

echo "Compiling ./src/snake.c to ./build/snake"
clang -DNDEBUG ./src/snake.c -o ./build/snake -O3 -lncurses -DNCURSES_STATIC -Wall -Wpedantic -Wextra

echo "Running ./build/snake"
./build/snake
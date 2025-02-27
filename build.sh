if [ -d build ]; then
    rm -r ./build
    mkdir build
    echo "./build/ found"
else
    mkdir build
    echo "./build/ created"
fi

echo "Compiling ./src/snake.c"
clang ./src/snake.c -o ./build/snake -Wall -Wpedantic -Wextra

echo "Running ./build/snake"
./build/snake
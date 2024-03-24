#!/usr/bin/env sh

set -xe

SRC="./src/main.c"
CFLAGS="-Wall -Wextra -ggdb -I./include/raylib-5.0"
LIBS="-lm -L./lib -l:libraylib.a"

if [ $1 = "build" ]; then
    mkdir -p ./build
    clang $CFLAGS -Wall -o ./build/main $SRC $LIBS

elif [ $1 = "run" ]; then
    mkdir -p ./build
    clang $CFLAGS -Wall -o ./build/main $SRC $LIBS && ./build/main

elif [ $1 = "clean" ]; then
    rm -rf ./build

else
    echo "Invalid command."

fi


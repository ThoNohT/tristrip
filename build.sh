#!/usr/bin/env sh

set -xe

SRC="./src/main.c"
CFLAGS="-Wall -Wextra -ggdb -I./raylib-5.0/src"
LIBS="-lm -ldl -lrt -L. -l:libraylib.a"

if [ $1 = "build" ]; then
    clang $CFLAGS -Wall -o ./main $SRC $LIBS

elif [ $1 = "run" ]; then
    clang $CFLAGS -Wall -o ./main $SRC $LIBS && ./main

else
    echo "Invalid command."

fi


#!/usr/bin/env sh

SRC="./src/main.c"
CFLAGS="-Wall -Wextra -ggdb"
LIBS="-lm -ldl lrt -I./raylib-5.5/src -L. -l:libraylib.a"

if [ $1 = "build" ]; then
    clang $CFLAGS -Wall -o ./main $SRC $LIBS

elif [ $1 = "run" ]; then
    clang $CFLAGS -Wall -o ./main $SRC $LIBS && ./main

else
    echo "Invalid command."

fi


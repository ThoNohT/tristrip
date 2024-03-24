#!/usr/bin/env sh

if test -f ./bld; then
    ./bld $@
else
    echo "Bootstrapping build system."
    cc -o ./bld ./bld.c
    ./bld $@
fi

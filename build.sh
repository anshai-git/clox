#!/bin/sh

gcc -g        \
    main.c    \
    memory.h  \
    memory.c  \
    chunk.h   \
    chunk.c   \
    common.h  \
    debug.h   \
    debug.c   \
    value.h   \
    value.c   \
    -o clox

# rm ./*.gch

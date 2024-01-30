#!/bin/sh

gcc -g          \
    main.c      \
    memory.h    \
    memory.c    \
    chunk.h     \
    chunk.c     \
    common.h    \
    debug.h     \
    debug.c     \
    value.h     \
    value.c     \
    vm.h        \
    vm.c        \
    compiler.h  \
    compiler.c  \
    scanner.h   \
    scanner.c   \
    object.h    \
    object.c    \
    -o clox

# rm ./*.gch

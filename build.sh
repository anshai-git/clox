# #!/bin/bash

set -e
SOURCES=(
  object.c
  compiler.c
  scanner.c
  vm.c
  value.c
  debug.c
  chunk.c   
  memory.c  
  main.c
)

echo "Compiling..."

clang -g -std=c99 -Wall -Wextra -O2 "${SOURCES[@]}" -o lox

echo "Build complete: ./$OUTPUT"

# #!/bin/bash

set -e
SOURCES=(
  debug.c
  chunk.c   
  memory.c  
  main.c
)

echo "Compiling..."

clang -std=c99 -Wall -Wextra -O2 "${SOURCES[@]}" -o lox

echo "Build complete: ./$OUTPUT"

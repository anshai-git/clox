# #!/bin/bash

set -e
SOURCES=(
  main.c
)

echo "Compiling..."

clang -std=c99 -Wall -Wextra -O2 "${SOURCES[@]}" -o lox

echo "Build complete: ./$OUTPUT"

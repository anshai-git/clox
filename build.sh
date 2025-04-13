# #!/bin/bash

set -e

# Name of the output executable
OUTPUT="lox"

# Compiler
CC=clang

# Compiler flags
CFLAGS="-std=c99 -Wall -Wextra -O2"

# Source files (order doesn't matter here, but can help readability)
SOURCES=(
  chunk.c
  compiler.c
  debug.c
  main.c
  memory.c
  object.c
  scanner.c
  table.c
  value.c
  vm.c
)

# Build command
echo "Compiling..."
$CC $CFLAGS "${SOURCES[@]}" -o $OUTPUT

echo "Build complete: ./$OUTPUT"

#include <stdio.h>
#include <stdlib.h>
#include "chunk.h"
#include "common.h"
#include "debug.h"

int main(int argc, const char** argv) {

  Chunk chunk;
  init_chunk(&chunk);
  int constant = add_constant(&chunk, 1.2);
  
  write_chunk(&chunk, OP_CONSTANT, 1);
  write_chunk(&chunk, constant, 1);
  write_chunk(&chunk, OP_RETURN, 2);
  disassemble_chunk(&chunk, "[TEST CHUNK]");

  free_chunk(&chunk);
  return 0;
}

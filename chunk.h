#ifndef clox_chunk_h
#define clox_chunk_h

#include "common.h"

typedef enum {
  OP_RETURN,
} Op_Code;

typedef struct {
  int count;
  int capacity;
  uint8_t* code;
} Chunk;

void init_chunk(Chunk* chunk);
void write_chunk(Chunk* chunk, uint8_t byte);

#endif // !clox_chunk_h


#ifndef clox_chunk_h
#define clox_chunk_h

#include "common.h"
#include "value.h"

// In our bytecode format, each instruction has a one-byte operation code
// (universally shortened to opcode). That number controls what kind of
// instruction we’re dealing with—add, subtract, look up variable, etc.
typedef enum {
  OP_CONSTANT,
  OP_NIL,
  OP_TRUE,
  OP_FALSE,
  OP_EQUAL,
  OP_GREATER,
  OP_LESS,
  OP_ADD,
  OP_SUBTRACT,
  OP_MULTIPLY,
  OP_DIVIDE,
  OP_NOT,
  OP_NEGATE,
  OP_RETURN,
} OpCode;

// Bytecode is a series of instructions.

// The Bytecode allows instructions to have operands.
// These are stored as binary data immediately after the opcode in the instruction stream
// and let us parameterize what the instruction does.
//
// OP_RETURN
// [01] <- opcode :: 1 byte
//
// OP_CONSTANT
// [00][23] <- opcode and constant index :: 2 bytes
// 
// Each opcode determines how many operand bytes it has and what they mean.
// For example, a simple operation like “return” may have no operands,
// where an instruction for “load local variable” needs an operand to identify which
// variable to load. Each time we add a new opcode to clox, we specify what its
// operands look like—its instruction format.
//
// Bytecode instruction operands are not the same as the operands passed to an arithmetic operator.
// Arithmetic operand values are tracked separately.
// Instruction operands are a lower-level notion that modify how the bytecode instruction itself behaves.
//
// EXAMPLE: 
// `int add_constant(Chunk* chunk, Value value);`
// In this case, OP_CONSTANT takes a single byte operand
// that specifies which constant to load from the chunk’s constant array.
//
// - We add the constant value itself to the chunk’s constant pool.
// - That returns the index of the constant in the array.
// - Then we write the constant instruction, starting with its opcode.
// - After that, we write the one-byte constant index operand.
typedef struct {
  int count;
  int capacity;

  //  In the chunk, we store a separate array of integers that parallels the bytecode.
  //  Each number in the array is the line number for the corresponding byte in the bytecode.
  //  When a runtime error occurs, we look up the line number
  //  at the same index as the current instruction’s offset in the code array.
  int* lines;
  // For small fixed-size values like integers, many instruction sets store the value directly
  // in the code stream right after the opcode. These are called immediate
  // instructions because the bits for the value are immediately after the opcode.
  uint8_t* code;

  ValueArray constants;
} Chunk;

void init_chunk(Chunk* chunk);
// This function can write opcodes or operands as well.
// It’s all raw bytes as far as that function is concerned.
void write_chunk(Chunk* chunk, uint8_t byte, int line);
void free_chunk(Chunk* chunk);
int add_constant(Chunk* chunk, Value value);

#endif

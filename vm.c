#include "vm.h"
#include "common.h"
#include "debug.h"
#include <stdio.h>

VM vm;

void init_vm() {}

void free_vm() {}

static InterpretResult run() {
#define READ_BYTE() (*vm.ip++)
// reads the next byte from the bytecode, treats the resulting number as an index,
// and looks up the corresponding Value in the chunk’s constant table.
#define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])

  for (;;) {
#ifdef DEBUG_TRACE_EXECUTION
    // Since disassembleInstruction() takes an integer byte offset and we store the
    // current instruction reference as a direct pointer, we first do a little pointer
    // math to convert ip back to a relative offset from the beginning of the bytecode.
    disassemble_instruction(vm.chunk, (int)(vm.ip - vm.chunk->code));
#endif
    uint8_t instruction;
    switch (instruction = READ_BYTE()) {
    case OP_CONSTANT: {
      Value constant = READ_CONSTANT();
      print_value(constant);
      printf("\n");
      break;
    }
    case OP_RETURN: {
      return INTERPRET_OK;
    }
    }
  }

#undef READ_BYTE
#undef READ_CONSTANT
}

InterpretResult interpret(Chunk* chunk) {
  vm.chunk = chunk;
  vm.ip = vm.chunk->code;
  return run();
}

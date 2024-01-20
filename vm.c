#include "vm.h"
#include "common.h"
#include "debug.h"
#include "compiler.h"
#include <stdio.h>

VM vm;

static void reset_stack() { vm.stack_top = vm.stack; }

void init_vm() { reset_stack(); }

void free_vm() {}

void push(Value value) {
  *vm.stack_top = value;
  vm.stack_top++;
}

Value pop() {
  vm.stack_top--;
  return *vm.stack_top;
}

// We have an outer loop that goes and goes.
// Each turn through that loop, we read and execute a single bytecode
// instruction.
//
// we have a single giant switch statement with a case for each opcode.
// The body of each case implements that opcode’s behavior.
static InterpretResult run() {

// Note that ip advances as soon as we read the opcode, before we’ve actually
// started executing the instruction. So, again, ip points to the next
// byte of code to be used.
#define READ_BYTE() (*vm.ip++)

#define BINARY_OP(op)                                                          \
  do {                                                                         \
    double b = pop();                                                          \
    double a = pop();                                                          \
    push(a op b);                                                              \
  } while (false)

// reads the next byte from the bytecode, treats the resulting number as an
// index, and looks up the corresponding Value in the chunk’s constant table.
#define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])

  for (;;) {
#ifdef DEBUG_TRACE_EXECUTION
    //  show the current contents of the stack before we interpret each
    //  instruction.
    printf(" ");
    for (Value* slot = vm.stack; slot < vm.stack_top; slot++) {
      printf("[");
      print_value(*slot);
      printf("]");
    }
    printf("\n");
    // Since disassemble_instruction() takes an integer byte offset and we store
    // the current instruction reference as a direct pointer, we first do a
    // little pointer math to convert ip back to a relative offset from the
    // beginning of the bytecode.
    disassemble_instruction(vm.chunk, (int)(vm.ip - vm.chunk->code));
#endif
    uint8_t instruction;
    switch (instruction = READ_BYTE()) {
    case OP_CONSTANT: {
      Value constant = READ_CONSTANT();
      push(constant);
      print_value(constant);
      printf("\n");
      break;
    }
    case OP_ADD: {
      BINARY_OP(+);
      break;
    }
    case OP_SUBTRACT: {
      BINARY_OP(-);
      break;
    }
    case OP_MULTIPLY: {
      BINARY_OP(*);
      break;
    }
    case OP_DIVIDE: {
      BINARY_OP(/);
      break;
    }
    case OP_NEGATE: {
      push(-pop());
      break;
    }
    case OP_RETURN: {
      print_value(pop());
      printf("\n");
      return INTERPRET_OK;
    }
    }
  }

#undef READ_BYTE
#undef READ_CONSTANT
#undef BINARY_OP
}

// First, we store the chunk being executed in the VM.
// Then we call run(), an internal helper function that actually runs the
// bytecode instructions.
InterpretResult interpret(const char* source) {
  compile(source);
  return INTERPRET_OK;
}

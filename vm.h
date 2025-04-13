#ifndef clox_vm_h
#define clox_vm_h

#include "chunk.h"
#include "value.h"
#include "table.h"

#define STACK_MAX 256

typedef struct {
  Chunk* chunk;
  // As the VM works its way through the bytecode,
  // it keeps track of where it is—the location of the instruction currently being executed.
  // We don’t use a local variable inside run() for this because eventually
  // other functions will need to access it.
  //
  // We use an actual real C pointer pointing right into the middle of the bytecode array
  // name ip that stands for Instruction Pointer
  //
  // We initialize ip by pointing it at the first byte of code in the chunk.
  // We haven’t executed that instruction yet, so ip points to the instruction about
  // to be executed.
  //
  // !! IP always points to the next instruction, not the one currently being handled.
  uint8_t* ip;
  Value stack[STACK_MAX];
  // The pointer points at the array element just past the element containing the top value on the stack.
  // It means we can indicate that the stack is empty by pointing at element zero in the array.
  //
  // EXAMPLE:
  // [1][2][3][4][5][ ] <- stack with elements: 1,2,3,4,5
  //                 |
  //              stack top
  //              
  // [ ][ ][ ][ ][ ][ ] <- empty stack
  //  |
  // stack top
  //
  Value* stack_top;

  // Linked list of objects for the VM to kepp track of.
  // Used for Garbage Collection
  Object* objects;

  // Internal Strings
  Table strings;

  // Since we want them to persist as long as clox is running, we store them right in the VM
  Table globals;
} VM;

typedef enum {
  INTERPRET_OK,
  INTERPRET_COMPILE_ERROR,
  INTERPRET_RUNTIME_ERROR
} InterpretResult;

extern VM vm;

void init_vm();
void free_vm();
InterpretResult interpret(const char* source);
void push(Value value);
Value pop();

#endif

#include "vm.h"
#include "common.h"
#include "compiler.h"
#include "debug.h"
#include "object.h"
#include "memory.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

VM vm;

static void reset_stack() { vm.stack_top = vm.stack; }

void init_vm() { 
  reset_stack();
  vm.objects = NULL;
}

void free_vm() {
  free_objects();
}

void push(Value value) {
  *vm.stack_top = value;
  vm.stack_top++;
}

Value pop() {
  vm.stack_top--;
  return *vm.stack_top;
}

static Value peek(int distance) {
  return vm.stack_top[-1 - distance];
}

// First, we calculate the length of the result string based on
// the lengths of the operands. We allocate a character array for
// the result and then copy the two halves in.
static void concatenate() {
  Object_String* b = AS_STRING(pop());
  Object_String* a = AS_STRING(pop());

  int length = a->length + b->length;
  char* chars = ALLOCATE(char, length + 1);
  memcpy(chars, a->chars, a->length);
  memcpy(chars + a->length, b->chars, b->length);
  chars[length] = '\0';

  Object_String* object = take_string(chars, length);
  push(OBJECT_VAL(object));
}

//  nil and false are falsey and every other value behaves like true.
static bool is_falsey(Value value) {
  return IS_NIL(value) || (IS_BOOL(value) && !AS_BOOL(value));
}

// the ... and va_list stuff let us pass an arbitrary number of arguments
// to runtimeError(). It forwards those on to vfprintf(), which is the
// flavor of printf() that takes an explicit va_list.
static void runtime_error(const char* format, ...) {
  va_list args;
  va_start(args, format);
  vfprintf(stderr, format, args);
  va_end(args);
  fputs("\n", stderr);

  
  // We look into the chunk’s debug line array using the current bytecode
  // instruction index minus one. That’s because the interpreter advances
  // past each instruction before executing it. So, at the point that we
  // call runtimeError(), the failed instruction is the previous one.
  size_t instruction = vm.ip - vm.chunk->code - 1;
  int line = vm.chunk->lines[instruction];
  fprintf(stderr, "[line %d] in script\n", line);
  reset_stack();
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

#define BINARY_OP(value_type, op)                                                          \
  do {                                                                         \
    if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) {                          \
      runtime_error("Operands must be numbers.");                              \
      return INTERPRET_RUNTIME_ERROR;                                          \
    }                                                                          \
    double b = AS_NUMBER(pop());                                                          \
    double a = AS_NUMBER(pop());                                                          \
    push(value_type(a op b));                                                              \
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
      case OP_ADD: {
        if (IS_STRING(peek(0)) && IS_STRING(peek(1))) {
          concatenate();
        } else if (IS_NUMBER(peek(0)) && IS_NUMBER(peek(1))) {
          double b = AS_NUMBER(pop());
          double a = AS_NUMBER(pop());
          push(NUMBER_VAL(a + b));
        } else {
          runtime_error("Operands must be two numbers or two strings.");
          return INTERPRET_RUNTIME_ERROR;
        }
        break;
      }
      case OP_CONSTANT: {
        Value constant = READ_CONSTANT();
        push(constant);
        print_value(constant);
        printf("\n");
        break;
      }
      case OP_NIL:      push(NIL_VAL); break;
      case OP_TRUE:     push(BOOL_VAL(true)); break;
      case OP_FALSE:    push(BOOL_VAL(false)); break;
      case OP_SUBTRACT: BINARY_OP(NUMBER_VAL, -); break;
      case OP_MULTIPLY: BINARY_OP(NUMBER_VAL, *); break;
      case OP_DIVIDE:   BINARY_OP(NUMBER_VAL, /); break;
      case OP_NOT:      push(BOOL_VAL(is_falsey(pop()))); break;
      case OP_GREATER:  BINARY_OP(BOOL_VAL, >); break;
      case OP_LESS:     BINARY_OP(BOOL_VAL, <); break;
      case OP_EQUAL: {
        Value b = pop();
        Value a = pop();
        push(BOOL_VAL(values_equal(a, b)));
        break;
      }
      case OP_NEGATE: {
        if (!IS_NUMBER(peek(0))) {
          runtime_error("Operand must be a number");
          return INTERPRET_RUNTIME_ERROR;
        }

        push(NUMBER_VAL(-AS_NUMBER(pop())));
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

// We create a new empty chunk and pass it over to the compiler.
// The compiler will take the user’s program and fill up the chunk with
// bytecode. If it does encounter an error, compile() returns false and we
// discard the unusable chunk.
//
// Otherwise, we send the completed chunk over to the VM to be executed.
// When the VM finishes, we free the chunk and we’re done
InterpretResult interpret(const char* source) {
  Chunk chunk;
  init_chunk(&chunk);

  if (!compile(source, &chunk)) {
    free_chunk(&chunk);
    return INTERPRET_COMPILE_ERROR;
  }

  vm.chunk = &chunk;
  vm.ip = vm.chunk->code;

  InterpretResult result = run();
  free_chunk(&chunk);

  return result;
}

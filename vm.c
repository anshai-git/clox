#include "vm.h"
#include "chunk.h"
#include "common.h"
#include "debug.h"
#include "value.h"
#include "compiler.h"
#include "memory.h"
#include "object.h"

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

VM vm;

static void reset_stack() { vm.stack_top = vm.stack; }

static Value peek(int distance) {
  return vm.stack_top[-1 - distance];
}

static bool is_falsey(Value value) {
  return IS_NIL(value) || (IS_BOOL(value) && !AS_BOOL(value));
}

static void concatenate() {
  printf("From concatenate\n");
  ObjString* a = AS_STRING(pop());
  ObjString* b = AS_STRING(pop());

  int length = a->length + b->length;
  char* chars = ALLOCATE(char, length + 1);
  memcpy(chars, a->chars, a->length);
  memcpy(chars + a->length, b->chars, b->length);
  chars[length] = '\0';

  ObjString* result = take_string(chars, length);
  push(OBJ_VAL(result));
}

static void runtime_error(const char* format, ...) {
  va_list args;
  va_start(args, format);
  vfprintf(stderr, format, args);
  va_end(args);
  fputs("\n", stderr);

  size_t instruction = vm.ip - vm.chunk->code - 1;
  int line = vm.chunk->lines[instruction];
  fprintf(stderr, "[line %d] in script\n", line);
  reset_stack();
}

void init_vm() { 
  reset_stack(); 
  vm.objects = NULL;
}

void free_vm() {
  free_objects();
}

static Interpret_Result run() {
#define READ_BYTE() (*vm.ip++)
#define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])
#define BINARY_OP(value_type, op)                                              \
  do {                                                                         \
    if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) {                          \
      runtime_error("Operands must be number.");                               \
      return INTERPRET_RUNTIME_ERROR;                                          \
    }                                                                          \
    double b = AS_NUMBER(pop());                                               \
    double a = AS_NUMBER(pop());                                               \
    push(value_type(a op b));                                                  \
  } while (false)                                                              

  for (;;) {
#ifdef DEBUG_TRACE_EXECUTION
    printf(" ");
    for (Value* slot = vm.stack; slot < vm.stack_top; slot++) {
      printf("[ ");
      print_value(*slot);
      printf(" ]");
    }
    printf("\n");

    disassemble_instruction(vm.chunk, (int)(vm.ip - vm.chunk->code));
#endif
    uint8_t instruction;
    switch (instruction = READ_BYTE()) {
    case OP_GREATER:    BINARY_OP(BOOL_VAL, >); break;
    case OP_LESS:       BINARY_OP(BOOL_VAL, <); break;
    case OP_ADD: {
      print_value(peek(0));
      print_value(peek(1));
      if (IS_STRING(peek(0)) && IS_STRING(peek(1))) {
        concatenate();
      } else if (IS_NUMBER(peek(0)) && IS_NUMBER(peek(1))) {
        BINARY_OP(NUMBER_VAL, +); break;
      }
    };
    case OP_SUBTRACT:   BINARY_OP(NUMBER_VAL, -); break;
    case OP_MULTIPLY:   BINARY_OP(NUMBER_VAL, *); break;
    case OP_DIVIDE:     BINARY_OP(NUMBER_VAL, /); break;
    case OP_NOT: push(BOOL_VAL(is_falsey(pop()))); break;
    case OP_NEGATE:  {
      if (!IS_NUMBER(peek(0))) {
        runtime_error("Operand must be a number.");
        return INTERPRET_RUNTIME_ERROR;
      }
      push (NUMBER_VAL(-AS_NUMBER(pop())));
      break;
    }

    case OP_CONSTANT: {
      Value constant = READ_CONSTANT();
      push(constant);
      print_value(constant);
      printf("\n");
      break;
    }
    case OP_NIL: push(NIL_VAL); break;
    case OP_TRUE: push(BOOL_VAL(true)); break;
    case OP_FALSE: push(BOOL_VAL(false)); break;
    case OP_EQUAL: {
      Value b = pop();
      Value a = pop();
      push(BOOL_VAL(values_equal(a, b)));
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

Interpret_Result interpret(const char* source) {
  Chunk chunk;
  init_chunk(&chunk);

  if (!compile(source, &chunk)) {
    free_chunk(&chunk);
    return INTERPRET_COMPLE_ERROR;
  }

  vm.chunk = &chunk;
  vm.ip = vm.chunk->code;

  Interpret_Result result = run();

  free_chunk(&chunk);
  return result;
}

void push(Value value) {
  *vm.stack_top = value;
  vm.stack_top += 1;
}

Value pop() {
  vm.stack_top -= 1;
  return *vm.stack_top;
}

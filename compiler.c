#include <stdio.h>
#include <stdlib.h>

#include "common.h"
#include "compiler.h"
#include "scanner.h"

typedef struct {
  Token current;
  Token previous;
  bool had_error;
  bool panic_mode;
} Parser;

Parser parser;
Chunk* compiling_chunk;

bool compile(const char* source, Chunk* chunk);

// FRONT END
static void advance();
static void error_at_current(const char* message);
static void error_at(Token* token, const char* message);
static void error(const char* message);
static void consume(TokenType type, const char* message);

// BACK END
static void emit_byte(uint8_t byte);
static void emit_bytes(uint8_t byte_1, uint8_t byte_2);
static void end_compiler();

static void expression();
static void grouping();
static void unary();
static void number();
static void emit_constant(Value value);
static uint8_t make_constant(Value value);

// FE FUNCTIONS START
static void error_at(Token* token, const char* message) {
  if (parser.panic_mode)
    return;
  parser.panic_mode = true;
  fprintf(stderr, "[line %d] Error", token->line);

  if (token->type == TOKEN_EOF) {
    fprintf(stderr, " at end");
  } else if (token->type == TOKEN_ERROR) {
    // Nothing
  } else {
    fprintf(stderr, " at '%.*s'", token->length, token->start);
  }

  fprintf(stderr, ": %s\n", message);
  parser.had_error = true;
}

static void error(const char* message) { error_at(&parser.previous, message); }

static void error_at_current(const char* message) {
  error_at(&parser.current, message);
}

static void consume(TokenType type, const char* message) {
  if (parser.current.type == type) {
    advance();
    return;
  }

  error_at_current(message);
}

static void advance() {
  parser.previous = parser.current;

  for (;;) {
    parser.current = scan_token();
    if (parser.current.type != TOKEN_ERROR)
      break;

    error_at_current(parser.current.start);
  }
}
// FE FUNCTIONS END

// BE FUNCTIONS START

//  It writes the given byte, which may be an opcode or an operand to an
//  instruction.
// It sends in the previous token’s line information so that runtime errors are
// associated with that line.
static void emit_byte(uint8_t byte) {
  write_chunk(current_chunk(), byte, parser.previous.line);
}

static void emit_bytes(uint8_t byte_1, uint8_t byte_2) {
  enit_byte(byte_1);
  enit_byte(byte_2);
}

static Chunk* current_chunk() { return compiling_chunk; }

static void end_compiler() { emit_byte(OP_RETURN); }

static void expression() {
  // NOT IMPLEMENTED
}

static void number() {
  double value = strtod(parser.previous.start, NULL);
  emit_constant(value);
}

static void unary() {
  TokenType operator_type = parser.previous.type;

  // Compile the operand
  expression();

  // Emit the operator instruction
  switch (operator_type) {
  case TOKEN_MINUS: {
    emit_byte(OP_NEGATE);
    break;
  }
  default:
    return;
  }
}

static void emit_constant(Value value) {
  emit_bytes(OP_CONSTANT, make_constant(value));
}

static uint8_t make_constant(Value value) {
  int constant = add_constant(current_chunk(), value);
  if (constant > UINT8_MAX) {
    error("Too Many constants in one chunk.");
    return 0;
  }

  return (uint8_t)constant;
}

// we assume the initial ( has already been consumed. We recursively call back
// into expression() to compile the expression between the parentheses, then
// parse the closing ) at the end.
static void grouping() {
  expression();
  consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression");
}

// BE FUNCTIONS END

bool compile(const char* source, Chunk* chunk) {
  init_scanner(source);
  compiling_chunk = chunk;

  // SYNCHRONIZATION POINT

  // We want to avoid error cascades.
  // If the user has a mistake in their code and the parser gets confused
  // about where it is in the grammar, we don’t want it to spew out a whole pile
  // of meaningless knock-on errors after the first one.

  // To solve this:
  // We add a flag to track whether we’re currently in panic mode.
  // (@SEE_VARIBALE :: panice_mode) When an error occurs, we set it.
  // (@SEE_FUNCTION :: error_at)

  // After that, we go ahead and keep compiling as normal as if the error never
  // occurred. The bytecode will never get executed, so it’s harmless to keep on
  // trucking.

  // The trick is that while the panic mode flag is set, we simply suppress
  // any other errors that get detected. (@SEE_FUNCTION :: error_at)

  // Panic mode ends when the parser reaches a synchronization point. (where the
  // flags get reset)
  parser.had_error = false;
  parser.panic_mode = false;

  advance();
  expression();
  consume(TOKEN_EOF, "Expect end of expression.");

  end_compiler();
  return !parser.had_error;
}

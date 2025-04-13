#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "common.h"
#include "compiler.h"
#include "scanner.h"
#include "object.h"
#include "value.h"

#ifdef DEBUG_PRINT_CODE
#include "debug.h"
#endif

typedef struct {
  Token current;
  Token previous;
  bool had_error;
  bool panic_mode;
} Parser;

typedef enum {
  // LOWEST PRECENDENCE
  PREC_NONE,
  PREC_ASSIGNMENT, // =
  PREC_OR,         // or
  PREC_AND,        // and
  PREC_EQUALITY,   // == !=
  PREC_COMPARISON, // < > <= >=
  PREC_TERM,       // + -
  PREC_FACTOR,     // * /
  PREC_UNARY,      // ! -
  PREC_CALL,       // . ()
  PREC_PRIMARY
  // HIGHEST PRECENDENCE
} Precendence;

typedef void (*ParseFn)(bool can_assign);

typedef struct {
  ParseFn prefix;
  ParseFn infix;
  Precendence precendence;
} ParseRule;


static void advance();
static void error_at_current(const char* message);
static void error_at(Token* token, const char* message);
static void error(const char* message);
static void consume(TokenType type, const char* message);
static void literal(bool can_assign);

static void emit_byte(uint8_t byte);
static void emit_bytes(uint8_t byte_1, uint8_t byte_2);
static void end_compiler();

static void define_variable(uint8_t global);
static uint8_t parse_variable(const char* error_message);
static uint8_t identifier_constant(Token* name);

static void expression();
static void statement();
static void declaration();
static void grouping(bool can_assign);
static void unary(bool can_assign);
static void binary(bool can_assign);
static void number(bool can_assign);
static void string(bool can_assign);
static void variable(bool can_assign);
static void emit_constant(Value value);
static uint8_t make_constant(Value value);
static void parse_precendence(Precendence precendence);
static ParseRule* get_rule(TokenType type);
static Chunk* current_chunk();

ParseRule rules[] = {
  // If you haven’t seen the [TOKEN_DOT] = syntax in a C array literal, that is
  // C99’s designated initializer syntax. It’s clearer than having to count array
  // indexes by hand.
  [TOKEN_LEFT_PAREN]    = {grouping, NULL,   PREC_NONE},
  [TOKEN_RIGHT_PAREN]   = {NULL,     NULL,   PREC_NONE},
  [TOKEN_LEFT_BRACE]    = {NULL,     NULL,   PREC_NONE},
  [TOKEN_RIGHT_BRACE]   = {NULL,     NULL,   PREC_NONE},
  [TOKEN_COMMA]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_DOT]           = {NULL,     NULL,   PREC_NONE},
  [TOKEN_MINUS]         = {unary,    binary, PREC_TERM},
  [TOKEN_PLUS]          = {NULL,     binary, PREC_TERM},
  [TOKEN_SEMICOLON]     = {NULL,     NULL,   PREC_NONE},
  [TOKEN_SLASH]         = {NULL,     binary, PREC_FACTOR},
  [TOKEN_STAR]          = {NULL,     binary, PREC_FACTOR},
  [TOKEN_BANG]          = {unary,    NULL,   PREC_NONE},
  [TOKEN_BANG_EQUAL]    = {NULL,     binary, PREC_EQUALITY},
  [TOKEN_EQUAL]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_EQUAL_EQUAL]   = {NULL,     binary, PREC_EQUALITY},
  [TOKEN_GREATER]       = {NULL,     binary, PREC_COMPARISON},
  [TOKEN_GREATER_EQUAL] = {NULL,     binary, PREC_COMPARISON},
  [TOKEN_LESS]          = {NULL,     binary, PREC_COMPARISON},
  [TOKEN_LESS_EQUAL]    = {NULL,     binary, PREC_COMPARISON},
  [TOKEN_IDENTIFIER]    = {variable,  NULL,   PREC_NONE},
  [TOKEN_STRING]        = {string,   NULL,   PREC_NONE},
  [TOKEN_NUMBER]        = {number,   NULL,   PREC_NONE},
  [TOKEN_AND]           = {NULL,     NULL,   PREC_NONE},
  [TOKEN_CLASS]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_ELSE]          = {NULL,     NULL,   PREC_NONE},
  [TOKEN_FALSE]         = {literal,  NULL,   PREC_NONE},
  [TOKEN_FOR]           = {NULL,     NULL,   PREC_NONE},
  [TOKEN_FUN]           = {NULL,     NULL,   PREC_NONE},
  [TOKEN_IF]            = {NULL,     NULL,   PREC_NONE},
  [TOKEN_NIL]           = {literal,  NULL,   PREC_NONE},
  [TOKEN_OR]            = {NULL,     NULL,   PREC_NONE},
  [TOKEN_PRINT]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_RETURN]        = {NULL,     NULL,   PREC_NONE},
  [TOKEN_SUPER]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_THIS]          = {NULL,     NULL,   PREC_NONE},
  [TOKEN_TRUE]          = {literal,  NULL,   PREC_NONE},
  [TOKEN_VAR]           = {NULL,     NULL,   PREC_NONE},
  [TOKEN_WHILE]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_ERROR]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_EOF]           = {NULL,     NULL,   PREC_NONE},
};

Parser parser;
Chunk* compiling_chunk;

bool compile(const char* source, Chunk* chunk);

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

static bool check(TokenType type) {
  return parser.current.type == type;
}

static bool match(TokenType type) {
  if (!check(type)) return false;
  advance();
  return true;
}

//  It steps forward through the token stream.
//  It asks the scanner for the next token and stores it for later use.
//  Before doing that, it takes the old current token and stashes that
//  in a previous field.
static void advance() {
  parser.previous = parser.current;

  for (;;) {
    parser.current = scan_token();
    if (parser.current.type != TOKEN_ERROR)
      break;

    error_at_current(parser.current.start);
  }
}

static void literal(bool can_assign) {
  switch (parser.previous.type) {
    case TOKEN_FALSE: emit_byte(OP_FALSE); break;
    case TOKEN_NIL:   emit_byte(OP_NIL); break;
    case TOKEN_TRUE:  emit_byte(OP_TRUE); break;
    default: return; // Unreachable
  }
}

// It writes the given byte, which may be an opcode or an operand to an
// instruction. It sends in the previous token’s line information so that
// runtime errors are associated with that line.
static void emit_byte(uint8_t byte) {
  write_chunk(current_chunk(), byte, parser.previous.line);
}

static void emit_bytes(uint8_t byte_1, uint8_t byte_2) {
  emit_byte(byte_1);
  emit_byte(byte_2);
}

static Chunk* current_chunk() { return compiling_chunk; }

static void end_compiler() {
  emit_byte(OP_RETURN);

#ifdef DEBUG_PRINT_CODE
  if (!parser.had_error) {
    disassemble_chunk(current_chunk(), "code");
  }
#endif
}

static void expression() {
  parse_precendence(PREC_ASSIGNMENT);
}

static void var_declaration() {
  uint8_t global = parse_variable("Expect variable name.");

  if (match(TOKEN_EQUAL)) {
      expression();
  } else {
    emit_byte(OP_NIL);
  }

  consume(TOKEN_SEMICOLON, "Expect ';' after variable declaration.");

  define_variable(global);
}

static void expression_statement() {
  expression();
  consume(TOKEN_SEMICOLON, "Expect ';' after expression");
  emit_byte(OP_POP);
}

static void print_statement() {
  expression();
  consume(TOKEN_SEMICOLON, "Expect ';' after value.");
  emit_byte(OP_PRINT);
}

static void synchronize() {
  parser.panic_mode = false;

  while (parser.current.type != TOKEN_EOF) {
    if (parser.previous.type == TOKEN_SEMICOLON) return;
    switch (parser.current.type) {
      case TOKEN_CLASS:
      case TOKEN_FUN:
      case TOKEN_VAR:
      case TOKEN_FOR:
      case TOKEN_IF:
      case TOKEN_WHILE:
      case TOKEN_PRINT:
      case TOKEN_RETURN:
        return;
      default:
        ;
    
    }
    advance();
  }
}

static void number(bool can_assign) {
  double value = strtod(parser.previous.start, NULL);
  emit_constant(NUMBER_VAL(value));
}

// This takes the string’s characters directly from the
// lexeme. The + 1 and - 2 parts trim the leading and trailing quotation marks.
// It then creates a string object, wraps it in a Value, and stuffs it into the
// constant table.
//
// NOTE:
// If Lox supported string escape sequences like \n, we’d translate those here.
// Since it doesn’t, we can take the characters as they are.
static void string(bool can_assign) {
  emit_constant(
      OBJECT_VAL(
        copy_string(parser.previous.start + 1, parser.previous.length - 2)));
}

static void named_variable(Token name, bool can_assign) {
  uint8_t arg = identifier_constant(&name);
  if (can_assign && match(TOKEN_EQUAL)) {
    expression();
    emit_bytes(OP_SET_GLOBAL, arg);
  } else {
    emit_bytes(OP_GET_GLOBAL, arg);
  }
}

static void variable(bool can_assign) {
  named_variable(parser.previous, can_assign);
}

static void unary(bool can_assign) {
  TokenType operator_type = parser.previous.type;

  // *Compile the operand*
  // We use the unary operator’s own PREC_UNARY precedence to permit nested
  // unary expressions like !!doubleNegative. Since unary operators have pretty
  // high precedence, that correctly excludes things like binary operators.
  parse_precendence(PREC_UNARY);

  // Emit the operator instruction
  switch (operator_type) {
    case TOKEN_MINUS: emit_byte(OP_NEGATE); break;
    case TOKEN_BANG:  emit_byte(OP_NOT); break;
    default: return;
  }
}

static void binary(bool can_assign) {
  TokenType operator_type = parser.previous.type;
  ParseRule* rule = get_rule(operator_type);
  // We use one higher level of precedence for the right operand because the binary
  // operators are left-associative.
  parse_precendence((Precendence)(rule->precendence + 1));

  switch (operator_type) {
  case TOKEN_PLUS:          emit_byte(OP_ADD); break;
  case TOKEN_MINUS:         emit_byte(OP_SUBTRACT); break;
  case TOKEN_STAR:          emit_byte(OP_MULTIPLY); break;
  case TOKEN_SLASH:         emit_byte(OP_DIVIDE); break;
  case TOKEN_BANG_EQUAL:    emit_bytes(OP_EQUAL, OP_NOT); break;
  case TOKEN_EQUAL_EQUAL:   emit_byte(OP_EQUAL); break;
  case TOKEN_GREATER:       emit_byte(OP_GREATER); break;
  case TOKEN_GREATER_EQUAL: emit_byte(OP_GREATER); break;
  case TOKEN_LESS:          emit_byte(OP_LESS); break;
  case TOKEN_LESS_EQUAL:    emit_bytes(OP_GREATER, OP_NOT); break;
  default: return; // Unreachable
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
static void grouping(bool can_assign) {
  expression();
  consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression");
}

// Starts at the current token and parses any expression at the given precedence
// level or higher.
//
// We look up a prefix parser for the current token. The first token is always
// going to belong to some kind of prefix expression, by definition.
// It may turn out to be nested as an operand inside one or more infix expressions,
// but as you read the code from left to right, the first token you hit always
// belongs to a prefix expression.
//
// 
// After parsing that, which may consume more tokens, the prefix expression is
// done. Now we look for an infix parser for the next token. If we find one, it
// means the prefix expression we already compiled might be an operand for it. But
// only if the call to parsePrecedence() has a precedence that is low enough to
// permit that infix operator.
// 
// If the next token is too low precedence, or isn’t an infix operator at all,
// we’re done. We’ve parsed as much expression as we can. Otherwise, we consume the
// operator and hand off control to the infix parser we found. It consumes whatever
// other tokens it needs (usually the right operand) and returns back to
// parsePrecedence(). Then we loop back around and see if the next token is
// also a valid infix operator that can take the entire preceding expression as its
// operand.
static void parse_precendence(Precendence precendence) {
  advance();
  ParseFn prefix_rule = get_rule(parser.previous.type)->prefix;
  if (prefix_rule == NULL) {
    error("Expect expression.");
    return;
  }

  bool can_assign = precendence <= PREC_ASSIGNMENT;
  prefix_rule(can_assign);

  while (precendence <= get_rule(parser.current.type)->precendence) {
    advance();
    ParseFn infix_rule = get_rule(parser.previous.type)->infix;
    infix_rule(can_assign);
  }

  if (can_assign && match(TOKEN_EQUAL)) {
    error("Invalid assignment target.");
  }
}

static uint8_t identifier_constant(Token* name) {
  return make_constant(OBJECT_VAL(copy_string(name->start, name->length)));
}

static uint8_t parse_variable(const char* error_message) {
  consume(TOKEN_IDENTIFIER, error_message);
  return identifier_constant(&parser.previous);
}

static void define_variable(uint8_t global) {
  emit_bytes(OP_DEFINE_GLOBAL, global);
}

static ParseRule* get_rule(TokenType type) {
  return &rules[type];
}

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
  while (!match(TOKEN_EOF)) {
    declaration();
  }

  end_compiler();
  return !parser.had_error;
}

static void declaration() {
  if (match(TOKEN_VAR)) {
    var_declaration();
  } else {
    statement();
  }
  if (parser.panic_mode) synchronize();
}

static void statement() {
  if (match(TOKEN_PRINT)) {
    print_statement();
  } else {
    expression_statement();
  }
}



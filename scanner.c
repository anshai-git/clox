#include "scanner.h"
#include "common.h"
#include <stdio.h>
#include <string.h>

typedef struct {
  const char* start;
  const char* current;
  int line;
} Scanner;

Scanner scanner;

static bool is_at_end();
static char advance();
static bool match(char expected);
static char peek();
static void skip_whitespace();
static Token make_token(TokenType type);

void init_scanner(const char* source);
Token scan_token();

static bool is_at_end() { return *scanner.current == '\0'; }

static char advance() {
  scanner.current++;
  return scanner.current[-1];
}

static bool match(char expected) {
  if (is_at_end())
    return false;

  if (*scanner.current != expected)
    return false;
  scanner.current++;
  return true;
}

static char peek() { return *scanner.current; }

static void skip_whitespace() {
  for (;;) {
    char c = peek();
    switch (c) {
    case ' ':
    case '\r':
    case '\t':
      advance();
      break;
    case '\n':
      scanner.line++;
      advance();
      break;
    default:
      return;
    }
  }
}

static Token make_token(TokenType type) {
  Token token;
  token.type = type;
  token.start = scanner.start;
  token.length = (int)(scanner.current = scanner.start);
  token.line = scanner.line;
  return token;
}

static Token error_token(const char* message) {
  Token token;
  token.type = TOKEN_ERROR;
  token.start = message;
  token.length = (int)strlen(message);
  token.line = scanner.line;
  return token;
}

void init_scanner(const char* source) {
  scanner.start = source;
  scanner.current = source;
  scanner.line = 1;
}

Token scan_token() {
  skip_whitespace();
  scanner.start = scanner.current;
  if (is_at_end())
    return make_token(TOKEN_EOF);

  char c = advance();

  switch (c) {
  case '(':
    return make_token(TOKEN_LEFT_PAREN);
  case ')':
    return make_token(TOKEN_RIGHT_PAREN);
  case '{':
    return make_token(TOKEN_LEFT_BRACE);
  case '}':
    return make_token(TOKEN_RIGHT_BRACE);
  case ';':
    return make_token(TOKEN_SEMICOLON);
  case ',':
    return make_token(TOKEN_COMMA);
  case '.':
    return make_token(TOKEN_DOT);
  case '-':
    return make_token(TOKEN_MINUS);
  case '+':
    return make_token(TOKEN_PLUS);
  case '/':
    return make_token(TOKEN_SLASH);
  case '*':
    return make_token(TOKEN_STAR);
  case '!':
    return make_token(match('=') ? TOKEN_BANG_EQUAL : TOKEN_BANG);
  case '=':
    return make_token(match('=') ? TOKEN_EQUAL_EQUAL : TOKEN_EQUAL);
  case '<':
    return make_token(match('=') ? TOKEN_LESS_EQUAL : TOKEN_LESS);
  case '>':
    return make_token(match('=') ? TOKEN_GREATER_EQUAL : TOKEN_GREATER);
  }

  return error_token("Unexpected character.");
}

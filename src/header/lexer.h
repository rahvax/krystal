#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum {
  TOKEN_EOF = 0,
  TOKEN_IDENT,
  TOKEN_INT,
  TOKEN_BOOL,
  TOKEN_ATOM,
  TOKEN_LET,
  TOKEN_EQUAL,
  TOKEN_SEMICOLON,
  TOKEN_ERROR
} TokenKind;

typedef struct {
  TokenKind kind;
  const char *start;
  size_t length;
  int line;
  int column;
  int64_t int_value;
  bool bool_value;
} Token;

typedef struct {
  Token *items;
  size_t count;
  size_t capacity;
} TokenStream;

bool lex_source(const char *source, TokenStream *out_stream, char *error,
                size_t error_size);
void free_token_stream(TokenStream *stream);

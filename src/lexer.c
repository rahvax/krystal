#include "header/lexer.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  const char *source;
  size_t length;
  size_t start;
  size_t current;
  int line;
  int column;
  int start_line;
  int start_column;
  TokenStream *stream;
  char *error;
  size_t error_size;
} Lexer;

static bool scan_token(Lexer *lexer);
static bool add_token(Lexer *lexer, Token token);
static bool ensure_capacity(TokenStream *stream);
static void set_error(Lexer *lexer, const char *message);
static char advance(Lexer *lexer);
static char peek(const Lexer *lexer);
static void skip_comment(Lexer *lexer);
static bool lex_number(Lexer *lexer);
static bool lex_identifier(Lexer *lexer);
static bool lex_atom(Lexer *lexer);

bool lex_source(const char *source, TokenStream *out_stream, char *error,
                size_t error_size) {
  if (!source || !out_stream) {
    return false;
  }

  out_stream->items = NULL;
  out_stream->count = 0;
  out_stream->capacity = 0;

  Lexer lexer = {
      .source = source,
      .length = strlen(source),
      .start = 0,
      .current = 0,
      .line = 1,
      .column = 1,
      .start_line = 1,
      .start_column = 1,
      .stream = out_stream,
      .error = error,
      .error_size = error_size,
  };

  while (lexer.current < lexer.length) {
    lexer.start = lexer.current;
    lexer.start_line = lexer.line;
    lexer.start_column = lexer.column;
    if (!scan_token(&lexer)) {
      free_token_stream(out_stream);
      return false;
    }
  }

  Token eof = {
      .kind = TOKEN_EOF,
      .start = lexer.source + lexer.length,
      .length = 0,
      .line = lexer.line,
      .column = lexer.column,
  };
  if (!add_token(&lexer, eof)) {
    set_error(&lexer, "Failed to allocate memory for tokens.");
    free_token_stream(out_stream);
    return false;
  }

  return true;
}

void free_token_stream(TokenStream *stream) {
  if (!stream) {
    return;
  }
  free(stream->items);
  stream->items = NULL;
  stream->count = 0;
  stream->capacity = 0;
}

static bool scan_token(Lexer *lexer) {
  char c = advance(lexer);
  switch (c) {
  case ' ':
  case '\r':
  case '\t':
    return true;
  case '\n':
    return true;
  case '#':
    skip_comment(lexer);
    return true;
  case '=': {
    Token token = {
        .kind = TOKEN_EQUAL,
        .start = lexer->source + lexer->start,
        .length = lexer->current - lexer->start,
        .line = lexer->start_line,
        .column = lexer->start_column,
    };
    return add_token(lexer, token);
  }
  case ';': {
    Token token = {
        .kind = TOKEN_SEMICOLON,
        .start = lexer->source + lexer->start,
        .length = lexer->current - lexer->start,
        .line = lexer->start_line,
        .column = lexer->start_column,
    };
    return add_token(lexer, token);
  }
  case ':':
    return lex_atom(lexer);
  default:
    break;
  }

  if (isdigit((unsigned char)c)) {
    return lex_number(lexer);
  }

  if (isalpha((unsigned char)c) || c == '_') {
    return lex_identifier(lexer);
  }

  set_error(lexer, "Unexpected character.");
  return false;
}

static bool lex_number(Lexer *lexer) {
  while (lexer->current < lexer->length &&
         isdigit((unsigned char)peek(lexer))) {
    advance(lexer);
  }
  const char *start = lexer->source + lexer->start;
  long long value = strtoll(start, NULL, 10);
  Token token = {
      .kind = TOKEN_INT,
      .start = start,
      .length = lexer->current - lexer->start,
      .line = lexer->start_line,
      .column = lexer->start_column,
      .int_value = value,
  };
  return add_token(lexer, token);
}

static bool lex_identifier(Lexer *lexer) {
  while (lexer->current < lexer->length) {
    char c = peek(lexer);
    if (!isalnum((unsigned char)c) && c != '_') {
      break;
    }
    advance(lexer);
  }

  const char *start = lexer->source + lexer->start;
  size_t length = lexer->current - lexer->start;

  Token token = {
      .kind = TOKEN_IDENT,
      .start = start,
      .length = length,
      .line = lexer->start_line,
      .column = lexer->start_column,
  };

  if (length == 3 && strncmp(start, "let", 3) == 0) {
    token.kind = TOKEN_LET;
  } else if (length == 4 && strncmp(start, "true", 4) == 0) {
    token.kind = TOKEN_BOOL;
    token.bool_value = true;
  } else if (length == 5 && strncmp(start, "false", 5) == 0) {
    token.kind = TOKEN_BOOL;
    token.bool_value = false;
  }

  return add_token(lexer, token);
}

static bool lex_atom(Lexer *lexer) {
  if (lexer->current >= lexer->length) {
    set_error(lexer, "Atom literal requires a name.");
    return false;
  }
  char first = peek(lexer);
  if (!isalpha((unsigned char)first) && first != '_') {
    set_error(lexer, "Atom literal must start with a letter or '_'.");
    return false;
  }
  advance(lexer);
  while (lexer->current < lexer->length) {
    char c = peek(lexer);
    if (!isalnum((unsigned char)c) && c != '_') {
      break;
    }
    advance(lexer);
  }
  Token token = {
      .kind = TOKEN_ATOM,
      .start = lexer->source + lexer->start,
      .length = lexer->current - lexer->start,
      .line = lexer->start_line,
      .column = lexer->start_column,
  };
  return add_token(lexer, token);
}

static bool add_token(Lexer *lexer, Token token) {
  if (!ensure_capacity(lexer->stream)) {
    set_error(lexer, "Failed to allocate memory for tokens.");
    return false;
  }
  lexer->stream->items[lexer->stream->count++] = token;
  return true;
}

static bool ensure_capacity(TokenStream *stream) {
  if (stream->count < stream->capacity) {
    return true;
  }
  size_t new_capacity = stream->capacity < 8 ? 8 : stream->capacity * 2;
  Token *items = realloc(stream->items, new_capacity * sizeof(Token));
  if (!items) {
    return false;
  }
  stream->items = items;
  stream->capacity = new_capacity;
  return true;
}

static void set_error(Lexer *lexer, const char *message) {
  if (!lexer->error || lexer->error_size == 0) {
    return;
  }
  snprintf(lexer->error, lexer->error_size,
           "Lexer error at line %d, column %d: %s", lexer->start_line,
           lexer->start_column, message);
}

static char advance(Lexer *lexer) {
  char c = lexer->source[lexer->current++];
  if (c == '\n') {
    lexer->line += 1;
    lexer->column = 1;
  } else {
    lexer->column += 1;
  }
  return c;
}

static char peek(const Lexer *lexer) {
  if (lexer->current >= lexer->length) {
    return '\0';
  }
  return lexer->source[lexer->current];
}

static void skip_comment(Lexer *lexer) {
  while (lexer->current < lexer->length) {
    char c = advance(lexer);
    if (c == '\n') {
      break;
    }
  }
}

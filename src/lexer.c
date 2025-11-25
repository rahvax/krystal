#include "header/lexer.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool LexSource(const char *source, TokenStream *outStream, char *error, size_t errorSize) {
  if (!source || !outStream) 
    return false;

  outStream->items = NULL;
  outStream->count = 0;
  outStream->capacity = 0;

  Lexer lexer = {
      .source = source,
      .length = strlen(source),
      .start = 0,
      .current = 0,
      .line = 1,
      .column = 1,
      .startLine = 1,
      .startColumn = 1,
      .stream = outStream,
      .error = error,
      .errorSize = errorSize,
  };

  while (lexer.current < lexer.length) {
    lexer.start = lexer.current;
    lexer.startLine = lexer.line;
    lexer.startColumn = lexer.column;
    if (!ScanToken(&lexer)) {
      FreeToken(outStream);
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
  if (!AddToken(&lexer, eof)) {
    SetError(&lexer, "Failed to allocate memory for tokens.");
    FreeToken(outStream);
    return false;
  }

  return true;
}

void FreeToken(TokenStream *stream) {
  if (!stream)
    return;
  free(stream->items);
  stream->items = NULL;
  stream->count = 0;
  stream->capacity = 0;
}

bool ScanToken(Lexer *lexer) {
  char c = AdvanceLexer(lexer);
  switch (c) {
    case ' ':
    case '\r':
    case '\t':
    case '\n':
      return true;
    case '#':
      SkipComment(lexer);
      return true;
    case '=': {
      Token token = {
        .kind = TOKEN_EQUAL,
        .start = lexer->source + lexer->start,
        .length = lexer->current - lexer->start,
        .line = lexer->startLine,
        .column = lexer->startColumn,
      };
      return AddToken(lexer, token);
    }
    case ';': {
      Token token = {
        .kind = TOKEN_SEMICOLON,
        .start = lexer->source + lexer->start,
        .length = lexer->current - lexer->start,
        .line = lexer->startLine,
        .column = lexer->startColumn,
      };
      return AddToken(lexer, token);
    }
    case ':':
      return LexAtom(lexer);
    default:
      break;
  }

  if (isdigit((unsigned char)c)) 
    return LexNumber(lexer);
  if (isalpha((unsigned char)c) || c == '_') 
    return LexIdentifier(lexer);

  SetError(lexer, "Unexpected character.");
  return false;
}

bool LexNumber(Lexer *lexer) {
  while (lexer->current < lexer->length && isdigit((unsigned char)PeekLexer(lexer)))
    AdvanceLexer(lexer);
  const char *start = lexer->source + lexer->start;
  long long value = strtoll(start, NULL, 10);
  Token token = {
      .kind = TOKEN_INT,
      .start = start,
      .length = lexer->current - lexer->start,
      .line = lexer->startLine,
      .column = lexer->startColumn,
      .intValue = value,
  };
  return AddToken(lexer, token);
}

bool LexIdentifier(Lexer *lexer) {
  while (lexer->current < lexer->length) {
    char c = PeekLexer(lexer);
    if (!isalnum((unsigned char)c) && c != '_') 
      break;
    AdvanceLexer(lexer);
  }

  const char *start = lexer->source + lexer->start;
  size_t length = lexer->current - lexer->start;

  Token token = {
      .kind = TOKEN_IDENT,
      .start = start,
      .length = length,
      .line = lexer->startLine,
      .column = lexer->startColumn,
  };

  if (length == 3 && strncmp(start, "let", 3) == 0)
    token.kind = TOKEN_LET;
  else if (length == 4 && strncmp(start, "true", 4) == 0) {
    token.kind = TOKEN_BOOL;
    token.boolValue = true;
  }
  else if (length == 5 && strncmp(start, "false", 5) == 0) {
    token.kind = TOKEN_BOOL;
    token.boolValue = false;
  }
  return AddToken(lexer, token);
}

bool LexAtom(Lexer *lexer) {
  if (lexer->current >= lexer->length) {
    SetError(lexer, "Atom literal requires a name.");
    return false;
  }
  char first = PeekLexer(lexer);
  if (!isalpha((unsigned char)first) && first != '_') {
    SetError(lexer, "Atom literal must start with a letter or '_'.");
    return false;
  }

  AdvanceLexer(lexer);
  while (lexer->current < lexer->length) {
    char c = PeekLexer(lexer);
    if (!isalnum((unsigned char)c) && c != '_') 
      break;
    AdvanceLexer(lexer);
  }

  Token token = {
      .kind = TOKEN_ATOM,
      .start = lexer->source + lexer->start,
      .length = lexer->current - lexer->start,
      .line = lexer->startLine,
      .column = lexer->startColumn,
  };
  return AddToken(lexer, token);
}

bool AddToken(Lexer *lexer, Token token) {
  if (!EnsureCapacity(lexer->stream)) {
    SetError(lexer, "Failed to allocate memory for tokens.");
    return false;
  }
  lexer->stream->items[lexer->stream->count++] = token;
  return true;
}

bool EnsureCapacity(TokenStream *stream) {
  if (stream->count < stream->capacity) 
    return true;
  size_t new_capacity = stream->capacity < 8 ? 8 : stream->capacity * 2;
  Token *items = realloc(stream->items, new_capacity * sizeof(Token));
  if (!items) 
    return false;
  stream->items = items;
  stream->capacity = new_capacity;
  return true;
}

void SetError(Lexer *lexer, const char *message) {
  if (!lexer->error || lexer->errorSize == 0) 
    return;
  snprintf(lexer->error, lexer->errorSize,
           "Lexer error at line %d, column %d: %s", lexer->startLine,
           lexer->startColumn, message);
}

char AdvanceLexer(Lexer *lexer) {
  char c = lexer->source[lexer->current++];
  if (c == '\n') {
    lexer->line += 1;
    lexer->column = 1;
  }
  else 
    lexer->column += 1;
  return c;
}

char PeekLexer(const Lexer *lexer) {
  if (lexer->current >= lexer->length) 
    return '\0';
  return lexer->source[lexer->current];
}

void SkipComment(Lexer *lexer) {
  while (lexer->current < lexer->length) {
    char c = AdvanceLexer(lexer);
    if (c == '\n')
      break;
  }
}

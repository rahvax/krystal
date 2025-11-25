#ifndef _LEXER_H
#define _LEXER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum {
  TOKEN_EOF,
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
  int64_t intValue;
  bool boolValue;
} Token;

typedef struct {
  Token *items;
  size_t count;
  size_t capacity;
} TokenStream;

bool LexSource(const char *source, TokenStream *outStream, char *error,
                size_t errorSize);
void FreeToken(TokenStream *stream);

typedef struct {
  const char *source;
  size_t length;
  size_t start;
  size_t current;
  int line;
  int column;
  int startLine;
  int startColumn;
  TokenStream *stream;
  char *error;
  size_t errorSize;
} Lexer;

bool ScanToken(Lexer *lexer);
bool AddToken(Lexer *lexer, Token token);
bool EnsureCapacity(TokenStream *stream);
void SetError(Lexer *lexer, const char *message);
char AdvanceLexer(Lexer *lexer);
char PeekLexer(const Lexer *lexer);
void SkipComment(Lexer *lexer);
bool LexNumber(Lexer *lexer);
bool LexIdentifier(Lexer *lexer);
bool LexAtom(Lexer *lexer);
#endif

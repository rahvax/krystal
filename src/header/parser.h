#ifndef _PARSER_H
#define _PARSER_H
#include <stdbool.h>
#include <stddef.h>
#include "lexer.h"
typedef enum { AST_INT, AST_BOOL, AST_ATOM, AST_LET, AST_SEQ } NodeKind;
typedef struct ASTNode ASTNode;

struct ASTNode {
  NodeKind kind;
  union {
    struct {
      long long value;
    } int_lit;
    struct {
      bool value;
    } bool_lit;
    struct {
      char *value;
    } atom_lit;
    struct {
      char *name;
      ASTNode *value;
    } let_stmt;
    struct {
      ASTNode **items;
      size_t count;
    } seq;
  };
};

bool ParseTokens(const TokenStream *stream, ASTNode **outAst, char *error, size_t errorSize);
void FreeAst(ASTNode *node);

typedef struct {
  const TokenStream *stream;
  size_t pos;
  char *error;
  size_t errorSize;
} Parser;

ASTNode *ParseSequence(Parser *parser);
ASTNode *ParseStatement(Parser *parser);
ASTNode *ParseLet(Parser *parser);
ASTNode *ParseLiteral(Parser *parser);
bool Match(Parser *parser, TokenKind kind);
bool Check(const Parser *parser, TokenKind kind);
const Token *Advance(Parser *parser);
bool IsAtEnd(const Parser *parser);
const Token *Peek(const Parser *parser);
const Token *Previous(const Parser *parser);
void SetErrorParser(Parser *parser, const Token *token, const char *fmt, ...);
ASTNode *MakeNode(NodeKind kind);
char *CopyToken(const Token *token, size_t skipPrefix);
bool AppendItem(ASTNode *seq, ASTNode *item);
#endif

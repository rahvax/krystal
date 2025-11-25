#include "header/parser.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
bool ParseTokens(const TokenStream *stream, ASTNode **outAst, char *error, size_t errorSize) {
  if (!stream || !outAst)
    return false;
  Parser parser = {
      .stream = stream,
      .pos = 0,
      .error = error,
      .errorSize = errorSize,
  };

  ASTNode *root = ParseSequence(&parser);
  if (!root) 
    return false;
  *outAst = root;
  return true;
}

ASTNode *ParseSequence(Parser *parser) {
  ASTNode *seq = MakeNode(AST_SEQ);
  if (!seq) {
    SetErrorParser(parser, Peek(parser), "Failed to allocate AST node.");
    return NULL;
  }
  seq->seq.items = NULL;
  seq->seq.count = 0;

  while (!IsAtEnd(parser)) {
    ASTNode *statement = ParseStatement(parser);
    if (!statement) {
      FreeAst(seq);
      return NULL;
    }
    if (!AppendItem(seq, statement)) {
      SetErrorParser(parser, Peek(parser), "Failed to allocate statement list.");
      FreeAst(statement);
      FreeAst(seq);
      return NULL;
    }
  }

  return seq;
}

ASTNode *ParseStatement(Parser *parser) {
  if (Match(parser, TOKEN_LET)) {
    return ParseLet(parser);
  }
  ASTNode *expr = ParseLiteral(parser);
  if (!expr) {
    return NULL;
  }
  if (!Match(parser, TOKEN_SEMICOLON)) {
    SetErrorParser(parser, Peek(parser), "Expected ';' after expression.");
    FreeAst(expr);
    return NULL;
  }
  return expr;
}

ASTNode *ParseLet(Parser *parser) {
  if (!Check(parser, TOKEN_IDENT)) {
    SetErrorParser(parser, Peek(parser), "Expected identifier after 'let'.");
    return NULL;
  }
  const Token *nameToken = Advance(parser);
  if (!Match(parser, TOKEN_EQUAL)) {
    SetErrorParser(parser, Peek(parser), "Expected '=' after identifier.");
    return NULL;
  }
  ASTNode *value = ParseLiteral(parser);
  if (!value) {
    return NULL;
  }
  if (!Match(parser, TOKEN_SEMICOLON)) {
    SetErrorParser(parser, Peek(parser), "Expected ';' after let value.");
    FreeAst(value);
    return NULL;
  }
  ASTNode *node = MakeNode(AST_LET);
  if (!node) {
    SetErrorParser(parser, nameToken, "Failed to allocate AST node.");
    FreeAst(value);
    return NULL;
  }
  node->let_stmt.value = value;
  node->let_stmt.name = CopyToken(nameToken, 0);
  if (!node->let_stmt.name) {
    SetErrorParser(parser, nameToken, "Failed to copy identifier.");
    FreeAst(node);
    return NULL;
  }
  return node;
}

ASTNode *ParseLiteral(Parser *parser) {
  if (Match(parser, TOKEN_INT)) {
    const Token *token = Previous(parser);
    ASTNode *node = MakeNode(AST_INT);
    if (!node) {
      SetErrorParser(parser, token, "Failed to allocate AST node.");
      return NULL;
    }
    node->int_lit.value = token->intValue;
    return node;
  }
  if (Match(parser, TOKEN_BOOL)) {
    const Token *token = Previous(parser);
    ASTNode *node = MakeNode(AST_BOOL);
    if (!node) {
      SetErrorParser(parser, token, "Failed to allocate AST node.");
      return NULL;
    }
    node->bool_lit.value = token->boolValue;
    return node;
  }
  if (Match(parser, TOKEN_ATOM)) {
    const Token *token = Previous(parser);
    ASTNode *node = MakeNode(AST_ATOM);
    if (!node) {
      SetErrorParser(parser, token, "Failed to allocate AST node.");
      return NULL;
    }
    node->atom_lit.value = CopyToken(token, 1);
    if (!node->atom_lit.value) {
      SetErrorParser(parser, token, "Failed to copy atom literal.");
      FreeAst(node);
      return NULL;
    }
    return node;
  }

  SetErrorParser(parser, Peek(parser), "Expected int, bool, or atom literal.");
  return NULL;
}

bool Match(Parser *parser, TokenKind kind) {
  if (Check(parser, kind)) {
    Advance(parser);
    return true;
  }
  return false;
}

bool Check(const Parser *parser, TokenKind kind) {
  if (IsAtEnd(parser)) {
    return false;
  }
  return Peek(parser)->kind == kind;
}

const Token *Advance(Parser *parser) {
  if (!IsAtEnd(parser)) {
    parser->pos++;
  }
  return Previous(parser);
}

bool IsAtEnd(const Parser *parser) {
  return Peek(parser)->kind == TOKEN_EOF;
}

const Token *Peek(const Parser *parser) {
  return &parser->stream->items[parser->pos];
}

const Token *Previous(const Parser *parser) {
  return &parser->stream->items[parser->pos - 1];
}

void SetErrorParser(Parser *parser, const Token *token, const char *fmt, ...) {
  if (!parser->error || parser->errorSize == 0) {
    return;
  }
  va_list args;
  va_start(args, fmt);
  if (token) {
    int prefix = snprintf(parser->error, parser->errorSize,
                          "Parser error at line %d, column %d: ", token->line,
                          token->column);
    if (prefix < 0 || (size_t)prefix >= parser->errorSize) {
      va_end(args);
      return;
    }
    vsnprintf(parser->error + prefix, parser->errorSize - (size_t)prefix, fmt,
              args);
  } else {
    vsnprintf(parser->error, parser->errorSize, fmt, args);
  }
  va_end(args);
}

ASTNode *MakeNode(NodeKind kind) {
  ASTNode *node = calloc(1, sizeof(ASTNode));
  if (node) {
    node->kind = kind;
  }
  return node;
}

char *CopyToken(const Token *token, size_t skipPrefix) {
  if (!token || token->length < skipPrefix) {
    return NULL;
  }
  size_t length = token->length - skipPrefix;
  char *text = malloc(length + 1);
  if (!text) {
    return NULL;
  }
  memcpy(text, token->start + skipPrefix, length);
  text[length] = '\0';
  return text;
}

bool AppendItem(ASTNode *seq, ASTNode *item) {
  size_t new_count = seq->seq.count + 1;
  ASTNode **new_items = realloc(seq->seq.items, new_count * sizeof(ASTNode *));
  if (!new_items) {
    return false;
  }
  new_items[seq->seq.count] = item;
  seq->seq.items = new_items;
  seq->seq.count = new_count;
  return true;
}

void FreeAst(ASTNode *node) {
  if (!node) {
    return;
  }
  switch (node->kind) {
  case AST_INT:
  case AST_BOOL:
    break;
  case AST_ATOM:
    free(node->atom_lit.value);
    break;
  case AST_LET:
    free(node->let_stmt.name);
    FreeAst(node->let_stmt.value);
    break;
  case AST_SEQ:
    for (size_t i = 0; i < node->seq.count; ++i) {
      FreeAst(node->seq.items[i]);
    }
    free(node->seq.items);
    break;
  }
  free(node);
}

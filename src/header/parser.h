#pragma once

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

bool parse_tokens(const TokenStream *stream, ASTNode **out_ast, char *error,
                  size_t error_size);
void free_ast(ASTNode *node);

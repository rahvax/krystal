#include "header/parser.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  const TokenStream *stream;
  size_t pos;
  char *error;
  size_t error_size;
} Parser;

static ASTNode *parse_sequence(Parser *parser);
static ASTNode *parse_statement(Parser *parser);
static ASTNode *parse_let(Parser *parser);
static ASTNode *parse_literal(Parser *parser);
static bool match(Parser *parser, TokenKind kind);
static bool check(const Parser *parser, TokenKind kind);
static const Token *advance(Parser *parser);
static bool is_at_end(const Parser *parser);
static const Token *peek(const Parser *parser);
static const Token *previous(const Parser *parser);
static void set_error(Parser *parser, const Token *token, const char *fmt, ...);
static ASTNode *make_node(NodeKind kind);
static char *copy_token_text(const Token *token, size_t skip_prefix);
static bool append_seq_item(ASTNode *seq, ASTNode *item);

bool parse_tokens(const TokenStream *stream, ASTNode **out_ast, char *error,
                  size_t error_size) {
  if (!stream || !out_ast) {
    return false;
  }
  Parser parser = {
      .stream = stream,
      .pos = 0,
      .error = error,
      .error_size = error_size,
  };

  ASTNode *root = parse_sequence(&parser);
  if (!root) {
    return false;
  }

  *out_ast = root;
  return true;
}

static ASTNode *parse_sequence(Parser *parser) {
  ASTNode *seq = make_node(AST_SEQ);
  if (!seq) {
    set_error(parser, peek(parser), "Failed to allocate AST node.");
    return NULL;
  }
  seq->seq.items = NULL;
  seq->seq.count = 0;

  while (!is_at_end(parser)) {
    ASTNode *statement = parse_statement(parser);
    if (!statement) {
      free_ast(seq);
      return NULL;
    }
    if (!append_seq_item(seq, statement)) {
      set_error(parser, peek(parser), "Failed to allocate statement list.");
      free_ast(statement);
      free_ast(seq);
      return NULL;
    }
  }

  return seq;
}

static ASTNode *parse_statement(Parser *parser) {
  if (match(parser, TOKEN_LET)) {
    return parse_let(parser);
  }
  ASTNode *expr = parse_literal(parser);
  if (!expr) {
    return NULL;
  }
  if (!match(parser, TOKEN_SEMICOLON)) {
    set_error(parser, peek(parser), "Expected ';' after expression.");
    free_ast(expr);
    return NULL;
  }
  return expr;
}

static ASTNode *parse_let(Parser *parser) {
  if (!check(parser, TOKEN_IDENT)) {
    set_error(parser, peek(parser), "Expected identifier after 'let'.");
    return NULL;
  }
  const Token *name_token = advance(parser);
  if (!match(parser, TOKEN_EQUAL)) {
    set_error(parser, peek(parser), "Expected '=' after identifier.");
    return NULL;
  }
  ASTNode *value = parse_literal(parser);
  if (!value) {
    return NULL;
  }
  if (!match(parser, TOKEN_SEMICOLON)) {
    set_error(parser, peek(parser), "Expected ';' after let value.");
    free_ast(value);
    return NULL;
  }
  ASTNode *node = make_node(AST_LET);
  if (!node) {
    set_error(parser, name_token, "Failed to allocate AST node.");
    free_ast(value);
    return NULL;
  }
  node->let_stmt.value = value;
  node->let_stmt.name = copy_token_text(name_token, 0);
  if (!node->let_stmt.name) {
    set_error(parser, name_token, "Failed to copy identifier.");
    free_ast(node);
    return NULL;
  }
  return node;
}

static ASTNode *parse_literal(Parser *parser) {
  if (match(parser, TOKEN_INT)) {
    const Token *token = previous(parser);
    ASTNode *node = make_node(AST_INT);
    if (!node) {
      set_error(parser, token, "Failed to allocate AST node.");
      return NULL;
    }
    node->int_lit.value = token->int_value;
    return node;
  }
  if (match(parser, TOKEN_BOOL)) {
    const Token *token = previous(parser);
    ASTNode *node = make_node(AST_BOOL);
    if (!node) {
      set_error(parser, token, "Failed to allocate AST node.");
      return NULL;
    }
    node->bool_lit.value = token->bool_value;
    return node;
  }
  if (match(parser, TOKEN_ATOM)) {
    const Token *token = previous(parser);
    ASTNode *node = make_node(AST_ATOM);
    if (!node) {
      set_error(parser, token, "Failed to allocate AST node.");
      return NULL;
    }
    node->atom_lit.value = copy_token_text(token, 1);
    if (!node->atom_lit.value) {
      set_error(parser, token, "Failed to copy atom literal.");
      free_ast(node);
      return NULL;
    }
    return node;
  }

  set_error(parser, peek(parser), "Expected int, bool, or atom literal.");
  return NULL;
}

static bool match(Parser *parser, TokenKind kind) {
  if (check(parser, kind)) {
    advance(parser);
    return true;
  }
  return false;
}

static bool check(const Parser *parser, TokenKind kind) {
  if (is_at_end(parser)) {
    return false;
  }
  return peek(parser)->kind == kind;
}

static const Token *advance(Parser *parser) {
  if (!is_at_end(parser)) {
    parser->pos++;
  }
  return previous(parser);
}

static bool is_at_end(const Parser *parser) {
  return peek(parser)->kind == TOKEN_EOF;
}

static const Token *peek(const Parser *parser) {
  return &parser->stream->items[parser->pos];
}

static const Token *previous(const Parser *parser) {
  return &parser->stream->items[parser->pos - 1];
}

static void set_error(Parser *parser, const Token *token, const char *fmt,
                      ...) {
  if (!parser->error || parser->error_size == 0) {
    return;
  }
  va_list args;
  va_start(args, fmt);
  if (token) {
    int prefix = snprintf(parser->error, parser->error_size,
                          "Parser error at line %d, column %d: ", token->line,
                          token->column);
    if (prefix < 0 || (size_t)prefix >= parser->error_size) {
      va_end(args);
      return;
    }
    vsnprintf(parser->error + prefix, parser->error_size - (size_t)prefix, fmt,
              args);
  } else {
    vsnprintf(parser->error, parser->error_size, fmt, args);
  }
  va_end(args);
}

static ASTNode *make_node(NodeKind kind) {
  ASTNode *node = calloc(1, sizeof(ASTNode));
  if (node) {
    node->kind = kind;
  }
  return node;
}

static char *copy_token_text(const Token *token, size_t skip_prefix) {
  if (!token || token->length < skip_prefix) {
    return NULL;
  }
  size_t length = token->length - skip_prefix;
  char *text = malloc(length + 1);
  if (!text) {
    return NULL;
  }
  memcpy(text, token->start + skip_prefix, length);
  text[length] = '\0';
  return text;
}

static bool append_seq_item(ASTNode *seq, ASTNode *item) {
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

void free_ast(ASTNode *node) {
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
    free_ast(node->let_stmt.value);
    break;
  case AST_SEQ:
    for (size_t i = 0; i < node->seq.count; ++i) {
      free_ast(node->seq.items[i]);
    }
    free(node->seq.items);
    break;
  }
  free(node);
}

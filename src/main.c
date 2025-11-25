#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "header/lexer.h"
#include "header/parser.h"

static char *ReadFile(const char *path);
static void PrintAst(const ASTNode *node, int indent);

int main(int argc, char **argv) {
  char *source, error[256] = {0};
  TokenStream stream = {0};
  ASTNode *ast = NULL;

  if (argc < 2) {
    fprintf(stderr, "Usage: %s <file.krys>\n", argv[0]);
    return 1;
  }

  if (!(source = ReadFile(argv[1]))) {
    fprintf(stderr, "Failed to read %s\n", argv[1]);
    return 1;
  }

  if (!LexSource(source, &stream, error, sizeof(error))) {
    fprintf(stderr, "%s\n", error[0] ? error : "Lexer failed.");
    free(source);
    return 1;
  }

  if (!ParseTokens(&stream, &ast, error, sizeof(error))) {
    fprintf(stderr, "%s\n", error[0] ? error : "Parser failed.");
    FreeToken(&stream);
    free(source);
    return 1;
  }

  PrintAst(ast, 0);
  FreeAst(ast);
  FreeToken(&stream);
  free(source);
  return 0;
}

static char *ReadFile(const char *path) {
  FILE *file;
  long size=0;
  char *buffer;
  size_t read;
  
  if (!(file=fopen(path, "rb")))
    return NULL;

  if (fseek(file, 0, SEEK_END) != 0) {
    fclose(file);
    return NULL;
  }

  size = ftell(file);
  if (size < 0) {
    fclose(file);
    return NULL;
  }

  rewind(file);
  buffer = malloc((size_t)size + 1);

  if (!buffer) {
    fclose(file);
    return NULL;
  }

  read = fread(buffer, 1, (size_t)size, file);
  buffer[read] = '\0';

  fclose(file);
  return buffer;
}

static void PrintAst(const ASTNode *node, int indent) {
  if (!node)
    return;

  for (int i = 0; i < indent; ++i) 
    printf("  ");

  switch (node->kind) {
  case AST_INT:
    printf("Int(%lld)\n", node->int_lit.value);
    break;
  case AST_BOOL:
    printf("Bool(%s)\n", node->bool_lit.value ? "true" : "false");
    break;
  case AST_ATOM:
    printf("Atom(:%s)\n", node->atom_lit.value ? node->atom_lit.value : "?");
    break;
  case AST_LET:
    printf("Let(%s)\n", node->let_stmt.name ? node->let_stmt.name : "?");
    PrintAst(node->let_stmt.value, indent + 1);
    break;
  case AST_SEQ:
    printf("Program\n");
    for (size_t i = 0; i < node->seq.count; ++i) 
      PrintAst(node->seq.items[i], indent + 1);
    break;
  }
}

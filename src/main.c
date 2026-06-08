#include "ast.h"
#include "interp.h"
#include "lexer.h"
#include "parser.h"
#include "tatari.h"
#include "token.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum { MODE_RUN, MODE_AST, MODE_TOKENS } Mode;

static char *read_file(const char *path, size_t *len_out) {
  FILE *fp = fopen(path, "rb");
  if (fp == NULL) {
    tatari_fatal(3, 0, 0, "源文を開けぬ：%s", path);
  }
  if (fseek(fp, 0, SEEK_END) != 0) {
    fclose(fp);
    tatari_fatal(3, 0, 0, "源文の長さを測れぬ：%s", path);
  }
  long end = ftell(fp);
  if (end < 0) {
    fclose(fp);
    tatari_fatal(3, 0, 0, "源文の長さを測れぬ：%s", path);
  }
  if (fseek(fp, 0, SEEK_SET) != 0) {
    fclose(fp);
    tatari_fatal(3, 0, 0, "源文を巻き戻せぬ：%s", path);
  }

  size_t len = (size_t)end;
  char *src = onmyo_xmalloc(len + 1);
  size_t got = fread(src, 1, len, fp);
  if (got != len) {
    fclose(fp);
    free(src);
    tatari_fatal(3, 0, 0, "源文を読み切れぬ：%s", path);
  }
  src[len] = '\0';
  fclose(fp);
  *len_out = len;
  return src;
}

static void usage(void) {
  fprintf(stderr, "使ひ方：onmyo <source.fu>\n");
  fprintf(stderr, "        onmyo --ast <source.fu>\n");
  fprintf(stderr, "        onmyo --tokens <source.fu>\n");
}

int main(int argc, char **argv) {
  Mode mode = MODE_RUN;
  const char *path = NULL;

  if (argc == 2) {
    path = argv[1];
  } else if (argc == 3) {
    if (strcmp(argv[1], "--ast") == 0) {
      mode = MODE_AST;
    } else if (strcmp(argv[1], "--tokens") == 0) {
      mode = MODE_TOKENS;
    } else {
      usage();
      return 3;
    }
    path = argv[2];
  } else {
    usage();
    return 3;
  }

  size_t len = 0;
  char *src = read_file(path, &len);
  TokenArray tokens = lex_source(src, len);

  if (mode == MODE_TOKENS) {
    for (int i = 0; i < tokens.count; i++) {
      tok_print(&tokens.items[i]);
    }
    tok_array_free(&tokens);
    free(src);
    return 0;
  }

  Program *program = parse_program(&tokens);
  if (mode == MODE_AST) {
    ast_print_program(stdout, program);
    ast_free_program(program);
    tok_array_free(&tokens);
    free(src);
    return 0;
  }

  interp_execute(program);
  ast_free_program(program);
  tok_array_free(&tokens);
  free(src);
  return 0;
}

#include "tatari.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void tatari_fatal(int exit_code, int line, int col, const char *fmt, ...) {
  va_list ap;

  fprintf(stderr, "祟り：");
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  va_end(ap);

  if (line > 0 && col > 0) {
    fprintf(stderr, "（行%d・列%d）", line, col);
  }
  fputc('\n', stderr);
  exit(exit_code);
}

void *onmyo_xmalloc(size_t size) {
  void *ptr = malloc(size == 0 ? 1 : size);
  if (ptr == NULL) {
    tatari_fatal(3, 0, 0, "記憶を確保できぬ");
  }
  return ptr;
}

void *onmyo_xcalloc(size_t count, size_t size) {
  void *ptr = calloc(count == 0 ? 1 : count, size == 0 ? 1 : size);
  if (ptr == NULL) {
    tatari_fatal(3, 0, 0, "記憶を確保できぬ");
  }
  return ptr;
}

void *onmyo_xrealloc(void *ptr, size_t size) {
  void *next = realloc(ptr, size == 0 ? 1 : size);
  if (next == NULL) {
    tatari_fatal(3, 0, 0, "記憶を確保できぬ");
  }
  return next;
}

char *onmyo_xstrndup(const char *src, size_t len) {
  char *out = onmyo_xmalloc(len + 1);
  memcpy(out, src, len);
  out[len] = '\0';
  return out;
}

char *onmyo_xstrdup(const char *src) {
  return onmyo_xstrndup(src, strlen(src));
}

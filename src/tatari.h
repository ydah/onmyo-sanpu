#ifndef ONMYO_TATARI_H
#define ONMYO_TATARI_H

#include <stddef.h>

void tatari_fatal(int exit_code, int line, int col, const char *fmt, ...);

void *onmyo_xmalloc(size_t size);
void *onmyo_xcalloc(size_t count, size_t size);
void *onmyo_xrealloc(void *ptr, size_t size);
char *onmyo_xstrndup(const char *src, size_t len);
char *onmyo_xstrdup(const char *src);

#endif

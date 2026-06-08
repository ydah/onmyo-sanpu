#ifndef ONMYO_LEXER_H
#define ONMYO_LEXER_H

#include "token.h"

#include <stddef.h>

TokenArray lex_source(const char *src, size_t len);

#endif

#ifndef ONMYO_PARSER_H
#define ONMYO_PARSER_H

#include "ast.h"
#include "token.h"

Program *parse_program(const TokenArray *tokens);

#endif

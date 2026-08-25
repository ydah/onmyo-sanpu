#ifndef ONMYO_INTERP_H
#define ONMYO_INTERP_H

#include "ast.h"
#include "value.h"

typedef enum { FLOW_NORMAL, FLOW_RETURN, FLOW_BREAK, FLOW_CONTINUE } Flow;

typedef struct {
  Flow flow;
  Value ret;
} ExecResult;

void interp_execute(const Program *program);

#endif

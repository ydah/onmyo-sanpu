#ifndef ONMYO_VALUE_H
#define ONMYO_VALUE_H

#include "token.h"

#include <stdio.h>

typedef enum { V_INT, V_BOOL, V_STR, V_KYO } VKind;

typedef struct {
  VKind kind;
  long long i;
  int b;
  char *s;
} Value;

Value val_int(long long value);
Value val_bool(int value);
Value val_str(const char *value);
Value val_kyo(void);
Value val_copy(const Value *value);
void val_free(Value *value);

Value val_binary(TokKind op, const Value *lhs, const Value *rhs, int line, int col);
Value val_logic(TokKind op, const Value *lhs, const Value *rhs, int line, int col);
Value val_not(const Value *value, int line, int col);
int val_as_bool(const Value *value, int line, int col);
long long val_as_int(const Value *value, int line, int col);
void val_print(FILE *out, const Value *value, int line, int col);

#endif

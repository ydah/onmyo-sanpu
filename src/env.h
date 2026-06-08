#ifndef ONMYO_ENV_H
#define ONMYO_ENV_H

#include "value.h"

typedef struct Env Env;

Env *env_new(void);
void env_free(Env *env);
void env_bind(Env *env, const char *name, const Value *value);
int env_assign(Env *env, const char *name, const Value *value);
Value env_get(const Env *env, const char *name, int line, int col);

#endif

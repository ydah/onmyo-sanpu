#ifndef ONMYO_ENV_H
#define ONMYO_ENV_H

#include "value.h"

typedef struct Env Env;

Env *env_new(Env *outer);
void env_free(Env *env);
int env_declare(Env *env, const char *name, const Value *value);
int env_assign(Env *env, const char *name, const Value *value);
int env_release(Env *env, const char *name);
Value env_get(Env *env, const char *name, int line, int col);

#endif

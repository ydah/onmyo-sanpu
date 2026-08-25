#include "env.h"

#include "tatari.h"

#include <stdlib.h>
#include <string.h>

typedef struct Entry {
  char *name;
  Value value;
  struct Entry *next;
} Entry;

struct Env {
  struct Env *outer;
  Entry *head;
};

Env *env_new(Env *outer) {
  Env *env = onmyo_xcalloc(1, sizeof(Env));
  env->outer = outer;
  return env;
}

void env_free(Env *env) {
  if (env == NULL) return;
  Entry *entry = env->head;
  while (entry != NULL) {
    Entry *next = entry->next;
    free(entry->name);
    val_free(&entry->value);
    free(entry);
    entry = next;
  }
  free(env);
}

static Entry *find_entry(const Env *env, const char *name) {
  for (Entry *entry = env->head; entry != NULL; entry = entry->next) {
    if (strcmp(entry->name, name) == 0) return entry;
  }
  return NULL;
}

int env_declare(Env *env, const char *name, const Value *value) {
  Entry *entry = find_entry(env, name);
  if (entry != NULL) return 0;

  entry = onmyo_xcalloc(1, sizeof(Entry));
  entry->name = onmyo_xstrdup(name);
  entry->value = val_copy(value);
  entry->next = env->head;
  env->head = entry;
  return 1;
}

int env_assign(Env *env, const char *name, const Value *value) {
  for (Env *scope = env; scope != NULL; scope = scope->outer) {
    Entry *entry = find_entry(scope, name);
    if (entry == NULL) continue;
    val_free(&entry->value);
    entry->value = val_copy(value);
    return 1;
  }
  return 0;
}

int env_release(Env *env, const char *name) {
  Entry **link = &env->head;
  while (*link != NULL) {
    Entry *entry = *link;
    if (strcmp(entry->name, name) == 0) {
      *link = entry->next;
      free(entry->name);
      val_free(&entry->value);
      free(entry);
      return 1;
    }
    link = &entry->next;
  }
  return 0;
}

Value env_get(Env *env, const char *name, int line, int col) {
  for (Env *scope = env; scope != NULL; scope = scope->outer) {
    Entry *entry = find_entry(scope, name);
    if (entry != NULL) return val_copy(&entry->value);
  }
  tatari_fatal(1, line, col, "式神『%s』は此の界に在らず", name);
  return val_kyo();
}

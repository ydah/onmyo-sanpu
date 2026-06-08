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
  Entry *head;
};

Env *env_new(void) {
  return onmyo_xcalloc(1, sizeof(Env));
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

void env_bind(Env *env, const char *name, const Value *value) {
  Entry *entry = find_entry(env, name);
  if (entry != NULL) {
    val_free(&entry->value);
    entry->value = val_copy(value);
    return;
  }

  entry = onmyo_xcalloc(1, sizeof(Entry));
  entry->name = onmyo_xstrdup(name);
  entry->value = val_copy(value);
  entry->next = env->head;
  env->head = entry;
}

int env_assign(Env *env, const char *name, const Value *value) {
  Entry *entry = find_entry(env, name);
  if (entry == NULL) return 0;
  val_free(&entry->value);
  entry->value = val_copy(value);
  return 1;
}

Value env_get(const Env *env, const char *name, int line, int col) {
  Entry *entry = find_entry(env, name);
  if (entry == NULL) {
    tatari_fatal(1, line, col, "式神『%s』は召喚されておらぬ", name);
  }
  return val_copy(&entry->value);
}

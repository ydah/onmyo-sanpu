#include "value.h"

#include "tatari.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Value val_int(long long value) {
  Value out;
  out.kind = V_INT;
  out.i = value;
  out.b = 0;
  out.s = NULL;
  return out;
}

Value val_bool(int value) {
  Value out;
  out.kind = V_BOOL;
  out.i = 0;
  out.b = value ? 1 : 0;
  out.s = NULL;
  return out;
}

Value val_str(const char *value) {
  Value out;
  out.kind = V_STR;
  out.i = 0;
  out.b = 0;
  out.s = onmyo_xstrdup(value);
  return out;
}

Value val_kyo(void) {
  Value out;
  out.kind = V_KYO;
  out.i = 0;
  out.b = 0;
  out.s = NULL;
  return out;
}

Value val_copy(const Value *value) {
  switch (value->kind) {
  case V_INT:
    return val_int(value->i);
  case V_BOOL:
    return val_bool(value->b);
  case V_STR:
    return val_str(value->s);
  case V_KYO:
    return val_kyo();
  }
  return val_kyo();
}

void val_free(Value *value) {
  if (value == NULL) return;
  if (value->kind == V_STR) {
    free(value->s);
  }
  value->kind = V_KYO;
  value->s = NULL;
  value->i = 0;
  value->b = 0;
}

static const char *vkind_name(VKind kind) {
  switch (kind) {
  case V_INT: return "籌";
  case V_STR: return "真言";
  case V_BOOL: return "卦";
  case V_KYO: return "虚";
  }
  return "不明";
}

static void type_tatari(int line, int col, const char *op, const Value *lhs, const Value *rhs) {
  tatari_fatal(1, line, col, "『%s』は %s と %s を結び得ず",
               op, vkind_name(lhs->kind), vkind_name(rhs->kind));
}

long long val_as_int(const Value *value, int line, int col) {
  if (value->kind != V_INT) {
    tatari_fatal(1, line, col, "籌にあらず");
  }
  return value->i;
}

int val_as_bool(const Value *value, int line, int col) {
  if (value->kind != V_BOOL) {
    tatari_fatal(1, line, col, "卦にあらず");
  }
  return value->b;
}

static Value val_concat(const Value *lhs, const Value *rhs) {
  size_t left_len = strlen(lhs->s);
  size_t right_len = strlen(rhs->s);
  char *joined = onmyo_xmalloc(left_len + right_len + 1);
  memcpy(joined, lhs->s, left_len);
  memcpy(joined + left_len, rhs->s, right_len);
  joined[left_len + right_len] = '\0';
  Value out;
  out.kind = V_STR;
  out.i = 0;
  out.b = 0;
  out.s = joined;
  return out;
}

static int compare_values(const Value *lhs, const Value *rhs, int line, int col) {
  if (lhs->kind == V_INT && rhs->kind == V_INT) {
    if (lhs->i < rhs->i) return -1;
    if (lhs->i > rhs->i) return 1;
    return 0;
  }
  if (lhs->kind == V_STR && rhs->kind == V_STR) {
    int cmp = strcmp(lhs->s, rhs->s);
    if (cmp < 0) return -1;
    if (cmp > 0) return 1;
    return 0;
  }
  type_tatari(line, col, "比較", lhs, rhs);
  return 0;
}

Value val_binary(TokKind op, const Value *lhs, const Value *rhs, int line, int col) {
  switch (op) {
  case T_SHOJI:
    if (lhs->kind == V_INT && rhs->kind == V_INT) return val_int(lhs->i + rhs->i);
    if (lhs->kind == V_STR && rhs->kind == V_STR) return val_concat(lhs, rhs);
    type_tatari(line, col, "生じ", lhs, rhs);
    break;
  case T_KOKUSHI:
    return val_int(val_as_int(lhs, line, col) - val_as_int(rhs, line, col));
  case T_JOJI:
    return val_int(val_as_int(lhs, line, col) * val_as_int(rhs, line, col));
  case T_HARAI: {
    long long right = val_as_int(rhs, line, col);
    if (right == 0) tatari_fatal(1, line, col, "零にて祓ふべからず");
    return val_int(val_as_int(lhs, line, col) / right);
  }
  case T_KEGASHI: {
    long long right = val_as_int(rhs, line, col);
    if (right == 0) tatari_fatal(1, line, col, "零にて穢すべからず");
    return val_int(val_as_int(lhs, line, col) % right);
  }
  case T_ONAJIKU:
    if (lhs->kind != rhs->kind) return val_bool(0);
    if (lhs->kind == V_INT) return val_bool(lhs->i == rhs->i);
    if (lhs->kind == V_BOOL) return val_bool(lhs->b == rhs->b);
    if (lhs->kind == V_STR) return val_bool(strcmp(lhs->s, rhs->s) == 0);
    return val_bool(1);
  case T_ONAJIKARAZU:
    if (lhs->kind != rhs->kind) return val_bool(1);
    if (lhs->kind == V_INT) return val_bool(lhs->i != rhs->i);
    if (lhs->kind == V_BOOL) return val_bool(lhs->b != rhs->b);
    if (lhs->kind == V_STR) return val_bool(strcmp(lhs->s, rhs->s) != 0);
    return val_bool(0);
  case T_MASARI:
    return val_bool(compare_values(lhs, rhs, line, col) > 0);
  case T_OTORI:
    return val_bool(compare_values(lhs, rhs, line, col) < 0);
  case T_MASARAZU:
    return val_bool(compare_values(lhs, rhs, line, col) <= 0);
  case T_OTORAZU:
    return val_bool(compare_values(lhs, rhs, line, col) >= 0);
  default:
    tatari_fatal(1, line, col, "未知の演算である");
  }
  return val_kyo();
}

Value val_not(const Value *value, int line, int col) {
  return val_bool(!val_as_bool(value, line, col));
}

void val_print(FILE *out, const Value *value, int line, int col) {
  switch (value->kind) {
  case V_INT:
    fprintf(out, "%lld\n", value->i);
    break;
  case V_BOOL:
    fputs(value->b ? "吉\n" : "凶\n", out);
    break;
  case V_STR:
    fprintf(out, "%s\n", value->s);
    break;
  case V_KYO:
    tatari_fatal(1, line, col, "虚は唱へられぬ");
  }
}

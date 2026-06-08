#ifndef ONMYO_TOKEN_H
#define ONMYO_TOKEN_H

#include <stddef.h>

typedef enum {
  T_NO_GYOHO,
  T_TAMAWARITE,
  T_SHU_SURUNI,
  T_SHU_SESHIMU,
  T_KENJI,
  T_MOTTE,
  T_TO,
  T_KECHIGAN,
  T_SHICCHI_JOJU,
  T_SHIKIGAMI,
  T_YOBI,
  T_TSUKASESHIMU,
  T_ARATAME,
  T_WO,
  T_NI,
  T_NITE,
  T_YORI,
  T_URANAUNI,
  T_SHIKARAZUSHITE_URANAUNI,
  T_SHIKARAZUBA,
  T_BA,
  T_YORI_FROM,
  T_ITARU,
  T_AYUMI,
  T_HENBAI,
  T_KAGIRI_HENBAI,
  T_TAKUSEN,
  T_TONAE,
  T_SHOJI,
  T_KOKUSHI,
  T_KOSHI,
  T_HARAI,
  T_KEGASHI,
  T_ONAJIKU,
  T_KOTONARI,
  T_MASARI,
  T_OTORI,
  T_MASARU_KA_ONAJIKU,
  T_OTORU_KA_ONAJIKU,
  T_KATSU,
  T_ARUIWA,
  T_NIARAZU,
  T_HAN,
  T_INT,
  T_STR,
  T_NAME,
  T_YO,
  T_IN,
  T_KYO,
  T_KEKKAI_OPEN,
  T_KEKKAI_CLOSE,
  T_HARAUNI_KEGARE_NAKU,
  T_KYUKYU,
  T_EOF
} TokKind;

typedef struct {
  TokKind kind;
  const char *lexeme;
  size_t len;
  long long ival;
  char *sval;
  int line;
  int col;
} Token;

typedef struct {
  Token *items;
  int count;
  int cap;
} TokenArray;

const char *tok_kind_name(TokKind kind);
void tok_array_push(TokenArray *array, Token token);
void tok_array_free(TokenArray *array);
void tok_print(const Token *token);

#endif

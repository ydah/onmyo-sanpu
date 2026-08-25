#ifndef ONMYO_TOKEN_H
#define ONMYO_TOKEN_H

#include <stddef.h>

typedef enum {
  T_TO_MOSU_GYOHO,
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
  T_HANACHI,
  T_KEKKAI_WO_HARI,
  T_HENBAI_WO_TOME,
  T_TSUGI_NO_HO,
  T_WO,
  T_WA,
  T_NI,
  T_NITE,
  T_YORI,
  T_URANAUNI,
  T_SHIKARAZUSHITE_URANAUNI,
  T_SHIKARAZUBA,
  T_BA,
  T_WOSHITE,
  T_NI_ITARU_MADE,
  T_HO_WO,
  T_TOSHI,
  T_HENBAI,
  T_KAGIRI_HENBAI,
  T_TAKUSEN,
  T_TONAE,
  T_SHOJI,
  T_KOKUSHI,
  T_JOJI,
  T_HARAI,
  T_KEGASHI,
  T_ONAJIKU,
  T_ONAJIKARAZU,
  T_MASARI,
  T_MASARAZU,
  T_OTORI,
  T_OTORAZU,
  T_KATSU,
  T_ARUIWA,
  T_NIARAZU,
  T_SARANI,
  T_KORE,
  T_INT,
  T_STR,
  T_NAME,
  T_KICHI,
  T_KYO_BAD,
  T_KYO_VOID,
  T_IN_SIGN,
  T_YO_SIGN,
  T_KEKKAI_OPEN,
  T_KEKKAI_CLOSE,
  T_HARAUNI_KEGARE_NAKU,
  T_KYUKYU,
  T_OLD_NO_GYOHO,
  T_OLD_KOSHI,
  T_OLD_KOTONARI,
  T_OLD_MASARU_KA_ONAJIKU,
  T_OLD_OTORU_KA_ONAJIKU,
  T_OLD_YORI_FROM,
  T_OLD_ITARU,
  T_OLD_AYUMI,
  T_OLD_HAN,
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

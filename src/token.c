#include "token.h"

#include "tatari.h"

#include <stdio.h>
#include <stdlib.h>

const char *tok_kind_name(TokKind kind) {
  switch (kind) {
  case T_TO_MOSU_GYOHO: return "と申す行法";
  case T_TAMAWARITE: return "賜りて";
  case T_SHU_SURUNI: return "修するに";
  case T_SHU_SESHIMU: return "修せしむ";
  case T_KENJI: return "献じ";
  case T_MOTTE: return "以て";
  case T_TO: return "と";
  case T_KECHIGAN: return "結願";
  case T_SHICCHI_JOJU: return "悉地成就";
  case T_SHIKIGAMI: return "式神";
  case T_YOBI: return "喚び";
  case T_TSUKASESHIMU: return "憑かせしむ";
  case T_ARATAME: return "改め";
  case T_HANACHI: return "放ち";
  case T_KEKKAI_WO_HARI: return "結界を張り";
  case T_HENBAI_WO_TOME: return "反閇を止め";
  case T_TSUGI_NO_HO: return "次の歩に移り";
  case T_WO: return "を";
  case T_WA: return "は";
  case T_NI: return "に";
  case T_NITE: return "にて";
  case T_YORI: return "より";
  case T_URANAUNI: return "占ふに";
  case T_SHIKARAZUSHITE_URANAUNI: return "然らずして占ふに";
  case T_SHIKARAZUBA: return "然らずば";
  case T_BA: return "ば";
  case T_WOSHITE: return "をして";
  case T_NI_ITARU_MADE: return "に至るまで";
  case T_HO_WO: return "歩を";
  case T_TOSHI: return "とし";
  case T_HENBAI: return "反閇せしむ";
  case T_KAGIRI_HENBAI: return "限り反閇せしむ";
  case T_TAKUSEN: return "託宣を仰ぎ";
  case T_TONAE: return "唱へ";
  case T_SHOJI: return "生じ";
  case T_KOKUSHI: return "剋し";
  case T_JOJI: return "乗じ";
  case T_HARAI: return "祓ひ";
  case T_KEGASHI: return "穢し";
  case T_ONAJIKU: return "同じく";
  case T_ONAJIKARAZU: return "同じからず";
  case T_MASARI: return "勝り";
  case T_MASARAZU: return "勝らず";
  case T_OTORI: return "劣り";
  case T_OTORAZU: return "劣らず";
  case T_KATSU: return "且つ";
  case T_ARUIWA: return "或いは";
  case T_NIARAZU: return "にあらず";
  case T_SARANI: return "更に";
  case T_KORE: return "これ";
  case T_INT: return "INT";
  case T_STR: return "STR";
  case T_NAME: return "NAME";
  case T_SHIKIGAMI_NAME: return "式神名";
  case T_KICHI: return "吉";
  case T_KYO_BAD: return "凶";
  case T_KYO_VOID: return "虚";
  case T_HARAUNI_KEGARE_NAKU: return "祓ふに穢れ無く";
  case T_KYUKYU: return "急急如律令";
  case T_EOF: return "EOF";
  }
  return "?";
}

void tok_array_push(TokenArray *array, Token token) {
  if (array->count == array->cap) {
    array->cap = array->cap == 0 ? 64 : array->cap * 2;
    array->items = onmyo_xrealloc(array->items, sizeof(Token) * (size_t)array->cap);
  }
  array->items[array->count++] = token;
}

void tok_array_free(TokenArray *array) {
  for (int i = 0; i < array->count; i++) {
    free(array->items[i].sval);
  }
  free(array->items);
  array->items = NULL;
  array->count = 0;
  array->cap = 0;
}

void tok_print(const Token *token) {
  if (token->kind == T_EOF) {
    printf("%d:%d EOF\n", token->line, token->col);
    return;
  }
  printf("%d:%d %-24s", token->line, token->col, tok_kind_name(token->kind));
  if (token->kind == T_INT) {
    printf(" %lld", token->ival);
  } else if (token->kind == T_STR || token->kind == T_NAME || token->kind == T_SHIKIGAMI_NAME) {
    printf(" %s", token->sval);
  } else {
    printf(" %.*s", (int)token->len, token->lexeme);
  }
  putchar('\n');
}

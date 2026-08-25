#include "lexer.h"

#include "tatari.h"

#include <string.h>

typedef struct { const char *text; TokKind kind; } Keyword;

static const Keyword KEYWORDS[] = {
  {"然らずして占ふに", T_SHIKARAZUSHITE_URANAUNI},
  {"祓ふに穢れ無く", T_HARAUNI_KEGARE_NAKU},
  {"限り反閇せしむ", T_KAGIRI_HENBAI},
  {"次の歩に移り", T_TSUGI_NO_HO},
  {"勝るか同じく", T_OLD_MASARU_KA_ONAJIKU},
  {"劣るか同じく", T_OLD_OTORU_KA_ONAJIKU},
  {"と申す行法", T_TO_MOSU_GYOHO},
  {"に至るまで", T_NI_ITARU_MADE},
  {"反閇を止め", T_HENBAI_WO_TOME},
  {"同じからず", T_ONAJIKARAZU},
  {"託宣を仰ぎ", T_TAKUSEN},
  {"急急如律令", T_KYUKYU},
  {"反閇せしむ", T_HENBAI},
  {"憑かせしむ", T_TSUKASESHIMU},
  {"結界を張り", T_KEKKAI_WO_HARI},
  {"悉地成就", T_SHICCHI_JOJU},
  {"修するに", T_SHU_SURUNI},
  {"修せしむ", T_SHU_SESHIMU},
  {"然らずば", T_SHIKARAZUBA},
  {"にあらず", T_NIARAZU},
  {"勝らず", T_MASARAZU},
  {"劣らず", T_OTORAZU},
  {"占ふに", T_URANAUNI},
  {"之行法", T_OLD_NO_GYOHO},
  {"賜りて", T_TAMAWARITE},
  {"同じく", T_ONAJIKU},
  {"異なり", T_OLD_KOTONARI},
  {"或いは", T_ARUIWA},
  {"をして", T_WOSHITE},
  {"結願", T_KECHIGAN},
  {"式神", T_SHIKIGAMI},
  {"喚び", T_YOBI},
  {"改め", T_ARATAME},
  {"放ち", T_HANACHI},
  {"献じ", T_KENJI},
  {"以て", T_MOTTE},
  {"歩を", T_HO_WO},
  {"唱へ", T_TONAE},
  {"生じ", T_SHOJI},
  {"剋し", T_KOKUSHI},
  {"乗じ", T_JOJI},
  {"蠱し", T_OLD_KOSHI},
  {"祓ひ", T_HARAI},
  {"穢し", T_KEGASHI},
  {"勝り", T_MASARI},
  {"劣り", T_OTORI},
  {"且つ", T_KATSU},
  {"更に", T_SARANI},
  {"にて", T_NITE},
  {"より", T_YORI},
  {"これ", T_KORE},
  {"とし", T_TOSHI},
  {"歩み", T_OLD_AYUMI},
  {"吉", T_KICHI},
  {"凶", T_KYO_BAD},
  {"虚", T_KYO_VOID},
  {"自", T_OLD_YORI_FROM},
  {"至", T_OLD_ITARU},
  {"と", T_TO},
  {"を", T_WO},
  {"に", T_NI},
  {"ば", T_BA},
  {"は", T_WA},
  {"反", T_OLD_HAN},
};

static const char *SHIKIGAMI_NAMES[] = {
  "貴人", "螣蛇", "朱雀", "六合", "勾陳", "青龍", "天空", "白虎", "太常", "玄武", "太陰", "天后",
  "甲", "乙", "丙", "丁", "戊", "己", "庚", "辛", "壬", "癸",
  "子", "丑", "寅", "卯", "辰", "巳", "午", "未", "申", "酉", "戌", "亥",
};

static int utf8_len(unsigned char ch) {
  if (ch < 0x80) return 1;
  if ((ch & 0xE0) == 0xC0) return 2;
  if ((ch & 0xF0) == 0xE0) return 3;
  if ((ch & 0xF8) == 0xF0) return 4;
  return 1;
}

static int starts_with(const char *src, size_t len, size_t pos, const char *lit) {
  size_t n = strlen(lit);
  return pos + n <= len && memcmp(src + pos, lit, n) == 0;
}

static void keywords_assert_ordered(void) {
  size_t count = sizeof(KEYWORDS) / sizeof(KEYWORDS[0]);
  for (size_t i = 0; i < count; i++) {
    size_t short_len = strlen(KEYWORDS[i].text);
    for (size_t j = i + 1; j < count; j++) {
      size_t long_len = strlen(KEYWORDS[j].text);
      if (short_len < long_len && memcmp(KEYWORDS[i].text, KEYWORDS[j].text, short_len) == 0) {
        tatari_fatal(3, 0, 0, "語彙表の序が乱れたり");
      }
    }
  }
}

static void advance_bytes(const char *src, size_t len, size_t *pos, int *line, int *col, size_t n) {
  size_t end = *pos + n;
  while (*pos < end && *pos < len) {
    unsigned char ch = (unsigned char)src[*pos];
    if (ch == '\n') {
      (*line)++;
      *col = 1;
      (*pos)++;
      continue;
    }
    int width = utf8_len(ch);
    if (*pos + (size_t)width > len || *pos + (size_t)width > end) width = 1;
    *pos += (size_t)width;
    (*col)++;
  }
}

static int match_digit(const char *src, size_t len, size_t pos, int *digit, size_t *dlen) {
  static const char *DIGITS[] = {"無", "臨", "兵", "闘", "者", "皆", "陣", "列", "在", "前"};
  for (int i = 0; i < 10; i++) {
    size_t n = strlen(DIGITS[i]);
    if (pos + n <= len && memcmp(src + pos, DIGITS[i], n) == 0) {
      *digit = i;
      *dlen = n;
      return 1;
    }
  }
  return 0;
}

static int is_number_start(const char *src, size_t len, size_t pos) {
  while (starts_with(src, len, pos, "陰") || starts_with(src, len, pos, "陽")) pos += strlen("陰");
  int digit = 0;
  size_t dlen = 0;
  return match_digit(src, len, pos, &digit, &dlen);
}

static const Keyword *keyword_at(const char *src, size_t len, size_t pos) {
  size_t count = sizeof(KEYWORDS) / sizeof(KEYWORDS[0]);
  for (size_t i = 0; i < count; i++) if (starts_with(src, len, pos, KEYWORDS[i].text)) return &KEYWORDS[i];
  return NULL;
}

static const char *shikigami_at(const char *src, size_t len, size_t pos) {
  size_t count = sizeof(SHIKIGAMI_NAMES) / sizeof(SHIKIGAMI_NAMES[0]);
  for (size_t i = 0; i < count; i++) if (starts_with(src, len, pos, SHIKIGAMI_NAMES[i])) return SHIKIGAMI_NAMES[i];
  return NULL;
}

static void skip_ignored(const char *src, size_t len, size_t *pos, int *line, int *col) {
  for (;;) {
    if (*pos >= len) return;
    unsigned char ch = (unsigned char)src[*pos];
    if (ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n') {
      advance_bytes(src, len, pos, line, col, 1);
      continue;
    }
    if (starts_with(src, len, *pos, "　") || starts_with(src, len, *pos, "、") ||
        starts_with(src, len, *pos, "。") || starts_with(src, len, *pos, "・")) {
      advance_bytes(src, len, pos, line, col, strlen("、"));
      continue;
    }
    if (starts_with(src, len, *pos, "註")) {
      while (*pos < len && src[*pos] != '\n' && src[*pos] != '\r') {
        advance_bytes(src, len, pos, line, col, (size_t)utf8_len((unsigned char)src[*pos]));
      }
      continue;
    }
    return;
  }
}

static Token plain_token(TokKind kind, const char *lexeme, size_t len, int line, int col) {
  Token token = {kind, lexeme, len, 0, NULL, line, col};
  return token;
}

static Token string_token(const char *src, size_t len, size_t *pos, int *line, int *col) {
  size_t start = *pos;
  int start_line = *line;
  int start_col = *col;
  advance_bytes(src, len, pos, line, col, strlen("阿"));
  size_t content = *pos;
  while (*pos < len && !starts_with(src, len, *pos, "吽")) {
    advance_bytes(src, len, pos, line, col, (size_t)utf8_len((unsigned char)src[*pos]));
  }
  if (*pos >= len) {
    tatari_fatal(2, start_line, start_col, "真言は 阿 に始まり 吽 に終はる。閉ぢられぬ");
  }
  Token token = plain_token(T_STR, src + start, *pos + strlen("吽") - start, start_line, start_col);
  token.sval = onmyo_xstrndup(src + content, *pos - content);
  advance_bytes(src, len, pos, line, col, strlen("吽"));
  return token;
}

static Token number_token(const char *src, size_t len, size_t *pos, int *line, int *col) {
  size_t start = *pos;
  int start_line = *line;
  int start_col = *col;
  int negative = 0;
  long long value = 0;
  while (starts_with(src, len, *pos, "陰") || starts_with(src, len, *pos, "陽")) {
    if (starts_with(src, len, *pos, "陰")) negative = !negative;
    advance_bytes(src, len, pos, line, col, strlen("陰"));
  }
  for (;;) {
    int digit = 0;
    size_t dlen = 0;
    if (!match_digit(src, len, *pos, &digit, &dlen)) break;
    value = value * 10 + digit;
    advance_bytes(src, len, pos, line, col, dlen);
  }
  Token token = plain_token(T_INT, src + start, *pos - start, start_line, start_col);
  token.ival = negative ? -value : value;
  return token;
}

static Token shikigami_token(const char *src, size_t len, size_t *pos, int *line, int *col) {
  const char *name = shikigami_at(src, len, *pos);
  size_t n = strlen(name);
  Token token = plain_token(T_SHIKIGAMI_NAME, src + *pos, n, *line, *col);
  token.sval = onmyo_xstrdup(name);
  advance_bytes(src, len, pos, line, col, n);
  return token;
}

static int name_boundary(const char *src, size_t len, size_t pos) {
  if (pos >= len) return 1;
  unsigned char ch = (unsigned char)src[pos];
  if (ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n') return 1;
  if (starts_with(src, len, pos, "　") || starts_with(src, len, pos, "、") ||
      starts_with(src, len, pos, "。") || starts_with(src, len, pos, "・") ||
      starts_with(src, len, pos, "註") || starts_with(src, len, pos, "阿")) return 1;
  if (is_number_start(src, len, pos) || keyword_at(src, len, pos) != NULL || shikigami_at(src, len, pos) != NULL) return 1;
  return 0;
}

static Token name_token(const char *src, size_t len, size_t *pos, int *line, int *col) {
  size_t start = *pos;
  int start_line = *line;
  int start_col = *col;
  while (*pos < len) {
    int digit = 0;
    size_t dlen = 0;
    size_t digit_pos = *pos;
    while (starts_with(src, len, digit_pos, "陰") || starts_with(src, len, digit_pos, "陽")) digit_pos += strlen("陰");
    if (match_digit(src, len, digit_pos, &digit, &dlen)) {
      tatari_fatal(2, *line, *col, "行法の名に九字を含むべからず。『%.*s』は九なり", (int)dlen, src + digit_pos);
    }
    if (name_boundary(src, len, *pos)) break;
    advance_bytes(src, len, pos, line, col, (size_t)utf8_len((unsigned char)src[*pos]));
  }
  Token token = plain_token(T_NAME, src + start, *pos - start, start_line, start_col);
  token.sval = onmyo_xstrndup(src + start, *pos - start);
  return token;
}

static void reject_old_boundary(const char *src, size_t len, size_t pos, int line, int col) {
  if (starts_with(src, len, pos, "「") || starts_with(src, len, pos, "」"))
    tatari_fatal(2, line, col, "文字列は 阿 に始まり 吽 に終はる。鉤括弧は廃されたり");
  if (starts_with(src, len, pos, "『") || starts_with(src, len, pos, "』"))
    tatari_fatal(2, line, col, "名に括弧を付くべからず。式神は十二天将、行法は無印なり");
  if (starts_with(src, len, pos, "〔") || starts_with(src, len, pos, "〕"))
    tatari_fatal(2, line, col, "括りは廃されたり。式神に憑かせて分かつべし");
  if (starts_with(src, len, pos, "〘") || starts_with(src, len, pos, "〙"))
    tatari_fatal(2, line, col, "注釈は 註 より行末までなり");
}

TokenArray lex_source(const char *src, size_t len) {
  keywords_assert_ordered();
  TokenArray tokens = {0};
  size_t pos = 0;
  int line = 1;
  int col = 1;
  while (pos < len) {
    skip_ignored(src, len, &pos, &line, &col);
    if (pos >= len) break;
    reject_old_boundary(src, len, pos, line, col);
    if (starts_with(src, len, pos, "阿")) {
      tok_array_push(&tokens, string_token(src, len, &pos, &line, &col));
      continue;
    }
    if (is_number_start(src, len, pos)) {
      tok_array_push(&tokens, number_token(src, len, &pos, &line, &col));
      continue;
    }
    const Keyword *keyword = keyword_at(src, len, pos);
    if (keyword != NULL) {
      size_t n = strlen(keyword->text);
      tok_array_push(&tokens, plain_token(keyword->kind, src + pos, n, line, col));
      advance_bytes(src, len, &pos, &line, &col, n);
      continue;
    }
    if (shikigami_at(src, len, pos) != NULL) {
      tok_array_push(&tokens, shikigami_token(src, len, &pos, &line, &col));
      continue;
    }
    tok_array_push(&tokens, name_token(src, len, &pos, &line, &col));
  }
  tok_array_push(&tokens, plain_token(T_EOF, src + len, 0, line, col));
  return tokens;
}

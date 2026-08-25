#include "lexer.h"

#include "tatari.h"

#include <string.h>

typedef struct {
  const char *text;
  TokKind kind;
} Keyword;

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
  {"陽", T_YO_SIGN},
  {"陰", T_IN_SIGN},
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

static void keywords_assert_ordered(void) {
  size_t count = sizeof(KEYWORDS) / sizeof(KEYWORDS[0]);
  for (size_t i = 1; i < count; i++) {
    if (strlen(KEYWORDS[i - 1].text) < strlen(KEYWORDS[i].text)) {
      tatari_fatal(3, 0, 0, "語彙表の序が乱れたり");
    }
  }
}

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
    int clen = utf8_len(ch);
    if (*pos + (size_t)clen > len || *pos + (size_t)clen > end) {
      clen = 1;
    }
    *pos += (size_t)clen;
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
  while (starts_with(src, len, pos, "陰") || starts_with(src, len, pos, "陽")) {
    pos += strlen("陰");
  }
  int digit = 0;
  size_t dlen = 0;
  return match_digit(src, len, pos, &digit, &dlen);
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
    if (starts_with(src, len, *pos, "〘")) {
      int start_line = *line;
      int start_col = *col;
      advance_bytes(src, len, pos, line, col, strlen("〘"));
      while (*pos < len && !starts_with(src, len, *pos, "〙")) {
        advance_bytes(src, len, pos, line, col, (size_t)utf8_len((unsigned char)src[*pos]));
      }
      if (*pos >= len) {
        tatari_fatal(2, start_line, start_col, "注釈が閉じられぬ");
      }
      advance_bytes(src, len, pos, line, col, strlen("〙"));
      continue;
    }
    return;
  }
}

static Token plain_token(TokKind kind, const char *lexeme, size_t len, int line, int col) {
  Token token;
  token.kind = kind;
  token.lexeme = lexeme;
  token.len = len;
  token.ival = 0;
  token.sval = NULL;
  token.line = line;
  token.col = col;
  return token;
}

static Token string_like_token(TokKind kind, const char *src, size_t content_start, size_t content_len,
                               size_t whole_start, size_t whole_len, int line, int col) {
  Token token = plain_token(kind, src + whole_start, whole_len, line, col);
  token.sval = onmyo_xstrndup(src + content_start, content_len);
  return token;
}

static Token lex_string_or_name(const char *src, size_t len, size_t *pos, int *line, int *col,
                                const char *open, const char *close, TokKind kind) {
  size_t start = *pos;
  int start_line = *line;
  int start_col = *col;
  size_t open_len = strlen(open);
  size_t close_len = strlen(close);

  advance_bytes(src, len, pos, line, col, open_len);
  size_t content_start = *pos;
  while (*pos < len && !starts_with(src, len, *pos, close)) {
    advance_bytes(src, len, pos, line, col, (size_t)utf8_len((unsigned char)src[*pos]));
  }
  if (*pos >= len) {
    tatari_fatal(2, start_line, start_col, kind == T_STR ? "真言が閉じられぬ" : "名が閉じられぬ");
  }
  size_t content_len = *pos - content_start;
  if (kind == T_NAME && content_len == 0) {
    tatari_fatal(2, start_line, start_col, "名が空である");
  }
  advance_bytes(src, len, pos, line, col, close_len);
  return string_like_token(kind, src, content_start, content_len, start, *pos - start, start_line, start_col);
}

static Token lex_number(const char *src, size_t len, size_t *pos, int *line, int *col) {
  size_t start = *pos;
  int start_line = *line;
  int start_col = *col;
  int negative = 0;
  long long value = 0;

  while (starts_with(src, len, *pos, "陰") || starts_with(src, len, *pos, "陽")) {
    if (starts_with(src, len, *pos, "陰")) negative = !negative;
    advance_bytes(src, len, pos, line, col, strlen("陰"));
  }

  int saw_digit = 0;
  for (;;) {
    int digit = 0;
    size_t dlen = 0;
    if (!match_digit(src, len, *pos, &digit, &dlen)) break;
    saw_digit = 1;
    value = value * 10 + digit;
    advance_bytes(src, len, pos, line, col, dlen);
  }

  if (!saw_digit) {
    tatari_fatal(2, start_line, start_col, "数が成り立たぬ");
  }
  if (negative) value = -value;

  Token token = plain_token(T_INT, src + start, *pos - start, start_line, start_col);
  token.ival = value;
  return token;
}

static int lex_keyword(const char *src, size_t len, size_t *pos, int *line, int *col, Token *out) {
  size_t nkeywords = sizeof(KEYWORDS) / sizeof(KEYWORDS[0]);
  for (size_t i = 0; i < nkeywords; i++) {
    size_t n = strlen(KEYWORDS[i].text);
    if (starts_with(src, len, *pos, KEYWORDS[i].text)) {
      *out = plain_token(KEYWORDS[i].kind, src + *pos, n, *line, *col);
      advance_bytes(src, len, pos, line, col, n);
      return 1;
    }
  }
  return 0;
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

    if (starts_with(src, len, pos, "「")) {
      tok_array_push(&tokens, lex_string_or_name(src, len, &pos, &line, &col, "「", "」", T_STR));
      continue;
    }
    if (starts_with(src, len, pos, "『")) {
      tok_array_push(&tokens, lex_string_or_name(src, len, &pos, &line, &col, "『", "』", T_NAME));
      continue;
    }
    if (starts_with(src, len, pos, "〔")) {
      Token token = plain_token(T_KEKKAI_OPEN, src + pos, strlen("〔"), line, col);
      advance_bytes(src, len, &pos, &line, &col, strlen("〔"));
      tok_array_push(&tokens, token);
      continue;
    }
    if (starts_with(src, len, pos, "〕")) {
      Token token = plain_token(T_KEKKAI_CLOSE, src + pos, strlen("〕"), line, col);
      advance_bytes(src, len, &pos, &line, &col, strlen("〕"));
      tok_array_push(&tokens, token);
      continue;
    }
    if (is_number_start(src, len, pos)) {
      tok_array_push(&tokens, lex_number(src, len, &pos, &line, &col));
      continue;
    }

    Token keyword;
    if (lex_keyword(src, len, &pos, &line, &col, &keyword)) {
      tok_array_push(&tokens, keyword);
      continue;
    }

    int bad_col = col;
    int bad_line = line;
    int clen = utf8_len((unsigned char)src[pos]);
    size_t bad_len = (pos + (size_t)clen <= len) ? (size_t)clen : 1;
    tatari_fatal(2, bad_line, bad_col, "読み解けぬ字句 '%.*s'", (int)bad_len, src + pos);
  }

  Token eof = plain_token(T_EOF, src + len, 0, line, col);
  tok_array_push(&tokens, eof);
  return tokens;
}

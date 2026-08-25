#include "parser.h"

#include "tatari.h"

#include <stdlib.h>
#include <string.h>

typedef struct {
  const TokenArray *tokens;
  int pos;
  int loop_depth;
  TokKind expr_stop;
} Parser;

typedef struct { Stmt **items; int count; int cap; } StmtVec;
typedef struct { Expr **items; int count; int cap; } ExprVec;
typedef struct { char **items; int count; int cap; } StrVec;
typedef struct { Rite **items; int count; int cap; } RiteVec;

typedef struct {
  TokKind pred;
  TokKind case1;
  TokKind case2;
  const char *name;
} CaseFrame;

static const CaseFrame FRAMES[] = {
  {T_SHOJI, T_NI, T_WO, "生じ"},
  {T_KOKUSHI, T_YORI, T_WO, "剋し"},
  {T_JOJI, T_NI, T_WO, "乗じ"},
  {T_HARAI, T_WO, T_NITE, "祓ひ"},
  {T_KEGASHI, T_WO, T_NITE, "穢し"},
  {T_HARAUNI_KEGARE_NAKU, T_WA, T_NITE, "祓ふに穢れ無く"},
  {T_ONAJIKU, T_WA, T_NI, "同じく"},
  {T_ONAJIKARAZU, T_WA, T_NI, "同じからず"},
  {T_MASARI, T_WA, T_NI, "勝り"},
  {T_MASARAZU, T_WA, T_NI, "勝らず"},
  {T_OTORI, T_WA, T_NI, "劣り"},
  {T_OTORAZU, T_WA, T_NI, "劣らず"},
};

static Expr *parse_expr(Parser *parser);
static Stmt *parse_stmt(Parser *parser);
static void parse_block_until(Parser *parser, StmtVec *body, const TokKind *terms, int nterms);

static const Token *peek_n(const Parser *parser, int offset) {
  int pos = parser->pos + offset;
  if (pos < 0) pos = 0;
  if (pos >= parser->tokens->count) pos = parser->tokens->count - 1;
  return &parser->tokens->items[pos];
}

static const Token *peek(const Parser *parser) { return peek_n(parser, 0); }
static int check(const Parser *parser, TokKind kind) { return peek(parser)->kind == kind; }

static const Token *advance(Parser *parser) {
  const Token *token = peek(parser);
  if (token->kind != T_EOF) parser->pos++;
  return token;
}

static int match(Parser *parser, TokKind kind) {
  if (!check(parser, kind)) return 0;
  advance(parser);
  return 1;
}

static void parse_error(const Token *token, const char *message) {
  tatari_fatal(2, token->line, token->col, "%s", message);
}

static const Token *consume(Parser *parser, TokKind kind, const char *message) {
  if (check(parser, kind)) return advance(parser);
  parse_error(peek(parser), message);
  return NULL;
}

static const Token *consume_shikigami_name(Parser *parser) {
  if (check(parser, T_SHIKIGAMI_NAME)) return advance(parser);
  if (check(parser, T_NAME)) {
    const Token *token = peek(parser);
    tatari_fatal(2, token->line, token->col,
                 "式神の名は十二天将・十干・十二支に限る。『%s』は在らず", token->sval);
  }
  parse_error(peek(parser), "式神の名は十二天将・十干・十二支に限る");
  return NULL;
}

static const Token *consume_rite_name(Parser *parser) {
  if (check(parser, T_NAME)) return advance(parser);
  if (check(parser, T_INT)) {
    const Token *token = peek(parser);
    tatari_fatal(2, token->line, token->col,
                 "行法の名に九字を含むべからず。『%.*s』は九なり",
                 (int)token->len, token->lexeme);
  }
  parse_error(peek(parser), "行法の名に語を含むべからず");
  return NULL;
}

static void *vec_grow(void *items, int *cap, size_t elem_size) {
  *cap = *cap == 0 ? 8 : *cap * 2;
  return onmyo_xrealloc(items, elem_size * (size_t)*cap);
}

static void stmt_vec_push(StmtVec *vec, Stmt *stmt) {
  if (vec->count == vec->cap) vec->items = vec_grow(vec->items, &vec->cap, sizeof(Stmt *));
  vec->items[vec->count++] = stmt;
}

static void expr_vec_push(ExprVec *vec, Expr *expr) {
  if (vec->count == vec->cap) vec->items = vec_grow(vec->items, &vec->cap, sizeof(Expr *));
  vec->items[vec->count++] = expr;
}

static void str_vec_push(StrVec *vec, char *str) {
  if (vec->count == vec->cap) vec->items = vec_grow(vec->items, &vec->cap, sizeof(char *));
  vec->items[vec->count++] = str;
}

static void rite_vec_push(RiteVec *vec, Rite *rite) {
  if (vec->count == vec->cap) vec->items = vec_grow(vec->items, &vec->cap, sizeof(Rite *));
  vec->items[vec->count++] = rite;
}

static int is_case(TokKind kind) {
  return kind == T_WA || kind == T_WO || kind == T_NI || kind == T_NITE || kind == T_YORI;
}

static int is_primary_start(TokKind kind) {
  return kind == T_INT || kind == T_STR || kind == T_SHIKIGAMI_NAME || kind == T_KICHI ||
         kind == T_KYO_BAD || kind == T_KYO_VOID;
}

static const CaseFrame *find_frame(TokKind pred) {
  size_t count = sizeof(FRAMES) / sizeof(FRAMES[0]);
  for (size_t i = 0; i < count; i++) if (FRAMES[i].pred == pred) return &FRAMES[i];
  return NULL;
}

static void check_case(const Token *pred, const CaseFrame *frame, TokKind actual, TokKind expected) {
  if (actual == expected) return;
  tatari_fatal(2, pred->line, pred->col,
               "『%s』は〈%s〉〈%s〉の格を取る。〈%s〉は用ゐられぬ",
               frame->name, tok_kind_name(frame->case1), tok_kind_name(frame->case2),
               tok_kind_name(actual));
}

static TokKind consume_case(Parser *parser) {
  if (!is_case(peek(parser)->kind)) parse_error(peek(parser), "格助詞が要る");
  return advance(parser)->kind;
}

static int expr_is_primary(const Expr *expr) { return expr->primary; }

static void consume_uke(Parser *parser, const Expr *expr) {
  const Token *token = peek(parser);
  if (match(parser, T_KORE)) {
    consume(parser, T_WO, "『これ』の後に『を』が要る");
    if (expr_is_primary(expr)) {
      tatari_fatal(2, token->line, token->col, "単項を『これを』にて受くべからず");
    }
    return;
  }
  if (match(parser, T_WO)) {
    if (!expr_is_primary(expr)) {
      tatari_fatal(2, token->line, token->col,
                   "述句を『を』にて受くべからず。『これを』と受けよ");
    }
    return;
  }
  parse_error(token, expr_is_primary(expr) ? "単項の後に『を』が要る" : "述句の後に『これを』が要る");
}

static Expr *parse_primary(Parser *parser) {
  const Token *token = advance(parser);
  switch (token->kind) {
  case T_INT:
    return ast_expr_int_literal(token->ival, token->lexeme, token->len, token->line, token->col);
  case T_STR:
    return ast_expr_str(token->sval, token->line, token->col);
  case T_KICHI:
    return ast_expr_bool(1, token->line, token->col);
  case T_KYO_BAD:
    return ast_expr_bool(0, token->line, token->col);
  case T_KYO_VOID:
    return ast_expr_kyo(token->line, token->col);
  case T_SHIKIGAMI_NAME:
    return ast_expr_var(token->sval, token->line, token->col);
  case T_NAME:
    if (check(parser, T_WO) && peek_n(parser, 1)->kind == T_SHU_SESHIMU) {
      advance(parser);
      advance(parser);
      return ast_expr_call(token->sval, NULL, 0, token->line, token->col);
    }
    if (strcmp(token->sval, "陰") == 0 || strcmp(token->sval, "陽") == 0)
      tatari_fatal(2, token->line, token->col, "陰陽は数の符なり。真偽には吉凶を用ゐよ");
    tatari_fatal(2, token->line, token->col,
                 "式神の名は十二天将・十干・十二支に限る。『%s』は在らず", token->sval);
  default:
    parse_error(token, "式が求められる");
    return NULL;
  }
}

static Expr *make_predicate(const Token *pred, Expr *lhs, Expr *rhs) {
  if (pred->kind == T_HARAUNI_KEGARE_NAKU) {
    Expr *mod = ast_expr_binop(T_KEGASHI, lhs, rhs, pred->line, pred->col);
    return ast_expr_binop(T_ONAJIKU, mod, ast_expr_int(0, pred->line, pred->col), pred->line, pred->col);
  }
  return ast_expr_binop(pred->kind, lhs, rhs, pred->line, pred->col);
}

static Expr *parse_chain(Parser *parser) {
  Expr *lhs = parse_primary(parser);
  if (peek(parser)->kind != parser->expr_stop && is_case(peek(parser)->kind) &&
      is_primary_start(peek_n(parser, 1)->kind)) {
    TokKind case1 = consume_case(parser);
    Expr *rhs = parse_primary(parser);
    TokKind case2 = consume_case(parser);
    const Token *pred = advance(parser);
    const CaseFrame *frame = find_frame(pred->kind);
    if (frame == NULL) parse_error(pred, "演算語が要る");
    check_case(pred, frame, case1, frame->case1);
    check_case(pred, frame, case2, frame->case2);
    lhs = make_predicate(pred, lhs, rhs);
  }
  while (match(parser, T_SARANI)) {
    Expr *rhs = parse_primary(parser);
    TokKind case2 = consume_case(parser);
    const Token *pred = advance(parser);
    const CaseFrame *frame = find_frame(pred->kind);
    if (frame == NULL) parse_error(pred, "演算語が要る");
    check_case(pred, frame, case2, frame->case2);
    lhs = make_predicate(pred, lhs, rhs);
  }
  if (match(parser, T_NIARAZU)) {
    const Token *op = peek_n(parser, -1);
    lhs = ast_expr_not(lhs, op->line, op->col);
  }
  return lhs;
}

static Expr *parse_logic(Parser *parser) {
  Expr *lhs = parse_chain(parser);
  TokKind first = T_EOF;
  while (check(parser, T_KATSU) || check(parser, T_ARUIWA)) {
    const Token *op = advance(parser);
    if (first == T_EOF) first = op->kind;
    else if (op->kind != first) {
      tatari_fatal(2, op->line, op->col, "『且つ』と『或いは』を混ずべからず。式神に憑かせて分かつべし");
    }
    lhs = ast_expr_logic(op->kind, lhs, parse_chain(parser), op->line, op->col);
  }
  return lhs;
}

static int call_suffix(const Parser *parser) {
  if (check(parser, T_WO)) return peek_n(parser, 1)->kind == T_MOTTE;
  return check(parser, T_KORE) && peek_n(parser, 1)->kind == T_WO && peek_n(parser, 2)->kind == T_MOTTE;
}

static Expr *parse_expr(Parser *parser) {
  Expr *first = parse_logic(parser);
  if (!check(parser, T_TO) && !call_suffix(parser)) return first;
  ExprVec args = {0};
  expr_vec_push(&args, first);
  while (match(parser, T_TO)) expr_vec_push(&args, parse_logic(parser));
  consume_uke(parser, args.items[args.count - 1]);
  consume(parser, T_MOTTE, "呼出には『以て』が要る");
  const Token *name = consume_rite_name(parser);
  consume(parser, T_WO, "行法名の後に『を』が要る");
  consume(parser, T_SHU_SESHIMU, "呼出は『修せしむ』で結ぶ");
  return ast_expr_call(name->sval, args.items, args.count, first->line, first->col);
}

static int is_term(const Parser *parser, const TokKind *terms, int nterms) {
  for (int i = 0; i < nterms; i++) if (check(parser, terms[i])) return 1;
  return 0;
}

static Stmt *parse_shikigami(Parser *parser) {
  const Token *start = consume(parser, T_SHIKIGAMI, "式神文が壊れている");
  const Token *name = consume_shikigami_name(parser);
  if (match(parser, T_WOSHITE)) {
    parser->expr_stop = T_YORI;
    Expr *from = parse_expr(parser);
    parser->expr_stop = T_EOF;
    consume(parser, T_YORI, "計数反閇には『より』が要る");
    Expr *to = parse_expr(parser);
    consume(parser, T_NI_ITARU_MADE, "計数反閇には『に至るまで』が要る");
    Expr *step = NULL;
    if (match(parser, T_HO_WO)) {
      step = parse_expr(parser);
      consume(parser, T_TOSHI, "歩は『とし』で定める");
    }
    consume(parser, T_HENBAI, "反閇は『反閇せしむ』で始める");
    StmtVec body = {0};
    TokKind terms[] = {T_KECHIGAN};
    parser->loop_depth++;
    parse_block_until(parser, &body, terms, 1);
    parser->loop_depth--;
    consume(parser, T_KECHIGAN, "反閇は『結願』で閉じる");
    return ast_stmt_for(from, to, step, name->sval, body.items, body.count, start->line, start->col);
  }
  if (match(parser, T_NI)) {
    consume(parser, T_TAKUSEN, "託宣には『託宣を仰ぎ』が要る");
    consume(parser, T_KYUKYU, "文末に『急急如律令』が要る");
    return ast_stmt_takusen(name->sval, start->line, start->col);
  }
  consume(parser, T_WO, "式神名の後に『を』が要る");
  if (match(parser, T_HANACHI)) {
    consume(parser, T_KYUKYU, "文末に『急急如律令』が要る");
    return ast_stmt_release(name->sval, start->line, start->col);
  }
  StmtKind kind = S_SUMMON;
  if (match(parser, T_YOBI)) kind = S_SUMMON;
  else if (match(parser, T_ARATAME)) kind = S_ASSIGN;
  else parse_error(peek(parser), "『喚び』『改め』『放ち』のいずれかが要る");
  Expr *expr = parse_expr(parser);
  consume_uke(parser, expr);
  consume(parser, T_TSUKASESHIMU, "式神文は『憑かせしむ』で結ぶ");
  consume(parser, T_KYUKYU, "文末に『急急如律令』が要る");
  return ast_stmt_bind(kind, name->sval, expr, kind == S_ASSIGN, start->line, start->col);
}

static Stmt *parse_if(Parser *parser) {
  const Token *start = consume(parser, T_URANAUNI, "占が壊れている");
  ExprVec conds = {0};
  Stmt ***blocks = NULL;
  int *counts = NULL;
  int nbranch = 0;
  int cap = 0;
  TokKind terms[] = {T_SHIKARAZUSHITE_URANAUNI, T_SHIKARAZUBA, T_KECHIGAN};
  for (;;) {
    Expr *cond = parse_expr(parser);
    consume(parser, T_BA, "占の条件には『ば』が要る");
    StmtVec body = {0};
    parse_block_until(parser, &body, terms, 3);
    if (nbranch == cap) {
      cap = cap == 0 ? 4 : cap * 2;
      blocks = onmyo_xrealloc(blocks, sizeof(Stmt **) * (size_t)cap);
      counts = onmyo_xrealloc(counts, sizeof(int) * (size_t)cap);
    }
    expr_vec_push(&conds, cond);
    blocks[nbranch] = body.items;
    counts[nbranch++] = body.count;
    if (!match(parser, T_SHIKARAZUSHITE_URANAUNI)) break;
  }
  Stmt **else_block = NULL;
  int nelse = 0;
  if (match(parser, T_SHIKARAZUBA)) {
    StmtVec body = {0};
    TokKind end[] = {T_KECHIGAN};
    parse_block_until(parser, &body, end, 1);
    else_block = body.items;
    nelse = body.count;
  }
  consume(parser, T_KECHIGAN, "占は『結願』で閉じる");
  return ast_stmt_if(conds.items, blocks, counts, nbranch, else_block, nelse, start->line, start->col);
}

static Stmt *parse_expr_started_stmt(Parser *parser) {
  Expr *expr = parse_expr(parser);
  if (match(parser, T_KAGIRI_HENBAI)) {
    StmtVec body = {0};
    TokKind terms[] = {T_KECHIGAN};
    parser->loop_depth++;
    parse_block_until(parser, &body, terms, 1);
    parser->loop_depth--;
    consume(parser, T_KECHIGAN, "条件反閇は『結願』で閉じる");
    return ast_stmt_while(expr, body.items, body.count, expr->line, expr->col);
  }
  if (check(parser, T_WO) || check(parser, T_KORE)) {
    consume_uke(parser, expr);
    if (match(parser, T_TONAE)) {
      consume(parser, T_KYUKYU, "文末に『急急如律令』が要る");
      return ast_stmt_print(expr, expr->line, expr->col);
    }
    if (match(parser, T_KENJI)) {
      consume(parser, T_KYUKYU, "文末に『急急如律令』が要る");
      return ast_stmt_return(expr, expr->line, expr->col);
    }
    parse_error(peek(parser), "受けの後に文の述語が要る");
  }
  if (expr->kind == E_CALL && match(parser, T_KYUKYU)) return ast_stmt_call(expr, expr->line, expr->col);
  parse_error(peek(parser), "文が成り立たぬ");
  return NULL;
}

static Stmt *parse_block_stmt(Parser *parser) {
  const Token *start = consume(parser, T_KEKKAI_WO_HARI, "結界文が壊れている");
  StmtVec body = {0};
  TokKind terms[] = {T_KECHIGAN};
  parse_block_until(parser, &body, terms, 1);
  consume(parser, T_KECHIGAN, "結界は『結願』で閉じる");
  return ast_stmt_block(S_KEKKAI, body.items, body.count, start->line, start->col);
}

static Stmt *parse_flow_stmt(Parser *parser, StmtKind kind) {
  const Token *start = advance(parser);
  if (parser->loop_depth == 0) {
    tatari_fatal(2, start->line, start->col, "反閇の外にて歩を止むべからず");
  }
  consume(parser, T_KYUKYU, "文末に『急急如律令』が要る");
  return ast_stmt_flow(kind, start->line, start->col);
}

static Stmt *parse_stmt(Parser *parser) {
  if (check(parser, T_SHIKIGAMI)) return parse_shikigami(parser);
  if (check(parser, T_URANAUNI)) return parse_if(parser);
  if (check(parser, T_KEKKAI_WO_HARI)) return parse_block_stmt(parser);
  if (check(parser, T_HENBAI_WO_TOME)) return parse_flow_stmt(parser, S_BREAK);
  if (check(parser, T_TSUGI_NO_HO)) return parse_flow_stmt(parser, S_CONTINUE);
  if (match(parser, T_KENJI)) {
    const Token *token = peek_n(parser, -1);
    consume(parser, T_KYUKYU, "文末に『急急如律令』が要る");
    return ast_stmt_return(NULL, token->line, token->col);
  }
  return parse_expr_started_stmt(parser);
}

static void parse_block_until(Parser *parser, StmtVec *body, const TokKind *terms, int nterms) {
  while (!is_term(parser, terms, nterms)) {
    if (check(parser, T_EOF)) parse_error(peek(parser), "段が閉じられぬ");
    stmt_vec_push(body, parse_stmt(parser));
  }
}

static Rite *parse_rite(Parser *parser) {
  const Token *name = consume_rite_name(parser);
  if (strcmp(name->sval, "主") == 0) tatari_fatal(2, name->line, name->col, "入口の行法は『開白』なり");
  consume(parser, T_TO_MOSU_GYOHO, "行法には『と申す行法』が要る");
  StrVec params = {0};
  if (!check(parser, T_SHU_SURUNI)) {
    for (;;) {
      const Token *param = consume_shikigami_name(parser);
      str_vec_push(&params, onmyo_xstrdup(param->sval));
      if (!match(parser, T_TO)) break;
    }
    match(parser, T_WO);
    consume(parser, T_TAMAWARITE, "仮引数は『賜りて』で結ぶ");
  }
  consume(parser, T_SHU_SURUNI, "行法本文には『修するに』が要る");
  StmtVec body = {0};
  TokKind terms[] = {T_KECHIGAN};
  parse_block_until(parser, &body, terms, 1);
  consume(parser, T_KECHIGAN, "行法は『結願』で閉じる");
  return ast_rite(name->sval, params.items, params.count, body.items, body.count, name->line, name->col);
}

static void reject_old_tokens(const TokenArray *tokens) {
  for (int i = 0; i < tokens->count; i++) {
    const Token *token = &tokens->items[i];
    const char *message = NULL;
    switch (token->kind) {
    case T_OLD_KOSHI: message = "『蠱し』は廃されたり。『乗じ』を用ゐよ"; break;
    case T_OLD_KOTONARI: message = "『異なり』は廃されたり。『同じからず』を用ゐよ"; break;
    case T_OLD_MASARU_KA_ONAJIKU: message = "『勝るか同じく』は廃されたり。『劣らず』を用ゐよ"; break;
    case T_OLD_OTORU_KA_ONAJIKU: message = "『劣るか同じく』は廃されたり。『勝らず』を用ゐよ"; break;
    case T_OLD_NO_GYOHO: message = "『之行法』は廃されたり。『と申す行法』を用ゐよ"; break;
    case T_OLD_YORI_FROM:
    case T_OLD_ITARU:
    case T_OLD_AYUMI:
      message = "反閇文は『式神貴人をして X より Y に至るまで 反閇せしむ』と書け";
      break;
    case T_OLD_HAN: message = "負の符は『陰』なり。『反』は廃されたり"; break;
    default: break;
    }
    if (message != NULL) tatari_fatal(2, token->line, token->col, "%s", message);
  }
}

Program *parse_program(const TokenArray *tokens) {
  reject_old_tokens(tokens);
  Parser parser = {tokens, 0, 0, T_EOF};
  RiteVec rites = {0};
  while (!check(&parser, T_SHICCHI_JOJU)) {
    if (check(&parser, T_EOF)) parse_error(peek(&parser), "祭文は『悉地成就』で終わらねばならぬ");
    rite_vec_push(&rites, parse_rite(&parser));
  }
  consume(&parser, T_SHICCHI_JOJU, "祭文は『悉地成就』で終わらねばならぬ");
  consume(&parser, T_EOF, "『悉地成就』の後に字句がある");
  if (rites.count == 0) tatari_fatal(2, 1, 1, "行法が一つもない");
  return ast_program(rites.items, rites.count);
}

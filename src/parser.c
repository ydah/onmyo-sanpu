#include "parser.h"

#include "tatari.h"

#include <stdlib.h>

typedef struct {
  const TokenArray *tokens;
  int pos;
} Parser;

typedef struct {
  Stmt **items;
  int count;
  int cap;
} StmtVec;

typedef struct {
  Expr **items;
  int count;
  int cap;
} ExprVec;

typedef struct {
  char **items;
  int count;
  int cap;
} StrVec;

typedef struct {
  Rite **items;
  int count;
  int cap;
} RiteVec;

static Expr *parse_expr(Parser *parser);
static Stmt *parse_stmt(Parser *parser);
static void parse_block_until(Parser *parser, StmtVec *body, const TokKind *terms, int nterms);

static const Token *peek(const Parser *parser) {
  return &parser->tokens->items[parser->pos];
}

static const Token *next_token(const Parser *parser) {
  if (parser->pos + 1 >= parser->tokens->count) return peek(parser);
  return &parser->tokens->items[parser->pos + 1];
}

static int check(const Parser *parser, TokKind kind) {
  return peek(parser)->kind == kind;
}

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

static int is_particle(TokKind kind) {
  return kind == T_WO || kind == T_NI || kind == T_NITE || kind == T_YORI;
}

static int is_binary_op(TokKind kind) {
  return kind == T_SHOJI || kind == T_KOKUSHI || kind == T_KOSHI || kind == T_HARAI ||
         kind == T_KEGASHI || kind == T_ONAJIKU || kind == T_KOTONARI || kind == T_MASARI ||
         kind == T_OTORI || kind == T_MASARU_KA_ONAJIKU || kind == T_OTORU_KA_ONAJIKU;
}

static int is_primary_start(TokKind kind) {
  return kind == T_INT || kind == T_STR || kind == T_NAME || kind == T_YO ||
         kind == T_IN || kind == T_KYO || kind == T_KEKKAI_OPEN;
}

static int is_term(const Parser *parser, const TokKind *terms, int nterms) {
  TokKind kind = peek(parser)->kind;
  for (int i = 0; i < nterms; i++) {
    if (kind == terms[i]) return 1;
  }
  return 0;
}

static Expr *parse_operand(Parser *parser, int allow_arg_call);

static Expr *parse_primary(Parser *parser) {
  const Token *token = advance(parser);
  switch (token->kind) {
  case T_INT:
    return ast_expr_int_literal(token->ival, token->lexeme, token->len, token->line, token->col);
  case T_STR:
    return ast_expr_str(token->sval, token->line, token->col);
  case T_YO:
    return ast_expr_bool(1, token->line, token->col);
  case T_IN:
    return ast_expr_bool(0, token->line, token->col);
  case T_KYO:
    return ast_expr_kyo(token->line, token->col);
  case T_NAME:
    if (check(parser, T_WO) && next_token(parser)->kind == T_SHU_SESHIMU) {
      advance(parser);
      advance(parser);
      return ast_expr_call(token->sval, NULL, 0, token->line, token->col);
    }
    return ast_expr_var(token->sval, token->line, token->col);
  case T_KEKKAI_OPEN: {
    Expr *inner = parse_expr(parser);
    consume(parser, T_KEKKAI_CLOSE, "結界が閉じられぬ");
    return inner;
  }
  default:
    parse_error(token, "式が求められる");
    return NULL;
  }
}

static Expr *finish_arg_call(Parser *parser, Expr *first_arg, int line, int col) {
  ExprVec args = {0};
  expr_vec_push(&args, first_arg);

  while (match(parser, T_TO)) {
    expr_vec_push(&args, parse_operand(parser, 0));
  }

  consume(parser, T_WO, "呼出の実引数の後に『を』が要る");
  consume(parser, T_MOTTE, "呼出には『以て』が要る");
  const Token *name = consume(parser, T_NAME, "呼び出す行法名が要る");
  consume(parser, T_WO, "行法名の後に『を』が要る");
  consume(parser, T_SHU_SESHIMU, "呼出は『修せしむ』で結ぶ");
  return ast_expr_call(name->sval, args.items, args.count, line, col);
}

static Expr *parse_operand(Parser *parser, int allow_arg_call) {
  Expr *expr = parse_primary(parser);
  if (!allow_arg_call) return expr;

  if (check(parser, T_TO) || (check(parser, T_WO) && next_token(parser)->kind == T_MOTTE)) {
    return finish_arg_call(parser, expr, expr->line, expr->col);
  }
  return expr;
}

static Expr *parse_expr(Parser *parser) {
  Expr *lhs = parse_operand(parser, 1);

  if (match(parser, T_KATSU)) {
    const Token *op = &parser->tokens->items[parser->pos - 1];
    Expr *rhs = parse_operand(parser, 1);
    return ast_expr_logic(op->kind, lhs, rhs, op->line, op->col);
  }
  if (match(parser, T_ARUIWA)) {
    const Token *op = &parser->tokens->items[parser->pos - 1];
    Expr *rhs = parse_operand(parser, 1);
    return ast_expr_logic(op->kind, lhs, rhs, op->line, op->col);
  }
  if (match(parser, T_NIARAZU)) {
    const Token *op = &parser->tokens->items[parser->pos - 1];
    return ast_expr_not(lhs, op->line, op->col);
  }

  if (is_particle(peek(parser)->kind) && is_primary_start(next_token(parser)->kind)) {
    advance(parser);
    Expr *rhs = parse_operand(parser, 1);
    if (is_particle(peek(parser)->kind)) {
      advance(parser);
    }

    if (match(parser, T_HARAUNI_KEGARE_NAKU)) {
      const Token *op = &parser->tokens->items[parser->pos - 1];
      Expr *mod = ast_expr_binop(T_KEGASHI, lhs, rhs, op->line, op->col);
      Expr *zero = ast_expr_int(0, op->line, op->col);
      return ast_expr_binop(T_ONAJIKU, mod, zero, op->line, op->col);
    }

    if (!is_binary_op(peek(parser)->kind)) {
      parse_error(peek(parser), "演算語が要る");
    }
    const Token *op = advance(parser);
    return ast_expr_binop(op->kind, lhs, rhs, op->line, op->col);
  }

  return lhs;
}

static Stmt *parse_bind_or_takusen(Parser *parser) {
  const Token *start = consume(parser, T_SHIKIGAMI, "式神文が壊れている");
  const Token *name = consume(parser, T_NAME, "式神名が要る");

  if (match(parser, T_WO)) {
    consume(parser, T_YOBI, "召喚には『喚び』が要る");
    Expr *expr = parse_expr(parser);
    consume(parser, T_WO, "憑かせる値の後に『を』が要る");
    consume(parser, T_TSUKASESHIMU, "召喚は『憑かせしむ』で結ぶ");
    consume(parser, T_KYUKYU, "文末に『急急如律令』が要る");
    return ast_stmt_bind(S_SUMMON, name->sval, expr, 0, start->line, start->col);
  }

  consume(parser, T_NI, "式神束縛には『に』が要る");
  if (match(parser, T_TAKUSEN)) {
    consume(parser, T_KYUKYU, "文末に『急急如律令』が要る");
    return ast_stmt_takusen(name->sval, start->line, start->col);
  }

  match(parser, T_ARATAME);
  Expr *expr = parse_expr(parser);
  consume(parser, T_WO, "憑かせる値の後に『を』が要る");
  consume(parser, T_TSUKASESHIMU, "束縛は『憑かせしむ』で結ぶ");
  consume(parser, T_KYUKYU, "文末に『急急如律令』が要る");
  return ast_stmt_bind(S_ASSIGN, name->sval, expr, 1, start->line, start->col);
}

static Stmt *parse_if(Parser *parser) {
  const Token *start = consume(parser, T_URANAUNI, "占が壊れている");
  ExprVec conds = {0};
  Stmt ***blocks = NULL;
  int *counts = NULL;
  int nbranch = 0;
  int cap = 0;
  TokKind branch_terms[] = {T_SHIKARAZUSHITE_URANAUNI, T_SHIKARAZUBA, T_KECHIGAN};

  for (;;) {
    Expr *cond = parse_expr(parser);
    consume(parser, T_BA, "占の条件には『ば』が要る");
    StmtVec body = {0};
    parse_block_until(parser, &body, branch_terms, 3);

    if (nbranch == cap) {
      cap = cap == 0 ? 4 : cap * 2;
      blocks = onmyo_xrealloc(blocks, sizeof(Stmt **) * (size_t)cap);
      counts = onmyo_xrealloc(counts, sizeof(int) * (size_t)cap);
    }
    expr_vec_push(&conds, cond);
    blocks[nbranch] = body.items;
    counts[nbranch] = body.count;
    nbranch++;

    if (!match(parser, T_SHIKARAZUSHITE_URANAUNI)) break;
  }

  Stmt **else_block = NULL;
  int nelse = 0;
  if (match(parser, T_SHIKARAZUBA)) {
    StmtVec body = {0};
    TokKind end_terms[] = {T_KECHIGAN};
    parse_block_until(parser, &body, end_terms, 1);
    else_block = body.items;
    nelse = body.count;
  }
  consume(parser, T_KECHIGAN, "占は『結願』で閉じる");
  return ast_stmt_if(conds.items, blocks, counts, nbranch, else_block, nelse, start->line, start->col);
}

static Stmt *parse_for(Parser *parser) {
  const Token *start = consume(parser, T_YORI_FROM, "反閇が壊れている");
  Expr *from = parse_expr(parser);
  consume(parser, T_ITARU, "計数反閇には『至』が要る");
  Expr *to = parse_expr(parser);
  Expr *step = NULL;
  if (match(parser, T_AYUMI)) {
    step = parse_expr(parser);
  }
  consume(parser, T_SHIKIGAMI, "反閇の計数式神が要る");
  const Token *name = consume(parser, T_NAME, "反閇の式神名が要る");
  consume(parser, T_WO, "反閇の式神名の後に『を』が要る");
  consume(parser, T_HENBAI, "反閇は『反閇せしむ』で始める");

  StmtVec body = {0};
  TokKind terms[] = {T_KECHIGAN};
  parse_block_until(parser, &body, terms, 1);
  consume(parser, T_KECHIGAN, "反閇は『結願』で閉じる");
  return ast_stmt_for(from, to, step, name->sval, body.items, body.count, start->line, start->col);
}

static Stmt *parse_expr_started_stmt(Parser *parser) {
  Expr *expr = parse_expr(parser);

  if (match(parser, T_KAGIRI_HENBAI)) {
    StmtVec body = {0};
    TokKind terms[] = {T_KECHIGAN};
    parse_block_until(parser, &body, terms, 1);
    consume(parser, T_KECHIGAN, "条件反閇は『結願』で閉じる");
    return ast_stmt_while(expr, body.items, body.count, expr->line, expr->col);
  }

  if (match(parser, T_WO)) {
    if (match(parser, T_TONAE)) {
      consume(parser, T_KYUKYU, "文末に『急急如律令』が要る");
      return ast_stmt_print(expr, expr->line, expr->col);
    }
    if (match(parser, T_KENJI)) {
      consume(parser, T_KYUKYU, "文末に『急急如律令』が要る");
      return ast_stmt_return(expr, expr->line, expr->col);
    }
    parse_error(peek(parser), "『を』の後に文の述語が要る");
  }

  if (expr->kind == E_CALL && match(parser, T_KYUKYU)) {
    return ast_stmt_call(expr, expr->line, expr->col);
  }

  parse_error(peek(parser), "文が成り立たぬ");
  return NULL;
}

static Stmt *parse_stmt(Parser *parser) {
  if (check(parser, T_SHIKIGAMI)) return parse_bind_or_takusen(parser);
  if (check(parser, T_URANAUNI)) return parse_if(parser);
  if (check(parser, T_YORI_FROM)) return parse_for(parser);
  if (match(parser, T_KENJI)) {
    const Token *token = &parser->tokens->items[parser->pos - 1];
    consume(parser, T_KYUKYU, "文末に『急急如律令』が要る");
    return ast_stmt_return(NULL, token->line, token->col);
  }
  return parse_expr_started_stmt(parser);
}

static void parse_block_until(Parser *parser, StmtVec *body, const TokKind *terms, int nterms) {
  while (!is_term(parser, terms, nterms)) {
    if (check(parser, T_EOF)) {
      parse_error(peek(parser), "段が閉じられぬ");
    }
    stmt_vec_push(body, parse_stmt(parser));
  }
}

static Rite *parse_rite(Parser *parser) {
  const Token *name = consume(parser, T_NAME, "行法名が要る");
  consume(parser, T_NO_GYOHO, "行法には『之行法』が要る");

  StrVec params = {0};
  if (!check(parser, T_SHU_SURUNI)) {
    for (;;) {
      const Token *param = consume(parser, T_NAME, "仮引数名が要る");
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

Program *parse_program(const TokenArray *tokens) {
  Parser parser;
  parser.tokens = tokens;
  parser.pos = 0;

  RiteVec rites = {0};
  while (!check(&parser, T_SHICCHI_JOJU)) {
    if (check(&parser, T_EOF)) {
      parse_error(peek(&parser), "祭文は『悉地成就』で終わらねばならぬ");
    }
    rite_vec_push(&rites, parse_rite(&parser));
  }
  consume(&parser, T_SHICCHI_JOJU, "祭文は『悉地成就』で終わらねばならぬ");
  consume(&parser, T_EOF, "『悉地成就』の後に字句がある");
  if (rites.count == 0) {
    tatari_fatal(2, 1, 1, "行法が一つもない");
  }
  return ast_program(rites.items, rites.count);
}

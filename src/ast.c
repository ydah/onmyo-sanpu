#include "ast.h"

#include "tatari.h"

#include <stdlib.h>

static Expr *new_expr(ExprKind kind, int line, int col) {
  Expr *expr = onmyo_xcalloc(1, sizeof(Expr));
  expr->kind = kind;
  expr->primary = kind == E_INT || kind == E_STR || kind == E_BOOL || kind == E_KYO || kind == E_VAR;
  expr->line = line;
  expr->col = col;
  return expr;
}

static Stmt *new_stmt(StmtKind kind, int line, int col) {
  Stmt *stmt = onmyo_xcalloc(1, sizeof(Stmt));
  stmt->kind = kind;
  stmt->line = line;
  stmt->col = col;
  return stmt;
}

Expr *ast_expr_int(long long value, int line, int col) {
  Expr *expr = new_expr(E_INT, line, col);
  expr->as.int_value = value;
  return expr;
}

Expr *ast_expr_int_literal(long long value, const char *lexeme, size_t len, int line, int col) {
  Expr *expr = ast_expr_int(value, line, col);
  expr->int_lexeme = onmyo_xstrndup(lexeme, len);
  return expr;
}

Expr *ast_expr_str(const char *value, int line, int col) {
  Expr *expr = new_expr(E_STR, line, col);
  expr->as.str_value = onmyo_xstrdup(value);
  return expr;
}

Expr *ast_expr_bool(int value, int line, int col) {
  Expr *expr = new_expr(E_BOOL, line, col);
  expr->as.bool_value = value ? 1 : 0;
  return expr;
}

Expr *ast_expr_kyo(int line, int col) {
  return new_expr(E_KYO, line, col);
}

Expr *ast_expr_var(const char *name, int line, int col) {
  Expr *expr = new_expr(E_VAR, line, col);
  expr->as.var_name = onmyo_xstrdup(name);
  return expr;
}

Expr *ast_expr_binop(TokKind op, Expr *lhs, Expr *rhs, int line, int col) {
  Expr *expr = new_expr(E_BINOP, line, col);
  expr->as.binop.op = op;
  expr->as.binop.lhs = lhs;
  expr->as.binop.rhs = rhs;
  return expr;
}

Expr *ast_expr_logic(TokKind op, Expr *lhs, Expr *rhs, int line, int col) {
  Expr *expr = new_expr(E_LOGIC, line, col);
  expr->as.logic.op = op;
  expr->as.logic.lhs = lhs;
  expr->as.logic.rhs = rhs;
  return expr;
}

Expr *ast_expr_not(Expr *inner, int line, int col) {
  Expr *expr = new_expr(E_NOT, line, col);
  expr->as.not_expr.expr = inner;
  return expr;
}

Expr *ast_expr_call(const char *name, Expr **args, int nargs, int line, int col) {
  Expr *expr = new_expr(E_CALL, line, col);
  expr->as.call.name = onmyo_xstrdup(name);
  expr->as.call.args = args;
  expr->as.call.nargs = nargs;
  return expr;
}

Stmt *ast_stmt_bind(StmtKind kind, const char *name, Expr *expr, int require_existing, int line, int col) {
  Stmt *stmt = new_stmt(kind, line, col);
  stmt->as.bind.name = onmyo_xstrdup(name);
  stmt->as.bind.expr = expr;
  stmt->as.bind.require_existing = require_existing;
  return stmt;
}

Stmt *ast_stmt_takusen(const char *name, int line, int col) {
  Stmt *stmt = new_stmt(S_TAKUSEN, line, col);
  stmt->as.takusen.name = onmyo_xstrdup(name);
  return stmt;
}

Stmt *ast_stmt_release(const char *name, int line, int col) {
  Stmt *stmt = new_stmt(S_RELEASE, line, col);
  stmt->as.release.name = onmyo_xstrdup(name);
  return stmt;
}

Stmt *ast_stmt_print(Expr *expr, int line, int col) {
  Stmt *stmt = new_stmt(S_PRINT, line, col);
  stmt->as.print_stmt.expr = expr;
  return stmt;
}

Stmt *ast_stmt_if(Expr **conds, Stmt ***blocks, int *block_counts, int nbranch,
                  Stmt **else_block, int nelse, int line, int col) {
  Stmt *stmt = new_stmt(S_IF, line, col);
  stmt->as.if_stmt.conds = conds;
  stmt->as.if_stmt.blocks = blocks;
  stmt->as.if_stmt.block_counts = block_counts;
  stmt->as.if_stmt.nbranch = nbranch;
  stmt->as.if_stmt.else_block = else_block;
  stmt->as.if_stmt.nelse = nelse;
  return stmt;
}

Stmt *ast_stmt_for(Expr *from, Expr *to, Expr *step, const char *var,
                   Stmt **body, int nbody, int line, int col) {
  Stmt *stmt = new_stmt(S_FOR, line, col);
  stmt->as.for_stmt.from = from;
  stmt->as.for_stmt.to = to;
  stmt->as.for_stmt.step = step;
  stmt->as.for_stmt.var = onmyo_xstrdup(var);
  stmt->as.for_stmt.body = body;
  stmt->as.for_stmt.nbody = nbody;
  return stmt;
}

Stmt *ast_stmt_while(Expr *cond, Stmt **body, int nbody, int line, int col) {
  Stmt *stmt = new_stmt(S_WHILE, line, col);
  stmt->as.while_stmt.cond = cond;
  stmt->as.while_stmt.body = body;
  stmt->as.while_stmt.nbody = nbody;
  return stmt;
}

Stmt *ast_stmt_block(StmtKind kind, Stmt **body, int nbody, int line, int col) {
  Stmt *stmt = new_stmt(kind, line, col);
  stmt->as.block_stmt.body = body;
  stmt->as.block_stmt.nbody = nbody;
  return stmt;
}

Stmt *ast_stmt_flow(StmtKind kind, int line, int col) {
  return new_stmt(kind, line, col);
}

Stmt *ast_stmt_call(Expr *call, int line, int col) {
  Stmt *stmt = new_stmt(S_CALLSTMT, line, col);
  stmt->as.call_stmt.call = call;
  return stmt;
}

Stmt *ast_stmt_return(Expr *expr, int line, int col) {
  Stmt *stmt = new_stmt(S_RETURN, line, col);
  stmt->as.return_stmt.expr = expr;
  return stmt;
}

Rite *ast_rite(const char *name, char **params, int nparams, Stmt **body, int nbody, int line, int col) {
  Rite *rite = onmyo_xcalloc(1, sizeof(Rite));
  rite->name = onmyo_xstrdup(name);
  rite->params = params;
  rite->nparams = nparams;
  rite->body = body;
  rite->nbody = nbody;
  rite->line = line;
  rite->col = col;
  return rite;
}

Program *ast_program(Rite **rites, int nrites) {
  Program *program = onmyo_xcalloc(1, sizeof(Program));
  program->rites = rites;
  program->nrites = nrites;
  return program;
}

static void free_expr(Expr *expr) {
  if (expr == NULL) return;
  switch (expr->kind) {
  case E_INT:
    free(expr->int_lexeme);
    break;
  case E_STR:
    free(expr->as.str_value);
    break;
  case E_VAR:
    free(expr->as.var_name);
    break;
  case E_BINOP:
    free_expr(expr->as.binop.lhs);
    free_expr(expr->as.binop.rhs);
    break;
  case E_LOGIC:
    free_expr(expr->as.logic.lhs);
    free_expr(expr->as.logic.rhs);
    break;
  case E_NOT:
    free_expr(expr->as.not_expr.expr);
    break;
  case E_CALL:
    free(expr->as.call.name);
    for (int i = 0; i < expr->as.call.nargs; i++) {
      free_expr(expr->as.call.args[i]);
    }
    free(expr->as.call.args);
    break;
  case E_BOOL:
  case E_KYO:
    break;
  }
  free(expr);
}

static void free_stmt(Stmt *stmt);

static void free_block(Stmt **body, int nbody) {
  for (int i = 0; i < nbody; i++) {
    free_stmt(body[i]);
  }
  free(body);
}

static void free_stmt(Stmt *stmt) {
  if (stmt == NULL) return;
  switch (stmt->kind) {
  case S_SUMMON:
  case S_ASSIGN:
    free(stmt->as.bind.name);
    free_expr(stmt->as.bind.expr);
    break;
  case S_TAKUSEN:
    free(stmt->as.takusen.name);
    break;
  case S_RELEASE:
    free(stmt->as.release.name);
    break;
  case S_PRINT:
    free_expr(stmt->as.print_stmt.expr);
    break;
  case S_IF:
    for (int i = 0; i < stmt->as.if_stmt.nbranch; i++) {
      free_expr(stmt->as.if_stmt.conds[i]);
      free_block(stmt->as.if_stmt.blocks[i], stmt->as.if_stmt.block_counts[i]);
    }
    free(stmt->as.if_stmt.conds);
    free(stmt->as.if_stmt.blocks);
    free(stmt->as.if_stmt.block_counts);
    free_block(stmt->as.if_stmt.else_block, stmt->as.if_stmt.nelse);
    break;
  case S_FOR:
    free_expr(stmt->as.for_stmt.from);
    free_expr(stmt->as.for_stmt.to);
    free_expr(stmt->as.for_stmt.step);
    free(stmt->as.for_stmt.var);
    free_block(stmt->as.for_stmt.body, stmt->as.for_stmt.nbody);
    break;
  case S_WHILE:
    free_expr(stmt->as.while_stmt.cond);
    free_block(stmt->as.while_stmt.body, stmt->as.while_stmt.nbody);
    break;
  case S_KEKKAI:
    free_block(stmt->as.block_stmt.body, stmt->as.block_stmt.nbody);
    break;
  case S_BREAK:
  case S_CONTINUE:
    break;
  case S_CALLSTMT:
    free_expr(stmt->as.call_stmt.call);
    break;
  case S_RETURN:
    free_expr(stmt->as.return_stmt.expr);
    break;
  }
  free(stmt);
}

void ast_free_program(Program *program) {
  if (program == NULL) return;
  for (int i = 0; i < program->nrites; i++) {
    Rite *rite = program->rites[i];
    free(rite->name);
    for (int p = 0; p < rite->nparams; p++) {
      free(rite->params[p]);
    }
    free(rite->params);
    free_block(rite->body, rite->nbody);
    free(rite);
  }
  free(program->rites);
  free(program);
}

static void indent(FILE *out, int depth) {
  for (int i = 0; i < depth; i++) {
    fputs("  ", out);
  }
}

static void print_expr(FILE *out, const Expr *expr, int depth);

static void print_stmt(FILE *out, const Stmt *stmt, int depth) {
  indent(out, depth);
  switch (stmt->kind) {
  case S_SUMMON:
  case S_ASSIGN:
    fprintf(out, "%s %s\n", stmt->kind == S_SUMMON ? "召喚" : "束縛", stmt->as.bind.name);
    print_expr(out, stmt->as.bind.expr, depth + 1);
    break;
  case S_TAKUSEN:
    fprintf(out, "託宣 %s\n", stmt->as.takusen.name);
    break;
  case S_RELEASE:
    fprintf(out, "放免 %s\n", stmt->as.release.name);
    break;
  case S_PRINT:
    fputs("唱\n", out);
    print_expr(out, stmt->as.print_stmt.expr, depth + 1);
    break;
  case S_IF:
    fputs("占\n", out);
    for (int i = 0; i < stmt->as.if_stmt.nbranch; i++) {
      indent(out, depth + 1);
      fputs("条件\n", out);
      print_expr(out, stmt->as.if_stmt.conds[i], depth + 2);
      for (int j = 0; j < stmt->as.if_stmt.block_counts[i]; j++) {
        print_stmt(out, stmt->as.if_stmt.blocks[i][j], depth + 2);
      }
    }
    if (stmt->as.if_stmt.nelse > 0) {
      indent(out, depth + 1);
      fputs("然らずば\n", out);
      for (int j = 0; j < stmt->as.if_stmt.nelse; j++) {
        print_stmt(out, stmt->as.if_stmt.else_block[j], depth + 2);
      }
    }
    break;
  case S_FOR:
    fprintf(out, "計数反閇 %s\n", stmt->as.for_stmt.var);
    print_expr(out, stmt->as.for_stmt.from, depth + 1);
    print_expr(out, stmt->as.for_stmt.to, depth + 1);
    if (stmt->as.for_stmt.step != NULL) print_expr(out, stmt->as.for_stmt.step, depth + 1);
    for (int i = 0; i < stmt->as.for_stmt.nbody; i++) print_stmt(out, stmt->as.for_stmt.body[i], depth + 1);
    break;
  case S_WHILE:
    fputs("条件反閇\n", out);
    print_expr(out, stmt->as.while_stmt.cond, depth + 1);
    for (int i = 0; i < stmt->as.while_stmt.nbody; i++) print_stmt(out, stmt->as.while_stmt.body[i], depth + 1);
    break;
  case S_KEKKAI:
    fputs("結界\n", out);
    for (int i = 0; i < stmt->as.block_stmt.nbody; i++) print_stmt(out, stmt->as.block_stmt.body[i], depth + 1);
    break;
  case S_BREAK:
    fputs("歩止\n", out);
    break;
  case S_CONTINUE:
    fputs("歩次\n", out);
    break;
  case S_CALLSTMT:
    fputs("呼出文\n", out);
    print_expr(out, stmt->as.call_stmt.call, depth + 1);
    break;
  case S_RETURN:
    fputs("献上\n", out);
    if (stmt->as.return_stmt.expr != NULL) print_expr(out, stmt->as.return_stmt.expr, depth + 1);
    break;
  }
}

static void print_expr(FILE *out, const Expr *expr, int depth) {
  indent(out, depth);
  switch (expr->kind) {
  case E_INT:
    if (expr->int_lexeme != NULL) {
      fprintf(out, "整数 %lld [%s]\n", expr->as.int_value, expr->int_lexeme);
    } else {
      fprintf(out, "整数 %lld\n", expr->as.int_value);
    }
    break;
  case E_STR:
    fprintf(out, "真言 \"%s\"\n", expr->as.str_value);
    break;
  case E_BOOL:
    fprintf(out, "卦 %s\n", expr->as.bool_value ? "吉" : "凶");
    break;
  case E_KYO:
    fputs("虚\n", out);
    break;
  case E_VAR:
    fprintf(out, "式神 %s\n", expr->as.var_name);
    break;
  case E_BINOP:
    fprintf(out, "二項 %s\n", tok_kind_name(expr->as.binop.op));
    print_expr(out, expr->as.binop.lhs, depth + 1);
    print_expr(out, expr->as.binop.rhs, depth + 1);
    break;
  case E_LOGIC:
    fprintf(out, "論理 %s\n", tok_kind_name(expr->as.logic.op));
    print_expr(out, expr->as.logic.lhs, depth + 1);
    print_expr(out, expr->as.logic.rhs, depth + 1);
    break;
  case E_NOT:
    fputs("否定\n", out);
    print_expr(out, expr->as.not_expr.expr, depth + 1);
    break;
  case E_CALL:
    fprintf(out, "呼出 %s\n", expr->as.call.name);
    for (int i = 0; i < expr->as.call.nargs; i++) {
      print_expr(out, expr->as.call.args[i], depth + 1);
    }
    break;
  }
}

void ast_print_program(FILE *out, const Program *program) {
  fputs("祭文\n", out);
  for (int i = 0; i < program->nrites; i++) {
    const Rite *rite = program->rites[i];
    fprintf(out, "  行法 %s", rite->name);
    if (rite->nparams > 0) {
      fputs("(", out);
      for (int p = 0; p < rite->nparams; p++) {
        fprintf(out, "%s%s", p == 0 ? "" : ", ", rite->params[p]);
      }
      fputs(")", out);
    }
    fputc('\n', out);
    for (int j = 0; j < rite->nbody; j++) {
      print_stmt(out, rite->body[j], 2);
    }
  }
}

#ifndef ONMYO_AST_H
#define ONMYO_AST_H

#include "token.h"

#include <stdio.h>

typedef struct Expr Expr;
typedef struct Stmt Stmt;
typedef struct Rite Rite;

typedef enum {
  S_SUMMON,
  S_ASSIGN,
  S_RELEASE,
  S_TAKUSEN,
  S_PRINT,
  S_IF,
  S_FOR,
  S_WHILE,
  S_KEKKAI,
  S_BREAK,
  S_CONTINUE,
  S_CALLSTMT,
  S_RETURN
} StmtKind;

typedef enum {
  E_INT,
  E_STR,
  E_BOOL,
  E_KYO,
  E_VAR,
  E_BINOP,
  E_LOGIC,
  E_NOT,
  E_CALL
} ExprKind;

struct Expr {
  ExprKind kind;
  int primary;
  int line;
  int col;
  char *int_lexeme;
  union {
    long long int_value;
    char *str_value;
    int bool_value;
    char *var_name;
    struct {
      TokKind op;
      Expr *lhs;
      Expr *rhs;
    } binop;
    struct {
      TokKind op;
      Expr *lhs;
      Expr *rhs;
    } logic;
    struct {
      Expr *expr;
    } not_expr;
    struct {
      char *name;
      Expr **args;
      int nargs;
    } call;
  } as;
};

struct Stmt {
  StmtKind kind;
  int line;
  int col;
  union {
    struct {
      char *name;
      Expr *expr;
      int require_existing;
    } bind;
    struct {
      char *name;
    } takusen;
    struct {
      char *name;
    } release;
    struct {
      Expr *expr;
    } print_stmt;
    struct {
      Expr **conds;
      Stmt ***blocks;
      int *block_counts;
      int nbranch;
      Stmt **else_block;
      int nelse;
    } if_stmt;
    struct {
      Expr *from;
      Expr *to;
      Expr *step;
      char *var;
      Stmt **body;
      int nbody;
    } for_stmt;
    struct {
      Expr *cond;
      Stmt **body;
      int nbody;
    } while_stmt;
    struct {
      Stmt **body;
      int nbody;
    } block_stmt;
    struct {
      Expr *call;
    } call_stmt;
    struct {
      Expr *expr;
    } return_stmt;
  } as;
};

struct Rite {
  char *name;
  char **params;
  int nparams;
  Stmt **body;
  int nbody;
  int line;
  int col;
};

typedef struct {
  Rite **rites;
  int nrites;
} Program;

Expr *ast_expr_int(long long value, int line, int col);
Expr *ast_expr_int_literal(long long value, const char *lexeme, size_t len, int line, int col);
Expr *ast_expr_str(const char *value, int line, int col);
Expr *ast_expr_bool(int value, int line, int col);
Expr *ast_expr_kyo(int line, int col);
Expr *ast_expr_var(const char *name, int line, int col);
Expr *ast_expr_binop(TokKind op, Expr *lhs, Expr *rhs, int line, int col);
Expr *ast_expr_logic(TokKind op, Expr *lhs, Expr *rhs, int line, int col);
Expr *ast_expr_not(Expr *expr, int line, int col);
Expr *ast_expr_call(const char *name, Expr **args, int nargs, int line, int col);

Stmt *ast_stmt_bind(StmtKind kind, const char *name, Expr *expr, int require_existing, int line, int col);
Stmt *ast_stmt_takusen(const char *name, int line, int col);
Stmt *ast_stmt_release(const char *name, int line, int col);
Stmt *ast_stmt_print(Expr *expr, int line, int col);
Stmt *ast_stmt_if(Expr **conds, Stmt ***blocks, int *block_counts, int nbranch,
                  Stmt **else_block, int nelse, int line, int col);
Stmt *ast_stmt_for(Expr *from, Expr *to, Expr *step, const char *var,
                   Stmt **body, int nbody, int line, int col);
Stmt *ast_stmt_while(Expr *cond, Stmt **body, int nbody, int line, int col);
Stmt *ast_stmt_block(StmtKind kind, Stmt **body, int nbody, int line, int col);
Stmt *ast_stmt_flow(StmtKind kind, int line, int col);
Stmt *ast_stmt_call(Expr *call, int line, int col);
Stmt *ast_stmt_return(Expr *expr, int line, int col);
Rite *ast_rite(const char *name, char **params, int nparams, Stmt **body, int nbody, int line, int col);
Program *ast_program(Rite **rites, int nrites);

void ast_free_program(Program *program);
void ast_print_program(FILE *out, const Program *program);

#endif

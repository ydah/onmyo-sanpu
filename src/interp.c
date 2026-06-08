#include "interp.h"

#include "env.h"
#include "tatari.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  const Program *program;
} Interp;

static Value eval_expr(Interp *interp, Env *env, const Expr *expr);
static ExecResult exec_block(Interp *interp, Env *env, Stmt **body, int nbody);

static ExecResult normal_result(void) {
  ExecResult result;
  result.flow = FLOW_NORMAL;
  result.ret = val_kyo();
  return result;
}

static ExecResult return_result(Value value) {
  ExecResult result;
  result.flow = FLOW_RETURN;
  result.ret = value;
  return result;
}

static const Rite *find_rite(const Program *program, const char *name) {
  for (int i = 0; i < program->nrites; i++) {
    if (strcmp(program->rites[i]->name, name) == 0) return program->rites[i];
  }
  return NULL;
}

static const Rite *entry_rite(const Program *program) {
  const Rite *main_rite = find_rite(program, "主");
  if (main_rite != NULL) return main_rite;
  if (program->nrites == 1) return program->rites[0];
  return program->rites[0];
}

static Value eval_call(Interp *interp, Env *caller_env, const Expr *expr) {
  const Rite *rite = find_rite(interp->program, expr->as.call.name);
  if (rite == NULL) {
    tatari_fatal(1, expr->line, expr->col, "行法『%s』は定められておらぬ", expr->as.call.name);
  }
  if (rite->nparams != expr->as.call.nargs) {
    tatari_fatal(1, expr->line, expr->col, "行法『%s』の賜り物の数が合わぬ", expr->as.call.name);
  }

  Env *env = env_new();
  for (int i = 0; i < expr->as.call.nargs; i++) {
    Value arg = eval_expr(interp, caller_env, expr->as.call.args[i]);
    env_bind(env, rite->params[i], &arg);
    val_free(&arg);
  }

  ExecResult result = exec_block(interp, env, rite->body, rite->nbody);
  env_free(env);
  if (result.flow == FLOW_RETURN) {
    return result.ret;
  }
  return val_kyo();
}

static Value eval_expr(Interp *interp, Env *env, const Expr *expr) {
  switch (expr->kind) {
  case E_INT:
    return val_int(expr->as.int_value);
  case E_STR:
    return val_str(expr->as.str_value);
  case E_BOOL:
    return val_bool(expr->as.bool_value);
  case E_KYO:
    return val_kyo();
  case E_VAR:
    return env_get(env, expr->as.var_name, expr->line, expr->col);
  case E_BINOP: {
    Value lhs = eval_expr(interp, env, expr->as.binop.lhs);
    Value rhs = eval_expr(interp, env, expr->as.binop.rhs);
    Value out = val_binary(expr->as.binop.op, &lhs, &rhs, expr->line, expr->col);
    val_free(&lhs);
    val_free(&rhs);
    return out;
  }
  case E_LOGIC: {
    Value lhs = eval_expr(interp, env, expr->as.logic.lhs);
    Value rhs = eval_expr(interp, env, expr->as.logic.rhs);
    Value out = val_logic(expr->as.logic.op, &lhs, &rhs, expr->line, expr->col);
    val_free(&lhs);
    val_free(&rhs);
    return out;
  }
  case E_NOT: {
    Value inner = eval_expr(interp, env, expr->as.not_expr.expr);
    Value out = val_not(&inner, expr->line, expr->col);
    val_free(&inner);
    return out;
  }
  case E_CALL:
    return eval_call(interp, env, expr);
  }
  return val_kyo();
}

static ExecResult exec_stmt(Interp *interp, Env *env, const Stmt *stmt) {
  switch (stmt->kind) {
  case S_SUMMON:
  case S_ASSIGN: {
    Value value = eval_expr(interp, env, stmt->as.bind.expr);
    if (stmt->as.bind.require_existing) {
      if (!env_assign(env, stmt->as.bind.name, &value)) {
        val_free(&value);
        tatari_fatal(1, stmt->line, stmt->col, "式神『%s』は改められぬ", stmt->as.bind.name);
      }
    } else {
      env_bind(env, stmt->as.bind.name, &value);
    }
    val_free(&value);
    return normal_result();
  }
  case S_TAKUSEN: {
    long long input = 0;
    if (scanf("%lld", &input) != 1) {
      tatari_fatal(1, stmt->line, stmt->col, "託宣を得られぬ");
    }
    Value value = val_int(input);
    env_bind(env, stmt->as.takusen.name, &value);
    val_free(&value);
    return normal_result();
  }
  case S_PRINT: {
    Value value = eval_expr(interp, env, stmt->as.print_stmt.expr);
    val_print(stdout, &value, stmt->line, stmt->col);
    val_free(&value);
    return normal_result();
  }
  case S_IF:
    for (int i = 0; i < stmt->as.if_stmt.nbranch; i++) {
      Value cond = eval_expr(interp, env, stmt->as.if_stmt.conds[i]);
      int truth = val_as_bool(&cond, stmt->as.if_stmt.conds[i]->line, stmt->as.if_stmt.conds[i]->col);
      val_free(&cond);
      if (truth) {
        return exec_block(interp, env, stmt->as.if_stmt.blocks[i], stmt->as.if_stmt.block_counts[i]);
      }
    }
    if (stmt->as.if_stmt.nelse > 0) {
      return exec_block(interp, env, stmt->as.if_stmt.else_block, stmt->as.if_stmt.nelse);
    }
    return normal_result();
  case S_FOR: {
    Value from_value = eval_expr(interp, env, stmt->as.for_stmt.from);
    Value to_value = eval_expr(interp, env, stmt->as.for_stmt.to);
    Value step_value = stmt->as.for_stmt.step != NULL
                           ? eval_expr(interp, env, stmt->as.for_stmt.step)
                           : val_int(1);
    long long current = val_as_int(&from_value, stmt->as.for_stmt.from->line, stmt->as.for_stmt.from->col);
    long long end = val_as_int(&to_value, stmt->as.for_stmt.to->line, stmt->as.for_stmt.to->col);
    long long step = val_as_int(&step_value, stmt->as.for_stmt.step != NULL ? stmt->as.for_stmt.step->line : stmt->line,
                                stmt->as.for_stmt.step != NULL ? stmt->as.for_stmt.step->col : stmt->col);
    val_free(&from_value);
    val_free(&to_value);
    val_free(&step_value);
    if (step == 0) {
      tatari_fatal(1, stmt->line, stmt->col, "歩みは零ならず");
    }

    Value initial = val_int(current);
    env_bind(env, stmt->as.for_stmt.var, &initial);
    val_free(&initial);

    while ((step > 0 && current <= end) || (step < 0 && current >= end)) {
      Value counter = val_int(current);
      env_bind(env, stmt->as.for_stmt.var, &counter);
      val_free(&counter);

      ExecResult result = exec_block(interp, env, stmt->as.for_stmt.body, stmt->as.for_stmt.nbody);
      if (result.flow == FLOW_RETURN) return result;

      current += step;
      Value updated = val_int(current);
      env_bind(env, stmt->as.for_stmt.var, &updated);
      val_free(&updated);
    }
    return normal_result();
  }
  case S_WHILE:
    for (;;) {
      Value cond = eval_expr(interp, env, stmt->as.while_stmt.cond);
      int truth = val_as_bool(&cond, stmt->as.while_stmt.cond->line, stmt->as.while_stmt.cond->col);
      val_free(&cond);
      if (!truth) break;
      ExecResult result = exec_block(interp, env, stmt->as.while_stmt.body, stmt->as.while_stmt.nbody);
      if (result.flow == FLOW_RETURN) return result;
    }
    return normal_result();
  case S_CALLSTMT: {
    Value value = eval_expr(interp, env, stmt->as.call_stmt.call);
    val_free(&value);
    return normal_result();
  }
  case S_RETURN:
    if (stmt->as.return_stmt.expr == NULL) return return_result(val_kyo());
    return return_result(eval_expr(interp, env, stmt->as.return_stmt.expr));
  }
  return normal_result();
}

static ExecResult exec_block(Interp *interp, Env *env, Stmt **body, int nbody) {
  for (int i = 0; i < nbody; i++) {
    ExecResult result = exec_stmt(interp, env, body[i]);
    if (result.flow == FLOW_RETURN) return result;
    val_free(&result.ret);
  }
  return normal_result();
}

void interp_execute(const Program *program) {
  Interp interp;
  interp.program = program;

  const Rite *entry = entry_rite(program);
  if (entry->nparams != 0) {
    tatari_fatal(1, entry->line, entry->col, "入口行法に賜り物がある");
  }

  Env *env = env_new();
  ExecResult result = exec_block(&interp, env, entry->body, entry->nbody);
  val_free(&result.ret);
  env_free(env);
}

#include "interp.h"

#include "env.h"
#include "tatari.h"

#include <limits.h>
#include <stdio.h>
#include <string.h>

typedef struct { const Program *program; } Interp;

static Value eval_expr(Interp *interp, Env *env, const Expr *expr);
static ExecResult exec_block(Interp *interp, Env *env, Stmt **body, int nbody);

static ExecResult flow_result(Flow flow) {
  ExecResult result = {flow, val_kyo()};
  return result;
}

static ExecResult return_result(Value value) {
  ExecResult result = {FLOW_RETURN, value};
  return result;
}

static const Rite *find_rite(const Program *program, const char *name) {
  for (int i = 0; i < program->nrites; i++) {
    if (strcmp(program->rites[i]->name, name) == 0) return program->rites[i];
  }
  return NULL;
}

static const Rite *entry_rite(const Program *program) {
  const Rite *entry = NULL;
  for (int i = 0; i < program->nrites; i++) {
    for (int j = i + 1; j < program->nrites; j++) {
      if (strcmp(program->rites[i]->name, program->rites[j]->name) == 0) {
        tatari_fatal(1, program->rites[j]->line, program->rites[j]->col,
                     "行法『%s』は一つに限る", program->rites[j]->name);
      }
    }
    if (strcmp(program->rites[i]->name, "開白") == 0) entry = program->rites[i];
  }
  if (entry == NULL) tatari_fatal(1, 0, 0, "開白の行法無し");
  return entry;
}

static Value eval_call(Interp *interp, Env *caller_env, const Expr *expr) {
  const Rite *rite = find_rite(interp->program, expr->as.call.name);
  if (rite == NULL) {
    tatari_fatal(1, expr->line, expr->col, "行法『%s』は定められておらぬ", expr->as.call.name);
  }
  if (rite->nparams != expr->as.call.nargs) {
    tatari_fatal(1, expr->line, expr->col, "行法『%s』の賜り物の数が合わぬ", expr->as.call.name);
  }

  Env *env = env_new(NULL);
  for (int i = 0; i < expr->as.call.nargs; i++) {
    Value arg = eval_expr(interp, caller_env, expr->as.call.args[i]);
    if (!env_declare(env, rite->params[i], &arg)) {
      val_free(&arg);
      env_free(env);
      tatari_fatal(1, rite->line, rite->col, "賜り物『%s』は重なれり", rite->params[i]);
    }
    val_free(&arg);
  }

  ExecResult result = exec_block(interp, env, rite->body, rite->nbody);
  env_free(env);
  if (result.flow == FLOW_RETURN) return result.ret;
  if (result.flow != FLOW_NORMAL) {
    val_free(&result.ret);
    tatari_fatal(1, expr->line, expr->col, "反閇の外にて歩を止むべからず");
  }
  val_free(&result.ret);
  return val_kyo();
}

static Value eval_expr(Interp *interp, Env *env, const Expr *expr) {
  switch (expr->kind) {
  case E_INT: return val_int(expr->as.int_value);
  case E_STR: return val_str(expr->as.str_value);
  case E_BOOL: return val_bool(expr->as.bool_value);
  case E_KYO: return val_kyo();
  case E_VAR: return env_get(env, expr->as.var_name, expr->line, expr->col);
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
    int left = val_as_bool(&lhs, expr->line, expr->col);
    val_free(&lhs);
    if (expr->as.logic.op == T_KATSU && !left) return val_bool(0);
    if (expr->as.logic.op == T_ARUIWA && left) return val_bool(1);
    Value rhs = eval_expr(interp, env, expr->as.logic.rhs);
    int right = val_as_bool(&rhs, expr->line, expr->col);
    val_free(&rhs);
    return val_bool(right);
  }
  case E_NOT: {
    Value inner = eval_expr(interp, env, expr->as.not_expr.expr);
    Value out = val_not(&inner, expr->line, expr->col);
    val_free(&inner);
    return out;
  }
  case E_CALL: return eval_call(interp, env, expr);
  }
  return val_kyo();
}

static ExecResult exec_scoped_block(Interp *interp, Env *outer, Stmt **body, int nbody) {
  Env *env = env_new(outer);
  ExecResult result = exec_block(interp, env, body, nbody);
  env_free(env);
  return result;
}

static ExecResult exec_stmt(Interp *interp, Env *env, const Stmt *stmt) {
  switch (stmt->kind) {
  case S_SUMMON:
  case S_ASSIGN: {
    Value value = eval_expr(interp, env, stmt->as.bind.expr);
    int ok = stmt->kind == S_ASSIGN
                 ? env_assign(env, stmt->as.bind.name, &value)
                 : env_declare(env, stmt->as.bind.name, &value);
    val_free(&value);
    if (!ok) {
      tatari_fatal(1, stmt->line, stmt->col,
                   stmt->kind == S_ASSIGN ? "式神%sは此の界に在らず" : "式神%sは既に此の界に在り",
                   stmt->as.bind.name);
    }
    return flow_result(FLOW_NORMAL);
  }
  case S_RELEASE:
    if (!env_release(env, stmt->as.release.name)) {
      tatari_fatal(1, stmt->line, stmt->col, "式神%sは此の界に在らず", stmt->as.release.name);
    }
    return flow_result(FLOW_NORMAL);
  case S_TAKUSEN: {
    long long input = 0;
    if (scanf("%lld", &input) != 1) tatari_fatal(1, stmt->line, stmt->col, "託宣を得られぬ");
    Value value = val_int(input);
    int ok = env_declare(env, stmt->as.takusen.name, &value);
    val_free(&value);
    if (!ok) tatari_fatal(1, stmt->line, stmt->col, "式神%sは既に此の界に在り", stmt->as.takusen.name);
    return flow_result(FLOW_NORMAL);
  }
  case S_PRINT: {
    Value value = eval_expr(interp, env, stmt->as.print_stmt.expr);
    val_print(stdout, &value, stmt->line, stmt->col);
    val_free(&value);
    return flow_result(FLOW_NORMAL);
  }
  case S_IF:
    for (int i = 0; i < stmt->as.if_stmt.nbranch; i++) {
      Value cond = eval_expr(interp, env, stmt->as.if_stmt.conds[i]);
      int truth = val_as_bool(&cond, stmt->as.if_stmt.conds[i]->line, stmt->as.if_stmt.conds[i]->col);
      val_free(&cond);
      if (truth) return exec_scoped_block(interp, env, stmt->as.if_stmt.blocks[i], stmt->as.if_stmt.block_counts[i]);
    }
    if (stmt->as.if_stmt.nelse > 0) return exec_scoped_block(interp, env, stmt->as.if_stmt.else_block, stmt->as.if_stmt.nelse);
    return flow_result(FLOW_NORMAL);
  case S_FOR: {
    Value from_value = eval_expr(interp, env, stmt->as.for_stmt.from);
    Value to_value = eval_expr(interp, env, stmt->as.for_stmt.to);
    Value step_value = stmt->as.for_stmt.step != NULL ? eval_expr(interp, env, stmt->as.for_stmt.step) : val_int(1);
    long long current = val_as_int(&from_value, stmt->as.for_stmt.from->line, stmt->as.for_stmt.from->col);
    long long end = val_as_int(&to_value, stmt->as.for_stmt.to->line, stmt->as.for_stmt.to->col);
    long long step = val_as_int(&step_value, stmt->as.for_stmt.step != NULL ? stmt->as.for_stmt.step->line : stmt->line,
                                stmt->as.for_stmt.step != NULL ? stmt->as.for_stmt.step->col : stmt->col);
    val_free(&from_value);
    val_free(&to_value);
    val_free(&step_value);
    if (step == 0) tatari_fatal(1, stmt->line, stmt->col, "歩は零ならず");

    Env *loop_env = env_new(env);
    Value counter = val_int(current);
    env_declare(loop_env, stmt->as.for_stmt.var, &counter);
    val_free(&counter);
    while ((step > 0 && current <= end) || (step < 0 && current >= end)) {
      counter = val_int(current);
      env_assign(loop_env, stmt->as.for_stmt.var, &counter);
      val_free(&counter);
      ExecResult result = exec_scoped_block(interp, loop_env, stmt->as.for_stmt.body, stmt->as.for_stmt.nbody);
      if (result.flow == FLOW_RETURN) {
        env_free(loop_env);
        return result;
      }
      Flow flow = result.flow;
      val_free(&result.ret);
      if (flow == FLOW_BREAK) break;
      if ((step > 0 && current > LLONG_MAX - step) ||
          (step < 0 && current < LLONG_MIN - step)) break;
      current += step;
    }
    env_free(loop_env);
    return flow_result(FLOW_NORMAL);
  }
  case S_WHILE:
    for (;;) {
      Value cond = eval_expr(interp, env, stmt->as.while_stmt.cond);
      int truth = val_as_bool(&cond, stmt->as.while_stmt.cond->line, stmt->as.while_stmt.cond->col);
      val_free(&cond);
      if (!truth) break;
      ExecResult result = exec_scoped_block(interp, env, stmt->as.while_stmt.body, stmt->as.while_stmt.nbody);
      if (result.flow == FLOW_RETURN) return result;
      Flow flow = result.flow;
      val_free(&result.ret);
      if (flow == FLOW_BREAK) break;
    }
    return flow_result(FLOW_NORMAL);
  case S_KEKKAI:
    return exec_scoped_block(interp, env, stmt->as.block_stmt.body, stmt->as.block_stmt.nbody);
  case S_BREAK: return flow_result(FLOW_BREAK);
  case S_CONTINUE: return flow_result(FLOW_CONTINUE);
  case S_CALLSTMT: {
    Value value = eval_expr(interp, env, stmt->as.call_stmt.call);
    val_free(&value);
    return flow_result(FLOW_NORMAL);
  }
  case S_RETURN:
    return stmt->as.return_stmt.expr == NULL
               ? return_result(val_kyo())
               : return_result(eval_expr(interp, env, stmt->as.return_stmt.expr));
  }
  return flow_result(FLOW_NORMAL);
}

static ExecResult exec_block(Interp *interp, Env *env, Stmt **body, int nbody) {
  for (int i = 0; i < nbody; i++) {
    ExecResult result = exec_stmt(interp, env, body[i]);
    if (result.flow != FLOW_NORMAL) return result;
    val_free(&result.ret);
  }
  return flow_result(FLOW_NORMAL);
}

void interp_execute(const Program *program) {
  Interp interp = {program};
  const Rite *entry = entry_rite(program);
  if (entry->nparams != 0) tatari_fatal(1, entry->line, entry->col, "開白に賜り物あるべからず");
  Env *env = env_new(NULL);
  ExecResult result = exec_block(&interp, env, entry->body, entry->nbody);
  val_free(&result.ret);
  env_free(env);
}

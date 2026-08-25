#!/bin/sh
set -u

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
ONMYO="$ROOT/onmyo"
CASES="$ROOT/test/cases"

if [ ! -x "$ONMYO" ]; then
  (cd "$ROOT" && make) || exit 1
fi

fail=0
for fu in "$CASES"/*.fu; do
  name=$(basename "$fu" .fu)
  expected="$CASES/$name.expected"
  input="$CASES/$name.in"
  exit_file="$CASES/$name.exit"
  out="$CASES/$name.out.tmp"
  err="$CASES/$name.err.tmp"
  expected_err="$CASES/$name.err.expected"

  expected_code=0
  if [ -f "$exit_file" ]; then
    expected_code=$(cat "$exit_file")
  fi

  if [ -f "$input" ]; then
    "$ONMYO" "$fu" < "$input" > "$out" 2> "$err"
  else
    "$ONMYO" "$fu" > "$out" 2> "$err"
  fi
  code=$?

  if [ "$code" -ne "$expected_code" ]; then
    echo "FAIL $name: exit $code, expected $expected_code"
    fail=1
  elif ! diff -u "$expected" "$out"; then
    echo "FAIL $name: output differs"
    fail=1
  elif [ -f "$expected_err" ] && ! diff -u "$expected_err" "$err"; then
    echo "FAIL $name: error output differs"
    fail=1
  else
    echo "PASS $name"
  fi

  rm -f "$out" "$err"
done

if "$ONMYO" --tokens "$CASES/lex_taiin.fu" > "$CASES/tokens.out.tmp" 2> "$CASES/tokens.err.tmp" &&
   diff -u "$CASES/cli_tokens.expected" "$CASES/tokens.out.tmp"; then
  echo "PASS cli_tokens"
else
  echo "FAIL cli_tokens"
  fail=1
fi
rm -f "$CASES/tokens.out.tmp" "$CASES/tokens.err.tmp"

if "$ONMYO" --ast "$CASES/chain.fu" > "$CASES/ast.out.tmp" 2> "$CASES/ast.err.tmp" &&
   diff -u "$CASES/cli_ast.expected" "$CASES/ast.out.tmp"; then
  echo "PASS cli_ast"
else
  echo "FAIL cli_ast"
  fail=1
fi
rm -f "$CASES/ast.out.tmp" "$CASES/ast.err.tmp"

exit "$fail"

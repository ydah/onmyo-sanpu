#!/bin/sh
set -u

ROOT=$(CDPATH='' cd -- "$(dirname -- "$0")/.." && pwd)
ONMYO="$ROOT/onmyo"
TMP=$(mktemp -d /tmp/onmyo-depth.XXXXXX) || exit 1
trap 'rm -rf "$TMP"' EXIT HUP INT TERM

fail=0
check_failure() {
  name=$1
  source=$2
  expected_code=$3
  expected_message=$4
  out="$TMP/$name.out"
  err="$TMP/$name.err"

  "$ONMYO" "$source" >"$out" 2>"$err"
  code=$?
  if [ "$code" -ne "$expected_code" ]; then
    echo "FAIL $name: exit $code, expected $expected_code"
    fail=1
  elif [ -s "$out" ]; then
    echo "FAIL $name: unexpected output"
    fail=1
  elif ! grep -q "^祟り：$expected_message" "$err"; then
    echo "FAIL $name: expected diagnostic missing"
    fail=1
  elif grep -Eq 'Sanitizer|runtime error:' "$err"; then
    echo "FAIL $name: sanitizer diagnostic"
    fail=1
  else
    echo "PASS $name"
  fi
}

expr="$TMP/expr.fu"
{
  echo '開白と申す行法、修するに'
  echo '臨に臨を生じ'
  i=0
  while [ "$i" -lt 1023 ]; do
    echo '更に臨を生じ'
    i=$((i + 1))
  done
  echo '、これを唱へ 急急如律令'
  echo '結願'
  echo '悉地成就'
} >"$expr"
check_failure depth_expr "$expr" 2 '式が深過ぎる'

block="$TMP/block.fu"
{
  echo '開白と申す行法、修するに'
  i=0
  while [ "$i" -lt 512 ]; do
    echo '結界を張り'
    i=$((i + 1))
  done
  echo '臨 を唱へ 急急如律令'
  i=0
  while [ "$i" -lt 513 ]; do
    echo '結願'
    i=$((i + 1))
  done
  echo '悉地成就'
} >"$block"
check_failure depth_block "$block" 2 '段の入れ子が深過ぎる'

call="$TMP/call.fu"
cat >"$call" <<'EOF'
再帰と申す行法、貴人を賜りて、修するに
  占ふに 貴人は 無 に勝らず ば
    無 を献じ 急急如律令
  結願
  貴人より臨を剋し、これを以て再帰を修せしむ、これを献じ 急急如律令
結願
開白と申す行法、修するに
  皆臨闘 を以て再帰を修せしむ、これを唱へ 急急如律令
結願
悉地成就
EOF
check_failure depth_call "$call" 1 '行法の呼出が深過ぎる'

exit "$fail"

#!/bin/sh
set -eu

write=0
if [ "${1-}" = "--write" ]; then
  write=1
  shift
elif [ "${1-}" = "--dry-run" ]; then
  shift
fi

if [ "$#" -eq 0 ]; then
  echo "使ひ方: tools/aratame.sh [--dry-run|--write] file.fu ..." >&2
  exit 3
fi

for source in "$@"; do
  tmp=$(mktemp "${TMPDIR:-/tmp}/onmyo-aratame.XXXXXX")
  trap 'rm -f "$tmp"' EXIT HUP INT TERM
  sed -E \
    -e 's/「([^」]*)」/阿\1吽/g' \
    -e 's/〘[[:space:]]*(.*)[[:space:]]*〙/註 \1/g' \
    -e 's/蠱し/乗じ/g' \
    -e 's/異なり/同じからず/g' \
    -e 's/勝るか同じく/劣らず/g' \
    -e 's/劣るか同じく/勝らず/g' \
    -e 's/之行法/と申す行法/g' \
    -e 's/『主』/開白/g' \
    -e 's/反([無臨兵闘者皆陣列在前])/陰\1/g' \
    -e 's/(^|[[:space:]])陽([[:space:],、]|$)/\1吉\2/g' \
    -e 's/(^|[[:space:]])陰([[:space:],、]|$)/\1凶\2/g' \
    -e 's/自[[:space:]]+([^[:space:]]+)[[:space:]]+至[[:space:]]+([^、[:space:]]+)[[:space:]]+歩み[[:space:]]+([^、[:space:]]+)、[[:space:]]*式神(『[^』]*』)を反閇せしむ/式神\4をして \1 より \2 に至るまで 歩を \3 とし 反閇せしむ/g' \
    -e 's/自[[:space:]]+([^[:space:]]+)[[:space:]]+至[[:space:]]+([^、[:space:]]+)、[[:space:]]*式神(『[^』]*』)を反閇せしむ/式神\3をして \1 より \2 に至るまで 反閇せしむ/g' \
    -e 's/式神(『[^』]*』)に[[:space:]]*改め/式神\1を改め/g' \
    -e 's/(.*)を([[:space:]]+[^[:space:]]+[[:space:]]+に(同じく|同じからず|勝り|勝らず|劣り|劣らず))/\1は\2/' \
    -e '/『[^』]*』/s/$/ 註 要確認：式神名を割り当て括弧を除く/' \
    -e '/自.*至|歩み/s/$/ 註 要確認：反閇文を宣命体へ改める/' \
    -e '/〔|〕/s/$/ 註 要確認：括りを連鎖または式神へ分解/' \
    -e '/生じ|剋し|乗じ|祓ひ|穢し/s/$/ 註 要確認：述句の受け/' \
    "$source" > "$tmp"

  if [ "$write" -eq 1 ]; then
    mv "$tmp" "$source"
  else
    diff -u "$source" "$tmp" || true
    rm -f "$tmp"
  fi
  trap - EXIT HUP INT TERM
done

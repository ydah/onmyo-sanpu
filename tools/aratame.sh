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
    -e 's/蠱し/乗じ/g' \
    -e 's/異なり/同じからず/g' \
    -e 's/勝るか同じく/劣らず/g' \
    -e 's/劣るか同じく/勝らず/g' \
    -e 's/之行法/と申す行法/g' \
    -e 's/『主』/『開白』/g' \
    -e 's/式神(『[^』]*』)に[[:space:]]*改め/式神\1を改め/g' \
    -e '/反[無臨兵闘者皆陣列在前]/s/$/ 〘 要確認：負の符 〙/' \
    -e '/[陰陽]/s/$/ 〘 要確認：卦または数の符 〙/' \
    -e '/同じく|同じからず|勝り|勝らず|劣り|劣らず/s/$/ 〘 要確認：第一格 〙/' \
    -e '/生じ|剋し|乗じ|祓ひ|穢し/s/$/ 〘 要確認：述句の受け 〙/' \
    "$source" > "$tmp"

  if [ "$write" -eq 1 ]; then
    mv "$tmp" "$source"
  else
    diff -u "$source" "$tmp" || true
    rm -f "$tmp"
  fi
  trap - EXIT HUP INT TERM
done

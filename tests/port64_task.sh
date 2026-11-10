#!/bin/sh
# port64_task: the same kernel-task scenario on two builds must print the same trace (tests/port64_task_test.c,
# docs/port64.md). Usage: port64_task.sh <reference exe> <exe under test> [SLUS_217.82]
#   port64_task         reference: the i386 build; under test: the rewritten x86_64 build
#   port64_task_retail  reference: the retail instructions in the R5900 interpreter; under test: the rewritten x86_64 build
# Exit 77 (skipped) when the 64-bit executable cannot map its guest block on this host.
set -e
exe32=$1
exe64=$2
retail=$3
out=$(mktemp -d)
trap 'rm -rf "$out"' EXIT
if ! "$exe64" $retail >"$out/64.txt" 2>"$out/64.err"; then
  if grep -q "cannot map guest memory" "$out/64.err"; then exit 77; fi
  cat "$out/64.err"
  exit 1
fi
"$exe32" $retail >"$out/32.txt"
lines=$(wc -l <"$out/32.txt")
if cmp "$out/32.txt" "$out/64.txt"; then
  echo "port64_task: reference and x86_64 traces identical ($lines lines, $(tail -1 "$out/64.txt"))"
else
  diff "$out/32.txt" "$out/64.txt" | head -40
  exit 1
fi

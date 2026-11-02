#!/bin/bash
# x86_64 syntax check of every port-owned src/ file with the pointer/int warnings on (docs/port64.md M3).
#   tools/port64/src_check.sh [out-dir] [files...]
# Prints the files that fail to compile, then every warning/error with its category. Needs clang with
# x86_64 and i386 headers (the p4pc-ci image, or Debian clang). Exit status 1 if anything is reported.
set -u
R=$(cd "$(dirname "$0")/../.." && pwd)
OUT=${1:-${TMPDIR:-/tmp}/p64_src_check}; shift || true
TGT=${P64_TARGET:---target=x86_64-linux-gnu}   # P64_TARGET="--target=i386-linux-gnu -m32" checks the shipping i386 build too
rm -rf "$OUT"; mkdir -p "$OUT"
cd "$R"
WARN="-Wpointer-to-int-cast -Wint-to-pointer-cast -Wint-conversion -Wpointer-sign -Wincompatible-pointer-types -Wformat -Wvarargs -Wshorten-64-to-32 -Wint-to-void-pointer-cast -Wvoid-pointer-to-int-cast"   # function-pointer casts between register ABIs are checked by the callback table (docs/port64.md 2.6), not here
INC="-I$R/include -I$R/tests -I$R/overrides -I$R/decomp/include -I$R/third_party $(find src -type d | sed "s#^#-I$R/#" | tr '\n' ' ')"
# os_win32.c needs the Windows headers: check it with a mingw clang (see docs/port64.md), not here
if [ $# -gt 0 ]; then FILES="$*"; else FILES=$(find src -name '*.c' ! -name os_win32.c | sort); fi
for f in $FILES; do echo "$f"; done | xargs -P "$(nproc)" -I{} sh -c \
  'clang '"$TGT"' -DP4_PC=1 -D_FILE_OFFSET_BITS=64 -O0 -fsyntax-only -include p4pc/compat.h -std=gnu11 '"$INC $WARN"' -ferror-limit=0 {} > "'"$OUT"'/$(echo {} | tr / _).log" 2>&1; echo $? {} >> "'"$OUT"'/rc.txt"'
echo "== failing files"; awk '$1!=0 {print $2}' "$OUT/rc.txt" | sort
echo "== diagnostics by category"
cat "$OUT"/*.log | grep -E "^[^ ]+:[0-9]+:[0-9]+: (warning|error):" | sed -E 's/.*\[(-W[a-z0-9-]+)\]$/\1/; t; s/.*error:.*/ERROR/' | sort | uniq -c | sort -rn
n=$(cat "$OUT"/*.log | grep -cE "^[^ ]+:[0-9]+:[0-9]+: (warning|error):")
echo "== total diagnostics: $n (logs in $OUT)"
[ "$n" -eq 0 ]

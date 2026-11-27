#!/bin/sh
# The production crash handler on the arm64 phone (the Android twin of tests/crash_log_test.py, which needs Python): the standalone fault binary
# (tests/crash_log_test.c) raises or faults each signal, and the report it leaves must name the signal, the fault address, an instruction
# and a frame, the guest address for a guest fault, and the marker file must hold the last staged marker whole. On AArch64 the report's
# instruction and frame come from the signal context's register block, the part x86_64 does not exercise.
#   sh tests/crash_log_test.sh <crash_log_test executable>
exe=${1:?usage: crash_log_test.sh <crash_log_test executable>}
dir=${TMPDIR:-/tmp}/p4pc-crash-$$
rm -rf "$dir"; mkdir -p "$dir" || exit 1
fail=0
for sig in 11 7 4 8 6; do
  d=$dir/sig$sig; mkdir -p "$d"
  "$exe" "$d/boot.log" "$sig" "$d/crash-7.marker" > "$d/out.txt" 2>&1
  rc=$?
  report=$d/p4pc-crash.log
  if [ "$rc" != $((128 + sig)) ]; then echo "FAIL signal $sig: exit $rc, expected $((128 + sig))"; fail=1; fi
  if [ ! -f "$report" ]; then echo "FAIL signal $sig: no report"; fail=1; continue; fi
  printf 'P4PC-CRASH 1 session=7\ntest.mod\tR1:B3\n' | cmp -s - "$d/crash-7.marker" || { echo "FAIL signal $sig: marker differs"; fail=1; }
  for text in "CRASH signal $sig" "fault address" "instruction 0:" "frame 0:"; do
    grep -q "$text" "$report" || { echo "FAIL signal $sig: report lacks '$text'"; fail=1; }
  done
  if [ "$sig" = 11 ]; then grep -q "guest address 0xdeadbeef" "$report" || { echo "FAIL signal 11: report lacks the guest address"; fail=1; }; fi
  [ "$fail" = 0 ] && echo "signal $sig: exit=$rc report=$(wc -c < "$report") bytes PASS"
done
if [ "$fail" != 0 ]; then
  for f in "$dir"/sig*/p4pc-crash.log; do echo "--- $f"; head -n 40 "$f"; done
fi
rm -rf "$dir"
exit $fail

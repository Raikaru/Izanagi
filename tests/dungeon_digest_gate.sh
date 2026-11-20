#!/bin/sh
# Cross-target dungeon determinism. Private checkouts compare the complete recorded
# rows; public exports compare only their one-way component hashes. In either
# case the input walk and checkpoint must complete before comparison.
# The gate pairs carry P4PC_GATE_TARGET (windows / linux).
#   usage: dungeon_digest_gate.sh [--record] <p4pc> <gate flip> <field:room[,month/day]> <save folder> <walk file>
set -u
here=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
. "$here/input_guard.sh"
record=0
if [ "${1:-}" = --record ]; then record=1; shift; fi
[ $# -eq 5 ] || { echo "usage: dungeon_digest_gate.sh [--record] <p4pc> <gate flip> <field:room[,month/day]> <save folder> <walk file>"; exit 2; }
expected=$here/dungeon_digests/${5%.walk}.txt
private_expected=$here/dungeon_digests_private/${5%.walk}.txt
private=0
if [ -f "$private_expected" ]; then expected=$private_expected; private=1; fi
if [ $record = 0 ] && [ ! -f "$expected" ]; then
    echo "dungeon_digest: no recorded digest $expected"
    exit 1
fi
log=$(mktemp)
rows=$(mktemp)
trap 'rm -f "$log" "$rows"' EXIT
P4PC_GATE_BOOT_LOG=$log P4PC_GATE_REQUIRE_SAVE=0 P4PC_DUNGEON_DIGEST=1 sh "$here/save_play_gate.sh" "$@"
rc=$?
[ $rc -eq 0 ] || exit $rc
if ! grep -aq 'walk script done' "$log"; then
    echo 'dungeon_digest: missing completed canonical input checkpoint'
    exit 1
fi
grep -a '^p4pc: dungeon digest flip ' "$log" | sed 's/^p4pc: //' | tr -d '\r' > "$rows"
if [ ! -s "$rows" ]; then
    echo 'dungeon_digest: the run printed no dungeon digest'
    exit 1
fi
if [ "$private" = 0 ]; then
    projected=$(mktemp)
    trap 'rm -f "$log" "$rows" "$projected"' EXIT
    "${P4PC_PYTHON:-python3}" "$here/dungeon_digest_project.py" < "$rows" > "$projected" || exit 1
    mv "$projected" "$rows" || exit 1
fi
if [ $record = 1 ]; then
    mkdir -p "$(dirname "$expected")" && cp "$rows" "$expected" || exit 1
    echo "dungeon_digest: recorded $(wc -l < "$expected") rows in $expected (target ${P4PC_GATE_TARGET:-unset})"
    exit 0
fi
if diff "$expected" "$rows" > /dev/null; then
    echo "dungeon_digest: PASS $(wc -l < "$rows") rows equal the cross-target recording (target ${P4PC_GATE_TARGET:-unset})"
    exit 0
fi
echo "dungeon_digest: differs from $expected (target ${P4PC_GATE_TARGET:-unset})"
awk 'NR == FNR { want[FNR] = $0; n = FNR; next }
     { got[FNR] = $0; m = FNR }
     END {
         for (i = 1; i <= (n > m ? n : m); i++) {
             if (want[i] == got[i]) continue;
             print "  first differing row " i ":"
             print "  recorded: " want[i]
             print "  this run: " got[i]
             a = split(want[i], w, " "); b = split(got[i], g, " "); d = ""
             for (k = 3; k < (a > b ? a : b); k += 2) if (w[k + 1] != g[k + 1]) d = d " " w[k]
             print "  differing:" (d == "" ? " row count" : d)
             exit
         }
     }' "$expected" "$rows"
exit 1

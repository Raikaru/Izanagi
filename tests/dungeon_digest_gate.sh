#!/bin/sh
# Cross-target dungeon determinism: a completed collection walk over generated dungeon floors (completed_walk_gate.sh's arguments) whose dungeon digest
# (P4PC_DUNGEON_DIGEST, src/platform/pad/pad_sdl.c: per-component hashes of the floor ids, step and seed, the floor's cell grid, the additive random
# generator, RpRandom's Mersenne Twister, the player's matrix, the floor's units and the battle state every 100 flips, with a rolling hash of the player
# and the generator over every flip) must equal tests/dungeon_digests/<walk>.txt, which the Windows x64 and the Linux x64 builds both recorded.
# A difference prints the first differing row and its differing components: the flip and the kind of state where the targets split
# (docs/platform/numeric_determinism.md). Re-record on both targets after a deliberate arithmetic change, never on one.
# The gate pairs in tests/cmake/180-story.cmake carry P4PC_GATE_TARGET (windows / linux): tools/land_game_split.py runs one of each pair per target.
#   usage: dungeon_digest_gate.sh [--record] <p4pc> <gate flip> <field:room[,month/day]> <save folder> <walk file>
set -u
here=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
. "$here/input_guard.sh"
record=0
if [ "${1:-}" = --record ]; then record=1; shift; fi
[ $# -eq 5 ] || { echo "usage: dungeon_digest_gate.sh [--record] <p4pc> <gate flip> <field:room[,month/day]> <save folder> <walk file>"; exit 2; }
expected=$here/dungeon_digests/${5%.walk}.txt
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
             for (k = 5; k < (a > b ? a : b); k += 2) if (w[k + 1] != g[k + 1]) d = d " " w[k]
             print "  differing:" (d == "" ? " row count" : d)
             exit
         }
     }' "$expected" "$rows"
exit 1

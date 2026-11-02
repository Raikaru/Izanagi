#!/bin/sh
# Cross-target determinism gate: the 04/11 save (<retail dir>/saves/card) loaded through the title, then the walk script tools/systems/save0411_to_morning.txt
# up the stairs, to the futon, into the fog dream and through its tutorial battle. The run prints a rolling digest of the
# player's matrix and RpRandom's state (P4PC_WALK_DIGEST, src/platform/pad/pad_sdl.c) every 500 flips and the battle's state changes (P4PC_BATTLE_LOG); both must
# equal the retained cross-target digest, which the Linux and the Windows build print alike: any float or library difference in the game's arithmetic
# (a driver that changed the x87 control word, the host's sinf, a fused multiply-add) moves the matrix within a few flips and the digest with it.
# On Windows (a GPU is needed) run this script with the Windows exe. Rerecord on both targets after a deliberate arithmetic change;
# preserve the full battle teardown and actual 04/12 living-room return, never shorten the golden to a still-running battle.
#   usage: walk_digest_gate.sh [--record] <p4pc>
# Skipped (exit 77) without P4PC_ISO, P4PC_ELF or the save next to the ELF.
. "$(dirname "$0")/input_guard.sh"

record=0
if [ "$1" = "--record" ]; then record=1; shift; fi
prog=$1
if [ -z "$P4PC_ISO" ] || [ -z "$P4PC_ELF" ]; then
  echo "walk_digest_gate: P4PC_ISO or P4PC_ELF unset, skipped"
  exit 77
fi
saves=$(dirname "$P4PC_ELF")/saves
if [ ! -d "$saves/card" ]; then
  echo "walk_digest_gate: no retail save in $saves, skipped"
  exit 77
fi
here=$(cd "$(dirname "$0")" && pwd)
goal=6600
last=6500
expected="$here/walk_digest_expected.txt"
. "$(dirname "$0")/artifacts.sh"
gate_artifacts_begin p4pc_walk_digest || exit 1
mkdir -p "$work/card/mc0" && cp -r "$saves/card"/. "$work/card/mc0/" || exit 1
cd "$work" || exit 1
env SDL_VIDEODRIVER="${SDL_VIDEODRIVER:-offscreen}" SDL_AUDIODRIVER=dummy P4PC_TRAP_CONTINUE=1 P4PC_SAVE_DIR="$work/card" P4PC_TURBO=2 \
  P4PC_AUTOPRESS_FRAMES=1 P4PC_AUTOPRESS_LOAD=0 P4PC_WALK_SCRIPT="$here/../tools/systems/save0411_to_morning.txt" P4PC_WALK_DIGEST=1 P4PC_BATTLE_LOG=1 \
  P4PC_NEWGAME_GATE="$goal" timeout -s KILL 560 "$prog" "$P4PC_ELF" > "$work/boot.log" 2>&1
if [ $? -ne 0 ] || ! grep -aq "new-game gate PASS at flip ${goal%%,*}:" "$work/boot.log"; then
  echo "walk_digest_gate: the run did not finish its $goal goal"
  tail -n 12 "$work/boot.log"
  exit 1
fi
grep -a "walk digest flip\|battle driver: flip" "$work/boot.log" | sed 's/^p4pc: //' > "$work/digest.txt"
if ! grep -q "walk digest flip $last " "$work/digest.txt"; then
  echo "walk_digest_gate: the run did not reach flip $last"
  tail -n 12 "$work/boot.log"
  exit 1
fi
if [ $record = 1 ]; then
  cp "$work/digest.txt" "$expected"
  echo "walk_digest_gate: recorded $expected"
  exit 0
fi
if diff "$expected" "$work/digest.txt" > "$work/diff.txt"; then
  echo "walk_digest_gate: digest and battle states equal the recorded run"
  exit 0
fi
echo "walk_digest_gate: differs from $expected (< expected, > this run):"
head -n 30 "$work/diff.txt"
exit 1

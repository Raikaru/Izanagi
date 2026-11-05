#!/bin/sh
# Shuffle Time gate: the harness battle (src/platform/battle_harness.c) ends in Shuffle Time with the pattern and variant P4PC_SHUFFLE forces
# through the game's own roll tables (src/platform/shuffle_harness.c), the walk script's battle driver wins the fight and plays the cards
# (tests/walks/shuffle_time.walk), and p4pc exits 0 once the game's shuffle task has run its sequence to the end and been destroyed. Mods and
# packs off, no trap continuation: a fault, an assertion, a trap or a sequence that never ends fails it.
#   usage: shuffle_gate.sh <p4pc> <pattern 0..4>:<first|variant> [encounter [hero level]]
# The default fight is encounter 16, one shadow 0x11 (73 HP), against the new-game hero at level 9: Attack wins it in a few turns, and below
# level 10 btlShuffleSelectMember (0036ee60) draws its Persona cards from the early-game level band, as a first Shuffle Time does.
# Skipped (exit 77) without P4PC_ISO or P4PC_ELF.
. "$(dirname "$0")/input_guard.sh"

if [ -z "$P4PC_ISO" ] || [ -z "$P4PC_ELF" ]; then
  echo "shuffle_gate: P4PC_ISO or P4PC_ELF unset, skipped"
  exit 77
fi
prog=$1 shuffle=$2 enc=${3:-16} level=${4:-9}
walk=$(cd "$(dirname "$0")" && pwd)/walks/shuffle_time.walk
. "$(dirname "$0")/artifacts.sh"
gate_artifacts_begin p4pc_shuffle_gate || exit 1
cd "$work" || exit 1
env SDL_VIDEODRIVER="${SDL_VIDEODRIVER:-offscreen}" SDL_AUDIODRIVER=dummy P4PC_MODS=0 P4PC_PACKS=0 P4PC_VIRTUAL_CLOCK=1 \
  P4PC_START_BATTLE="$enc" P4PC_BATTLE_HERO_LEVEL="$level" P4PC_SHUFFLE="$shuffle" P4PC_WALK_SCRIPT="$walk" \
  P4PC_TURBO=2 P4PC_TURBO_WINDOW="${P4PC_TURBO_WINDOW:-4}" \
  timeout -s KILL 560 "$prog" "$P4PC_ELF" > "$work/boot.log" 2>&1
rc=$?
grep -a "shuffle gate\|guest fault\|kAssert\|CRASH" "$work/boot.log" | tail -n 40
if [ $rc -ne 0 ]; then
  echo "shuffle_gate: p4pc exited $rc"
  tail -n 30 "$work/boot.log"
elif ! grep -aq "shuffle gate: pattern .*: PASS" "$work/boot.log"; then
  echo "shuffle_gate: p4pc exited 0 without the shuffle gate's PASS"
  rc=1
fi
exit $rc

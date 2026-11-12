#!/bin/sh
# Scene gate for the battle: boots the engine as the real boot does, enters encounter 20 through the battle harness
# (src/platform/battle_harness.c). It retains the original state-7/nonblack
# witness at its fixed flip; the process timeout still fails a hang.
#   usage: battle_gate.sh <p4pc> [encounter] [gate frame]
# Skipped (exit 77) without P4PC_ISO or P4PC_ELF.
. "$(dirname "$0")/input_guard.sh"

if [ -z "$P4PC_ISO" ] || [ -z "$P4PC_ELF" ]; then
  echo "battle_gate: P4PC_ISO or P4PC_ELF unset, skipped"
  exit 77
fi
prog=$1 enc=${2:-20} gate=${3:-420}
. "$(dirname "$0")/artifacts.sh"
gate_artifacts_begin p4pc_battle_gate || exit 1
capture=$((gate - 1))
cd "$work" || exit 1
env SDL_VIDEODRIVER="${SDL_VIDEODRIVER:-offscreen}" SDL_AUDIODRIVER=dummy P4PC_TRAP_CONTINUE=1 P4PC_START_BATTLE="$enc" \
  P4PC_TURBO=2 P4PC_TURBO_WINDOW="${P4PC_TURBO_WINDOW:-4}" P4PC_BATTLE_GATE="$gate" P4PC_CAPTURE_FRAMES="$capture" P4PC_CAPTURE_PNG="$work/gate_%d.png" \
  timeout -s KILL 280 "$prog" "$P4PC_ELF" > "$work/boot.log" 2>&1
rc=$?
grep -a "battle gate\|btlMain state ->" "$work/boot.log" | tail -n 12
if [ $rc -ne 0 ]; then
  echo "battle_gate: p4pc exited $rc"
  tail -n 20 "$work/boot.log"
fi
exit $rc

#!/bin/sh
# Scene gate for the camp menu: boots the engine through the menu harness (src/platform/menu_harness.c) into Dojima's living room
# (field 7, room 2), opens the camp menu with the game's own Triangle trigger and walks Skill, Item, Equip, Persona, Status, S.Link
# and System, taking a frame of each. The harness exits 0 only when the camp task opened, every frame is lit (more than 30% of
# its pixels) and every PNG was written; 1 otherwise. A crash, write error or hang fails the test.
#   usage: menu_gate.sh <p4pc> [script | --expect-write-failure]
# P4PC_TURBO=2: only the last P4PC_DRAW_WINDOW (default 10) flips before each shot: draw (menu_harness.c, hintShotDraws).
# Skipped (exit 77) without P4PC_ISO or P4PC_ELF.
. "$(dirname "$0")/input_guard.sh"

if [ -z "$P4PC_ISO" ] || [ -z "$P4PC_ELF" ]; then
  echo "menu_gate: P4PC_ISO or P4PC_ELF unset, skipped"
  exit 77
fi
prog=$1
script=$2
expect_write_failure=0
if [ "$script" = "--expect-write-failure" ]; then
  expect_write_failure=1
  script='open,wait30,shot:write_failure'
fi
if [ -z "$script" ]; then
  script='open,wait30,shot:top'
  for n in skill item equip persona status slink system; do
    script="$script,cross,wait110,shot:$n,circle,wait60,down"
  done
fi
. "$(dirname "$0")/artifacts.sh"
gate_artifacts_begin p4pc_menu_gate || exit 1
cd "$work" || exit 1
if [ "$expect_write_failure" = 1 ]; then
  # A directory is not writable as a PNG, even when CI runs as root.
  mkdir menu_write_failure.png || exit 1
fi
env SDL_VIDEODRIVER="${SDL_VIDEODRIVER:-offscreen}" SDL_AUDIODRIVER=dummy P4PC_TRAP_CONTINUE=1 P4PC_START_MENU=7:2 P4PC_MENU_EXIT=1 \
  P4PC_TURBO=2 P4PC_DRAW_WINDOW="${P4PC_DRAW_WINDOW:-10}" \
  P4PC_MENU_SCRIPT="$script" timeout -s KILL 540 "$prog" "$P4PC_ELF" > "$work/boot.log" 2>&1
rc=$?
grep -a "menu harness: frame [0-9]* \(camp is open\|shot lit\|script finished\)" "$work/boot.log" | tail -n 14
if [ "$expect_write_failure" = 1 ]; then
  if [ "$rc" -eq 1 ] && grep -a -q 'p4gl: writing .*menu_write_failure.png failed' "$work/boot.log"; then
    echo "menu_gate: unwritable capture correctly failed the run"
    exit 0
  fi
  echo "menu_gate: expected an unwritable PNG to exit 1, got $rc"
  exit 1
fi
if [ $rc -ne 0 ]; then
  echo "menu_gate: p4pc exited $rc"
  tail -n 20 "$work/boot.log"
fi
exit $rc

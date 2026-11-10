#!/bin/sh
# Save/load round trip through the game's own screens and the port's memory card folder (src/platform/memcard/ps2_memcard.c). The menu
# harness (src/platform/menu_harness.c) enters Dojima's living room, changes the game state (money, the hero's level and HP, story flags),
# runs the game's save screen (Cross through slot, confirm and overwrite), resets to the new-game state, runs the load screen and
# compares the save images (datSaveBuildImage) before and after: all but a few play-time counter bytes must match, and the reset state must
# differ. The save folder is checked for the retail layout: BASLUS-21782Save0 with BASLUS-21782Save0 (171112 bytes), icon.ico, icon.sys (964).
#   usage: save_gate.sh <p4pc>
# P4PC_TURBO=2: the gate takes no frame, so no draw call is made (the save and load screens are task state, not pixels).
# Skipped (exit 77) without P4PC_ISO or P4PC_ELF.
. "$(dirname "$0")/input_guard.sh"

if [ -z "$P4PC_ISO" ] || [ -z "$P4PC_ELF" ]; then
  echo "save_gate: P4PC_ISO or P4PC_ELF unset, skipped"
  exit 77
fi
prog=$1
. "$(dirname "$0")/artifacts.sh"
gate_artifacts_begin p4pc_save_gate || exit 1
cd "$work" || exit 1
script='wait0,mod,keep:saved,save,wait150,cross,wait250,cross,wait250,cross,wait400,cross,wait400,reset,keep:reset,load'
script="$script,wait150,cross,wait250,cross,wait250,cross,wait300,cross,wait400,wait300,keep:loaded,check:saved:reset:loaded"
env SDL_VIDEODRIVER="${SDL_VIDEODRIVER:-offscreen}" SDL_AUDIODRIVER=dummy P4PC_TRAP_CONTINUE=1 P4PC_START_MENU=7:2 P4PC_MENU_EXIT=1 P4PC_SAVE_DIR="$work/card" P4PC_TURBO=2 \
  P4PC_MENU_SCRIPT="$script" timeout -s KILL 560 "$prog" "$P4PC_ELF" > "$work/boot.log" 2>&1
rc=$?
grep -a "menu harness: frame [0-9]* \(kept\|check\|script finished\)" "$work/boot.log" | tail -n 8
dir="$work/card/mc0/BASLUS-21782Save0"
size() { wc -c < "$1" | tr -d ' '; }
if [ "$(size "$dir/BASLUS-21782Save0" 2>/dev/null)" != 171112 ] || [ "$(size "$dir/icon.sys" 2>/dev/null)" != 964 ] || [ ! -s "$dir/icon.ico" ]; then
  echo "save_gate: the save folder does not have the retail layout:"
  ls -l "$dir" 2>&1
  exit 1
fi
if [ $rc -ne 0 ]; then
  echo "save_gate: p4pc exited $rc"
  tail -n 20 "$work/boot.log"
fi
exit $rc

#!/bin/sh
# Real retail saves (downloaded PS3 .psv files, converted to the card folder layout by tools/ps2save.py and kept beside the disc, never committed:
# <retail dir>/saves/card for "Hiro Blaze", April 11, the Dojima living room, and <retail dir>/saves/card_apollo for a late-game save, March 20,
# Yomotsu Hirasaka, level 99, in slot 1) load through the game's own title screen: LOAD GAME, the slot, Yes, then the field starts. The menu
# harness (src/platform/menu_harness.c) then compares the game state the port builds for saving (datSaveBuildImage) with the retail file: only
# play-time counters, the checksums over them and nothing else may differ (16 bytes or fewer).
#   usage: retail_save_gate.sh <p4pc>
# P4PC_TURBO=2: the gate takes no frame, so no draw call is made.
# Skipped (exit 77) without P4PC_ISO, P4PC_ELF or the saves next to the ELF.
. "$(dirname "$0")/input_guard.sh"

if [ -z "$P4PC_ISO" ] || [ -z "$P4PC_ELF" ]; then
  echo "retail_save_gate: P4PC_ISO or P4PC_ELF unset, skipped"
  exit 77
fi
saves=$(dirname "$P4PC_ELF")/saves
if [ ! -d "$saves/card" ] || [ ! -d "$saves/card_apollo" ]; then
  echo "retail_save_gate: no retail saves in $saves, skipped"
  exit 77
fi
. "$(dirname "$0")/artifacts.sh"
prog=$1
rc=0
# run <seed dir> <slot keys> <folder name>
run() {
  gate_artifacts_begin p4pc_retail_save || exit 1
  mkdir -p "$work/card/mc0" && cp -r "$1"/. "$work/card/mc0/" || exit 1
  # the Windows exe (run from Git sh) does not understand the shell's /d/... or /tmp/... paths: the card folder and the file are given as mixed paths
  wwork=$(cygpath -m "$work" 2>/dev/null || echo "$work")
  file="$wwork/card/mc0/$3/$3"
  script="wait0,totitle,wait800,start,wait200,start,wait200,cross,wait250,cross,wait250,$2cross,wait250,cross,wait300,cross,wait300,cross,wait600,keep:field,vsfile:field:$file"
  (cd "$work" && env SDL_VIDEODRIVER="${SDL_VIDEODRIVER:-offscreen}" SDL_AUDIODRIVER=dummy P4PC_TRAP_CONTINUE=1 P4PC_START_MENU=7:2 P4PC_MENU_EXIT=1 P4PC_SAVE_DIR="$wwork/card" P4PC_TURBO=2 \
    P4PC_MENU_SCRIPT="$script" timeout -s KILL 560 "$prog" "$P4PC_ELF" > "$work/boot.log" 2>&1)
  r=$?
  echo "retail_save_gate: $3"
  grep -a "menu harness: .*vsfile\|menu harness: frame [0-9]* script finished" "$work/boot.log" | tail -n 8
  [ $r -eq 0 ] || { echo "retail_save_gate: $3: p4pc exited $r"; tail -n 20 "$work/boot.log"; rc=1; }
  gate_artifacts_cleanup "$r"
}
run "$saves/card" "" BASLUS-21782Save0
run "$saves/card_apollo" "down,wait60," BASLUS-21782Save1
exit $rc

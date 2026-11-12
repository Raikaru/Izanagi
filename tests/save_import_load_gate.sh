#!/bin/sh
# Opt-in private-data smoke: native CLI import, then the game's LOAD GAME flow.
# P4PC_IMPORT_SOURCE names a container/folder; all output stays in gate artifacts.
. "$(dirname "$0")/input_guard.sh"
if [ -z "${P4PC_IMPORT_SOURCE:-}" ] || [ -z "${P4PC_ELF:-}" ] || [ -z "${P4PC_ISO:-}" ]; then
  echo "save_import_load: private source/disc/ELF unset, skipped"
  exit 77
fi
. "$(dirname "$0")/artifacts.sh"
gate_artifacts_begin p4pc_save_import || exit 1
prog=$1
wwork=$(cygpath -m "$work" 2>/dev/null || echo "$work")
mkdir -p "$work/card"
if ! env P4PC_SAVE_DIR="$wwork/card" "$prog" --import-save "$P4PC_IMPORT_SOURCE" --slot 0 > "$work/import.log" 2>&1; then
  cat "$work/import.log"
  exit 1
fi
script="wait0,totitle,wait800,start,wait200,start,wait200,cross,wait250,cross,wait250,cross,wait250,cross,wait300,cross,wait300,cross,wait600,shot:imported,dump:loaded,state,keep:field,vsfile:field:$wwork/card/mc0/BASLUS-21782Save0/BASLUS-21782Save0"
(cd "$work" && env SDL_VIDEODRIVER="${SDL_VIDEODRIVER:-offscreen}" SDL_AUDIODRIVER=dummy P4PC_INPUT=script P4PC_CONFIG=none \
  P4PC_TRAP_CONTINUE=1 P4PC_START_MENU=7:2 P4PC_MENU_EXIT=1 P4PC_SAVE_DIR="$wwork/card" P4PC_TURBO=2 \
  P4PC_FIELD_ENTRY_DUMP="$wwork/field.jsonl" P4PC_EVENT_LOG=300 P4PC_MENU_SCRIPT="$script" \
  timeout -s KILL 560 "$prog" "$P4PC_ELF" > "$work/boot.log" 2>&1)
rc=$?
cat "$work/import.log"
if [ -f "$work/state_loaded.bin" ]; then
  "${P4PC_PYTHON:-python3}" "$(dirname "$0")/save_import_state.py" \
    "$wwork/card/mc0/BASLUS-21782Save0/BASLUS-21782Save0" "$wwork/state_loaded.bin" > "$work/state_words.json" || rc=1
fi
# Artifacts are intentionally retained: screenshots and state words are evidence.
echo "save_import_load: artifacts $wwork (status $rc)"
exit "$rc"

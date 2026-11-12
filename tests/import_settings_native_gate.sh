#!/bin/sh
# Actual Config task + production picker-completion boundary, with a private save.
. "$(dirname "$0")/input_guard.sh"
if [ -z "${P4PC_IMPORT_SOURCE:-}" ] || [ -z "${P4PC_ISO:-}" ] || [ -z "${P4PC_ELF:-}" ]; then
  echo "import_settings_native: private source/disc/ELF unset, skipped"
  exit 77
fi
prog=$1
. "$(dirname "$0")/artifacts.sh"
gate_artifacts_begin p4pc_import_settings_native || exit 1
wwork=$(cygpath -m "$work" 2>/dev/null || echo "$work")
unset P4PC_SETTINGS_RETAIL_ONLY
script='cross,wait90,settings-open,wait240,settings-check:open,settings-row:Import save,settings-import:cancel,wait20,settings-check:import-count:0,shot:import_before'
script="$script,settings-key:Escape,wait90,settings-check:closed,settings-import:complete,wait20,settings-check:import-deferred"
script="$script,settings-open,wait240,settings-check:open,settings-row:Import save,wait30,settings-check:import-count:1,shot:imported"
if [ "${P4PC_IMPORT_FOLDER_TEST:-0}" = 1 ]; then
  # Device harness stages exactly one genuine private container in files/import.
  # The C command refuses this operation on desktop, so no OS picker can open.
  script="$script,settings-import:folder,wait30,settings-check:import-count:2,shot:import_folder"
fi
script="$script,settings-key:Escape,wait90,settings-check:closed,shot:import_closed"
(cd "$work" && env SDL_VIDEODRIVER="${SDL_VIDEODRIVER:-offscreen}" SDL_AUDIODRIVER=dummy \
  P4PC_HIDDEN_WINDOW=1 P4PC_INPUT=script P4PC_CONFIG=none P4PC_SAVE_DIR="$wwork/card" \
  P4PC_PRESENT=blit P4PC_SETTINGS_TEST=1 P4PC_START_MENU=7:2 P4PC_MENU_EXIT=1 \
  P4PC_MENU_STEP=20 P4PC_TURBO=1 P4PC_MENU_SCRIPT="$script" \
  timeout -s KILL 540 "$prog" "$P4PC_ELF" > boot.log 2>&1)
rc=$?
if [ "$rc" -ne 0 ]; then
  echo "import_settings_native: game exited $rc"
  tail -n 30 "$work/boot.log"
  exit 1
fi
for check in open import-count:0 import-deferred import-count:1 closed; do
  if ! grep -Fq "native settings check $check: PASS" "$work/boot.log"; then
    echo "import_settings_native: missing $check witness"
    exit 1
  fi
done
if [ "${P4PC_IMPORT_FOLDER_TEST:-0}" = 1 ] &&
   ! grep -Fq 'native settings check import-count:2: PASS' "$work/boot.log"; then
  echo "import_settings_native: missing actual Android folder-action witness"
  exit 1
fi
echo "import_settings_native: PASS native row; cancel=no writes, closed=deferred, paused=imported"

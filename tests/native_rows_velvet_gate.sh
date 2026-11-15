#!/bin/sh
# A mod card in the real Velvet Room fusion card list (p4.ui.native_row,
# capability ui.native_row@1): the witness.ui-native-row fixture adds one card;
# the list shows it after the spreads, the cursor reaches it from the first card
# by wrapping left past Back, Talk and Forecast, Cross opens its page hosted in
# the Velvet task, and closing the page slides the cards back in with the cursor
# on it. Mod-witness lane (P4PC_MOD_WITNESS=1): never a retail reference.
# Level-99 Apollo save as in tests/settings_host_velvet_gate.sh.
# Usage: native_rows_velvet_gate.sh <p4pc> [out]
. "$(dirname "$0")/input_guard.sh"
prog=$1
if [ -z "$P4PC_ISO" ] || [ -z "$P4PC_ELF" ]; then
  echo "native_rows_velvet: P4PC_ISO or P4PC_ELF unset, skipped"
  exit 77
fi
saves=${P4PC_SAVES_DIR:-$(dirname "$P4PC_ELF")/saves}/card_apollo
if [ ! -d "$saves" ]; then
  echo "native_rows_velvet: no retail save in $saves, skipped"
  exit 77
fi
case $(uname -s) in MINGW*|MSYS*|CYGWIN*) video=windows ;; *) video=offscreen ;; esac
here=$(cd "$(dirname "$0")" && { pwd -W 2>/dev/null || pwd; })
tools=$(cd "$(dirname "$0")/../tools" && pwd) || exit 1
. "$(dirname "$0")/artifacts.sh"
gate_artifacts_begin p4pc_native_rows_velvet || exit 1
mkdir -p "$work/card/mc0" && cp -r "$saves"/. "$work/card/mc0/" || exit 1
cp -r "$here/mod_fixtures" "$work/mods" || exit 1
cd "$work" || exit 1
# The walk ends at the fusion root; Cross on Fuse Personas opens the card list.
script="velvet-root,settings-check:velvet-root,cross,wait150,settings-check:velvet-left-root,shot:cards"
script="$script,left,left,left,left,wait40,shot:card_selected"
script="$script,cross,wait150,shot:card_page,circle,wait180,settings-check:velvet-left-root,shot:cards_returned"
unset P4PC_RESOLUTION P4PC_SCALE P4PC_WINDOW_W P4PC_WINDOW_H P4PC_FULLSCREEN P4PC_SETTINGS_RETAIL_ONLY
env SDL_VIDEODRIVER="${SDL_VIDEODRIVER:-$video}" SDL_AUDIODRIVER=dummy P4PC_HIDDEN_WINDOW=1 \
  P4PC_MOD_WITNESS=1 P4PC_MODS=1 P4PC_PACKS=0 P4PC_INPUT=script \
  P4PC_MOD_DIR="$work/mods" P4PC_CONFIG="$work/mods/ui_native_row.ini" \
  P4PC_TRAP_CONTINUE=1 P4PC_TURBO=2 P4PC_AUTOPRESS_FRAMES=1 P4PC_AUTOPRESS_LOAD=0 \
  P4PC_SAVE_DIR="$work/card" P4PC_WALK_SCRIPT="$here/../tools/systems/apollo_velvet_root.txt" \
  P4PC_MENU_EXIT=1 P4PC_MENU_STEP=20 \
  P4PC_MENU_SCRIPT="$script" timeout -s KILL 1500 "$prog" "$P4PC_ELF" > boot.log 2>&1
rc=$?
if [ "$rc" -ne 0 ]; then
  echo "native_rows_velvet: exited $rc"
  grep -a -E 'native row|ROW_|menu harness: (frame|cannot)' boot.log | tail -n 30
  exit 1
fi
for marker in ROW_READY "native row velvet witness opened page" ROW_PAGE_OPEN ROW_PAGE_CLOSE \
    "native row velvet witness closed page" '4 shots, 0 dark, 0 failed writes'; do
  if ! grep -aFq "$marker" boot.log; then
    echo "native_rows_velvet: missing '$marker'"
    exit 1
  fi
done
if ! "${P4PC_PYTHON:-python3}" - "$tools" <<'PY'
import sys
sys.path.insert(0, sys.argv[1])
from png_diff import read_png
def mad(a, b):
    (w0, h0, c0, p0), (w1, h1, c1, p1) = read_png(a), read_png(b)
    if (w0, h0, c0) != (w1, h1, c1):
        raise SystemExit("capture sizes differ: %s %s" % (a, b))
    return 100.0 * sum(abs(x - y) for x, y in zip(p0, p1)) / (255.0 * len(p0))
# The page was presented over the list, and the list came back with the card selected.
for a, b in (("menu_cards.png", "menu_card_page.png"), ("menu_card_page.png", "menu_cards_returned.png")):
    if mad(a, b) < 0.5:
        raise SystemExit("no visible change between %s and %s" % (a, b))
print("native_rows_velvet: selected vs returned list MAD %.3f%%" % mad("menu_card_selected.png", "menu_cards_returned.png"))
PY
then
  echo "native_rows_velvet: capture comparison failed"
  exit 1
fi
if [ -n "${2:-}" ]; then
  mkdir -p "$2" && cp menu_*.png boot.log "$2/" || exit 1
fi
echo "native_rows_velvet: PASS mod card listed, selected, page hosted and closed, list returned"

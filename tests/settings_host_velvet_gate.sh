#!/bin/sh
# A registered native page flow hosted by the real Velvet Room fusion menu
# task: a Camp-mode child is refused, then its title-mode cmpConfigCreate
# child runs list -> detail -> back -> close, nLine is off again, and the
# untouched fusion root takes ordinary input. Mod-witness lane
# (P4PC_MOD_WITNESS=1): never a retail reference. Level-99 Apollo save as in
# tests/systems_gate.sh. Usage: settings_host_velvet_gate.sh <p4pc> [out root]
# An output root retains the captures and log under <out root>/<p4pc name>.
. "$(dirname "$0")/input_guard.sh"
prog=$1
out=${2:+$2/$(basename "$prog")}
if [ -z "$P4PC_ISO" ] || [ -z "$P4PC_ELF" ]; then
  echo "settings_host_velvet: P4PC_ISO or P4PC_ELF unset, skipped"
  exit 77
fi
saves=${P4PC_SAVES_DIR:-$(dirname "$P4PC_ELF")/saves}/card_apollo
if [ ! -d "$saves" ]; then
  echo "settings_host_velvet: no retail save in $saves, skipped"
  exit 77
fi
case $(uname -s) in MINGW*|MSYS*|CYGWIN*) video=windows ;; *) video=offscreen ;; esac
here=$(cd "$(dirname "$0")" && { pwd -W 2>/dev/null || pwd; })
tools=$(cd "$(dirname "$0")/../tools" && pwd) || exit 1
. "$(dirname "$0")/artifacts.sh"
gate_artifacts_begin p4pc_settings_host_velvet || exit 1
mkdir -p "$work/card/mc0" && cp -r "$saves"/. "$work/card/mc0/" || exit 1
cd "$work" || exit 1
script="velvet-root,settings-check:velvet-root,settings-check:nline-off,shot:host_before"
script="$script,settings-host-refused:0:test.pages,settings-host:1:test.pages,wait120,settings-check:page-home,shot:host_page"
script="$script,settings-touch:535:198,wait30,settings-touch:535:198,wait90,settings-check:page-detail,shot:host_detail"
script="$script,settings-key:Escape,wait90,settings-check:page-home,settings-key:Escape,settings-host-done,wait60"
script="$script,settings-check:host-returned,settings-check:closed,settings-check:velvet-root,settings-check:nline-off,shot:host_after"
script="$script,cross,wait120,settings-check:velvet-left-root,shot:host_resumed"
unset P4PC_RESOLUTION P4PC_SCALE P4PC_WINDOW_W P4PC_WINDOW_H P4PC_FULLSCREEN P4PC_SETTINGS_RETAIL_ONLY
env SDL_VIDEODRIVER="${SDL_VIDEODRIVER:-$video}" SDL_AUDIODRIVER=dummy P4PC_HIDDEN_WINDOW=1 \
  P4PC_MOD_WITNESS=1 P4PC_MODS=0 P4PC_PACKS=0 P4PC_CONFIG=none \
  P4PC_TRAP_CONTINUE=1 P4PC_TURBO=2 P4PC_AUTOPRESS_FRAMES=1 P4PC_AUTOPRESS_LOAD=0 \
  P4PC_SAVE_DIR="$work/card" P4PC_WALK_SCRIPT="$here/../tools/systems/apollo_velvet_root.txt" \
  P4PC_SETTINGS_TEST=1 P4PC_SETTINGS_PAGES_TEST=1 P4PC_MENU_EXIT=1 P4PC_MENU_STEP=20 \
  P4PC_MENU_SCRIPT="$script" timeout -s KILL 1300 "$prog" "$P4PC_ELF" > boot.log 2>&1
rc=$?
if [ "$rc" -ne 0 ]; then
  echo "settings_host_velvet: p4pc exited $rc"
  grep -a -E 'native Config|menu harness: (frame|native settings|fusion|cannot|hosted|refused|Config)' boot.log | tail -n 30
  exit 1
fi
for check in velvet-root nline-off page-home page-detail host-returned closed velvet-left-root; do
  if ! grep -Fq "native settings check $check: PASS" boot.log; then
    echo "settings_host_velvet: missing $check witness"
    exit 1
  fi
done
if ! grep -Fq '5 shots, 0 dark, 0 failed writes' boot.log; then
  echo "settings_host_velvet: missing or dark native capture"
  exit 1
fi
# The returned root must be the same root: compare before/after captures.
if ! "${P4PC_PYTHON:-python3}" - "$tools" <<'PY'
import sys
sys.path.insert(0, sys.argv[1])
from png_diff import read_png
w0, h0, c0, before = read_png("menu_host_before.png")
w1, h1, c1, after = read_png("menu_host_after.png")
if (w0, h0, c0) != (w1, h1, c1):
    raise SystemExit("returned root changed size")
total = sum(abs(a - b) for a, b in zip(before, after))
mad = 100.0 * total / (255.0 * len(before))
print("settings_host_velvet: root before/after MAD %.3f%%" % mad)
if mad > 2.0:
    raise SystemExit("returned Velvet root differs from the pre-host root")
for name in ("menu_host_page.png", "menu_host_detail.png"):
    if open(name, "rb").read() == open("menu_host_before.png", "rb").read():
        raise SystemExit("hosted page was not presented: " + name)
PY
then
  echo "settings_host_velvet: capture comparison failed"
  exit 1
fi
if [ -n "$out" ]; then
  mkdir -p "$out" && cp menu_host_*.png boot.log "$out/" || exit 1
fi
echo "settings_host_velvet: PASS Velvet fusion task: Camp mode refused, title mode hosted list/detail/back/close, nLine restored, clean root return and resumed input"

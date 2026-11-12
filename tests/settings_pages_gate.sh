#!/bin/sh
# The production native Config, virtual providers and real stock font/widgets.
. "$(dirname "$0")/input_guard.sh"
if [ -z "$P4PC_ISO" ] || [ -z "$P4PC_ELF" ]; then
  echo "settings_pages: P4PC_ISO or P4PC_ELF unset, skipped"
  exit 77
fi
prog=$1
script=$(cat "$(dirname "$0")/settings_pages.script") || exit 1
tools=$(cd "$(dirname "$0")/../tools" && pwd) || exit 1
. "$(dirname "$0")/artifacts.sh"
gate_artifacts_begin p4pc_settings_pages || exit 1
cd "$work" || exit 1
mkdir -p save || exit 1
cat > settings.ini <<'INI'
[video]
present = blit
width = 1280
height = 960
resolution = scale:1
aspect = 4:3
INI
unset P4PC_RESOLUTION P4PC_SCALE P4PC_WINDOW_W P4PC_WINDOW_H P4PC_FULLSCREEN
unset P4PC_PRESENT_FIT P4PC_ASPECT P4PC_MOVIE_FIT P4PC_PRESENT P4PC_VSYNC P4PC_SETTINGS_RETAIL_ONLY
env SDL_VIDEODRIVER="${SDL_VIDEODRIVER:-offscreen}" SDL_AUDIODRIVER=dummy \
  P4PC_HIDDEN_WINDOW=1 P4PC_INPUT=script P4PC_CONFIG=settings.ini P4PC_SAVE_DIR="$work/save" \
  P4PC_SETTINGS_TEST=1 P4PC_SETTINGS_PAGES_TEST=1 P4PC_START_MENU=7:2 \
  P4PC_MENU_EXIT=1 P4PC_MENU_STEP=20 P4PC_TURBO=2 P4PC_MENU_SCRIPT="$script" \
  timeout -s KILL 540 "$prog" "$P4PC_ELF" > boot.log 2>&1
rc=$?
if [ "$rc" -ne 0 ]; then
  echo "settings_pages: game exited $rc"
  tail -n 40 boot.log
  exit 1
fi
for check in home toggle last-toggle catalog-last catalog-identity detail detail-paged choice depth unregister-callback unregister-deep empty unregister-gone error error-glyphs closed; do
  if ! grep -Fq "native settings check page-$check: PASS" boot.log; then
    echo "settings_pages: missing $check witness"
    exit 1
  fi
done
if ! grep -Fq '14 shots, 0 dark, 0 failed writes' boot.log; then
  echo "settings_pages: missing or stale native capture"
  exit 1
fi
# Python is already a build/test prerequisite. Do not treat a missing external
# comparison program (exit 127) as evidence that two captures differ.
if ! "${P4PC_PYTHON:-python3}" - "$tools" <<'PY'
from pathlib import Path
import sys
sys.path.insert(0, sys.argv[1])
from png_diff import read_png
for left, right in (
    ("menu_pages_home.png", "menu_pages_toggle.png"),
    ("menu_pages_catalog.png", "menu_pages_detail.png"),
    ("menu_pages_detail.png", "menu_pages_detail_paged.png"),
    ("menu_pages_home.png", "menu_pages_choice.png"),
):
    if Path(left).read_bytes() == Path(right).read_bytes():
        raise SystemExit("identical native captures: " + left + " / " + right)
width, height, channels, pixels = read_png("menu_pages_detail.png")
for x in (592, 596):
    offset = ((149 * height // 448) * width + x * width // 640) * channels
    if max(pixels[offset:offset + 3]) >= 96:
        raise SystemExit("native INFO selection strip stops before its right cap")
PY
then
  echo "settings_pages: native capture comparison failed"
  exit 1
fi
if [ -n "${2:-}" ]; then
  mkdir -p "$2" && cp menu_pages_*.png settings.ini boot.log "$2/" || exit 1
fi
echo "settings_pages: PASS 37 toggles, million-row virtual catalog, stable revision, choice commit, measured detail/help paging, 16-frame history, callback fence, synchronous unregister, provider error, normal close"

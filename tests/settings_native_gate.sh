#!/bin/sh
# Real game Config: a refused shortcut expires; the F1 shortcut opens the native task;
# native pad/list input changes resolution and starts press-to-bind; Esc closes.
# An optional absolute second argument retains native Config PNGs, INI and log for visual qualification.
. "$(dirname "$0")/input_guard.sh"
# This witness edits its private INI; inherited video overrides must not mask it.
unset P4PC_RESOLUTION P4PC_SCALE P4PC_WINDOW_W P4PC_WINDOW_H P4PC_FULLSCREEN
unset P4PC_PRESENT_FIT P4PC_ASPECT P4PC_MOVIE_FIT P4PC_PRESENT P4PC_VSYNC P4PC_SETTINGS_RETAIL_ONLY
if [ -z "$P4PC_ISO" ] || [ -z "$P4PC_ELF" ]; then
  echo "settings_native: P4PC_ISO or P4PC_ELF unset, skipped"
  exit 77
fi
prog=$1
. "$(dirname "$0")/artifacts.sh"
gate_artifacts_begin p4pc_settings_native || exit 1
cd "$work" || exit 1
# The counter passes an extended-length root to its callback on Windows.
# Child joins must be normalized too; an unrelated directory is not a save.
mkdir -p card/mc0/BASLUS-21782Save2 card/mc0/unrelated || exit 1
wwork=$(cygpath -m "$work" 2>/dev/null || echo "$work")
cat > settings.ini <<'INI'
# Keep this comment
[video]
present = blit
fit = 4:3
aspect = 4:3
movie_fit = bars
width = 1280
height = 960
resolution = scale:1 ; keep inline
unknown = untouched
[future]
key = value
INI
# The fresh room starts with its real Help event. A shortcut must not dismiss
# that event, nor replay after Cross closes it.
script='settings-open,wait60,settings-check:closed,cross,wait90,settings-check:closed,settings-block:1,settings-open,wait60,settings-check:closed,settings-block:0,wait60,settings-check:closed,settings-open,wait240,settings-check:open,settings-check:platform,shot:settings_native'
script="$script,settings-check:save-count:1,settings-check:draw-skipped,settings-row:Resolution,up,settings-touch-row:Resolution,wait30,settings-check:scale1,settings-touch-row:Resolution,wait30,settings-check:scale2,shot:settings_resolution"
script="$script,settings-row:Controls,shot:settings_pcrows,cross,wait90,settings-row:Key cross (1)"
script="$script,cross,settings-key:K,wait30,settings-check:binding,shot:settings_controls,settings-row:Key circle (1),cross,settings-key:K,wait30,shot:settings_conflict,settings-key:Escape,settings-check:binding,settings-check:open,settings-key:Escape,wait90"
script="$script,settings-row:Aspect ratio,right,wait20,settings-check:aspect169,shot:settings_aspect"
script="$script,right,wait20,settings-check:aspectauto,right,wait20,settings-check:aspect43"
script="$script,settings-row:Movie fit,settings-check:movie-bars,shot:settings_movie_bars"
script="$script,right,wait20,settings-check:movie-zoom,shot:settings_movie_zoom"
script="$script,right,wait20,settings-check:movie-stretch,shot:settings_movie_stretch"
script="$script,settings-key:Escape,wait90,settings-check:closed,settings-check:movie-persisted,shot:settings_closed"
script="$script,settings-check:persisted,totitle,settings-title-ready,settings-open,wait240,settings-check:title,shot:settings_title,settings-key:Escape,wait90,settings-check:closed"
env SDL_VIDEODRIVER="${SDL_VIDEODRIVER:-offscreen}" SDL_AUDIODRIVER=dummy \
  P4PC_HIDDEN_WINDOW=1 P4PC_INPUT=script P4PC_CONFIG=settings.ini P4PC_SAVE_DIR="$wwork/card" \
  P4PC_SETTINGS_TEST=1 P4PC_START_MENU=7:2 P4PC_MENU_EXIT=1 \
  P4PC_MENU_STEP=20 P4PC_TURBO=2 P4PC_MENU_SCRIPT="$script" \
  timeout -s KILL 540 "$prog" "$P4PC_ELF" > boot.log 2>&1
rc=$?
if [ "$rc" -ne 0 ]; then
  echo "settings_native: game exited $rc"
  tail -n 30 boot.log
  exit 1
fi
# These row captures follow a retrying list-navigation token, not a fixed wait.
# With skipped draws they must show the new selection, not a stale lit frame.
if ! "${P4PC_PYTHON:-python3}" - <<'PY'
from pathlib import Path
for left, right in (
    ("menu_settings_resolution.png", "menu_settings_pcrows.png"),
    ("menu_settings_aspect.png", "menu_settings_movie_bars.png"),
):
    if Path(left).read_bytes() == Path(right).read_bytes():
        raise SystemExit("identical native captures: " + left + " / " + right)
PY
then
  echo "settings_native: native capture comparison failed"
  exit 1
fi
for check in open platform save-count:1 draw-skipped scale1 scale2 binding aspect169 aspectauto aspect43 movie-bars movie-zoom movie-stretch movie-persisted persisted title closed; do
  if ! grep -Fq "native settings check $check: PASS" boot.log; then
    echo "settings_native: missing live $check witness"
    exit 1
  fi
done
if ! grep -Eq '^[[:space:]]*resolution[[:space:]]*=[[:space:]]*scale:2[[:space:]]*; keep inline' settings.ini ||
   ! grep -Eq '^[[:space:]]*aspect[[:space:]]*=[[:space:]]*4:3' settings.ini ||
   ! grep -Eq '^[[:space:]]*movie_fit[[:space:]]*=[[:space:]]*stretch' settings.ini ||
   ! grep -Eq '^[[:space:]]*kb\.cross[[:space:]]*=[[:space:]]*K, none' settings.ini ||
   ! grep -Fq '# Keep this comment' settings.ini ||
   ! grep -Fq 'unknown = untouched' settings.ini ||
   ! grep -Fq '[future]' settings.ini || ! grep -Fq 'key = value' settings.ini ||
   [ -e settings.ini.tmp ]; then
  echo "settings_native: atomic INI persistence or preserved content failed"
  exit 1
fi
if [ -n "${2:-}" ]; then
  mkdir -p "$2" && cp menu_settings_*.png settings.ini boot.log "$2/" || exit 1
fi
echo "settings_native: PASS native task, live scale:2 1280x896, live K binding, conflict cancel, restart-only aspect, live/persisted Movie fit bars/zoom/stretch, rejected shortcut, atomic INI"

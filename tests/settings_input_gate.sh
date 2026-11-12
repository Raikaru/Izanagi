#!/bin/sh
# Real native Config with SDL's virtual controller and tagged mouse events.
. "$(dirname "$0")/input_guard.sh"
if [ -z "$P4PC_ISO" ] || [ -z "$P4PC_ELF" ]; then
  echo 'settings_input: P4PC_ISO or P4PC_ELF unset, skipped'
  exit 77
fi
prog=$1
script=$(cat "$(dirname "$0")/settings_input.script") || exit 1
. "$(dirname "$0")/artifacts.sh"
gate_artifacts_begin p4pc_settings_input || exit 1
cd "$work" || exit 1
mkdir -p save mods/example packs/example || exit 1
cat > mods/example/mod.ini <<'INI'
[mod]
id=input.mod
name=Input fixture
version=1
api=1.0
priority=0
order=0
INI
cat > packs/example/pack.ini <<'INI'
[pack]
id=input.pack
name=Input fixture
version=1
api=1.0
priority=0
order=0
INI
unset P4PC_RESOLUTION P4PC_SCALE P4PC_WINDOW_W P4PC_WINDOW_H P4PC_FULLSCREEN
unset P4PC_PRESENT_FIT P4PC_ASPECT P4PC_MOVIE_FIT P4PC_PRESENT P4PC_VSYNC P4PC_SETTINGS_RETAIL_ONLY
unset P4PC_MOD_DIR P4PC_PACK_DIR P4PC_MOD_WITNESS
for fullscreen in 0 1; do
  cat > settings.ini <<INI
# preserved input witness
[video]
present = blit
fit = 4:3
width = 1300
height = 900
fullscreen = $fullscreen
resolution = scale:1 ; preserve comment
[future]
unknown = untouched
INI
  env SDL_VIDEODRIVER="${SDL_VIDEODRIVER:-offscreen}" SDL_AUDIODRIVER=dummy \
    P4PC_HIDDEN_WINDOW=1 P4PC_INPUT=virtual P4PC_PAD_VIRTUAL=0,0,0,0,0,0 \
    P4PC_MODS=0 P4PC_PACKS=0 P4PC_CONFIG="$work/settings.ini" P4PC_SAVE_DIR="$work/save" \
    P4PC_SETTINGS_TEST=1 P4PC_SETTINGS_VIRTUAL_TEST=1 P4PC_SETTINGS_PAGES_TEST=1 \
    P4PC_SETTINGS_EXPECT_FULLSCREEN="$fullscreen" \
    P4PC_START_MENU=7:2 P4PC_MENU_EXIT=1 P4PC_MENU_STEP=20 P4PC_TURBO=2 \
    P4PC_MENU_SCRIPT="$script" timeout -s KILL 540 "$prog" "$P4PC_ELF" > "boot-$fullscreen.log" 2>&1
  rc=$?
  if [ "$rc" -ne 0 ]; then echo "settings_input: fullscreen=$fullscreen exit=$rc"; exit 1; fi
  for check in window-mode cursor-visible cursor-hidden cursor-restored cursor-repeat row:Resolution scale2 scale1 row:Controls page-home page-toggle row:Page\ substrate active-page:mods active-page:mods/installation active-page:mods/details closed; do
    if ! grep -Fq "native settings check $check: PASS" "boot-$fullscreen.log"; then
      echo "settings_input: missing $check witness fullscreen=$fullscreen"; exit 1
    fi
  done
  if ! grep -Fq 'resolution = scale:1 ; preserve comment' settings.ini ||
     ! grep -Fq 'unknown = untouched' settings.ini ||
     ! grep -Fq 'enabled = input.mod' settings.ini; then
    echo "settings_input: missing persisted INI witness fullscreen=$fullscreen"; exit 1
  fi
  if [ -n "${2:-}" ]; then
    mkdir -p "$2/$fullscreen" && cp menu_input_*.png settings.ini "boot-$fullscreen.log" "$2/$fullscreen/" || exit 1
  fi
done
echo 'settings_input: PASS real virtual-pad navigation, menu focus/hover, live resolution, native pages and package selectors; windowed/fullscreen'

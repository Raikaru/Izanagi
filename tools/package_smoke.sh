#!/bin/sh
# Smoke test of a package made by tools/package_win.sh, on this machine, the way a player starts it (p4pc64w.exe with no arguments, the disc named in a
# settings file, nothing retail beside it), but hidden and silent with scripted input.
# P4PC_INPUT=script is pinned after inherited/run-specific settings.
# P4PC_HIDDEN_WINDOW=1 and SDL_AUDIODRIVER=dummy protect the desktop. Three runs:
#   paced   boot, the title and New Game for 3600 flips at the real 60 Hz (30 presents a second: the game flips every second 60 Hz tick as on the PS2; the wall time, 90 s or more on a loaded machine, says it is paced)
#   story   New Game with the facing gate (P4PC_FACING_GATE: walkers face their way, Yu/Dojima/Nanako in the Inaba greeting), the opening, the fog-dream tutorial battle (the battle driver), 04/12 to 04/15 and the first save at the calendar into a fresh memory card
#           (tests/story_gate.sh --make-save on the packaged exe; P4PC_TURBO=1: the virtual clock, every frame drawn on the GPU)
#   load    the save of that run loaded through the title, and the game is where it was saved (field 7 room 2 on 04/15)
#   P4PC_ISO=<your disc image> sh tools/package_smoke.sh <package.zip> [work dir]      (default work: <zip folder>/<name>-smoke)
# Runs under Git Bash on Windows. Nothing retail is copied: the settings file names your own disc image.
set -eu
# the git-bash sh this runs under has sed and awk but no grep on its PATH: lines <file> <extended regex>, has <file> <extended regex>
lines() { awk -v re="$2" '$0 ~ re' "$1"; }
has() { awk -v re="$2" 'BEGIN { r = 1 } $0 ~ re { r = 0; exit } END { exit r }' "$1"; }
root=$(cd "$(dirname "$0")/.." && pwd)
zip=${1:?usage: P4PC_ISO=<disc image> sh tools/package_smoke.sh <package.zip> [work dir]}
iso=${P4PC_ISO:?set P4PC_ISO to your own Persona 4 (USA) disc image}
name=$(basename "$zip" .zip)
work=$(cygpath -am "${2:-$(dirname "$zip")/$name-smoke}")
mkdir -p "$work/tmp"
export TMPDIR="$work/tmp" TMP="$work/tmp" TEMP="$work/tmp"
rm -rf "$work/$name"
powershell -NoProfile -Command "Expand-Archive -Path '$(cygpath -w "$zip")' -DestinationPath '$(cygpath -w "$work")' -Force"
pkg=$work/$name
exe=p4pc64w.exe; [ -f "$pkg/p4pc64w.exe" ] || exe=p4pc.exe
[ -f "$pkg/$exe" ] || { echo "smoke: no exe in $pkg" >&2; exit 1; }
ini=$(cygpath -w "$work/$name-p4pc.ini")
printf '[game]\niso = %s\n' "$(cygpath -w "$iso")" > "$work/$name-p4pc.ini"
walks=$root/tests/walks
ok=1

# run <tag> <timeout> <env...>: the exe from the package folder, run directory $work/<tag>, log boot.log
run() {
  tag=$1; to=$2; shift 2
  rm -rf "$work/$tag"; mkdir -p "$work/$tag"
  start=$(date +%s)
  ( cd "$work/$tag" && env P4PC_CONFIG="$ini" P4PC_HIDDEN_WINDOW=1 SDL_AUDIODRIVER=dummy "$@" P4PC_INPUT=script timeout "$to" "$pkg/$exe" > boot.log 2>&1 ) && rc=0 || rc=$?
  secs=$(( $(date +%s) - start ))
  echo "smoke: $tag: exit $rc in ${secs}s ($(wc -l < "$work/$tag/boot.log") log lines)"
  eval "${tag}_rc=$rc ${tag}_secs=$secs"
}

# 1. paced: no virtual clock, no turbo
run paced 260 P4PC_SAVE_DIR="$(cygpath -w "$work/paced/card")" P4PC_TRAP_CONTINUE=1 P4PC_AUTOPRESS_FRAMES=1 P4PC_AUTOPRESS_NEWGAME=1560 \
  P4PC_AUTOPRESS='cross@2300:2/150*20,start@6500:2/300*14' P4PC_NEWGAME_GATE=3600,-1:-1
lines "$work/paced/boot.log" "new-game gate|p4pc: read|CRASH|p4pc turbo" | tail -n 4
if has "$work/paced/boot.log" "new-game gate .* at flip 3600" && [ "$paced_secs" -ge 90 ]; then
  echo "smoke: paced: 3600 flips in ${paced_secs}s ($((3600 / paced_secs)) presents a second: the game flips every second vblank, 30 a second, as on the PS2): real-time pacing holds (faster would mean unpaced)"
else
  echo "smoke: paced: FAILED (flip 3600 not reached, or not at real time: ${paced_secs}s for 3600 flips, expected 90 s or more at the game's 30 presents a second)"; ok=0
fi

# 2. story + first save (the same script as tests/story_gate.sh --make-save)
{ cat "$walks/newgame_to_0413.walk"
  printf '%s\n' "# 04/15 evening, free in the living room: the calendar, Save, slot 1" "timeout:3000" "walk:103,-217,25" "wait:30" "press:cross" "wait:300" "press:cross" \
      "wait:300" "press:cross" "wait:300" "press:cross" "wait:900"; } > "$work/make_save.walk"
run story 1700 P4PC_SAVE_DIR="$(cygpath -w "$work/story/card")" P4PC_TRAP_CONTINUE=1 P4PC_FACING_GATE=1 P4PC_TURBO=1 P4PC_AUTOPRESS_FRAMES=1 P4PC_AUTOPRESS_NEWGAME=1560 \
  P4PC_AUTOPRESS='cross@2300:2/150*20,start@6500:2/300*14' P4PC_WALK_SCRIPT="$(cygpath -w "$work/make_save.walk")" \
  P4PC_BATTLE_FIRST=down,cross,cross,cross P4PC_BATTLE_DRIVER=cross,cross,cross P4PC_BATTLE_SKIP=140 P4PC_NEWGAME_GATE=160000,7:2,4/15
lines "$work/story/boot.log" "new-game gate|facing gate|walk script done|WALK SCRIPT|btlMain state 8 ->|CRASH" | tail -n 7
card=$work/story/card/mc0/BASLUS-21782Save0/BASLUS-21782Save0
if [ "$story_rc" -eq 0 ] && has "$work/story/boot.log" "new-game gate PASS" && has "$work/story/boot.log" "retail poses seen .Inaba greeting, 04/12 breakfast.: 7 of 7" && [ -f "$card" ] && [ "$(wc -c < "$card")" -eq 171112 ]; then
  echo "smoke: story: New Game through the fog battle to 04/15, saved: $card (171112 bytes); all seven Inaba greeting and breakfast retail poses passed (P4PC_FACING_GATE)"
else
  echo "smoke: story: FAILED"; ok=0
fi

# 3. load the save through the title
if [ -f "$card" ]; then
  printf '%s\n' "timeout:90000" "wait:140" "press:cross" "wait:140" "press:cross" "wait:140" "press:cross" "skipmsg" "wait:60" > "$work/load.walk"
  rm -rf "$work/load-card"; mkdir -p "$work/load-card/mc0"; cp -r "$work/story/card/mc0/." "$work/load-card/mc0/"
  run load 300 P4PC_SAVE_DIR="$(cygpath -w "$work/load-card")" P4PC_TRAP_CONTINUE=1 P4PC_TURBO=1 P4PC_AUTOPRESS_FRAMES=1 P4PC_AUTOPRESS_LOAD=0 \
    P4PC_WALK_SCRIPT="$(cygpath -w "$work/load.walk")" P4PC_NEWGAME_GATE=9000,7:2,4/15
  lines "$work/load/boot.log" "new-game gate|CRASH" | tail -n 2
  if [ "$load_rc" -eq 0 ] && has "$work/load/boot.log" "new-game gate PASS"; then echo "smoke: load: the save loads into the living room on 04/15"; else echo "smoke: load: FAILED"; ok=0; fi
fi
[ $ok = 1 ] && echo "smoke: PASS ($name)" || { echo "smoke: FAIL ($name)"; exit 1; }

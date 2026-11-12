#!/bin/sh
# Renderer regression gate for a battle frame: boots the engine as the real boot does, enters an encounter through the battle harness
# (src/platform/battle_harness.c). Keep the original scripted presses and fixed
# capture flip, comparing RGB against the pinned captured-target-sized raw reference.
# Exits 77 without P4PC_ISO or P4PC_ELF; missing, unpinned or changed references fail.
# P4PC_TURBO=2: draws only within P4PC_TURBO_WINDOW (default 4; 1 already gives the full-render frame byte for byte) flips of the captured frame.
# usage: battle_mad.sh <p4pc> <encounter> <reference name> <capture flip> <max MAD percent> <P4PC_AUTOPRESS value> [VAR=value ...]
. "$(dirname "$0")/input_guard.sh"

if [ -z "$P4PC_ISO" ] || [ -z "$P4PC_ELF" ]; then
  echo "battle_mad: P4PC_ISO or P4PC_ELF unset, skipped"
  exit 77
fi
prog=$1 enc=$2 name=$3 flip=$4 limit=$5 press=$6
shift 6
ref=${P4PC_REF_DIR:-$(dirname "$P4PC_ISO")/refs}/$name.rgb
sh "$(dirname "$0")/../tools/ref_guard.sh" --file "$ref" || exit 1
seam_check=$(CDPATH= cd -- "$(dirname "$0")" && pwd)/fog_seam.py
. "$(dirname "$0")/artifacts.sh"
gate_artifacts_begin p4pc_battle_mad || exit 1
cd "$work" || exit 1
env P4PC_TURBO=2 P4PC_TURBO_WINDOW="${P4PC_TURBO_WINDOW:-4}" "$@" SDL_VIDEODRIVER="${SDL_VIDEODRIVER:-offscreen}" SDL_AUDIODRIVER=dummy P4PC_TRAP_CONTINUE=1 P4PC_START_BATTLE="$enc" P4PC_AUTOPRESS_FRAMES=1 \
  P4PC_AUTOPRESS="$press" P4PC_CAPTURE_FRAMES="$flip" P4PC_CAPTURE_PNG="$work/frame_%d.png" P4PC_CAPTURE_REF="$ref" \
  P4PC_CAPTURE_REF_MAX="$limit" timeout -s KILL 280 "$prog" "$P4PC_ELF" > "$work/boot.log" 2>&1
rc=$?
grep -a "mean abs diff\|FAIL" "$work/boot.log" | tail -n 4
if [ $rc -ne 0 ]; then
  echo "battle_mad: p4pc exited $rc"
  tail -n 12 "$work/boot.log"
fi
if [ "$rc" -eq 0 ] && { [ "$name" = p4-fogbattle ] || [ "${P4PC_FOG_SEAM:-0}" = 1 ]; }; then
  for image in "$work"/frame_*.png; do
    python3 "$seam_check" "$image" || rc=$?
  done
fi
exit $rc

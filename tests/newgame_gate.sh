#!/bin/sh
# New Game to the first controllable field, scripted and unattended: boots p4pc with P4PC_TURBO=2 (no 60 Hz pacing; the game still
# sees one VSync per presented frame, so the frame-scheduled P4PC_AUTOPRESS_FRAMES script means the same on any host), answers the
# difficulty prompt, types the name, plays the opening events (Velvet Room, Inaba, the Dojima house) and requires that at the gate
# flip the game is in the living room (field 7 room 2) and the frame captured one flip earlier is within the limit of the reference
# refs/newgame_field.rgb (a raw 640x448 RGB frame beside the retail image, never committed; `newgame_gate.sh --make-ref` writes it
# from the current build, the way refs of the other gates that have no PCSX2 frame are made). src/platform/newgame_gate.c does the check.
#   usage: newgame_gate.sh <p4pc> [limit percent] [gate flip] | newgame_gate.sh --make-ref <p4pc> [gate flip]
# Skipped (77) without ISO or ELF; missing, unpinned or changed references fail.
. "$(dirname "$0")/input_guard.sh"

make=0
if [ "$1" = "--make-ref" ]; then make=1; shift; fi
prog=$1
if [ $make = 1 ]; then gate=${2:-30500}; limit=; else limit=${2:-5.0}; gate=${3:-30500}; fi
if [ -z "$P4PC_ISO" ] || [ -z "$P4PC_ELF" ]; then
    echo "newgame_gate: P4PC_ISO or P4PC_ELF unset, skipped"
    exit 77
fi
ref=${P4PC_REF_DIR:-$(dirname "$P4PC_ISO")/refs}/newgame_field.rgb
if [ $make = 0 ]; then
    sh "$(dirname "$0")/../tools/ref_guard.sh" --file "$ref" || exit 1
elif [ -e "$ref" ]; then
    echo "ref_guard: refusing to overwrite $ref; generate into a fresh P4PC_REF_DIR, then stage a new filename for the cutover" >&2
    exit 1
fi
. "$(dirname "$0")/artifacts.sh"
gate_artifacts_begin p4pc_newgame_gate || exit 1
cd "$work" || exit 1
script='cross@2300:2/150,start@6500:2/300'
if [ $make = 1 ]; then
    mkdir -p "$(dirname "$ref")"
    env SDL_VIDEODRIVER="${SDL_VIDEODRIVER:-offscreen}" SDL_AUDIODRIVER=dummy P4PC_TRAP_CONTINUE=1 P4PC_TURBO=2 P4PC_FACING_GATE=1 P4PC_AUTOPRESS_FRAMES=1 P4PC_AUTOPRESS_NEWGAME=1560 P4PC_AUTOPRESS="$script" \
        P4PC_CAPTURE_FRAMES=$((gate - 1)) P4PC_CAPTURE_PNG="$work/gate_%d.png" P4PC_CAPTURE_RGB="$ref" \
        P4PC_NEWGAME_GATE="$gate,7:2" timeout -s KILL 1500 "$prog" "$P4PC_ELF" > "$work/boot.log" 2>&1
else
    env SDL_VIDEODRIVER="${SDL_VIDEODRIVER:-offscreen}" SDL_AUDIODRIVER=dummy P4PC_TRAP_CONTINUE=1 P4PC_TURBO=2 P4PC_FACING_GATE=1 P4PC_AUTOPRESS_FRAMES=1 P4PC_AUTOPRESS_NEWGAME=1560 P4PC_AUTOPRESS="$script" \
        P4PC_CAPTURE_FRAMES=$((gate - 1)) P4PC_CAPTURE_PNG="$work/gate_%d.png" P4PC_REF_RGB="$ref" P4PC_REF_MAX="$limit" \
        P4PC_NEWGAME_GATE="$gate,7:2" timeout -s KILL 1500 "$prog" "$P4PC_ELF" > "$work/boot.log" 2>&1
fi
rc=$?
grep -a "new-game gate\|p4pc turbo:" "$work/boot.log" | tail -n 4
if [ $rc -ne 0 ]; then
    echo "newgame_gate: p4pc exited $rc"
    tail -n 20 "$work/boot.log"
fi
exit $rc

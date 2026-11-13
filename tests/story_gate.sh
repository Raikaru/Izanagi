#!/bin/sh
# Story progress from New Game, scripted and unattended: the title driver (P4PC_AUTOPRESS_NEWGAME) starts a game, the waypoint walker
# (tests/walks/newgame_to_0413.walk, the file format retail's p4hook --walk runs too) plays the opening events, the stairs and the futon
# of the first night, the fog dream (its door and the scripted tutorial fight), 04/12 (school), 04/13 (Junes, the Midnight Channel), 04/14 (the first
# TV entry with Teddie) and 04/15 (the Persona awakening, the first fight, Shadow Yosuke, the second night). The
# standalone battle driver uses Zio. The process exits 0 at the gate flip only in the
# Dojima living room (field 7 room 2) on 04/15 (datGetDateIndex), 3 if the walk leaves its path.
#   usage: story_gate.sh <p4pc> [gate flip] [field:room,month/day] [retail reference name] [MAD limit] [walk file] | story_gate.sh --make-save <p4pc>
#   P4PC_GATE_BOOT_LOG=<absolute file>: retain the application's complete log before scratch cleanup.
#   --make-save plays the same walk, then saves at the calendar (the living room's `save_point` area, x 19..188 z -362..-72) into slot 1 of a fresh card
#   and copies it to $P4PC_SAVE_OUT (default <retail dir>/saves/story0415, never committed): the checkpoint of ctest street_0417 (save_play_gate.sh).
# P4PC_TURBO=2: no pacing, no draw outside capture windows (about 2 minutes). Skipped (exit 77) without P4PC_ISO and P4PC_ELF.
. "$(dirname "$0")/input_guard.sh"

make=0
if [ "$1" = "--make-save" ]; then make=1; shift; fi
prog=$1
gate=${2:-140000}
want=${3:-7:2,4/15}
if [ $make = 1 ]; then gate=160000; want=7:2,4/15; fi
if [ -z "$P4PC_ISO" ] || [ -z "$P4PC_ELF" ]; then
    echo "story_gate: P4PC_ISO or P4PC_ELF unset, skipped"
    exit 77
fi
if [ -n "${4:-}" ]; then
    ref=${P4PC_REF_DIR:-$(dirname "$P4PC_ISO")/refs}/$4.rgb
    sh "$(dirname "$0")/../tools/ref_guard.sh" --file "$ref" || exit 1
    export P4PC_REF_RGB="$ref" P4PC_REF_MAX="${5:-6.0}" P4PC_CAPTURE_FRAMES=$((gate - 1))
fi
walkdir=${P4PC_WALK_DIR:-$(dirname "$0")/walks}
walkfile=${6:-newgame_to_0413.walk}
walk=$(cd "$walkdir" && pwd)/$walkfile
first=down,cross,cross,cross driver=cross,cross,cross skip=140
if [ ! -f "$walk" ]; then
    echo "story_gate: no walk script $walk"
    exit 1
fi
. "$(dirname "$0")/artifacts.sh"
gate_artifacts_begin p4pc_story || exit 1
cd "$work" || exit 1
if [ -n "${4:-}" ]; then
    export P4PC_CAPTURE_PNG="$work/gate_%d.png"
fi
saveenv=
if [ $make = 1 ]; then
    { cat "$walk"
      printf '%s\n' "# 04/15 evening, free in the living room: the calendar, Save, slot 1" "timeout:3000" "walk:103,-217,25" "wait:30" "press:cross" "wait:300" "press:cross" \
          "wait:300" "press:cross" "wait:300" "press:cross" "wait:900"; } > "$work/make_save.walk"
    walk=$work/make_save.walk
    saveenv=P4PC_SAVE_DIR=$work/card
fi
env $saveenv SDL_VIDEODRIVER="${SDL_VIDEODRIVER:-offscreen}" SDL_AUDIODRIVER=dummy P4PC_TRAP_CONTINUE=1 P4PC_TURBO=2 P4PC_FACING_GATE=1 P4PC_AUTOPRESS_FRAMES=1 P4PC_AUTOPRESS_NEWGAME=1560 \
    P4PC_AUTOPRESS='cross@2300:2/150*20,start@6500:2/300*14' P4PC_WALK_SCRIPT="$walk" \
    P4PC_BATTLE_FIRST="$first" P4PC_BATTLE_DRIVER="$driver" P4PC_BATTLE_SKIP="$skip" \
    P4PC_NEWGAME_GATE="$gate,$want" timeout -s KILL 1700 "$prog" "$P4PC_ELF" > "$work/boot.log" 2>&1
rc=$?
gate_artifacts_boot_log p4pc_story
if [ $make = 1 ] && [ $rc -eq 0 ]; then
    out=${P4PC_SAVE_OUT:-$(dirname "$P4PC_ELF")/saves/story0415}
    if mkdir -p "$out" && cp -r "$work/card/mc0/." "$out/"; then
        echo "story_gate: save written to $out"
    else
        echo "story_gate: cannot write $out (the retail directory may be read-only): the save is in $work/card/mc0 (set P4PC_SAVE_OUT)"
        cp -r "$work/card" "${TMPDIR:-/tmp}/p4pc_story_card" 2>/dev/null
        rc=1
    fi
fi
grep -a "new-game gate\|walk script done\|WALK SCRIPT\|btlMain state 8 ->" "$work/boot.log" | tail -n 6
if [ $rc -ne 0 ]; then
    echo "story_gate: p4pc exited $rc"
    tail -n 20 "$work/boot.log"
fi
exit $rc

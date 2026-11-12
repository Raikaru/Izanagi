#!/bin/sh
# Story progress from a retail-made checkpoint: the GameFAQs 18664 save (04/11 evening, Dojima living room; a real save kept beside the disc in
# <retail dir>/saves/card, never committed) is loaded through the title (the title driver, P4PC_AUTOPRESS_LOAD: LOAD GAME, then the walk presses Cross through the slot, Yes and "Load successful"), then the waypoint walker
# (P4PC_WALK_SCRIPT=tests/walks/save0411_to_dream.walk, the same file drives retail's p4hook --walk) walks to the stairs, goes up
# (a field transition: living room 7:2 -> Your Room), checks the futon, sleeps (the sleep event, Help windows, the date flip) and plays into the
# fog dream. The process exits 0 at the gate flip only when the game state words say field 68 room 1 (src/platform/newgame_gate.c); a failed walk
# (it left the path: position never reached, an event that never ended) exits 3 before that.
#   usage: save_play_gate.sh <p4pc> [gate flip] [field:room[,month/day]] [save folder under saves/ (card)] [walk file (save0411_to_dream.walk)]
#   P4PC_SAVE_OUT=<folder>: copy the card there after a passed gate (home_0417 saves at the calendar: saves/story0417, the checkpoint of the next days).
#   P4PC_SAVE_ROOT=<folder>: private checkpoint-chain root instead of <retail dir>/saves; subsequent legs load only the just-written card.
#   The castle scenario (ctest castle_2f_0417, save0415_to_castle_2f.walk) takes the longest: P4PC_GATE_TIMEOUT (seconds, default 1300) bounds a run.
#   P4PC_GATE_BOOT_LOG=<absolute file>: retain the application's complete log before scratch cleanup.
#   The street scenario (ctest street_0417): saves/story0415 (made with story_gate.sh --make-save) and save0415_to_street.walk: the story plays on
#   from the 04/15 evening to the free roam in the shopping district (field 8 room 2) on 04/17.
# P4PC_TURBO=2: no pacing, no draw call outside the capture windows. Skipped (exit 77) without P4PC_ISO, P4PC_ELF or the saves.
. "$(dirname "$0")/input_guard.sh"

prog=$1
gate=${2:-14000}
want=${3:-68:1}
savedir=${4:-card}
walkfile=${5:-save0411_to_dream.walk}
if [ -z "$P4PC_ISO" ] || [ -z "$P4PC_ELF" ]; then
    echo "save_play_gate: P4PC_ISO or P4PC_ELF unset, skipped"
    exit 77
fi
saves=${P4PC_SAVE_ROOT:-$(dirname "$P4PC_ELF")/saves}
if [ ! -d "$saves/$savedir" ]; then
    echo "save_play_gate: no save in $saves/$savedir, skipped"
    exit 77
fi
walk=${P4PC_WALK_DIR:-$(dirname "$0")/walks}/$walkfile
if [ ! -f "$walk" ]; then
    echo "save_play_gate: no walk script $walk"
    exit 1
fi
. "$(dirname "$0")/artifacts.sh"
gate_artifacts_begin p4pc_save_play || exit 1
mkdir -p "$work/card/mc0" && cp -r "$saves/$savedir/." "$work/card/mc0/" || exit 1
walk=$(cd "$(dirname "$walk")" && pwd)/$(basename "$walk")
cd "$work" || exit 1
env SDL_VIDEODRIVER="${SDL_VIDEODRIVER:-offscreen}" SDL_AUDIODRIVER=dummy P4PC_TRAP_CONTINUE=1 P4PC_SAVE_DIR="$work/card" P4PC_TURBO=2 \
    P4PC_AUTOPRESS_FRAMES=1 P4PC_AUTOPRESS_LOAD=0 P4PC_WALK_SCRIPT="$walk" P4PC_NEWGAME_GATE="$gate,$want" \
    timeout -s KILL "${P4PC_GATE_TIMEOUT:-1300}" "$prog" "$P4PC_ELF" > "$work/boot.log" 2>&1
rc=$?
if [ -n "${P4PC_GATE_BOOT_LOG:-}" ] && ! cp "$work/boot.log" "$P4PC_GATE_BOOT_LOG"; then
    echo "save_play_gate: cannot retain log at $P4PC_GATE_BOOT_LOG"
    rc=1
fi
if [ -n "${P4PC_SCENE_ARTIFACTS:-}" ]; then
    cp "$work/boot.log" "$P4PC_SCENE_ARTIFACTS/boot.log" || exit 1
fi
# Checkpoint gates must finish their walk and write a save, not merely reach the same
# field/date earlier in a scene (04/18 morning and after school both use Class 2-2).
if [ $rc -eq 0 ] && [ "${P4PC_GATE_REQUIRE_SAVE:-0}" = 1 ]; then
    if ! grep -aq "walk script done" "$work/boot.log" || ! grep -aq "SDKMEMCARD_SAVE OK" "$work/boot.log"; then
        echo "save_play_gate: checkpoint walk did not finish and save"
        rc=1
    fi
fi
# P4PC_SAVE_OUT=<folder>: after a passed gate the card (the walk saved through the game's own save screen) is copied there: the next checkpoint (never committed).
if [ $rc -eq 0 ] && [ -n "$P4PC_SAVE_OUT" ]; then
    if mkdir -p "$P4PC_SAVE_OUT" && cp -r "$work/card/mc0/." "$P4PC_SAVE_OUT/"; then
        echo "save_play_gate: save written to $P4PC_SAVE_OUT"
    else
        echo "save_play_gate: cannot write $P4PC_SAVE_OUT"
        rc=1
    fi
fi
grep -a "new-game gate\|walk script done\|WALK SCRIPT" "$work/boot.log" | tail -n 4
if [ $rc -ne 0 ]; then
    echo "save_play_gate: p4pc exited $rc"
    tail -n 20 "$work/boot.log"
fi
exit $rc

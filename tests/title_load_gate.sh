#!/bin/sh
# Fresh-process title automation must reach LOAD GAME and restore the real card
# in every run, not merely log an attempted Cross on the title menu.
# Usage: title_load_gate.sh <p4pc>
. "$(dirname "$0")/input_guard.sh"
prog=$1
if [ -z "$P4PC_ISO" ] || [ -z "$P4PC_ELF" ]; then
    echo "title_load_gate: P4PC_ISO or P4PC_ELF unset, skipped"
    exit 77
fi
saves=${P4PC_SAVE_ROOT:-$(dirname "$P4PC_ELF")/saves}
if [ ! -d "$saves/story0420tv" ]; then
    echo "title_load_gate: story0420tv checkpoint unavailable, skipped"
    exit 77
fi
here=$(cd "$(dirname "$0")" && pwd) || exit 1
. "$here/artifacts.sh"
gate_artifacts_begin p4pc_title_load || exit 1
printf '%s\n' 'timeout:6000' 'wait:140' 'press:cross' 'wait:140' 'press:cross' 'wait:140' 'press:cross' 'skipmsg' 'waitcontrol:on' 'log' > "$work/load.walk" || exit 1
passed=0
for run in 1 2 3 4 5; do
    case_dir=$work/run-$run
    mkdir -p "$case_dir" || exit 1
    env P4PC_GATE_REQUIRE_SAVE=0 P4PC_GATE_TIMEOUT=180 \
        P4PC_SCENE_ARTIFACTS="$case_dir" P4PC_GATE_BOOT_LOG="$case_dir/boot.log" P4PC_WALK_DIR="$work" \
        sh "$here/save_play_gate.sh" "$prog" 4000 20:1,4/20 story0420tv load.walk > "$case_dir/driver.log" 2>&1
    rc=$?
    if [ "$rc" -ne 0 ] || ! grep -aq 'title driver: LOAD GAME confirmed' "$case_dir/boot.log" || ! grep -aq 'walk script done' "$case_dir/boot.log"; then
        echo "title_load_gate: run $run failed (exit $rc)"
        cat "$case_dir/driver.log"
        exit 1
    fi
    passed=$((passed + 1))
done
echo "title_load_gate: LOAD GAME $passed/5 PASS (restored 20:1,4/20)"

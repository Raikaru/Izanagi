#!/bin/sh
# A corpus moment reached by normal controller input, never a viewer camera/procedure override.
# usage: event_scene_gate.sh <p4pc> <corpus scene> <nominal flip (sanity bound)> <MAD limit|state-only> <save folder|newgame> <walk file> [nearby-fog region x,y,w,h] [region MAD limit]
# References are the existing refs/p4-<scene>.rgb beside the disc; snapshots are private scratch files.
. "$(dirname "$0")/input_guard.sh"

prog=$1
scene=$2
capture=$3
limit=$4
seed=$5
walk=$6
region=${7:-}
regionlimit=${8:-}
state_only=0
if [ "$limit" = state-only ]; then
    state_only=1
    limit=0
fi
here=$(cd "$(dirname "$0")" && pwd) || exit 1
if [ -z "$P4PC_ISO" ] || [ -z "$P4PC_ELF" ]; then
    echo "event_scene_gate: P4PC_ISO or P4PC_ELF unset, skipped"
    exit 77
fi
ref=${P4PC_REF_DIR:-$(dirname "$P4PC_ISO")/refs}/${P4PC_EVENT_REF:-p4-$scene}.rgb
if [ "$state_only" -eq 0 ] && [ ! -f "$ref" ]; then
    echo "event_scene_gate: no reference $ref, skipped"
    exit 77
fi
. "$(dirname "$0")/artifacts.sh"
gate_artifacts_begin "$scene" || exit 1
capture_spec=$(python3 "$here/event_scene_state.py" capture-spec "$scene" "$here/../docs/skygl_scenes.json" "$capture") || exit 1
last=${capture_spec##*,}
deadline=$((last + 1))
unset P4PC_CAPTURE_FRAMES P4PC_DUMP_RAM_AT
if [ "$state_only" -eq 1 ]; then
    unset P4PC_REF_RGB P4PC_REF_MAX P4PC_CAPTURE_REF P4PC_CAPTURE_RGB
else
    export P4PC_REF_RGB="$ref" P4PC_REF_MAX="$limit"
fi
export P4PC_CAPTURE_STATE="$capture_spec" P4PC_CAPTURE_PNG="$work/frame_%d.png"
export P4PC_DUMP_RAM="$work/state" P4PC_TURBO_WINDOW="${P4PC_TURBO_WINDOW:-150}"
if [ "$seed" = newgame ]; then
    P4PC_SCENE_ARTIFACTS="${P4PC_SCENE_ARTIFACTS:+$work}" sh "$here/story_gate.sh" "$prog" "$deadline" -1:-1 "" "$limit" "$walk"
else
    P4PC_SCENE_ARTIFACTS="${P4PC_SCENE_ARTIFACTS:+$work}" sh "$here/save_play_gate.sh" "$prog" "$deadline" -1:-1 "$seed" "$walk"
fi
rc=$?
[ "$rc" -eq 0 ] || exit "$rc"
set -- "$work"/state_*.bin
if [ "$#" -ne 1 ] || [ ! -f "$1" ]; then
    echo "event_scene_gate: expected exactly one state-triggered RAM snapshot"
    exit 1
fi
snapshot=$1
if [ "$state_only" -eq 1 ]; then
    python3 "$here/event_scene_state.py" "$snapshot" "$scene" "$here/../docs/skygl_scenes.json" --state-only
elif [ -n "$region" ]; then
    image=
    for candidate in "$work"/frame_*.png; do
        case "$candidate" in *.win.png) continue ;; esac
        if [ -n "$image" ]; then
            echo "event_scene_gate: expected one physical capture for regional comparison"
            exit 1
        fi
        image=$candidate
    done
    [ -f "$image" ] || { echo "event_scene_gate: regional PNG missing"; exit 1; }
    python3 "$here/event_scene_state.py" "$snapshot" "$scene" "$here/../docs/skygl_scenes.json" \
        --nearby-camera --pixels "$image" "$ref" "$regionlimit" "$region"
else
    python3 "$here/event_scene_state.py" "$snapshot" "$scene" "$here/../docs/skygl_scenes.json"
fi

#!/bin/sh
# Native port-frame references, not retail-image replacements. Reuse the real
# save/walk runner and its retained-artifact handling; each reference is scoped
# to its qualified renderer by P4PC_GATE_RENDERER in the test registration.
# Usage: widescreen_saved_gate.sh <p4pc> allout|allout-intro|cast-eyes|shuffle 16:9|21:9 REF_NAME
. "$(dirname "$0")/input_guard.sh"
prog=$1 scene=$2 aspect=$3 name=$4
if [ -z "${P4PC_ISO:-}" ] || [ -z "${P4PC_ELF:-}" ]; then
    echo 'widescreen_saved_gate: private retail inputs unset, skipped'
    exit 77
fi
case "$scene" in
    allout) frame=6660; want=-1:-1,5/23; seed=story0523tv; walk=save0523tv_to_bathhouse_5f.walk ;;
    allout-intro) frame=6640; want=-1:-1,5/23; seed=story0523tv; walk=save0523tv_to_bathhouse_5f.walk ;;
    cast-eyes) frame=6180; want=-1:-1,5/23; seed=story0523tv; walk=save0523tv_to_bathhouse_5f.walk ;;
    shuffle) frame=9160; want=-1:-1,4/20; seed=story0420_5f; walk=save0420_5f_to_training.walk ;;
    *) echo "widescreen_saved_gate: unknown scene $scene" >&2; exit 2 ;;
esac
case "$aspect" in
    16:9) mode=16:9; width=1280 ;;
    21:9) mode=auto; width=1680 ;;
    *) echo "widescreen_saved_gate: unknown aspect $aspect" >&2; exit 2 ;;
esac
ref=${P4PC_REF_DIR:-$(dirname "$P4PC_ISO")/refs}/$name.rgb
sh "$(dirname "$0")/../tools/ref_guard.sh" --file "$ref" || exit 1
export P4PC_CONFIG=none P4PC_ASPECT="$mode" P4PC_WINDOW_W="$width" P4PC_WINDOW_H=720 P4PC_PRESENT=blit
export P4PC_RESOLUTION=scale:1
unset P4PC_CAPTURE_STATE P4PC_CAPTURE_REF P4PC_CAPTURE_REF_MAX
export P4PC_VIRTUAL_CLOCK=1 P4PC_TURBO_WINDOW=4 P4PC_CAPTURE_FRAMES="$frame"
export P4PC_REF_RGB="$ref" P4PC_REF_MAX=0.1
case "$scene" in
    allout-intro|cast-eyes)
        . "$(dirname "$0")/artifacts.sh"
        gate_artifacts_begin p4pc_cutins || exit 1
        export P4PC_CAPTURE_PNG="$work/frame.png" P4PC_CAPTURE_WINDOW=1
        sh "$(dirname "$0")/save_play_gate.sh" "$prog" "$((frame + 1))" "$want" "$seed" "$walk" || exit "$?"
        here=$(cd "$(dirname "$0")" && pwd) || exit 1
        case "$(uname -s)" in
            MINGW*|MSYS*|CYGWIN*)
                py -3 "$(cygpath -w "$here/cutin_edges.py")" "$scene" \
                    "$(cygpath -w "$work/frame.png")" "$(cygpath -w "$work/frame.png.win.png")" ;;
            *) python3 "$here/cutin_edges.py" "$scene" "$work/frame.png" "$work/frame.png.win.png" ;;
        esac
        exit "$?"
        ;;
esac
exec sh "$(dirname "$0")/save_play_gate.sh" "$prog" "$((frame + 1))" "$want" "$seed" "$walk"

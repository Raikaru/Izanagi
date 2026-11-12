#!/bin/sh
# Renderer regression gate for one viewer scene: p4pc_viewer <mode> against a reference frame, as mean absolute difference over RGB.
# The reference is a raw 640x448 RGB file refs/<name>.rgb beside the retail image (tools/png_to_rgb.py makes it from a PNG); it is
# never committed. Exits 77 without P4PC_ISO or P4PC_ELF; missing, unpinned or changed references fail.
# usage: scene_mad.sh <p4pc_viewer> <out.png> <viewer mode> <reference name> <max MAD percent> [VAR=value ...]
. "$(dirname "$0")/input_guard.sh"

viewer=$1
out=$2
mode=$3
name=$4
limit=$5
shift 5
if [ -z "$P4PC_ISO" ] || [ -z "$P4PC_ELF" ]; then
    echo "scene_mad: P4PC_ISO or P4PC_ELF unset, skipped"
    exit 77
fi
ref=${P4PC_REF_DIR:-$(dirname "$P4PC_ISO")/refs}/$name.rgb
sh "$(dirname "$0")/../tools/ref_guard.sh" --file "$ref" || exit 1
. "$(dirname "$0")/artifacts.sh"
gate_artifacts_begin p4pc_scene viewer || exit 1
env "$@" P4PC_REF_RGB="$ref" P4PC_REF_MAX="$limit" SDL_VIDEODRIVER="${SDL_VIDEODRIVER:-offscreen}" "$viewer" "$P4PC_ELF" "$mode" "$out" > "$work/boot.log" 2>&1
rc=$?
if [ "$rc" -eq 0 ] && [ "${P4PC_TITLE_EDGES:-0}" = 1 ]; then
    if [ -n "${P4PC_PYTHON:-}" ]; then edge_python=$P4PC_PYTHON
    elif command -v python3 >/dev/null 2>&1; then edge_python=python3
    else edge_python=python
    fi
    "$edge_python" "$(dirname "$0")/title_edges.py" "$out" "${out}.win.png" >> "$work/boot.log" 2>&1 || rc=$?
fi
cat "$work/boot.log"
if [ "$rc" -ne 0 ] && [ -f "$out" ]; then cp "$out" "$work/frame.png" || exit 1; fi
exit "$rc"

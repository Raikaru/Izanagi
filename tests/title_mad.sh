#!/bin/sh
# Renderer regression gate: the title frame (p4pc_viewer title, frame 400 of the intro, the settled "press any
# button" screen) against the PCSX2 frame timeline/title.png, as mean absolute difference over RGB. The reference
# is a raw 640x448 RGB file (refs/title.rgb beside the retail image: tools/png_to_rgb.py makes it from the
# PortTrace PNG); it is never committed. Exits 77 without P4PC_ISO or P4PC_ELF; missing or changed pinned references fail.
# usage: title_mad.sh <p4pc_viewer> <out.png>
. "$(dirname "$0")/input_guard.sh"

viewer=$1
out=$2
limit=${P4PC_TITLE_MAD_MAX:-1.25}
if [ -z "$P4PC_ISO" ] || [ -z "$P4PC_ELF" ]; then
    echo "title_mad: P4PC_ISO or P4PC_ELF unset, skipped"
    exit 77
fi
ref=${P4PC_REF_DIR:-$(dirname "$P4PC_ISO")/refs}/title.rgb
sh "$(dirname "$0")/../tools/ref_guard.sh" --file "$ref" || exit 1
. "$(dirname "$0")/artifacts.sh"
gate_artifacts_begin p4pc_title viewer || exit 1
P4PC_TITLE_FRAME=400 P4PC_DRAW_WINDOW=${P4PC_DRAW_WINDOW:-4} P4PC_REF_RGB=$ref P4PC_REF_MAX=$limit SDL_VIDEODRIVER=${SDL_VIDEODRIVER:-offscreen} \
    "$viewer" "$P4PC_ELF" title "$out" > "$work/boot.log" 2>&1
rc=$?
cat "$work/boot.log"
if [ "$rc" -ne 0 ] && [ -f "$out" ]; then cp "$out" "$work/frame.png" || exit 1; fi
exit "$rc"

#!/bin/sh
# A run without a usable GL context ends cleanly: SDL's dummy video driver has no OpenGL (as SDL's offscreen driver on Windows), so the
# RenderWare device cannot open, and the game exits 1 with "no OpenGL context" instead of going on and jumping to address 0.
#   usage: gl_open_failure.sh <p4pc>
# Skipped (exit 77) without P4PC_ISO and P4PC_ELF.
. "$(dirname "$0")/input_guard.sh"

prog=$1
if [ -z "$P4PC_ISO" ] || [ -z "$P4PC_ELF" ]; then
    echo "gl_open_failure: P4PC_ISO or P4PC_ELF unset, skipped"
    exit 77
fi
. "$(dirname "$0")/artifacts.sh"
gate_artifacts_begin p4pc_gl_open_failure || exit 1
cd "$work" || exit 1
env SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy timeout -s KILL 60 "$prog" "$P4PC_ELF" > "$work/boot.log" 2>&1
rc=$?
if [ "$rc" -eq 1 ] && grep -aq "no OpenGL context, the game cannot start" "$work/boot.log" && ! grep -aq "CRASH\|p4 guest fault" "$work/boot.log"; then
    echo "gl_open_failure: exit 1 with the reason"
    exit 0
fi
echo "gl_open_failure: exit $rc; last lines of the log:"
tail -n 25 "$work/boot.log"
exit 1

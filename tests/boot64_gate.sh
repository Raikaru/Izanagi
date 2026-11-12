#!/bin/sh
# The 64-bit executable boots (docs/port64.md 2.12): p4pc64 runs headless against the retail image and disc for at most <seconds>, and the
# gate passes when its log reaches <marker> with no guest fault, signal or trap abort before it. The marker moves forward as the 64-bit
# build gets further; the end of the road is the same gates as the i386 build (newgame_gate.sh, title_mad.sh ... with p4pc64).
#   usage: boot64_gate.sh <p4pc64> [marker] [seconds]
# Skipped (exit 77) without P4PC_ISO and P4PC_ELF.
. "$(dirname "$0")/input_guard.sh"

prog=$1
marker=${2:-'##### MOVIE START #####'}
tmo=${3:-150}
if [ -z "$P4PC_ISO" ] || [ -z "$P4PC_ELF" ]; then
    echo "boot64_gate: P4PC_ISO or P4PC_ELF unset, skipped"
    exit 77
fi
. "$(dirname "$0")/artifacts.sh"
gate_artifacts_begin p4pc_boot64 || exit 1
cd "$work" || exit 1
env SDL_VIDEODRIVER="${SDL_VIDEODRIVER:-offscreen}" SDL_AUDIODRIVER=dummy P4PC_TRAP_CONTINUE=1 "$prog" "$P4PC_ELF" > "$work/boot.log" 2>&1 &
pid=$!
n=0
status=timeout
while [ "$n" -lt "$tmo" ]; do
    sleep 1
    n=$((n + 1))
    if grep -aq -F "$marker" "$work/boot.log"; then
        status=marker
        break
    fi
    if ! kill -0 "$pid" 2>/dev/null; then
        status=exited
        break
    fi
done
kill -9 "$pid" 2>/dev/null
wait "$pid" 2>/dev/null
if [ "$status" = marker ] && ! grep -aq "p4 guest fault" "$work/boot.log"; then
    echo "boot64_gate: reached '$marker' after $n s"
    exit 0
fi
echo "boot64_gate: $status (marker '$marker') after $n s; last lines of the log:"
tail -n 25 "$work/boot.log"
exit 1

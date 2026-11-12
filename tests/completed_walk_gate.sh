#!/bin/sh
# Native goal checkpoints whose canonical tape must finish, without a save claim.
set -eu
here=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
log=$(mktemp)
trap 'rm -f "$log"' EXIT
export P4PC_GATE_BOOT_LOG="$log"
export P4PC_GATE_REQUIRE_SAVE=0
sh "$here/save_play_gate.sh" "$@" || exit $?
if ! grep -aq 'walk script done' "$log"; then
    echo 'completed_walk: missing completed canonical input checkpoint'
    exit 1
fi
echo 'completed_walk: PASS native goal and completed canonical input'

#!/bin/sh
# Pristine collection card3-slot07; require the native protagonist COMMAND witness.
set -eu
here=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
log=$(mktemp)
trap 'rm -f "$log"' EXIT
export P4PC_GATE_BOOT_LOG="$log"
export P4PC_GATE_REQUIRE_SAVE=0
sh "$here/save_play_gate.sh" "$1" 20000 243:2,8/3 playthrough0803voidquest collection0803_voidquest_opening.walk || exit $?
if ! grep -aq 'walk script done' "$log"; then
    echo 'voidquest_opening: missing protagonist COMMAND checkpoint'
    exit 1
fi
echo 'voidquest_opening: PASS collection -> Mitsuo COMMAND (not victory)'

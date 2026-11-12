#!/bin/sh
# Pristine collection card3-slot14: require completed native travel and live step 80.
set -eu
here=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
log=$(mktemp)
trap 'rm -f "$log"' EXIT
export P4PC_GATE_BOOT_LOG="$log"
export P4PC_GATE_REQUIRE_SAVE=0
sh "$here/save_play_gate.sh" "$1" 12000 27:1,9/18 playthrough0918labentry collection0918_to_secretlab_entrance.walk || exit $?
if ! grep -aq 'walk script done' "$log"; then
    echo 'secretlab_entrance: missing live exterior checkpoint'
    exit 1
fi
echo 'secretlab_entrance: PASS collection -> Secret Laboratory exterior (step 80)'

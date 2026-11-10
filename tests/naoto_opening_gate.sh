#!/bin/sh
# Pristine card3-slot15: real B9F door/event -> fresh protagonist COMMAND.
set -eu
here=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
log=$(mktemp)
trap 'rm -f "$log"' EXIT
export P4PC_GATE_BOOT_LOG="$log"
export P4PC_GATE_REQUIRE_SAVE=0
sh "$here/save_play_gate.sh" "$1" 15000 245:2,9/18 playthrough0918lab9f collection0918_lab9f_to_shadow_naoto.walk || exit $?
if ! grep -aq 'walk script done' "$log"; then
    echo 'naoto_opening: missing protagonist COMMAND checkpoint'
    exit 1
fi
echo 'naoto_opening: PASS B9F -> Shadow Naoto COMMAND (not victory)'

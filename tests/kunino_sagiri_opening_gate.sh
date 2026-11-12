#!/bin/sh
# Pristine card4-slot09: live Heaven10 door/story -> fresh party COMMAND.
set -eu
here=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
log=$(mktemp)
trap 'rm -f "$log"' EXIT
export P4PC_GATE_BOOT_LOG="$log"
export P4PC_GATE_REQUIRE_SAVE=0
sh "$here/save_play_gate.sh" "$1" 15000 244:2,11/6 playthrough1106heaven10f collection1106_heaven10f_to_kunino_sagiri.walk || exit $?
if ! grep -aq 'walk script done' "$log"; then
    echo 'kunino_sagiri_opening: missing fresh party COMMAND checkpoint'
    exit 1
fi
echo 'kunino_sagiri_opening: PASS Heaven10 -> Kunino Sagiri COMMAND (not victory)'

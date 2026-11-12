#!/bin/sh
# Private pristine collection card2-slot15, copied by save_play_gate.
# Completion of the tape proves Rise cleanup/event and Teddie's fresh COMMAND.
# The following encounter uses arena 242:3, not the pre-battle Marukyu field 25:2.
set -eu
here=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
log=$(mktemp)
trap 'rm -f "$log"' EXIT
export P4PC_GATE_BOOT_LOG="$log"
export P4PC_GATE_REQUIRE_SAVE=0
sh "$here/save_play_gate.sh" "$1" 60000 242:3,6/25 playthrough0625marukyu11f collection0625_rise_to_teddie_menu.walk || exit $?
if ! grep -aq 'walk script done' "$log"; then
    echo 'marukyu_allocator: missing post-event Teddie COMMAND checkpoint'
    exit 1
fi
echo 'marukyu_allocator: PASS Rise cleanup/event -> Teddie COMMAND'

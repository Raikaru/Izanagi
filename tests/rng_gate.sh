#!/bin/sh
# Determinism against retail (PCSX2) values, recorded with p4hook --call-trace 003b6f00 / 003b7060 (PCSX2 trace tooling, not in the public repository):
#   boot   from boot: the generator's second boot seed is 0x14bc (RpRandomSeed(*(u32 *)0x10000000), TIMER0 stands still in the port) and the seed's mixing
#          leaves draw 311 = 6b0c7ac3 and draw 620 = 0d656b36. With the old timer value 0 the whole stream differed from the second seed on.
#   floor  from the Apollo save (the card in <ELF dir>/saves/card_apollo, as systems_gate.sh): LOAD GAME, Goho-M, the Entrance, the Marukyu club, "Start from 1F"
#          with the floor seed pinned to the retail run's (0x19c3, P4PC_BOARD_SEED): the 21 occupied rows of the floor's cell grid (exits, type and piece id
#          per cell, the `CHECK grid` lines of P4PC_DUMP_CHECKS) checksum to the retail floor's.
#   usage: rng_gate.sh <p4pc> boot|floor
# Skipped (exit 77) without P4PC_ISO and P4PC_ELF (and for floor without the save).
. "$(dirname "$0")/input_guard.sh"

prog=$1
scenario=$2
if [ -z "$P4PC_ISO" ] || [ -z "$P4PC_ELF" ]; then
    echo "rng_gate: P4PC_ISO or P4PC_ELF unset, skipped"
    exit 77
fi
here=$(cd "$(dirname "$0")" && { pwd -W 2>/dev/null || pwd; })
. "$(dirname "$0")/artifacts.sh"
gate_artifacts_begin p4pc_rng_gate || exit 1
case $scenario in
    boot)
        env SDL_VIDEODRIVER="${SDL_VIDEODRIVER:-offscreen}" SDL_AUDIODRIVER=dummy P4PC_TRAP_CONTINUE=1 P4PC_TURBO=2 P4PC_LOG_RANDOM=700 \
            P4PC_NEWGAME_GATE=700 P4PC_SAVE_DIR="$work/card" timeout -s KILL 600 "$prog" "$P4PC_ELF" > "$work/boot.log" 2>&1
        grep -a "RngSeed seed=000014bc draws=310" "$work/boot.log" > /dev/null || { echo "rng_gate: second boot seed is not 0x14bc"; grep -a RngSeed "$work/boot.log" | head -3; exit 1; }
        grep -a "RngDraw 311 6b0c7ac3" "$work/boot.log" > /dev/null || { echo "rng_gate: draw 311 differs from retail (6b0c7ac3)"; grep -a "RngDraw 311 " "$work/boot.log"; exit 1; }
        grep -a "RngDraw 620 0d656b36" "$work/boot.log" > /dev/null || { echo "rng_gate: draw 620 differs from retail (0d656b36)"; grep -a "RngDraw 620 " "$work/boot.log"; exit 1; }
        echo "rng_gate: boot seeds and draws equal retail's"
        ;;
    floor)
        saves=${P4PC_SAVES_DIR:-$(dirname "$P4PC_ELF")/saves}/card_apollo
        if [ ! -d "$saves" ]; then
            echo "rng_gate: no retail save in $saves, skipped"
            exit 77
        fi
        floor_goal=7000
        mkdir -p "$work/card/mc0" && cp -r "$saves"/. "$work/card/mc0/" || exit 1
        env SDL_VIDEODRIVER="${SDL_VIDEODRIVER:-offscreen}" SDL_AUDIODRIVER=dummy P4PC_TRAP_CONTINUE=1 P4PC_TURBO=2 P4PC_AUTOPRESS_FRAMES=1 P4PC_AUTOPRESS_LOAD=0 \
            P4PC_SAVE_DIR="$work/card" P4PC_WALK_SCRIPT="$here/../tools/systems/apollo_club_1f.txt" P4PC_BOARD_SEED=0x19c3 P4PC_DUMP_CHECKS=1 \
            P4PC_NEWGAME_GATE="$floor_goal" timeout -s KILL 1500 "$prog" "$P4PC_ELF" > "$work/boot.log" 2>&1
        # Include the populated 900-update diagnostic after floor entry, not just
        # its early 60-update header. Keep the first complete grid's retail checksum.
        grep -a "CHECKS field 42" "$work/boot.log" > /dev/null || { echo "rng_gate: the club floor was not reached"; tail -n 5 "$work/boot.log"; exit 1; }
        sed -n 's/^p4pc: CHECK grid row *\([0-9]*\): *\(.*\)$/row \1: \2/p' "$work/boot.log" | tr -s ' ' > "$work/grid_all.txt"
        awk '{ if (seen[$2]) exit; seen[$2] = 1; print }' "$work/grid_all.txt" | tr -d '\r' > "$work/grid.txt"
        got=$(cksum < "$work/grid.txt")
        if [ "$got" != "4034924771 2512" ]; then
            echo "rng_gate: floor grid differs from retail's (cksum $got, want 4034924771 2512)"
            cat "$work/grid.txt"
            exit 1
        fi
        echo "rng_gate: floor grid for seed 0x19c3 equals retail's"
        ;;
    *) echo "rng_gate: unknown scenario '$scenario'"; exit 2 ;;
esac

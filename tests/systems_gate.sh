#!/bin/sh
# Game systems off the story path, from the level-99 Apollo save (tools/ps2save.py import of the Apollo Save Database's PS2/SLUS21782/00000001.zip,
# kept beside the disc in saves/card_apollo, never committed): the title's LOAD GAME, then a walk script (tools/systems/*.txt, include/p4pc/walk_script.h)
# plays the game's own flows: the camp menu, Goho-M (the escape item) out of the final dungeon to the TV-world Entrance, the Velvet Room door.
# The gate flip's frame (one flip earlier, as in tests/newgame_gate.sh) is compared with a raw 640x448 RGB reference (refs/systems_<scenario>.rgb beside the
# retail image, never committed; `systems_gate.sh --make-ref <p4pc> <scenario>` writes it from the current build).
#   scenario fusion      Velvet Room > Fuse Personas > Normal Spread: "Select the 1st card" (the fusion results are random, so the gate stops at the list)
#   scenario compendium  Velvet Room > Check Compendium > View Compendium
#   scenario club        Rise's navigator > Go to the club > the Marukyu Striptease stage steps > "Which floor?"
#   scenario minimap     Marukyu 1F, standing in the first room: the minimap (explored cells, player arrow, enemy dots, stairs star) in the corner. The whole frame is compared, so ctest passes a limit of 0.7 percent
#   scenario game1f      Rise's navigator > Go to the game: Void Quest's entry room, its CHECK, 1F
#   scenario heaven1f    Rise's navigator > Go to Heaven: the Pearly Gates, Rise's "good luck", 1F
#   scenario daidara     Teddie > Return to shop > Daidara > Buy weapon > Tajikarao Sword bought (yen 9,941,999, "Do you want to equip")
#   scenario shiroku     Teddie > Shiroku Store > Buy item > Medicine: "The price for 1 will be 680. Do you want to purchase it?"
#   scenario entrance / seats  Retail-aligned 3D/post-processing frames (GS audit, renderer_port.md 11); defaults remain retail, while distinct port witnesses supply a reference name.
#   usage: systems_gate.sh <p4pc> <scenario> [limit percent] [reference name] | systems_gate.sh --make-ref <p4pc> <scenario>
# P4PC_SAVES_DIR overrides <ELF dir>/saves. Skips (77) without ISO, ELF or save; missing, unpinned or changed references fail.
. "$(dirname "$0")/input_guard.sh"

make=0
if [ "$1" = "--make-ref" ]; then make=1; shift; fi
prog=$1
scenario=$2
limit=${3:-6.0}
goal=-1:-1
case $scenario in
    fusion) script=apollo_velvet_fusion_gate.txt; gate=4400 ;;
    compendium) script=apollo_velvet_compendium_gate.txt; gate=4300 ;;
    club) script=apollo_club_floor_menu_gate.txt; gate=5950 ;;
    minimap) script=apollo_club_1f.txt; gate=8300; refname=systems_minimap_async ;;
    game1f) script=apollo_game_1f_gate.txt; gate=6000; refname=systems_game1f_faced ;; # its enemies steer by their facing: the reference is from the build where walkers face their way
    heaven1f) script=apollo_heaven_1f_gate.txt; gate=6000; refname=systems_heaven1f_faced ;; # as game1f below
    daidara) script=apollo_daidara_purchase_gate.txt; gate=5300 ;;
    shiroku) script=apollo_shiroku_purchase.txt; gate=5500 ;;
    entrance) script=apollo_goho_to_entrance.txt; gate=2900 ;;
    seats) script=apollo_entrance_to_club.txt; gate=5101 ;;
    *) echo "systems_gate: unknown scenario '$scenario'"; exit 2 ;;
esac
if [ -z "$P4PC_ISO" ] || [ -z "$P4PC_ELF" ]; then
    echo "systems_gate: P4PC_ISO or P4PC_ELF unset, skipped"
    exit 77
fi
# SDL's offscreen video driver is the Linux headless one; the Windows exe on this machine draws on its GPU in a hidden window (P4PC_HIDDEN_WINDOW)
case $(uname -s) in MINGW*|MSYS*|CYGWIN*) video=windows ;; *) video=offscreen ;; esac
saves=${P4PC_SAVES_DIR:-$(dirname "$P4PC_ELF")/saves}/card_apollo
if [ ! -d "$saves" ]; then
    echo "systems_gate: no retail save in $saves, skipped"
    exit 77
fi
ref=${P4PC_REF_DIR:-$(dirname "$P4PC_ISO")/refs}/${4:-${refname:-systems_$scenario}}.rgb
if [ $make = 0 ]; then
    sh "$(dirname "$0")/../tools/ref_guard.sh" --file "$ref" || exit 1
elif [ -e "$ref" ]; then
    echo "ref_guard: refusing to overwrite $ref; generate into a fresh P4PC_REF_DIR, then stage a new filename for the cutover" >&2
    exit 1
fi
here=$(cd "$(dirname "$0")" && { pwd -W 2>/dev/null || pwd; })
. "$(dirname "$0")/artifacts.sh"
gate_artifacts_begin p4pc_systems_gate || exit 1
mkdir -p "$work/card/mc0" && cp -r "$saves"/. "$work/card/mc0/" || exit 1
cd "$work" || exit 1
if [ $make = 1 ]; then
    mkdir -p "$(dirname "$ref")"
    export P4PC_CAPTURE_RGB="$ref"
else
    export P4PC_REF_RGB="$ref" P4PC_REF_MAX="$limit"
fi
env SDL_VIDEODRIVER="${SDL_VIDEODRIVER:-$video}" SDL_AUDIODRIVER=dummy P4PC_TRAP_CONTINUE=1 P4PC_TURBO=2 P4PC_AUTOPRESS_FRAMES=1 P4PC_AUTOPRESS_LOAD=0 \
    P4PC_SAVE_DIR="$work/card" P4PC_WALK_SCRIPT="$here/../tools/systems/$script" P4PC_CAPTURE_FRAMES=$((gate - 1)) P4PC_CAPTURE_PNG="$work/gate_%d.png" \
    P4PC_NEWGAME_GATE="$gate,$goal" timeout -s KILL 1500 "$prog" "$P4PC_ELF" > "$work/boot.log" 2>&1
rc=$?
grep -a "new-game gate\|p4pc turbo:" "$work/boot.log" | tail -n 4
if [ $rc -ne 0 ]; then
    echo "systems_gate: $scenario: p4pc exited $rc"
    tail -n 20 "$work/boot.log"
fi
exit $rc

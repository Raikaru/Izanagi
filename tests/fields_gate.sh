#!/bin/sh
# Rooms the story path does not show first: each F:R is started through the menu harness (P4PC_START_MENU=F:R, src/platform/menu_harness.c, the new-game
# state) and must still be that field and room at flip 950 with no crash or trap; the frame at flip 949 must not be black (more than 2% of its pixels
# lit, checked when python3 is there). The default list is the rooms that crashed or drew wrong once: the school roof and a shrine room (64x8 and
# 16x32 4-bit native textures the decoder refused: the field load dereferenced a null texture dictionary), Junes, the riverbank, the shops.
#   usage: fields_gate.sh <p4pc> [F:R ...]      fields_gate.sh <p4pc> --all   (every field pack on the disc but the generated dungeon floors 40-48, which need the dungeon flow: 133 rooms)
# Skipped (exit 77) without P4PC_ISO and P4PC_ELF.
. "$(dirname "$0")/input_guard.sh"

if [ -z "$P4PC_ISO" ] || [ -z "$P4PC_ELF" ]; then
    echo "fields_gate: P4PC_ISO or P4PC_ELF unset, skipped"
    exit 77
fi
prog=$1
shift
if [ "$1" = "--all" ]; then
    set -- 1:1 4:1 4:2 4:3 6:1 6:2 6:3 6:4 6:5 6:6 6:7 6:8 6:9 6:10 6:11 6:12 6:13 6:14 6:15 6:16 6:17 7:1 7:2 7:3 8:1 8:2 8:3 8:4 8:5 8:6 8:7 8:8 8:9 8:10 9:1 9:2 9:3 9:4 10:1 10:2 10:3 10:4 11:1 11:2 12:2 12:3 12:4 13:1 13:6 13:7 13:8 14:1 14:2 15:1 15:2 15:3 16:1 16:2 16:3 16:4 16:5 17:1 17:2 17:3 20:1 21:1 21:2 22:1 22:2 23:1 23:2 23:3 24:1 24:2 25:1 25:2 26:1 26:2 27:1 27:2 28:1 28:2 29:1 29:2 30:1 30:2 60:1 61:1 61:2 61:3 62:1 62:2 62:3 63:1 63:2 64:1 64:2 64:3 65:1 65:2 66:1 66:2 66:3 67:1 67:2 67:3 68:1 100:1 100:2 100:3 222:1 222:2 222:3 240:1 240:2 240:3 241:1 241:2 242:1 242:2 242:3 243:1 243:2 244:1 244:2 244:3 245:1 245:2 246:1 246:2 246:3 247:1 247:2
elif [ $# -eq 0 ]; then
    set -- 6:14 8:9 9:1 9:2 9:3 10:1 11:1 16:1 7:2 8:4 8:6 15:2
fi
. "$(dirname "$0")/artifacts.sh"
rc=0
for fr in "$@"; do
    gate_artifacts_begin p4pc_fields_gate || exit 1
    (cd "$work" && env SDL_VIDEODRIVER="${SDL_VIDEODRIVER:-offscreen}" SDL_AUDIODRIVER=dummy P4PC_TRAP_CONTINUE=1 P4PC_TURBO=2 P4PC_START_MENU="$fr" \
        P4PC_CAPTURE_FRAMES=949 P4PC_CAPTURE_PNG="$work/f.png" P4PC_CAPTURE_RGB="$work/f.rgb" \
        P4PC_NEWGAME_GATE="950,$fr" timeout -s KILL 600 "$prog" "$P4PC_ELF" > "$work/boot.log" 2>&1)
    r=$?
    lit="?"
    if [ -f "$work/f.rgb" ] && command -v python3 > /dev/null 2>&1; then
        lit=$(python3 -c "import sys; d=open(sys.argv[1],'rb').read(); n=len(d)//3; print('%.1f' % (100.0*sum(1 for i in range(0,len(d)-2,3) if max(d[i],d[i+1],d[i+2])>16)/max(n,1)))" "$work/f.rgb")
    fi
    echo "fields_gate: $fr: $(grep -a 'new-game gate' "$work/boot.log" | tail -n 1 | sed 's/.*new-game gate //') lit $lit%"
    if [ $r -ne 0 ]; then
        echo "fields_gate: $fr: p4pc exited $r"
        grep -a "CRASH\|p4gl_native\|trap" "$work/boot.log" | head -n 4
        rc=1
    elif [ "$lit" != "?" ] && [ "${lit%%.*}" -lt 2 ]; then
        echo "fields_gate: $fr: black frame ($lit% lit)"
        rc=1
        r=1
    fi
    gate_artifacts_cleanup "$r"
done
exit $rc

/* rpPTankValidateFlag (retail 0x003a20e0): normalizes PTank data flags. Retail tests the ORIGINAL bits and
 * applies each mask to the running value v (checked against the asm): matrix 8 with 0x8000 gives
 * v = orig & 0x8000; then 0x20 with 0x20000, 0x40 with 0x40000, 0x80 with 0x80000 and 0x100 with
 * 0x100000 each v &= the constant flag; a matrix drops position (v &= ~1), size (v &= ~4) and
 * rotate (v &= ~0x20); a tank with neither matrix nor position gets position; and a tank with no
 * structure or array layout bit (0x30000000) gets the array layout 0x10000000. */
u32 func_003a20e0(u32 flags)
{
    const u32 orig = flags;
    u32 v = flags;

    if ((orig & 8) && (orig & 0x8000)) {
        v = orig & 0x8000;
    }
    if ((orig & 0x20) && (orig & 0x20000)) {
        v &= 0x20000;
    }
    if ((orig & 0x40) && (orig & 0x40000)) {
        v &= 0x40000;
    }
    if ((orig & 0x80) && (orig & 0x80000)) {
        v &= 0x80000;
    }
    if ((orig & 0x100) && (orig & 0x100000)) {
        v &= 0x100000;
    }
    if ((orig & 8) && (orig & 1)) {
        v &= ~1u;
    }
    if ((orig & 8) && (orig & 4)) {
        v &= ~4u;
    }
    if ((orig & 8) && (orig & 0x20)) {
        v &= ~0x20u;
    }
    if (!(orig & 8) && !(orig & 1)) {
        v |= 1;
    }
    if ((v & 0x30000000) == 0) {
        v |= 0x10000000;
    }
    return v;
}

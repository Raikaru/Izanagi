/* func_00413290: records the index of `entry` in the registry table. `reg` must be open (word 0
 * non-zero), `out` must point at a word that is still -1, and `entry` must be a live entry (its
 * word 0 non-zero) sitting exactly on a 40-byte slot of the table at +8, below the count at +4.
 * Then *out receives the slot index and reg is returned; otherwise 0. */
u8 *func_00413290(u8 *reg, u32 *out, u8 *entry)
{
    u8 *base;
    s32 index;

    if (reg == 0 || *(s32 *)reg == 0 || out == 0 || *out != 0xFFFFFFFFu || entry == 0 || *(s32 *)entry == 0) {
        return 0;
    }
    base = *(u8 **)(reg + 8);
    index = (s32)(entry - base) / 40;
    if (base + index * 40 != entry || (u32)index >= *(u32 *)(reg + 4)) {
        return 0;
    }
    *out = index;
    return reg;
}

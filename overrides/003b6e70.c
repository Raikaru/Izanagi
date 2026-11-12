/* func_003b6e70 (RpRandom module 0x10C open callback, registered by func_003b7110 through
 * func_003e8930; func_003b6e00 is the matching close): allocates the 0x7C-byte generator state
 * (hint 0x0004010C) into the module's engine-block slot at 0x008872E0 + iGpffffb618, points the
 * slot's second and third words at the state's words and its fourth at the state's end, and seeds
 * the generator with 0x9A319039. Returns arg0 on success and 0 when the allocation fails.
 *
 * This is the decomp's NON_MATCHING body (same behaviour as retail), checked against the retail
 * asm. */

s32 func_003b6e70(s32 arg0)
{
    u8 **base;
    u8 *(*alloc)(s32 size, s32 flags) = *(u8 *(**)(s32, s32))0x008873E8;

    base = (u8 **)(D_008872E0 + iGpffffb618);
    *base = alloc(0x7C, 0x4010C);
    if (*base != NULL) {
        base[1] = *base + 0xC;
        base[2] = *base;
        base[3] = *base + 0x7C;
        RpRandomSeed(0x9A319039, (u8 *)base);
        return arg0;
    }
    return 0;
}

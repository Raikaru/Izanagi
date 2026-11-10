/* RpRandomSeed (retail func_003b6f00): seeds the generator whose state is the module's
 * engine-block slot (0x008872E0 + iGpffffb618) {base, front, back, end}. base[0] = seed and
 * base[i] = base[i-1] * 0x41C64E6D + 12345 (mod 2^32) for i = 1..30. `front` is set to
 * base + 3 words and `back` to base, then the generator is run 310 times to mix the state. The
 * second argument is not used by the retail code (the callers leave their own a1 in place).
 *
 * Direct transliteration of the retail code. */

extern u32 RpRandom(void);
extern void p4pc_rng_note(s32 seed);

void RpRandomSeed(s32 arg0, u8 *arg1)
{
    u32 *base = *(u32 **)(D_008872E0 + iGpffffb618);
    s32 i;

    p4pc_rng_note(arg0);

    base[0] = (u32)arg0;
    for (i = 1; i < 31; i++)
        base[i] = base[i - 1] * 0x41C64E6D + 0x3039;
    *(u32 **)(D_008872E0 + iGpffffb618 + 4) = (u32 *)((u8 *)*(u32 **)(D_008872E0 + iGpffffb618) + 12);
    *(u32 **)(D_008872E0 + iGpffffb618 + 8) = *(u32 **)(D_008872E0 + iGpffffb618);
    for (i = 0; i < 310; i++)
        RpRandom();
}

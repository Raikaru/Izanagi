/* RpRandom (retail func_003b7060): next value of the additive lagged-Fibonacci generator. The
 * generator state is the module's engine-block slot (0x008872E0 + iGpffffb618) {base, front,
 * back, end}: the word at `front` becomes *front + *back, its value is the result (shifted right
 * by one, so 31 bits), then `front` advances one word; if it reaches `end`, it wraps to `base`
 * and `back` advances without a wrap test, otherwise `back` advances and wraps to `base` at
 * `end`.
 *
 * Direct transliteration of the retail code (the decomp's NON_MATCHING body has the same
 * behaviour). */

extern u32 p4pc_rng_draws;
extern void p4pc_rng_value(u32 draw, u32 value);
extern int p4pc_align_rng_calls_enabled;
extern void p4pc_align_rng_call(u32 draw, const void *caller);

/* Keep the opt-in return-address witness at this real call boundary at -O2,
 * including calls from the seed warmup in the same translation unit. */
extern u32 RpRandom(void) __attribute__((noinline));
u32 RpRandom(void)
{
    p4pc_rng_draws++;
    if (p4pc_align_rng_calls_enabled)
        p4pc_align_rng_call(p4pc_rng_draws, __builtin_return_address(0));
    u8 *blk = D_008872E0 + iGpffffb618;
    u32 *front = *(u32 **)(blk + 4);
    u32 *back = *(u32 **)(blk + 8);
    u32 raw;
    u32 *end;

    *front = *back + *front;
    front = *(u32 **)(D_008872E0 + iGpffffb618 + 4);
    raw = *front;
    blk = D_008872E0 + iGpffffb618;
    *(u32 **)(blk + 4) = front + 1;
    end = *(u32 **)(blk + 12);
    if (front + 1 < end) {
        *(u32 **)(blk + 8) = *(u32 **)(blk + 8) + 1;
        if (!(*(u32 **)(blk + 8) < end))
            *(u32 **)(blk + 8) = *(u32 **)blk;
    } else {
        *(u32 **)(blk + 4) = *(u32 **)blk;
        *(u32 **)(blk + 8) = *(u32 **)(blk + 8) + 1;
    }
    p4pc_rng_value(p4pc_rng_draws, (raw >> 1) & 0x7FFFFFFF);
    return (raw >> 1) & 0x7FFFFFFF;
}

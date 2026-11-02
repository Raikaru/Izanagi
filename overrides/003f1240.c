/* Palette quantizer: pick the channel whose best split (003f1030) lowers variance the
 * most and split the box rng in place along it. out receives the new upper half
 * (out = rng, out.lo[best] = split), rng keeps the lower half (rng.hi[best] = split).
 * Returns 1 when a split was made, 0 when no channel beats total[5]. Retail 003f1240. */
extern float func_003f1030(u32 *node, const s32 *rng, s32 ch, s32 *split, const float *total);

s32 func_003f1240(u32 *node, const float *total, s32 *rng, s32 *out)
{
    float vals[4];
    s32 splits[4];
    s32 best = 0;
    s32 ch;
    s32 k;

    for (ch = 0; ch < 4; ch++) {
        vals[ch] = func_003f1030(node, rng, ch, splits, total);
    }
    for (ch = 0; ch < 4; ch++) {
        if (vals[ch] < vals[best]) {
            best = ch;
        }
    }
    if (!(vals[best] < total[5])) {
        return 0;
    }

    for (k = 0; k < 8; k++) {
        out[k] = rng[k];
    }
    /* The asm loads the split with lbu: only the low byte is used. */
    out[best] = (s32)(u8)splits[best];
    rng[best + 4] = (s32)(u8)splits[best];
    return 1;
}

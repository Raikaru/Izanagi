/* Palette quantizer: binary search for the best split of box rng along channel ch.
 * The stats struct is [0]=weight, [1..4]=moments, [5]=variance. rng[0..3] is the lower
 * corner and rng[4..7] the upper corner. Each probe takes the left half (channel ch
 * below mid) from the trie with 003f0de0, the right half as total - left (003f03f0),
 * and keeps the mid with the lowest combined variance in split[ch]. Returns the best
 * variance, starting from total[5]. Retail 003f1030. */
extern void func_003f0de0(u32 *node, const s32 *box, s32 depth, const s32 *bounds, float *out);
extern void func_003f03f0(float *dst, const float *a, const float *b);

float func_003f1030(u32 *node, const s32 *rng, s32 ch, s32 *split, const float *total)
{
    s32 bounds[8];
    s32 origin[4] = { 0, 0, 0, 0 };
    float out[6];
    float dst[6];
    float best = total[5];
    s32 lo = rng[ch];
    s32 hi = rng[ch + 4];
    s32 depth = *(s32 *)(0x007690F0 - 21552);
    s32 k;

    for (k = 0; k < 8; k++) {
        bounds[k] = rng[k];
    }
    if (hi - lo < 2) {
        return best;
    }

    do {
        s32 mid = lo + ((hi - lo) >> 1);

        bounds[4 + ch] = mid;
        for (k = 0; k < 6; k++) {
            out[k] = 0.0f;
        }
        func_003f0de0(node, origin, depth, bounds, out);
        func_003f03f0(dst, total, out);

        if (out[0] == 0.0f) {
            lo = mid;
        } else if (dst[0] == 0.0f) {
            hi = mid;
        } else {
            float sum = out[5] + dst[5];

            if (sum < best) {
                best = sum;
                split[ch] = mid;
            }
            if (out[5] < dst[5]) {
                lo = mid;
            } else {
                hi = mid;
            }
        }
    } while (hi - lo >= 2);

    return best;
}

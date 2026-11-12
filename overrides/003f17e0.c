/* Palette quantizer root (retail 003f17e0). Pushes a black pixel through the trie, then
 * either (a) when nout >= the trie's leaf count, gives every leaf its own palette entry,
 * or (b) splits the colour box into at most nout buckets by repeatedly cutting the bucket
 * with the largest variance (003f1240), refreshes each bucket's stats (003f0de0), and
 * writes one palette entry per bucket (003f0c10 tags the trie leaves, 003f0510 writes the
 * colour). Returns the palette size used.
 *
 * ctx layout: root trie pointer at +16384; box k (8 s32, 32 bytes) at +32*k; split
 * variance k (float) at +8192+4*k; stats k (float[6], index byte at +24, 28 bytes) at
 * +9216+28*k. Retail 003f17e0. */
extern u32 *func_003f13b0(u32 *node, const u8 *px, s32 depth);
extern s32 func_003f1520(u32 *node, s32 depth);
extern void func_003f0de0(u32 *node, const s32 *box, s32 depth, const s32 *bounds, float *out);
extern s32 func_003f1240(u32 *node, const float *total, s32 *rng, s32 *out);
extern void func_003f0c10(u32 *node, const s32 *box, s32 depth, const s32 *bounds, s32 index);
extern s32 func_003f1600(u32 *node, u8 *pal, s32 idx, s32 depth);
extern void func_003f0510(u8 *out, const float *leaf);

static void palq17_clear_stats(float *stats)
{
    s32 i;

    for (i = 0; i < 6; i++) {
        stats[i] = 0.0f;
    }
    *((u8 *)stats + 24) = 0;
}

s32 func_003f17e0(u8 *pal, s32 nout, u8 *ctx)
{
    u32 *root = *(u32 **)(ctx + 16384);
    s32 depth = *(s32 *)(0x007690F0 - 21552);
    u8 px[4] = { 0, 0, 0, 0 };
    s32 origin[4] = { 0, 0, 0, 0 };
    s32 leaves = 0;
    s32 i;

    func_003f13b0(root, px, depth);

    for (i = 0; i < 4 * nout; i++) {
        pal[i] = 0;
    }

    if (root != 0) {
        if (depth > 0) {
            for (i = 0; i < 16; i++) {
                leaves += func_003f1520((u32 *)root[7 + i], depth - 1);
            }
        } else {
            leaves = 1;
        }
    }

    if (nout >= leaves) {
        nout = leaves;
        if (root != 0) {
            if (depth <= 0) {
                func_003f0510(pal, (const float *)root);
                *((u8 *)root + 24) = 0;
            } else {
                s32 idx = 0;

                for (i = 0; i < 16; i++) {
                    idx = func_003f1600((u32 *)root[7 + i], pal, idx, depth - 1);
                }
            }
        }
        return nout;
    }

    /* Too few palette entries for the leaves: split the box. */
    {
        s32 *box = (s32 *)ctx;
        float *var = (float *)(ctx + 8192);
        s32 cnt = 1;
        s32 k;

        box[0] = 0;
        box[1] = 0;
        box[2] = 0;
        box[3] = 0;
        box[4] = 1 << depth;
        box[5] = 1 << depth;
        box[6] = 1 << depth;
        box[7] = 1 << depth;

        palq17_clear_stats((float *)(ctx + 9216));
        func_003f0de0(root, origin, depth, box, (float *)(ctx + 9216));
        var[0] = ((float *)(ctx + 9216))[5];

        if (nout >= 2) {
            for (;;) {
                float maxv = 0.0f;
                s32 best = -1;

                for (k = 0; k < cnt; k++) {
                    if (!(var[k] <= maxv)) {
                        maxv = var[k];
                        best = k;
                    }
                }
                if (best == -1) {
                    break;
                }

                {
                    s32 *box_best = (s32 *)(ctx + 32 * best);
                    s32 *box_new = (s32 *)(ctx + 32 * cnt);
                    float *st_best = (float *)(ctx + 9216 + 28 * best);
                    float *st_new = (float *)(ctx + 9216 + 28 * cnt);

                    if (func_003f1240(root, st_best, box_best, box_new) != 0) {
                        palq17_clear_stats(st_best);
                        func_003f0de0(root, origin, depth, box_best, st_best);
                        palq17_clear_stats(st_new);
                        func_003f0de0(root, origin, depth, box_new, st_new);
                        var[best] = st_best[5];
                        var[cnt] = st_new[5];
                        cnt++;
                    } else {
                        var[best] = 0.0f;
                    }
                }

                if (!(cnt < nout)) {
                    break;
                }
            }
        }

        if (nout > 0) {
            for (k = 0; k < nout; k++) {
                func_003f0c10(root, origin, depth, (s32 *)(ctx + 32 * k), k);
                func_003f0510(pal + 4 * k, (const float *)(ctx + 9216 + 28 * k));
            }
        }
    }

    return nout;
}

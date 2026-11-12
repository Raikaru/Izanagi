/* rpPTankSkyAllocCallBack (retail 0x003a3670), the alloc callback of the sky platform: lays out the
 * tank's particle arrays and the platform's user data and returns the raw block (kept at record +0x0C
 * and freed with it), or 0 when the allocation fails.
 *
 * The retail version sizes DMA chains, per-buffer instance heads and VU1 input rings in a
 * 0xA0-stride table at 0x00707ED0 and builds the packet records the instance callbacks fill. The
 * port draws the particle arrays directly (src/render/p4gl_ptank.c), so none of that has a
 * consumer; what is kept is the layout every reader of the record sees:
 *   - pd + 0x50 the cluster count, pd + 0x54 the structure stride (0 for arrays), pd + 0x4C the
 *     user data, which holds the draw state at +0xB8 (GS ALPHA, 0x44) and +0xC8 (GS TEST, ZTE with
 *     GEQUAL) until RpPTankAtomicSkyRenderStateSet changes them, a count at +0xE8 and the zeroed
 *     table the destructors (func_003a3d50, func_003a3de0) walk;
 *   - the clusters, in the order position, matrix, normal, size, colour, vertex colour, rotate, uv2,
 *     uv4 (flag bits 1, 8, 0x10, 4, 2, 0x40, 0x20, 0x80, 0x100; element sizes 12, 64, 12, 8, 4, 16, 4,
 *     16, 32), each with its data pointer at pd + 4 + 8c and stride at pd + 8 + 8c. A structure
 *     layout (dataFlags 0x20000000) interleaves them in one stride rounded up to 16; the array layout
 *     gives each its own 16-aligned block. Everything is zeroed. */
typedef void *(*P4PtankAlloc_3670)(u32 size, u32 flags);

#define P4_PTANK_USERDATA_BYTES 0x300u

u32 func_003a3670(u8 *pd, u32 maxP, u32 dataFlags, u32 platFlags)
{
    static const u32 sBit[9] = {0x1, 0x8, 0x10, 0x4, 0x2, 0x40, 0x20, 0x80, 0x100};
    static const u32 sSize[9] = {12, 64, 12, 8, 4, 16, 4, 16, 32};
    u32 live[9];
    u32 total;
    u32 stride = 0;
    u32 count = 0;
    u32 off;
    u8 *raw;
    u8 *base;
    s32 c;

    (void)platFlags;
    for (c = 0; c < 9; c++) {
        live[c] = (dataFlags & sBit[c]) == sBit[c];
        count += live[c];
    }
    total = P4_PTANK_USERDATA_BYTES + 16;
    for (c = 0; c < 9; c++) {
        if (live[c]) {
            if (dataFlags & 0x20000000) {
                stride += sSize[c];
            } else {
                total += (maxP * sSize[c] + 15) & ~15u;
            }
        }
    }
    if (dataFlags & 0x20000000) {
        stride = (stride + 15) & ~15u;
        total += maxP * stride;
    }
    raw = (u8 *)(*(P4PtankAlloc_3670 *)0x008873E8)(total, 0x3012F);
    if (raw == (u8 *)0) {
        return 0;
    }
    __builtin_memset(raw, 0, total);
    base = (u8 *)(((u32)raw + 15) & ~15u);

    *(u32 *)(pd + 0x50) = count;
    *(u32 *)(pd + 0x54) = stride;
    *(u8 **)(pd + 0x4C) = base;
    *(u64 *)(base + 0xB8) = 0x44;
    *(u64 *)(base + 0xC8) = 0x50000;
    off = P4_PTANK_USERDATA_BYTES;
    for (c = 0; c < 9; c++) {
        if (live[c]) {
            *(u8 **)(pd + 4 + 8 * c) = base + off;
            if (dataFlags & 0x20000000) {
                *(u32 *)(pd + 8 + 8 * c) = stride;
                off += sSize[c];
            } else {
                *(u32 *)(pd + 8 + 8 * c) = sSize[c];
                off += (maxP * sSize[c] + 15) & ~15u;
            }
        }
    }
    return (u32)raw;
}

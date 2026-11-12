/* func_003a5180 (248 bytes): advances a frame index and copies one frame's three words
 * into three 0x18-byte slots. Returns 1 when the animation record is unused, else whether
 * the current frame's flag word is zero.
 *
 * obj = *(arg0 + *(s32 *)0x00764700) (gp-0x49F0; gp is 0x007690F0); dst = obj + 0x50;
 * rec = *(obj+0x9C). If rec[0xD8] == 0, returns 1. Otherwise rec[0xE4] (signed frame
 * index) becomes rec[0xE4] + 1 while it is below rec[0xE8], else 0. When rec[0xE8] != 0,
 * for j = 0, 1, 2 the words at rec + 0x24 * rec[0xE4] + 0x140 + 0xC * j + {0, 4, 8} are
 * stored at dst + 0x18 * j + {4, 0xC, 0x14}. Returns rec[0x20 * rec[0xE4] + 0x10] == 0. */

s32 func_003a5180(u8 *arg0)
{
    u8 *obj;
    u8 *rec;
    u8 *dst;
    s32 j;

    obj = *(u8 **)(arg0 + *(s32 *)0x00764700);
    rec = *(u8 **)(obj + 0x9C);
    dst = obj + 0x50;
    if (*(s32 *)(rec + 0xD8) == 0) {
        return 1;
    }
    if (*(s32 *)(rec + 0xE4) < *(s32 *)(rec + 0xE8)) {
        *(s32 *)(rec + 0xE4) = *(s32 *)(rec + 0xE4) + 1;
    } else {
        *(s32 *)(rec + 0xE4) = 0;
    }
    if (*(s32 *)(rec + 0xE8) != 0) {
        for (j = 0; j < 3; j++) {
            u8 *frame;

            frame = rec + 0x24 * *(s32 *)(rec + 0xE4) + 0xC * j;
            *(u32 *)(dst + 4) = *(u32 *)(frame + 0x140);
            frame = rec + 0x24 * *(s32 *)(rec + 0xE4) + 0xC * j;
            *(u32 *)(dst + 0xC) = *(u32 *)(frame + 0x144);
            frame = rec + 0x24 * *(s32 *)(rec + 0xE4) + 0xC * j;
            *(u32 *)(dst + 0x14) = *(u32 *)(frame + 0x148);
            dst += 0x18;
        }
    }
    return *(u32 *)(rec + 0x20 * *(s32 *)(rec + 0xE4) + 0x10) == 0;
}

/* func_003d0790 (188 bytes): consistency check of a pipeline node's cached counts. Returns 1.
 * arg0 fields: +0x0C points at an object whose first word is a flag word, +0x2C points at a
 * record whose word at +4 is a count, +0x30 points at a structure that holds a second pointer
 * (the cache) whose word at +0x24 is the expected count and word at +0x2C the expected flags,
 * +0x34 is a status word, +0x38 receives the flag word.
 *
 * count = *(+0x2C)[1]; if (flags & 2) count = (count - 2) * 3.
 * If count != cache[0x24], status |= 8.
 * If status bit 3 is still clear: *(arg0+0x38) = *(flag object); if that != cache[0x2C],
 * status |= 8. Leaf function. */

s32 func_003d0790(u8 *arg0)
{
    s32 count;
    u8 *cache;

    count = *(s32 *)(*(u8 **)(arg0 + 0x2C) + 4);
    cache = *(u8 **)*(u8 **)(arg0 + 0x30);
    if (**(u32 **)(arg0 + 0xC) & 2) {
        count = (count - 2) * 3;
    }
    if (count != *(s32 *)(cache + 0x24)) {
        *(u32 *)(arg0 + 0x34) |= 8;
    }
    if (!(*(u32 *)(arg0 + 0x34) & 8)) {
        cache = *(u8 **)*(u8 **)(arg0 + 0x30);
        *(u32 *)(arg0 + 0x38) = **(u32 **)(arg0 + 0xC);
        if (*(u32 *)(arg0 + 0x38) != *(u32 *)(cache + 0x2C)) {
            *(u32 *)(arg0 + 0x34) |= 8;
        }
    }
    return 1;
}

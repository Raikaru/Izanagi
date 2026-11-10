/* func_0039ae90 (176 bytes): duplicates a block of 20-byte entries between two owners. Both
 * keep a block header at owner + *(s32 *)0x007646E0 (gp-0x4A10; gp is 0x007690F0): +0 entry
 * count, +8 the entry array. arg1 is the source and arg0 the destination. A source count of 0 or
 * less does nothing. Otherwise 20*count bytes come from the allocator at 0x008873E8 (tag 0x30105)
 * into the destination's +8; when that fails it returns 0, else the destination's count is set and
 * the entries are copied (the port's memcpy, func_0043f810). Returns arg0. */

typedef void *(*P4AllocFn)(u32 size, u32 tag);
extern void *p4_memcpy(void *dst, const void *src, u32 n) __asm__("func_0043f810");

s32 func_0039ae90(s32 arg0, s32 arg1)
{
    u8 *src;
    u8 *dst;
    s32 count;
    s32 bytes;

    src = (u8 *)(arg1 + *(s32 *)0x007646E0);
    count = *(s32 *)src;
    dst = (u8 *)(arg0 + *(s32 *)0x007646E0);
    if (count <= 0) {
        return arg0;
    }
    bytes = count * 0x14;
    *(void **)(dst + 8) = (*(P4AllocFn *)0x008873E8)((u32)bytes, 0x30105);
    if (*(u8 **)(dst + 8) == (u8 *)0) {
        return 0;
    }
    *(s32 *)dst = *(s32 *)src;
    p4_memcpy(*(u8 **)(dst + 8), *(u8 **)(src + 8), (u32)bytes);
    return arg0;
}

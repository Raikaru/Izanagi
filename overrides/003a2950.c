/* func_003a2950 (152 bytes): updates one packed field of an object's record. Returns 1 when
 * it handled arg1, else 0.
 *
 * obj = *(arg0 + *(s32 *)0x00764700) (gp-0x49F0; gp is 0x007690F0); rec = *(obj+0x9C).
 *   arg1 == 1: 64-bit (arg2 & 0xFF) | ((u64)(arg2 & 0xFF000000) << 8) at rec+0xB8, then
 *              obj[0x40] &= 0xEFFFFFFF
 *   arg1 == 2: 64-bit at rec+0xC8 = (old & ~0xFFFF) | (arg2 & 0xFFFF)
 * Any other arg1 returns 0 without touching memory. */

s32 func_003a2950(u8 *arg0, s32 arg1, u32 arg2)
{
    u8 *obj;
    u8 *rec;

    obj = *(u8 **)(arg0 + *(s32 *)0x00764700);
    rec = *(u8 **)(obj + 0x9C);
    if (arg1 == 2) {
        *(u64 *)(rec + 0xC8) = (*(u64 *)(rec + 0xC8) & 0xFFFFFFFFFFFF0000ull) | (u64)(arg2 & 0xFFFF);
        return 1;
    }
    if (arg1 == 1) {
        *(u64 *)(rec + 0xB8) = (u64)(arg2 & 0xFF) | ((u64)(arg2 & 0xFF000000u) << 8);
        *(u32 *)(obj + 0x40) = *(u32 *)(obj + 0x40) & 0xEFFFFFFFu;
        return 1;
    }
    return 0;
}

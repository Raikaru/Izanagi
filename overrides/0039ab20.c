/* func_0039ab20 (256 bytes): stores a value into one of two 0x40-byte state slots. Returns 1
 * when it stored, 0 for an unknown selector.
 *
 * rec = *(arg0 + *(s32 *)0x007646D0) (gp-0x4A20; gp is 0x007690F0). code = *(rec+0x80).
 * slot = 1 when (code == 3 ? arg1 == 2 : code == 6), else 0; base = rec + slot*0x40.
 * arg2 selects the field:
 *   1: 64-bit (arg3 & 0xFF) | ((u64)(arg3 & 0xFF000000) << 8) at base+0x28; and when
 *      arg1 == 4 the words at base+4 and base+8 are cleared
 *   2: 64-bit (u32)arg3 at base+0x30
 *   3: 64-bit (u32)arg3 at base+0x38
 *   other: nothing, returns 0 */

s32 func_0039ab20(u8 *arg0, s32 arg1, s32 arg2, u32 arg3)
{
    u8 *rec;
    u8 *base;
    s32 code;
    s32 slot;

    rec = *(u8 **)(arg0 + *(s32 *)0x007646D0);
    code = *(s32 *)(rec + 0x80);
    if (code == 3) {
        slot = (arg1 == 2) ? 1 : 0;
    } else {
        slot = (code == 6) ? 1 : 0;
    }
    base = rec + slot * 0x40;
    switch (arg2) {
    case 1:
        *(u64 *)(base + 0x28) = (u64)(arg3 & 0xFF) | ((u64)(arg3 & 0xFF000000u) << 8);
        if (arg1 == 4) {
            *(u32 *)(base + 4) = 0;
            *(u32 *)(base + 8) = 0;
        }
        return 1;
    case 2:
        *(u64 *)(base + 0x30) = (u64)arg3;
        return 1;
    case 3:
        *(u64 *)(base + 0x38) = (u64)arg3;
        return 1;
    default:
        return 0;
    }
}

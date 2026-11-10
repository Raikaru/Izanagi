/* func_00396e80 (120 bytes): takes a0 as an index. The record is at
 * a0 + *(s32 *)0x007646C8 (gp-0x4A28). Returns 0 when the record's first
 * word is 0xFFFFFFFF and its second word is 0. Otherwise it reads the
 * second word as a pointer p: returns 12 if p is null or p's first word
 * has bit 0 set; else returns 20 + 12 * p[1].
 *
 * gp is 0x007690F0, so gp-18984 is the absolute slot 0x007646C8.
 * Leaf function: no calls, no stack frame. */

s32 func_00396e80(s32 arg0)
{
    u32 *rec;
    u32 *p;

    rec = (u32 *)(arg0 + *(s32 *)0x007646C8);
    if (rec[0] == 0xFFFFFFFFU && rec[1] == 0) {
        return 0;
    }
    p = (u32 *)rec[1];
    if (p == 0) {
        return 12;
    }
    if (p[0] & 1) {
        return 12;
    }
    return (s32)(20 + 12 * p[1]);
}

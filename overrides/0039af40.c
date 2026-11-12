/* func_0039af40 (320 bytes): writes an owner's table of 20-byte entries to a stream. arg0 is
 * the stream, arg2 the owner; arg1 is not used. The table header is at owner + *(s32 *)0x007646E0
 * (gp-0x4A10; gp is 0x007690F0): +0 entry count, +8 the entry array. Returns the stream, or 0 when
 * a write fails (func_003df240 writes a word, func_003e2ab0 a block; both return 0 on failure).
 *
 * The count is written; then for each entry a 20-byte record is written: the entry's first word,
 * its halfword +4 and halfword +6 (each sign-extended to a word), its float +8 and the index of the
 * entry that its +0x10 pointer refers to ((pointer - array) / 20, signed). */

s32 func_0039af40(s32 arg0, s32 arg1, u8 *arg2)
{
    u8 *hdr;
    s32 count;
    s32 i;
    u32 out[5];
    u8 *e;
    s32 off;

    (void)arg1;
    hdr = arg2 + *(s32 *)0x007646E0;
    count = *(s32 *)hdr;
    if (func_003df240(arg0, &count, 4) == 0) {
        return 0;
    }
    if (!(0 < count)) {
        return arg0;
    }
    off = 0;
    for (i = 0; i < count;) {
        e = *(u8 **)(hdr + 8) + off;
        out[0] = *(u32 *)e;
        out[1] = (u32)(s32) * (s16 *)(e + 4);
        out[2] = (u32)(s32) * (s16 *)(e + 6);
        out[3] = *(u32 *)(e + 8);
        out[4] = (u32)((s32)(*(u32 *)(e + 0x10) - *(u32 *)(hdr + 8)) / 20);
        if (func_003e2ab0(arg0, out, 0x14) == 0) {
            return 0;
        }
        i += 1;
        off += 0x14;
    }
    return arg0;
}

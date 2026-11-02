/* func_003de020 (224 bytes): RwFseek. arg0 is the file object, arg1 the offset, arg2 the origin
 * (0, 1 or 2). Returns 0 when the origin is valid, else -1 without calling anything.
 *
 * tbl = *(arg0+0x50) + 0x28. If *(arg0+0x30) != 0 it first sets *(arg0+0x40) = 4 and
 * *(arg0+0x38) = 3. It then calls the slot at tbl+0x10 (the seek entry) with a 16-byte scratch
 * buffer (that callee fills it in; the result is not used), the file object, the offset and a
 * mode: origin 0 -> 1, origin 1 -> 2, origin 2 -> 3. */

typedef void (*P4SeekFn)(u8 *out, u8 *file, s32 offset, s32 mode);

s32 func_003de020(u8 *arg0, s32 arg1, s32 arg2)
{
    u8 *tbl;
    P4SeekFn seek;
    u32 scratch[4];

    tbl = *(u8 **)(arg0 + 0x50) + 0x28;
    if (*(s32 *)(arg0 + 0x30) != 0) {
        *(s32 *)(arg0 + 0x40) = 4;
        *(s32 *)(arg0 + 0x38) = 3;
    }
    /* retail loads the seek slot only on the paths with a valid origin: an invalid origin returns -1 without reading it */
    if (arg2 == 0) {
        seek = *(P4SeekFn *)(tbl + 0x10);
        seek((u8 *)scratch, arg0, arg1, 1);
        return 0;
    }
    if (arg2 == 2) {
        seek = *(P4SeekFn *)(tbl + 0x10);
        seek((u8 *)scratch, arg0, arg1, 3);
        return 0;
    }
    if (arg2 == 1) {
        seek = *(P4SeekFn *)(tbl + 0x10);
        seek((u8 *)scratch, arg0, arg1, 2);
        return 0;
    }
    return -1;
}

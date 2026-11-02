/* func_003b71b0 (224 bytes): writes a table object's index block to a stream. arg0 is the stream
 * (func_003df240 writes a word and returns 0 on failure, func_003e2ab0 writes a block), arg1 the
 * object. The block header is at arg1+0x2C: +0 kind word, +4 count, +8 second count, +0xC data. Returns
 * the stream, or 0 on a failed write.
 *
 * The three header words are written; when the count at arg1+0x30 is zero that is all. Otherwise the
 * data block (func_003e2ab0(stream, data, *(arg1) + 2*count + 2*second)) follows. */

s32 func_003b71b0(s32 arg0, u8 *arg1)
{
    u8 *hdr;

    hdr = arg1 + 0x2C;
    if (func_003df240(arg0, (s32 *)hdr, 4) == 0) {
        return 0;
    }
    if (func_003df240(arg0, (s32 *)(hdr + 4), 4) == 0) {
        return 0;
    }
    if (func_003df240(arg0, (s32 *)(hdr + 8), 4) == 0) {
        return 0;
    }
    if (*(s32 *)(arg1 + 0x30) == 0) {
        return arg0;
    }
    if (func_003e2ab0(arg0, *(s32 *)(hdr + 0xC), *(s32 *)arg1 + *(s32 *)(hdr + 4) * 2 + *(s32 *)(hdr + 8) * 2) != 0) {
        return arg0;
    }
    return 0;
}

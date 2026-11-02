/* func_003b7ca0 (352 bytes): stream writer for a record. src = *(record + [0x00886498])
 * (returns the stream when src is NULL). If record flag 0x01000000 is set, the
 * result is func_003b8930(stream, record) != 0 ? stream : NULL. Otherwise it writes
 * a packed 4-byte header (src[0] | src[4]<<8 & 0xff00 | src[16]<<16 & 0xff0000),
 * then func_003e2ab0(stream, src[8], src[4]), func_003df240(stream, src[20],
 * count*4), func_003df1a0(stream, src[24], count*16), func_003df1a0(stream, src[12],
 * src[0]*64), and func_003b71b0(stream, src). Each step returns NULL on failure.
 * count = record[20]. The second argument is unused in the retail code. */

extern s32 func_003b8930(u8 *arg0, u8 *arg1);
extern s32 func_003b71b0(u8 *arg0, u8 *arg1);
extern s32 func_003e2ab0(u8 *arg0, s32 arg1, s32 arg2);
extern s32 func_003df240(s32 arg0, s32 arg1, s32 arg2);
extern s32 func_003df1a0(s32 arg0, s32 arg1, s32 arg2);

s32 *func_003b7ca0(s32 *arg0, s32 arg1, u8 *arg2)
{
    u8 *stream;
    u8 *src;
    u32 hdr;
    s32 count;

    stream = (u8 *)arg0;
    src = *(u8 **)(arg2 + *(s32 *)0x00886498);
    if (src == NULL) {
        return arg0;
    }

    if ((*(u32 *)(arg2 + 8) & 0x01000000) != 0) {
        return func_003b8930(stream, arg2) != 0 ? arg0 : NULL;
    }

    hdr = (u32)*src
        | (((u32)*(u32 *)(src + 4) << 8) & 0x0000FF00)
        | (((u32)*(u32 *)(src + 16) << 16) & 0x00FF0000);
    count = *(s32 *)(arg2 + 20);

    if (func_003df240((s32)arg0, (s32)&hdr, 4) == 0) {
        return NULL;
    }
    if (func_003e2ab0(stream, *(s32 *)(src + 8), *(s32 *)(src + 4)) == 0) {
        return NULL;
    }
    if (func_003df240((s32)arg0, *(s32 *)(src + 20), count << 2) == 0) {
        return NULL;
    }
    if (func_003df1a0((s32)arg0, *(s32 *)(src + 24), count << 4) == 0) {
        return NULL;
    }
    if (func_003df1a0((s32)arg0, *(s32 *)(src + 12), *(u32 *)src << 6) == 0) {
        return NULL;
    }
    if (func_003b71b0(stream, src) == 0) {
        return NULL;
    }
    return arg0;
}

/* func_003b5d20 (644 bytes): replicates one source word into four slots per record. arg0: +0
 * destination base, +4 slot stride S. arg1 is the record count; arg2: +0x24 source base, +0x28
 * source stride. For each of arg1 > 0 records the word at the source goes to dst, dst+S,
 * dst+2*S and dst+3*S; then dst advances by 4*S and the source by its stride. The retail code
 * unrolls the loop by eight; the result is the same for every count. Leaf function. */

void func_003b5d20(u8 *arg0, s32 arg1, u8 *arg2)
{
    u8 *dst;
    u8 *src;
    s32 stride;
    s32 srcStride;
    s32 i;

    dst = *(u8 **)arg0;
    stride = *(s32 *)(arg0 + 4);
    src = *(u8 **)(arg2 + 0x24);
    srcStride = *(s32 *)(arg2 + 0x28);
    for (i = 0; i < arg1; i++) {
        u32 w = *(u32 *)src;

        *(u32 *)dst = w;
        *(u32 *)(dst + stride) = w;
        *(u32 *)(dst + 2 * stride) = w;
        *(u32 *)(dst + 3 * stride) = w;
        src += srcStride;
        dst += 4 * stride;
    }
}

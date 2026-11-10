/* func_0039b080 (400 bytes): reads an owner's table of 20-byte entries from a stream (the mirror
 * of func_0039af40). arg0 is the stream (func_003df360 reads a word, func_003e2910 a block and
 * returns the byte count), arg2 the owner, arg1 is not used. The header is at
 * owner + *(s32 *)0x007646E0 (gp-0x4A10): +8 the entry array. Returns the stream, 0 on a read
 * failure or when func_0039b2c0 cannot allocate.
 *
 * The entry count is read; 0 or less ends the read. Otherwise func_0039b2c0(owner, count) allocates
 * the array and each 20-byte record is read (it must come back as 0x14 bytes): entry +0 = word 0,
 * halfwords +4 and +6 = words 1 and 2, +8 = word 3, +0xC = 1.0f / word 3 (as a float), +0x10 =
 * array + 20*word 4. */

s32 func_0039b080(s32 arg0, s32 arg1, u8 *arg2)
{
    u8 *hdr;
    s32 count;
    s32 i;
    s32 off;
    u32 in[5];
    u8 *e;

    (void)arg1;
    hdr = arg2 + *(s32 *)0x007646E0;
    if (func_003df360(arg0, &count, 4) == 0) {
        return 0;
    }
    if (count <= 0) {
        return arg0;
    }
    if (func_0039b2c0(arg2, count) == 0) {
        return 0;
    }
    i = 0;
    off = 0;
    if (0 < count) {
        do {
            if (func_003e2910(arg0, in, 0x14) != 0x14) {
                return 0;
            }
            i += 1;
            e = *(u8 **)(hdr + 8) + off;
            *(u32 *)e = in[0];
            *(s16 *)(e + 4) = (s16)in[1];
            *(s16 *)(e + 6) = (s16)in[2];
            *(u32 *)(e + 8) = in[3];
            *(f32 *)(*(u8 **)(hdr + 8) + off + 0xC) = 1.0f / *(f32 *)&in[3];
            *(u32 *)(*(u8 **)(hdr + 8) + off + 0x10) = *(u32 *)(hdr + 8) + (u32)(in[4] * 20);
            off += 0x14;
        } while (i < count);
    }
    return arg0;
}

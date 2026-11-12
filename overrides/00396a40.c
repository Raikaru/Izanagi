/* func_00396a40 (444 bytes): writes a frame's hierarchy link to a stream. arg0 is the stream
 * (the writer is func_003df240(stream, pointer, 4), which returns 0 on failure), arg2 the frame;
 * arg1 is not used. Returns the stream, or 0 when a write fails.
 *
 * Writes the word 0x100, then the link id at frame + *(s32 *)0x007646C8 (gp-0x4A28; gp is
 * 0x007690F0). With no hierarchy record (link +4 null) or bit 0 set in its flags word, a zero word
 * follows. Otherwise it writes record+4 (the count), the record's flags word, then word +0x20 of
 * the record's +0x20 object, and for each of the count nodes (16 bytes from record+0x10) the words at
 * +0, +4 and +8. */

s32 func_00396a40(s32 arg0, s32 arg1, u8 *arg2)
{
    u32 magic;
    u32 zero;
    u8 *link;
    u8 *rec;
    u8 *node;
    s32 i;

    (void)arg1;
    magic = 0x100;
    if (func_003df240(arg0, (s32 *)&magic, 4) == 0) {
        return 0;
    }
    link = arg2 + *(s32 *)0x007646C8;
    if (func_003df240(arg0, (s32 *)link, 4) == 0) {
        return 0;
    }
    rec = *(u8 **)(link + 4);
    if (rec == (u8 *)0 || (*(u32 *)rec & 1)) {
        zero = 0;
        if (func_003df240(arg0, (s32 *)&zero, 4) != 0) {
            return arg0;
        }
        return 0;
    }
    if (func_003df240(arg0, (s32 *)(rec + 4), 4) == 0) {
        return 0;
    }
    if (func_003df240(arg0, (s32 *)rec, 4) == 0) {
        return 0;
    }
    if (func_003df240(arg0, (s32 *)(*(u8 **)(rec + 0x20) + 0x20), 4) == 0) {
        return 0;
    }
    node = *(u8 **)(rec + 0x10);
    i = 0;
    if (0 < *(s32 *)(rec + 4)) {
        do {
            if (func_003df240(arg0, (s32 *)node, 4) == 0) {
                return 0;
            }
            if (func_003df240(arg0, (s32 *)(node + 4), 4) == 0) {
                return 0;
            }
            if (func_003df240(arg0, (s32 *)(node + 8), 4) == 0) {
                return 0;
            }
            i += 1;
            node += 0x10;
        } while (i < *(s32 *)(rec + 4));
    }
    return arg0;
}

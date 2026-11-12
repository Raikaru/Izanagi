/* HAnimDestructor (0x00396890, 164 bytes): clears a frame's hierarchy link. arg0 is the frame;
 * rec = arg0 + *(s32 *)0x007646C8 (gp-0x4A28; gp is 0x007690F0) holds +0 an id and +4 the
 * hierarchy record. When a record is linked, every node's +0xC (16-byte nodes at record+0x10,
 * count at +4) is cleared, and when the record's +0x14 is arg0 func_00397120(record) runs. The link
 * +4 is cleared and the id +0 set to -1. Returns arg0. */

s32 HAnimDestructor(s32 arg0)
{
    u8 *rec;
    u8 *hier;
    s32 i;

    rec = (u8 *)(arg0 + *(s32 *)0x007646C8);
    hier = *(u8 **)(rec + 4);
    if (hier != (u8 *)0) {
        i = 0;
        if (0 < *(s32 *)(hier + 4)) {
            s32 off = 0;

            do {
                i += 1;
                *(s32 *)(*(u8 **)(hier + 0x10) + off + 0xC) = 0;
                off += 0x10;
            } while (i < *(s32 *)(hier + 4));
        }
        if (*(s32 *)(hier + 0x14) == arg0) {
            func_00397120(hier);
        }
        *(u32 *)(rec + 4) = 0;
    }
    *(s32 *)rec = -1;
    return arg0;
}

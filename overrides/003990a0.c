/* func_003990a0 (368 bytes): byte size of the serialized form of the object's two pipeline slots. The record is the
 * pointer at arg0 + the plugin offset in 0x007646D0; 0 is returned when it is null or its word at +0x80 is 0.
 * Otherwise the size starts at 4 and each of the two slot entries (0x40 bytes each, the type word at +0x20) adds 4, then by type:
 * 1 adds 12 plus A plus B, 2 adds 12 plus A, 4 adds 12 plus A, any other type adds nothing, where A is
 * func_003e6240(entry word) + 12 when the entry word at +4 (types 1 and 2) or +0 (type 4) is non-null and else 0, and
 * B is the same for the entry word at +8 (type 1 only). */

extern u32 p4c_003e6240_003990a0(u32 p) __asm__("func_003e6240");

u32 func_003990a0(u8 *arg0)
{
    u8 *rec;
    u32 size;
    u32 a;
    u32 b;
    u32 i;
    u8 *e;
    u32 w;

    rec = *(u8 **)(arg0 + *(s32 *)0x007646D0);
    if (rec == (u8 *)0 || *(u32 *)(rec + 0x80) == 0) {
        return 0;
    }
    size = 4;
    for (i = 0; i < 2; i++) {
        e = rec + (i << 6);
        size += 4;
        switch (*(s32 *)(e + 0x20)) {
        case 4:
            a = 0;
            w = *(u32 *)e;
            if (w != 0) {
                a = p4c_003e6240_003990a0(w) + 12;
            }
            size += 12 + a;
            break;
        case 2:
            a = 0;
            w = *(u32 *)(e + 4);
            if (w != 0) {
                a = p4c_003e6240_003990a0(w) + 12;
            }
            size += 12 + a;
            break;
        case 1:
            a = 0;
            w = *(u32 *)(e + 4);
            if (w != 0) {
                a = p4c_003e6240_003990a0(w) + 12;
            }
            b = 0;
            w = *(u32 *)(e + 8);
            if (w != 0) {
                b = p4c_003e6240_003990a0(w) + 12;
            }
            size += 12 + a + b;
            break;
        default:
            break;
        }
    }
    return size;
}

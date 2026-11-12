/* func_003bc880 (312 bytes): writes a list's used entries to the stream arg1. arg0 is the list header
 * (+0 count, +4 entries of 16 bytes). Returns the (possibly advanced) stream value, or 0.
 *
 * A null header or a count of 0 or less returns 0. The entries whose word +4 is non-zero are counted; with
 * none the stream is returned unchanged. Otherwise the count is written (func_003df240; 0 on failure
 * returns 0) and each used entry goes through arg1 = func_003bc400(entry, arg1). */

extern s32 p4c_003bc400_003bc880(u8 *e, s32 s) __asm__("func_003bc400");
extern s32 p4c_003df240_003bc880(s32 stream, void *p, s32 n) __asm__("func_003df240");

s32 func_003bc880(u8 *arg0, s32 arg1)
{
    s32 count;
    s32 i;
    u8 *e;

    if (arg0 == (u8 *)0) {
        return 0;
    }
    if (*(s32 *)arg0 <= 0) {
        return 0;
    }
    count = 0;
    for (i = 0; i < *(s32 *)arg0; i++) {
        e = *(u8 **)(arg0 + 4) + 16 * i;
        if (*(u32 *)(e + 4) != 0) {
            count += 1;
        }
    }
    if (count == 0) {
        return arg1;
    }
    if (p4c_003df240_003bc880(arg1, &count, 4) == 0) {
        return 0;
    }
    for (i = 0; i < *(s32 *)arg0; i++) {
        e = *(u8 **)(arg0 + 4) + 16 * i;
        if (*(u32 *)(e + 4) != 0) {
            arg1 = p4c_003bc400_003bc880(e, arg1);
        }
    }
    return arg1;
}

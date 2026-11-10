/* Retail 004f13c0, 88 bytes. Checked native romwright R5900 draft against
 * 004f13c0..004f1414: a0/a1 are live on both valid seek tails; a2 is
 * normalized to 0, 1 or 2. Both tails preserve their callee's v0 result.
 * The promoted NON_MATCHING draft incorrectly declared only a2 and void.
 */
s32 func_004f13c0(void *handle, s32 position, s32 type)
{
    if ((u32)type > 2) {
        return func_004f53c0(P4_G2H(0x0075d3e0), 132,
                             P4_G2H(0x0075d3f8), 0, -100);
    }
    return func_004e2108(handle, position, type);
}

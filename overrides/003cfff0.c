/* func_003cfff0 (336 bytes): refreshes the dirty flags of a texture binding. arg0 + 0x18 points at the material m1 (flags at
 * +8, byte +0x18, halfword +0xC), arg1 is the binding record (+0xC the texture, +0x10 the stage, +0x14 the dirty flags, +0x18 the
 * packed key). Nothing happens when m1.flags has bit 0x1000000. Otherwise the key is
 * (m1[0x18] << 24) | ((m1.flags & 0x9F) << 16) | the halfword at texture + 6, +0x14 loses bit 0, and with a stage record s = (*(arg1[4] + 4)): when the
 * key differs from s[10] bit 4 is set if only the low 16 bits differ (bytes 2 and 3 equal) and bit 3 otherwise; a non-zero m1 halfword
 * at +0xC sets bit 3 when its bit 0 is set, else bit 1 and its value shifted by 3. Finally, with bit 0 of the word at
 * arg0 + 0x4C set, the dirty bit 2 is set unless s[16] equals the halfword at arg0 + 0x52 and s[15] the halfword at arg0 + 0x50.
 * Returns nothing. */

void func_003cfff0(u8 *arg0, u32 *arg1)
{
    u8 *m1 = *(u8 **)(arg0 + 0x18);
    u32 flags = *(u32 *)(m1 + 8);
    u32 key;
    u32 *stage;
    u32 cur;
    u32 h;
    u8 *a4;

    if ((flags & 0x1000000) != 0) {
        return;
    }
    key = ((u32)m1[0x18] << 24) | (((flags & 0x9F) & 0xFF) << 16) | (u32)*(u16 *)((u8 *)arg1[3] + 6);
    arg1[6] = key;
    arg1[5] = arg1[5] & ~1u;
    stage = *(u32 **)(arg1[4] + 4);
    if (stage == (u32 *)0) {
        return;
    }
    cur = stage[10];
    a4 = arg0 + 0x4C;
    if (key != cur) {
        if (((key >> 16) & 0xFF) == ((cur >> 16) & 0xFF) && ((key >> 24) & 0xFF) == ((cur >> 24) & 0xFF)) {
            arg1[5] = arg1[5] | 4;
        } else {
            arg1[5] = arg1[5] | 8;
        }
    }
    h = *(u16 *)(m1 + 0xC);
    if (h != 0) {
        if ((h & 1) != 0) {
            arg1[5] = arg1[5] | 8;
        } else {
            arg1[5] = arg1[5] | 2;
            arg1[5] = arg1[5] | (h << 3);
        }
    }
    if ((*(u32 *)a4 & 1) != 0) {
        s32 hv = *(s16 *)(a4 + 6);
        s32 hu = *(s16 *)(a4 + 4);

        if ((s32)stage[16] == hv && (s32)stage[15] == hu) {
            return;
        }
        arg1[5] = arg1[5] | 4;
    }
}

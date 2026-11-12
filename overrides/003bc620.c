/* func_003bc620 (288 bytes): serialized size of a name list record. 0 for a null record. Otherwise 4 + 8, plus strlen + 1
 * (strlen slot 0x008873D4) of the string at +0 when it is set, and by the type word at +4: 1 or 2 add 4 * arg0[2]; 3 adds,
 * for each of the arg0[2] strings of the array at +0xC, 4 plus strlen + 1 when the string is set; other types add nothing. */

typedef s32 (*P4StrlenFn_003bc620)(u8 *s);

s32 func_003bc620(u32 *arg0)
{
    P4StrlenFn_003bc620 *slot = (P4StrlenFn_003bc620 *)0x008873D4;
    s32 size = 0;
    s32 i;
    u32 *list;

    if (arg0 == (u32 *)0) {
        return 0;
    }
    size += 4;
    if (arg0[0] != 0) {
        size += (*slot)((u8 *)arg0[0]) + 1;
    }
    size += 8;
    if (arg0[1] == 3) {
        list = (u32 *)arg0[3];
        for (i = 0; i < (s32)arg0[2]; i++) {
            size += 4;
            if (list[i] != 0) {
                size += (*slot)((u8 *)list[i]) + 1;
            }
        }
        return size;
    }
    if (arg0[1] == 2 || arg0[1] == 1) {
        return size + (s32)(arg0[2] << 2);
    }
    return size;
}

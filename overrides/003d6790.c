/* func_003d6790 (rpuvanim, 280 bytes): stream writer, the counterpart of
 * func_003d6680. Writes a zero 4-byte header, then func_003d7ac0(obj+20, stream),
 * then for each of obj[+4] records: writes 28 bytes from record+4, and writes the
 * index (record[0] - base) / 32 as a 4-byte value (base = obj+16). Returns 1 on
 * success, 0 on any write failure. */

extern s32 func_003df240(s32 arg0, void *arg1, s32 arg2);
extern s32 func_003df1a0(s32 arg0, void *arg1, s32 arg2);
extern s32 func_003d7ac0(s32 arg0, s32 arg1);

s32 func_003d6790(u8 *arg0, u8 *arg1)
{
    s32 header;
    s32 index;
    s32 count;
    s32 i;
    u8 *base;
    u8 *record;

    header = 0;
    if (func_003df240((s32)arg1, &header, 4) == 0) {
        return 0;
    }

    if (func_003d7ac0(*(s32 *)(arg0 + 20), (s32)arg1) == 0) {
        return 0;
    }

    count = *(s32 *)(arg0 + 4);
    base = *(u8 **)(arg0 + 16);
    if (count <= 0) {
        return 1;
    }

    for (i = 0; i < count; i++) {
        record = base + i * 32;
        if (func_003df1a0((s32)arg1, record + 4, 28) == 0) {
            return 0;
        }
        index = ((s32)*(u8 **)record - (s32)base) / 32;
        if (func_003df240((s32)arg1, &index, 4) == 0) {
            return 0;
        }
    }
    return 1;
}

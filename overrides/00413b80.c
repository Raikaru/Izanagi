extern s32 func_003df590();
extern u8 *func_003df4d0();
extern u8 *func_00413350(u32 *table, s32 id, s32 flag, s32 value, u8 *owner);

/* func_00413b80: first pass over a registry's entries. `reg` has the entry count at +4 and the
 * 40-byte entry table at +8. For each entry E (record R = word 0, descriptor D = word 7 at +0x1c
 * whose word 2 is the cell table S of func_00413350):
 *  - R's list at R+0x20 (count, 12-byte items at +4) holds (id, ?, ?) items: every item with a
 *    non-zero word 1 makes sure S has a cell for its id (func_00413350(S, id, 0, 1, E)).
 *  - For each of E's links j (E+4 of them, link indices at E+8, -1 = none) pointing at entry T,
 *    every cell of T's table is looked up in R's item list. The item's state for that link is
 *    item[1][found] when the id is in R's list, else item[2]. State 2 does nothing. A non-zero
 *    state other than 2 finds or adds the id's cell in E's table S, while state 0 only finds it;
 *    the cell's mask word (+0x18) gets bit j set when it exists.
 * A cell that cannot be made reports error {1, 0x20} and returns 0x20. Returns 0. */
s32 func_00413b80(s32 reg_addr)
{
    u8 *reg = (u8 *)reg_addr;
    s32 n = *(s32 *)(reg + 4);
    u8 *entry = *(u8 **)(reg + 8);
    s32 err[2];
    u32 i;
    u32 j;
    u32 k;
    s32 *rec;
    u32 *list;
    u32 *tbl;

    do {
        rec = *(s32 **)entry;
        list = (u32 *)((u8 *)rec + 0x20);
        tbl = *(u32 **)(*(u8 **)(entry + 0x1C) + 8);
        for (i = 0; i < list[0]; i++) {
            u8 *item = (u8 *)list[1] + i * 12;

            if (*(s32 *)(item + 4) != 0) {
                if (func_00413350(tbl, *(s32 *)item, 0, 1, entry) == 0) {
                    err[0] = 1;
                    err[1] = func_003df590(0x20);
                    func_003df4d0(err);
                    return 0x20;
                }
            }
        }
        for (j = 0; j < *(u32 *)(entry + 4); j++) {
            s32 link = *(s32 *)(*(u8 **)(entry + 8) + j * 4);
            u8 *target;
            u8 *item;
            u32 *ttbl;
            u32 mask;

            if (link == -1) {
                continue;
            }
            target = *(u8 **)(reg + 8) + link * 40;
            item = (u8 *)(*(u32 *)((u8 *)*(s32 **)entry + 0x30)) + j * 12;
            ttbl = *(u32 **)(*(u8 **)(target + 0x1C) + 8);
            mask = 1u << j;
            for (k = 0; k < ttbl[0]; k++) {
                s32 id = *(s32 *)((u8 *)ttbl[3] + k * 36);
                s32 found = -1;
                s32 state;
                u8 *cell;
                u32 x;

                for (x = 0; x < list[0]; x++) {
                    if (*(s32 *)((u8 *)list[1] + x * 12) == id) {
                        found = (s32)x;
                        break;
                    }
                }
                if (found == -1) {
                    state = *(s32 *)(item + 8);
                } else {
                    state = *(s32 *)(*(u8 **)(item + 4) + found * 4);
                }
                if (state != 2) {
                    if (state == 0) {
                        u32 *stbl = *(u32 **)(*(u8 **)(entry + 0x1C) + 8);

                        cell = 0;
                        if (stbl[0] != 0) {
                            u8 *c = (u8 *)stbl[3];

                            for (x = 0; x < stbl[0]; x++, c += 36) {
                                if (*(s32 *)c == id) {
                                    cell = c;
                                    break;
                                }
                            }
                        }
                    } else {
                        cell = func_00413350(*(u32 **)(*(u8 **)(entry + 0x1C) + 8), id, 0, 1, entry);
                        if (cell == 0) {
                            err[0] = 1;
                            err[1] = func_003df590(0x20);
                            func_003df4d0(err);
                            return 0x20;
                        }
                    }
                    if (cell != 0) {
                        *(u32 *)(cell + 0x18) |= mask;
                    }
                }
            }
        }
        entry += 40;
        n--;
    } while (n != 0);
    return 0;
}

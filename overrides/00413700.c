extern s32 func_003df590();
extern u8 *func_003df4d0();
extern u32 func_00411740(s32 size);
extern s32 func_00411820(s32 reg);
extern u8 *func_00413350(u32 *table, s32 id, s32 flag, s32 value, u8 *owner);

/* func_00413700: builds each registry entry's cell table, from the last entry to the first.
 * `reg` has the entry count at +4 and the 40-byte entry table at +8; an entry E has record
 * R = E[0], links (count E[1], indices at E+8), and descriptor E[7] whose word 2 is the cell
 * table S. For each entry:
 *  - R's item list (count at R+0x20, 12-byte items at R+0x24, word 0 = key pointer) is checked: a
 *    null key reports error {1, 31 (R word 0)} and returns 31, a repeated key reports
 *    {1, 30 (R word 0, the key's first word)} and returns 30.
 *  - S is allocated: a 20-byte header (count 0, capacity F = func_00411820(reg), owner E) with F
 *    36-byte cells; failure reports {1, 0x20} and returns 0x20.
 *  - For each link j to entry T, every cell C of T's table is looked up in R's items; the item's
 *    state is item[1][found] or item[2]. State 0 adds C to S (func_00413350(S, C.id, C.word1,
 *    E[7] word 0, C.owner)); a cell whose word 1 is 1 with a state other than 1 reports error
 *    {1, 29 ...} and returns 29; a state 2 on another cell runs func_00413410(reg, T, ...) with
 *    the entry. (Registered cells' word 1 values: 1 = declared, 2 = optional.)
 *  - Each item with a non-zero value in R's value array (R+0x28) is added to S with
 *    func_00413350(S, item key, value, E[7] word 0, E).
 * Returns 0. */
s32 func_00413700(s32 reg_addr)
{
    u8 *reg = (u8 *)reg_addr;
    s32 n = *(s32 *)(reg + 4);
    u8 *entry = *(u8 **)(reg + 8) + (n - 1) * 40;
    s32 err[2];

    do {
        u8 *rec = *(u8 **)entry;
        u32 *list = (u32 *)(rec + 0x20);
        u32 i;
        u32 j;
        u32 *node;
        s32 f;

        for (i = 0; i < list[0]; i++) {
            u32 *item = (u32 *)((u8 *)list[1] + i * 12);
            u32 k;

            if (item[0] == 0) {
                err[0] = 1;
                err[1] = func_003df590(31, *(s32 *)rec);
                func_003df4d0(err);
                return 31;
            }
            for (k = i + 1; k < list[0]; k++) {
                u32 *other = (u32 *)((u8 *)list[1] + k * 12);

                if (item[0] == other[0]) {
                    err[0] = 1;
                    err[1] = func_003df590(30, *(s32 *)rec, *(s32 *)other[0]);
                    func_003df4d0(err);
                    return 30;
                }
            }
        }

        f = func_00411820((s32)reg);
        node = (u32 *)func_00411740(20);
        if (node != 0) {
            node[3] = func_00411740(f * 36);
            if (node[3] != 0) {
                node[1] = f;
                node[0] = 0;
                node[4] = (u32)entry;
                node[2] = 0;
            } else {
                node = 0;
            }
        }
        *(u32 **)(*(u8 **)(entry + 0x1C) + 8) = node;
        if (node == 0) {
            err[0] = 1;
            err[1] = func_003df590(0x20);
            func_003df4d0(err);
            return 0x20;
        }

        for (j = 0; j < *(u32 *)(entry + 4); j++) {
            s32 link = *(s32 *)(*(u8 **)(entry + 8) + j * 4);
            u8 *target;
            u8 *item;
            u32 *ttbl;
            u32 k;

            if (link == -1) {
                continue;
            }
            target = *(u8 **)(reg + 8) + link * 40;
            item = *(u8 **)(*(u8 **)entry + 0x30) + j * 12;
            ttbl = *(u32 **)(*(u8 **)(target + 0x1C) + 8);
            for (k = 0; k < ttbl[0]; k++) {
                s32 *cell = (s32 *)(ttbl[3] + k * 36);
                s32 found = -1;
                s32 state;
                u32 x;

                for (x = 0; x < list[0]; x++) {
                    if (*(s32 *)((u8 *)list[1] + x * 12) == cell[0]) {
                        found = (s32)x;
                        break;
                    }
                }
                if (found == -1) {
                    state = *(s32 *)(item + 8);
                } else {
                    state = *(s32 *)(*(u8 **)(item + 4) + found * 4);
                }
                if (state != 0) {
                    if (cell[1] == 1) {
                        if (state != 1) {
                            err[0] = 1;
                            err[1] = func_003df590(29, *(s32 *)cell[0], **(s32 **)cell[8], *(s32 *)rec, (s32)j,
                                                   *(s32 *)item);
                            func_003df4d0(err);
                            return 29;
                        }
                    } else if (state == 2) {
                        func_00413410(reg, target, cell[0]);
                    }
                } else {
                    if (func_00413350(*(u32 **)(*(u8 **)(entry + 0x1C) + 8), cell[0], cell[1],
                                      *(s32 *)*(u8 **)(entry + 0x1C), (u8 *)cell[8]) == 0) {
                        err[0] = 1;
                        err[1] = func_003df590(0x20);
                        func_003df4d0(err);
                        return 0x20;
                    }
                }
                ttbl = *(u32 **)(*(u8 **)(target + 0x1C) + 8);
            }
        }

        for (i = 0; i < list[0]; i++) {
            s32 value = *(s32 *)(*(u8 **)(rec + 0x28) + i * 4);

            if (value != 0 &&
                func_00413350(*(u32 **)(*(u8 **)(entry + 0x1C) + 8), *(s32 *)((u8 *)list[1] + i * 12), value,
                              *(s32 *)*(u8 **)(entry + 0x1C), entry) == 0) {
                err[0] = 1;
                err[1] = func_003df590(0x20);
                func_003df4d0(err);
                return 0x20;
            }
        }
        entry -= 40;
        n--;
    } while (n != 0);
    return 0;
}

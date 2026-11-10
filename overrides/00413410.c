/* func_00413410: drops one use of cell `id` from `entry`'s cell table (descriptor word 7 -> word
 * 2; count at +0, 36-byte cells at +0xC; a cell's use count is word 2). When the count reaches 0
 * the cell is removed (the last cell moves into its slot, the table count shrinks) and the removal
 * is propagated: for each link j of `entry` (link indices at entry+8, -1 = none) to entry T, the
 * item j of the record's item array (R+0x30, 12-byte items) gives the state of the id on that link
 * (item word 2 when the id is not in R's list at R+0x20/+0x24, else item word 1 [index]); when the
 * state is 0 the same removal runs on T. */
void func_00413410(u8 *reg, u8 *entry, s32 id)
{
    u8 *rec = *(u8 **)entry;
    u32 *tbl = *(u32 **)(*(u8 **)(entry + 0x1C) + 8);
    s32 *cell = 0;
    u32 i;
    u32 j;

    if (tbl[0] != 0) {
        s32 *c = (s32 *)tbl[3];

        for (i = 0; i < tbl[0]; i++, c += 9) {
            if (c[0] == id) {
                cell = c;
                break;
            }
        }
    }
    if (cell == 0) {
        return;
    }
    cell[2] = cell[2] - 1;
    if (cell[2] != 0) {
        return;
    }
    tbl = *(u32 **)(*(u8 **)(entry + 0x1C) + 8);
    if (cell != (s32 *)(tbl[3] + (tbl[0] - 1) * 36)) {
        s32 *last = (s32 *)(tbl[3] + tbl[0] * 36) - 9;

        for (i = 0; i < 9; i++) {
            cell[i] = last[i];
        }
    }
    tbl[0] = tbl[0] - 1;
    for (j = 0; j < *(u32 *)(entry + 4); j++) {
        s32 link = *(s32 *)(*(u8 **)(entry + 8) + j * 4);
        u8 *item;
        s32 found = -1;
        s32 state;

        if (link == -1) {
            continue;
        }
        item = *(u8 **)(rec + 0x30) + j * 12;
        if (*(u32 *)(rec + 0x20) != 0) {
            for (i = 0; i < *(u32 *)(rec + 0x20); i++) {
                if (*(s32 *)(*(u8 **)(rec + 0x24) + i * 12) == id) {
                    found = (s32)i;
                    break;
                }
            }
        }
        if (found == -1) {
            state = *(s32 *)(item + 8);
        } else {
            state = *(s32 *)(*(u8 **)(item + 4) + found * 4);
        }
        if (state == 0) {
            func_00413410(reg, *(u8 **)(reg + 8) + link * 40, id);
        }
    }
}

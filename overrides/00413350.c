/* func_00413350: finds or adds the 36-byte cell with identifier `id` in `table` (count at +0,
 * cell array at +0xC). A cell found keeps its contents; when `flag` is 1 its word 1 is set to 1
 * and its owner word (+0x20) to `owner`. A missing cell is appended: word 0 = id, 1 = flag,
 * 2 = value, 3 = 0, 4 = 0, 5 = table, 6 = 0 (mask), 7 = -1, 8 = owner, and the count grows.
 * Returns the cell. Retail takes `owner` in $t0, the fifth argument register. */
u8 *func_00413350(u32 *table, s32 id, s32 flag, s32 value, u8 *owner)
{
    u32 count = table[0];
    u32 i;
    s32 *cell = 0;

    if (count != 0) {
        cell = (s32 *)table[3];
        for (i = 0; i < count; i++, cell += 9) {
            if (cell[0] == id) {
                break;
            }
        }
        if (i == count) {
            cell = 0;
        }
    }
    if (cell != 0) {
        if (flag == 1) {
            cell[1] = 1;
            cell[8] = (s32)owner;
        }
        return (u8 *)cell;
    }
    cell = (s32 *)(table[3] + count * 36);
    table[0] = count + 1;
    cell[0] = id;
    cell[1] = flag;
    cell[2] = value;
    cell[4] = 0;
    cell[5] = (s32)table;
    cell[3] = 0;
    cell[6] = 0;
    cell[7] = -1;
    cell[8] = (s32)owner;
    return (u8 *)cell;
}

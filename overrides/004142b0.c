extern void func_004156e0(void *base, u32 n, u32 size, s32 key_offset, s32 lo, s32 hi);

/* func_004142b0: collects the live cells (word 1 equal to 1 or 2) of the first entry's cell table
 * (reg+8 -> entry word 7 -> word 2; 36-byte cells) into the 12-byte array at reg+0x1c as
 * (cell word 0, cell word 1, cell word 7), stores the count at reg+0x18, and sorts the array by
 * its first word with func_004156e0. */
void func_004142b0(u8 *reg)
{
    u8 *entry = *(u8 **)(reg + 8);
    u32 *tbl = *(u32 **)(*(u8 **)(entry + 0x1C) + 8);
    s32 live = 0;
    u32 i;

    for (i = 0; i < tbl[0]; i++) {
        s32 *cell = (i < tbl[0]) ? (s32 *)(tbl[3] + i * 36) : 0;

        if ((u32)(cell[1] - 1) < 2) {
            s32 *out = (s32 *)(*(u8 **)(reg + 0x1C) + live * 12);

            live++;
            out[0] = cell[0];
            out[1] = cell[1];
            out[2] = cell[7];
        }
        tbl = *(u32 **)(*(u8 **)(entry + 0x1C) + 8);
    }
    *(s32 *)(reg + 0x18) = live;
    func_004156e0(*(void **)(reg + 0x1C), *(u32 *)(reg + 0x18), 12, 0, 0, -1);
}

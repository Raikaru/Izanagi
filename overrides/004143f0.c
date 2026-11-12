extern u32 func_00411740(s32 size);
extern void *func_0043f9c8(void *dst, s32 c, u32 n);

/* func_004143f0: lays out a registry after grouping (list = group nodes from func_00413e50).
 * `reg` has the entry count at +4, the 40-byte entry table at +8, the per-entry plugin count at
 * +0xC, a header at +0x14, the live-cell count at +0x18 and the 12-byte live-cell array at +0x1c.
 *  - s3 counts the first entry's live cells (word 1 equal to 1 or 2) for the array allocated at
 *    the end through the bump allocator func_004117b0.
 *  - The allocator's free area is cleared (func_0043f9c8 over [func_004117b0(0), func_00411740(0))).
 *    reg+0x14 gets a header of (reg[3] - 1) * 28 + 48 bytes (halfword at +2 = reg[3], +4 = reg).
 *    func_004141e0 then builds the array of distinct group keys (callback func_00414390 allocates
 *    8-byte pairs (key, key word 2)), the pair array start going to `pairs`.
 *  - For each entry E: E[3] (reg[3] pointers) and E[4] (reg[3] + 1 words) are allocated, E[6]
 *    (R[0x20] words) when the record R = E[0] has items, and E[5] (R[0x34] rounded up to 4 bytes)
 *    when it has data. For each cell of E's table: E[3][cell[7]] = pair for cell[0] (0 if none),
 *    E[4][cell[7] + 1] = cell[6]; E[4][0] = AND of all cell[6]. Then for each of R's items
 *    (12-byte, R[0x24]) E[6][i] is the index of the first non-null pair in E[3] whose key equals
 *    the item's word 0, else -1.
 *  - Finally the live array is allocated and func_004142b0 fills and sorts it. */
void func_004143f0(s32 reg_addr, s32 list_addr)
{
    u8 *reg = (u8 *)reg_addr;
    u32 *tbl;
    s32 live = 0;
    s32 groups;
    u32 free_end;
    u8 *free_start;
    s32 *pairs = 0;
    u32 i;
    u32 e;
    u32 c;

    tbl = *(u32 **)(*(u8 **)(*(u8 **)(reg + 8) + 0x1C) + 8);
    for (i = 0; i < tbl[0]; i++) {
        s32 *cell = (i < tbl[0]) ? (s32 *)(tbl[3] + i * 36) : 0;

        if ((u32)(cell[1] - 1) < 2) {
            live++;
        }
    }
    groups = func_004141e0((u32 *)list_addr, 0, 0);
    free_end = func_00411740(0);
    free_start = (u8 *)func_004117b0(0);
    func_0043f9c8(free_start, 0, free_end - (u32)free_start);
    *(u8 **)(reg + 0x14) = (u8 *)func_004117b0((*(s32 *)(reg + 0xC) - 1) * 0x1C + 0x30);
    *(s16 *)(*(u8 **)(reg + 0x14) + 2) = (s16)*(s32 *)(reg + 0xC);
    *(u8 **)(*(u8 **)(reg + 0x14) + 4) = reg;
    *(s32 *)(reg + 0x10) = 0;
    func_004141e0((u32 *)list_addr, (s32 (*)(u32, s32, void *))func_00414390, &pairs);

    for (e = 0; e < *(u32 *)(reg + 4); e++) {
        u8 *entry = *(u8 **)(reg + 8) + e * 40;
        u8 *rec = *(u8 **)entry;
        u32 mask;
        u32 k;

        if (*(s32 *)(reg + 0xC) != 0) {
            *(u8 ***)(entry + 0xC) = func_004117b0(*(s32 *)(reg + 0xC) << 2);
        }
        *(u8 ***)(entry + 0x10) = func_004117b0((*(s32 *)(reg + 0xC) + 1) * 4);
        if (*(s32 *)(rec + 0x20) != 0) {
            *(u8 ***)(entry + 0x18) = func_004117b0(*(s32 *)(rec + 0x20) << 2);
        }
        if (*(s32 *)(rec + 0x34) != 0) {
            *(u8 ***)(entry + 0x14) = func_004117b0((*(s32 *)(rec + 0x34) + 3) & ~3);
        }
        mask = 0xFFFFFFFFu;
        tbl = *(u32 **)(*(u8 **)(entry + 0x1C) + 8);
        for (c = 0; c < tbl[0]; c++) {
            s32 *cell = (c < tbl[0]) ? (s32 *)(tbl[3] + c * 36) : 0;
            s32 *pair = 0;
            u32 x;

            for (x = 0; x < (u32)groups; x++) {
                if (pairs[x * 2] == cell[0]) {
                    pair = pairs + x * 2;
                    break;
                }
            }
            (*(s32 ***)(entry + 0xC))[cell[7]] = (s32 *)pair;
            (*(s32 **)(entry + 0x10))[cell[7] + 1] = cell[6];
            mask &= (u32)cell[6];
            tbl = *(u32 **)(*(u8 **)(entry + 0x1C) + 8);
        }
        (*(u32 **)(entry + 0x10))[0] = mask;
        for (k = 0; k < *(u32 *)(rec + 0x20); k++) {
            u32 a;

            (*(s32 **)(entry + 0x18))[k] = -1;
            for (a = 0; a < *(u32 *)(reg + 0xC); a++) {
                s32 *slot = (*(s32 ***)(entry + 0xC))[a];

                if (slot != 0 && slot[0] == *(s32 *)(*(u8 **)(rec + 0x24) + k * 12)) {
                    (*(s32 **)(entry + 0x18))[k] = (s32)a;
                    break;
                }
            }
        }
    }
    if (live != 0) {
        *(u8 ***)(reg + 0x1C) = func_004117b0(live * 12);
    }
    func_004142b0(reg);
}

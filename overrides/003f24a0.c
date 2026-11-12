/* func_003f24a0 (288 bytes): builds a table of 2^count 64-bit masks at 0x0088B400
 * (row t2: for each bit t1 set in t2, OR the sign-extended 32-bit value
 * 1 << (4 * (count - 1 - t1)) into the row), then creates a pool
 * func_003e1210(92, 1024, 4, 0x30411) into obj+16388, allocates the table block
 * at obj+16384 through D_008873F8[0](obj+16388, 0x30411), and zeroes 64 bytes
 * starting at table+28. Returns 1. count = [gp-21552]. */

extern u8 *func_003e1210(s32 arg0, s32 arg1, s32 arg2, s32 arg3);

s32 func_003f24a0(u8 *arg0)
{
    s32 count;
    s32 rows;
    s32 t1;
    s32 t2;
    s64 acc;
    s64 *masks;
    u32 *tbl;

    count = *(s32 *)(0x007690F0 - 21552);
    rows = (s32)((u32)1 << (count & 31));

    masks = (s64 *)0x0088B400;
    if (rows != 0) {
        for (t2 = 0; t2 < rows; t2++) {
            acc = 0;
            if (count != 0) {
                for (t1 = 0; t1 < count; t1++) {
                    if ((t2 & ((s32)1 << (t1 & 31))) != 0) {
                        s32 sh = ((count - 1 - t1) * 4) & 31;
                        acc |= (s64)(s32)((u32)1 << sh);
                    }
                }
            }
            masks[t2] = acc;
        }
    }

    *(u32 *)(arg0 + 16388) = (u32)func_003e1210(92, 1024, 4, 0x00030411);
    *(u32 *)(arg0 + 16384) = (u32)D_008873F8[0](*(u8 **)(arg0 + 16388), 0x00030411);

    tbl = *(u32 **)(arg0 + 16384);
    for (t1 = 0; t1 < 16; t1 += 8) {
        tbl[7 + t1] = 0;
        tbl[8 + t1] = 0;
        tbl[9 + t1] = 0;
        tbl[10 + t1] = 0;
        tbl[11 + t1] = 0;
        tbl[12 + t1] = 0;
        tbl[13 + t1] = 0;
        tbl[14 + t1] = 0;
    }
    return 1;
}

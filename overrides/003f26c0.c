/* func_003f26c0 (160 bytes): teardown of the per-object table at obj+16384.
 * If the table is set: for 16 entries (tbl[7 + i]) call func_003f25c0(obj, entry,
 * count - 1), but only when count = [gp-21552] > 0; then jtbl_008873FC[0](obj+16388
 * value, tbl). Always clears obj+16384 and calls func_003e12f0(obj+16388 value)
 * (RwFreeListDestroy). */

extern void func_003f25c0(u8 *arg0, void *arg1, s32 arg2);

void func_003f26c0(u8 *arg0)
{
    u32 *tbl;
    s32 count;
    s32 i;

    tbl = *(u32 **)(arg0 + 16384);
    count = *(s32 *)(0x007690F0 - 21552);
    if (tbl != NULL) {
        if (count > 0) {
            for (i = 0; i < 16; i++) {
                func_003f25c0(arg0, (void *)tbl[i + 7], count - 1);
            }
        }
        jtbl_008873FC[0](*(u8 **)(arg0 + 16388), (u8 *)tbl);
    }

    *(u32 *)(arg0 + 16384) = 0;
    func_003e12f0(*(s32 *)(arg0 + 16388));
}

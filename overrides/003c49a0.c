/* func_003c49a0 (152 bytes): empties a pointer array (arg0: +0 data, +4 count, +8 capacity). Each of the
 * count entries goes through func_003c4220 and is then cleared; the data block is freed through the slot
 * 0x008873EC and cleared. Count and capacity are cleared and arg0 is returned. */

extern s32 p4c_003c4220_003c49a0(u8 *obj) __asm__("func_003c4220");

typedef void (*P4FreeFn_003c49a0)(void *p);

u8 *func_003c49a0(u8 *arg0)
{
    u32 *data;
    s32 n;
    s32 i;

    data = *(u32 **)arg0;
    if (data != (u32 *)0) {
        n = *(s32 *)(arg0 + 4);
        if (n > 0) {
            i = 0;
            do {
                p4c_003c4220_003c49a0((u8 *)data[i]);
                data[i] = 0;
                i += 1;
            } while (i < n);
        }
        (*(P4FreeFn_003c49a0 *)0x008873EC)(data);
        *(u32 *)arg0 = 0;
    }
    *(u32 *)(arg0 + 4) = 0;
    *(u32 *)(arg0 + 8) = 0;
    return arg0;
}

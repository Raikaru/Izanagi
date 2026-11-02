/* func_003cdfa0 (172 bytes): destroys a table of objects (arg0: +0 count, +4.. the object pointers) from
 * the last entry to the first, then frees the table through the slot 0x008873EC. An object whose first
 * word is set only has its word +0x10 cleared; one whose word +4 is set likewise; an object with neither
 * goes through func_003cc6e0 and func_003e1ea0. */

extern void p4c_003cc6e0_003cdfa0(u8 *p) __asm__("func_003cc6e0");
extern void p4c_003e1ea0_003cdfa0(u32 p) __asm__("func_003e1ea0");

typedef void (*P4FreeFn_003cdfa0)(void *p);

void func_003cdfa0(u8 *arg0)
{
    s32 i;

    if (*(s32 *)arg0 != 0) {
        i = *(s32 *)arg0 - 1;
        for (;;) {
            u8 *obj = *(u8 **)(arg0 + 4 + 4 * i);

            if (obj != (u8 *)0) {
                if (*(u32 *)obj != 0 || *(u32 *)(obj + 4) != 0) {
                    *(u32 *)(obj + 0x10) = 0;
                } else {
                    p4c_003cc6e0_003cdfa0(obj);
                    p4c_003e1ea0_003cdfa0((u32)obj);
                }
            }
            if (i == 0) {
                break;
            }
            i -= 1;
        }
    }
    (*(P4FreeFn_003cdfa0 *)0x008873EC)(arg0);
}

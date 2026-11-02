/* _rpObjectGetMeshCacheResize (0x003ce050, 284 bytes): (re)creates the mesh cache table in the slot arg0
 * for arg1 entries. Returns the slot.
 *
 * A previous table (first word = entry count, entries from +4) is released from its last entry to its first: an entry with
 * word +0 or +4 set only has +0x10 cleared, any other goes through func_003cc6e0 and func_003e1ea0; the
 * table is freed through the slot 0x008873EC and the slot cleared. The new table is 8 + 4 * (arg1 - 1)
 * bytes from 0x008873E8 (tag 0x3050D); its first word is arg1 and the arg1 entry words are zeroed
 * (func_0043f9c8 is the port's memset). */

extern void p4c_003cc6e0_003ce050(u8 *p) __asm__("func_003cc6e0");
extern void p4c_003e1ea0_003ce050(u32 p) __asm__("func_003e1ea0");
extern void *p4c_0043f9c8_003ce050(void *dst, s32 c, u32 n) __asm__("func_0043f9c8");

typedef void *(*P4AllocFn_003ce050)(u32 size, u32 tag);
typedef void (*P4FreeFn_003ce050)(void *p);

u8 *func_003ce050(u8 *arg0, s32 arg1)
{
    u8 *table;
    s32 i;

    table = *(u8 **)arg0;
    if (table != (u8 *)0) {
        if (*(s32 *)table != 0) {
            i = *(s32 *)table - 1;
            for (;;) {
                u8 *obj = *(u8 **)(table + 4 + 4 * i);

                if (obj != (u8 *)0) {
                    if (*(u32 *)obj != 0 || *(u32 *)(obj + 4) != 0) {
                        *(u32 *)(obj + 0x10) = 0;
                    } else {
                        p4c_003cc6e0_003ce050(obj);
                        p4c_003e1ea0_003ce050((u32)obj);
                    }
                }
                if (i == 0) {
                    break;
                }
                i -= 1;
            }
        }
        (*(P4FreeFn_003ce050 *)0x008873EC)(table);
        *(u32 *)arg0 = 0;
    }
    *(void **)arg0 = (*(P4AllocFn_003ce050 *)0x008873E8)((u32)(((arg1 - 1) << 2) + 8), 0x3050D);
    if (*(u32 *)arg0 != 0) {
        **(s32 **)arg0 = arg1;
        p4c_0043f9c8_003ce050(*(u8 **)arg0 + 4, 0, (u32)(arg1 << 2));
    }
    return arg0;
}

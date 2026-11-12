/* func_003c9d40 (320 bytes): destroys a scene object. Always returns 1.
 * The list at 0x008872E0 + *(s32 *)0x007647F0 (gp-0x4900) + 4 (a circular list of 0x10-byte records, the
 * record at node - 8) is searched for the record whose word +0 is arg0; it is unlinked
 * (record[0xC] -> record[8]) and returned to the freelist through the slot 0x008873FC (pool = the word at
 * 0x008872E0 + that offset). Then func_003c9a80(arg0) and func_003c49a0(arg0 + 0x10) run. With bit 0 of byte +3 set,
 * func_003c91d0(arg0[0x1C]) runs when set; otherwise func_003c9300(arg0[0x1C]) does. Then
 * func_003e3c20(0x0070B060, arg0) and the object is freed through the slot 0x008873EC. */

extern void p4c_003c49a0_003c9d40(void *p) __asm__("func_003c49a0");
extern void p4c_003c91d0_003c9d40(u32 p) __asm__("func_003c91d0");
extern void p4c_003c9300_003c9d40(u8 *n) __asm__("func_003c9300");
extern void p4c_003c9a80_003c9d40(u8 *o) __asm__("func_003c9a80");
extern void p4c_003e3c20_003c9d40(u32 list, void *obj) __asm__("func_003e3c20");

typedef void (*P4FreeListFn_003c9d40)(u32 pool, u8 *rec);
typedef void (*P4FreeFn_003c9d40)(void *p);

s32 func_003c9d40(u8 *arg0)
{
    u8 *list;
    u8 *head;
    u8 *node;

    list = (u8 *)(0x008872E0 + *(s32 *)0x007647F0);
    head = list + 4;
    node = *(u8 **)head;
    if (node != head) {
        for (;;) {
            u8 *rec = node - 8;

            if (*(u8 **)rec == arg0) {
                **(u32 **)(rec + 0xC) = *(u32 *)(rec + 8);
                *(u32 *)(*(u8 **)(rec + 8) + 4) = *(u32 *)(rec + 0xC);
                (*(P4FreeListFn_003c9d40 *)0x008873FC)(*(u32 *)(0x008872E0 + *(s32 *)0x007647F0), rec);
                break;
            }
            node = *(u8 **)node;
            if (node == head) {
                break;
            }
        }
    }
    p4c_003c9a80_003c9d40(arg0);
    p4c_003c49a0_003c9d40(arg0 + 0x10);
    if (*(u8 *)(arg0 + 3) & 1) {
        if (*(u32 *)(arg0 + 0x1C) != 0) {
            p4c_003c91d0_003c9d40(*(u32 *)(arg0 + 0x1C));
        }
    } else if (*(u32 *)(arg0 + 0x1C) != 0) {
        p4c_003c9300_003c9d40(*(u8 **)(arg0 + 0x1C));
    }
    p4c_003e3c20_003c9d40(0x0070B060, arg0);
    (*(P4FreeFn_003c9d40 *)0x008873EC)(arg0);
    return 1;
}

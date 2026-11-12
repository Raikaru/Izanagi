/* func_003d7c50 (120 bytes): drops one reference of the object at arg0[0x14] (count at +0x40). When it
 * reaches zero the object is returned to the freelist through the slot 0x008873FC (pool *(u32 *)0x00764838 =
 * gp-0x48B8; gp is 0x007690F0) and func_003d5300(arg0) runs. Returns 1. */

extern void p4c_003d5300_003d7c50(u8 *obj) __asm__("func_003d5300");

typedef void (*P4FreeListFn_003d7c50)(u32 pool, u8 *obj);

s32 func_003d7c50(u8 *arg0)
{
    u8 *obj;

    obj = *(u8 **)(arg0 + 0x14);
    *(s32 *)(obj + 0x40) = *(s32 *)(obj + 0x40) - 1;
    if (*(s32 *)(obj + 0x40) == 0) {
        (*(P4FreeListFn_003d7c50 *)0x008873FC)(*(u32 *)0x00764838, *(u8 **)(arg0 + 0x14));
        p4c_003d5300_003d7c50(arg0);
    }
    return 1;
}

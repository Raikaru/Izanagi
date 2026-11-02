/* func_003c4220 (136 bytes): drops one reference of the object arg0 (halfword +0x18). With a count above
 * 1 it only decrements; at 1 func_003e3c20(0x0070AFF0, obj) and func_003c42b0(obj, 0) run and the object
 * is returned to the freelist through the slot 0x008873FC (pool = *(0x008872E0 + *(s32 *)0x007647D0),
 * gp-0x4920). Returns 1. */

extern void p4c_003c42b0_003c4220(u8 *obj, s32 flag) __asm__("func_003c42b0");
extern void p4c_003e3c20_003c4220(u32 list, void *obj) __asm__("func_003e3c20");

typedef void (*P4FreeListFn_003c4220)(u32 pool, u8 *obj);

s32 func_003c4220(u8 *arg0)
{
    if (*(s16 *)(arg0 + 0x18) != 1) {
        *(s16 *)(arg0 + 0x18) = (s16)(*(s16 *)(arg0 + 0x18) - 1);
        return 1;
    }
    p4c_003e3c20_003c4220(0x0070AFF0, arg0);
    p4c_003c42b0_003c4220(arg0, 0);
    (*(P4FreeListFn_003c4220 *)0x008873FC)(*(u32 *)(0x008872E0 + *(s32 *)0x007647D0), arg0);
    return 1;
}

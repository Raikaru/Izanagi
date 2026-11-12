/* func_003c66c0 (240 bytes): destroys a pool pair and its object table; returns 0. arg1 holds two pools: +4 (nodes)
 * and +0 (objects). The linked list starting at the node arg2 (next pointer at +0xC) is returned node by node to the
 * pool at arg1[1] through the pool-free slot 0x008873FC, then func_003e12f0(arg1[1]) destroys that pool and clears arg1[1]. Then
 * the first arg0 entries of the array arg3 are returned to the pool at arg1[0] and cleared, func_003e12f0(arg1[0]) destroys
 * it and clears arg1[0], and arg3 is released through the free slot 0x008873EC. */

extern void p4c_003e12f0_003c66c0(u32 pool) __asm__("func_003e12f0");

typedef void (*P4PoolFreeFn_003c66c0)(u32 pool, u32 obj);
typedef void (*P4FreeFn_003c66c0)(void *p);

s32 func_003c66c0(u32 arg0, u32 *arg1, u8 *arg2, u32 *arg3)
{
    u8 *next;
    u32 i;

    while (arg2 != (u8 *)0) {
        next = *(u8 **)(arg2 + 0xC);
        (*(P4PoolFreeFn_003c66c0 *)0x008873FC)(arg1[1], (u32)arg2);
        arg2 = next;
    }
    p4c_003e12f0_003c66c0(arg1[1]);
    arg1[1] = 0;
    for (i = 0; i < arg0; i++) {
        (*(P4PoolFreeFn_003c66c0 *)0x008873FC)(arg1[0], arg3[i]);
        arg3[i] = 0;
    }
    p4c_003e12f0_003c66c0(arg1[0]);
    arg1[0] = 0;
    (*(P4FreeFn_003c66c0 *)0x008873EC)(arg3);
    return 0;
}

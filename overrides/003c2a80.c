/* func_003c2a80 (236 bytes): releases one reference to an object. Returns 1.
 * The reference count is the halfword at +0xE. While it is above 1 it is only decremented. At 1 (or
 * less) the object is torn down: func_003e1ea0(obj[0x58]) when set, halfword +0xC |= 0xFFF,
 * func_003c5760(obj[0x54]) (and the field cleared) when set, func_003e3c20(0x0070AFB0, obj), the buffer
 * at +0x5C freed through the slot 0x008873EC (and cleared), func_003c49a0(obj + 0x20), the count
 * decremented to its final value and the object itself freed through 0x008873EC. */

extern void p4c_003c49a0_003c2a80(void *p) __asm__("func_003c49a0");
extern void p4c_003c5760_003c2a80(u32 p) __asm__("func_003c5760");
extern void p4c_003e1ea0_003c2a80(u32 p) __asm__("func_003e1ea0");
extern void p4c_003e3c20_003c2a80(u32 list, void *obj) __asm__("func_003e3c20");

typedef void (*P4FreeFn_003c2a80)(void *ptr);

s32 func_003c2a80(u8 *arg0)
{
    if (*(s16 *)(arg0 + 0xE) - 1 > 0) {
        *(s16 *)(arg0 + 0xE) = (s16)(*(s16 *)(arg0 + 0xE) - 1);
        return 1;
    }
    if (*(u32 *)(arg0 + 0x58) != 0) {
        p4c_003e1ea0_003c2a80(*(u32 *)(arg0 + 0x58));
    }
    *(u16 *)(arg0 + 0xC) = (u16)(*(u16 *)(arg0 + 0xC) | 0xFFF);
    if (*(u32 *)(arg0 + 0x54) != 0) {
        u32 p = *(u32 *)(arg0 + 0x54);

        *(u32 *)(arg0 + 0x54) = 0;
        p4c_003c5760_003c2a80(p);
    }
    p4c_003e3c20_003c2a80(0x0070AFB0, arg0);
    if (*(u32 *)(arg0 + 0x5C) != 0) {
        u32 p = *(u32 *)(arg0 + 0x5C);

        *(u32 *)(arg0 + 0x5C) = 0;
        (*(P4FreeFn_003c2a80 *)0x008873EC)((void *)p);
    }
    p4c_003c49a0_003c2a80(arg0 + 0x20);
    *(s16 *)(arg0 + 0xE) = (s16)(*(s16 *)(arg0 + 0xE) - 1);
    (*(P4FreeFn_003c2a80 *)0x008873EC)(arg0);
    return 1;
}

/* func_003c0700 (324 bytes): destroys a scene object. Returns 1.
 *
 * func_003e3c20(0x0070AF90, arg0) unregisters it. Every entry of the circular list headed at arg0 + 8 (the
 * entry sits 0x40 bytes before its list node) goes to func_003c02e0; the walk also stops when that entry is
 * null. The list at arg0 + 0x10 (entry = node - 4 - *(s32 *)0x007647A4, gp-0x494C) goes to func_003c0880(entry's
 * word at +offset, entry) and func_003c3cc0(entry); the list at arg0 + 0x18 (offset *(s32 *)0x007647A0, gp-0x4950)
 * to func_003c08c0 and func_003e8440 the same way. Then func_003e95c0(arg0[4]) runs when set and the object is
 * returned to the freelist through the slot 0x008873FC (pool *(0x008872E0 + *(s32 *)0x007647B0) + 4, gp-0x4940). */

extern void p4c_003c02e0_003c0700(u8 *e) __asm__("func_003c02e0");
extern void p4c_003c0880_003c0700(u32 a, u8 *e) __asm__("func_003c0880");
extern void p4c_003c08c0_003c0700(u32 a, u8 *e) __asm__("func_003c08c0");
extern void p4c_003c3cc0_003c0700(u8 *e) __asm__("func_003c3cc0");
extern void p4c_003e3c20_003c0700(u32 list, void *obj) __asm__("func_003e3c20");
extern void p4c_003e8440_003c0700(u8 *e) __asm__("func_003e8440");
extern void p4c_003e95c0_003c0700(u32 a) __asm__("func_003e95c0");

typedef void (*P4FreeListFn_003c0700)(u32 pool, u8 *obj);

s32 func_003c0700(u8 *arg0)
{
    u8 *head;
    u8 *node;
    u8 *e;
    s32 off;

    p4c_003e3c20_003c0700(0x0070AF90, arg0);
    head = arg0 + 8;
    node = *(u8 **)head;
    if (node != head) {
        do {
            e = node - 0x40;
            node = *(u8 **)node;
            p4c_003c02e0_003c0700(e);
            if (e == (u8 *)0) {
                break;
            }
        } while (node != head);
    }
    head = arg0 + 0x10;
    node = *(u8 **)head;
    if (node != head) {
        do {
            off = *(s32 *)0x007647A4;
            e = node - 4 - off;
            node = *(u8 **)node;
            p4c_003c0880_003c0700(*(u32 *)(e + off), e);
            p4c_003c3cc0_003c0700(e);
            if (e == (u8 *)0) {
                break;
            }
        } while (node != head);
    }
    head = arg0 + 0x18;
    node = *(u8 **)head;
    if (node != head) {
        do {
            off = *(s32 *)0x007647A0;
            e = node - 4 - off;
            node = *(u8 **)node;
            p4c_003c08c0_003c0700(*(u32 *)(e + off), e);
            p4c_003e8440_003c0700(e);
            if (e == (u8 *)0) {
                break;
            }
        } while (node != head);
    }
    if (*(u32 *)(arg0 + 4) != 0) {
        p4c_003e95c0_003c0700(*(u32 *)(arg0 + 4));
    }
    (*(P4FreeListFn_003c0700 *)0x008873FC)(*(u32 *)(0x008872E0 + *(s32 *)0x007647B0 + 4), arg0);
    return 1;
}

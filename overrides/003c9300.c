/* func_003c9300 (556 bytes): destroys a scene node. The node's first word is its kind.
 *
 *   -2: the node is freed through the slot 0x008873EC.
 *   -1: func_003e1ea0(node[0x34]) when set; every entry of the circular list headed at node+0x38 goes
 *       to func_003ca9d0 and every entry of the list at node+0x40 to func_003ca960 (the next pointer
 *       is read before the call); func_003e3c20(0x0070B040, node); the buffers at +8, +0xC, +0x30,
 *       +4 and the eight at +0x10..+0x2C are freed (each cleared as it is freed); func_003c5760(node[0x78])
 *       when set (cleared first); then the node is freed.
 *   other: node[8] and node[0xC] are destroyed recursively (each cleared afterwards) and the node is
 *       freed. */

extern void p4c_003c5760_003c9300(u32 p) __asm__("func_003c5760");
extern void p4c_003ca960_003c9300(void *p) __asm__("func_003ca960");
extern void p4c_003ca9d0_003c9300(void *p) __asm__("func_003ca9d0");
extern void p4c_003e1ea0_003c9300(u32 p) __asm__("func_003e1ea0");
extern void p4c_003e3c20_003c9300(u32 list, void *obj) __asm__("func_003e3c20");

typedef void (*P4FreeFn_003c9300)(void *ptr);

void func_003c9300(u8 *arg0)
{
    u8 *e;
    u8 *head;
    s32 i;

    if (*(s32 *)arg0 == -2) {
        (*(P4FreeFn_003c9300 *)0x008873EC)(arg0);
        return;
    }
    if (*(s32 *)arg0 != -1) {
        if (*(u32 *)(arg0 + 8) != 0) {
            func_003c9300(*(u8 **)(arg0 + 8));
        }
        *(u32 *)(arg0 + 8) = 0;
        if (*(u32 *)(arg0 + 0xC) != 0) {
            func_003c9300(*(u8 **)(arg0 + 0xC));
        }
        *(u32 *)(arg0 + 0xC) = 0;
        (*(P4FreeFn_003c9300 *)0x008873EC)(arg0);
        return;
    }
    if (*(u32 *)(arg0 + 0x34) != 0) {
        p4c_003e1ea0_003c9300(*(u32 *)(arg0 + 0x34));
    }
    head = arg0 + 0x38;
    e = *(u8 **)head;
    if (e != head) {
        do {
            u8 *next = *(u8 **)e;

            p4c_003ca9d0_003c9300(e);
            e = next;
        } while (e != head);
        e = *(u8 **)(arg0 + 0x40);
    } else {
        e = *(u8 **)(arg0 + 0x40);
    }
    head = arg0 + 0x40;
    if (e != head) {
        do {
            u8 *next = *(u8 **)e;

            p4c_003ca960_003c9300(e);
            e = next;
        } while (e != head);
    }
    p4c_003e3c20_003c9300(0x0070B040, arg0);
    if (*(u32 *)(arg0 + 8) != 0) {
        u32 p = *(u32 *)(arg0 + 8);

        *(u32 *)(arg0 + 8) = 0;
        (*(P4FreeFn_003c9300 *)0x008873EC)((void *)p);
    }
    if (*(u32 *)(arg0 + 0xC) != 0) {
        u32 p = *(u32 *)(arg0 + 0xC);

        *(u32 *)(arg0 + 0xC) = 0;
        (*(P4FreeFn_003c9300 *)0x008873EC)((void *)p);
    }
    if (*(u32 *)(arg0 + 0x30) != 0) {
        u32 p = *(u32 *)(arg0 + 0x30);

        *(u32 *)(arg0 + 0x30) = 0;
        (*(P4FreeFn_003c9300 *)0x008873EC)((void *)p);
    }
    if (*(u32 *)(arg0 + 4) != 0) {
        u32 p = *(u32 *)(arg0 + 4);

        *(u32 *)(arg0 + 4) = 0;
        (*(P4FreeFn_003c9300 *)0x008873EC)((void *)p);
    }
    for (i = 0; i < 8; i++) {
        u32 p = *(u32 *)(arg0 + 0x10 + 4 * i);

        if (p != 0) {
            *(u32 *)(arg0 + 0x10 + 4 * i) = 0;
            (*(P4FreeFn_003c9300 *)0x008873EC)((void *)p);
        }
    }
    if (*(u32 *)(arg0 + 0x78) != 0) {
        u32 p = *(u32 *)(arg0 + 0x78);

        *(u32 *)(arg0 + 0x78) = 0;
        p4c_003c5760_003c9300(p);
    }
    (*(P4FreeFn_003c9300 *)0x008873EC)(arg0);
}

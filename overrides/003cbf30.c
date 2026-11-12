/* func_003cbf30 (220 bytes): removes a light (arg1) from its world (arg0). Returns arg0.
 *
 * Clears the world pointer stored in the light at the plugin offset held in the global at
 * 0x00764804 (gp-0x48EC; gp is 0x007690F0). Then every node of the light's list head at
 * arg1+0x2C (the list is circular, its head is arg1+0x2C) is unlinked from the list it is on
 * and again from the second list that sits 0x0C bytes below it, and the element (node-0x0C)
 * is returned to the free list whose pointer is at 0x008872E4 + *(0x00764808) (gp-0x48E8) via
 * the function pointer at 0x008873FC (RwFreeListFree). Finally the light's own link at
 * arg1+0x34/+0x38 is unlinked. Links are { next, prev } pairs. */

typedef void (*P4FreeListFreeFn)(u8 *list, u8 *elem);

u8 *func_003cbf30(u8 *arg0, u8 *arg1)
{
    u8 *head;
    u32 *node;
    u32 *next;
    u32 *elem;

    head = arg1 + 0x2C;
    *(u32 *)(arg1 + *(s32 *)0x00764804) = 0;
    node = *(u32 **)head;
    while (node != (u32 *)head) {
        elem = node - 3;
        next = (u32 *)node[0];
        *(u32 *)node[1] = node[0];
        *(u32 *)(node[0] + 4) = node[1];
        *(u32 *)elem[1] = elem[0];
        *(u32 *)(elem[0] + 4) = elem[1];
        (*(P4FreeListFreeFn *)0x008873FC)(*(u8 **)(0x008872E4 + *(s32 *)0x00764808), (u8 *)elem);
        node = next;
    }
    **(u32 **)(arg1 + 0x38) = *(u32 *)(arg1 + 0x34);
    *(u32 *)(*(u32 *)(arg1 + 0x34) + 4) = *(u32 *)(arg1 + 0x38);
    return arg0;
}

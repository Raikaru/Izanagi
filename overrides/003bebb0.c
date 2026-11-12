/* func_003bebb0 (352 bytes): collects the distinct owner objects of a circular list into the {items, count} pair arg0 and
 * returns arg0 (0 on allocation failure). n = func_003bfe60(arg1) sizes the array (allocator slot 0x008873E8, n * 4 bytes,
 * tag 0x3000F; failure reports {2, func_003df590(0x80000013, n * 4)} through func_003df4d0; n <= 0 leaves items null). The list head
 * is at arg1 + 8 (links in word 0 of each node); each node's owner is the word at node - 0x28; owners not yet in the array
 * get func_003c2a60(owner) called, are appended and counted. */

extern s32 p4c_003bfe60_003bebb0(u8 *l) __asm__("func_003bfe60");
extern void p4c_003c2a60_003bebb0(u32 o) __asm__("func_003c2a60");
extern void p4c_003df4d0_003bebb0(s32 *err) __asm__("func_003df4d0");
extern s32 p4c_003df590_003bebb0(s32 code, ...) __asm__("func_003df590");

typedef void *(*P4AllocFn_003bebb0)(u32 size, u32 tag);

typedef struct P4List_003bebb0 {
    u32 *items;
    s32 count;
} P4List_003bebb0;

P4List_003bebb0 *func_003bebb0(P4List_003bebb0 *arg0, u8 *arg1)
{
    s32 n;
    s32 err[2];
    u32 *head = (u32 *)(arg1 + 8);
    u32 *node;
    u32 *cursor;
    u32 owner;
    s32 i;
    s32 found;

    n = p4c_003bfe60_003bebb0(arg1);
    if (n > 0) {
        arg0->items = (u32 *)(*(P4AllocFn_003bebb0 *)0x008873E8)((u32)n << 2, 0x3000F);
        if (arg0->items == (u32 *)0) {
            err[0] = 2;
            err[1] = p4c_003df590_003bebb0(0x80000013, (u32)n << 2);
            p4c_003df4d0_003bebb0(err);
            return (P4List_003bebb0 *)0;
        }
    } else {
        arg0->items = (u32 *)0;
    }
    arg0->count = 0;
    node = (u32 *)head[0];
    cursor = arg0->items;
    while (node != head) {
        owner = *(u32 *)((u8 *)node - 0x28);
        found = 0;
        for (i = 0; i < arg0->count; i++) {
            if (arg0->items[i] == owner) {
                found = 1;
                break;
            }
        }
        if (!found) {
            p4c_003c2a60_003bebb0(owner);
            *cursor = owner;
            cursor++;
            arg0->count = arg0->count + 1;
        }
        node = (u32 *)node[0];
    }
    return arg0;
}

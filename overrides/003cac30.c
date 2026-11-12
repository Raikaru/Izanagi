/* func_003cac30 (264 bytes): re-links an object into its tree. Returns arg0, or 0 when the owner's
 * function fails. The owner record is at arg0 + *(s32 *)0x007647FC (gp-0x48F4; gp is 0x007690F0): +0 the tree,
 * +4 a function f. f(arg0) must return non-zero. With a tree, every link entry of the circular list
 * headed at arg0 + 0x64 (an entry sits 0xC bytes before the list node) whose word +8 and +0x14 are set
 * is unlinked from both of its lists and returned to the freelist through the slot 0x008873FC (pool
 * *(0x008872E0 + *(s32 *)0x00764808)); afterwards func_003caa50(tree, arg0) links the object again. */

extern void p4c_003caa50_003cac30(u8 *t, u8 *o) __asm__("func_003caa50");

typedef s32 (*P4OwnerFn_003cac30)(u8 *obj);
typedef void (*P4FreeListFn_003cac30)(u32 pool, u8 *entry);

u8 *func_003cac30(u8 *arg0)
{
    u8 *owner;
    u8 *tree;
    u8 *head;
    u8 *node;

    owner = arg0 + *(s32 *)0x007647FC;
    if ((*(P4OwnerFn_003cac30 *)(owner + 4))(arg0) == 0) {
        return (u8 *)0;
    }
    tree = *(u8 **)owner;
    if (tree == (u8 *)0) {
        return arg0;
    }
    head = arg0 + 0x64;
    node = *(u8 **)head;
    if (node != head) {
        do {
            u8 *e = node - 0xC;

            node = *(u8 **)node;
            if (*(u32 *)(e + 8) != 0 && *(u32 *)(e + 0x14) != 0) {
                u32 pool;

                **(u32 **)(e + 0x10) = *(u32 *)(e + 0xC);
                *(u32 *)(*(u8 **)(e + 0xC) + 4) = *(u32 *)(e + 0x10);
                **(u32 **)(e + 4) = *(u32 *)e;
                *(u32 *)(*(u8 **)e + 4) = *(u32 *)(e + 4);
                pool = *(u32 *)(0x008872E0 + *(s32 *)0x00764808);
                (*(P4FreeListFn_003cac30 *)0x008873FC)(pool, e);
            }
        } while (node != head);
    }
    p4c_003caa50_003cac30(tree, arg0);
    return arg0;
}

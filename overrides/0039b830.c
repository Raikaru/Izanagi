/* func_0039b830 (PDS registry destroy): destroys the pipe of every entry (entry word 4,
 * entries are 20 bytes) that has one, frees the entry array through the RenderWare
 * free slot at retail 0x008873EC, and clears {items, count, capacity, flags}.
 *
 * Direct transliteration of the retail code (the decomp's NON_MATCHING body has the same
 * control flow). The list is the 16-byte registry header {items, count, capacity, flags}. */

typedef void (*P4PdsFreeFn)(void *p);

void func_0039b830(u8 *list_arg)
{
    u32 *list = (u32 *)list_arg;
    u32 i;

    for (i = 0; i < list[1]; i++) {
        u32 pipe = ((u32 *)list[0])[i * 5 + 4];

        if (pipe != 0)
            func_00411670((s32)pipe);
    }
    (*(P4PdsFreeFn *)0x008873EC)((void *)list[0]);
    list[2] = 0;
    list[1] = 0;
    list[0] = 0;
    list[3] = 0;
}

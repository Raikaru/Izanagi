/* func_003bbea0 (224 bytes): releases the buffers of a record through the free slot 0x008873EC: the pointer at +0
 * (cleared), then, when the word at +4 is 3, each of the arg0[2] pointers of the array at +0xC (not cleared), and finally the
 * array at +0xC itself (cleared). */

typedef void (*P4FreeFn_003bbea0)(void *p);

void func_003bbea0(u32 *arg0)
{
    P4FreeFn_003bbea0 *slot = (P4FreeFn_003bbea0 *)0x008873EC;
    s32 i;
    u32 *p;

    if (arg0[0] != 0) {
        (*slot)((void *)arg0[0]);
        arg0[0] = 0;
    }
    if (arg0[1] == 3) {
        p = (u32 *)arg0[3];
        for (i = 0; i < (s32)arg0[2]; i++) {
            if (p[i] != 0) {
                (*slot)((void *)p[i]);
            }
        }
    }
    if (arg0[3] != 0) {
        (*slot)((void *)arg0[3]);
        arg0[3] = 0;
    }
}

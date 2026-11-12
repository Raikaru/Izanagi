/* func_003bc740 (320 bytes): frees a name list. arg0[0] is the entry count and arg0[1] the entry array (16 bytes each;
 * an entry has a string pointer at +0, a type word at +4, a count at +8 and a pointer array at +0xC). Without an array
 * only the two words are cleared. Otherwise each entry's string is freed and cleared, a type 3 entry frees its count strings, the
 * entry's array is freed and cleared, and finally the entry array is freed; arg0[1] and arg0[0] are set to 0. All frees go
 * through the slot 0x008873EC. */

typedef void (*P4FreeFn_003bc740)(void *p);

void func_003bc740(u32 *arg0)
{
    P4FreeFn_003bc740 *slot = (P4FreeFn_003bc740 *)0x008873EC;
    s32 i;
    s32 j;
    u32 *e;
    u32 *list;

    if (arg0[1] != 0) {
        for (i = 0; i < (s32)arg0[0]; i++) {
            e = (u32 *)(arg0[1] + i * 16);
            if (e[0] != 0) {
                (*slot)((void *)e[0]);
                e[0] = 0;
            }
            if (e[1] == 3) {
                list = (u32 *)e[3];
                for (j = 0; j < (s32)e[2]; j++) {
                    if (list[j] != 0) {
                        (*slot)((void *)list[j]);
                    }
                }
            }
            if (e[3] != 0) {
                (*slot)((void *)e[3]);
                e[3] = 0;
            }
        }
        (*slot)((void *)arg0[1]);
    }
    arg0[1] = 0;
    arg0[0] = 0;
}

/* Retail 003d6170 (rtanim.c): finds the entry of an object's list (word [1], an RwSList) whose name
 * equals `key`. The name of an entry is the result of the owner's function at owner word [0] + 0x20
 * applied to the entry; names are compared with the RenderWare string-compare slot at 0x008873c8
 * (0 means equal). Returns the entry or 0. */
extern void *p4_sl_begin(void *list) __asm__("func_003df890");
extern void *p4_sl_end(void *list) __asm__("func_003df8a0");
extern int (*p4_rw_strcmp[])(const void *a, const void *b) __asm__("D_008873C8");

unsigned int func_003d6170(unsigned int *object, const void *key)
{
    void *(*name_of)(unsigned int) = *(void *(**)(unsigned int))(object[0] + 0x20);
    unsigned int *end = (unsigned int *)p4_sl_end((void *)object[1]);
    unsigned int *it;

    for (it = (unsigned int *)p4_sl_begin((void *)object[1]); it != end; it++) {
        if (p4_rw_strcmp[0](name_of(*it), key) == 0) {
            return *it;
        }
    }
    return 0;
}

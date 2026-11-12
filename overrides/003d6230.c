/* Retail 003d6230 (rtanim.c, called by mdlReleaseBlendTrack): releases a track object. `object` word
 * [0] is its owner and word [1] a list of entries (RwSLists). If the object is the owner's current
 * track (owner word [5]) that reference is cleared; the object is removed from the owner's track
 * list (owner word [4]) by overwriting its slot with the last entry and shortening the list; the
 * owner's entry destructor (function at owner + 0x1c) runs on every entry of the object's own list,
 * that list is destroyed, and the object is freed through the RenderWare free slot. Returns 1. */
extern void *p4_sl_begin(void *list) __asm__("func_003df890");            /* _rwSListGetBegin */
extern void *p4_sl_end(void *list) __asm__("func_003df8a0");              /* _rwSListGetEnd */
extern int p4_sl_destroy(void *list) __asm__("func_003df7f0");            /* _rwSListDestroy */
extern int p4_sl_count(void *list) __asm__("func_003df860");              /* _rwSListGetNumEntries */
extern void *p4_sl_entry(void *list, int index) __asm__("func_003df870"); /* _rwSListGetEntry */
extern void p4_sl_pop(void *list, int count) __asm__("func_003df7e0");    /* _rwSListDestroyEndEntries */
extern void (*p4_rt_free_slot[])(void *) __asm__("jtbl_008873EC");

int func_003d6230(unsigned int *object)
{
    unsigned int *owner = (unsigned int *)object[0];
    unsigned int *it, *end;

    if (object == (unsigned int *)owner[5]) {
        owner[5] = 0;
    }
    owner = (unsigned int *)object[0];
    it = (unsigned int *)p4_sl_begin((void *)owner[4]);
    end = (unsigned int *)p4_sl_end((void *)owner[4]);
    for (; it != end; it++) {
        if (object == (unsigned int *)*it) {
            int n = p4_sl_count((void *)owner[4]);
            unsigned int *last = (unsigned int *)p4_sl_entry((void *)owner[4], n - 1);

            *it = *last;
            p4_sl_pop((void *)owner[4], 1);
            break;
        }
    }
    end = (unsigned int *)p4_sl_end((void *)object[1]);
    for (it = (unsigned int *)p4_sl_begin((void *)object[1]); it != end; it++) {
        (*(void (**)(unsigned int))(object[0] + 0x1c))(*it);
    }
    p4_sl_destroy((void *)object[1]);
    p4_rt_free_slot[0](object);
    return 1;
}

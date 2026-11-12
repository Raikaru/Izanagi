/* func_00412e90: finds the next table entry whose name equals `name`. `reg` holds the entry
 * count at +4 and the entry table at +8 (40-byte entries). Each entry's word 0 points at a
 * record whose first word is the entry's name string. The names are compared through the
 * function pointer stored at retail slot 0x008873C8 (a strcmp-style callback; 0 means equal).
 * With `start` non-zero the search resumes after that entry. On a hit the entry is returned and
 * *index (when given) receives its position; otherwise 0 is returned and *index is -1. */
s32 **func_00412e90(u8 *reg, s32 name, s32 start, s32 *index)
{
    s32 count;
    s32 *entry;
    s32 (*compare)(s32, s32);

    if (reg != 0 && name != 0 && *(s32 *)(reg + 4) != 0) {
        count = *(s32 *)(reg + 4);
        entry = *(s32 **)(reg + 8);
        if (start != 0) {
            while (entry != (s32 *)start && count > 0) {
                entry += 10;
                count--;
            }
            entry += 10;
            count--;
        }
        while (count > 0) {
            if (*(s32 **)entry != 0) {
                compare = *(s32 (**)(s32, s32))0x008873C8;
                if (compare(**(s32 **)entry, name) == 0) {
                    if (index != 0) {
                        *index = *(s32 *)(reg + 4) - count;
                    }
                    return (s32 **)entry;
                }
            }
            entry += 10;
            count--;
        }
    }
    if (index != 0) {
        *index = -1;
    }
    return 0;
}

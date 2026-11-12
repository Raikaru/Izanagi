/* func_004141e0: walks the group list `list` (nodes chain through word 2; word 0 of a node is
 * its first cell, whose word 0 is the group's key). For each node whose key differs from the key
 * of every earlier node, `cb(key, index, arg)` runs (when cb is not null) with a running index
 * from 0. Returns the number of distinct keys. */
s32 func_004141e0(u32 *list, s32 (*cb)(u32, s32, void *), void *arg)
{
    s32 count = 0;
    u32 *cur;
    u32 *scan;

    for (cur = list; cur != 0; cur = (u32 *)cur[2]) {
        scan = list;
        if (list != cur) {
            u32 key = *(u32 *)cur[0];

            do {
                if (*(u32 *)scan[0] == key) {
                    break;
                }
                scan = (u32 *)scan[2];
            } while (scan != cur);
        }
        if (scan == cur) {
            if (cb != 0) {
                cb(*(u32 *)cur[0], count, arg);
            }
            count++;
        }
    }
    return count;
}

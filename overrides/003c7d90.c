/* func_003c7d90 (1136 bytes): joins the list of index strips at arg0[0] into one strip and returns 1 (0 when arg0[0] is null). A strip node is {+0 = array of 16-bit
 * indices, +4 = count, +0xC = next}. The result record (pool slot 0x008873F8, pool = *(u32 *)(0x008872E4 + *(s32 *)0x007647E0) (gp-0x4910; gp is 0x007690F0), tag 0x30502)
 * is {+0 = index array (allocator slot 0x008873E8, 4 * capacity bytes where the capacity is the sum of count + 6 over the nodes), +4 = count,
 * +8 = capacity, +0xC = 0} and replaces arg0[0]. The first node is copied; then, while nodes remain, the next node is the first one in the list whose first index
 * equals the strip's last index (when the count is odd and arg1 is set that index is repeated first to flip the winding), else one whose first index equals the second to last index (the
 * index is repeated, twice when the count is even and arg1 is set), else the first remaining node, joined by repeating the last index and the new
 * node's first index (three times when the count is odd and arg1 is set). Each node used is freed (index array through slot 0x008873EC, node through slot 0x008873FC) and unlinked. */

typedef void *(*P4PoolAllocFn_003c7d90)(u32 pool, u32 tag);
typedef void *(*P4AllocFn_003c7d90)(u32 size, u32 tag);
typedef void (*P4FreeFn_003c7d90)(void *p);
typedef void (*P4PoolFreeFn_003c7d90)(u32 pool, u32 obj);

static u32 p4_pool_003c7d90(void)
{
    return *(u32 *)(0x008872E4 + *(s32 *)0x007647E0);
}

static void p4_append_003c7d90(u32 *res, u32 *node)
{
    u32 i;

    for (i = 0; i < node[1]; i++) {
        ((u16 *)res[0])[res[1]] = ((u16 *)node[0])[i];
        res[1] = res[1] + 1;
    }
}

u32 func_003c7d90(u32 *arg0, s32 arg1)
{
    u32 *res;
    u32 *node;
    u32 *next;
    u32 *cur;
    u32 *pred;
    u16 *hu;
    s32 n;
    u32 last;
    u32 first;

    if (arg0[0] == 0) {
        return 0;
    }
    res = (u32 *)(*(P4PoolAllocFn_003c7d90 *)0x008873F8)(*(u32 *)(0x008872E4 + *(s32 *)0x007647E0), 0x30502);
    res[1] = 0;
    res[2] = 0;
    for (node = (u32 *)arg0[0]; node != (u32 *)0; node = (u32 *)node[3]) {
        res[2] = res[2] + (node[1] + 6);
    }
    res[0] = (u32)(*(P4AllocFn_003c7d90 *)0x008873E8)(res[2] << 2, 0x30502);
    cur = (u32 *)arg0[0];
    p4_append_003c7d90(res, cur);
    (*(P4FreeFn_003c7d90 *)0x008873EC)((void *)cur[0]);
    cur[0] = 0;
    next = (u32 *)cur[3];
    (*(P4PoolFreeFn_003c7d90 *)0x008873FC)(p4_pool_003c7d90(), (u32)cur);
    while (next != (u32 *)0) {
        n = (s32)res[1];
        hu = (u16 *)res[0];
        last = hu[n - 1];
        for (cur = next; cur != (u32 *)0; cur = (u32 *)cur[3]) {
            first = ((u16 *)cur[0])[0];
            if (first == last) {
                if ((n & 1) != 0 && arg1 != 0) {
                    hu[n] = (u16)first;
                    res[1] = res[1] + 1;
                }
                break;
            }
        }
        if (cur == (u32 *)0) {
            n = (s32)res[1];
            hu = (u16 *)res[0];
            last = hu[n - 2];
            for (cur = next; cur != (u32 *)0; cur = (u32 *)cur[3]) {
                first = ((u16 *)cur[0])[0];
                if (first == last) {
                    if ((n & 1) != 0 || arg1 == 0) {
                        hu[n] = (u16)first;
                        res[1] = res[1] + 1;
                    } else {
                        hu[n] = (u16)first;
                        hu[n + 1] = *(u16 *)cur[0];
                        res[1] = res[1] + 2;
                    }
                    break;
                }
            }
        }
        if (cur == (u32 *)0) {
            n = (s32)res[1];
            hu = (u16 *)res[0];
            cur = next;
            if ((n & 1) != 0 && arg1 != 0) {
                hu[n] = hu[n - 1];
                hu[n + 1] = *(u16 *)cur[0];
                hu[n + 2] = *(u16 *)cur[0];
                res[1] = (u32)(n + 3);
            } else {
                hu[n] = hu[n - 1];
                hu[n + 1] = *(u16 *)cur[0];
                res[1] = (u32)(n + 2);
            }
        }
        p4_append_003c7d90(res, cur);
        (*(P4FreeFn_003c7d90 *)0x008873EC)((void *)cur[0]);
        cur[0] = 0;
        if (next == cur) {
            next = (u32 *)next[3];
        } else if (((u32 *)next[3]) == cur) {
            next[3] = cur[3];
        } else {
            pred = (u32 *)next;
            do {
                pred = (u32 *)pred[3];
            } while ((u32 *)pred[3] != cur);
            pred[3] = cur[3];
        }
        (*(P4PoolFreeFn_003c7d90 *)0x008873FC)(p4_pool_003c7d90(), (u32)cur);
    }
    arg0[0] = (u32)res;
    res[3] = 0;
    return 1;
}

/* Retail 003f25c0 (palquant.c): frees a colour-trie subtree. A node is 16 child pointers (+0 ..
 * +0x3c); at most `depth` levels below it are followed (each level's children, 16 per node, with the
 * depth lowered by 2), and every node is returned to the pool (ctx + 0x4004) through the free slot
 * at 0x008873fc. A null node is ignored. */
extern void (*p4_pq_pool_free[])(void *pool, void *ptr) __asm__("D_008873FC");

void func_003f25c0(unsigned char *ctx, unsigned char *node, int depth)
{
    int i;

    if (node == 0) {
        return;
    }
    if (depth > 0) {
        for (i = 0; i < 16; i++) {
            unsigned char *child = *(unsigned char **)(node + i * 4 + 0x1c);

            if (child != 0) {
                if (depth - 1 > 0) {
                    int j;

                    for (j = 0; j < 16; j++) {
                        func_003f25c0(ctx, *(unsigned char **)(child + j * 4 + 0x1c), depth - 2);
                    }
                }
                p4_pq_pool_free[0](*(void **)(ctx + 0x4004), child);
            }
        }
    }
    p4_pq_pool_free[0](*(void **)(ctx + 0x4004), node);
}

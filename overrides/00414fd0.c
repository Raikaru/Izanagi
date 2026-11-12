/* func_00414fd0 (RxHeapDestroy): frees the heap's dirty-list buffer (heap[3]) when set,
 * then every data block in the chain that starts at heap[1] (next block at block[2]),
 * then the heap header itself. NULL is a no-op. All frees go through the RenderWare
 * free slot at retail 0x008873EC.
 *
 * Direct transliteration of the retail code; the decomp's NON_MATCHING body has the same
 * control flow. The inner NULL test of the retail loop is dead (the block pointer is
 * non-NULL on every entry) and is omitted. */

typedef void (*P4HeapFreeFn)(void *p);

void func_00414fd0(u32 *heap)
{
    P4HeapFreeFn free_fn = *(P4HeapFreeFn *)0x008873EC;
    u32 *blk;
    u32 *next;

    if (heap == NULL)
        return;

    if (heap[3] != 0) {
        free_fn((void *)heap[3]);
        heap[3] = 0;
    }
    blk = (u32 *)heap[1];
    while (blk != NULL) {
        next = (u32 *)blk[2];
        free_fn(blk);
        blk = next;
    }
    free_fn(heap);
}

/* func_00414a30 (RxHeap grow): allocates a new superblock for a request of `size` bytes (at least
 * the heap's minimum superblock size, heap[0]), carves it into the heap's block list with
 * func_00414930 and links it in at heap[1]. Returns the new superblock's first block header
 * (data base + 0x20), or 0 when the allocation or the carve fails (the superblock is freed then).
 *
 * Retail also formats the total heap size into a stack buffer with a sprintf (format at 0x70c380)
 * and never reads the buffer; the port omits it. The allocator and free callbacks are the
 * RenderWare memory function table slots at 0x008873e8 and 0x008873ec. */

extern void *(*p4_rx_mem_alloc[])(s32, s32) __asm__("jtbl_008873E8");
extern void (*p4_rx_mem_free[])(void *) __asm__("jtbl_008873EC");
s32 func_00414930(u32 *blk, u32 *prev, u32 *w);

u8 *func_00414a30(u32 *heap, u32 size)
{
    u32 total_size = size + 0x60;
    u32 *sb;

    if (total_size < heap[0]) {
        total_size = heap[0];
    }
    sb = (u32 *)p4_rx_mem_alloc[0](total_size + 0x8b, 0x1040409);
    if (sb == NULL) {
        return NULL;
    }
    sb[0] = ((u32)sb + 12 + 127) & 0xffffff80u;
    sb[1] = total_size;
    sb[2] = 0;
    if (func_00414930(sb, (u32 *)heap[1], heap) == 0) {
        p4_rx_mem_free[0](sb);
        return NULL;
    }
    sb[2] = heap[1];
    heap[1] = (u32)sb;
    return (u8 *)(sb[0] + 0x20);
}

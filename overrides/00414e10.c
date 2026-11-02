/* func_00414e10 (RxHeapAlloc): returns `size` bytes (rounded up to a multiple of 0x20) from the
 * heap, or 0 after raising the out-of-memory RenderWare error (code 0x80000013, argument the
 * rounded size). The free entries (heap +0x0c array of {size, header}, +0x14 count) are scanned
 * from the last entry back; when none is large enough the heap grows by a superblock
 * (func_00414a30). The heap's dirty flag (+0x18) is set before each carve. */

extern s32 p4_rx_error_code(s32, s32) __asm__("func_003df590");
extern void p4_rx_error_set(void *) __asm__("func_003df4d0");
u8 *func_00414a30(u32 *heap, u32 size);
u8 *func_00414b50(u8 *heap, u32 *entry, u32 size);

u8 *func_00414e10(u8 *heap, s32 requested)
{
    u32 size = ((u32)requested + 0x1f) & 0xffffffe0u;
    s32 count = *(s32 *)(heap + 0x14);
    u32 *entry = (u32 *)(*(u32 *)(heap + 0x0c) + (count - 1) * 8);
    u8 *header;
    s32 error[2];

    for (;;) {
        if (entry[0] >= size) {
            *(u32 *)(heap + 0x18) = 1;
            return func_00414b50(heap, entry, size);
        }
        count--;
        entry -= 2;
        if (count == 0) {
            break;
        }
    }
    header = func_00414a30((u32 *)heap, size);
    if (header != NULL) {
        *(u32 *)(heap + 0x18) = 1;
        return func_00414b50(heap, (u32 *)*(u32 *)(header + 0x0c), size);
    }
    error[0] = 1;
    error[1] = p4_rx_error_code((s32)0x80000013, (s32)size);
    p4_rx_error_set(error);
    return NULL;
}

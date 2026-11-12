/* Boot heap, just below 0x02000000 (the top of PS2 RAM the loader maps).
 * The decomp computes it from &D_A0000 (0x000A0000) and &D_938A00 (0x00938A00).
 * Both lie outside the retail image, so the placement tool cannot place them; the port
 * uses the literal values.
 *
 * The original arena starts at 0x00938A00 and EndOfHeap is
 * 0x02000000 - 0xA0000. The budget probe subtracts a further 0x2000 and
 * sixteen bytes before malloc/free; that is a reported budget, not a second
 * allocator arena limit. p4pc_heap_init supplies those same mapped break
 * bounds to the literal retail newlib allocator before preserving the probe.
 * The allocator takes a numeric EE size word and returns a pointer; the probe
 * stores its address in the existing 32-bit bookkeeping slot before release. */
extern int32_t p4pc_heap_init(void *base, size_t capacity);
extern void *func_0043ece8(u32 size);
extern void func_0043ed08(s32 arg0);
extern u8 D_00559800[];

u8 *kwlnProbeHeapBudget(void) {
    u32 free_area;
    s32 status;
    free_area = 0x02000000u - 0x2000u - 0x000A0000u - 0x00938A00u - 16u;
    p4pc_heap_init((void *)(uintptr_t)0x00938A00u, 0x02000000u - 0x000A0000u - 0x00938A00u);
    status = (s32)(uintptr_t)func_0043ece8(free_area);
    if (status == 0) {
        func_00440b68(&D_00559800);
    }
    func_0043ed08(status);
    return (u8 *)(uintptr_t)free_area;
}

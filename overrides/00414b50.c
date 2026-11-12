/* func_00414b50 (RxHeap allocate from a free entry): carves `size` bytes off the free block that
 * `entry` ({size, header}) describes, and returns the user pointer (header + 0x20). A remainder
 * under 0x100 bytes is not split off: the whole block is taken and the entry is removed by moving
 * the last entry into its slot. Otherwise the block is split and the entry now describes the
 * remainder. Entry words are copied raw (retail lwc1/swc1).
 *
 * Heap: +0x0c = free-entry array, +0x14 = entries used. Block header (0x20 bytes, before the
 * user data): +0 prev, +4 next, +8 size, +0xc free-entry pointer (0 when allocated). */

#define P4_HEAP_WD(p, off) (*(u32 *)((u8 *)(p) + (off)))

u8 *func_00414b50(u8 *heap, u32 *entry, u32 size)
{
    u8 *header = (u8 *)entry[1];
    u32 *entries = (u32 *)P4_HEAP_WD(heap, 0x0c);
    u32 used = P4_HEAP_WD(heap, 0x14);

    if (entry[0] - size < 0x100) {
        u32 *last = entries + (used - 1) * 2;

        if (last != entry) {
            u32 *src = entries + used * 2;

            entry[0] = src[-2];
            entry[1] = src[-1];
            P4_HEAP_WD(entry[1], 0x0c) = (u32)entry;
        }
        P4_HEAP_WD(heap, 0x14) = P4_HEAP_WD(heap, 0x14) - 1;
        P4_HEAP_WD(header, 0x0c) = 0;
        return header + 0x20;
    } else {
        u8 *split = header + size;
        u8 *rest = split + 0x20;
        u32 rest_size = entry[0] - size - 0x20;

        P4_HEAP_WD(split, 0x28) = rest_size;
        P4_HEAP_WD(split, 0x20) = (u32)header;
        P4_HEAP_WD(split, 0x24) = P4_HEAP_WD(header, 4);
        P4_HEAP_WD(split, 0x2c) = (u32)entry;
        P4_HEAP_WD(header, 8) = size;
        P4_HEAP_WD(header, 4) = (u32)rest;
        if (P4_HEAP_WD(split, 0x24) != 0) {
            P4_HEAP_WD(P4_HEAP_WD(split, 0x24), 0) = (u32)rest;
        }
        entry[1] = (u32)rest;
        entry[0] = P4_HEAP_WD(rest, 8);
        P4_HEAP_WD(header, 0x0c) = 0;
        return header + 0x20;
    }
}

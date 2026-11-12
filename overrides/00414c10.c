/* func_00414c10 (RxHeapFree): returns the block whose user pointer is `block` to the heap and merges
 * it with a free neighbour on either side. Block header (0x20 bytes before the data): +0 prev,
 * +4 next, +8 size, +0xc free-entry pointer (non-zero when the block is free). Heap: +0x0c = free
 * entry array of {size, header}, +0x14 = entries used. A merge that removes an entry moves the
 * last entry into its slot. With no free neighbour a new entry comes from func_00414820. */

#define P4_HEAP_WD(p, off) (*(u32 *)((u8 *)(p) + (off)))

u8 *func_00414820(u32 *w);

void func_00414c10(u8 *heap, u8 *block)
{
    u8 *header = block - 0x20;
    u8 *prev = (u8 *)P4_HEAP_WD(header, 0);
    u8 *next = (u8 *)P4_HEAP_WD(header, 4);
    int prev_free = prev != NULL && P4_HEAP_WD(prev, 0x0c) != 0;
    int next_free = next != NULL && P4_HEAP_WD(next, 0x0c) != 0;

    if (prev_free && next_free) {
        u32 *entry = (u32 *)P4_HEAP_WD(next, 0x0c);
        u32 *entries = (u32 *)P4_HEAP_WD(heap, 0x0c);
        u32 used = P4_HEAP_WD(heap, 0x14);
        u8 *after;

        if (entries + (used - 1) * 2 != entry) {
            u32 *last = entries + used * 2;

            entry[0] = last[-2];
            entry[1] = last[-1];
            P4_HEAP_WD(entry[1], 0x0c) = (u32)entry;
        }
        P4_HEAP_WD(heap, 0x14) = P4_HEAP_WD(heap, 0x14) - 1;
        P4_HEAP_WD(prev, 8) = P4_HEAP_WD(prev, 8) + P4_HEAP_WD(header, 8) + P4_HEAP_WD(next, 8) + 0x40;
        *(u32 *)P4_HEAP_WD(prev, 0x0c) = P4_HEAP_WD(prev, 8);
        P4_HEAP_WD(prev, 4) = P4_HEAP_WD(next, 4);
        after = (u8 *)P4_HEAP_WD(next, 4);
        if (after != NULL) {
            P4_HEAP_WD(after, 0) = (u32)prev;
        }
    } else if (prev_free) {
        P4_HEAP_WD(prev, 8) = P4_HEAP_WD(prev, 8) + P4_HEAP_WD(header, 8) + 0x20;
        *(u32 *)P4_HEAP_WD(prev, 0x0c) = P4_HEAP_WD(prev, 8);
        P4_HEAP_WD(prev, 4) = (u32)next;
        if (next != NULL) {
            P4_HEAP_WD(next, 0) = (u32)prev;
        }
    } else if (next_free) {
        P4_HEAP_WD(header, 8) = P4_HEAP_WD(header, 8) + P4_HEAP_WD(next, 8) + 0x20;
        P4_HEAP_WD(header, 0x0c) = P4_HEAP_WD(next, 0x0c);
        P4_HEAP_WD(P4_HEAP_WD(next, 0x0c), 4) = (u32)header;
        *(u32 *)P4_HEAP_WD(next, 0x0c) = P4_HEAP_WD(header, 8);
        P4_HEAP_WD(header, 4) = P4_HEAP_WD(next, 4);
        if (P4_HEAP_WD(header, 4) != 0) {
            P4_HEAP_WD(P4_HEAP_WD(header, 4), 0) = (u32)header;
        }
    } else {
        u32 *entry = (u32 *)func_00414820((u32 *)heap);

        if (entry != NULL) {
            entry[1] = (u32)header;
            entry[0] = P4_HEAP_WD(header, 8);
            P4_HEAP_WD(header, 0x0c) = (u32)entry;
        }
    }
}

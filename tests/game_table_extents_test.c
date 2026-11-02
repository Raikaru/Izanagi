/* The table service's battle-bank extent registry: exact original sizes from
 * the native copy, publication with every slot store and fail-closed readers.
 * No retail assets, combat or native launch. */
#include "guest_test.h"
#include "p4pc/game_table_extents.h"
#undef NDEBUG
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

int main(void)
{
    if (p4t_guest_map(0x00764000u, 0x2000) != 0) return 77;
    const uint32_t slot = P4_GPADDR(0xb414), base = 0x007f17b0u;
    uint32_t actual_base = 0, rows = 0;
    P4_G32(slot) = base;
    P4_GPDATA(uint32_t, 0xb438) = 8;
    assert(!p4_game_table_extent_rows(slot, 24, &actual_base, &rows));
    /* A one-row block may have 8 bytes of allocator padding. The native note
     * contains 24, so no neighbor/padding/zero-size alias can manufacture rows. */
    p4_game_table_extent_note(slot, base, 24);
    assert(p4_game_table_extent_rows(slot, 24, &actual_base, &rows));
    assert(actual_base == base && rows == 1);
    assert(!p4_game_table_extent_rows(slot, 0, &actual_base, &rows));
    assert(!p4_game_table_extent_rows(slot, 24, NULL, &rows));
    assert(!p4_game_table_extent_rows(slot + 1, 24, &actual_base, &rows));
    assert(!p4_game_table_extent_rows(0x02000000u, 24, &actual_base, &rows));
    P4_GPDATA(uint32_t, 0xb438) = 7;
    assert(!p4_game_table_extent_rows(slot, 24, &actual_base, &rows));
    P4_GPDATA(uint32_t, 0xb438) = 8;
    /* A writer that stores a slot without publishing fails closed. */
    P4_G32(slot) = base + 16;
    assert(!p4_game_table_extent_rows(slot, 24, &actual_base, &rows));
    P4_G32(slot) = base;
    p4_game_table_extent_note(slot, base, 47);
    assert(!p4_game_table_extent_rows(slot, 24, &actual_base, &rows));
    p4_game_table_extent_note(slot, base, 48);
    assert(p4_game_table_extent_rows(slot, 24, &actual_base, &rows) && rows == 2);
    p4_game_table_extent_note(slot, base, 0);
    assert(p4_game_table_extent_rows(slot, 24, &actual_base, &rows) && rows == 0);
    /* Relocation and restore publish the pointer and its extent together. */
    p4_game_table_slot_publish(slot, 0x01000000u, 24 * 5);
    assert(P4_G32(slot) == 0x01000000u);
    assert(p4_game_table_extent_rows(slot, 24, &actual_base, &rows) && actual_base == 0x01000000u && rows == 5);
    p4_game_table_slot_publish(slot, base, 48);
    assert(P4_G32(slot) == base);
    assert(p4_game_table_extent_rows(slot, 24, &actual_base, &rows) && actual_base == base && rows == 2);
    p4_game_table_extent_note(slot, 0x01fffff0u, 24);
    P4_G32(slot) = 0x01fffff0u;
    assert(!p4_game_table_extent_rows(slot, 24, &actual_base, &rows));
    /* The fixed catalog never silently drops a binding when capacity changes. */
    for (unsigned i = 0; i < 160; i++)
        p4_game_table_extent_note(0x00764000u + i * 4, base, 24);
    assert(!p4_game_table_extent_rows(slot, 24, &actual_base, &rows));
    puts("game_table_extents: original extents, same-step publication and fail-closed readers PASS");
    return 0;
}

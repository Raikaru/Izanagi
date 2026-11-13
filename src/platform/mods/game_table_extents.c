#include "p4pc/game_table_extents.h"
#include "p4pc/gmem.h"

static struct TableExtent { uint32_t slot, base, bytes; } extents[160];
static unsigned extent_count;
static int extent_overflow;
static P4GameTableExtentObserver extent_observer;
static void *extent_observer_user;

void p4_game_table_extent_observe(P4GameTableExtentObserver observer, void *user)
{
    extent_observer = observer;
    extent_observer_user = observer ? user : NULL;
}
static void notify(uint32_t slot, uint32_t base, uint32_t bytes)
{
    if (extent_observer) extent_observer(extent_observer_user, slot, base, bytes);
}
void p4_game_table_extent_note(uint32_t slot, uint32_t base, uint32_t bytes)
{
    for (unsigned i = 0; i < extent_count; i++) {
        if (extents[i].slot == slot) {
            extents[i].base = base;
            extents[i].bytes = bytes;
            notify(slot, base, bytes);
            return;
        }
    }
    if (extent_count == sizeof extents / sizeof *extents) {
        extent_overflow = 1;
        return;
    }
    extents[extent_count++] = (struct TableExtent){slot, base, bytes};
    notify(slot, base, bytes);
}
void p4_game_table_slot_publish(uint32_t slot, uint32_t base, uint32_t bytes)
{
    P4_G32(slot) = base;
    p4_game_table_extent_note(slot, base, bytes);
}
int p4_game_table_extent_rows(uint32_t slot, uint32_t stride, uint32_t *base, uint32_t *rows)
{
    if (!base || !rows || !stride || extent_overflow || P4_GPDATA(uint32_t, 0xb438) != 8 ||
        slot < 0x00100000u || slot > 0x01fffffcu || (slot & 3)) return 0;
    for (unsigned i = 0; i < extent_count; i++) {
        const struct TableExtent *e = &extents[i];
        if (e->slot != slot) continue;
        /* A writer that skipped publication fails closed here. */
        if (e->base != P4_G32(slot) || e->base < 0x00100000u ||
            e->base > 0x02000000u || e->bytes > 0x02000000u - e->base || e->bytes % stride) return 0;
        *base = e->base;
        *rows = e->bytes / stride;
        return 1;
    }
    return 0;
}

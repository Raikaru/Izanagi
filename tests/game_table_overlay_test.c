/* The table service's session bank overlays over synthetic banks in guest
 * memory: allowlist, exact extents, one owner per bank, battle and package
 * catalog refusals, all-or-none layers, the loader/relocation observer and the
 * session-end restore at the save commit. No retail bytes or combat. */
#include "guest_test.h"
#include "p4pc/game_table_extents.h"
#include "p4pc/game_table_overlay.h"
#include "p4pc/mod_save.h"
#undef NDEBUG
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* Fixture seams: the catalog identity, the save provider registry and the
 * canonical guest byte copy. */
static int catalog_expanded;
int p4_game_table_catalog_identity(uint8_t sha256[32])
{
    memset(sha256, catalog_expanded, 32);
    return catalog_expanded;
}
static P4ModSaveProvider saved_provider;
static int registrations;
static P4ModSaveRegistration *fake_registration = (P4ModSaveRegistration *)&saved_provider;
int p4_mod_save_register(const P4ModSaveProvider *provider, P4ModSaveRegistration **out)
{
    saved_provider = *provider;
    ++registrations;
    *out = fake_registration;
    return 0;
}
static unsigned guest_copies;
void *func_0043f810(void *destination, const void *source, uint32_t count)
{
    ++guest_copies;
    return memmove(destination, source, count);
}

#define PERSONA0_SLOT 0x007644c4u
#define PERSONA1_SLOT 0x007644ccu
#define UNIT2_SLOT 0x007644c8u
#define BATTLE_STATE 0x0076449cu
static uint8_t *guest(uint32_t address) { return P4_G2H(address); }
static void load_bank(uint32_t slot, uint32_t base, uint32_t bytes, uint8_t fill)
{
    memset(guest(base), fill, bytes);
    P4_G32(slot) = base;
    p4_game_table_extent_note(slot, base, bytes);
}
static int all(uint32_t base, uint32_t bytes, uint8_t value)
{
    for (uint32_t i = 0; i < bytes; ++i)
        if (guest(base)[i] != value) return 0;
    return 1;
}

int main(void)
{
    if (p4t_guest_map(0x00764000u, 0x2000) != 0 || p4t_guest_map(0x007f0000u, 0x20000) != 0 ||
        p4t_guest_map(0x00642000u, 0x1000) != 0 || p4t_guest_map(0x0079b000u, 0x1c000) != 0)
        return 77;
    char reason[P4_TABLE_OVERLAY_REASON];
    uint8_t layer_a[64], layer_b[32], layer_c[64], stock[64];
    memset(layer_a, 0xa1, sizeof layer_a);
    memset(layer_b, 0xb2, sizeof layer_b);
    memset(layer_c, 0xc3, sizeof layer_c);

    /* The catalog: the actual descriptor labels, blocks and slots. */
    const P4TableOverlayInfo *persona1 = p4_game_tables_overlay_find("PERSONA1");
    assert(persona1 && persona1->slot == PERSONA1_SLOT && persona1->block == 1 && persona1->overlayable);
    assert(!strcmp(persona1->file, "battle/PERSONA.TBL"));
    assert(p4_game_tables_overlay_find("ENCOUNT3")->slot == 0x00764508u);
    assert(!p4_game_tables_overlay_find("MSG3")->overlayable && !p4_game_tables_overlay_find("AICALC8")->overlayable);
    assert(!p4_game_tables_overlay_find("persona1") && !p4_game_tables_overlay_info(P4_TABLE_OVERLAY_BANKS));

    P4_GPDATA(uint32_t, 0xb438) = 8; /* all eight files loaded */
    load_bank(PERSONA0_SLOT, 0x007f0000u, 64, 0x10);
    load_bank(PERSONA1_SLOT, 0x007f1000u, 64, 0x20);
    load_bank(UNIT2_SLOT, 0x007f2000u, 32, 0x30);
    P4_G32(BATTLE_STATE) = 0;
    const P4TableOverlayBank a = {"PERSONA1", layer_a, 64}, b = {"UNIT2", layer_b, 32};
    const P4TableOverlayBank pair[2] = {a, b};

    /* No session-end restore registered: nothing may be overlaid. */
    assert(p4_game_tables_overlay_apply(1, "p4pc.first", &a, 1, reason, sizeof reason) == -1);
    assert(strstr(reason, "restore") && all(0x007f1000u, 64, 0x20));
    assert(p4_game_tables_overlay_session_register() == 0 && p4_game_tables_overlay_session_register() == 0);
    assert(registrations == 1 && saved_provider.flags == 0 && !strcmp(saved_provider.owner, "p4pc"));

    /* Refusals leave every byte and the observer untouched. */
    unsigned copies = guest_copies;
    const P4TableOverlayBank unknown = {"PERSONA9", layer_a, 64}, text = {"MSG3", layer_a, 64},
                             small = {"PERSONA1", layer_a, 63}, unloaded = {"ENCOUNT0", layer_a, 64};
    assert(p4_game_tables_overlay_apply(1, "p4pc.first", &unknown, 1, reason, sizeof reason) == -1);
    assert(p4_game_tables_overlay_apply(1, "p4pc.first", &text, 1, reason, sizeof reason) == -1);
    assert(strstr(reason, "cannot be overlaid"));
    assert(p4_game_tables_overlay_apply(1, "p4pc.first", &small, 1, reason, sizeof reason) == -1);
    assert(strstr(reason, "size"));
    assert(p4_game_tables_overlay_apply(1, "p4pc.first", &unloaded, 1, reason, sizeof reason) == -1);
    const P4TableOverlayBank twice[2] = {a, a};
    assert(p4_game_tables_overlay_apply(1, "p4pc.first", twice, 2, reason, sizeof reason) == -1);
    P4_G32(BATTLE_STATE) = 0x01000000u;
    assert(p4_game_tables_overlay_apply(1, "p4pc.first", &a, 1, reason, sizeof reason) == -1);
    assert(strstr(reason, "battle"));
    P4_G32(BATTLE_STATE) = 0;
    catalog_expanded = 1;
    assert(p4_game_tables_overlay_apply(1, "p4pc.first", &a, 1, reason, sizeof reason) == -1);
    assert(strstr(reason, "package table source"));
    catalog_expanded = 0;
    P4_GPDATA(uint32_t, 0xb438) = 7; /* banks not yet complete */
    assert(p4_game_tables_overlay_apply(1, "p4pc.first", &a, 1, reason, sizeof reason) == -1);
    P4_GPDATA(uint32_t, 0xb438) = 8;
    assert(guest_copies == copies && all(0x007f1000u, 64, 0x20) && p4_game_tables_overlay_count((size_t)-1) == 0);

    /* All or none: one bad bank refuses the whole layer. */
    const P4TableOverlayBank mixed[2] = {a, small};
    assert(p4_game_tables_overlay_apply(1, "p4pc.first", mixed, 2, reason, sizeof reason) == -1);
    assert(all(0x007f1000u, 64, 0x20));

    /* Apply: live banks change, stock stays the loaded bytes. */
    assert(p4_game_tables_overlay_apply(1, "p4pc.first", pair, 2, reason, sizeof reason) == 0);
    assert(all(0x007f1000u, 64, 0xa1) && all(0x007f2000u, 32, 0xb2) && all(0x007f0000u, 64, 0x10));
    assert(p4_game_tables_overlay_stock(persona1, stock, 64) == 0 && stock[0] == 0x20 && stock[63] == 0x20);
    assert(p4_game_tables_overlay_stock(persona1, stock, 63) == -1);
    assert(p4_game_tables_overlay_count(1) == 2 && !strcmp(p4_game_tables_overlay_owner(persona1), "p4pc.first"));

    /* One owner per bank. */
    const P4TableOverlayBank c = {"PERSONA1", layer_c, 64};
    assert(p4_game_tables_overlay_apply(2, "p4pc.second", &c, 1, reason, sizeof reason) == -1);
    assert(strstr(reason, "p4pc.first") && all(0x007f1000u, 64, 0xa1));
    const P4TableOverlayBank other = {"PERSONA0", layer_c, 64};
    assert(p4_game_tables_overlay_apply(2, "p4pc.second", &other, 1, reason, sizeof reason) == 0);
    assert(all(0x007f0000u, 64, 0xc3));

    /* Replacing the layer: a bank left out gets its stock back. */
    assert(p4_game_tables_overlay_apply(1, "p4pc.first", &c, 1, reason, sizeof reason) == 0);
    assert(all(0x007f1000u, 64, 0xc3) && all(0x007f2000u, 32, 0x30) && p4_game_tables_overlay_count(1) == 1);

    /* A loader re-copy of an overlaid slot re-captures stock and re-applies. */
    load_bank(PERSONA1_SLOT, 0x007f1000u, 64, 0x21);
    assert(all(0x007f1000u, 64, 0xc3));
    assert(p4_game_tables_overlay_stock(persona1, stock, 64) == 0 && stock[0] == 0x21);
    /* A relocation to the same extent follows the bank to its new base. */
    load_bank(PERSONA1_SLOT, 0x007f3000u, 64, 0x22);
    assert(all(0x007f3000u, 64, 0xc3));
    /* Another extent drops the overlay rather than copying a mismatched layer. */
    load_bank(PERSONA1_SLOT, 0x007f4000u, 128, 0x23);
    assert(all(0x007f4000u, 128, 0x23) && p4_game_tables_overlay_count(1) == 0);
    assert(p4_game_tables_overlay_stock(persona1, stock, 64) == -1);

    /* Clearing a layer is refused during a battle too; VM teardown (process
     * shutdown only) restores the owner's banks immediately, battle or not. */
    assert(p4_game_tables_overlay_apply(1, "p4pc.first", &b, 1, reason, sizeof reason) == 0);
    P4_G32(BATTLE_STATE) = 0x01000000u;
    assert(p4_game_tables_overlay_apply(2, "p4pc.second", NULL, 0, reason, sizeof reason) == -1);
    assert(strstr(reason, "battle") && all(0x007f0000u, 64, 0xc3));
    assert(p4_game_tables_overlay_apply(3, "p4pc.third", NULL, 0, reason, sizeof reason) == 0);
    p4_game_tables_overlay_clear_owner(2);
    P4_G32(BATTLE_STATE) = 0;
    assert(all(0x007f0000u, 64, 0x10) && all(0x007f2000u, 32, 0xb2));

    /* Session end at the save commit: every bank's stock is back, no block. */
    void *staged = (void *)1;
    assert(saved_provider.encode(NULL, NULL, NULL) == 1);
    assert(saved_provider.decode(NULL, 0, NULL, &staged, NULL) == 0 && staged == NULL);
    saved_provider.commit(NULL, NULL);
    assert(all(0x007f2000u, 32, 0x30) && p4_game_tables_overlay_count((size_t)-1) == 0);
    /* With nothing overlaid the observer is gone: a later note changes nothing. */
    load_bank(UNIT2_SLOT, 0x007f2000u, 32, 0x31);
    assert(all(0x007f2000u, 32, 0x31));

    /* Static banks: the fusion charts' fixed spans, values 0..22 only. */
    const P4TableOverlayInfo *chart = p4_game_tables_overlay_find("FUSION0");
    uint32_t chart_base = 0, chart_bytes = 0;
    assert(chart && chart->overlayable && !chart->slot && chart->byte_limit == 22);
    assert(p4_game_tables_overlay_extent(chart, &chart_base, &chart_bytes));
    assert(chart_base == 0x00642147u + 25u && chart_bytes == 526u);
    assert(p4_game_tables_overlay_find("FUSION3")->address == 0x00642967u + 25u);
    memset(guest(0x00642000u), 0xee, 0x1000);
    memset(guest(chart_base), 7, chart_bytes);
    uint8_t cells[526], bad[526];
    memset(cells, 21, sizeof cells);
    memcpy(bad, cells, sizeof bad);
    bad[300] = 23;
    const P4TableOverlayBank good_chart = {"FUSION0", cells, sizeof cells}, bad_chart = {"FUSION0", bad, sizeof bad};
    assert(p4_game_tables_overlay_apply(1, "p4pc.first", &bad_chart, 1, reason, sizeof reason) == -1);
    assert(strstr(reason, "above 22") && all(chart_base, chart_bytes, 7));
    assert(p4_game_tables_overlay_apply(1, "p4pc.first", &good_chart, 1, reason, sizeof reason) == 0);
    assert(all(chart_base, chart_bytes, 21) && guest(chart_base)[-1] == 0xee && guest(chart_base)[chart_bytes] == 0xee);
    assert(p4_game_tables_overlay_stock(chart, stock, 64) == -1);
    saved_provider.commit(NULL, NULL);
    assert(all(chart_base, chart_bytes, 7) && p4_game_tables_overlay_count((size_t)-1) == 0);

    /* The retail item table (API 1.11): loaded only while the item pointer names
     * the built-in copy and no package source is published; only each row's
     * unlock-gate words (+0x3c..+0x43) may differ from stock. */
    enum { ITEM_ROWS = 1536, ITEM_ROW = 68, ITEM_BASE = 0x0079bf10, ITEM_POINTER = 0x007642a0 };
    const P4TableOverlayInfo *items = p4_game_tables_overlay_find("ITEMTBL");
    uint32_t items_base = 0, items_bytes = 0;
    assert(items && items->overlayable && !items->slot && items->address == ITEM_BASE &&
           items->size == ITEM_ROWS * ITEM_ROW && items->pointer == ITEM_POINTER);
    memset(guest(ITEM_BASE), 0x5a, ITEM_ROWS * ITEM_ROW);
    P4_G32(ITEM_POINTER) = 0; /* before the init copy publishes it */
    assert(!p4_game_tables_overlay_extent(items, &items_base, &items_bytes));
    static uint8_t table[ITEM_ROWS * ITEM_ROW], outside[ITEM_ROWS * ITEM_ROW];
    memcpy(table, guest(ITEM_BASE), sizeof table);
    table[5 * ITEM_ROW + 0x3c] = 0x01;
    table[9 * ITEM_ROW + 0x43] = 0x02;
    memcpy(outside, table, sizeof outside);
    outside[5 * ITEM_ROW + 0x34] ^= 1; /* the price */
    const P4TableOverlayBank gates = {"ITEMTBL", table, sizeof table}, priced = {"ITEMTBL", outside, sizeof outside};
    assert(p4_game_tables_overlay_apply(1, "p4pc.first", &gates, 1, reason, sizeof reason) == -1);
    assert(strstr(reason, "not loaded"));
    P4_G32(ITEM_POINTER) = ITEM_BASE;
    assert(p4_game_tables_overlay_extent(items, &items_base, &items_bytes) && items_base == ITEM_BASE &&
           items_bytes == ITEM_ROWS * ITEM_ROW);
    assert(p4_game_tables_overlay_apply(1, "p4pc.first", &priced, 1, reason, sizeof reason) == -1);
    assert(strstr(reason, "row 5 byte 0x34 outside its fields") && all(ITEM_BASE, ITEM_ROWS * ITEM_ROW, 0x5a));
    P4_G32(ITEM_POINTER) = 0x01000000u; /* an item catalog's own base */
    assert(p4_game_tables_overlay_apply(1, "p4pc.first", &gates, 1, reason, sizeof reason) == -1);
    P4_G32(ITEM_POINTER) = ITEM_BASE;
    catalog_expanded = 1;
    assert(p4_game_tables_overlay_apply(1, "p4pc.first", &gates, 1, reason, sizeof reason) == -1);
    catalog_expanded = 0;
    assert(p4_game_tables_overlay_apply(1, "p4pc.first", &gates, 1, reason, sizeof reason) == 0);
    assert(guest(ITEM_BASE)[5 * ITEM_ROW + 0x3c] == 0x01 && guest(ITEM_BASE)[9 * ITEM_ROW + 0x43] == 0x02);
    /* The init copy again: the fresh bytes become stock and the layer is back. */
    memset(guest(ITEM_BASE), 0x5a, ITEM_ROWS * ITEM_ROW);
    p4_game_tables_overlay_static_published(ITEM_BASE, ITEM_ROWS * ITEM_ROW);
    assert(guest(ITEM_BASE)[5 * ITEM_ROW + 0x3c] == 0x01 && p4_game_tables_overlay_count(1) == 1);
    /* A copy with other contents outside the gates drops the overlay, leaving the copy. */
    memset(guest(ITEM_BASE), 0x5a, ITEM_ROWS * ITEM_ROW);
    guest(ITEM_BASE)[0] = 0x11;
    p4_game_tables_overlay_static_published(ITEM_BASE, ITEM_ROWS * ITEM_ROW);
    assert(p4_game_tables_overlay_count(1) == 0 && guest(ITEM_BASE)[0] == 0x11 &&
           guest(ITEM_BASE)[5 * ITEM_ROW + 0x3c] == 0x5a);
    /* Session end restores the gates, but never once the pointer names another table. */
    table[0] = 0x11;
    assert(p4_game_tables_overlay_apply(1, "p4pc.first", &gates, 1, reason, sizeof reason) == 0);
    saved_provider.commit(NULL, NULL);
    assert(guest(ITEM_BASE)[5 * ITEM_ROW + 0x3c] == 0x5a && p4_game_tables_overlay_count((size_t)-1) == 0);
    assert(p4_game_tables_overlay_apply(1, "p4pc.first", &gates, 1, reason, sizeof reason) == 0);
    P4_G32(ITEM_POINTER) = 0x01000000u;
    saved_provider.commit(NULL, NULL);
    assert(guest(ITEM_BASE)[5 * ITEM_ROW + 0x3c] == 0x01 && p4_game_tables_overlay_count((size_t)-1) == 0);
    puts("game_table_overlay: allowlist, exact extents, owners, refusals, observer, static charts, the gate-restricted "
         "item table and session restore PASS");
    return 0;
}

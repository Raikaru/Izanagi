/* The side-ailment service over a real frozen registry and guest DatUnits:
 * publication, refusals, the per-action mark, observer events and lifecycle.
 * A state witness, not a battle qualification. */
#include "p4pc/game_side_ailment.h"
#include "p4pc/game_side_ailment_native.h"
#include "p4pc/game_table_use.h"
#include "p4pc/package_boot.h"
#include "guest_test.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

_Noreturn void p4_package_boot_runtime_failed(const char *reason)
{
    fprintf(stderr, "unexpected native refusal: %s\n", reason);
    abort();
}
static unsigned uses;
static int use(const P4GameTables *view, uint32_t domain, const uint32_t ids[5], void *context)
{
    assert(view == context && ids && domain == P4_TABLE_SOURCE_SIDE_AILMENT && (ids[0] == 24 || ids[0] == 25));
    ++uses;
    return P4_TABLE_OK;
}
typedef struct Event { uint32_t unit, leaf, event, reason; } Event;
static Event events[32];
static unsigned event_count, event_seen;
static void observe(uint32_t unit, uint32_t leaf, uint32_t event, uint32_t reason)
{
    assert(event_count < sizeof events / sizeof events[0]);
    events[event_count++] = (Event){unit, leaf, event, reason};
    /* Delivery is synchronous and refuses re-entry. */
    assert(p4_side_ailment_set(unit, leaf, P4_SIDE_REASON_MOD) == P4_TABLE_NOT_READY);
    assert(p4_side_ailment_mark_landed(unit, leaf) == P4_TABLE_NOT_READY);
    assert(p4_side_ailment_clear(unit, P4_SIDE_REASON_MOD) == P4_SIDE_DEFAULT_NONE);
}
/* The next unseen event is exactly this one. */
static void expect(uint32_t unit, uint32_t leaf, uint32_t event, uint32_t reason)
{
    assert(event_seen < event_count);
    const Event *e = &events[event_seen++];
    assert(e->unit == unit && e->leaf == leaf && e->event == event && e->reason == reason);
}
static void quiet(void) { assert(event_seen == event_count); }
static P4GameTables *side_tables(void)
{
    P4GameTables *tables = p4_game_tables_create(); assert(tables);
    for (uint32_t leaf = 24; leaf <= 25; ++leaf) {
        P4ElementDesc d = {.id = leaf, .flags = P4_ELEMENT_AFFINITY | P4_ELEMENT_VISIBLE, .family = leaf,
            .order = leaf, .revision = 1, .display_name = "Ailment", .short_name = "Ail"};
        strcpy(d.key, leaf == 24 ? "test:freeze" : "test:shock");
        assert(p4_game_tables_element(tables, "test", &d) == P4_TABLE_OK);
    }
    const P4SideAilmentDef freeze = {24, P4_SIDE_FLAGS, 0x16, 2, 100, 0, 0, 1, 1, "test:freeze-ailment"};
    const P4SideAilmentDef shock = {25, P4_SIDE_LOSE_TURN, 0x16, 3, 100, 0, 0, 1, 1, "test:shock-ailment"};
    assert(p4_game_tables_side_ailment(tables, "test", &shock) == P4_TABLE_OK);
    assert(p4_game_tables_side_ailment(tables, "test", &freeze) == P4_TABLE_OK);
    assert(!p4_game_tables_bind_runtime_use(tables, use, tables));
    assert(p4_game_tables_freeze(tables, 1) == P4_TABLE_OK);
    return tables;
}
int main(void)
{
    if (p4t_guest_init()) return 77;
    uint8_t *storage = p4t_galloc(3 * 0x40, 16); assert(storage);
    const uint32_t a = P4_H2G(storage), b = a + 0x40, c = a + 0x80;
    const P4SideAilmentDef *held = NULL;
    uint32_t leaf = 0;

    /* Nothing published: every call is inert and reads no unit. */
    p4_game_side_ailment_native_battle_begin();
    assert(!p4_side_ailment_count() && !p4_side_ailment_at(0) && !p4_side_ailment_find_leaf(24));
    assert(!p4_side_ailment_find_key("test:freeze-ailment"));
    assert(p4_side_ailment_set(a, 24, P4_SIDE_REASON_RIDER) == P4_TABLE_NOT_READY);
    assert(p4_side_ailment_mark_landed(a, 24) == P4_TABLE_NOT_READY);
    assert(!p4_side_ailment_query(a, &held) && !p4_side_ailment_landed(a, &leaf) && !p4_side_ailment_turn_lost(a));
    assert(p4_side_ailment_clear(a, P4_SIDE_REASON_CURE) == P4_SIDE_DEFAULT_NONE && !p4_side_ailment_landings(a, 24));
    p4_side_ailment_action_end();
    p4_game_side_ailment_native_unit_exit(a);
    p4_game_side_ailment_native_battle_end();
    P4GameTables *empty = p4_game_tables_create(); assert(empty && p4_game_tables_freeze(empty, 1) == P4_TABLE_OK);
    assert(p4_game_side_ailment_prepare(empty) == P4_TABLE_OK);
    p4_game_side_ailment_publish(); /* A catalog without rows publishes nothing. */
    assert(!p4_side_ailment_count());
    p4_game_side_ailment_session_end();
    p4_game_tables_destroy(empty);

    P4GameTables *tables = side_tables();
    assert(p4_game_side_ailment_prepare(NULL) == P4_TABLE_INVALID);
    assert(p4_game_side_ailment_prepare(tables) == P4_TABLE_OK && p4_game_side_ailment_prepare(tables) == P4_TABLE_OK);
    assert(!p4_side_ailment_count()); /* Not before publication. */
    p4_game_side_ailment_publish();
    const P4SideAilmentDef *freeze = p4_side_ailment_find_key("test:freeze-ailment"), *shock = p4_side_ailment_find_leaf(25);
    assert(p4_side_ailment_count() == 2 && freeze == p4_side_ailment_at(0) && shock == p4_side_ailment_at(1));
    assert(freeze->leaf == 24 && freeze == p4_side_ailment_find_leaf(24) && !p4_side_ailment_at(2));
    assert(!p4_side_ailment_find_key("test:none") && !p4_side_ailment_find_key(NULL) && !p4_side_ailment_find_leaf(26));
    p4_side_ailment_set_observer(observe);

    /* Outside a battle nothing lands. */
    assert(p4_side_ailment_set(a, 24, P4_SIDE_REASON_RIDER) == P4_TABLE_NOT_READY);
    /* The per-run gate is off by default: a battle runs, but nothing lands,
     * nothing is protected and the gate cannot change mid-battle. */
    p4_game_side_ailment_native_battle_begin();
    assert(!p4_side_ailment_active() && p4_side_ailment_count() == 2);
    assert(p4_side_ailment_set(a, 24, P4_SIDE_REASON_RIDER) == P4_TABLE_NOT_READY);
    assert(p4_side_ailment_mark_landed(a, 24) == P4_TABLE_NOT_READY);
    assert(p4_side_ailment_protect(a, 1) == P4_TABLE_NOT_READY);
    assert(p4_side_ailment_set_enabled(1) == P4_TABLE_NOT_READY);
    p4_game_side_ailment_native_battle_end();
    assert(p4_side_ailment_set_enabled(1) == P4_TABLE_OK);
    assert(p4_side_ailment_protect(a, 1) == P4_TABLE_NOT_READY); /* Outside a battle. */
    p4_game_side_ailment_native_battle_begin();
    assert(p4_side_ailment_active());
    quiet();
    /* A protected unit refuses like a dead one; the window lifts per unit. */
    assert(p4_side_ailment_protect(a, 1) == P4_TABLE_OK && p4_side_ailment_protected(a) && !p4_side_ailment_protected(b));
    assert(p4_side_ailment_set(a, 24, P4_SIDE_REASON_RIDER) == P4_TABLE_INVALID);
    assert(p4_side_ailment_mark_landed(a, 24) == P4_TABLE_INVALID);
    assert(p4_side_ailment_protect(a, 0) == P4_TABLE_OK && !p4_side_ailment_protected(a));
    assert(p4_side_ailment_protect(0, 1) == P4_TABLE_INVALID);
    quiet();
    assert(p4_side_ailment_set(a, 26, P4_SIDE_REASON_RIDER) == P4_TABLE_NOT_FOUND);
    assert(p4_side_ailment_set(a, 24, P4_SIDE_REASON_CURE) == P4_TABLE_INVALID);
    assert(p4_side_ailment_set(0, 24, P4_SIDE_REASON_RIDER) == P4_TABLE_INVALID);
    P4_G32(a + 0xC) = 0x80000u; /* Dead. */
    assert(p4_side_ailment_set(a, 24, P4_SIDE_REASON_RIDER) == P4_TABLE_INVALID);
    P4_G32(a + 0xC) = 0x100000u; /* Down is not the ailment slot. */
    uses = 0;
    assert(p4_side_ailment_set(a, 24, P4_SIDE_REASON_RIDER) == P4_TABLE_OK && uses == 1);
    expect(a, 24, P4_SIDE_EVENT_LANDED, P4_SIDE_REASON_RIDER); quiet();
    assert(p4_side_ailment_query(a, &held) == 1 && held == freeze && p4_side_ailment_query(a, NULL) == 1);
    assert(p4_side_ailment_landings(a, 24) == 1 && !p4_side_ailment_landings(a, 25) && !p4_side_ailment_landings(b, 24));
    assert(p4_side_ailment_set(a, 25, P4_SIDE_REASON_MOD) == P4_TABLE_CONFLICT); /* One side ailment per unit. */
    assert(p4_side_ailment_set(a, 24, P4_SIDE_REASON_MOD) == P4_TABLE_CONFLICT);

    /* Single slot: a retail ailment (Dizzy here) excludes Freeze, not Shock. */
    P4_G32(b + 0xC) = 1u;
    assert(p4_side_ailment_set(b, 24, P4_SIDE_REASON_RIDER) == P4_TABLE_CONFLICT);
    assert(p4_side_ailment_set(b, 25, P4_SIDE_REASON_MOD) == P4_TABLE_OK);
    expect(b, 25, P4_SIDE_EVENT_LANDED, P4_SIDE_REASON_MOD);
    assert(p4_side_ailment_clear(b, P4_SIDE_REASON_RIDER) == P4_SIDE_DEFAULT_NONE); /* Not a clear reason. */
    assert(p4_side_ailment_clear(b, P4_SIDE_REASON_CURE) == 25);
    expect(b, 25, P4_SIDE_EVENT_CLEARED, P4_SIDE_REASON_CURE);
    assert(p4_side_ailment_clear(b, P4_SIDE_REASON_CURE) == P4_SIDE_DEFAULT_NONE && !p4_side_ailment_query(b, NULL));
    assert(p4_side_ailment_landings(b, 25) == 1); /* Landings outlive the cure. */
    quiet();

    /* The lost turn, then recovery at its end. */
    assert(p4_side_ailment_turn_lost(a) == 1);
    expect(a, 24, P4_SIDE_EVENT_TURN_LOST, P4_SIDE_REASON_NONE);
    assert(!p4_side_ailment_turn_lost(b) && !p4_side_ailment_turn_lost(c));
    assert(p4_side_ailment_clear(a, P4_SIDE_REASON_RECOVERED) == 24);
    expect(a, 24, P4_SIDE_EVENT_CLEARED, P4_SIDE_REASON_RECOVERED);
    assert(p4_side_ailment_set(a, 24, P4_SIDE_REASON_RIDER) == P4_TABLE_OK && p4_side_ailment_landings(a, 24) == 2);
    expect(a, 24, P4_SIDE_EVENT_LANDED, P4_SIDE_REASON_RIDER);
    assert(p4_side_ailment_clear(a, P4_SIDE_REASON_DEFEAT) == 24);
    expect(a, 24, P4_SIDE_EVENT_CLEARED, P4_SIDE_REASON_DEFEAT);
    quiet();

    /* The per-action mark: later hits see it, its own commit consumes it. */
    assert(p4_side_ailment_mark_landed(c, 24) == P4_TABLE_OK && !p4_side_ailment_query(c, NULL));
    assert(p4_side_ailment_landed(c, &leaf) == 1 && leaf == 24 && !p4_side_ailment_landed(a, &leaf));
    assert(p4_side_ailment_mark_landed(c, 25) == P4_TABLE_CONFLICT);
    assert(p4_side_ailment_set(c, 25, P4_SIDE_REASON_RIDER) == P4_TABLE_CONFLICT);
    assert(p4_side_ailment_set(c, 24, P4_SIDE_REASON_RIDER) == P4_TABLE_OK && !p4_side_ailment_landed(c, &leaf));
    expect(c, 24, P4_SIDE_EVENT_LANDED, P4_SIDE_REASON_RIDER);
    assert(p4_side_ailment_mark_landed(c, 24) == P4_TABLE_CONFLICT); /* Already held. */
    assert(p4_side_ailment_clear(c, P4_SIDE_REASON_MOD) == 24);
    expect(c, 24, P4_SIDE_EVENT_CLEARED, P4_SIDE_REASON_MOD);
    assert(p4_side_ailment_mark_landed(c, 24) == P4_TABLE_OK);
    p4_side_ailment_action_end();
    assert(!p4_side_ailment_landed(c, &leaf) && p4_side_ailment_landings(c, 24) == 1);
    quiet();

    /* A unit's exit ends its state, landings included; others keep theirs. */
    assert(p4_side_ailment_set(c, 24, P4_SIDE_REASON_RIDER) == P4_TABLE_OK);
    expect(c, 24, P4_SIDE_EVENT_LANDED, P4_SIDE_REASON_RIDER);
    p4_game_side_ailment_native_unit_exit(c);
    expect(c, 24, P4_SIDE_EVENT_CLEARED, P4_SIDE_REASON_BATTLE_END);
    assert(!p4_side_ailment_query(c, NULL) && !p4_side_ailment_landings(c, 24) && p4_side_ailment_landings(a, 24) == 2);
    p4_game_side_ailment_native_unit_exit(b); /* Nothing held: no event. */
    quiet();

    /* Battle release ends every side ailment. */
    assert(p4_side_ailment_set(a, 24, P4_SIDE_REASON_RIDER) == P4_TABLE_OK);
    expect(a, 24, P4_SIDE_EVENT_LANDED, P4_SIDE_REASON_RIDER);
    p4_game_side_ailment_native_battle_end();
    expect(a, 24, P4_SIDE_EVENT_CLEARED, P4_SIDE_REASON_BATTLE_END);
    assert(!p4_side_ailment_query(a, NULL) && !p4_side_ailment_landings(a, 24));
    assert(p4_side_ailment_set(a, 24, P4_SIDE_REASON_RIDER) == P4_TABLE_NOT_READY);
    quiet();

    /* A battle that was never released is reset by the next one. */
    p4_game_side_ailment_native_battle_begin();
    assert(p4_side_ailment_set(b, 25, P4_SIDE_REASON_RIDER) == P4_TABLE_OK);
    expect(b, 25, P4_SIDE_EVENT_LANDED, P4_SIDE_REASON_RIDER);
    p4_game_side_ailment_native_battle_begin();
    expect(b, 25, P4_SIDE_EVENT_CLEARED, P4_SIDE_REASON_RESET);
    assert(!p4_side_ailment_query(b, NULL) && !p4_side_ailment_landings(b, 25));

    /* Battle begin lifts every protection window. */
    assert(p4_side_ailment_protect(c, 1) == P4_TABLE_OK);
    p4_game_side_ailment_native_battle_begin();
    assert(!p4_side_ailment_protected(c));
    /* Transfer is a set reason and a clear reason; BROKEN clears. */
    assert(p4_side_ailment_set(c, 25, P4_SIDE_REASON_TRANSFER) == P4_TABLE_OK);
    expect(c, 25, P4_SIDE_EVENT_LANDED, P4_SIDE_REASON_TRANSFER);
    assert(p4_side_ailment_clear(c, P4_SIDE_REASON_BROKEN) == 25);
    expect(c, 25, P4_SIDE_EVENT_CLEARED, P4_SIDE_REASON_BROKEN);
    assert(p4_side_ailment_set(c, 25, P4_SIDE_REASON_BROKEN) == P4_TABLE_INVALID);
    quiet();

    /* Session end resets the state and withdraws the definitions. */
    assert(p4_side_ailment_set(a, 24, P4_SIDE_REASON_RIDER) == P4_TABLE_OK);
    expect(a, 24, P4_SIDE_EVENT_LANDED, P4_SIDE_REASON_RIDER);
    p4_game_side_ailment_session_end();
    expect(a, 24, P4_SIDE_EVENT_CLEARED, P4_SIDE_REASON_RESET);
    quiet();
    assert(!p4_side_ailment_count() && p4_side_ailment_set(a, 24, P4_SIDE_REASON_RIDER) == P4_TABLE_NOT_READY);
    assert(!p4_side_ailment_active()); /* The gate is off again after session end. */
    p4_side_ailment_set_observer(NULL);
    p4_game_tables_destroy(tables);
    puts("game_side_ailment: publication, refusals, per-action mark, observer and lifecycle passed");
    return 0;
}

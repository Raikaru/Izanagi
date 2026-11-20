/* Inventory and money actions: include/p4pc/mod_inventory.h. */
#include "p4pc/mod_inventory.h"
#include "p4pc/game_data_catalog.h"
#include "p4pc/mod_entry.h"
#include "p4pc/mod_services.h"
#include <stdio.h>

extern int32_t datItemCount(int16_t item);
extern void datItemSetCount(int16_t item, int32_t count);
extern uint32_t datGetMoney(void);
extern int32_t datAddMoney(int32_t delta);

static int item_known(uint32_t item, const char **reason)
{
    size_t index;
    char name[P4_DATA_NAME_SIZE];
    if (item > INT16_MAX || p4_data_catalog_index(P4_DATA_ITEM, item, &index) ||
        !p4_data_catalog_name(P4_DATA_ITEM, index, name, sizeof name)) {
        *reason = "no such item (a placeholder or an ID outside the item table)";
        return 0;
    }
    return 1;
}
int p4_mod_inventory_ready(const char **reason)
{
    P4ModSceneState scene;
    p4_mod_scene_state(&scene);
    switch (scene.context) {
    case P4_MOD_SCENE_FIELD: case P4_MOD_SCENE_DUNGEON: case P4_MOD_SCENE_EVENT:
        return 0;
    case P4_MOD_SCENE_BATTLE:
        *reason = "queued until the battle ends";
        return 1;
    case P4_MOD_SCENE_TITLE:
        *reason = "no game is loaded";
        return -1;
    default: /* between scenes */
        *reason = "queued until the field is ready";
        return 1;
    }
}
int p4_mod_item_count(uint32_t item, uint32_t *count, const char **reason)
{
    if (!item_known(item, reason)) return -1;
    *count = (uint32_t)datItemCount((int16_t)item);
    return 0;
}
#define PENDING 64u
static struct Pending { uint32_t item; int32_t delta; uint8_t money; } pending[PENDING];
static size_t pending_count;
static int defer(uint32_t item, int32_t delta, int money, int32_t *changed, const char **reason)
{
    if (pending_count == PENDING) { *reason = "too many changes queued during this battle"; return -1; }
    pending[pending_count++] = (struct Pending){item, delta, (uint8_t)money};
    p4_mod_safe_point |= P4_MOD_SAFE_SERVICES;
    *changed = delta;
    return 1;
}
static void apply_item(uint32_t item, int32_t delta, int32_t *changed)
{
    const int32_t before = datItemCount((int16_t)item);
    int64_t target = (int64_t)before + delta;
    if (target < 0) target = 0;
    if (target > (int64_t)P4_MOD_ITEM_STACK) target = P4_MOD_ITEM_STACK;
    if (target != before) datItemSetCount((int16_t)item, (int32_t)target);
    *changed = datItemCount((int16_t)item) - before;
}
static void apply_money(int32_t delta, int32_t *changed)
{
    const uint32_t before = datGetMoney();
    if (delta) datAddMoney(delta); /* the game's own 0..9,999,999 clamp */
    *changed = (int32_t)(datGetMoney() - before);
}
int p4_mod_item_add(uint32_t item, int32_t delta, int32_t *changed, const char **reason)
{
    *changed = 0;
    if (!item_known(item, reason)) return -1;
    const int ready = p4_mod_inventory_ready(reason);
    if (ready < 0) return -1;
    if (ready > 0) return defer(item, delta, 0, changed, reason);
    apply_item(item, delta, changed);
    return 0;
}
uint32_t p4_mod_money(void) { return datGetMoney(); }
int p4_mod_money_add(int32_t delta, int32_t *changed, const char **reason)
{
    *changed = 0;
    const int ready = p4_mod_inventory_ready(reason);
    if (ready < 0) return -1;
    if (ready > 0) return defer(0, delta, 1, changed, reason);
    apply_money(delta, changed);
    return 0;
}
int p4_mod_inventory_pump(void)
{
    if (!pending_count) return 0;
    const char *reason;
    const int ready = p4_mod_inventory_ready(&reason);
    if (ready > 0) return 1;
    if (ready < 0) {
        fprintf(stderr, "p4pc: mods: %zu item/money changes queued during a battle were dropped: %s\n",
                pending_count, reason);
    } else {
        for (size_t i = 0; i < pending_count; ++i) {
            int32_t changed;
            if (pending[i].money) apply_money(pending[i].delta, &changed);
            else apply_item(pending[i].item, pending[i].delta, &changed);
        }
    }
    pending_count = 0;
    return 0;
}

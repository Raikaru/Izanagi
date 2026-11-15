/* Inventory and money actions (src/platform/mods/game_inventory.c) over stub
 * game accessors: validity, scene gating, stack and money clamps and the
 * reported change. The real accessors are witnessed in game. */
#include "p4pc/mod_inventory.h"
#include "p4pc/game_data_catalog.h"
#include "p4pc/mod_entry.h"
#include "p4pc/mod_services.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static uint8_t counts[2048];
static uint32_t money;
static int context = P4_MOD_SCENE_FIELD;
static unsigned sets;
uint32_t p4_mod_safe_point;

int32_t datItemCount(int16_t item) { return counts[item]; }
void datItemSetCount(int16_t item, int32_t count)
{
    ++sets;
    int32_t value = count & 0xFF;
    counts[item] = (uint8_t)(value > 99 ? 99 : value);
}
uint32_t datGetMoney(void) { return money; }
int32_t datAddMoney(int32_t delta)
{
    int64_t value = (int64_t)money + delta;
    money = (uint32_t)(value < 0 ? 0 : value > 9999999 ? 9999999 : value);
    return (int32_t)money;
}
void p4_mod_scene_state(P4ModSceneState *out) { memset(out, 0, sizeof *out); out->context = context; }
/* Items 1..1535 exist; 7 is a placeholder row. */
int p4_data_catalog_index(P4DataKind kind, uint32_t id, size_t *index)
{
    assert(kind == P4_DATA_ITEM);
    if (id == 0 || id >= 1536) return -1;
    *index = id;
    return 0;
}
const char *p4_data_catalog_name(P4DataKind kind, size_t index, char *out, size_t capacity)
{
    assert(kind == P4_DATA_ITEM);
    if (index == 7) return NULL;
    snprintf(out, capacity, "Item %zu", index);
    return out;
}

int main(void)
{
    const char *reason = NULL;
    uint32_t count;
    int32_t changed;
    assert(p4_mod_item_count(7, &count, &reason) == -1 && strstr(reason, "no such item"));
    assert(p4_mod_item_count(4000, &count, &reason) == -1);
    assert(!p4_mod_item_count(1, &count, &reason) && count == 0);

    assert(!p4_mod_item_add(1, 3, &changed, &reason) && changed == 3 && counts[1] == 3);
    assert(!p4_mod_item_add(1, 120, &changed, &reason) && changed == 96 && counts[1] == 99);
    assert(!p4_mod_item_add(1, 5, &changed, &reason) && changed == 0);
    assert(!p4_mod_item_add(1, -100, &changed, &reason) && changed == -99 && counts[1] == 0);
    unsigned before = sets;
    assert(!p4_mod_item_add(1, -1, &changed, &reason) && changed == 0 && sets == before); /* nothing to take */
    assert(p4_mod_item_add(7, 1, &changed, &reason) == -1 && !changed);

    money = 100;
    assert(!p4_mod_money_add(5000, &changed, &reason) && changed == 5000 && p4_mod_money() == 5100);
    assert(!p4_mod_money_add(-6000, &changed, &reason) && changed == -5100 && money == 0);
    money = 9999990;
    assert(!p4_mod_money_add(100, &changed, &reason) && changed == 9 && money == 9999999);

    /* During a battle changes queue (validated now) and apply at the first
     * safe point outside it; at the title they are refused or dropped. */
    context = P4_MOD_SCENE_BATTLE;
    money = 10;
    assert(p4_mod_item_add(1, 2, &changed, &reason) == 1 && changed == 2 && strstr(reason, "battle") && !counts[1]);
    assert(p4_mod_item_add(7, 2, &changed, &reason) == -1); /* still validated at once */
    assert(p4_mod_money_add(-20, &changed, &reason) == 1 && changed == -20 && money == 10);
    assert(p4_mod_safe_point & P4_MOD_SAFE_SERVICES);
    assert(!p4_mod_item_count(1, &count, &reason)); /* reads are always legal */
    assert(p4_mod_inventory_pump() == 1 && !counts[1]);
    context = P4_MOD_SCENE_NONE;
    assert(p4_mod_inventory_pump() == 1 && !counts[1]);
    context = P4_MOD_SCENE_FIELD;
    assert(!p4_mod_inventory_pump() && counts[1] == 2 && money == 0);
    assert(!p4_mod_inventory_pump());
    context = P4_MOD_SCENE_BATTLE;
    assert(p4_mod_item_add(1, 1, &changed, &reason) == 1);
    context = P4_MOD_SCENE_TITLE;
    assert(!p4_mod_inventory_pump() && counts[1] == 2); /* quit to title: dropped */
    assert(p4_mod_item_add(1, 1, &changed, &reason) == -1 && strstr(reason, "no game"));
    context = P4_MOD_SCENE_DUNGEON;
    assert(!p4_mod_item_add(1, 1, &changed, &reason) && changed == 1);
    puts("game_inventory: item validity, scene gating, stack and money clamps and the applied change");
    return 0;
}

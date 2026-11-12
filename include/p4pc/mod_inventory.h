#ifndef P4PC_MOD_INVENTORY_H
#define P4PC_MOD_INVENTORY_H
/* Inventory and money actions (capabilities inventory@1, give.item@1,
 * give.money@1): the game's own accessors (datItemCount/datItemSetCount
 * 0x00106600/0x00106620, which also own expanded item IDs and the key-item
 * flags, and datGetMoney/datAddMoney 0x00105ED0/0x00106020). A change made
 * while a game is loaded outside battle applies at once; one made during a
 * battle is validated, queued and applied at the first frame boundary after
 * it (game_services.c's safe-point pump). Items clamp to the game's 99 per
 * stack and money to 0..9,999,999. Refusals change nothing and give a static
 * reason. */
#include <stddef.h>
#include <stdint.h>

#define P4_MOD_ITEM_STACK 99u
#define P4_MOD_MONEY_MAX 9999999u

/* 0 and the stack count, or -1 with reason for an ID that is no item. */
int p4_mod_item_count(uint32_t item, uint32_t *count, const char **reason);
/* Adds delta (negative takes) within 0..99. 0 applied (*changed the applied
 * change), 1 queued until the battle ends (*changed the requested delta),
 * -1 refused (reason). */
int p4_mod_item_add(uint32_t item, int32_t delta, int32_t *changed, const char **reason);
uint32_t p4_mod_money(void);
/* Adds delta (negative takes) within 0..9,999,999; results as p4_mod_item_add. */
int p4_mod_money_add(int32_t delta, int32_t *changed, const char **reason);
/* 0 in a field, dungeon or event; 1 during a battle or between scenes (changes
 * queue); -1 at the title (no game is loaded). */
int p4_mod_inventory_ready(const char **reason);
/* Applies changes queued during a battle once outside it; nonzero while some
 * remain (the services safe-point pump keeps running). */
int p4_mod_inventory_pump(void);
#endif

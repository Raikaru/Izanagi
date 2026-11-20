/* Equipment exchange for mods (p4.equipment, docs/modding/lua.md "Equipment
 * exchange"): put one held item on a party member, returning the item it
 * replaces to the items, the way the Camp equipment menu does (0x0013D5D0).
 * A request is validated whole and queued; the next frame safe point outside
 * battle revalidates against the state as it is then and equips or refuses.
 * Nothing is written unless every check passes. Game thread only. */
#ifndef P4PC_MOD_EQUIPMENT_H
#define P4PC_MOD_EQUIPMENT_H

#include <stddef.h>
#include <stdint.h>

typedef struct P4ModEquip {
    uint8_t character;  /* party character ID: 1 Yu, 2 Yosuke, 3 Chie, 4 Yukiko, 6 Kanji, 7 Naoto, 8 Teddie; joined */
    uint8_t slot;       /* 0 weapon, 1 armor, 2 accessory */
    uint16_t item;      /* an item of that slot the character may equip, at least one held */
    int16_t previous;   /* outcome only: the item it replaced (0 or negative: none) */
} P4ModEquip;

/* Outcome of the owner's last applied request: EQUIPPED, or REFUSED because
 * the state changed after queueing (reason says how; nothing was written). */
enum { P4_MOD_EQUIP_NONE = 0, P4_MOD_EQUIP_EQUIPPED = 1, P4_MOD_EQUIP_REFUSED = 2 };

/* The item the character has in a slot now (0 or negative: none). 0, or -1
 * with a reason for a character or slot outside the party's equipment. */
int p4_mod_equipment_get(int character, int slot, int32_t *item, char *reason, size_t capacity);
/* Validates a request against the state now without queueing it; previous
 * gets the item it would replace. 0, or -1 with a reason. */
int p4_mod_equipment_check(const P4ModEquip *equip, int16_t *previous, char *reason, size_t capacity);
/* Validates now and queues for the next frame safe point outside battle.
 * 0, or -1 with a reason; a refusal queues nothing. One request at a time. */
int p4_mod_equipment_queue(size_t owner, const P4ModEquip *equip, char *reason, size_t capacity);
/* The safe point's step. Nonzero while waiting for a battle to end. */
int p4_mod_equipment_apply_pending(void);
/* Takes the owner's last outcome (P4_MOD_EQUIP_*), clearing it; reason gets
 * the refusal text. NONE while nothing has completed since the last take. */
int p4_mod_equipment_take_result(size_t owner, P4ModEquip *equip, char *reason, size_t capacity);
/* 1 while the owner has a request queued. */
int p4_mod_equipment_pending(size_t owner);
/* An owner's VM teardown drops its queued request and outcome. */
void p4_mod_equipment_clear_owner(size_t owner);

#endif

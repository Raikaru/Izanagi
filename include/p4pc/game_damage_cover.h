#ifndef P4PC_GAME_DAMAGE_COVER_H
#define P4PC_GAME_DAMAGE_COVER_H
#include <stdint.h>
#include "p4pc/game_tables.h"
/* Damage cover (docs/modding/tables.md "Damage cover"): the sealed catalog's
 * <damage-cover> rows become one MULTIPLY modifier per owning package in the
 * final-damage stage, at this priority and that package's load order. */
#define P4_DAMAGE_COVER_PRIORITY 1000
/* The owning package's load position for an owner of cover rows. */
typedef uint32_t (*P4DamageCoverLoadOrderFn)(const char *owner);
/* Catalog runtime preparation: plan one modifier per owner of the frozen
 * tables' cover rows, registering nothing (no rows plan nothing). Publication
 * registers the plan; an allocation failure there is a controlled runtime
 * failure. Session end unregisters every modifier and forgets the tables. */
int p4_game_damage_cover_prepare(const P4GameTables *tables, P4DamageCoverLoadOrderFn load_order);
void p4_game_damage_cover_publish(void);
void p4_game_damage_cover_session_end(void);
/* The factor one owner's cover rows give a hit of element_id on a unit (side
 * and DatUnit id, as the stage reports them): the product over that owner's
 * families of (100 - the highest matching percent) / 100. Returns 1 with
 * *factor set, 0 when nothing matches, or a P4_TABLE_* error. Sources are read
 * through p4_game_elements_unit_sources, as the added affinity effects read
 * them; runtime records each applied row's use through the table service. No
 * RNG and no game-state write. */
int p4_game_damage_cover_factor(const P4GameTables *tables, const char *owner,
    uint32_t enemy, uint32_t unit_id, uint32_t element_id, int runtime, double *factor);
#endif

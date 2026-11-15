/* Golden mode ID domains (docs/golden_mode/README.md, plan GATE A: full Golden content).
 *
 * Golden numbers skills, enemies, items and personas past the PS2 game's ranges. The port keeps every ID the PS2
 * code and the PS2 save formats already know and places each Golden-only entity at an expanded port ID that the
 * table service's existing expansion admits (skills 576..4095, enemies 336..4095, items 1536..32765, personas
 * 256..511), so Golden mode needs no change to a PS2 code path, constant or save layout:
 *
 *   skills    Golden actives below 440 keep their ID. Golden's own actives 440..A-1 (A = Golden's active count)
 *             become 576.., right after PS2's domain. A Golden passive g is PS2 passive g - (A - 440) while that
 *             is below 576 (Golden shifts PS2's passives up by the number of actives it adds); the passives
 *             past PS2's domain follow the Golden-only actives.
 *   enemies   identity; IDs from 336 are Golden-only (dense, as the expansion requires).
 *   items     identity; IDs from 1536 are Golden-only.
 *   personas  identity; IDs from 256 (none in the measured build) are Golden-only.
 *
 * Every function here is pure CPU work over caller-owned data; nothing touches guest memory. Declaring entities
 * refuses outside Golden mode, so PS2 mode can never register a Golden row. */
#ifndef P4PC_GOLDEN_DOMAINS_H
#define P4PC_GOLDEN_DOMAINS_H
#include <stddef.h>
#include <stdint.h>
#include "p4pc/game_mode.h"
#include "p4pc/game_tables.h"
#include "p4pc/game_skills.h"
#include "p4pc/game_enemies.h"
#include "p4pc/game_items.h"
#include "p4pc/game_personas.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum P4GoldenDomain {
    P4_GOLDEN_DOMAIN_SKILL, P4_GOLDEN_DOMAIN_ENEMY, P4_GOLDEN_DOMAIN_ITEM, P4_GOLDEN_DOMAIN_PERSONA,
    P4_GOLDEN_DOMAIN_COUNT
} P4GoldenDomain;

/* The PS2 retail domains and expansion ceilings are the table families' own constants (P4_SKILL_RETAIL_COUNT,
 * P4_SKILL_RETAIL_ACTIVE_COUNT, P4_SKILL_MAX_ID, P4_ENEMY_RETAIL_ROWS, P4_ENEMY_MAX_ID, P4_ITEM_RETAIL_COUNT,
 * P4_ITEM_MAX_ID, P4_PERSONA_RETAIL_ROWS, P4_PERSONA_MAX_ID).
 *
 * Golden's domain sizes come from the converted tables (docs/golden_mode/specs/battle_tables.md): build 10836033
 * has 624 skills of which 472 are active, 368 enemies, 2560 items and 256 personas. */
typedef struct P4GoldenDomainShape {
    uint32_t skills, active_skills, enemies, items, personas;
} P4GoldenDomainShape;

typedef struct P4GoldenDomainMap {
    P4GoldenDomainShape shape;
    uint32_t added_actives;   /* Golden actives past PS2's 440 */
    uint32_t mapped_passives; /* Golden passives that land in PS2's passive slots */
} P4GoldenDomainMap;

/* Validates the shape against the PS2 domains and the expansion ceilings. 0, or -1 with reason. */
int p4_golden_domain_map_build(P4GoldenDomainMap *map, const P4GoldenDomainShape *shape, char *reason, size_t capacity);

/* Golden ID -> port ID. 0 mapped, -1 outside Golden's domain. *expanded (optional) is 1 for a Golden-only
 * entity, which needs declaring, and 0 for an ID the PS2 game already has. */
int p4_golden_domain_to_port(const P4GoldenDomainMap *map, P4GoldenDomain domain, uint32_t golden, uint32_t *port,
                             int *expanded);
/* Port ID -> Golden ID. 0 mapped, -1 when the port ID has no Golden entity (a PS2 slot Golden does not use, or an
 * ID past both domains). */
int p4_golden_domain_from_port(const P4GoldenDomainMap *map, P4GoldenDomain domain, uint32_t port, uint32_t *golden);
/* Number of Golden-only (expanded) entities in a domain. */
uint32_t p4_golden_domain_expanded_count(const P4GoldenDomainMap *map, P4GoldenDomain domain);

/* Golden-only entities to declare, each keyed by its Golden ID. Rows are already in the PS2 row layout the
 * table service validates (the M5 converters produce them); this layer only checks coverage, assigns the port ID
 * and key, and registers them. */
typedef struct P4GoldenDeclaredSkill { uint32_t golden_id; P4GameSkillDesc desc; } P4GoldenDeclaredSkill;
typedef struct P4GoldenDeclaredEnemy { uint32_t golden_id; P4GameEnemyDesc desc; } P4GoldenDeclaredEnemy;
typedef struct P4GoldenDeclaredItem { uint32_t golden_id; P4GameItemDesc desc; } P4GoldenDeclaredItem;
typedef struct P4GoldenDeclaredPersona { uint32_t golden_id; P4GamePersonaDesc desc; } P4GoldenDeclaredPersona;

typedef struct P4GoldenDeclarations {
    const P4GoldenDeclaredSkill *skills; size_t skill_count;
    const P4GoldenDeclaredEnemy *enemies; size_t enemy_count;
    const P4GoldenDeclaredItem *items; size_t item_count;
    const P4GoldenDeclaredPersona *personas; size_t persona_count;
} P4GoldenDeclarations;

/* Where declarations go: the table families' own register functions in production. Each returns a P4_TABLE_*
 * status. */
typedef struct P4GoldenDeclareSink {
    void *user;
    int (*skill)(void *user, const char *owner, const P4GameSkillDesc *desc);
    int (*enemy)(void *user, const char *owner, const P4GameEnemyDesc *desc);
    int (*item)(void *user, const char *owner, const P4GameItemDesc *desc);
    int (*persona)(void *user, const char *owner, const P4GamePersonaDesc *desc);
} P4GoldenDeclareSink;

/* Owner of every Golden-mode declaration; keys are "<owner>:<domain>-<golden id in hex>". */
#define P4_GOLDEN_DECLARE_OWNER "golden-mode"

/* Registers every Golden-only entity of the map, exactly once each, at its port ID and key. Refuses (-1, reason,
 * nothing registered) outside Golden mode, when a Golden-only ID has no declaration, when a declaration names an
 * ID the PS2 game already has or one outside Golden's domain, or when a declaration is duplicated; a sink refusal
 * stops at that entity (-1, reason). 0 when all are registered. */
int p4_golden_domains_declare(P4GameMode mode, const P4GoldenDomainMap *map, const P4GoldenDeclarations *declarations,
                              const P4GoldenDeclareSink *sink, char *reason, size_t capacity);

#ifdef __cplusplus
}
#endif
#endif

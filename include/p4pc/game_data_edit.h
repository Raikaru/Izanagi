/* p4.data edits: typed field-level changes to the game tables, written by a
 * mod's data.luau or data/edits.xml (docs/modding/systems/data-and-edits.md).
 *
 * Every enabled mod with either file contributes one layer. The layers run
 * once per boot at table seal, inside the derived compose stage (after every
 * derived producer and table delta), in package load order; within a mod,
 * edits.xml before data.luau. Each layer reads the rows as the layers before
 * it left them, so relative edits compose. Writes land in the composed
 * inputs, which the loader's buffers are rewritten with on every load; a mod
 * that is disabled is simply absent on the next boot. When two layers assign
 * the same field, the later one wins and the pair is recorded as a conflict.
 * A refused write refuses the whole mod (admission then retries without it),
 * so no layer is ever partly applied. */
#ifndef P4PC_GAME_DATA_EDIT_H
#define P4PC_GAME_DATA_EDIT_H
#include <stddef.h>
#include <stdint.h>
#include "p4pc/game_data.h"
#include "p4pc/packages.h"

#define P4_DATA_EDIT_LUAU "data.luau"
#define P4_DATA_EDIT_XML "data/edits.xml"
#define P4_DATA_EDIT_LUAU_MAX (1024u * 1024u)
#define P4_DATA_EDIT_XML_MAX (256u * 1024u)

/* ---- boot ---- */

/* Package boot, from the game-data handler (phase 100, before the
 * table-source handler seals its builder): records one layer per admitted
 * package with data.luau or data/edits.xml in load order, requests the
 * inputs they edit and joins the derived compose stage as its layers. 0, or
 * -1 after refusing a package. Table boot destroy/rollback resets them, as
 * discard does for the handler's own rollback. */
int p4_data_edits_prepare(void);
void p4_data_edits_discard(void);
size_t p4_data_edits_count(void);

/* ---- writes (edits.xml decoder and the data.luau runner) ---- */

/* A value is a number, or text a field resolves by name (an element, an
 * affinity, a status, a skill or item name or key). */
typedef struct P4DataEditValue {
    int text;
    double number;
    const char *string;
} P4DataEditValue;
/* Assigns one field of one row in the running layer. Paths:
 *   skill:   name, power, cost.sp, cost.hp_percent, accuracy, critical,
 *            hits.min, hits.max, element, ailment.status, ailment.chance
 *   enemy:   name, level, hp, sp, stats.st|ma|en|ag|lu, exp, yen,
 *            affinities.<element>, skills.1..8, drops.1..4.item,
 *            drops.1..4.chance
 *   persona: name, level, stats.st|ma|en|ag|lu, affinities.<element>
 *   item:    name, price, attack, hit, defense, evasion, skill, shop.sold
 *            ("true"/"false"), shop.from.month, shop.from.day,
 *            shop.unlock ("none"), shop.unlock.1..2.material,
 *            shop.unlock.1..2.count
 * Text values are names, keys, "none" or "true"/"false". 0 written; -1 with
 * a readable reason (unknown field, out of range, a name that is not
 * found). Outside a running layer: -1. */
int p4_data_edit_set(P4DataKind kind, size_t index, const char *path, const P4DataEditValue *value,
    char *error, size_t capacity);
/* Persona learnsets. add: a learn event of skill at level (absolute);
 * remove: drops skill's event; move: skill's event to level; replace: skill's
 * event teaches with instead. Party Personas keep their events sorted by
 * level, as the level-up scan requires. 0 done, -1 with a reason. */
typedef enum P4DataLearnOp { P4_DATA_LEARN_ADD, P4_DATA_LEARN_REMOVE, P4_DATA_LEARN_MOVE, P4_DATA_LEARN_REPLACE } P4DataLearnOp;
int p4_data_edit_learn(size_t persona, P4DataLearnOp op, uint32_t skill, uint32_t value,
    char *error, size_t capacity);
/* The running layer's package id, or NULL outside one. */
const char *p4_data_edit_owner(void);

/* ---- data.luau ---- */

/* The Luau runner (lua_data_edit.c, registered by the game bindings): runs a
 * package's data.luau in a restricted VM over the running layer. 0 ran, -1
 * with a readable reason. Without a runner a data.luau refuses its mod. */
typedef int (*P4DataEditRunner)(const P4PackageProvider *provider, const char *source, size_t size,
    char *error, size_t capacity);
void p4_data_edits_runner(P4DataEditRunner runner);

/* ---- conflicts (main.luau p4.data.conflicts) ---- */

typedef struct P4DataConflict {
    P4DataKind kind;
    uint32_t id;
    const char *field;          /* the written path, e.g. "power" or "learnset.p4:skill:bufula" */
    const char *owners[8];      /* every writer in load order; the last one won */
    size_t owner_count;
} P4DataConflict;
size_t p4_data_edits_conflict_count(void);
int p4_data_edits_conflict_at(size_t index, P4DataConflict *out);

/* ---- session edits (main.luau p4.data.session.edit) ---- */

/* The same fields and learnsets, written into the tables the game has
 * loaded for this run only. Game thread, after boot. begin binds the loaded
 * banks and makes p4_data_edit_set/learn write them for owner (a package id);
 * persist keeps this call's edits in the save. end checks the cross-field
 * rules; keep = 0 or a broken rule undoes every write since begin (-1 with
 * the reason). Item names and item rows of a build with added items change
 * only at boot (data.luau). */
int p4_data_session_begin(const char *owner, int persist, char *error, size_t capacity);
int p4_data_session_end(int keep, char *error, size_t capacity);
/* Session edits made so far and persisted ones. */
size_t p4_data_session_count(void);
size_t p4_data_session_persisted(void);
/* Undo every session edit (infallible, allocation-free): what a save load or
 * a new game does first. */
void p4_data_session_revert(void);
/* The "p4.data"/"session" save block: persisted records, re-applied after a
 * save that carries them loads (game thread; once, before any save loads). */
int p4_data_session_save_register(void);
#endif

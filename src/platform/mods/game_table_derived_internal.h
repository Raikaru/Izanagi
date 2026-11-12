#ifndef P4PC_GAME_TABLE_DERIVED_INTERNAL_H
#define P4PC_GAME_TABLE_DERIVED_INTERNAL_H
#include <stddef.h>
#include <stdint.h>
#include "p4pc/content_digest.h"
#include "p4pc/game_table_inputs.h"
#include "p4pc/game_tables.h"
#include "p4pc/packages.h"
#include "p4pc/game_table_derived.h" /* P4_TABLE_DERIVED_ITEM_NAME_MAX */

/* Table boot: one [table-derived KEY] section of a package whose section gate
 * is on. *inputs receives the declared input mask. 0 selected, -1 refused
 * with a bounded reason in error. */
int p4_game_tables_derived_select(const P4PackageProvider *provider, const P4PackageField *fields,
    size_t count, uint32_t *inputs, char *error, size_t capacity);
size_t p4_game_tables_derived_count(void);
/* Mod-authored named-field deltas ([table-delta KEY], game_table_delta.c).
 * They patch named fields of the composed rows inside
 * p4_game_tables_derived_compose, after every producer and in package boot
 * order; they own no family. Skill fields patch retail active rows of SKILL 1;
 * learn deltas patch the 32 learn events of a party Persona's PERSONA 2 row,
 * which keeps its 622 bytes and is re-sorted by level afterwards. */
typedef enum P4TableDeltaKind {
    P4_TABLE_DELTA_SKILL_POWER,    /* +0x12 s16, 1..9999 */
    P4_TABLE_DELTA_SKILL_COST,     /* +0x04 u16, 1..999 (SP, or HP percent) */
    P4_TABLE_DELTA_SKILL_ACCURACY, /* +0x0E u8, 1..100 */
    P4_TABLE_DELTA_SKILL_CRITICAL, /* +0x25 u8, 0..100 */
    P4_TABLE_DELTA_LEARN_LEVEL,    /* the learn event of `skill` moves to level `value` */
    P4_TABLE_DELTA_LEARN_REPLACE,  /* the learn event of `skill` teaches skill `value` instead */
    P4_TABLE_DELTA_LEARN_ADD,      /* a free event learns `skill` at level `value` */
    P4_TABLE_DELTA_KIND_COUNT
} P4TableDeltaKind;
typedef struct P4TableDelta {
    P4TableDeltaKind kind;
    uint32_t row;   /* skill id (1..439), or party Persona id (0xC0..0xD7) */
    uint32_t skill; /* learn deltas: the event's skill, or the added one */
    uint32_t value; /* field value, learn level (0..99) or replacement skill */
} P4TableDelta;
/* Table boot: one [table-delta KEY] declaration, decoded. Each delta is
 * range-checked and copied. A target (row and field, or Persona and skill)
 * named twice in one declaration refuses it; one another package's
 * declaration already names refuses both packages. *inputs receives the
 * inputs the deltas patch. 0 recorded, -1 refused with a bounded reason. */
int p4_game_tables_derived_delta_add(const P4PackageProvider *provider, const char *key, uint32_t revision,
    const char *content_hex, const P4TableDelta *deltas, size_t count, uint32_t *inputs, char *error, size_t capacity);
size_t p4_game_tables_derived_delta_count(void);
/* The section parser and file decoder (game_table_delta.c): 0 recorded, -1
 * refused with a bounded reason in error. */
int p4_game_tables_delta_definition(const P4PackageProvider *provider, const P4PackageField *fields,
    size_t count, uint32_t *inputs, char *error, size_t capacity);
/* Builder seal, after every requested input is prepared and sealed and before
 * any family reads a span: run each selected producer against the uncomposed
 * inputs, check that relocated ENCOUNT rows name only existing rows, check and
 * stage the table deltas over the produced rows, then replace the produced
 * rows and spans in place and patch the deltas' fields; relocated spans stay
 * aside for native publication. Input fingerprints stay those of the selected
 * originals. */
int p4_game_tables_derived_compose(P4GameTableInput *const *inputs, size_t count,
    const P4PackageProvider **failed_provider);
typedef enum P4TableDerivedDomain {
    P4_TABLE_DERIVED_SKILL, P4_TABLE_DERIVED_ENEMY, P4_TABLE_DERIVED_ITEM
} P4TableDerivedDomain;
/* Builder seal, once every family is sealed: each nonzero skill, enemy or
 * item ID a relocated block names (PERSONA 4 skill pairs, ENCOUNT formation
 * enemies and record-group items), and each skill a learn delta teaches, must
 * satisfy known (a retail ID or a declared one). Otherwise refuses the
 * producing or declaring package with P4_TABLE_NOT_FOUND. */
int p4_game_tables_derived_check_references(int (*known)(const void *context, P4TableDerivedDomain domain, uint32_t id),
    const void *context, const P4PackageProvider **failed_provider);
/* A relocated span (game_table_derived.h): the selected input whose original
 * block the loader still loads, and the producer's bytes. Both are borrowed
 * until reset. 1 found, 0 none. */
int p4_game_tables_derived_relocated(P4GameTableInputKind input, uint32_t span,
    const P4GameTableInput **loaded, const uint8_t **bytes, size_t *size);
/* Edit layers, during compose: a relocated span's rows to edit in place
 * (their digest is taken after the layers), or NULL when not relocated. */
uint8_t *p4_game_tables_derived_relocated_rows(P4GameTableInputKind input, uint32_t span, size_t *size);
/* Canonical serialization of every selection, fingerprint, composed input
 * digest and relocated span digest, then every delta declaration and delta;
 * appended to the catalog identity only when a producer is selected or a
 * delta declared. */
void p4_game_tables_derived_identity(P4ContentDigest *digest);
/* Admission: the loaded file already holds this input's composed bytes. */
int p4_game_tables_derived_matches(const P4GameTableInput *input, const void *bytes, size_t size);
/* Admission, after the loaded file matched the selected original: overwrite it
 * with the composed bytes before the native copy. No-op for other inputs. */
void p4_game_tables_derived_rewrite(const P4GameTableInput *input, uint8_t *bytes, size_t size);
/* Table boot destroy/rollback: forget selections and productions, and reset
 * and forget the registered layers. Registered producers stay. */
void p4_game_tables_derived_reset(void);
/* The table boot published the composed catalog: recorded sources become
 * readable (p4_table_derived_source) until reset. */
void p4_game_tables_derived_expose(void);
/* Composition layers after every producer and delta: the p4.data edit layers
 * (game_data_edit.c), registered by their boot handler during table prepare,
 * before the table-source handler seals. apply edits the inputs in place and
 * calls composed() for each it changed; a refused layer refuses its package
 * and returns -1 with *failed_provider. identity appends to the catalog
 * identity; reset runs at table boot destroy/rollback. */
typedef struct P4TableDerivedLayers {
    size_t (*count)(void);
    int (*apply)(P4GameTableInput *const *inputs, size_t count, void (*composed)(const P4GameTableInput *input),
        const P4PackageProvider **failed_provider);
    void (*identity)(P4ContentDigest *digest);
    void (*reset)(void);
} P4TableDerivedLayers;
void p4_game_tables_derived_layers(const P4TableDerivedLayers *layers);
size_t p4_game_tables_derived_layer_count(void);
/* Retail item display names (p4_table_derived_sink_item_names). valid: 1 to
 * P4_TABLE_DERIVED_ITEM_NAME_MAX - 1 printable ASCII characters. set: an
 * edit layer renames during apply (later wins). name: the composed name, or
 * NULL for the executable's; valid until reset. */
static inline int p4_game_tables_derived_item_name_valid(const char *name)
{
    if (!name || !*name) return 0;
    for (size_t length = 0; name[length]; ++length)
        if ((unsigned char)name[length] < 0x20 || (unsigned char)name[length] > 0x7E ||
            length + 1u >= P4_TABLE_DERIVED_ITEM_NAME_MAX) return 0;
    return 1;
}
int p4_game_tables_derived_item_name_set(uint32_t id, const char *name);
const char *p4_game_tables_derived_item_name(uint32_t id);
size_t p4_game_tables_derived_item_name_count(void);

/* Native publication of relocated spans and item names
 * (game_table_derived_native.c). Table boot, before raw exposure: register
 * the `table-derived` completion participant when a span is relocated or an
 * item renamed (nothing otherwise). Its preparation checks the loaded
 * original and copies the list, plus a zero guard, and the names through
 * allocator; its commit moves the slot with the new extent and publishes the
 * names to datGetItemNameId (p4pc/game_item_names_native.h). */
int p4_game_tables_derived_boot_stage(const P4GameTableAllocator *allocator);
/* Exact EARLY rollback only; a prepared copy locks the stage. */
int p4_game_tables_derived_boot_unstage(void);
/* Session end once tasks drain: a live image gets each loaded original block
 * back in its slot; the copies are released. */
int p4_game_tables_derived_session_end(int image_live);
#endif

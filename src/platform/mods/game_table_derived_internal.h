#ifndef P4PC_GAME_TABLE_DERIVED_INTERNAL_H
#define P4PC_GAME_TABLE_DERIVED_INTERNAL_H
#include <stddef.h>
#include <stdint.h>
#include "p4pc/content_digest.h"
#include "p4pc/game_table_inputs.h"
#include "p4pc/game_tables.h"
#include "p4pc/packages.h"

/* Table boot: one [table-derived KEY] section of a package whose section gate
 * is on. *inputs receives the declared input mask. 0 selected, -1 refused
 * with a bounded reason in error. */
int p4_game_tables_derived_select(const P4PackageProvider *provider, const P4PackageField *fields,
    size_t count, uint32_t *inputs, char *error, size_t capacity);
size_t p4_game_tables_derived_count(void);
/* Builder seal, after every requested input is prepared and sealed and before
 * any family reads a span: run each selected producer against the uncomposed
 * inputs, check that relocated ENCOUNT rows name only existing rows, then
 * replace the produced rows and spans in place; relocated spans stay aside
 * for native publication. Input fingerprints stay those of the selected
 * originals. */
int p4_game_tables_derived_compose(P4GameTableInput *const *inputs, size_t count,
    const P4PackageProvider **failed_provider);
typedef enum P4TableDerivedDomain {
    P4_TABLE_DERIVED_SKILL, P4_TABLE_DERIVED_ENEMY, P4_TABLE_DERIVED_ITEM
} P4TableDerivedDomain;
/* Builder seal, once every family is sealed: each nonzero skill, enemy or
 * item ID a relocated block names (PERSONA 4 skill pairs, ENCOUNT formation
 * enemies and record-group items) must satisfy known (a retail ID or a
 * declared one). Otherwise refuses the producing package with
 * P4_TABLE_NOT_FOUND. */
int p4_game_tables_derived_check_references(int (*known)(const void *context, P4TableDerivedDomain domain, uint32_t id),
    const void *context, const P4PackageProvider **failed_provider);
/* A relocated span (game_table_derived.h): the selected input whose original
 * block the loader still loads, and the producer's bytes. Both are borrowed
 * until reset. 1 found, 0 none. */
int p4_game_tables_derived_relocated(P4GameTableInputKind input, uint32_t span,
    const P4GameTableInput **loaded, const uint8_t **bytes, size_t *size);
/* Canonical serialization of every selection, fingerprint, composed input
 * digest and relocated span digest; appended to the catalog identity only when
 * a producer is selected. */
void p4_game_tables_derived_identity(P4ContentDigest *digest);
/* Admission: the loaded file already holds this input's composed bytes. */
int p4_game_tables_derived_matches(const P4GameTableInput *input, const void *bytes, size_t size);
/* Admission, after the loaded file matched the selected original: overwrite it
 * with the composed bytes before the native copy. No-op for other inputs. */
void p4_game_tables_derived_rewrite(const P4GameTableInput *input, uint8_t *bytes, size_t size);
/* Table boot destroy/rollback: forget selections and productions. Registered
 * producers stay. */
void p4_game_tables_derived_reset(void);
/* The table boot published the composed catalog: recorded sources become
 * readable (p4_table_derived_source) until reset. */
void p4_game_tables_derived_expose(void);

/* Native publication of relocated spans (game_table_derived_native.c). Table
 * boot, before raw exposure: register the `table-derived` completion
 * participant when a span is relocated (nothing otherwise). Its preparation
 * checks the loaded original and copies the list, plus a zero guard, through
 * allocator; its commit moves the slot with the new extent. */
int p4_game_tables_derived_boot_stage(const P4GameTableAllocator *allocator);
/* Exact EARLY rollback only; a prepared copy locks the stage. */
int p4_game_tables_derived_boot_unstage(void);
/* Session end once tasks drain: a live image gets each loaded original block
 * back in its slot; the copies are released. */
int p4_game_tables_derived_session_end(int image_live);
#endif

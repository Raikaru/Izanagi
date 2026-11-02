#ifndef P4PC_GAME_TABLE_BOOT_H
#define P4PC_GAME_TABLE_BOOT_H
#include "p4pc/game_table_builder.h"
#define P4_GAME_TABLES_BOOT_PROVIDER "game-tables.sources"
struct P4GameTableNativeUses;
/* Host-only typed product of the sole package handler above, never an active
 * global fallback. Each pass explicitly provides its own borrowed product;
 * NULL means no authored table cohort. The owner retains the exact products.
 *
 * tables is the same immutable semantic registry in both passes. Its image
 * references remain stable(owner,key,revision), never process resource tokens.
 * sources is the same sealed immutable entry/origin ledger in both passes.
 * Every semantic view seals before exposure; actual native-view stamps and
 * require_owner attestation complete in the shared native finalizer. The uses
 * lease is the owner's actual optional provenance context, not readiness.
 *
 * Consumers check revision(tables), is_sealed(sources), and
 * sources_validate_view(sources,tables); a different registry is an error.
 * Resources200 and UI300 own their distinct CPU and real-runtime products.
 * Tables100 neither imports Resources nor changes frozen descriptors later. */
typedef struct P4GameTablesBootProduct {
    const P4GameTables *tables;
    const P4GameTableSources *sources;
    struct P4GameTableNativeUses *native_uses; /* Opaque, never Lua-writable. */
    const P4GameSkills *skills;
    const P4GameItems *items;
    const P4GameEnemies *enemies;
    const P4GamePersonas *personas;
    const P4GameText *names, *descriptions;
    P4GameTableInput *const *inputs; /* Stable owner array, borrowed, never destroy. */
    size_t input_count;
} P4GameTablesBootProduct;
/* Trusted compiled native declarations only, before the CPU seal. The sole
 * source handler owns these objects; caller registers rows AND explicit origin
 * bindings through the existing C APIs. Request only for real contributions:
 * an inactive provider must not allocate a fabricated default cohort.
 * NULL after seal/exposure or on allocation failure; never a Lua runtime API. */
P4GameTablesDefinitionBuilder *p4_game_tables_boot_builder(void);
/* Sole actual Core action bridge, registered before source preparation. Does
 * not allocate a builder for inactive SDKs. Context outlives early retries and
 * all owning views; registration survives candidate rollback, not session_end. */
int p4_game_tables_boot_action_preparer_register(P4GameItemActionPrepareFn prepare, void *context);
/* Core-only after all dependent native/UI/save users quiesce and detach.
 * Does not clear the shared native completion coordinator. */
int p4_game_tables_boot_session_end(void);
#endif

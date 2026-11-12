#ifndef P4PC_GAME_TABLE_BUILDER_H
#define P4PC_GAME_TABLE_BUILDER_H
#include "p4pc/game_table_inputs_use.h"
#include "p4pc/game_table_sources.h"
#include "p4pc/game_skills.h"
#include "p4pc/game_items.h"
#include "p4pc/game_enemies.h"
#include "p4pc/game_personas.h"

/* Trusted C-only preseal declaration owner. No late Luau registration. Fields
 * borrow this stable owner through native/save/UI quiescence; do not copy it.
 * Families and original inputs are allocated only when actually requested. */
typedef struct P4GameTablesDefinitionBuilder {
    P4GameTables *tables;
    P4GameTableSources *sources;
    uint32_t registry_revision;
    P4GameSkills *skills;
    P4GameItems *items;
    P4GameEnemies *enemies;
    P4GamePersonas *personas;
    P4GameText *names, *descriptions;
    P4GameTableAllocator allocator;
    P4GameTableInput *inputs[P4_TABLE_INPUT_COUNT];
    size_t input_count;
    uint32_t input_mask;
    unsigned sealing, sealed;
    P4GameTableNativeUses *native_uses;
    P4GameItemActionPrepareFn action_prepare;
    void *action_context;
} P4GameTablesDefinitionBuilder;
/* Initialize an all-zero stable owner. Allocator is the real late guest heap;
 * CPU-only users may pass NULL, but that does not grant runtime readiness. */
int p4_game_tables_builder_init(P4GameTablesDefinitionBuilder *builder,
    const P4GameTableAllocator *allocator, uint32_t revision);
P4GameSkills *p4_game_tables_builder_skills(P4GameTablesDefinitionBuilder *builder);
P4GameItems *p4_game_tables_builder_items(P4GameTablesDefinitionBuilder *builder);
P4GameEnemies *p4_game_tables_builder_enemies(P4GameTablesDefinitionBuilder *builder);
P4GamePersonas *p4_game_tables_builder_personas(P4GameTablesDefinitionBuilder *builder);
P4GameText *p4_game_tables_builder_text(P4GameTablesDefinitionBuilder *builder, uint32_t role);
/* Request actual producer-owned packed inputs before the seal. Capturing them
 * never constitutes native loaded-range or gameplay admission. */
int p4_game_tables_builder_request_input(P4GameTablesDefinitionBuilder *builder,
    P4GameTableInputKind kind);
const P4GameTableInput *p4_game_tables_builder_input(
    const P4GameTablesDefinitionBuilder *builder, P4GameTableInputKind kind);
/* Capture requested sources, seal all actual views, then create exactly one
 * provenance context over the stable dense input array and complete ledger.
 * Failure consumes this attempt: discard the ENTIRE unpublished candidate.
 * failed_provider identifies only an actual failing package input. */
int p4_game_tables_builder_seal(P4GameTablesDefinitionBuilder *builder,
    const struct P4PackageProvider **failed_provider);
/* Only after all actual native/UI/save users detached and drained. No inferred
 * image lifetime, atexit, rollback-after-exposure or implicit session teardown. */
int p4_game_tables_builder_destroy(P4GameTablesDefinitionBuilder *builder);
#endif

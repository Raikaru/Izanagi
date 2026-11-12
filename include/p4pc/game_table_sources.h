#ifndef P4PC_GAME_TABLE_SOURCES_H
#define P4PC_GAME_TABLE_SOURCES_H
#include <stddef.h>
#include <stdint.h>
#include "p4pc/game_tables.h"
#include "p4pc/mod_save.h"
#include "p4pc/game_table_use.h"
struct P4PackageProvider;

typedef struct P4GameTableSources P4GameTableSources;
typedef enum P4TableSourceViewKind {
    P4_TABLE_SOURCE_REGISTRY = 0, P4_TABLE_SOURCE_SKILLS = 1,
    P4_TABLE_SOURCE_ITEMS = 2, P4_TABLE_SOURCE_TEXT = 3, P4_TABLE_SOURCE_ENEMIES = 4,
    P4_TABLE_SOURCE_PERSONAS = 5
} P4TableSourceViewKind;
/* Stable identity, not an address/ordinal/generation. ids are domain-specific:
 * element/channel/skill/item/category/enemy/persona: id; affinity: kind,entity,profile,leaf,ch;
 * action: source-kind,id,component; alias: kind,entity,native-profile;
 * effect: source-kind,id,leaf,ch; string: table,id,role. Unused ids are zero.
 * Affinity/string have no separate key. Their canonical numeric key is exact.
 * Affinity revision is its frozen semantic-registry revision; other revisions
 * and schemas are the actual owned descriptor's, or schema1 where fixed. */
typedef struct P4TableSourceEntry {
    uint32_t domain, ids[5], schema, revision;
    char owner[64], key[P4_TABLE_KEY_SIZE];
} P4TableSourceEntry;
typedef struct P4TableSourceCursor { uint32_t domain; size_t index; } P4TableSourceCursor;
/* Start cursor at{0}; returns OK/NOT_FOUND(end)/negative failure. Enumerates
 * owned entries of the actual CPU-sealed semantic view. Built-in unowned retail
 * descriptors are known native, not missing-owner inference for authored rows. */
int p4_game_table_sources_next(P4TableSourceViewKind kind, const void *frozen_view,
    P4TableSourceCursor *cursor, P4TableSourceEntry *entry);

typedef enum P4TableSourceOriginKind {
    P4_TABLE_SOURCE_NATIVE = 1, P4_TABLE_SOURCE_PACKAGE = 2
} P4TableSourceOriginKind;
typedef struct P4TableSourceOrigin {
    uint32_t kind, schema, revision, package_kind;
    char source_key[P4_TABLE_KEY_SIZE], package_id[64], package_version[64];
} P4TableSourceOrigin;
/* The core owns the complete view tuple through destroy, after save-provider
 * unregister and actual native task/queue quiescence. This context pins real
 * package providers and copies source identities; it never owns/frees views. */
P4GameTableSources *p4_game_table_sources_create(void);
void p4_game_table_sources_destroy(P4GameTableSources *sources);
/* Registration may precede semantic seal, but every exact view must be CPU
 * sealed before sources_seal. No active()/pending-object substitution. */
int p4_game_table_sources_view(P4GameTableSources *sources,
    P4TableSourceViewKind kind, const void *owned_view);
/* Source handles are local builder references, never persisted IDs. Native
 * sources require explicit selected mode/schema/revision, never catalog miss.
 * Package sources require the ACTUAL live immutable provider; retain may fail. */
int p4_game_table_sources_native(P4GameTableSources *sources,
    const char *mode, uint32_t schema, uint32_t revision, uint32_t *source);
/* Authored declaration identity/revision is distinct from package identity.
 * source_key names the actual decoded table source; native origins use their
 * explicit mode key in the same field. No filename/timestamp inference. */
int p4_game_table_sources_package(P4GameTableSources *sources,
    const struct P4PackageProvider *provider, const char *source_key,
    uint32_t schema, uint32_t revision, uint32_t *source);
/* Called by the real registration/source-preparation path for each entry it
 * authored, with its captured source handle. Does not infer origin from owner.
 * Freeze rejects duplicate, missing, extra or identity-mismatched bindings. */
int p4_game_table_sources_bind(P4GameTableSources *sources, const void *owned_view,
    const P4TableSourceEntry *entry, uint32_t source);
/* TABLES100 seals source identities/bindings immutably and validates COMPLETE
 * owned-entry coverage of every CPU-sealed semantic view before exposure.
 * Missing/unsealed views refuse. This is not guest/resource/gameplay readiness. */
int p4_game_table_sources_seal(P4GameTableSources *sources);
int p4_game_table_sources_is_sealed(const P4GameTableSources *sources);
int p4_game_table_sources_is_frozen(const P4GameTableSources *sources);
int p4_game_table_sources_validate_view(const P4GameTableSources *sources, const void *frozen_view);
/* Requires every actual native view revision to match its earlier semantic
 * stamp, without late reparsing/enumeration. Only then may require_owner run. */
int p4_game_table_sources_freeze(P4GameTableSources *sources);
/* The table-source participant prepares after item/skill/persona/enemy and
 * before element consumers. It checks actual materialized view stamps, then
 * publishes the prepared context only after all cohort prepares succeed.
 * Reloads check exact revisions/provider lineage without rebuilding metadata.
 * session_end detaches only; core quiesces and clears the shared coordinator. */
int p4_game_table_sources_boot_stage(P4GameTableSources *sources);
const P4GameTableSources *p4_game_table_sources_active(void);
int p4_game_table_sources_session_end(void);
/* Exact shared P4ModSaveRequireOwnerFn signature:0 authorized, negative failure.
 * A non-NULL writer records actual provider(kind/id/version) requirements.
 * NULL writer validates current ownership only. Native-mode compatibility must
 * also be represented/validated by the component's versioned saved identities;
 * this callback never appends unannounced payload bytes or proves gameplay. */
int p4_game_table_sources_require_owner(const void *frozen_view,
    P4ModSaveWriter *nullable_writer, const char *owner, void *immutable_sources);
/* Unique explicit origins for an exact CPU-sealed semantic view/owner, for
 * native-mode compatibility metadata. Available before guest materialization;
 * metadata is NOT require_owner authority. NULL is absent/invalid, never an
 * attestation that an owner is native. */
const P4TableSourceOrigin *p4_game_table_sources_origin_at(const P4GameTableSources *sources,
    const void *frozen_view, const char *owner, size_t index);
/* Pure metadata enumeration of issued native-mode origins after CPU seal.
 * Cursor starts0, advances once over origins; no process use/native admission. */
size_t p4_game_table_sources_native_mode_count(const P4GameTableSources *sources);
const P4TableSourceOrigin *p4_game_table_sources_native_mode_next(
    const P4GameTableSources *sources, size_t *cursor);
typedef struct P4TableSourceResolved {
    const P4TableSourceOrigin *origin;
    const struct P4PackageProvider *provider; /* NULL only for explicit NATIVE. */
    const P4TableSourceEntry *entry;
    uint32_t origin_index; /* Context-local builder handle, NEVER a saved ID. */
} P4TableSourceResolved;
/* Exact runtime entry-origin lookup after actual native-view validation. Pure:
 * no touch, string-to-provider lookup or native-mode inference. The caller's
 * runtime use service checks actual selected-provider authority/records use.
 * Borrowed results outlive only this context; output unchanged on failure. */
int p4_game_table_sources_resolve_entry(const P4GameTableSources *sources,
    const void *exact_view, uint32_t domain, const uint32_t ids[5],
    P4TableSourceResolved *out);
size_t p4_game_table_sources_origin_count(const P4GameTableSources *sources);
#endif

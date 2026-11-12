/* Owned game-encoded names/descriptions for expanded table IDs. */
#ifndef P4PC_GAME_TEXT_H
#define P4PC_GAME_TEXT_H
#include <stddef.h>
#include <stdint.h>
#include "p4pc/game_tables.h"
#include "p4pc/native_glyphs.h"
#include "p4pc/game_text_native.h"

typedef enum P4TextTable {
    P4_TEXT_ITEM = 0, P4_TEXT_SKILL = 1, P4_TEXT_PERSONA = 2,
    P4_TEXT_ENEMY = 3, P4_TEXT_ITEM_CATEGORY = 4
} P4TextTable;
typedef enum P4TextRole { P4_TEXT_NAME = 0, P4_TEXT_DESCRIPTION = 1 } P4TextRole;
#define P4_TEXT_ENCODING_US_NATIVE 1u
#define P4_TEXT_ENCODING_US_RAW_NATIVE 2u
#define P4_TEXT_FONT_US_NATIVE 1u
typedef struct P4TextProfile { uint32_t encoding, font; } P4TextProfile;
typedef struct P4GameTextRow {
    uint32_t table, id, role, revision, size, stream;
} P4GameTextRow;
/* CPU semantic definition, deliberately without a fabricated guest stream.
 * owner/bytes borrow this snapshot; bytes must be requeried after materialize. */
typedef struct P4GameTextDefinition {
    uint32_t table, id, role, revision, size;
    const char *owner;
    const uint8_t *bytes;
} P4GameTextDefinition;
typedef struct P4GameText P4GameText;
typedef int (*P4GameTextUseFn)(const P4GameText *exact_view, uint32_t table,
    uint32_t id, uint32_t role, void *immutable_source_context);
P4GameText *p4_game_tables_text_create(const P4GameTableAllocator *allocator, const P4TextProfile *profile);
void p4_game_tables_text_destroy(P4GameText *text);
/* Copies bytes including the final NUL. Encoding is P4 ASCII+two-byte glyphs,
 * not UTF-8 or Shift-JIS. Commands/substitutions are refused. Names cannot
 * contain newlines. Native adapters consult this only for IDs beyond their
 * actual retail domain; retail name/description banks remain untouched. */
/* Registry IDs are full-width u32 keys; native table consumers separately
 * enforce their qualified physical/semantic domains without truncating them. */
int p4_game_tables_text_validate(const P4TextProfile *profile, uint32_t role,
    const uint8_t *bytes, size_t size);
int p4_game_tables_text_register(P4GameText *text, const char *owner, uint32_t table,
    uint32_t id, uint32_t role, uint32_t revision, const uint8_t *bytes, size_t size);
/* Preseal immutable runtime-origin transport, not an origin/readiness proof.
 * No pure lookup, seal, materializer or stream binding invokes the callback. */
int p4_game_tables_text_bind_runtime_use(P4GameText *text, P4GameTextUseFn use,
    void *owning_source_context);
int p4_game_tables_text_validate_runtime_sources(const P4GameText *text);
/* Explicit actual native consumption only, after guest materialization.
 * Exact row/object/role dispatch; only callback0 succeeds, never owner lookup.
 * The native caller routes missing/invalid binding to core controlled recovery. */
int p4_game_tables_text_runtime_use(const P4GameText *text, uint32_t table,
    uint32_t id, uint32_t role);
/* CPU-only immutable definition seal; no guest allocation/copy or publication.
 * Same revision is idempotent; a different revision conflicts. */
int p4_game_tables_text_seal(P4GameText *text, uint32_t revision);
uint32_t p4_game_tables_text_semantic_revision(const P4GameText *text);
int p4_game_tables_text_definition_find(const P4GameText *text, uint32_t table,
    uint32_t id, uint32_t role, P4GameTextDefinition *definition);
int p4_game_tables_text_definition_at(const P4GameText *text, size_t index,
    P4GameTextDefinition *definition);
/* Late actual guest-heap phase. Failed materialization leaves sealed metadata,
 * revision()==0 and no streams; registration never reopens after the seal. */
int p4_game_tables_text_materialize(P4GameText *text);
/* Convenience seal+materialize for existing actual-runtime native clients. */
int p4_game_tables_text_freeze(P4GameText *text, uint32_t revision);
uint32_t p4_game_tables_text_revision(const P4GameText *text);
const P4TextProfile *p4_game_tables_text_profile(const P4GameText *text);
size_t p4_game_tables_text_count(const P4GameText *text);
const P4GameTextRow *p4_game_tables_text_find(const P4GameText *text, uint32_t table, uint32_t id, uint32_t role);
const P4GameTextRow *p4_game_tables_text_at(const P4GameText *text, size_t index);
const uint8_t *p4_game_tables_text_bytes(const P4GameText *text, size_t index);
const char *p4_game_tables_text_owner(const P4GameText *text, size_t index);
#endif

#ifndef P4PC_GAME_ELEMENTS_AI_H
#define P4PC_GAME_ELEMENTS_AI_H
#include "p4pc/game_tables.h"
#include "p4pc/game_elements_ai_native.h"
#include "p4pc/game_table_use.h"
#include <stddef.h>
#include <stdint.h>
/* An authored, source-qualified role at an actual native-command instruction.
 * In particular a proven basic-attack guard uses BASIC, not permission borrowed
 * from another equipped physical skill. A tagged exact argument remains exact. */
typedef struct P4GameAiScriptQueryScope {
    uint32_t instruction_pc, command_id, role;
} P4GameAiScriptQueryScope;

/* Supplied only by the authoritative source producer. source_identity is Core's
 * sealed immutable identity for THIS byte span (not a path, owner spelling,
 * physical-disc offset or a guess about the current GP value). Core retains the
 * actual winning bytes, length and identity until the prepared product dies.
 * Distinct spans in one resource have distinct sealed span identities. */
typedef struct P4GameAiScriptSource {
    const uint8_t *bytes;
    size_t size;
    const void *source_identity;
    /* Additional source-qualified per-instruction roles. There is deliberately
     * no blanket AVAILABLE default. The BF compiler proves basic guards from
     * actual native bindings/control flow; exact tagged arguments need no
     * legacy role. Any other untagged Phys0 needs an explicit proven site. */
    const P4GameAiScriptQueryScope *query_scopes;
    size_t query_scope_count;
} P4GameAiScriptSource;
typedef struct P4GameAiPrepared P4GameAiPrepared;
struct P4GameSkills;
typedef struct P4GameAiBankSource {
    const uint8_t *rows;
    size_t rows_size;
    const void *rows_identity, *player_script_identity, *enemy_script_identity;
    const struct P4GameSkills *skills;
    P4GameTableSourceUseFn use;
    void *use_context;
} P4GameAiBankSource;

/* Early CPU-only phase, before native raw exposure: validate complete BF byte
 * spans, instruction/label/procedure/local bounds and scoped CFG products.
 * request==0 returns NULL without parsing, copying or allocating source data. */
int p4_game_elements_ai_sources_prepare(const P4GameTables *tables, int request,
    const P4GameAiScriptSource *sources, size_t count, P4GameAiPrepared **prepared);
/* CPU-only: validate the actual164-byte AICALC rows and their real sealed skill
 * and enemy-BF procedure references. All spans/context/views remain borrowed.
 * A script-only product cannot register native AI completion. Never touch
 * process-use history during this operation or either late bind. */
int p4_game_elements_ai_bank_stage(P4GameAiPrepared *prepared,
    const P4GameAiBankSource *source);
int p4_game_elements_ai_bank_bind(P4GameAiPrepared *prepared,
    const void *rows_identity, const uint8_t *actual_loaded_rows, size_t actual_size,
    uint32_t actual_native_rows_guest);
/* Late actual native-loader phase: bind an already prepared span to the actual
 * copied native header. Verify identity, pointer, exact length and bytes; never
 * compile CFG, infer addresses, allocate or publish an active route here. */
int p4_game_elements_ai_source_bind(P4GameAiPrepared *prepared,
    const void *source_identity, const uint8_t *actual_loaded_bytes, size_t actual_size,
    uint32_t actual_native_header_guest);
/* Register the real element-ai prepare/commit participant over this exact view.
 * request==0 registers nothing; a mistakenly expanded element cohort therefore
 * still refuses its missing required AI participant before ANY prepare. */
int p4_game_elements_ai_boot_stage(const P4GameTables *tables, int request,
    P4GameAiPrepared *prepared);
/* Core calls session_end only after AI/script/action consumers quiesce, then
 * destroys the product before releasing its borrowed table/source identities.
 * Destroy refuses a product still held by staged or active native consumers. */
int p4_game_elements_ai_session_end(void);
int p4_game_elements_ai_sources_destroy(P4GameAiPrepared *prepared);
/* Owning Core consumer teardown: quiesce AI/script/action/sidecar users first;
 * detach this product before the shared completion graph and Root's borrowed
 * input/ledger products are released. Never a render-device-loss callback. */
int p4_game_elements_ai_boot_session_end(void);
#endif

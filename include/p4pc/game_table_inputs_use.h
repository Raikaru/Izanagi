#ifndef P4PC_GAME_TABLE_INPUTS_USE_H
#define P4PC_GAME_TABLE_INPUTS_USE_H
#include "p4pc/game_table_inputs.h"
struct P4GameTableSources;
typedef struct P4GameTableNativeUses P4GameTableNativeUses;
/* One actual prepared input/source context's optional native provenance block.
 * Register before Load; inputs, stable borrowed array and sealed source ledger
 * outlive it. CPU preparation/unused DEV files/native modes emit nothing.
 * Historical source warnings never authorize required expanded references.
 * Destroy only after native consumers quiesce; success unregisters first. */
int p4_game_table_native_uses_prepare(P4GameTableInput *const *inputs, size_t count,
    const struct P4GameTableSources *sealed_sources, P4GameTableNativeUses **out);
int p4_game_table_native_uses_destroy(P4GameTableNativeUses *);
/* Runtime authored-row use: exact view/domain/five-ID tuple resolves only from
 * the same immutable source ledger owned by this native-use context. Package
 * use and issued NATIVE_MODE use share the actual provider/optional history
 * service; no owner-string lookup or default-native fallback. */
int p4_game_table_sources_semantic_use(const void *exact_view, uint32_t domain,
    const uint32_t ids[5], void *owning_native_use_context);
/* Actual native semantic consumption callback. Context is the exact owning
 * immutable input; accepts only its issued sealed spans. Called only AFTER
 * native binding proved actual loaded identity/bytes, never during prepare or
 * bind. Real package use is process-sticky; actual DEV consumption updates only
 * its owning input's native history (never a fabricated package or witness).
 * Origin/use failures enter core controlled recovery. DISC/default is untagged. */
int p4_game_table_input_semantic_use(const void *sealed_span_identity, void *immutable_input_context);
#endif

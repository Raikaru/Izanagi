#ifndef P4PC_GAME_TABLE_INPUTS_INTERNAL_H
#define P4PC_GAME_TABLE_INPUTS_INTERNAL_H
#include "p4pc/game_table_inputs.h"
#include "p4pc/content_digest.h"
struct P4GameTableNativeUses;
struct P4GameTableInputSpan { const P4GameTableInput *input; size_t offset, size; };
struct P4GameTableInput {
    P4GameTableInputKind kind;
    const P4PackageProvider *provider;
    P4PackageFilePlan *package;
    P4VfsSourcePlan *native;
    size_t size;
    /* Exact corresponding disc file and aligned payload footprint; unavailable
     * is distinct from zero and cannot authorize a native packed copy. */
    uint64_t retail_size, retail_footprint;
    uint32_t retail_bounds_valid, retail_is_selected;
    uint32_t sealed;
    P4ContentFingerprint fingerprint;
    uint8_t *bytes;
    /* Only actual native semantic use mutates this owner's use ledger. */
    struct P4GameTableNativeUses *native_uses;
    uint32_t native_used, digest_valid;
    uint8_t digest[32];
    P4GameTableInputSpan spans[];
};
#endif

#ifndef P4PC_MOD_SAVE_INTERNAL_H
#define P4PC_MOD_SAVE_INTERNAL_H
#include "p4pc/mod_save.h"
#include "mod_save_format.h"

typedef struct P4ModSaveCaptureStage P4ModSaveCaptureStage;
typedef struct P4ModSaveCapture {
    uint64_t retail_size;
    int witness;
    unsigned char retail_hash[32], sidecar_hash[32];
    unsigned char *sidecar;
    P4ModSaveCaptureStage *stages;
    size_t stage_count;
    size_t sidecar_size;
} P4ModSaveCapture;
/* Internal VM participant: on success transfers even a NULL staged value to
 * the capture; provider commit/discard consume it after publication/abort.
 * On failure ownership remains with the caller and the encode is poisoned. */
int p4_mod_save_capture_stage(P4ModSaveWriter *, void *staged);
/* Install one post-native-commit notification on a live VM registration. */
int p4_mod_save_set_notify(P4ModSaveRegistration *, void (*notify)(void *));
/* Install one durable-publication notification. It is queued only when a
 * committed capture carried this registration's emitted block. */
int p4_mod_save_set_published(P4ModSaveRegistration *, void (*published)(void *));
/* Game-thread safe point only (never inside the native memcard write): runs the
 * queued publication notifications; a busy codec leaves them queued. */
int p4_mod_save_published_pending(void);
void p4_mod_save_published_notify(void);
/* Pure codec/ownership boundary. A successful NULL result is wholly retail.
 * Image memory is borrowed only for this call, never retained by providers. */
int p4_mod_save_capture_image(const P4ModSaveImage *, P4ModSaveCapture **out,
                              char *error, size_t capacity);
/* Called only after the complete native slot has been published. Consumes
 * the capture; stage callbacks do native slot swaps only. */
void p4_mod_save_capture_commit(P4ModSaveCapture *);
void p4_mod_save_capture_free(P4ModSaveCapture *);
/* Consumes the sidecar allocation on every path, including failure. */
int p4_mod_save_stage_image(const P4ModSaveImage *, unsigned char *sidecar,
    size_t sidecar_size, P4ModSaveLoad **out, char *error, size_t capacity);
int p4_mod_save_load_witness(const P4ModSaveLoad *);
const char *p4_mod_save_load_warning(const P4ModSaveLoad *);
void p4_mod_save_load_commit(P4ModSaveLoad *);
/* Invoke ordinary notifications only after all native commits have finished. */
void p4_mod_save_load_notify(void);
void p4_mod_save_load_abort(P4ModSaveLoad *);
int p4_mod_save_load_matches(const P4ModSaveLoad *, int32_t version,
                            const void *payload, size_t payload_size);
int p4_mod_save_reset_session(char *error, size_t capacity);
int p4_mod_save_has_providers(void);
/* Consult registered decoders/defaults/encoders or retained use/custody state.
 * This is NOT a "modded" bit: registered providers may all omit their data. */
int p4_mod_save_needs_codec(void);
/* Sticky process custody after consuming a non-reference save or marker.
 * New Game resets provider state, not the provenance of this native run. */
void p4_mod_save_witness_source(void);
#endif

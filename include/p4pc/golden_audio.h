/* Original bounded Golden audio readers; docs/golden_mode/audio.md. */
#ifndef P4PC_GOLDEN_AUDIO_H
#define P4PC_GOLDEN_AUDIO_H
#include "p4pc/golden_source.h"

/* Every returned span/string borrows the original mutable packet. */
typedef struct P4GoldenWave {
    uint32_t id;
    P4GoldenSpan payload;
} P4GoldenWave;
typedef struct P4GoldenAwb {
    P4GoldenWave *waves; /* owned index, sorted by id */
    uint32_t count;
    uint16_t subkey;
} P4GoldenAwb;
int p4_golden_awb_open(P4GoldenAwb *awb, uint8_t *bytes, size_t size);
void p4_golden_awb_close(P4GoldenAwb *awb);
const P4GoldenWave *p4_golden_awb_find(const P4GoldenAwb *awb, uint32_t id);

enum {
    P4_ACB_CUE, P4_ACB_NAME, P4_ACB_WAVE, P4_ACB_SYNTH, P4_ACB_SEQUENCE,
    P4_ACB_TRACK, P4_ACB_EVENT, P4_ACB_COMMAND, P4_ACB_EXTENSION,
    P4_ACB_SEQ_COMMAND, P4_ACB_TRACK_COMMAND, P4_ACB_SYNTH_COMMAND,
    P4_ACB_STRING, P4_ACB_TABLE_COUNT
};
/* Inclusive integer [0,upper] supplied by the player, never an invented Atom RNG.
 * Calls to selection/planning on one sheet must be producer-thread serialized. */
typedef int (*P4GoldenCueRandom)(void *context, uint32_t upper, uint32_t *value);
typedef struct P4GoldenCueSelectors P4GoldenCueSelectors;
/* Hash-qualified snapshot of the two measured ACF configurations, not a
 * general ACF/DSP implementation. No source bytes or allocations are retained. */
typedef struct P4GoldenAcf { unsigned profile; } P4GoldenAcf;
int p4_golden_acf_qualify(P4GoldenAcf *acf, const uint8_t *bytes, size_t size);
typedef struct P4GoldenAcb {
    P4GoldenUtf header, tables[P4_ACB_TABLE_COUNT];
    P4GoldenAwb memory;
    P4GoldenCueSelectors *selectors; /* optional owned selection state, not authored bytes */
    unsigned dry_acf_profile; /* implementation state; attach through the API */
} P4GoldenAcb;
typedef struct P4GoldenCue {
    uint32_t id, index, length_ms;
    const char *name;
} P4GoldenCue;
typedef struct P4GoldenCueWave {
    uint32_t index, id, rate, channels, samples, codec, port;
    uint32_t loop_start, loop_end; /* optional ACB override, sample interval [start,end) */
    int streaming, loop;
} P4GoldenCueWave;
#define P4_GOLDEN_CUE_MAX_WAVES 64u
/* Open parses framing, all present nested tables and the embedded bank. */
int p4_golden_acb_open(P4GoldenAcb *acb, uint8_t *bytes, size_t size);
void p4_golden_acb_close(P4GoldenAcb *acb);
/* 0 found, 1 absent, -1 malformed. Lookup by stable CueId, not row number. */
int p4_golden_acb_cue(const P4GoldenAcb *acb, uint32_t id, P4GoldenCue *cue);
/* Resolve the cue graph. Returns waveform count, -1 malformed/unsupported.
 * Duplicate graph edges to a wave are retained (layers are not deduplicated). */
int p4_golden_acb_resolve(const P4GoldenAcb *acb, const P4GoldenCue *cue,
                         P4GoldenCueWave *waves, size_t capacity);

typedef struct P4GoldenCuePlan {
    P4GoldenCueWave waves[P4_GOLDEN_CUE_MAX_WAVES];
    uint32_t starts[P4_GOLDEN_CUE_MAX_WAVES]; /* 48kHz output frames, per occurrence */
    float gains[P4_GOLDEN_CUE_MAX_WAVES]; /* authored/category product, once per note */
    uint32_t count;
    uint32_t category_mask; /* approved category IDs, one membership per cue */
    uint64_t frames; /* finite max-end bound, not CueTable.Length */
} P4GoldenCuePlan;
/* Explicit port selection policy. Setting/replacing the callback resets the
 * sheet-local previous choices; that reset is not a claimed retail lifecycle. */
int p4_golden_acb_set_random(P4GoldenAcb *acb, P4GoldenCueRandom random, void *context);
/* Attach only in a clean dry session with known unity CRI player/category
 * controls and no old wet tails; host mixer controls still apply once.
 * NULL clears qualification. Stop all cues before changing registered ACF.
 * The snapshot is copied; the ACF bytes/object need not outlive this call. */
int p4_golden_acb_set_dry_acf(P4GoldenAcb *acb, const P4GoldenAcf *acf);
/* Bounded fixed-rate plan; named OPEN refusals or pending:M8d3-dsp obligations.
 * Planning consumes injected selections, retaining Type4 state per sheet/row.
 * Missing Type/command columns use the documented port fixture profile only. */
int p4_golden_acb_plan(const P4GoldenAcb *acb, const P4GoldenCue *cue,
                        P4GoldenCuePlan *plan, char *reason, size_t capacity);

typedef struct P4GoldenHcaInfo {
    uint32_t rate, channels, blocks, frames, loop_start, loop_end;
    uint16_t version, header_size, block_size, delay, padding, cipher;
    int has_loop;
    float volume;
} P4GoldenHcaInfo;
int p4_golden_hca_info(const uint8_t *bytes, size_t size, P4GoldenHcaInfo *info);

typedef struct P4GoldenHca P4GoldenHca;
/* Decoder has no key discovery. Caller supplies only the user's setting.
 * Header plus BE64 key + BE16 AWB subkey is FFmpeg's extradata protocol.
 * 0 success, -2 keyed input without a key, -1 invalid/decoder unavailable. */
int p4_golden_hca_open(P4GoldenHca **decoder, const uint8_t *bytes, size_t size,
                       uint64_t user_key, uint16_t subkey, char *reason, size_t capacity);
const P4GoldenHcaInfo *p4_golden_hca_format(const P4GoldenHca *decoder);
/* Interleaved s16, trimmed for encoder delay/padding, volume applied once.
 * 0 EOF, positive frames, -1 corrupt input. No partial success after failure. */
int p4_golden_hca_pull(P4GoldenHca *decoder, int16_t *pcm, uint32_t capacity);
/* Absolute audible sample position. Reconstructs overlap before the target. */
int p4_golden_hca_seek(P4GoldenHca *decoder, uint32_t sample);
void p4_golden_hca_close(P4GoldenHca *decoder);

/* Golden voice AWBs contain ordinary ADX, not HCA. This bounded adapter feeds
 * the existing, unchanged movie ADX decoder incrementally and trims its last
 * 32-sample packet to the ADX header's original frame count. */
typedef struct P4GoldenAdx P4GoldenAdx;
typedef struct P4GoldenAdxInfo { uint32_t rate, channels, frames; } P4GoldenAdxInfo;
int p4_golden_adx_open(P4GoldenAdx **decoder, const uint8_t *bytes, size_t size);
const P4GoldenAdxInfo *p4_golden_adx_format(const P4GoldenAdx *decoder);
int p4_golden_adx_pull(P4GoldenAdx *decoder, int16_t *pcm, uint32_t capacity);
void p4_golden_adx_close(P4GoldenAdx *decoder);
#endif

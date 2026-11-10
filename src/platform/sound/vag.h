/* Sony SPU ADPCM ("VAG") decoder.
 *
 * Data is a run of 16-byte blocks, each 28 samples: byte 0 holds the shift (low nibble) and the
 * filter index 0..4 (high nibble), byte 1 the flags, bytes 2..15 the 4-bit samples, low nibble
 * first. Flags: bit 0 end of the sound, bit 1 repeat (jump to the loop start at the end block),
 * bit 2 loop start. The IOP sound driver stores sample banks (.bd) as consecutive VAGs, each
 * ending on a block with the end bit. */
#ifndef P4PC_VAG_H
#define P4PC_VAG_H

#include <stddef.h>
#include <stdint.h>

#define P4VAG_BLOCK_BYTES 16
#define P4VAG_BLOCK_SAMPLES 28

#define P4VAG_FLAG_END 0x01
#define P4VAG_FLAG_REPEAT 0x02
#define P4VAG_FLAG_LOOP_START 0x04

typedef struct P4VagInfo {
    uint32_t frames;     /* decoded samples */
    uint32_t loop_start; /* first sample of the loop, valid when has_loop */
    int has_loop;        /* the end block repeats */
    uint32_t bytes;      /* bytes consumed, including the end block */
} P4VagInfo;

/* Decodes blocks from src until an end block or max_bytes. dst must hold at least
 * (max_bytes / 16) * 28 samples; with dst NULL only the info is computed. The decoder state
 * (the two previous samples) starts at zero, as on the SPU at the start of a voice. Returns
 * the sample count. */
uint32_t p4vag_decode(const uint8_t *src, size_t max_bytes, int16_t *dst, P4VagInfo *info);

/* Decodes one block with the given history; returns the 28 samples in out and updates h1, h2
 * (the previous and the one before). */
void p4vag_block(const uint8_t block[16], int32_t *h1, int32_t *h2, int16_t out[28]);

#endif

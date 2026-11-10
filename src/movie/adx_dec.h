/* adx_dec: the ADX audio track of an SFD (program stream 0xC0) decoded to PCM with libavcodec.
 *
 * The stream is the bytes of an ADX file: a header (`80 00`, big-endian offset to the copyright string,
 * encoding 3, block size 18, bits 4, channels, sample rate, total samples; the header is offset+4 bytes)
 * followed by frames of 18 bytes per channel, 32 samples each. Bytes arrive in PES-sized pieces. */
#ifndef P4PC_MOVIE_ADX_DEC_H
#define P4PC_MOVIE_ADX_DEC_H

#include <stdint.h>

typedef struct P4Adx P4Adx;

P4Adx *p4adx_open(void);
void p4adx_close(P4Adx *a);
/* Bounds the queue: past twice `frames` the oldest are dropped down to `frames`. */
void p4adx_set_limit(P4Adx *a, int frames);
/* Appends stream bytes and decodes what completes. Returns 0, or -1 on a malformed header or a decoder error. */
int p4adx_feed(P4Adx *a, const uint8_t *data, int size);
/* Format from the header, 0 before it has been read. */
int p4adx_sample_rate(const P4Adx *a);
int p4adx_channels(const P4Adx *a);
/* Decoded sample frames waiting. */
int p4adx_available(P4Adx *a);
/* Sample frames decoded since the start (pulled or not). */
int64_t p4adx_total(P4Adx *a);
/* Copies up to max_frames interleaved 16-bit sample frames out and removes them. Returns the count. */
int p4adx_pull(P4Adx *a, int16_t *dst, int max_frames);

#endif

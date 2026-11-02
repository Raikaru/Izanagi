/* mwply: the port's movie player (CRI Sofdec's MWPLY API). See mwply.c. */
#ifndef P4PC_MOVIE_MWPLY_H
#define P4PC_MOVIE_MWPLY_H

#include <stdint.h>

/* ---- Audio lane interface ----------------------------------------------------------------------
 * The ADX track of the playing movie is decoded to 16-bit interleaved PCM. The audio lane:
 *   1. finds the stream with p4pc_movie_audio_current(), which returns the handle of the movie that has
 *      audio and is started (NULL otherwise), and p4pc_movie_audio_format() (rate, channels; returns 0
 *      until the ADX header has been read);
 *   2. pulls PCM from its device callback with p4pc_movie_audio_pull() (thread-safe against the game
 *      thread; pads nothing: it returns fewer frames than asked, or 0, when the queue runs dry);
 *   3. reports progress with p4pc_movie_audio_played(h, total): the number of sample frames of this
 *      movie's audio that the device has actually played (not merely pulled) since its first sample,
 *      absolute. Once it has been called the movie follows it, as Sofdec does: video frame k is due when
 *      played/rate reaches k/fps, so the video waits for the audio and cannot run ahead. When the audio
 *      is exhausted (stream over and everything played) the clock continues from where it stopped.
 * Without a report the movie runs on the game's update count (tests), or the wall clock. */
int32_t p4pc_movie_audio_format(void *h, int32_t *rate, int32_t *channels);
int32_t p4pc_movie_audio_pull(void *h, int16_t *dst, int32_t max_frames);
void p4pc_movie_audio_played(void *h, uint64_t frames);
void *p4pc_movie_audio_current(void);

/* Fills the image buffer of the last delivered frame with the IPU's RGB32 macroblock columns (the player hands out stamped, pixel-free
 * buffers otherwise; see mwply.c). For tests and tools that read the buffer. Returns 0 or -1. */
int p4pc_mwply_fill_tiled(void *h);

#endif

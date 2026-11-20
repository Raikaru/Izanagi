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

/* Lockstep-only online replay of retail's movie (src/platform/lockstep_disc.c, NG-7j). Retail's preparation time and
 * frame schedule depend on its own frame history (docs/movie_player.md), so before the initial anchor a
 * state-aligned run answers each mwPlyGetCurFrm poll and status read of a movie with the live retail run's answer
 * to the same poll. NULL in every other run: the player paces itself.
 * start returns the movie's replay id (non-zero) when it is replayed; poll returns retail's GetCurFrm answer for
 * the movie (its frame number, or -1: no frame); status its status read. observe_* get the port's own result
 * (the replay layer fails the run when a delivered frame is not retail's) and the status read. */
typedef struct P4MovieReplay {
    int (*start)(const char *path);
    int32_t (*poll)(int id);
    int32_t (*status)(int id);
    void (*observe_poll)(uint32_t calls, int32_t stat, int32_t retail, int32_t delivered);
    void (*observe_status)(uint32_t calls, int32_t stat);
} P4MovieReplay;
extern const P4MovieReplay *p4pc_movie_replay;

/* 1 while a movie is being prepared or played. */
int p4pc_mwply_active(void);

#endif

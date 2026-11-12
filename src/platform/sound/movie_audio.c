/* Movie audio on the mixer.
 *
 * The movie player (src/movie/mwply.c) decodes the ADX track of the playing SFD to PCM and
 * exposes it through the audio lane interface of src/movie/mwply.h. This is the audio side of
 * that interface: a mixer tick hook (the audio thread, every 5 ms of output) that
 *   - finds the current movie (p4pc_movie_audio_current) and, once its format is known, a stream
 *     voice for it;
 *   - pulls PCM into the voice's ring (p4pc_movie_audio_pull is thread safe and returns less when
 *     its queue is dry; nothing is padded: the voice just has no data and plays silence);
 *   - reports the frames the mixer has played of that movie (p4pc_movie_audio_played), from the
 *     moment the movie appears, so the video follows the audio.
 * A movie that ends keeps its voice until the buffered tail has played. The hook registers
 * itself when the program starts (a constructor), so linking this file is all it takes; the
 * device is opened by sceSdInit (ps2_sndcom.c). */
#include <stddef.h>
#include <stdint.h>

#include "mwply.h"
#include "p4snd.h"

#define MOVIE_RING_FRAMES 0x8000
#define PULL_FRAMES 2048

static struct {
    void *h;            /* the movie being followed */
    P4SndVoice *voice;  /* its stream voice, once the format is known */
    int channels;
    uint32_t last_consumed;
    uint64_t played;    /* frames of this movie's audio the mixer has consumed */
} M;

static void drop_voice(void)
{
    if (M.voice != NULL) {
        p4snd_voice_destroy(M.voice);
        M.voice = NULL;
    }
}

static void pump(void)
{
    int16_t pcm[PULL_FRAMES * 2];
    int16_t left[PULL_FRAMES];
    int16_t right[PULL_FRAMES];

    for (;;) {
        uint32_t room = p4snd_stream_room(M.voice);
        int32_t want = room > PULL_FRAMES ? PULL_FRAMES : (int32_t)room;
        int32_t n;
        int32_t i;

        if (want == 0) {
            break;
        }
        n = p4pc_movie_audio_pull(M.h, pcm, want);
        if (n <= 0) {
            break;
        }
        if (M.channels == 1) {
            p4snd_stream_write(M.voice, 0, 0, pcm, (uint32_t)n);
        } else {
            for (i = 0; i < n; i++) {
                left[i] = pcm[i * 2];
                right[i] = pcm[i * 2 + 1];
            }
            p4snd_stream_write(M.voice, 0, 0, left, (uint32_t)n);
            p4snd_stream_write(M.voice, 1, 0, right, (uint32_t)n);
        }
        p4snd_stream_commit(M.voice, (uint32_t)n);
    }
}

static void movie_tick(double seconds)
{
    void *h = p4pc_movie_audio_current();

    (void)seconds;
    if (h != M.h) {
        /* a different movie, or none: the old one is cut off only once its buffer has played */
        if (M.voice != NULL && (h != NULL || p4snd_stream_data(M.voice) == 0)) {
            drop_voice();
        }
        if (M.voice == NULL) {
            M.h = h;
            M.played = 0;
            M.last_consumed = 0;
        } else {
            return; /* draining the last movie's tail */
        }
    }
    if (h == NULL) {
        return;
    }
    if (M.voice == NULL) {
        int32_t rate = 0;
        int32_t ch = 0;

        if (!p4pc_movie_audio_format(h, &rate, &ch) || rate <= 0 || ch < 1 || ch > 2) {
            p4pc_movie_audio_played(h, 0);
            return;
        }
        M.voice = p4snd_stream_create(ch, MOVIE_RING_FRAMES);
        if (M.voice == NULL) {
            return;
        }
        p4snd_voice_set_category(M.voice, P4SND_CAT_MOVIE);
        M.channels = ch;
        M.last_consumed = 0;
        p4snd_voice_set_rate(M.voice, rate);
        if (ch == 1) {
            p4snd_voice_set_gain(M.voice, 0, 1.0f, 1.0f);
        } else {
            p4snd_voice_set_gain(M.voice, 0, 1.0f, 0.0f);
            p4snd_voice_set_gain(M.voice, 1, 0.0f, 1.0f);
        }
        p4snd_voice_play(M.voice, 1);
    }
    pump();
    {
        uint32_t now = p4snd_stream_consumed(M.voice);

        M.played += (uint32_t)(now - M.last_consumed);
        M.last_consumed = now;
    }
    p4pc_movie_audio_played(h, M.played);
}

__attribute__((constructor)) static void movie_audio_register(void)
{
    p4snd_add_tick_hook(movie_tick);
}

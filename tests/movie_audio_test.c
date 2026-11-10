/* movie_audio_test: the movie audio hook (src/platform/sound/movie_audio.c) against a stand-in
 * for the movie player's audio lane interface (src/movie/mwply.h): a stereo 32 kHz track, left a
 * 440 Hz sine, right an 880 Hz sine, pulled in uneven pieces.
 *
 * The hook runs inside p4snd_render. Checks: nothing is pulled before the format is known (but
 * the played count is reported from the first sight of the movie); every output frame after the
 * first 10 ms of the voice equals the track resampled to 48 kHz the way the mixer resamples
 * (linear), left channel on the left only and right on the right only, within 2 LSB; the
 * reported played count ends at the track length; and the voice is gone once the movie ends and
 * its buffer has played. Needs no disc. */
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "mwply.h"
#include "p4snd.h"

#define RATE 32000
#define FRAMES 32000 /* one second */

static int16_t track[FRAMES][2];
static int fed;
static int active;
static int format_ready;
static uint64_t reported;
static int reports;
static int pulled_early;
static int handle;

void *p4pc_movie_audio_current(void)
{
    return active ? &handle : NULL;
}

int32_t p4pc_movie_audio_format(void *h, int32_t *rate, int32_t *channels)
{
    (void)h;
    if (!format_ready) {
        return 0;
    }
    *rate = RATE;
    *channels = 2;
    return 1;
}

int32_t p4pc_movie_audio_pull(void *h, int16_t *dst, int32_t max_frames)
{
    /* the demuxer's pieces: 700 frames at a time */
    int n = FRAMES - fed < 700 ? FRAMES - fed : 700;
    int i;

    (void)h;
    if (!format_ready) {
        pulled_early = 1;
    }
    if (n > max_frames) {
        n = max_frames;
    }
    for (i = 0; i < n; i++) {
        dst[i * 2] = track[fed + i][0];
        dst[i * 2 + 1] = track[fed + i][1];
    }
    fed += n;
    return n;
}

void p4pc_movie_audio_played(void *h, uint64_t frames)
{
    (void)h;
    reported = frames;
    reports++;
}

int main(void)
{
    static float out[(48000 + 9600 + 480) * 2];
    static float blk[480 * 2];
    int i, rendered = 0, worst = 0, failures = 0;
    int start;
    uint32_t idx = 0;
    double frac = 0.0;

    for (i = 0; i < FRAMES; i++) {
        track[i][0] = (int16_t)(10000.0 * sin(2.0 * 3.14159265358979 * 440.0 * i / RATE));
        track[i][1] = (int16_t)(6000.0 * sin(2.0 * 3.14159265358979 * 880.0 * i / RATE));
    }
    p4snd_use_offline();

    /* no movie: nothing happens */
    p4snd_render(blk, 480);
    if (reports != 0) {
        fprintf(stderr, "movie_audio_test: FAIL a report without a movie\n");
        failures++;
    }
    /* a movie whose header has not been read: reported as played 0, nothing pulled */
    active = 1;
    p4snd_render(blk, 480);
    if (reports == 0 || reported != 0 || pulled_early || fed != 0) {
        fprintf(stderr, "movie_audio_test: FAIL before the format is known (reports %d, pulled %d)\n", reports,
                pulled_early);
        failures++;
    }
    format_ready = 1;
    start = rendered;

    /* 10 ms blocks until the track and its buffer are consumed */
    while (rendered + 480 <= 48000 + 9600) {
        p4snd_render(blk, 480);
        for (i = 0; i < 480 * 2; i++) {
            out[rendered * 2 + i] = blk[i];
        }
        rendered += 480;
    }
    if (reported != FRAMES) {
        fprintf(stderr, "movie_audio_test: FAIL played count %llu, want %d\n", (unsigned long long)reported, FRAMES);
        failures++;
    }

    for (i = 0; i < 48000 - 480; i++) {
        int c;

        for (c = 0; c < 2; c++) {
            double a = track[idx][c];
            double b = track[idx + 1][c];
            double want = a + (b - a) * (float)frac;
            double got = (double)out[(start + i) * 2 + c] * 32768.0;

            if (i >= 480) {
                int err = (int)ceil(fabs(want - got));

                if (err > worst) {
                    worst = err;
                }
            }
        }
        frac += (double)RATE / 48000.0;
        while (frac >= 1.0) {
            frac -= 1.0;
            idx++;
        }
    }
    printf("movie_audio_test: worst deviation %d LSB\n", worst);
    if (worst > 2) {
        fprintf(stderr, "movie_audio_test: FAIL output differs from the resampled track\n");
        failures++;
    }

    /* the movie ends: its voice goes once the buffer has played, and the mix is silent */
    active = 0;
    p4snd_render(blk, 480);
    p4snd_render(blk, 480);
    for (i = 0; i < 480 * 2; i++) {
        if (blk[i] != 0.0f) {
            fprintf(stderr, "movie_audio_test: FAIL sound after the movie ended\n");
            failures++;
            break;
        }
    }
    p4snd_shutdown();
    return failures != 0;
}

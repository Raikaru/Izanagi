/* mixer_device_test: the mixer on a real (headless) output path.
 *
 * argv[1] names the SDL audio driver to select ("dummy" is SDL's silent driver that consumes at the
 * device rate; a name SDL does not have, such as "none", makes the device open fail, which must
 * leave the pacing thread consuming at the same rate). A stream voice is given four seconds of
 * samples and started. The test does not assert a wall-clock rate window (that fails on a loaded
 * host); it waits, with a generous timeout, until the device has pulled a quarter of a second, and
 * then checks what holds at any load:
 *   - the device pulls the mixer at all (progress within the timeout);
 *   - it never runs ahead of real time: the frames consumed are at most the elapsed wall time
 *     plus a fixed allowance for the driver's buffering (a load only makes the elapsed time longer);
 *   - the frames are accounted for exactly: consumed + still buffered == pushed;
 *   - the voice is not drained (data is still buffered), and a sample voice starts and ends by
 *     itself. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "p4pc/os.h"
#include "../src/platform/sound/p4snd_mixer.c"

#define RATE 48000

static void sleep_ms(int ms)
{
    p4_os_sleep_ns((uint64_t)ms * 1000000u);
}

static int virtual_test(P4SndVoice *voice, P4SndVoice *sample, int live, int device)
{
    const uint64_t expected = 20ull * P4SND_RATE * 1001ull / 60000ull;
    uint64_t before = p4snd_frames_rendered();
    int failures = 0, queued = 0;
    if (!g.virtual_clock || g.pacer != NULL || (g.stream != NULL) != device) {
        fprintf(stderr, "mixer_device_test: FAIL virtual/live device ownership\n");
        failures++;
    }
    if (g.stream != NULL && !SDL_PauseAudioStreamDevice(g.stream)) return 1;
    sleep_ms(30);
    if (p4snd_frames_rendered() != before || p4snd_stream_consumed(voice) != 0) failures++;
    for (int i = 0; i < 20; i++) p4snd_virtual_tick();
    if (p4snd_frames_rendered() - before != expected || p4snd_stream_consumed(voice) != expected ||
        sample == NULL || !p4snd_sample_done(sample)) {
        fprintf(stderr, "mixer_device_test: FAIL virtual tick sample accounting\n");
        failures++;
    }
    if (g.stream != NULL) {
        queued = SDL_GetAudioStreamQueued(g.stream);
        if (queued != (int)(expected * sizeof(float) * 2)) {
            fprintf(stderr, "mixer_device_test: FAIL virtual samples not queued for playback (%d)\n", queued);
            failures++;
        }
        if (!SDL_ResumeAudioStreamDevice(g.stream)) return 1;
        const uint64_t deadline = p4_os_now_ns() + 60000000000ull;
        while (SDL_GetAudioStreamQueued(g.stream) >= queued && p4_os_now_ns() < deadline) sleep_ms(10);
        if (SDL_GetAudioStreamQueued(g.stream) >= queued) {
            fprintf(stderr, "mixer_device_test: FAIL playback-only device did not drain\n");
            failures++;
        }
    } else {
        sleep_ms(30);
    }
    if (p4snd_frames_rendered() - before != expected || p4snd_stream_consumed(voice) != expected) {
        fprintf(stderr, "mixer_device_test: FAIL device/failure advanced the virtual game clock\n");
        failures++;
    }
    fprintf(stderr, "mixer_device_test: virtual live=%d device=%d ticks=20 frames=%llu queued=%d clock stable\n",
            live, device, (unsigned long long)expected, queued);
    p4snd_shutdown();
    return failures != 0;
}

static int zero_pitch_test(void)
{
    int16_t pcm[16];
    float output[480];
    P4SndSample sample = {pcm, 16, 0, 0};
    P4SndEnv envelope = {0, 0, 1, 0, 0};
    P4SndVoice *voice;
    int failed;
    for (unsigned i = 0; i < 16; i++)
        pcm[i] = (int16_t)(1000 + i * 100);
    p4snd_use_offline();
    voice = p4snd_sample_create();
    if (!voice)
        return 1;
    p4snd_sample_start(voice, &sample, P4SND_RATE, &envelope);
    p4snd_voice_set_rate(voice, 0);
    p4snd_render(output, 240);
    failed = voice->rate != 0 || voice->spos != 0 || p4snd_sample_done(voice);
    p4snd_voice_set_rate(voice, P4SND_RATE);
    p4snd_render(output, 8);
    failed |= voice->spos == 0;
    p4snd_shutdown();
    printf("mixer_device_test: zero SPU pitch freezes source; nonzero pitch resumes: %s\n", failed ? "FAIL" : "ok");
    return failed;
}

int main(int argc, char **argv)
{
    static int16_t pcm[RATE * 4];
    static int16_t blip[480];
    P4SndSample smp = {blip, 480, 0, 0};
    P4SndVoice *v, *s;
    uint32_t used, data, again;
    uint64_t t0, elapsed_ns;
    int i, failures = 0;

    if (argc > 2 && strcmp(argv[2], "zero-pitch") == 0) return zero_pitch_test();
    const int live = argc > 2 && strcmp(argv[2], "live") == 0;
    const int virtual_clock = live || (argc > 2 && strcmp(argv[2], "virtual") == 0);
    p4_os_setenv("P4PC_VIRTUAL_CLOCK", virtual_clock ? "1" : "0", 1);
    p4_os_setenv("P4PC_TURBO", "0", 1);
    p4_os_setenv("P4PC_LOCKSTEP_LIVE", live ? "1" : "0", 1);
    p4_os_setenv("SDL_AUDIODRIVER", argc > 1 ? argv[1] : "dummy", 1);
    for (i = 0; i < RATE * 4; i++) {
        pcm[i] = (int16_t)((i % 100) * 100 - 5000);
    }
    for (i = 0; i < 480; i++) {
        blip[i] = (int16_t)((i % 40) * 500 - 10000);
    }
    v = p4snd_stream_create(1, RATE * 8);
    if (v == NULL) {
        fprintf(stderr, "mixer_device_test: FAIL no stream voice\n");
        return 1;
    }
    p4snd_voice_set_rate(v, RATE);
    p4snd_voice_set_gain(v, 0, 0.5f, 0.5f);
    p4snd_stream_write(v, 0, 0, pcm, RATE * 4);
    p4snd_stream_commit(v, RATE * 4);
    t0 = p4_os_now_ns();
    p4snd_voice_play(v, 1);

    s = p4snd_sample_create();
    if (s != NULL) {
        P4SndEnv env = {0.0f, 0.0f, 1.0f, 0.01f, 0.0f};

        p4snd_voice_set_gain(s, 0, 0.5f, 0.5f);
        p4snd_sample_start(s, &smp, RATE, &env);
    }
    if (virtual_clock) return virtual_test(v, s, live, live && strcmp(argv[1], "dummy") == 0);

    /* wait for the device to pull a quarter of a second (or the timeout: a host this loaded is broken) */
    do {
        sleep_ms(10);
        used = p4snd_stream_consumed(v);
        elapsed_ns = p4_os_now_ns() - t0;
    } while (used < RATE / 4 && elapsed_ns < 60ull * 1000000000ull);
    fprintf(stderr, "mixer_device_test: driver %s consumed %u frames in %llu ms\n", argc > 1 ? argv[1] : "dummy", used,
            (unsigned long long)(elapsed_ns / 1000000u));
    if (used < RATE / 4) {
        fprintf(stderr, "mixer_device_test: FAIL the device did not pull the mixer\n");
        failures++;
    }
    /* never ahead of real time: allow the driver half a second of buffering */
    if ((double)used > (double)elapsed_ns * 1e-9 * RATE + RATE / 2) {
        fprintf(stderr, "mixer_device_test: FAIL consumption ran ahead of real time\n");
        failures++;
    }
    /* accounting: a stable read of consumed around the buffered count */
    do {
        used = p4snd_stream_consumed(v);
        data = p4snd_stream_data(v);
        again = p4snd_stream_consumed(v);
    } while (used != again);
    if (used + data != (uint32_t)RATE * 4) {
        fprintf(stderr, "mixer_device_test: FAIL consumed %u + buffered %u != pushed %u\n", used, data, RATE * 4);
        failures++;
    }
    if (data == 0) {
        fprintf(stderr, "mixer_device_test: FAIL the ring emptied\n");
        failures++;
    }
    for (i = 0; s != NULL && !p4snd_sample_done(s) && i < 6000; i++) {
        sleep_ms(10);
    }
    if (s == NULL || !p4snd_sample_done(s)) {
        fprintf(stderr, "mixer_device_test: FAIL the 10 ms sample did not finish\n");
        failures++;
    }
    p4snd_shutdown();
    if (failures == 0) {
        printf("mixer_device_test: ok\n");
    }
    return failures != 0;
}

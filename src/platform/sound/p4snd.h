/* Host audio for the PC port: the SDL3 output device and a small mixer of voices.
 *
 * The device is 48 kHz stereo float. Every producer in the port reaches it through voices:
 *
 *   stream voices   PCM that a producer pushes into a per-channel ring (CRI's ADX output, AXRNA).
 *                   The mixer consumes it at the voice's source rate with linear resampling, so
 *                   the producer sees real-time drain: GetNumData/GetNumRoom behave like the
 *                   PS2 RNA ring they replace.
 *   sample voices   a decoded mono PCM sample (VAG) played at a pitch with loop points and an
 *                   ADSR envelope: the SPU2 voices the IOP sound driver (sdr/sksound) drives.
 *
 * Threads: the game runs on one host thread and calls everything here; the SDL audio thread
 * only runs p4snd_render. A mutex guards voice state and render; the stream rings are single
 * producer (game thread) / single consumer (mixer) with atomic counters, so pushing PCM never
 * waits for the mixer.
 *
 * Headless: SDL_AUDIODRIVER=dummy (or disk) selects the SDL driver; with no device at all a
 * pacing thread consumes in real time so stream state machines still advance. Tests call
 * p4snd_use_offline() and pull p4snd_render themselves. */
#ifndef P4PC_P4SND_H
#define P4PC_P4SND_H

#include <stdint.h>

#define P4SND_RATE 48000
#define P4SND_MAX_CH 8

typedef struct P4SndVoice P4SndVoice;

/* ------------------------------------------------------------------ device */

/* Select offline mode: no SDL device and no pacing thread. The caller pulls p4snd_render. Call
 * before the first voice is created. */
void p4snd_use_offline(void);

/* Open the SDL device (idempotent, called by the first voice). Returns 0 on success; on failure
 * the pacing thread takes over and the return value is negative. */
int p4snd_open(void);

/* Close the device and free every voice. */
void p4snd_shutdown(void);

/* Mix `frames` stereo frames (interleaved float, clamped to +-1) into out. The SDL callback
 * calls this; offline callers call it directly. */
void p4snd_render(float *out, int frames);

/* The reverb unit (the SPU2 effect unit; one for the whole mixer). p4snd_reverb_setup loads a preset: the 32
 * register values in libsd's order and the work area size in bytes (it also clears the area). The output of
 * the voices' reverb sends is scaled by the effect volume (1.15 fixed point, 0x7fff full). Off by default. */
void p4snd_reverb_setup(const int16_t regs[32], uint32_t size_bytes);
void p4snd_reverb_enable(int on);
void p4snd_reverb_volume(int32_t evol_l, int32_t evol_r);

/* Linear master gain (default 1). */
void p4snd_set_master(float gain);

/* The player's volume, 0 to 1 (default 1): the gain of the device's stream, on top of the master gain the game sets. It does not touch p4snd_render, so
 * the offline renders of the tests and gates are as they were. Applied when the device opens, and at once to an open one. */
void p4snd_set_user_volume(float gain);


/* Registers a function called with the elapsed seconds before every 5 ms chunk of p4snd_render,
 * on whichever thread renders, outside the mixer lock (so it may start and release voices).
 * The SE sequencer and the movie audio pump use it as their clock. Up to four, each added
 * once. */
void p4snd_add_tick_hook(void (*hook)(double seconds));

/* Once per VBLANK period on a virtual-clock run (p4_os_virtual_clock; the kernel's p4pc_vblank_period_hook, which
 * p4pc_platform_init sets): renders that period's share of samples, with no realtime mixer/pacing thread. A game update
 * spanning two vsyncs gets two periods.
 * Explicit P4PC_LOCKSTEP_LIVE=1 also sends these samples to a playback-only device; its consumption never advances this clock.
 * Does nothing otherwise, or when no voice has opened the mixer yet. */
void p4snd_virtual_tick(void);

/* Frames rendered since the device was opened (for tests and clocks). */
uint64_t p4snd_frames_rendered(void);

/* ------------------------------------------------------------------ stream voices */

/* A stream voice with nch (1..P4SND_MAX_CH) channels and a per-channel ring of ring_frames s16
 * samples. Created stopped. Returns NULL when no voice is free. */
P4SndVoice *p4snd_stream_create(int nch, uint32_t ring_frames);

/* Frames pushed and not yet consumed, and the free space, in frames. */
uint32_t p4snd_stream_data(const P4SndVoice *v);
uint32_t p4snd_stream_room(const P4SndVoice *v);

/* Copy n frames of channel ch into the ring at `ahead` frames past the write position (ahead is
 * 0 for the first call of a transfer, and the running total for a later chunk of the same
 * transfer). The frames become audible after p4snd_stream_commit. */
void p4snd_stream_write(P4SndVoice *v, int ch, uint32_t ahead, const int16_t *src, uint32_t n);
void p4snd_stream_commit(P4SndVoice *v, uint32_t n);

/* Drop everything buffered (the consumer position jumps to the write position). */
void p4snd_stream_flush(P4SndVoice *v);

/* Frames the mixer has consumed since the voice was created (wraps at 2^32, like the ring's
 * counters). With the data count it tells how much of what was pushed has been played. */
uint32_t p4snd_stream_consumed(const P4SndVoice *v);

/* Skip up to n buffered frames; returns how many were skipped. */
uint32_t p4snd_stream_discard(P4SndVoice *v, uint32_t n);

/* ------------------------------------------------------------------ sample voices */

/* A decoded sample: mono s16 frames, an optional loop [loop_start, frames) and its own rate.
 * The mixer reads it by pointer, so it must outlive every voice that plays it. */
typedef struct P4SndSample {
    int16_t *pcm;
    uint32_t frames;
    uint32_t loop_start;
    int has_loop;
} P4SndSample;

/* A voice for sample playback. Created stopped. NULL when none is free. */
P4SndVoice *p4snd_sample_create(void);

/* Start sample at its first frame. rate_hz is the source rate the sample plays at after pitch
 * (frames per second of the sample), env is the ADSR in seconds and sustain level 0..1. */
typedef struct P4SndEnv {
    float attack, decay, sustain, release; /* seconds, seconds, level 0..1, seconds */
    float sustain_decay;                   /* seconds to fall 60 dB while sustaining, 0 = hold */
} P4SndEnv;
void p4snd_sample_start(P4SndVoice *v, const P4SndSample *s, double rate_hz, const P4SndEnv *env);

/* Key off: the envelope goes to release and the voice ends when it reaches silence. */
void p4snd_sample_release(P4SndVoice *v);

/* True once a sample voice has ended by itself (sample end without loop, or release finished).
 * Also true for a voice that was never started. */
int p4snd_sample_done(const P4SndVoice *v);

/* ------------------------------------------------------------------ common controls */

void p4snd_voice_destroy(P4SndVoice *v);

/* Source rate in frames per second (stream: ADX rate; sample: base rate x pitch).
 * Zero is a valid SPU pitch: freeze source position without stopping its envelope. */
void p4snd_voice_set_rate(P4SndVoice *v, double hz);

/* Output gains of channel ch (linear, 0..1 per side). */
void p4snd_voice_set_gain(P4SndVoice *v, int ch, float left, float right);

/* Routing of a sample voice: whether its left and right outputs reach the dry buses and the reverb's left and
 * right inputs (all four switches are the SPU2's VMIXL, VMIXR, VMIXEL, VMIXER bits of the voice). Default: dry
 * on, reverb off. */
void p4snd_voice_set_mix(P4SndVoice *v, int dry_l, int dry_r, int wet_l, int wet_r);

/* Run (1) or pause (0). A paused stream voice keeps its buffered data. */
void p4snd_voice_play(P4SndVoice *v, int on);
int p4snd_voice_playing(const P4SndVoice *v);

/* ------------------------------------------------------------------ helpers */

/* Gains for a CRI output: vol in 1/10 dB (0 = full, more negative = quieter) and pan in -15..15
 * (-15 left, 0 centre, 15 right). Equal power. */
void p4snd_cri_gains(int32_t vol_db10, int32_t pan, float *left, float *right);

#endif /* P4PC_P4SND_H */

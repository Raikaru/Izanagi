/* SDL3 output device and voice mixer. See p4snd.h for the model. */
#include "p4snd.h"
#include "p4snd_reverb.h"

#include <math.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <SDL3/SDL.h>

#include "p4pc/os.h"
#include "p4pc/profile_trace.h"

#define MAX_VOICES 64
#define RENDER_CHUNK 1024
#define CHUNK_FRAMES 240
#define MAX_HOOKS 4

enum { KIND_FREE = 0, KIND_STREAM, KIND_SAMPLE };
enum { ENV_OFF = 0, ENV_ATTACK, ENV_DECAY, ENV_SUSTAIN, ENV_RELEASE };

struct P4SndVoice {
    int kind;
    int playing;
    int nch;
    double rate; /* source frames per second */
    float target_l[P4SND_MAX_CH], target_r[P4SND_MAX_CH];
    float cur_l[P4SND_MAX_CH], cur_r[P4SND_MAX_CH];

    /* stream: ring per channel; counters wrap at 2^32 */
    int16_t *ring[P4SND_MAX_CH];
    uint32_t cap;
    _Atomic uint32_t wr; /* frames committed by the producer */
    _Atomic uint32_t rd; /* frames consumed, published by the mixer */
    uint32_t rd_local;   /* mixer-private copy */
    double frac;

    /* sample */
    const P4SndSample *smp;
    uint32_t spos;
    int done;
    int env_state;
    float env_level;
    float env_atk, env_dec, env_sus, env_rel; /* per-output-frame increments / decay factors */
    float env_sus_level;
    float env_sus_dec; /* per-output-frame factor while sustaining (1 = hold) */
    /* routing of a sample voice: its left output to the dry left bus and the reverb's left input, and so on */
    float mix_dry_l, mix_dry_r, mix_wet_l, mix_wet_r;
};

static struct {
    SDL_Mutex *lock;
    P4SndVoice voices[MAX_VOICES];
    float master;
    float user_gain; /* the player's volume (settings), applied to the device stream only */
    int offline;
    int virtual_clock; /* tick-driven; live lockstep may also have a playback-only device */
    unsigned long long virtual_frac; /* samples owed to the flips so far, in units of 1/60000 sample */
    int opened;
    SDL_AudioStream *stream;
    SDL_Thread *pacer;
    _Atomic int pacer_run;
    uint64_t frames_rendered;
    void (*tick_hooks[MAX_HOOKS])(double seconds);
    float scratch[RENDER_CHUNK * 2];
    float wet[CHUNK_FRAMES * 2]; /* the reverb's input for the chunk being mixed */
} g = { .master = 1.0f, .user_gain = 1.0f };

static void ensure_lock(void)
{
    if (g.lock == NULL) {
        g.lock = SDL_CreateMutex();
    }
}

static void log_audio(const char *what)
{
    fprintf(stderr, "p4snd: %s\n", what);
}

/* ------------------------------------------------------------------ gains */

void p4snd_cri_gains(int32_t vol_db10, int32_t pan, float *left, float *right)
{
    double amp;

    if (vol_db10 > 0) {
        vol_db10 = 0;
    }
    amp = vol_db10 <= -960 ? 0.0 : pow(10.0, (double)vol_db10 / 200.0);
    if (pan < -15) {
        pan = -15;
    }
    if (pan > 15) {
        pan = 15;
    }
    /* balance law: both sides full at the centre, the far side falls linearly to 0 at +-15 */
    *left = (float)(amp * (pan <= 0 ? 1.0 : (double)(15 - pan) / 15.0));
    *right = (float)(amp * (pan >= 0 ? 1.0 : (double)(15 + pan) / 15.0));
}

/* ------------------------------------------------------------------ voices */

static P4SndVoice *voice_alloc(int kind)
{
    int i;
    P4SndVoice *v = NULL;
    uint64_t trace_start = p4pc_prof_span_begin();

    ensure_lock();
    SDL_LockMutex(g.lock);
    for (i = 0; i < MAX_VOICES; i++) {
        if (g.voices[i].kind == KIND_FREE) {
            v = &g.voices[i];
            memset(v, 0, sizeof(*v));
            v->kind = kind;
            v->rate = P4SND_RATE;
            v->mix_dry_l = v->mix_dry_r = 1.0f;
            break;
        }
    }
    SDL_UnlockMutex(g.lock);
    p4pc_prof_span_end(trace_start, "audio.voice.allocate", kind, (uintptr_t)v, 0, NULL);
    return v;
}

P4SndVoice *p4snd_stream_create(int nch, uint32_t ring_frames)
{
    P4SndVoice *v;
    int ch;

    if (nch < 1 || nch > P4SND_MAX_CH || ring_frames < 2) {
        return NULL;
    }
    p4snd_open();
    v = voice_alloc(KIND_STREAM);
    if (v == NULL) {
        log_audio("no free voice for a stream");
        return NULL;
    }
    SDL_LockMutex(g.lock);
    v->nch = nch;
    v->cap = ring_frames;
    for (ch = 0; ch < nch; ch++) {
        v->ring[ch] = (int16_t *)calloc(ring_frames, sizeof(int16_t));
        if (v->ring[ch] == NULL) {
            while (ch-- > 0) {
                free(v->ring[ch]);
            }
            v->kind = KIND_FREE;
            SDL_UnlockMutex(g.lock);
            return NULL;
        }
    }
    SDL_UnlockMutex(g.lock);
    return v;
}

P4SndVoice *p4snd_sample_create(void)
{
    P4SndVoice *v;

    p4snd_open();
    v = voice_alloc(KIND_SAMPLE);
    if (v != NULL) {
        SDL_LockMutex(g.lock);
        v->nch = 1;
        v->done = 1;
        SDL_UnlockMutex(g.lock);
    }
    return v;
}

void p4snd_voice_destroy(P4SndVoice *v)
{
    int ch;

    if (v == NULL) {
        return;
    }
    uint64_t trace_start = p4pc_prof_span_begin();
    SDL_LockMutex(g.lock);
    for (ch = 0; ch < P4SND_MAX_CH; ch++) {
        free(v->ring[ch]);
        v->ring[ch] = NULL;
    }
    v->playing = 0;
    v->kind = KIND_FREE;
    SDL_UnlockMutex(g.lock);
    p4pc_prof_span_end(trace_start, "audio.voice.destroy", 0, 0, (uintptr_t)v, NULL);
}

void p4snd_voice_set_rate(P4SndVoice *v, double hz)
{
    if (v != NULL && hz >= 0.0) {
        uint64_t trace_start = p4pc_prof_span_begin();
        SDL_LockMutex(g.lock);
        v->rate = hz;
        SDL_UnlockMutex(g.lock);
        p4pc_prof_span_end(trace_start, "audio.voice.rate", 0, 0, (uintptr_t)v, NULL);
    }
}

void p4snd_voice_set_gain(P4SndVoice *v, int ch, float left, float right)
{
    if (v != NULL && ch >= 0 && ch < P4SND_MAX_CH) {
        uint64_t trace_start = p4pc_prof_span_begin();
        SDL_LockMutex(g.lock);
        v->target_l[ch] = left;
        v->target_r[ch] = right;
        SDL_UnlockMutex(g.lock);
        p4pc_prof_span_end(trace_start, "audio.voice.gain", ch, 0, (uintptr_t)v, NULL);
    }
}

void p4snd_voice_play(P4SndVoice *v, int on)
{
    if (v != NULL) {
        uint64_t trace_start = p4pc_prof_span_begin();
        SDL_LockMutex(g.lock);
        v->playing = on != 0;
        SDL_UnlockMutex(g.lock);
        p4pc_prof_span_end(trace_start, "audio.voice.play", on, 0, (uintptr_t)v, NULL);
    }
}

int p4snd_voice_playing(const P4SndVoice *v)
{
    return v != NULL && v->playing;
}

/* ------------------------------------------------------------------ stream ring */

uint32_t p4snd_stream_data(const P4SndVoice *v)
{
    uint32_t wr = atomic_load_explicit((_Atomic uint32_t *)&v->wr, memory_order_acquire);
    uint32_t rd = atomic_load_explicit((_Atomic uint32_t *)&v->rd, memory_order_acquire);

    return wr - rd;
}

uint32_t p4snd_stream_room(const P4SndVoice *v)
{
    return v->cap - p4snd_stream_data(v);
}

void p4snd_stream_write(P4SndVoice *v, int ch, uint32_t ahead, const int16_t *src, uint32_t n)
{
    uint32_t pos = atomic_load_explicit(&v->wr, memory_order_relaxed) + ahead;
    uint32_t i;

    if (ch < 0 || ch >= v->nch) {
        return;
    }
    for (i = 0; i < n; i++) {
        v->ring[ch][(pos + i) % v->cap] = src[i];
    }
}

void p4snd_stream_commit(P4SndVoice *v, uint32_t n)
{
    atomic_fetch_add_explicit(&v->wr, n, memory_order_release);
}

void p4snd_stream_flush(P4SndVoice *v)
{
    uint64_t trace_start = p4pc_prof_span_begin();
    SDL_LockMutex(g.lock);
    atomic_store(&v->rd, atomic_load(&v->wr));
    v->rd_local = atomic_load(&v->wr);
    v->frac = 0.0;
    SDL_UnlockMutex(g.lock);
    p4pc_prof_span_end(trace_start, "audio.stream.flush", 0, 0, (uintptr_t)v, NULL);
}

uint32_t p4snd_stream_discard(P4SndVoice *v, uint32_t n)
{
    uint32_t avail;

    SDL_LockMutex(g.lock);
    avail = atomic_load(&v->wr) - v->rd_local;
    if (n > avail) {
        n = avail;
    }
    v->rd_local += n;
    atomic_store(&v->rd, v->rd_local);
    SDL_UnlockMutex(g.lock);
    return n;
}

/* ------------------------------------------------------------------ sample voices */

static float env_step(float seconds)
{
    /* per-output-frame linear step to cross 0..1 in `seconds` */
    if (seconds <= 0.0005f) {
        return 1.0f;
    }
    return 1.0f / (seconds * (float)P4SND_RATE);
}

static float env_decay(float seconds)
{
    /* exponential: reaches ~-60 dB (0.001) after `seconds` */
    if (seconds <= 0.0005f) {
        return 0.0f;
    }
    return powf(0.001f, 1.0f / (seconds * (float)P4SND_RATE));
}

void p4snd_sample_start(P4SndVoice *v, const P4SndSample *s, double rate_hz, const P4SndEnv *env)
{
    if (v == NULL || s == NULL || s->pcm == NULL || s->frames == 0) {
        return;
    }
    SDL_LockMutex(g.lock);
    v->smp = s;
    v->spos = 0;
    v->frac = 0.0;
    v->rate = rate_hz > 0.0 ? rate_hz : P4SND_RATE;
    v->done = 0;
    v->env_state = ENV_ATTACK;
    v->env_level = 0.0f;
    v->env_atk = env_step(env ? env->attack : 0.0f);
    v->env_dec = env_decay(env ? env->decay : 0.0f);
    v->env_sus_level = env ? env->sustain : 1.0f;
    v->env_sus_dec = env && env->sustain_decay > 0.0005f ? env_decay(env->sustain_decay) : 1.0f;
    v->env_rel = env_decay(env ? env->release : 0.0f);
    v->playing = 1;
    SDL_UnlockMutex(g.lock);
}

void p4snd_sample_release(P4SndVoice *v)
{
    if (v == NULL) {
        return;
    }
    SDL_LockMutex(g.lock);
    if (v->env_state != ENV_OFF) {
        v->env_state = ENV_RELEASE;
    }
    SDL_UnlockMutex(g.lock);
}

int p4snd_sample_done(const P4SndVoice *v)
{
    return v == NULL || v->done;
}

/* ------------------------------------------------------------------ mixing */

static void mix_stream(P4SndVoice *v, float *out, int frames)
{
    uint32_t wr = atomic_load_explicit(&v->wr, memory_order_acquire);
    uint32_t rd = v->rd_local;
    double frac = v->frac;
    const double step = v->rate / (double)P4SND_RATE;
    float l[P4SND_MAX_CH], r[P4SND_MAX_CH];
    float dl[P4SND_MAX_CH], dr[P4SND_MAX_CH];
    int ch;
    int i;

    for (ch = 0; ch < v->nch; ch++) {
        l[ch] = v->cur_l[ch];
        r[ch] = v->cur_r[ch];
        dl[ch] = (v->target_l[ch] - l[ch]) / (float)frames;
        dr[ch] = (v->target_r[ch] - r[ch]) / (float)frames;
    }
    for (i = 0; i < frames; i++) {
        uint32_t avail = wr - rd;
        float fl = 0.0f;
        float fr = 0.0f;

        if (avail >= 1) {
            for (ch = 0; ch < v->nch; ch++) {
                float a = (float)v->ring[ch][rd % v->cap];
                float b = avail >= 2 ? (float)v->ring[ch][(rd + 1) % v->cap] : a;
                float s = (a + (b - a) * (float)frac) * (1.0f / 32768.0f);

                fl += s * l[ch];
                fr += s * r[ch];
            }
            frac += step;
            while (frac >= 1.0) {
                frac -= 1.0;
                rd++;
            }
        }
        out[i * 2] += fl;
        out[i * 2 + 1] += fr;
        for (ch = 0; ch < v->nch; ch++) {
            l[ch] += dl[ch];
            r[ch] += dr[ch];
        }
    }
    for (ch = 0; ch < v->nch; ch++) {
        v->cur_l[ch] = v->target_l[ch];
        v->cur_r[ch] = v->target_r[ch];
    }
    /* a consumed position past the writer (a fractional step at the very end) stays at wr */
    if ((int32_t)(rd - wr) > 0) {
        rd = wr;
        frac = 0.0;
    }
    v->rd_local = rd;
    v->frac = frac;
    atomic_store_explicit(&v->rd, rd, memory_order_release);
}

static void mix_sample(P4SndVoice *v, float *out, int frames)
{
    const P4SndSample *s = v->smp;
    const double step = v->rate / (double)P4SND_RATE;
    float l = v->cur_l[0];
    float r = v->cur_r[0];
    const float dl = (v->target_l[0] - l) / (float)frames;
    const float dr = (v->target_r[0] - r) / (float)frames;
    int i;

    for (i = 0; i < frames; i++) {
        float a;
        float b;
        float smp;
        uint32_t next;

        if (v->done) {
            break;
        }
        switch (v->env_state) {
        case ENV_ATTACK:
            v->env_level += v->env_atk;
            if (v->env_level >= 1.0f) {
                v->env_level = 1.0f;
                v->env_state = ENV_DECAY;
            }
            break;
        case ENV_DECAY:
            v->env_level *= v->env_dec;
            if (v->env_level <= v->env_sus_level) {
                v->env_level = v->env_sus_level;
                v->env_state = ENV_SUSTAIN;
            }
            break;
        case ENV_RELEASE:
            v->env_level *= v->env_rel;
            if (v->env_level < 0.0005f) {
                v->done = 1;
                v->playing = 0;
                v->env_state = ENV_OFF;
            }
            break;
        case ENV_SUSTAIN:
            if (v->env_sus_dec < 1.0f) {
                v->env_level *= v->env_sus_dec;
                if (v->env_level < 0.0005f) {
                    v->done = 1;
                    v->playing = 0;
                    v->env_state = ENV_OFF;
                }
            }
            break;
        default:
            break;
        }
        if (v->done) {
            break;
        }
        next = v->spos + 1;
        if (next >= s->frames) {
            next = s->has_loop ? s->loop_start : v->spos;
        }
        a = (float)s->pcm[v->spos];
        b = (float)s->pcm[next];
        smp = (a + (b - a) * (float)v->frac) * (1.0f / 32768.0f) * v->env_level;
        out[i * 2] += smp * l * v->mix_dry_l;
        out[i * 2 + 1] += smp * r * v->mix_dry_r;
        g.wet[i * 2] += smp * l * v->mix_wet_l;
        g.wet[i * 2 + 1] += smp * r * v->mix_wet_r;
        l += dl;
        r += dr;
        v->frac += step;
        while (v->frac >= 1.0) {
            v->frac -= 1.0;
            v->spos++;
            if (v->spos >= s->frames) {
                if (s->has_loop) {
                    v->spos = s->loop_start;
                } else {
                    v->done = 1;
                    v->playing = 0;
                    v->env_state = ENV_OFF;
                    break;
                }
            }
        }
    }
    v->cur_l[0] = v->target_l[0];
    v->cur_r[0] = v->target_r[0];
}

/* Mixes one chunk of at most CHUNK_FRAMES frames. */
static void mix_chunk(float *out, int frames)
{
    int i;

    uint64_t trace_start = p4pc_prof_span_begin();
    uint64_t trace_locked;
    SDL_LockMutex(g.lock);
    trace_locked = p4pc_prof_span_stop(trace_start);
    memset(g.wet, 0, (size_t)frames * 2 * sizeof(float));
    for (i = 0; i < MAX_VOICES; i++) {
        P4SndVoice *v = &g.voices[i];

        if (v->kind == KIND_FREE || !v->playing) {
            continue;
        }
        if (v->kind == KIND_STREAM) {
            mix_stream(v, out, frames);
        } else {
            mix_sample(v, out, frames);
        }
    }
    if (!g.virtual_clock || g.stream != NULL) {
        p4rev_process(g.wet, out, frames); /* only skip effects when virtual samples are discarded */
    }
    g.frames_rendered += (uint64_t)frames;
    SDL_UnlockMutex(g.lock);
    uint64_t trace_end = p4pc_prof_span_stop(trace_start);
    p4pc_prof_span_record(trace_start, trace_locked, "audio.mix.lock", frames, 0, 0, NULL);
    p4pc_prof_span_record(trace_start, trace_end, "audio.mix.chunk", frames, 0, 0, NULL);
}

void p4snd_add_tick_hook(void (*hook)(double seconds))
{
    int i;

    ensure_lock();
    SDL_LockMutex(g.lock);
    for (i = 0; i < MAX_HOOKS; i++) {
        if (g.tick_hooks[i] == hook) {
            break;
        }
        if (g.tick_hooks[i] == NULL) {
            g.tick_hooks[i] = hook;
            break;
        }
    }
    SDL_UnlockMutex(g.lock);
}

uint32_t p4snd_stream_consumed(const P4SndVoice *v)
{
    return atomic_load_explicit((_Atomic uint32_t *)&v->rd, memory_order_acquire);
}

void p4snd_render(float *out, int frames)
{
    int i;
    int n = frames * 2;
    int done = 0;
    uint64_t trace_start = p4pc_prof_span_begin();

    ensure_lock();
    memset(out, 0, (size_t)n * sizeof(float));
    /* The hook (the SE sequencer) runs before each chunk, outside the mixer lock, so a note it
     * starts is audible in that chunk: sequencer timing is exact to CHUNK_FRAMES (5 ms). */
    while (done < frames) {
        int c = frames - done > CHUNK_FRAMES ? CHUNK_FRAMES : frames - done;

        for (i = 0; i < MAX_HOOKS && g.tick_hooks[i] != NULL; i++) {
            uint64_t hook_start = p4pc_prof_span_begin();
            g.tick_hooks[i]((double)c / (double)P4SND_RATE);
            p4pc_prof_span_end(hook_start, "audio.tick", c, 0, (uintptr_t)g.tick_hooks[i], NULL);
        }
        mix_chunk(out + done * 2, c);
        done += c;
    }
    for (i = 0; i < n; i++) {
        float x = out[i] * g.master;

        out[i] = x > 1.0f ? 1.0f : (x < -1.0f ? -1.0f : x);
    }
    p4pc_prof_span_end(trace_start, "audio.render", frames, 0, 0, NULL);
}

void p4snd_voice_set_mix(P4SndVoice *v, int dry_l, int dry_r, int wet_l, int wet_r)
{
    ensure_lock();
    uint64_t trace_start = p4pc_prof_span_begin();
    SDL_LockMutex(g.lock);
    v->mix_dry_l = dry_l ? 1.0f : 0.0f;
    v->mix_dry_r = dry_r ? 1.0f : 0.0f;
    v->mix_wet_l = wet_l ? 1.0f : 0.0f;
    v->mix_wet_r = wet_r ? 1.0f : 0.0f;
    SDL_UnlockMutex(g.lock);
    p4pc_prof_span_end(trace_start, "audio.voice.mix", 0, 0, (uintptr_t)v, NULL);
}

void p4snd_reverb_setup(const int16_t regs[32], uint32_t size_bytes)
{
    ensure_lock();
    uint64_t trace_start = p4pc_prof_span_begin();
    SDL_LockMutex(g.lock);
    p4rev_setup(regs, size_bytes);
    SDL_UnlockMutex(g.lock);
    p4pc_prof_span_end(trace_start, "audio.reverb.setup", size_bytes, 0, 0, NULL);
}

void p4snd_reverb_enable(int on)
{
    ensure_lock();
    uint64_t trace_start = p4pc_prof_span_begin();
    SDL_LockMutex(g.lock);
    p4rev_enable(on);
    SDL_UnlockMutex(g.lock);
    p4pc_prof_span_end(trace_start, "audio.reverb.enable", on, 0, 0, NULL);
}

void p4snd_reverb_volume(int32_t evol_l, int32_t evol_r)
{
    ensure_lock();
    uint64_t trace_start = p4pc_prof_span_begin();
    SDL_LockMutex(g.lock);
    p4rev_volume(evol_l, evol_r);
    SDL_UnlockMutex(g.lock);
    p4pc_prof_span_end(trace_start, "audio.reverb.volume", 0, 0, 0, NULL);
}

void p4snd_set_master(float gain)
{
    g.master = gain;
}

void p4snd_set_user_volume(float gain)
{
    g.user_gain = gain < 0.0f ? 0.0f : (gain > 1.0f ? 1.0f : gain);
    if (g.stream != NULL) {
        SDL_SetAudioStreamGain(g.stream, g.user_gain);
    }
}


uint64_t p4snd_frames_rendered(void)
{
    return g.frames_rendered;
}

/* ------------------------------------------------------------------ device */

static void SDLCALL audio_cb(void *userdata, SDL_AudioStream *stream, int additional, int total)
{
    int frames = additional / (int)(sizeof(float) * 2);

    (void)userdata;
    (void)total;
    while (frames > 0) {
        int n = frames > RENDER_CHUNK ? RENDER_CHUNK : frames;

        p4snd_render(g.scratch, n);
        SDL_PutAudioStreamData(stream, g.scratch, n * (int)(sizeof(float) * 2));
        frames -= n;
    }
}

/* No device: keep time ourselves so stream producers see a draining ring. */
static int SDLCALL pacer_main(void *arg)
{
    float buf[480 * 2];
    uint64_t next = SDL_GetTicksNS();

    (void)arg;
    while (atomic_load(&g.pacer_run)) {
        p4snd_render(buf, 480);
        next += (uint64_t)480 * 1000000000ull / P4SND_RATE;
        {
            uint64_t now = SDL_GetTicksNS();

            if (next > now) {
                SDL_DelayNS(next - now);
            } else {
                next = now;
            }
        }
    }
    return 0;
}

/* The flip's share of audio on a virtual-clock run: 48000 / (60000 / 1001) samples, the remainder carried. */
void p4snd_virtual_tick(void)
{
    int frames;

    if (!g.virtual_clock) {
        return;
    }
    g.virtual_frac += (unsigned long long)P4SND_RATE * 1001ull;
    frames = (int)(g.virtual_frac / 60000ull);
    g.virtual_frac %= 60000ull;
    while (frames > 0) {
        const int n = frames > RENDER_CHUNK ? RENDER_CHUNK : frames;

        p4snd_render(g.scratch, n);
        if (g.stream != NULL)
            SDL_PutAudioStreamData(g.stream, g.scratch, n * (int)(sizeof(float) * 2));
        frames -= n;
    }
}

void p4snd_use_offline(void)
{
    g.offline = 1;
    g.opened = 1;
    ensure_lock();
}

static int start_pacer(void)
{
    atomic_store(&g.pacer_run, 1);
    g.pacer = SDL_CreateThread(pacer_main, "p4snd-pacer", NULL);
    return g.pacer != NULL ? 0 : -1;
}

int p4snd_open(void)
{
    SDL_AudioSpec spec;

    if (g.opened) {
        return g.stream != NULL || g.offline ? 0 : -1;
    }
    g.opened = 1;
    ensure_lock();
    if (p4_os_virtual_clock()) {
        /* A virtual-clock run (kernel_thread.c) must not see audio consumed in real time: the stream producers and the sequencers that
         * run off the mixer's clock would make the game's frame at which things happen depend on the host's speed. */
        g.offline = 1;
        g.virtual_clock = 1;
        const char *live = getenv("P4PC_LOCKSTEP_LIVE");
        if (live == NULL || strcmp(live, "1") != 0) return 0;
    }
    if (!SDL_InitSubSystem(SDL_INIT_AUDIO)) {
        fprintf(stderr, "p4snd: SDL audio init failed: %s; %s\n", SDL_GetError(),
                g.virtual_clock ? "retaining virtual audio clock" : "pacing without a device");
        if (!g.virtual_clock) start_pacer();
        return -1;
    }
    spec.format = SDL_AUDIO_F32;
    spec.channels = 2;
    spec.freq = P4SND_RATE;
    /* A live deterministic session feeds samples from virtual_tick. The SDL
     * consumer must never render or advance game-visible voice/sequence time. */
    g.stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, g.virtual_clock ? NULL : audio_cb, NULL);
    if (g.stream == NULL) {
        fprintf(stderr, "p4snd: no audio device (%s); %s\n", SDL_GetError(),
                g.virtual_clock ? "retaining virtual audio clock" : "pacing without a device");
        if (!g.virtual_clock) start_pacer();
        return -1;
    }
    SDL_SetAudioStreamGain(g.stream, g.user_gain);
    SDL_ResumeAudioStreamDevice(g.stream);
    return 0;
}

void p4snd_shutdown(void)
{
    int i;

    if (g.pacer != NULL) {
        atomic_store(&g.pacer_run, 0);
        SDL_WaitThread(g.pacer, NULL);
        g.pacer = NULL;
    }
    if (g.stream != NULL) {
        SDL_DestroyAudioStream(g.stream);
        g.stream = NULL;
    }
    if (g.lock != NULL) {
        SDL_LockMutex(g.lock);
        for (i = 0; i < MAX_VOICES; i++) {
            int ch;

            for (ch = 0; ch < P4SND_MAX_CH; ch++) {
                free(g.voices[i].ring[ch]);
            }
            memset(&g.voices[i], 0, sizeof(g.voices[i]));
        }
        SDL_UnlockMutex(g.lock);
    }
    g.opened = 0;
    g.offline = 0;
    g.virtual_clock = 0;
}

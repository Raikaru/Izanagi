/* Golden cue -> existing stream voice. PS2 mixer and ADX producers are untouched. */
#include "p4snd.h"
#include "p4pc/golden_audio.h"
#include "p4pc/game_mode.h"
#include "../config/p4cfg.h"
#include <stdio.h>
#include <stdlib.h>

typedef struct P4SndGoldenLayer {
    P4GoldenHca *decoder;
    P4GoldenAdx *adx;
    P4SndVoice *voice;
    uint32_t channels, position;
    uint32_t frames, loop_start, loop_end;
    int eof, loop;
} P4SndGoldenLayer;
struct P4SndGoldenCue {
    uint32_t count;
    int failed;
    uint64_t origin, frames;
    uint32_t category_mask;
    unsigned acf_profile;
    struct P4SndGoldenCue *next;
    P4SndGoldenLayer layers[];
};
/* Start/update/destroy are producer-thread serialized, including category
 * admission. Mixer callbacks touch only their own voice/ring state. */
static P4SndGoldenCue *category_cues;
static int refuse(char *reason, uint32_t capacity, const char *text)
{
    if (reason && capacity) snprintf(reason, capacity, "%s", text);
    return -1;
}
static const char *admit_categories(const P4GoldenAcb *acb, uint32_t mask)
{
    if (!mask) return NULL;
    uint32_t count2 = 0, count8 = 0, count9 = 0;
    for (const P4SndGoldenCue *q = category_cues; q; q = q->next) {
        if (q->failed || p4snd_golden_cue_done(q)) continue;
        if (q->acf_profile != acb->dry_acf_profile)
            return "A3: registered ACF replacement while categories are active is not approved";
        if (((mask & (1u << 8)) && (q->category_mask & (1u << 9))) ||
            ((mask & (1u << 9)) && (q->category_mask & (1u << 8))))
            return "A3: competing same-group category arbitration is not approved";
        count2 += !!(q->category_mask & (1u << 2));
        count8 += !!(q->category_mask & (1u << 8));
        count9 += !!(q->category_mask & (1u << 9));
    }
    if (((mask & (1u << 2)) && count2 >= 4) ||
        ((mask & (1u << 8)) && count8 >= 1) ||
        ((mask & (1u << 9)) && count9 >= 2))
        return "A3: category instance capacity is exhausted; stealing is not approved";
    return NULL;
}
static uint64_t play_layers(P4SndGoldenCue *p, int on)
{
    P4SndVoice *voices[P4_GOLDEN_CUE_MAX_WAVES];
    for (uint32_t i = 0; i < p->count; ++i) voices[i] = p->layers[i].voice;
    return p4snd_voices_play(voices, p->count, on);
}
int p4snd_golden_cue_start(P4SndGoldenCue **out, const P4GoldenAcb *acb,
                           const P4GoldenAwb *stream_bank, uint32_t cue_id,
                           int category, int loop, char *reason, uint32_t capacity)
{
    /* First branch: no key access, parse, allocation, registration or device on PS2. */
    if (p4_game_mode() != P4_GAME_MODE_GOLDEN) {
        if (out) *out = NULL;
        return refuse(reason, capacity, "Golden cue playback requires Golden mode");
    }
    if (!out) return -1;
    *out = NULL;
    P4GoldenCue cue;
    P4GoldenCuePlan plan;
    if (!acb || category < P4SND_CAT_MUSIC || category > P4SND_CAT_VOICE ||
        p4_golden_acb_cue(acb, cue_id, &cue)) return refuse(reason, capacity, "invalid or absent Golden cue");
    if (p4_golden_acb_plan(acb, &cue, &plan, reason, capacity)) return -1;
    if (loop && plan.count > 1) return refuse(reason, capacity, "A5: multi-wave loop override is not specified");
    const char *admission = admit_categories(acb, plan.category_mask);
    if (admission) return refuse(reason, capacity, admission);
    P4SndGoldenCue *p = calloc(1, sizeof *p + plan.count * sizeof *p->layers);
    if (!p) return refuse(reason, capacity, "Golden audio allocation failed");
    p->count = plan.count;
    p->frames = plan.frames;
    p->category_mask = plan.category_mask;
    p->acf_profile = acb->dry_acf_profile;
    for (uint32_t i = 0; i < plan.count; ++i) {
        const P4GoldenCueWave *w = &plan.waves[i];
        P4SndGoldenLayer *layer = &p->layers[i];
        const P4GoldenAwb *bank = w->streaming ? stream_bank : &acb->memory;
        const P4GoldenWave *wave = bank ? p4_golden_awb_find(bank, w->id) : NULL;
        P4GoldenHcaInfo info;
        const char *failure = NULL;
        if (!wave) failure = "Golden cue wave bank entry is missing";
        else if (p4_golden_hca_info(wave->payload.bytes, wave->payload.size, &info) ||
                 info.rate != w->rate || info.channels != w->channels || info.frames < w->samples)
            failure = "HCA format does not match cue waveform metadata";
        if (failure) {
            p4snd_golden_cue_destroy(p);
            return refuse(reason, capacity, failure);
        }
        layer->channels = info.channels;
        layer->frames = w->samples;
        layer->loop_start = w->loop_end ? w->loop_start : info.loop_start;
        layer->loop_end = w->loop_end ? w->loop_end : (info.has_loop ? info.loop_end : 0);
        layer->loop = loop && w->loop;
        if (layer->loop && (layer->loop_start >= layer->loop_end || layer->loop_end > w->samples)) {
            p4snd_golden_cue_destroy(p);
            return refuse(reason, capacity, "Golden cue loop metadata is missing or out of range");
        }
        uint64_t key = 0;
        if (info.cipher == 56 && p4cfg_hca_key(&key) < 0) {
            p4snd_golden_cue_destroy(p);
            return refuse(reason, capacity, "invalid private HCA key setting");
        }
        int result = p4_golden_hca_open(&layer->decoder, wave->payload.bytes, wave->payload.size, key, bank->subkey, reason, capacity);
        *(volatile uint64_t *)&key = 0;
        if (result) { p4snd_golden_cue_destroy(p); return result; }
        layer->voice = p4snd_stream_create((int)info.channels, 8192);
        if (!layer->voice) {
            p4snd_golden_cue_destroy(p);
            return refuse(reason, capacity, "Golden audio voice allocation failed");
        }
        p4snd_voice_set_category(layer->voice, category);
        p4snd_voice_set_rate(layer->voice, info.rate);
        p4snd_voice_set_gain(layer->voice, 0, plan.gains[i], info.channels == 1 ? plan.gains[i] : 0);
        if (info.channels == 2) p4snd_voice_set_gain(layer->voice, 1, 0, plan.gains[i]);
        p4snd_stream_set_delay(layer->voice, plan.starts[i]);
    }
    if (p4snd_golden_cue_update(p)) {
        p4snd_golden_cue_destroy(p);
        return refuse(reason, capacity, "HCA packet decode failed");
    }
    if (!p->count && p->frames) p4snd_open(); /* timed event program, no invented waveform */
    p->origin = play_layers(p, 1);
    if (p->category_mask) { p->next = category_cues; category_cues = p; }
    *out = p;
    return 0;
}
int p4snd_golden_awb_voice_start(P4SndGoldenCue **out, const P4GoldenAwb *bank,
                                 uint32_t wave_id, char *reason, uint32_t capacity)
{
    if (p4_game_mode() != P4_GAME_MODE_GOLDEN) {
        if (out) *out = NULL;
        return refuse(reason, capacity, "Golden voice playback requires Golden mode");
    }
    if (!out) return -1;
    *out = NULL;
    const P4GoldenWave *wave = bank ? p4_golden_awb_find(bank, wave_id) : NULL;
    if (!wave) return refuse(reason, capacity, "Golden voice bank entry is missing");
    P4SndGoldenCue *p = calloc(1, sizeof *p + sizeof *p->layers);
    if (!p) return refuse(reason, capacity, "Golden audio allocation failed");
    p->count = 1;
    P4SndGoldenLayer *layer = &p->layers[0];
    if (p4_golden_adx_open(&layer->adx, wave->payload.bytes, wave->payload.size)) {
        p4snd_golden_cue_destroy(p);
        return refuse(reason, capacity, "invalid or unsupported Golden ADX voice");
    }
    const P4GoldenAdxInfo *info = p4_golden_adx_format(layer->adx);
    layer->channels = info->channels;
    layer->frames = info->frames;
    layer->voice = p4snd_stream_create((int)info->channels, 8192);
    if (!layer->voice) { p4snd_golden_cue_destroy(p); return refuse(reason, capacity, "Golden audio voice allocation failed"); }
    p4snd_voice_set_category(layer->voice, P4SND_CAT_VOICE);
    p4snd_voice_set_rate(layer->voice, info->rate);
    p4snd_voice_set_gain(layer->voice, 0, 1, info->channels == 1 ? 1 : 0);
    if (info->channels == 2) p4snd_voice_set_gain(layer->voice, 1, 0, 1);
    if (p4snd_golden_cue_update(p)) {
        p4snd_golden_cue_destroy(p);
        return refuse(reason, capacity, "Golden ADX packet decode failed");
    }
    play_layers(p, 1);
    *out = p;
    return 0;
}
static int update_layer(P4SndGoldenLayer *layer, int16_t *interleaved, int16_t *channel)
{
    while (!layer->eof && p4snd_stream_room(layer->voice)) {
        uint32_t n = p4snd_stream_room(layer->voice);
        uint32_t end = layer->loop ? layer->loop_end : layer->frames;
        if (n > 1024) n = 1024;
        if (n > end - layer->position) n = end - layer->position;
        if (!n) {
            if (!layer->loop) { layer->eof = 1; break; }
            if (p4_golden_hca_seek(layer->decoder, layer->loop_start)) return -1;
            layer->position = layer->loop_start;
            continue;
        }
        int got = layer->decoder ? p4_golden_hca_pull(layer->decoder, interleaved, n) :
                                   p4_golden_adx_pull(layer->adx, interleaved, n);
        if (got <= 0) return -1;
        for (uint32_t c = 0; c < layer->channels; ++c) {
            for (int i = 0; i < got; ++i) channel[i] = interleaved[(size_t)i * layer->channels + c];
            p4snd_stream_write(layer->voice, (int)c, 0, channel, (uint32_t)got);
        }
        p4snd_stream_commit(layer->voice, (uint32_t)got);
        layer->position += (uint32_t)got;
    }
    return 0;
}
int p4snd_golden_cue_update(P4SndGoldenCue *p)
{
    if (!p || p->failed) return -1;
    if (!p->count) return 0;
    int16_t interleaved[1024 * 2], channel[1024];
    for (uint32_t i = 0; i < p->count; ++i) {
        if (update_layer(&p->layers[i], interleaved, channel)) {
            p->failed = 1;
            /* A corrupt layer must not leave a partially playing cue behind. */
            play_layers(p, 0);
            for (uint32_t j = 0; j < p->count; ++j)
                p4snd_stream_flush(p->layers[j].voice);
            return -1;
        }
    }
    return 0;
}
int p4snd_golden_cue_done(const P4SndGoldenCue *p)
{
    if (!p || p->failed) return 0;
    if (p->frames && p4snd_frames_snapshot() - p->origin < p->frames) return 0;
    for (uint32_t i = 0; i < p->count; ++i)
        if (!p->layers[i].eof || p4snd_stream_data(p->layers[i].voice) ||
            p4snd_stream_delay(p->layers[i].voice)) return 0;
    return 1;
}
void p4snd_golden_cue_destroy(P4SndGoldenCue *p)
{
    if (!p) return;
    P4SndGoldenCue **link = &category_cues;
    while (*link && *link != p) link = &(*link)->next;
    if (*link) *link = p->next;
    for (uint32_t i = 0; i < p->count; ++i) {
        p4snd_voice_destroy(p->layers[i].voice);
        p4_golden_hca_close(p->layers[i].decoder);
        p4_golden_adx_close(p->layers[i].adx);
    }
    free(p);
}

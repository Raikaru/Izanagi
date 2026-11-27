/* Synthetic/owner-local qualification driver. Never prints keys or game payload. */
#include "p4pc/golden_audio.h"
#include "p4pc/game_mode.h"
#include "p4snd.h"
#include "../src/platform/config/p4cfg.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif

static uint8_t *read_file(const char *path, size_t *size)
{
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    if (fseek(f, 0, SEEK_END)) { fclose(f); return NULL; }
    long n = ftell(f);
    if (n < 0 || (unsigned long)n > (512u << 20)) { fclose(f); return NULL; }
    rewind(f);
    uint8_t *bytes = malloc(n ? (size_t)n : 1);
    if (!bytes || fread(bytes, 1, (size_t)n, f) != (size_t)n) { free(bytes); fclose(f); return NULL; }
    fclose(f); *size = (size_t)n; return bytes;
}
static int load_acf(const char *path, P4GoldenAcf *acf)
{
    size_t size;
    uint8_t *bytes = read_file(path, &size);
    if (!bytes) return -1;
    int result = p4_golden_acf_qualify(acf, bytes, size);
    free(bytes);
    if (result) fprintf(stderr, "A3: ACF configuration is outside the hash-qualified dry profile\n");
    return result;
}
static int print_plan(const P4GoldenAcb *acb, const P4GoldenCue *cue)
{
    P4GoldenCuePlan plan;
    char reason[192];
    int result = p4_golden_acb_plan(acb, cue, &plan, reason, sizeof reason);
    if (result) printf("{\"ok\":false,\"reason\":\"%s\"}", reason);
    else {
        printf("{\"ok\":true,\"frames\":%llu,\"count\":%u,\"category_mask\":%u,\"notes\":[",
               (unsigned long long)plan.frames, plan.count, plan.category_mask);
        for (uint32_t i = 0; i < plan.count; ++i)
            printf("%s{\"index\":%u,\"id\":%u,\"start_frame\":%u,\"gain\":%.9g}",
                   i ? "," : "", plan.waves[i].index, plan.waves[i].id, plan.starts[i], plan.gains[i]);
        printf("]}");
    }
    return result;
}
typedef struct Injected { uint32_t value, upper, calls; } Injected;
static int injected(void *context, uint32_t upper, uint32_t *value)
{
    Injected *draw = context;
    draw->upper = upper;
    ++draw->calls;
    *value = draw->value;
    return 0;
}
static int inspect(uint8_t *bytes, size_t size, int awb_only, const P4GoldenAcf *acf,
                   const P4GoldenAwb *stream, int qualify)
{
    P4GoldenAcb acb = {0};
    P4GoldenAwb awb = {0};
    if (awb_only ? p4_golden_awb_open(&awb, bytes, size) : p4_golden_acb_open(&acb, bytes, size)) return -1;
    if (!awb_only && acf && p4_golden_acb_set_dry_acf(&acb, acf)) goto bad;
    const P4GoldenAwb *bank = awb_only ? &awb : &acb.memory;
    unsigned hca = 0, other = 0;
    for (uint32_t i = 0; i < bank->count; ++i) {
        P4GoldenSpan span = bank->waves[i].payload;
        if (span.size >= 4 && (span.bytes[0] & 127) == 'H' && (span.bytes[1] & 127) == 'C' && (span.bytes[2] & 127) == 'A') {
            P4GoldenHcaInfo info;
            if (p4_golden_hca_info(span.bytes, span.size, &info)) goto bad;
            ++hca;
        } else ++other;
    }
    printf("{\"bank_waves\":%u,\"hca\":%u,\"other\":%u,\"tables\":[", bank->count, hca, other);
    /* StringValueTable is an auxiliary lookup; retain the established table census plus parameter tables. */
    for (unsigned i = 0; i < P4_ACB_STRING; ++i) printf("%s%u", i ? "," : "", acb.tables[i].row_count);
    printf("],\"cues\":[");
    if (!awb_only) {
        for (uint32_t row = 0; row < acb.tables[P4_ACB_CUE].row_count; ++row) {
            uint64_t id;
            P4GoldenCue cue;
            P4GoldenCueWave waves[P4_GOLDEN_CUE_MAX_WAVES];
            if (p4_golden_utf_u64(&acb.tables[P4_ACB_CUE], row, "CueId", &id) || id > UINT32_MAX ||
                p4_golden_acb_cue(&acb, (uint32_t)id, &cue)) goto bad;
            int count = p4_golden_acb_resolve(&acb, &cue, waves, P4_GOLDEN_CUE_MAX_WAVES);
            if (count < 0) goto bad;
            printf("%s{\"id\":%u,\"length_ms\":%u,\"waves\":[", row ? "," : "", cue.id, cue.length_ms);
            for (int j = 0; j < count; ++j) {
                P4GoldenCueWave *w = &waves[j];
                printf("%s{\"id\":%u,\"index\":%u,\"rate\":%u,\"channels\":%u,\"samples\":%u,\"codec\":%u,\"streaming\":%d,\"loop\":%d,\"port\":%u,\"loop_start\":%u,\"loop_end\":%u}",
                       j ? "," : "", w->id, w->index, w->rate, w->channels, w->samples, w->codec, w->streaming, w->loop, w->port, w->loop_start, w->loop_end);
            }
            printf("],\"playback\":");
            int planned = print_plan(&acb, &cue);
            if (qualify && !planned) {
                P4SndGoldenCue *playing = NULL;
                char reason[192];
                int started = p4snd_golden_cue_start(&playing, &acb, stream, cue.id, P4SND_CAT_EFFECTS, 0, reason, sizeof reason);
                printf(",\"start\":{\"ok\":%s", started ? "false" : "true");
                if (started) printf(",\"reason\":\"%s\"", reason);
                printf("}");
                p4snd_golden_cue_destroy(playing);
            }
            printf("}");
        }
    }
    printf("]}\n"); fflush(stdout);
    p4_golden_acb_close(&acb); p4_golden_awb_close(&awb); return 0;
bad:
    p4_golden_acb_close(&acb); p4_golden_awb_close(&awb); return -1;
}
static int word(FILE *f, uint32_t value, unsigned n)
{
    uint8_t b[4];
    for (unsigned i = 0; i < n; ++i) b[i] = (uint8_t)(value >> (i * 8));
    return fwrite(b, 1, n, f) == n ? 0 : -1;
}
static FILE *wav_open(const char *path, uint32_t frames, uint32_t channels, uint32_t rate)
{
    uint64_t n = (uint64_t)frames * channels * 2;
    if (n > UINT32_MAX - 36) return NULL;
    FILE *f = fopen(path, "wb");
    if (!f) return NULL;
    int bad = fwrite("RIFF", 1, 4, f) != 4 || word(f, (uint32_t)n + 36, 4) || fwrite("WAVEfmt ", 1, 8, f) != 8 ||
        word(f, 16, 4) || word(f, 1, 2) || word(f, channels, 2) || word(f, rate, 4) || word(f, rate * channels * 2, 4) ||
        word(f, channels * 2, 2) || word(f, 16, 2) || fwrite("data", 1, 4, f) != 4 || word(f, (uint32_t)n, 4);
    if (bad) { fclose(f); remove(path); return NULL; }
    return f;
}
static int decode(P4GoldenSpan span, uint16_t subkey, const char *path, uint32_t frames)
{
    P4GoldenHca *h = NULL;
    char reason[160]; uint64_t key = 0;
    P4GoldenHcaInfo info;
    if (p4_golden_hca_info(span.bytes, span.size, &info)) return -1;
    if (!frames) frames = info.frames;
    if (frames > info.frames) return -1;
    if (info.cipher == 56 && p4cfg_hca_key(&key) < 0) return -1;
    int result = p4_golden_hca_open(&h, span.bytes, span.size, key, subkey, reason, sizeof reason);
    *(volatile uint64_t *)&key = 0;
    if (result) { fprintf(stderr, "%s\n", reason); return result; }
    FILE *f = wav_open(path, frames, info.channels, info.rate);
    if (!f) { p4_golden_hca_close(h); return -1; }
    int16_t pcm[1024 * 8];
    uint32_t written = 0;
    double energy = 0;
    int got = 0;
    while (written < frames) {
        uint32_t count = frames - written; if (count > 1024) count = 1024;
        got = p4_golden_hca_pull(h, pcm, count);
        if (got <= 0) break;
        for (uint32_t i = 0; i < (uint32_t)got * info.channels; ++i) {
            energy += (double)pcm[i] * pcm[i];
            if (word(f, (uint16_t)pcm[i], 2)) { got = -1; break; }
        }
        if (got < 0) break;
        written += (uint32_t)got;
    }
    int bad = got < 0 || written != frames;
    if (fclose(f)) bad = 1;
    if (bad) remove(path);
    else printf("{\"frames\":%u,\"rate\":%u,\"channels\":%u,\"rms\":%.9g,\"seconds\":%.9g,\"cipher\":%u}\n",
                written, info.rate, info.channels, sqrt(energy / ((double)written * info.channels)), (double)written / info.rate, info.cipher);
    p4_golden_hca_close(h);
    return bad ? -1 : 0;
}
static int mix(const P4GoldenAcb *acb, const P4GoldenAwb *bank, uint32_t id, int category, const char *path, int loop)
{
    P4GoldenCue cue;
    P4GoldenCuePlan plan;
    char reason[192];
    if (p4_golden_acb_cue(acb, id, &cue)) return -1;
    if (p4_golden_acb_plan(acb, &cue, &plan, reason, sizeof reason)) {
        fprintf(stderr, "%s\n", reason);
        return -1;
    }
    uint64_t total = plan.frames;
    if (loop) {
        if (plan.count != 1) return -1;
        const P4GoldenCueWave *wave = &plan.waves[0];
        if (!wave->loop || !wave->loop_end) return -1;
        total = plan.starts[0] + (((uint64_t)wave->loop_end + 1024) * P4SND_RATE + wave->rate - 1) / wave->rate;
    }
    if (total > (UINT32_MAX - 36) / 4) return -1;
    uint32_t frames = (uint32_t)total;
    p4snd_use_offline();
    P4SndGoldenCue *p = NULL;
    reason[0] = 0;
    int result = p4snd_golden_cue_start(&p, acb, bank, id, category, loop, reason, sizeof reason);
    if (result) { fprintf(stderr, "%s\n", reason); p4snd_shutdown(); return result; }
    FILE *f = wav_open(path, frames, 2, P4SND_RATE);
    if (!f) { p4snd_golden_cue_destroy(p); p4snd_shutdown(); return -1; }
    double energy = 0;
    uint32_t done = 0;
    float output[240 * 2];
    while (done < frames) {
        unsigned n = frames - done; if (n > 240) n = 240;
        if (p4snd_golden_cue_update(p)) { result = -1; break; }
        p4snd_render(output, (int)n);
        for (unsigned i = 0; i < n * 2; ++i) {
            int value = (int)lrint(output[i] * 32767.0f);
            energy += (double)value * value;
            if (word(f, (uint16_t)value, 2)) result = -1;
        }
        done += n;
    }
    if (p4snd_golden_cue_update(p) || (loop ? p4snd_golden_cue_done(p) : !p4snd_golden_cue_done(p))) result = -1;
    if (fclose(f)) result = -1;
    if (result) remove(path);
    else printf("{\"frames\":%u,\"rate\":48000,\"channels\":2,\"rms\":%.9g,\"seconds\":%.9g}\n",
                frames, frames ? sqrt(energy / ((double)frames * 2)) : 0, (double)frames / 48000);
    p4snd_golden_cue_destroy(p); p4snd_shutdown();
    return result;
}
static uint32_t big32(const uint8_t *p)
{
    return (uint32_t)p[0] << 24 | (uint32_t)p[1] << 16 | (uint32_t)p[2] << 8 | p[3];
}
static int voice(const P4GoldenAwb *bank, uint32_t id, const char *path, int mixer)
{
    const P4GoldenWave *source = p4_golden_awb_find(bank, id);
    P4GoldenAdx *adx = NULL;
    P4SndGoldenCue *playing = NULL;
    char reason[160];
    if (!source || source->payload.size < 20) return -1;
    if (mixer) {
        p4snd_use_offline();
        if (p4snd_golden_awb_voice_start(&playing, bank, id, reason, sizeof reason)) {
            fprintf(stderr, "%s\n", reason); p4snd_shutdown(); return -1;
        }
    } else if (p4_golden_adx_open(&adx, source->payload.bytes, source->payload.size)) return -1;
    uint32_t input_rate = big32(source->payload.bytes + 8), original = big32(source->payload.bytes + 12);
    uint32_t rate = mixer ? P4SND_RATE : input_rate;
    uint32_t channels = mixer ? 2 : source->payload.bytes[7];
    uint32_t frames = mixer ? (uint32_t)(((uint64_t)original * rate + input_rate - 1) / input_rate) : original;
    FILE *f = wav_open(path, frames, channels, rate);
    int result = f ? 0 : -1;
    uint32_t done = 0;
    double energy = 0;
    int16_t pcm[240 * 2];
    float mixed[240 * 2];
    while (!result && done < frames) {
        uint32_t n = frames - done; if (n > 240) n = 240;
        if (mixer) {
            if (p4snd_golden_cue_update(playing)) { result = -1; break; }
            p4snd_render(mixed, (int)n);
            for (uint32_t i = 0; i < n * 2; ++i) pcm[i] = (int16_t)lrint(mixed[i] * 32767.0f);
        } else if (p4_golden_adx_pull(adx, pcm, n) != (int)n) { result = -1; break; }
        for (uint32_t i = 0; i < n * channels; ++i) {
            energy += (double)pcm[i] * pcm[i];
            if (word(f, (uint16_t)pcm[i], 2)) result = -1;
        }
        done += n;
    }
    if (mixer && (p4snd_golden_cue_update(playing) || !p4snd_golden_cue_done(playing))) result = -1;
    if (f && fclose(f)) result = -1;
    if (result) remove(path);
    else printf("{\"codec\":\"adx\",\"frames\":%u,\"original_frames\":%u,\"rate\":%u,\"channels\":%u,\"rms\":%.9g,\"seconds\":%.9g}\n",
                frames, original, rate, channels, sqrt(energy / ((double)frames * channels)), (double)frames / rate);
    p4_golden_adx_close(adx);
    p4snd_golden_cue_destroy(playing);
    if (mixer) p4snd_shutdown();
    return result;
}
static int group_mix(uint32_t delay)
{
    P4SndVoice *voices[2] = {0};
    int16_t pcm[32];
    float output[16 * 2];
    int result = -1;
    p4snd_use_offline();
    for (unsigned i = 0; i < 32; ++i) pcm[i] = 4096;
    for (unsigned i = 0; i < 2; ++i) {
        voices[i] = p4snd_stream_create(1, 64);
        if (!voices[i]) goto done;
        p4snd_voice_set_rate(voices[i], P4SND_RATE);
        p4snd_voice_set_gain(voices[i], 0, 1, 1);
        p4snd_stream_write(voices[i], 0, 0, pcm, 32);
        p4snd_stream_commit(voices[i], 32);
    }
    p4snd_stream_set_delay(voices[1], delay);
    p4snd_voices_play(voices, 2, 1);
    p4snd_render(output, 16);
    for (unsigned i = 0; i < 16; ++i) {
        float expected = (float)i / 128 + (i < delay ? 0 : (float)(i - delay) / (8 * (16 - delay)));
        if (output[i * 2] != expected || output[i * 2 + 1] != output[i * 2]) goto done;
    }
    p4snd_voices_play(voices, 2, 0);
    p4snd_render(output, 16);
    for (unsigned i = 0; i < 32; ++i) if (output[i] != 0) goto done;
    for (unsigned i = 0; i < 2; ++i) {
        uint32_t consumed = 16 - (i ? delay : 0);
        if (p4snd_stream_consumed(voices[i]) != consumed || p4snd_stream_data(voices[i]) != 32 - consumed) goto done;
    }
    p4snd_voices_play(voices, 2, 1);
    p4snd_render(output, 16);
    for (unsigned i = 0; i < 32; ++i) if (output[i] != 0.25f) goto done;
    if (delay) {
        if (p4snd_stream_data(voices[0]) || p4snd_stream_data(voices[1]) != delay) goto done;
        p4snd_render(output, (int)delay);
        for (unsigned i = 0; i < delay * 2; ++i) if (output[i] != 0.125f) goto done;
    }
    for (unsigned i = 0; i < 2; ++i)
        if (p4snd_stream_data(voices[i]) || p4snd_stream_delay(voices[i])) goto done;
    printf("{\"group_voices\":2,\"frames_per_voice\":32,\"delay_frames\":%u}\n", delay);
    result = 0;
done:
    for (unsigned i = 0; i < 2; ++i) p4snd_voice_destroy(voices[i]);
    p4snd_shutdown();
    return result;
}
int main(int argc, char **argv)
{
    if (argc == 2 && !strcmp(argv[1], "--group-mix")) return group_mix(0) ? 1 : 0;
    if (argc == 2 && !strcmp(argv[1], "--group-delay")) return group_mix(8) ? 1 : 0;
    if (argc == 2 && !strcmp(argv[1], "--ps2")) {
        P4SndGoldenCue *out = (void *)1;
        if (p4_game_mode() != P4_GAME_MODE_PS2 ||
            p4snd_golden_cue_start(&out, (const P4GoldenAcb *)(uintptr_t)1, (const P4GoldenAwb *)(uintptr_t)1, 0, 0, 0, NULL, 0) != -1 || out)
            return 1;
        out = (void *)1;
        if (p4snd_golden_awb_voice_start(&out, (const P4GoldenAwb *)(uintptr_t)1, 0, NULL, 0) != -1 || out) return 1;
        puts("PS2: Golden audio not opened"); return 0;
    }
    p4cfg_load();
    if (argc == 2 && (!strcmp(argv[1], "--batch") || !strcmp(argv[1], "--batch-qualify"))) {
#ifdef _WIN32
        _setmode(_fileno(stdin), _O_BINARY);
#endif
        int qualify = !strcmp(argv[1], "--batch-qualify");
        P4GoldenAcf acf = {0};
        P4GoldenAwb stream = {0};
        uint8_t *stream_bytes = NULL;
        if (qualify) p4snd_use_offline();
        uint8_t header[5];
        while (fread(header, 1, 5, stdin) == 5) {
            uint32_t size = (uint32_t)header[1] | (uint32_t)header[2] << 8 | (uint32_t)header[3] << 16 | (uint32_t)header[4] << 24;
            if (size > (512u << 20)) return 1;
            uint8_t *bytes = malloc(size ? size : 1);
            if (!bytes || fread(bytes, 1, size, stdin) != size) { free(bytes); return 1; }
            int result = 0;
            if (header[0] == 'F') {
                result = p4_golden_acf_qualify(&acf, bytes, size);
                if (!result) printf("{\"acf_profile\":%u}\n", acf.profile);
            } else if (header[0] == 'B') {
                p4_golden_awb_close(&stream); free(stream_bytes); stream_bytes = NULL;
                if (size) {
                    result = p4_golden_awb_open(&stream, bytes, size);
                    if (!result) { stream_bytes = bytes; bytes = NULL; }
                }
                if (!result) printf("{\"stream_bank_waves\":%u}\n", stream.count);
            } else result = inspect(bytes, size, header[0] == 'W', acf.profile ? &acf : NULL, &stream, qualify);
            free(bytes);
            if (result) puts("{\"error\":\"audio/configuration parse failed\"}");
            fflush(stdout);
        }
        p4_golden_awb_close(&stream); free(stream_bytes);
        if (qualify) p4snd_shutdown();
        return ferror(stdin) ? 1 : 0;
    }
    if (argc < 3) return 2;
    size_t size; uint8_t *bytes = read_file(argv[2], &size);
    if (!bytes) return 3;
    int result = -1;
    if (!strcmp(argv[1], "--acf-check")) {
        P4GoldenAcf acf;
        result = p4_golden_acf_qualify(&acf, bytes, size);
        if (!result) printf("{\"acf_profile\":%u}\n", acf.profile);
    } else if (!strcmp(argv[1], "--inspect") || !strcmp(argv[1], "--awb")) {
        P4GoldenAcf acf = {0};
        if (argc == 3 || (argc == 4 && !load_acf(argv[3], &acf)))
            result = inspect(bytes, size, !strcmp(argv[1], "--awb"), acf.profile ? &acf : NULL, NULL, 0);
    }
    else if (!strcmp(argv[1], "--decode") && argc == 4) result = decode((P4GoldenSpan){bytes, size}, 0, argv[3], 0);
    else if (!strcmp(argv[1], "--plan") && argc >= 5) {
        P4GoldenAcb acb;
        if (!p4_golden_acb_open(&acb, bytes, size)) {
            P4GoldenCue cue;
            Injected draw = {0};
            uint32_t default_id = (uint32_t)strtoul(argv[3], NULL, 10);
            if (!p4_golden_acb_cue(&acb, default_id, &cue) &&
                !p4_golden_acb_set_random(&acb, injected, &draw)) {
                printf("{\"plans\":[");
                for (int i = 4; i < argc; ++i) {
                    char *separator;
                    uint32_t first = (uint32_t)strtoul(argv[i], &separator, 10);
                    uint32_t id = *separator == ':' ? first : default_id;
                    draw.value = *separator == ':' ? (uint32_t)strtoul(separator + 1, NULL, 10) : first;
                    if (i > 4) printf(",");
                    if (p4_golden_acb_cue(&acb, id, &cue)) printf("{\"ok\":false,\"reason\":\"A8: invalid selected cue ID\"}");
                    else print_plan(&acb, &cue);
                }
                printf("],\"last_upper\":%u,\"draw_calls\":%u}\n", draw.upper, draw.calls);
                result = 0;
            }
            p4_golden_acb_close(&acb);
        }
    }
    else if ((!strcmp(argv[1], "--voice-decode") || !strcmp(argv[1], "--voice-mix")) && argc == 5) {
        P4GoldenAwb bank;
        if (!p4_golden_awb_open(&bank, bytes, size)) {
            result = voice(&bank, (uint32_t)strtoul(argv[3], NULL, 10), argv[4], !strcmp(argv[1], "--voice-mix"));
            p4_golden_awb_close(&bank);
        }
    }
    else if ((!strcmp(argv[1], "--cue-decode") || !strcmp(argv[1], "--mix") || !strcmp(argv[1], "--mix-loop")) && (argc == 7 || argc == 8)) {
        P4GoldenAcb acb;
        P4GoldenAwb bank = {0};
        uint8_t *bank_bytes = NULL;
        if (!p4_golden_acb_open(&acb, bytes, size)) {
            size_t bank_size;
            P4GoldenAcf acf = {0};
            int configured = argc == 7 || (!load_acf(argv[7], &acf) && !p4_golden_acb_set_dry_acf(&acb, &acf));
            if (configured && strcmp(argv[3], "-")) bank_bytes = read_file(argv[3], &bank_size);
            if (!configured || (strcmp(argv[3], "-") && (!bank_bytes || p4_golden_awb_open(&bank, bank_bytes, bank_size)))) result = -1;
            else {
                uint32_t id = (uint32_t)strtoul(argv[4], NULL, 10);
                if (strcmp(argv[1], "--cue-decode")) result = mix(&acb, &bank, id, atoi(argv[5]), argv[6], !strcmp(argv[1], "--mix-loop"));
                else {
                    P4GoldenCue cue; P4GoldenCueWave w;
                    if (!p4_golden_acb_cue(&acb, id, &cue) && p4_golden_acb_resolve(&acb, &cue, &w, 1) == 1) {
                        const P4GoldenAwb *b = w.streaming ? &bank : &acb.memory;
                        const P4GoldenWave *source = p4_golden_awb_find(b, w.id);
                        if (source) result = decode(source->payload, b->subkey, argv[6], w.samples);
                    }
                }
            }
            p4_golden_acb_close(&acb); p4_golden_awb_close(&bank); free(bank_bytes);
        }
    }
    free(bytes);
    if (result) fprintf(stderr, "Golden audio operation refused (%d)\n", result);
    return result ? 1 : 0;
}

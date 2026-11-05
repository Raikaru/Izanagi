/* aixp_wav_test: an ADX file from the retail disc, wrapped as a one track AIX, through the translated AIX
 * player (src/cri/aixp.c) and the game's CRI decoder to a WAV file.
 *
 * The disc has no AIX files and the game never creates an AIX player (its ACSSND parameter blocks all select
 * the ADXT kind), so the test builds the container from an ADX: an "AIXF" header (one track, the ADX's
 * rate and channel count, the "(c)CRI" mark) and "AIXP" packets of 4096 bytes of the ADX stream, ended by
 * an "AIXE" packet. The header is 0x1800 bytes: the demultiplexer asks its input for 0x1800 bytes and accepts
 * the header only when the chunk it gets is exactly the header's size. aixp_Create makes the player (and its
 * ADXT track), func_004decb8 starts it on the memory image, and every tick runs the AIXP server (func_004de2e8: the player state machine and the
 * AIXSJD demultiplexer that parses the header and fills the track's ring) and ADXT_ExecServer. The
 * track's PCM leaves through AXRNA into the mixer like aixp_wav_test's.
 *
 * Checks, as aixp_wav_test: the played time matches the header, the waveform is neither silent nor clipped,
 * the player reaches its end state (3), and every output frame between the 2000th and the 2000th from the
 * end equals the reference decode of the ADX resampled the way the mixer resamples, within 2 LSB: the
 * demultiplexing neither drops, repeats nor reorders data.
 *
 * The ADX is P4PC_ADX (a disc path) or BGM/THEME.ADX. Exits 77 (CTest skip) when P4PC_ISO or P4PC_ELF is
 * unset. */
#define _FILE_OFFSET_BITS 64
#define _POSIX_C_SOURCE 200809L

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "p4pc/gmem.h"
#include "p4pc/image.h"
#include "p4pc/os.h"
#include "p4pc/platform.h"
#include "guest_test.h"
#include "p4snd.h"
#include "vfs.h"

#define SKIP_RETURN 77
#define TICK_FRAMES 800 /* 1/60 s at 48 kHz */
#define ADXT_STAT_PLAYING 3
#define ADXT_STAT_PLAYEND 5
#define EDGE 2000

typedef struct {
    uint8_t *data;
    int32_t len;
} SjCk;
typedef struct SjObj SjObj;
typedef struct {
    void *qi, *addref, *release;
    void (*destroy)(SjObj *);
    const void *(*uuid)(SjObj *);
    void (*reset)(SjObj *);
    void (*get_chunk)(SjObj *, int32_t, int32_t, SjCk *);
    void (*unget_chunk)(SjObj *, int32_t, SjCk *);
    void (*put_chunk)(SjObj *, int32_t, SjCk *);
    int32_t (*num_data)(SjObj *, int32_t);
} SjIf;
struct SjObj {
    const SjIf *vtbl;
};

void ADXT_Init(void);
void ADXT_Finish(void);
void *ADXT_Create(int32_t maxnch, void *work, int32_t worksize);
void ADXT_Destroy(void *adxt);
void ADXT_StartSj(void *adxt, SjObj *sj);
void ADXT_SetLpFlg(void *adxt, int32_t flg);
void ADXT_ExecServer(void);
int32_t ADXT_GetStat(void *adxt);
SjObj *SJMEM_Create(void *buf, int32_t bsize);
void func_004de340(void);                                  /* AIXP_Init */
void *aixp_Create(int32_t tracks, int32_t channels, void *work, int32_t size);
void func_004de860(void *aixp);                            /* AIXP_Destroy */
void func_004decb8(void *aixp, void *data, int32_t size);  /* start on a memory image */
int32_t func_004de2e8(int32_t);                            /* AIXP_ExecServer */
#define AIXP_STAT_END 3
void *kwlnProbeHeapBudget(void);

static int failures;

static void fail(const char *what)
{
    fprintf(stderr, "aixp_wav_test: FAIL %s\n", what);
    failures++;
}

static uint32_t be32(const uint8_t *p)
{
    return (uint32_t)p[0] << 24 | (uint32_t)p[1] << 16 | (uint32_t)p[2] << 8 | p[3];
}

static uint32_t be16(const uint8_t *p)
{
    return (uint32_t)p[0] << 8 | p[1];
}

static uint8_t *read_disc_file(const char *path, long long *size)
{
    int fd = vfs_open(path, 0);
    long long end;
    uint8_t *buf;
    long long got = 0;

    if (fd < 0) {
        return NULL;
    }
    end = vfs_seek(fd, 0, 2);
    vfs_seek(fd, 0, 0);
    buf = p4pc_heap_malloc((size_t)end);   /* the file is read by guest address (SJMEM): game heap */
    while (buf != NULL && got < end) {
        int n = vfs_read(fd, buf + got, (int)(end - got > (1 << 20) ? (1 << 20) : end - got));

        if (n <= 0) {
            break;
        }
        got += n;
    }
    vfs_close(fd);
    if (got != end) {
        p4pc_heap_free(buf);
        return NULL;
    }
    *size = end;
    return buf;
}

/* Reference ADX type 3 decode of channel ch: total samples, 32 per 18-byte frame. */
static int16_t *ref_decode(const uint8_t *adx, long long size, int ch, int nch, uint32_t total,
                           uint32_t rate)
{
    const uint32_t hp = be16(adx + 16);
    /* the low-pass coefficients in single precision, as the retail ADX_GetCoefficient computes them */
    const float a = sqrtf(2.0f) - cosf((float)hp * 6.2831855f / (float)rate);
    const float b = sqrtf(2.0f) - 1.0f;
    const float c = (a - sqrtf((a + b) * (a - b))) / b;
    const int32_t c1 = (int16_t)(int32_t)((c + c) * 4096.0f);
    const int32_t c2 = (int16_t)(int32_t)(-c * c * 4096.0f);
    const uint8_t *p = adx + be16(adx + 2) + 4;
    int16_t *out = calloc(total + 64, sizeof(int16_t));
    int32_t h1 = 0, h2 = 0;
    uint32_t n = 0;

    while (out != NULL && n < total && p + 18 * nch <= adx + size) {
        const uint8_t *f = p + 18 * ch;
        const int32_t scale = (int32_t)(be16(f) & 0x1fff) + 1;
        int i;

        for (i = 0; i < 32 && n < total; i++, n++) {
            int32_t nib = (i & 1) ? (f[2 + i / 2] & 15) : (f[2 + i / 2] >> 4);
            int32_t s;

            if (nib >= 8) {
                nib -= 16;
            }
            s = nib * scale + ((c1 * h1 + c2 * h2) >> 12);
            s = s > 32767 ? 32767 : (s < -32768 ? -32768 : s);
            out[n] = (int16_t)s;
            h2 = h1;
            h1 = s;
        }
        p += 18 * nch;
    }
    return out;
}

static void put32(FILE *f, uint32_t v)
{
    fputc((int)(v & 255), f);
    fputc((int)(v >> 8 & 255), f);
    fputc((int)(v >> 16 & 255), f);
    fputc((int)(v >> 24 & 255), f);
}

static void put16(FILE *f, uint32_t v)
{
    fputc((int)(v & 255), f);
    fputc((int)(v >> 8 & 255), f);
}

static int write_wav(const char *path, const int16_t *pcm, uint32_t frames)
{
    FILE *f = fopen(path, "wb");

    if (f == NULL) {
        return -1;
    }
    fwrite("RIFF", 1, 4, f);
    put32(f, 36 + frames * 4);
    fwrite("WAVEfmt ", 1, 8, f);
    put32(f, 16);
    put16(f, 1);
    put16(f, 2);
    put32(f, P4SND_RATE);
    put32(f, P4SND_RATE * 4);
    put16(f, 4);
    put16(f, 16);
    fwrite("data", 1, 4, f);
    put32(f, frames * 4);
    fwrite(pcm, 4, frames, f);
    fclose(f);
    return 0;
}

static int test_main(int argc, char **argv)
{
    const char *iso = getenv("P4PC_ISO");
    const char *elf = getenv("P4PC_ELF");
    const char *wavPath = argc > 1 ? argv[1] : "aixp_wav_test.wav";
    const char *adxPath = getenv("P4PC_ADX") ? getenv("P4PC_ADX") : "BGM/THEME.ADX";
    long long adxSize = 0;
    uint8_t *adx;
    uint32_t rate, total;
    int nch;
    void *work;
    void *aixp;
    uint8_t *aix;
    int32_t aixLen;
    int sawEnd = 0, i;
    int16_t *pcm = NULL;
    float *blk;
    uint32_t cap, frames = 0;
    long long startFrame = -1, endFrame = -1;
    int32_t stat = 0;
    int ticks = 0;

    if (iso == NULL || iso[0] == '\0' || elf == NULL || elf[0] == '\0') {
        fprintf(stderr, "aixp_wav_test: P4PC_ISO or P4PC_ELF unset, skipped\n");
        return SKIP_RETURN;
    }
    p4_os_setenv("P4PC_TRAP_CONTINUE", "1", 0);
    p4pc_trap_configure();
    p4pc_load_image(elf);
    p4pc_apply_patches();
    p4pc_kernel_pre_init();
    kwlnProbeHeapBudget();
    if (vfs_mount(iso, NULL) != 0) {
        fprintf(stderr, "aixp_wav_test: cannot mount %s\n", iso);
        return 1;
    }
    adx = read_disc_file(adxPath, &adxSize);
    if (adx == NULL) {
        fprintf(stderr, "aixp_wav_test: cannot read %s\n", adxPath);
        return 1;
    }

    /* ADX header: 0x8000, copyright offset, encoding 3, 18-byte frames of 4-bit samples. */
    if (adx[0] != 0x80 || adx[1] != 0x00 || adx[4] != 3 || adx[5] != 18 || adx[6] != 4) {
        fail("not a 4-bit ADX type 3");
        return 1;
    }
    nch = adx[7];
    rate = be32(adx + 8);
    total = be32(adx + 12);
    fprintf(stderr, "aixp_wav_test: %s, %d channel(s), %u Hz, %u samples (%.2f s), %lld bytes\n", adxPath,
            nch, rate, total, (double)total / rate, adxSize);
    if (nch < 1 || nch > 2 || rate < 8000 || rate > 48000 || total == 0) {
        fail("header values out of range");
        return 1;
    }

    p4snd_use_offline();
    ADXT_Init();
    func_004de340();
    {
        /* the AIX file: header 0x1800 bytes, 4096 byte packets of the ADX, an end packet */
        const uint32_t payload = 4096, hlen = 0x1800;
        uint32_t npk = (uint32_t)((adxSize + payload - 1) / payload), k;
        long long aixSize = (long long)hlen + (long long)npk * (payload + 16) + 16;
        uint8_t *o;

        aix = p4pc_heap_calloc(1, (size_t)aixSize);   /* the memory image the player reads: game heap */
        o = aix;
        memcpy(o, "AIXF", 4);
        o[4] = (uint8_t)((hlen - 8) >> 24); o[5] = (uint8_t)((hlen - 8) >> 16);
        o[6] = (uint8_t)((hlen - 8) >> 8); o[7] = (uint8_t)(hlen - 8);
        o[0x18] = 0; o[0x19] = 1;                       /* one phrase */
        memcpy(o + 0x20, "\0\0\0\0\0\0\0\0", 8);
        o[0x38] = 1;                                    /* one track at 0x38 */
        o[0x40] = (uint8_t)(rate >> 24); o[0x41] = (uint8_t)(rate >> 16); o[0x42] = (uint8_t)(rate >> 8); o[0x43] = (uint8_t)rate;
        o[0x44] = (uint8_t)nch;
        memcpy(o + hlen - strlen((const char *)P4_G2H(0x00759c48)), (const char *)P4_G2H(0x00759c48), strlen((const char *)P4_G2H(0x00759c48)));
        o += hlen;
        for (k = 0; k < npk; k++) {
            uint32_t n = (uint32_t)(adxSize - (long long)k * payload);

            if (n > payload) {
                n = payload;
            }
            memcpy(o, "AIXP", 4);
            o[4] = 0; o[5] = 0; o[6] = (uint8_t)((n + 8) >> 8); o[7] = (uint8_t)(n + 8);
            o[8] = 0;                                   /* track 0 */
            o[10] = (uint8_t)(n >> 8); o[11] = (uint8_t)n;
            memcpy(o + 16, adx + (size_t)k * payload, n);
            o += 16 + n;
        }
        memcpy(o, "AIXE", 4);
        o[7] = 8;
        o += 16;
        aixSize = o - aix;
        aixLen = (int32_t)aixSize;
    }
    work = p4pc_heap_calloc(1, 0x100000);
    aixp = aixp_Create(1, 2, work, 0x100000);
    if (aixp == NULL) {
        fail("aixp_Create");
        return 1;
    }
    ADXT_SetLpFlg(P4_G2H(*(uint32_t *)((uint8_t *)aixp + 0xc)), 0);   /* the track's ADXT: a guest pointer field */
    func_004decb8(aixp, aix, aixLen);

    cap = (uint32_t)((double)total * P4SND_RATE / rate) + 20u * TICK_FRAMES;
    pcm = calloc((size_t)cap * 2, sizeof(int16_t));
    blk = malloc(TICK_FRAMES * 2 * sizeof(float));
    while (ticks < 60 * (int)(total / rate + 30) && frames + TICK_FRAMES <= cap) {
        int i;

        func_004de2e8(0);
        ADXT_ExecServer();
        stat = ((uint8_t *)aixp)[1];
        if (stat == AIXP_STAT_END) {
            sawEnd = 1;
        }
        stat = ADXT_GetStat(P4_G2H(*(uint32_t *)((uint8_t *)aixp + 0xc)));
        if (startFrame < 0 && stat == ADXT_STAT_PLAYING) {
            startFrame = (long long)frames;
        }
        if (stat == ADXT_STAT_PLAYEND) {
            endFrame = (long long)frames;
            break;
        }
        p4snd_render(blk, TICK_FRAMES);
        for (i = 0; i < TICK_FRAMES * 2; i++) {
            float v = blk[i] * 32768.0f;

            pcm[(size_t)frames * 2 + (size_t)i] = (int16_t)(v > 32767.0f ? 32767.0f : (v < -32768.0f ? -32768.0f : v));
        }
        frames += TICK_FRAMES;
        ticks++;
    }
    if (startFrame < 0 || endFrame < 0) {
        fprintf(stderr, "aixp_wav_test: ADXT stat %d after %d ticks\n", stat, ticks);
        fail("ADXT did not play to the end");
        return 1;
    }
    for (i = 0; i < 10 && !sawEnd; i++) {
        func_004de2e8(0);
        ADXT_ExecServer();
        sawEnd = ((uint8_t *)aixp)[1] == AIXP_STAT_END;
    }
    if (!sawEnd) {
        fail("the AIX player never reached its end state");
    }
    if (write_wav(wavPath, pcm, frames) != 0) {
        fail("write WAV");
    }

    /* duration: from the first frame the voice played to the end state */
    {
        double played = (double)(endFrame - startFrame) / P4SND_RATE;
        double want = (double)total / rate;

        fprintf(stderr, "aixp_wav_test: played %.3f s, header %.3f s\n", played, want);
        if (fabs(played - want) > 0.1) {
            fail("played time differs from the header duration");
        }
    }

    /* not silent, not clipped */
    {
        long long clipped = 0;
        double energy = 0.0;
        int peak = 0;
        long long i;

        for (i = startFrame * 2; i < endFrame * 2; i++) {
            int v = pcm[i] < 0 ? -pcm[i] : pcm[i];

            energy += (double)pcm[i] * pcm[i];
            if (v > peak) {
                peak = v;
            }
            if (v >= 32767) {
                clipped++;
            }
        }
        energy = sqrt(energy / (double)((endFrame - startFrame) * 2));
        fprintf(stderr, "aixp_wav_test: peak %d, rms %.1f, clipped %lld\n", peak, energy, clipped);
        if (energy < 100.0) {
            fail("waveform is silent");
        }
        /* mastered music touches full scale now and then (200 of 8.7 million samples here); a gain or
         * wrap error puts far more than 0.01% of the samples on the rails */
        if (clipped * 10000 > (endFrame - startFrame) * 2) {
            fail("waveform clips");
        }
    }

    /* sample-exact against the reference decode through the mixer's resampler */
    {
        int16_t *ref[2] = {NULL, NULL};
        int ch;
        const double step = (double)rate / P4SND_RATE;
        long long worst = 0;

        for (ch = 0; ch < nch; ch++) {
            ref[ch] = ref_decode(adx, adxSize, ch, nch, total, rate);
            if (ref[ch] == NULL) {
                fail("reference decode");
                return 1;
            }
        }
        {
            /* the player starts the track a tick or two before the first sample is rendered (the ADXT stat
             * leaves PREP when the demultiplexer has handed it the header): find the offset between the
             * stat-based start and the first sample by correlation, and require it to be small */
            long long lag, bestlag = 0;
            double best = -1e300;

            for (lag = -3 * TICK_FRAMES; lag <= 3 * TICK_FRAMES; lag++) {
                double c = 0.0;
                long long j;

                for (j = 0; j < 4000; j++) {
                    c += (double)pcm[(startFrame + EDGE + 5000 + j) * 2] * ref[0][EDGE + 5000 + j + lag];
                }
                if (c > best) {
                    best = c;
                    bestlag = lag;
                }
            }
            fprintf(stderr, "aixp_wav_test: the first sample is rendered %lld frames after the stat reached playing\n", -bestlag);
            if (bestlag > 0 || bestlag < -3 * TICK_FRAMES + 1) {
                fail("the stream is not where the player was started");
            }
            startFrame -= bestlag;
        }
        for (ch = 0; ch < 2; ch++) {
            const int16_t *r = ref[nch == 1 ? 0 : ch];
            /* the mixer gain of the stream; fitted over the first second after the ramp-in */
            double num = 0.0, den = 0.0, gain;
            uint32_t idx = 0;
            double frac = 0.0;
            long long j;
            /* the voice's own frames: the end state comes the renderer's report latency later */
            long long n = endFrame - startFrame < (long long)((double)(total - 1) / step) ?
                          endFrame - startFrame : (long long)((double)(total - 1) / step);
            double err;

            for (j = 0; j < n; j++) {
                double s = (double)r[idx] + ((double)r[idx + 1] - (double)r[idx]) * (float)frac;

                if (j >= EDGE && j < EDGE + 48000) {
                    num += s * pcm[(startFrame + j) * 2 + ch];
                    den += s * s;
                }
                frac += step;
                while (frac >= 1.0) {
                    frac -= 1.0;
                    idx++;
                }
            }
            gain = den > 0.0 ? num / den : 0.0;
            if (nch == 2 && (gain < 0.2 || gain > 1.01)) {
                fprintf(stderr, "aixp_wav_test: channel %d gain %.4f\n", ch, gain);
                fail("stream gain out of range");
            }
            idx = 0;
            frac = 0.0;
            for (j = 0; j < n; j++) {
                double s = ((double)r[idx] + ((double)r[idx + 1] - (double)r[idx]) * (float)frac) * gain;

                if (j >= EDGE && j < n - EDGE) {
                    err = fabs(s - (double)pcm[(startFrame + j) * 2 + ch]);
                    if (err > (double)worst) {
                        worst = (long long)ceil(err);
                    }
                }
                frac += step;
                while (frac >= 1.0) {
                    frac -= 1.0;
                    idx++;
                }
            }
            fprintf(stderr, "aixp_wav_test: channel %d gain %.4f\n", ch, gain);
        }
        fprintf(stderr, "aixp_wav_test: worst deviation from the reference %lld LSB\n", worst);
        if (worst > 2) {
            fail("output deviates from the reference decode");
        }
        for (ch = 0; ch < nch; ch++) {
            free(ref[ch]);
        }
    }

    func_004de860(aixp);
    free(pcm);
    free(blk);
    p4pc_heap_free(work);
    p4pc_heap_free(adx);
    p4snd_shutdown();
    if (failures != 0) {
        return 1;
    }
    printf("aixp_wav_test: ok, %s -> %s\n", adxPath, wavPath);
    return 0;
}

int main(int argc, char **argv)
{
    return p4t_on_guest_stack(test_main, argc, argv);
}

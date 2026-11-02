/* adx_wav_test: an ADX file from the retail disc through the game's CRI decoder to a WAV file.
 *
 * ADXT (the game's own CRI code: stream joints, ADXSJD decoder, ADXRNA) plays an ADX held in a
 * memory joint. Its PCM leaves through AXRNA (src/platform/sound/axrna.c) into the mixer, which
 * the test pulls offline 800 frames (1/60 s) per server tick and writes as a 16-bit stereo 48 kHz
 * WAV (argv[1], default adx_wav_test.wav).
 *
 * Checks, against the ADX header and an independent reference decoder in this file:
 *   - the header is a plain 4-bit ADX (type 3) with the rate, channel count and sample total
 *     the check compares against;
 *   - ADXT reaches the playing state and then the end, and the played time (from the first
 *     rendered frame of the voice to the end state) matches total / rate within 100 ms;
 *   - the waveform is not silent and does not clip (no more than 0.01% of the samples on the rails);
 *   - every output frame between the 2000th and the 2000th from the end equals the reference
 *     decode resampled the way the mixer resamples (linear, stream rate to 48 kHz), within 2
 *     LSB at the stream's gain: no dropped, repeated or reordered samples on the way.
 *
 * The ADX is P4PC_ADX (a disc path) or BGM/THEME.ADX, the title music. With P4PC_ADX_STREAM (a CRI
 * name such as "VOL:EVE_521.ADX" or "ENV:ENV_346.ADX") the file is not held in a memory joint but streamed
 * the way the game plays it: the CRI and ROFS set-up of kwlnInitSubsystems (CDV device, the ROFS
 * device and its four volumes), H_Cdvd_SelectDirectory(P4PC_ADX_DIR, e.g. "\\SOUND\\EVENT\\") for a file
 * of the main volume, ADXT_StartFname, and the file server passes next to ADXT_ExecServer. The same
 * checks then prove that the name resolves through the volume's directory list (no basename search),
 * is read through CVFS and ROFS, and reaches the mixer sample-exact. With P4PC_ADX_AFS_MEMBER too, P4PC_ADX
 * is an AFS archive of the disc, P4PC_ADX_STREAM its CRI name ("VOL:V101001.AFS"): the partition is loaded
 * with the directory list of \\SOUND\\, as the game loads its voice archives, and the member is played
 * with ADXT_StartAfs. Exits 77 (CTest skip) when P4PC_ISO or P4PC_ELF is unset. */
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
#include "p4pc/tables.h"
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
void ADXT_ExecFsSvr(void);
int32_t ADXT_GetStat(void *adxt);
int32_t ADXT_GetNumChan(void *adxt);
int32_t adxt_GetFmtBps(void *adxt);
int32_t ADXSTM_GetStat(void *stm);
void func_004d6fb8(void *adxt, const char *fname); /* ADXT_StartFname */
void func_004d32f8(int32_t *prm);                 /* CRI file system set-up: MFS and CDV devices */
int32_t func_004f34b0(void *config);              /* ROFS_Init */
int32_t func_004f5bf0(int32_t handle, int32_t option, int32_t value);
int32_t func_004c6eb0(void);                      /* ROFS device registration */
int32_t func_004c6f50(const char *name, const char *image, void *arg); /* attach a volume */
int32_t func_004f2798(void *volume, const void *dirlist);
void H_Cdvd_SelectDirectory(const char *dir);
int func_004c74f8(int ptid, const char *fname, void *dir, unsigned char *ptinfo); /* ADXF_LoadPartitionNw */
int func_004c7ef8(int ptid);                                                       /* ADXF_GetPtStat */
void func_004d6d68(void *adxt, int patid, int fid);                                /* ADXT_StartAfs */
SjObj *SJMEM_Create(void *buf, int32_t bsize);
void *kwlnProbeHeapBudget(void);

static int failures;

/* The directory list the game's table holds for `dir` of the main volume (names at `names`, lists at `blobs`). */
static const void *blob_of(uint32_t names, uint32_t blobs, const char *dir)
{
    int i;

    for (i = 0; i < 300; i++) {
        const char *name = (const char *)P4_G2H(P4_G32(names + 4u * (uint32_t)i));

        if (name[0] == '\0') {
            return NULL;
        }
        if (strcmp(name, dir) == 0) {
            return P4_G2H(P4_G32(blobs + 4u * (uint32_t)i));
        }
    }
    return NULL;
}

static void fail(const char *what)
{
    fprintf(stderr, "adx_wav_test: FAIL %s\n", what);
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
    const char *wavPath = argc > 1 ? argv[1] : "adx_wav_test.wav";
    const char *adxPath = getenv("P4PC_ADX") ? getenv("P4PC_ADX") : "BGM/THEME.ADX";
    const char *streamName = getenv("P4PC_ADX_STREAM");
    const char *streamDir = getenv("P4PC_ADX_DIR");
    int afsMember = getenv("P4PC_ADX_AFS_MEMBER") != NULL ? atoi(getenv("P4PC_ADX_AFS_MEMBER")) : -1;
    uint8_t *adxBase;
    long long adxSize = 0;
    uint8_t *adx;
    uint32_t rate, total;
    int nch;
    void *work;
    void *adxt;
    SjObj *sj;
    int16_t *pcm = NULL;
    float *blk;
    uint32_t cap, frames = 0;
    long long startFrame = -1, endFrame = -1;
    int32_t stat = 0;
    int ticks = 0;

    if (iso == NULL || iso[0] == '\0' || elf == NULL || elf[0] == '\0') {
        fprintf(stderr, "adx_wav_test: P4PC_ISO or P4PC_ELF unset, skipped\n");
        return SKIP_RETURN;
    }
    p4_os_setenv("P4PC_TRAP_CONTINUE", "1", 0);
    p4_os_setenv("P4PC_VIRTUAL_CLOCK", "1", 1); /* offline PCM ticks, not wall-clock streaming */
    p4pc_trap_configure();
    p4pc_load_image(elf);
    p4pc_apply_patches();
    p4pc_kernel_pre_init();
    kwlnProbeHeapBudget();
    if (vfs_mount(iso, NULL) != 0) {
        fprintf(stderr, "adx_wav_test: cannot mount %s\n", iso);
        return 1;
    }
    adx = read_disc_file(adxPath, &adxSize);
    if (adx == NULL) {
        fprintf(stderr, "adx_wav_test: cannot read %s\n", adxPath);
        return 1;
    }

    adxBase = adx;
    if (afsMember >= 0) {
        /* an AFS archive: "AFS\0", the member count, then (offset, size) pairs, little endian */
        const uint8_t *t = adx + 8 + 8 * afsMember;
        uint32_t count = adx[4] | adx[5] << 8 | adx[6] << 16 | (uint32_t)adx[7] << 24;
        uint32_t off = t[0] | t[1] << 8 | t[2] << 16 | (uint32_t)t[3] << 24;
        uint32_t sz = t[4] | t[5] << 8 | t[6] << 16 | (uint32_t)t[7] << 24;

        if (memcmp(adx, "AFS\0", 4) != 0 || (uint32_t)afsMember >= count) {
            fail("not an AFS archive or no such member");
            return 1;
        }
        adx += off;
        adxSize = sz;
    }

    /* ADX header: 0x8000, copyright offset, encoding 3, 18-byte frames of 4-bit samples. */
    if (adx[0] != 0x80 || adx[1] != 0x00 || adx[4] != 3 || adx[5] != 18 || adx[6] != 4) {
        fail("not a 4-bit ADX type 3");
        return 1;
    }
    nch = adx[7];
    rate = be32(adx + 8);
    total = be32(adx + 12);
    fprintf(stderr, "adx_wav_test: %s, %d channel(s), %u Hz, %u samples (%.2f s), %lld bytes\n", adxPath,
            nch, rate, total, (double)total / rate, adxSize);
    if (nch < 1 || nch > 2 || rate < 8000 || rate > 48000 || total == 0) {
        fail("header values out of range");
        return 1;
    }

    p4snd_use_offline();
    if (streamName != NULL) {
        /* kwlnInitSubsystems: the CRI devices, then the ROFS device and its four volumes with their root
         * directory lists (0x00761690: "/" and the volume names; the lists at 0x00559840 ..) */
        static int32_t prm[5];
        int32_t cfg[4] = {6, 4, 0x3D2, 0};
        static const uint32_t names[4] = {0x00761698u, 0x0076169cu, 0x007616a0u, 0x007616a4u};
        static const uint32_t images[4] = {0x005dc788u, 0x005dc798u, 0x005dc7a8u, 0x005dc7b8u};
        static const uint32_t lists[4] = {0x00559840u, 0x005c4ec0u, 0x005c6350u, 0x005db850u};
        int v;

        prm[0] = (int32_t)0x00761694u;
        func_004d32f8(prm);
        ADXT_Init();
        func_004f34b0(cfg);
        func_004f5bf0(0, 0, 1);
        func_004c6eb0();
        for (v = 0; v < 4; v++) {
            if (func_004c6f50((const char *)P4_G2H(names[v]), (const char *)P4_G2H(images[v]), 0) != 0) {
                fprintf(stderr, "adx_wav_test: volume %d did not attach\n", v);
                return 1;
            }
            func_004f2798((void *)P4_G2H(names[v]), P4_G2H(lists[v]));
        }
        if (streamDir != NULL) {
            char dir[256] = "VOL:\\";
            size_t k;

            strncat(dir, streamDir, sizeof dir - strlen(dir) - 2);
            strcat(dir, "\\");
            for (k = 0; dir[k] != '\0'; k++) {
                if (dir[k] == '/') {
                    dir[k] = '\\';
                }
            }
            H_Cdvd_SelectDirectory(dir);
        }
    } else {
        ADXT_Init();
    }
    work = p4pc_heap_calloc(1, 0x80000);
    adxt = ADXT_Create(2, work, 0x80000);
    if (adxt == NULL) {
        fail("ADXT_Create");
        return 1;
    }
    ADXT_SetLpFlg(adxt, 0);
    if (streamName != NULL && afsMember >= 0) {
        unsigned char *ptinfo = p4pc_heap_calloc(1, 0x8000);
        int polls = 0, st;
        /* ADXF retains its filename as a guest-address word; getenv storage is host-owned. */
        const char *partitionName = P4_G2H(p4_gport_str(streamName));

        if (func_004c74f8(1, partitionName, (void *)blob_of(0x00711190u, 0x007113e0u, "\\SOUND\\"), ptinfo) != 0) {
            fail("ADXF_LoadPartitionNw");
            return 1;
        }
        do {
            ADXT_ExecFsSvr();
            st = func_004c7ef8(1);
            polls++;
            if (st == 2) {
                p4pc_wait_frame_boundary();
            }
        } while (st == 2 && polls < 1000);
        if (st != 3) {
            fprintf(stderr, "adx_wav_test: partition state %d\n", st);
            fail("partition did not load");
            return 1;
        }
        func_004d6d68(adxt, 1, afsMember);
    } else if (streamName != NULL) {
        func_004d6fb8(adxt, streamName);
    } else {
        sj = SJMEM_Create(adx, (int32_t)adxSize);
        if (sj == NULL) {
            fail("SJMEM_Create");
            return 1;
        }
        ADXT_StartSj(adxt, sj);
    }

    cap = (uint32_t)((double)total * P4SND_RATE / rate) + 20u * TICK_FRAMES;
    pcm = calloc((size_t)cap * 2, sizeof(int16_t));
    blk = malloc(TICK_FRAMES * 2 * sizeof(float));
    while (ticks < 60 * (int)(total / rate + 30) && frames + TICK_FRAMES <= cap) {
        int i;

        ADXT_ExecServer();
        if (streamName != NULL) {
            ADXT_ExecFsSvr();
        }
        stat = ADXT_GetStat(adxt);
        if (getenv("P4PC_ADX_VERBOSE") != NULL && (ticks < 40 || ticks % 20 == 0 || stat >= 4)) {
            fprintf(stderr, "adx_wav_test: tick %d adxt stat %d stm stat %d errcode %d\n", ticks, stat,
                    ADXSTM_GetStat(P4_G2H(*(uint32_t *)((uint8_t *)adxt + 8))), (int)*(int16_t *)((uint8_t *)adxt + 0x60));
        }
        if (startFrame < 0 && stat == ADXT_STAT_PLAYING) {
            /* Native C getters and public/inner retail entries must distinguish
             * ADX's 4-bit format from the one/two output joints. The old native-name
             * binding returned 4 channels and overran the joint array at EOF. */
            static const uint32_t getters[] = {0x004d4c58u, 0x004d4c90u, 0x004d4d68u, 0x004d4da0u};
            unsigned k;

            if (ADXT_GetNumChan(adxt) != nch || adxt_GetFmtBps(adxt) != adx[6]) {
                fail("ADXT channel/format getter semantics");
            }
            for (k = 0; k < sizeof getters / sizeof getters[0]; k++) {
                size_t row;
                int32_t (*getter)(void *) = NULL;

                for (row = 0; row < p4pc_native_count; row++) {
                    if (p4pc_native_table[row].addr == getters[k]) {
                        getter = (int32_t (*)(void *))p4pc_native_table[row].fn;
                        break;
                    }
                }
                if (getter == NULL || getter(adxt) != (k < 2 ? nch : adx[6])) {
                    fail("retail-address ADXT channel/format getter");
                }
            }
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
        if (streamName != NULL) {
            p4pc_wait_frame_boundary();
        }
    }
    if (startFrame < 0 || endFrame < 0) {
        fprintf(stderr, "adx_wav_test: ADXT stat %d after %d ticks\n", stat, ticks);
        fail("ADXT did not play to the end");
        return 1;
    }
    if (write_wav(wavPath, pcm, frames) != 0) {
        fail("write WAV");
    }

    /* duration: from the first frame the voice played to the end state */
    {
        double played = (double)(endFrame - startFrame) / P4SND_RATE;
        double want = (double)total / rate;

        fprintf(stderr, "adx_wav_test: played %.3f s, header %.3f s\n", played, want);
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
        fprintf(stderr, "adx_wav_test: peak %d, rms %.1f, clipped %lld\n", peak, energy, clipped);
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
        for (ch = 0; ch < 2; ch++) {
            const int16_t *r = ref[nch == 1 ? 0 : ch];
            /* the mixer gain of the stream; fitted over the first second after the ramp-in */
            double num = 0.0, den = 0.0, gain;
            uint32_t idx = 0;
            double frac = 0.0;
            long long j;
            long long n = endFrame - startFrame;
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
                fprintf(stderr, "adx_wav_test: channel %d gain %.4f\n", ch, gain);
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
            fprintf(stderr, "adx_wav_test: channel %d gain %.4f\n", ch, gain);
        }
        fprintf(stderr, "adx_wav_test: worst deviation from the reference %lld LSB\n", worst);
        if (worst > 2) {
            fail("output deviates from the reference decode");
        }
        for (ch = 0; ch < nch; ch++) {
            free(ref[ch]);
        }
    }

    ADXT_Destroy(adxt);
    free(pcm);
    free(blk);
    p4pc_heap_free(work);
    p4pc_heap_free(adxBase);
    p4snd_shutdown();
    if (failures != 0) {
        return 1;
    }
    printf("adx_wav_test: ok, %s -> %s\n", adxPath, wavPath);
    return 0;
}

int main(int argc, char **argv)
{
    return p4t_on_guest_stack(test_main, argc, argv);
}

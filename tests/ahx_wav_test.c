/* ahx_wav_test: an AHX voice file from the retail disc through the game's own sound object to a WAV file.
 *
 * The test builds the ACSSND object the way sdkSndInit and sdkSndReq do for a battle voice (the parameter
 * block of mode 2: two buffers' worth of work from func_004d8d10, extra-buffer flag set, func_004d8cc0 to
 * create, func_004d8fb8 = ACSSND_AttachAhx to hand every buffer's ADXT the AHX decoder, func_004d8d60 =
 * ACSSND_StartMem to play the file from memory). The server tick is the ACS server step plus
 * ADXT_ExecServer, and the mixer is pulled offline 800 frames (1/60 s) per tick into a 16-bit stereo
 * 48 kHz WAV (argv[1], default ahx_wav_test.wav).
 *
 * Checks, against the AHX header and an independent decoder:
 *   - the header is a one channel AHX (coding 0x10 or 0x11);
 *   - the sound reaches the playing state and then the end, and the played time matches total / rate
 *     within 100 ms, plus the renderer's report latency at the end;
 *   - the waveform is not silent and (nearly) does not clip;
 *   - the output equals the reference within the difference of fixed point and float arithmetic. The
 *     reference is libavcodec's MPEG audio layer II decoder: the AHX frames are layer II frames (MPEG-2
 *     LSF header 0xfff5e0c0 at the head of each, mono), cut at their sync words, resampled the way the
 *     mixer resamples (linear, 24 kHz to 48 kHz) and fitted for the stream gain.
 *
 * It plays all twelve battle end voices of the disc (DATA/SOUND/BATTLE/EM1xx_END.AHX), or P4PC_AHX (one disc path). Exits 77
 * (CTest skip) when P4PC_ISO or P4PC_ELF is unset. */
#define _FILE_OFFSET_BITS 64
#define _POSIX_C_SOURCE 200809L

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libavcodec/avcodec.h>

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
#define EDGE 2000 /* output frames to skip at the ends (the mixer ramps in and out) */

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
/* the game's sound facade (decomp cri/re4: every argument a register word, so a pointer is its guest address) */
int32_t func_004d8c78(void);                                            /* ACSSVR_Init */
int32_t func_004d8d10(uint32_t blk);                                    /* ACSSVR_CalcWorkCprm */
uint32_t func_004d8cc0(uint32_t blk, uint32_t work, uint32_t size);     /* ACSSVR_CreatSnd */
void func_004d8cf0(uint32_t snd);                                       /* ACSSVR_DestroySnd */
void func_004d8fb8(uint32_t snd);                                       /* ACSSND_AttachAhx */
void func_004d8ec8(uint32_t snd, int32_t v);
void func_004d8eb0(uint32_t snd, int32_t v);
void func_004d8f40(uint32_t snd, int32_t v);
void func_004d8f70(uint32_t snd, int32_t v);
void func_004d8d60(uint32_t snd, uint32_t data, uint32_t size);         /* ACSSND_StartMem */
int32_t func_004d8dd8(uint32_t snd);                                    /* ACSSND_GetStat */
void *ACSSND_GetActiveAdxt(void *snd);                                  /* port native: a host pointer to the sound object */
uint32_t func_004c0e50(void *svr);                                      /* ACS server step (port native) */
void ADXT_SetLpFlg(void *adxt, int32_t flg);
void ADXT_ExecServer(void);
int32_t ADXT_GetStat(void *adxt);
SjObj *SJMEM_Create(void *buf, int32_t bsize);
void *kwlnProbeHeapBudget(void);

static int failures;

static void fail(const char *what)
{
    fprintf(stderr, "ahx_wav_test: FAIL %s\n", what);
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
    buf = p4pc_heap_malloc((size_t)end);   /* the file is read by guest address (the AHX decoder): game heap */
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

/* The reference: libavcodec's MPEG audio layer II decoder on the AHX frames. Each frame starts with the
 * same MPEG-2 LSF header (0xfff5e0c0 for most files) and runs to the next one; the header's bitrate field is nominal (the real
 * frames are shorter), so a frame is zero padded to the size the header announces. Returns the mono PCM. */
static int16_t *ref_decode(const uint8_t *d, long long size, int *nout)
{
    const AVCodec *codec = avcodec_find_decoder(AV_CODEC_ID_MP2);
    AVCodecContext *cx;
    AVPacket *pk = av_packet_alloc();
    AVFrame *fr = av_frame_alloc();
    int16_t *out;
    int cap = 0, n = 0;
    long long pos = be16(d + 2) + 4, e;
    uint32_t sync = be32(d + pos);

    *nout = 0;
    if (codec == NULL) {
        *nout = -1; /* this FFmpeg build has no MP2 decoder */
        return NULL;
    }
    if (pk == NULL || fr == NULL) {
        return NULL;
    }
    cx = avcodec_alloc_context3(codec);
    if (cx == NULL || avcodec_open2(cx, codec, NULL) < 0) {
        return NULL;
    }
    cap = (int)(size / 8) * 1152 / 16 + 4608;
    out = malloc((size_t)cap * sizeof(int16_t));
    while (out != NULL && pos + 4 <= size) {
        int r;

        for (e = pos + 4; e + 4 <= size && be32(d + e) != sync; e++) {
        }
        if (e + 4 > size) {
            /* the last frame runs into the footer: 0x8001, its length, "AHXE(c)CRI" */
            for (e = size - 4; e > pos && !(d[e] == 0x80 && d[e + 1] == 0x01 && e + 4 + be16(d + e + 2) == size); e--) {
            }
            if (e <= pos) {
                break;
            }
        }
        if (av_new_packet(pk, 4096) < 0) {
            return NULL;
        }
        memset(pk->data, 0, (size_t)pk->size);
        memcpy(pk->data, d + pos, (size_t)(e - pos));
        r = avcodec_send_packet(cx, pk);
        av_packet_unref(pk);
        if (r < 0) {
            fprintf(stderr, "ahx_wav_test: reference: frame at %lld: error %d\n", pos, r);
        }
        while (r >= 0 && avcodec_receive_frame(cx, fr) >= 0) {
            int i;

            for (i = 0; i < fr->nb_samples && n < cap; i++) {
                if (fr->format == AV_SAMPLE_FMT_S16 || fr->format == AV_SAMPLE_FMT_S16P) {
                    out[n++] = ((const int16_t *)fr->data[0])[i];
                } else {
                    float v = ((const float *)fr->data[0])[i] * 32768.0f;

                    out[n++] = (int16_t)(v > 32767.0f ? 32767.0f : (v < -32768.0f ? -32768.0f : lrintf(v)));
                }
            }
        }
        pos = e;
        if (e + 4 > size || be32(d + e) != sync) {
            break;
        }
    }
    avcodec_free_context(&cx);
    av_packet_free(&pk);
    av_frame_free(&fr);
    *nout = n;
    return out;
}

static int play_one(const char *adxPath, const char *wavPath)
{
    long long adxSize = 0;
    uint8_t *adx;
    uint32_t rate, total;
    int nch;
    void *work;
    void *adxt;
    uint32_t snd;
    int16_t *pcm = NULL;
    float *blk;
    uint32_t cap, frames = 0;
    long long startFrame = -1, endFrame = -1;
    int32_t stat = 0;
    int ticks = 0;
    int before = failures;

    adx = read_disc_file(adxPath, &adxSize);
    if (adx == NULL) {
        fprintf(stderr, "ahx_wav_test: cannot read %s\n", adxPath);
        failures++;
        return 1;
    }

    /* ADX container header: 0x8000, copyright offset, coding 0x10 or 0x11 (AHX), one channel. */
    if (adx[0] != 0x80 || adx[1] != 0x00 || (adx[4] != 0x10 && adx[4] != 0x11)) {
        fail("not an AHX");
        return 1;
    }
    nch = adx[7];
    rate = be32(adx + 8);
    total = be32(adx + 12);
    fprintf(stderr, "ahx_wav_test: %s, %d channel(s), %u Hz, %u samples (%.2f s), %lld bytes\n", adxPath,
            nch, rate, total, (double)total / rate, adxSize);
    fprintf(stderr, "ahx_wav_test: coding 0x%02x, first frame header %08x, header bytes 0x10..0x23: %08x %08x %08x %08x %08x\n", adx[4], be32(adx + be16(adx + 2) + 4), be32(adx + 16), be32(adx + 20), be32(adx + 24), be32(adx + 28), be32(adx + 32));
    if (nch != 1 || rate < 8000 || rate > 48000 || total == 0) {
        fail("header values out of range");
        return 1;
    }

    {
        /* the parameter block of sdkSndReq's mode 2 (memory start of an AHX voice) in slot 2 */
        static const int32_t pbInit[10] = { 2, 1, 0x5DC0, 0, 0, 3, 0, 0, 1, 0 };
        int32_t *pb = p4t_galloc(sizeof pbInit, 16);   /* read by guest address */
        int32_t size;

        memcpy(pb, pbInit, sizeof pbInit);
        size = func_004d8d10(P4_H2G(pb));
        work = p4pc_heap_calloc(1, (size_t)size + 64);
        snd = func_004d8cc0(P4_H2G(pb), P4_H2G(work), (uint32_t)size);
        if (snd == 0) {
            fail("ACSSVR_CreatSnd");
            return 1;
        }
        func_004d8fb8(snd);
        func_004d8ec8(snd, 0);
        func_004d8eb0(snd, 0);
        func_004d8f40(snd, 0);
        func_004d8f70(snd, 0);
        func_004d8d60(snd, P4_H2G(adx), (uint32_t)adxSize);
    }
    adxt = ACSSND_GetActiveAdxt(P4_G2H(snd));
    if (adxt == NULL) {
        fail("no ADXT behind the sound object");
        return 1;
    }
    ADXT_SetLpFlg(adxt, 0);

    cap = (uint32_t)((double)total * P4SND_RATE / rate) + 20u * TICK_FRAMES;
    pcm = calloc((size_t)cap * 2, sizeof(int16_t));
    blk = malloc(TICK_FRAMES * 2 * sizeof(float));
    while (ticks < 60 * (int)(total / rate + 30) && frames + TICK_FRAMES <= cap) {
        int i;

        func_004c0e50(P4_G2H(P4_G32(0x00725294)));
        ADXT_ExecServer();
        stat = ADXT_GetStat(adxt);
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
    if (wavPath != NULL && write_wav(wavPath, pcm, frames) != 0) {
        fail("write WAV");
    }
    if (startFrame < 0 || endFrame < 0) {
        fprintf(stderr, "ahx_wav_test: ADXT stat %d after %d ticks\n", stat, ticks);
        fail("ADXT did not play to the end");
        return 1;
    }

    /* duration: from the first frame the voice played to the end state. The end state comes after
     * the last sample by the renderer's report latency, as on the PS2 (docs/audio.md, "End of a
     * voice"): 2.75 server passes (one per 1/60 s tick here) and up to one 256-sample block. */
    {
        double played = (double)(endFrame - startFrame) / P4SND_RATE;
        double want = (double)total / rate;
        double report = 2.75 / 60.0 + 256.0 / rate;

        fprintf(stderr, "ahx_wav_test: played %.3f s, header %.3f s\n", played, want);
        if (played - want < -0.1 || played - want > 0.1 + report) {
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
        fprintf(stderr, "ahx_wav_test: peak %d, rms %.1f, clipped %lld\n", peak, energy, clipped);
        if (energy < 200.0) {
            fail("waveform is silent");
        }
        if (energy > 20000.0) {
            fail("waveform level is implausible");
        }
        /* a shouted battle voice reaches full scale now and then (the decoder itself clamps its sums);
         * a gain or wrap error puts far more than 0.5% of the samples on the rails */
        if (clipped * 200 > (endFrame - startFrame) * 2) {
            fail("waveform clips");
        }
    }

    /* against the independent decoder: find the offset between the stat-based start and the first rendered
     * sample, fit the stream gain, and bound the difference */
    {
        int nref = 0;
        int16_t *ref = ref_decode(adx, adxSize, &nref);
        long long n = endFrame - startFrame;
        long long lag, bestlag = 0;
        double best = -1e300, num = 0.0, den = 0.0, gain, se = 0.0, sig = 0.0, maxerr = 0.0;
        long long j, cnt = 0;

        if (ref == NULL && nref < 0) {
            fprintf(stderr, "ahx_wav_test: the reference decoder (libavcodec MP2) is not in this FFmpeg build: comparison skipped\n");
            goto done;
        }
        if (ref == NULL || nref < (int)total) {
            fail("reference decode");
            return 1;
        }
        /* output frame j (stream rate 24 kHz into 48 kHz) is the reference at j / 2, linearly interpolated */
        #define REF48(k) (((k) < 0 || (k) / 2 + 1 >= nref) ? 0.0 : \
            (double)ref[(k) / 2] + ((double)ref[(k) / 2 + 1] - (double)ref[(k) / 2]) * (((k) & 1) ? 0.5 : 0.0))
        for (lag = -8 * TICK_FRAMES; lag <= 8 * TICK_FRAMES; lag++) {
            double c = 0.0;

            /* the first 4 s of the voice, every 5th sample (a voice may start with silence) */
            for (j = EDGE; j < n - EDGE && j < EDGE + 200000; j += 5) {
                c += (double)pcm[(startFrame + j) * 2] * REF48(j + lag);
            }
            if (c > best) {
                best = c;
                bestlag = lag;
            }
        }
        fprintf(stderr, "ahx_wav_test: the first sample is rendered %lld frames after the stat reached playing\n", -bestlag);
        if (bestlag > 0 || bestlag < -8 * TICK_FRAMES + 1) {
            fail("the stream is not where the sound was started");
        }
        for (j = EDGE; j < n - EDGE; j++) {
            double x = REF48(j + bestlag);

            num += x * pcm[(startFrame + j) * 2];
            den += x * x;
        }
        gain = den > 0.0 ? num / den : 0.0;
        for (j = EDGE; j < n - EDGE; j++) {
            double x = REF48(j + bestlag) * gain;
            double e = fabs(x - (double)pcm[(startFrame + j) * 2]);

            se += e * e;
            sig += x * x;
            cnt++;
            if (e > maxerr) {
                maxerr = e;
            }
        }
        fprintf(stderr, "ahx_wav_test: gain %.4f, rms error %.2f of %.1f, max error %.0f\n", gain,
                sqrt(se / (double)cnt), sqrt(sig / (double)cnt), maxerr);
        if (gain < 0.9 || gain > 1.1) {
            fail("stream gain out of range");
        }
        /* fixed point against float synthesis: a few hundredths of a percent of the level, well under 1% */
        if (sqrt(se / (double)cnt) > 0.005 * sqrt(sig / (double)cnt) || maxerr > 330.0) {
            fail("output deviates from the reference decode");
        }
        free(ref);
    }
done:

    func_004d8cf0(snd);
    free(pcm);
    free(blk);
    p4pc_heap_free(work);
    p4pc_heap_free(adx);
    if (failures != before) {
        return 1;
    }
    printf("ahx_wav_test: ok, %s%s%s\n", adxPath, wavPath != NULL ? " -> " : "", wavPath != NULL ? wavPath : "");
    return 0;
}

/* The twelve battle end voices of the disc (EM100 .. EM10F, with the ids the game has). */
static const char *const voices[] = {
    "EM100", "EM101", "EM102", "EM103", "EM105", "EM106", "EM107", "EM108", "EM10A", "EM10B", "EM10E", "EM10F",
};

static int test_main(int argc, char **argv)
{
    const char *iso = getenv("P4PC_ISO");
    const char *elf = getenv("P4PC_ELF");
    const char *wavPath = argc > 1 ? argv[1] : "ahx_wav_test.wav";
    const char *one = getenv("P4PC_AHX");
    size_t i;

    if (iso == NULL || iso[0] == '\0' || elf == NULL || elf[0] == '\0') {
        fprintf(stderr, "ahx_wav_test: P4PC_ISO or P4PC_ELF unset, skipped\n");
        return SKIP_RETURN;
    }
    p4_os_setenv("P4PC_TRAP_CONTINUE", "1", 0);
    p4pc_trap_configure();
    p4pc_load_image(elf);
    p4pc_apply_patches();
    p4pc_kernel_pre_init();
    kwlnProbeHeapBudget();
    if (vfs_mount(iso, NULL) != 0) {
        fprintf(stderr, "ahx_wav_test: cannot mount %s\n", iso);
        return 1;
    }
    p4snd_use_offline();
    ADXT_Init();
    func_004d8c78();
    if (one != NULL) {
        play_one(one, wavPath);
    } else {
        for (i = 0; i < sizeof voices / sizeof voices[0]; i++) {
            char path[64];

            snprintf(path, sizeof path, "DATA/SOUND/BATTLE/%s_END.AHX", voices[i]);
            play_one(path, i == 0 ? wavPath : NULL);
        }
    }
    p4snd_shutdown();
    return failures != 0 ? 1 : 0;
}

int main(int argc, char **argv)
{
    return p4t_on_guest_stack(test_main, argc, argv);
}

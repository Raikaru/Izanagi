/* axrna_test: the AXRNA renderer (src/platform/sound/axrna.c) behind CRI's ADXRNA front end, on
 * CRI's own stream joints (SJRBF, the retail ring joint), with the mixer pulled offline.
 *
 * The contract ADXT's server (src/cri/adxt_svr.c) relies on:
 *   - GetNumRoom is PS2RNA's IOP buffer (0x2000 samples) minus GetNumData; nothing is pulled
 *     from the joints while the transfer switch is off;
 *   - ExecServer moves the same number of samples from every channel's joint, returns the
 *     consumed chunks to the joint's free line, and never more than GetNumRoom;
 *   - while playing, GetNumData falls by what retail's EE hears back from the IOP: whole blocks
 *     of 512 SPU2 output samples (256 of a 24 kHz stream) on the IOP's clock, which runs at a
 *     fixed phase against ADXT's pass count (0x007188b8). The first block is heard 6 or 7 block
 *     ends after key-on. Keyed on at count 13600 and at count 8811, a 24 kHz stream is heard
 *     exactly as retail heard collision 900's cue 0x116 at those counts. Paused (play off,
 *     transfer on) it holds; it never reports more than was pushed; stopped (play off after
 *     transfer off) it empties at once; a discard leaves it at once;
 *   - what comes out is what went in, in order, channel 0 on the left and channel 1 on the right
 *     at stereo pan, scaled by the volume (1/10 dB), with a long input spread over several
 *     ExecServer calls.
 * Boots the image like render_smoke (the CRI code is compiled decomp). Exits 77 (CTest skip)
 * when P4PC_ISO or P4PC_ELF is unset, as the other tests that boot the image do. */
#define _FILE_OFFSET_BITS 64
#define _POSIX_C_SOURCE 200809L

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "p4pc/image.h"
#include "p4pc/os.h"
#include "guest_test.h"
#include "p4snd.h"

#define SKIP_RETURN 77
#define RING 0x2000        /* the samples PS2RNA reports: its IOP buffer of 0x4000 bytes */
#define ADXT_VSYNC_CNT 0x007188b8u /* adxt_vsync_cnt: one count per server pass */
#define JOINT_BYTES 0x3000 /* the joint holds 0x1800 samples, less than the ring: the tail waits */

/* A stream joint is a guest object whose first word is the guest address of its vtable of guest code addresses
 * (cri_mem.h): +0x14 Reset, +0x18 GetChunk, +0x20 PutChunk, +0x24 GetNumData. The chunk is a guest data address and a length. */
typedef struct {
    uint32_t data;
    int32_t len;
} SjCk;
typedef struct SjObj SjObj;

static const uint32_t *sj_vt(SjObj *sj)
{
    return (const uint32_t *)P4_G2H(*(const uint32_t *)sj);
}

static void sj_get(SjObj *sj, int32_t line, int32_t n, SjCk *ck)
{
    ((void (*)(SjObj *, int32_t, int32_t, SjCk *))P4_FN_G2H(sj_vt(sj)[6]))(sj, line, n, ck);
}

static void sj_put(SjObj *sj, int32_t line, SjCk *ck)
{
    ((void (*)(SjObj *, int32_t, SjCk *))P4_FN_G2H(sj_vt(sj)[8]))(sj, line, ck);
}

static int32_t sj_num(SjObj *sj, int32_t line)
{
    return ((int32_t(*)(SjObj *, int32_t))P4_FN_G2H(sj_vt(sj)[9]))(sj, line);
}

static void sj_reset(SjObj *sj)
{
    ((void (*)(SjObj *))P4_FN_G2H(sj_vt(sj)[5]))(sj);
}

void SJRBF_Init(void);
SjObj *SJRBF_Create(void *buf, int32_t bsize, int32_t xsize);
void AXRNA_Init(void);
void *AXRNA_Create(uint32_t sjo, uint32_t nch);   /* the address of the joint table, the channel count */
void AXRNA_Destroy(void *rna);
void AXRNA_ExecServer(void);
void AXRNA_SetTransSw(void *rna, int32_t sw);
void AXRNA_SetPlaySw(void *rna, int32_t sw);
void AXRNA_SetSfreq(void *rna, int32_t sfreq);
void AXRNA_SetNumChan(void *rna, int32_t nch);
void AXRNA_SetOutVol(void *rna, int32_t vol);
void AXRNA_SetOutPan(void *rna, int32_t ch, int32_t pan);
void AXRNA_DiscardData(void *rna, int32_t nsmpl);
int32_t AXRNA_GetNumData(void *rna);
int32_t AXRNA_GetNumRoom(void *rna);
void *kwlnProbeHeapBudget(void);

static int failures;

static void fail(const char *what)
{
    fprintf(stderr, "axrna_test: FAIL %s\n", what);
    failures++;
}

static void check(const char *what, long got, long want)
{
    if (got != want) {
        fprintf(stderr, "axrna_test: FAIL %s: got %ld want %ld\n", what, got, want);
        failures++;
    }
}

/* One server pass, as ADXM_ExecVint drives it: the pass count, then the renderer's server. */
static void pass(void)
{
    *(uint32_t *)P4_G2H(ADXT_VSYNC_CNT) += 1;
    AXRNA_ExecServer();
}

static void set_count(uint32_t cnt)
{
    *(uint32_t *)P4_G2H(ADXT_VSYNC_CNT) = cnt;
}

/* Retail's reported consumption (PS2RNA_GetNumData's complement) after each pass of collision
 * 900's cue 0x116 (24 kHz), keyed on at pass count 13600 and at 8811 (index: passes after key-on). */
static const int32_t heard_13600[9] = {0, 0, 0, 768, 1536, 2304, 3072, 4096, 4864};
static const int32_t heard_8811[9] = {0, 0, 0, 768, 1792, 2560, 3328, 4096, 4864};

static int16_t pattern(int ch, int i)
{
    return (int16_t)(ch == 0 ? (i * 7) % 20000 - 10000 : 9000 - (i * 5) % 18000);
}

/* Appends n samples of channel ch starting at pattern index `from` to a joint's data line. */
static int put_samples(SjObj *sj, int ch, int from, int n)
{
    int done = 0;

    while (done < n) {
        SjCk ck;
        int k, got;

        sj_get(sj, 0, (n - done) * 2, &ck);
        got = ck.len / 2;
        if (got <= 0) {
            return done;
        }
        for (k = 0; k < got; k++) {
            ((int16_t *)P4_G2H(ck.data))[k] = pattern(ch, from + done + k);
        }
        sj_put(sj, 1, &ck);
        done += got;
    }
    return done;
}

/* Renders `frames` and collects both outputs. */
static void render(float *l, float *r, int frames)
{
    static float blk[240 * 2];
    int done = 0;

    while (done < frames) {
        int c = frames - done > 240 ? 240 : frames - done;
        int i;

        p4snd_render(blk, c);
        for (i = 0; i < c; i++) {
            l[done + i] = blk[i * 2];
            r[done + i] = blk[i * 2 + 1];
        }
        done += c;
    }
}

static int test_main(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    const char *iso = getenv("P4PC_ISO");
    const char *elf = getenv("P4PC_ELF");
    uint8_t *buf[2];
    static float l[48000], r[48000];
    SjObj *sj[2];
    uint32_t *sjo;
    void *rna;
    int i, n;

    if (iso == NULL || iso[0] == '\0' || elf == NULL || elf[0] == '\0') {
        fprintf(stderr, "axrna_test: P4PC_ISO or P4PC_ELF unset, skipped\n");
        return SKIP_RETURN;
    }
    p4_os_setenv("P4PC_TRAP_CONTINUE", "1", 0);
    p4pc_trap_configure();
    p4pc_load_image(elf);
    p4pc_apply_patches();
    p4pc_kernel_pre_init();
    kwlnProbeHeapBudget();
    p4snd_use_offline();

    SJRBF_Init();
    AXRNA_Init();
    sjo = p4t_galloc(2 * sizeof *sjo, 16);   /* the joint table AXRNA_Create reads: guest storage */
    for (i = 0; i < 2; i++) {
        buf[i] = p4t_galloc(JOINT_BYTES + 0x200, 16);
        sj[i] = SJRBF_Create(buf[i], JOINT_BYTES, 0x200);
        if (sj[i] == NULL) {
            fail("SJRBF_Create");
            return 1;
        }
        sjo[i] = P4_H2G(sj[i]);
    }
    rna = AXRNA_Create(P4_H2G(sjo), 2);
    if (rna == NULL) {
        fail("AXRNA_Create");
        return 1;
    }
    AXRNA_SetSfreq(rna, 24000);
    AXRNA_SetNumChan(rna, 2);
    AXRNA_SetOutVol(rna, 0);
    AXRNA_SetOutPan(rna, 0, -15);
    AXRNA_SetOutPan(rna, 1, 15);

    check("empty ring, data", AXRNA_GetNumData(rna), 0);
    check("empty ring, room", AXRNA_GetNumRoom(rna), RING);

    /* a joint holds 0x3000 samples: fill both, but the transfer switch is off */
    n = JOINT_BYTES / 2;
    check("fill ch0", put_samples(sj[0], 0, 0, n), n);
    check("fill ch1", put_samples(sj[1], 1, 0, n), n);
    pass();
    check("nothing moves while the transfer is off", AXRNA_GetNumData(rna), 0);

    AXRNA_SetTransSw(rna, 1);
    pass();
    check("all joint data moves", AXRNA_GetNumData(rna), n);
    check("room shrinks", AXRNA_GetNumRoom(rna), RING - n);
    check("joint data line empties", sj_num(sj[0], 1), 0);
    check("joint free line refills", sj_num(sj[0], 0), JOINT_BYTES);

    /* stopped voice: time does not pass */
    render(l, r, 480);
    pass();
    check("not playing, data holds", AXRNA_GetNumData(rna), n);
    for (i = 0; i < 480; i++) {
        if (l[i] != 0.0f || r[i] != 0.0f) {
            fail("sound before the play switch");
            break;
        }
    }

    /* key-on in the pass counted 13600: the mixer plays at once, the IOP's blocks are heard as on retail */
    set_count(13600);
    AXRNA_SetPlaySw(rna, 1);
    AXRNA_ExecServer(); /* the key-on pass's own renderer step */
    check("key-on pass: nothing heard", AXRNA_GetNumData(rna), n);
    render(l, r, 960);
    for (i = 1; i <= 8; i++) {
        char what[64];

        pass();
        snprintf(what, sizeof what, "heard %d passes after key-on at 13600", i);
        check(what, AXRNA_GetNumData(rna), n - heard_13600[i]);
    }

    /* pause: play off, transfer on: the data stays; a resume is heard after its own start */
    AXRNA_SetPlaySw(rna, 0);
    for (i = 0; i < 3; i++) {
        pass();
    }
    check("paused, data holds", AXRNA_GetNumData(rna), n - heard_13600[8]);
    AXRNA_SetPlaySw(rna, 1);
    pass();
    pass();
    check("resumed, nothing heard for two passes", AXRNA_GetNumData(rna), n - heard_13600[8]);
    pass();
    check("resumed, the third pass hears three blocks", AXRNA_GetNumData(rna), n - heard_13600[8] - 768);
    pass();
    check("never more heard than pushed", AXRNA_GetNumData(rna), 0);

    /* stop: both off empties the ring; key-on in the pass counted 8811 is heard in the other phase */
    AXRNA_SetPlaySw(rna, 0);
    AXRNA_SetTransSw(rna, 0);
    AXRNA_SetPlaySw(rna, 0);
    check("stop empties the ring", AXRNA_GetNumData(rna), 0);
    sj_reset(sj[0]);
    sj_reset(sj[1]);
    put_samples(sj[0], 0, 0, n);
    put_samples(sj[1], 1, 0, n);
    AXRNA_SetTransSw(rna, 1);
    pass();
    set_count(8811);
    AXRNA_SetPlaySw(rna, 1);
    AXRNA_ExecServer();
    for (i = 1; i <= 8; i++) {
        char what[64];

        pass();
        snprintf(what, sizeof what, "heard %d passes after key-on at 8811", i);
        check(what, AXRNA_GetNumData(rna), n - heard_8811[i]);
    }

    /* volume categories, at 48 kHz where an output frame is a stream sample: a handle whose +0x0c
     * names this renderer tags its voice; the voice category at gain 0.5 halves the output, at 0
     * silences it, and back at 1 the output is the input again */
    AXRNA_SetPlaySw(rna, 0);
    AXRNA_SetTransSw(rna, 0);
    AXRNA_SetPlaySw(rna, 0);
    sj_reset(sj[0]);
    sj_reset(sj[1]);
    AXRNA_SetSfreq(rna, 48000);
    put_samples(sj[0], 0, 0, n);
    put_samples(sj[1], 1, 0, n);
    AXRNA_SetTransSw(rna, 1);
    pass();
    AXRNA_SetPlaySw(rna, 1);
    render(l, r, 240); /* the first chunk ramps its gain in */
    {
        uint32_t *adxt = p4t_galloc(0x20, 16);
        const int at = 240;

        adxt[3] = P4_H2G(rna);
        check("a word naming no renderer is refused", p4snd_adxt_set_category(P4_H2G(adxt) + 4u, P4SND_CAT_VOICE), -1);
        check("the handle's renderer is tagged", p4snd_adxt_set_category(P4_H2G(adxt), P4SND_CAT_VOICE), 0);
        check("a gain above 1 is refused", p4snd_set_category_gain(P4SND_CAT_VOICE, 1.5f), -1);
        check("voice gain 0.5", p4snd_set_category_gain(P4SND_CAT_VOICE, 0.5f), 0);
        check("voice reads 0.5", p4snd_category_gain(P4SND_CAT_VOICE) == 0.5f, 1);
        check("music stays 1", p4snd_category_gain(P4SND_CAT_MUSIC) == 1.0f, 1);
        render(l, r, 240);
        for (i = 0; i < 240; i++) {
            if (fabs(l[i] - 0.5 * (double)pattern(0, at + i) / 32768.0) > 2.0 / 32768.0) {
                fail("voice at 0.5 is not half the input");
                break;
            }
        }
        check("voice gain 0", p4snd_set_category_gain(P4SND_CAT_VOICE, 0.0f), 0);
        render(l, r, 240);
        for (i = 0; i < 240; i++) {
            if (l[i] != 0.0f || r[i] != 0.0f) {
                fail("voice at 0 is not silent");
                break;
            }
        }
        check("music gain 0 leaves a voice stream", p4snd_set_category_gain(P4SND_CAT_MUSIC, 0.0f), 0);
        check("voice gain 1", p4snd_set_category_gain(P4SND_CAT_VOICE, 1.0f), 0);
        render(l, r, 240);
        for (i = 0; i < 240; i++) {
            if (fabs(l[i] - (double)pattern(0, at + 480 + i) / 32768.0) > 2.0 / 32768.0) {
                fail("voice back at 1 is not the input");
                break;
            }
        }
        check("music gain 1", p4snd_set_category_gain(P4SND_CAT_MUSIC, 1.0f), 0);
    }

    /* the ring was filled with n < RING; push the rest of a longer stream through several
     * passes while playing and compare the whole output */
    AXRNA_SetPlaySw(rna, 0);
    AXRNA_SetTransSw(rna, 0);
    AXRNA_SetPlaySw(rna, 0); /* stop: both off empties the ring */
    check("stop empties the ring again", AXRNA_GetNumData(rna), 0);
    sj_reset(sj[0]);
    sj_reset(sj[1]);
    AXRNA_SetSfreq(rna, 48000);
    {
        const int total = 40000; /* more than the ring and the joint together */
        static float ol[40000 + 2000], or_[40000 + 2000];
        int sent = 0, got = 0;
        double worst = 0.0;

        AXRNA_SetTransSw(rna, 1);
        AXRNA_SetPlaySw(rna, 1);
        while (got < total + 480) {
            int room = JOINT_BYTES / 2;
            int step = sent < total ? (total - sent < room ? total - sent : room) : 0;

            if (step > 0) {
                int a = put_samples(sj[0], 0, sent, step);
                int b = put_samples(sj[1], 1, sent, step);

                if (a != b) {
                    fail("joint fill mismatch");
                    break;
                }
                sent += a;
            }
            pass();
            render(ol + got, or_ + got, 480);
            got += 480;
            if (AXRNA_GetNumData(rna) > RING || AXRNA_GetNumData(rna) < 0) {
                fail("report outside the IOP buffer");
                break;
            }
        }
        /* the stream is done: the transfer switch goes off, the report drains to zero */
        AXRNA_SetTransSw(rna, 0);
        n = 0;
        while (AXRNA_GetNumData(rna) > 0 && n++ < 100) {
            static float tl[480], tr[480];

            render(tl, tr, 480);
            pass();
        }
        check("the report drains", AXRNA_GetNumData(rna), 0);
        for (i = 0; i < total; i++) {
            /* the voice starts at the first rendered chunk; the first chunk ramps its gain in */
            if (i < 240) {
                continue;
            }
            worst = fmax(worst, fabs(ol[i] - (double)pattern(0, i) / 32768.0));
            worst = fmax(worst, fabs(or_[i] - (double)pattern(1, i) / 32768.0));
        }
        if (worst > 2.0 / 32768.0) {
            fprintf(stderr, "axrna_test: long stream deviates by %.1f LSB\n", worst * 32768.0);
            fail("a long stream through several passes is not intact");
        }
    }

    /* volume: -60 (1/10 dB) is 0.501; pan 0 on a mono source feeds both sides at that gain */
    AXRNA_SetPlaySw(rna, 0);
    AXRNA_SetTransSw(rna, 0);
    AXRNA_SetPlaySw(rna, 0);
    sj_reset(sj[0]);
    sj_reset(sj[1]);
    AXRNA_SetOutVol(rna, -60);
    AXRNA_SetOutPan(rna, 0, 0);
    AXRNA_SetOutPan(rna, 1, 0);
    AXRNA_SetTransSw(rna, 1);
    put_samples(sj[0], 0, 0, 2000);
    put_samples(sj[1], 1, 0, 2000);
    pass();
    AXRNA_SetPlaySw(rna, 1);
    render(l, r, 960);
    {
        const double gain = pow(10.0, -60.0 / 200.0);
        double worst = 0.0;

        for (i = 480; i < 960; i++) {
            /* ch0 and ch1 both centred: both sides carry their sum at gain */
            double e = ((double)pattern(0, i) + (double)pattern(1, i)) / 32768.0 * gain;

            worst = fmax(worst, fabs(l[i] - e));
            worst = fmax(worst, fabs(r[i] - e));
        }
        if (worst > 4.0 / 32768.0) {
            fprintf(stderr, "axrna_test: volume/pan deviates by %.1f LSB\n", worst * 32768.0);
            fail("volume and centre pan");
        }
    }

    /* discard: the discarded samples leave the report at once */
    {
        int before = AXRNA_GetNumData(rna);

        check("nothing heard before the first pass", before, 2000);
        AXRNA_DiscardData(rna, 100);
        check("discard", AXRNA_GetNumData(rna), before - 100);
    }

    AXRNA_Destroy(rna);
    p4snd_shutdown();
    if (failures != 0) {
        return 1;
    }
    printf("axrna_test: ok\n");
    return 0;
}

int main(int argc, char **argv)
{
    return p4t_on_guest_stack(test_main, argc, argv);
}

/* sdr_test: the IOP sound driver commands the game uses on the mixer, through the EE wrappers
 * in the order sdkSndcom.c issues them.
 *
 * Part 1 builds a tiny sound bank in memory (one looping VAG, one tone, one SE sequence: note
 * on at 0, note off at 50 ms) and checks every result code the game branches on, the bank
 * transfer path (SifSetDma, voice transfer into SPU RAM, the SPU heap), and the audio: the note
 * sounds, stops after its note off, and a stop command silences a running one.
 *
 * Part 2 (only with P4PC_ISO) loads the disc's spu/test.hd, .bd and .sq and plays sequence 0:
 * two notes 100 ms apart. It checks the same result codes and that both notes are audible.
 *
 * The mixer runs offline; the test pulls 5 ms at a time and measures what comes out. */
#define _FILE_OFFSET_BITS 64
#define _POSIX_C_SOURCE 200809L

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "guest_test.h"
#include "p4pc/platform.h"
#include "p4snd.h"
#include "sdr.h"
#include "vfs.h"

#define STEP 240 /* frames: 5 ms */

/* the EE wrappers (ps2_sndcom.c) and the SIF/heap natives the game calls */
int32_t func_0043c0c0(int32_t a0, int32_t a1, int32_t a2, int32_t a3, int32_t t0);
int32_t func_0043c180(int32_t a0, int32_t a1, int32_t a2, int32_t a3);
int32_t func_0043c230(int32_t a0, int32_t a1, int32_t a2, int32_t a3, int32_t t0, int32_t t1);
int32_t func_0043c308(int32_t a0, int32_t a1);
int32_t func_0043c3b0(int32_t a0, int32_t a1, int32_t a2, int32_t a3);
int32_t func_0043c470(int32_t a0, int32_t a1);
int32_t func_0043c518(int32_t a0, int32_t a1, int32_t a2, int32_t a3, int32_t t0);
int32_t func_0043c5e8(int32_t a0, int32_t a1, int32_t a2);
int32_t func_00430e28(void);
int32_t func_0043bb70(void *table, int32_t entries, int32_t mode);
int32_t func_0043bcb8(int32_t size);
int32_t func_00429d10(int32_t mode, uint32_t size, int32_t address);
int32_t func_00429d90(int32_t address);
int32_t func_00424708(const void *dmat, int32_t count);

typedef struct {
    uint32_t src, dest;
    int32_t size, attr;
} Dma;

static int failures;

static void fail(const char *what)
{
    fprintf(stderr, "sdr_test: FAIL %s\n", what);
    failures++;
}

static void check_eq(const char *what, int32_t got, int32_t want)
{
    if (got != want) {
        fprintf(stderr, "sdr_test: %s: got %d want %d\n", what, (int)got, (int)want);
        failures++;
    }
}

static void check_ge0(const char *what, int32_t got)
{
    if (got < 0) {
        fprintf(stderr, "sdr_test: %s: got %d, want >= 0\n", what, (int)got);
        failures++;
    }
}

static void voice_transfer(int32_t iop, int32_t spu, uint32_t bytes)
{
    check_eq("voice transfer", func_0043c180(1, iop, spu, (int32_t)bytes), 0);
}

static void put32(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)v;
    p[1] = (uint8_t)(v >> 8);
    p[2] = (uint8_t)(v >> 16);
    p[3] = (uint8_t)(v >> 24);
}

/* sdkSndcom.c's upload: EE memory to IOP memory with SifSetDma. */
static int32_t upload(const void *data, uint32_t size, int32_t iop)
{
    Dma d;
    uint8_t *ee = p4t_galloc(size != 0 ? size : 1, 16); /* EE memory: the DMA source is a guest address */

    memcpy(ee, data, size);
    d.src = P4_H2G(ee);
    d.dest = (uint32_t)iop;
    d.size = (int32_t)size;
    d.attr = 0;
    return func_00424708(&d, 1);
}

/* Renders `ms` milliseconds and returns the RMS of the left+right mix. */
static double render_rms(int ms)
{
    static float buf[STEP * 2];
    double sum = 0.0;
    long n = 0;
    int frames = ms * 48;
    int done;

    for (done = 0; done < frames; done += STEP) {
        int i;

        p4snd_render(buf, STEP);
        for (i = 0; i < STEP * 2; i++) {
            sum += (double)buf[i] * buf[i];
        }
        n += STEP * 2;
    }
    return n ? sqrt(sum / (double)n) : 0.0;
}

/* ------------------------------------------------------------------ synthetic bank */

static void chunk_head(uint8_t *p, const char type[4], uint32_t size)
{
    memcpy(p, "IECS", 4);
    p[4] = (uint8_t)type[3];
    p[5] = (uint8_t)type[2];
    p[6] = (uint8_t)type[1];
    p[7] = (uint8_t)type[0];
    put32(p + 8, size);
}

/* .hd with one VAG at .bd offset 0 (rate 48000) and one tone: volume 127, centre, ADSR 0x80ff /
 * 0x5fc0 */
static uint8_t tone_mix = 0x03; /* byte 19 of the tone: dry left and right */
static int8_t tone_key, tone_fine; /* bytes 11 and 12: pitch in semitones and 1/128 semitone */

static uint32_t make_hd(uint8_t *hd, uint32_t bd_size)
{
    uint8_t *t;

    memset(hd, 0, 0x100);
    chunk_head(hd, "Vers", 0x10);
    chunk_head(hd + 0x10, "Head", 0x40);
    put32(hd + 0x1c, 0xbc);
    put32(hd + 0x20, bd_size);
    put32(hd + 0x24, 0xffffffffu);
    put32(hd + 0x28, 0xffffffffu);
    put32(hd + 0x2c, 0xffffffffu);
    put32(hd + 0x30, 0x50);
    put32(hd + 0x34, 0x6c);
    chunk_head(hd + 0x50, "Vagi", 0x1c);
    put32(hd + 0x50 + 0x0c, 0);
    put32(hd + 0x50 + 0x10, 0x14);
    put32(hd + 0x50 + 0x14, 0);        /* VAG offset */
    hd[0x50 + 0x18] = 48000 & 255;     /* rate */
    hd[0x50 + 0x19] = 48000 >> 8;
    chunk_head(hd + 0x6c, "Setb", 0x50);
    put32(hd + 0x6c + 0x0c, 0);
    put32(hd + 0x6c + 0x10, 0x14);
    put32(hd + 0x6c + 0x14, 0);        /* one set */
    put32(hd + 0x6c + 0x18, 0x1c);
    t = hd + 0x6c + 0x1c + 2;          /* the set's first tone */
    t[6] = 0;                          /* VAG 0 */
    t[9] = 127;
    t[10] = 64;
    t[11] = (uint8_t)tone_key;
    t[12] = (uint8_t)tone_fine;
    t[19] = tone_mix;
    t[20] = 0xff;                      /* ADSR1 0x80ff */
    t[21] = 0x80;
    t[22] = 0xc0;                      /* ADSR2 0x5fc0 */
    t[23] = 0x5f;
    return 0xbc;
}

/* .sq: one set, one sequence: note on set 0 tone 0 velocity 127 at 0, note off 50 ms later */
static uint32_t make_sq_ev(uint8_t *sq, const uint8_t *ev, uint32_t evlen)
{
    const uint32_t base = 0x30;
    uint8_t *set = sq + base + 0x24;
    uint8_t *seq = sq + base + 0x38;

    memset(sq, 0, 0x100);
    chunk_head(sq, "Vers", 0x10);
    chunk_head(sq + 0x10, "Sequ", 0x20);
    put32(sq + 0x28, base);
    chunk_head(sq + base, "Sesq", 0x60);
    put32(sq + base + 0x0c, 0);
    put32(sq + base + 0x20, 0x24);
    put32(set, 0);
    put32(set + 4, 0x0c);
    set[8] = 0x7f;
    set[9] = 0x40;
    set[10] = 1000 & 255;
    set[11] = 1000 >> 8;
    put32(set + 12, 0xffffffffu);
    put32(set + 16, 0x38);
    put32(seq, 0x10);
    seq[4] = 0x7f;
    seq[5] = 0x40;
    seq[6] = 1000 & 255;
    seq[7] = 1000 >> 8;
    put32(seq + 8, evlen);
    memcpy(seq + 0x10, ev, evlen);
    return base + 0x38 + 0x10 + evlen;
}

static uint32_t make_sq(uint8_t *sq)
{
    static const uint8_t ev[] = {0x00, 0x90, 0x00, 0x00, 0x7f, 0x32, 0xa0, 0x00, 0x00, 0x00, 0x00, 0xff};

    return make_sq_ev(sq, ev, sizeof ev);
}

/* four blocks: loop start, two plain, end+repeat; shift 2, filter 0, a square wave */
static uint32_t make_bd(uint8_t *bd)
{
    int b, i;

    for (b = 0; b < 4; b++) {
        uint8_t *blk = bd + b * 16;

        blk[0] = 2;
        blk[1] = b == 0 ? 0x04 : (b == 3 ? 0x03 : 0x00);
        for (i = 0; i < 14; i++) {
            blk[2 + i] = 0x97; /* nibbles 7, 9 (-7) */
        }
    }
    return 64;
}

/* The sequence of sdkSndcom.c sdkSndUpdateDecodeSlots states 3 to 7 and the play/stop of
 * sdkSndStartStreamSlot, with the results the game checks. */
static void run_bank(const char *name, const uint8_t *hd, uint32_t hd_size, const uint8_t *bd,
                     uint32_t bd_size, const uint8_t *sq, uint32_t sq_size, int strict)
{
    int32_t spu, hd_iop, sq_iop, tmp, bank, inst, sqd, player, h, r;
    char what[96];
    double rms;

    fprintf(stderr, "sdr_test: %s\n", name);
    p4sdr_reset();
    check_eq("sceSdInit", func_00430e28(), 0);
    check_eq("heap init", func_0043bb70((void *)hd, 32, 0), 0);
    spu = func_0043bcb8((int32_t)bd_size);
    check_eq("first SPU allocation", spu, 0x5010);
    check_eq("second SPU allocation", func_0043bcb8(0x1000), 0x5010 + (int32_t)((bd_size + 15u) & ~15u));

    hd_iop = func_00429d10(0, hd_size, 0);
    sq_iop = func_00429d10(0, sq_size, 0);
    tmp = func_00429d10(0, 0x1000, 0);
    if (!hd_iop || !sq_iop || !tmp) {
        fail("IOP heap allocation");
        return;
    }
    if (upload(hd, hd_size, hd_iop) == 0 || upload(sq, sq_size, sq_iop) == 0) {
        fail("SifSetDma");
        return;
    }
    /* the sample bank goes through a 0x1000-byte IOP buffer into SPU RAM */
    {
        uint32_t done = 0;

        while (done < bd_size) {
            uint32_t n = bd_size - done > 0x1000 ? 0x1000 : bd_size - done;

            if (upload(bd + done, n, tmp) == 0) {
                fail("SifSetDma bd");
                return;
            }
            voice_transfer(tmp, spu + (int32_t)done, n);
            done += n;
        }
    }

    /* bad requests first */
    check_eq("hsyn load of a bank that was never set up", func_0043c230(5, -1, 7, 0, 0, 0), -201);
    bank = func_0043c230(3, -1, hd_iop, (int32_t)hd_size, spu, (int32_t)bd_size);
    check_ge0("hsyn bank entry", bank);
    inst = func_0043c230(5, -1, bank, 0, 0, 0);
    check_ge0("hsyn instance", inst);
    sqd = func_0043c3b0(0, -1, sq_iop, (int32_t)sq_size);
    check_ge0("sesq data", sqd);
    player = func_0043c3b0(5, inst, sqd, 0);
    check_eq("sesq player handle is the instance handle", player, inst);
    check_eq("play on an unknown player", func_0043c518(inst + 5, 0, 10, 0, 0), -101);
    check_eq("play of a sequence the data lacks", func_0043c518(player, 0, 10, 0, 9), -101);
    check_eq("stop with the wrong type is ignored", func_0043c5e8(player, 1, 3), 0);

    /* start: the result is the play handle */
    h = func_0043c518(player, 0, 10, 0, 0);
    check_eq("first play handle", h, 0);
    rms = render_rms(20);
    snprintf(what, sizeof what, "%s: note sounds (rms %.4f)", name, rms);
    if (rms < (strict ? 0.01 : 0.0003)) {
        fail(what);
    }
    if (strict) {
        rms = render_rms(20);
        if (rms < 0.01) {
            fail("note still sounds at 40 ms");
        }
        render_rms(40); /* the note off at 50 ms, its release, and a margin */
    } else {
        /* the second note of spu/test sequence 0 starts at 100 ms */
        rms = render_rms(200);
        snprintf(what, sizeof what, "%s: audio between 20 and 220 ms (rms %.4f)", name, rms);
        if (rms < 0.0003) {
            fail(what);
        }
    }
    if (strict) {
        rms = render_rms(20);
        snprintf(what, sizeof what, "%s: silent after the note off (rms %.5f)", name, rms);
        if (rms > 0.0005) {
            fail(what);
        }
    }
    if (strict) {
        check_eq("stop of a finished sequence", func_0043c5e8(player, 1, 10), -299);
    }
    check_eq("release of the play handle", func_0043c518(player, 2, 10, h, 0), 0);

    /* a looping note runs until a stop command */
    h = func_0043c518(player, 0, 10, 0, 0);
    check_eq("second play handle", h, 0);
    render_rms(10);
    check_eq("stop of a running sequence", func_0043c5e8(player, 1, 10), 0);
    render_rms(30);
    check_eq("release", func_0043c518(player, 2, 10, h, 0), 0);
    if (strict) {
        rms = render_rms(20);
        if (rms > 0.0005) {
            fail("silent after the stop command");
        }
    }

    /* teardown in the order sdkSnd's sdkSndReleaseDecodeSlot uses */
    check_eq("free sesq player", func_0043c470(5, player), 0);
    check_eq("free sesq data", func_0043c470(0, sqd), 0);
    check_eq("free hsyn instance", func_0043c308(5, inst), 0);
    check_eq("free hsyn bank", func_0043c308(3, bank), 0);
    check_eq("IOP free", func_00429d90(hd_iop), 0);
    check_eq("IOP free again fails", func_00429d90(hd_iop), -1);
    func_00429d90(sq_iop);
    func_00429d90(tmp);
}

/* ------------------------------------------------------------------ sequence events */

/* four blocks of a square wave of 28 samples per period (14 of +7, 14 of -7, shift 2): 1714 Hz at the
 * VAG's 48 kHz, looping */
static uint32_t make_bd_square(uint8_t *bd)
{
    int b, i;

    for (b = 0; b < 4; b++) {
        uint8_t *blk = bd + b * 16;

        blk[0] = 2;
        blk[1] = b == 0 ? 0x04 : (b == 3 ? 0x03 : 0x00);
        for (i = 0; i < 14; i++) {
            blk[2 + i] = i < 7 ? 0x77 : 0x99;
        }
    }
    return 64;
}

/* Brings up the synthetic bank with the given bank data and sequence events and starts the sequence. */
static int start_events(const uint8_t *bd, uint32_t bd_size, const uint8_t *ev, uint32_t evlen, int32_t *handles)
{
    uint8_t hd[0x100], sq[0x100];
    uint32_t hd_size = make_hd(hd, 64);
    uint32_t sq_size = make_sq_ev(sq, ev, evlen);
    int32_t spu, hd_iop, sq_iop, tmp, bank, inst, sqd, player;

    p4sdr_reset();
    func_00430e28();
    func_0043bb70((void *)hd, 32, 0);
    spu = func_0043bcb8((int32_t)bd_size);
    hd_iop = func_00429d10(0, hd_size, 0);
    sq_iop = func_00429d10(0, sq_size, 0);
    tmp = func_00429d10(0, 0x1000, 0);
    if (!hd_iop || !sq_iop || !tmp || upload(hd, hd_size, hd_iop) == 0 || upload(sq, sq_size, sq_iop) == 0 ||
        upload(bd, bd_size, tmp) == 0) {
        fail("event test setup");
        return -1;
    }
    voice_transfer(tmp, spu, bd_size);
    bank = func_0043c230(3, -1, hd_iop, (int32_t)hd_size, spu, (int32_t)bd_size);
    inst = func_0043c230(5, -1, bank, 0, 0, 0);
    sqd = func_0043c3b0(0, -1, sq_iop, (int32_t)sq_size);
    player = func_0043c3b0(5, inst, sqd, 0);
    if (bank < 0 || inst < 0 || sqd < 0 || player < 0) {
        fail("event test bank");
        return -1;
    }
    handles[0] = player;
    handles[1] = sqd;
    handles[2] = inst;
    handles[3] = bank;
    return func_0043c518(player, 0, 10, 0, 0);
}

static void stop_events(const int32_t *handles)
{
    func_0043c470(5, handles[0]);
    func_0043c470(0, handles[1]);
    func_0043c308(5, handles[2]);
    func_0043c308(3, handles[3]);
}

/* Zero crossings per second of the left channel over `ms` milliseconds. */
static double render_crossings(int ms)
{
    static float buf[STEP * 2];
    int frames = ms * 48, done, cross = 0;
    float last = 0.0f;

    for (done = 0; done < frames; done += STEP) {
        int i;

        p4snd_render(buf, STEP);
        for (i = 0; i < STEP; i++) {
            float v = buf[i * 2];

            if ((v > 0.001f && last < -0.001f) || (v < -0.001f && last > 0.001f)) {
                cross++;
            }
            if (v > 0.001f || v < -0.001f) {
                last = v;
            }
        }
    }
    return (double)cross * 1000.0 / (double)ms;
}

/* A loop end (meta 1) repeats the events from its target: the note (10 ms on, then 20 ms to the loop end,
 * which jumps back to the note on at once, so one note every 30 ms) plays three times with 2 repeats. */
static void test_loop(void)
{
    uint8_t bd[64];
    static const uint8_t ev[] = {
        0x00, 0xc0, 0x7f,                         /* loop start marker */
        0x00, 0x90, 0x00, 0x00, 0x7f,             /* note on, at event byte 3 (status at 4) */
        0x0a, 0xa0, 0x00, 0x00, 0x00,             /* 10 ms: note off */
        0x14, 0xc0, 0x01, 0x05, 12, 0x00, 0x02,   /* 20 ms: loop end 5, to status byte 12 - 8 = 4, 2 repeats */
        0x00, 0xff,
    };
    int32_t h[4];
    int play;
    double r;

    fprintf(stderr, "sdr_test: sequence loop\n");
    make_bd(bd);
    play = start_events(bd, 64, ev, sizeof ev, h);
    if (play < 0) {
        return;
    }
    r = render_rms(8);
    if (r < 0.01) {
        fail("loop: first note silent");
    }
    render_rms(40); /* to 48 ms: the second note (30..40 ms) is over */
    r = render_rms(4); /* 52..56 ms: before the third note at 60 ms */
    r = render_rms(14); /* 56..70 ms: the third note sounds */
    if (r < 0.005) {
        fail("loop: third note silent (the loop did not repeat twice)");
    }
    render_rms(60);
    r = render_rms(30); /* 130..160 ms: no fourth note */
    if (r > 0.0005) {
        fail("loop: a fourth note sounds (the loop repeated too often)");
    }
    stop_events(h);
}

/* A pitch ramp (control 0x0e: up, by value / 100 semitones over `time` ticks) retunes a playing note and
 * leaves it there; 0x0f goes down. */
static void test_ramp(void)
{
    uint8_t bd[64];
    /* note on, 20 ms, ramp up 1200 cents over 20 ms (value 1200 = 0x04b0, time 20), 80 ms, ramp down 1200
     * cents over 20 ms, the note goes on for a further 100 ms, then off */
    static const uint8_t ev[] = {
        0x00, 0x90, 0x00, 0x00, 0x7f,
        0x14, 0xb0, 0x00, 0x00, 0x0e, 0x14, 0xb0, 0x04,
        0x50, 0xb0, 0x00, 0x00, 0x0f, 0x14, 0xb0, 0x04,
        0x64, 0xa0, 0x00, 0x00, 0x00,
        0x00, 0xff,
    };
    int32_t h[4];
    int play;
    double f0, f1, f2;

    fprintf(stderr, "sdr_test: pitch ramp\n");
    make_bd_square(bd);
    play = start_events(bd, 64, ev, sizeof ev, h);
    if (play < 0) {
        return;
    }
    f0 = render_crossings(15);  /* 0..15 ms: the tone's own pitch */
    render_rms(25);             /* to 40 ms: the ramp (20..40 ms) is over */
    f1 = render_crossings(30);  /* 40..70 ms: an octave up */
    render_rms(50);             /* to 120 ms: the second ramp (100..120 ms) */
    f2 = render_crossings(40);  /* 120..160 ms: back down */
    fprintf(stderr, "sdr_test: crossings per second %.0f, %.0f, %.0f\n", f0, f1, f2);
    if (f0 < 2500.0 || f0 > 4400.0) {
        fail("ramp: unexpected base frequency");
    }
    if (f1 < f0 * 1.8 || f1 > f0 * 2.2) {
        fail("ramp: the note is not an octave up after the up ramp");
    }
    if (f2 < f0 * 0.9 || f2 > f0 * 1.1) {
        fail("ramp: the note did not come back down");
    }
    stop_events(h);
}

/* The reverb unit with the room preset's registers (the PS1 / libsd values; the work area is 0x26c0 bytes): a
 * note whose tone routes to the reverb leaves a tail after it ends, the same note without the wet switches
 * does not, and the tail dies away. */
static void test_reverb(void)
{
    static const int16_t room[32] = {
        0x007d, 0x005b, 0x6d80, 0x54b8, (int16_t)0xbed0, 0x0000, 0x0000, (int16_t)0xba80, 0x5800, 0x5300,
        0x04d6, 0x0333, 0x03f0, 0x0227, 0x0374, 0x01ef, 0x0336, 0x01b7, 0x0335, 0x01b6,
        0x0334, 0x01b5, 0x0334, 0x01b5, 0x0334, 0x01b5, 0x01b4, 0x0136, 0x00b8, 0x005c,
        (int16_t)0x8000, (int16_t)0x8000,
    };
    static const uint8_t ev[] = { 0x00, 0x90, 0x00, 0x00, 0x7f, 0x14, 0xa0, 0x00, 0x00, 0x00, 0x00, 0xff };
    uint8_t bd[64];
    int32_t h[4];
    int pass;
    double tail[2], late[2];

    fprintf(stderr, "sdr_test: reverb\n");
    make_bd_square(bd); /* the effect unit works at 24 kHz: a tone it can pass */
    for (pass = 0; pass < 2; pass++) {
        tone_mix = pass ? 0x0f : 0x03; /* pass 1: dry and reverb, both sides */
        if (start_events(bd, 64, ev, sizeof ev, h) < 0) {
            return;
        }
        p4snd_reverb_setup(room, 0x26c0);
        p4snd_reverb_enable(1);
        p4snd_reverb_volume(0x3fff, 0x3fff);
        render_rms(30);          /* the note (20 ms) and its release */
        tail[pass] = render_rms(60); /* 30..90 ms: the first reflections arrive after about 40 ms */
        render_rms(400);
        late[pass] = render_rms(100);
        stop_events(h);
    }
    tone_mix = 0x03;
    fprintf(stderr, "sdr_test: tail rms %.5f dry only, %.5f routed; later %.5f, %.5f\n", tail[0], tail[1], late[0], late[1]);
    if (tail[0] > 0.0002) {
        fail("reverb: a tone without the reverb switches leaves a tail");
    }
    if (tail[1] < 0.002) {
        fail("reverb: a tone routed to the reverb leaves no tail");
    }
    if (!(late[1] < tail[1])) {
        fail("reverb: the tail does not die away");
    }
    p4snd_reverb_enable(0);
}

/* The game's own reverb setup (mdlRenderInitializeBuffers): mode 5 (hall) with the work area cleared, effect volume 60 of
 * 127, effect on; the presets come from LIBSD.IRX on the disc. A routed tone then leaves a tail. */
static void test_reverb_game(void)
{
    static const uint8_t ev[] = { 0x00, 0x90, 0x00, 0x00, 0x7f, 0x14, 0xa0, 0x00, 0x00, 0x00, 0x00, 0xff };
    uint8_t bd[64];
    int32_t h[4];
    double tail, late;

    fprintf(stderr, "sdr_test: reverb, the game's commands and the disc's presets\n");
    make_bd_square(bd);
    tone_mix = 0x0f;
    if (start_events(bd, 64, ev, sizeof ev, h) < 0) {
        tone_mix = 0x03;
        return;
    }
    check_eq("reverb mode", func_0043c0c0(3, 0x83, 0x105, 0x3c, 0), 0);
    check_eq("reverb depth", func_0043c0c0(3, 0x84, 0x3c, 0x3c, 0), 0);
    check_eq("reverb on", func_0043c0c0(3, 0x82, 1, 0, 0), 0);
    render_rms(30);
    tail = render_rms(150);
    render_rms(800);
    late = render_rms(100);
    stop_events(h);
    tone_mix = 0x03;
    fprintf(stderr, "sdr_test: hall tail rms %.5f, later %.5f\n", tail, late);
    if (tail < 0.002) {
        fail("hall reverb: no tail (presets not found?)");
    }
    if (!(late < tail)) {
        fail("hall reverb: the tail does not die away");
    }
}

/* A tone's own pitch (bytes 11 and 12): +12 semitones doubles the frequency, -12 halves it, and 64/128 is half a
 * semitone. */
static void test_tone_pitch(void)
{
    static const uint8_t ev[] = { 0x00, 0x90, 0x00, 0x00, 0x7f, 0x64, 0xa0, 0x00, 0x00, 0x00, 0x00, 0xff };
    uint8_t bd[64];
    int32_t h[4];
    static const int8_t keys[4] = { 0, 12, -12, 0 };
    static const int8_t fines[4] = { 0, 0, 0, 64 };
    double f[4];
    int i;

    fprintf(stderr, "sdr_test: tone pitch\n");
    make_bd_square(bd);
    for (i = 0; i < 4; i++) {
        tone_key = keys[i];
        tone_fine = fines[i];
        if (start_events(bd, 64, ev, sizeof ev, h) < 0) {
            break;
        }
        render_rms(10);
        f[i] = render_crossings(60);
        stop_events(h);
    }
    tone_key = tone_fine = 0;
    fprintf(stderr, "sdr_test: crossings per second %.0f, %.0f, %.0f, %.0f\n", f[0], f[1], f[2], f[3]);
    if (f[0] < 2500.0 || f[1] < f[0] * 1.8 || f[1] > f[0] * 2.2 || f[2] < f[0] * 0.45 || f[2] > f[0] * 0.55) {
        fail("tone pitch: an octave up or down is not twice or half the frequency");
    }
    if (f[3] < f[0] * 1.02 || f[3] > f[0] * 1.04) {
        fail("tone pitch: a fine tune of 64/128 semitone is not half a semitone (a factor of 1.029)");
    }
}

static uint8_t *read_disc(const char *path, uint32_t *size)
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
    buf = malloc((size_t)end);
    while (buf != NULL && got < end) {
        int n = vfs_read(fd, buf + got, (int)(end - got > (1 << 20) ? (1 << 20) : end - got));

        if (n <= 0) {
            break;
        }
        got += n;
    }
    vfs_close(fd);
    if (got != end) {
        free(buf);
        return NULL;
    }
    *size = (uint32_t)end;
    return buf;
}

int main(void)
{
    const char *iso = getenv("P4PC_ISO");
    uint8_t hd[0x100], sq[0x100], bd[64];
    uint32_t hd_size, sq_size, bd_size;
    int skipped = 0;

    p4snd_use_offline();

    hd_size = make_hd(hd, 64);
    sq_size = make_sq(sq);
    bd_size = make_bd(bd);
    run_bank("synthetic bank", hd, hd_size, bd, bd_size, sq, sq_size, 1);

    test_loop();
    test_ramp();
    test_reverb();
    test_tone_pitch();

    /* commands of modules the game does not use are refused */
    {
        int32_t a[14] = {0};

        check_eq("unused module", p4sdr_command(0x02000000u, 0, a, 14), -310);
    }

    if (iso != NULL && iso[0] != '\0' && vfs_mount(iso, NULL) == 0) {
        uint32_t dhd, dbd, dsq;
        uint8_t *d_hd = read_disc("spu/test.hd", &dhd);
        uint8_t *d_bd = read_disc("spu/test.bd", &dbd);
        uint8_t *d_sq = read_disc("spu/test.sq", &dsq);

        test_reverb_game();
        if (d_hd == NULL || d_bd == NULL || d_sq == NULL) {
            fail("spu/test.* not found on the disc");
        } else {
            run_bank("disc bank spu/test", d_hd, dhd, d_bd, dbd, d_sq, dsq, 0);
        }
        free(d_hd);
        free(d_bd);
        free(d_sq);
    } else {
        skipped = 1;
        fprintf(stderr, "sdr_test: P4PC_ISO unset, disc bank skipped\n");
    }
    p4snd_shutdown();
    if (failures != 0) {
        return 1;
    }
    printf("sdr_test: ok%s\n", skipped ? " (disc part skipped)" : "");
    return 0;
}

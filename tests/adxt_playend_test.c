/* adxt_playend_test: when an ADX voice ends, counted in game updates, with the battle's clocks.
 *
 * Retail runs battle at two VBLANKs per game update. The SPU2 plays through both of them, and the
 * CRI server runs on one of them: the game's first VBLANK handler (FldShadow_PollIntcBit13) returns
 * negative on alternate GS fields, which stops the chain before kwlnEnableInterrupts / ADXM_ExecVint.
 * Collision 900's voice (cue 308, 24 kHz mono) therefore drains about 800 samples per update and its
 * handle is stepped once per update (docs/audio.md, "End of a voice").
 *
 * The test drives the real kernel clock: per update it calls p4pc_wait_frame_boundary twice. Each
 * VBLANK renders that period's audio (P4PC_VIRTUAL_CLOCK=1, p4snd_virtual_tick from the kernel) and
 * runs the VBLANK chain: the game's field gate (FldShadow_InstallIntcHandler) and, behind it, a
 * handler standing in for kwlnEnableInterrupts that asks for one ADXT server pass. A synthetic
 * 24 kHz mono ADX of 37216 samples (no loop, 11 sectors like the voice) arrives in the handle's input
 * joint at update ARRIVE, as a disc read completing. Checks:
 *   - the handle comes from the retail create (func_004d3850 -> adxt_Create) with the battle voice's
 *     work size: file ring 39 sectors, reload threshold 33;
 *   - the field gate passes one VBLANK in two, and every VBLANK renders 48000 * 1001 / 60000 frames;
 *   - DECINFO -> PREP in the arrival pass, PLAYING three passes later (two passes of decoded data
 *     reach 2 * maxdecsmpl = 2432 samples), then PLAYEND_WAIT, then PLAYEND at update ARRIVE + 53,
 *     50 passes after key-on: the renderer reports what played 2.75 passes back in 256-sample
 *     blocks, as retail's PS2RNA does, so the 37216 samples (37376 in whole blocks) are heard in
 *     the 50th pass (collision 900's 37222-sample voice: key-on at anchor+248, PLAYEND at 298);
 *   - each PLAYEND_WAIT pass pads at most maxdecsmpl (1216) samples and flush_nsmpl (+0xa8)
 *     counts them;
 *   - the renderer is stopped and empty at PLAYEND.
 * Exits 77 (CTest skip) when P4PC_ISO or P4PC_ELF is unset, as the other tests that boot the image do. */
#define _FILE_OFFSET_BITS 64
#define _POSIX_C_SOURCE 200809L

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "p4pc/gmem.h"
#include "p4pc/image.h"
#include "p4pc/os.h"
#include "p4pc/platform.h"
#include "p4pc/vblank_hook.h"
#include "guest_test.h"
#include "p4snd.h"

#define SKIP_RETURN 77
#define RATE 24000
#define NSMPL 37216                 /* 1163 blocks of 32 */
#define FILE_BYTES (11 * 0x800)
#define WORK_BYTES 0x1c1d8          /* ACSSND_Init's ADXT work for a two-channel voice at 24 kHz */
#define ARRIVE 5
#define UPDATES 80

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

void ADXT_Init(void);
void ADXT_SetDefSvrFreq(int32_t freq);
void *func_004d3850(int32_t maxnch, void *work, int32_t worksize); /* ADXT_Create (retail wrapper) */
void ADXT_StartSj(void *adxt, SjObj *sj);
void ADXT_SetLnkSw(void *adxt, int32_t sw);
void ADXT_ExecServer(void);
int32_t ADXRNA_GetNumData(void *rna);
SjObj *SJRBF_Create(void *buf, int32_t bsize, int32_t xsize);
int32_t FldShadow_InstallIntcHandler(void);
int32_t func_004214c0(int32_t cause, int32_t (*handler)(int32_t), int32_t next); /* AddIntcHandler */
void *kwlnProbeHeapBudget(void);

static int failures;
static int wakeups;     /* VBLANKs that passed the field gate */
static int vblanks;

static void check(const char *what, long got, long want)
{
    if (got != want) {
        fprintf(stderr, "adxt_playend_test: FAIL %s: got %ld want %ld\n", what, got, want);
        failures++;
    }
}

/* Stands in for kwlnEnableInterrupts (ADXM_ExecVint wakes the CRI server threads). */
static int32_t server_wake(int32_t cause)
{
    (void)cause;
    wakeups++;
    return 0;
}

static void put_be16(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)(v >> 8);
    p[1] = (uint8_t)v;
}

static void put_be32(uint8_t *p, uint32_t v)
{
    put_be16(p, v >> 16);
    put_be16(p + 2, v);
}

/* A plain 4-bit ADX (type 3, version 4, no loop): header, NSMPL / 32 frames, end frame (0x8001). */
static void build_adx(uint8_t *f)
{
    uint8_t *p;
    int b;

    memset(f, 0, FILE_BYTES);
    put_be16(f, 0x8000);
    put_be16(f + 2, 0x24);
    f[4] = 3;
    f[5] = 18;
    f[6] = 4;
    f[7] = 1;
    put_be32(f + 8, RATE);
    put_be32(f + 12, NSMPL);
    put_be16(f + 16, 500);
    f[18] = 4;
    memcpy(f + 0x22, "(c)CRI", 6);
    p = f + 0x28;
    for (b = 0; b < NSMPL / 32; b++, p += 18) {
        put_be16(p, 0x0040);
        memset(p + 2, (b & 1) ? 0x71 : 0x17, 16);
    }
    put_be16(p, 0x8001);
    put_be16(p + 2, 0x000e);
}

static void arrive(SjObj *sj, const uint8_t *file)
{
    int done = 0;

    while (done < FILE_BYTES) {
        SjCk ck;

        sj_get(sj, 0, FILE_BYTES - done, &ck);
        if (ck.len <= 0) {
            break;
        }
        memcpy(P4_G2H(ck.data), file + done, (size_t)ck.len);
        sj_put(sj, 1, &ck);
        done += ck.len;
    }
    check("input joint takes the whole file", done, FILE_BYTES);
}

static int test_main(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    const char *iso = getenv("P4PC_ISO");
    const char *elf = getenv("P4PC_ELF");
    uint8_t *adxt, *file, *jbuf;
    SjObj *sj;
    int u, v, stat, prev = -1;
    int prep = -1, playing = -1, wait = -1, end = -1;
    int pads = 0;

    if (iso == NULL || iso[0] == '\0' || elf == NULL || elf[0] == '\0') {
        fprintf(stderr, "adxt_playend_test: P4PC_ISO or P4PC_ELF unset, skipped\n");
        return SKIP_RETURN;
    }
    p4_os_setenv("P4PC_VIRTUAL_CLOCK", "1", 1);
    p4_os_setenv("P4PC_TRAP_CONTINUE", "1", 0);
    p4pc_trap_configure();
    p4pc_load_image(elf);
    p4pc_apply_patches();
    p4pc_kernel_pre_init();
    kwlnProbeHeapBudget();
    if (p4snd_open() != 0) {
        fprintf(stderr, "adxt_playend_test: FAIL mixer did not open on the virtual clock\n");
        return 1;
    }
    p4pc_vblank_period_hook = p4snd_virtual_tick; /* as p4pc_platform_init installs it */

    /* The VBLANK chain the game builds: the field gate first, the CRI server's wake-up last. */
    if (FldShadow_InstallIntcHandler() != 1 || func_004214c0(2, server_wake, -1) < 0) {
        fprintf(stderr, "adxt_playend_test: FAIL VBLANK handlers\n");
        return 1;
    }

    ADXT_Init();
    ADXT_SetDefSvrFreq(30);
    adxt = func_004d3850(2, p4pc_heap_calloc(1, WORK_BYTES), WORK_BYTES);
    if (adxt == NULL) {
        fprintf(stderr, "adxt_playend_test: FAIL ADXT_Create\n");
        return 1;
    }
    check("file ring bytes (+0x24)", *(int32_t *)(adxt + 0x24), 39 * 0x800);
    check("file ring sectors (+0x3c)", *(int16_t *)(adxt + 0x3c), 39);
    check("reload threshold (+0x3e)", *(int16_t *)(adxt + 0x3e), 33);
    check("server frequency (+0x38)", *(int32_t *)(adxt + 0x38), 30);

    file = malloc(FILE_BYTES);
    build_adx(file);
    jbuf = p4pc_heap_calloc(1, 0x6000 + 0x24);
    sj = SJRBF_Create(jbuf, 0x6000, 0x24);
    ADXT_StartSj(adxt, sj);
    ADXT_SetLnkSw(adxt, 0);

    for (u = 0; u < UPDATES && end < 0; u++) {
        int passes = 0;

        if (u == ARRIVE) {
            arrive(sj, file);
        }
        for (v = 0; v < 2; v++) {
            uint64_t before = p4snd_frames_rendered();
            int woke = wakeups;

            p4pc_wait_frame_boundary();
            vblanks++;
            if (p4snd_frames_rendered() - before != 800 && p4snd_frames_rendered() - before != 801) {
                check("frames rendered by one VBLANK", (long)(p4snd_frames_rendered() - before), 801);
            }
            if (wakeups != woke) {
                int32_t flush = *(int32_t *)(adxt + 0xa8);

                ADXT_ExecServer();
                passes++;
                if (prev == 4) {
                    int32_t pad = *(int32_t *)(adxt + 0xa8) - flush;

                    if (pad <= 0 || pad > 1216) {
                        check("PLAYEND_WAIT pass pads 1..maxdecsmpl samples", pad, 1216);
                    }
                    pads++;
                } else if (*(int32_t *)(adxt + 0xa8) != 0) {
                    check("flush_nsmpl outside PLAYEND_WAIT", *(int32_t *)(adxt + 0xa8), 0);
                }
                prev = (int8_t)adxt[1];
            }
        }
        check("server passes per update", passes, 1);
        stat = (int8_t)adxt[1];
        if (stat == 2 && prep < 0) {
            prep = u;
        } else if (stat == 3 && playing < 0) {
            playing = u;
        } else if (stat == 4 && wait < 0) {
            wait = u;
        } else if (stat == 5) {
            end = u;
        }
    }
    fprintf(stderr, "adxt_playend_test: PREP %d PLAYING %d PLAYEND_WAIT %d PLAYEND %d (arrival %d), flush_nsmpl %d\n",
            prep, playing, wait, end, ARRIVE, *(int32_t *)(adxt + 0xa8));
    check("field gate: wake-ups per VBLANK pair", wakeups * 2, vblanks);
    check("maxdecsmpl (+0x48)", *(int32_t *)(adxt + 0x48), 1216);
    check("PREP update", prep, ARRIVE);
    check("PLAYING update", playing, ARRIVE + 3);
    if (!(wait > playing && wait < ARRIVE + 53)) {
        check("PLAYEND_WAIT update between PLAYING and PLAYEND", wait, ARRIVE + 40);
    }
    check("PLAYEND update", end, ARRIVE + 53);
    if (pads == 0 || *(int32_t *)(adxt + 0xa8) <= 0) {
        check("flush_nsmpl (+0xa8) counts the PLAYEND_WAIT padding", *(int32_t *)(adxt + 0xa8), 1216);
    }
    check("renderer empty at PLAYEND", ADXRNA_GetNumData(P4_G2H(*(uint32_t *)(adxt + 0x0c))), 0);
    free(file);
    if (failures == 0) {
        fprintf(stderr, "adxt_playend_test: ok\n");
    }
    return failures ? 1 : 0;
}

int main(int argc, char **argv)
{
    return p4t_on_guest_stack(test_main, argc, argv);
}

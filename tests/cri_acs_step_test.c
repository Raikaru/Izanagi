/* ACS per-frame steps (src/cri/acs_step.c): the fader tick, the voice-highlight state machine and one
 * ACSSND frame, against recording stubs for their callees. The expected branches are read from the
 * retail asm (0x004bd890, 0x004c1038, 0x004bf960).
 */
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../src/cri/acs_step.c"
#include "guest_test.h"

static int failures;
static char log_buf[4096];
static int log_len;

static void rec(const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    log_len += vsnprintf(log_buf + log_len, sizeof log_buf - (size_t)log_len, fmt, ap);
    va_end(ap);
    if (log_len < (int)sizeof log_buf - 2) {
        log_buf[log_len++] = ' ';
        log_buf[log_len] = 0;
    }
}

static void clear_log(void)
{
    log_len = 0;
    log_buf[0] = 0;
}

#define CHECK(c) do { if (!(c)) { printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #c); failures++; } } while (0)
#define CHECK_LOG(s) do { if (strcmp(log_buf, (s)) != 0) { printf("FAIL %s:%d log\n  got  %s\n  want %s\n", __FILE__, __LINE__, log_buf, (s)); failures++; } } while (0)

static int32_t db_level;      /* what func_004bdaf0 reports */
static int32_t phase_of[16];  /* handle table: phase, wanted flag by handle index (set per test) */
static int32_t want_of[16];
static uint8_t *handles_mem;   /* 16 handles of 8 bytes, guest storage: the tables hold their addresses */
#define HM(i) (handles_mem + 8 * (i))
static int32_t len_a8 = 77, len_f8 = 88;

int32_t func_004bd6a0(uint32_t msg) { rec("err(0x%x)", (unsigned)msg); return 0; }
void func_004bd658(void) { rec("lock"); }
void func_004bd670(void) { rec("unlock"); }
int32_t func_004bd748(uint8_t *f) { return (int8_t)f[0]; }
void func_004bd760(uint8_t *f, int32_t v) { rec("set(%d)", v); W32(f, 0x1c) = v; }
void func_004bd790(uint8_t *f, int32_t to, int32_t steps, int32_t a3, int32_t a4)
{
    (void)f;
    rec("fade(%d,%d,%d,%d)", to, steps, a3, a4);
}
int32_t func_004bdd48(void *f) { (void)f; rec("start_hook"); return 1; }
int32_t func_004bdd78(void *f) { (void)f; rec("end_hook"); return 1; }
int32_t func_004bdaf0(void *f, int32_t level) { (void)f; (void)level; return db_level; }
int32_t func_004bea98(uint32_t snd) { (void)snd; return 1; }
int32_t func_004beec0(uint32_t snd) { (void)snd; return 2; }
static int idx_of(void *h) { return (int)(((uint8_t *)h - handles_mem) / 8); }
int32_t func_004bee38(void *snd) { return phase_of[idx_of(snd)]; }
int32_t func_004bedf0(void *snd) { return want_of[idx_of(snd)]; }
void func_004c0450(void *snd) { (void)snd; rec("c0450"); }
void func_004c0518(void *snd, void *slot) { rec("c0518(%d)", (int)(((uint8_t *)slot - (uint8_t *)snd - 0x18) / 12)); }
void func_004c12a0(void *blk, int32_t state) { rec("state(%d)", state); B8(blk, 0x50) = (int8_t)state; }
int32_t func_004c12a8(void *blk) { (void)blk; return len_a8; }
int32_t func_004c12f8(void *blk) { (void)blk; return len_f8; }

static void fader_setup(uint8_t *f, int st, int32_t tick, int32_t delay)
{
    memset(f, 0, 0x44);
    f[0] = (uint8_t)st;
    W32(f, 8) = tick;
    W32(f, 0x10) = delay;
}

static void test_fader(void)
{
    static uint8_t f[0x44];

    /* idle: nothing, tick untouched */
    fader_setup(f, 0, 5, 0);
    clear_log();
    func_004bd890(f);
    CHECK(W32(f, 8) == 5);
    CHECK_LOG("");

    /* the start delay holds the tick count only */
    fader_setup(f, 1, 0, 2);
    func_004bd890(f);
    CHECK(W32(f, 8) == 1 && f[0] == 1);
    CHECK_LOG("");

    /* second tick: state 1 -> 2 with the start hook; up-step not yet at the target keeps state 2 */
    W32(f, 4) = 50;
    W32(f, 0x14) = 2;
    W32(f, 0x20) = 100;
    W32(f, 0x24) = -100;
    db_level = -300;
    func_004bd890(f);
    CHECK(f[0] == 2 && W32(f, 0x20) == 150 && W32(f, 0x1c) == -300);
    CHECK_LOG("start_hook ");

    /* up-step reaching the target: the target is set, state 3, tick noted */
    db_level = -100;
    clear_log();
    func_004bd890(f);
    CHECK(f[0] == 3 && W32(f, 0xc) == 3 && W32(f, 0x1c) == -100);
    CHECK_LOG("set(-100) ");

    /* state 3 waits +0x14 ticks, then goes idle with the end hook */
    clear_log();
    func_004bd890(f);
    CHECK(f[0] == 3);
    CHECK_LOG("");
    func_004bd890(f);
    CHECK(f[0] == 0);
    CHECK_LOG("end_hook ");

    /* down-step: reached when the level is at or below the target; clamps at 0 */
    fader_setup(f, 2, 0, 0);
    W32(f, 4) = -500;
    W32(f, 0x14) = 2;
    W32(f, 0x20) = 100;
    W32(f, 0x24) = -400;
    db_level = -200;
    clear_log();
    func_004bd890(f);
    CHECK(W32(f, 0x20) == 0 && f[0] == 2);
    CHECK_LOG("");
    db_level = -400;
    func_004bd890(f);
    CHECK(f[0] == 3);
    CHECK_LOG("set(-400) ");

    /* zero step finishes at once; the linear level clamps at 0x316938 */
    fader_setup(f, 2, 0, 0);
    W32(f, 0x20) = 0x316000;
    W32(f, 4) = 0;
    W32(f, 0x14) = 2;
    W32(f, 0x24) = -10;
    db_level = -50;
    clear_log();
    func_004bd890(f);
    CHECK(f[0] == 3 && W32(f, 0x20) == 0x316000);
    CHECK_LOG("set(-10) ");
    fader_setup(f, 2, 0, 0);
    W32(f, 0x20) = 0x316900;
    W32(f, 4) = 0x1000;
    W32(f, 0x24) = 0;
    db_level = -1;
    clear_log();
    func_004bd890(f);
    CHECK(W32(f, 0x20) == 0x316938 && f[0] == 2);
}

static void hl_setup(uint8_t *blk, uint32_t *tbl, uint8_t *fdr, int st)
{
    memset(blk, 0, 0x60);
    memset(tbl, 0, 16 * 4);
    U32(blk, 0) = P4_H2G(tbl);
    U32(blk, 4) = P4_H2G(fdr);
    B8(blk, 0x50) = (int8_t)st;
    W32(blk, 0x54) = 30;
    W32(blk, 0x58) = 60;
    W32(blk, 0x5c) = 120;
    memset(fdr, 0, 0x44);
    memset(phase_of, 0, sizeof phase_of);
    memset(want_of, 0, sizeof want_of);
}

static void test_highlight(void)
{
    uint8_t *blk = p4t_galloc(0x60, 16), *fdr = p4t_galloc(0x44, 16);
    uint32_t *tbl = p4t_galloc(16 * 4, 16);

    /* idle, nothing wanted: just the fader tick */
    hl_setup(blk, tbl, fdr, 0);
    clear_log();
    func_004c1038(blk);
    CHECK_LOG("");
    CHECK(B8(blk, 0x50) == 0);

    /* idle, a playing handle in phase 1 reports func_004bedf0 == 0: fade down by +0x5c over +0x54 */
    hl_setup(blk, tbl, fdr, 0);
    tbl[3] = P4_H2G(HM(3));
    phase_of[3] = 1;
    want_of[3] = 0;
    clear_log();
    func_004c1038(blk);
    CHECK_LOG("fade(-120,30,0,0) state(1) ");

    /* phase 0 and 3 and a non-zero func_004bedf0 want nothing */
    hl_setup(blk, tbl, fdr, 0);
    tbl[3] = P4_H2G(HM(3));
    tbl[5] = P4_H2G(HM(5));
    phase_of[3] = 3;
    phase_of[5] = 2;
    want_of[5] = 1;
    clear_log();
    func_004c1038(blk);
    CHECK_LOG("");

    /* fading down, nothing wanted any more: fade back up over func_004c12a8, state 3 */
    hl_setup(blk, tbl, fdr, 1);
    clear_log();
    func_004c1038(blk);
    CHECK_LOG("fade(0,77,0,0) state(3) ");

    /* fading down, still wanted, fader stopped (state byte 0): held (2); running fader: unchanged */
    hl_setup(blk, tbl, fdr, 1);
    tbl[0] = P4_H2G(HM(0));
    phase_of[0] = 2;
    clear_log();
    func_004c1038(blk);
    CHECK_LOG("state(2) ");
    hl_setup(blk, tbl, fdr, 1);
    tbl[0] = P4_H2G(HM(0));
    phase_of[0] = 2;
    fdr[0] = 1;
    W32(fdr, 0x10) = 1000;
    clear_log();
    func_004c1038(blk);
    CHECK_LOG("");

    /* held and nothing wanted: fade up over +0x58, state 3 */
    hl_setup(blk, tbl, fdr, 2);
    clear_log();
    func_004c1038(blk);
    CHECK_LOG("fade(0,60,0,0) state(3) ");

    /* fading up, wanted again: fade down over func_004c12f8; else idle once the fader has stopped */
    hl_setup(blk, tbl, fdr, 3);
    tbl[1] = P4_H2G(HM(1));
    phase_of[1] = 1;
    clear_log();
    func_004c1038(blk);
    CHECK_LOG("fade(-120,88,0,0) state(1) ");
    hl_setup(blk, tbl, fdr, 3);
    clear_log();
    func_004c1038(blk);
    CHECK_LOG("state(0) ");
    hl_setup(blk, tbl, fdr, 3);
    fdr[0] = 3;
    W32(fdr, 0x14) = 1000;
    clear_log();
    func_004c1038(blk);
    CHECK_LOG("");
}

static int32_t vstat;
static int32_t v_status(void *o) { (void)o; rec("status"); return vstat; }
static void v_stop(void *o, int32_t a) { (void)o; rec("stop(%d)", a); }

static void test_snd(void)
{
    uint8_t *snd = p4t_galloc(0x100, 16), *fdr0 = p4t_galloc(0x44, 16), *fdr1 = p4t_galloc(0x44, 16), *obj = p4t_galloc(16, 16);
    uint32_t *vtbl = p4t_galloc(16 * 4, 16);

    vtbl[9] = P4_FN_H2G(v_status);
    vtbl[7] = P4_FN_H2G(v_stop);
    U32(obj, 0) = P4_H2G(vtbl);

    memset(snd, 0, 0x100);
    clear_log();
    func_004bf960(snd);
    CHECK_LOG("err(0x757288) ");

    /* initialised but locked out (+0x60): lock and unlock only */
    W32(snd, 0x50) = 1;
    W32(snd, 0x60) = 1;
    clear_log();
    func_004bf960(snd);
    CHECK_LOG("lock unlock ");
    W32(snd, 0x60) = 0;

    /* not stream style (+0xa4 != 1): with +0xc8 == 1 every buffer slot is serviced */
    W32(snd, 0xa0) = 2;
    W32(snd, 200) = 1;
    clear_log();
    func_004bf960(snd);
    CHECK_LOG("lock c0518(0) c0518(1) unlock ");
    W32(snd, 200) = 0;
    clear_log();
    func_004bf960(snd);
    CHECK_LOG("lock unlock ");

    /* stream style, no pending switch: slots flagged in use (byte +0x1c + 12 * i) are ticked and
     * serviced, then c0450 */
    snd[0xa4] = 1;
    U32(snd, 0x20) = P4_H2G(fdr0);
    U32(snd, 0x20 + 12) = P4_H2G(fdr1);
    snd[0x1c + 12] = 1;
    clear_log();
    func_004bf960(snd);
    CHECK_LOG("lock c0518(1) c0450 unlock ");

    /* a pending switch (+0x64) with the current buffer's fader idle asks the voice object (+8) for its
     * status; anything but 2 leaves at once, 2 stops it and clears the switch */
    U32(snd, 8) = P4_H2G(obj);
    W32(snd, 0x64) = 1;
    W32(snd, 0x10) = 0;
    vstat = 1;
    clear_log();
    func_004bf960(snd);
    CHECK_LOG("lock status unlock ");
    CHECK(W32(snd, 0x64) == 1);
    vstat = 2;
    clear_log();
    func_004bf960(snd);
    CHECK_LOG("lock status stop(0) c0518(1) c0450 unlock ");
    CHECK(W32(snd, 0x64) == 0);

    /* a pending switch with the current buffer's fader running (state 1) skips the status query */
    W32(snd, 0x64) = 1;
    W32(snd, 0x10) = 1;
    fdr1[0] = 1;
    W32(fdr1, 0x10) = 1000;
    clear_log();
    func_004bf960(snd);
    CHECK_LOG("lock c0518(1) c0450 unlock ");
    CHECK(W32(snd, 0x64) == 1);
}

int main(void)
{
    if (p4t_guest_map(0x00757000, 0x1000) != 0) {
        return 1;
    }
    handles_mem = p4t_galloc(16 * 8, 16);
    test_fader();
    test_highlight();
    test_snd();
    if (failures) {
        return 1;
    }
    printf("cri_acs_step: ok\n");
    return 0;
}

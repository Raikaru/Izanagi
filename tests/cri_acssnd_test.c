/* ACSSND creation (src/cri/acssnd.c): object layout, work-area carving and error paths against
 * recording stubs for the voice objects, fader and error report. */
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../src/cri/acssnd.c"
#include "guest_test.h"

static int fails;
static char log_buf[4096];
static int log_len;
static uint32_t last_err;   /* the message's guest address */

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

#define CHECK_LOG(s) do { if (strcmp(log_buf, (s)) != 0) { printf("FAIL line %d: log\n  got  %s\n  want %s\n", __LINE__, log_buf, (s)); fails++; } } while (0)
static int err_n, finish_n, fail_voice, fail_init, fail_fdr;
static uint32_t *vt;
static uint8_t *init_work[4];
static int32_t init_size[4];
static int init_n;

#define CHECK(c) do { if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); fails++; } } while (0)

void func_0043f9c8(void *d, int32_t c, uint32_t n) { memset(d, c, n); }
int32_t func_004bd6a0(uint32_t m) { last_err = m; err_n++; return 0; }
void func_004be9c0(void *s) { (void)s; finish_n++; }
static int fader_state_of(uint8_t *f) { return (int8_t)f[0]; }
void func_004bd658(void) { rec("lock"); }
void func_004bd670(void) { rec("unlock"); }
int32_t func_004bd748(uint8_t *f) { return fader_state_of(f); }
int32_t func_004bd758(uint8_t *f) { return W32(f, 0x1c); }
void func_004bd760(uint8_t *f, int32_t v) { (void)f; rec("set(%d)", v); }
void func_004bd790(uint8_t *f, int32_t to, int32_t steps, int32_t a3, int32_t a4)
{
    (void)f;
    rec("fade(%d,%d,%d,%d)", to, steps, a3, a4);
}
void func_004bd850(uint8_t *f) { (void)f; rec("reverse"); }
void func_004bd9d0(uint8_t *f, int32_t c) { (void)f; rec("curve(%d)", c); }
void func_004bd9d8(uint8_t *f, uint32_t fn, uint32_t obj, uint32_t ev) { (void)f; (void)obj; rec("hs(%d,%d)", P4_FN_G2H(fn) == (void *)func_004c0658, (int)ev); }
void func_004bd9e8(uint8_t *f, uint32_t fn, uint32_t obj, uint32_t ev) { (void)f; (void)obj; rec("he(%d,%d)", P4_FN_G2H(fn) == (void *)func_004c0658, (int)ev); }
int32_t func_004bf930(uint32_t snd) { return B8(P4_G2H(snd), 0xa4); }
void func_004c01c0(uint32_t snd, int32_t v) { B8(P4_G2H(snd), 0xa4) = (int8_t)v; rec("a4(%d)", v); }
int32_t d43f8_ret;
int32_t func_004d43f8(void *h) { (void)h; return d43f8_ret; }
void *func_004deec0(void *aixp, int32_t i) { rec("aixp_adxt(%d)", i); return aixp; }
void func_004db250(void *adxt, void *work, int32_t size) { rec("ahx_attach(0x%x,%d)", (unsigned)P4_H2G(adxt), size); (void)work; }
void func_004db398(void *adxt) { rec("ahx_detach(0x%x)", (unsigned)P4_H2G(adxt)); }
int32_t func_004be3a8(void *voice, int32_t track, int32_t vol) { (void)voice; rec("trkvol(%d,%d)", track, vol); return 0; }
static int32_t vinit(void *o, void *cfg, void *w, int32_t sz)
{
    (void)o; (void)cfg;
    if (init_n < 4) { init_work[init_n] = w; init_size[init_n] = sz; }
    init_n++;
    return !fail_init;
}
static void *mkvoice(void *w, int sz)
{
    uint32_t *o = (uint32_t *)w;
    (void)sz;
    if (fail_voice) return 0;
    vt[0] = P4_FN_H2G(vinit);
    o[0] = P4_H2G(vt);
    return o;
}
void *func_004be0e8(void *w, uint32_t sz) { return mkvoice(w, (int)sz); }
void *func_004bdda8(void *w, uint32_t sz) { return mkvoice(w, (int)sz); }
void *func_004bd6b8(void *w, uint32_t sz) { return fail_fdr ? 0 : (sz == 68 ? w : 0); }

/* Everything the code under test reaches through a guest address lives in guest storage (allocated in main). */
static uint8_t *voice_mem, *fdr_mem;
#define VOICE(i) (voice_mem + 16 * (i))
#define FDR(i) (fdr_mem + 0x44 * (i))
static uint32_t *vtbl;
static int32_t vstatus[2];

static int vidx(void *o) { return o == (void *)VOICE(0) ? 0 : 1; }
static int32_t v_startA(void *o, int32_t a) { rec("startA(%d,%d)", vidx(o), a); return 0; }
static int32_t v_startB(void *o, int32_t a, int32_t b) { rec("startB(%d,%d,%d)", vidx(o), a, b); return 0; }
static int32_t v_startC(void *o, int32_t a, int32_t b) { rec("startC(%d,%d,%d)", vidx(o), a, b); return 0; }
static int32_t v_startD(void *o, int32_t a, int32_t b) { rec("startD(%d,%d,%d)", vidx(o), a, b); return 0; }
static int32_t v_pause(void *o) { rec("pause(%d)", vidx(o)); return 0; }
static int32_t v_stop(void *o, int32_t a) { rec("stop(%d,%d)", vidx(o), a); return 0; }
static int32_t v_status(void *o) { return vstatus[vidx(o)]; }
static int32_t v_vol(void *o, int32_t a) { rec("vol(%d,%d)", vidx(o), a); return 0; }
static int32_t v_trk(void *o, int32_t t, int32_t a) { rec("trk(%d,%d,%d)", vidx(o), t, a); return 0; }
static int32_t v_loop(void *o, int32_t a) { rec("loop(%d,%d)", vidx(o), a); return 0; }
static int32_t v_handle(void *o) { return 0x1000 + vidx(o); }

static void user_cb(void *o, int32_t ev) { rec("usercb(%d,%d)", o != 0, ev); }

static void snd_setup(uint8_t *snd, int n)
{
    int i;

    memset(snd, 0, 0x100);
    vtbl[2] = P4_FN_H2G(v_startA);
    vtbl[3] = P4_FN_H2G(v_startB);
    vtbl[4] = P4_FN_H2G(v_startC);
    vtbl[5] = P4_FN_H2G(v_startD);
    vtbl[6] = P4_FN_H2G(v_pause);
    vtbl[7] = P4_FN_H2G(v_stop);
    vtbl[9] = P4_FN_H2G(v_status);
    vtbl[10] = P4_FN_H2G(v_vol);
    vtbl[12] = P4_FN_H2G(v_trk);
    vtbl[14] = P4_FN_H2G(v_loop);
    vtbl[15] = P4_FN_H2G(v_handle);
    memset(fdr_mem, 0, 2 * 0x44);
    W32(snd, 0x50) = 1;
    W32(snd, 0xa0) = n;
    W32(snd, 0x5c) = n == 2;
    for (i = 0; i < n; i++) {
        U32(VOICE(i), 0) = P4_H2G(vtbl);
        U32(snd, i * 4) = P4_H2G(VOICE(i));
        U32(snd, 0x18 + i * 12) = P4_H2G(VOICE(i));
        U32(snd, 0x20 + i * 12) = P4_H2G(FDR(i));
    }
    U32(snd, 8) = P4_H2G(VOICE(0));
    W32(snd, 0xa8) = 30;
    W32(snd, 0xac) = 40;
    W32(snd, 0xb0) = 50;
    W32(snd, 0xb8) = 60;
}

static void test_api(void)
{
    uint8_t *snd = p4t_galloc(0x100, 16);
    int32_t p[8] = { 0, 11, 22, 33, 44, 55, 66, 77 };

    /* not initialised: each reports its text and does nothing */
    memset(snd, 0, 0x100);
    clear_log();
    ACSSND_Stop(snd);
    CHECK(last_err == 0x00756e50u);
    ACSSND_SetOutVol(snd, 0);
    CHECK(last_err == 0x00756f48u);
    ACSSND_Start(snd, p);
    CHECK(last_err == 0x007572c0u);
    CHECK(ACSSND_GetNumAdxt(snd) == 0 && last_err == 0x00757140u);
    CHECK(ACSSND_GetNumAixp(snd) == 0 && last_err == 0x00757218u);
    CHECK_LOG("");

    /* Stop: the current slot fades out; with the lock-out set the voice is paused and muted at once */
    snd_setup(snd, 1);
    clear_log();
    ACSSND_Stop(snd);
    CHECK_LOG("lock a4(1) fade(-960,30,0,60) hs(1,0) he(1,3) unlock ");
    CHECK(snd[0x1c] == 1 && snd[0x1d] == 2);
    snd_setup(snd, 2);
    W32(snd, 0x60) = 1;
    W32(snd, 0x10) = 1;
    W32(snd, 0x14) = 0;
    clear_log();
    ACSSND_Stop(snd);
    CHECK_LOG("lock a4(1) fade(-960,30,0,60) hs(1,0) he(1,3) pause(1) set(-960) pause(0) set(-960) unlock ");

    /* SetOutVol: fade to the level over +0xb0 unless the slot is already fading out */
    snd_setup(snd, 1);
    clear_log();
    ACSSND_SetOutVol(snd, -200);
    CHECK_LOG("lock a4(1) fade(-200,50,0,60) hs(1,2) he(1,5) unlock ");
    CHECK(W32(snd, 0xbc) == -200 && snd[0x1c] == 3 && snd[0x1d] == 0);
    snd[0x1c] = 1;
    clear_log();
    ACSSND_SetOutVol(snd, -100);
    CHECK_LOG("lock unlock ");
    CHECK(W32(snd, 0xbc) == -100);

    /* SetTrackVol: AIXP kind, table, range; applied to every voice at output volume + level */
    snd_setup(snd, 2);
    W32(snd, 0x94) = 1;
    W32(snd, 0x84) = 4;
    W32(snd, 0xbc) = -10;
    {
        static int32_t trk[4];

        U32(snd, 0xc0) = P4_H2G(trk);
        clear_log();
        ACSSND_SetTrackVol(snd, 2, -30);
        CHECK(trk[2] == -30);
        CHECK_LOG("trk(0,2,-40) trk(1,2,-40) ");
        /* stream style (+0xa4 == 1) applies only while locked out */
        snd[0xa4] = 1;
        clear_log();
        ACSSND_SetTrackVol(snd, 1, 7);
        CHECK(trk[1] == 7);
        CHECK_LOG("");
        W32(snd, 0x60) = 1;
        ACSSND_SetTrackVol(snd, 3, 5);
        CHECK(trk[3] == 5);
        CHECK_LOG("trk(0,3,-5) trk(1,3,-5) ");
        CHECK(func_004bf0e8(snd, 3) == 5 && func_004bf0e8(snd, 4) == 0 && func_004bf0e8(snd, -1) == 0);
        clear_log();
        ACSSND_SetTrackVol(snd, 9, 1);
        CHECK_LOG("");
    }

    /* loop flag on every voice */
    snd_setup(snd, 2);
    clear_log();
    ACSSND_SetLpFlg(snd, 1);
    CHECK_LOG("loop(0,1) loop(1,1) ");

    /* Start, single buffer: a stopped voice (status != 2) is stopped and the buffer fades in; level pushed;
     * parameters applied (kind 0 -> start A(p[1])) */
    snd_setup(snd, 1);
    vstatus[0] = 0;
    clear_log();
    ACSSND_Start(snd, p);
    CHECK_LOG("lock a4(1) stop(0,1) set(-960) fade(0,40,0,60) hs(1,1) he(1,4) unlock vol(0,0) startA(0,11) ");
    CHECK(W32(snd, 0x64) == 1 && snd[0x1c] == 2);
    /* status 2 without lock-out: fade out again, nothing started */
    vstatus[0] = 2;
    clear_log();
    ACSSND_Start(snd, p);
    CHECK_LOG("lock a4(1) fade(-960,30,0,60) hs(1,0) he(1,3) unlock ");
    /* status 2 with lock-out: restarted */
    W32(snd, 0x60) = 1;
    p[0] = 1;
    clear_log();
    ACSSND_Start(snd, p);
    CHECK_LOG("lock a4(1) stop(0,1) set(-960) fade(0,40,0,60) hs(1,1) he(1,4) unlock vol(0,0) startB(0,22,33) ");

    /* Start, double buffer: advance to the next buffer, fade it in, fade the previous out */
    snd_setup(snd, 2);
    W32(snd, 0x10) = 0;
    W32(snd, 0x14) = 1;
    vstatus[0] = vstatus[1] = 1;
    p[0] = 2;
    clear_log();
    ACSSND_Start(snd, p);
    CHECK(W32(snd, 0x10) == 1 && W32(snd, 0x14) == 0);
    CHECK_LOG("lock a4(1) lock unlock stop(1,1) set(-960) fade(0,40,0,60) hs(1,1) he(1,4) fade(-960,30,0,60) hs(1,0) he(1,3) unlock vol(1,0) startC(1,44,55) ");
    /* a negative +0xb4 delays the fade-out of the previous buffer instead */
    snd_setup(snd, 2);
    W32(snd, 0x14) = 1;
    W32(snd, 0xb4) = -7;
    vstatus[0] = vstatus[1] = 0;
    p[0] = 3;
    clear_log();
    ACSSND_Start(snd, p);
    CHECK_LOG("lock a4(1) lock unlock stop(1,1) set(-960) fade(0,40,0,60) hs(1,1) he(1,4) unlock vol(1,0) startD(1,66,77) ");

    /* CancelFading: fade-out turns into fade-in and back; only while +0xa4 is 1 */
    snd_setup(snd, 1);
    snd[0xa4] = 0;
    clear_log();
    ACSSND_CancelFading(snd);
    CHECK_LOG("");
    snd[0xa4] = 1;
    snd[0x1c] = 1;
    clear_log();
    ACSSND_CancelFading(snd);
    CHECK_LOG("lock hs(1,1) he(1,4) reverse unlock ");
    CHECK(snd[0x1c] == 2 && snd[0x1d] == 0);
    clear_log();
    ACSSND_CancelFading(snd);
    CHECK_LOG("lock hs(1,0) he(1,3) reverse unlock ");
    CHECK(snd[0x1c] == 1 && snd[0x1d] == 2);
    snd[0x1c] = 0;
    clear_log();
    ACSSND_CancelFading(snd);
    CHECK_LOG("lock reverse unlock ");

    /* getters: ADX kind uses the table at +0, AIXP kind likewise with the other test */
    snd_setup(snd, 2);
    W32(snd, 0x10) = 1;
    CHECK(ACSSND_GetActiveAdxt(snd) == P4_G2H(0x1000));
    CHECK(ACSSND_GetNextAdxt(snd) == P4_G2H(0x1000));
    CHECK(ACSSND_GetAdxt(snd, 1) == P4_G2H(0x1001) && ACSSND_GetAdxt(snd, 2) == 0 && ACSSND_GetAdxt(snd, -1) == 0);
    CHECK(ACSSND_GetNumAdxt(snd) == 2 && ACSSND_GetNumAixp(snd) == 0 && ACSSND_GetActiveAixp(snd) == 0);
    W32(snd, 0x94) = 1;
    CHECK(ACSSND_GetNumAdxt(snd) == 0 && ACSSND_GetNumAixp(snd) == 2 && ACSSND_GetActiveAdxt(snd) == 0);
    CHECK(ACSSND_GetActiveAixp(snd) == P4_G2H(0x1000) && ACSSND_GetNextAixp(snd) == P4_G2H(0x1000) && ACSSND_GetAixp(snd, 1) == P4_G2H(0x1001));

    /* AHX attach and detach: the +0x6c area, 0x3000 bytes per buffer; the AIXP kind goes through its first ADXT */
    snd_setup(snd, 2);
    clear_log();
    ACSSND_AttachAhx(snd);
    CHECK(last_err == 0x00757010u);
    W32(snd, 0x98) = 1;
    U32(snd, 0x6c) = 0x4000;
    ACSSND_AttachAhx(snd);
    CHECK_LOG("ahx_attach(0x1000,12288) ahx_attach(0x1001,12288) ");
    clear_log();
    ACSSND_DetachAhx(snd);
    CHECK_LOG("ahx_detach(0x1000) ahx_detach(0x1001) ");
    W32(snd, 0x94) = 1;
    clear_log();
    ACSSND_DetachAhx(snd);
    CHECK_LOG("aixp_adxt(0) ahx_detach(0x1000) aixp_adxt(0) ahx_detach(0x1001) ");

    /* fade curve on every fader; voice play state map */
    snd_setup(snd, 2);
    clear_log();
    func_004bfb28(snd, 1);
    CHECK_LOG("curve(1) curve(1) ");
    {
        static const int32_t want[8] = { 0, 1, 1, 2, 2, 3, 4, 4 };
        int32_t k;

        U32(VOICE(0), 4) = 5;
        for (k = 0; k < 8; k++) {
            d43f8_ret = k;
            CHECK(func_004bdfa0(VOICE(0)) == want[k]);
        }
    }

    /* all faders idle: 1; a running one: 0; retail returns i == count, so a negative count gives 0 and none gives 1 */
    snd_setup(snd, 2);
    CHECK(func_004c05f0(snd) == 1);
    FDR(1)[0] = 1;
    CHECK(func_004c05f0(snd) == 0);
    FDR(1)[0] = 0;
    W32(snd, 0xa0) = 0;
    CHECK(func_004c05f0(snd) == 1);
    W32(snd, 0xa0) = -1;
    CHECK(func_004c05f0(snd) == 0);

    /* all tracks of a voice object; the fade hook calls the user callback with the object and the event */
    {
        uint8_t *obj = p4t_galloc(16, 16);

        W32(obj, 0xc) = 2;
        clear_log();
        func_004be318(obj, -50);
        CHECK_LOG("trkvol(0,-50) trkvol(1,-50) ");
        snd_setup(snd, 1);
        func_004c0658(0, 3);
        func_004c0658(snd, 3);
        clear_log();
        U32(snd, 0xcc) = P4_FN_H2G(user_cb);
        func_004c0658(snd, 4);
        CHECK_LOG("usercb(1,4) ");
    }

    /* highlight block ticks: +0x54 - level * mult / -(+0x5c) */
    {
        uint8_t *blk = p4t_galloc(0x60, 16), *f = p4t_galloc(0x44, 16);

        U32(blk, 4) = P4_H2G(f);
        W32(f, 0x1c) = -480;
        W32(blk, 0x54) = 30;
        W32(blk, 0x58) = 60;
        W32(blk, 0x5c) = 120;
        CHECK(func_004c12a8(blk) == -90 && func_004c12f8(blk) == -210);
        func_004c12a0(blk, 3);
        CHECK(blk[0x50] == 3);
    }

    /* advancing and stepping back through the buffers */
    snd_setup(snd, 2);
    clear_log();
    func_004c0308(snd);
    CHECK(W32(snd, 0x10) == 1 && W32(snd, 0x14) == 0 && U32(snd, 8) == P4_H2G(VOICE(1)) && U32(snd, 0xc) == P4_H2G(VOICE(0)));
    CHECK_LOG("lock unlock ");
    func_004c0308(snd);
    CHECK(W32(snd, 0x10) == 0 && W32(snd, 0x14) == 1);
    func_004c03b8(snd);
    CHECK(W32(snd, 0x10) == 1 && W32(snd, 0x14) == 0);
    /* a pending switch with the lock-out clear stops the old voice before advancing */
    W32(snd, 0x64) = 1;
    clear_log();
    func_004c0308(snd);
    CHECK_LOG("lock stop(1,0) unlock ");

    /* after a frame: idle faders with the slot byte set run the slot's follow-up; all idle -> event 6 */
    snd_setup(snd, 1);
    snd[0x1c] = 1;
    snd[0x1d] = 3;
    snd[0xa4] = 1;
    clear_log();
    func_004c0450(snd);
    CHECK_LOG("stop(0,1) a4(0) ");
    CHECK(snd[0x1c] == 0 && snd[0xa4] == 0);
    /* a running fader keeps its slot byte */
    snd[0x1c] = 1;
    FDR(0)[0] = 1;
    clear_log();
    func_004c0450(snd);
    CHECK_LOG("");
    CHECK(snd[0x1c] == 1);
    FDR(0)[0] = 0;
    /* the follow-up restarts playback: lock, fade-in with delay +0xb4 - +0xa8, saved start parameters */
    snd[0x1d] = 1;
    W32(snd, 0xb4) = 100;
    clear_log();
    func_004c01c8(snd, snd + 0x18);
    CHECK_LOG("lock a4(1) stop(0,1) set(-960) fade(0,40,70,60) hs(1,1) he(1,4) unlock vol(0,0) startA(0,0) ");
}

int main(void)
{
    uint8_t *snd, *work;
    int32_t p[10] = { 4, 2, 48000, 3, 5, 0, 0, 1, 0, 0 };
    int32_t ring, vsz;

    if (p4t_guest_map(0x00756000, 0x2000) != 0) {
        return 1;
    }
    snd = p4t_galloc(0x100, 16);
    work = p4t_galloc(0x400000, 16);
    vt = p4t_galloc(16, 16);
    vtbl = p4t_galloc(16 * 4, 16);
    voice_mem = p4t_galloc(2 * 16, 16);
    fdr_mem = p4t_galloc(2 * 0x44, 16);

    /* reset defaults */
    memset(snd, 0xaa, 0x100);
    func_004bfba0(snd);
    CHECK(W32(snd, 168) == 30 && W32(snd, 184) == 60 && W32(snd, 160) == 0 && W32(snd, 80) == 0);
    CHECK(snd[164] == 0 && W32(snd, 0x78) == 0 && W32(snd, 0x9c) == 0 && W32(snd, 0xcc) == 0);
    CHECK(W32(snd, 0) == 0 && W32(snd, 4) == 0 && W32(snd, 24) == 0 && W32(snd, 44) == 0);
    CHECK(snd[164] == 0 && ((uint32_t *)snd)[0xd0 / 4] == 0xaaaaaaaau); /* untouched past +0xcc */

    /* mode A (voice object of 16 bytes), mode B 0, single buffer */
    ring = ((((2 * (48000 / 1000) * 25000) / 0x2c) * 5) + 0x800) >> 11 << 11;
    vsz = (ring + 2 * 0x40c0 + 0x24) * 3 + 0x40;
    memset(work, 0, 0x400000);
    CHECK(func_004be4a8(snd, p, work, 0x400000) == 1);
    CHECK(W32(snd, 80) == 1 && W32(snd, 84) == 0 && W32(snd, 160) == 1 && W32(snd, 92) == 0);
    CHECK(init_n == 1 && init_work[0] == work + 16 && init_size[0] == vsz);
    CHECK(U32(snd, 0) == P4_H2G(work) && U32(snd, 24) == U32(snd, 0) && U32(snd, 8) == U32(snd, 24));
    CHECK(U32(snd, 32) == P4_H2G(work + 16 + vsz) && W32(snd, 16) == 0);
    CHECK(U32(snd, 52) == P4_H2G(work + 16 + vsz + 68) && U32(snd, 112) == U32(snd, 52));
    CHECK(U32(snd, 192) == P4_H2G(work + 16 + vsz + 68 + 0x100));
    CHECK(memcmp(snd + 120, p, 40) == 0);
    CHECK(func_004be4a8(snd, p, work, 0x400000) == 0 && last_err == 0x00756be8u);

    /* double buffer, extra buffers, mode B: two voice objects, +0x6c set */
    p[6] = 1; p[8] = 1; p[9] = 1; init_n = 0;
    func_004bfba0(snd); err_n = 0;
    CHECK(func_004be4a8(snd, p, work, 0x400000) == 1 && W32(snd, 160) == 2 && W32(snd, 92) == 1 && init_n == 2);
    CHECK(U32(snd, 4) != 0 && U32(snd, 36) == U32(snd, 4) && U32(snd, 44) != 0 && U32(snd, 108) != 0);
    CHECK(U32(snd, 52) - U32(snd, 108) == 2 * 0x3000);

    /* work area too small */
    func_004bfba0(snd); finish_n = 0; p[6] = 0; p[8] = 0; p[7] = 1; p[9] = 0;
    CHECK(func_004be4a8(snd, p, work, 100) == 0 && last_err == 0x00756d38u && finish_n == 1);

    /* voice failures pick their texts */
    func_004bfba0(snd); fail_voice = 1;
    CHECK(func_004be4a8(snd, p, work, 0x400000) == 0 && last_err == 0x00756c18u);
    func_004bfba0(snd); p[7] = 0;
    CHECK(func_004be4a8(snd, p, work, 0x400000) == 0 && last_err == 0x00756c88u);
    fail_voice = 0; fail_init = 1;
    func_004bfba0(snd);
    CHECK(func_004be4a8(snd, p, work, 0x400000) == 0 && last_err == 0x00756cc0u);
    p[7] = 1; func_004bfba0(snd);
    CHECK(func_004be4a8(snd, p, work, 0x400000) == 0 && last_err == 0x00756c50u);
    fail_init = 0; fail_fdr = 1; func_004bfba0(snd);
    CHECK(func_004be4a8(snd, p, work, 0x400000) == 0 && last_err == 0x00756cf8u);

    test_api();
    if (fails) return 1;
    printf("cri_acssnd: ok\n");
    return 0;
}

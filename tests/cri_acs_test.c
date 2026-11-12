/* ACS server and fader (src/cri/acs.c): the call sequence ACSSVR_Init and ACSSVR_Finish make and the
 * fader's level maths, with the retail callees replaced by recording stubs. The expected call
 * orders and arguments are read from the retail asm (0x004c0718, 0x004c07f0, 0x004bd790,
 * 0x004bdc20, 0x004bda68); a mismatch between this file and the asm is a bug in one of them.
 */
#include <math.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../src/cri/acs.c"
#include "../src/cri/acsfdr_hooks.c"
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
    log_buf[log_len++] = ';';
    log_buf[log_len] = 0;
}

static void clear_log(void)
{
    log_len = 0;
    log_buf[0] = 0;
}

#define CHECK(c) do { if (!(c)) { printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #c); failures++; } } while (0)
#define CHECK_LOG(s) do { if (strcmp(log_buf, (s)) != 0) { printf("FAIL %s:%d log\n  got  %s\n  want %s\n", __FILE__, __LINE__, log_buf, (s)); failures++; } } while (0)

void func_0043f9c8(void *dst, int32_t c, uint32_t n) { memset(dst, c, n); }
int32_t func_004bd6a0(uint32_t msg) { rec("err(0x%x)", (unsigned)msg); return 0; }
void func_004ca720(void) { rec("adxt_init"); }
void func_004ca8b0(void) { rec("adxt_fin"); }
void func_004de340(void) { rec("aixp_init"); }
void func_004de3f0(void) { rec("aixp_fin"); }
int32_t func_004f0b88(int32_t svtype, int32_t id) { rec("del(%d,%d)", svtype, id); return 0; }
int32_t func_004f0c78(int32_t svtype, int32_t id, void *func, void *obj, const char *name)
{
    rec("setid(%d,%d,%s,%s,0x%x)", svtype, id, func == (void *)func_004c0e50 ? "exec" : "?", obj ? "svr" : "null", (unsigned)P4_H2G(name));
    return 0;
}
void *func_004be418(void *work, int32_t size) { rec("snd_create(%d)", size); return P4_G2H(0xabc0); }
int32_t func_004be4a8(void *snd, const int32_t *prm, void *work, int32_t size) { rec("snd_setup(%d,%d)", (int)P4_H2G(prm), size); return 1; }
void func_004be480(void *snd) { rec("snd_free"); }
void func_004be9c0(void *snd) { rec("snd_stop"); }
int32_t func_004bea98(uint32_t snd) { return 1; }
void func_004bf950(uint32_t snd) { rec("snd_prep"); }
int32_t func_004beec0(uint32_t snd) { return 0; }
void func_004bf938(uint32_t snd, int32_t v) { }
void func_004bf960(void *snd) { rec("snd_fin"); }
void func_004c1038(void *fdr) { rec("hl_step"); }

static void test_levels(void)
{
    uint8_t f[0x44];
    int32_t v;
    float exp0;

    func_004bd9f8(f);
    CHECK(W32(f, 0x1c) == -960 && W32(f, 0x24) == -960 && W32(f, 0x28) == -960);
    CHECK(func_004bdc20(f, 0) == 0x316938);
    CHECK(func_004bdc20(f, -960) == 0);
    /* curve 0: 10^(v/200) of full scale */
    v = -100;
    exp0 = (float)pow(10.0, (double)((float)v / 200.0f)) * 3238200.0f;
    CHECK(func_004bdc20(f, v) == (int32_t)exp0);
    CHECK(func_004bdc20(f, -200) == (int32_t)((float)pow(10.0, -1.0) * 3238200.0f));
    /* curve 1: log curve */
    f[0x18] = 1;
    v = -480;
    CHECK(abs(func_004bdc20(f, v) - (int32_t)(3238200.0 * log10(480.0) / log10(960.0))) <= 2);
    CHECK(func_004bdc20(f, -959) > 3000000);
    /* the inverse: end points, a round trip on curve 0, and the curve 1 closed form */
    f[0x18] = 0;
    CHECK(func_004bdaf0(f, 0) == -960 && func_004bdaf0(f, 0x316938) == 0);
    CHECK(abs(func_004bdaf0(f, func_004bdc20(f, -100)) - (-100)) <= 1);
    CHECK(func_004bdaf0(f, 1) == -960);
    CHECK(func_004bdaf0(f, 3238199) == (int32_t)(float)(log10((double)(3238199.0f / 3238200.0f)) * 200.0));
    f[0x18] = 1;
    CHECK(func_004bdaf0(f, 0x316938 - 1) == (int32_t)-(float)pow(10.0, (double)(((float)log10(960.0) * 1.0f) / 3238200.0f)));
    CHECK(func_004bdaf0(f, 1000000) == (int32_t)-(float)pow(10.0, (double)(((float)log10(960.0) * (float)(0x316938 - 1000000)) / 3238200.0f)));
    f[0x18] = 0;
    /* the step per tick is the linear difference over the ticks; no ticks is full scale */
    f[0x18] = 0;
    CHECK(func_004bda68(f, -960, 0, 10) == (0x316938 - 0) / 10);
    CHECK(func_004bda68(f, 0, -960, 4) == (0 - 0x316938) / 4);
    CHECK(func_004bda68(f, 0, -960, 0) == 0x316938);
}

static uint32_t hook_args[4];
static void hook_a(uint32_t a, uint32_t b) { hook_args[0] = a; hook_args[1] = b; }
static void hook_b(uint32_t a, uint32_t b) { hook_args[2] = a; hook_args[3] = b; }

static void test_hooks(void)
{
    uint8_t f[0x44];

    /* the setters (0x004bd9d0, 0x004bd9d8, 0x004bd9e8) write the fields the hook callers read */
    memset(f, 0, sizeof f);
    func_004bd9d0(f, 1);
    CHECK(f[0x18] == 1);
    func_004bd9d8(f, P4_FN_H2G(hook_a), 5, 6);
    func_004bd9e8(f, P4_FN_H2G(hook_b), 7, 8);
    CHECK(func_004bdd48(f) == 1 && hook_args[0] == 5 && hook_args[1] == 6);
    CHECK(func_004bdd78(f) == 1 && hook_args[2] == 7 && hook_args[3] == 8);
    memset(hook_args, 0, sizeof hook_args);
    memset(f, 0, sizeof f);

    memset(f, 0, sizeof f);
    CHECK(func_004bdd48(f) == 0 && func_004bdd78(f) == 0);
    U32(f, 0x2c) = P4_FN_H2G(hook_a);
    U32(f, 0x30) = 11;
    U32(f, 0x34) = 22;
    U32(f, 0x38) = P4_FN_H2G(hook_b);
    U32(f, 0x3c) = 33;
    U32(f, 0x40) = 44;
    CHECK(func_004bdd48(f) == 1 && hook_args[0] == 11 && hook_args[1] == 22 && hook_args[2] == 0);
    CHECK(func_004bdd78(f) == 1 && hook_args[2] == 33 && hook_args[3] == 44);
}

static void test_fade(void)
{
    uint8_t f[0x44];

    func_004bd9f8(f);
    func_004bd790(f, 0, 8, 7, 9);
    CHECK(f[0] == 1 && W32(f, 8) == 0 && W32(f, 0xc) == 0 && W32(f, 0x10) == 7 && W32(f, 0x14) == 9);
    CHECK(W32(f, 0x24) == 0 && W32(f, 0x28) == -960);
    CHECK(W32(f, 4) == func_004bda68(f, -960, 0, 8));
    func_004bd850(f);
    CHECK(W32(f, 4) == -func_004bda68(f, -960, 0, 8));
    CHECK(W32(f, 0x24) == -960 && W32(f, 0x28) == 0);
    /* a zero-tick fade completes at once */
    func_004bd790(f, -200, 0, 0, 0);
    CHECK(f[0] == 2 && W32(f, 0x1c) == -200 && W32(f, 0x20) == func_004bdc20(f, -200));
    f[0] = 3;
    func_004bd850(f);
    CHECK(f[0] == 2);
}

static void test_server(void)
{
    uint8_t *svr = p4t_galloc(0xb0, 16);   /* the server holds pointers to itself: guest storage */
    uint32_t ret;

    memset(svr, 0xcc, 0xb0);
    memset(svr, 0, 0xb0);
    clear_log();
    ret = ACSSVR_Init(svr);
    CHECK(ret == 1);
    CHECK(W32(svr, 0xa4) == 1 && W32(svr, 0xa8) == 0);
    CHECK(U32(svr, 0x40) == P4_H2G(svr + 0x44));
    CHECK(U32(svr, 0x44) == P4_H2G(svr));
    CHECK(U32(svr, 0x48) == P4_H2G(svr + 0x4c));
    CHECK(W32(svr, 0x44 + 0x54) == 0x1e && W32(svr, 0x44 + 0x58) == 0x3c && W32(svr, 0x44 + 0x5c) == 0x78);
    CHECK_LOG("adxt_init;aixp_init;setid(2,4,exec,svr,0x757368);");
    clear_log();
    CHECK(ACSSVR_Init(svr) == 1);
    CHECK(W32(svr, 0xa4) == 2);
    CHECK_LOG("");
    /* sounds take the first free slot and carry their work-area split */
    clear_log();
    CHECK(ACSSVR_CreatSnd(svr, 5, (uint8_t *)svr, 0x400) == 0xabc0);
    CHECK(U32(svr, 0) == 0xabc0);
    CHECK_LOG("snd_create(1024);snd_setup(5,816);");
    /* exec walks the live handles once */
    clear_log();
    func_004c0e50(svr);
    CHECK_LOG("snd_prep;snd_fin;");
    /* finish: first release only counts, second tears down */
    clear_log();
    ACSSVR_Finish(svr);
    CHECK(W32(svr, 0xa4) == 1);
    CHECK_LOG("");
    ACSSVR_Finish(svr);
    CHECK(W32(svr, 0xa4) == 0 && U32(svr, 0x40) == 0);
    CHECK(strstr(log_buf, "snd_stop;snd_free;") == log_buf);
    CHECK(strstr(log_buf, "del(2,4);del(5,0);aixp_fin;adxt_fin;") != NULL);
    /* a release without an init is the error path */
    clear_log();
    ACSSVR_Finish(svr);
    CHECK(strstr(log_buf, "err(0x7573b0)") != NULL);
    CHECK(func_004c0e00(svr) == 0);
    ACSVHL_EnableVoiceHighlight(svr + 0x44);
    CHECK(func_004c0fd8(svr + 0x44) == 1);
    func_004c0fd0(svr + 0x44);
    CHECK(func_004c0fd8(svr + 0x44) == 0);
}

int main(void)
{
    if (p4t_guest_map(0x00757000, 0x1000) != 0) {
        return 1;
    }
    test_levels();
    test_fade();
    test_hooks();
    test_server();
    if (failures) {
        printf("cri_acs: %d failure(s)\n", failures);
        return 1;
    }
    printf("cri_acs: ok\n");
    return 0;
}

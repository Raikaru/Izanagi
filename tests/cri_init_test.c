/* ADX setup and CVFS device servers (src/cri/adx_init.c): the dvCi root and mode records, the
 * ADXPS2 setup call order, the PS2RNA voice configuration, the ADX finish bracket and the dvCi / mfCi
 * server steps, over memory mapped at the retail globals, against recording stubs. Expected values are
 * read from the retail asm (0x004d32f8, 0x004e72a0, 0x004eb318, 0x004ca8b0, 0x004e5b10, 0x004e5bd8).
 */
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../src/cri/adx_init.c"
#include "guest_test.h"

static int failures;
static char log_buf[4096];
static int log_len;
static int32_t req_status[8];
int32_t adxt_init_cnt;

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

void func_004c6a98(const char *msg) { rec("err(%s)", msg); }
void func_004e2e28(void *f, void *o) { (void)o; rec("errfunc(%d)", f == (void *)func_004d32e0); }
void func_004e17a0(const char *name, void *getif, int32_t a)
{
    rec("adddev(%s,%s,%d)", name, getif == (void *)func_004e9e50 ? "mf" : getif == (void *)func_004e5a08 ? "dv" : "?", a);
}
void func_004e1ad8(const char *name) { rec("defdev(%s)", name); }
int32_t func_004f1278(int32_t *flag) { rec("tas"); if (*flag == 1) { return 0; } *flag = 1; return 1; }
void func_004f0638(void) { rec("lock"); }
void func_004f06b0(void) { rec("unlock"); }
int32_t func_004e39b0(int32_t req) { return req_status[req]; }
void func_004e3770(int32_t req) { rec("release(%d)", req); }
void func_0043f9c8(void *d, int32_t c, uint32_t n) { memset(d, c, n); }
int32_t func_004f0c78(int32_t a, int32_t b, void *c, void *d, const char *e) { (void)a; (void)b; (void)c; (void)d; (void)e; return 0; }
int32_t func_004f0a60(int32_t a, void *c, void *d, const char *e) { (void)a; (void)c; (void)d; (void)e; return 0; }
int32_t func_004f0b88(int32_t a, int32_t b) { (void)a; (void)b; return 0; }
void func_004f0608(void) {}
void func_004f0620(void) {}
void func_004ca720(void) {}
void ADXT_Finish(void) { rec("finish(%d)", (int)M32(0x0073249c)); }
int32_t func_00446ed8(char *buf, const char *fmt, va_list ap) { return vsprintf(buf, fmt, ap); }
void func_004244c8(const char *text) { rec("print(%s)", text); }
uint32_t func_00442948(const char *s) { return (uint32_t)strlen(s); }
void func_004df800(void *h) { (void)h; }
void func_004e0c60(void *h) { (void)h; }
int32_t func_004e0db0(void *h) { (void)h; return 0; }
void func_004e1100(void *h) { (void)h; }
void func_004de860(void *h) { (void)h; }
void func_004e0560(void) {}
void func_004e0408(void) {}
uint32_t func_004de310(void) { return 0; }
static int mf_steps;
void func_004e9fa8(void *o) { (void)o; mf_steps++; }
void func_004e0380(void) {}
void func_004e0398(void) {}

static void put_str(uint32_t addr, const char *s) { strcpy((char *)P4_G2H(addr), s); }

static void test_root_and_mode(void)
{
    char *root = (char *)P4_G2H(0x00733dd0);

    M8(0x0075b250) = 'c';
    M8(0x0075b251) = ':';
    func_004e7280(1, 2, 3, 0x12345678);
    CHECK(M8(0x00733dc0) == 1 && M8(0x00733dc1) == 2 && M8(0x00733dc2) == 3 && M32(0x00733dc4) == 0x12345678);
    func_004e72a0("DIR\\SUB\\");
    CHECK(strcmp(root, "c:DIR\\SUB") == 0);
    func_004e72a0("/abs/path");
    CHECK(strcmp(root, "/abs/path") == 0);
    func_004e72a0("\\x");
    CHECK(strcmp(root, "\\x") == 0);
    func_004e72a0(0);
    CHECK(root[0] == 0);
}

static void test_setup(void)
{
    int32_t prm[5];

    put_str(0x00758b10, "MFS");
    put_str(0x00758b18, "CDV");
    prm[0] = (int32_t)p4_gport_str("ROOT/");
    prm[1] = 0x55;
    prm[2] = 7;
    prm[3] = 8;
    prm[4] = 9;
    clear_log();
    func_004d32f8(prm);
    CHECK_LOG("errfunc(1) adddev(MFS,mf,0) errfunc(1) adddev(CDV,dv,0) defdev(CDV) ");
    CHECK(M8(0x00733dc0) == 7 && M8(0x00733dc1) == 8 && M8(0x00733dc2) == 9 && M32(0x00733dc4) == 0x55);
    CHECK(strcmp((char *)P4_G2H(0x00733dd0), "c:ROOT") == 0);
    func_004d32f8(0);
    CHECK(M8(0x00733dc0) == 0 && M32(0x00733dc4) == 0 && ((char *)P4_G2H(0x00733dd0))[0] == 0);
    CHECK(func_004e9e50() == (void *)P4_G2H(0x00738d00));
    CHECK(func_004e5a08() == (void *)P4_G2H(0x007330d8));
}

static void test_voice_config(void)
{
    M32(0x007212d0) = 0;
    M32(0x0073a740) = M32(0x0073a744) = M32(0x0073a748) = 0;
    clear_log();
    criConfigureVoiceCounts(5, 5);
    CHECK(M32(0x0073a768) == 5 && M32(0x0073a76c) == 5 && M32(0x0073a740) == 15 && M32(0x0073a744) == 15 * 0x4100 && M32(0x0073a748) == 1);
    CHECK_LOG("");
    /* 33 voices (2 * 16 + 1) is over the limit; nothing changes, the message prints */
    put_str(0x0075c718, "limit %d");
    clear_log();
    criConfigureVoiceCounts(16, 1);
    CHECK_LOG("print(limit 32) ");
    CHECK(M32(0x0073a740) == 15);
    /* exactly 32 is fine */
    clear_log();
    criConfigureVoiceCounts(16, 0);
    CHECK(M32(0x0073a740) == 32 && M32(0x0073a744) == 32 * 0x4100);
    /* negative counts */
    put_str(0x0075c6b8, "bad0 %d");
    put_str(0x0075c6e8, "bad1");
    clear_log();
    criConfigureVoiceCounts(-1, 2);
    criConfigureVoiceCounts(2, -1);
    CHECK_LOG("print(bad0 -1) print(bad1) ");
    CHECK(M32(0x0073a740) == 32);
    /* the debug-off word silences the printf */
    M32(0x007212d0) = 2;
    clear_log();
    criConfigureVoiceCounts(-1, 0);
    CHECK_LOG("");
}

static void test_finish_and_clear(void)
{
    M32(0x0073249c) = 1;
    adxt_init_cnt = 1;
    clear_log();
    func_004ca8b0();
    CHECK_LOG("finish(0) ");
    CHECK(M32(0x0073249c) == 1);
    adxt_init_cnt = 2;
    M32(0x0073249c) = 5;
    clear_log();
    func_004ca8b0();
    CHECK_LOG("finish(5) ");
    memset((void *)P4_G2H(0x00724ed0), 0xaa, 0x3c0);
    func_004d8830();
    CHECK(M32(0x00724ed0) == 0 && M32(0x00724ed0 + 0x3bc) == 0);
}

static void test_device_servers(void)
{
    uint8_t *objs = (uint8_t *)P4_G2H(0x00733140);
    uint8_t *o0 = objs;
    uint8_t *o5 = objs + 5 * 0x48;

    memset(objs, 0, 40 * 0x48);
    M32(0x007330d0) = 0;
    /* nothing in use: the flag is taken and dropped */
    clear_log();
    func_004e5bd8();
    CHECK_LOG("tas ");
    CHECK(M32(0x007330d0) == 0);
    /* re-entry is refused */
    M32(0x007330d0) = 1;
    clear_log();
    func_004e5bd8();
    CHECK_LOG("tas ");
    CHECK(M32(0x007330d0) == 1);
    M32(0x007330d0) = 0;

    /* object 5 in use with request 3 done: offset += size, state 1, request released */
    o5[0] = 1;
    W32(o5, 0x1c) = 3;
    W32(o5, 0x10) = 0x800;
    W32(o5, 0xc) = 0x1000;
    req_status[3] = 3;
    clear_log();
    func_004e5bd8();
    CHECK_LOG("tas lock release(3) unlock ");
    CHECK(o5[2] == 1 && W32(o5, 0xc) == 0x1800 && W32(o5, 0x14) == 0x800 && W32(o5, 0x1c) == 0);
    /* idle request (0): state 0; error (9): state 3; busy (1): stays pending */
    W32(o5, 0x1c) = 3;
    req_status[3] = 0;
    func_004e5b10(o5);
    CHECK(o5[2] == 0 && W32(o5, 0x1c) == 0);
    W32(o5, 0x1c) = 3;
    req_status[3] = 9;
    func_004e5b10(o5);
    CHECK(o5[2] == 3 && W32(o5, 0x1c) == 0);
    W32(o5, 0x1c) = 3;
    req_status[3] = 1;
    o5[2] = 7;
    clear_log();
    func_004e5b10(o5);
    CHECK_LOG("lock unlock ");
    CHECK(o5[2] == 7 && W32(o5, 0x1c) == 3);
    /* no pending request: not even the lock */
    W32(o5, 0x1c) = 0;
    clear_log();
    func_004e5b10(o5);
    CHECK_LOG("");
    /* an object that is not in use (byte != 1) is skipped, like one in use with byte 2 */
    o0[0] = 2;
    W32(o0, 0x1c) = 3;
    req_status[3] = 3;
    clear_log();
    func_004e5bd8();
    CHECK_LOG("tas ");

    /* the mfCi server only visits in-use objects and does nothing */
    memset((void *)P4_G2H(0x00738d68), 1, 40 * 0x38);
    func_004e9fb0();
    CHECK(mf_steps == 40);
}

int main(void)
{
    if (p4t_guest_map(0x00720000, 0x40000) != 0) {
        printf("cri_init: cannot map the retail globals\n");
        return 77;
    }
    test_root_and_mode();
    test_setup();
    test_voice_config();
    test_finish_and_clear();
    test_device_servers();
    if (failures) {
        return 1;
    }
    printf("cri_init: ok\n");
    return 0;
}

/* ADXM server dispatch and vsync body (src/cri/adxm.c): the server passes by mode (0x004cb360,
 * 0x004cb2d0, 0x004cb3f8), the thread wake-ups (0x004ccb68), the vsync server entry (0x004ccc98) and the
 * vsync interrupt body (0x004cca10), run over memory mapped at the retail globals they read, against
 * recording stubs for the kernel and the SVM. Expected orders are read from the retail asm.
 */
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../src/cri/adxm.c"
#include "guest_test.h"

static int failures;
static char log_buf[4096];
static int log_len;
static int32_t thrd_status[64];
static int32_t setup_thrd;

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

int32_t func_004215e0(void *p) { (void)p; return 0; }
int32_t func_00421620(void) { return 0; }
int32_t func_00421670(int32_t id, int32_t prio) { (void)id; (void)prio; return 0; }
int32_t func_004216d0(void) { return 1; }
int32_t func_004216e0(int32_t id, void *st) { ((int32_t *)st)[0] = thrd_status[id]; return thrd_status[id]; }
int32_t func_004216f0(int32_t id, void *st) { ((int32_t *)st)[0] = thrd_status[id]; return thrd_status[id]; }
int32_t func_00421710(int32_t id) { rec("wakeup(%d)", id); return id; }
int32_t func_00421750(int32_t id) { rec("suspend(%d)", id); return id; }
int32_t func_00421770(int32_t id) { rec("resume(%d)", id); return id; }
int32_t func_004226a8(int32_t id) { rec("iwakeup(%d)", id); return id; }
int32_t func_00422920(int32_t id, uint32_t a) { (void)a; return id; }
int32_t func_00421700(void) { rec("sleep"); return 0; }
int32_t func_004f1050(void) { rec("svr0"); return 0; }
int32_t func_004f1068(void) { rec("svr1"); return 0; }
int32_t func_004f1080(void) { rec("svr2"); return 0; }
int32_t func_004f1098(void) { rec("svr3"); return 0; }
int32_t func_004f10b0(void) { rec("svr4"); return 0; }
int32_t func_004f10c8(void) { rec("svr5"); return 0; }
int32_t func_004f10e0(void) { rec("svr6"); return 0; }
int32_t func_004f10f8(void) { rec("svr7"); return 0; }
void func_004f0de8(int32_t t, void *f, void *o) { (void)t; (void)f; (void)o; }
void func_004f0ec0(void *f, void *o) { (void)f; (void)o; }
void func_004f0ed8(void *f, void *o) { (void)f; (void)o; }
void func_004f07e0(const char *m) { (void)m; }
void func_004f11e0(void) {}
void func_004f0e80(void (*f)(void *, const char *), void *o) { (void)f; (void)o; }
int32_t func_004cc1e0(void) { return setup_thrd; }
void func_004f1350(void) { rec("lock"); }
void func_004f1380(void) { rec("unlock"); }
void ADXM_WaitVsync(void) { rec("waitvsync"); }

static int32_t user_cb_arg;
static int32_t user_cb(void *a) { user_cb_arg = (int32_t)P4_H2G(a); rec("usercb"); return 4242; }   /* the vsync server's argument word reaches it as a pointer */
static int32_t user_cb_int(int32_t a) { user_cb_arg = a; rec("usercb"); return 4242; }                /* the interrupt body's argument is an integer */

static void test_server_modes(void)
{
    M32(0x00719994) = 0;
    M32(0x00719980) = 2;
    clear_log();
    func_004cb360();
    CHECK_LOG("svr0 ");
    M32(0x00719980) = 3;
    clear_log();
    func_004cb360();
    CHECK_LOG("svr0 svr1 svr2 svr3 svr5 ");
    M32(0x00719980) = 1;
    clear_log();
    func_004cb360();
    CHECK_LOG("");
    /* -1 picks from the setup state: not set up -> 1 (nothing), set up -> 2 */
    M32(0x00719980) = -1;
    clear_log();
    func_004cb360();
    CHECK_LOG("");
    M32(0x00719994) = 1;
    clear_log();
    func_004cb360();
    CHECK_LOG("svr0 ");
    M32(0x00719994) = 0;

    M32(0x00719980) = 1;
    clear_log();
    func_004cb2d0();
    CHECK_LOG("svr0 svr1 svr2 svr3 svr4 svr5 svr6 svr7 ");
    M32(0x00719980) = 2;
    clear_log();
    func_004cb2d0();
    CHECK_LOG("svr5 ");
    M32(0x00719980) = 3;
    clear_log();
    func_004cb2d0();
    CHECK_LOG("svr6 svr7 ");
    M32(0x00719980) = 0;
    clear_log();
    CHECK(func_004cb2d0() == 0);
    CHECK_LOG("");
}

static void test_wake(void)
{
    memset(thrd_status, 0, sizeof thrd_status);
    M32(0x00719a3c) = 5;
    M32(0x00719a40) = 6;
    setup_thrd = 0;
    /* a suspended vsync thread (8) is resumed and then woken only when it reports as itself */
    thrd_status[5] = 8;
    clear_log();
    func_004ccb68();
    CHECK(strstr(log_buf, "resume(5)") != 0);
    CHECK(strstr(log_buf, "resume(6)") == 0);
    setup_thrd = 1;
    thrd_status[6] = 8;
    clear_log();
    func_004ccb68();
    CHECK(strstr(log_buf, "resume(6)") != 0);

    /* vsync server entry: lock, wakes, vsync callback, WaitVsync, unlock */
    M32(0x00922e20) = (int32_t)P4_FN_H2G(user_cb);
    M32(0x00922e24) = 99;
    memset(thrd_status, 0, sizeof thrd_status);
    setup_thrd = 0;
    clear_log();
    func_004ccc98();
    CHECK(user_cb_arg == 99);
    CHECK(strncmp(log_buf, "lock ", 5) == 0);
    CHECK(strstr(log_buf, "usercb waitvsync unlock ") != 0);
    M32(0x00922e20) = 0;
    clear_log();
    func_004cd170();
    CHECK_LOG("");
}

static void test_vsync(void)
{
    memset(thrd_status, 0, sizeof thrd_status);
    M32(0x007188b8) = 10;
    M64(0x00719a20) = 40;
    M32(0x00719aac) = 0;
    M32(0x00719994) = 1;
    M32(0x00719a30) = 3;
    M32(0x00719a34) = 4;
    M32(0x00719a2c) = 2;
    M32(0x007199d8) = 0;
    M32(0x007199e0) = 0;
    M32(0x007199dc) = 0;
    M32(0x00719980) = 2;
    M32(0x00719ab0) = 0;
    thrd_status[3] = thrd_status[4] = 4; /* waiting: woken */
    setup_thrd = 0;
    clear_log();
    CHECK(func_004cca10(7) == 0);
    CHECK(M32(0x007188b8) == 11 && M64(0x00719a20) == 41 && M32(0x00719aac) == 0);
    CHECK_LOG("iwakeup(3) iwakeup(4) svr0 ");

    /* threaded setup also wakes the third thread; threads whose loop is running (flags == 1) are left */
    setup_thrd = 1;
    M32(0x007199d8) = 1;
    thrd_status[2] = 4;
    M32(0x00719ab0) = (int32_t)P4_FN_H2G(user_cb_int);
    clear_log();
    CHECK(func_004cca10(7) == 4242);
    CHECK(user_cb_arg == 7);
    CHECK_LOG("iwakeup(4) iwakeup(2) svr0 usercb ");

    /* re-entered: only the count and the user callback */
    M32(0x00719aac) = 1;
    clear_log();
    func_004cca10(8);
    CHECK(M32(0x007188b8) == 13 && M64(0x00719a20) == 42);
    CHECK_LOG("usercb ");
    M32(0x00719aac) = 0;

    /* no servers set up (init count 0): no thread wakes, the pass still runs */
    M32(0x00719994) = 0;
    M32(0x00719ab0) = 0;
    clear_log();
    func_004cca10(0);
    CHECK_LOG("svr0 ");
    CHECK(M64(0x00719a20) == 42);
}

int main(void)
{
    if (p4t_guest_map(0x00718000, 0x14000) != 0 || p4t_guest_map(0x00922000, 4096) != 0) {
        printf("cri_adxm: cannot map the retail globals\n");
        return 77;
    }
    test_server_modes();
    test_wake();
    test_vsync();
    if (failures) {
        return 1;
    }
    printf("cri_adxm: ok\n");
    return 0;
}

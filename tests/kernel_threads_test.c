/* Host checks for the thread, semaphore, alarm and VSync model (src/platform/kernel_thread.c).
 *
 * The cases share one process and check the native priority/order contract,
 * status output parameters, and zero-on-success thread wrappers. The VSync
 * case links the actual INTC table from src/platform/ps2_kernel.c. */
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "guest_test.h"
#include "p4pc/platform.h"

/* Wrappers under test. */
int32_t func_004215e0(void *param);
int32_t func_00421600(int32_t id, uint32_t arg);
void func_00421620(void);
int32_t func_004216b0(int32_t id);
int32_t func_00421670(int32_t id, int32_t prio);
int32_t func_00421690(int32_t prio);
int32_t func_00421700(void);
int32_t func_00421710(int32_t id);
int32_t func_00421720(int32_t id);
int32_t func_00421730(int32_t id);
int32_t func_00421750(int32_t id);
int32_t func_00421760(int32_t id);
int32_t func_00421770(int32_t id);
int32_t func_00421780(int32_t id);
int32_t func_00421500(int32_t cause, int32_t (*handler)(int32_t), int32_t next);
int32_t func_00421510(int32_t cause, int32_t handle);
int32_t RemoveIntcHandler(int32_t cause, int32_t handle);
int32_t func_00421540(int32_t cause);
int32_t func_00421550(int32_t cause);
int32_t func_00421b80(int32_t id);
int32_t func_004216e0(int32_t id, void *info);
int32_t func_004216f0(int32_t id, void *info);
int32_t func_004217e0(void *param);
int32_t func_004217f0(int32_t id);
int32_t func_00421800(int32_t id);
int32_t func_00421810(int32_t id);
int32_t func_00421820(int32_t id);
int32_t func_00421830(int32_t id);

/* ThreadParam and SemaParam, as the game and the PS2 SDK lay them out. */
typedef struct {
    int32_t status;
    uint32_t function; /* guest code address */
    uint32_t stack;    /* guest address */
    int32_t stackSize;
    uint32_t gpReg;    /* guest address */
    int32_t initialPriority;
    int32_t currentPriority;
    uint32_t attributes;
    uint32_t option;
} TestThreadParam;

typedef struct {
    uint32_t attr;
    uint32_t option;
    int32_t initCount;
    int32_t maxCount;
} TestSemaParam;

enum { THS_RUN = 0x01, THS_READY = 0x02, THS_WAIT = 0x04, THS_SUSPEND = 0x08, THS_DORMANT = 0x10 };

static int failures;

#define CHECK(cond)                                                       \
    do {                                                                  \
        if (!(cond)) {                                                    \
            fprintf(stderr, "%s:%d: failed: %s\n", __FILE__, __LINE__, #cond); \
            failures++;                                                   \
        }                                                                 \
    } while (0)

/* Order log, written by the test threads. */
static char order[64];
static int order_n;

static void reset_order(void) {
    order_n = 0;
    order[0] = 0;
}

static void mark(char c) {
    order[order_n++] = c;
    order[order_n] = 0;
}

static int32_t spawn(void (*fn)(void *), int32_t prio) {
    TestThreadParam p;
    memset(&p, 0, sizeof p);
    p.function = P4_FN_H2G(fn);
    p.initialPriority = prio;
    p.currentPriority = prio;
    return func_004215e0(&p);
}

static int32_t make_sema(int32_t init, int32_t max) {
    TestSemaParam p;
    p.attr = 0;
    p.option = 0;
    p.initCount = init;
    p.maxCount = max;
    return func_004217e0(&p);
}

static void p_noop(void *a) {
    (void)a;
}

/* Priority order: B (5) runs before A (10) even though A was started first. A wakes main
 * (0), which preempts A at once. */
static void p_prio_a(void *a) {
    (void)a;
    mark('A');
    func_00421710(1);
}

static void p_prio_b(void *a) {
    (void)a;
    mark('B');
}

static void test_priority(void) {
    int32_t a = spawn(p_prio_a, 10);
    int32_t b = spawn(p_prio_b, 5);
    reset_order();
    CHECK(a > 1 && b > 1 && a != b);
    CHECK(func_00421600(a, 0) == a);
    CHECK(func_00421600(b, 0) == b);
    CHECK(func_00421700() == 0);
    CHECK(strcmp(order, "BA") == 0);
}

/* Semaphore ping-pong: main signals ping and waits for pong; the partner does the reverse.
 * Both sides run at priority 0, so the handoffs are FIFO and nothing preempts. */
#define PP_ROUNDS 100
static int32_t pp_ping;
static int32_t pp_pong;
static int pp_count;

static void p_pong(void *a) {
    int i;
    (void)a;
    for (i = 0; i < PP_ROUNDS; i++) {
        func_00421820(pp_ping);
        pp_count++;
        func_00421800(pp_pong);
    }
}

static void test_pingpong(void) {
    int i;
    int bad = 0;
    pp_ping = make_sema(0, 1);
    pp_pong = make_sema(0, 1);
    pp_count = 0;
    CHECK(pp_ping > 0 && pp_pong > 0);
    CHECK(func_00421600(spawn(p_pong, 0), 0) > 1);
    for (i = 0; i < PP_ROUNDS; i++) {
        if (func_00421800(pp_ping) != pp_ping)
            bad++;
        if (func_00421820(pp_pong) != pp_pong)
            bad++;
    }
    CHECK(bad == 0);
    CHECK(pp_count == PP_ROUNDS);
}

/* Wakeup with no sleeper is queued. Sleep consumes the queued wakeup instead of blocking. */
static void test_wakeup(void) {
    CHECK(func_00421710(0) == -1);
    CHECK(func_00421710(1) == 0);
    CHECK(func_00421730(1) == 1);
    CHECK(func_00421730(1) == 0);
    CHECK(func_00421710(1) == 0);
    CHECK(func_00421700() == 0);
    CHECK(func_00421730(1) == 0);
    CHECK(func_00421720(1) == 0);
    CHECK(func_00421730(1) == 1);
}

/* RotateThreadReadyQueue from outside: main moves X to the tail of priority 20, so Y runs
 * first when main sleeps. */
static void p_rot_x(void *a) {
    (void)a;
    mark('X');
    func_00421710(1);
}

static void p_rot_y(void *a) {
    (void)a;
    mark('Y');
}

static void test_rotate(void) {
    int32_t x = spawn(p_rot_x, 20);
    int32_t y = spawn(p_rot_y, 20);
    reset_order();
    CHECK(func_00421600(x, 0) == x);
    CHECK(func_00421600(y, 0) == y);
    CHECK(func_00421690(20) == 0);
    CHECK(func_00421700() == 0);
    CHECK(strcmp(order, "YX") == 0);
}

/* RotateThreadReadyQueue from a caller at that priority yields. X yields to Y, Y yields back,
 * and X finishes. The order is x, y, X. */
static void p_yield_x(void *a) {
    (void)a;
    mark('x');
    func_00421690(20);
    mark('X');
    func_00421710(1);
}

static void p_yield_y(void *a) {
    (void)a;
    mark('y');
    func_00421690(20);
}

static void test_rotate_yield(void) {
    int32_t x = spawn(p_yield_x, 20);
    int32_t y = spawn(p_yield_y, 20);
    reset_order();
    CHECK(func_00421600(x, 0) == x);
    CHECK(func_00421600(y, 0) == y);
    CHECK(func_00421700() == 0);
    CHECK(strcmp(order, "xyX") == 0);
}

/* ReferThreadStatus: running, dormant, ready, and waiting (seen from a thread while main
 * sleeps). Dormant reports the initial priority. */
static int32_t ref_main_status;

static void p_refer(void *a) {
    TestThreadParam info;
    (void)a;
    memset(&info, 0, sizeof info);
    if (func_004216e0(1, &info) == 0)
        ref_main_status = info.status;
    func_00421710(1);
}

static void test_refer(void) {
    TestThreadParam info;
    int32_t dormant;
    int32_t w;
    memset(&info, 0, sizeof info);
    CHECK(func_004216e0(1, &info) == 0);
    CHECK(info.status == THS_RUN);
    CHECK(info.currentPriority == 0);

    dormant = spawn(p_noop, 30);
    CHECK(func_004216e0(dormant, &info) == 0);
    CHECK(info.status == THS_DORMANT);
    CHECK(info.initialPriority == 30);

    w = spawn(p_refer, 30);
    CHECK(func_00421600(w, 0) == w);
    CHECK(func_004216e0(w, &info) == 0);
    CHECK(info.status == THS_READY);
    CHECK(func_004216f0(w, &info) == 0);

    ref_main_status = 0;
    CHECK(func_00421700() == 0);
    CHECK(ref_main_status == THS_WAIT);

    CHECK(func_004216e0(0, &info) == -1);
}

/* SuspendThread / ResumeThread. A suspended READY thread does not run while a lower-priority
 * one does; Resume puts it back. A thread suspended while asleep is WAIT|SUSPEND, and once
 * woken it stays SUSPEND until resumed. Dormant, running and already-suspended threads refuse. */
static int32_t sus_target;
static int sus_wait_status;

static void p_sus_victim(void *a) {
    (void)a;
    mark('S');
    func_00421710(1);
}

static void p_sus_waker(void *a) {
    (void)a;
    mark('W');
    func_00421710(1);
}

static void p_sus_sleeper(void *a) {
    (void)a;
    mark('a');
    func_00421700();
    mark('b');
    func_00421710(1);
}

static void p_sus_suspender(void *a) {
    TestThreadParam info;
    (void)a;
    mark('w');
    CHECK(func_00421750(sus_target) == 0);
    memset(&info, 0, sizeof info);
    CHECK(func_004216e0(sus_target, &info) == 0);
    sus_wait_status = info.status;
    CHECK(func_00421710(sus_target) == 0);
    func_00421710(1);
}

static void test_suspend(void) {
    TestThreadParam info;
    int32_t v = spawn(p_sus_victim, 30);
    int32_t w = spawn(p_sus_waker, 20);
    int32_t s;
    int32_t g;

    CHECK(func_00421750(v) == -1);          /* dormant */
    CHECK(func_00421770(v) == -1);          /* not suspended */
    CHECK(func_00421750(1) == -1);          /* the running thread */
    CHECK(func_00421750(0) == -1);          /* no such thread */
    CHECK(func_00421600(v, 0) == v);
    CHECK(func_00421750(v) == 0);
    CHECK(func_00421750(v) == -1);          /* already suspended */
    CHECK(func_00421780(v) == 0);
    CHECK(func_00421760(v) == 0);
    memset(&info, 0, sizeof info);
    CHECK(func_004216e0(v, &info) == 0);
    CHECK(info.status == THS_SUSPEND);

    reset_order();
    CHECK(func_00421600(w, 0) == w);
    CHECK(func_00421700() == 0);
    CHECK(strcmp(order, "W") == 0);         /* the suspended victim did not run */
    CHECK(func_00421770(v) == 0);
    CHECK(func_00421770(v) == -1);
    CHECK(func_00421700() == 0);
    CHECK(strcmp(order, "WS") == 0);

    /* WAIT-SUSPEND: suspended while asleep, woken, still suspended, then resumed. */
    s = spawn(p_sus_sleeper, 10);
    g = spawn(p_sus_suspender, 20);
    sus_target = s;
    reset_order();
    CHECK(func_00421600(s, 0) == s);
    CHECK(func_00421600(g, 0) == g);
    CHECK(func_00421700() == 0);
    CHECK(strcmp(order, "aw") == 0);
    CHECK(sus_wait_status == (THS_WAIT | THS_SUSPEND));
    memset(&info, 0, sizeof info);
    CHECK(func_004216e0(s, &info) == 0);
    CHECK(info.status == THS_SUSPEND);      /* woken, held back by the suspension */
    CHECK(func_00421780(s) == 0);           /* iResumeThread */
    CHECK(func_00421700() == 0);
    CHECK(strcmp(order, "awb") == 0);
}

/* DMAC and INTC handler tables, and SifDmaStat. */
static int32_t dummy_handler(int32_t cause) {
    return cause;
}

static void test_dmac(void) {
    CHECK(func_00421500(-1, dummy_handler, 0) == -1);
    CHECK(func_00421500(3, NULL, 0) == -1);
    CHECK(func_00421500(3, dummy_handler, 1) == -1);     /* inserting before a named handler is not modelled */
    CHECK(func_00421540(3) == -1);                       /* no handler yet */
    CHECK(func_00421500(3, dummy_handler, 0) == 3);
    CHECK(func_00421500(3, dummy_handler, 0) == -1);     /* one handler per cause */
    CHECK(func_00421540(3) == 0);
    CHECK(func_00421550(3) == 0);
    CHECK(func_00421510(3, 4) == -1);                    /* wrong handle */
    CHECK(func_00421510(3, 3) == 0);
    CHECK(func_00421510(3, 3) == -1);
    CHECK(func_00421b80(1) < 0);                         /* SifDmaStat: always complete */
}

static int intc_chain_result;

static int32_t intc_first(int32_t cause) {
    (void)cause;
    mark('I');
    return intc_chain_result;
}

static int32_t intc_last(int32_t cause) {
    (void)cause;
    mark('J');
    return 0;
}

/* GS CSR.FIELD flips on every VBLANK whether or not the INTC cause is enabled (it is GS state), and a
 * handler that returns a negative value ends the chain for that interrupt, as the PS2 kernel does:
 * the game's field gate (0x001026b0) relies on both. */
static void test_intc_chain(void) {
    const uint64_t field = p4pc_gs_csr_read();
    CHECK(p4pc_intc_disable(2) == 0);
    p4pc_intc_raise(2);
    CHECK(p4pc_gs_csr_read() == (field ^ (1ull << 13)));
    CHECK(p4pc_intc_stat_read() == 0);
    p4pc_intc_raise(2);
    CHECK(p4pc_gs_csr_read() == field);

    CHECK(p4pc_intc_add_handler(5, intc_first, 0) == 5);
    CHECK(p4pc_intc_add_handler(5, intc_last, -1) == 5 + 32);
    CHECK(p4pc_intc_enable(5) == 0);
    intc_chain_result = -1;
    reset_order();
    p4pc_intc_raise(5);
    CHECK(strcmp(order, "I") == 0);
    CHECK(p4pc_intc_stat_read() == 0);
    intc_chain_result = 0;
    reset_order();
    p4pc_intc_raise(5);
    CHECK(strcmp(order, "IJ") == 0);
    CHECK(RemoveIntcHandler(5, 5 + 32) == 0);
    CHECK(RemoveIntcHandler(5, 5) == 0);
}

/* PollSema: takes a count without blocking, fails at zero, and Signal fails past the maximum. */
static void test_pollsema(void) {
    int32_t s = make_sema(1, 2);
    CHECK(s > 0);
    CHECK(func_00421830(s) == s);
    CHECK(func_00421830(s) == -1);
    CHECK(func_00421800(s) == s);
    CHECK(func_00421800(s) == s);
    CHECK(func_00421800(s) == -1);
    CHECK(func_00421830(s) == s);
    CHECK(func_00421830(s) == s);
    CHECK(func_00421830(s) == -1);
    CHECK(func_004217f0(s) == s);                        /* DeleteSema: the id is gone */
    CHECK(func_00421830(s) == -1);
    CHECK(func_004217f0(s) == -1);
}

/* An alarm fires from the idle wait. Its callback signals a semaphore that main waits on, and
 * it reschedules itself 1 ms later for three runs, then returns 0 and is removed. */
static int32_t al_sema;
static int32_t al_calls;

static uint32_t al_handler(int32_t id, void *arg) {
    (void)id;
    (void)arg;
    al_calls++;
    func_00421810(al_sema);
    return al_calls < 3 ? 1000u : 0u;
}

static void test_alarm(void) {
    int32_t id;
    int i;
    al_sema = make_sema(0, 8);
    al_calls = 0;
    id = p4pc_set_alarm(1000u, al_handler, NULL);
    CHECK(id > 0);
    for (i = 0; i < 3; i++)
        CHECK(func_00421820(al_sema) == al_sema);
    CHECK(al_calls == 3);
    CHECK(p4pc_cancel_alarm(id) == -1);
}


/* VSync: the handler runs from the 60 Hz tick with its cause bit set in the status word, and
 * the bit is clear again once it returns. */
static int32_t vs_sema;
static int vs_calls;
static int vs_bit_seen;

static int32_t vs_handler(int32_t cause) {
    vs_calls++;
    if (p4pc_intc_stat_read() & (1ull << cause))
        vs_bit_seen++;
    func_00421810(vs_sema);
    return 0;
}

static void test_vsync(void) {
    int i;
    vs_sema = make_sema(0, 8);
    vs_calls = 0;
    vs_bit_seen = 0;
    CHECK(p4pc_intc_add_handler(5, vs_handler, 7) == -1);  /* inserting before a named handler is not modelled */
    CHECK(p4pc_intc_add_handler(5, vs_handler, -1) == 5); /* -1 = append, as func_00418b18 passes */
    CHECK(p4pc_intc_add_handler(5, vs_handler, -1) == 5 + 32); /* a second handler chains behind the first */
    CHECK(RemoveIntcHandler(5, 5 + 32) == 0);
    CHECK(RemoveIntcHandler(5, 5 + 32) == -1);
    CHECK(RemoveIntcHandler(5, 5) == 0);
    CHECK(p4pc_intc_add_handler(2, vs_handler, 0) == 2);
    CHECK(p4pc_intc_enable(2) == 0);
    for (i = 0; i < 3; i++)
        CHECK(func_00421820(vs_sema) == vs_sema);
    CHECK(vs_calls >= 3);
    CHECK(vs_bit_seen == vs_calls);
    CHECK(p4pc_intc_stat_read() == 0);
}

/* Device completion is deferred to the next scheduler frame, after its VBLANK interrupt,
 * independently of how often the caller polls in the current update. */
static int32_t frame_alarm_sema;
static int frame_alarm_calls, frame_alarm_vsync;

static uint32_t frame_alarm_handler(int32_t id, void *arg) {
    (void)id;
    (void)arg;
    frame_alarm_calls++;
    frame_alarm_vsync = vs_calls;
    func_00421810(frame_alarm_sema);
    return 0;
}

static void test_frame_alarm(void) {
    int32_t id;
    int scheduled_vsync;

    frame_alarm_sema = make_sema(0, 1);
    id = p4pc_set_next_frame_alarm(frame_alarm_handler, NULL);
    scheduled_vsync = vs_calls;
    CHECK(id > 0 && frame_alarm_calls == 0);
    CHECK(func_00421820(frame_alarm_sema) == frame_alarm_sema);
    CHECK(frame_alarm_calls == 1 && frame_alarm_vsync == scheduled_vsync + 1);
    CHECK(p4pc_cancel_alarm(id) == -1);
    id = p4pc_set_next_frame_alarm(frame_alarm_handler, NULL);
    CHECK(p4pc_cancel_alarm(id) == 0);
    p4pc_vsync_wait();
    CHECK(frame_alarm_calls == 1);
    {
        uint32_t flips = p4pc_flip_count();
        int polls;

        CHECK(p4pc_set_next_frame_alarm(frame_alarm_handler, NULL) > 0);
        for (polls = 0; polls < 2000000 && frame_alarm_calls == 1; polls++)
            p4pc_poll_devices();
        CHECK(frame_alarm_calls == 2 && p4pc_flip_count() == flips);
    }
}

/* Exercise the actual synchronous hardware wrapper, not a copied wait loop.
 * The request is opaque here; only its two SDK helpers are test doubles. */
typedef struct {
    int loaded, updates, checks;
} HCdvd;
static HCdvd hs_request;
static int32_t hs_go, hs_join;

static int H_Cdvd_IsFileLoaded(HCdvd *request) {
    request->checks++;
    return request->loaded;
}

static void H_Cdvd_UpdateRequests(void) {
    hs_request.updates++;
}

#include "../overrides/00456150.c"

static uint32_t hs_alarm(int32_t id, void *arg) {
    (void)id;
    (void)arg;
    mark('!');
    CHECK(func_00421810(hs_go) == hs_go);
    return 0;
}

static void hs_worker(void *arg) {
    (void)arg;
    CHECK(func_00421820(hs_go) == hs_go);
    CHECK(hs_request.updates > 0);
    mark('W');
    hs_request.loaded = 1;
    CHECK(func_00421810(hs_join) == hs_join);
}

static void test_hardware_sync_wait(void) {
    int32_t worker, main_priority;
    uint64_t now;
    memset(&hs_request, 0, sizeof hs_request);
    hs_go = make_sema(0, 1);
    hs_join = make_sema(0, 1);
    main_priority = func_00421670(1, 40);
    CHECK(main_priority >= 0); /* this harness otherwise keeps main at priority 0 */
    worker = spawn(hs_worker, 20);
    CHECK(func_00421600(worker, 0) == worker);
    reset_order();
    CHECK(p4pc_set_alarm(1000, hs_alarm, NULL) > 0);
    H_Cdvd_ReadSync(&hs_request);
    CHECK(hs_request.loaded && hs_request.updates > 0);
    CHECK(hs_request.checks == hs_request.updates + 1);
    CHECK(strcmp(order, "!W") == 0);
    CHECK(func_00421820(hs_join) == hs_join);
    now = p4pc_scheduler_time_ns();
    H_Cdvd_ReadSync(&hs_request);
    CHECK(p4pc_scheduler_time_ns() == now); /* already loaded: no needless poll */
    CHECK(func_004217f0(hs_go) == hs_go);
    CHECK(func_004217f0(hs_join) == hs_join);
    CHECK(func_00421670(1, main_priority) == 40);
}

static uint64_t context_seen;
static void context_worker(void *unused) {
    (void)unused;
    context_seen = p4pc_thread_context_key();
}

static void test_context_incarnations(void) {
    uint64_t main_key = p4pc_thread_context_key();
    CHECK(main_key != 0 && (main_key & 127u) == 1);
    int32_t old_priority = func_00421670(1, 50);
    CHECK(old_priority >= 0);
    int32_t child = spawn(context_worker, 40);
    CHECK(child > 1);
    CHECK(func_00421600(child, 0) == child);
    uint64_t first = context_seen;
    CHECK(first != 0 && first != main_key && (first & 127u) == (uint32_t)child);
    CHECK(p4pc_thread_context_key() == main_key);
    CHECK(func_00421600(child, 0) == child); /* restart the same dormant slot */
    CHECK(context_seen != first && (context_seen & 127u) == (uint32_t)child);
    CHECK(p4pc_thread_context_key() == main_key);
    CHECK(func_00421670(1, old_priority) == 50);
}

static uint32_t small_stack_local;
static void small_stack_worker(void *unused) {
    (void)unused;
    volatile uint32_t local = 0;
    small_stack_local = P4_H2G(&local);
    func_00421620();
}

static void test_small_stack_relocation(void) {
    void *stack = p4_guest_stack_alloc(4096);
    CHECK(stack != NULL);
    if (!stack) return;
    const uint32_t address = P4_H2G(stack);
    TestThreadParam p = {0}, info = {0};
    p.function = P4_FN_H2G(small_stack_worker);
    p.stack = address;
    p.stackSize = 4096;
    p.initialPriority = 40;
    int32_t old_priority = func_00421670(1, 50);
    int32_t child = func_004215e0(&p);
    CHECK(child > 1);
    if (child > 1) {
        CHECK(func_004216e0(child, &info) == 0);
        CHECK(info.stack == address && info.stackSize == 4096);
        small_stack_local = 0;
        CHECK(func_00421600(child, 0) == child);
        CHECK(small_stack_local != 0 &&
              (small_stack_local < address || small_stack_local >= address + 4096));
        /* Reusing the exited slot releases its owned coroutine stack. */
        CHECK(spawn(p_noop, 40) == child);
    }
    CHECK(func_00421670(1, old_priority) == 50);
    p4_guest_stack_free(stack);
}

static void test_stack_admission(void) {
    /* Reserve without running: neither declared nor enlarged native stacks
     * may become writable just because a coroutine is still dormant. */
    void *stack = p4_guest_stack_alloc(4096);
    CHECK(stack != NULL);
    if (stack == NULL) return;
    uint32_t address = P4_H2G(stack);
    CHECK(!p4pc_thread_guest_stack_overlap(address, 4096));
    TestThreadParam p;
    memset(&p, 0, sizeof p);
    p.function = P4_FN_H2G(p_noop);
    p.stack = address;
    p.stackSize = 4096;
    p.initialPriority = 40;
    CHECK(func_004215e0(&p) > 1);
    uint32_t flips = p4pc_flip_count();
    CHECK(p4pc_thread_guest_stack_overlap(address, 4096));
    CHECK(p4pc_thread_guest_stack_overlap(address - 1, 2));
    CHECK(p4pc_thread_guest_stack_overlap(address + 4095, 1));
    CHECK(!p4pc_thread_guest_stack_overlap(address + 4096, 1));
    CHECK(!p4pc_thread_guest_stack_overlap(address, 0));
    CHECK(p4pc_thread_guest_stack_overlap(UINT32_MAX - 1, 4));
    CHECK(p4pc_thread_guest_stack_overlap(4, SIZE_MAX));
    CHECK(p4pc_flip_count() == flips);
}


int main(void) {
    if (p4t_guest_init() != 0) {   /* the default thread stacks come from the guest arena */
        return 1;
    }
    CHECK(p4pc_thread_context_key() == 0);
    test_priority();
    test_pingpong();
    test_wakeup();
    test_rotate();
    test_rotate_yield();
    test_refer();
    test_suspend();
    test_dmac();
    test_intc_chain();
    test_pollsema();
    test_alarm();
    test_hardware_sync_wait();
    test_vsync();
    test_frame_alarm();
    test_small_stack_relocation();
    test_stack_admission();
    test_context_incarnations();
    if (failures) {
        fprintf(stderr, "kernel_threads: %d failure(s)\n", failures);
        return 1;
    }
    printf("kernel_threads: all checks passed\n");
    return 0;
}

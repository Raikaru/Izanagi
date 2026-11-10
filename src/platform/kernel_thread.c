/* PS2 thread, semaphore and alarm kernel: a port-owned model of the EE kernel calls the game
 * makes (syscalls 0x20..0x45 and their negative interrupt-context variants). The wrappers sit
 * at their retail addresses, so the game's calls land here.
 *
 * Model:
 *  - Threads are coroutines (p4_coro in include/p4pc/os.h: ucontext on Linux, fibers on Windows)
 *    on the one host thread. Scheduling is cooperative with
 *    priority preemption. A thread gives up the CPU when it blocks, exits, yields with
 *    RotateThreadReadyQueue, or when a higher-priority thread becomes ready inside a kernel
 *    call. A thread that never calls the kernel is never interrupted.
 *  - Priorities are 0..127; a lower number runs first. Threads of equal priority run in FIFO
 *    order. Thread id 1 is the boot thread, priority 0 [INFERENCE: the retail boot priority is
 *    not confirmed]. Ids are slot numbers, 1..64; 0 is never a thread.
 *  - Timers are serviced at kernel-call entry and while the scheduler is idle. Idle sleeps until
 *    the next timer is due, so the model does not spin. Timers are the alarms and VBLANK_START
 *    at 60 Hz (INTC cause 2, which p4pc_intc_raise dispatches). Callbacks run in interrupt
 *    context: the i-variant wrappers do not switch threads, and a callback that blocks is fatal.
 *  - Failures return -1. Error codes are not modelled.
 *    Successful wake/suspend/resume and thread-status calls return zero;
 *    thread-status flags are written to the result object.
 *
 * ThreadParam is the layout the game uses (HSfdThreadParam in h_sfdply.c): status, function,
 * stack, stackSize, gpReg, initialPriority, currentPriority, attributes, option. SemaParam is
 * the PS2 SDK layout (attr, option, initCount, maxCount) [INFERENCE: the decomp has no SemaParam
 * use to confirm it]. ThreadParam is guest memory: its function is a guest code address, its stack and gp
 * guest addresses (u32), so the layout is the same on a 32-bit and a 64-bit host.
 *
 * Alarms: p4pc_set_alarm / p4pc_cancel_alarm are the model API (the SetAlarm wrapper address
 * is not mapped yet). The callback returns the delay in microseconds until its next run, 0 to
 * stop. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "p4pc/guest.h"
#include "p4pc/os.h"
#include "p4pc/guest.h"
#include "p4pc/platform.h"
#include "p4pc/frame_alarm_seam.h"
#include "p4pc/vblank_hook.h"
#include "p4pc/pace.h"
#ifdef P4_MOD_GAME_BINDINGS
#include "p4pc/mod_entry.h"
#endif

#define KT_MAX_THREADS 64
#define KT_MAX_SEMAS 64
#define KT_MAX_ALARMS 16
#define KT_PRIORITIES 128
#define KT_MAIN_ID 1
#define KT_MAIN_PRIORITY 0
#define KT_DEFAULT_STACK (64u * 1024u)
/* The game's own thread stacks are as small as the PS2 code needed (the ADX server thread has 4 KB); the same code compiled for a 64-bit host, and the host
 * library calls it makes (printf needs more than 4 KB of stack on Windows, where the frames are checked against the stack limit), need more. On a 64-bit host
 * a thread whose stack is smaller than this runs on a default stack of the guest stack arena; the game never addresses the memory of its own stack. */
#define KT_MIN_NATIVE_STACK (64u * 1024u)
#define KT_VBLANK_START 2
#define KT_VSYNC_NS (1000000000ull / 60u)

/* ReferThreadStatus status values (PS2 THS_*). */
#define THS_RUN 0x01
#define THS_READY 0x02
#define THS_WAIT 0x04
#define THS_SUSPEND 0x08
#define THS_DORMANT 0x10

enum kt_state { KT_DORMANT, KT_READY, KT_RUN, KT_WAIT };

typedef struct kt_thread_param {
    int32_t status;
    uint32_t function; /* guest code address */
    uint32_t stack;    /* guest address */
    int32_t stackSize;
    uint32_t gpReg;    /* guest address */
    int32_t initialPriority;
    int32_t currentPriority;
    uint32_t attributes;
    uint32_t option;
} kt_thread_param;

_Static_assert(sizeof(kt_thread_param) == 36, "ThreadParam layout");
_Static_assert(offsetof(kt_thread_param, function) == 4, "ThreadParam.function offset");
_Static_assert(offsetof(kt_thread_param, stack) == 8, "ThreadParam.stack offset");
_Static_assert(offsetof(kt_thread_param, stackSize) == 12, "ThreadParam.stackSize offset");
_Static_assert(offsetof(kt_thread_param, gpReg) == 16, "ThreadParam.gpReg offset");
_Static_assert(offsetof(kt_thread_param, option) == 32, "ThreadParam.option offset");

typedef struct kt_sema_param {
    uint32_t attr;
    uint32_t option;
    int32_t initCount;
    int32_t maxCount;
} kt_sema_param;

typedef struct kt_thread {
    int used;
    enum kt_state state;
    int32_t priority;
    int32_t init_priority;
    uint32_t entry;    /* guest code address */
    uint32_t arg;      /* an integer word: the thread's entry takes it as a pointer or an integer, the table adapts */
    void *stack;       /* host pointer into guest memory */
    uint32_t stack_size;
    void *owned_stack; /* a default stack (p4_guest_stack_alloc) */
    uint32_t guest_stack;       /* what the game declared (ThreadParam), reported back by ReferThreadStatus when the thread runs on an owned stack */
    int32_t guest_stack_size;
    uint32_t gp;       /* guest address */
    uint32_t attr;
    uint32_t option;
    int32_t wakeup_count;
    int sleeping;
    int suspended;
    int32_t wait_sema;
    int32_t wait_result;
    uint64_t wake_ns; /* timed wait (p4pc_vsync_wait): the time it ends, 0 when not timed */
    uint64_t order;
    p4_coro *co;
    uint64_t incarnation; /* host-only identity, renewed when a native coroutine starts */
} kt_thread;

typedef struct kt_sema {
    int used;
    int32_t count;
    int32_t max;
} kt_sema;

typedef struct kt_alarm {
    int used;
    int32_t id;
    uint64_t due_ns;
    uint32_t (*handler)(int32_t, void *);
    void *arg;
} kt_alarm;

static kt_thread kt_threads[KT_MAX_THREADS + 1];
static kt_sema kt_semas[KT_MAX_SEMAS + 1];
static kt_alarm kt_alarms[KT_MAX_ALARMS + 1];
static int32_t kt_cur;
static int kt_inited;
static int kt_in_intr;
static int kt_dispatch_disabled;
static uint64_t kt_order;
static uint64_t kt_incarnation;
static uint64_t kt_vsync_next_ns;

static void kt_fatal(const char *why) {
    fprintf(stderr, "p4pc kernel: %s (thread %d)\n", why, (int)kt_cur);
    exit(3);
}

/* Virtual runs advance at kernel waits and deterministic busy-poll safepoints.
 * Ordinary play uses the monotonic host clock; neither path restores PS2 media
 * bandwidth or instruction-execution delays. */
static uint64_t kt_virtual_ns = 1000000000ull;
static unsigned kt_virtual_polls;

static int kt_virtual_clock(void) {
    return p4_os_virtual_clock();
}

/* Practice speed (p4pc_pace_request, the mod services' p4.pace): real-time
 * play only. kt_pace_quarters is the effective rate in quarters, 0 while no
 * pace was ever applied, so the clock is then exactly the host's. Paced, the
 * scheduler clock is kt_pace_sched + (host - kt_pace_host) * quarters / 4; a
 * new rate is taken at a VBLANK with the anchor at the current scheduler
 * time, so the clock never jumps. Every timer (VBLANK, alarms, timed waits)
 * keeps its spacing in scheduler time: the game runs the same updates per
 * frame, the host only waits less or more between them. */
static unsigned kt_pace_quarters, kt_pace_pending;
static uint64_t kt_pace_host, kt_pace_sched;
static uint64_t kt_pace_periods; /* VBLANK periods run at a rate other than 1x, for the change log */

static uint64_t kt_paced(uint64_t host) {
    return kt_pace_sched + (host - kt_pace_host) * kt_pace_quarters / 4u;
}

static uint64_t kt_now_ns(void) {
    if (!kt_virtual_clock())
        return kt_pace_quarters ? kt_paced(p4_os_now_ns()) : p4_os_now_ns();
    if (++kt_virtual_polls >= 4096u) {
        kt_virtual_polls = 0;
        kt_virtual_ns += 1000u;
    }
    return kt_virtual_ns;
}

/* 1..16 quarters; refused (-1) on a virtual clock, where time is the run's. */
int p4pc_pace_request(unsigned quarters) {
    if (kt_virtual_clock() || quarters < 1 || quarters > 16)
        return -1;
    kt_pace_pending = quarters;
    return 0;
}

unsigned p4pc_pace_effective(void) {
    return kt_pace_quarters ? kt_pace_quarters : 4u;
}

void (*p4pc_pace_audio_hook)(int frame_driven);

/* At a VBLANK: the pending rate, anchored at the current scheduler time. */
static void kt_pace_apply(void) {
    const uint64_t host = p4_os_now_ns();
    const uint64_t sched = kt_pace_quarters ? kt_paced(host) : host;
    const unsigned before = kt_pace_quarters ? kt_pace_quarters : 4u;
    const unsigned after = kt_pace_pending;
    kt_pace_pending = 0;
    if (after == before)
        return;
    kt_pace_host = host;
    kt_pace_sched = sched;
    kt_pace_quarters = after;
    fprintf(stderr, "p4pc: practice speed %u/4 (was %u/4) at flip %u, paced VBLANK periods %llu\n", after, before,
            (unsigned)p4pc_flip_count(), (unsigned long long)kt_pace_periods);
    if ((before != 4u) != (after != 4u) && p4pc_pace_audio_hook != NULL)
        p4pc_pace_audio_hook(after != 4u);
}

uint64_t p4pc_scheduler_time_ns(void) {
    return kt_virtual_ns;
}

uint64_t p4pc_thread_context_key(void) {
    if (!kt_inited || kt_cur < 1 || kt_cur > KT_MAX_THREADS) return 0;
    return (kt_threads[kt_cur].incarnation << 7) | (uint32_t)kt_cur;
}

static void kt_init(void) {
    if (kt_inited)
        return;
    kt_inited = 1;
    kt_threads[KT_MAIN_ID].used = 1;
    kt_threads[KT_MAIN_ID].co = p4_coro_adopt();
    if (kt_threads[KT_MAIN_ID].co == NULL)
        kt_fatal("cannot adopt the boot thread as a coroutine");
    kt_threads[KT_MAIN_ID].incarnation = ++kt_incarnation;
    kt_threads[KT_MAIN_ID].state = KT_RUN;
    kt_threads[KT_MAIN_ID].priority = KT_MAIN_PRIORITY;
    kt_threads[KT_MAIN_ID].init_priority = KT_MAIN_PRIORITY;
    kt_cur = KT_MAIN_ID;
    kt_vsync_next_ns = kt_now_ns() + KT_VSYNC_NS;
}

static kt_thread *kt_thr(int32_t id) {
    if (id < 1 || id > KT_MAX_THREADS || !kt_threads[id].used)
        return NULL;
    return &kt_threads[id];
}

static kt_sema *kt_sema_get(int32_t id) {
    if (id < 1 || id > KT_MAX_SEMAS || !kt_semas[id].used)
        return NULL;
    return &kt_semas[id];
}

/* The READY thread that runs next: lowest priority number, then earliest in FIFO order. */
static int32_t kt_pick(void) {
    int32_t best = 0;
    int32_t id;
    for (id = 1; id <= KT_MAX_THREADS; id++) {
        kt_thread *t = &kt_threads[id];
        if (!t->used || t->state != KT_READY || t->suspended)
            continue;
        if (best == 0 || t->priority < kt_threads[best].priority ||
            (t->priority == kt_threads[best].priority && t->order < kt_threads[best].order))
            best = id;
    }
    return best;
}

/* The thread a SignalSema on `sema` wakes: a semaphore waiter, highest priority, then FIFO. */
static int32_t kt_sema_waiter(int32_t sema) {
    int32_t best = 0;
    int32_t id;
    for (id = 1; id <= KT_MAX_THREADS; id++) {
        kt_thread *t = &kt_threads[id];
        if (!t->used || t->state != KT_WAIT || t->sleeping || t->wait_sema != sema)
            continue;
        if (best == 0 || t->priority < kt_threads[best].priority ||
            (t->priority == kt_threads[best].priority && t->order < kt_threads[best].order))
            best = id;
    }
    return best;
}

/* The first READY thread at `prio`, for RotateThreadReadyQueue. */
static int32_t kt_head_ready(int32_t prio) {
    int32_t best = 0;
    int32_t id;
    for (id = 1; id <= KT_MAX_THREADS; id++) {
        kt_thread *t = &kt_threads[id];
        if (!t->used || t->state != KT_READY || t->suspended || t->priority != prio)
            continue;
        if (best == 0 || t->order < kt_threads[best].order)
            best = id;
    }
    return best;
}

static void kt_make_ready(kt_thread *t, int32_t result) {
    t->state = KT_READY;
    t->wait_result = result;
    t->sleeping = 0;
    t->wait_sema = 0;
    t->wake_ns = 0;
    t->order = ++kt_order;
}

/* The earliest pending timer. VBLANK_START counts only while its handler is enabled. */
static int kt_next_due(uint64_t *due) {
    int have = 0;
    uint64_t best = 0;
    int32_t i;
    if (p4pc_intc_is_enabled(KT_VBLANK_START)) {
        best = kt_vsync_next_ns;
        have = 1;
    }
    for (i = 1; i <= KT_MAX_ALARMS; i++) {
        if (kt_alarms[i].used && (!have || kt_alarms[i].due_ns < best)) {
            best = kt_alarms[i].due_ns;
            have = 1;
        }
    }
    for (i = 1; i <= KT_MAX_THREADS; i++) {
        if (kt_threads[i].used && kt_threads[i].state == KT_WAIT && kt_threads[i].wake_ns != 0 &&
            (!have || kt_threads[i].wake_ns < best)) {
            best = kt_threads[i].wake_ns;
            have = 1;
        }
    }
    *due = best;
    return have;
}

/* No thread is runnable: sleep until the next timer. With no timer pending, nothing can wake
 * the blocked thread, so that is a fatal deadlock. */
static void kt_idle(void) {
    uint64_t due;
    uint64_t now;
    if (!kt_next_due(&due))
        kt_fatal("blocked with no runnable thread and no pending alarm or VBLANK handler");
    now = kt_now_ns();
    if (due > now) {
        if (kt_virtual_clock()) {
            kt_virtual_ns = due;
        } else if (!kt_pace_quarters) {
            const uint64_t spin = 300000u;
            if (due - now > spin)
                p4_os_sleep_ns(due - now - spin);
            while (p4_os_now_ns() < due) {
            }
        } else {
            /* Paced: the scheduler interval in host time. */
            const uint64_t spin = 300000u;
            const uint64_t wait = (due - now) * 4u / kt_pace_quarters;
            if (wait > spin)
                p4_os_sleep_ns(wait - spin);
            while (kt_now_ns() < due) {
            }
        }
    }
}

/* Runs the due timers: VBLANK_START at 60 Hz, then each alarm whose time has come. On a virtual
 * clock every VBLANK period that has passed first runs p4pc_vblank_period_hook (the mixer renders
 * that period's audio: the SPU2 plays in real time, so a game update that spans two vsyncs hears
 * two periods), whether or not the VBLANK interrupt is enabled. */
void (*p4pc_vblank_period_hook)(void);

static void kt_service_timers(void) {
    uint64_t now;
    int32_t i;
    if (kt_in_intr)
        return;
    kt_in_intr = 1;
    now = kt_now_ns();
    if (now >= kt_vsync_next_ns) {
        uint64_t periods = (now - kt_vsync_next_ns) / KT_VSYNC_NS + 1;
        if (kt_pace_quarters && kt_pace_quarters != 4u)
            kt_pace_periods += periods;
        kt_vsync_next_ns += KT_VSYNC_NS;
        if (kt_vsync_next_ns <= now)
            kt_vsync_next_ns = now + KT_VSYNC_NS;
        if ((kt_virtual_clock() || (kt_pace_quarters && kt_pace_quarters != 4u)) && p4pc_vblank_period_hook != NULL) {
            while (periods-- > 0)
                p4pc_vblank_period_hook();
        }
        p4pc_intc_raise(KT_VBLANK_START);
        if (kt_pace_pending)
            kt_pace_apply();
    }
    for (i = 1; i <= KT_MAX_THREADS; i++) {
        kt_thread *t = &kt_threads[i];
        if (t->used && t->state == KT_WAIT && t->wake_ns != 0 && now >= t->wake_ns)
            kt_make_ready(t, 0);
    }
    for (i = 1; i <= KT_MAX_ALARMS; i++) {
        kt_alarm *a = &kt_alarms[i];
        uint32_t again;
        if (!a->used || now < a->due_ns)
            continue;
        again = a->handler(a->id, a->arg);
        if (!a->used)
            continue; /* the callback cancelled its own alarm */
        if (again == 0)
            a->used = 0;
        else
            a->due_ns = now + (uint64_t)again * 1000u;
    }
    kt_in_intr = 0;
}

/* Gives the CPU to the next READY thread. The caller has already set its own state. Returns
 * when the caller runs again, or never, for a thread that exited. */
static void kt_switch_away(void) {
    if (kt_in_intr)
        kt_fatal("thread blocked inside an interrupt callback");
    for (;;) {
        int32_t next;
        kt_service_timers();
        next = kt_pick();
        if (next != 0) {
            int32_t prev = kt_cur;
            kt_threads[next].state = KT_RUN;
            if (next == prev)
                return;
            kt_cur = next;
            p4_coro_switch(kt_threads[prev].co, kt_threads[next].co);
            return;
        }
        kt_idle();
    }
}

/* Preempts the running thread when a READY thread outranks it. Not during an interrupt
 * callback, and not with dispatch disabled. */
static void kt_reschedule(void) {
    kt_thread *cur;
    int32_t next;
    if (kt_in_intr || kt_dispatch_disabled)
        return;
    next = kt_pick();
    cur = &kt_threads[kt_cur];
    if (next == 0 || kt_threads[next].priority >= cur->priority)
        return;
    cur->state = KT_READY;
    cur->order = ++kt_order;
    kt_switch_away();
}

/* Normal kernel calls run timers on entry and reschedule on exit. The i-variants do neither. */
static void kt_enter(void) {
    kt_init();
    kt_service_timers();
}

static void kt_leave(void) {
    kt_reschedule();
}

void p4pc_poll_devices(void) {
    kt_init();
    if (kt_virtual_clock())
        kt_virtual_ns += 1000u;
    kt_service_timers();
    if (p4pc_frame_alarm_seam)
        p4pc_frame_alarm_seam->poll();
    kt_leave();
}


/* Debug aid for gdb (call p4pc_kt_dump()): lists the live kernel threads, their state and what they wait on. */
void p4pc_kt_dump(void) {
    static const char *const names[] = {"DORMANT", "READY", "RUN", "WAIT"};
    int32_t i;
    for (i = 1; i <= KT_MAX_THREADS; i++) {
        const kt_thread *t = &kt_threads[i];
        if (!t->used)
            continue;
        fprintf(stderr, "kt %2d %-7s prio %3d entry %08x sleeping %d suspended %d wait_sema %d (count %d)%s\n", i,
                names[t->state], t->priority, (unsigned)t->entry, t->sleeping, t->suspended, t->wait_sema,
                t->wait_sema > 0 ? kt_semas[t->wait_sema].count : 0, i == kt_cur ? " <- current" : "");
    }
}

static void kt_exit_current(void) {
    kt_threads[kt_cur].state = KT_DORMANT;
    kt_switch_away();
    kt_fatal("exited thread resumed");
}

static void kt_trampoline(void) {
    kt_thread *t = &kt_threads[kt_cur];
    p4pc_fpu_state_log("at the start of a game thread"); /* P4PC_FPU_LOG: a coroutine runs in the EE float mode like the thread that created it */
    ((void (*)(uint32_t))p4_fn_g2h_sig(t->entry, P4_SIG(1, 0, 0, 0, 0, 0, 0, 0)))(t->arg);
    kt_exit_current();
}

/* CreateThread (0x20). */
int32_t func_004215e0(void *param) {
    kt_thread_param *p = (kt_thread_param *)param;
    void *owned = NULL;
    kt_thread *t;
    int32_t id;
    kt_init();
    if (p == NULL || p->function == 0 || p->initialPriority < 0 ||
        p->initialPriority >= KT_PRIORITIES)
        return -1;
    if (p->stack == 0) {
        owned = p4_guest_stack_alloc(KT_DEFAULT_STACK); /* a thread without a stack of its own (the model's tests): the guest stack arena (host memory on i386) */
        if (owned == NULL)
            return -1;
    } else if (p->stackSize <= 0) {
        return -1;
    }
#if __SIZEOF_POINTER__ == 8
    else if ((uint32_t)p->stackSize < KT_MIN_NATIVE_STACK) {
        owned = p4_guest_stack_alloc(KT_DEFAULT_STACK);
        if (owned == NULL)
            return -1;
    }
#endif
    for (id = 2; id <= KT_MAX_THREADS && kt_threads[id].used; id++) {
    }
    if (id > KT_MAX_THREADS) {
        p4_guest_stack_free(owned);
        return -1;
    }
    t = &kt_threads[id];
    p4_guest_stack_free(t->owned_stack);
    p4_coro_destroy(t->co);
    memset(t, 0, sizeof *t);
    t->used = 1;
    t->state = KT_DORMANT;
    t->entry = p->function;
    t->gp = p->gpReg;
    t->init_priority = p->initialPriority;
    t->priority = p->initialPriority;
    t->attr = p->attributes;
    t->option = p->option;
    t->owned_stack = owned;
    t->guest_stack = p->stack;
    t->guest_stack_size = p->stackSize;
    t->stack = owned != NULL ? owned : P4_G2H(p->stack);
    t->stack_size = owned != NULL ? KT_DEFAULT_STACK : (uint32_t)p->stackSize;
    return id;
}

/* StartThread (0x22): makes a dormant thread runnable at its initial priority. */
int32_t func_00421600(int32_t id, uint32_t arg) {
    kt_thread *t;
    kt_enter();
    t = kt_thr(id);
    if (t == NULL || t->state != KT_DORMANT)
        return -1;
    p4_coro_destroy(t->co);
    t->co = p4_coro_create(t->stack, t->stack_size, kt_trampoline);
    if (t->co == NULL)
        return -1;
    t->incarnation = ++kt_incarnation;
    t->arg = arg;
    t->priority = t->init_priority;
    kt_make_ready(t, 0);
    kt_leave();
    return id;
}

/* ExitThread (0x23). */
void func_00421610(void) {
    kt_init();
    kt_exit_current();
}

/* ExitDeleteThread (0x24): the slot is free for CreateThread once the thread has switched out. */
void func_00421620(void) {
    kt_init();
    kt_threads[kt_cur].used = 0;
    kt_exit_current();
}

/* DisableDispatchThread (0x27) and EnableDispatchThread (0x28). */
int32_t func_00421650(void) {
    kt_enter();
    kt_dispatch_disabled = 1;
    return 0;
}

int32_t func_00421660(void) {
    kt_enter();
    kt_dispatch_disabled = 0;
    kt_leave();
    return 0;
}

static int32_t kt_change_priority(int32_t id, int32_t prio) {
    kt_thread *t = kt_thr(id);
    int32_t old;
    if (t == NULL || prio < 0 || prio >= KT_PRIORITIES)
        return -1;
    old = t->priority;
    t->priority = prio;
    if (t->state == KT_READY)
        t->order = ++kt_order;
    return old;
}

/* ChangeThreadPriority (0x29) returns the previous priority. A READY thread joins the tail of
 * its new priority's queue. */
int32_t func_00421670(int32_t id, int32_t prio) {
    int32_t r;
    kt_enter();
    r = kt_change_priority(id, prio);
    kt_leave();
    return r;
}

/* iChangeThreadPriority (-42): interrupt context, no reschedule. */
int32_t func_00421680(int32_t id, int32_t prio) {
    kt_init();
    return kt_change_priority(id, prio);
}

/* RotateThreadReadyQueue (0x2b): moves the head of the queue at `prio` to its tail. A caller at
 * that priority yields to the next thread in the queue. */
static int32_t kt_rotate(int32_t prio, int may_yield) {
    kt_thread *cur;
    int32_t head;
    if (prio < 0 || prio >= KT_PRIORITIES)
        return -1;
    cur = &kt_threads[kt_cur];
    if (may_yield && !kt_dispatch_disabled && cur->priority == prio) {
        cur->state = KT_READY;
        cur->order = ++kt_order;
        kt_switch_away();
        return 0;
    }
    head = kt_head_ready(prio);
    if (head != 0)
        kt_threads[head].order = ++kt_order;
    return 0;
}

int32_t func_00421690(int32_t prio) {
    int32_t r;
    kt_enter();
    r = kt_rotate(prio, 1);
    kt_leave();
    return r;
}

/* iRotateThreadReadyQueue (-44). */
int32_t func_004216a0(int32_t prio) {
    kt_init();
    return kt_rotate(prio, 0);
}

static int32_t kt_release_wait(int32_t id) {
    kt_thread *t = kt_thr(id);
    if (t == NULL || t->state != KT_WAIT)
        return -1;
    kt_make_ready(t, -1);
    return 0;
}

/* ReleaseWaitThread (0x2d): the released wait returns -1. */
int32_t func_004216b0(int32_t id) {
    int32_t r;
    kt_enter();
    r = kt_release_wait(id);
    kt_leave();
    return r;
}

/* iReleaseWaitThread (-46). */
int32_t func_004216c0(int32_t id) {
    kt_init();
    return kt_release_wait(id);
}

/* GetThreadId (0x2f). */
int32_t func_004216d0(void) {
    kt_init();
    return kt_cur;
}

static int32_t kt_refer(int32_t id, kt_thread_param *info) {
    kt_thread *t = kt_thr(id);
    int32_t status;
    if (t == NULL || info == NULL)
        return -1;
    switch (t->state) {
    case KT_RUN:
        status = THS_RUN;
        break;
    case KT_READY:
        status = THS_READY;
        break;
    case KT_WAIT:
        status = THS_WAIT;
        break;
    default:
        status = THS_DORMANT;
        break;
    }
    if (t->suspended) {
        /* SUSPEND replaces READY; a waiting thread reports WAIT | SUSPEND. */
        status = t->state == KT_WAIT ? (THS_WAIT | THS_SUSPEND) : THS_SUSPEND;
    }
    info->status = status;
    info->function = t->entry;
    /* a default stack is host memory or the stack arena: what the game declared is what it is told */
    info->stack = t->guest_stack != 0 ? t->guest_stack : (t->owned_stack != NULL ? 0 : P4_H2G(t->stack));
    info->stackSize = t->guest_stack != 0 ? t->guest_stack_size : (int32_t)t->stack_size;
    info->gpReg = t->gp;
    info->initialPriority = t->init_priority;
    info->currentPriority = t->priority;
    info->attributes = t->attr;
    info->option = t->option;
    return 0;
}

/* ReferThreadStatus (0x30). */
int32_t func_004216e0(int32_t id, void *info) {
    int32_t r;
    kt_enter();
    r = kt_refer(id, (kt_thread_param *)info);
    kt_leave();
    return r;
}

/* iReferThreadStatus (-49). */
int32_t func_004216f0(int32_t id, void *info) {
    kt_init();
    return kt_refer(id, (kt_thread_param *)info);
}

extern int32_t func_0042ba20(void);
extern int32_t func_0042ba70(void);

/* func_00422920 starts a dormant thread with `arg`. Retail refuses when called from interrupt
 * context (the port never runs in one), or when interrupts are already disabled (DIntr
 * returns 0): both return -1. With interrupts on, it reads the thread's status, returns the
 * error for an unknown id and -1 unless the thread is dormant (THS_DORMANT), pokes `arg` into
 * the thread's saved a0 and calls StartThread. The port's StartThread takes the argument
 * directly. Interrupts are restored before StartThread, as retail does. */
int32_t func_00422920(int32_t id, uint32_t arg) {
    kt_thread_param status;
    int32_t r;

    if (func_0042ba20() == 0)
        return -1;
    r = func_004216e0(id, &status);
    if (r < 0) {
        func_0042ba70();
        return r;
    }
    if (status.status != THS_DORMANT) {
        func_0042ba70();
        return -1;
    }
    func_0042ba70();
    return func_00421600(id, arg);
}

/* SleepThread (0x32): blocks until WakeupThread. A queued wakeup is consumed instead. */
int32_t func_00421700(void) {
    kt_thread *t;
    kt_enter();
    t = &kt_threads[kt_cur];
    if (t->wakeup_count > 0) {
        t->wakeup_count--;
        return 0;
    }
    t->state = KT_WAIT;
    t->sleeping = 1;
    t->order = ++kt_order;
    kt_switch_away();
    return t->wait_result;
}

/* A WakeupThread on a thread that is not asleep is queued, and the next SleepThread consumes it. */
static int32_t kt_wakeup(int32_t id) {
    kt_thread *t = kt_thr(id);
    if (t == NULL || t->state == KT_DORMANT)
        return -1;
    if (t->state == KT_WAIT && t->sleeping)
        kt_make_ready(t, 0);
    else
        t->wakeup_count++;
    return 0;
}

/* WakeupThread (0x33). */
int32_t func_00421710(int32_t id) {
    int32_t r;
    kt_enter();
    r = kt_wakeup(id);
    kt_leave();
    return r;
}

/* iWakeupThread (-52). */
int32_t func_00421720(int32_t id) {
    kt_init();
    return kt_wakeup(id);
}

/* func_004226a8: the interrupt-safe wake the sound server's vsync pass uses (retail 0x004226a8). The retail
 * body asks the kernel for the running thread id (syscall -47) and, when the target is a different
 * thread, calls iWakeupThread (func_00421720). When the target is the thread the interrupt landed on it
 * cannot wake it directly, so it queues the id and signals a semaphore for a helper thread to do it
 * later. Here a wake of the running thread just counts (kt_wakeup), which is what the helper achieves,
 * so both paths are iWakeupThread. */
int32_t func_004226a8(int32_t id) {
    return func_00421720(id);
}

/* SuspendThread (0x37): a READY thread becomes SUSPEND and a waiting one WAIT-SUSPEND (its wait
 * still ends normally, and it then stays suspended). Failures return -1: an unknown or dormant
 * thread, the running thread (a thread cannot suspend itself), or one already suspended. */
static int32_t kt_suspend(int32_t id) {
    kt_thread *t = kt_thr(id);
    if (t == NULL || t->state == KT_DORMANT || t->state == KT_RUN || t->suspended)
        return -1;
    t->suspended = 1;
    return 0;
}

/* ResumeThread (0x39): a SUSPEND thread rejoins the ready queue at the tail; a WAIT-SUSPEND one
 * goes back to WAIT. Fails with -1 for a thread that is not suspended. */
static int32_t kt_resume(int32_t id) {
    kt_thread *t = kt_thr(id);
    if (t == NULL || !t->suspended)
        return -1;
    t->suspended = 0;
    if (t->state == KT_READY)
        t->order = ++kt_order;
    return 0;
}

int32_t func_00421750(int32_t id) {
    int32_t r;
    kt_enter();
    r = kt_suspend(id);
    kt_leave();
    return r;
}

/* iSuspendThread (-56). */
int32_t func_00421760(int32_t id) {
    kt_init();
    return kt_suspend(id);
}

int32_t func_00421770(int32_t id) {
    int32_t r;
    kt_enter();
    r = kt_resume(id);
    kt_leave();
    return r;
}

/* iResumeThread (-58). */
int32_t func_00421780(int32_t id) {
    kt_init();
    return kt_resume(id);
}

/* CancelWakeupThread (0x35) returns the wakeup count it cleared. */
int32_t func_00421730(int32_t id) {
    kt_thread *t;
    int32_t n;
    kt_enter();
    t = kt_thr(id);
    if (t == NULL)
        return -1;
    n = t->wakeup_count;
    t->wakeup_count = 0;
    return n;
}

/* iCancelWakeupThread (-54). */
int32_t func_00421740(int32_t id) {
    kt_thread *t;
    int32_t n;
    kt_init();
    t = kt_thr(id);
    if (t == NULL)
        return -1;
    n = t->wakeup_count;
    t->wakeup_count = 0;
    return n;
}

/* CreateSema (0x40, the stub at 0x004217e0; 0x004217d0 is syscall 0x3f). The SemaParam layout is the
 * PS2 SDK one (see the file header). */
int32_t func_004217e0(void *param) {
    kt_sema_param *p = (kt_sema_param *)param;
    int32_t id;
    kt_init();
    if (p == NULL || p->maxCount <= 0 || p->initCount < 0 || p->initCount > p->maxCount)
        return -1;
    for (id = 1; id <= KT_MAX_SEMAS && kt_semas[id].used; id++) {
    }
    if (id > KT_MAX_SEMAS)
        return -1;
    kt_semas[id].used = 1;
    kt_semas[id].count = p->initCount;
    kt_semas[id].max = p->maxCount;
    return id;
}

/* DeleteSema (0x41, 0x004217f0): frees the semaphore; its waiters wake with -1 (retail reports a
 * wait-delete error; callers only test for a negative result). Returns the id, or -1. */
int32_t func_004217f0(int32_t id) {
    kt_sema *s;
    int32_t w;
    kt_enter();
    s = kt_sema_get(id);
    if (s == NULL)
        return -1;
    while ((w = kt_sema_waiter(id)) != 0)
        kt_make_ready(&kt_threads[w], -1);
    s->used = 0;
    kt_leave();
    return id;
}

/* A signal with a waiter wakes the highest-priority waiter and leaves the count alone. Without
 * one, the count goes up, and it fails at the maximum. */
static int32_t kt_signal(int32_t id) {
    kt_sema *s = kt_sema_get(id);
    int32_t best;
    if (s == NULL)
        return -1;
    best = kt_sema_waiter(id);
    if (best != 0) {
        kt_make_ready(&kt_threads[best], id);
        return id;
    }
    if (s->count >= s->max)
        return -1;
    s->count++;
    return id;
}

/* SignalSema (0x42). */
int32_t func_00421800(int32_t id) {
    int32_t r;
    kt_enter();
    r = kt_signal(id);
    kt_leave();
    return r;
}

/* iSignalSema (-67): interrupt context, no reschedule. */
int32_t func_00421810(int32_t id) {
    kt_init();
    return kt_signal(id);
}

/* WaitSema (0x44): takes a count, or blocks until a signal. Returns the id, or -1 if the wait
 * was released by ReleaseWaitThread. */
int32_t func_00421820(int32_t id) {
    kt_sema *s;
    kt_thread *t;
    kt_enter();
    s = kt_sema_get(id);
    if (s == NULL)
        return -1;
    if (s->count > 0) {
        s->count--;
        return id;
    }
    t = &kt_threads[kt_cur];
    t->state = KT_WAIT;
    t->sleeping = 0;
    t->wait_sema = id;
    t->order = ++kt_order;
    kt_switch_away();
    return t->wait_result;
}

/* PollSema (0x45): takes a count without blocking, or fails. */
int32_t func_00421830(int32_t id) {
    kt_sema *s;
    kt_enter();
    s = kt_sema_get(id);
    if (s == NULL || s->count <= 0)
        return -1;
    s->count--;
    return id;
}

/* Model API: waits for the next 60 Hz VBLANK_START tick, like a thread blocked on the GS vsync. The
 * calling thread sleeps (state WAIT with a wake time) so lower-priority threads, such as the game's
 * async loaders, run meanwhile; with none ready the host sleeps until the tick. WakeupThread and
 * semaphore signals do not end the wait; ReleaseWaitThread does. Pacing the frame flip with this call is
 * what lets a game loop that never blocks otherwise (kwlnMainLoop) share the CPU with its workers. */
static uint32_t kt_flips;

uint32_t p4pc_flip_count(void) {
    return kt_flips;
}

/* P4PC_DUMP_TASKS_FRAMES=a,b,c prints the game's task tree (kwlnTaskDumpAll) at those frame flips: where a boot that
 * went black is stuck, without a debugger. */
extern void kwlnTaskDumpAll(void) __attribute__((weak)); /* absent from the platform unit tests */

static void kt_dump_tasks_at_flip(uint32_t flip) {
    static const char *list;
    static int init;

    if (!init) {
        init = 1;
        list = getenv("P4PC_DUMP_TASKS_FRAMES");
    }
    if (list != NULL && kwlnTaskDumpAll != NULL) {
        const char *p = list;

        while (*p != 0) {
            char *end;
            unsigned long v = strtoul(p, &end, 10);

            if (end == p) {
                break;
            }
            if (v == flip) {
                kwlnTaskDumpAll();
                break;
            }
            p = *end == ',' ? end + 1 : end;
        }
    }
}

/* P4PC_PACE_LOG: legacy 600-flip wall-period summaries. Retail game flips normally
 * run at 30 Hz on the 60 Hz scheduler; its >20 ms count is not a hitch count.
 * P4PC_PACE_TRACE records individual periods and host-thread CPU accounting. */
static void kt_pace_note(void) {
    static int on = -1;
    static uint64_t last, sum, max;
    static uint64_t lastCpu = UINT64_MAX;
    static unsigned n, late;
    static FILE *trace;
    uint64_t now;
    uint64_t cpu = UINT64_MAX;

    if (on < 0) {
        const char *path = getenv("P4PC_PACE_TRACE");
        on = (getenv("P4PC_PACE_LOG") != NULL || (path != NULL && *path != '\0')) && !kt_virtual_clock();
        if (on && path != NULL && *path != '\0') {
            trace = fopen(path, "w");
            if (trace != NULL) {
                setvbuf(trace, NULL, _IOFBF, 65536);
#ifdef P4_GUEST64
                fprintf(stderr, "p4pc memory: guest-base %p bytes %u\n", (void *)p4_guest_base, (unsigned)P4_GUEST_SIZE);
#else
                fprintf(stderr, "p4pc memory: guest-base %p bytes %u\n", (void *)(uintptr_t)0, 0x02000000u);
#endif
                fprintf(trace, "flip,timestamp_ns,period_ns,thread_cpu_ns,thread_cpu_delta_ns\n");
            } else {
                fprintf(stderr, "p4pc pace: cannot open trace %s\n", path);
            }
        }
    }
    if (!on)
        return;
    now = p4_os_now_ns();
    if (trace != NULL && p4_os_thread_cpu_ns(&cpu) != 0) cpu = UINT64_MAX;
    if (last != 0) {
        const uint64_t d = now - last;
        if (trace != NULL) {
            const uint64_t delta = cpu != UINT64_MAX && lastCpu != UINT64_MAX && cpu >= lastCpu ? cpu - lastCpu : UINT64_MAX;
            fprintf(trace, "%u,%llu,%llu,%llu,%llu\n", (unsigned)kt_flips, (unsigned long long)now,
                    (unsigned long long)d, (unsigned long long)cpu, (unsigned long long)delta);
        }

        sum += d;
        if (d > max)
            max = d;
        if (d > 20000000u)
            late++;
        if (++n == 600u) {
            fprintf(stderr, "p4pc pace: flip %u, frame %.3f ms mean, %.3f ms longest, %u over 20 ms (of 600)\n", (unsigned)kt_flips, (double)sum / 600e6,
                    (double)max / 1e6, late);
            sum = max = 0;
            n = late = 0;
        }
        if (trace != NULL && n % 60u == 0)
            fflush(trace);
    }
    last = now;
    lastCpu = cpu;
}

/* Opt-in measurement only: actual game-thread CPU and wall windows. No guest
 * clock/state receives these values. Disabled runs make no clock queries here;
 * enabled runs query only at window boundaries, not on every frame. */
static void kt_frame_cpu_note(void) {
    static int on = -1, started;
    static uint32_t first;
    static uint64_t cpu_start, wall_start, quantum;
    uint32_t frames = kt_flips - first;
    uint64_t cpu, wall;

    if (on < 0)
        on = getenv("P4PC_FRAME_CPU_LOG") != NULL;
    if (!on || (started && frames < 1000u))
        return;
    if ((!started && (p4_os_thread_cpu_resolution_ns(&quantum) || !quantum)) ||
        p4_os_thread_cpu_ns(&cpu)) {
        fprintf(stderr, "p4pc frame_cpu: unavailable real thread CPU counter/resolution\n");
        on = 0;
        return;
    }
    wall = p4_os_now_ns();
    if (started) {
        if (cpu < cpu_start || wall < wall_start) {
            fprintf(stderr, "p4pc frame_cpu: non-monotonic host measurement\n");
            on = 0;
            return;
        }
        fprintf(stderr, "p4pc frame_cpu: first %u last %u frames %u cpu_ns %llu wall_ns %llu quantum_ns %llu virtual %u\n",
                (unsigned)first, (unsigned)kt_flips, (unsigned)frames,
                (unsigned long long)(cpu - cpu_start), (unsigned long long)(wall - wall_start),
                (unsigned long long)quantum, (unsigned)kt_virtual_clock());
    }
    cpu_start = cpu;
    wall_start = wall;
    first = kt_flips;
    started = 1;
}

void p4pc_wait_frame_boundary(void) {
    kt_thread *t;
    kt_enter();
    t = &kt_threads[kt_cur];
    t->wake_ns = kt_vsync_next_ns;
    t->state = KT_WAIT;
    t->sleeping = 0;
    t->wait_sema = 0;
    t->order = ++kt_order;
    kt_switch_away();
}

void p4pc_vsync_wait(void) {
    kt_flips++;
    kt_dump_tasks_at_flip(kt_flips);
    if (kt_virtual_clock() && kt_flips % 1000u == 0) {
        static uint64_t t0;
        uint64_t now = p4_os_now_ns();

        if (t0 != 0) {
            fprintf(stderr, "p4pc turbo: flip %u, %.1f flips/s over the last 1000\n", (unsigned)kt_flips, 1e12 / (double)(now - t0));
        }
        t0 = now;
    }
    p4pc_wait_frame_boundary();
    kt_pace_note();
    #if defined(P4_MOD_GAME_BINDINGS) && defined(P4_MOD_GUARDS)
    if ((p4_mod_event_mask & P4_MOD_FRAME_START_BIT) | p4_mod_safe_point)
        p4_mod_event_frame(kt_flips);
    #endif
    kt_frame_cpu_note();
}

static int32_t kt_set_alarm_at(uint64_t due_ns, uint32_t (*handler)(int32_t, void *), void *arg) {
    int32_t id;
    if (handler == NULL)
        return -1;
    for (id = 1; id <= KT_MAX_ALARMS && kt_alarms[id].used; id++) {
    }
    if (id > KT_MAX_ALARMS)
        return -1;
    kt_alarms[id].used = 1;
    kt_alarms[id].id = id;
    kt_alarms[id].due_ns = due_ns;
    kt_alarms[id].handler = handler;
    kt_alarms[id].arg = arg;
    return id;
}

/* Model alarms use deterministic logical time in both paced and unpaced runs. */
int32_t p4pc_set_alarm(uint32_t us, uint32_t (*handler)(int32_t, void *), void *arg) {
    kt_init();
    return kt_set_alarm_at(kt_now_ns() + (uint64_t)us * 1000u, handler, arg);
}

/* Lockstep disc replay only (src/platform/lockstep_disc.c); NULL in every other run. */
const P4FrameAlarmSeam *p4pc_frame_alarm_seam;

int32_t p4pc_set_frame_boundary_alarm(uint32_t (*handler)(int32_t, void *), void *arg) {
    kt_enter();
    return kt_set_alarm_at(kt_vsync_next_ns, handler, arg);
}

int32_t p4pc_set_next_frame_alarm(uint32_t (*handler)(int32_t, void *), void *arg) {
    if (p4pc_frame_alarm_seam)
        return p4pc_frame_alarm_seam->set(handler, arg);
    return p4pc_set_frame_boundary_alarm(handler, arg);
}

int32_t p4pc_cancel_alarm(int32_t id) {
    if (p4pc_frame_alarm_seam)
        p4pc_frame_alarm_seam->cancel(id);
    if (id < 1 || id > KT_MAX_ALARMS || !kt_alarms[id].used)
        return -1;
    kt_alarms[id].used = 0;
    return 0;
}

int p4pc_thread_guest_stack_overlap(uint32_t address, size_t size) {
    if (size == 0) return 0;
    if (size > UINT64_C(0x100000000) - address) return 1;
    uint64_t end = (uint64_t)address + size;
    for (int32_t id = 1; id <= KT_MAX_THREADS; ++id) {
        const kt_thread *t = &kt_threads[id];
        if (!t->used && t->co == NULL && t->owned_stack == NULL) continue;
        if (t->stack != NULL && t->stack_size != 0) {
            uint64_t begin = P4_H2G(t->stack);
            if (address < begin + t->stack_size && begin < end) return 1;
        }
        if (t->guest_stack != 0 && t->guest_stack_size > 0 &&
            address < (uint64_t)t->guest_stack + (uint32_t)t->guest_stack_size &&
            t->guest_stack < end)
            return 1;
    }
    return 0;
}


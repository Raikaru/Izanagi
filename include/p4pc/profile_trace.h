/* Native timing records for the existing PROF stream. Diagnostic-only: no
 * guest reads/writes, allocation or dependency on a game/renderer object.
 * Disabled calls do not read a clock, format text, or resolve a file path. */
#ifndef P4PC_PROFILE_TRACE_H
#define P4PC_PROFILE_TRACE_H
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdatomic.h>
#include <errno.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <time.h>
#include <pthread.h>
/* Standalone strict-C heap oracles need not enable glibc's feature macros.
 * Linux clockid_t is int; timespec is the public C11 type in either mode. */
#if defined(__linux__) && !defined(CLOCK_MONOTONIC)
#define CLOCK_MONOTONIC 1
#if defined(__USE_TIME64_REDIRECTS)
extern int clock_gettime(int, struct timespec *) __asm__("__clock_gettime64");
#else
extern int clock_gettime(int, struct timespec *);
#endif
#endif
#endif

#ifdef _WIN32
/* Published by the opt-in configuration flag before any clock is read. */
static uint64_t p4pc_prof_span_frequency;
#endif
/* Optional output threshold for high-volume heap/audio/sleep records only.
 * Physical I/O and async boundaries remain complete at every threshold. */
static uint64_t p4pc_prof_span_slow_ns;

static inline uint64_t p4pc_prof_span_now(void)
{
#ifdef _WIN32
    LARGE_INTEGER ticks;
    QueryPerformanceCounter(&ticks);
    return (uint64_t)ticks.QuadPart / p4pc_prof_span_frequency * UINT64_C(1000000000)
        + (uint64_t)ticks.QuadPart % p4pc_prof_span_frequency * UINT64_C(1000000000) / p4pc_prof_span_frequency;
#else
    struct timespec value;
    if (clock_gettime(CLOCK_MONOTONIC, &value) != 0) return 0;
    return (uint64_t)value.tv_sec * UINT64_C(1000000000) + (uint64_t)value.tv_nsec;
#endif
}

static inline uint64_t p4pc_prof_span_begin(void)
{
    /* Each instrumented unit initializes once; release publication keeps an
     * audio callback from observing a partially initialized window. */
    static _Atomic int state;
    static uint64_t from, until;
    int value = atomic_load_explicit(&state, memory_order_acquire);
    if (value <= 0) {
        int expected = 0;
        if (atomic_compare_exchange_strong_explicit(&state, &expected, -1, memory_order_acq_rel, memory_order_acquire)) {
            const int saved_errno = errno;
#ifdef _WIN32
            const DWORD saved_error = GetLastError();
#endif
            const char *enabled = getenv("P4PC_PROFILE_SPANS");
            int active = enabled && enabled[0] && enabled[0] != '0';
            if (active) {
#ifdef _WIN32
                LARGE_INTEGER frequency;
                QueryPerformanceFrequency(&frequency);
                p4pc_prof_span_frequency = (uint64_t)frequency.QuadPart;
#endif
                const char *a = getenv("P4PC_PROFILE_SPANS_FROM_NS");
                const char *b = getenv("P4PC_PROFILE_SPANS_UNTIL_NS");
                from = a ? (uint64_t)strtoull(a, NULL, 10) : 0;
                until = b ? (uint64_t)strtoull(b, NULL, 10) : UINT64_MAX;
                const char *slow = getenv("P4PC_PROFILE_SPANS_SLOW_US");
                const uint64_t us = slow ? (uint64_t)strtoull(slow, NULL, 10) : 0;
                p4pc_prof_span_slow_ns = us <= UINT64_MAX / 1000 ? us * 1000 : UINT64_MAX;
            }
            value = active ? 1 : 2;
            atomic_store_explicit(&state, value, memory_order_release);
            errno = saved_errno;
#ifdef _WIN32
            SetLastError(saved_error);
#endif
        } else {
            do { value = atomic_load_explicit(&state, memory_order_acquire); } while (value < 0);
        }
    }
    if (value != 1) return 0;
    {
        const int saved_errno = errno;
#ifdef _WIN32
        const DWORD saved_error = GetLastError();
#endif
        const uint64_t now = p4pc_prof_span_now();
        errno = saved_errno;
#ifdef _WIN32
        SetLastError(saved_error);
#endif
        return now >= from && now <= until ? now : 0;
    }
}

static inline uint64_t p4pc_prof_span_stop(uint64_t start)
{
    uint64_t end;
    if (!start) return 0;
    const int saved_errno = errno;
#ifdef _WIN32
    const DWORD saved_error = GetLastError();
#endif
    end = p4pc_prof_span_now();
    errno = saved_errno;
#ifdef _WIN32
    SetLastError(saved_error);
#endif
    return end;
}

static inline void p4pc_prof_span_record(uint64_t start, uint64_t end, const char *operation,
                                 uint64_t requested, int64_t result, uint64_t detail,
                                 const char *context)
{
    uintptr_t thread;
    if (!start) return;
    /* Keep fast physical I/O records, but avoid turning an allocation/mixer
     * burst into hundreds of synchronous diagnostic writes in one flip. */
    if (p4pc_prof_span_slow_ns && end - start < p4pc_prof_span_slow_ns
        && (operation[0] == 'h' || operation[0] == 's'
            || (operation[0] == 'a' && operation[1] == 'u')))
        return;
    const int saved_errno = errno;
#ifdef _WIN32
    const DWORD saved_error = GetLastError();
    thread = (uintptr_t)GetCurrentThreadId();
#else
    thread = (uintptr_t)pthread_self();
#endif
    /* One stdio call makes the record indivisible across native audio/game
     * threads. Its own logging time is excluded from the reported API span.
     * The context is the remainder of the line; it may contain spaces. */
    fprintf(stderr, "PROF-SPAN %llu %llu %llu %s %llu %lld 0x%llx %s\n",
            (unsigned long long)start, (unsigned long long)end, (unsigned long long)thread,
            operation, (unsigned long long)requested, (long long)result,
            (unsigned long long)detail, context ? context : "-");
    errno = saved_errno;
#ifdef _WIN32
    SetLastError(saved_error);
#endif
}

static inline void p4pc_prof_span_end(uint64_t start, const char *operation, uint64_t requested,
                              int64_t result, uint64_t detail, const char *context)
{
    if (start)
        p4pc_prof_span_record(start, p4pc_prof_span_stop(start), operation, requested, result, detail, context);
}
#endif

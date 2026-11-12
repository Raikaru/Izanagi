/* P4PC_PROFILE=<hz>: a sampling profiler for unattended runs where perf and gdb cannot attach. The OS shim
 * (p4_os_profile_start) delivers the interrupted pc, sp and frame pointer about <hz> times a second: SIGPROF on
 * CPU time on Linux, a sampler thread that suspends the game thread on Windows (wall time, so waits show up).
 * The callback records the pc and up to eight return addresses (frame-pointer chain, bounded to the thread's
 * stack: every unit is built with -fno-omit-frame-pointer). p4pc_profile_flip(), called from the flip, prints the
 * samples collected since the last report every P4PC_PROFILE_EVERY flips (default 3000) as `PROF pc ra1 ra2 ...`
 * lines, preceded once by `MOD base size name` lines for the mapped modules; resolve them with `nm -n` of the
 * same build (tools/profile_report.py: it also names pcs inside DLLs and shared objects from the MOD lines). */
#include <stdint.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

#include "p4pc/os.h"

enum { MAX_SAMPLES = 2048, DEPTH = 9 };

static uintptr_t sSamples[MAX_SAMPLES][DEPTH];
static uint64_t sSampleNs[MAX_SAMPLES];
static volatile unsigned sCount, sWrite;
static volatile int sReporting; /* the sampler thread (Windows) must not add while the flip prints */
static int sOn = -1;
static unsigned sEvery = 3000;
static unsigned sFrom;
static int sStarted, sTimestamps;
static const char *sRate;

static uint64_t sLastFlipNs, sLastDumpNs, sHangNs; /* P4PC_PROFILE_HANG=<seconds>: dump the samples when no flip has come for that long */
static uint64_t sHitchNs; /* P4PC_PROFILE_HITCH_MS: dump after a slow flip, never from a suspended thread. */
static uintptr_t sExeLo, sExeHi; /* the module this code is in: pcs outside it (a driver, the window system) have no frame pointers */

static void findExe(uintptr_t base, uintptr_t size, const char *name)
{
    const uintptr_t here = (uintptr_t)&findExe;

    (void)name;
    if (here >= base && here < base + size) {
        sExeLo = base;
        sExeHi = base + size;
    }
}

/* True when `ra` is a return address in the exe: the bytes before it are a call (E8 rel32, FF /2 with a mod/rm byte, or FF 15 abs32). */
static int isCallSite(uintptr_t ra)
{
    const unsigned char *p = (const unsigned char *)ra;

    /* Below 0x02000000 the Windows exe holds only its headers and PS2 RAM (guest data, MIPS code, a gap that is not mapped): never host code */
    if (ra < sExeLo + 8 || ra >= sExeHi || ra < 0x02000000u) {
        return 0;
    }
    if (!p4_os_code_readable(ra)) {
        return 0;
    }
    return p[-5] == 0xE8 || (p[-2] == 0xFF && (p[-1] & 0x38) == 0x10) || (p[-6] == 0xFF && p[-5] == 0x15) ||
           (p[-3] == 0xFF && (p[-2] & 0x38) == 0x10);
}

static void dumpSamples(unsigned from, unsigned n)
{
    unsigned i, d;
    const unsigned oldest = (sWrite + MAX_SAMPLES - n) % MAX_SAMPLES;

    for (i = from; i < n; i++) {
        const unsigned slot = (oldest + i) % MAX_SAMPLES;
        /* An AT/sample pair is indivisible beside native audio PROF-SPAN
         * records. No allocation/copy or change to the existing wire format. */
#ifdef _WIN32
        _lock_file(stderr);
#else
        flockfile(stderr);
#endif
        if (sTimestamps) fprintf(stderr, "PROF-AT %" PRIu64 "\n", sSampleNs[slot]);
        fprintf(stderr, "PROF");
        for (d = 0; d < DEPTH && sSamples[slot][d] != 0; d++) {
            fprintf(stderr, " %" PRIxPTR, sSamples[slot][d]);
        }
        fprintf(stderr, "\n");
#ifdef _WIN32
        _unlock_file(stderr);
#else
        funlockfile(stderr);
#endif
    }
}

static void printModule(uintptr_t base, uintptr_t size, const char *name);

static void onSampleRecord(uintptr_t pc, uintptr_t sp, uintptr_t fpv, uintptr_t end)
{
    uintptr_t *fp = (uintptr_t *)fpv;
    unsigned n = sWrite, d = 1;

    if (sReporting) {
        return;
    }
    if (sTimestamps) sSampleNs[n] = p4_os_now_ns();
    sSamples[n][0] = pc;
    if (sExeHi != 0 && (pc < sExeLo || pc >= sExeHi)) {
        /* Inside a library: the frame-pointer chain is not there. Scan the stack for words that are call returns into the exe. */
        const uintptr_t *w = (const uintptr_t *)sp;
        unsigned i;

        for (i = 0; i < 1024 && d < DEPTH && (uintptr_t)(w + i + 1) <= end; i++) {
            if (isCallSite(w[i])) {
                sSamples[n][d++] = w[i];
            }
        }
        while (d < DEPTH) {
            sSamples[n][d++] = 0;
        }
        sWrite = (n + 1) % MAX_SAMPLES;
        if (sCount < MAX_SAMPLES) sCount++;
        return;
    }
    while (d < DEPTH && (uintptr_t)fp > sp && (uintptr_t)(fp + 2) <= end && (uintptr_t)fp - sp < (1u << 20) && ((uintptr_t)fp & 3u) == 0) {
        sSamples[n][d++] = fp[1];
        fp = (uintptr_t *)fp[0];
    }
    while (d < DEPTH) {
        sSamples[n][d++] = 0;
    }
    sWrite = (n + 1) % MAX_SAMPLES;
    if (sCount < MAX_SAMPLES) sCount++;
}

static void onSample(uintptr_t pc, uintptr_t sp, uintptr_t fpv, uintptr_t end)
{
    onSampleRecord(pc, sp, fpv, end);
    if (sHangNs != 0 && sLastFlipNs != 0) {
        const uint64_t now = p4_os_now_ns();

        if (now - sLastFlipNs > sHangNs && now - sLastDumpNs > 3000000000ull) {
            const unsigned n = sCount;

            sLastDumpNs = now;
            fprintf(stderr, "PROF-HANG no flip for %.0f s; the last samples:\n", (double)(now - sLastFlipNs) * 1e-9);
            p4_os_list_modules(printModule);
            dumpSamples(n > 24 ? n - 24 : 0, n);
            fprintf(stderr, "PROF-END hang\n");
        }
    }
}

static void printModule(uintptr_t base, uintptr_t size, const char *name)
{
    fprintf(stderr, "MOD %" PRIxPTR " %" PRIxPTR " %s\n", base, size, name);
}

static void startProfile(const char *hz)
{
    int rate = atoi(hz);
    const char *e = getenv("P4PC_PROFILE_EVERY");

    if (e != NULL && atoi(e) > 0) {
        sEvery = (unsigned)atoi(e);
    }
    if (rate <= 0) {
        rate = 199;
    }
    e = getenv("P4PC_PROFILE_HANG");
    if (e != NULL && atoi(e) > 0) {
        sHangNs = (uint64_t)atoi(e) * 1000000000ull;
    }
    e = getenv("P4PC_PROFILE_HITCH_MS");
    if (e != NULL && atoi(e) > 0) {
        sHitchNs = (uint64_t)atoi(e) * 1000000ull;
    }
    e = getenv("P4PC_PROFILE_TIMESTAMPS");
    sTimestamps = e != NULL && e[0] != '\0' && e[0] != '0';
    p4_os_list_modules(findExe);
    if (p4_os_profile_start(rate, onSample) != 0) {
        fprintf(stderr, "p4pc: profiler not available\n");
        return;
    }
    fprintf(stderr, "p4pc: profiler on at %d Hz, report every %u flips\n", rate, sEvery);
}

void p4pc_profile_flip(unsigned flip)
{
    unsigned n;
    uint64_t report_start;
    uint64_t flip_start, flip_end;
    int hitch;

    if (sOn < 0) {
        sRate = getenv("P4PC_PROFILE");
        sOn = sRate != NULL && *sRate != '\0' && *sRate != '0';
        if (sOn) {
            const char *from = getenv("P4PC_PROFILE_FROM");
            if (from != NULL && atoi(from) > 0) sFrom = (unsigned)atoi(from);
        }
    }
    if (!sOn || flip < sFrom) return;
    if (!sStarted) {
        startProfile(sRate);
        sStarted = 1;
    }
    flip_start = sLastFlipNs;
    flip_end = p4_os_now_ns();
    sLastFlipNs = flip_end;
    hitch = sHitchNs && flip_start && flip_end - flip_start >= sHitchNs;
    if (!hitch && flip % sEvery != 0) {
        return;
    }
    sReporting = 1;
    report_start = (sTimestamps || sHitchNs) ? p4_os_now_ns() : 0;
    n = sCount;
    if (hitch)
        fprintf(stderr, "PROF-HITCH %" PRIu64 " %" PRIu64 " flip %u samples %u\n", flip_start, flip_end, flip, n);
    p4_os_list_modules(printModule);
    dumpSamples(0, n);
    fprintf(stderr, "PROF-END flip %u samples %u\n", flip, n);
    sCount = sWrite = 0;
    if (sHitchNs) sLastFlipNs = p4_os_now_ns(); /* Do not retrigger on the report's own work. */
    if (sTimestamps || sHitchNs)
        fprintf(stderr, "PROF-REPORT %" PRIu64 " %" PRIu64 " flip %u\n", report_start, p4_os_now_ns(), flip);
    sReporting = 0;
}

/* A restart is an exit boundary, never re-entry into the game's globals. */
#include "p4pc/os.h"
#include <errno.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <process.h>
#define restart_pid _getpid
#else
#include <unistd.h>
#define restart_pid getpid
#endif

static int sInitialized, sRequested, sArgc;
static char **sArgv;
static const char *sTestMarker;

#ifndef __ANDROID__
static void restart_after_cleanup(void)
{
    if (!sRequested) return;
    fflush(NULL);
    if (p4_os_restart_self(sArgc, sArgv) != 0) {
        fprintf(stderr, "p4pc restart: relaunch failed after cleanup: %s\n", strerror(errno));
        fflush(NULL);
        _Exit(1);
    }
}
#endif

static void test_fail(const char *why)
{
    fprintf(stderr, "p4pc restart test: FAIL: %s\n", why);
    exit(1);
}

static void test_drained(void)
{
    FILE *file = fopen(sTestMarker, "ab");
    if (!file || fputs("drained\n", file) < 0 || fclose(file) != 0) {
        fputs("p4pc restart test: cannot record drained exit\n", stderr);
        fflush(NULL);
        _Exit(1);
    }
}

/* Test-only hook, before loading retail data: the second image must see both
 * the same arguments and a marker written by the old image's exit handler. */
static void restart_test(void)
{
    static int entered;
    uint64_t hash = UINT64_C(14695981039346656037), previous;
    long previous_pid, pid = (long)restart_pid();
    char line[32];
    FILE *file;
    sTestMarker = getenv("P4PC_TEST_RESTART");
    if (!sTestMarker || !*sTestMarker) return;
    if (entered++) test_fail("same-process re-entry");
    for (int i = 0; i < sArgc; ++i) {
        const unsigned char *p = (const unsigned char *)sArgv[i];
        do { hash = (hash ^ *p) * UINT64_C(1099511628211); } while (*p++);
    }
    file = fopen(sTestMarker, "rb");
    if (file) {
        int valid = fscanf(file, "requested %" SCNx64 " pid=%ld\n", &previous, &previous_pid) == 2 && previous == hash &&
            fgets(line, sizeof line, file) && !strcmp(line, "drained\n") && fgetc(file) == EOF;
        fclose(file);
        if (!valid) test_fail("arguments or drained-exit marker differ");
#if defined(_WIN32) || defined(__ANDROID__)
        if (previous_pid == pid) test_fail("process was not replaced");
#endif
        file = fopen(sTestMarker, "ab");
        if (!file || fputs("second\n", file) < 0 || fclose(file) != 0) test_fail("cannot mark second start");
        fprintf(stderr, "p4pc restart test: PASS: old pid=%ld new pid=%ld, fresh image, same arguments, drained exit\n",
            previous_pid, pid);
        exit(0);
    }
    if (errno != ENOENT) test_fail("cannot read marker");
    file = fopen(sTestMarker, "wb");
    if (!file || fprintf(file, "requested %016" PRIx64 " pid=%ld\n", hash, pid) < 0 || fclose(file) != 0)
        test_fail("cannot write first-start marker");
    if (atexit(test_drained) != 0) test_fail("cannot register drain witness");
    if (p4_os_request_restart("synthetic once-only restart") != 0) test_fail("cannot schedule restart");
    exit(0);
}

int p4_os_restart_init(int argc, char **argv)
{
    if (sInitialized) {
        if (getenv("P4PC_TEST_RESTART")) test_fail("same-process re-entry");
        return 0;
    }
    if (argc < 1 || !argv || !argv[0]) { errno = EINVAL; return -1; }
#ifndef __ANDROID__
    /* Registered first in main, so later resource/VM teardown runs before it. */
    if (atexit(restart_after_cleanup) != 0) return -1;
#endif
    sInitialized = 1;
    sArgc = argc;
    sArgv = argv;
    restart_test();
    return 0;
}

int p4_os_request_restart(const char *reason)
{
    if (!sInitialized) { errno = EINVAL; return -1; }
    if (sRequested) return 0;
#ifdef __ANDROID__
    if (p4_android_prepare_restart() != 0) return -1;
#endif
    fprintf(stderr, "p4pc restart: scheduled: %s\n", reason ? reason : "requested");
    sRequested = 1;
    return 0; /* The native error UI owns the final ordinary exit(0). */
}

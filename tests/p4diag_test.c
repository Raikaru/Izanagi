/* Player diagnostics (src/platform/diag/p4diag.c): the frame statistics, the area tag, the redaction, and the log a run writes: header, perf lines, summary, clean-exit line.
 * The game's memory is a few fake words; the mixer and the window are stubs. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../src/platform/config/p4cfg.h"
#include "../src/platform/config/p4iso.h"
#include "../src/platform/diag/p4diag.h"
#include "../src/platform/sound/p4snd.h"
#include "p4pc/os.h"

static int failures;

#define CHECK(c)                                                         \
    do {                                                                 \
        if (!(c)) {                                                      \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #c); \
            failures++;                                                  \
        }                                                                \
    } while (0)

/* the parts of the program that p4diag reads */
void (*p4pc_frame_hook)(unsigned flip);
struct SDL_Window *p4pc_pad_sdl_window(void)
{
    return NULL;
}
void p4snd_diag_poll(P4SndDiag *out)
{
    memset(out, 0, sizeof *out);
}

/* a field 12 room 3, no battle, and with `with_event` an event 176.1.0 running (the task list the capture tooling walks) */
static int with_event;
static int fake_read(uint32_t address, uint32_t *out, void *user)
{
    (void)user;
    if (address < 0x00100000u || address >= 0x02000000u) {
        return 0;
    }
    *out = 0;
    switch (address) {
    case 0x00762ea0u: *out = 0x00400000u; break; /* the scene: field, room */
    case 0x00400000u: *out = 12u; break;
    case 0x00400004u: *out = 3u; break;
    case 0x00764adcu: *out = with_event ? 0x00410000u : 0u; break; /* first task list: one node */
    case 0x00410000u: *out = 0x45747665u; break;                   /* "evtE" "vent" */
    case 0x00410004u: *out = 0x746e6576u; break;
    case 0x00410038u: *out = 0x00420000u; break;                   /* its driver, kind 4 */
    case 0x00420000u: *out = 4u; break;
    case 0x0042000cu: *out = 0x00430000u; break;                   /* its task and the work block */
    case 0x00430038u: *out = 0x00440000u; break;
    case 0x0044078cu: *out = 176u; break;
    case 0x00440790u: *out = 1u; break;
    default: break;
    }
    return 1;
}

static char *slurp(const char *path)
{
    FILE *f = fopen(path, "rb");
    char *t;
    long n;

    if (f == NULL) {
        return NULL;
    }
    fseek(f, 0, SEEK_END);
    n = ftell(f);
    fseek(f, 0, SEEK_SET);
    t = calloc((size_t)n + 1, 1);
    if (t != NULL && fread(t, 1, (size_t)n, f) != (size_t)n) {
        free(t);
        t = NULL;
    }
    fclose(f);
    return t;
}

static void test_stats(void)
{
    P4DiagStats s;
    int i;

    memset(&s, 0, sizeof s);
    CHECK(p4diag_stats_percentile_ms(&s, 95) == 0);
    for (i = 0; i < 90; i++) {
        p4diag_stats_add(&s, 33333333u, 12000000u); /* a 30 Hz flip: two ticks */
    }
    for (i = 0; i < 8; i++) {
        p4diag_stats_add(&s, 50000000u, 30000000u); /* three ticks, slow */
    }
    p4diag_stats_add(&s, 120000000u, 100000000u);  /* a hitch */
    p4diag_stats_add(&s, 16666667u, 5000000u);     /* one tick */
    CHECK(s.frames == 100);
    CHECK(s.cadence[0] == 1 && s.cadence[1] == 90 && s.cadence[2] == 8 && s.cadence[3] == 1);
    CHECK(s.slow == 9 && s.hitch == 1);
    CHECK(s.max_ns == 120000000u);
    CHECK(p4diag_stats_percentile_ms(&s, 50) == 34);  /* the 33 ms bin's upper edge */
    CHECK(p4diag_stats_percentile_ms(&s, 95) == 51);  /* 91..98 are the 50 ms flips */
    CHECK(p4diag_stats_percentile_ms(&s, 99) == 51);
    CHECK(p4diag_stats_percentile_ms(&s, 100) == 121);
    CHECK(s.busy_ns == 90u * 12000000u + 8u * 30000000u + 100000000u + 5000000u);
    p4diag_stats_add(&s, 9000000000ull, 0); /* a stall lands in the last bin */
    CHECK(p4diag_stats_percentile_ms(&s, 100) == P4DIAG_HIST);
}

static void test_area(void)
{
    P4DiagArea a, b;
    char text[120];

    with_event = 0;
    p4diag_area_read_with(&a, fake_read, NULL);
    CHECK(a.scene && a.field == 12 && a.room == 3 && !a.event && !a.battle);
    p4diag_area_format(&a, text, sizeof text);
    CHECK(strcmp(text, "f12:r3") == 0);
    with_event = 1;
    p4diag_area_read_with(&b, fake_read, NULL);
    CHECK(b.event && b.event_id[0] == 176 && b.event_id[1] == 1 && b.event_id[2] == 0);
    p4diag_area_format(&b, text, sizeof text);
    CHECK(strcmp(text, "f12:r3 ev 176.1.0") == 0);
    CHECK(!p4diag_area_equal(&a, &b) && p4diag_area_equal(&b, &b));
    b.battle = 1;
    p4diag_area_format(&b, text, sizeof text);
    CHECK(strcmp(text, "f12:r3 ev 176.1.0 battle") == 0);
    memset(&a, 0, sizeof a);
    p4diag_area_format(&a, text, sizeof text);
    CHECK(strcmp(text, "no-scene") == 0);
    p4diag_area_format(&b, text, 8); /* a small buffer is cut, not overrun */
    CHECK(strlen(text) < 8);
}

static void test_redact(void)
{
    char out[200];

    p4_os_setenv("HOME", "/home/tester", 1);
    p4_os_setenv("USERPROFILE", "/home/tester", 1);
    p4_os_setenv("LOCALAPPDATA", "", 1);
    p4_os_setenv("APPDATA", "", 1);
    p4_os_setenv("USERNAME", "", 1);
    p4_os_setenv("LOGNAME", "", 1);
    p4_os_setenv("USER", "alice", 1);
    p4diag_redact("/home/tester/mods/a and alice's pack", out, sizeof out);
    CHECK(strcmp(out, "~/mods/a and <user>'s pack") == 0);
    p4diag_redact("no secrets here", out, sizeof out);
    CHECK(strcmp(out, "no secrets here") == 0);
    p4diag_redact("/home/tester/some/long/path", out, 8);
    CHECK(strlen(out) < 8);
}

static void test_run(const char *log)
{
    char *t;
    unsigned i;

    p4_os_setenv("P4PC_DIAG", "1", 1);
    p4_os_setenv("P4PC_CONFIG", "none", 1);
    p4_os_setenv("P4PC_ISO", "", 1);
    p4cfg_clear();
    p4cfg_load();
    CHECK(p4diag_active());
    CHECK(freopen(log, "wb", stderr) != NULL);
    with_event = 0;
    p4diag_use_memory(fake_read, NULL);
    p4diag_startup("SLUS_217.82");
    CHECK(p4pc_frame_hook != NULL);
    for (i = 1; i <= 300 && p4pc_frame_hook != NULL; i++) {
        p4_os_usleep(1000);
        p4pc_frame_hook(i);
    }
    p4diag_shutdown();
    p4diag_shutdown(); /* once */
    fflush(stderr);
    t = slurp(log);
    CHECK(t != NULL);
    if (t == NULL) {
        return;
    }
    CHECK(strstr(t, "p4diag: ---- diagnostic header") != NULL);
    CHECK(strstr(t, "p4diag: build: ") != NULL);
    CHECK(strstr(t, "p4diag: os: ") != NULL);
    CHECK(strstr(t, "p4diag: cpu: ") != NULL);
    CHECK(strstr(t, "p4diag: settings: ") != NULL && strstr(t, "video.resolution=") != NULL);
    CHECK(strstr(t, "p4diag: mods: enabled=") != NULL);
    CHECK(strstr(t, "p4diag: disc: ") != NULL);
    CHECK(strstr(t, "p4diag: perf t=") != NULL && strstr(t, "area=\"f12:r3\"") != NULL);
    CHECK(strstr(t, "p4diag: summary run=") != NULL);
    CHECK(strstr(t, "p4diag: summary area \"f12:r3\" n=") != NULL);
    CHECK(strstr(t, "/home/tester") == NULL);
    CHECK(strstr(t, P4_LOG_CLEAN_EXIT "\n") != NULL);
    /* the clean-exit line is the last one, once */
    CHECK(strstr(t, P4_LOG_CLEAN_EXIT) == strstr(t + strlen(t) - strlen(P4_LOG_CLEAN_EXIT) - 1, P4_LOG_CLEAN_EXIT));
    free(t);
}

int main(int argc, char **argv)
{
    test_stats();
    test_area();
    test_redact();
    test_run(argc > 1 ? argv[1] : "p4diag_test.log");
    if (failures == 0) {
        fprintf(stdout, "p4diag_test: ok\n");
    }
    return failures != 0;
}

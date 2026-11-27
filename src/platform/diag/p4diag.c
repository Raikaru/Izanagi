/* See p4diag.h and docs/platform/diagnostics.md. */
#include "p4diag.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <SDL3/SDL.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/utsname.h>
#include <unistd.h>
#endif
#ifdef __ANDROID__
#include <sys/system_properties.h>
#endif
#ifdef __APPLE__
#include <mach-o/dyld.h>
#include <sys/sysctl.h>
#endif
#if defined(__x86_64__) || defined(__i386__)
#include <cpuid.h>
#endif

#include "../config/p4cfg.h"
#include "../config/p4iso.h"
#include "../sound/p4snd.h"
#include "p4pc/guest.h"
#include "p4pc/hostpath.h"
#include "p4pc/os.h"

struct SDL_Window *p4pc_pad_sdl_window(void); /* src/platform/pad/pad_sdl.c */
/* Called by the kernel after every flip's wait with the flip count, when set (src/platform/kernel_thread.c). Declared here, not in p4pc/platform.h: the
 * decomp's translation units include that header, and a new declaration there changed the 64-bit rewriter's census of a kernel file. */
extern void (*p4pc_frame_hook)(unsigned flip);

/* ------------------------------------------------------------------ state */

enum {
    SLOW_NS = 42000000,         /* 2.5 ticks of 60 Hz */
    HITCH_NS = 100000000,
    WINDOW_NS = 10000000000ll,  /* one perf line per ten seconds of play ... */
    MIN_LINE_FRAMES = 30,       /* ... or per area change, when at least this many flips are in it */
    AREA_EVERY = 15,            /* flips between area samples */
    AUDIO_EVERY = 30,           /* flips between audio polls */
    MAX_ROWS = 96,
    MAX_PERF_LINES = 3000,      /* about eight hours of play */
    MAX_AUDIO_LINES = 60,
    MAX_HITCH_LINES = 100,
    HITCH_LOG_NS = 200000000,
    PAUSE_NS = 5000000000ll     /* a flip that took this long was the app in the background, not a frame */
};

typedef struct AreaRow {
    P4DiagArea area;
    P4DiagStats stats;
} AreaRow;

static struct {
    int started, finished, active; /* active: -1 unknown */
    uint64_t start_ns, last_ns, last_cpu_ns, window_start_ns, last_hitch_ns;
    int have_last, have_cpu;
    P4DiagStats window, total;
    P4DiagArea area;
    AreaRow *row; /* the current area's row, NULL once the table is full */
    AreaRow rows[MAX_ROWS], other; /* other: every area after the table is full */
    unsigned nrows, perf_lines, audio_lines, hitch_lines, pauses;
    unsigned cpu_hits[64], cpu_samples;
    unsigned cpu_mhz[64];
    int display_done;
    /* audio, from the mixer's polls */
    uint64_t audio_callbacks_seen, audio_late, audio_slow, audio_locks, audio_clipped, audio_render_ns;
    uint64_t audio_window_callbacks;
    uint64_t audio_render_max_ns, audio_gap_max_ns, audio_lock_max_ns;
    float audio_peak, audio_peak_total;
    uint32_t audio_period_us;
    int audio_device;
    char build[200];
} D = {.active = -1};

static void say(const char *fmt, ...)
{
    va_list ap;

    fputs("p4diag: ", stderr);
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fputc('\n', stderr);
}

int p4diag_active(void)
{
    if (D.active < 0) {
        const char *want = getenv("P4PC_DIAG");

        if (want != NULL && want[0] == '0') {
            D.active = 0;
        } else if (want != NULL && want[0] == '1') {
            D.active = 1;
        } else {
            D.active = p4cfg_player_run();
        }
    }
    return D.active;
}

/* ------------------------------------------------------------------ statistics */

void p4diag_stats_add(P4DiagStats *s, uint64_t frame_ns, uint64_t busy_ns)
{
    uint64_t ms = frame_ns / 1000000u, ticks = (frame_ns * 60u + 500000000u) / 1000000000u;

    s->frames++;
    s->sum_ns += frame_ns;
    s->busy_ns += busy_ns;
    if (frame_ns > s->max_ns) {
        s->max_ns = frame_ns;
    }
    s->hist[ms < P4DIAG_HIST ? ms : P4DIAG_HIST - 1]++;
    if (frame_ns >= SLOW_NS) {
        s->slow++;
    }
    if (frame_ns >= HITCH_NS) {
        s->hitch++;
    }
    s->cadence[ticks < 1 ? 0 : ticks > 4 ? 3 : ticks - 1]++;
}

unsigned p4diag_stats_percentile_ms(const P4DiagStats *s, unsigned percent)
{
    uint64_t want, seen = 0;
    unsigned i;

    if (s->frames == 0) {
        return 0;
    }
    want = ((uint64_t)s->frames * percent + 99u) / 100u;
    for (i = 0; i < P4DIAG_HIST; i++) {
        seen += s->hist[i];
        if (seen >= want) {
            return i + 1;
        }
    }
    return P4DIAG_HIST;
}

/* ------------------------------------------------------------------ the area */

static int range(uint32_t address, uint32_t size)
{
    return address >= 0x00100000u && address < 0x02000000u && size <= 0x02000000u - address;
}

typedef int (*WordReader)(uint32_t address, uint32_t *out, void *user);

static int guest_word(uint32_t address, uint32_t *out, void *user)
{
    (void)user;
    if (!range(address, 4)) {
        return 0;
    }
    memcpy(out, P4_G2H(address), 4);
    return 1;
}

static uint32_t wordAt(WordReader read, void *user, uint32_t address)
{
    uint32_t v = 0;

    return read(address, &v, user) ? v : 0;
}

/* The running event, found as the capture tooling finds it (capture_state.c): the "evtEvent" task in the three task lists, its driver, its task, its work block. */
static int read_event(WordReader read, void *user, uint32_t id[3])
{
    static const uint32_t heads[] = {0x00764adcu, 0x00764af4u, 0x00764ae8u};
    unsigned i, count;

    for (i = 0; i < sizeof heads / sizeof heads[0]; i++) {
        uint32_t node = wordAt(read, user, heads[i]);

        for (count = 0; count < 256 && range(node, 0x40); count++) {
            uint32_t name[6], driver, task, work, k;
            int ok = 1;

            for (k = 0; k < 6 && ok; k++) {
                ok = read(node + 4 * k, &name[k], user);
            }
            if (ok && memcmp(name, "evtEvent", 8) == 0) {
                driver = wordAt(read, user, node + 0x38);
                if (range(driver, 0x10) && wordAt(read, user, driver) == 4u) {
                    task = wordAt(read, user, driver + 0x0c);
                    if (range(task, 0x3c)) {
                        work = wordAt(read, user, task + 0x38);
                        if (range(work, 0x798)) {
                            id[0] = wordAt(read, user, work + 0x78c);
                            id[1] = wordAt(read, user, work + 0x790);
                            id[2] = wordAt(read, user, work + 0x794);
                            return 1;
                        }
                    }
                }
            }
            node = wordAt(read, user, node + 0x3c);
        }
    }
    return 0;
}

void p4diag_area_read_with(P4DiagArea *area, int (*read)(uint32_t address, uint32_t *out, void *user), void *user)
{
    const uint32_t scene = wordAt(read, user, 0x00762ea0u), battle = wordAt(read, user, 0x0076449cu);

    memset(area, 0, sizeof *area);
    if (range(scene, 0x14)) {
        area->scene = 1;
        area->field = wordAt(read, user, scene) & 0xffffu;
        area->room = wordAt(read, user, scene + 4u) & 0xffffu;
    }
    area->event = read_event(read, user, area->event_id);
    area->battle = range(battle, 0x10);
}

static WordReader sRead = guest_word;
static void *sReadUser;

void p4diag_use_memory(int (*read)(uint32_t address, uint32_t *out, void *user), void *user)
{
    sRead = read != NULL ? read : guest_word;
    sReadUser = user;
}

void p4diag_area_read(P4DiagArea *area)
{
    p4diag_area_read_with(area, sRead, sReadUser);
}

int p4diag_area_equal(const P4DiagArea *a, const P4DiagArea *b)
{
    return a->scene == b->scene && a->event == b->event && a->battle == b->battle && a->field == b->field && a->room == b->room &&
           a->event_id[0] == b->event_id[0] && a->event_id[1] == b->event_id[1] && a->event_id[2] == b->event_id[2];
}

void p4diag_area_format(const P4DiagArea *a, char *out, size_t capacity)
{
    size_t n = 0;

    if (capacity == 0) {
        return;
    }
    out[0] = '\0';
    if (a->scene) {
        n = (size_t)snprintf(out, capacity, "f%u:r%u", (unsigned)a->field, (unsigned)a->room);
    } else {
        n = (size_t)snprintf(out, capacity, "no-scene");
    }
    if (n < capacity && a->event) {
        n += (size_t)snprintf(out + n, capacity - n, " ev %u.%u.%u", (unsigned)a->event_id[0], (unsigned)a->event_id[1], (unsigned)a->event_id[2]);
    }
    if (n < capacity && a->battle) {
        snprintf(out + n, capacity - n, " battle");
    }
}

/* ------------------------------------------------------------------ privacy */

static int same_char(char a, char b)
{
#ifdef _WIN32
    if (a >= 'A' && a <= 'Z') a = (char)(a - 'A' + 'a');
    if (b >= 'A' && b <= 'Z') b = (char)(b - 'A' + 'a');
    if (a == '\\') a = '/';
    if (b == '\\') b = '/';
#endif
    return a == b;
}

static size_t match_at(const char *text, const char *needle)
{
    size_t n = 0;

    while (needle[n] != '\0' && text[n] != '\0' && same_char(text[n], needle[n])) {
        n++;
    }
    return needle[n] == '\0' ? n : 0;
}

void p4diag_redact(const char *text, char *out, size_t capacity)
{
    static const struct {
        const char *variable, *replacement;
    } secrets[] = {{"USERPROFILE", "~"}, {"HOME", "~"}, {"LOCALAPPDATA", "~"}, {"APPDATA", "~"}, {"USERNAME", "<user>"}, {"USER", "<user>"}, {"LOGNAME", "<user>"}};
    size_t o = 0;

    if (capacity == 0) {
        return;
    }
    while (*text != '\0' && o + 1 < capacity) {
        size_t skip = 0, k;
        const char *replacement = NULL;

        for (k = 0; k < sizeof secrets / sizeof secrets[0] && skip == 0; k++) {
            const char *value = getenv(secrets[k].variable);

            /* "/" and a one-letter name would blank half the log */
            if (value != NULL && strlen(value) >= 3 && strcmp(value, "/root") != 0) {
                skip = match_at(text, value);
                replacement = secrets[k].replacement;
            }
        }
        if (skip != 0) {
            size_t r = strlen(replacement);

            if (o + r + 1 >= capacity) {
                break;
            }
            memcpy(out + o, replacement, r);
            o += r;
            text += skip;
        } else {
            out[o++] = *text++;
        }
    }
    out[o] = '\0';
}

/* ------------------------------------------------------------------ the header */

static void utc_text(time_t t, char *out, size_t cap)
{
    struct tm tmv;
    const struct tm *p;

#ifdef _WIN32
    p = gmtime_s(&tmv, &t) == 0 ? &tmv : NULL;
#else
    p = gmtime_r(&t, &tmv);
#endif
    if (p == NULL || strftime(out, cap, "%Y-%m-%dT%H:%M:%SZ", p) == 0) {
        snprintf(out, cap, "?");
    }
}

static const char *arch_name(void)
{
#if defined(__x86_64__) || defined(_M_X64)
    return "x86_64";
#elif defined(__aarch64__) || defined(_M_ARM64)
    return "arm64";
#elif defined(__i386__) || defined(_M_IX86)
    return "x86";
#else
    return "unknown-arch";
#endif
}

/* The path of the running program; 0 when it cannot be found (Android has no executable to name: the app is the version). */
static int program_path(char *out, size_t cap)
{
#if defined(_WIN32)
    WCHAR wide[2048];
    DWORD n = GetModuleFileNameW(NULL, wide, 2048);

    if (n == 0 || n >= 2048 || WideCharToMultiByte(CP_UTF8, 0, wide, -1, out, (int)cap, NULL, NULL) == 0) {
        return 0;
    }
#elif defined(__APPLE__)
    uint32_t size = (uint32_t)cap;

    if (_NSGetExecutablePath(out, &size) != 0) {
        return 0;
    }
#elif defined(__ANDROID__)
    (void)out;
    (void)cap;
    return 0;
#else
    ssize_t n = readlink("/proc/self/exe", out, cap - 1);

    if (n <= 0) {
        return 0;
    }
    out[n] = '\0';
#endif
    return 1;
}

static char *last_separator(char *path)
{
    char *slash = strrchr(path, '/'), *back = strrchr(path, '\\');

    return back != NULL && (slash == NULL || back > slash) ? back : slash;
}

/* The build line: VERSION.txt of the package (beside the executable, or up to four folders above it: the AppImage and the macOS bundle keep it at their
 * root), else what the app says (Android: P4PC_APP_VERSION from the activity), else the executable's size and date. */
static void find_build(char *out, size_t cap)
{
    char dir[2100], path[2200], line[256];
    const char *app = getenv("P4PC_APP_VERSION");
    int up;

#ifdef P4PC_BUILD_ID
    snprintf(out, cap, "%s", P4PC_BUILD_ID);
    return;
#endif
    if (app != NULL && app[0] != '\0') {
        snprintf(out, cap, "%s (app version)", app);
        return;
    }
    if (program_path(dir, sizeof dir) && last_separator(dir) != NULL) {
        *last_separator(dir) = '\0';
        for (up = 0; up < 5; up++) {
            FILE *f;
            char *cut;

            snprintf(path, sizeof path, "%s/VERSION.txt", dir);
            f = p4_os_fopen_utf8(path, "rb"); /* the program may sit under a user name that is not ASCII */
            if (f != NULL) {
                size_t o = 0;
                int lines = 0;

                out[0] = '\0';
                while (lines < 2 && fgets(line, sizeof line, f) != NULL) {
                    size_t n = strlen(line);

                    while (n > 0 && (line[n - 1] == '\n' || line[n - 1] == '\r' || line[n - 1] == ' ')) {
                        line[--n] = '\0';
                    }
                    if (n == 0) {
                        continue;
                    }
                    o += (size_t)snprintf(out + o, o < cap ? cap - o : 0, "%s%s", lines > 0 ? " | " : "", line);
                    lines++;
                }
                fclose(f);
                if (o > 0) {
                    return;
                }
            }
            cut = last_separator(dir);
            if (cut == NULL) {
                break;
            }
            *cut = '\0';
        }
    }
    snprintf(out, cap, "unknown (no VERSION.txt near the program)");
}

/* The program's own file name, size and date: which build it is when VERSION.txt is gone. */
static void print_program(void)
{
    char path[2100];
    SDL_PathInfo info;
    const char *slash;
    char date[40];

    if (!program_path(path, sizeof path) || !SDL_GetPathInfo(path, &info)) {
        return;
    }
    slash = last_separator(path);
    utc_text((time_t)(info.modify_time / 1000000000ll), date, sizeof date);
    say("program: %s, %llu bytes, modified %s", slash != NULL ? slash + 1 : path, (unsigned long long)info.size, date);
}

static void cpu_name(char *out, size_t cap)
{
    out[0] = '\0';
#if defined(__x86_64__) || defined(__i386__)
    {
        unsigned a, b, c, d, i;
        char brand[49] = {0};

        if (__get_cpuid(0x80000000u, &a, &b, &c, &d) && a >= 0x80000004u) {
            for (i = 0; i < 3; i++) {
                __get_cpuid(0x80000002u + i, &a, &b, &c, &d);
                memcpy(brand + 16 * i, &a, 4);
                memcpy(brand + 16 * i + 4, &b, 4);
                memcpy(brand + 16 * i + 8, &c, 4);
                memcpy(brand + 16 * i + 12, &d, 4);
            }
            for (i = 0; brand[i] == ' '; i++) {
            }
            snprintf(out, cap, "%s", brand + i);
        }
    }
#endif
#ifdef __APPLE__
    if (out[0] == '\0') {
        size_t size = cap;

        if (sysctlbyname("machdep.cpu.brand_string", out, &size, NULL, 0) != 0) {
            out[0] = '\0';
        }
    }
#endif
#if defined(__linux__)
    if (out[0] == '\0') {
        FILE *f = p4_host_fopen("/proc/cpuinfo", "r");
        char line[256];
        unsigned parts[8] = {0}, counts[8] = {0}, nparts = 0;

        while (f != NULL && fgets(line, sizeof line, f) != NULL) {
            const char *colon = strchr(line, ':');

            if (colon != NULL && strncmp(line, "CPU part", 8) == 0) {
                /* an ARM core: the cluster layout (big and little cores) is what explains a phone's speed */
                const unsigned part = (unsigned)strtoul(colon + 1, NULL, 16);
                unsigned i;

                for (i = 0; i < nparts && parts[i] != part; i++) {
                }
                if (i == nparts && nparts < 8) {
                    parts[nparts++] = part;
                }
                if (i < 8) {
                    counts[i]++;
                }
            }
            if (colon != NULL && (strncmp(line, "model name", 10) == 0 || strncmp(line, "Hardware", 8) == 0)) {
                size_t n;

                colon++;
                while (*colon == ' ' || *colon == '\t') {
                    colon++;
                }
                snprintf(out, cap, "%s", colon);
                n = strlen(out);
                while (n > 0 && (out[n - 1] == '\n' || out[n - 1] == '\r')) {
                    out[--n] = '\0';
                }
                break;
            }
        }
        if (f != NULL) {
            fclose(f);
        }
        if (nparts > 0) {
            size_t o = strlen(out);
            unsigned i;

            for (i = 0; i < nparts && o + 40 < cap; i++) {
                static const struct {
                    unsigned part;
                    const char *name;
                } known[] = {{0xd03, "Cortex-A53"}, {0xd04, "Cortex-A35"}, {0xd05, "Cortex-A55"}, {0xd07, "Cortex-A57"}, {0xd08, "Cortex-A72"},
                             {0xd09, "Cortex-A73"}, {0xd0a, "Cortex-A75"}, {0xd0b, "Cortex-A76"}, {0xd0d, "Cortex-A77"}, {0xd41, "Cortex-A78"},
                             {0xd44, "Cortex-X1"},  {0xd46, "Cortex-A510"}, {0xd47, "Cortex-A710"}, {0xd48, "Cortex-X2"}, {0xd4d, "Cortex-A715"},
                             {0xd4e, "Cortex-X3"},  {0xd80, "Cortex-A520"}, {0xd81, "Cortex-A720"}, {0xd82, "Cortex-X4"}};
                const char *name = NULL;
                size_t k;

                for (k = 0; k < sizeof known / sizeof known[0]; k++) {
                    if (known[k].part == parts[i]) {
                        name = known[k].name;
                    }
                }
                if (name != NULL) {
                    o += (size_t)snprintf(out + o, cap - o, "%s%u x %s", o > 0 ? ", " : "ARM ", counts[i], name);
                } else {
                    o += (size_t)snprintf(out + o, cap - o, "%s%u x ARM part 0x%x", o > 0 ? ", " : "", counts[i], parts[i]);
                }
            }
        }
    }
#endif
    if (out[0] == '\0') {
        snprintf(out, cap, "unknown");
    }
}

#ifdef __ANDROID__
static const char *prop(const char *name, char *out)
{
    out[0] = '\0';
    __system_property_get(name, out);
    return out[0] != '\0' ? out : "?";
}
#endif

static void print_system(void)
{
    char cpu[160];
    char os[200] = "";

#ifdef _WIN32
    {
        typedef LONG(WINAPI * RtlGetVersionFn)(OSVERSIONINFOW *);
        typedef const char *(*WineVersionFn)(void);
        HMODULE nt = GetModuleHandleA("ntdll.dll");
        RtlGetVersionFn version = nt != NULL ? (RtlGetVersionFn)(void (*)(void))GetProcAddress(nt, "RtlGetVersion") : NULL;
        WineVersionFn wine = nt != NULL ? (WineVersionFn)(void (*)(void))GetProcAddress(nt, "wine_get_version") : NULL;
        OSVERSIONINFOW info;

        memset(&info, 0, sizeof info);
        info.dwOSVersionInfoSize = sizeof info;
        if (version != NULL && version(&info) == 0) {
            snprintf(os, sizeof os, "Windows %lu.%lu build %lu%s", (unsigned long)info.dwMajorVersion, (unsigned long)info.dwMinorVersion,
                     (unsigned long)info.dwBuildNumber, info.dwBuildNumber >= 22000 ? " (Windows 11)" : "");
        } else {
            snprintf(os, sizeof os, "Windows");
        }
        if (wine != NULL) {
            size_t n = strlen(os);

            snprintf(os + n, sizeof os - n, ", under Wine %s", wine());
        }
    }
#elif defined(__ANDROID__)
    {
        char a[PROP_VALUE_MAX], b[PROP_VALUE_MAX];

        snprintf(os, sizeof os, "Android %s (API %s)", prop("ro.build.version.release", a), prop("ro.build.version.sdk", b));
    }
#else
    {
        struct utsname u;
        FILE *f = p4_host_fopen("/etc/os-release", "r");
        char line[256], pretty[160] = "";

        while (f != NULL && fgets(line, sizeof line, f) != NULL) {
            if (strncmp(line, "PRETTY_NAME=", 12) == 0) {
                char *v = line + 12;
                size_t n = strlen(v);

                while (n > 0 && (v[n - 1] == '\n' || v[n - 1] == '"')) {
                    v[--n] = '\0';
                }
                snprintf(pretty, sizeof pretty, "%s", v[0] == '"' ? v + 1 : v);
            }
        }
        if (f != NULL) {
            fclose(f);
        }
        if (uname(&u) == 0) {
            snprintf(os, sizeof os, "%s%s%s %s", pretty, pretty[0] != '\0' ? ", " : "", u.sysname, u.release);
        }
#ifdef __APPLE__
        {
            char version[64] = "";
            size_t size = sizeof version;

            if (sysctlbyname("kern.osproductversion", version, &size, NULL, 0) == 0) {
                size_t n = strlen(os);

                snprintf(os + n, sizeof os - n, ", macOS %s", version);
            }
        }
#endif
    }
#endif
    say("os: %s, %s, SDL platform %s", os[0] != '\0' ? os : "unknown", arch_name(), SDL_GetPlatform());
    cpu_name(cpu, sizeof cpu);
    say("cpu: %s, %d logical cores; memory %d MiB", cpu, SDL_GetNumLogicalCPUCores(), SDL_GetSystemRAM());
#ifdef __ANDROID__
    {
        char a[PROP_VALUE_MAX], b[PROP_VALUE_MAX], c[PROP_VALUE_MAX], d[PROP_VALUE_MAX], e[PROP_VALUE_MAX], f[PROP_VALUE_MAX], g[PROP_VALUE_MAX],
            h[PROP_VALUE_MAX]; /* one buffer per value: they are all alive at the say() */

        say("device: %s %s (%s), SoC %s %s, platform %s, hardware %s, ABI %s", prop("ro.product.manufacturer", a), prop("ro.product.model", b),
            prop("ro.product.device", c), prop("ro.soc.manufacturer", d), prop("ro.soc.model", e), prop("ro.board.platform", f),
            prop("ro.hardware", g), prop("ro.product.cpu.abi", h));
    }
#endif
#ifdef __linux__
    {
        /* big.LITTLE and governors: the highest clock of every core, when the system lets a program read it */
        char list[256] = "";
        size_t o = 0;
        int core, shown = 0;

        for (core = 0; core < 32 && o + 8 < sizeof list; core++) {
            char path[96];
            FILE *f;
            unsigned khz = 0;

            snprintf(path, sizeof path, "/sys/devices/system/cpu/cpu%d/cpufreq/cpuinfo_max_freq", core);
            f = p4_host_fopen(path, "r");
            if (f == NULL) {
                continue;
            }
            if (fscanf(f, "%u", &khz) == 1) {
                o += (size_t)snprintf(list + o, sizeof list - o, "%scpu%d=%u", shown++ ? " " : "", core, khz / 1000u);
                if (core < 64) {
                    D.cpu_mhz[core] = khz / 1000u;
                }
            }
            fclose(f);
        }
        if (shown > 0) {
            say("cpu max MHz: %s", list);
        }
    }
#endif
}

static void print_settings(void)
{
    static const char *const keys[] = {"video.fullscreen", "video.width", "video.height", "video.resolution", "video.fit", "video.aspect", "video.movie_fit",
                                       "video.present", "video.vsync", "audio.volume", "input.deadzone", "input.touch_scroll", "game.mode"};
    char line[600], value[160];
    size_t o = 0, i;
    const P4CfgVideo *video;

    for (i = 0; i < sizeof keys / sizeof keys[0] && o + 80 < sizeof line; i++) {
        const char *v = p4cfg_get(keys[i]);

        p4diag_redact(v != NULL ? v : "(default)", value, sizeof value);
        o += (size_t)snprintf(line + o, sizeof line - o, "%s%s=%s", i > 0 ? " " : "", keys[i], value);
    }
    say("settings: %s", line);
    video = p4cfg_video();
    say("video policy: present=%s resolution=%s scale=%d render=%dx%d fit=%d aspect=%d fullscreen=%d window=%dx%d vsync=%d", video->blit ? "blit" : "direct",
        video->resolution == P4CFG_RESOLUTION_NATIVE ? "native" : video->resolution == P4CFG_RESOLUTION_EXPLICIT ? "explicit" : "scale", video->scale,
        video->renderWidth, video->renderHeight, video->fit, video->aspect, video->fullscreen, video->width, video->height, video->vsync);
}

static void print_mods(void)
{
    char mods[300], order[200], packs[300];
    const char *m = p4cfg_get("mods.enabled"), *o = p4cfg_get("mods.load_order"), *p = p4cfg_get("packs.enabled"), *sw = getenv("P4PC_MODS");

    p4diag_redact(m != NULL && m[0] != '\0' ? m : "none", mods, sizeof mods);
    p4diag_redact(o != NULL && o[0] != '\0' ? o : "default", order, sizeof order);
    p4diag_redact(p != NULL && p[0] != '\0' ? p : "none", packs, sizeof packs);
    say("mods: enabled=%s load_order=%s packs=%s switch=%s", mods, order, packs, sw != NULL ? sw : "(unset)");
}

static void print_environment(void)
{
    SDL_Environment *environment = SDL_GetEnvironment();
    char **variables = environment != NULL ? SDL_GetEnvironmentVariables(environment) : NULL;
    char line[600], value[200];
    size_t o = 0, i;

    for (i = 0; variables != NULL && variables[i] != NULL && o + 100 < sizeof line; i++) {
        const char *v = variables[i];

        /* the switches that change behaviour; not the ones that hold paths or are set by the program itself */
        if (strncmp(v, "P4PC_", 5) != 0 || strncmp(v, "P4PC_ISO=", 9) == 0 || strncmp(v, "P4PC_ELF=", 9) == 0 || strncmp(v, "P4PC_SAVE_DIR=", 14) == 0 ||
            strncmp(v, "P4PC_LOG_FILE=", 14) == 0 || strncmp(v, "P4PC_CONFIG=", 12) == 0 || strncmp(v, "P4PC_APP_VERSION=", 17) == 0 ||
            strncmp(v, "P4PC_LOGCAT=", 12) == 0) {
            continue;
        }
        p4diag_redact(v, value, sizeof value);
        o += (size_t)snprintf(line + o, sizeof line - o, "%s%s", o > 0 ? " " : "", value);
    }
    if (variables != NULL) {
        SDL_free(variables);
    }
    if (o > 0) {
        say("environment: %s", line);
    }
}

static unsigned long long file_size(const char *path)
{
    P4OSFileIdentity id;
    FILE *f = path != NULL && path[0] != '\0' ? p4_os_fopen_rb(path) : NULL;
    unsigned long long size = 0;

    if (f != NULL) {
        if (p4_os_file_identity(f, &id) == 0) {
            size = id.size;
        }
        fclose(f);
    }
    return size;
}

static void print_disc(const char *executable)
{
    const char *iso = p4_host_getenv("P4PC_ISO");
    unsigned long long image = file_size(iso), elf = file_size(executable);

    if (iso == NULL || iso[0] == '\0') {
        say("disc: no disc image named (started from an extracted executable); executable %s", elf != 0 ? "present" : "not readable");
    } else if (elf == 0) {
        say("disc: image %llu bytes; the game executable could not be read from it", image);
    } else {
        /* p4iso_locate_elf (desktop) and the Android launcher read SLUS_217.82 from the image's root before this point: that is the region check */
        say("disc: image %llu bytes; SLUS_217.82 (%llu bytes) found in its root: Persona 4 USA (SLUS-21782) check passed", image, elf);
    }
}

static void print_display(void)
{
    struct SDL_Window *window = p4pc_pad_sdl_window();
    const char *driver = SDL_GetCurrentVideoDriver();
    int w = 0, h = 0, interval = 0;

    D.display_done = 1;
    if (window == NULL) {
        say("display: SDL video driver %s, no window", driver != NULL ? driver : "none");
        return;
    }
    if (!SDL_GetWindowSizeInPixels(window, &w, &h)) {
        w = h = 0;
    }
    {
        const SDL_DisplayID display = SDL_GetDisplayForWindow(window);
        const SDL_DisplayMode *mode = display != 0 ? SDL_GetCurrentDisplayMode(display) : NULL;
        const char *name = display != 0 ? SDL_GetDisplayName(display) : NULL;

        if (!SDL_GL_GetSwapInterval(&interval)) {
            interval = -1;
        }
        say("display: SDL video driver %s, window %dx%d px%s, display \"%s\" %dx%d @ %.1f Hz, pixel density %.2f, swap interval %d",
            driver != NULL ? driver : "none", w, h, (SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN) ? " (fullscreen)" : "", name != NULL ? name : "?",
            mode != NULL ? mode->w : 0, mode != NULL ? mode->h : 0, mode != NULL ? (double)mode->refresh_rate : 0.0, mode != NULL ? (double)mode->pixel_density : 0.0,
            interval);
    }
}

static void print_header(const char *executable)
{
    char when[40];
    time_t now = time(NULL);

    utc_text(now, when, sizeof when);
    say("---- diagnostic header (format 1; the GPU is the next \"p4gl:\" line, the audio device the \"p4snd: output\" line) ----");
    say("started %s UTC; t= values below are seconds since then", when);
    find_build(D.build, sizeof D.build);
    say("build: %s", D.build);
    print_program();
    print_system();
    print_settings();
    print_mods();
    print_environment();
    print_disc(executable);
    {
        char note[256];

        snprintf(note, sizeof note, "build=%.120s", D.build);
        p4_os_crash_note_set(note);
    }
}

/* ------------------------------------------------------------------ frames */

#ifdef __linux__
/* The core the game thread last ran on (field 39 of /proc/thread-self/stat), counted at every area sample. */
static int current_cpu(void)
{
    char text[640];
    FILE *f = p4_host_fopen("/proc/thread-self/stat", "r");
    size_t n;
    char *p;
    int field;

    if (f == NULL) {
        return -1;
    }
    n = fread(text, 1, sizeof text - 1, f);
    fclose(f);
    text[n] = '\0';
    p = strrchr(text, ')');
    if (p == NULL) {
        return -1;
    }
    /* after the ")": field 3 is the state, field 39 the processor */
    for (field = 3, p++; field < 39 && *p != '\0'; p++) {
        if (*p == ' ') {
            field++;
        }
    }
    return field == 39 && *p >= '0' && *p <= '9' ? atoi(p) : -1;
}
#endif

static AreaRow *find_row(const P4DiagArea *area)
{
    unsigned i;

    for (i = 0; i < D.nrows; i++) {
        if (p4diag_area_equal(&D.rows[i].area, area)) {
            return &D.rows[i];
        }
    }
    if (D.nrows == MAX_ROWS) {
        return &D.other;
    }
    D.rows[D.nrows].area = *area;
    return &D.rows[D.nrows++];
}

static double seconds(uint64_t now)
{
    return (double)(now - D.start_ns) / 1e9;
}

static void print_stats_line(const char *label, const P4DiagStats *s, const char *extra)
{
    const double avg = s->frames != 0 ? (double)s->sum_ns / (double)s->frames / 1e6 : 0.0;
    const double busy = s->frames != 0 ? (double)s->busy_ns / (double)s->frames / 1e6 : 0.0;

    fprintf(stderr, "p4diag: %s n=%u avg=%.1fms p50=%u p95=%u p99=%u max=%.0fms slow=%u hitch=%u cad=%u/%u/%u/%u busy=%.1fms(%d%%)%s\n", label, (unsigned)s->frames,
            avg, p4diag_stats_percentile_ms(s, 50), p4diag_stats_percentile_ms(s, 95), p4diag_stats_percentile_ms(s, 99), (double)s->max_ns / 1e6,
            (unsigned)s->slow, (unsigned)s->hitch, (unsigned)s->cadence[0], (unsigned)s->cadence[1], (unsigned)s->cadence[2], (unsigned)s->cadence[3], busy,
            s->sum_ns != 0 ? (int)(s->busy_ns * 100u / s->sum_ns) : 0, extra);
}

static void cpu_text(char *out, size_t cap)
{
    unsigned best = 0, i;

    out[0] = '\0';
    if (D.cpu_samples == 0) {
        return;
    }
    for (i = 1; i < 64; i++) {
        if (D.cpu_hits[i] > D.cpu_hits[best]) {
            best = i;
        }
    }
    snprintf(out, cap, " core=%u@%uMHz(%u%%)", best, D.cpu_mhz[best], D.cpu_hits[best] * 100u / D.cpu_samples);
}

static void flush_window(uint64_t now)
{
    char label[200], area[120], cpu[64];

    if (D.window.frames >= MIN_LINE_FRAMES && D.perf_lines < MAX_PERF_LINES) {
        const double span = (double)(now - D.window_start_ns) / 1e9;

        p4diag_area_format(&D.area, area, sizeof area);
        snprintf(label, sizeof label, "perf t=%.0fs area=\"%s\" fps=%.1f", seconds(now), area, span > 0.0 ? (double)D.window.frames / span : 0.0);
        cpu_text(cpu, sizeof cpu);
        print_stats_line(label, &D.window, cpu);
        D.perf_lines++;
        if (D.perf_lines == MAX_PERF_LINES) {
            say("perf lines stop here (%d): the summary at exit still counts everything", MAX_PERF_LINES);
        }
    }
    memset(&D.window, 0, sizeof D.window);
    memset(D.cpu_hits, 0, sizeof D.cpu_hits);
    D.cpu_samples = 0;
    D.window_start_ns = now;
}

static void note_crash_context(uint64_t now, unsigned flip)
{
    char area[120], note[256];

    p4diag_area_format(&D.area, area, sizeof area);
    snprintf(note, sizeof note, "build=%.100s area=%s t=%.0fs flip=%u", D.build, area, seconds(now), flip);
    p4_os_crash_note_set(note);
}

static void poll_audio(uint64_t now)
{
    P4SndDiag a;
    unsigned i;

    p4snd_diag_poll(&a);
    if (!a.device && a.callbacks == 0) {
        return;
    }
    D.audio_device = 1;
    D.audio_period_us = a.period_us;
    D.audio_late = a.late;
    D.audio_slow = a.slow;
    D.audio_locks = a.lock_waits;
    D.audio_clipped = a.clipped_samples;
    D.audio_render_ns = a.render_sum_ns;
    D.audio_callbacks_seen = a.callbacks;
    if (a.render_max_ns > D.audio_render_max_ns) D.audio_render_max_ns = a.render_max_ns;
    if (a.gap_max_ns > D.audio_gap_max_ns) D.audio_gap_max_ns = a.gap_max_ns;
    if (a.lock_max_ns > D.audio_lock_max_ns) D.audio_lock_max_ns = a.lock_max_ns;
    if (a.peak > D.audio_peak) D.audio_peak = a.peak;
    if (a.peak > D.audio_peak_total) D.audio_peak_total = a.peak;
    for (i = 0; i < a.nevents; i++) {
        const P4SndDiagEvent *e = &a.events[i];
        char area[120];

        if (D.audio_lines >= MAX_AUDIO_LINES) {
            if (D.audio_lines == MAX_AUDIO_LINES) {
                say("audio event lines stop here (%d): the counters in the summary still count everything", MAX_AUDIO_LINES);
                D.audio_lines++;
            }
            break;
        }
        p4diag_area_format(&D.area, area, sizeof area);
        if (e->kind == P4SND_DIAG_LATE) {
            say("audio underrun? t=%.1fs area=\"%s\": the device callback came %.1f ms after the last one (device buffer %.1f ms)", seconds(e->t_ns), area,
                (double)e->a_us / 1000.0, (double)e->b_us / 1000.0);
        } else if (e->kind == P4SND_DIAG_SLOW) {
            say("audio slow render t=%.1fs area=\"%s\": the callback took %.1f ms to make %.1f ms of audio", seconds(e->t_ns), area, (double)e->a_us / 1000.0,
                (double)e->b_us / 1000.0);
        } else {
            say("audio lock wait t=%.1fs area=\"%s\": the mixer lock was held by the game thread for %.1f ms", seconds(e->t_ns), area, (double)e->a_us / 1000.0);
        }
        D.audio_lines++;
    }
    (void)now;
}

/* One line of audio statistics with every perf window: counters are cumulative, the maxima cover the window. */
static void print_audio_window(uint64_t now)
{
    if (!D.audio_device) {
        return;
    }
    say("audio t=%.0fs callbacks=%llu(+%llu) late=%llu slow=%llu lock-waits=%llu clipped=%llu peak=%.2f render-avg=%.2fms render-max=%.2fms gap-max=%.1fms "
        "device-buffer=%.1fms",
        seconds(now), (unsigned long long)D.audio_callbacks_seen, (unsigned long long)(D.audio_callbacks_seen - D.audio_window_callbacks),
        (unsigned long long)D.audio_late, (unsigned long long)D.audio_slow, (unsigned long long)D.audio_locks, (unsigned long long)D.audio_clipped,
        (double)D.audio_peak, D.audio_callbacks_seen != 0 ? (double)D.audio_render_ns / (double)D.audio_callbacks_seen / 1e6 : 0.0,
        (double)D.audio_render_max_ns / 1e6, (double)D.audio_gap_max_ns / 1e6, (double)D.audio_period_us / 1000.0);
    D.audio_window_callbacks = D.audio_callbacks_seen;
    D.audio_render_max_ns = D.audio_gap_max_ns = D.audio_lock_max_ns = 0;
    D.audio_peak = 0.0f;
}

static void on_frame(unsigned flip)
{
    const uint64_t now = p4_os_now_ns();
    uint64_t cpu = 0;
    const int have_cpu = p4_os_thread_cpu_ns(&cpu) == 0;

    if (!D.display_done) {
        print_display();
    }
    if (D.have_last) {
        const uint64_t dt = now - D.last_ns;
        const uint64_t busy = have_cpu && D.have_cpu && cpu >= D.last_cpu_ns ? cpu - D.last_cpu_ns : 0;

        if (dt >= (uint64_t)PAUSE_NS) {
            D.pauses++; /* the app was in the background (Android) or the machine slept: not a frame */
        } else {
            p4diag_stats_add(&D.window, dt, busy);
            p4diag_stats_add(&D.total, dt, busy);
            if (D.row != NULL) {
                p4diag_stats_add(&D.row->stats, dt, busy);
            }
            if (dt >= (uint64_t)HITCH_LOG_NS && D.hitch_lines < MAX_HITCH_LINES && now - D.last_hitch_ns > 1000000000ull) {
                char area[120];

                p4diag_area_format(&D.area, area, sizeof area);
                say("hitch t=%.1fs area=\"%s\": one flip took %.0f ms, %.0f ms of it game-thread CPU", seconds(now), area, (double)dt / 1e6, (double)busy / 1e6);
                D.hitch_lines++;
                D.last_hitch_ns = now;
            }
            if (now - D.window_start_ns >= (uint64_t)WINDOW_NS) {
                flush_window(now);
                print_audio_window(now);
            }
        }
    }
    D.last_ns = now;
    D.last_cpu_ns = cpu;
    D.have_last = 1;
    D.have_cpu = have_cpu;
    if (flip % AREA_EVERY == 0) {
        P4DiagArea area;

#ifdef __linux__
        {
            const int core = current_cpu();

            if (core >= 0 && core < 64) {
                D.cpu_hits[core]++;
                D.cpu_samples++;
            }
        }
#endif
        p4diag_area_read(&area);
        if (!p4diag_area_equal(&area, &D.area) || D.row == NULL) {
            flush_window(now);
            D.area = area;
            D.row = find_row(&area);
            note_crash_context(now, flip);
        } else if (flip % (AREA_EVERY * 4u) == 0) {
            note_crash_context(now, flip);
        }
    }
    if (flip % AUDIO_EVERY == 0) {
        poll_audio(now);
    }
}

/* ------------------------------------------------------------------ exit */

static int row_order(const void *a, const void *b)
{
    const AreaRow *x = *(const AreaRow *const *)a, *y = *(const AreaRow *const *)b;
    const unsigned px = p4diag_stats_percentile_ms(&x->stats, 95), py = p4diag_stats_percentile_ms(&y->stats, 95);

    if (px != py) {
        return px < py ? 1 : -1;
    }
    return x->stats.slow < y->stats.slow ? 1 : x->stats.slow > y->stats.slow ? -1 : 0;
}

void p4diag_shutdown(void)
{
    const uint64_t now = p4_os_now_ns();
    AreaRow *order[MAX_ROWS];
    unsigned i, n = 0;
    char label[200], extra[64];

    if (!D.started || D.finished) {
        return;
    }
    D.finished = 1;
    flush_window(now);
    poll_audio(now);
    snprintf(label, sizeof label, "summary run=%.0fs", seconds(now));
    snprintf(extra, sizeof extra, " background-gaps=%u", D.pauses);
    print_stats_line(label, &D.total, extra);
    for (i = 0; i < D.nrows; i++) {
        if (D.rows[i].stats.frames >= 120) {
            order[n++] = &D.rows[i];
        }
    }
    if (D.other.stats.frames >= 120 && n < MAX_ROWS) {
        order[n++] = &D.other;
    }
    qsort(order, n, sizeof order[0], row_order);
    for (i = 0; i < n && i < 8; i++) {
        char area[120];

        if (order[i] == &D.other) {
            snprintf(area, sizeof area, "other areas");
        } else {
            p4diag_area_format(&order[i]->area, area, sizeof area);
        }
        snprintf(label, sizeof label, "summary area \"%s\"", area);
        print_stats_line(label, &order[i]->stats, "");
    }
    if (D.audio_device) {
        say("audio summary: callbacks=%llu late=%llu slow-renders=%llu lock-waits=%llu clipped-samples=%llu peak=%.2f render-avg=%.2fms device-buffer=%.1fms",
            (unsigned long long)D.audio_callbacks_seen, (unsigned long long)D.audio_late, (unsigned long long)D.audio_slow, (unsigned long long)D.audio_locks,
            (unsigned long long)D.audio_clipped, (double)D.audio_peak_total,
            D.audio_callbacks_seen != 0 ? (double)D.audio_render_ns / (double)D.audio_callbacks_seen / 1e6 : 0.0, (double)D.audio_period_us / 1000.0);
    }
    fputs(P4_LOG_CLEAN_EXIT "\n", stderr);
    fflush(stderr);
}

void p4diag_startup(const char *executable)
{
    if (D.started || !p4diag_active()) {
        return;
    }
    D.started = 1;
    D.start_ns = D.window_start_ns = p4_os_now_ns();
    print_header(executable);
    p4pc_frame_hook = on_frame;
    atexit(p4diag_shutdown);
}

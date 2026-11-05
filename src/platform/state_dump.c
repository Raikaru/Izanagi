/* P4PC_DUMP_RAM_AT=<flip>[,<flip>...] with P4PC_DUMP_RAM=<prefix>: writes the PS2 RAM (0x00100000..0x02000000, 31 MB) at those flips
 * to <prefix>_<flip>.bin, to diff two builds' game state at the same flip (tools/ram_diff.py; src/viewer/p4pc_viewer.c does the same
 * after a render). Called from the flip with the flip count, before the flip's present. Host pointers in the image differ by build.
 * One run's RAM dumps (these and the captured-frame dumps of p4gl_context.c) stop at P4PC_DUMP_RAM_MAX_MB, 2048 by default. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "p4pc/gmem.h"

#if defined(__SSE2__)
#include <xmmintrin.h>
#endif

#include "p4pc/hostmath.h"
#include "p4pc/os.h"
#ifdef P4_MOD_GAME_BINDINGS
#include "p4pc/mod_fidelity.h"
#endif

/* The FPU state the game's float code runs in: MXCSR (EE mode: 0xE0C0 in the rounding, flush and denormal bits) and the x87 control word
 * (0x037F: extended precision, nearest; the nearest division and square root of eefloat.h run there). P4PC_FPU_LOG=1 prints it. */
void p4pc_fpu_state_log(const char *where)
{
#if defined(__SSE2__)
    unsigned short cw;

    if (getenv("P4PC_FPU_LOG") == NULL) {
        return;
    }
    __asm__ volatile("fnstcw %0" : "=m"(cw));
    fprintf(stderr, "p4pc: fpu state %s: mxcsr %08x x87 cw %04x\n", where, _mm_getcsr(), (unsigned)cw);
#else
    (void)where;
#endif
}

/* A driver or system DLL may change the thread's float state in a call (the NVIDIA OpenGL driver on Windows sets the x87 control word to round
 * toward zero, 0x0F7F, in its first call, and leaves it: every nearest division and square root of the game then truncated, the guest RAM
 * differed from the Linux build's from flip 2, and the fog-dream tutorial battle stalled). Called after such calls and once per flip: when the state is not the
 * EE mode it says so (the first few times) and restores it. */
void p4pc_fpu_check(const char *where)
{
#if defined(__SSE2__)
    static int reports;
    unsigned short cw;
    const unsigned csr = _mm_getcsr();

    __asm__ volatile("fnstcw %0" : "=m"(cw));
    if (cw == 0x037F && (csr & P4PC_MXCSR_EE) == P4PC_MXCSR_EE) {
        return;
    }
    if (reports < 8) {
        reports++;
        fprintf(stderr, "p4pc: float state changed by %s (mxcsr %08x x87 cw %04x), restoring the EE mode\n", where, csr, (unsigned)cw);
    }
    {
        const unsigned short good = 0x037F;

        __asm__ volatile("fldcw %0" : : "m"(good));
    }
    _mm_setcsr(csr | P4PC_MXCSR_EE);
#else
    (void)where;
#endif
}

/* P4PC_EVENT_LOG=N: observe every Nth presented frame without changing event state. Used to register real-play captures to
 * corpus savestates: calendar/field, camera LTM/view window/clip planes and the active ScrScriptWork procedure list. */
static uint32_t event_word(uint32_t address)
{
    uint32_t value;

    memcpy(&value, P4_G2H(address), sizeof value);
    return value;
}

static int event_address(uint32_t address, uint32_t size)
{
#ifdef P4_GUEST64
    return address >= 0x00100000u && size <= P4_GUEST_SIZE && address <= P4_GUEST_SIZE - size;
#else
    return address >= 0x00100000u && size <= 0x02000000u && address <= 0x02000000u - size;
#endif
}

static void event_log(unsigned flip)
{
    static int interval = -1;
    uint32_t scene, camera, frame, script;
    float matrix[16], view[2], planes[2];
    unsigned n;

    if (interval < 0) {
        const char *value = getenv("P4PC_EVENT_LOG");

        interval = value != NULL ? atoi(value) : 0;
        #ifdef P4_MOD_GAME_BINDINGS
        if (interval > 0) p4_mod_fidelity_check_start();
        #endif
    }
    if (interval <= 0 || flip % (unsigned)interval != 0) {
        return;
    }
    scene = P4_GPDATA(uint32_t, 0x9db0);
    camera = event_word(0x00764b70u);
    frame = event_address(camera, 136) ? event_word(camera + 4) : 0;
    if (!event_address(scene, 8) || !event_address(frame, 144)) {
        return;
    }
    memcpy(matrix, P4_G2H(frame + 80), sizeof matrix);
    memcpy(view, P4_G2H(camera + 104), sizeof view);
    memcpy(planes, P4_G2H(camera + 128), sizeof planes);
    fprintf(stderr, "EVENTSTATE flip %u clock %u field %u:%u date %u slot %u host_ns %llu camera", flip,
            (unsigned)event_word(0x00764290u), (unsigned)event_word(scene), (unsigned)event_word(scene + 4),
            (unsigned)P4_GDATA(uint16_t, 0x00797b7au), (unsigned)P4_GDATA(uint8_t, 0x00797b7cu),
            (unsigned long long)p4_os_now_ns());
    for (n = 0; n < 16; n++) {
        fprintf(stderr, " %.9g", (double)matrix[n]);
    }
    fprintf(stderr, " %.9g %.9g %.9g %.9g procs", (double)view[0], (double)view[1], (double)planes[0], (double)planes[1]);
    script = event_word(0x00764608u);
    for (n = 0; n < 32 && event_address(script, 0x154); n++) {
        fprintf(stderr, " %.*s:%u", 64, (const char *)P4_G2H(script), (unsigned)event_word(script + 0x12c));
        script = event_word(script + 0x150);
    }
    fputc('\n', stderr);
}

/* A semantic entry, not a chosen absolute frame: FldRootUpdate reaches 12
 * after installing/settling actors and starting the entry script, 13 for
 * ordinary control, or 14 for its event task. Bits 2/4 are pending requests;
 * bit 1 is a scene-reuse shortcut, not a mandatory completion bit. */
static void field_entry_fail(const char *why)
{
    fprintf(stderr, "p4pc: field entry dump: %s\n", why);
    exit(4);
}

static void field_entry_words(FILE *out, const char *name, uint32_t address, unsigned count)
{
    fprintf(out, ",\"%s\":", name);
    if (!address) {
        fputs("null", out);
        return;
    }
    if (!event_address(address, count * 4u)) field_entry_fail("unmapped environment data");
    fputc('[', out);
    for (unsigned i = 0; i < count; i++)
        fprintf(out, "%s\"%08x\"", i ? "," : "", (unsigned)event_word(address + i * 4u));
    fputc(']', out);
}

static void field_entry_dump(unsigned flip, const char *path, int capture)
{
    extern int64_t calGetDayEntryByPhase(int32_t day, uint32_t phase);
    static FILE *out;
    static uint32_t last_root, last_ids, entries;
    static int captured;
    const uint32_t root = event_word(0x007d2510u);
    const uint32_t work = event_address(root, 0x3cu) ? event_word(root + 0x38u) : 0;
    /* A selected compositor moment can precede FldRootUpdate's ready state
     * (the TV entrance is state 5). Sample its actual live scene, not a
     * previous completed visit; only ordinary entry observations need this
     * load-completion predicate. */
    if (!capture && (!event_address(work, 0x24u) ||
                     event_word(work) < 12u || event_word(work) > 14u)) {
        captured = 0;
        return;
    }
    const uint32_t ids = event_word(work + 0x18u);
    const uint32_t scene = event_word(0x00762ea0u);
    if (!event_address(scene, 0x14u)) {
        if (capture) field_entry_fail("unmapped captured scene");
        return;
    }
    if (!capture && (event_word(scene + 0xcu) & 6u) != 0u) return;
    const uint32_t field = event_word(scene) & 0xffffu, room = event_word(scene + 4u) & 0xffffu;
    if (!capture && (field != (ids & 0xffffu) || room != (ids >> 16))) return;
    if (!capture && captured && root == last_root && ids == last_ids) return;
    const uint32_t manager = event_word(scene + 8u);
    if (!event_address(manager, 22u * 4u)) field_entry_fail("unmapped ready scene manager");
    const uint32_t camera = event_word(0x00764b70u);
    const uint32_t environment = event_word(manager + 12u * 4u);
    const uint32_t date = event_word(0x00797b78u) >> 16;
    const uint32_t slot = event_word(0x00797b7cu) & 0xffu;
    const uint32_t selection = event_word(scene + 0x10u);
    if (!out) {
        out = fopen(path, "wb");
        if (!out) field_entry_fail("opening output file");
    }
    fprintf(out, "{\"schema\":2,\"capture\":%s,\"entry\":%u,\"field\":%u,\"room\":%u,\"game_frame\":%u,\"flip\":%u,"
                 "\"date\":%u,\"time_slot\":%u,\"weather_condition\":%d,"
                 "\"map_state\":%u,\"environment_state\":%u,\"filter_index\":%u",
            capture ? "true" : "false", (unsigned)++entries, (unsigned)field, (unsigned)room, (unsigned)event_word(0x00764290u), flip,
            (unsigned)date, (unsigned)slot,
            (int)calGetDayEntryByPhase((int16_t)date, slot), (unsigned)(selection & 0xffffu),
            (unsigned)(selection >> 16), (unsigned)event_word(0x007643a8u));
    /* Observe the actual latch, not only the date-table value recomputed above:
     * CalendarState packs state, date/period and current/saved weather here. */
    field_entry_words(out, "calendar_latch_bits", event_word(0x007642ccu), 3);
    field_entry_words(out, "camera_clip_bits", event_address(camera, 140u) ? camera + 128u : 0, 3);
    field_entry_words(out, "background", 0x00764b3cu, 8);
    field_entry_words(out, "field_far_bits", 0x00764b5cu, 1);
    field_entry_words(out, "environment_colors", environment ? environment + 0x140u : 0, 8);
    field_entry_words(out, "environment_transform", environment ? environment + 0x160u : 0, 16);
    field_entry_words(out, "event_flags", 0x0079b1ccu, 0xb0u);
    fputs(",\"resources\":[", out);
    /* resrc_manager_internal.h: these are live placed-object records shared
     * by update/render, not file-cache assets. Types 1/2/3 include actors;
     * u16 IDs encode type<<10 | instance, next is +0x138. Preserve every
     * instance/count, never a capped subset or a host/heap address. */
    for (unsigned type = 0; type < 22; type++) {
        uint32_t node = event_word(manager + type * 4u);
        unsigned count = 0;
        fprintf(out, "%s{\"type\":%u,\"ids\":[", type ? "," : "", type);
        while (node) {
            if (!event_address(node, 0x140u) || ++count > P4_GUEST_SIZE / 0x140u)
                field_entry_fail("invalid/cyclic scene resource list");
            fprintf(out, "%s%u", count > 1 ? "," : "", (unsigned)(event_word(node) & 0xffffu));
            node = event_word(node + 0x138u);
        }
        fprintf(out, "],\"count\":%u}", count);
    }
    fputs("]}\n", out);
    if (fflush(out)) field_entry_fail("writing output file");
    if (!capture) {
        captured = 1;
        last_root = root;
        last_ids = ids;
    }
    fprintf(stderr, "p4pc: field %s %u %u:%u game %u -> %s\n", capture ? "capture" : "entry", (unsigned)entries,
            (unsigned)field, (unsigned)room, (unsigned)event_word(0x00764290u), path);
}

/* The run refuses to continue past its RAM-dump volume: on 2026-11-03 timed dumps wrote 39 GB to C: in half an hour. Exits before
 * writing the dump that would pass P4PC_DUMP_RAM_MAX_MB (default 2048 MB, about 66 dumps). */
void p4pc_dump_ram_admit(const char *path)
{
    static unsigned long long used, cap;
    static int init;
    const unsigned long long size = 0x02000000u - 0x00100000u;

    if (!init) {
        const char *value = getenv("P4PC_DUMP_RAM_MAX_MB");
        init = 1;
        cap = (value != NULL && *value != '\0' ? strtoull(value, NULL, 10) : 2048ull) << 20;
    }
    if (used + size > cap) {
        fprintf(stderr, "p4pc: refusing RAM dump %s: this run already wrote %llu dumps (%llu MB) and the next would pass "
                        "P4PC_DUMP_RAM_MAX_MB=%llu; stopping. Dump fewer flips, delete dumps once read, or raise the cap "
                        "deliberately.\n", path, used / size, used >> 20, cap >> 20);
        exit(1);
    }
    used += size;
}

static unsigned sRequestedDump;

void p4pc_state_dump_request(unsigned flip)
{
    sRequestedDump = flip;
}

void p4pc_state_dump_flip(unsigned flip)
{
    static const char *prefix, *list, *entry_path;
    static int init;
    const char *c;
    int wanted = sRequestedDump == flip;

    if (!init) {
        init = 1;
        prefix = getenv("P4PC_DUMP_RAM");
        list = getenv("P4PC_DUMP_RAM_AT");
        entry_path = getenv("P4PC_FIELD_ENTRY_DUMP");
        #ifdef P4_MOD_GAME_BINDINGS
        if (prefix || (entry_path && *entry_path)) p4_mod_fidelity_check_start();
        #endif
    }
    event_log(flip);
    if (entry_path && *entry_path) field_entry_dump(flip, entry_path, 0);
    if (getenv("P4PC_FPU_LOG") != NULL && (flip <= 3 || flip % 500 == 0)) {
        p4pc_fpu_state_log("at a flip");
    }
    if (prefix == NULL) return;
    for (c = list; c != NULL && *c != '\0';) {
        char *end;
        const unsigned long v = strtoul(c, &end, 10);
        if (end == c) break;
        if (v == flip) {
            wanted = 1;
            break;
        }
        c = *end == ',' ? end + 1 : end;
        if (*end != ',') break;
    }
    if (wanted) {
        char path[512];
        FILE *f;
        if (entry_path && *entry_path) field_entry_dump(flip, entry_path, 1);
        p4pc_fpu_state_log("at the dump");
        snprintf(path, sizeof path, "%s_%u.bin", prefix, flip);
        p4pc_dump_ram_admit(path);
        f = fopen(path, "wb");
        if (f != NULL) {
            fwrite(P4_G2H(0x00100000u), 1, 0x02000000u - 0x00100000u, f);
            fclose(f);
            fprintf(stderr, "p4pc: dumped guest RAM at flip %u to %s\n", flip, path);
        }
    }
}

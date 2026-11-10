/* Diagnostic GPU timestamps. Four preallocated query banks retain at least two
 * frames of latency. An unavailable bank drops a sample, never waits for the GPU.
 * Timestamp intervals are GPU timeline elapsed time, not GPU active/busy time. */
#include "p4gl_profile.h"
#include "p4gl_gl.h"
#include "p4gl_win.h"
#include <SDL3/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define GPU_BANKS 4
#define GPU_MARKERS 129
#define GPU_TIMESTAMP 0x8E28
#define GPU_COUNTER_BITS 0x8864
#define GPU_RESULT 0x8866
#define GPU_AVAILABLE 0x8867
#define GPU_DISJOINT 0x8FBB
#define GPU_EXTENSIONS 0x1F03
#define GPU_NUM_EXTENSIONS 0x821D

typedef struct GpuBank {
    GLuint query[GPU_MARKERS];
    unsigned pass[GPU_MARKERS];
    unsigned count, frame, serial;
    int pending, overflow, disjoint;
    uint64_t cpuStart, cpuEnd;
} GpuBank;

typedef struct GpuTrace {
    GpuBank bank[GPU_BANKS];
    unsigned write, serial, selected;
    int active, skipping, isEs;
    uint64_t mask;
    void (P4GL_APIENTRY *gen)(GLsizei, GLuint *);
    void (P4GL_APIENTRY *del)(GLsizei, const GLuint *);
    void (P4GL_APIENTRY *stamp)(GLuint, GLenum);
    void (P4GL_APIENTRY *available)(GLuint, GLenum, GLuint *);
    void (P4GL_APIENTRY *result)(GLuint, GLenum, uint64_t *);
} GpuTrace;

static GpuTrace *sGpu;
static FILE *sFile;
static int sExitRegistered;
void (*p4gl_profile_pass)(unsigned pass);

uint64_t p4gl_profile_now_ns(void)
{
    const uint64_t ticks = SDL_GetPerformanceCounter();
    const uint64_t frequency = SDL_GetPerformanceFrequency();
    return ticks / frequency * 1000000000ull + ticks % frequency * 1000000000ull / frequency;
}

static const char *pass_name(unsigned pass)
{
    static const char *const names[] = {"other", "scene", "feedback", "ui", "present", "movie"};
    return pass < sizeof names / sizeof names[0] ? names[pass] : "other";
}

static void status(unsigned frame, const char *why)
{
    fprintf(sFile, "%u,-1,frame,0,0,0,%s,0,0\n", frame, why);
}

static void trace_exit(void)
{
    /* Context teardown ordering is not under our control at process exit.
     * Record outstanding samples without making any GL calls or waiting. */
    if (sFile == NULL) return;
    if (sGpu != NULL) {
        for (unsigned b = 0; b < GPU_BANKS; b++) {
            if (sGpu->bank[b].pending) status(sGpu->bank[b].frame, "pending_at_exit");
        }
        if (sGpu->active || sGpu->skipping) status(0, "incomplete_frame_at_exit");
    }
    fclose(sFile);
    sFile = NULL;
}

static void stamp_query(GLuint query)
{
    const uint64_t start = p4gl_gl_hitch_begin();
    sGpu->stamp(query, GPU_TIMESTAMP);
    if (start) p4gl_gl_hitch_api("query_counter_api", start, query, 0, 0);
}

static void consume(void)
{
    for (unsigned b = 0; b < GPU_BANKS; b++) {
        GpuBank *bank = &sGpu->bank[b];
        GLuint ready = 0;
        uint64_t first = 0, previous = 0;
        if (!bank->pending || sGpu->serial - bank->serial < 2u) continue;
        const uint64_t available_start = p4gl_gl_hitch_begin();
        sGpu->available(bank->query[bank->count - 1], GPU_AVAILABLE, &ready);
        if (available_start) p4gl_gl_hitch_api("query_available_api", available_start, bank->query[bank->count - 1], 0, 0);
        if (!ready) continue;
        if (bank->overflow || bank->disjoint) {
            status(bank->frame, bank->overflow ? "marker_overflow" : "disjoint");
        } else if (bank->cpuEnd - bank->cpuStart > sGpu->mask) {
            status(bank->frame, "counter_wrap_ambiguous");
        } else {
            for (unsigned i = 0; i < bank->count; i++) {
                uint64_t now = 0;
                const uint64_t result_start = p4gl_gl_hitch_begin();
                /* Only after the final query is available: command ordering makes
                 * every earlier query available too. No blocking result fetch. */
                sGpu->result(bank->query[i], GPU_RESULT, &now);
                if (result_start) p4gl_gl_hitch_api("query_result_api", result_start, bank->query[i], 0, 0);
                if (i == 0) first = now;
                else fprintf(sFile, "%u,%u,%s,%llu,%llu,%llu,ok,%llu,%llu\n",
                    bank->frame, i - 1, pass_name(bank->pass[i - 1]),
                    (unsigned long long)previous, (unsigned long long)now,
                    (unsigned long long)((now - previous) & sGpu->mask),
                    (unsigned long long)bank->cpuStart, (unsigned long long)bank->cpuEnd);
                previous = now;
            }
            fprintf(sFile, "%u,-1,frame,%llu,%llu,%llu,ok,%llu,%llu\n", bank->frame,
                (unsigned long long)first, (unsigned long long)previous,
                (unsigned long long)((previous - first) & sGpu->mask),
                (unsigned long long)bank->cpuStart, (unsigned long long)bank->cpuEnd);
        }
        bank->pending = 0;
        bank->count = 0;
    }
}

void p4gl_profile_touch(void)
{
    GpuBank *bank;
    if (sGpu == NULL || sGpu->active || sGpu->skipping) return;
    bank = &sGpu->bank[sGpu->write];
    if (bank->pending) {
        sGpu->skipping = 1;
        return;
    }
    bank->count = 1;
    bank->overflow = bank->disjoint = 0;
    bank->pass[0] = sGpu->selected;
    bank->cpuStart = p4gl_profile_now_ns();
    stamp_query(bank->query[0]);
    sGpu->active = 1;
}

static void mark_pass(unsigned pass)
{
    GpuBank *bank;
    sGpu->selected = pass;
    if (!sGpu->active) return;
    bank = &sGpu->bank[sGpu->write];
    if (bank->overflow || bank->pass[bank->count - 1] == pass) return;
    /* Keep one query for the frame's final endpoint. Overflow invalidates the
     * whole frame rather than publishing an apparently complete partial sum. */
    if (bank->count >= GPU_MARKERS - 1) {
        bank->overflow = 1;
        return;
    }
    stamp_query(bank->query[bank->count]);
    bank->pass[bank->count++] = pass;
}
unsigned p4gl_profile_feedback_begin(void)
{
    unsigned previous = P4GL_PROFILE_OTHER;
    if (sGpu != NULL) {
        previous = sGpu->selected;
        if (previous != P4GL_PROFILE_PRESENT) mark_pass(P4GL_PROFILE_FEEDBACK);
    }
    return previous;
}

void p4gl_profile_feedback_end(unsigned previous)
{
    if (sGpu != NULL) mark_pass(previous);
}


void p4gl_profile_frame_end(unsigned frame)
{
    GpuBank *bank;
    if (sGpu == NULL) return;
    if (!sGpu->active && !sGpu->skipping) return;
    if (sGpu->isEs) {
        GLint disjoint = 0;
        p4gl_glGetIntegerv(GPU_DISJOINT, &disjoint);
        if (disjoint) {
            for (unsigned b = 0; b < GPU_BANKS; b++) sGpu->bank[b].disjoint = 1;
        }
    }
    consume();
    if (sGpu->skipping) {
        status(frame, "query_ring_busy");
        sGpu->skipping = 0;
    } else {
        bank = &sGpu->bank[sGpu->write];
        stamp_query(bank->query[bank->count++]);
        bank->cpuEnd = p4gl_profile_now_ns();
        bank->frame = frame;
        bank->serial = sGpu->serial;
        bank->pending = 1;
        sGpu->write = (sGpu->write + 1) % GPU_BANKS;
        sGpu->active = 0;
    }
    sGpu->serial++;
    sGpu->selected = P4GL_PROFILE_OTHER;
    if (frame % 60u == 0) fflush(sFile);
}

static int has_extension(const char *wanted)
{
    GLint count = 0;
    p4gl_glGetIntegerv(GPU_NUM_EXTENSIONS, &count);
    for (GLint i = 0; i < count; i++) {
        const char *name = (const char *)p4gl_glGetStringi(GPU_EXTENSIONS, (GLuint)i);
        if (name != NULL && strcmp(name, wanted) == 0) return 1;
    }
    return 0;
}

/* Called only with a replacement/new context current: old GL names must not be
 * deleted in the new context. The old context already released its own storage. */
void p4gl_profile_init(int isEs)
{
    const char *path = getenv("P4PC_GPU_TRACE");
    void (P4GL_APIENTRY *bits)(GLenum, GLenum, GLint *);
    GLint precision = 0;
    if (sGpu != NULL) {
        if (sGpu->active || sGpu->skipping) status(0, "incomplete_frame_context_lost");
        for (unsigned b = 0; b < GPU_BANKS; b++) {
            if (sGpu->bank[b].pending) status(sGpu->bank[b].frame, "context_lost");
        }
        free(sGpu);
        sGpu = NULL;
    }
    p4gl_profile_pass = NULL;
    if (path == NULL || *path == '\0') return;
    if (sFile == NULL) {
        sFile = fopen(path, "w");
        if (sFile == NULL) {
            fprintf(stderr, "p4gl: cannot open GPU trace %s\n", path);
            return;
        }
        setvbuf(sFile, NULL, _IOFBF, 65536);
        fprintf(sFile, "frame,segment,pass,gpu_start_ns,gpu_end_ns,elapsed_ns,status,cpu_start_ns,cpu_end_ns\n");
        if (!sExitRegistered) {
            if (atexit(trace_exit) != 0) {
                status(0, "exit_handler_unavailable");
                fclose(sFile);
                sFile = NULL;
                return;
            }
            sExitRegistered = 1;
        }
    }
    if (isEs && !has_extension("GL_EXT_disjoint_timer_query")) {
        status(0, "unsupported_disjoint_timer_query");
        fflush(sFile);
        return;
    }
    sGpu = calloc(1, sizeof *sGpu);
    if (sGpu == NULL) {
        status(0, "allocation_failed");
        return;
    }
    sGpu->isEs = isEs;
    sGpu->gen = (void (P4GL_APIENTRY *)(GLsizei, GLuint *))p4gl_win_proc("glGenQueries");
    sGpu->del = (void (P4GL_APIENTRY *)(GLsizei, const GLuint *))p4gl_win_proc("glDeleteQueries");
    sGpu->stamp = (void (P4GL_APIENTRY *)(GLuint, GLenum))p4gl_win_proc(isEs ? "glQueryCounterEXT" : "glQueryCounter");
    sGpu->available = (void (P4GL_APIENTRY *)(GLuint, GLenum, GLuint *))p4gl_win_proc("glGetQueryObjectuiv");
    sGpu->result = (void (P4GL_APIENTRY *)(GLuint, GLenum, uint64_t *))p4gl_win_proc(isEs ? "glGetQueryObjectui64vEXT" : "glGetQueryObjectui64v");
    bits = (void (P4GL_APIENTRY *)(GLenum, GLenum, GLint *))p4gl_win_proc(isEs ? "glGetQueryivEXT" : "glGetQueryiv");
    if (!sGpu->gen || !sGpu->del || !sGpu->stamp || !sGpu->available || !sGpu->result || !bits) {
        status(0, "unsupported_query_entrypoints");
        free(sGpu);
        sGpu = NULL;
        fflush(sFile);
        return;
    }
    bits(GPU_TIMESTAMP, GPU_COUNTER_BITS, &precision);
    if (precision <= 0 || precision > 64) {
        status(0, "unsupported_timestamp_counter");
        free(sGpu);
        sGpu = NULL;
        fflush(sFile);
        return;
    }
    sGpu->mask = precision == 64 ? UINT64_MAX : (1ull << precision) - 1;
    for (unsigned b = 0; b < GPU_BANKS; b++) sGpu->gen(GPU_MARKERS, sGpu->bank[b].query);
    p4gl_profile_pass = mark_pass;
    fprintf(stderr, "p4gl: nonblocking GPU timestamp trace enabled (%s, %d-bit counter, 4 query banks)\n",
        isEs ? "EXT_disjoint_timer_query" : "ARB_timer_query", precision);
}

void p4gl_profile_close(void)
{
    p4gl_profile_pass = NULL;
    if (sGpu != NULL) {
        consume();
        if (sGpu->active || sGpu->skipping) status(0, "incomplete_frame_at_close");
        for (unsigned b = 0; b < GPU_BANKS; b++) {
            if (sGpu->bank[b].pending) status(sGpu->bank[b].frame, "pending_at_close");
            sGpu->del(GPU_MARKERS, sGpu->bank[b].query);
        }
        free(sGpu);
        sGpu = NULL;
    }
    if (sFile != NULL) {
        fclose(sFile);
        sFile = NULL;
    }
}

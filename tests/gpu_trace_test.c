/* A deterministic query transport proves the observer never requests unavailable
 * results. This is not GPU performance evidence; real-driver captures are separate. */
#include <SDL3/SDL.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "p4pc/os.h"
#include "p4gl_gl.h"
#include "p4gl_profile.h"

static GLuint nextQuery = 1;
static uint64_t clockNs, timestamp[8192];
static unsigned reads, deletes;
static int ready, disjoint, extension = 1, failed;
static const char kTracePath[] = "gpu_trace_host.csv";

static void P4GL_APIENTRY gen(GLsizei n, GLuint *ids)
{
    while (n--) *ids++ = nextQuery++;
}
static void P4GL_APIENTRY del(GLsizei n, const GLuint *ids)
{
    (void)ids;
    deletes += (unsigned)n;
}
static void P4GL_APIENTRY stamp(GLuint id, GLenum target)
{
    (void)target;
    if (id >= sizeof timestamp / sizeof timestamp[0]) abort();
    timestamp[id] = clockNs += 1000000;
}
static void P4GL_APIENTRY available(GLuint id, GLenum target, GLuint *value)
{
    (void)id; (void)target;
    *value = (GLuint)ready;
}
static void P4GL_APIENTRY result(GLuint id, GLenum target, uint64_t *value)
{
    (void)target;
    if (!ready) { fprintf(stderr, "blocking GPU result request\n"); failed = 1; }
    reads++;
    *value = timestamp[id];
}
static void P4GL_APIENTRY bits(GLenum target, GLenum name, GLint *value)
{
    (void)target; (void)name;
    *value = 64;
}
static void P4GL_APIENTRY get_integer(GLenum name, GLint *value)
{
    if (name == 0x821D) *value = extension;
    else if (name == 0x8FBB) { *value = disjoint; disjoint = 0; }
    else abort();
}
static const GLubyte *P4GL_APIENTRY get_string(GLenum name, GLuint index)
{
    (void)name; (void)index;
    return (const GLubyte *)"GL_EXT_disjoint_timer_query";
}

SDL_FunctionPointer SDLCALL p4gpu_test_proc(const char *name)
{
#define RESOLVE(prefix, function) if (strncmp(name, prefix, strlen(prefix)) == 0) return (SDL_FunctionPointer)function
    RESOLVE("glGenQueries", gen);
    RESOLVE("glDeleteQueries", del);
    RESOLVE("glQueryCounter", stamp);
    RESOLVE("glGetQueryObjectui64v", result);
    RESOLVE("glGetQueryObjectuiv", available);
    RESOLVE("glGetQueryiv", bits);
#undef RESOLVE
    return NULL;
}

static void frame(unsigned id)
{
    p4gl_profile_pass(P4GL_PROFILE_SCENE);
    p4gl_profile_touch();
    p4gl_profile_pass(P4GL_PROFILE_UI);
    p4gl_profile_pass(P4GL_PROFILE_PRESENT);
    p4gl_profile_frame_end(id);
}

static void expect(int value, const char *message)
{
    if (!value) { fprintf(stderr, "gpu_trace_test: %s\n", message); failed = 1; }
}

static int contains(const char *path, const char *text)
{
    FILE *file = fopen(path, "rb");
    char line[512];
    int found = 0;
    if (file == NULL) return 0;
    while (fgets(line, sizeof line, file) != NULL) {
        if (strstr(line, text) != NULL) { found = 1; break; }
    }
    fclose(file);
    return found;
}

static void check_exit_receipt(void)
{
    if (!contains(kTracePath, "pending_at_exit")) {
        fprintf(stderr, "gpu_trace_test: pending process-exit receipt absent\n");
        _Exit(1);
    }
    remove(kTracePath);
}

int main(void)
{
    const char *path = kTracePath;
    unsigned before;
    if (atexit(check_exit_receipt) != 0) return 1;
    p4gl_glGetIntegerv = get_integer;
    p4gl_glGetStringi = get_string;
    /* Set the CRT read by profile_init, not SDL's potentially different DLL CRT. */
    expect(p4_os_setenv("P4PC_GPU_TRACE", path, 1) == 0, "trace environment setup failed");
    p4gl_profile_init(0);
    expect(p4gl_profile_pass != NULL, "desktop timer transport not enabled");
    if (p4gl_profile_pass == NULL) return 1;
    for (unsigned id = 1; id <= 5; id++) frame(id);
    expect(reads == 0, "unavailable results were fetched");
    ready = 1;
    for (unsigned id = 6; id <= 10; id++) frame(id);
    expect(reads > 0, "available delayed results were not consumed");
    p4gl_profile_touch();
    for (unsigned i = 0; i < 300; i++) p4gl_profile_pass(i % 2 ? P4GL_PROFILE_UI : P4GL_PROFILE_SCENE);
    p4gl_profile_frame_end(11);
    for (unsigned id = 12; id <= 15; id++) frame(id);
    p4gl_profile_init(1); /* old-context names must not be deleted in this context */
    expect(deletes == 0, "context recovery deleted stale names");
    ready = 0;
    frame(20);
    frame(21);
    before = reads;
    disjoint = 1;
    frame(22);
    ready = 1;
    frame(23);
    expect(reads == before, "disjoint timestamp results were published");
    p4gl_profile_close();
    expect(deletes == 4u * 129u, "current-context queries not released");
    expect(contains(path, "query_ring_busy"), "busy bank receipt absent");
    expect(contains(path, "marker_overflow"), "overflow frame receipt absent");
    expect(contains(path, "disjoint"), "disjoint frame receipt absent");
    expect(contains(path, "context_lost"), "lost-context receipt absent");
    extension = 0;
    p4gl_profile_init(1);
    expect(p4gl_profile_pass == NULL, "missing GLES extension incorrectly enabled queries");
    p4gl_profile_close();
    expect(contains(path, "unsupported_disjoint_timer_query"), "unsupported capability receipt absent");
    extension = 1;
    ready = 0;
    p4gl_profile_init(0);
    frame(99); /* leave a pending bank for the non-GL process-exit handler */
    return failed ? 1 : 0;
}

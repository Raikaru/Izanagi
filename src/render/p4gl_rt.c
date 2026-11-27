/* The render thread: a command stream between the game thread's GL calls and the driver (p4gl_rt.h, docs/platform/render_thread.md).
 *
 * Record format: 8-byte header (op:16, words:8, payload bytes:32), `words` 64-bit argument words, the payload padded to 8 bytes. Floats travel as
 * their bits. Chunks of the stream are filled by the game thread and queued; the render thread executes them in order. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* On Windows SDL_CreateThread hands _beginthreadex and _endthreadex over as function pointers, which the CRT guard reads as callbacks of the wrong shape;
 * with NULL SDL creates the thread itself. */
#define SDL_BeginThreadFunction NULL
#define SDL_EndThreadFunction NULL
#include <SDL3/SDL.h>
#if defined(__SSE2__)
#include <xmmintrin.h>
#endif

#include "p4pc/os.h"
#include "p4gl_gl.h"
#include "p4gl_rt.h"
#include "p4gl_win.h"

#define GL_RED_ 0x1903u
#define GL_UNPACK_SKIP_ROWS_ 0x0CF3u
#define GL_UNPACK_SKIP_PIXELS_ 0x0CF4u
#define GL_UNPACK_IMAGE_HEIGHT_ 0x806Eu
#define GL_UNPACK_SKIP_IMAGES_ 0x806Du
#define GL_MAX_TEXTURE_SIZE_ 0x0D33u
#define GL_MAX_RENDERBUFFER_SIZE_ 0x84E8u
#define GL_FRAMEBUFFER_BINDING_ 0x8CA6u
#define GL_READ_FRAMEBUFFER_BINDING_ 0x8CAAu

/* The float state of the thread that starts the render thread (the game thread: the EE mode, MXCSR round toward zero with flush and denormals-are-zero and the
 * x87 control word 0x037F, or FPCR on AArch64; hostmath.h). The new thread takes it over, so a driver such as llvmpipe rounds as it does when the game thread
 * runs it (the scene gates' pixels follow). A new thread would start in the OS default. */
static struct {
    int valid;
#if defined(__SSE2__)
    unsigned csr;
    unsigned short cw;
#elif defined(__aarch64__)
    uint64_t fpcr;
#endif
} sFloat;

static void float_capture(void)
{
#if defined(__SSE2__)
    sFloat.csr = _mm_getcsr();
    __asm__ volatile("fnstcw %0" : "=m"(sFloat.cw));
    sFloat.valid = 1;
#elif defined(__aarch64__)
    __asm__ volatile("mrs %0, fpcr" : "=r"(sFloat.fpcr));
    sFloat.valid = 1;
#endif
}

static void float_adopt(void)
{
    if (!sFloat.valid) {
        return;
    }
#if defined(__SSE2__)
    _mm_setcsr(sFloat.csr);
    __asm__ volatile("fldcw %0" : : "m"(sFloat.cw));
#elif defined(__aarch64__)
    __asm__ volatile("msr fpcr, %0" : : "r"(sFloat.fpcr));
#endif
}

#define RT_NOINLINE __attribute__((noinline))

enum { RT_CHUNK_BYTES = 256 * 1024, RT_SUBMIT_BYTES = 48 * 1024, RT_CHUNKS = 12, RT_RPC_WORDS = 10 };

typedef struct Chunk {
    struct Chunk *next;
    uint8_t *data;
    size_t used, cap;
    int oversize, frameEnd;
} Chunk;

/* The driver's entry points, kept when the recorders replace them in the table. */
#define RT_NAMES(X) \
    X(GetString) X(GetStringi) X(GetIntegerv) X(GenTextures) X(DeleteTextures) X(BindTexture) X(TexImage2D) X(CopyTexSubImage2D) X(ActiveTexture) \
    X(TexSubImage2D) X(TexParameteri) X(PixelStorei) X(ClearColor) X(Clear) X(Flush) X(Viewport) X(GetError) X(Enable) X(Disable) X(BlendFunc) \
    X(GenVertexArrays) X(BindVertexArray) X(GenBuffers) X(BindBuffer) X(BufferData) X(EnableVertexAttribArray) X(VertexAttribPointer) \
    X(VertexAttribDivisor) X(CreateShader) X(DeleteShader) X(ShaderSource) X(CompileShader) X(GetShaderiv) X(GetShaderInfoLog) X(CreateProgram) \
    X(DeleteProgram) X(AttachShader) X(LinkProgram) X(GetProgramiv) X(UseProgram) X(GetUniformLocation) X(Uniform2f) X(Uniform1i) X(Uniform1f) \
    X(DrawArrays) X(DrawArraysInstanced) X(DrawElements) X(DeleteBuffers) X(DepthFunc) X(DepthMask) X(CullFace) X(BlitFramebuffer) X(FrontFace) \
    X(ColorMask) X(BlendEquation) X(BlendFuncSeparate) X(BlendColor) X(Uniform3f) X(Uniform3fv) X(Uniform4f) X(Uniform4fv) X(UniformMatrix3fv) \
    X(UniformMatrix4fv) X(GenFramebuffers) X(BindFramebuffer) X(FramebufferTexture2D) X(CheckFramebufferStatus) X(ReadPixels) X(GenRenderbuffers) \
    X(DeleteFramebuffers) X(DeleteRenderbuffers) X(BindRenderbuffer) X(RenderbufferStorage) X(FramebufferRenderbuffer)

static struct {
#define X(n) __typeof__(p4gl_gl##n) n;
    RT_NAMES(X)
#undef X
} R;

static inline uint64_t wi(int64_t v) { return (uint64_t)v; }
static inline uint64_t wf(float v) { uint32_t b; memcpy(&b, &v, sizeof b); return b; }
static inline float fw(uint64_t w) { uint32_t b = (uint32_t)w; float v; memcpy(&v, &b, sizeof v); return v; }
#define UNPAREN(...) __VA_ARGS__
#define W(x) _Generic((x), float: wf(x), default: wi(x))

/* Argument-only calls: name, the recorder's parameters, the call's arguments, the words to record and the arguments rebuilt from them. */
#define RT_SCALAR(X) \
    X(ActiveTexture, (GLenum a0), (a0), (W(a0)), ((GLenum)w[0])) \
    X(BindTexture, (GLenum a0, GLuint a1), (a0, a1), (W(a0), W(a1)), ((GLenum)w[0], (GLuint)w[1])) \
    X(CopyTexSubImage2D, (GLenum a0, GLint a1, GLint a2, GLint a3, GLint a4, GLint a5, GLsizei a6, GLsizei a7), (a0, a1, a2, a3, a4, a5, a6, a7), \
      (W(a0), W(a1), W(a2), W(a3), W(a4), W(a5), W(a6), W(a7)), \
      ((GLenum)w[0], (GLint)w[1], (GLint)w[2], (GLint)w[3], (GLint)w[4], (GLint)w[5], (GLsizei)w[6], (GLsizei)w[7])) \
    X(TexParameteri, (GLenum a0, GLenum a1, GLint a2), (a0, a1, a2), (W(a0), W(a1), W(a2)), ((GLenum)w[0], (GLenum)w[1], (GLint)w[2])) \
    X(ClearColor, (GLfloat a0, GLfloat a1, GLfloat a2, GLfloat a3), (a0, a1, a2, a3), (W(a0), W(a1), W(a2), W(a3)), (fw(w[0]), fw(w[1]), fw(w[2]), fw(w[3]))) \
    X(Clear, (GLbitfield a0), (a0), (W(a0)), ((GLbitfield)w[0])) \
    X(Flush, (void), (), (0), ()) \
    X(Viewport, (GLint a0, GLint a1, GLsizei a2, GLsizei a3), (a0, a1, a2, a3), (W(a0), W(a1), W(a2), W(a3)), \
      ((GLint)w[0], (GLint)w[1], (GLsizei)w[2], (GLsizei)w[3])) \
    X(Enable, (GLenum a0), (a0), (W(a0)), ((GLenum)w[0])) \
    X(Disable, (GLenum a0), (a0), (W(a0)), ((GLenum)w[0])) \
    X(BlendFunc, (GLenum a0, GLenum a1), (a0, a1), (W(a0), W(a1)), ((GLenum)w[0], (GLenum)w[1])) \
    X(BindVertexArray, (GLuint a0), (a0), (W(a0)), ((GLuint)w[0])) \
    X(BindBuffer, (GLenum a0, GLuint a1), (a0, a1), (W(a0), W(a1)), ((GLenum)w[0], (GLuint)w[1])) \
    X(EnableVertexAttribArray, (GLuint a0), (a0), (W(a0)), ((GLuint)w[0])) \
    X(VertexAttribPointer, (GLuint a0, GLint a1, GLenum a2, GLboolean a3, GLsizei a4, const void *a5), (a0, a1, a2, a3, a4, a5), \
      (W(a0), W(a1), W(a2), W(a3), W(a4), W((uintptr_t)a5)), \
      ((GLuint)w[0], (GLint)w[1], (GLenum)w[2], (GLboolean)w[3], (GLsizei)w[4], (const void *)(uintptr_t)w[5])) \
    X(VertexAttribDivisor, (GLuint a0, GLuint a1), (a0, a1), (W(a0), W(a1)), ((GLuint)w[0], (GLuint)w[1])) \
    X(DeleteShader, (GLuint a0), (a0), (W(a0)), ((GLuint)w[0])) \
    X(CompileShader, (GLuint a0), (a0), (W(a0)), ((GLuint)w[0])) \
    X(DeleteProgram, (GLuint a0), (a0), (W(a0)), ((GLuint)w[0])) \
    X(AttachShader, (GLuint a0, GLuint a1), (a0, a1), (W(a0), W(a1)), ((GLuint)w[0], (GLuint)w[1])) \
    X(LinkProgram, (GLuint a0), (a0), (W(a0)), ((GLuint)w[0])) \
    X(UseProgram, (GLuint a0), (a0), (W(a0)), ((GLuint)w[0])) \
    X(Uniform2f, (GLint a0, GLfloat a1, GLfloat a2), (a0, a1, a2), (W(a0), W(a1), W(a2)), ((GLint)w[0], fw(w[1]), fw(w[2]))) \
    X(Uniform1i, (GLint a0, GLint a1), (a0, a1), (W(a0), W(a1)), ((GLint)w[0], (GLint)w[1])) \
    X(Uniform1f, (GLint a0, GLfloat a1), (a0, a1), (W(a0), W(a1)), ((GLint)w[0], fw(w[1]))) \
    X(Uniform3f, (GLint a0, GLfloat a1, GLfloat a2, GLfloat a3), (a0, a1, a2, a3), (W(a0), W(a1), W(a2), W(a3)), \
      ((GLint)w[0], fw(w[1]), fw(w[2]), fw(w[3]))) \
    X(Uniform4f, (GLint a0, GLfloat a1, GLfloat a2, GLfloat a3, GLfloat a4), (a0, a1, a2, a3, a4), (W(a0), W(a1), W(a2), W(a3), W(a4)), \
      ((GLint)w[0], fw(w[1]), fw(w[2]), fw(w[3]), fw(w[4]))) \
    X(DrawArrays, (GLenum a0, GLint a1, GLsizei a2), (a0, a1, a2), (W(a0), W(a1), W(a2)), ((GLenum)w[0], (GLint)w[1], (GLsizei)w[2])) \
    X(DrawArraysInstanced, (GLenum a0, GLint a1, GLsizei a2, GLsizei a3), (a0, a1, a2, a3), (W(a0), W(a1), W(a2), W(a3)), \
      ((GLenum)w[0], (GLint)w[1], (GLsizei)w[2], (GLsizei)w[3])) \
    X(DrawElements, (GLenum a0, GLsizei a1, GLenum a2, const void *a3), (a0, a1, a2, a3), (W(a0), W(a1), W(a2), W((uintptr_t)a3)), \
      ((GLenum)w[0], (GLsizei)w[1], (GLenum)w[2], (const void *)(uintptr_t)w[3])) \
    X(FramebufferTexture2D, (GLenum a0, GLenum a1, GLenum a2, GLuint a3, GLint a4), (a0, a1, a2, a3, a4), (W(a0), W(a1), W(a2), W(a3), W(a4)), \
      ((GLenum)w[0], (GLenum)w[1], (GLenum)w[2], (GLuint)w[3], (GLint)w[4])) \
    X(BindRenderbuffer, (GLenum a0, GLuint a1), (a0, a1), (W(a0), W(a1)), ((GLenum)w[0], (GLuint)w[1])) \
    X(RenderbufferStorage, (GLenum a0, GLenum a1, GLsizei a2, GLsizei a3), (a0, a1, a2, a3), (W(a0), W(a1), W(a2), W(a3)), \
      ((GLenum)w[0], (GLenum)w[1], (GLsizei)w[2], (GLsizei)w[3])) \
    X(FramebufferRenderbuffer, (GLenum a0, GLenum a1, GLenum a2, GLuint a3), (a0, a1, a2, a3), (W(a0), W(a1), W(a2), W(a3)), \
      ((GLenum)w[0], (GLenum)w[1], (GLenum)w[2], (GLuint)w[3])) \
    X(DepthFunc, (GLenum a0), (a0), (W(a0)), ((GLenum)w[0])) \
    X(DepthMask, (GLboolean a0), (a0), (W(a0)), ((GLboolean)w[0])) \
    X(CullFace, (GLenum a0), (a0), (W(a0)), ((GLenum)w[0])) \
    X(BlitFramebuffer, (GLint a0, GLint a1, GLint a2, GLint a3, GLint a4, GLint a5, GLint a6, GLint a7, GLbitfield a8, GLenum a9), \
      (a0, a1, a2, a3, a4, a5, a6, a7, a8, a9), (W(a0), W(a1), W(a2), W(a3), W(a4), W(a5), W(a6), W(a7), W(a8), W(a9)), \
      ((GLint)w[0], (GLint)w[1], (GLint)w[2], (GLint)w[3], (GLint)w[4], (GLint)w[5], (GLint)w[6], (GLint)w[7], (GLbitfield)w[8], (GLenum)w[9])) \
    X(FrontFace, (GLenum a0), (a0), (W(a0)), ((GLenum)w[0])) \
    X(ColorMask, (GLboolean a0, GLboolean a1, GLboolean a2, GLboolean a3), (a0, a1, a2, a3), (W(a0), W(a1), W(a2), W(a3)), \
      ((GLboolean)w[0], (GLboolean)w[1], (GLboolean)w[2], (GLboolean)w[3])) \
    X(BlendEquation, (GLenum a0), (a0), (W(a0)), ((GLenum)w[0])) \
    X(BlendFuncSeparate, (GLenum a0, GLenum a1, GLenum a2, GLenum a3), (a0, a1, a2, a3), (W(a0), W(a1), W(a2), W(a3)), \
      ((GLenum)w[0], (GLenum)w[1], (GLenum)w[2], (GLenum)w[3])) \
    X(BlendColor, (GLfloat a0, GLfloat a1, GLfloat a2, GLfloat a3), (a0, a1, a2, a3), (W(a0), W(a1), W(a2), W(a3)), \
      (fw(w[0]), fw(w[1]), fw(w[2]), fw(w[3])))

enum {
    OP_NONE,
#define X(n, p, c, wl, u) OP_##n,
    RT_SCALAR(X)
#undef X
    OP_TEXIMAGE, OP_TEXSUBIMAGE, OP_BUFFERDATA, OP_UNIFORMV, OP_DELETE, OP_REFILL, OP_SWAP, OP_SWAPINT, OP_RPC, OP_PIXELSTORE, OP_BINDFRAMEBUFFER, OP_REBIND
};

/* Calls that wait for the render thread. The caller's stack holds the arguments and the result. */
enum {
    RPC_GETSTRING, RPC_GETSTRINGI, RPC_GETINTEGERV, RPC_GETERROR, RPC_CREATESHADER, RPC_CREATEPROGRAM, RPC_SHADERSOURCE, RPC_GETSHADERIV,
    RPC_GETSHADERINFOLOG, RPC_GETPROGRAMIV, RPC_GETUNIFORMLOCATION, RPC_CHECKFRAMEBUFFER, RPC_READPIXELS, RPC_GEN, RPC_TEXIMAGE, RPC_TEXSUBIMAGE,
    RPC_BUFFERDATA, RPC_NOP
};
enum { K_TEX, K_BUF, K_VAO, K_FBO, K_RBO, K_N };
enum { DEL_TEX, DEL_BUF, DEL_FBO, DEL_RBO };

typedef struct RpcCall {
    int op;
    uint64_t a[RT_RPC_WORDS];
    uint64_t ret;
    SDL_AtomicInt done;
} RpcCall;

static struct {
    SDL_Mutex *mu;
    SDL_Condition *cv; /* work queued, chunk freed, frame or call done: one condition, every wait re-checks its own predicate */
    SDL_Thread *thread;
    Chunk *qHead, *qTail, *freeList, *cur;
    volatile int quit, started, startOk, lost;
    uint64_t framesSubmitted, framesDone;
    unsigned depth;
    SDL_Window *window;
    P4GlContext context;
    int headless, installed, on, exitHook;
    int defaultOn;
    int stats;
    /* names the render thread generated ahead (glGen*), one pool per kind; guarded by mu */
    struct { GLuint name[512]; int head, count, target, pending; } pool[K_N];
    /* what the game thread believes the context holds, for glGetIntegerv without a round trip */
    GLint readFb, drawFb, maxTex, maxRb, unpackAlign, unpackRow;
    int unpackSkip;
    unsigned long long records, rpcs, chunks, poolMisses, swaps, swapIntervals;
    uint64_t rpcWaitNs, fenceWaitNs, renderBusyNs, swapNs, bytes, freeWaitNs;
    unsigned statFrames;
    unsigned long long headlessSwaps;
    unsigned long long rpcOps[RPC_NOP + 1];
    /* P4PC_RENDER_THREAD_STATS: what each kind of record costs the driver, and how long the draws run without a state change */
    unsigned long long opCount[128];
    uint64_t opNs[128];
    uint64_t opBytes[128];
    unsigned long long drawsSeen, runsSeen, breakCause[128];
    volatile uint64_t rtCpuNs;         /* the render thread's own CPU time so far (P4PC_RENDER_THREAD_STATS; set after every chunk) */
    uint64_t statGameCpu0, statRtCpu0;
    int merge;                         /* P4PC_RT_MERGE: runs of immediate-mode draws are merged (merge_draw_run) */
    unsigned long long mergedCalls, mergedRuns; /* driver calls the merging saved, runs merged, since the last statistics line */
    int runDirty, runCause;
    unsigned long long dumpAt;
} S;

static const char *const kOpName[] = {
    "none",
#define X(n, p, c, wl, u) #n,
    RT_SCALAR(X)
#undef X
    "TexImage", "TexSubImage", "BufferData", "UniformV", "Delete", "Refill", "Swap", "SwapInt", "Rpc", "PixelStore", "BindFramebuffer", "Rebind"
};

void p4gl_rt_set_default(int on) { S.defaultOn = on != 0; }
int p4gl_rt_active(void) { return S.on; }
unsigned long long p4gl_rt_frames_done(void) { return S.framesDone; }

void p4gl_rt_counters(unsigned long long out[6])
{
    out[0] = S.records;
    out[1] = S.rpcs;
    out[2] = S.chunks;
    out[3] = S.poolMisses;
    out[4] = S.headless ? S.headlessSwaps : S.swaps;
    out[5] = S.swapIntervals;
}

static int env_is(const char *name, char c)
{
    const char *v = SDL_getenv(name);
    return v != NULL && v[0] == c;
}

static int env_set(const char *name)
{
    const char *v = SDL_getenv(name);
    return v != NULL && v[0] != '\0';
}

RT_NOINLINE int p4gl_rt_wanted(void)
{
    const char *v = SDL_getenv("P4PC_RENDER_THREAD");
    static const char *const kBlockers[] = {"P4PC_GPU_TRACE", "P4PC_GL_HITCH_TRACE", "P4PC_GLSTATS", "P4PC_GLERR", "P4PC_GLFINISH"};
    unsigned i;

    if (v != NULL && v[0] == '0') {
        return 0;
    }
    if (v == NULL || v[0] == '\0') {
        if (!S.defaultOn) {
            return 0;
        }
    }
    for (i = 0; i < sizeof kBlockers / sizeof kBlockers[0]; i++) {
        if (env_set(kBlockers[i])) {
            fprintf(stderr, "p4gl: render thread off: %s calls the driver from the game thread\n", kBlockers[i]);
            return 0;
        }
    }
    return 1;
}

/* ---- chunk queue ---- */

static Chunk *chunk_new(size_t cap, int oversize)
{
    Chunk *c = (Chunk *)calloc(1, sizeof *c);

    if (c != NULL) {
        c->data = (uint8_t *)malloc(cap);
        if (c->data == NULL) {
            free(c);
            return NULL;
        }
        c->cap = cap;
        c->oversize = oversize;
    }
    return c;
}

static void chunk_free(Chunk *c)
{
    free(c->data);
    free(c);
}

/* The standard chunks come from a list the render thread refills as it finishes them; waits (and says so after 5 s) when the thread is that far behind. */
static RT_NOINLINE Chunk *chunk_acquire(size_t need)
{
    Chunk *c;

    if (need > RT_CHUNK_BYTES) {
        c = chunk_new(need, 1);
        if (c == NULL) {
            fprintf(stderr, "p4gl: render thread: out of memory for a %zu byte record\n", need);
            exit(1);
        }
        return c;
    }
    SDL_LockMutex(S.mu);
    if (S.freeList == NULL) {
        const uint64_t t0 = SDL_GetTicksNS();
        int warned = 0;

        while (S.freeList == NULL) {
            SDL_WaitConditionTimeout(S.cv, S.mu, 100);
            if (!warned && SDL_GetTicksNS() - t0 > 5000000000ull) {
                warned = 1;
                fprintf(stderr, "p4gl: render thread: waiting 5 s for the driver to finish a chunk of commands\n");
            }
        }
        S.freeWaitNs += SDL_GetTicksNS() - t0;
    }
    c = S.freeList;
    S.freeList = c->next;
    SDL_UnlockMutex(S.mu);
    c->next = NULL;
    c->used = 0;
    c->frameEnd = 0;
    return c;
}

/* Queues the chunk being filled. A frame end always queues a chunk (empty if need be) so the frame fence has something to count. */
static RT_NOINLINE void submit_cur(int frameEnd)
{
    Chunk *c = S.cur;

    if (c == NULL && !frameEnd) {
        return;
    }
    if (c == NULL) {
        c = chunk_acquire(1);
    }
    S.cur = NULL;
    c->frameEnd = frameEnd;
    SDL_LockMutex(S.mu);
    c->next = NULL;
    if (S.qTail != NULL) {
        S.qTail->next = c;
    } else {
        S.qHead = c;
    }
    S.qTail = c;
    S.chunks++;
    S.bytes += c->used;
    if (frameEnd) {
        S.framesSubmitted++;
    }
    SDL_SignalCondition(S.cv);
    SDL_UnlockMutex(S.mu);
}

static void rec(unsigned op, const uint64_t *words, unsigned nw, const void *payload, size_t bytes, size_t reserve)
{
    const size_t padded = (reserve + 7u) & ~(size_t)7u;
    const size_t total = 8u + (size_t)nw * 8u + padded;
    uint8_t *p;
    uint64_t hdr;

    if (S.cur != NULL && S.cur->cap - S.cur->used < total) {
        submit_cur(0);
    }
    if (S.cur == NULL) {
        S.cur = chunk_acquire(total);
    }
    p = S.cur->data + S.cur->used;
    hdr = (uint64_t)op | ((uint64_t)nw << 16) | ((uint64_t)reserve << 32);
    memcpy(p, &hdr, 8);
    if (nw != 0) {
        memcpy(p + 8, words, (size_t)nw * 8u);
    }
    if (payload != NULL && bytes != 0) {
        memcpy(p + 8 + (size_t)nw * 8u, payload, bytes);
    }
    if (reserve > bytes) {
        memset(p + 8 + (size_t)nw * 8u + bytes, 0, padded - bytes);
    }
    S.cur->used += total;
    S.records++;
    if (S.cur->used >= RT_SUBMIT_BYTES) {
        submit_cur(0);
    }
}

#define REC(op, ...) do { const uint64_t w_[] = {__VA_ARGS__}; rec((op), w_, (unsigned)(sizeof w_ / sizeof w_[0]), NULL, 0, 0); } while (0)

/* ---- calls that wait ---- */

static RT_NOINLINE uint64_t rpc(RpcCall *c)
{
    const uint64_t w[1] = {(uint64_t)(uintptr_t)c};
    const uint64_t t0 = SDL_GetTicksNS();

    SDL_SetAtomicInt(&c->done, 0);
    rec(OP_RPC, w, 1, NULL, 0, 0);
    submit_cur(0);
    S.rpcs++;
    S.rpcOps[(unsigned)c->op <= RPC_NOP ? (unsigned)c->op : RPC_NOP]++;
    if (!SDL_GetAtomicInt(&c->done)) {
        int warned = 0;

        SDL_LockMutex(S.mu);
        while (!SDL_GetAtomicInt(&c->done)) {
            SDL_WaitConditionTimeout(S.cv, S.mu, 100);
            if (!warned && SDL_GetTicksNS() - t0 > 5000000000ull) {
                warned = 1;
                fprintf(stderr, "p4gl: render thread: waiting 5 s for a driver call to return\n");
            }
        }
        SDL_UnlockMutex(S.mu);
    }
    S.rpcWaitNs += SDL_GetTicksNS() - t0;
    return c->ret;
}

#define RPC_BEGIN(opcode) RpcCall c_; memset(&c_, 0, sizeof c_); c_.op = (opcode)

static int unpack_bytes(GLenum format, GLenum type, GLsizei w, GLsizei h, size_t *out)
{
    size_t bpp, rowPx, stride;

    if (type != GL_UNSIGNED_BYTE || S.unpackSkip || w < 0 || h < 0) {
        return 0;
    }
    bpp = format == GL_RGBA ? 4u : format == GL_RED_ ? 1u : 0u;
    if (bpp == 0) {
        return 0;
    }
    rowPx = S.unpackRow > 0 ? (size_t)S.unpackRow : (size_t)w;
    stride = (rowPx * bpp + (size_t)S.unpackAlign - 1u) / (size_t)S.unpackAlign * (size_t)S.unpackAlign;
    *out = (w == 0 || h == 0) ? 0u : stride * (size_t)(h - 1) + (size_t)w * bpp;
    return 1;
}

/* ---- recorders ---- */

#define X(n, params, call, wl, unp) \
    static void P4GL_APIENTRY rt_##n params \
    { \
        if (!S.on) { \
            R.n call; \
            return; \
        } \
        { \
            const uint64_t w_[] = {UNPAREN wl}; \
            rec(OP_##n, w_, (unsigned)(sizeof w_ / sizeof w_[0]), NULL, 0, 0); \
        } \
    }
RT_SCALAR(X)
#undef X

static void P4GL_APIENTRY rt_BindFramebuffer(GLenum target, GLuint fb)
{
    if (!S.on) {
        R.BindFramebuffer(target, fb);
        return;
    }
    if (target == GL_FRAMEBUFFER || target == GL_READ_FRAMEBUFFER) {
        S.readFb = (GLint)fb;
    }
    if (target == GL_FRAMEBUFFER || target == GL_DRAW_FRAMEBUFFER) {
        S.drawFb = (GLint)fb;
    }
    REC(OP_BINDFRAMEBUFFER, target, fb);
}

static void P4GL_APIENTRY rt_PixelStorei(GLenum pname, GLint param)
{
    if (!S.on) {
        R.PixelStorei(pname, param);
        return;
    }
    if (pname == GL_UNPACK_ALIGNMENT) {
        S.unpackAlign = param > 0 ? param : 4;
    } else if (pname == GL_UNPACK_ROW_LENGTH) {
        S.unpackRow = param;
    } else if (pname == GL_UNPACK_SKIP_ROWS_ || pname == GL_UNPACK_SKIP_PIXELS_ || pname == GL_UNPACK_IMAGE_HEIGHT_ || pname == GL_UNPACK_SKIP_IMAGES_) {
        S.unpackSkip = param != 0; /* the size of an upload is then not the one computed here: uploads wait for the thread */
    }
    REC(OP_PIXELSTORE, pname, wi(param));
}

static void P4GL_APIENTRY rt_TexImage2D(GLenum t, GLint l, GLint ifmt, GLsizei w, GLsizei h, GLint b, GLenum f, GLenum ty, const void *px)
{
    size_t bytes = 0;

    if (!S.on) {
        R.TexImage2D(t, l, ifmt, w, h, b, f, ty, px);
        return;
    }
    if (px != NULL && !unpack_bytes(f, ty, w, h, &bytes)) {
        RPC_BEGIN(RPC_TEXIMAGE);
        c_.a[0] = t; c_.a[1] = wi(l); c_.a[2] = wi(ifmt); c_.a[3] = wi(w); c_.a[4] = wi(h); c_.a[5] = wi(b); c_.a[6] = f; c_.a[7] = ty;
        c_.a[8] = (uint64_t)(uintptr_t)px;
        rpc(&c_);
        return;
    }
    {
        const uint64_t wd[9] = {t, wi(l), wi(ifmt), wi(w), wi(h), wi(b), f, ty, px != NULL};
        rec(OP_TEXIMAGE, wd, 9, px, bytes, px != NULL ? bytes + 8u : 0);
    }
}

static void P4GL_APIENTRY rt_TexSubImage2D(GLenum t, GLint l, GLint x, GLint y, GLsizei w, GLsizei h, GLenum f, GLenum ty, const void *px)
{
    size_t bytes = 0;

    if (!S.on) {
        R.TexSubImage2D(t, l, x, y, w, h, f, ty, px);
        return;
    }
    if (px != NULL && !unpack_bytes(f, ty, w, h, &bytes)) {
        RPC_BEGIN(RPC_TEXSUBIMAGE);
        c_.a[0] = t; c_.a[1] = wi(l); c_.a[2] = wi(x); c_.a[3] = wi(y); c_.a[4] = wi(w); c_.a[5] = wi(h); c_.a[6] = f; c_.a[7] = ty;
        c_.a[8] = (uint64_t)(uintptr_t)px;
        rpc(&c_);
        return;
    }
    {
        const uint64_t wd[9] = {t, wi(l), wi(x), wi(y), wi(w), wi(h), f, ty, px != NULL};
        rec(OP_TEXSUBIMAGE, wd, 9, px, bytes, px != NULL ? bytes + 8u : 0);
    }
}

static void P4GL_APIENTRY rt_BufferData(GLenum target, GLsizeiptr size, const void *data, GLenum usage)
{
    if (!S.on) {
        R.BufferData(target, size, data, usage);
        return;
    }
    if (size < 0) {
        RPC_BEGIN(RPC_BUFFERDATA);
        c_.a[0] = target; c_.a[1] = wi(size); c_.a[2] = (uint64_t)(uintptr_t)data; c_.a[3] = usage;
        rpc(&c_);
        return;
    }
    {
        const uint64_t wd[4] = {target, wi(size), usage, data != NULL};
        rec(OP_BUFFERDATA, wd, 4, data, data != NULL ? (size_t)size : 0, data != NULL ? (size_t)size : 0);
    }
}

static RT_NOINLINE void record_uniformv(unsigned kind, GLint loc, GLsizei count, GLboolean transpose, const GLfloat *v, unsigned perItem)
{
    const size_t bytes = count > 0 ? (size_t)count * perItem * sizeof(GLfloat) : 0;
    const uint64_t wd[4] = {kind, wi(loc), wi(count), transpose};

    rec(OP_UNIFORMV, wd, 4, v, bytes, bytes);
}

static void P4GL_APIENTRY rt_Uniform3fv(GLint loc, GLsizei count, const GLfloat *v)
{
    if (!S.on) {
        R.Uniform3fv(loc, count, v);
        return;
    }
    record_uniformv(0, loc, count, 0, v, 3);
}

static void P4GL_APIENTRY rt_Uniform4fv(GLint loc, GLsizei count, const GLfloat *v)
{
    if (!S.on) {
        R.Uniform4fv(loc, count, v);
        return;
    }
    record_uniformv(3, loc, count, 0, v, 4);
}

static void P4GL_APIENTRY rt_UniformMatrix3fv(GLint loc, GLsizei count, GLboolean transpose, const GLfloat *v)
{
    if (!S.on) {
        R.UniformMatrix3fv(loc, count, transpose, v);
        return;
    }
    record_uniformv(1, loc, count, transpose, v, 9);
}

static void P4GL_APIENTRY rt_UniformMatrix4fv(GLint loc, GLsizei count, GLboolean transpose, const GLfloat *v)
{
    if (!S.on) {
        R.UniformMatrix4fv(loc, count, transpose, v);
        return;
    }
    record_uniformv(2, loc, count, transpose, v, 16);
}

static RT_NOINLINE void record_delete(unsigned kind, GLsizei n, const GLuint *names)
{
    const uint64_t wd[2] = {kind, wi(n)};

    if (n > 0 && names != NULL) {
        rec(OP_DELETE, wd, 2, names, (size_t)n * sizeof(GLuint), (size_t)n * sizeof(GLuint));
    }
}

static void P4GL_APIENTRY rt_DeleteTextures(GLsizei n, const GLuint *names)
{
    if (!S.on) {
        R.DeleteTextures(n, names);
        return;
    }
    record_delete(DEL_TEX, n, names);
}

static void P4GL_APIENTRY rt_DeleteBuffers(GLsizei n, const GLuint *names)
{
    if (!S.on) {
        R.DeleteBuffers(n, names);
        return;
    }
    record_delete(DEL_BUF, n, names);
}

static void P4GL_APIENTRY rt_DeleteFramebuffers(GLsizei n, const GLuint *names)
{
    GLsizei i;

    if (!S.on) {
        R.DeleteFramebuffers(n, names);
        return;
    }
    for (i = 0; i < n; i++) { /* deleting the bound framebuffer binds the default one */
        if ((GLint)names[i] == S.readFb) {
            S.readFb = 0;
        }
        if ((GLint)names[i] == S.drawFb) {
            S.drawFb = 0;
        }
    }
    record_delete(DEL_FBO, n, names);
}

static void P4GL_APIENTRY rt_DeleteRenderbuffers(GLsizei n, const GLuint *names)
{
    if (!S.on) {
        R.DeleteRenderbuffers(n, names);
        return;
    }
    record_delete(DEL_RBO, n, names);
}

/* glGen*: names come from the pool the render thread keeps filled (real driver names), so creating an object never waits for the driver. */
static RT_NOINLINE void gen_names(int kind, GLsizei n, GLuint *out)
{
    int refill = 0;
    GLsizei i;

    SDL_LockMutex(S.mu);
    if (n > 0 && S.pool[kind].count >= n) {
        for (i = 0; i < n; i++) {
            out[i] = S.pool[kind].name[S.pool[kind].head++];
        }
        S.pool[kind].count -= n;
        if (S.pool[kind].count < S.pool[kind].target / 2 && !S.pool[kind].pending) {
            S.pool[kind].pending = refill = 1;
        }
        SDL_UnlockMutex(S.mu);
        if (refill) {
            REC(OP_REFILL, (uint64_t)kind);
            submit_cur(0); /* the thread refills while the game goes on: the record must not wait for the chunk to fill */
        }
        return;
    }
    S.poolMisses++;
    SDL_UnlockMutex(S.mu);
    {
        RPC_BEGIN(RPC_GEN);
        c_.a[0] = (uint64_t)kind; c_.a[1] = wi(n); c_.a[2] = (uint64_t)(uintptr_t)out;
        rpc(&c_);
    }
}

static void P4GL_APIENTRY rt_GenTextures(GLsizei n, GLuint *out) { if (!S.on) { R.GenTextures(n, out); return; } gen_names(K_TEX, n, out); }
static void P4GL_APIENTRY rt_GenBuffers(GLsizei n, GLuint *out) { if (!S.on) { R.GenBuffers(n, out); return; } gen_names(K_BUF, n, out); }
static void P4GL_APIENTRY rt_GenVertexArrays(GLsizei n, GLuint *out) { if (!S.on) { R.GenVertexArrays(n, out); return; } gen_names(K_VAO, n, out); }
static void P4GL_APIENTRY rt_GenFramebuffers(GLsizei n, GLuint *out) { if (!S.on) { R.GenFramebuffers(n, out); return; } gen_names(K_FBO, n, out); }
static void P4GL_APIENTRY rt_GenRenderbuffers(GLsizei n, GLuint *out) { if (!S.on) { R.GenRenderbuffers(n, out); return; } gen_names(K_RBO, n, out); }

static const GLubyte *P4GL_APIENTRY rt_GetString(GLenum name)
{
    if (!S.on) {
        return R.GetString(name);
    }
    {
        RPC_BEGIN(RPC_GETSTRING);
        c_.a[0] = name;
        return (const GLubyte *)(uintptr_t)rpc(&c_);
    }
}

static const GLubyte *P4GL_APIENTRY rt_GetStringi(GLenum name, GLuint index)
{
    if (!S.on) {
        return R.GetStringi(name, index);
    }
    {
        RPC_BEGIN(RPC_GETSTRINGI);
        c_.a[0] = name; c_.a[1] = index;
        return (const GLubyte *)(uintptr_t)rpc(&c_);
    }
}

static void P4GL_APIENTRY rt_GetIntegerv(GLenum pname, GLint *data)
{
    if (!S.on) {
        R.GetIntegerv(pname, data);
        return;
    }
    switch (pname) { /* what the game asks every frame, or that never changes: no round trip */
    case GL_MAX_TEXTURE_SIZE_: *data = S.maxTex; return;
    case GL_MAX_RENDERBUFFER_SIZE_: *data = S.maxRb; return;
    case GL_FRAMEBUFFER_BINDING_: *data = S.drawFb; return;
    case GL_READ_FRAMEBUFFER_BINDING_: *data = S.readFb; return;
    default: break;
    }
    {
        RPC_BEGIN(RPC_GETINTEGERV);
        c_.a[0] = pname; c_.a[1] = (uint64_t)(uintptr_t)data;
        rpc(&c_);
    }
}

static GLenum P4GL_APIENTRY rt_GetError(void)
{
    if (!S.on) {
        return R.GetError();
    }
    {
        RPC_BEGIN(RPC_GETERROR);
        return (GLenum)rpc(&c_);
    }
}

static GLuint P4GL_APIENTRY rt_CreateShader(GLenum type)
{
    if (!S.on) {
        return R.CreateShader(type);
    }
    {
        RPC_BEGIN(RPC_CREATESHADER);
        c_.a[0] = type;
        return (GLuint)rpc(&c_);
    }
}

static GLuint P4GL_APIENTRY rt_CreateProgram(void)
{
    if (!S.on) {
        return R.CreateProgram();
    }
    {
        RPC_BEGIN(RPC_CREATEPROGRAM);
        return (GLuint)rpc(&c_);
    }
}

static void P4GL_APIENTRY rt_ShaderSource(GLuint shader, GLsizei count, const GLchar *const *string, const GLint *length)
{
    if (!S.on) {
        R.ShaderSource(shader, count, string, length);
        return;
    }
    {
        RPC_BEGIN(RPC_SHADERSOURCE);
        c_.a[0] = shader; c_.a[1] = wi(count); c_.a[2] = (uint64_t)(uintptr_t)string; c_.a[3] = (uint64_t)(uintptr_t)length;
        rpc(&c_);
    }
}

static void P4GL_APIENTRY rt_GetShaderiv(GLuint shader, GLenum pname, GLint *params)
{
    if (!S.on) {
        R.GetShaderiv(shader, pname, params);
        return;
    }
    {
        RPC_BEGIN(RPC_GETSHADERIV);
        c_.a[0] = shader; c_.a[1] = pname; c_.a[2] = (uint64_t)(uintptr_t)params;
        rpc(&c_);
    }
}

static void P4GL_APIENTRY rt_GetShaderInfoLog(GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *infoLog)
{
    if (!S.on) {
        R.GetShaderInfoLog(shader, bufSize, length, infoLog);
        return;
    }
    {
        RPC_BEGIN(RPC_GETSHADERINFOLOG);
        c_.a[0] = shader; c_.a[1] = wi(bufSize); c_.a[2] = (uint64_t)(uintptr_t)length; c_.a[3] = (uint64_t)(uintptr_t)infoLog;
        rpc(&c_);
    }
}

static void P4GL_APIENTRY rt_GetProgramiv(GLuint program, GLenum pname, GLint *params)
{
    if (!S.on) {
        R.GetProgramiv(program, pname, params);
        return;
    }
    {
        RPC_BEGIN(RPC_GETPROGRAMIV);
        c_.a[0] = program; c_.a[1] = pname; c_.a[2] = (uint64_t)(uintptr_t)params;
        rpc(&c_);
    }
}

static GLint P4GL_APIENTRY rt_GetUniformLocation(GLuint program, const GLchar *name)
{
    if (!S.on) {
        return R.GetUniformLocation(program, name);
    }
    {
        RPC_BEGIN(RPC_GETUNIFORMLOCATION);
        c_.a[0] = program; c_.a[1] = (uint64_t)(uintptr_t)name;
        return (GLint)rpc(&c_);
    }
}

static GLenum P4GL_APIENTRY rt_CheckFramebufferStatus(GLenum target)
{
    if (!S.on) {
        return R.CheckFramebufferStatus(target);
    }
    {
        RPC_BEGIN(RPC_CHECKFRAMEBUFFER);
        c_.a[0] = target;
        return (GLenum)rpc(&c_);
    }
}

static void P4GL_APIENTRY rt_ReadPixels(GLint x, GLint y, GLsizei w, GLsizei h, GLenum format, GLenum type, void *pixels)
{
    if (!S.on) {
        R.ReadPixels(x, y, w, h, format, type, pixels);
        return;
    }
    {
        RPC_BEGIN(RPC_READPIXELS);
        c_.a[0] = wi(x); c_.a[1] = wi(y); c_.a[2] = wi(w); c_.a[3] = wi(h); c_.a[4] = format; c_.a[5] = type; c_.a[6] = (uint64_t)(uintptr_t)pixels;
        rpc(&c_);
    }
}

/* ---- the render thread ---- */

static RT_NOINLINE void pool_refill(int kind)
{
    GLuint names[64];
    int need;

    SDL_LockMutex(S.mu);
    need = S.pool[kind].target - S.pool[kind].count;
    SDL_UnlockMutex(S.mu);
    while (need > 0) {
        const int n = need > 64 ? 64 : need;

        switch (kind) {
        case K_TEX: R.GenTextures(n, names); break;
        case K_BUF: R.GenBuffers(n, names); break;
        case K_VAO: R.GenVertexArrays(n, names); break;
        case K_FBO: R.GenFramebuffers(n, names); break;
        default: R.GenRenderbuffers(n, names); break;
        }
        SDL_LockMutex(S.mu);
        if (S.pool[kind].head + S.pool[kind].count + n > (int)(sizeof S.pool[kind].name / sizeof S.pool[kind].name[0])) {
            memmove(S.pool[kind].name, S.pool[kind].name + S.pool[kind].head, (size_t)S.pool[kind].count * sizeof(GLuint));
            S.pool[kind].head = 0;
        }
        memcpy(S.pool[kind].name + S.pool[kind].head + S.pool[kind].count, names, (size_t)n * sizeof(GLuint));
        S.pool[kind].count += n;
        SDL_UnlockMutex(S.mu);
        need -= n;
    }
    SDL_LockMutex(S.mu);
    S.pool[kind].pending = 0;
    SDL_UnlockMutex(S.mu);
}

static RT_NOINLINE void rpc_exec(RpcCall *c)
{
    const uint64_t *a = c->a;

    c->ret = 0;
    if (!S.lost) {
        switch (c->op) {
        case RPC_GETSTRING: c->ret = (uint64_t)(uintptr_t)R.GetString((GLenum)a[0]); break;
        case RPC_GETSTRINGI: c->ret = (uint64_t)(uintptr_t)R.GetStringi((GLenum)a[0], (GLuint)a[1]); break;
        case RPC_GETINTEGERV: R.GetIntegerv((GLenum)a[0], (GLint *)(uintptr_t)a[1]); break;
        case RPC_GETERROR: c->ret = R.GetError(); break;
        case RPC_CREATESHADER: c->ret = R.CreateShader((GLenum)a[0]); break;
        case RPC_CREATEPROGRAM: c->ret = R.CreateProgram(); break;
        case RPC_SHADERSOURCE:
            R.ShaderSource((GLuint)a[0], (GLsizei)a[1], (const GLchar *const *)(uintptr_t)a[2], (const GLint *)(uintptr_t)a[3]);
            break;
        case RPC_GETSHADERIV: R.GetShaderiv((GLuint)a[0], (GLenum)a[1], (GLint *)(uintptr_t)a[2]); break;
        case RPC_GETSHADERINFOLOG:
            R.GetShaderInfoLog((GLuint)a[0], (GLsizei)a[1], (GLsizei *)(uintptr_t)a[2], (GLchar *)(uintptr_t)a[3]);
            break;
        case RPC_GETPROGRAMIV: R.GetProgramiv((GLuint)a[0], (GLenum)a[1], (GLint *)(uintptr_t)a[2]); break;
        case RPC_GETUNIFORMLOCATION: c->ret = (uint64_t)(uint32_t)R.GetUniformLocation((GLuint)a[0], (const GLchar *)(uintptr_t)a[1]); break;
        case RPC_CHECKFRAMEBUFFER: c->ret = R.CheckFramebufferStatus((GLenum)a[0]); break;
        case RPC_READPIXELS:
            R.ReadPixels((GLint)a[0], (GLint)a[1], (GLsizei)a[2], (GLsizei)a[3], (GLenum)a[4], (GLenum)a[5], (void *)(uintptr_t)a[6]);
            break;
        case RPC_GEN: {
            GLuint *out = (GLuint *)(uintptr_t)a[2];
            const GLsizei n = (GLsizei)a[1];

            switch ((int)a[0]) {
            case K_TEX: R.GenTextures(n, out); break;
            case K_BUF: R.GenBuffers(n, out); break;
            case K_VAO: R.GenVertexArrays(n, out); break;
            case K_FBO: R.GenFramebuffers(n, out); break;
            default: R.GenRenderbuffers(n, out); break;
            }
            break;
        }
        case RPC_TEXIMAGE:
            R.TexImage2D((GLenum)a[0], (GLint)a[1], (GLint)a[2], (GLsizei)a[3], (GLsizei)a[4], (GLint)a[5], (GLenum)a[6], (GLenum)a[7],
                         (const void *)(uintptr_t)a[8]);
            break;
        case RPC_TEXSUBIMAGE:
            R.TexSubImage2D((GLenum)a[0], (GLint)a[1], (GLint)a[2], (GLint)a[3], (GLsizei)a[4], (GLsizei)a[5], (GLenum)a[6], (GLenum)a[7],
                            (const void *)(uintptr_t)a[8]);
            break;
        case RPC_BUFFERDATA: R.BufferData((GLenum)a[0], (GLsizeiptr)(int64_t)a[1], (const void *)(uintptr_t)a[2], (GLenum)a[3]); break;
        default: break;
        }
    }
    SDL_LockMutex(S.mu);
    SDL_SetAtomicInt(&c->done, 1);
    SDL_BroadcastCondition(S.cv);
    SDL_UnlockMutex(S.mu);
}

static RT_NOINLINE void do_swap(SDL_Window *window)
{
    const uint64_t t0 = SDL_GetTicksNS();

    if (S.headless) {
        S.headlessSwaps++;
        return;
    }
    S.swaps++;
    if (!p4gl_win_swap(window)) {
        const char *err = p4gl_win_error();

        if (err != NULL && (strstr(err, "CONTEXT_LOST") != NULL || strstr(err, "0x300e") != NULL)) {
            S.lost = 1;
        }
    }
    S.swapNs += SDL_GetTicksNS() - t0;
}

/* ---- order-safe merging of consecutive immediate-mode draws (P4PC_RT_MERGE=0 turns it off) ----
 * The HUD, text and effect sprites arrive as runs of "upload four vertices, draw them" with no other call between (the state filters already dropped every repeated
 * state call): BufferData(ARRAY_BUFFER, n*stride), DrawArrays(strip or list, 0, n), again and again. The executor sees such a run whole and issues one upload of all
 * the vertices and one draw. Triangle lists concatenate; a strip of an even vertex count is joined to the next by repeating the last vertex of one and the first of
 * the other, which makes two zero-area triangles (no fragments; a strip's order and winding parity are kept and these programs have no flat inputs). Primitives of
 * one draw are rasterised in order, so overlapping sprites blend exactly as they did, and nothing else runs between them. The last pair of a run stays as it was:
 * a later draw that reuses the buffer without uploading again (the second draw of an alpha-test pair) must still find that pair's vertices in it. Only the vertex
 * sizes of the two immediate-mode paths (the 64-byte Sky2 vertex, the 40-byte clip vertex) qualify, because the executor knows a vertex's size only from the
 * upload's size and the draw's vertex count. */
enum { RT_MERGE_MAX_BYTES = 256 * 1024, RT_MERGE_MAX_PAIRS = 1024, RT_MERGE_MIN_PAIRS = 3 };

typedef struct MergePair {
    const uint8_t *start;   /* the pair's BufferData record */
    const uint8_t *data;    /* its vertices */
    uint32_t count;
} MergePair;

static MergePair sMergePairs[RT_MERGE_MAX_PAIRS + 1];
static uint8_t *sMergeBuf;
static size_t sMergeCap;

/* The record at p: its op, words and payload, and the pointer past it (NULL when it does not lie whole inside the chunk). */
static const uint8_t *rec_parse(const uint8_t *p, const uint8_t *end, unsigned *op, const uint64_t **w, unsigned *nw, const uint8_t **payload)
{
    uint64_t hdr;
    size_t pay;

    if ((size_t)(end - p) < 8u) {
        return NULL;
    }
    memcpy(&hdr, p, 8);
    *op = (unsigned)(hdr & 0xFFFFu);
    *nw = (unsigned)((hdr >> 16) & 0xFFu);
    pay = (size_t)(((hdr >> 32) + 7u) & ~(uint64_t)7u);
    if ((size_t)(end - p) < 8u + (size_t)*nw * 8u + pay) {
        return NULL;
    }
    *w = (const uint64_t *)(const void *)(p + 8);
    *payload = p + 8 + (size_t)*nw * 8u;
    return *payload + pay;
}

/* Runs the merged form of the pairs starting at p when there are enough of them; returns where execution goes on (the start of the run's last pair), or NULL to run p normally. */
static RT_NOINLINE const uint8_t *merge_draw_run(const uint8_t *p, const uint8_t *end)
{
    unsigned count = 0, usage = 0, mode = 0;
    size_t stride = 0, bytes = 0, vertices = 0;
    const uint8_t *q = p;

    while (count < RT_MERGE_MAX_PAIRS) {
        unsigned op, nw, op2, nw2;
        const uint64_t *w, *w2;
        const uint8_t *pay, *pay2, *next, *after;
        uint64_t size, n;
        size_t st, added;

        next = rec_parse(q, end, &op, &w, &nw, &pay);
        if (next == NULL || op != OP_BUFFERDATA || nw < 4 || w[0] != GL_ARRAY_BUFFER || w[3] == 0) {
            break;
        }
        after = rec_parse(next, end, &op2, &w2, &nw2, &pay2);
        if (after == NULL || op2 != OP_DrawArrays || nw2 < 3 || w2[1] != 0) {
            break;
        }
        size = w[1];
        n = w2[2];
        if (n == 0 || n > 0xFFFFu || size == 0 || size > RT_MERGE_MAX_BYTES || size % n != 0) {
            break;
        }
        st = (size_t)(size / n);
        if (st != 64u && st != 40u) {
            break;
        }
        if (w2[0] != GL_TRIANGLES && !(w2[0] == GL_TRIANGLE_STRIP && n >= 4u && (n & 1u) == 0)) {
            break;
        }
        if (w2[0] == GL_TRIANGLES && n % 3u != 0) {
            break;
        }
        if (count != 0 && (st != stride || (unsigned)w2[0] != mode || (unsigned)w[2] != usage)) {
            break;
        }
        added = (size_t)size + (w2[0] == GL_TRIANGLE_STRIP && count != 0 ? 2u * st : 0u);
        if (bytes + added > RT_MERGE_MAX_BYTES) {
            break;
        }
        stride = st;
        mode = (unsigned)w2[0];
        usage = (unsigned)w[2];
        sMergePairs[count].start = q;
        sMergePairs[count].data = pay;
        sMergePairs[count].count = (uint32_t)n;
        count++;
        bytes += added;
        vertices += (size_t)n + (mode == GL_TRIANGLE_STRIP && count > 1 ? 2u : 0u);
        q = after;
    }
    if (count < RT_MERGE_MIN_PAIRS) {
        return NULL;
    }
    count--; /* the last pair runs on its own, so the buffer ends up holding its vertices */
    bytes -= (size_t)sMergePairs[count].count * stride + (mode == GL_TRIANGLE_STRIP ? 2u * stride : 0u);
    vertices -= (size_t)sMergePairs[count].count + (mode == GL_TRIANGLE_STRIP ? 2u : 0u);
    if (sMergeCap < bytes) {
        uint8_t *grown = (uint8_t *)realloc(sMergeBuf, bytes);

        if (grown == NULL) {
            return NULL;
        }
        sMergeBuf = grown;
        sMergeCap = bytes;
    }
    {
        uint8_t *dst = sMergeBuf;

        for (unsigned i = 0; i < count; i++) {
            if (mode == GL_TRIANGLE_STRIP && i != 0) {
                memcpy(dst, sMergePairs[i - 1].data + ((size_t)sMergePairs[i - 1].count - 1u) * stride, stride);
                dst += stride;
                memcpy(dst, sMergePairs[i].data, stride);
                dst += stride;
            }
            memcpy(dst, sMergePairs[i].data, (size_t)sMergePairs[i].count * stride);
            dst += (size_t)sMergePairs[i].count * stride;
        }
    }
    if (S.dumpAt != 0 && S.framesDone == S.dumpAt) {
        fprintf(stderr, "rtdump Merged %u %x %zx %zx%c", count, mode, vertices, stride, 10);
    }
    R.BufferData(GL_ARRAY_BUFFER, (GLsizeiptr)bytes, sMergeBuf, (GLenum)usage);
    R.DrawArrays((GLenum)mode, 0, (GLsizei)vertices);
    S.mergedCalls += 2u * (count + 1u) - 4u; /* the 2*count calls of the merged pairs became two (the last pair still runs its own two) */
    S.mergedRuns++;
    return sMergePairs[count].start;
}

static RT_NOINLINE void exec_chunk(Chunk *c)
{
    const uint8_t *p = c->data;
    const uint8_t *const end = c->data + c->used;

    while (p < end) {
        uint64_t hdr;
        unsigned op, nw;
        size_t pay;
        const uint64_t *w;
        const uint8_t *payload;

        memcpy(&hdr, p, 8);
        if (S.merge && !S.lost && (unsigned)(hdr & 0xFFFFu) == OP_BUFFERDATA) {
            const uint8_t *next = merge_draw_run(p, end);

            if (next != NULL) {
                p = next;
                continue;
            }
        }
        op = (unsigned)(hdr & 0xFFFFu);
        nw = (unsigned)((hdr >> 16) & 0xFFu);
        pay = (size_t)(((hdr >> 32) + 7u) & ~(uint64_t)7u);
        w = (const uint64_t *)(const void *)(p + 8);
        payload = p + 8 + (size_t)nw * 8u;
        p = payload + pay;
        if (S.lost && op != OP_RPC && op != OP_REFILL) {
            continue; /* the context is gone: nothing can run until it is replaced */
        }
        const uint64_t opT0 = S.stats ? SDL_GetTicksNS() : 0;
        if (S.dumpAt != 0 && S.framesDone == S.dumpAt && op < sizeof kOpName / sizeof kOpName[0]) {
            fprintf(stderr, "rtdump %s %llx %llx %llx %llx%c", kOpName[op], nw > 0 ? (unsigned long long)w[0] : 0ull, nw > 1 ? (unsigned long long)w[1] : 0ull,
                    nw > 2 ? (unsigned long long)w[2] : 0ull, nw > 3 ? (unsigned long long)w[3] : 0ull, 10);
        }
        if (S.stats) {
            switch (op) {
            case OP_UseProgram: case OP_BindTexture: case OP_TexParameteri: case OP_Enable: case OP_Disable: case OP_BlendFunc:
            case OP_BlendFuncSeparate: case OP_BlendEquation: case OP_BlendColor: case OP_DepthFunc: case OP_DepthMask: case OP_CullFace:
            case OP_FrontFace: case OP_ColorMask: case OP_BINDFRAMEBUFFER: case OP_Clear: case OP_CopyTexSubImage2D: case OP_BlitFramebuffer:
            case OP_BindVertexArray: case OP_FramebufferTexture2D: case OP_Viewport: case OP_TEXIMAGE: case OP_TEXSUBIMAGE:
                if (!S.runDirty) { S.runDirty = 1; S.runCause = (int)op; }
                break;
            case OP_DrawElements: case OP_DrawArrays: case OP_DrawArraysInstanced:
                S.drawsSeen++;
                if (S.runDirty) { S.runsSeen++; S.breakCause[S.runCause]++; S.runDirty = 0; }
                break;
            default: break;
            }
        }
        switch (op) {
#define X(n, params, call, wl, unp) case OP_##n: R.n unp; break;
        RT_SCALAR(X)
#undef X
        case OP_BINDFRAMEBUFFER: R.BindFramebuffer((GLenum)w[0], (GLuint)w[1]); break;
        case OP_PIXELSTORE: R.PixelStorei((GLenum)w[0], (GLint)w[1]); break;
        case OP_TEXIMAGE:
            R.TexImage2D((GLenum)w[0], (GLint)w[1], (GLint)w[2], (GLsizei)w[3], (GLsizei)w[4], (GLint)w[5], (GLenum)w[6], (GLenum)w[7],
                         w[8] != 0 ? (const void *)payload : NULL);
            break;
        case OP_TEXSUBIMAGE:
            R.TexSubImage2D((GLenum)w[0], (GLint)w[1], (GLint)w[2], (GLint)w[3], (GLsizei)w[4], (GLsizei)w[5], (GLenum)w[6], (GLenum)w[7],
                            w[8] != 0 ? (const void *)payload : NULL);
            break;
        case OP_BUFFERDATA: R.BufferData((GLenum)w[0], (GLsizeiptr)(int64_t)w[1], w[3] != 0 ? (const void *)payload : NULL, (GLenum)w[2]); break;
        case OP_UNIFORMV:
            if (w[0] == 0) {
                R.Uniform3fv((GLint)w[1], (GLsizei)w[2], (const GLfloat *)(const void *)payload);
            } else if (w[0] == 1) {
                R.UniformMatrix3fv((GLint)w[1], (GLsizei)w[2], (GLboolean)w[3], (const GLfloat *)(const void *)payload);
            } else if (w[0] == 3) {
                R.Uniform4fv((GLint)w[1], (GLsizei)w[2], (const GLfloat *)(const void *)payload);
            } else {
                R.UniformMatrix4fv((GLint)w[1], (GLsizei)w[2], (GLboolean)w[3], (const GLfloat *)(const void *)payload);
            }
            break;
        case OP_DELETE:
            switch ((int)w[0]) {
            case DEL_TEX: R.DeleteTextures((GLsizei)w[1], (const GLuint *)(const void *)payload); break;
            case DEL_BUF: R.DeleteBuffers((GLsizei)w[1], (const GLuint *)(const void *)payload); break;
            case DEL_FBO: R.DeleteFramebuffers((GLsizei)w[1], (const GLuint *)(const void *)payload); break;
            default: R.DeleteRenderbuffers((GLsizei)w[1], (const GLuint *)(const void *)payload); break;
            }
            break;
        case OP_REFILL:
            if (S.lost) {
                SDL_LockMutex(S.mu);
                S.pool[(int)w[0]].pending = 0;
                SDL_UnlockMutex(S.mu);
            } else {
                pool_refill((int)w[0]);
            }
            break;
        case OP_SWAP: do_swap((SDL_Window *)(uintptr_t)w[0]); break;
        case OP_SWAPINT:
            S.swapIntervals++;
            if (!S.headless) {
                p4gl_win_set_swap_interval((int)w[0]);
            }
            break;
        case OP_REBIND:
            if (!S.headless && !p4gl_win_make_current(S.window, S.context)) {
                S.lost = 1; /* the context did not survive the pause: the game thread replaces it at the next present */
            }
            break;
        case OP_RPC: rpc_exec((RpcCall *)(uintptr_t)w[0]); break;
        default: break;
        }
        if (S.stats && op < sizeof S.opNs / sizeof S.opNs[0]) {
            S.opNs[op] += SDL_GetTicksNS() - opT0;
            S.opCount[op]++;
            S.opBytes[op] += pay;
        }
    }
}

static void (*sFrameCheck)(const char *);
void p4gl_rt_set_frame_check(void (*check)(const char *)) { sFrameCheck = check; }

static RT_NOINLINE int SDLCALL render_main(void *unused)
{
    (void)unused;
    float_adopt();
    {
        /* The thread's work is the frame's critical path: ask for a high priority (on Android that keeps it off the small cores). P4PC_RENDER_THREAD_PRIORITY=normal leaves it. */
        const char *prio = SDL_getenv("P4PC_RENDER_THREAD_PRIORITY");

        if (prio == NULL || prio[0] != 'n') {
            SDL_SetCurrentThreadPriority(prio != NULL && prio[0] == 't' ? SDL_THREAD_PRIORITY_TIME_CRITICAL : SDL_THREAD_PRIORITY_HIGH);
        }
    }
    SDL_LockMutex(S.mu);
    S.startOk = S.headless || p4gl_win_make_current(S.window, S.context);
    S.started = 1;
    SDL_BroadcastCondition(S.cv);
    SDL_UnlockMutex(S.mu);
    if (!S.startOk) {
        return 1;
    }
    for (;;) {
        Chunk *c;
        uint64_t t0;

        SDL_LockMutex(S.mu);
        while (S.qHead == NULL && !S.quit) {
            SDL_WaitCondition(S.cv, S.mu);
        }
        c = S.qHead;
        if (c == NULL) {
            SDL_UnlockMutex(S.mu);
            break;
        }
        S.qHead = c->next;
        if (S.qHead == NULL) {
            S.qTail = NULL;
        }
        SDL_UnlockMutex(S.mu);
        t0 = SDL_GetTicksNS();
        exec_chunk(c);
        if (c->frameEnd && sFrameCheck != NULL) {
            sFrameCheck("the render thread's frame");
        }
        S.renderBusyNs += SDL_GetTicksNS() - t0;
        if (S.stats) {
            uint64_t cpu = 0;

            if (p4_os_thread_cpu_ns(&cpu) == 0 && cpu != UINT64_MAX) {
                S.rtCpuNs = cpu;
            }
        }
        SDL_LockMutex(S.mu);
        if (c->frameEnd) {
            S.framesDone++;
        }
        if (c->oversize) {
            chunk_free(c);
        } else {
            c->next = S.freeList;
            S.freeList = c;
        }
        SDL_BroadcastCondition(S.cv);
        SDL_UnlockMutex(S.mu);
    }
    if (!S.headless) {
        p4gl_win_make_current(S.window, NULL);
    }
    return 0;
}

/* ---- the game thread's side ---- */

RT_NOINLINE void p4gl_rt_install(void)
{
    if (S.on) {
        return; /* loaded again with the thread running: callers stop it first */
    }
    S.installed = 0;
    if (!p4gl_rt_wanted()) {
        return;
    }
#define X(n) R.n = p4gl_gl##n; p4gl_gl##n = rt_##n;
    RT_NAMES(X)
#undef X
    memset(S.pool, 0, sizeof S.pool);
    S.installed = 1;
}

static RT_NOINLINE int sync_init(void)
{
    int i;

    if (S.mu != NULL) {
        return 1;
    }
    S.mu = SDL_CreateMutex();
    S.cv = SDL_CreateCondition();
    if (S.mu == NULL || S.cv == NULL) {
        return 0;
    }
    for (i = 0; i < RT_CHUNKS; i++) {
        Chunk *c = chunk_new(RT_CHUNK_BYTES, 0);

        if (c == NULL) {
            return 0;
        }
        c->next = S.freeList;
        S.freeList = c;
    }
    return 1;
}

static RT_NOINLINE int start_common(void)
{
    static const int kTarget[K_N] = {256, 32, 32, 32, 32};
    const char *depth = SDL_getenv("P4PC_RENDER_THREAD_DEPTH");
    int k;

    if (!S.installed || S.on || !sync_init()) {
        return 0;
    }
    float_capture();
    S.depth = depth != NULL && depth[0] >= '1' && depth[0] <= '4' ? (unsigned)(depth[0] - '0') : 1u;
    S.stats = env_is("P4PC_RENDER_THREAD_STATS", '1');
    S.merge = !env_is("P4PC_RT_MERGE", '0');
    {
        const char *d = SDL_getenv("P4PC_RT_DUMP_FRAME"); /* the records of that frame, printed as they run (a diagnostic of the stream, not of the game) */

        S.dumpAt = 0;
        while (d != NULL && *d >= '0' && *d <= '9') {
            S.dumpAt = S.dumpAt * 10u + (unsigned)(*d - '0');
            d++;
        }
    }
    S.quit = S.lost = S.started = S.startOk = 0;
    S.framesSubmitted = S.framesDone = 0;
    S.cur = NULL;
    S.records = S.rpcs = S.chunks = S.poolMisses = S.swaps = S.swapIntervals = S.headlessSwaps = 0;
    S.rpcWaitNs = S.fenceWaitNs = S.renderBusyNs = S.swapNs = S.bytes = S.freeWaitNs = 0;
    S.statFrames = 0;
    memset(S.rpcOps, 0, sizeof S.rpcOps);
    S.unpackAlign = 4;
    S.unpackRow = 0;
    S.unpackSkip = 0;
    S.readFb = S.drawFb = 0;
    S.maxTex = S.maxRb = 2048;
    {
        GLint v = 0;

        R.GetIntegerv(GL_MAX_TEXTURE_SIZE_, &S.maxTex);
        R.GetIntegerv(GL_MAX_RENDERBUFFER_SIZE_, &S.maxRb);
        R.GetIntegerv(GL_READ_FRAMEBUFFER_BINDING_, &S.readFb);
        R.GetIntegerv(GL_FRAMEBUFFER_BINDING_, &S.drawFb);
        R.GetIntegerv(GL_UNPACK_ALIGNMENT, &v);
        S.unpackAlign = v > 0 ? v : 4;
        R.GetIntegerv(GL_UNPACK_ROW_LENGTH, &S.unpackRow);
    }
    for (k = 0; k < K_N; k++) {
        S.pool[k].head = S.pool[k].count = S.pool[k].pending = 0;
        S.pool[k].target = kTarget[k];
        pool_refill(k);
    }
    return 1;
}

static RT_NOINLINE int start_thread(void)
{
    S.thread = SDL_CreateThread(render_main, "p4gl render", NULL);
    if (S.thread == NULL) {
        return 0;
    }
    SDL_LockMutex(S.mu);
    while (!S.started) {
        SDL_WaitCondition(S.cv, S.mu);
    }
    SDL_UnlockMutex(S.mu);
    if (!S.startOk) {
        SDL_WaitThread(S.thread, NULL);
        S.thread = NULL;
        return 0;
    }
    S.on = 1;
    if (!S.exitHook) {
        S.exitHook = 1;
        atexit(p4gl_rt_stop_at_exit);
    }
    return 1;
}

RT_NOINLINE int p4gl_rt_start(SDL_Window *window, P4GlContext context)
{
    if (window == NULL || context == NULL || !S.installed || S.on || p4gl_win_current_context() != context) {
        return 0;
    }
    S.headless = 0;
    S.window = window;
    S.context = context;
    if (!start_common()) {
        return 0;
    }
    if (!p4gl_win_make_current(window, NULL)) {
        return 0;
    }
    if (!start_thread()) {
        p4gl_win_make_current(window, context);
        fprintf(stderr, "p4gl: the render thread could not start; drawing on the game thread\n");
        return 0;
    }
    fprintf(stderr, "p4gl: render thread started (depth %u)\n", S.depth);
    return 1;
}

RT_NOINLINE int p4gl_rt_start_headless(void)
{
    if (!S.installed || S.on) {
        return 0;
    }
    S.headless = 1;
    if (!start_common()) {
        return 0;
    }
    return start_thread();
}

RT_NOINLINE void p4gl_rt_stop(int reacquire)
{
    if (!S.on) {
        return;
    }
    submit_cur(0);
    SDL_LockMutex(S.mu);
    S.quit = 1;
    SDL_BroadcastCondition(S.cv);
    SDL_UnlockMutex(S.mu);
    SDL_WaitThread(S.thread, NULL);
    S.thread = NULL;
    S.on = 0;
    S.lost = 0;
    if (reacquire && !S.headless) {
        p4gl_win_make_current(S.window, S.context);
    }
}

RT_NOINLINE void p4gl_rt_stop_at_exit(void)
{
    p4gl_rt_stop(0);
}

void p4gl_rt_drain(void)
{
    if (S.on) {
        RPC_BEGIN(RPC_NOP);
        rpc(&c_);
    }
}

void p4gl_rt_swap(SDL_Window *window)
{
    REC(OP_SWAP, (uint64_t)(uintptr_t)window);
}

void p4gl_rt_rebind(void)
{
    if (S.on) {
        REC(OP_REBIND, 0);
    }
}

void p4gl_rt_swap_interval(int interval)
{
    REC(OP_SWAPINT, wi(interval));
}

int p4gl_rt_context_lost(void)
{
    return S.lost;
}

RT_NOINLINE void p4gl_rt_frame_end(void)
{
    uint64_t t0;

    if (!S.on) {
        return;
    }
    submit_cur(1);
    t0 = SDL_GetTicksNS();
    SDL_LockMutex(S.mu);
    {
        int warned = 0;

        while (S.framesSubmitted - S.framesDone > S.depth) {
            SDL_WaitConditionTimeout(S.cv, S.mu, 100);
            if (!warned && SDL_GetTicksNS() - t0 > 5000000000ull) {
                warned = 1;
                fprintf(stderr, "p4gl: render thread has not finished frame %llu after 5 s\n", (unsigned long long)S.framesDone + 1u);
            }
        }
    }
    SDL_UnlockMutex(S.mu);
    S.fenceWaitNs += SDL_GetTicksNS() - t0;
    if (S.stats && ++S.statFrames >= 1000u) {
        const double f = (double)S.statFrames;

        fprintf(stderr,
                "p4gl render thread: %u frames: per frame %.0f records %.1f KB %.2f rpcs; game thread waits %.3f ms for the fence, %.3f ms for rpcs, %.3f ms for chunks; "
                "render thread busy %.3f ms (swap %.3f ms); name pool misses %llu\n",
                S.statFrames, (double)S.records / f, (double)S.bytes / 1024.0 / f, (double)S.rpcs / f, (double)S.fenceWaitNs * 1e-6 / f,
                (double)S.rpcWaitNs * 1e-6 / f, (double)S.freeWaitNs * 1e-6 / f, (double)S.renderBusyNs * 1e-6 / f, (double)S.swapNs * 1e-6 / f,
                S.poolMisses);
        {
            unsigned op;

            fprintf(stderr, "p4gl render thread: round trips by call (op:count):");
            for (op = 0; op <= RPC_NOP; op++) {
                if (S.rpcOps[op] != 0) {
                    fprintf(stderr, " %u:%llu", op, S.rpcOps[op]);
                }
            }
            fprintf(stderr, "%c", 10);
            memset(S.rpcOps, 0, sizeof S.rpcOps);
        }
        {
            unsigned k, shown;

            for (shown = 0; shown < 14; shown++) {
                unsigned best = 0;
                uint64_t bestNs = 0;

                for (k = 1; k < sizeof S.opNs / sizeof S.opNs[0]; k++) {
                    if (S.opNs[k] > bestNs) { bestNs = S.opNs[k]; best = k; }
                }
                if (best == 0) break;
                fprintf(stderr, "p4gl render thread:   op %-18s %7.1f/frame %7.3f ms/frame (%5.2f us each) %7.1f KB/frame\n",
                        best < sizeof kOpName / sizeof kOpName[0] ? kOpName[best] : "?", (double)S.opCount[best] / f,
                        (double)S.opNs[best] * 1e-6 / f, (double)S.opNs[best] * 1e-3 / (double)(S.opCount[best] ? S.opCount[best] : 1),
                        (double)S.opBytes[best] / 1024.0 / f);
                S.opNs[best] = 0;
            }
            memset(S.opNs, 0, sizeof S.opNs); memset(S.opCount, 0, sizeof S.opCount); memset(S.opBytes, 0, sizeof S.opBytes);
            fprintf(stderr, "p4gl render thread:   draws %.1f/frame, state-run starts %.1f/frame (draws with no state change since the previous draw: %.1f%%); run breaks by first state op:",
                    (double)S.drawsSeen / f, (double)S.runsSeen / f, S.drawsSeen != 0 ? 100.0 * (double)(S.drawsSeen - S.runsSeen) / (double)S.drawsSeen : 0.0);
            for (k = 1; k < sizeof S.breakCause / sizeof S.breakCause[0]; k++) {
                if (S.breakCause[k] * 50 >= S.runsSeen && S.breakCause[k] != 0) {
                    fprintf(stderr, " %s=%.1f", k < sizeof kOpName / sizeof kOpName[0] ? kOpName[k] : "?", (double)S.breakCause[k] / f);
                }
            }
            fprintf(stderr, "%c", 10);
            {
                uint64_t game = 0;
                const uint64_t rt = S.rtCpuNs;

                if (p4_os_thread_cpu_ns(&game) == 0 && game != UINT64_MAX && S.statGameCpu0 != 0 && rt >= S.statRtCpu0 && S.statRtCpu0 != 0) {
                    fprintf(stderr, "p4gl render thread:   thread CPU time per frame (not wall time): game thread %.3f ms, render thread %.3f ms%c", (double)(game - S.statGameCpu0) * 1e-6 / f,
                            (double)(rt - S.statRtCpu0) * 1e-6 / f, 10);
                }
                S.statGameCpu0 = game;
                S.statRtCpu0 = rt;
            }
            if (S.merge) {
                fprintf(stderr, "p4gl render thread:   merged immediate-mode draws: %.1f runs/frame, %.1f driver calls saved/frame%c", (double)S.mergedRuns / f, (double)S.mergedCalls / f, 10);
            }
            S.mergedCalls = S.mergedRuns = 0;
            S.drawsSeen = S.runsSeen = 0;
            memset(S.breakCause, 0, sizeof S.breakCause);
        }
        S.statFrames = 0;
        S.records = S.rpcs = S.bytes = S.poolMisses = 0;
        S.fenceWaitNs = S.rpcWaitNs = S.freeWaitNs = S.renderBusyNs = S.swapNs = 0;
    }
}

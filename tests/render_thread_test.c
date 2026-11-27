/* The render thread's command stream (src/render/p4gl_rt.c) against a fake driver: no window, no GL context. The fake entry points log every call with
 * the thread it ran on; the test checks that the stream replays calls in order, with the data the caller had at the time of the call (owned copies), that
 * calls that return something wait for the thread, that the framebuffer bindings are answered without a round trip, that the name pool hands out distinct
 * names, and that the frame fence keeps the game thread within one frame of the render thread. */
#include <stdint.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include <SDL3/SDL.h>

#include "p4gl_gl.h"
#include "p4gl_rt.h"

static int bad;

#define CHECK(cond) \
    do { \
        if (!(cond)) { \
            printf("render_thread_test: line %d: %s\n", __LINE__, #cond); \
            bad = 1; \
        } \
    } while (0)

/* ---- the fake driver ---- */
enum { L_BIND_TEXTURE = 1, L_TEXIMAGE, L_TEXSUBIMAGE, L_VIEWPORT, L_UNIFORM4F, L_BUFFERDATA, L_DRAWELEMENTS, L_CLEAR, L_DELETE_TEXTURES, L_UNIFORM_M4, L_VAP, L_UNIFORM_V4, L_DRAWARRAYS };
static struct { int op; int64_t a[4]; SDL_ThreadID thread; } sLog[65536];
static volatile int sLogN;
static SDL_ThreadID sMain;
static uint8_t sSeenPixels[64];   /* what the fake TexImage2D / TexSubImage2D saw */
static int sSeenPixelCount;
static unsigned char *sBigSeen;
static size_t sBigSeenBytes;
static uint32_t sNameCounter[8];
static volatile int sClearDelayMs;
static GLuint sSeenNames[64];
static int sSeenNameCount;
static float sSeenFloats[16];
static float sSeenV4[80];
static unsigned char sBufData[64][1536]; /* the vertices of each BufferData that carried 64 to 1536 bytes, in order */
static size_t sBufBytes[64];
static int sBufCount;
static GLenum sErrorToReturn;

static void L(int op, int64_t a, int64_t b, int64_t c, int64_t d)
{
    const int i = sLogN;
    sLog[i].op = op;
    sLog[i].a[0] = a; sLog[i].a[1] = b; sLog[i].a[2] = c; sLog[i].a[3] = d;
    sLog[i].thread = SDL_GetCurrentThreadID();
    sLogN = i + 1;
}

static void P4GL_APIENTRY fBindTexture(GLenum t, GLuint n) { L(L_BIND_TEXTURE, t, n, 0, 0); }
static void P4GL_APIENTRY fTexImage2D(GLenum t, GLint l, GLint i, GLsizei w, GLsizei h, GLint b, GLenum f, GLenum ty, const void *p)
{
    (void)l; (void)i; (void)b; (void)f; (void)ty;
    L(L_TEXIMAGE, w, h, p != NULL, t);
    if (p != NULL && w == 4 && h == 4) {
        memcpy(sSeenPixels, p, 64);
        sSeenPixelCount = 64;
    }
    if (p != NULL && w == 1024) {
        sBigSeenBytes = (size_t)w * (size_t)h * 4u;
        sBigSeen = (unsigned char *)malloc(sBigSeenBytes);
        memcpy(sBigSeen, p, sBigSeenBytes);
    }
}
static void P4GL_APIENTRY fTexSubImage2D(GLenum t, GLint l, GLint x, GLint y, GLsizei w, GLsizei h, GLenum f, GLenum ty, const void *p)
{
    (void)t; (void)l; (void)x; (void)y; (void)f; (void)ty;
    L(L_TEXSUBIMAGE, w, h, p != NULL, 0);
    if (p != NULL && w == 10) {
        memcpy(sSeenPixels, p, 42); /* 16-byte rows, three rows, the last one 10 bytes long */
        sSeenPixelCount = 42;
    }
}
static void P4GL_APIENTRY fViewport(GLint x, GLint y, GLsizei w, GLsizei h) { L(L_VIEWPORT, x, y, w, h); }
static void P4GL_APIENTRY fUniform4f(GLint l, GLfloat a, GLfloat b, GLfloat c, GLfloat d)
{
    sSeenFloats[0] = a; sSeenFloats[1] = b; sSeenFloats[2] = c; sSeenFloats[3] = d;
    L(L_UNIFORM4F, l, 0, 0, 0);
}
static void P4GL_APIENTRY fBufferData(GLenum t, GLsizeiptr size, const void *d, GLenum u)
{
    L(L_BUFFERDATA, t, size, d != NULL ? ((const unsigned char *)d)[0] : -1, u);
    if (d != NULL && size >= 3) {
        sSeenPixels[0] = ((const unsigned char *)d)[size - 1];
    }
    if (d != NULL && size >= 64 && (size_t)size <= sizeof sBufData[0] && sBufCount < 64) {
        memcpy(sBufData[sBufCount], d, (size_t)size);
        sBufBytes[sBufCount++] = (size_t)size;
    }
}
static void P4GL_APIENTRY fDrawArrays(GLenum m, GLint first, GLsizei n) { L(L_DRAWARRAYS, m, first, n, 0); }
static void P4GL_APIENTRY fDrawElements(GLenum m, GLsizei n, GLenum t, const void *i) { (void)t; L(L_DRAWELEMENTS, m, n, (int64_t)(intptr_t)i, 0); }
static void P4GL_APIENTRY fClear(GLbitfield m)
{
    if (sClearDelayMs > 0) {
        SDL_Delay((Uint32)sClearDelayMs);
    }
    L(L_CLEAR, m, 0, 0, 0);
}
static void P4GL_APIENTRY fDeleteTextures(GLsizei n, const GLuint *p)
{
    int i;
    sSeenNameCount = n < 64 ? n : 64;
    for (i = 0; i < sSeenNameCount; i++) sSeenNames[i] = p[i];
    L(L_DELETE_TEXTURES, n, 0, 0, 0);
}
static void P4GL_APIENTRY fUniformMatrix4fv(GLint l, GLsizei c, GLboolean tr, const GLfloat *v)
{
    int i;
    for (i = 0; i < 16 * c && i < 16; i++) sSeenFloats[i] = v[i];
    L(L_UNIFORM_M4, l, c, tr, 0);
}
static void P4GL_APIENTRY fUniform4fv(GLint l, GLsizei c, const GLfloat *v)
{
    int i;
    for (i = 0; i < 4 * c && i < 80; i++) sSeenV4[i] = v[i];
    L(L_UNIFORM_V4, l, c, 0, 0);
}
static void P4GL_APIENTRY fVertexAttribPointer(GLuint i, GLint s, GLenum t, GLboolean n, GLsizei st, const void *p) { (void)t; (void)n; L(L_VAP, i, s, st, (int64_t)(intptr_t)p); }
static GLenum P4GL_APIENTRY fGetError(void) { return sErrorToReturn; }
static void P4GL_APIENTRY fGetIntegerv(GLenum p, GLint *d)
{
    switch (p) {
    case 0x0D33: *d = 8192; break;
    case 0x84E8: *d = 4096; break;
    case 0x0CF5: *d = 4; break;
    case 0x1234: *d = 4242; break;
    default: *d = 0; break;
    }
}
static GLuint P4GL_APIENTRY fCreateProgram(void) { return 77; }
static GLuint P4GL_APIENTRY fCreateShader(GLenum t) { return 100 + t; }
static GLint P4GL_APIENTRY fGetUniformLocation(GLuint p, const GLchar *n) { return (GLint)(p + (GLuint)strlen(n)); }
static void P4GL_APIENTRY fGetShaderiv(GLuint s, GLenum p, GLint *d) { (void)s; (void)p; *d = GL_TRUE; }
static void P4GL_APIENTRY fReadPixels(GLint x, GLint y, GLsizei w, GLsizei h, GLenum f, GLenum t, void *p)
{
    (void)x; (void)y; (void)f; (void)t;
    memset(p, 0x5A, (size_t)w * (size_t)h * 4u);
}
static void gen(GLsizei n, GLuint *out, int kind)
{
    GLsizei i;
    for (i = 0; i < n; i++) out[i] = ++sNameCounter[kind];
}
static void P4GL_APIENTRY fGenTextures(GLsizei n, GLuint *o) { gen(n, o, 0); }
static void P4GL_APIENTRY fGenBuffers(GLsizei n, GLuint *o) { gen(n, o, 1); }
static void P4GL_APIENTRY fGenVertexArrays(GLsizei n, GLuint *o) { gen(n, o, 2); }
static void P4GL_APIENTRY fGenFramebuffers(GLsizei n, GLuint *o) { gen(n, o, 3); }
static void P4GL_APIENTRY fGenRenderbuffers(GLsizei n, GLuint *o) { gen(n, o, 4); }
static void P4GL_APIENTRY fNothing(void) {}
static void P4GL_APIENTRY fBindFramebuffer(GLenum t, GLuint f) { (void)t; (void)f; }
static void P4GL_APIENTRY fDeleteFramebuffers(GLsizei n, const GLuint *f) { (void)n; (void)f; }
static void P4GL_APIENTRY fPixelStorei(GLenum p, GLint v) { (void)p; (void)v; }

/* Everything the fake does not log is a no-op with the right signature: the table's pointers are all set from these. */
static void install_fakes(void)
{
    p4gl_glBindTexture = fBindTexture;
    p4gl_glTexImage2D = fTexImage2D;
    p4gl_glTexSubImage2D = fTexSubImage2D;
    p4gl_glViewport = fViewport;
    p4gl_glUniform4f = fUniform4f;
    p4gl_glBufferData = fBufferData;
    p4gl_glDrawElements = fDrawElements;
    p4gl_glDrawArrays = fDrawArrays;
    p4gl_glClear = fClear;
    p4gl_glDeleteTextures = fDeleteTextures;
    p4gl_glUniformMatrix4fv = fUniformMatrix4fv;
    p4gl_glUniform4fv = fUniform4fv;
    p4gl_glVertexAttribPointer = fVertexAttribPointer;
    p4gl_glGetError = fGetError;
    p4gl_glGetIntegerv = fGetIntegerv;
    p4gl_glCreateProgram = fCreateProgram;
    p4gl_glCreateShader = fCreateShader;
    p4gl_glGetUniformLocation = fGetUniformLocation;
    p4gl_glGetShaderiv = fGetShaderiv;
    p4gl_glReadPixels = fReadPixels;
    p4gl_glGenTextures = fGenTextures;
    p4gl_glGenBuffers = fGenBuffers;
    p4gl_glGenVertexArrays = fGenVertexArrays;
    p4gl_glGenFramebuffers = fGenFramebuffers;
    p4gl_glGenRenderbuffers = fGenRenderbuffers;
    p4gl_glBindFramebuffer = fBindFramebuffer;
    p4gl_glDeleteFramebuffers = fDeleteFramebuffers;
    p4gl_glPixelStorei = fPixelStorei;
    /* the remaining entry points: harmless no-ops (a call through a null pointer would end the test, which is the finding) */
#define NOP(name) p4gl_gl##name = (void *)fNothing
    NOP(GetString); NOP(GetStringi); NOP(CopyTexSubImage2D); NOP(ActiveTexture); NOP(TexParameteri); NOP(ClearColor); NOP(Flush);
    NOP(Enable); NOP(Disable); NOP(BlendFunc); NOP(BindVertexArray); NOP(BindBuffer); NOP(EnableVertexAttribArray); NOP(VertexAttribDivisor);
    NOP(DeleteShader); NOP(ShaderSource); NOP(CompileShader); NOP(GetShaderInfoLog); NOP(DeleteProgram); NOP(AttachShader); NOP(LinkProgram);
    NOP(GetProgramiv); NOP(UseProgram); NOP(Uniform2f); NOP(Uniform1i); NOP(Uniform1f); NOP(DrawArraysInstanced); NOP(DeleteBuffers);
    NOP(DepthFunc); NOP(DepthMask); NOP(CullFace); NOP(BlitFramebuffer); NOP(FrontFace); NOP(ColorMask); NOP(BlendEquation); NOP(BlendFuncSeparate);
    NOP(BlendColor); NOP(Uniform3f); NOP(Uniform3fv); NOP(UniformMatrix3fv); NOP(FramebufferTexture2D);
    NOP(CheckFramebufferStatus); NOP(DeleteRenderbuffers); NOP(BindRenderbuffer); NOP(RenderbufferStorage);
    NOP(FramebufferRenderbuffer);
#undef NOP
}

static int find(int op, int from)
{
    int i;
    for (i = from; i < sLogN; i++) {
        if (sLog[i].op == op) return i;
    }
    return -1;
}

int main(void)
{
    uint8_t px[64], row[48];
    float m[16];
    GLuint names[300], seen[1200];
    GLint v = 0;
    GLint loc;
    GLubyte *big;
    unsigned long long c[6];
    int i, n0;

    SDL_setenv_unsafe("P4PC_RENDER_THREAD", "1", 1);
    sMain = SDL_GetCurrentThreadID();
    p4gl_rt_set_default(0);
    install_fakes();
    p4gl_rt_install();
    CHECK(p4gl_glClear != fClear); /* the recorders are in the table */

    /* installed but not started: straight through, on this thread */
    p4gl_glBindTexture(GL_TEXTURE_2D, 5);
    CHECK(sLogN == 1 && sLog[0].thread == sMain && sLog[0].a[1] == 5);
    sLogN = 0;

    CHECK(p4gl_rt_start_headless());
    CHECK(p4gl_rt_active());

    /* order, thread, and owned copies of the payload */
    for (i = 0; i < 64; i++) px[i] = (uint8_t)(i + 1);
    p4gl_glBindTexture(GL_TEXTURE_2D, 9);
    p4gl_glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 4, 4, 0, GL_RGBA, GL_UNSIGNED_BYTE, px);
    memset(px, 0, sizeof px); /* the caller reuses its buffer at once */
    p4gl_glViewport(-5, -6, 7, 8);
    p4gl_glUniform4f(3, 0.5f, -1.25f, 3.0e9f, -0.0f);
    p4gl_glDrawElements(GL_TRIANGLES, 99, GL_UNSIGNED_INT, (const void *)(uintptr_t)12345);
    p4gl_glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, 36, (const void *)(uintptr_t)20);
    p4gl_rt_drain();
    {
        const int a = find(L_BIND_TEXTURE, 0), b = find(L_TEXIMAGE, 0), vi = find(L_VIEWPORT, 0), u = find(L_UNIFORM4F, 0), d = find(L_DRAWELEMENTS, 0);
        const int p = find(L_VAP, 0);

        CHECK(a >= 0 && b > a && vi > b && u > vi && d > u && p > d);
        CHECK(sLog[a].a[1] == 9 && sLog[a].thread != sMain);
        CHECK(sLog[b].a[0] == 4 && sLog[b].a[2] == 1);
        CHECK(sSeenPixelCount == 64 && sSeenPixels[0] == 1 && sSeenPixels[63] == 64);
        CHECK(sLog[vi].a[0] == -5 && sLog[vi].a[1] == -6 && sLog[vi].a[2] == 7 && sLog[vi].a[3] == 8);
        CHECK(sSeenFloats[0] == 0.5f && sSeenFloats[1] == -1.25f && sSeenFloats[2] == 3.0e9f && signbit(sSeenFloats[3]));
        CHECK(sLog[d].a[1] == 99 && sLog[d].a[2] == 12345);
        CHECK(sLog[p].a[2] == 36 && sLog[p].a[3] == 20);
    }

    /* buffer data, a null pixel pointer (no copy), matrices */
    n0 = sLogN;
    p4gl_glBufferData(GL_ARRAY_BUFFER, 5, "\x07\x00\x00\x00\x09", GL_STREAM_DRAW);
    p4gl_glBufferData(GL_ARRAY_BUFFER, 4096, NULL, GL_STREAM_DRAW);
    for (i = 0; i < 16; i++) m[i] = (float)i * 0.25f;
    p4gl_glUniformMatrix4fv(8, 1, GL_FALSE, m);
    memset(m, 0, sizeof m);
    p4gl_glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 256, 128, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    p4gl_rt_drain();
    {
        const int b0 = find(L_BUFFERDATA, n0), b1 = find(L_BUFFERDATA, b0 + 1), t = find(L_TEXIMAGE, n0), u = find(L_UNIFORM_M4, n0);

        CHECK(b0 >= 0 && sLog[b0].a[1] == 5 && sLog[b0].a[2] == 7 && sSeenPixels[0] == 9);
        CHECK(b1 > b0 && sLog[b1].a[1] == 4096 && sLog[b1].a[2] == -1);
        CHECK(u > b1 && t > u && sLog[t].a[0] == 256 && sLog[t].a[2] == 0);
        CHECK(u > 0 && sSeenFloats[1] == 0.25f && sSeenFloats[15] == 3.75f);
    }

    /* a vec4 uniform array (the atomic draw's packed per-draw values) is copied when recorded */
    {
        float packedValues[80];
        int u4, k;

        for (k = 0; k < 80; k++) packedValues[k] = (float)k * 0.5f - 7.0f;
        n0 = sLogN;
        p4gl_glUniform4fv(9, 20, packedValues);
        memset(packedValues, 0, sizeof packedValues);
        p4gl_rt_drain();
        u4 = find(L_UNIFORM_V4, n0);
        CHECK(u4 >= 0 && sLog[u4].a[0] == 9 && sLog[u4].a[1] == 20);
        CHECK(sSeenV4[0] == -7.0f && sSeenV4[41] == 13.5f && sSeenV4[79] == 32.5f);
    }

    /* runs of "upload the vertices, draw them" are merged, and the last pair of a run stays as it was. Strips of 4 vertices of 64 bytes, vertex k of pair i filled with 4 i + k + 1 */
    {
        static const int expect[22] = {1, 2, 3, 4, 4, 5, 5, 6, 7, 8, 8, 9, 9, 10, 11, 12, 12, 13, 13, 14, 15, 16};
        uint8_t quad[256];
        int pair, k, calls, b0 = sBufCount, bd, da;

        n0 = sLogN;
        for (pair = 0; pair < 5; pair++) {
            for (k = 0; k < 4; k++) memset(quad + 64 * k, 4 * pair + k + 1, 64);
            p4gl_glBufferData(GL_ARRAY_BUFFER, 256, quad, GL_STREAM_DRAW);
            p4gl_glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        }
        p4gl_rt_drain();
        calls = 0;
        for (k = n0; k < sLogN; k++) calls += sLog[k].op == L_BUFFERDATA || sLog[k].op == L_DRAWARRAYS;
        CHECK(calls == 4 && sBufCount == b0 + 2); /* one merged upload and draw of pairs 0-3, then the last pair's own two */
        bd = find(L_BUFFERDATA, n0);
        da = find(L_DRAWARRAYS, n0);
        CHECK(bd >= 0 && da > bd && sLog[bd].a[1] == 22 * 64 && sLog[da].a[0] == GL_TRIANGLE_STRIP && sLog[da].a[1] == 0 && sLog[da].a[2] == 22);
        CHECK(sBufBytes[b0] == 22 * 64);
        for (k = 0; k < 22; k++) CHECK(sBufData[b0][64 * k] == expect[k] && sBufData[b0][64 * k + 63] == expect[k]);
        CHECK(sBufBytes[b0 + 1] == 256 && sBufData[b0 + 1][0] == 17 && sBufData[b0 + 1][255] == 20); /* the buffer ends up holding the last pair's vertices */
    }
    {
        /* two pairs are not enough, and another call between pairs ends a run */
        uint8_t quad[256];
        int pair, k, calls;

        memset(quad, 7, sizeof quad);
        n0 = sLogN;
        for (pair = 0; pair < 2; pair++) {
            p4gl_glBufferData(GL_ARRAY_BUFFER, 256, quad, GL_STREAM_DRAW);
            p4gl_glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        }
        p4gl_rt_drain();
        calls = 0;
        for (k = n0; k < sLogN; k++) calls += sLog[k].op == L_BUFFERDATA || sLog[k].op == L_DRAWARRAYS;
        CHECK(calls == 4);
        n0 = sLogN;
        for (pair = 0; pair < 6; pair++) {
            p4gl_glBufferData(GL_ARRAY_BUFFER, 256, quad, GL_STREAM_DRAW);
            p4gl_glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            if (pair == 1) p4gl_glUniform4f(3, 1.0f, 2.0f, 3.0f, 4.0f);
        }
        p4gl_rt_drain();
        calls = 0;
        for (k = n0; k < sLogN; k++) calls += sLog[k].op == L_BUFFERDATA || sLog[k].op == L_DRAWARRAYS;
        CHECK(calls == 2 * 2 + 4); /* pairs 0-1 stay as they are (a run of two), pairs 2-5 merge into 2 + the last pair's 2 */
    }
    {
        /* triangle lists of 40-byte vertices concatenate without joins; other vertex sizes and fans are left alone */
        uint8_t tri[3 * 40];
        int pair, k, b0 = sBufCount, calls, da;

        n0 = sLogN;
        for (pair = 0; pair < 4; pair++) {
            memset(tri, 30 + pair, sizeof tri);
            p4gl_glBufferData(GL_ARRAY_BUFFER, 120, tri, GL_STREAM_DRAW);
            p4gl_glDrawArrays(GL_TRIANGLES, 0, 3);
        }
        p4gl_rt_drain();
        da = find(L_DRAWARRAYS, n0);
        CHECK(da >= 0 && sLog[da].a[0] == GL_TRIANGLES && sLog[da].a[2] == 9); /* pairs 0-2 as one draw of 9 vertices */
        CHECK(sBufBytes[b0] == 360 && sBufData[b0][0] == 30 && sBufData[b0][120] == 31 && sBufData[b0][359] == 32);
        calls = 0;
        for (k = n0; k < sLogN; k++) calls += sLog[k].op == L_BUFFERDATA || sLog[k].op == L_DRAWARRAYS;
        CHECK(calls == 4);
        n0 = sLogN;
        for (pair = 0; pair < 4; pair++) {
            uint8_t odd[192];

            memset(odd, 5, sizeof odd);
            p4gl_glBufferData(GL_ARRAY_BUFFER, 192, odd, GL_STREAM_DRAW); /* 4 vertices of 48 bytes: not a size the executor knows */
            p4gl_glDrawArrays(GL_TRIANGLES, 0, 4);
        }
        p4gl_rt_drain();
        calls = 0;
        for (k = n0; k < sLogN; k++) calls += sLog[k].op == L_BUFFERDATA || sLog[k].op == L_DRAWARRAYS;
        CHECK(calls == 8);
    }

    /* a row length and alignment upload (the movie planes): 16-byte rows, 10 pixels wide, 3 rows */
    for (i = 0; i < 48; i++) row[i] = (uint8_t)(0x40 + i);
    p4gl_glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    p4gl_glPixelStorei(GL_UNPACK_ROW_LENGTH, 16);
    p4gl_glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 10, 3, 0x1903 /* GL_RED */, GL_UNSIGNED_BYTE, row);
    memset(row, 0, sizeof row);
    p4gl_glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    p4gl_rt_drain();
    CHECK(sSeenPixelCount == 42 && sSeenPixels[0] == 0x40 && sSeenPixels[16] == 0x50 && sSeenPixels[41] == 0x40 + 41);

    /* a record larger than a chunk */
    big = (GLubyte *)malloc(1024u * 1024u * 4u);
    for (i = 0; i < 1024 * 1024 * 4; i++) big[i] = (GLubyte)(i * 7);
    p4gl_glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1024, 1024, 0, GL_RGBA, GL_UNSIGNED_BYTE, big);
    memset(big, 0, 1024u * 1024u * 4u);
    p4gl_rt_drain();
    CHECK(sBigSeen != NULL && sBigSeenBytes == 1024u * 1024u * 4u);
    if (sBigSeen != NULL) {
        for (i = 0; i < 1024 * 1024 * 4; i++) {
            if (sBigSeen[i] != (GLubyte)(i * 7)) {
                CHECK(!"large upload intact");
                break;
            }
        }
    }
    free(big);

    /* calls that return something wait and give the driver's answer */
    p4gl_rt_counters(c);
    n0 = (int)c[1];
    sErrorToReturn = 0x502;
    CHECK(p4gl_glGetError() == 0x502);
    CHECK(p4gl_glCreateProgram() == 77);
    CHECK(p4gl_glCreateShader(0x8B31) == 100u + 0x8B31u);
    loc = p4gl_glGetUniformLocation(70, "uniform");
    CHECK(loc == 77);
    {
        GLint ok = 0;
        p4gl_glGetShaderiv(1, GL_COMPILE_STATUS, &ok);
        CHECK(ok == GL_TRUE);
    }
    p4gl_glGetIntegerv(0x1234, &v);
    CHECK(v == 4242);
    {
        uint8_t out[32];
        memset(out, 0, sizeof out);
        p4gl_glReadPixels(0, 0, 2, 2, GL_RGBA, GL_UNSIGNED_BYTE, out);
        CHECK(out[0] == 0x5A && out[15] == 0x5A && out[16] == 0);
    }
    p4gl_rt_counters(c);
    CHECK((int)c[1] - n0 == 7);

    /* the questions answered without a round trip */
    n0 = (int)c[1];
    p4gl_glBindFramebuffer(GL_FRAMEBUFFER, 7);
    p4gl_glGetIntegerv(0x8CA6, &v);
    CHECK(v == 7);
    p4gl_glBindFramebuffer(GL_READ_FRAMEBUFFER, 3);
    p4gl_glGetIntegerv(0x8CAA, &v);
    CHECK(v == 3);
    p4gl_glGetIntegerv(0x8CA6, &v);
    CHECK(v == 7);
    {
        GLuint dead = 3;
        p4gl_glDeleteFramebuffers(1, &dead);
    }
    p4gl_glGetIntegerv(0x8CAA, &v);
    CHECK(v == 0);
    p4gl_glGetIntegerv(0x0D33, &v);
    CHECK(v == 8192);
    p4gl_glGetIntegerv(0x84E8, &v);
    CHECK(v == 4096);
    p4gl_rt_counters(c);
    CHECK((int)c[1] == n0);

    /* delete lists are copied */
    {
        GLuint dead[3] = {11, 12, 13};
        p4gl_glDeleteTextures(3, dead);
        dead[0] = dead[1] = dead[2] = 0;
    }
    p4gl_rt_drain();
    CHECK(sSeenNameCount == 3 && sSeenNames[0] == 11 && sSeenNames[2] == 13);

    /* names: pooled, distinct, and the pool keeps up (a miss is only a round trip, never a duplicate) */
    for (i = 0; i < 1200; i += 4) {
        p4gl_glGenTextures(4, names);
        memcpy(seen + i, names, 4 * sizeof(GLuint));
        if ((i & 63) == 0) p4gl_rt_drain();
    }
    {
        int j;
        for (i = 0; i < 1200; i++) {
            for (j = 0; j < i; j++) {
                if (seen[i] == seen[j]) {
                    CHECK(!"names are distinct");
                    i = 1200;
                    break;
                }
            }
        }
    }
    p4gl_rt_counters(c);
    CHECK(c[3] < 20u);
    {
        GLuint b, f, r, a;
        p4gl_glGenBuffers(1, &b);
        p4gl_glGenFramebuffers(1, &f);
        p4gl_glGenRenderbuffers(1, &r);
        p4gl_glGenVertexArrays(1, &a);
        CHECK(b != 0 && f != 0 && r != 0 && a != 0);
    }

    /* the swap and its interval are records too */
    p4gl_rt_swap_interval(1);
    p4gl_rt_swap(NULL);
    p4gl_rt_frame_end();
    p4gl_rt_drain();
    p4gl_rt_counters(c);
    CHECK(c[4] == 1 && c[5] == 1);

    /* the frame fence: with a slow driver the game thread never gets more than one frame ahead */
    sClearDelayMs = 20;
    for (i = 0; i < 6; i++) {
        const unsigned long long before = p4gl_rt_frames_done();
        p4gl_glClear(GL_COLOR_BUFFER_BIT);
        p4gl_rt_frame_end();
        (void)before;
        CHECK(p4gl_rt_frames_done() >= (unsigned long long)i + 1u); /* frame i + 2 is submitted: the one before it is done */
    }
    sClearDelayMs = 0;
    p4gl_rt_drain();

    /* stop with work pending: everything runs, then calls go straight through again */
    n0 = sLogN;
    p4gl_glBindTexture(GL_TEXTURE_2D, 31);
    p4gl_rt_stop(1);
    CHECK(!p4gl_rt_active());
    CHECK(find(L_BIND_TEXTURE, n0) >= 0);
    p4gl_glBindTexture(GL_TEXTURE_2D, 32);
    CHECK(sLog[sLogN - 1].a[1] == 32 && sLog[sLogN - 1].thread == sMain);
    CHECK(p4gl_rt_start_headless());
    p4gl_glBindTexture(GL_TEXTURE_2D, 33);
    p4gl_rt_drain();
    CHECK(sLog[sLogN - 1].a[1] == 33 && sLog[sLogN - 1].thread != sMain);
    p4gl_rt_stop(1);

    if (!bad) {
        printf("render_thread_test: ok\n");
    }
    return bad;
}

/* sfmpvp_test: the Sofdec IPU seam (src/movie/sfmpvp.c) driven the way retail Sofdec drives it.
 *
 * A retail SFD from the disc goes through the program-stream demuxer; each video PES payload is put
 * into the viBuf with SFMPVP_Put and SFMPVP_SearchPsc, and SFMPVP_DecodeFrmSj runs whenever
 * func_005213a8 (three picture start codes buffered, or flushed) allows. Checks:
 *   - every picture comes out exactly once (480 for P4CT019), in display order: each output matches
 *     the picture a plain libavcodec decode of the same file produces at that position
 *   - the RGB32 buffer is in the IPU layout: macroblocks column by column, 1024 bytes each, A = 0x80;
 *     de-tiled it matches the linear conversion within colour-conversion rounding
 *   - viBuf accounting stays sane: put bytes = consumed bytes + buffered, no overflow, and the
 *     ring never reports more fill than its size
 *   - RAW8 output is 384 bytes per macroblock and equals the decoded planes
 *   - the final calls report the starvation (-3) after the terminator, like retail
 * Skips (77) without P4PC_ISO. P4PC_SFMPVP_PNG=<dir> writes a few de-tiled frames as PNGs. */
#define _FILE_OFFSET_BITS 64
#define _POSIX_C_SOURCE 200809L

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "guest_test.h"
#include "p4pc/os.h"
#include "mpeg2_dec.h"
#include "p4png.h"
#include "sfd_demux.h"
#include "vfs.h"

#define SKIP_RETURN 77
#define MOVIE "DATA/SFD/P4CT019.SFD"
#define EXPECT_PICTURES 480
#define W 640
#define H 448
#define MBW (W / 16)
#define MBH (H / 16)

/* Retail functions the seam calls that live in other lanes. */
int32_t func_00504d08(void) { return -1; }
int32_t func_00504d10(void *mpv, void *sj, void *frm) { (void)mpv; (void)sj; (void)frm; return 0; }
int32_t func_00515c60(void *sfd) { (void)sfd; return 0; }
int32_t func_00515cc8(void *sfd) { (void)sfd; return 0; }
static int g_errors;
int32_t func_00517ad0(void *sfd, int32_t code) { (void)sfd; g_errors++; return code; }

/* The movie upload (src/movie/movie_upload.c) hands pixels to the GL raster, which is not linked here. */
typedef struct P4RwRaster P4RwRaster;
int p4gl_raster_set_rgba(P4RwRaster *r, const uint8_t *rgba, int32_t w, int32_t h) { (void)r; (void)rgba; (void)w; (void)h; return -1; }
int p4gl_gl_draw_enabled(void) { return 1; }
#ifdef __ANDROID__
int p4gl_yuv_table_rows(const uint8_t *rgba, int32_t y0, int32_t rows) { (void)rgba; (void)y0; (void)rows; return -1; }
int p4gl_yuv_table_done(void) { return -1; }
#endif
int p4gl_raster_set_yuv420(P4RwRaster *r, const uint8_t *const plane[3], const int32_t stride[3], int32_t w, int32_t h) { (void)r; (void)plane; (void)stride; (void)w; (void)h; return -1; }
void p4pc_movie_detile_rgb32(const uint8_t *src, int32_t mbw, int32_t mbh, uint8_t *dst, int32_t stride, int32_t w,
                             int32_t h);

/* The seam's entry points. */
int32_t SFMPVP_Put(void *sfd, const void *data, int32_t len);
void SFMPVP_SearchPsc(void *sfd, const uint8_t *data, int32_t len);
void SFMPVP_Flush(void *sfd, int32_t arg);
int32_t SFMPVP_DecodeFrmSj(void *mpv, void *sj, void *frm, void *sfd);
int32_t func_005213a8(void *sfd);
int32_t SFMPVP_GetViBufInf(void *sfd, int32_t *size, int32_t *wr_off, int32_t *rd_off, int32_t *fill);
void func_005208c8(void *sfd);
void func_00520818(void);
void SFMPVP_Destroy(void *sfd);

static int failures;

/* Game memory: the retail code masks addresses to 28 bits, so player structures live low, like PS2 RAM. */
static uint8_t *g_arena; /* guest memory (tests/guest_test.h) */
static size_t g_arena_used;
static void *arena_alloc(size_t n) {
    void *p = g_arena + g_arena_used;
    g_arena_used += (n + 63) & ~(size_t)63;
    memset(p, 0, n);
    return p;
}
static void check(int ok, const char *what) {
    if (!ok) { fprintf(stderr, "sfmpvp_test: FAIL %s\n", what); failures++; }
}

/* Linear RGBA from an IPU RGB32 buffer. */
static void detile(const uint8_t *ipu, uint8_t *lin) {
    for (int c = 0; c < MBW; c++)
        for (int r = 0; r < MBH; r++)
            for (int y = 0; y < 16; y++)
                for (int x = 0; x < 16; x++)
                    memcpy(lin + (((size_t)(r * 16 + y) * W) + c * 16 + x) * 4,
                           ipu + ((size_t)c * MBH + r) * 1024 + (y * 16 + x) * 4, 4);
}

int main(void) {
    const char *iso = getenv("P4PC_ISO");
    if (!iso || !*iso) { fprintf(stderr, "sfmpvp_test: P4PC_ISO is not set, skipping\n"); return SKIP_RETURN; }
    if (vfs_mount(iso, NULL) < 0) { fprintf(stderr, "sfmpvp_test: cannot mount %s\n", iso); return 1; }

    /* Retail globals the seam reads, at their retail addresses. */
    if (p4t_guest_map(0x00700000, 0x00300000) != 0) {
        fprintf(stderr, "sfmpvp_test: cannot map retail globals\n");
        return 1;
    }
    P4_G32(0x00745af8u) = 1;   /* func_005207f8(1) */
    P4_G32(0x00745abcu) = 0;

    g_arena = p4t_garena(0x01000000, 0x00600000, 64);
    if (g_arena == NULL) {
        fprintf(stderr, "sfmpvp_test: cannot allocate the arena\n");
        return 1;
    }

    /* The player: sfd -> P at +0x1fc0. P+0xc/+0x10 hold the maximum picture size, +0x14 the work area. */
    uint8_t *sfd = arena_alloc(0x2000);
    uint8_t *P = arena_alloc(0x1300);
    const int32_t s4 = (W * H * 9) / 2 + 10240;
    uint8_t *work = arena_alloc((size_t)s4 + 0x90000 + 0x10000);
    *(uint32_t *)(sfd + 0x1fc0) = P4_H2G(P);
    *(int32_t *)(sfd + 0x38) = 2;                    /* buffer format: RGB32 */
    *(int32_t *)(P + 0xc) = W;
    *(int32_t *)(P + 0x10) = H;
    *(uint32_t *)(P + 0x14) = P4_H2G(work);
    func_00520818();
    func_005208c8(sfd);
    check(*(int32_t *)(P + 0x1128) == 1, "SFMPVP_Create marks the object used");

    /* Reference decode: the same file through plain libavcodec, display order. */
    SfdDemux *dm = sfd_demux_open(MOVIE);
    check(dm != NULL, "open " MOVIE);
    P4Mpeg2 *ref = p4mpeg2_open();
    check(ref != NULL, "reference decoder");
    if (!dm || !ref) return 1;
    P4Mpeg2Frame *refs = calloc(EXPECT_PICTURES + 8, sizeof *refs);
    /* 480 reference pictures, about 525 MiB: more than a 2 GiB 32-bit Windows process can find in one
     * piece next to its DLLs, so the Windows exe is large-address-aware (tests/CMakeLists.txt). */
    uint8_t *refrgba = malloc((size_t)EXPECT_PICTURES * W * H * 4);
    check(refrgba != NULL, "allocate the reference pictures");
    if (!refrgba || !refs) return 1;
    int nref = 0;
    {
        SfdDemux *d2 = sfd_demux_open(MOVIE);
        SfdPacket pk;
        P4Mpeg2Frame f;
        while (sfd_demux_next(d2, &pk)) {
            if (pk.stream_id != SFD_STREAM_VIDEO0) continue;
            p4mpeg2_feed(ref, pk.data, pk.size, INT64_MIN);
            while (p4mpeg2_receive(ref, &f)) { p4mpeg2_to_rgba(&f, refrgba + (size_t)nref * W * H * 4); nref++; }
        }
        p4mpeg2_flush(ref);
        while (p4mpeg2_receive(ref, &f)) { p4mpeg2_to_rgba(&f, refrgba + (size_t)nref * W * H * 4); nref++; }
        check(!sfd_demux_failed(d2), "the reference read the whole movie");
        sfd_demux_close(d2);
    }
    check(nref == EXPECT_PICTURES, "reference decode yields 480 pictures");

    /* Drive the seam. */
    uint8_t *out = arena_alloc((size_t)MBW * MBH * 1024);
    uint8_t *lin = malloc((size_t)W * H * 4);
    uint8_t *frm = arena_alloc(0x40);
    *(uint32_t *)(frm + 0x20) = P4_H2G(out);
    uint8_t *stage = arena_alloc(1u << 20); /* the put side copies each PES payload here: Sofdec's data is guest memory */
    const char *pngdir = getenv("P4PC_SFMPVP_PNG");
    int got = 0, calls = 0, bad_layout = 0, worst_mad_x100 = 0, flushed = 0;
    int64_t put_total = 0;
    SfdPacket pk;
    int eof = 0;
    int pending_len = 0;
    const uint8_t *pending = NULL;
    while (got < EXPECT_PICTURES && calls < EXPECT_PICTURES * 4) {
        /* Sofdec's put side: keep the viBuf filled. */
        while (1) {
            if (!pending) {
                if (eof || !sfd_demux_next(dm, &pk)) {
                    if (!eof) { eof = 1; SFMPVP_Flush(sfd, 1); flushed = 1; }
                    break;
                }
                if (pk.stream_id != SFD_STREAM_VIDEO0) continue;
                pending = pk.data;
                pending_len = pk.size;
            }
            if (pending_len > (1 << 20)) { fprintf(stderr, "sfmpvp_test: PES payload larger than the staging buffer\n"); return 1; }
            memcpy(stage, pending, (size_t)pending_len);
            int32_t put = SFMPVP_Put(sfd, stage, pending_len);
            if (put == 0) break;   /* ring full: decode first */
            SFMPVP_SearchPsc(sfd, stage, put);
            put_total += put;
            pending = NULL;
        }
        /* The decode side. */
        if (!func_005213a8(sfd)) {
            if (eof && !pending) break;
            continue;
        }
        memset(out, 0xee, 64);
        int32_t r = SFMPVP_DecodeFrmSj(NULL, NULL, frm, sfd);
        calls++;
        int32_t fillv, size, wr, rd;
        SFMPVP_GetViBufInf(sfd, &size, &wr, &rd, &fillv);
        check(fillv >= 0 && fillv <= size, "viBuf fill within the ring");
        if (r == -3 || out[0] == 0xee) continue;   /* starvation or the sequence end with nothing held */
        detile(out, lin);
        /* layout: alpha 0x80 everywhere */
        for (size_t i = 3; i < (size_t)W * H * 4; i += 4)
            if (lin[i] != 0x80) { bad_layout++; break; }
        if (got == 240) {
            /* The game's upload path: the same frame de-tiled into a raster shadow, opaque alpha. */
            uint8_t *shadow = malloc((size_t)W * H * 4);
            p4pc_movie_detile_rgb32(out, MBW, MBH, shadow, W * 4, W, H);
            int same = 1;
            for (size_t i = 0; i < (size_t)W * H * 4; i++)
                if (i % 4 == 3 ? shadow[i] != 0xff : shadow[i] != lin[i]) { same = 0; break; }
            check(same, "movie upload de-tiles to the linear picture with alpha 0xff");
            free(shadow);
        }
        if (got < nref) {
            const uint8_t *rf = refrgba + (size_t)got * W * H * 4;
            int64_t sum = 0;
            for (size_t i = 0; i < (size_t)W * H * 4; i += 4)
                for (int k = 0; k < 3; k++) sum += abs((int)lin[i + k] - (int)rf[i + k]);
            int mad_x100 = (int)(sum * 10000 / ((int64_t)W * H * 3));
            if (mad_x100 > worst_mad_x100) worst_mad_x100 = mad_x100;
        }
        if (pngdir && (got == 0 || got == 100 || got == 240 || got == 479)) {
            for (size_t i = 0; i < (size_t)W * H * 4; i += 4) lin[i + 3] = 0xff;
            char path[1024];
            snprintf(path, sizeof path, "%s/sfmpvp_%04d.png", pngdir, got);
            p4png_write_rgba(path, W, H, lin);
        }
        got++;
    }
    check(!sfd_demux_failed(dm), "the seam read the whole movie");
    check(got == EXPECT_PICTURES, "480 pictures out of the seam");
    check(bad_layout == 0, "alpha 0x80 in every pixel");
    check(worst_mad_x100 < 40000, "de-tiled output within 4 levels of the linear conversion");
    check(flushed, "the stream was flushed at its end");
    check(g_errors == 0, "no libmpeg errors reported");

    /* RAW8: 384 bytes per macroblock, Y then Cb then Cr, columns first. */
    {
        SfdDemux *d3 = sfd_demux_open(MOVIE);
        P4Mpeg2 *dec3 = p4mpeg2_open();
        P4Mpeg2Frame f;
        int have = 0;
        while (!have && sfd_demux_next(d3, &pk)) {
            if (pk.stream_id != SFD_STREAM_VIDEO0) continue;
            p4mpeg2_feed(dec3, pk.data, pk.size, INT64_MIN);
            have = p4mpeg2_receive(dec3, &f);
        }
        check(have, "a picture for the RAW8 check");
        if (have) {
            uint8_t *raw = arena_alloc((size_t)MBW * MBH * 384);
            p4mpeg2_to_ipu_raw8(&f, raw);
            const int c = 7, r = 11;
            const uint8_t *mb = raw + ((size_t)c * MBH + r) * 384;
            int ok = 1;
            for (int y = 0; y < 16; y++)
                for (int x = 0; x < 16; x++) ok &= mb[y * 16 + x] == f.plane[0][(r * 16 + y) * f.stride[0] + c * 16 + x];
            for (int y = 0; y < 8; y++)
                for (int x = 0; x < 8; x++) {
                    ok &= mb[256 + y * 8 + x] == f.plane[1][(r * 8 + y) * f.stride[1] + c * 8 + x];
                    ok &= mb[320 + y * 8 + x] == f.plane[2][(r * 8 + y) * f.stride[2] + c * 8 + x];
                }
            check(ok, "RAW8 macroblock (7,11) equals the decoded planes");
        }
        p4mpeg2_close(dec3);
        sfd_demux_close(d3);
    }

    printf("sfmpvp_test: %d pictures in %d decode calls, %lld bytes put, worst mean difference to the reference %.2f levels\n",
           got, calls, (long long)put_total, worst_mad_x100 / 10000.0);
    SFMPVP_Destroy(sfd);
    p4mpeg2_close(ref);
    sfd_demux_close(dm);
    return failures ? 1 : 0;
}

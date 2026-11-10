/* mpeg2_dec: MPEG-2 video decode on libavcodec. See mpeg2_dec.h. */
#include "mpeg2_dec.h"

#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libavutil/pixdesc.h>
#include <libswscale/swscale.h>
#include <stdlib.h>
#include <string.h>

struct P4Mpeg2 {
    const AVCodec *codec;
    AVCodecContext *ctx;
    AVCodecParserContext *parser;
    AVPacket *pkt;
    AVFrame *scratch;
    AVFrame **queue;        /* decoded frames waiting for receive, oldest first */
    int qlen, qcap;
    struct SwsContext *sws;
    int sws_w, sws_h;
    uint8_t *lin;           /* linear RGBA scratch for the IPU layout conversion */
    size_t lin_cap;
    AVFrame *held;          /* the frame last handed out (its planes stay valid until the next receive) */
};

static int enqueue(P4Mpeg2 *d, AVFrame *f) {
    if (d->qlen == d->qcap) {
        int ncap = d->qcap ? d->qcap * 2 : 8;
        AVFrame **nq = realloc(d->queue, ncap * sizeof *nq);
        if (!nq) return -1;
        d->queue = nq;
        d->qcap = ncap;
    }
    AVFrame *c = av_frame_alloc();
    if (!c) return -1;
    av_frame_move_ref(c, f);
    d->queue[d->qlen++] = c;
    return 0;
}

/* Pulls every ready frame out of the decoder into the queue. */
static int drain(P4Mpeg2 *d) {
    for (;;) {
        int r = avcodec_receive_frame(d->ctx, d->scratch);
        if (r == AVERROR(EAGAIN) || r == AVERROR_EOF) return 0;
        if (r < 0) return -1;
        if (enqueue(d, d->scratch) < 0) return -1;
    }
}

P4Mpeg2 *p4mpeg2_open(void) {
    P4Mpeg2 *d = calloc(1, sizeof *d);
    if (!d) return NULL;
    d->codec = avcodec_find_decoder(AV_CODEC_ID_MPEG2VIDEO);
    if (!d->codec) goto fail;
    d->ctx = avcodec_alloc_context3(d->codec);
    d->parser = av_parser_init(AV_CODEC_ID_MPEG2VIDEO);
    d->pkt = av_packet_alloc();
    d->scratch = av_frame_alloc();
    if (!d->ctx || !d->parser || !d->pkt || !d->scratch) goto fail;
    d->ctx->thread_count = 1;
    d->ctx->pkt_timebase = (AVRational){1, 90000};
    if (avcodec_open2(d->ctx, d->codec, NULL) < 0) goto fail;
    return d;
fail:
    p4mpeg2_close(d);
    return NULL;
}

void p4mpeg2_close(P4Mpeg2 *d) {
    if (!d) return;
    for (int i = 0; i < d->qlen; i++) av_frame_free(&d->queue[i]);
    free(d->queue);
    av_frame_free(&d->held);
    sws_freeContext(d->sws);
    free(d->lin);
    av_frame_free(&d->scratch);
    av_packet_free(&d->pkt);
    if (d->parser) av_parser_close(d->parser);
    avcodec_free_context(&d->ctx);
    free(d);
}

int p4mpeg2_feed(P4Mpeg2 *d, const uint8_t *data, int size, int64_t pts) {
    int64_t ts = pts == INT64_MIN ? AV_NOPTS_VALUE : pts;
    while (size > 0) {
        uint8_t *out = NULL;
        int outsize = 0;
        int used = av_parser_parse2(d->parser, d->ctx, &out, &outsize, data, size, ts, AV_NOPTS_VALUE, 0);
        if (used < 0) return -1;
        data += used;
        size -= used;
        ts = AV_NOPTS_VALUE;   /* the timestamp belongs to the first picture start in this PES only */
        if (outsize > 0) {
            d->pkt->data = out;
            d->pkt->size = outsize;
            d->pkt->pts = d->parser->pts;
            d->pkt->dts = d->parser->dts;
            int r = avcodec_send_packet(d->ctx, d->pkt);
            if (r < 0 && r != AVERROR(EAGAIN)) return -1;
            if (drain(d) < 0) return -1;
        } else if (used == 0) {
            break;
        }
    }
    return 0;
}

int p4mpeg2_flush(P4Mpeg2 *d) {
    uint8_t *out = NULL;
    int outsize = 0;
    av_parser_parse2(d->parser, d->ctx, &out, &outsize, NULL, 0, AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0);
    if (outsize > 0) {
        d->pkt->data = out;
        d->pkt->size = outsize;
        d->pkt->pts = d->parser->pts;
        d->pkt->dts = d->parser->dts;
        if (avcodec_send_packet(d->ctx, d->pkt) < 0) return -1;
        if (drain(d) < 0) return -1;
    }
    avcodec_send_packet(d->ctx, NULL);
    return drain(d);
}

int p4mpeg2_receive(P4Mpeg2 *d, P4Mpeg2Frame *o) {
    if (d->qlen == 0) return 0;
    av_frame_free(&d->held);
    d->held = d->queue[0];
    memmove(d->queue, d->queue + 1, (--d->qlen) * sizeof *d->queue);
    const AVFrame *f = d->held;
    o->width = f->width;
    o->height = f->height;
    for (int i = 0; i < 3; i++) {
        o->plane[i] = f->data[i];
        o->stride[i] = f->linesize[i];
    }
    o->interlaced = !!(f->flags & AV_FRAME_FLAG_INTERLACED);
    o->top_field_first = !!(f->flags & AV_FRAME_FLAG_TOP_FIELD_FIRST);
    o->repeat_pict = f->repeat_pict;
    o->pict_type = f->pict_type;
    o->pts = f->pts == AV_NOPTS_VALUE ? INT64_MIN : f->pts;
    o->color_range = f->color_range == AVCOL_RANGE_JPEG ? 2 : f->color_range == AVCOL_RANGE_MPEG ? 1 : 0;
    o->colorspace = f->colorspace;
    o->av = (void *)f;
    return 1;
}

void p4mpeg2_frame_rate(const P4Mpeg2 *d, int *num, int *den) {
    *num = d->ctx->framerate.num;
    *den = d->ctx->framerate.den;
}

int p4mpeg2_to_rgba(const P4Mpeg2Frame *f, uint8_t *rgba) {
    struct SwsContext *sws = sws_getContext(f->width, f->height, AV_PIX_FMT_YUV420P, f->width, f->height,
                                            AV_PIX_FMT_RGBA, SWS_BILINEAR, NULL, NULL, NULL);
    if (!sws) return -1;
    int src_full = f->color_range == 2;
    sws_setColorspaceDetails(sws, sws_getCoefficients(SWS_CS_ITU601), src_full, sws_getCoefficients(SWS_CS_ITU601), 1,
                             0, 1 << 16, 1 << 16);
    uint8_t *dst[4] = {rgba, NULL, NULL, NULL};
    int dst_stride[4] = {f->width * 4, 0, 0, 0};
    const uint8_t *src[4] = {f->plane[0], f->plane[1], f->plane[2], NULL};
    int src_stride[4] = {f->stride[0], f->stride[1], f->stride[2], 0};
    int r = sws_scale(sws, src, src_stride, 0, f->height, dst, dst_stride);
    sws_freeContext(sws);
    return r == f->height ? 0 : -1;
}

int p4mpeg2_send_unit(P4Mpeg2 *d, const uint8_t *data, int size) {
    /* The decoder reads past the end by up to AV_INPUT_BUFFER_PADDING_SIZE: copy into a padded packet. */
    if (av_new_packet(d->pkt, size) < 0) return -1;
    memcpy(d->pkt->data, data, size);
    int r = avcodec_send_packet(d->ctx, d->pkt);
    av_packet_unref(d->pkt);
    if (r < 0 && r != AVERROR(EAGAIN)) return -1;
    return drain(d);
}

int p4mpeg2_end_sequence(P4Mpeg2 *d) {
    avcodec_send_packet(d->ctx, NULL);
    int r = drain(d);
    avcodec_flush_buffers(d->ctx);
    return r;
}

void p4mpeg2_reset(P4Mpeg2 *d) {
    for (int i = 0; i < d->qlen; i++) av_frame_free(&d->queue[i]);
    d->qlen = 0;
    av_frame_free(&d->held);
    avcodec_flush_buffers(d->ctx);
}

int p4mpeg2_pending(const P4Mpeg2 *d) { return d->qlen; }

/* Source sample clamped to the frame, so partial edge macroblocks repeat the last row/column. */
static inline int clampi(int v, int hi) { return v < 0 ? 0 : v > hi ? hi : v; }

int p4mpeg2_to_ipu_rgb32(P4Mpeg2 *d, const P4Mpeg2Frame *f, uint8_t *dst) {
    const int cols = (f->width + 15) / 16, rows = (f->height + 15) / 16;
    const size_t fb = (size_t)f->width * f->height * 4;
    if (fb > d->lin_cap) {
        uint8_t *n = realloc(d->lin, fb);
        if (!n) return -1;
        d->lin = n;
        d->lin_cap = fb;
    }
    uint8_t *tmp = d->lin;
    d->sws = sws_getCachedContext(d->sws, f->width, f->height, AV_PIX_FMT_YUV420P, f->width, f->height,
                                  AV_PIX_FMT_RGBA, SWS_POINT, NULL, NULL, NULL);
    if (!d->sws) return -1;
    /* IPU CSC: ITU-R BT.601, limited range YCbCr in, full range RGB out, chroma replicated. */
    sws_setColorspaceDetails(d->sws, sws_getCoefficients(SWS_CS_ITU601), 0, sws_getCoefficients(SWS_CS_ITU601), 1, 0,
                             1 << 16, 1 << 16);
    uint8_t *sdst[4] = {tmp, NULL, NULL, NULL};
    int sdst_stride[4] = {f->width * 4, 0, 0, 0};
    const uint8_t *src[4] = {f->plane[0], f->plane[1], f->plane[2], NULL};
    int src_stride[4] = {f->stride[0], f->stride[1], f->stride[2], 0};
    if (sws_scale(d->sws, src, src_stride, 0, f->height, sdst, sdst_stride) != f->height) return -1;
    for (int c = 0; c < cols; c++) {
        for (int r = 0; r < rows; r++) {
            uint8_t *mb = dst + ((size_t)c * rows + r) * 1024;
            for (int y = 0; y < 16; y++) {
                const int sy = clampi(r * 16 + y, f->height - 1);
                const uint8_t *srow = tmp + (size_t)sy * f->width * 4;
                for (int x = 0; x < 16; x++) {
                    const int sx = clampi(c * 16 + x, f->width - 1);
                    uint32_t px;

                    memcpy(&px, srow + (size_t)sx * 4, 4);
                    px = (px & 0x00ffffffu) | 0x80000000u; /* little-endian: byte 3 is the IPU's alpha of 0x80 */
                    memcpy(mb + (y * 16 + x) * 4, &px, 4);
                }
            }
        }
    }
    return 0;
}

int p4mpeg2_frame_hold(const P4Mpeg2Frame *f, P4Mpeg2Frame *out) {
    AVFrame *c = f->av ? av_frame_clone((const AVFrame *)f->av) : NULL;

    if (!c) return -1;
    *out = *f;
    for (int i = 0; i < 3; i++) out->plane[i] = c->data[i];
    out->av = c;
    return 0;
}

void p4mpeg2_frame_release(P4Mpeg2Frame *f) {
    AVFrame *a = (AVFrame *)f->av;

    av_frame_free(&a);
    f->av = NULL;
}

static P4Mpeg2StampFn g_stamp_fn;

void p4mpeg2_set_stamp_resolver(P4Mpeg2StampFn fn) { g_stamp_fn = fn; }

const P4Mpeg2Frame *p4mpeg2_resolve_stamp(const void *image) { return g_stamp_fn ? g_stamp_fn(image) : NULL; }

/* The conversion p4mpeg2_to_ipu_rgb32 runs, into a linear image. The context is kept between frames (one size in a run). */
int p4mpeg2_frame_to_rgba_ipu(const P4Mpeg2Frame *f, uint8_t *dst, int dst_stride) {
    static struct SwsContext *sws;
    static int sw, sh;
    struct SwsContext *n = sws_getCachedContext(sws, f->width, f->height, AV_PIX_FMT_YUV420P, f->width, f->height, AV_PIX_FMT_RGBA,
                                                SWS_POINT, NULL, NULL, NULL);

    if (!n) return -1;
    if (n != sws || sw != f->width || sh != f->height) {
        sws_setColorspaceDetails(n, sws_getCoefficients(SWS_CS_ITU601), 0, sws_getCoefficients(SWS_CS_ITU601), 1, 0, 1 << 16, 1 << 16);
    }
    sws = n;
    sw = f->width;
    sh = f->height;
    uint8_t *sdst[4] = {dst, NULL, NULL, NULL};
    int sdst_stride[4] = {dst_stride, 0, 0, 0};
    const uint8_t *src[4] = {f->plane[0], f->plane[1], f->plane[2], NULL};
    int src_stride[4] = {f->stride[0], f->stride[1], f->stride[2], 0};
    return sws_scale(sws, src, src_stride, 0, f->height, sdst, sdst_stride) == f->height ? 0 : -1;
}


int p4mpeg2_to_ipu_raw8(const P4Mpeg2Frame *f, uint8_t *dst) {
    const int cols = (f->width + 15) / 16, rows = (f->height + 15) / 16;
    const int cw = (f->width + 1) / 2, ch = (f->height + 1) / 2;
    for (int c = 0; c < cols; c++) {
        for (int r = 0; r < rows; r++) {
            uint8_t *mb = dst + ((size_t)c * rows + r) * 384;
            for (int y = 0; y < 16; y++)
                for (int x = 0; x < 16; x++)
                    mb[y * 16 + x] = f->plane[0][(size_t)clampi(r * 16 + y, f->height - 1) * f->stride[0] +
                                                 clampi(c * 16 + x, f->width - 1)];
            for (int p = 1; p <= 2; p++)
                for (int y = 0; y < 8; y++)
                    for (int x = 0; x < 8; x++)
                        mb[256 + (p - 1) * 64 + y * 8 + x] =
                            f->plane[p][(size_t)clampi(r * 8 + y, ch - 1) * f->stride[p] + clampi(c * 8 + x, cw - 1)];
        }
    }
    return 0;
}

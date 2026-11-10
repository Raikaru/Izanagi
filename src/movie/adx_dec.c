/* adx_dec: ADX audio on libavcodec's adpcm_adx decoder. See adx_dec.h. */
#include "adx_dec.h"

#include <libavcodec/avcodec.h>
#include <libavutil/channel_layout.h>
#include <libavutil/samplefmt.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include "p4pc/profile_trace.h"

#define MAX_CH 8
#define HDR_MIN 16
#define BLOCK 18

struct P4Adx {
    AVCodecContext *ctx;
    AVPacket *pkt;
    AVFrame *frame;
    uint8_t *in;            /* bytes not yet consumed */
    int in_len, in_cap;
    int header_done;
    int hdr_size;
    int channels, rate;
    int16_t *pcm;           /* interleaved FIFO */
    int pcm_frames, pcm_cap; /* sample frames */
    pthread_mutex_t lock;   /* the queue is read by the audio device thread */
    int64_t total;          /* sample frames decoded so far */
    int limit;              /* most sample frames kept; the oldest go first (0: unlimited) */
};

P4Adx *p4adx_open(void) {
    P4Adx *a = calloc(1, sizeof(P4Adx));
    if (a) pthread_mutex_init(&a->lock, NULL);
    return a;
}

void p4adx_set_limit(P4Adx *a, int frames) { a->limit = frames; }

void p4adx_close(P4Adx *a) {
    if (!a) return;
    avcodec_free_context(&a->ctx);
    av_packet_free(&a->pkt);
    av_frame_free(&a->frame);
    free(a->in);
    free(a->pcm);
    pthread_mutex_destroy(&a->lock);
    free(a);
}

int p4adx_sample_rate(const P4Adx *a) { return a->header_done ? a->rate : 0; }
int p4adx_channels(const P4Adx *a) { return a->header_done ? a->channels : 0; }
int p4adx_available(P4Adx *a) {
    pthread_mutex_lock(&a->lock);
    int n = a->pcm_frames;
    pthread_mutex_unlock(&a->lock);
    return n;
}
int64_t p4adx_total(P4Adx *a) {
    pthread_mutex_lock(&a->lock);
    int64_t n = a->total;
    pthread_mutex_unlock(&a->lock);
    return n;
}

static int be16(const uint8_t *p) { return (p[0] << 8) | p[1]; }
static int be32(const uint8_t *p) { return (int)(((uint32_t)p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3]); }

static int open_decoder(P4Adx *a, const uint8_t *hdr, int hdr_size) {
    const AVCodec *c = avcodec_find_decoder(AV_CODEC_ID_ADPCM_ADX);
    if (!c) return -1;
    a->ctx = avcodec_alloc_context3(c);
    a->pkt = av_packet_alloc();
    a->frame = av_frame_alloc();
    if (!a->ctx || !a->pkt || !a->frame) return -1;
    a->ctx->sample_rate = a->rate;
    av_channel_layout_default(&a->ctx->ch_layout, a->channels);
    a->ctx->block_align = BLOCK * a->channels;
    /* The decoder takes the coefficients and layout from the file header as extradata. */
    a->ctx->extradata = av_mallocz(hdr_size + AV_INPUT_BUFFER_PADDING_SIZE);
    if (!a->ctx->extradata) return -1;
    memcpy(a->ctx->extradata, hdr, hdr_size);
    a->ctx->extradata_size = hdr_size;
    return avcodec_open2(a->ctx, c, NULL);
}

static int store_locked(P4Adx *a, const AVFrame *f, int n, int ch);
static int store(P4Adx *a, const AVFrame *f) {
    const int n = f->nb_samples, ch = a->channels;
    pthread_mutex_lock(&a->lock);
    int rc = store_locked(a, f, n, ch);
    pthread_mutex_unlock(&a->lock);
    return rc;
}

static int store_locked(P4Adx *a, const AVFrame *f, int n, int ch) {
    if (a->pcm_frames + n > a->pcm_cap) {
        int ncap = (a->pcm_frames + n) * 2 + 4096;
        int16_t *np = realloc(a->pcm, (size_t)ncap * ch * sizeof *np);
        if (!np) return -1;
        a->pcm = np;
        a->pcm_cap = ncap;
    }
    int16_t *o = a->pcm + (size_t)a->pcm_frames * ch;
    for (int i = 0; i < n; i++)
        for (int c = 0; c < ch; c++) {
            int16_t v = 0;
            if (f->format == AV_SAMPLE_FMT_S16P) v = ((const int16_t *)f->data[c])[i];
            else if (f->format == AV_SAMPLE_FMT_S16) v = ((const int16_t *)f->data[0])[i * ch + c];
            o[i * ch + c] = v;
        }
    a->pcm_frames += n;
    a->total += n;
    if (a->limit > 0 && a->pcm_frames > a->limit * 2) {
        /* Nobody is pulling: forget the oldest half so the queue stays bounded. */
        int drop = a->pcm_frames - a->limit;
        memmove(a->pcm, a->pcm + (size_t)drop * ch, (size_t)(a->pcm_frames - drop) * ch * sizeof *a->pcm);
        a->pcm_frames -= drop;
    }
    return 0;
}

int p4adx_feed(P4Adx *a, const uint8_t *data, int size) {
    uint64_t trace_start = p4pc_prof_span_begin();
    if (a->in_len + size > a->in_cap) {
        int ncap = (a->in_len + size) * 2;
        uint8_t *n = realloc(a->in, ncap);
        if (!n) {
            p4pc_prof_span_end(trace_start, "audio.adx.feed", size, -1, (uintptr_t)a, NULL);
            return -1;
        }
        a->in = n;
        a->in_cap = ncap;
    }
    memcpy(a->in + a->in_len, data, size);
    a->in_len += size;
    int pos = 0;
    if (!a->header_done) {
        if (a->in_len < HDR_MIN) {
            p4pc_prof_span_end(trace_start, "audio.adx.feed", size, 0, (uintptr_t)a, NULL);
            return 0;
        }
        if (a->in[0] != 0x80 || a->in[1] != 0x00 || a->in[4] != 3 || a->in[5] != BLOCK || a->in[6] != 4) {
            p4pc_prof_span_end(trace_start, "audio.adx.feed", size, -1, (uintptr_t)a, NULL);
            return -1;
        }
        a->hdr_size = be16(a->in + 2) + 4;
        a->channels = a->in[7];
        a->rate = be32(a->in + 8);
        if (a->channels < 1 || a->channels > MAX_CH || a->rate <= 0 || a->hdr_size > 0x10000) {
            p4pc_prof_span_end(trace_start, "audio.adx.feed", size, -1, (uintptr_t)a, NULL);
            return -1;
        }
        if (a->in_len < a->hdr_size) {
            p4pc_prof_span_end(trace_start, "audio.adx.feed", size, 0, (uintptr_t)a, NULL);
            return 0;
        }
        if (open_decoder(a, a->in, a->hdr_size) < 0) {
            p4pc_prof_span_end(trace_start, "audio.adx.feed", size, -1, (uintptr_t)a, NULL);
            return -1;
        }
        a->header_done = 1;
        pos = a->hdr_size;
    }
    const int frame_bytes = BLOCK * a->channels;
    int ret = 0;
    /* Whole frames only; an ADX end marker frame (first bytes 80 01) is not audio. */
    while (a->in_len - pos >= frame_bytes) {
        int n = (a->in_len - pos) / frame_bytes;
        if (n > 1500) n = 1500;
        int take = 0;
        while (take < n && !(a->in[pos + take * frame_bytes] == 0x80 && a->in[pos + take * frame_bytes + 1] == 0x01)) take++;
        if (take == 0) { pos = a->in_len; break; }   /* end marker */
        if (av_new_packet(a->pkt, take * frame_bytes) < 0) {
            p4pc_prof_span_end(trace_start, "audio.adx.feed", size, -1, (uintptr_t)a, NULL);
            return -1;
        }
        memcpy(a->pkt->data, a->in + pos, (size_t)take * frame_bytes);
        int r = avcodec_send_packet(a->ctx, a->pkt);
        av_packet_unref(a->pkt);
        pos += take * frame_bytes;
        if (r < 0) { ret = -1; continue; }
        while (avcodec_receive_frame(a->ctx, a->frame) >= 0) {
            if (store(a, a->frame) < 0) ret = -1;
            av_frame_unref(a->frame);
        }
    }
    if (pos > 0) {
        memmove(a->in, a->in + pos, a->in_len - pos);
        a->in_len -= pos;
    }
    p4pc_prof_span_end(trace_start, "audio.adx.feed", size, ret, (uintptr_t)a, NULL);
    return ret;
}

int p4adx_pull(P4Adx *a, int16_t *dst, int max_frames) {
    uint64_t trace_start = p4pc_prof_span_begin();
    pthread_mutex_lock(&a->lock);
    int n = a->pcm_frames < max_frames ? a->pcm_frames : max_frames;
    if (n <= 0) {
        pthread_mutex_unlock(&a->lock);
        p4pc_prof_span_end(trace_start, "audio.adx.pull", max_frames, 0, (uintptr_t)a, NULL);
        return 0;
    }
    memcpy(dst, a->pcm, (size_t)n * a->channels * sizeof *dst);
    a->pcm_frames -= n;
    memmove(a->pcm, a->pcm + (size_t)n * a->channels, (size_t)a->pcm_frames * a->channels * sizeof *a->pcm);
    pthread_mutex_unlock(&a->lock);
    p4pc_prof_span_end(trace_start, "audio.adx.pull", max_frames, n, (uintptr_t)a, NULL);
    return n;
}

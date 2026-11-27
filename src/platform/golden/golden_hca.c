/* FFmpeg 7.1.1 HCA adapter, no libavformat and no key discovery. */
#include "p4pc/golden_audio.h"
#include <libavcodec/avcodec.h>
#include <libavutil/channel_layout.h>
#include <libavutil/mem.h>
#include <libavutil/samplefmt.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct P4GoldenHca {
    P4GoldenHcaInfo info;
    const uint8_t *bytes;
    AVCodecContext *codec;
    AVPacket *packet;
    AVFrame *frame;
    uint32_t block, position, frame_start, frame_used;
    int failed;
};
static int fail(char *reason, size_t capacity, const char *text, int code)
{
    if (reason && capacity) snprintf(reason, capacity, "%s", text);
    return code;
}
int p4_golden_hca_open(P4GoldenHca **out, const uint8_t *bytes, size_t size,
                       uint64_t user_key, uint16_t subkey, char *reason, size_t capacity)
{
    P4GoldenHcaInfo info;
    const AVCodec *codec;
    P4GoldenHca *h;
    if (!out) return -1;
    *out = NULL;
    if (p4_golden_hca_info(bytes, size, &info)) return fail(reason, capacity, "invalid HCA header or extent", -1);
    if (info.cipher == 56 && !user_key) return fail(reason, capacity, "keyed HCA requires the private golden.hca_key setting", -2);
    /* FFmpeg 7.1.1 implements cipher 0/56, not keyless cipher 1. */
    if (info.cipher == 1) return fail(reason, capacity, "HCA cipher 1 is not supported by FFmpeg 7.1.1", -1);
    codec = avcodec_find_decoder(AV_CODEC_ID_HCA);
    if (!codec) return fail(reason, capacity, "FFmpeg was built without the HCA decoder", -1);
    h = calloc(1, sizeof *h);
    if (!h) return fail(reason, capacity, "HCA allocation failed", -1);
    h->info = info;
    h->bytes = bytes;
    h->codec = avcodec_alloc_context3(codec);
    h->packet = av_packet_alloc();
    h->frame = av_frame_alloc();
    if (!h->codec || !h->packet || !h->frame) goto bad;
    h->codec->sample_rate = (int)info.rate;
    av_channel_layout_default(&h->codec->ch_layout, (int)info.channels);
    h->codec->block_align = info.block_size;
    h->codec->err_recognition = AV_EF_CRCCHECK | AV_EF_EXPLODE;
    h->codec->thread_count = 1;
    h->codec->extradata_size = info.header_size + 10;
    h->codec->extradata = av_mallocz((size_t)h->codec->extradata_size + AV_INPUT_BUFFER_PADDING_SIZE);
    if (!h->codec->extradata) goto bad;
    memcpy(h->codec->extradata, bytes, info.header_size);
    uint8_t *key = h->codec->extradata + info.header_size;
    if (info.cipher != 56) { user_key = 0; subkey = 0; }
    for (unsigned i = 0; i < 8; ++i) key[i] = (uint8_t)(user_key >> ((7 - i) * 8));
    key[8] = (uint8_t)(subkey >> 8); key[9] = (uint8_t)subkey;
    *(volatile uint64_t *)&user_key = 0;
    if (avcodec_open2(h->codec, codec, NULL) < 0 || av_new_packet(h->packet, info.block_size) < 0) goto bad;
    *out = h;
    if (reason && capacity) *reason = 0;
    return 0;
bad:
    p4_golden_hca_close(h);
    return fail(reason, capacity, "HCA decoder initialization failed", -1);
}
const P4GoldenHcaInfo *p4_golden_hca_format(const P4GoldenHca *h) { return h ? &h->info : NULL; }
static int next_frame(P4GoldenHca *h)
{
    if (h->block >= h->info.blocks) return 0;
    if (av_packet_make_writable(h->packet) < 0) return -1;
    memcpy(h->packet->data, h->bytes + h->info.header_size + (size_t)h->block * h->info.block_size, h->info.block_size);
    av_frame_unref(h->frame);
    if (avcodec_send_packet(h->codec, h->packet) < 0 || avcodec_receive_frame(h->codec, h->frame) < 0 ||
        h->frame->nb_samples != 1024 || h->frame->format != AV_SAMPLE_FMT_FLTP ||
        h->frame->ch_layout.nb_channels != (int)h->info.channels) return -1;
    h->frame_start = h->block++ * 1024u;
    h->frame_used = 0;
    return 1;
}
int p4_golden_hca_pull(P4GoldenHca *h, int16_t *pcm, uint32_t capacity)
{
    uint32_t written = 0;
    if (!h || !pcm || capacity > INT32_MAX || h->failed) return -1;
    while (written < capacity && h->position < h->info.frames) {
        uint32_t absolute = h->position + h->info.delay;
        if (!h->frame->nb_samples || absolute >= h->frame_start + 1024u) {
            int result = next_frame(h);
            if (result <= 0) { h->failed = 1; return -1; }
            if (absolute >= h->frame_start + 1024u) continue;
        }
        h->frame_used = absolute - h->frame_start;
        uint32_t n = 1024u - h->frame_used;
        if (n > h->info.frames - h->position) n = h->info.frames - h->position;
        if (n > capacity - written) n = capacity - written;
        for (uint32_t c = 0; c < h->info.channels; ++c) {
            const float *src = (const float *)h->frame->extended_data[c] + h->frame_used;
            for (uint32_t i = 0; i < n; ++i) {
                double value = (double)src[i] * h->info.volume * 32768.0;
                if (!isfinite(value)) { h->failed = 1; return -1; }
                pcm[((size_t)written + i) * h->info.channels + c] =
                    value <= -32768 ? -32768 : value >= 32767 ? 32767 : (int16_t)lrint(value);
            }
        }
        written += n;
        h->position += n;
    }
    return (int)written;
}
int p4_golden_hca_seek(P4GoldenHca *h, uint32_t sample)
{
    if (!h || h->failed || sample > h->info.frames) return -1;
    avcodec_flush_buffers(h->codec);
    av_frame_unref(h->frame);
    h->block = (sample + h->info.delay) / 1024u;
    /* HCA's IMDCT overlap depends on one preceding block. */
    if (h->block) --h->block;
    h->position = sample;
    return 0;
}
void p4_golden_hca_close(P4GoldenHca *h)
{
    if (!h) return;
    if (h->codec && h->codec->extradata && h->codec->extradata_size >= 10) {
        volatile uint8_t *key = h->codec->extradata + h->codec->extradata_size - 10;
        for (unsigned i = 0; i < 10; ++i) key[i] = 0;
    }
    av_frame_free(&h->frame);
    av_packet_free(&h->packet);
    avcodec_free_context(&h->codec);
    free(h);
}

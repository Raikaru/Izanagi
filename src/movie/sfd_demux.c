/* MPEG program-stream demuxer over the PC VFS. See sfd_demux.h. */
#include "sfd_demux.h"

#include <stdlib.h>
#include <string.h>

#include "vfs.h"

#define READ_CHUNK (256 * 1024)
/* A PES packet is at most 6 + 65535 bytes; the buffer holds that plus one read chunk. */
#define BUF_SIZE (READ_CHUNK + 65536 + 64)

struct SfdDemux {
    int fd;
    int eof;
    int failed;         /* a read error ended the stream before the end of the file */
    uint8_t *buf;
    int start;          /* first unconsumed byte in buf */
    int end;            /* one past the last valid byte */
    long long base;     /* file offset of buf[0] */
    int mpeg2;          /* -1 unknown */
    int mux_rate;
    int64_t scr;
};

SfdDemux *sfd_demux_open(const char *path) {
    int fd = vfs_open(path, 0);
    if (fd < 0) return NULL;
    SfdDemux *d = calloc(1, sizeof *d);
    if (!d) { vfs_close(fd); return NULL; }
    d->buf = malloc(BUF_SIZE);
    if (!d->buf) { free(d); vfs_close(fd); return NULL; }
    d->fd = fd;
    d->mpeg2 = -1;
    return d;
}

void sfd_demux_close(SfdDemux *d) {
    if (!d) return;
    vfs_close(d->fd);
    free(d->buf);
    free(d);
}

int sfd_demux_is_mpeg2(const SfdDemux *d) { return d->mpeg2; }
int sfd_demux_failed(const SfdDemux *d) { return d->failed; }
int sfd_demux_mux_rate(const SfdDemux *d) { return d->mux_rate; }

/* Makes at least n bytes available at buf[start]; returns how many are (fewer only at EOF). */
static int ensure(SfdDemux *d, int n) {
    if (d->end - d->start >= n || d->eof) return d->end - d->start;
    if (d->start > 0) {
        memmove(d->buf, d->buf + d->start, d->end - d->start);
        d->base += d->start;
        d->end -= d->start;
        d->start = 0;
    }
    while (d->end - d->start < n && !d->eof) {
        int room = BUF_SIZE - d->end;
        if (room > READ_CHUNK) room = READ_CHUNK;
        if (room <= 0) break;
        int got = vfs_read(d->fd, d->buf + d->end, room);
        if (got < 0) d->failed = 1;
        if (got <= 0) { d->eof = 1; break; }
        d->end += got;
    }
    return d->end - d->start;
}

static int64_t read_ts(const uint8_t *p) {
    return ((int64_t)((p[0] >> 1) & 7) << 30) | ((int64_t)p[1] << 22) | ((int64_t)(p[2] >> 1) << 15) |
           ((int64_t)p[3] << 7) | (p[4] >> 1);
}

int sfd_demux_next(SfdDemux *d, SfdPacket *out) {
    for (;;) {
        int have = ensure(d, 16);
        if (have < 4) return 0;
        const uint8_t *b = d->buf + d->start;
        if (b[0] != 0 || b[1] != 0 || b[2] != 1 || b[3] < 0xB9) {
            d->start++;   /* resync: skip to the next start code */
            continue;
        }
        int id = b[3];
        if (id == 0xB9) return 0;
        if (id == 0xBA) {
            if ((b[4] & 0xC0) == 0x40) {
                if (have < 14) return 0;
                int pack_size = 14 + (b[13] & 7);
                if (ensure(d, pack_size) < pack_size) return 0;
                b = d->buf + d->start;
                d->mpeg2 = 1;
                d->scr = ((int64_t)((b[4] >> 3) & 7) << 30) | ((int64_t)(b[4] & 3) << 28) | ((int64_t)b[5] << 20) |
                         ((int64_t)((b[6] >> 3) & 0x1F) << 15) | ((int64_t)(b[6] & 3) << 13) | ((int64_t)b[7] << 5) |
                         (b[8] >> 3);
                d->mux_rate = (b[10] << 14) | (b[11] << 6) | (b[12] >> 2);
                d->start += pack_size;
            } else {
                if (have < 12) return 0;
                d->mpeg2 = 0;
                d->scr = ((int64_t)((b[4] >> 1) & 7) << 30) | ((int64_t)b[5] << 22) | ((int64_t)(b[6] >> 1) << 15) |
                         ((int64_t)b[7] << 7) | (b[8] >> 1);
                d->mux_rate = ((b[9] & 0x7F) << 15) | (b[10] << 7) | (b[11] >> 1);
                d->start += 12;
            }
            continue;
        }
        int len = (b[4] << 8) | b[5];
        int total = 6 + len;
        if (ensure(d, total) < total) return 0;
        b = d->buf + d->start;
        long long off = d->base + d->start;
        d->start += total;
        if (id == 0xBB || id == 0xBC || id == 0xBE || id == 0xBF || id < 0xBD) continue;
        /* PES: 0xBD private stream 1, 0xC0..0xDF audio, 0xE0..0xEF video. */
        const uint8_t *p = b + 6;
        const uint8_t *e = b + total;
        int has_pts = 0, has_dts = 0;
        int64_t pts = 0, dts = 0;
        if ((p[0] & 0xC0) == 0x80) {
            if (total < 9) continue;
            int flags = p[1], hl = p[2];
            if (9 + hl > len + 3) continue;
            if ((flags & 0x80) && hl >= 5) { has_pts = 1; pts = read_ts(p + 3); }
            if ((flags & 0x40) && hl >= 10) { has_dts = 1; dts = read_ts(p + 8); }
            p += 3 + hl;
        } else {
            while (p < e && *p == 0xFF) p++;
            if (p < e && (*p & 0xC0) == 0x40) p += 2;
            if (p < e && (*p & 0xF0) == 0x20) { has_pts = 1; pts = read_ts(p); p += 5; }
            else if (p < e && (*p & 0xF0) == 0x30) { has_pts = 1; pts = read_ts(p); has_dts = 1; dts = read_ts(p + 5); p += 10; }
            else if (p < e) p++;
        }
        if (p > e) continue;
        out->stream_id = id;
        out->data = p;
        out->size = (int)(e - p);
        out->has_pts = has_pts;
        out->pts = pts;
        out->has_dts = has_dts;
        out->dts = dts;
        out->scr = d->scr;
        out->offset = off;
        return 1;
    }
}

/* p4pc_sfdview: opens an SFD movie from the retail disc through the port VFS, demuxes the MPEG
 * program stream, decodes pictures with libavcodec and writes PNGs.
 *
 *   p4pc_sfdview --list                          list the disc's movie files
 *   p4pc_sfdview <disc path> <out dir> [N] [step] decode N pictures (default 8), write every step-th
 *
 * The disc comes from P4PC_ISO (the loose-file override directory from P4PC_OVERRIDE_DIR). It
 * reports the stream layout, the sequence/extension headers it saw over the whole file (frame
 * size and rate, picture counts by type, field order, progressive flags) and what libavcodec
 * decoded, so the two can be compared by eye. Exits 77 when P4PC_ISO is unset.
 *
 * This tool demuxes with its own program-stream parser. The game demuxes with CRI's Sofdec code. */
#define _FILE_OFFSET_BITS 64
#define _POSIX_C_SOURCE 200809L
#include <strings.h>

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "p4pc/os.h"
#include "mpeg2_dec.h"
#include "p4png.h"
#include "sfd_demux.h"
#include "vfs.h"

#define SKIP_RETURN 77

/* ---- Movie listing ---------------------------------------------------------------------- */

static int is_movie_name(const char *name) {
    const char *dot = strrchr(name, '.');
    if (!dot) return 0;
    return !strcasecmp(dot, ".SFD") || !strcasecmp(dot, ".PSS") || !strcasecmp(dot, ".MPG") ||
           !strcasecmp(dot, ".M2V") || !strcasecmp(dot, ".MPEG");
}

static int list_dir(const char *path, int depth, int *count) {
    int dh = vfs_dir_open(path);
    if (dh < 0) return -1;
    char name[256];
    int is_dir;
    long long size;
    /* Collect first: the directory handle table is small, so recurse after closing it. */
    char (*dirs)[256] = NULL;
    int ndirs = 0, cap = 0;
    while (vfs_dir_next(dh, name, sizeof name, &is_dir, &size) == 1) {
        if (!strcmp(name, ".") || !strcmp(name, "..")) continue;
        if (is_dir) {
            if (ndirs == cap) {
                cap = cap ? cap * 2 : 16;
                dirs = realloc(dirs, cap * sizeof *dirs);
            }
            snprintf(dirs[ndirs++], 256, "%s", name);
        } else if (is_movie_name(name)) {
            printf("%s%s%s\t%lld\n", path, path[0] && path[strlen(path) - 1] != '/' ? "/" : "", name, size);
            (*count)++;
        }
    }
    vfs_dir_close(dh);
    if (depth < 8) {
        for (int i = 0; i < ndirs; i++) {
            char sub[1024];
            snprintf(sub, sizeof sub, "%s%s%s", path, path[0] && path[strlen(path) - 1] != '/' ? "/" : "", dirs[i]);
            list_dir(sub, depth + 1, count);
        }
    }
    free(dirs);
    return 0;
}

/* ---- Elementary-stream header scan ------------------------------------------------------ */

typedef struct EsScan {
    uint32_t win;       /* last four ES bytes */
    int code;           /* start code being collected, -1 none */
    int len;
    uint8_t cur[16];
    /* results */
    int seq_headers, seq_changes;
    int width, height, aspect, rate_code;
    int bit_rate;       /* 400 bit/s units */
    int have_seqext, progressive_sequence, chroma_format, profile_level;
    int gops, closed_gops, broken_links;
    long pictures, by_type[8];
    long pce;           /* picture coding extensions seen */
    long structure[4];  /* picture_structure 1 top, 2 bottom, 3 frame */
    long tff, rff, progressive_frame;
    int first_pic_tff, first_pic_prog, first_pic_struct;
    int seen_first_pce;
} EsScan;

static void es_finish(EsScan *s) {
    const uint8_t *c = s->cur;
    switch (s->code) {
    case 0xB3:
        if (s->len < 8) break;
        {
            int w = (c[0] << 4) | (c[1] >> 4), h = ((c[1] & 15) << 8) | c[2];
            int ar = c[3] >> 4, fr = c[3] & 15;
            int br = (c[4] << 10) | (c[5] << 2) | (c[6] >> 6);
            if (s->seq_headers && (w != s->width || h != s->height || fr != s->rate_code)) s->seq_changes++;
            if (!s->seq_headers) {
                s->width = w; s->height = h; s->aspect = ar; s->rate_code = fr; s->bit_rate = br;
            }
            s->seq_headers++;
        }
        break;
    case 0xB5:
        if (s->len < 1) break;
        if ((c[0] >> 4) == 1 && s->len >= 6) {           /* sequence extension */
            if (!s->have_seqext) {
                s->have_seqext = 1;
                s->profile_level = ((c[0] & 15) << 4) | (c[1] >> 4);
                s->progressive_sequence = (c[1] >> 3) & 1;
                s->chroma_format = (c[1] >> 1) & 3;
            }
        } else if ((c[0] >> 4) == 8 && s->len >= 5) {    /* picture coding extension */
            int structure = c[2] & 3, tff = c[3] >> 7, rff = (c[3] >> 1) & 1, prog = c[4] >> 7;
            s->pce++;
            s->structure[structure]++;
            s->tff += tff;
            s->rff += rff;
            s->progressive_frame += prog;
            if (!s->seen_first_pce) {
                s->seen_first_pce = 1;
                s->first_pic_tff = tff; s->first_pic_prog = prog; s->first_pic_struct = structure;
            }
        }
        break;
    case 0xB8:
        if (s->len < 4) break;
        s->gops++;
        s->closed_gops += (c[3] >> 6) & 1;
        s->broken_links += (c[3] >> 5) & 1;
        break;
    case 0x00:
        if (s->len < 2) break;
        s->pictures++;
        s->by_type[(c[1] >> 3) & 7]++;
        break;
    }
    s->code = -1;
}

static void es_push(EsScan *s, const uint8_t *p, int n) {
    for (int i = 0; i < n; i++) {
        s->win = (s->win << 8) | p[i];
        if ((s->win & 0xFFFFFF00u) == 0x00000100u) {
            if (s->code >= 0) es_finish(s);
            s->code = (int)(s->win & 0xFF);
            s->len = 0;
        } else if (s->code >= 0 && s->len < (int)sizeof s->cur) {
            s->cur[s->len++] = p[i];
            if (s->len == (int)sizeof s->cur) es_finish(s);
        }
    }
}

static const char *const RATE_NAME[] = {"forbidden", "23.976", "24", "25", "29.97", "30", "50", "59.94", "60"};
static const double RATE_VAL[] = {0, 24000 / 1001.0, 24, 25, 30000 / 1001.0, 30, 50, 60000 / 1001.0, 60};

int main(int argc, char **argv) {
    const char *iso = getenv("P4PC_ISO");
    if (!iso || !*iso) {
        fprintf(stderr, "p4pc_sfdview: P4PC_ISO is not set\n");
        return SKIP_RETURN;
    }
    if (vfs_mount(iso, getenv("P4PC_OVERRIDE_DIR")) < 0) {
        fprintf(stderr, "p4pc_sfdview: cannot mount %s\n", iso);
        return 1;
    }
    if (argc >= 2 && !strcmp(argv[1], "--list")) {
        int n = 0;
        list_dir("", 0, &n);
        fprintf(stderr, "%d movie file(s)\n", n);
        return n ? 0 : 1;
    }
    if (argc < 3) {
        fprintf(stderr, "usage: p4pc_sfdview --list | <disc path> <out dir> [N] [step]\n");
        return 2;
    }
    const char *path = argv[1], *outdir = argv[2];
    int want = argc > 3 ? atoi(argv[3]) : 8;
    int step = argc > 4 ? atoi(argv[4]) : 1;
    if (want < 0) want = 0;
    if (step < 1) step = 1;
    {
        char tmp[1024];
        snprintf(tmp, sizeof tmp, "%s", outdir);
        for (char *p = tmp + 1; *p; p++) {
            if (*p == '/') { *p = 0; p4_os_mkdir(tmp); *p = '/'; }
        }
        p4_os_mkdir(tmp);
    }

    SfdDemux *dm = sfd_demux_open(path);
    if (!dm) {
        fprintf(stderr, "p4pc_sfdview: cannot open %s\n", path);
        return 1;
    }
    P4Mpeg2 *dec = p4mpeg2_open();
    if (!dec) {
        fprintf(stderr, "p4pc_sfdview: cannot open the MPEG-2 decoder\n");
        return 1;
    }

    /* Per-stream accounting. */
    struct { long packets; long long bytes; int has_pts; int64_t first_pts, last_pts; int64_t first_scr, last_scr; } st[256];
    memset(st, 0, sizeof st);
    EsScan es;
    memset(&es, 0, sizeof es);
    es.code = -1;
    unsigned char aud_head[4][16];
    int aud_id[4];
    int naud_head = 0;

    uint8_t *rgba = NULL;
    size_t rgba_cap = 0;
    int got = 0, written = 0;
    int first_decoded_logged = 0;
    long decoded_interlaced = 0, decoded_tff = 0, decoded_repeat = 0;
    int64_t prev_pts = INT64_MIN;
    int pts_monotonic = 1;
    SfdPacket pk;
    while (sfd_demux_next(dm, &pk)) {
        int id = pk.stream_id;
        if (!st[id].packets) st[id].first_scr = pk.scr;
        st[id].last_scr = pk.scr;
        st[id].packets++;
        st[id].bytes += pk.size;
        if (pk.has_pts) {
            if (!st[id].has_pts) { st[id].has_pts = 1; st[id].first_pts = pk.pts; }
            st[id].last_pts = pk.pts;
        }
        if (id != SFD_STREAM_VIDEO0 && naud_head < 4 && pk.size >= 16) {
            aud_id[naud_head] = id;
            memcpy(aud_head[naud_head++], pk.data, 16);
        }
        if (id != SFD_STREAM_VIDEO0) continue;
        es_push(&es, pk.data, pk.size);
        if (got < want) {
            if (p4mpeg2_feed(dec, pk.data, pk.size, pk.has_pts ? pk.pts : INT64_MIN) < 0) {
                fprintf(stderr, "p4pc_sfdview: decode error near file offset %lld\n", pk.offset);
                return 1;
            }
            P4Mpeg2Frame f;
            while (got < want && p4mpeg2_receive(dec, &f)) {
                if (!first_decoded_logged) {
                    first_decoded_logged = 1;
                    printf("decoded: %dx%d, 4:2:0, color_range %d, colorspace %d\n", f.width, f.height, f.color_range,
                           f.colorspace);
                }
                decoded_interlaced += f.interlaced;
                decoded_tff += f.top_field_first;
                decoded_repeat += f.repeat_pict;
                if (f.pts != INT64_MIN && prev_pts != INT64_MIN && f.pts < prev_pts) pts_monotonic = 0;
                if (f.pts != INT64_MIN) prev_pts = f.pts;
                if (got % step == 0) {
                    size_t need = (size_t)f.width * f.height * 4;
                    if (need > rgba_cap) { rgba = realloc(rgba, need); rgba_cap = need; }
                    char out[1024];
                    snprintf(out, sizeof out, "%s/frame_%04d.png", outdir, got);
                    if (p4mpeg2_to_rgba(&f, rgba) < 0 || p4png_write_rgba(out, f.width, f.height, rgba) < 0) {
                        fprintf(stderr, "p4pc_sfdview: cannot write %s\n", out);
                        return 1;
                    }
                    printf("frame %d: type %c pts %" PRId64 "%s%s -> %s\n", got, " IPBD"[f.pict_type & 7 ? f.pict_type : 0],
                           f.pts == INT64_MIN ? -1 : f.pts, f.interlaced ? " interlaced" : " progressive",
                           f.interlaced ? (f.top_field_first ? " tff" : " bff") : "", out);
                    written++;
                }
                got++;
            }
        }
    }
    if (got < want) {
        p4mpeg2_flush(dec);
        P4Mpeg2Frame f;
        while (got < want && p4mpeg2_receive(dec, &f)) {
            if (got % step == 0) {
                size_t need = (size_t)f.width * f.height * 4;
                if (need > rgba_cap) { rgba = realloc(rgba, need); rgba_cap = need; }
                char out[1024];
                snprintf(out, sizeof out, "%s/frame_%04d.png", outdir, got);
                if (p4mpeg2_to_rgba(&f, rgba) == 0 && p4png_write_rgba(out, f.width, f.height, rgba) == 0) {
                    printf("frame %d (flushed): pts %" PRId64 " -> %s\n", got, f.pts == INT64_MIN ? -1 : f.pts, out);
                    written++;
                }
            }
            got++;
        }
    }
    if (es.code >= 0) es_finish(&es);

    printf("file: %s (%s program stream, mux rate %d x 50 B/s = %d kbit/s)\n", path,
           sfd_demux_is_mpeg2(dm) == 1 ? "MPEG-2" : "MPEG-1", sfd_demux_mux_rate(dm), sfd_demux_mux_rate(dm) * 50 * 8 / 1000);
    for (int id = 0; id < 256; id++) {
        if (!st[id].packets) continue;
        printf("stream 0x%02X: %ld packets, %lld bytes", id, st[id].packets, st[id].bytes);
        if (st[id].has_pts)
            printf(", PTS %" PRId64 " .. %" PRId64 " (%.3f s .. %.3f s)", st[id].first_pts, st[id].last_pts,
                   st[id].first_pts / 90000.0, st[id].last_pts / 90000.0);
        printf(", SCR %.3f s .. %.3f s\n", st[id].first_scr / 90000.0, st[id].last_scr / 90000.0);
    }
    for (int i = 0; i < naud_head; i++) {
        printf("stream 0x%02X payload head %d:", aud_id[i], i);
        for (int k = 0; k < 16; k++) printf(" %02x", aud_head[i][k]);
        printf("\n");
    }
    double rate = es.rate_code >= 1 && es.rate_code <= 8 ? RATE_VAL[es.rate_code] : 0;
    printf("sequence header: %dx%d, aspect code %d, frame rate code %d (%s fps), bit rate %d x 400 b/s, %d header(s), %d change(s)\n",
           es.width, es.height, es.aspect, es.rate_code, es.rate_code <= 8 ? RATE_NAME[es.rate_code] : "?", es.bit_rate,
           es.seq_headers, es.seq_changes);
    if (es.have_seqext)
        printf("sequence extension: profile/level 0x%02x, progressive_sequence %d, chroma_format %d (1 = 4:2:0)\n",
               es.profile_level, es.progressive_sequence, es.chroma_format);
    printf("GOPs %d (closed %d, broken link %d); pictures %ld: I %ld, P %ld, B %ld\n", es.gops, es.closed_gops,
           es.broken_links, es.pictures, es.by_type[1], es.by_type[2], es.by_type[3]);
    printf("picture coding extensions %ld: structure frame %ld, top %ld, bottom %ld; top_field_first %ld, repeat_first_field %ld, progressive_frame %ld\n",
           es.pce, es.structure[3], es.structure[1], es.structure[2], es.tff, es.rff, es.progressive_frame);
    if (es.seen_first_pce)
        printf("first picture: structure %d, top_field_first %d, progressive_frame %d\n", es.first_pic_struct,
               es.first_pic_tff, es.first_pic_prog);
    if (rate > 0 && es.pictures > 0) {
        double video_secs = es.pictures / rate;
        printf("frame count %ld at %.3f fps = %.3f s of video", es.pictures, rate, video_secs);
        if (st[SFD_STREAM_VIDEO0].has_pts && st[SFD_STREAM_VIDEO0].last_pts > st[SFD_STREAM_VIDEO0].first_pts)
            printf("; video PTS span %.3f s (first to last PES, so about the last GOP short)",
                   (st[SFD_STREAM_VIDEO0].last_pts - st[SFD_STREAM_VIDEO0].first_pts) / 90000.0);
        printf("\n");
    }
    int num, den;
    p4mpeg2_frame_rate(dec, &num, &den);
    printf("libavcodec: frame rate %d/%d; decoded %d picture(s), %d PNG(s); interlaced %ld, top-field-first %ld, repeat_pict %ld, display PTS monotonic %s\n",
           num, den, got, written, decoded_interlaced, decoded_tff, decoded_repeat, pts_monotonic ? "yes" : "no");
    free(rgba);
    p4mpeg2_close(dec);
    sfd_demux_close(dm);
    return got > 0 || want == 0 ? 0 : 1;
}

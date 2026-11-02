/* mwply: the movie player the game talks to (CRI Sofdec's MWPLY API), on libavcodec.
 *
 * The game plays a movie through ten calls (see movieNativeTaskUpdate, decomp
 * src/Kosaka/Field/k_fldrc_inferred_00468d10.c): mwPlyInitSfdFx at boot, mwPlyCalcWorkCprmSfd and
 * func_00509268 (create) with an MWSFD_CRPRM, mwPlySetFrmSync, H_Cdvd_StartPlayback (-> start with a
 * file name), then once per task update mwPlyGetCurFrm / mwPlyRelCurFrm and func_0050d4d0 (status),
 * and mwPlyReleaseLp / func_005097e8 (destroy) at the end. Retail Sofdec implements them over a few
 * hundred functions (SFD ring buffers, MPS demux, MPV headers, the IPU through src/movie/sfmpvp.c,
 * ADX audio, a server thread). This file is that API on the port's own parts: the SFD demuxer
 * (sfd_demux.c), libavcodec (mpeg2_dec.c, adx_dec.c) and a frame schedule over
 * the PC VFS; it does not route movie bytes through the CRI audio stream rings.
 *
 * What the game observes, measured on retail P4CTOP1 and P4CT001 in PCSX2 (the trace tooling's movie trace,
 * docs/movie_player.md): the task updates 29.97 times a second, once per two vsyncs, and the movie is
 * 29.97 fps, so every update after the first frame gets the next frame number (+1). The first frame comes
 * PREP_CALLS updates after the start. Until ready, GetCurFrm returns no frame
 * and the status is 1 (prepare). The status is 2 from the first frame on and 3 after the last
 * frame has had its time; the game ends the task itself earlier when the table gives a fade-out.
 *
 * Pacing. The schedule is per update, not per wall-clock second, so a frame number is never skipped while
 * the game keeps up (the staff roll tests `mwPlayerGetCurrent() == 0xCC6` for equality). The target frame is
 * floor(clock * fps); with the target at most one frame ahead of or on the last frame shown the next
 * frame is returned, a target further ahead (the game ran late) skips to it like Sofdec's catch-up, and a
 * target behind the last frame (the game runs fast) returns nothing. The clock is, in order of
 * preference: the audio device's played-sample count when the audio lane reports one
 * (p4pc_movie_audio_played, mwply.h: video waits on audio as in Sofdec), one 1/29.97 s per GetCurFrm call
 * with P4PC_MOVIE_CLOCK=tick (deterministic tests and captures), or the monotonic clock.
 * Audio: the ADX track (stream 0xC0) is decoded to PCM as the demuxer reaches it and queued for the audio
 * lane, which has to pull it. */
#define _GNU_SOURCE
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "p4pc/gmem.h"
#include "p4pc/os.h"
#include "p4pc/platform.h"

#include "adx_dec.h"
#include "mpeg2_dec.h"
#include "mwply.h"
#include "sfd_demux.h"


/* MWSFD_CRPRM (0x30 bytes, decomp k_fldrc_inferred_00468d10.c) */
typedef struct {
    int32_t mode, max_bps, max_width, max_height, max_skip, nsec;
    uint32_t work; /* guest address */
    int32_t wksize, compo, buffmt, x28, x2c;
} Crprm;

_Static_assert(sizeof(Crprm) == 0x30, "MWSFD_CRPRM is 0x30 bytes");
_Static_assert(offsetof(Crprm, work) == 0x18, "MWSFD_CRPRM.work offset");
_Static_assert(offsetof(Crprm, wksize) == 0x1c, "MWSFD_CRPRM.wksize offset");

#define STAT_STOP 0
#define STAT_PREP 1
#define STAT_PLAYING 2
#define STAT_PLAYEND 3
#define STAT_ERROR 4

#define MAX_PLAYERS 8
/* GetCurFrm calls between the start and the first frame (retail P4CTOP1 and P4CT001: 20 updates). */
#define PREP_CALLS 20
/* Audio queued ahead of the video when an audio clock drives the movie. */
#define AUDIO_AHEAD_SECONDS 1

typedef struct MwPly {
    int used;
    Crprm prm;
    int stat;
    int loop;               /* loop player: restart at the end */
    int noskip;             /* mwPlySetFrmSync(h, 1): never drop frames */
    char path[256];         /* as the game gave it */
    SfdDemux *dm;
    P4Mpeg2 *dec;
    P4Adx *adx;             /* the ADX track (stream 0xC0), decoded as the demuxer reaches it */
    int eof;                /* demux exhausted, decoder flushed */
    int have_pending;
    P4Mpeg2Frame pending;
    int64_t pending_idx;
    double fps;
    int64_t next_idx;       /* display index of the next picture the decoder yields */
    int64_t delivered_idx;  /* last frame handed out (-1 none) */
    int started_clock;
    int64_t calls;          /* GetCurFrm calls since the start */
    double base;            /* raw clock at movie time 0 (the first delivered frame) */
    int src;                /* clock source `base` belongs to: 0 fallback, 1 audio, 2 audio after its end */
    double last_mt;         /* movie time at the last call */
    /* audio clock */
    int audio_clock;        /* the audio lane has reported played samples */
    uint64_t audio_played;
    int audio_done;         /* audio exhausted: the clock continues on the fallback */
    double audio_end_time, fb_at_done;
    uint8_t *buf;           /* the image buffer handed to the game: stamped, no pixels (see the stamp below) */
    size_t buf_cap;
    P4Mpeg2Frame shown;     /* the picture the stamp stands for: an owned reference, valid whatever the decoder does next */
    uint32_t serial;        /* the stamp of `shown`; 0 while none has been delivered */
    int mbw, mbh, width, height;
    int64_t nskipdisp;
} MwPly;

/* The players are guest memory: the handle the game gets (and keeps in its own structs) is a player's address. */
static MwPly *g_ply_tab;
static MwPly *ply_table(void) {
    if (g_ply_tab == NULL) g_ply_tab = (MwPly *)P4_G2H(p4_gport_alloc(sizeof(MwPly) * MAX_PLAYERS, 16));
    return g_ply_tab;
}
#define g_ply ply_table()

/* The image buffer the game gets is not a picture. Retail's holds the IPU's RGB32 macroblocks, which the game copies to its own
 * image buffer (movieNativeTaskUpdate: memcpy(work->imageBuffer, frame.buffer, w * h * 4)) and gsUploadRasterTiles later copies to the
 * GS; the port's upload turns them back into a linear image for OpenGL. Nothing else reads them, so the player does not build them:
 * the buffer carries a stamp (16 bytes: magic, player, serial, 0; the magic's fourth byte is not the IPU's alpha 0x80, so it cannot be
 * mistaken for a picture) that travels with the game's copy, and the upload converts the held picture, when it is drawn, straight to the
 * raster (movie_upload.c). A frame that is not drawn (P4PC_TURBO=2 outside the draw windows) costs no conversion at all. */
#define STAMP_MAGIC 0x564D3450u /* "P4MV" */

static int tiled_buffers(void) {
    static int on = -1;

    if (on < 0) {
        const char *e = getenv("P4PC_MOVIE_TILED");
        on = e && e[0] == '1';
    }
    return on;
}
static const P4Mpeg2Frame *stamp_resolve(const void *image);
static int g_tick_clock = -1;
static MwPly *g_audio_ply;

static MwPly *ply_from(void *h) {
    for (int i = 0; i < MAX_PLAYERS; i++)
        if (g_ply[i].used && &g_ply[i] == h) return &g_ply[i];
    return NULL;
}

static double now_s(void) {
    return (double)p4_os_now_ns() * 1e-9;
}

/* ---- Library entry points -------------------------------------------------------------------- */

/* mwPlyInitSfdFx(MWSFD_INIT_PRM *): Sofdec's library init (SFD, MPV, server threads). Not needed: the
 * player here has no library state. */
void p4pc_mwply_init(void *iprm) {
    (void)iprm;
    p4mpeg2_set_stamp_resolver(stamp_resolve);
}

/* mwPlyCalcWorkCprmSfd: bytes of work memory the game must supply. The player allocates its own. */
int32_t p4pc_mwply_calc_work(const void *prm) {
    (void)prm;
    return 0x1000;
}

/* func_00509268: create a player from MWSFD_CRPRM. Returns the handle, 0 on failure. */
void *func_00509268(const void *prm) {
    if (!prm) return NULL;
    p4mpeg2_set_stamp_resolver(stamp_resolve);
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (g_ply[i].used) continue;
        memset(&g_ply[i], 0, sizeof g_ply[i]);
        g_ply[i].used = 1;
        memcpy(&g_ply[i].prm, prm, sizeof(Crprm));
        g_ply[i].stat = STAT_STOP;
        g_ply[i].delivered_idx = -1;
        g_ply[i].fps = 30000.0 / 1001.0;
        return &g_ply[i];
    }
    return NULL;
}


static void ply_close_stream(MwPly *p) {
    if (g_audio_ply == p) g_audio_ply = NULL;
    if (p->dec) { p4mpeg2_close(p->dec); p->dec = NULL; }
    if (p->adx) { p4adx_close(p->adx); p->adx = NULL; }
    if (p->dm) { sfd_demux_close(p->dm); p->dm = NULL; }
    p->have_pending = 0;
    p->eof = 0;
}

/* func_005097e8: destroy. */
void func_005097e8(void *h) {
    MwPly *p = ply_from(h);
    if (!p) return;
    ply_close_stream(p);
    if (p->shown.av) p4mpeg2_frame_release(&p->shown);
    p4pc_heap_free(p->buf);
    memset(p, 0, sizeof *p);
}

void p4pc_mwply_set_frm_sync(void *h, int32_t sw) {
    MwPly *p = ply_from(h);
    if (p) p->noskip = sw;
}

void p4pc_mwply_release_lp(void *h) {
    MwPly *p = ply_from(h);
    if (p) p->loop = 0;
}

/* ---- Playback -------------------------------------------------------------------------------- */

static int ply_open_stream(MwPly *p) {
    char up[256];
    size_t n = 0;
    for (const char *s = p->path; *s && n + 1 < sizeof up; s++, n++) {
        char c = *s == '\\' ? '/' : *s;
        up[n] = (c >= 'a' && c <= 'z') ? (char)(c - 32) : c;
    }
    up[n] = 0;
    p->dm = sfd_demux_open(up);
    if (!p->dm) {
        char alt[300];
        snprintf(alt, sizeof alt, "DATA/%s", up);
        p->dm = sfd_demux_open(alt);
    }
    p->dec = p4mpeg2_open();
    p->adx = p4adx_open();
    if (p->adx) p4adx_set_limit(p->adx, 48000 * 10);
    if (!p->dm || !p->dec || !p->adx) {
        ply_close_stream(p);
        return -1;
    }
    p->next_idx = 0;
    p->eof = 0;
    p->have_pending = 0;
    return 0;
}

/* p4pc_movie_start: H_Cdvd_StartPlayback(handle, path, synchronous). The synchronous form is the loop
 * player (mwPlyStartFnameLp). */
void p4pc_movie_start(void *h, const char *path, int32_t loop) {
    MwPly *p = ply_from(h);
    if (!p || !path) return;
    ply_close_stream(p);
    size_t path_length = 0;
    while (path_length + 1 < sizeof p->path && path[path_length] != '\0') {
        p->path[path_length] = path[path_length];
        ++path_length;
    }
    p->path[path_length] = '\0';
    p->loop = loop != 0;
    p->started_clock = 0;
    p->delivered_idx = -1;
    p->calls = 0;
    p->audio_clock = 0;
    p->audio_played = 0;
    p->audio_done = 0;
    if (g_tick_clock < 0) {
        const char *e = getenv("P4PC_MOVIE_CLOCK");
        const char *t = getenv("P4PC_TURBO");
        const char *v = getenv("P4PC_VIRTUAL_CLOCK");

        /* A virtual scheduler clock (P4PC_TURBO / P4PC_VIRTUAL_CLOCK) runs flips faster or slower than real time, so a movie timed by
         * the monotonic clock would end at a host-dependent flip: the unattended runs use the per-update tick instead. */
        g_tick_clock = (e && !strcmp(e, "tick")) || (t && (t[0] == '1' || t[0] == '2')) || (v && v[0] == '1');
    }
    p->stat = ply_open_stream(p) == 0 ? STAT_PREP : STAT_ERROR;
    if (p->stat == STAT_PREP) g_audio_ply = p;
}

/* Reads one demuxer packet: video goes to the decoder, audio to the ADX decoder. Returns 0 at the end. */
static int ply_pump(MwPly *p) {
    SfdPacket pk;
    if (p->eof) return 0;
    int result = sfd_demux_next(p->dm, &pk);
    if (result == 0) {
        p4mpeg2_flush(p->dec);
        p->eof = 1;
        return 0;
    }
    if (pk.stream_id == SFD_STREAM_VIDEO0) {
        if (p4mpeg2_feed(p->dec, pk.data, pk.size, pk.has_pts ? pk.pts : INT64_MIN) < 0) {
            p->stat = STAT_ERROR;
            return 0;
        }
    } else if (pk.stream_id == 0xC0) {
        p4adx_feed(p->adx, pk.data, pk.size);
    }
    return 1;
}

/* Next picture in display order into p->pending. Returns 0 when the stream is over. */
static int ply_next_picture(MwPly *p) {
    if (p->have_pending) return 1;
    for (;;) {
        P4Mpeg2Frame f;
        if (p4mpeg2_receive(p->dec, &f)) {
            p->pending = f;
            p->pending_idx = p->next_idx++;
            p->have_pending = 1;
            return 1;
        }
        int pumped = ply_pump(p);
        if (!pumped) {
            if (p->stat == STAT_ERROR) return 0;
            if (p4mpeg2_pending(p->dec) == 0) return 0;
        }
    }
}

/* The movie clock in seconds. */
static double fallback_clock(MwPly *p) {
    if (g_tick_clock) return p->calls * (1001.0 / 30000.0);
    return now_s();
}

static double raw_clock(MwPly *p, int *src) {
    *src = 0;
    if (!p->audio_clock) return fallback_clock(p);
    int32_t rate = 0, ch = 0;
    if (!p->audio_done && p->adx && p4adx_sample_rate(p->adx) > 0) {
        rate = p4adx_sample_rate(p->adx);
        ch = p4adx_channels(p->adx);
        (void)ch;
        const double t = (double)p->audio_played / rate;
        if (p->eof && (int64_t)p->audio_played >= p4adx_total(p->adx)) {
            /* The audio has all been played: carry on from here on the fallback clock. */
            p->audio_done = 1;
            p->audio_end_time = t;
            p->fb_at_done = fallback_clock(p);
        }
        *src = 1;
        return t;
    }
    if (p->audio_done) { *src = 2; return p->audio_end_time + (fallback_clock(p) - p->fb_at_done); }
    return fallback_clock(p);   /* no audio header yet, or the movie has no audio */
}

/* Seconds since the first frame. The sources run on different clocks (the audio lane's played-sample
 * count starts when the device does, the fallback when the movie did), so a change of source rebases
 * the movie time instead of jumping it. */
static double movie_time(MwPly *p, int starting) {
    int src;
    const double raw = raw_clock(p, &src);
    if (starting) {
        p->src = src;
        p->base = raw;
        p->last_mt = 0;
        return 0;
    }
    if (src != p->src) {
        p->base = raw - p->last_mt;
        p->src = src;
    }
    p->last_mt = raw - p->base;
    return p->last_mt;
}

static void put32(uint8_t *base, int off, int32_t v) { memcpy(base + off, &v, 4); }

/* mwPlyGetCurFrm(handle, MWS_FRM *): the frame due now, if any. */
static void get_cur_frm(void *h, void *frmp) {
    MwPly *p = ply_from(h);
    uint8_t *frm = frmp;
    if (!frm) return;
    put32(frm, 0, 0);   /* bufadr NULL: nothing to show */
    if (!p || (p->stat != STAT_PREP && p->stat != STAT_PLAYING)) return;
    p->calls++;
    if (!p->started_clock && p->calls < PREP_CALLS) return;

    /* With an audio clock, keep the audio queue filled whatever the video does. */
    if (p->audio_clock && !p->eof && p->adx && p4adx_sample_rate(p->adx) > 0) {
        const int want = p4adx_sample_rate(p->adx) * AUDIO_AHEAD_SECONDS;
        while (p4adx_available(p->adx) < want && ply_pump(p) > 0) {
        }
    }

    const double mt = movie_time(p, !p->started_clock);
    int64_t want_idx;   /* the frame to show, -1: none yet */
    if (!p->started_clock) {
        want_idx = 0;
    } else {
        const int64_t target = (int64_t)(mt * p->fps);
        const int64_t last = p->delivered_idx;
        if (p->noskip) want_idx = last + 1;
        else if (p->src == 1) want_idx = target > last ? target : -1;   /* audio clock: a frame is shown when its time has come */
        else if (target > last + 1) want_idx = target;       /* late: catch up */
        else if (target >= last) want_idx = last + 1;        /* on time, or less than a frame early */
        else want_idx = -1;                                  /* ahead of the clock */
    }
    if (want_idx < 0) return;

    int have_frame = 0;
    int64_t show_idx = -1;
    for (;;) {
        int picture = ply_next_picture(p);
        if (!picture) {
            if (p->stat == STAT_ERROR) return;
            if (p->loop) {
                ply_close_stream(p);
                if (ply_open_stream(p) < 0) { p->stat = STAT_ERROR; return; }
                g_audio_ply = p;
                movie_time(p, 1);
                p->delivered_idx = -1;
                want_idx = 0;
                continue;
            }
            /* stream over: the movie ends once the last frame has had its time */
            if (!have_frame && p->started_clock && (int64_t)(mt * p->fps) >= p->next_idx) p->stat = STAT_PLAYEND;
            break;
        }
        if (p->pending_idx < want_idx && !p->noskip) {   /* before the wanted frame: dropped */
            p->have_pending = 0;
            p->nskipdisp++;
            continue;
        }
        if (p->pending_idx > want_idx && have_frame) break;
        /* Hold the picture (a reference: the decoder may reuse its buffers on the next receive) and stamp the image buffer. */
        p->width = p->pending.width;
        p->height = p->pending.height;
        p->mbw = (p->width + 15) / 16;
        p->mbh = (p->height + 15) / 16;
        size_t need = (size_t)p->mbw * p->mbh * 1024;
        if (need > p->buf_cap) {
            uint8_t *nb = p4pc_heap_realloc(p->buf, need); /* the game reads the frame: guest memory */
            if (!nb) { p->stat = STAT_ERROR; return; }
            memset(nb, 0, need);
            p->buf = nb;
            p->buf_cap = need;
        }
        {
            P4Mpeg2Frame held;
            static uint32_t s_serial;
            uint32_t stamp[4];

            if (p4mpeg2_frame_hold(&p->pending, &held) < 0) { p->stat = STAT_ERROR; return; }
            if (p->shown.av) p4mpeg2_frame_release(&p->shown);
            p->shown = held;
            p->serial = ++s_serial;
            stamp[0] = STAMP_MAGIC;
            stamp[1] = (uint32_t)(p - g_ply);
            stamp[2] = p->serial;
            stamp[3] = 0;
            memcpy(p->buf, stamp, sizeof stamp);
        }
        if (tiled_buffers()) {
            /* P4PC_MOVIE_TILED=1: the buffer holds the IPU macroblock columns, as retail's does and as this player's did before the stamp
             * (the reference the stamp's picture is compared with in the tests and in A/B runs). The tiles overwrite the stamp. */
            if (p4mpeg2_to_ipu_rgb32(p->dec, &p->pending, p->buf) < 0) { p->stat = STAT_ERROR; return; }
        }
        show_idx = p->pending_idx;
        have_frame = 1;
        p->have_pending = 0;
        break;
    }
    if (!have_frame) return;

    if (!p->started_clock) {
        /* the schedule starts with the first frame */
        int num, den;
        p4mpeg2_frame_rate(p->dec, &num, &den);
        p->fps = num > 0 && den > 0 ? (double)num / den : 30000.0 / 1001.0;
        p->started_clock = 1;
        p->stat = STAT_PLAYING;
    }
    p->delivered_idx = show_idx;
    const int fps1000 = (int)(p->fps * 1000.0 + 0.5);
    put32(frm, 0x00, P4_H2G(p->buf));
    put32(frm, 0x04, (int32_t)show_idx);
    put32(frm, 0x08, 2);                         /* MwsfdBufFmt: RGB32 */
    put32(frm, 0x0c, p->width);
    put32(frm, 0x10, p->height);
    put32(frm, 0x14, p->mbw);
    put32(frm, 0x18, p->mbh);
    put32(frm, 0x1c, 3);                         /* picture structure: frame */
    put32(frm, 0x20, fps1000);
    put32(frm, 0x24, (int32_t)show_idx);         /* frame number (retail: the frame's time in frame units) */
    put32(frm, 0x28, (int32_t)(show_idx * 1000)); /* retail: frame number * 1000 */
    put32(frm, 0x2c, 1001);                      /* time unit */
    put32(frm, 0x30, (int32_t)show_idx);
}

/* mwPlyGetCurFrm. P4PC_MOVIE_LOG=<file>: one line per call with everything the game can see of the answer (the status, the frame handed
 * out, its number and size), to compare two runs: the frame, the status and the end of a movie must not depend on how the picture is made
 * or on whether it is drawn. */
void p4pc_mwply_get_cur_frm(void *h, void *frmp) {
    static FILE *log;
    static int log_open;

    get_cur_frm(h, frmp);
    if (!log_open) {
        const char *e = getenv("P4PC_MOVIE_LOG");
        log_open = 1;
        log = e && *e ? fopen(e, "w") : NULL;
    }
    if (log && frmp) {
        const uint8_t *frm = frmp;
        const MwPly *p = ply_from(h);
        int32_t w[8];
        memcpy(w, frm + 0x04, 4);
        memcpy(w + 1, frm + 0x0c, 4);
        memcpy(w + 2, frm + 0x10, 4);
        memcpy(w + 3, frm + 0x24, 4);
        memcpy(w + 4, frm + 0x00, 4);
        if (w[4] != 0)
            fprintf(log, "calls %lld stat %d frame index %d size %dx%d number %d\n", p ? (long long)p->calls : -1, p ? p->stat : -1, w[0], w[1], w[2],
                    w[3]);
        else
            fprintf(log, "calls %lld stat %d no frame\n", p ? (long long)p->calls : -1, p ? p->stat : -1);
        fflush(log);
    }
}

void p4pc_mwply_rel_cur_frm(void *h) { (void)h; }

/* The picture a stamped image buffer stands for, or NULL (not a stamp, a player that is gone, or a picture that has been replaced). */
static const P4Mpeg2Frame *stamp_resolve(const void *image) {
    uint32_t stamp[3];

    memcpy(stamp, image, sizeof stamp);
    if (stamp[0] != STAMP_MAGIC || stamp[1] >= MAX_PLAYERS) return NULL;
    const MwPly *p = &g_ply[stamp[1]];
    return p->used && p->serial == stamp[2] && p->shown.av ? &p->shown : NULL;
}

/* Fills the buffer of the last delivered frame with the IPU's RGB32 macroblock columns, as retail's player does and as this player
 * did before the stamp: for the tests that compare pictures and for tools that read the buffer. Returns 0 or -1. */
int p4pc_mwply_fill_tiled(void *h) {
    MwPly *p = ply_from(h);
    if (!p || !p->shown.av || !p->buf || !p->dec) return -1;
    uint8_t *tiled = malloc(p->buf_cap);
    if (!tiled) return -1;
    int r = p4mpeg2_to_ipu_rgb32(p->dec, &p->shown, tiled);
    if (r == 0) memcpy(p->buf, tiled, p->buf_cap);
    free(tiled);
    return r;
}

/* func_0050d4d0: MWSFD_STAT_*. */
int32_t func_0050d4d0(void *h) {
    MwPly *p = ply_from(h);
    return p ? p->stat : STAT_ERROR;
}

/* ---- Audio for the audio lane (mwply.h) -------------------------------------------------------- */

void *p4pc_movie_audio_current(void) { return g_audio_ply && g_audio_ply->used && g_audio_ply->adx ? g_audio_ply : NULL; }

int32_t p4pc_movie_audio_format(void *h, int32_t *rate, int32_t *channels) {
    MwPly *p = ply_from(h);
    if (!p || !p->adx || p4adx_sample_rate(p->adx) == 0) return 0;
    *rate = p4adx_sample_rate(p->adx);
    *channels = p4adx_channels(p->adx);
    return 1;
}

int32_t p4pc_movie_audio_pull(void *h, int16_t *dst, int32_t max_frames) {
    MwPly *p = ply_from(h);
    return p && p->adx ? p4adx_pull(p->adx, dst, max_frames) : 0;
}

void p4pc_movie_audio_played(void *h, uint64_t frames) {
    MwPly *p = ply_from(h);
    if (!p) return;
    p->audio_played = frames;
    p->audio_clock = 1;
}

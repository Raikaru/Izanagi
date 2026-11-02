/* mwply_test: the movie player (src/movie/mwply.c) driven through the calls the game makes.
 *
 * Runs DATA/SFD/P4CT019.SFD (16 s, 480 pictures, 29.97 fps) the way movieNativeTaskUpdate does: create
 * with an MWSFD_CRPRM, start (non-loop), then one mwPlyGetCurFrm / mwPlyRelCurFrm / status per tick.
 * P4PC_MOVIE_CLOCK=tick makes each GetCurFrm call 1/29.97 s (the game's update rate), so the run is deterministic. Checks: status
 * goes 1 (prepare) -> 2 (playing) -> 3 (play end) and never 4; the first frame arrives at update 20
 * and every later update delivers the next frame number (+1, none skipped); every frame is 640x448 with 40x28 macroblocks at 29970; the
 * RGB32 buffer de-tiles to a picture with alpha 0x80 everywhere and is not black; the movie lasts
 * 480 + 20 updates; destroy frees the slot for a second player. A loop player restarts at the
 * end (frame numbers start again) until released. Skips (77) without P4PC_ISO.
 * P4PC_MWPLY_PNG=<dir> writes a few frames. */
#define _FILE_OFFSET_BITS 64
#define _POSIX_C_SOURCE 200809L

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "guest_test.h"
#include "mwply.h"
#include "mpeg2_dec.h"
#include "p4pc/os.h"
#include "p4png.h"
#include "vfs.h"

#define SKIP_RETURN 77

void *func_00509268(const void *prm);
int32_t func_0050d4d0(void *h);
void func_005097e8(void *h);
void p4pc_movie_start(void *h, const char *path, int32_t loop);
void p4pc_mwply_get_cur_frm(void *h, void *frm);
void p4pc_mwply_rel_cur_frm(void *h);
void p4pc_mwply_set_frm_sync(void *h, int32_t sw);
void p4pc_mwply_release_lp(void *h);
int32_t p4pc_mwply_calc_work(const void *prm);
void p4pc_mwply_init(void *iprm);
int p4pc_mwply_fill_tiled(void *h);

static int failures;
static void check(int ok, const char *what) {
    if (!ok) { fprintf(stderr, "mwply_test: FAIL %s\n", what); failures++; }
}

static int32_t rd32(const uint8_t *f, int off) { int32_t v; memcpy(&v, f + off, 4); return v; }

int main(void) {
    const char *iso = getenv("P4PC_ISO");
    if (!iso || !*iso) { fprintf(stderr, "mwply_test: P4PC_ISO is not set, skipping\n"); return SKIP_RETURN; }
    if (vfs_mount(iso, NULL) < 0) { fprintf(stderr, "mwply_test: cannot mount %s\n", iso); return 1; }
    p4_os_setenv("P4PC_MOVIE_CLOCK", "tick", 1);
    const char *pngdir = getenv("P4PC_MWPLY_PNG");

    /* MWSFD_CRPRM as movieNativeTaskUpdate builds it */
    int32_t prm[12] = {1, 0x4C4B40, 640, 448, 2, 1, 0, 0, 0x11, 2, 0, 0};
    p4pc_mwply_init(NULL);
    prm[7] = p4pc_mwply_calc_work(prm);
    check(prm[7] > 0, "work size is positive");
    void *h = func_00509268(prm);
    check(h != NULL, "create");
    if (!h) return 1;
    p4pc_mwply_set_frm_sync(h, 0);
    check(func_0050d4d0(h) == 0, "status before start is stop (0)");
    p4pc_movie_start(h, "sfd/P4CT019.sfd", 0);
    check(func_0050d4d0(h) == 1, "status after start is prepare (1)");

    uint8_t frm[0x90];
    uint8_t *lin = malloc(640 * 448 * 4);
    uint8_t *direct = malloc(640 * 448 * 4);
    int direct_bad = 0;
    int ticks = 0, frames = 0, saw_playing = 0, end_tick = -1;
    int32_t last_no = -1;
    int first_tick = -1, gaps = 0;
    int64_t audio_frames = 0;
    double audio_energy = 0;
    static int16_t pcm[8192 * 2];
    int non_black_frames = 0, bad_alpha = 0, bad_inf = 0, order_ok = 1;
    while (ticks < 30 * 40) {
        ticks++;
        p4pc_mwply_get_cur_frm(h, frm);
        uint32_t buf = (uint32_t)rd32(frm, 0);
        if (buf) {
            int32_t no = rd32(frm, 0x24);
            if (no <= last_no) order_ok = 0;
            if (first_tick < 0) first_tick = ticks;
            if (last_no >= 0 && no != last_no + 1) gaps++;
            last_no = no;
            if (rd32(frm, 0x0c) != 640 || rd32(frm, 0x10) != 448 || rd32(frm, 0x14) != 40 || rd32(frm, 0x18) != 28 ||
                rd32(frm, 0x20) != 29970 || rd32(frm, 0x08) != 2)
                bad_inf++;
            const uint8_t *ipu = (const uint8_t *)P4_G2H(buf); /* the frame is guest memory */
            /* The player hands out a stamped buffer without pixels; the picture it stands for converts straight to linear RGBA. The
             * IPU macroblock columns (fill_tiled, what the buffer held before the stamp) must hold the same pixels. */
            const P4Mpeg2Frame *pic = p4mpeg2_resolve_stamp(ipu);
            check(pic != NULL, "the image buffer is stamped");
            if (!pic) break;
            p4mpeg2_frame_to_rgba_ipu(pic, direct, 640 * 4);
            check(p4pc_mwply_fill_tiled(h) == 0, "the IPU macroblock columns can be filled");
            int64_t sum = 0;
            for (int c = 0; c < 40; c++)
                for (int r = 0; r < 28; r++)
                    for (int y = 0; y < 16; y++)
                        for (int x = 0; x < 16; x++) {
                            const uint8_t *s = ipu + ((size_t)c * 28 + r) * 1024 + (y * 16 + x) * 4;
                            memcpy(lin + ((size_t)(r * 16 + y) * 640 + c * 16 + x) * 4, s, 4);
                            sum += s[0] + s[1] + s[2];
                            if (s[3] != 0x80) bad_alpha++;
                        }
            if (sum > (int64_t)640 * 448 * 3 * 8) non_black_frames++;
            for (size_t i = 0; i < (size_t)640 * 448 * 4; i += 4)
                if (memcmp(lin + i, direct + i, 3) != 0 || direct[i + 3] != 0xff) { direct_bad++; break; }
            if (pngdir && (frames == 0 || frames == 120 || frames == 300 || frames == 470)) {
                for (size_t i = 3; i < 640 * 448 * 4; i += 4) lin[i] = 0xff;
                char path[1024];
                snprintf(path, sizeof path, "%s/mwply_%04d.png", pngdir, no);
                p4png_write_rgba(path, 640, 448, lin);
            }
            frames++;
        }
        for (int32_t n; (n = p4pc_movie_audio_pull(h, pcm, 8192)) > 0;) {
            audio_frames += n;
            for (int i = 0; i < n * 2; i++) audio_energy += (double)pcm[i] * pcm[i];
        }
        p4pc_mwply_rel_cur_frm(h);
        int32_t st = func_0050d4d0(h);
        if (st == 2) saw_playing = 1;
        if (st == 4) { check(0, "status never 4 (error)"); break; }
        if (st == 3) { end_tick = ticks; break; }
    }
    {
        int32_t rate = 0, ch = 0;
        check(p4pc_movie_audio_format(h, &rate, &ch) == 1 && rate == 48000 && ch == 2, "ADX track is 48 kHz stereo");
        check(audio_frames > 48000 * 15 && audio_frames < 48000 * 17, "about 16 s of ADX audio decoded");
        check(audio_energy / (audio_frames * 2.0 + 1) > 1e4, "the audio is not silent");
        printf("mwply_test: %lld audio frames, rms %.0f\n", (long long)audio_frames, __builtin_sqrt(audio_energy / (audio_frames * 2.0 + 1)));
    }
    check(saw_playing, "status reached playing (2)");
    check(end_tick > 0, "status reached play end (3)");
    check(order_ok, "frame numbers increase");
    check(direct_bad == 0, "the direct RGBA conversion equals the IPU macroblock columns, pixel for pixel (alpha 0xff)");
    check(bad_inf == 0, "frame information (size, macroblocks, rate, format)");
    check(bad_alpha == 0, "alpha 0x80 in every pixel");
    check(non_black_frames > frames / 2, "most frames are not black");
    check(frames == 480, "all 480 frames delivered");
    check(first_tick == 20, "the first frame arrives at update 20");
    check(gaps == 0, "frame numbers advance by one per update");
    check(end_tick >= 500 && end_tick <= 503, "play end follows the last frame by one frame time");
    printf("mwply_test: %d frames in %d ticks (end at tick %d, %.1f s), last frame number %d\n", frames, ticks, end_tick,
           end_tick * (1001.0 / 30000.0), last_no);
    func_005097e8(h);

    /* A loop player restarts until released. */
    void *lp = func_00509268(prm);
    check(lp != NULL, "second player after destroy");
    p4pc_movie_start(lp, "sfd/P4CT019.sfd", 1);
    int restarts = 0, prev = -1, lticks = 0;
    int32_t lst = 0;
    while (lticks < 30 * 40 && restarts < 1) {
        lticks++;
        p4pc_mwply_get_cur_frm(lp, frm);
        if (rd32(frm, 0)) {
            int32_t no = rd32(frm, 0x24);
            if (prev >= 0 && no < prev) restarts++;
            prev = no;
        }
        lst = func_0050d4d0(lp);
        if (lst >= 3) break;
    }
    check(restarts == 1, "the loop player restarts at the end");
    check(lst < 3, "the loop player is still playing");
    p4pc_mwply_release_lp(lp);
    for (int i = 0; i < 30 * 40 && func_0050d4d0(lp) < 3; i++) p4pc_mwply_get_cur_frm(lp, frm);
    check(func_0050d4d0(lp) == 3, "after mwPlyReleaseLp the loop player ends");
    func_005097e8(lp);

    /* Audio clock: the audio lane reports played samples at half speed, so the video runs at half speed
     * (it waits for the audio); with nothing played the video stays on its first frame. */
    void *ah = func_00509268(prm);
    p4pc_movie_start(ah, "sfd/P4CT019.sfd", 0);
    int32_t afmt_rate = 0, afmt_ch = 0;
    int32_t a_last = -1;
    uint64_t played = 0;
    for (int t = 1; t <= 400; t++) {
        p4pc_mwply_get_cur_frm(ah, frm);
        if (t == 30) p4pc_movie_audio_format(ah, &afmt_rate, &afmt_ch);
        if (rd32(frm, 0)) a_last = rd32(frm, 0x24);
        p4pc_movie_audio_played(ah, 0);   /* the lane reports from the start; nothing has played yet */
        if (t < 60) { /* the video waits */
            check(a_last <= 0, "video waits while no audio has played");
        } else {
            played += 800;   /* half of the 1601.6 frames a tick would play */
            p4pc_movie_audio_played(ah, played);
        }
    }
    check(afmt_rate == 48000 && afmt_ch == 2, "audio format is known by update 30");
    check(a_last >= 165 && a_last <= 175, "with the audio at half speed the video is at about frame 170 after 340 updates");
    printf("mwply_test: audio clock: frame %d after 340 updates at half speed\n", a_last);
    func_005097e8(ah);
    free(lin);
    return failures ? 1 : 0;
}

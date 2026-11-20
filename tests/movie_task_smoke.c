/* movie_task_smoke: the game's own movie task (movieNativeTaskUpdate, decomp
 * src/Kosaka/Field/k_fldrc_inferred_00468d10.c) running against the port's player and GL device.
 *
 * The task is the one mwPlayerCreate registers (task sdkSfdPlay): it creates the player and the 640x448
 * raster, starts the movie through H_Cdvd_StartPlayback, and each update fetches a frame with
 * mwPlyGetCurFrm, copies it, uploads it with gsUploadRasterTiles, draws the Im2D quad and polls the
 * status, ending in state 6 when the player reports play end. The test boots the engine the way
 * p4pc_viewer does (kwlnInitIop, kwlnInitEngine), makes a game camera (kwlnCameraCreate, installed as
 * the scene camera kwlnGetCamera returns), builds the 0x220-byte work block like mwPlayerCreate does
 * (movie index, "not skipped" flag), and calls the update once per tick between RwCameraBeginUpdate and
 * End, with the player on the tick clock (1/60 s per GetCurFrm call).
 *
 * Environment: P4PC_MOVIE_INDEX (default 19), P4PC_MOVIE_SHOTS (comma list
 * of ticks to write as PNG, default 120,300,600), P4PC_MOVIE_PNG (output
 * prefix, default movie_task). Checks: the task leaves state 0, draws frames that are not black
 * (readback), frame numbers reported through mwPlayerGetCurrent (iGpffffbaf8) increase,
 * and the task ends in state 6. Needs P4PC_ISO and P4PC_ELF;
 * exits 77 without them. */
#define _FILE_OFFSET_BITS 64
#define _POSIX_C_SOURCE 200809L

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "p4gl.h"
#include "p4gl_gl.h"
#include "p4gl_readback.h"
#include "p4pc/image.h"
#include "movie_events.h"
#ifdef P4PC_MOVIE_RETAIL_EVENTS
#include "movie_retail_events.h"
#endif
#include "p4pc/os.h"
#include "p4png.h"
#include "vfs.h"
#include "guest_test.h"

#define SKIP_RETURN 77
#define VIEW_W 640u
#define VIEW_H 448u
#define WORK_SIZE 0x220
#define OFF_STATE 0x1E4
#define OFF_INDEX 0x1EA
#define OFF_NOTSKIP 0x1F4
#define OFF_PATH 0x218

void *kwlnProbeHeapBudget(void);
void kwlnInitIop(void);
void kwlnInitEngine(void);
void *kwlnCameraCreate(uint32_t width, uint32_t height, uint32_t createZBuffer);
void *RwCameraBeginUpdate(void *camera);
void *RwCameraEndUpdate(void *camera);
void movieNativeTaskUpdate(void *arg0, void *work);

static int failures;
static void fail(const char *what) { fprintf(stderr, "movie_task_smoke: FAIL %s\n", what); failures++; }

static int test_main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    const char *iso = getenv("P4PC_ISO");
    const char *elf = getenv("P4PC_ELF");
    if (!iso || !*iso || !elf || !*elf) {
        fprintf(stderr, "movie_task_smoke: P4PC_ISO or P4PC_ELF unset, skipped\n");
        return SKIP_RETURN;
    }
    const char *idxs = getenv("P4PC_MOVIE_INDEX");
    const int index = idxs ? atoi(idxs) : 19;
    const char *shots = getenv("P4PC_MOVIE_SHOTS");
    if (!shots) shots = "120,300,600";
    const char *prefix = getenv("P4PC_MOVIE_PNG");
    if (!prefix) prefix = "movie_task";

    p4_os_headless_env();
    p4_os_setenv("P4PC_TRAP_CONTINUE", "1", 0);
    p4_os_setenv("P4PC_MOVIE_CLOCK", "tick", 1);
    p4pc_trap_configure();
    p4pc_load_image(elf);
    p4pc_apply_patches();
    p4pc_kernel_pre_init();
    kwlnProbeHeapBudget();
    kwlnInitIop();
    kwlnInitEngine();
    if (p4gl_context_open() != 0) { fail("GL context"); return 1; }
    void *camera = kwlnCameraCreate(VIEW_W, VIEW_H, 1);
    if (!camera) { fail("camera"); return 1; }
    P4_G32(0x00764b70u) = P4_H2G(camera);   /* iGpffffba80: the scene camera */
    if (p4gl_readback_target_create(VIEW_W, VIEW_H, VIEW_W, VIEW_H) != 0) { fail("readback target"); return 1; }

    /* mwPlayerCreate: a calloc'd 0x220-byte block, "not skipped" at +0x1F4, the movie index at +0x1EA. */
    uint8_t *work = p4t_galloc(WORK_SIZE, 16);   /* guest storage, like the game heap block it stands for */
    *(int32_t *)(work + OFF_NOTSKIP) = 1;
    *(int16_t *)(work + OFF_INDEX) = (int16_t)index;
    *(uint32_t *)(work + OFF_PATH) = p4_gport_str("sdkSfdPlay");

    uint8_t *rgba = malloc((size_t)VIEW_W * VIEW_H * 4);
    /* P4PC_MOVIE_TRACE=<file>: one JSON line per change of the task's polled words, same schema as
     * the PCSX2 trace tooling writes for retail (t is the update number / 29.97). */
    FILE *trace = getenv("P4PC_MOVIE_TRACE") ? fopen(getenv("P4PC_MOVIE_TRACE"), "w") : NULL;
    int32_t tr_prev[9] = {-99, -99, -99, -99, -99, -99, -99, -99, -99};
    int ticks = 0, drawn = 0, black = 0, shot_count = 0;
    int end_tick = -1;
    int32_t last_frame = -1, backward = 0;
    int16_t state = -1;
    MovieEvents events = MOVIE_EVENTS_INIT;
    const int max_ticks = index == 19 || index == 20 ? 800 : 3200;
    /* P4PC_MOVIE_DRAW_ON=a-b[,c-d...]: draw calls only in those update ranges (like P4PC_TURBO=2 outside its windows), so a run
     * with part of the movie undrawn can be compared with one that draws all of it: the trace must not change. */
    const char *draw_on = getenv("P4PC_MOVIE_DRAW_ON");
    while (ticks < max_ticks) {
        ticks++;
        if (draw_on) {
            int on = 0;
            for (const char *q = draw_on; *q;) {
                char *e;
                long a = strtol(q, &e, 10), b = a;
                if (*e == '-') b = strtol(e + 1, &e, 10);
                if (ticks >= a && ticks <= b) on = 1;
                q = *e == ',' ? e + 1 : e;
                if (e == q && *q) break;
            }
            p4gl_gl_set_draw(on);
        }
        p4gl_glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        p4gl_glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        if (RwCameraBeginUpdate(camera) == NULL) { fail("camera begin"); break; }
        movieNativeTaskUpdate(NULL, work);
        RwCameraEndUpdate(camera);
        state = *(int16_t *)(work + OFF_STATE);
        int32_t cur = (int32_t)P4_G32(0x00764be8u);   /* iGpffffbaf8: frame number of the last frame */
        if (cur != last_frame) {
            if (cur < last_frame) backward++;
            last_frame = cur;
        }
        if (trace) {
            int32_t cur_w[9] = {state, *(int16_t *)(work + OFF_INDEX), *(int16_t *)(work + 0x1EE), *(int32_t *)(work + 0x38),
                                cur, *(int32_t *)(work + 0x40), *(int32_t *)(work + 0x68), *(int32_t *)(work + 0x204),
                                *(int32_t *)(work + 0x208)};
            if (memcmp(cur_w, tr_prev, sizeof cur_w) != 0) {
                memcpy(tr_prev, cur_w, sizeof cur_w);
                fprintf(trace,
                        "{\"state\": %d, \"index\": %d, \"startup\": %d, \"status\": %d, \"frame\": %d, \"buffer\": %u, "
                        "\"time\": %d, \"fade_out\": %d, \"fade_in\": %d, \"t\": %.4f}\n",
                        cur_w[0], cur_w[1], cur_w[2], cur_w[3], cur_w[4], (unsigned)cur_w[5], cur_w[6], cur_w[7], cur_w[8],
                        ticks * (1001.0 / 30000.0));
            }
        }
        movie_events_step(&events, ticks, state, cur, *(int32_t *)(work + 0x208), *(int32_t *)(work + 0x204),
                          *(int32_t *)(work + 0x40));
        int want_shot = 0;
        for (const char *p = shots; *p;) {
            if (atoi(p) == ticks) want_shot = 1;
            while (*p && *p != ',') p++;
            if (*p == ',') p++;
        }
        if ((state == 4 || want_shot) && (!draw_on || p4gl_gl_draw_enabled())) {
            if (p4gl_readback_rgba(VIEW_W, VIEW_H, rgba) == 0) {
                int64_t sum = 0;
                for (size_t i = 0; i < (size_t)VIEW_W * VIEW_H * 4; i += 4) sum += rgba[i] + rgba[i + 1] + rgba[i + 2];
                drawn++;
                if (sum < (int64_t)VIEW_W * VIEW_H * 3 * 4) black++;
                if (want_shot) {
                    char path[512];
                    snprintf(path, sizeof path, "%s_%05d.png", prefix, ticks);
                    if (p4png_write_rgba(path, VIEW_W, VIEW_H, rgba) != 0) fail("PNG write");
                    shot_count++;
                    printf("movie_task_smoke: tick %d state %d frame %d brightness %.1f -> %s\n", ticks, state, last_frame,
                           (double)sum / ((double)VIEW_W * VIEW_H * 3), path);
                }
            }
        }
        if (state == 6 || state == 7 || state == 0xB) { end_tick = ticks; break; }
    }
    printf("movie_task_smoke: movie %d ended in state %d at tick %d (%.1f s of updates), last frame %d\n", index, state, end_tick,
           end_tick * (1001.0 / 30000.0), last_frame);
#ifdef P4PC_MOVIE_RETAIL_EVENTS
    for (size_t i = 0; i < sizeof kMovieRetail / sizeof kMovieRetail[0]; i++) {
        const MovieRetailRow *r = &kMovieRetail[i];
        if (r->index != index) continue;
        const MovieExpected expected = {MOVIE_RETAIL_PREP_MIN, MOVIE_RETAIL_PREP_MAX, r->s4_frame, r->s5_frame,
                                        r->s6_frame, r->s6_fade_out, r->last_frame};
        const unsigned bad = movie_events_compare(&events, last_frame, &expected);
        printf("movie_task_smoke: retail events: prep %d updates (retail %.1f), state 4 at frame %d (%d), state 5 at %d (%d), "
               "state 6 at %d (%d) fade-out %d (%d), last %d (%d), %d irregular steps\n",
               events.t_first - events.t_s3, r->prep, events.s4_frame, r->s4_frame, events.s5_frame, r->s5_frame,
               events.s6_frame, r->s6_frame, events.s6_fade_out, r->s6_fade_out, last_frame, r->last_frame, events.non_unit_steps);
        if (bad & MOVIE_BAD_PREP) fail("updates from state 3 to the first frame differ from retail");
        if (bad & MOVIE_BAD_S4) fail("state 4 starts at another frame number than retail");
        if (bad & MOVIE_BAD_S5) fail("the fade-out starts at another frame number than retail");
        if (bad & MOVIE_BAD_S6) fail("the task ends at another frame than retail");
        if (bad & MOVIE_BAD_LAST) fail("the last frame number differs from retail");
        if (bad & MOVIE_BAD_STEP) fail("a frame number was skipped (retail steps by +1 per update)");
    }
#endif
    if (end_tick < 0) fail("the task never reached its end state");
    if (state == 0xB) fail("the task failed to create the player or raster (state 0xB)");
    if (last_frame < 100) fail("frame numbers advanced");
    if (backward) fail("frame numbers went backwards");
    if (shot_count && black == drawn) fail("every captured frame is black");
#ifdef P4PC_MOVIE_RETAIL_EVENTS
    if (index == 19 && (end_tick < MOVIE_RETAIL_INDEX19_END_MIN || end_tick > MOVIE_RETAIL_INDEX19_END_MAX))
        fail("movie ends outside its expected interval");
#endif
    if (trace) fclose(trace);
    free(rgba);
    p4gl_context_close();
    return failures ? 1 : 0;
}

int main(int argc, char **argv)
{
    return p4t_on_guest_stack(test_main, argc, argv);
}

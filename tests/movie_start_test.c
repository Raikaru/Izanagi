/* movie_start_test: the boot movie's player start, in an exe that links the generated tables.
 *
 * The port exe carries a trap stub for every undefined name the tables step cannot place. A host
 * library name that step failed to recognise (winpthreads' pthread_mutex_* on Windows) becomes such
 * a stub and shadows the real function, and nothing fails until the movie player first calls it:
 * "TRAP pthread_mutex_init (PS2 0x00000000)" at MOVIE START. mwply_test does not link the tables, so
 * it cannot see that. This test creates and starts a player on the shortest disc movie and pulls
 * frames and audio through the same calls the game makes, in an exe linked like p4pc. P4PC_TRAP_CONTINUE
 * is forced off, so any trap aborts the run (a non-zero exit). Exits 77 (CTest skip) without P4PC_ISO.
 */
#define _FILE_OFFSET_BITS 64
#define _POSIX_C_SOURCE 200809L

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "guest_test.h"
#include "mwply.h"
#include "p4pc/image.h"
#include "p4pc/os.h"
#include "vfs.h"

#define SKIP_RETURN 77

void *func_00509268(const void *prm);
int32_t func_0050d4d0(void *h);
void func_005097e8(void *h);
void p4pc_movie_start(void *h, const char *path, int32_t loop);
void p4pc_mwply_get_cur_frm(void *h, void *frm);
void p4pc_mwply_rel_cur_frm(void *h);
void p4pc_mwply_set_frm_sync(void *h, int32_t sw);
int32_t p4pc_mwply_calc_work(const void *prm);
void p4pc_mwply_init(void *iprm);

static int test_main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    const char *iso = getenv("P4PC_ISO");
    int32_t prm[12] = {1, 0x4C4B40, 640, 448, 2, 1, 0, 0, 0x11, 2, 0, 0};
    uint32_t frm[0x90 / 4] = {0};
    int16_t pcm[8192 * 2];
    int ticks, frames = 0;
    int64_t audio = 0;
    void *h;

    if (!iso || !*iso) { fprintf(stderr, "movie_start_test: P4PC_ISO is not set, skipping\n"); return SKIP_RETURN; }
    p4_os_setenv("P4PC_TRAP_CONTINUE", "0", 1);
    p4_os_setenv("P4PC_MOVIE_CLOCK", "tick", 1);
    p4pc_trap_configure();
    /* No ELF is loaded here. Supply the retail controls/reent and the
     * morecore errno word (009389e0), as the ps2_libc fixture does. */
    if (p4t_guest_map(0x0070c000u, 0x6000) != 0 ||
        p4t_guest_map(0x00938000u, 4096) != 0) return 1;
    *(uint32_t *)P4_G2H(0x00710070u) = 0x00710500u;
    if (p4t_heap_init(12u << 20) != 0) { fprintf(stderr, "movie_start_test: cannot create the heap\n"); return 1; }
    if (vfs_mount(iso, NULL) < 0) { fprintf(stderr, "movie_start_test: cannot mount %s\n", iso); return 1; }

    p4pc_mwply_init(NULL);
    prm[7] = p4pc_mwply_calc_work(prm);
    h = func_00509268(prm);
    if (!h) { fprintf(stderr, "movie_start_test: FAIL create\n"); return 1; }
    p4pc_mwply_set_frm_sync(h, 0);
    p4pc_movie_start(h, "sfd/P4CT019.SFD", 0);
    for (ticks = 0; ticks < 60; ticks++) {
        p4pc_mwply_get_cur_frm(h, frm);
        if (frm[0]) frames++; /* the picture pointer is word 0 of the frame record */
        p4pc_mwply_rel_cur_frm(h);
        {
            int32_t n;
            while ((n = p4pc_movie_audio_pull(h, pcm, 8192)) > 0) audio += n;
        }
        if (func_0050d4d0(h) == 4) { fprintf(stderr, "movie_start_test: FAIL player error\n"); return 1; }
    }
    func_005097e8(h);
    if (frames == 0) { fprintf(stderr, "movie_start_test: FAIL no frame in 60 updates\n"); return 1; }
    printf("movie_start_test: ok (%d frames, %lld audio frames, no trap)\n", frames, (long long)audio);
    return 0;
}

int main(int argc, char **argv)
{
    return p4t_on_guest_stack(test_main, argc, argv);
}

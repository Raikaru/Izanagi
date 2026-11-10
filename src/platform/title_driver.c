/* P4PC_AUTOPRESS_NEWGAME=<T> (or P4PC_AUTOPRESS_LOAD=<T>, the cursor on LOAD GAME instead): the title is driven by the game's own state instead of flip numbers, then the P4PC_AUTOPRESS script starts
 * with its clock at T on the flip NEW GAME is confirmed (the retail hook's --script-after-newgame T). The opening movie is
 * skipped with Start, "PRESS ANY BUTTON" (titleDrawProc state 7) with Start, and in the menu (state 9) Down moves the cursor (work + 0x20:
 * 0 LOAD GAME, 1 NEW GAME, 2 CONFIG) until it is on NEW GAME, then Cross. A script keyed to flip numbers misses the menu whenever the title
 * takes a different number of flips (a Cross on LOAD GAME opens the memory card browser and waits there); this does not.
 * P4PC_TITLE_WATCH_OPENING=1 lets the opening movie play to its natural end instead of skipping it (mod witnesses of movie
 * completion). */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "p4pc/package_boot.h"

#include "p4pc/hostmath.h"
#include "p4pc/os.h"
extern void *kwlnTaskGetTaskByName(const void *name);
extern void p4pc_pad_inject(unsigned bits, unsigned frames);
extern void p4pc_pad_autopress_set_origin(unsigned flipAtT, double T);

enum { PAD_START = 0x0008, PAD_DOWN = 0x0040, PAD_CROSS = 0x4000 };   /* the snapshot layout of pad_sdl.h */

static int sWantCursor = 1; /* 1 NEW GAME (P4PC_AUTOPRESS_NEWGAME), 0 LOAD GAME (P4PC_AUTOPRESS_LOAD) */

static int32_t titleWork(void)
{
    const uint8_t *task = kwlnTaskGetTaskByName("titleDrawProc");   /* the task: a host pointer into guest memory */

    return task != NULL ? *(const int32_t *)(task + 0x38) : 0;
}

void p4pc_title_driver_frame(unsigned flip)
{
    static int mode = -1;
    static double originT;
    static unsigned next;
    static int traceTitle;
    if(p4_package_boot_waiting_stable){
        const int32_t work=titleWork();
        if(work&&*(const int32_t *)P4_G2H((uint32_t)work)==9)p4_package_boot_stable();
    }

    if (mode < 0) {
        const char *e = getenv("P4PC_AUTOPRESS_NEWGAME");
        const char *trace = getenv("P4PC_PACE_TRACE");
        traceTitle = trace != NULL && *trace != '\0';

        if (e == NULL || *e == '\0') {
            e = getenv("P4PC_AUTOPRESS_LOAD");
            sWantCursor = 0;
        }

        mode = e != NULL && *e != '\0';
        if (mode) {
            /* round-to-nearest, as the pad's script times (the Windows CRT's strtod follows the EE mode's truncation, glibc's does not) */
            const unsigned csr = p4pc_fpu_host_enter();

            originT = strtod(e, NULL);
            p4pc_fpu_host_leave(csr);
        }
        if (mode) {
            fprintf(stderr, "p4pc: title driver on: the script starts at T=%g when %s is confirmed\n", originT, sWantCursor ? "NEW GAME" : "LOAD GAME");
        }
    }
    if (traceTitle) {
        const int32_t w = titleWork();
        const int32_t state = w != 0 ? *(int32_t *)P4_G2H((uint32_t)w) : -1;
        if (state == 7 || state == 9) {
            /* Readiness means the retail title accepts input, not its first painted pixel.
             * Observe every traced flip, independently of automation's 40-flip polling. */
            fprintf(stderr, "p4pc: presentation: title-ready state %d flip %u timestamp_ns %llu\n",
                (int)state, flip, (unsigned long long)p4_os_now_ns());
            traceTitle = 0;
        }
    }
    if (mode != 1 || flip < next) {
        return;
    }
    next = flip + 40;
    if (kwlnTaskGetTaskByName("sdkSfdPlay") != NULL) {            /* the opening movie: Start skips it */
        static int watch = -1;
        if (watch < 0) {
            const char *w = getenv("P4PC_TITLE_WATCH_OPENING");
            watch = w != NULL && w[0] == '1';
        }
        if (!watch) {
            p4pc_pad_inject(PAD_START, 8);
        }
        next = flip + 90;
        return;
    }
    {
        const int32_t w = titleWork();
        const int32_t state = w != 0 ? *(int32_t *)P4_G2H((uint32_t)w) : -1;

        if (state == 7) {
            p4pc_pad_inject(PAD_START, 8);
        } else if (state == 9) {
            if (*(int32_t *)P4_G2H((uint32_t)w + 0x20u) == sWantCursor) {
                p4pc_pad_inject(PAD_CROSS, 8);
                p4pc_pad_autopress_set_origin(flip, originT);
                mode = 2;
                fprintf(stderr, "p4pc: title driver: %s confirmed at flip %u, script clock %g\n", sWantCursor ? "NEW GAME" : "LOAD GAME", flip, originT);
            } else {
                p4pc_pad_inject(PAD_DOWN, 8);
            }
        }
    }
}

/* PS2 boot, EE side. overrides/00100350.c replaces kwlnInitIop with a call to
 * p4pc_platform_init, because the IOP reset, module loading and RPC binding need an
 * IOP the port does not have. The retail retry loops in that code wait for IOP
 * replies that never arrive.
 *
 * What is kept (the EE side of retail kwlnInitIop):
 *   - the TIMER0 callback on INTC cause 9, and enabling it
 *   - the TIMER0 registers the game programs before it uses the timer
 *   - the two kwln globals it clears
 *   - the allocator slots 0x008873E8..0x008873F4, filled from the RenderWare memory table
 *
 * What is not run:
 *   - func_00424f28, func_00429c18, func_0042eae0, sceCdMmode, func_0042b098:
 *     IOP and CD reset plus RPC binding. The retries never end without an IOP.
 *   - sceSifSyncIop: waits for the IOP.
 *   - func_0042ab68 (module loads).
 *     memcardInitBackend (memory card start-up) is run below; its IOP RPC bind, func_0043a978, is a no-op override.
 *     Their owners add their init calls to p4pc_platform_init below.
 *   - func_004216d0 (syscall 0x2f), whose result goes to D_00764280. Modelled in kernel_thread.c, which returns the boot thread id.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "p4pc/gmem.h"
#include "p4pc/platform.h"
#include "config/p4cfg.h"
#include "pad/pad_sdl.h"
#include "sound/p4snd.h"

/* TIMER0 callback that kwlnInitIop registers (overrides/00100310.c). */
extern int32_t kwlnHandleTimerInterrupt(int32_t arg0);

extern int func_0042eae0(int mode);
extern void memcardInitBackend(void);
extern void sdkPadInit(void);
extern int32_t (**chkmemInitRenderWareMemoryFunctions(void))(uint32_t);

/* kwln globals that kwlnInitIop clears (defined by the kwln unit, retail data). */
#define iGpffffb19c P4_GPDATA(int32_t, 0xb19c)
#define iGpffffb1a0 P4_GPDATA(int32_t, 0xb1a0)
#define iGpffffb214 P4_GPDATA(int32_t, 0xb214)

#define EE_T0_COUNT 0x10000000u
#define EE_T0_MODE 0x10000010u
#define EE_T0_COMP 0x10000020u
#define EE_T0_HOLD 0x10000030u
#define INTC_TIMER0 9

/* Boot has two COUNT reads separated by RpRandomSeed's 310 warmup draws.
 * The virtual scheduler cannot advance during that straight-line native work.
 * Retail's read-only trace observes 0x14bc -> 0x16c3 (519 TIMER0 ticks).
 * Fold that measured EE execution time into the second MMIO read, not either
 * RNG function. All normal boots use this; the differential harness is uninvolved.
 * The next kwlnRunFrame resets COUNT and changes MODE to 0x83 before its first
 * profiling read, so the boot progression must not repeat on later frames. */
static unsigned sTimer0BootReads;

void p4pc_timer0_boot_reset(uint32_t initial_count) {
    sTimer0BootReads=0;
    P4_G32(EE_T0_COUNT)=initial_count&0xffffu;
}

uint32_t p4pc_timer0_boot_count(void) {
    if (sTimer0BootReads==1) P4_G32(EE_T0_COUNT)=(P4_G32(EE_T0_COUNT)+0x207u)&0xffffu;
    if (sTimer0BootReads<2) sTimer0BootReads++;
    return P4_G32(EE_T0_COUNT);
}

/* P4PC_LOG_RANDOM=1: one line per RpRandomSeed (the boot seed, the floor builder's seed): the seed, the generator draws since the previous seed and the flip,
 * for comparing with retail's `p4hook --call-trace 003b6f00:FILE --call-count 003b7060` (draws=). */
uint32_t p4pc_rng_draws;
void p4pc_rng_note(int32_t seed) {
    static int on = -1;
    static uint32_t last;
    if (on < 0) on = getenv("P4PC_LOG_RANDOM") != NULL;
    if (on) fprintf(stderr, "p4pc: RngSeed seed=%08x draws=%u since_last=%u flip=%u\n", (uint32_t)seed, p4pc_rng_draws, p4pc_rng_draws - last, p4pc_flip_count());
    last = p4pc_rng_draws;
}

/* P4PC_LOG_RANDOM=N: also the value of every draw from draw number 311 on (after the boot seed's own 310) up to N, as `RngDraw n value flip=f`. */
void p4pc_rng_value(uint32_t draw, uint32_t value) {
    static long limit = -1;
    if (limit < 0) limit = getenv("P4PC_LOG_RANDOM") ? strtol(getenv("P4PC_LOG_RANDOM"), NULL, 0) : 0;
    if (limit > 1 && draw > 310 && draw <= (uint32_t)limit) fprintf(stderr, "p4pc: RngDraw %u %08x flip=%u\n", draw, value, p4pc_flip_count());
}

void p4pc_platform_init(void) {
    p4cfg_load();
    p4snd_set_user_volume((float)p4cfg_float("audio.volume", 100.0) / 100.0f);
    if (p4pc_intc_add_handler(INTC_TIMER0, kwlnHandleTimerInterrupt, 0) != INTC_TIMER0) {
        fprintf(stderr, "p4pc: AddIntcHandler(TIMER0) refused\n");
        exit(2);
    }
    /* The first boot COUNT value is calibrated from retail. The second read
     * progresses during the native RNG warmup (p4pc_timer0_boot_count above).
     * P4PC_T0_COUNT overrides the initial 16-bit count, not the progression. */
    p4pc_timer0_boot_reset(getenv("P4PC_T0_COUNT") != NULL ? (uint32_t)strtoul(getenv("P4PC_T0_COUNT"), NULL, 0) : 0x14bcu);
    P4_G32(EE_T0_COMP) = 0;
    P4_G32(EE_T0_HOLD) = 0;
    P4_G32(EE_T0_MODE) = 0x281;
    if (p4pc_intc_enable(INTC_TIMER0) != 0) {
        fprintf(stderr, "p4pc: EnableIntc(TIMER0) failed\n");
        exit(2);
    }
    iGpffffb19c = 0;
    iGpffffb1a0 = 0;
    /* P4PC_BOARD_SEED=N: FldBoardGenerate's debug override (iGpffffb214): every dungeon floor is built from seed N instead of the frame counter */
    if (getenv("P4PC_BOARD_SEED") != NULL) iGpffffb214 = (int32_t)strtoul(getenv("P4PC_BOARD_SEED"), NULL, 0);
    /* Install the matched retail chkmem table before any RW allocation. On
     * 64-bit builds its function words remain guest code addresses; the i386
     * mirror stores native code addresses, like the rest of its callback tables. */
    memcpy(P4_G2H(0x008873E8u), chkmemInitRenderWareMemoryFunctions(), 16);
    /* sceCdInit (func_0042eae0, src/platform/files/sce_cdvd.c) mounts the disc and the override
     * directory for the host VFS; retail kwlnInitIop calls it between the CD reset and the IOP
     * sync that this function replaces. */
    func_0042eae0(0);
    p4pc_pad_autopress_set_clock(p4pc_flip_count);
    /* Pad start-up: retail kwlnInitIop calls sdkPadInit after the module loads (scePadInit and the
     * port opens through src/platform/pad/ps2_pad.c). Without it sdkPadUpdateAll never saw a button. */
    sdkPadInit();
    /* Memory card start-up (retail kwlnInitIop calls it after sdkPadInit): libmc2 Init and the
     * CreateSocket whose id h_memcard.c keeps. Without it every sceMc2 call fails on the socket. */
    memcardInitBackend();
}

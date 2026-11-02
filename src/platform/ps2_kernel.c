/* PS2 kernel entry points, ported as platform code.
 *
 * func_0042bcd8 is libkernl's init (_InitSys). It sets up the PS2 kernel's
 * thread, exception and timer state. The port has no PS2 kernel, and that state
 * is not modelled, so the call does nothing.
 *
 * func_00421a60 is a bare `syscall 100` stub (asm/code1.s). The kernel call it
 * makes is not modelled, so it does nothing.
 *
 * The malloc family (func_0043ece8, func_0043ed08, func_0043ef80, func_0043e170)
 * is in kernel_heap.c + ee_heap.c: retail guest-word newlib over the boot arena.
 *
 * DMAC (AddDmacHandler, RemoveDmacHandler, EnableDmac, DisableDmac and the raw syscall wrappers
 * around INTC enable/disable): the same one-callback-per-cause tables, DMAC causes 0..15. The port
 * emulates no DMA channel, so a DMAC handler is registered and masked but never raised.
 *
 * INTC (AddIntcHandler, EnableIntc): a chain of up to INTC_CHAIN callbacks per cause, 0..31, run in
 * order when the cause is raised. The thread model (kernel_thread.c) raises VBLANK_START at 60 Hz
 * through p4pc_intc_raise; the game puts two handlers on it (the shadow module's and the sound
 * server's).
 *
 * All of these are defined here rather than generated as traps, so start-up runs
 * past them. None stands in for unported game logic. */
#include <stdint.h>

#include "p4pc/platform.h"
#include "p4pc/guest.h"
#include "p4pc/tables.h"

#define P4PC_INTC_CAUSES 32
#define P4PC_DMAC_CAUSES 16

#define INTC_CHAIN 8
static int32_t (*intc_handler[P4PC_INTC_CAUSES][INTC_CHAIN])(int32_t);
static uint32_t intc_enabled;
static int32_t (*dmac_handler[P4PC_DMAC_CAUSES])(int32_t);
static uint32_t dmac_enabled;
static uint64_t intc_stat;

uint32_t p4pc_game_frame_count(void) {
    return *(const uint32_t *)P4_G2H(0x00764290u);
}

void func_0042bcd8(void) {}

void func_00421a60(int arg0) {
    (void)arg0;
}

/* libkernl DIntr (0x42ba20) and EIntr (0x42ba70): the game brackets critical sections
 * with them. Retail masks SR bit 0x10000 and returns 1 if interrupts were enabled before
 * the call, else 0. DI clears the bit and EI sets it. The port runs one host thread with
 * no interrupt dispatch, so only this flag is modelled. It starts enabled, as the kernel
 * leaves it after boot [INFERENCE: the boot path sets no state, so enabled is the default]. */
static uint32_t ps2_intr_enabled = 1;

int32_t func_0042ba20(void) {
    int32_t was = ps2_intr_enabled != 0;
    ps2_intr_enabled = 0;
    return was;
}

int32_t func_0042ba70(void) {
    int32_t was = ps2_intr_enabled != 0;
    ps2_intr_enabled = 1;
    return was;
}

/* A handler's id is cause + 32 * its slot in the chain, so the first handler of a cause has the cause as
 * its id. `next` is where the new handler goes: 0 first, -1 last (what func_00418b18 passes); any other
 * value would name an existing handler to insert before, which is not modelled. */
int32_t p4pc_intc_add_handler(int32_t cause, int32_t (*handler)(int32_t), int32_t next) {
    int32_t n = 0, i;
    if (cause < 0 || cause >= P4PC_INTC_CAUSES || handler == NULL || (next != 0 && next != -1))
        return -1;
    while (n < INTC_CHAIN && intc_handler[cause][n] != NULL)
        n++;
    if (n == INTC_CHAIN)
        return -1;
    if (next == 0) {
        for (i = n; i > 0; i--)
            intc_handler[cause][i] = intc_handler[cause][i - 1];
        intc_handler[cause][0] = handler;
        return cause;
    }
    intc_handler[cause][n] = handler;
    return cause + P4PC_INTC_CAUSES * n;
}

int32_t p4pc_intc_disable(int32_t cause) {
    if (cause < 0 || cause >= P4PC_INTC_CAUSES)
        return -1;
    intc_enabled &= ~(1u << cause);
    return 0;
}

int32_t p4pc_intc_enable(int32_t cause) {
    if (cause < 0 || cause >= P4PC_INTC_CAUSES || intc_handler[cause][0] == NULL)
        return -1;
    intc_enabled |= 1u << cause;
    return 0;
}

/* Delivers an enabled cause through its ordered native handler chain.
 * The INTC cause bit is set during delivery and cleared when the chain finishes. */
void p4pc_intc_raise(int32_t cause) {
    int32_t i;
    if (cause < 0 || cause >= P4PC_INTC_CAUSES || !(intc_enabled & (1u << cause)))
        return;
    intc_stat |= 1ull << cause;
    for (i = 0; i < INTC_CHAIN && intc_handler[cause][i] != NULL; i++) {
        intc_handler[cause][i](cause);
    }
    intc_stat &= ~(1ull << cause);
}

int32_t p4pc_intc_is_enabled(int32_t cause) {
    return cause >= 0 && cause < P4PC_INTC_CAUSES && (intc_enabled & (1u << cause)) != 0;
}

/* INTC_STAT is at 0x1000f000, distinct from GS CSR at 0x12001000. */
uint64_t p4pc_intc_stat_read(void) {
    return intc_stat;
}

/* Preserve the native display gate's INTC snapshot. */
uint64_t p4pc_gs_csr_read(void) {
    return intc_stat;
}

/* INTC syscall wrappers at their retail addresses. AddIntcHandler (0x4214c0, syscall 0x10)
 * returns the handler's id on success (the cause for the first handler of a cause). RemoveIntcHandler
 * (0x4214e0, syscall 0x11) takes that id and unlinks the handler; an id that does not name a live
 * handler of the cause is refused. EnableIntc
 * (0x422328) brackets the enable with DIntr/EIntr only when interrupts were enabled, as the
 * retail wrapper does. Failures return -1. */
int32_t func_004214c0(int32_t cause, int32_t (*handler)(int32_t), int32_t next) {
    return p4pc_intc_add_handler(cause, handler, next);
}

int32_t RemoveIntcHandler(int32_t cause, int32_t handle) {
    int32_t slot, i;
    if (cause < 0 || cause >= P4PC_INTC_CAUSES || handle < 0 || handle % P4PC_INTC_CAUSES != cause)
        return -1;
    slot = handle / P4PC_INTC_CAUSES;
    if (slot >= INTC_CHAIN || intc_handler[cause][slot] == NULL)
        return -1;
    for (i = slot; i < INTC_CHAIN - 1; i++)
        intc_handler[cause][i] = intc_handler[cause][i + 1];
    intc_handler[cause][INTC_CHAIN - 1] = NULL;
    if (intc_handler[cause][0] == NULL)
        intc_enabled &= ~(1u << cause);
    return 0;
}

int32_t func_00422328(int32_t cause) {
    int32_t was = ps2_intr_enabled != 0;
    int32_t r;

    if (was)
        func_0042ba20();
    r = p4pc_intc_enable(cause);
    if (was)
        func_0042ba70();
    return r;
}

/* DMAC enable mask. The kernel's EnableDmac/DisableDmac syscalls (0x421540/0x421550) switch a
 * DMA channel cause on or off; the port has no DMA interrupt delivery, so only the mask is
 * modelled. */
static uint32_t dmac_enabled;

/* func_004222c0 is the DisableIntc wrapper: DIntr when interrupts are enabled, the syscall
 * (0x421530), then EIntr again. The syscall clears the cause's enable bit; a cause outside
 * 0..31 is refused with -1, as EnableIntc does. */
int32_t func_004222c0(int32_t cause) {
    int32_t was = ps2_intr_enabled != 0;
    int32_t r = 0;

    if (was)
        func_0042ba20();
    if (cause < 0 || cause >= P4PC_INTC_CAUSES)
        r = -1;
    else
        intc_enabled &= ~(1u << cause);
    if (was)
        func_0042ba70();
    return r;
}

/* func_00422390 is the DisableDmac wrapper around syscall 0x421550, bracketed like
 * func_004222c0. */
int32_t func_00422390(int32_t cause) {
    int32_t was = ps2_intr_enabled != 0;
    int32_t r = 0;

    if (was)
        func_0042ba20();
    if (cause < 0 || cause >= 32)
        r = -1;
    else
        dmac_enabled &= ~(1u << cause);
    if (was)
        func_0042ba70();
    return r;
}

/* Raw EE kernel syscall wrappers at their retail addresses (libkernl, each `li v1,N; syscall`).
 * Failures return -1; success returns 0 (AddDmacHandler returns the cause as its handle), the
 * convention of the INTC wrappers above. */

/* AddDmacHandler (0x12). One handler per cause; chaining (next != 0) is not modelled. */
int32_t func_00421500(int32_t cause, int32_t (*handler)(int32_t), int32_t next) {
    if (cause < 0 || cause >= P4PC_DMAC_CAUSES || handler == NULL || next != 0)
        return -1;
    if (dmac_handler[cause] != NULL)
        return -1;
    dmac_handler[cause] = handler;
    return cause;
}

/* RemoveDmacHandler (0x13) takes the handle AddDmacHandler returned (the cause). */
int32_t func_00421510(int32_t cause, int32_t handle) {
    if (handle != cause || cause < 0 || cause >= P4PC_DMAC_CAUSES || dmac_handler[cause] == NULL)
        return -1;
    dmac_handler[cause] = NULL;
    dmac_enabled &= ~(1u << cause);
    return 0;
}

/* EnableIntc (0x14) and DisableIntc (0x15), the unbracketed syscalls; func_00422328 above is the
 * libkernl EnableIntc that wraps the enable in DIntr/EIntr. The i-variants (-26, -27) are the
 * same operation from interrupt context. */
int32_t func_00421520(int32_t cause) { return p4pc_intc_enable(cause); }
int32_t func_00421530(int32_t cause) { return p4pc_intc_disable(cause); }
int32_t func_00421580(int32_t cause) { return p4pc_intc_enable(cause); }
int32_t func_00421590(int32_t cause) { return p4pc_intc_disable(cause); }

/* EnableDmac (0x16), DisableDmac (0x17) and their i-variants (-28, -29). Enabling needs a
 * handler, as for INTC. */
static int32_t dmac_enable(int32_t cause) {
    if (cause < 0 || cause >= P4PC_DMAC_CAUSES || dmac_handler[cause] == NULL)
        return -1;
    dmac_enabled |= 1u << cause;
    return 0;
}

static int32_t dmac_disable(int32_t cause) {
    if (cause < 0 || cause >= P4PC_DMAC_CAUSES)
        return -1;
    dmac_enabled &= ~(1u << cause);
    return 0;
}

int32_t func_00421540(int32_t cause) { return dmac_enable(cause); }
int32_t func_00421550(int32_t cause) { return dmac_disable(cause); }
int32_t func_004215a0(int32_t cause) { return dmac_enable(cause); }
int32_t func_004215b0(int32_t cause) { return dmac_disable(cause); }

/* SifDmaStat (0x76): the state of a SIF DMA transfer id. SifSetDma (func_00424708,
 * src/platform/sound/sdr_mem.c) copies in the call and queues nothing, so every id is already
 * complete, which the callers (the sound command loop in sdkSndcom) read as a negative result. */
int32_t func_00421b80(int32_t id) {
    (void)id;
    return -1;
}

/* Syscall 0x56 (func_0042b3f0 range-checks its argument to 13..47 and forwards it). Nothing
 * reaches it and the call is not identified, so it reports failure. */
int32_t func_0042b3e0(int32_t arg) {
    (void)arg;
    return -1;
}

/* func_00422218 (retail 0x00422218): SyncDCache(start, end). It rounds both addresses down to 64 bytes and
 * writes the data cache lines back (func_00422170, syscall), with interrupts off around it. The host has
 * one coherent cache and no DMA reading stale lines, so there is nothing to write back. */
void func_00422218(uint32_t start, uint32_t end) {   /* a range of guest addresses: integers, nothing dereferences them */
    (void)start;
    (void)end;
}

/* func_004220d8 (retail 0x004220d8): the sibling of SyncDCache for the range that writes the data cache lines back and invalidates them
 * (func_00422030 instead of func_00422170): it rounds both addresses down to 64 bytes and runs the cache operation with interrupts
 * off around it. The host has one coherent cache, so there is nothing to do (the geometry and sector native reads call it after
 * filling a mesh block). */
void func_004220d8(uint32_t start, uint32_t end) {
    (void)start;
    (void)end;
}

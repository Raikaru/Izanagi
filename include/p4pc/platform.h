/* PS2 platform layer, shared by the boot code, the kernel model and the overrides.
 * The IOP and the kernel services the game calls are modelled in src/platform/.
 * See overrides/README.md for the boundary. */
#ifndef P4PC_PLATFORM_H
#define P4PC_PLATFORM_H

#include <stddef.h>
#include <stdint.h>

/* Boot. overrides/00100350.c (kwlnInitIop) calls this in place of the IOP reset.
 * Its body is src/platform/kernel_init.c. */
void p4pc_platform_init(void);
/* Calibrated TIMER0 COUNT progression across kwlnMainLoop's two boot reads.
 * Both RNGs retain their original algorithms and consume the count either side
 * of the 310 RNG warmup draws; ordinary COUNT resets/profiling stay MMIO-backed. */
void p4pc_timer0_boot_reset(uint32_t initial_count);
uint32_t p4pc_timer0_boot_count(void);

/* INTC, the EE interrupt controller: ordered handler chains for causes 0..31.
 * p4pc_intc_add_handler returns its handler id, or -1 for an invalid cause,
 * exhausted chain or unsupported insertion point. `next` is 0 first or -1 last.
 * p4pc_intc_enable returns 0, or -1 if the cause has no handler. */
int32_t p4pc_intc_add_handler(int32_t cause, int32_t (*handler)(int32_t), int32_t next);
int32_t p4pc_intc_enable(int32_t cause);
int32_t p4pc_intc_disable(int32_t cause);
uint64_t p4pc_intc_stat_read(void);
/* Read-only native display-gate status snapshot. */
uint64_t p4pc_gs_csr_read(void);
/* Delivers an enabled cause through the native handler chain, with its INTC
 * status bit set for the duration of delivery. */
void p4pc_intc_raise(int32_t cause);
int32_t p4pc_intc_is_enabled(int32_t cause);

/* Blocks the calling thread until the next 60 Hz VBLANK_START tick (src/platform/kernel_thread.c); other
 * threads run meanwhile. The GL device calls it when it presents a frame. */
void p4pc_vsync_wait(void);
/* Waits for a scheduler frame boundary without counting a presented frame. Loader/test pumps
 * use this instead of a host sleep so asynchronous hardware also progresses under virtual time. */
void p4pc_wait_frame_boundary(void);
/* Nonblocking hardware status polls service device timers and runnable kernel work. Each
 * busy poll consumes a fixed logical microsecond, so bootstrap I/O progresses before any flip. */
void p4pc_poll_devices(void);
/* Read-only logical scheduler time; querying it never polls, advances or services devices. */
uint64_t p4pc_scheduler_time_ns(void);
/* Read-only kwlnRunFrame update counter (iGpffffb1a0), independent of presentation/vsyncs. */
uint32_t p4pc_game_frame_count(void);
/* Readonly overlap query over reserved native coroutine stacks and the guest
 * stacks reported to retail. Does not initialize, schedule or poll the kernel. */
int p4pc_thread_guest_stack_overlap(uint32_t address, size_t size);
/* Readonly native coroutine incarnation/slot identity; zero before kernel init.
 * A restart gets a different key. No initialization, scheduling or guest reads.
 * Native scripting uses it to keep suspended Lua stacks off other coroutines. */
uint64_t p4pc_thread_context_key(void);

/* Frame flips so far (calls of p4pc_vsync_wait): the frame counter that P4PC_AUTOPRESS_FRAMES=1 scripts against. */
uint32_t p4pc_flip_count(void);

/* Alarms, the native API behind SetAlarm (src/platform/kernel_thread.c). The callback gets
 * its id and arg, and returns the delay in microseconds until its next run, 0 to stop.
 * p4pc_set_alarm returns the id, or -1. p4pc_cancel_alarm returns 0, or -1 if no such alarm. */
int32_t p4pc_set_alarm(uint32_t us, uint32_t (*handler)(int32_t, void *), void *arg);
/* Schedules after VBLANK at the next scheduler frame boundary. Disc/ADXF reads
 * use this minimum asynchronous publication fence, not a measured load duration. */
int32_t p4pc_set_next_frame_alarm(uint32_t (*handler)(int32_t, void *), void *arg);
int32_t p4pc_cancel_alarm(int32_t id);

/* Heap. Initialize the retail EE newlib arena and guest-word bin state.
 * Return zero, or -1 if already initialized. Storage must be mapped guest RAM. */
int32_t p4pc_heap_init(void *base, size_t capacity);
/* Readonly initialized arena bounds [begin,end). Returns 0 on success, -1 for
 * null outputs or an uninitialized/invalid arena; failure leaves outputs alone. */
int p4pc_heap_guest_span(uint32_t *begin, uint32_t *end);
/* Debug headers contain native pointers. Also protects tail guards and freed
 * quarantined payloads. Does not enable heap checking or mutate its state. */
int p4pc_heap_guest_host_overlap(uint32_t address, size_t size);

/* Native adapters requiring storage the game can address use the same retail
 * EE allocator as the canonical libc entries. Defined in kernel_heap.c. Use it
 * (not the host malloc) for any storage the game can address (pixel shadows, palettes, buffers a guest
 * struct points at). */
void *p4pc_heap_realloc(void *p, size_t n);
void *p4pc_heap_calloc(size_t count, size_t size);
void *p4pc_heap_malloc(size_t n);
void p4pc_heap_free(void *p);
/* Optional native metadata invalidation before a live allocation is released or
 * resized. No observer means no size inspection or metadata work. A competing
 * owner is refused; NULL unregisters. The callback must not mutate guest bytes. */
typedef void (*P4HeapObserver)(const void *pointer, size_t size);
int p4pc_heap_observer(P4HeapObserver observer);

#endif

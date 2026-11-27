/* The render thread (docs/platform/render_thread.md): the GL driver calls of the p4gl_gl* table, and the buffer swap, run on a thread of their own.
 *
 * The recorders sit at the bottom of the table (p4gl_gl_load installs them before the state cache), so the game thread records exactly the calls that
 * would have reached the driver, with owned copies of their data, and a dedicated thread executes them in order on the same GL context. Calls that return
 * something (or write through a pointer) wait for the thread; the framebuffer bindings the game asks for every frame are answered from a shadow.
 * P4PC_RENDER_THREAD=0 never installs the recorders; installed but not started, every recorder calls straight through. */
#ifndef P4PC_RENDER_P4GL_RT_H
#define P4PC_RENDER_P4GL_RT_H

#include <SDL3/SDL.h>

#include "p4gl_win.h"

/* The default for runs that do not set P4PC_RENDER_THREAD: 1 = on. p4gl_context.c sets it before the context opens (player runs on the desktop and Android). */
void p4gl_rt_set_default(int on);
/* 1 when the recorders should be installed: P4PC_RENDER_THREAD=1, or unset with the default on, and none of the diagnostics that call the driver
 * directly from the game thread (P4PC_GPU_TRACE, P4PC_GL_HITCH_TRACE, P4PC_GLSTATS, P4PC_GLERR, P4PC_GLFINISH) is set. */
int p4gl_rt_wanted(void);
/* Called by p4gl_gl_load with the driver's entry points just resolved into the p4gl_gl* table: keeps them and puts the recorders in the table. Pass-through
 * until p4gl_rt_start. Does nothing unless p4gl_rt_wanted. */
void p4gl_rt_install(void);
/* Hands the current context of this thread to a new render thread and starts recording. Returns 1 when the render thread runs, 0 when it cannot (recorders
 * not installed, no current context: the caller carries on without it) . */
int p4gl_rt_start(SDL_Window *window, P4GlContext context);
/* Test hook: starts the thread with no window or context (swaps and swap intervals are counted, not performed). */
int p4gl_rt_start_headless(void);
/* Executes everything recorded, ends the thread and goes back to pass-through. With `reacquire` the context is made current on this thread again; without
 * it (a lost context about to be replaced, the process exiting) it is left unbound. */
void p4gl_rt_stop(int reacquire);
int p4gl_rt_active(void);
/* Waits until the render thread has run everything recorded so far. */
void p4gl_rt_drain(void);
/* Records the buffer swap of the window (the render thread swaps) / the swap interval. A context lost at a swap is reported by p4gl_rt_context_lost. */
void p4gl_rt_swap(SDL_Window *window);
void p4gl_rt_swap_interval(int interval);
/* Binds the context to the window's current surface again (the render thread does it, in order with the commands before it): after Android replaced the surface. */
void p4gl_rt_rebind(void);
/* 1 from the moment the render thread's swap said the context is lost until p4gl_rt_stop: the thread drops its commands in between. */
int p4gl_rt_context_lost(void);
/* Called on the render thread at the end of each frame (p4gl_context.c: p4pc_fpu_check, which puts the EE float mode back after a driver call changed it). */
void p4gl_rt_set_frame_check(void (*check)(const char *where));
/* p4gl_rt_stop(0), registered with atexit by the first start: a process that exits with the thread running leaves with the context unbound. */
void p4gl_rt_stop_at_exit(void);
/* The end of a frame: hands the rest of the frame's commands to the render thread and, when it is still more than the allowed number of frames behind
 * (P4PC_RENDER_THREAD_DEPTH, default 1), waits for it. */
void p4gl_rt_frame_end(void);
/* Frames executed so far, and a per-frame counters report (P4PC_RENDER_THREAD_STATS=1, every 1000 frames, stderr). */
unsigned long long p4gl_rt_frames_done(void);
/* Counters for tests: records, rpcs, chunks submitted, name pool misses, swaps, swap intervals. */
void p4gl_rt_counters(unsigned long long out[6]);

#endif

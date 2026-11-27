# Render thread

[Platform index](../platform.md) · [Performance](performance.md) · [Renderer](../renderer_port.md)

The GL driver calls and the buffer swap run on a thread of their own, so a crowded frame no
longer spends a third of the game thread's time inside the driver. Guest threads stay on one
host thread, and the game's logic, scene traversal and the translation of its requests into GL
calls are unchanged.

## What moves

`src/render/p4gl_rt.c` puts a recorder under every entry point of the `p4gl_gl*` table, below
the redundant-state cache of `p4gl_gl.c`. The game thread records exactly the calls that would
have reached the driver, in order, with owned copies of their data (pixels, buffer data,
uniform values, name lists). The render thread replays them on the same context. The pictures
are therefore identical to the unthreaded path: the same driver calls in the same order.

- Calls that return a value or write through a pointer (`glGetError`, `glReadPixels`,
  shader and program queries, `glGetUniformLocation`, `glCheckFramebufferStatus`, most of
  `glGetIntegerv`) wait for the thread. Captures, the viewer's readback and the gates read
  pixels this way.
- The limits and the framebuffer bindings the game asks for every frame are answered from a
  shadow without waiting.
- `glGen*` names come from a pool the thread keeps filled; an empty pool is a round trip.
- Uploads whose size cannot be computed from the recorded pixel-store state wait for the
  thread instead of being copied.

## Frame pacing

`p4gl_context_present` hands the frame to the thread and returns; the swap runs there. The
game thread waits at the end of a frame only while the thread is still on the frame before
(`P4PC_RENDER_THREAD_DEPTH`, default 1), so the display is at most one frame behind what the
game has drawn and the swap interval is honoured as before. The 60 Hz tick the game paces
itself by is the host clock's and is unchanged.

## Context ownership

The context is created and loaded on the game thread (probes, the present target and the
settings are done there too), then made current on the render thread. It comes back to the
game thread at shutdown and around a context replacement (EGL loss at a swap, SDL's device
reset after an Android resume), which run unchanged on the game thread. The render thread
starts in the EE float mode (`p4pc_fpu_ee_mode`) and puts it back after each frame, as the
game thread does, because the driver's rounding and the scene gates' pixels follow it.

## Switches

- `P4PC_RENDER_THREAD=0` never installs the recorders; the code path is the unthreaded one.
  `=1` forces it. Unset: on for player runs on Windows, Linux and Android (not for test and
  harness runs, and not on macOS and iOS, where it has not been tried).
- `P4PC_RENDER_THREAD_DEPTH=1..4` allows more frames in flight (more latency, more slack).
- `P4PC_RENDER_THREAD_STATS=1` prints, every 1000 frames, the records, bytes and round trips
  per frame, the time the game thread waited for the fence, for round trips and for chunks,
  and the time the thread was busy. It also lists the fourteen kinds of record that took the
  thread longest (count, time and bytes per frame, timed one by one, so each includes about
  0.4 us of clock reads on a phone) and how often a draw follows another with no state record
  in between ("state runs": the draws that could share a call if their per-draw values came
  from a buffer).
- `P4PC_RT_DUMP_FRAME=<n>` prints every record of the n-th finished frame
  (`rtdump <call> <first four argument words>`), for finding which calls a scene makes.
- `P4PC_RT_MERGE=0` turns off the merging of immediate-mode draw runs (below).
- The diagnostics that call the driver from the game thread (`P4PC_GPU_TRACE`,
  `P4PC_GL_HITCH_TRACE`, `P4PC_GLSTATS`, `P4PC_GLERR`, `P4PC_GLFINISH`) switch it off.

## Merged draw runs

HUD sprites, text and effect quads reach the driver as runs of "upload the vertices, draw them" (`BufferData` of an `ARRAY_BUFFER`, then `DrawArrays` of a triangle strip or list
from vertex 0) with no other call between, because the state filters already removed every repeated state call. The executor looks ahead along the chunk and, for three or
more such pairs of one vertex size (the 64-byte Im2D vertex, the 40-byte clip vertex), one mode and one usage, issues one upload of all the vertices and one draw instead.
Lists are concatenated. A strip of an even vertex count is joined to the next by repeating its last vertex and the next one's first: the two zero-area triangles that makes
cover no pixel, the primitives still come in the same order (a draw rasterises its primitives in order, so overlapping sprites blend exactly as before), and these programs
have no flat inputs that the joins could move. The last pair of a run is left as it was, so the buffer ends up holding the vertices a later draw without a new upload (the second
draw of an alpha-test pair) expects. `P4PC_RENDER_THREAD_STATS=1` adds a line with the merged runs and the driver calls they saved per frame.

## Tests

`render_thread_test` drives the stream against a fake driver with no window: order, the
thread each call runs on, owned copies of every payload, round trips, the shadow, the name
pool and the frame fence. `gl_cache_rt` and `render_smoke_rt` run the existing GL tests with
the thread on.

## Measured

Galaxy S20+ (Snapdragon 865, Adreno 650), one debug APK, `P4PC_RENDER_THREAD=0` against `=1`,
scripted walks on the same 12/05 save, `P4PC_PACE_TRACE` over about 4000 flips each. The
game thread's CPU per flip is its own thread clock.

| Area | Thread | Avg flip | p95 | 2-tick flips | 3-tick | 4+-tick | Game-thread CPU/flip |
|---|---|---|---|---|---|---|---|
| North Shopping District | off | 39.6 ms (25.2 fps) | 66.6 ms | 73% | 17% | 10% | 12.4 ms |
| North Shopping District | on | 33.4 ms (29.9 fps) | 34.5 ms | 100% | 0% | 0% | 6.6 ms |
| South Shopping District | off | 38.1 ms (26.2 fps) | 66.1 ms | 78% | 15% | 7% | 12.4 ms |
| South Shopping District | on | 33.4 ms (29.9 fps) | 34.5 ms | 100% | 0% | 0% | 6.7 ms |

With the thread on, a North Shopping frame records about 6500 commands (810 KB), the
render thread is busy 8.1 ms of each 33 ms (1.0 ms of it in the swap) and the game thread
waits for it less than 0.01 ms per frame. A forced context loss and a real pause and resume
(home key, task back to the front) leave the game running with the thread on.

The nightly run repeats the 64-bit scene gates with `P4PC_RENDER_THREAD=1`.

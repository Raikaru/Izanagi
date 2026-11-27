# Performance and profiling

[Platform index](../platform.md) · [Numeric invariants](numeric_determinism.md)

## Optimized builds

Optimized player builds use `-O2 -g1 -fno-strict-aliasing -fwrapv -fno-omit-frame-pointer`.
`P4_OPT` changes the level (`O0` only for investigating a suspect); `P4_DEBUG_INFO`
selects debug data. Decomp RenderWare `-O4` flags are followed by `-O3`. Line tables
and frame pointers retain profiler/debugger/crash-stack usefulness.

The decomp uses `-mno-implicit-float` on the reference x86 mirror so EE 128-bit byte
copies do not become aligned `movaps` operations on 4-aligned m2c locals. Do not fix
undefined behavior by dropping optimization. Mirrored `RwMatrix` layouts and matrix-bearing
locals keep required 16-byte alignment. Misaligned quadword interpreter cases are invalid
cases, like misaligned word loads, not evidence that native unaligned typed access is safe.

x86 `p4_vu0_madd` (`src/vu0.c`) evaluates two SSE2 lanes together, preserving double
product, float truncation, double accumulator addition and final float truncation.
It is not FMA or the EE scalar adder. DAZ/FTZ, signed zero, NaN/infinity, lane masks,
vf0, broadcasts and aliases retain scalar result bits. Other CPUs keep the scalar path.
`vu0_helpers` compares 262,144 deterministic edge/full-bit-pattern cases across masks
and broadcasts on both host ABI variants.

## Player telemetry

Player runs log frame-time statistics per area, game-thread CPU time and audio underruns without any switch; see
[diagnostics](diagnostics.md). The switches below are for investigations.

## Profiling

- `P4PC_PROFILE=<hz>` samples the game thread (`src/platform/profile.c`): Linux SIGPROF
  CPU time; Windows suspends it from a sampler thread and observes wall time, including waits.
  Unset and `P4PC_PROFILE=0` both leave the sampler off; ordinary players enable
  neither the sampler nor its report writer.
- `P4PC_PROFILE_EVERY=<flips>` selects report interval. `P4PC_PROFILE_HANG=<seconds>`
  emits the last samples if no flip arrives for that interval.
  `P4PC_PROFILE_FROM=<flip>` delays starting the sampler; the Windows sampler
  owns a separate waitable timer and closes it at thread exit. Optional
  `P4PC_PROFILE_TIMESTAMPS=1` precedes each existing sample with `PROF-AT <ns>`
  and records report output as `PROF-REPORT <start_ns> <end_ns> flip <n>`.
  The ring retains the most recent2048 records (160KiB on x64 including timestamps).
  `P4PC_PROFILE_HITCH_MS=<integer milliseconds>` additionally dumps on the game
  thread after a slow flip, rather than printing while it is suspended.
  `PROF-HITCH <start_ns> <end_ns> flip <n> samples <count>` identifies that
  interval; the report's own work cannot retrigger another hitch dump.
  Each timestamp/sample pair holds the stdio lock so concurrent audio span
  records cannot split the existing sample wire format.
  At high sample rates choose a bounded burst and report outside the interval
  being diagnosed: printing samples and enumerating modules itself causes stalls.
  The original `MOD`, `PROF` and `PROF-END` records remain unchanged.
- `tools/profile_report.py boot.log symbols.txt` needs `nm -n` / `llvm-nm -n` output
  from the exact executable. Windows relocation/ASLR resolution also needs
  `--image path/to/p4pc64w.exe`. Full-width `PROF` PCs and `MOD base size name` ranges
  preserve x64 identity. Older low-word logs also need the matching image.
- A relocated Windows log without relocation input is rejected, not misattributed to the
  final symbol. Reports show self/inclusive samples, modules and the first executable caller
  recovered from library stacks (a frame-pointer-dependent heuristic, not ETW/perf attribution).
- `P4PC_PROFILE_SPANS=1` adds native `PROF-SPAN` records for physical VFS reads,
  synchronous reads, asynchronous submit/completion/wait boundaries, public EE
  allocator calls, audio mixing/RPC/ADX decoder calls, and Windows sleep calls.
  Fields are start/end monotonic nanoseconds, thread identity (Windows TID;
  POSIX pthread token), operation, requested count, result, detail and context
  (the rest of the line). Counts/details are operation-specific: heap calloc
  retains count/element-size separately; physical disc detail is byte offset.
  The accepted ADXF partition state-machine body remains untouched: its physical
  reads are observable through VFS, but ADXF-specific submit/completion spans
  are not emitted.
  `PROF-EXTENT <disc_offset> <size> <canonical_key>` dumps the already-built
  immutable disc index once; raw-sector reads can intersect several extents and
  must not be guessed to belong to one file. Physical read spans exclude the
  existing provenance observer, which still sees the same bytes in the same order.
  Absolute `P4PC_PROFILE_SPANS_FROM_NS`/`P4PC_PROFILE_SPANS_UNTIL_NS` bound logging.
  `P4PC_PROFILE_SPANS_SLOW_US=<microseconds>` suppresses shorter heap, audio/mixer
  and sleep output records (1000 is a useful low-volume diagnostic setting).
  Calls are still timed; physical reads and async submit/completion/wait records
  remain complete, irrespective of this threshold. The default0 retains all
  records. This avoids hundreds of synchronous stderr writes during one effect's
  allocation burst; such full-output diagnostic stalls are not shipping hitches.
  Disabled span hooks read no clock, format no records, and resolve no paths.
  Enabled logging is diagnostic overhead, not an unprofiled speed measurement.
- `P4PC_GLSTATS=1` reports driver calls and swap time every 1000 frames (it runs the driver on the game thread; see
  [render thread](render_thread.md)).
- `P4PC_PACE_TRACE=<csv path>` records paced scheduler-return intervals and main
  host-thread CPU accounting as `flip,timestamp_ns,period_ns,thread_cpu_ns,thread_cpu_delta_ns`,
  using the same observer as `P4PC_PACE_LOG`. CPU-accounting failure is `UINT64_MAX`,
  not zero; Windows accounting resolution is coarser than its nanosecond units.
  It is disabled under virtual time. Buffered output flushes every 60 intervals;
  forced termination can lose the final partial buffer. This measures scheduler
  cadence, not display scanout. The game normally presents at **30 Hz** on the
  60 Hz scheduler: use **>40 ms** for hitches, not the legacy logger's >20 ms count.
- `P4PC_PACING=carry|legacy` chooses how a real-time run paces the game's 60 Hz tick ([video presentation](video_presentation.md#pacing)): `carry`
  (the player default) runs the scheduler on virtual time and paces it from the host clock, so a frame that needs up to two ticks of CPU still
  flips every two ticks; `legacy` counts the host clock itself, and a frame over one tick (16.7 ms) of CPU work drops to 3-4 ticks. `P4PC_PACE_LOG=1` adds
  the carried time's sleeps, resyncs and lateness to its 600-flip line. `P4PC_KT_WHY=1` prints every wait of the game thread over 0.4 ms per flip, with
  the call chain and the waker of a sleep.
- `P4PC_GPU_TRACE=<csv path>` enables native ARB timer-query timestamps on desktop
  GL or EXT disjoint-timer-query timestamps on GLES/ANGLE. Four preallocated banks
  impose at least two frames of result latency. Results are fetched only after
  the bank's final query is available; a busy ring drops a sample rather than
  stalling. Disjoint timers, context loss, unsupported capabilities, marker
  overflow and pending teardown samples have explicit non-`ok` receipts.
  A process-exit handler records pending/incomplete tails without any GL calls,
  so exit ordering cannot introduce a GPU wait or query a destroyed context.
  The deterministic host fixture sets its environment through `p4_os_setenv` so
  Windows DLL and executable CRT environments cannot diverge; Linux large-file
  `fopen64` adapters retain the same exact reviewed tracing callers as `fopen`.
  Frame timestamps surround submitted GPU work and end **before** the display swap;
  scene, framebuffer feedback/post-effects, UI, movie and present segments sum
  to the frame interval. This is GPU timeline elapsed time, not active/busy time.
- `P4PC_GL_HITCH_TRACE=<csv path>` records shader-compile/link, upload, draw,
  buffer allocation/orphaning, clear/copy/blit, readback/flush/error-query,
  timer-query submission/poll/result and display-swap **API wall elapsed** times
  in the SDL performance-counter time base. These are not thread CPU time or
  GPU execution durations; driver work may be deferred.
  Compile/link and texture-upload events retain every call.
  Other potentially blocking API rows are emitted at >=1ms to avoid millions
  of CSV writes for fast timestamp polls/draws. GL statistics retain call counts.
  The existing GL-statistics function-pointer wrappers carry these opt-in hooks;
  ordinary unobserved draws retain their original entry points.
  Shader creation remains lazy and uses the driver's normal cache; there is no
  application program-binary cache or startup prewarm. The cold-NVIDIA study
  found compile stalls only before the first ready field, during startup/title
  and loading, not in the measured town/dungeon/battle/event/Velvet gameplay.

Do not use an intrusive Windows sampled run as the unprofiled speed measurement.
The same profiler works with `p4pc_viewer`.

## Draw-call savings that keep the picture identical

The translation to GL is measured by the records a frame sends to the driver (`P4PC_RENDER_THREAD_STATS=1` prints them per frame) and, on a phone, by the time the render
thread needs for them. Each saving below has an environment switch (`=0` restores the old behaviour, for A/B captures and for diagnosing a driver) and changes no pixel.

- **Alpha-test pass culling** (`P4PC_PASSCULL`). The game's models use the GS alpha test with the failure mode "write the colour, skip the depth write", which the port draws
  as two draws: fragments that pass the test, then fragments that fail it. The fragment alpha is the product of the vertex, material and texture alpha, and each factor is
  interpolated or filtered, so the product stays inside the product of the ranges (the port keeps the smallest and largest alpha of each material's triangles, of each
  skinned mesh per upload, and of each plain texture it uploads). With a margin above the fragment shader's float error, a draw whose fragments cannot all pass skips the
  first pass and one whose fragments cannot fail skips the second (`src/render/p4gl_passcull.h`, tested against the shader's comparison by `alpha_pass_cull`). Textures
  with padding, border addressing, frame and camera textures and authored or movie uploads are treated as unknown, so they keep both passes. In a crowded street (North
  Shopping District, a phone with an Adreno 650) 394 of the 790 model draws per frame were such second passes: 313 of them were dropped, draws per frame fell from 992 to
  679 and the records sent to the driver from 6481 to 5075, with identical captures at three frames.

- **The same pass culling for UI and effect draws** (`P4PC_PASSCULL`). Immediate-mode (Im2D) sprites and Im3D/PTank effects use the same failure mode, and a typical HUD
  sprite uses ATST NEVER: its first draw discards every fragment and its second draws all of them. A NEVER draw now issues only the second draw with the alpha function
  switched off (it cannot discard anything), which also removes the two uniform calls that inverted the test around it. Im2D draws whose vertex and texture alpha bounds decide
  the test get the same treatment as model draws (plain triangles only; AA lines, rectangle-cell transport, frame or camera textures, border addressing and textures with an
  unknown alpha range keep both draws). North Shopping District on the phone (Adreno 650): the ~100 HUD quads of the frame went from 4 records per quad (two draws, two
  uniform calls) to 1 draw.

- **One vertex array per cached mesh** (`P4PC_ATOMIC_VAO`). A model's cached vertex and index buffers and its four attribute pointers are set up once in a vertex array
  object, and a draw binds that one object instead of two buffers and four pointers (six records became one). A freed cache entry hands its vertex array to the next entry
  built; the GL table has no `glDeleteVertexArrays`, and every reuse rebinds both buffers and all four pointers. North Shopping on the phone: 5075 records per frame
  became 3947 (the render thread's measured busy time 12.5 to 11.2 ms), with identical captures.

- **One uniform call per model draw** (packed per-draw values). A model draw used to set about a dozen uniforms (matrices, light directions and colours, fog, the
  alpha test, the material colour and flags), each a recorded call. The atomic shaders now read all of them from one array of 20 `vec4` (`uD`), written by one
  `glUniform4fv` when the values differ from what the program holds. The shader code is unchanged (macros name the array's parts) and every value is the same float, so the
  pixels are the same bytes; integers and flags travel as small exact floats.

- **Merged draw runs** (render thread, `P4PC_RT_MERGE`). Runs of three or more "upload four vertices, draw the strip" pairs (HUD sprites, text) become one upload and one draw
  in the render thread's executor, with the last pair left alone (see `render_thread.md`). The picture is unchanged: the primitives keep their order and the joins between strips
  are zero-area triangles. Per frame on the phone: the castle floor has 5 runs and 258 driver calls saved, North Shopping District 100 saved and the school classroom 106.

## GL state and draw windows

`src/render/p4gl_gl.c` filters redundant program/uniform/texture/sampler/buffer/VAO/viewport
state and defers fixed-function blend/depth/cull/mask/enables until a draw, clear or copy.
`P4PC_GLCACHE=0` disables this host optimization for diagnosis.
`P4PC_OFFSCREEN=1` renders to an FBO without swapping a window; it is not guaranteed
pixel-identical to a window's framebuffer and must not silently replace a gate's policy.
Virtual-clock/turbo runs set swap interval zero and limit window pumping to once per 2 ms.

`P4PC_TURBO=1` enables virtual time without draw suppression. `P4PC_TURBO=2` also drops
clear/draw calls outside capture windows. Uploads, blits and state updates still execute;
repeated display-raster feedback copies are skipped while drawing is off.
`P4PC_TURBO_WINDOW` controls pre-capture flips; menu `shot:` requests use
`P4PC_DRAW_WINDOW`. Preserve each gate's registered window: feedback-heavy scenes may
need every frame, and changing a window can alter captured pixels even with identical logic.

Undrawn flips bypass port-only Atomic/Im2D/Im3D preparation: CPU skinning, transient
vertex/index uploads, material uniforms and destination-alpha copies. Guest animation,
callbacks, texture upload and render-target lifetimes continue, as do kernel/audio clocks.
The next drawn flip builds geometry from live state; no guest work is cached/skipped.
Explicit `P4PC_DRAW_LOG`, `P4PC_DRAW_UPTO`, `P4PC_DRAW_SKIP`, `P4PC_GS_LOG`,
`P4PC_GS_SKIP` or `P4PC_GLSTATS` retains full submission even on undrawn flips, preserving
diagnostic numbering/statistics. `render_smoke` covers input preservation and deferred
transient uploads followed by correct drawn output.

## Audio and virtual time

On virtual time (`P4PC_VIRTUAL_CLOCK` or turbo), `p4snd_open` uses offline mixing with no
audio device or pacing thread. The kernel calls `p4snd_virtual_tick` (installed by
`p4pc_platform_init` as `p4pc_vblank_period_hook`) once per VBLANK period that passes, each for its
fractional share of `48000 * 1001 / 60000` samples, not once per flip: a 30 fps update spans two
periods and hears both, as the SPU2 does.
Inaudible reverb is skipped. Guest sequencers,
stream consumers and movie audio pumps therefore advance independently of host throughput.
A real-time dummy consumer is not equivalent: the game observes audio time.
Interactive play retains the real-time device. See [audio](../audio.md) and
[movie scheduling](movies.md) for the shared clock contract.

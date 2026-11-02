# Performance and profiling

[Platform index](../platform.md) · [Numeric invariants](numeric_determinism.md)

## Optimized builds

Land/player builds use `-O2 -g1 -fno-strict-aliasing -fwrapv -fno-omit-frame-pointer`.
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
  The capture runner converts case-relative `P4PC_PROFILE_SPANS_DELAY_MS` and
  `P4PC_PROFILE_SPANS_DURATION_MS` to those bounds immediately before launch.
  `P4PC_PROFILE_SPANS_SLOW_US=<microseconds>` suppresses shorter heap, audio/mixer
  and sleep output records (1000 is a useful low-volume diagnostic setting).
  Calls are still timed; physical reads and async submit/completion/wait records
  remain complete, irrespective of this threshold. The default0 retains all
  records. This avoids hundreds of synchronous stderr writes during one effect's
  allocation burst; such full-output diagnostic stalls are not shipping hitches.
  Disabled span hooks read no clock, format no records, and resolve no paths.
  Enabled logging is diagnostic overhead, not an unprofiled speed measurement.
- `P4PC_GLSTATS=1` reports driver calls and swap time every 1000 frames.
- `P4PC_PACE_TRACE=<csv path>` records paced scheduler-return intervals and main
  host-thread CPU accounting as `flip,timestamp_ns,period_ns,thread_cpu_ns,thread_cpu_delta_ns`,
  using the same observer as `P4PC_PACE_LOG`. CPU-accounting failure is `UINT64_MAX`,
  not zero; Windows accounting resolution is coarser than its nanosecond units.
  It is disabled under virtual time. Buffered output flushes every 60 intervals;
  forced termination can lose the final partial buffer. This measures scheduler
  cadence, not display scanout. The game normally presents at **30 Hz** on the
  60 Hz scheduler: use **>40 ms** for hitches, not the legacy logger's >20 ms count.
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
- `tools/polish_capture.py --binary <exe> --cases <private JSON> --out <managed run>`
  runs serial full-render, paced, hidden/offscreen studies at 1x, 1080p and 4K.
  It records executable identity, effective diagnostic environment, process CPU
  time, working set/private commit, per-frame wall/CPU/GPU distributions and >40 ms
  hitches. Read-only working-set snapshots distinguish resident private, shareable
  and currently shared pages by observed guest range and mapped-image owner.
  Unprivileged WDDM `GPU Process Memory` counters also record dedicated, shared
  and committed GPU-process totals, with exact PID/adapter instances and explicit
  unavailable statuses rather than invented zeroes. These totals do not identify
  individual textures or render targets. Image ownership does not attribute a
  library's private heap: audio/decoder/GL heap ownership still needs native counters.
  Owner schema2 includes the observed NVIDIA `nvgpucomp` compiler image in the GL
  driver bucket; older receipts put it in other libraries and cannot be silently
  reclassified from virtual-region sizes. Executable-resident bytes are already
  retained per owner as a subset of resident bytes.
  Each external memory-observer interval records its own wall duration so sampling
  work can be considered when correlating frame outliers.
  **Presentation measurements must run at normal priority with observers off.**
  Visible captures default to normal OS priority, `--no-memory` and `--no-gl`;
  they retain normal rendering/presentation and the minimal frame-clock log.
  `--gl` explicitly opts into GL statistics, GPU queries and API/hitch observers;
  `--memory` explicitly opts into intrusive owner snapshots. Such diagnostic
  runs are not default player-path presentation measurements. Hidden diagnostic
  defaults remain below-normal with GL/memory observers enabled; use
  `--priority normal --no-gl --no-memory` for a hidden observer-off control.
  `--cold-nvidia-cache` moves the Windows NVIDIA OpenGL `GLCache` directory into
  each admitted run's `nvidia-driver-cache-before` before launch, preserving its
  contents rather than deleting them; `driver-cache.json` records the boundary.
  It does not clear DX/application caches, and a locked/unmovable cache fails the
  attempt rather than silently labelling a warm run cold. Use only for an
  explicitly authorized cold-driver study, with lazy compilation selected.
  `--host-context --land-state <central .git/p4pc-land directory>` records
  one-second and immediate post-`PROF-HITCH` contention receipts, using read-only
  Windows PDH counters: system CPU, other-process CPU (one-core and machine
  shares), available RAM, and hard-fault page-read operations/pages per second.
  Defender instances are also retained separately even below the top12 CPU users.
  Competing game processes are retained without that top12 cutoff. Windows PDH
  repeats bare executable names, so collection identity includes the occurrence
  ordinal before invalid-value filtering; never collapse instances by name.
  These are CPU-process identities, not proof of active GPU-context ownership.
  `--priority normal` selects real player OS process priority (already the
  visible default). The receipt records the requested priority and actual
  `GetPriorityClass` value. Below-normal runs are explicitly diagnostic, not
  evidence of player presentation stalls under shared-host load.
  `--hard-faults` adds50ms-target, timestamp-bracketed native process snapshots.
  On64-bit Windows7+ these read `SystemProcessInformation.HardFaultCount`,
  separate from total soft+hard page faults, plus working-set counts and the
  oldest surviving thread's CPU/state/wait-reason counters. Corroborate that
  thread's ID with the game PROF/span thread; it is a candidate, not a guessed
  main-thread guarantee. No privilege elevation or resident-page walk is used.
  Actual sample/query times and failures remain explicit. Fault deltas bracket
  intervals rather than claiming exact per-fault ETW timestamps; a missing
  historical process counter cannot be reconstructed from global PageReads.
  Missing/warm-up counters remain unavailable, never zero substitutes. Each
  sample records its actual QPC interval and observer cost. Land stage/active
  receipts are included with their provenance/liveness limitation. These
  observations distinguish host-correlated stalls from reproducible port work;
  missing historical context is not retrospective proof of a port or host bug.
  `--admission-priority live` uses the existing public live admission class only
  for an owner-coordinated comparison; the default remains study. It neither
  bypasses admission nor changes the production pool. `--cohort-seconds` can
  additionally bound active capture and admission across all selected cases,
  including eligibility/setup waits. Game cleanup and result finalization
  follow that deadline; use the caller's command deadline when bounding the
  entire diagnostic process, not just its visible windows.
  Launch is bracketed with the same Windows performance-counter clock as the
  native traces. Under `P4PC_PACE_TRACE`, the first retail title state7/9 emits an
  interactive-ready timestamp; this means input is accepted, not first painted
  pixel or OS-cache-cold startup. Save-load durations run from LOAD confirmation
  to the first completed field using exact flip timestamps, not raw-disc I/O time.
  Field/room witnesses, failed/incomplete walks and truncated entry tails remain
  explicit in the result instead of silently qualifying the requested scene.
  A process-cold launch is not an OS-cache-cold launch; private commit is not
  resident private memory. Diagnostic trace I/O and sampling overhead must be
  held identical on both sides of comparisons.
  An operator's explicit visible-study approval can be selected with
  `--visible --authorized-visible`. It bypasses the automatic idle/fullscreen,
  daily and user-input-abort policy, and the automatic whole-cohort wall cap.
  Use it only for that authorized study, not an unattended recurring launch.
  PAUSE cancellation, muted audio, the selected process priority (normal
  by default), bounded per-case durations and public land-priority admission
  remain active. Each receipt records
  this explicit authorization. Run one resolution per invocation to preserve
  scene ordering for a six-scene1x, then1080p, then4K comparison.
  `--visible` enables conditional presentation studies only after 30 minutes of
  Windows input inactivity and observed XInput inactivity, with neutral connected
  controllers, no foreground fullscreen app and no `<private data>/PAUSE`.
  `GetLastInputInfo` does not include XInput: a connected pad therefore needs an
  actually observed 30-minute quiet interval. `--wait-idle` observes eligibility
  without opening windows until all conditions hold; ordinary refusal exits3.
  Runs are muted, normal priority with GL/memory observers off by default,
  capped at 15 minutes wall time and at one started attempt per local calendar day.
  The hard wall cap includes waits between cases: admission checks cancellation
  while waiting for queue/slot mutexes and higher-priority lands, without changing
  their priority or ordinary study wait budgets. Input/PAUSE cancellation also
  remains active there, before another window can open. A private case's optional
  `seconds` value overrides `--seconds`; budget enough time to reach the actual
  battle/effects or Velvet Room witness, rather than qualifying a scene by its name.
  `collection0803_voidquest_effects.walk` extends the pristine collection opening
  through the existing controller-only ordinary-attack plan for40s at the first
  battle menu, then disables it. The original opening checkpoint is unchanged;
  neither route asserts victory or writes HP/RNG/event/camera state.
  Any observed keyboard/mouse/controller input aborts the run and hides/closes only
  its own windows. Eligibility/start/abort/finish records live in the managed work
  root's `polish-visible-attempts.jsonl`; do not delete its daily admission markers
  to bypass the operator policy.
  New ledger rows include QPC and monotonic nanoseconds as well as calendar time;
  calendar differences alone do not establish an elapsed-time guard violation.

Do not use an intrusive Windows sampled run as the unprofiled speed measurement.
The same profiler works with `p4pc_viewer`.

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
audio device or pacing thread. The flip calls `p4snd_virtual_tick` for its fractional
share of `48000 * 1001 / 60000` samples; inaudible reverb is skipped. Guest sequencers,
stream consumers and movie audio pumps therefore advance independently of host throughput.
A real-time dummy consumer is not equivalent: the game observes audio time.
Interactive play retains the real-time device. See [audio](../audio.md) and
[movie scheduling](movies.md) for the shared clock contract.

# Native execution accounting

`p4_os_thread_cpu_ns` in `include/p4pc/os.h` returns the calling OS thread's
actual CPU consumption in nanoseconds. It is not the game's virtual clock and
does not charge a sleeping or preempted thread as if it were interpreting a mod.
The API returns 0 on success and -1 when the OS cannot provide the counter;
performance studies must not silently substitute wall time.

Linux/Android use `CLOCK_THREAD_CPUTIME_ID`; its resolution comes from
`clock_getres`. Windows uses `GetThreadTimes`, available on the Windows 7
baseline, and converts its 100 ns units without floating-point arithmetic.
`p4_os_thread_cpu_resolution_ns` reports the quantization bound (Windows'
system-time increment, not a claim of 100 ns precision).

The Luau execution guard is deterministic rather than derived from this coarse
clock: each callback has 4096 safepoints and startup source execution has 65536.
Each mod also has 8192 frame units per presentation frame; an ordinary callback
entry and each of its safepoints consume one unit. Nested invocation safepoints
consume each active invocation's allowance but only one frame unit. The frame
allowance renews on a real presentation flip; exhaustion is sticky across
script-side pcall/coroutines. Explicit host-side source execution can reset a
completed invocation, never an active script. Original/native game work does not
consume Luau units. These guards do not preempt a running native C builtin and
are not a millisecond CPU deadline.

Each mod VM has a separate Luau callback thread per native coroutine incarnation.
Native scheduler yields therefore cannot reuse an active Lua stack; restarting
a game thread replaces its old binding state. Script execution uses the default
host FP environment, while original calls restore/preserve the game's actual
environment. These services do not expose a scripting/gameplay clock.

Native Config row/count/find/adjust/detail/open/close providers use the ordinary
callback limits, not the startup allowance. Their invocations are synchronous:
no native yielding calls/continuations and no screen registration inside a
provider callback. The native viewport and bounded RAW body handling remain
native work; these limits do not claim to preempt a font/layout C operation.

`tests/os_cpu_time_test.c` exercises the real implementation: CPU work advances
the counter, and a native sleep does not consume comparable CPU time. The test
allows Windows' coarse resolution, but does not replace either OS backend with
a fake timer. Native/Python fixture command prefixes remain the configured
executable target's responsibility; host Python must forward any emulator when
starting a cross-ABI child.

## Hook-cost acceptance

An inactive catalogued entry costs one volatile byte load and a predictable
branch. This is deliberately **not described as zero machine cost**: no
marshalling, allocation, descriptor lookup, indirect call or Luau invocation
runs in that arm. Only ABI-admitted entries in the generated `mod_hooks.tsv`
catalogue receive guards; unsupported/skipped definitions and native platform
functions are not blanket-instrumented. Overrides are installed before those
catalogue entries are generated, so the active hook wraps the actual override.

Compiled-in/no-loaded-mod acceptance requires byte-identical behavior plus
before/after overhead within measured run-to-run noise: retain both turbo-suite
wall time and paced frame CPU time on the same workload/ABI/settings. A fixed
percentage is not a substitute for those paired measurements.

The hooked-call dispatch budget is at most **10 microseconds of amortized added
thread CPU per warm no-op scalar Luau callback** on the admitted i386/x64
profiles, excluding the original game's own work. This is an acceptance budget,
not a reported measurement. Benchmark unhooked/native-only/Luau-only and
before/after chains against the same original, retain sample/batch counts,
median/p95 and CPU-counter quantization. Cold VM/compiler/setup costs are
reported separately, not hidden in or subtracted from callback execution.
Interpreter-work limits remain 4096 safepoints per callback and 8192 units per
mod frame as above; those safety limits do not establish a dispatch-overhead result.

`mod_hook_cost[64|64w]` runs the real generated scalar entry, native dispatcher
and Luau bindings against authored synthetic fixtures (never retail state). Each
case warms1000 calls then measures10 batches of20000 calls: inactive,
native observer, one Luau before-hook, and a before/after Luau chain. The real
production virtual-frame boundary advances between calls so ordinary callback
and per-frame safety budgets remain enabled; the same boundary is included in
the inactive baseline. Setup, per-case configuration and warm-up CPU are reported
separately. No wall-clock or virtual-time fallback supplies a CPU measurement.

The report retains raw batch CPU intervals, total/median/p95, checksum, pointer
width, CPU counter quantum, added mean CPU per callback, and the bound on
counter-quantization uncertainty. The10-microsecond check compares accumulated
added CPU with that explicit uncertainty, not a rounded per-call value. A Wine
run measures that actual adapter execution, not native-Windows timing. These
microbenchmarks do not replace paired retail-workload inactive-guard/noise
qualification, Android gameplay receipts, or no-loaded-mod byte identity.

## Real frame windows and paired guard study

`P4PC_FRAME_CPU_LOG=1` enables actual game-thread CPU/wall windows at the
production vsync-return boundary. Each complete window covers at least1000
actual flips and records first/last flip, frame count, thread CPU nanoseconds,
wall nanoseconds, actual CPU counter quantum and virtual/real clock mode.
The measurements live in host-only statics/logs; they never feed guest state,
RNG, game clocks, pacing or budgets. Disabled measurement makes no new clock
queries; enabled measurement queries only at window boundaries, not every flip.
Unavailable/non-monotonic counters emit an error, never a wall-time CPU fallback.
CPU time excludes sleeping/descheduling but includes the actual game-thread
work between those boundaries; it does not include work on other OS threads.

When comparing builds, keep the workload, configuration and measurement mode
consistent. Report variation and counter resolution alongside timing results;
microbenchmarks alone do not establish gameplay performance.

`mod_frame_cpu[64|64w]` defines a real cooperative-kernel/OS-counter fixture
without loaded mods, testing disabled logging and contiguous native counter
windows. This fixture is not an observed game performance result.




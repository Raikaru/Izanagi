# Boot, threads and kernel

[Platform index](../platform.md)

## Startup and memory

`src/crt.c` implements `_start` (retail `0x00100008`): platform initialization no-ops
`func_0042bcd8` and `func_00421a60`, native `kwlnBoot` (`func_001002c0`), then exit glue
`func_0043dc60`. The IOP reset override (`overrides/00100350.c`) calls
`p4pc_platform_init()` in `src/platform/kernel_init.c` instead of rebooting an IOP.
Unimplemented calls remain generated traps; normally the first trap aborts.
`P4PC_TRAP_CONTINUE=1` allows diagnostic continuation, not a successful implementation.

The CRT clears only BSS, `[0x00764280, 0x00938A00)`. The preceding initialized ELF data
must not be zeroed. Compare RAM only after retail has finished loading the ELF: the trace
point named `rw` can observe partially loaded data and is not an engine-init boundary.
The [64-bit memory contract](../port64.md) keeps guest addresses four bytes wide.

`src/platform/kernel_heap.c` bridges the newlib malloc family (`0043ece8`,
`0043ed08`, `0043ef80`, `0043e170`, `0043c710`, `00441558`) to the literal
retail EE implementation in `ee_heap.c`. Chunk headers/bin links remain four-byte
guest words, with original alignment, coalescing, reuse, calloc clearing,
realloc and trim behavior. The active guest reent and errno are retained.
The boot arena starts at `0x00938A00`, capacity `0x1627600`
(`overrides/00100230.c`); only sbrk/EndOfHeap is a mapped-RAM OS seam.
RW uses the matched chkmem wrappers/table, including its tracking header and
payload-4 backpointer. Host buffers remain host-allocated. See the
[CRT boundary and allocator lifetime evidence](crt_boundary.md).

`P4PC_HEAP_CHECK=1` deliberately perturbs that layout with a debug header
(48 bytes on i386, 80 on 64-bit hosts), 16-byte tail guard, freed-block poisoning,
a 2 MiB / 8192-block quarantine, verification each flip and every 32 allocations, and
allocation-site diagnostics before aborting. `=2` additionally fills new blocks with
`0xCD`; `=3` protects quarantined blocks of at least 4 KiB against stale accesses.
Resolve reported PCs with `P4PC_GDB_SYM=0xaddr` in the boot runner.

## Fresh-image restart

Executable entry points call `p4_os_restart_init(argc, argv)` before any other
startup or exit-handler registration. `p4_os_request_restart(reason)` schedules
a restart and returns 0; it never exits or re-enters SDL. A failure returns -1
so the native error UI can retain the explanation and offer Quit/manual relaunch.
The caller must first persist its INI change, then call ordinary `exit(0)` on
success. Unsafe mod VM/provider pins remain alive until that process exits.

On desktop, the first-registered exit handler runs last, after resource cleanup.
Linux executes `/proc/self/exe` with the original argument vector; Windows uses
`GetModuleFileNameW` and an editable copy of the original Unicode command line
with `CreateProcessW`, inheriting the environment. A late launch failure is
reported after the already-shown controlled restart choice and terminates with
failure; it cannot return to unsafe game/mod execution after teardown.
The macOS path uses `_NSGetExecutablePath` plus `execv`; a packaged app must retain
its original argument/environment contract rather than restarting SDL in place.
That macOS branch needs native app-bundle/signing qualification, not a Linux claim.

Android sends the original URI-based intent and a Binder lifetime token
to a non-exported Activity in the separate `:restart` process/task. Only after
Binder death confirms that the old game process exited does it launch a clean
game task. The existing wrapped exit drains native logging; app files, media
inbox and persisted document grants survive, while descriptors are reopened.

`P4PC_TEST_RESTART=<private marker path>` performs exactly one synthetic restart
before loading retail data. The second image checks the original argument hash
and an old-image exit-handler marker; Windows/Android additionally require a
different PID. `process_restart` exercises spaces, quotes, a trailing backslash
and preserved environment through the real Linux/Windows implementations.
The Python driver explicitly receives the probe target's `CROSSCOMPILING_EMULATOR`,
so Win32 Wine runs the PE child rather than trying to execute it as a host ELF;
the host-twin factory independently supplies the Windows x64 runner.

## Cooperative EE kernel

`src/platform/kernel_thread.c` owns the thread/semaphore/alarm kernel, covering syscalls
`0x20..0x45` and their negative interrupt-context variants. The modeled operations include
creation/start/exit, dispatch control, priority and ready queues, release-wait, status,
sleep/wakeup, suspension/resumption and semaphore signal/wait/poll.

Coroutines run on one host thread through `p4_coro_*` (`include/p4pc/os.h`): ucontext on
Linux and fibers on Windows for the reference ABI, with the 64-bit context implementation
specified in [port64](../port64.md). Scheduling is cooperative, with priority preemption at
kernel calls. Timers run on kernel entry and while idle. An idle wait sleeps; a blocked
thread with no pending timer is a fatal deadlock naming that thread.
`p4pc_set_alarm` and `p4pc_cancel_alarm` own host completion alarms. See the source header
for modeled limits and explicitly inferred kernel behavior.

Success values are part of the ABI: `WakeupThread`, `SuspendThread` and `ResumeThread`
return the requested thread ID; `ReferThreadStatus` returns the flags written to its result.
Interrupt-context variants share those helpers. ADXM's VSync handshake depends on the
ID return value; returning zero is not equivalent.

`DIntr` (`0042ba20`) and `EIntr` (`0042ba70`) return the previous interrupt-enable state,
then clear/set the modeled enable flag. `_InitSys`, syscall 100 and the boot interrupt-enable
override are no-ops where the port does not model their PS2 setup.

## VBLANK and clocks

VBLANK_START comes from the physical 60 Hz source through `p4pc_intc_raise`. Ordered
handlers stop at a negative result. The first game handler rejects alternating fields
using **GS CSR.FIELD**, bit 13 at `0x12001000`, not INTC_STAT at `0x1000f000`.
FIELD advances even while INTC2 is masked. Do not halve VBLANK or add CRI delays to
reproduce the service cadence: the original field predicate determines it.

`P4PC_VIRTUAL_CLOCK=1` advances scheduler time only when all threads block, jumping to
the next timer. Repeated polling adds 1 microsecond every 4096 polls so a busy observer
cannot freeze time forever. `P4PC_TURBO` also selects virtual time. Leave virtual time off
for interactive play. Virtual audio advances at the flip boundary rather than a real-time
consumer; see [performance](performance.md#audio-and-virtual-time) and [movies](movies.md).
The ordinary real-time pacing policy is in [video presentation](video_presentation.md#pacing).

Device completion alarms publish at the [next scheduler frame](disc_cri.md#pc-disc-contract),
not in the read's submission update. Native busy-wait safepoints service the scheduler
without pumping the game's CRI publication work.

## TIMER0 boot seam

The two `kwlnMainLoop` TIMER0 COUNT reads straddle `RpRandomSeed`'s 310 warmup draws.
`kernel_init.c` models the measured `0x207` progression between them, with 16-bit wrap;
the default initial value is `0x14bc`, making the second value `0x16c3`.
`P4PC_T0_COUNT` overrides the initial value, not that progression. Neither RNG algorithm
nor the seeds passed by retail are changed. The first `kwlnRunFrame` resets COUNT and
sets MODE to `0x83`; that profiling reset must not restart boot progression.

`boot_seams` covers COUNT progression/reset/wrap and libpad deadlines without SDL or game
inputs. `kernel_threads` and its real-clock twin cover scheduling, interrupt delivery,
alarm cancellation, semaphore wakeups and next-frame device completion.

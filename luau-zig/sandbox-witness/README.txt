p4_luau sandbox behavioral witnesses
====================================

Standalone C programs that exercise a built implementation of the production
mod-script contract, include/p4pc/luau_c.h, and the lua.h/lualib.h API that
header exposes. The same sources run against two flavors:

  reference  production src/platform/mods/luau_c.cpp over vendored upstream
             Luau 0.744 (third_party/luau; MIT, see third_party/luau/LICENSE.txt
             and lua_LICENSE.txt). This is the authoritative behavior: every
             expectation here must hold for it.
  candidate  a prebuilt library implementing the same contract, normally the
             Zig archive from `zig build p4pc` (artifact p4pc-luau). Nothing
             from upstream Luau is compiled into candidate executables.

Nothing is mocked: each witness drives the real create/check/load/run API, the
real interrupt callback and the real allocator, and compares results with
independent C expectations. These complement, and do not replace, the
unchanged tests/luau_sandbox_test.c.

Exit status: 0 passed; 1 a check failed (file:line and the VM diagnostic go to
stderr); 77 the property cannot be observed on this platform (CTest SKIP, never
PASS).

Witnesses
---------
witness_globals    Exact frozen global set (base/coroutine/table/string/math/
                   utf8/bit32/buffer, including luaopen_table's Lua 5.1 compat
                   global `unpack`, plus host names, nothing else); os, io,
                   debug, package, require, load/loadstring/loadfile/dofile,
                   collectgarbage, integer/vector/class absent from _G and the
                   thread proxy; every library and the string metatable frozen
                   and inert (functions and constants only); 14 mutation
                   attempts fail with the readonly error and leave builtins
                   intact; script globals live in the per-VM thread proxy,
                   persist across chunks, shadowing cannot tamper with the
                   builtin string metatable, and nothing crosses VMs; host-made
                   threads share the sandboxed environment; bytecode-shaped
                   input is compiled as source (a leading version byte is a
                   syntax error, a leading NUL compiles to an empty chunk and is
                   never handed to luau_load); buffer/string.unpack bounds
                   checks; setfenv/getfenv and coroutine semantics retained.
witness_budget     Budget exhaustion through loops, recursion, pcall, retrying
                   pcall, xpcall with a looping handler, a script error after a
                   caught exhaustion, coroutine.resume/wrap, yield ping-pong,
                   table.sort/gsub/__index/__tostring callbacks, a host
                   lua_call and a host lua_pcall that swallows the error. For
                   each: LUA_ERRRUN with "Execution budget exhausted", stack
                   restored, the latch alone (callback made permissive) keeps
                   failing until p4_luau_reset_interrupt, then normal runs.
                   Also: per-back-edge charging, GC callbacks delivered with
                   gc != 0 without raising, a GC-phase trip that raises at the
                   next execution safepoint, the lua_pcall stack contract with
                   arguments, compile-only admission while latched, the
                   production pattern of lua_pcall on a host-created thread,
                   per-VM latches and a VM without an interrupt.
witness_memory     1 MiB cap: retained growth fails as LUA_ERRMEM "not enough
                   memory" with the bridge's accounting sampled at every
                   safepoint never above the cap and the failure within one VM
                   page of it; collection reclaims to the settled level and the
                   VM stays usable; script-side pcall and coroutine.resume see
                   the memory error as a value; oversized buffer/string.rep/
                   table.create/concatenation requests; accounting follows a
                   retained 256 KiB buffer up and down; caps below steady usage
                   refuse creation; at the smallest workable cap a large chunk
                   load fails as LUA_ERRMEM with the stack restored; caps are
                   per VM; p4_luau_create(NULL) is NULL.
witness_allocator  The VM behind p4_luau through lua_newstate with a tracking
                   allocator, opening the production library set and calling
                   luaL_sandbox/luaL_sandboxthread exactly as the bridge does,
                   then tables, strings, userdata with destructors, buffers,
                   C closures, refs, library calls, error unwinding and C-level
                   coroutines. Every free/resize must name a live block with
                   its exact size; 16-byte alignment on 64-bit; lua_close
                   releases every byte and runs every destructor once. The
                   workload is repeated with one refused allocation at every
                   request index (stride only beyond 20000 requests). Each
                   index must be reached and refused exactly once; refusals
                   must occur in lua_newstate, in library opening/sandbox and
                   in the workload, never in lua_close; at least one must
                   surface as LUA_ERRMEM; cleanup holds after every one.
witness_heap       Host heap in-use bytes (Windows HeapWalk, glibc mallinfo2/
                   mallinfo, macOS malloc_zone_statistics; otherwise SKIP)
                   around 2000 complete lifecycles: failed creation at varying
                   small caps, compile-only checks, syntax errors, oversized
                   source, normal runs, runtime errors, budget exhaustion and
                   reset, memory exhaustion, destroy. Covers the bridge object,
                   compiler scratch and compiled blobs that the VM cap cannot
                   see. Tolerance: 16 bytes per lifecycle.
                   It also checks gcinfo and the absence of collectgarbage,
                   checks weak keys, and repeatedly collects weak values while
                   checking reachable tables, buffers, userdata, closures and
                   suspended coroutines remain usable and dropped ones clear.
witness_mod        A production-shaped mod (lua_runtime.c patterns: 8 MiB cap,
                   per-invocation fuel charged at every safepoint, frozen p4 API
                   from config.initialize, registry-held handlers, lua_pcall on
                   a host-created thread, retained main chunk under startup
                   fuel) running idiomatic Luau: metatable classes, type
                   annotations, compound assignment, a long-lived coroutine
                   task, buffer/string.pack/format/utf8/bit32, table.sort with
                   a comparator. 2000 frames of events checked against a C
                   model with production-style collector steps and no steady-
                   state growth; contained handler errors with chunk positions;
                   budget failure, latch and reset; later chunks see globals
                   but not locals; two mods share nothing.

Building and running
--------------------
Paths are relative to the worktree root. Use a C11 compiler; the reference
flavor also needs a C++17 compiler (clang/clang++ as in tools/luau).

Reference (production) flavor:

  cmake -S luau-zig/sandbox-witness -B build/luau-sandbox-witness \
        -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++
  cmake --build build/luau-sandbox-witness --config Release
  ctest --test-dir build/luau-sandbox-witness -C Release -L reference --output-on-failure

Candidate (Zig) flavor, after building the archive for the same target/ABI as
the C compiler (for example -Dtarget=x86_64-windows-msvc with clang-cl/MSVC,
x86_64-windows-gnu with MinGW clang):

  (cd luau-zig && zig build p4pc -Doptimize=ReleaseSafe)
  cmake -S luau-zig/sandbox-witness -B build/luau-sandbox-witness-zig \
        -DCMAKE_BUILD_TYPE=Release -DP4PC_LUAU_WITNESS_REFERENCE=OFF \
        -DP4PC_LUAU_CANDIDATE_LIBRARY="$PWD/luau-zig/zig-out/lib/libp4pc-luau.a"
  cmake --build build/luau-sandbox-witness-zig --config Release
  ctest --test-dir build/luau-sandbox-witness-zig -C Release -L candidate --output-on-failure

On Windows the archive is luau-zig/zig-out/lib/p4pc-luau.lib. If the linker
reports symbols the archive imports from the platform (Zig compiler-rt,
ntdll/kernel32), list them in -DP4PC_LUAU_CANDIDATE_LINK_LIBRARIES=...;
libm is added automatically on non-Apple Unix. Leave both flavors enabled
(omit P4PC_LUAU_WITNESS_REFERENCE=OFF) to build and run them side by side
from one tree; label "luau-sandbox-witness" selects every witness.

Without CMake, zig cc supplies compiler-rt and libc itself:

  zig cc -std=c11 -O2 -Iluau-zig/sandbox-witness -Iinclude -Ithird_party/luau/VM/include \
         luau-zig/sandbox-witness/witness_budget.c luau-zig/zig-out/lib/libp4pc-luau.a \
         -o build/witness_budget

(repeat for witness_globals, witness_memory, witness_allocator, witness_heap,
witness_mod; add -lm on Linux if your zig cc does not already link it).
P4PC_INPUT=script P4PC_MODS=0 P4PC_PACKS=0 matches the CTest environment.
Use -DP4PC_LUAU_WITNESS_EMULATOR=wine64 (or another runner) for cross builds.

Gate: a candidate qualifies only when every witness passes for both flavors.
A reference failure means the witness, not production, is wrong and must be
fixed against the contract; never relax a candidate expectation to pass.

Known limits (reported, not relaxed)
------------------------------------
* The bridge's own lua_Alloc is private by contract. Its cap accounting is
  witnessed through p4_luau_memory_used (peak, recovery, retention), and VM
  block-size/cleanup correctness through lua_newstate in witness_allocator.
  No boundary API accepts bytecode and luau_compile is not part of the
  contract, so the tracked-allocator workload is driven through the C API
  and library functions rather than compiled scripts; script-driven cleanup
  is covered by witness_heap through p4_luau.
* witness_heap sees only the C runtime heap. Memory a candidate obtains
  directly from the OS (VirtualAlloc/mmap, e.g. a Zig page allocator) is
  invisible to it; platforms without a heap statistic SKIP with 77.
* Exhaustion raised from the interrupt uses luaL_where(L, 1), which in that
  context names the caller of the running function, so the production
  diagnostic is not required to carry the looping chunk's position. The
  witnesses check the message, never a position, for budget errors.
* `print` remains available (upstream base library, production contract); it
  writes to the process's stdout and is not a filesystem capability.
* Wall-clock/timer-driven interrupts, concurrent host threads, the Analysis
  type checker (a tooling CLI) and 32-bit Windows allocator alignment are
  outside these witnesses.

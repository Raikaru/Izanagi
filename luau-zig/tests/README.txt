Luau differential conformance (test-only)
=========================================

These tests compare consumer-observable behavior with this checkout's vendored
Luau 0.744, and separately check fixed expected results. They do not replace the
full upstream conformance suite, prove complete language support, authorize a
production runtime cutover, or use a private game image/save. The bundled mods
are real scripts executed against the project's existing synthetic p4 providers;
that is not a real-game integration test. Reference C++ is confined to these
test executables: it is never a Zig engine dependency or fallback.
The gc-sandbox fixture checks the mod-visible collector surface and keeps a
weakly referenced live table reachable through allocation stress on both
engines. It does not establish when unreachable weak entries are collected or
cover host allocator failure and GC internals.


Optimized protected-call ABI regression
---------------------------------------
Run this separately from the main test suite, from the worktree root:

  (cd luau-zig && zig build protected-c-regression -Doptimize=ReleaseSafe)

The build step uses the repository's Luau headers, a dedicated ReleaseSafe Zig
VM archive and a C host. It creates a state with a real tracking allocator,
checks a successful protected C callback, verifies that an error preserves the
exact light-userdata value and the caller's stack, exercises nested protected
unwinding and subsequent reuse, closes the state and checks allocator balance.
It targets the optimized naked-call/longjmp regression; it is not a substitute
for full VM or sandbox conformance. A native executable and the pinned Zig
nightly with LLVM are prerequisites.

Alternatively, after building a ReleaseSafe Zig VM ABI archive, configure this
test directory with LUAU_ZIG_RUNTIME_LIBRARY set to its absolute path and
LUAU_ZIG_EXECUTABLE set to the pinned Zig executable. Then run:

  cmake --build build/luau-conformance-ref --target luau_zig_protected_call
  ctest --test-dir build/luau-conformance-ref -R '^luau_zig_protected_call$' --output-on-failure

The CMake target links with zig cc, which supplies compiler-rt. CMake cannot
verify the supplied archive's optimization mode; an archive built in Debug
does not exercise the optimizer regression.

Direct-userdata fallback debugger regression
--------------------------------------------
Run `(cd luau-zig && zig build udata-debugstep-regression -Doptimize=ReleaseSafe)`.
The C host verifies that first and later calls deliver the same single-step
notifications when GET, SET and NAMECALL deoptimize to their generic forms.

Debugger local-assignment stack regression
------------------------------------------
`zig build debug-setlocal-regression -Doptimize=ReleaseSafe` runs a C host
against the Zig compiler and VM; CMake target `debug_setlocal_reference` builds
the same host against pinned upstream. It checks that invalid stack levels leave
the replacement on the stack, while a missing local in a valid frame consumes
it, and that a valid local assignment changes the running Lua function. This
extends the pinned Debugger case's local-inspection checks; it does not cover
native/JIT frames or establish full debugger conformance.

GC interrupt and allocator recovery
-----------------------------------
`zig build gc-interrupt-regression -Doptimize=ReleaseSafe` in luau-zig runs a
consumer C host against the Zig VM. The CMake reference target
`gc_interrupt_reference` runs the same host against pinned upstream Luau built
with longjmp. It raises an exact error from each of the two GC-step interrupt
positions, then requires an unreachable userdata destructor to run on a later
full collection. It also rejects an oversized allocation under a live-byte cap,
then checks collection, destructor execution and balanced close. It also
checks that zero-sized steps leave substantial live marking work incomplete
while negative step budgets use upstream's size_t conversion and complete the
cycle, retaining the caller's stack and live data. This is a focused recovery
and C API contract, not upstream's original GC case or a claim that arbitrary
errors from GC destructors and tracing callbacks are safe.

Prerequisites: Python 3, CMake >= 3.20, a native C/C++ toolchain and the pinned
third_party/luau sources. From the worktree root, build the reference side:

  cmake -S luau-zig/tests -B build/luau-conformance-ref -DCMAKE_BUILD_TYPE=Release
  cmake --build build/luau-conformance-ref --config Release --target reference_runner reference_bytecode reference_sandbox

For a single-configuration generator the reference binaries are in the build
directory; multiconfiguration generators may put them under Release/ and append
.exe. Use native matching-architecture builds; no emulator command is inferred.
Build the unchanged project script and sandbox hosts against the Zig bridge:

  (cd luau-zig && zig build script-runner sandbox-host -Doptimize=ReleaseSafe)

The candidate runner is luau-zig/zig-out/bin/luau_script_runner (with .exe on
Windows). It includes the project's sha256, emit, heap, safepoints, read_file
and chunk-result argument propagation, and links no C++ Luau implementation.

Example, with executable paths replaced by actual build outputs:

  python luau-zig/tests/conformance.py \
    --reference-runner build/luau-conformance-ref/reference_runner \
    --candidate-runner luau-zig/zig-out/bin/luau_script_runner \
    --reference-bytecode build/luau-conformance-ref/reference_bytecode \
    --candidate-bytecode build/luau-zig/bytecode_adapter \
    --reference-sandbox build/luau-conformance-ref/reference_sandbox \
    --candidate-sandbox luau-zig/zig-out/bin/luau_sandbox_test \
    --reference-mod build/native/mod_luau_test64 \
    --candidate-mod luau-zig/zig-out/bin/mod_luau_test \
    --reference-mod-save build/native/mod_luau_save_test64 \
    --candidate-mod-save luau-zig/zig-out/bin/mod_luau_save_test \
    --report build/luau-conformance-results.json

All paths are resolved by the subprocess from the worktree root; use absolute
paths when invoking the harness elsewhere. A candidate runner or adapter that
is not yet implemented is an unsupported prerequisite, not a passing test.
Omit unavailable options to obtain explicit SKIP records. Supplying a nonexistent
or crashing program is a failure, not a skip. Exit status: 0 all checks passed;
1 any failure; 77 no failure but one or more prerequisites/checks were skipped.
Failure process stdout, stderr, command and status are printed and every
observation, including successes and skips, is retained in the optional JSON.
Timeouts are configurable via --timeout and --mod-timeout. P4PC_ISO is removed
from subprocess environment; no private corpus is accessed. Only fresh temporary
directories are passed to the project host-witness programs.
On native Windows, expected-text comparisons account for C text-mode CRLF
translation; raw process output is retained in the report.

The reference and candidate sandbox programs must each compile and actually run
unchanged tests/luau_sandbox_test.c against their respective implementations of
include/p4pc/luau_c.h and the corresponding lua.h API. It exercises compile-only
admission, host userdata, readonly globals, error source/line, allocator cap,
interrupt exhaustion and stickiness/reset, including nested pcall. No separate
script-level substitute can test those host-facing guarantees.

Likewise the mod and mod-save executables must build the unchanged project
witnesses tests/mod_luau_test.c and tests/mod_luau_save_test.c against their
respective runtimes, with the real host bindings; the mod-save check runs both
normal and --callback-failure cases. Project native host target configuration,
generated bridges and platform prerequisites are in tools/mods/CMakeLists.txt;
the separate Zig `mod-luau-tests` target is described below. The reference
mod-host executables are not provided by the small reference-only build above.
The randomizer and skill-affinity scripts, when present, run with the exact
existing Python qualification drivers, including their independent vectors,
expected results, real mod execution and synthetic p4 fixtures. A missing mod
or driver is reported as SKIP and keeps the suite from exiting successfully.
Counters are intentionally not compared byte-for-byte across implementations.

Test-only bytecode adapter protocol
-----------------------------------
  ADAPTER compile SOURCE OUTPUT
  ADAPTER run BYTECODE

compile uses the compiler's default options and writes raw upstream bytecode to
OUTPUT only on successful compilation; exit 0 with empty stdout/stderr. A syntax
error exits 1, reports a diagnostic on stderr and does not write OUTPUT. Other
I/O/setup errors exit 2. run creates a fresh p4_luau-compatible sandbox with
emit(text), loads the supplied bytecode directly through the VM's test interface,
then executes it. emit writes its exact string plus newline to stdout. Successful
execution exits 0 with empty stderr; loader or runtime rejection exits 1 with a
diagnostic on stderr; setup/I/O failure exits 2. This protocol is for test
executables only; production p4_luau must not accept externally supplied bytecode.
The reference adapter is built above. A Zig adapter is an explicit prerequisite.

The harness checks default version 13 from pinned Bytecode.h, compiles each
fixture on both sides, runs both compilers' outputs on both VMs, and checks
rejection of versions 2 and 15. It checks behavior, not identical serialized
bytes: valid compiler optimizations need not emit the same instructions.
Malformed bytecode fuzzing and comprehensive version/opcode coverage require
additional tests. Syntax and runtime source-name/line errors are exercised via
the normal source runner, not guessed from source text.

Attribution: the reference helpers link vendored upstream Luau; its MIT licenses
are third_party/luau/LICENSE.txt and third_party/luau/lua_LICENSE.txt. The small
fixtures/ witnesses are independently written from the pinned interfaces and
existing project tests. The upstream suite below is copied from Luau itself.

Pinned upstream Luau.Conformance
--------------------------------
upstream-0.744 contains the unchanged tests/Conformance.test.cpp, every
tests/conformance/*.luau fixture, the other sources of the upstream
Luau.Conformance target, and the matching doctest header. The git object hashes
in manifest.json and target-manifest.json identify commit
32c28089305f488f901aaf1dbf3fd1cb65fa3d11. The drivers check every blob before
building/running adapters; the non-UTF8 fixture bytes are preserved. Upstream
MIT notices are in upstream-0.744/LICENSE.txt, lua_LICENSE.txt, and
extern/doctest_LICENSE.txt.

Parser/report regressions cover C++ source inventory, cross-platform doctest XML
filenames, duplicate identities, failure preservation and API optimization accounting. Run separately:

  python luau-zig/tests/test_upstream_inventory.py
  python luau-zig/tests/test_upstream_suite_xml.py

upstream-api-skipped-actual.xml is an unchanged XML report from the isolated
pinned upstream reference ApiAlloc case; it includes 39 filtered-out cases.
The regression ensures those skipped entries cannot pass a requested case.

Build an unchanged independent reference executable from that exact upstream
checkout (or use an existing executable demonstrably built from it):

  cmake -S /path/to/pinned-luau -B build/upstream-luau \
    -DLUAU_BUILD_CLI=OFF -DLUAU_BUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Release
  cmake --build build/upstream-luau --config Release --target Luau.Conformance

Build the Zig candidate adapters with the pinned Zig nightly. The API adapter
also requires Python 3 to generate its C++ test host from the preserved upstream
cases. From the worktree root:

  (cd luau-zig && zig build -Doptimize=ReleaseSafe upstream-script-candidate upstream-api-candidate)
  # If python3 is not on PATH, add -Dpython=/absolute/path/to/python.

The resulting test-only executables link the Zig VM, standard libraries and
compiler bridge; no upstream C++ VM or compiler is linked into either. They
accept exactly the same fixture paths and produce the same test reports as the
reference adapters. They require the underlying Zig engine to build and do not
make incomplete engine functionality available.

The separate upstream reference adapters can be built with CMake as a diagnostic
reference path (they do link the pinned upstream C++ reference):

  cmake -S luau-zig/tests -B build/luau-conformance-ref -DCMAKE_BUILD_TYPE=Release
  cmake --build build/luau-conformance-ref --config Release \
    --target upstream_script_reference upstream_api_reference buffer_cage_reference

The executable names/output directories vary on Windows and with
multiconfiguration generators.

The independent cage-boundary regression can be run against both VMs:

  (cd luau-zig && zig build -Doptimize=ReleaseSafe buffer-cage-regression)
  build/luau-conformance-ref/buffer_cage_reference

It checks cage-on allocation routing and type, protected allocator failure,
buffer contents, total-byte accounting, collection/close, and the cage-off
fallback. Its tracking allocator is a host-boundary witness, not a virtual
address reservation or a production memory-budget qualification.

Run the genuine full upstream target first at each of -O0, -O1 and -O2, then
the reference and Zig-linked adapters, preserving XML and complete process
output in a fresh output directory:

  python luau-zig/tests/upstream_suite.py \
    --reference-binary build/upstream-luau/Luau.Conformance \
    --reference-script-host build/luau-conformance-ref/upstream_script_reference \
    --candidate-script-host luau-zig/zig-out/bin/upstream_script_candidate \
    --reference-api-host build/luau-conformance-ref/upstream_api_reference \
    --candidate-api-host luau-zig/zig-out/bin/upstream_api_candidate \
    --output-dir build/upstream-results

The driver sets LUAU_TEST_FIXTURES_DIR to the verified preserved fixture
directory for both API hosts. If invoking an API host directly, set that
variable to the absolute path of luau-zig/tests/upstream-0.744/tests/conformance.
API cases are run as isolated doctest processes so a crash or timeout cannot
hide later cases. Each process has its own XML and process JSON. Filtered-out
XML entries are retained in the raw evidence but excluded from executed results;
a selected case missing from the executed set is an error. A test that uses
no compiler is reported once with optimization null; compiler-using API
cases are attributed only to the compiler optimization actually executed
(currently O1 except the upstream NDebugGetUpValue test, which forces O0).
Other optimization variants are explicitly skipped, not repeated passes.

Alternatively supply --reference-xml-o0/--reference-xml-o1/--reference-xml-o2
from genuine doctest XML reference runs. The supplied XML must be from the same
pinned source, toolchain/platform and optimization variants; the driver cannot
independently attest the provenance of an externally supplied binary or XML.
Missing executables, non-executed upstream cases, unadapted callbacks, scoped
flags, generated/codegen/JIT/typechecker tests, and native-only variants are
reported explicitly; exit 77 means one or more unsupported/skipped checks.
Failures exit 1 and all-complete success exits 0. Existing output directories
must be empty and are never silently overwritten.
The optional `zig build luau-zig-upstream-evidence` step builds and compares
against a clean, externally staged pinned upstream checkout. Set
P4PC_LUAU_REFERENCE_DIR to its read-only mount; the lane default is
/lane/build/luau-upstream-32c28089. It requires 12 GiB free for the build and is
not part of the hermetic gating test suite. Its retained output includes
results.json, per-process evidence and fixture-results.md; body results are
separate from original case coverage.
It also executes the same host-driven weak-table/heap witness against both the
vendored C++ reference and the Zig bridge, reporting each exit code separately.
For GCDump, the driver reports that witness as the semantic replacement for
private heap-enumerator assertions; absent or skipped witness results never pass.
The fixture-parity records additionally attempt preserved non-native .luau
files at O0/O1/O2 using the first associated upstream case's host setup, or the file
stem when no literal call is known. A failing reference host identifies a
fixture needing upstream-specific setup; it is not attributed to the Zig VM.
These exploratory body results do not replace the original case assertions.
For types.luau, the optional evidence build generates the real upstream
frontend's builtin RTTI table from the pinned checkout, then supplies the same
table to both script hosts; no upstream Analysis code is linked to the Zig VM.
Synthetic native-codegen and jit-inliner variant records are counted as N/A,
separately from passes: tools/luau/CMakeLists.txt excludes CodeGen and Inliner
from the shipped runtime, and src/platform/mods/luau_c.cpp initializes the
interpreter. These variant records are not a count of upstream native cases.
The upstream Integers, Native and NativeTypeAnnotations cases require native
execution for integers_regspill.luau, native.luau and native_types.luau; the
JitInliner case forcibly enables the JIT. Those fixtures are N/A. The
native-only CodegenSupported, Native, JitInliner, NativeTypeAnnotations,
NativeAttribute, IrInstructionLimit and three CodegenRandomize/NopPadding
cases are also N/A. IrLowering tests only CodeGen IR/assembly output, and
SharedCodeAllocator tests executable native-code allocation, so those suites
are N/A; FeedbackVector remains separate because it also checks VM/compiler
behavior. Cases with interpreter behavior as well as native branches remain
accounted separately rather than being classified wholesale as N/A.


Each source script-body comparison checks the real upstream fixture's returned
\"OK\" under the upstream host's globals, sandbox, compiler options and applicable
adapted test callbacks. Each successful producer also writes actual serialized
bytecode; the harness separately loads and executes upstream bytecode on the
Zig VM and Zig bytecode on the upstream VM under the same fixture host setup.
This crosses the main fixture chunk; fixture calls to loadstring still compile
on the consuming host. Both outputs and each cross-run process are retained.
These test-only admission paths do not expose external bytecode through the
production boundary and do not assert serialized byte identity.

A script-body or bytecode-cross PASS alone is deliberately partial. For cases
whose entire upstream body is a plain runConformance call, the optional graph
witness parses each function emitted by both compilers with the pinned upstream
graph parser and requires the same nonempty reserialization as upstream. Their
candidate-case can pass only when the body, both graph checks and both sandbox
GC semantic witnesses pass; native lowering is N/A because it is not shipped.
Other cases and unadapted subcases remain SKIP, never a full passing claim.
Coverage forces O1 upstream, so it is recorded once at that actual optimization.

BuffersWithCage uses the upstream 0.744 BufferCage and scoped LuauBufferCage
flag on both script hosts, including the upstream lastAllocationType check.
It compares buffers.luau and crossed bytecode at O0/O1/O2 only when the pinned
reference case was observed. Unsupported platforms or unavailable cage
reservation explicitly skip the script run. Candidate-case graph/GC and native
validation remain skipped; this adapter does not add a VM implementation or
establish full conformance. The cage reserves a large virtual address range.

Integers additionally runs integers.luau with LuauIntegerType2 and
LuauIntegerLibrary explicitly enabled in both implementations, as a separate
feature-on interpreter experiment. The default upstream target may leave these
flags disabled and execute none of that fixture. Its scoped
LuauCodegenBufferInteger flag applies to native compilation, which is not
present in these hosts. integers_regspill.luau is N/A: upstream guards it on
codegen and the fixture itself requires is_native().

The generated API host copies selected actual upstream case bodies and executes
their assertions against the real Zig C ABI; DirectFieldAccess uses the original
test bodies with its compiler call bridged to Zig. SameHash retains its original
assertions through test-only bindings to each runtime's actual VM and compiler
hash implementations, including the unaligned-input check; no production C
export is added. The allocation-failure loader case exercises failed loads and
post-failure collection; HugeConstantTable
and LargeNestedClosure retain their interpreter assertions while excluding
inactive native branches. HugeFunction is partial because its graph validation
is omitted. Interpreter assertions that call the compiler with upstream's fixed
default options are recorded once, not as fictitious O0/O2 runs; separate graph
checks at those outer variants remain explicitly unadapted. Two original
vector-metatable sandbox tests run with their scoped library flag. ApiCalls,
Interrupt, Debugger, InterruptInspection, NDebugGetUpValue and TagMethodError
run their upstream post-execution/host callback assertions but remain partial
because bytecode-graph and GC-internal validation is omitted. UserdataAlignment
also remains partial: its original alignment conditions are promoted from
non-doctest assertions to observable checks for both runtimes.
ExportEdgeCase runs its original interpreter load and resume assertions with
the export-syntax flag on both compilers; upstream graph validation is omitted,
so this case remains partial.
LargeModuleA64 runs its original O2 interpreter load and result checks; its
A64 native-lowering checks are N/A because native codegen is not shipped.
UserdataDirectAccess runs both original DirectAccess and ValidateMetatable
subcases with their upstream atom, cached-slot, index, newindex and namecall
callbacks and the upstream userdata helpers. It remains partial because the
private luaC_validate and bytecode-graph validation are omitted; individual
subcase results are not extracted from the aggregate API result.
ApiCalls, Debugger, InterruptInspection, TagMethodError and
UserdataDirectAccess also run their adapted host/fixture callbacks at each
compiler optimization level rather than treating O1 as all variants.
NativeUserdata's NoIrHooks interpreter subcases use the upstream userdata
helpers and compiler userdata type list at O0/O1/O2. Native IR hook subcases
are N/A; bytecode-graph and GC-internal validation remains separate.
The six ExceptionObject fixture functions also run under protected calls against
both VMs, with error values checked. The original uncaught C++ exception test
is N/A, not a pass: tools/luau/CMakeLists.txt:32-60 applies LUA_USE_LONGJMP=1
to every provider profile (including Android, macOS and Windows through
cmake/host-cxx.cmake:60-83), while luau-zig/src/vm/jmp.zig:1-12 describes the Zig VM's
longjmp-style unwinding.

The Classes script runs with the upstream class, runtime, call-feedback and
bytecode-cost flags. The API adapter also runs the upstream repeated class
inheritance/member-offset case with those flags and the Zig compiler. Flag
availability is not evidence of correctness: only actual execution determines
whether either case passes.

All other cases and each named subcase, script call and native/JIT variant remain
present in results.json with a specific unsupported prerequisite. This is not
yet full candidate coverage of the suite and is not a runtime migration or
sandbox qualification.

Project mod VM witnesses against the Zig bridge
-----------------------------------------------
The registered candidate contract lane also runs the hermetic upstream-inventory
and result-accounting regressions against the vendored subset.
The unchanged tests/mod_luau_test.c and tests/mod_luau_save_test.c build with
the production bindings, cooperative kernel and fixture codegen of
tools/mods/CMakeLists.txt's 64-bit (P4_GUEST64) profiles, linked to the Zig
p4pc-luau archive instead of p4_luau (no C++ Luau). The witnesses compile the
same sources as those CMake targets, including every native stage (damage,
recovery, cost, chance, wear-off, buff, reward, unit, hit), the real p4.passive
binding with its grant table and semantic catalog reader (lua_passive.c,
game_passive_grant.c, game_data.c) and the project's own passive game facts
fixture (tests/mod_passive_facts.c). Python generates the hook fixture,
mod_luau_api.h and package_registry.c's p4_capabilities.inc (from every
include/p4pc/p4.d fragment, by cmake/sources/052-capabilities.cmake's rule);
32-bit targets are refused (the "" profile needs the fixed PS2 RAM image):

  (cd luau-zig && zig build mod-luau-tests -Doptimize=ReleaseSafe)
  # If python3 is not on PATH, add -Dpython=/absolute/path/to/python.

Run them as CTest does, with P4PC_MODS=0 P4PC_PACKS=0 P4PC_INPUT=script
P4PC_VIRTUAL_CLOCK=1 SDL_AUDIODRIVER=dummy and a scratch data directory:

  luau-zig/zig-out/bin/mod_luau_test build/mod-luau-zig/fixture-data
  luau-zig/zig-out/bin/mod_luau_save_test build/mod-luau-zig/save-fixture-data

On Windows the same step builds native x86_64-windows executables
(os_win32.c replaces os_posix.c, as in the 64w profile); run
luau-zig\zig-out\bin\mod_luau_test.exe and mod_luau_save_test.exe with the
same environment and arguments.

FASTCALL differential evidence
------------------------------
The separate upstream evidence lane also builds fastcall_reference and
fastcall-candidate and runs fastcall_differential.py. Each case executes the
same chunk in fresh safe and unsafe environments on both VMs, checks its
compiled graph for the requested builtin ID, compares results, errors, named
side effects and interrupt counts, and uses the interrupt delta to distinguish
the fast and fallback paths. All builtin IDs require both paths; missing cases
are failures, not skips. The math and bit32 IDs additionally run 10,000
deterministic random samples at a single call site and require bit-identical
fast and fallback results within the candidate. Specified exact operations are
bit-exact against the reference. Finite transcendental results must be within
one ULP of a 256-bit MPFR oracle; glibc differences and closer-reference counts
are separately classified, not treated as failures.
Integer witnesses are also compiled with both integer flags at their shipped
defaults, where the integer library is unavailable and the graph must not
contain the corresponding integer FASTCALL. Assertion fallback compares the
error and traceback; the select count witness runs in a real vararg frame.
The same evidence lane compares a separate 10,009-pattern direct numeric trace
per ID, including subnormal, NaN and infinity patterns. It checks the actual
library call against its fast path bit-for-bit, then compares exact IDs against
the reference and transcendental results against MPFR. NaN payload differences
are counted explicitly. Cross-target traces compare x64 with a64 and Android
using a user-mode aarch64 emulator; raw arithmetic NaNs are measured separately.
The evidence container needs a read-only MPFR shared library and aarch64 emulator.
The external pinned checkout and these diagnostics are not dependencies of the
hermetic land-gated CTest set.

For a single ID (or use --builtin all), with actual absolute executable paths:

  python luau-zig/tests/fastcall_differential.py --builtin 24 \
    --reference /path/to/fastcall_reference \
    --candidate /path/to/fastcall-candidate \
    --graph /path/to/upstream_graph_reference \
    --output-dir /path/to/empty-results

The report includes commands, raw process output, bytecode, compiled graph
checks, per-case failures and interrupt counts. It does not link the reference
VM or compiler into the candidate or change the reusable module's no-libc
contract.

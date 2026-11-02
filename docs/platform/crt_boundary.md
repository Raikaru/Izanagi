# Game CRT boundary

[Platform index](../platform.md) · [Numeric determinism](numeric_determinism.md) · [Port compilation](../porting.md)

Game logic must not acquire the host CRT's permitted ordering, integer widths,
rounding, locale or hidden state. The boundary covers generated decomp units,
spliced overrides, native game-facing sources including `src/cri`, and generated
executable address/alias tables. `src/platform` is the host implementation layer,
not permission to forward a game operation to an arbitrary host implementation.

## Binding inventory

`include/p4pc/libc_rename.h` gives generated callers their canonical bindings.
Native callers use the same entries explicitly. The authoritative per-build
caller table is `gen/crt-<profile>.tsv` (and the corresponding JSON), plus
`gen/crt-entry-<executable>.tsv` for bootstrap/viewer/generated-table objects.
Every row records the requested CRT operation, actual binding, disposition,
object, source, calling function and review reason. Address-taken relocations
are included, not just direct calls. These reports describe compiled references,
not a source-text guess about which preprocessor branches might run.
The guarded final links also emit `gen/crt-link-<target>.map`, retaining the
definition/import origin used by the actual linker. Android's combined `main`
library is audited before both its 32-bit and 64-bit early-return link paths,
including loader, viewer and generated-table objects.

The checked-in [compiled caller inventory](crt_callers.tsv) merges every
source/caller/binding from the qualified Linux i386/x64, Win32/Win64, AArch64 and
Android-arm64 objects and entry tables. Rows retain profile membership
and point to the strict symbol reason or exact source adapter review. It is a
snapshot of actual generated references, not a second permissive policy; each
build regenerates the full object-attributed JSON/TSV and final link map.

| Actual profile closure | Game/native objects / caller rows | Entry-object caller rows |
| --- | --- | --- |
| Linux i386 | 1,257 / 2,965 | game 116, viewer 146, movie inspector 28 |
| Linux x64 | 1,257 / 2,879 | game 126, viewer 160 |
| Win32 (O2, target-prefixed MinGW driver) | 1,257 / 2,974 | game 117, viewer 147, movie inspector 30 |
| Win64 (O0 mirror, retail float launcher) | 1,257 / 2,948 | game 127, viewer 161 |
| AArch64 | 1,257 / 2,869 | game 126 |
| Android arm64 | 1,257 / 2,963 | combined SDL/loader/viewer library 425 |

All listed object guards passed, and the actual executables/Android shared
library linked with the final-entry guard enabled. Full reports and link maps
are retained externally in `<scratch>/studies/PortCRT/final-compiled-inventory/`,
whose `manifest.json` records the exact source archive, overlays and report/map hashes.
The added Win32 production reports/maps and causal-fix input hashes are retained
separately in `<scratch>/studies/PortCRT/win32-f370c1ab-fix/`.
These are compile/link qualifications, not a claim of AArch64/Android gameplay
runtime coverage.

| Operation / retail role | Boundary | Implementation / reason |
| --- | --- | --- |
| qsort | `00440bb8` | Retail partition, median selection, equal-key swaps and insertion order; never host qsort. The keyed RW sort `004156e0` calls this same core. |
| strcmp / strncmp / memcmp | `004426e8` / `00442c30` / `0043f778` | Exact first differing **unsigned-byte difference**, including high bytes, zero counts and terminators; not merely a host-permitted sign. |
| atoi | `0043c6b0` | Signed-64 base-10 parse followed by low-word truncation, not a host-long conversion. |
| Signed-64 conversion | `00444210` → `00443fd0` | Retail signed accumulation, saturation, prefix/whitespace/end and guest errno rules. |
| Unsigned-64 conversion | `00444458` → `00444240` | Retail unsigned accumulation and saturation. **00444458 is not signed strtol.** |
| sprintf / printf | `00442088` / `00440b68` | Shared retail formatter; printf uses host stdout only as a byte-output device. |
| vsprintf / vprintf / scePrintf | `00446ed8` / `00446e88` / `004244c8` | Same engine and retail conversion/count rules; native va_list forwards retain their native register class. |
| vfprintf core / dtoa | `00444870` / `0043ca70` | Retail formatting decisions and Gay bigint conversion, with shared guest caches and power tables; no host sprintf, snprintf or dtoa arithmetic. |
| sscanf / reentrant sscanf / scanner | `00442100` / `00442188` / `00446108` | Retail scanning, destination sizes, counts, suppression, EOF and errno. Native frontends accept native destination pointers; the scanner core reads eight-byte guest argument slots containing four-byte guest destination addresses. |
| Decimal input | `00443068` | Retail raw software-double approximation and bigint/ulp correction; no host strtod. |
| strtok / tokenizer | `00443f18` / `00443f38` | Retail byte loop and four-byte continuation word in the active guest reent at `+0x5c`; no host TLS or native static save pointer. |
| EE SDK fstat / kill lookalikes | `00421f98` / `00421fb8` | Existing matched game bodies, canonically renamed to avoid host-symbol interposition. fstat ignores its first argument, stores a zero 64-bit field at buffer+0x48 and mode `0x2000` at +4; it is not host struct stat. kill returns zero and forwards only command 1 to the existing EE cleanup/exit path; it is not a host PID/signal operation. |
| memcpy / memmove / memset boundaries | `0043f810` / `0043f8c0` / `0043f9c8` | EE `uint32_t` byte counts, widened only inside the reviewed host byte-operation implementation. Valid spans; memcpy requires nonoverlap, while memmove preserves either overlap direction. |
| strncat / strncpy / strlen / strstr boundaries | `00442a80` / `00442de8` / `00442948` / `00443010` | EE-width bounded append/copy, including strncpy's zero padding, EE-width string length, and first exact substring including the empty needle. No locale, ordering choice or host-long result. |
| malloc / free / calloc / realloc | `0043ece8` / `0043ed08` / `0043c710` / `00441558` | Native-pointer/EE-width entry points over literal retail newlib in `ee_heap.c`: original guest-word chunk/bin layout, alignment, split/coalesce/reuse and break/trim behavior. Calloc preserves the low 32-bit product and usable-chunk clearing; realloc preserves null/minimum-zero-size policy. Host scratch remains host-allocated. |
| Single libm | `src/platform/ee_libm.c` | Asm-translated retail float kernels, including sin/cos/tan/atan/fabs/sqrt. RW's nominal double trigonometric macros use the original float kernels. |
| Double acos / log / log10 / pow / sqrt | `004470f0` / `004476b0` / `00447b88` / `00447d00` / `00448a00` | Literal R5900 soft-double cores in `ee_dlibm.c`. `ee_dfloat.c` provides bit-copy-only native floating-register adapters. |
| Double arithmetic and integer conversions | `src/platform/ps2_fpbit.c` | Retail pack/unpack, rounding, FTZ, NaN, overflow and signed/unsigned conversion behavior; no native double arithmetic/casts as a substitute. |

The strict host allowlist is [`config/host_symbols_allow.txt`](../../config/host_symbols_allow.txt).
Its reasons cover byte semantics and the actual bounded valid-buffer domain:
guest strings/counts fit the 32-bit guest address space, so LP64 versus LLP64
cannot change lengths or append counts. Pointer results refer to the original
input/destination and are converted back to guest words at guest-storage seams.
Compiler-runtime entries are separately explained there; they are not permission
for host parsers, sorting, locale, clocks or math.

There is no speculative permission for unused bsearch, rand/srand, time/clock,
other strto*/ato* or formatter variants. A new such compiled reference fails and
needs a real retail binding or an input-domain equivalence review.

### Fixed-width byte and string callbacks

A host operation's byte semantics do not make its native `size_t` prototype an
EE callback ABI. The RW string vectors call `strncmp`, `strncat` and `strncpy`
through four-byte EE count parameters. Their canonical entries, the memory
copy/move/set/compare entries, and `strlen` now expose explicit `uint32_t` counts
or length results on every host. `libc_rename.h` names those same boundaries;
there is no separate address-taken host `strncpy` path. The reviewed host call
widens the count internally, after the callback ABI has been satisfied.
`ps2_libc_test.c` checks the actual aliased function-pointer types, both memmove
overlap directions, zero counts, bounded append, strncpy padding, canaries,
high-byte comparison and length, and an early mismatch with `UINT32_MAX`.

Those unchanged byte/string bodies live in `src/platform/ee_bytes.c`, separate
from formatter, parser and guest-reent state. Both the full port and isolated
qsort, animation and task fixtures link this same implementation; the fixtures
do not copy the bindings, undefine the game's aliases or rely on unrelated
formatter/heap objects being discarded by section GC.

The strict allowlist still governs compiler-emitted byte helpers, including
aggregate memcpy/memset: an EE-width source alias does not make an explicitly
reviewed identical host byte operation forbidden. This does not permit a
native-size_t function to masquerade as an EE callback; its actual indirect
prototype remains subject to the width/class check.

The actual CMake `ps2_libc_test64` and `ps2_libc_test64w` binaries passed these
cases on Linux and native Windows/UCRT. Their four overlaid source hashes,
actual build/run commands, binary hashes and rc=0 logs are retained in
`<scratch>/studies/PortCRT/byte-boundary-native-windows.json` and the
corresponding `.log`; the Linux build receipt is
`byte-boundary-results-linux.json`. This is the byte-binding regression proof,
not a substitute for full-profile links or gameplay qualification.

### Varargs and long widths

The EE uses one 64-bit GPR bit pattern for software double. A native double uses
a floating register. `tools/port64/gptr_rewrite.py` sends both original formatter
names and canonical `func_00442088` / `func_00440b68` aliases through the existing
`p4pf_*` mask adapters, preserving the leading GP argument mask. Treating a raw
alias as an ordinary native varargs call loses `%s` guest addresses and soft-double
bits. Native va_list forwarding instead bit-copies `fptodp` output to the native
double register class; it does not numerically cast the integer bit pattern.

The SFXZ caller retains its original `%lx` and `%lx %lx %lx` formats. Its four
scan locals are **eight-byte** retail slots on Linux and Windows. Only their
original uses narrow: result words are low 32 bits, and frame offset is
`(Sint32)((Uint32)n * (Uint32)src)`, matching the low-word multiply/sign extension
at `0052c244/248`. The actual mirrored caller fixture includes wide operands,
unsigned negative input, saturation and a wrapped/signed frame offset.

The decomp's `__sqrt(value, 0.0f)` in `effMiscQuatAngle` is a MWCC **sqrt.s
intrinsic**, not double libm. `004bd508` and `004bd54c` execute sqrt.s without a
call. The mirror uses `p4pc_ee_sqrt(float)` and no longer creates a host `__sqrt`
thunk. Raw soft-double declarations and native double-register declarations are
kept in separate headers so LP64 `uint64_t` cannot collide with decomp `u64`.

## Fail-closed build guard

CMake's `p4crt` target runs `tools/crt_guard.py` over the actual game object lists
with nm/llvm-nm and relocation disassembly. The i386, mirror64/mirror64w and native
64-bit profiles use the same policy; Android and AArch64 select their actual
compiler/object formats rather than a separate permissive implementation.
Executable PRE_LINK checks also include generated tables and native entry objects.
The tables generation/link dependencies require the relevant profile guard.
Generation-only census runs have no objects and are not an object/link audit.

The guard rejects an unknown external, a host CRT rename bypass, an unavailable
retail implementation at final-profile closure, a generated trap masquerading
as such an implementation, or an unreviewed host-adapter symbol/caller pair.
The CRT address interval starts at `0043c6a0`; preceding `0043c0xx` sound-client
wrappers are not mislabeled as libc. A retail-looking metadata name alone never authorizes a host
call. Compiler-generated imports/version decoration and 32-bit COFF decoration
are normalized before comparison.

Named compiler-runtime aliases such as `fptodp` remain in the caller inventory
before their address-table thunks are emitted. Final closure requires their
actual core in the game/port objects: generated table bodies, thunks and traps
are not implementation evidence. Compile-only mirrors obtain native names from
CMake's exact current per-source definition-ABI manifest, not merged host-header
declarations or stale deleted-source files. Final/native audits obtain them from
their supplied compiled definition objects instead: a cold i386-only profile
does not require optional 64-bit ABI sidecars.

Host-only source contracts are [`config/crt_host_adapters.json`](../../config/crt_host_adapters.json).
They enumerate **(host symbol, calling function)** pairs from each source's
compiled object, with a reason. A new pair needs review; unrelated source or
header edits do not expire every approval. There are no source/include hashes,
wildcard host-family exemptions or per-platform warn-only switches.
Android arm64 can inline the image loader's fatal `fail` diagnostic into
`fail_read`, so that exact caller is reviewed for its errno, stderr, formatted
output, strerror and exit operations; they only report host I/O failure.
The direct `read_at` errno access is reviewed for interrupted-read retries.
The AArch64 viewer's `frame_camera` zeroes a 28-byte `Bounds` stack object
and emits three four-byte `memcpy` calls: its inlined `f32_at` copies the
camera view-window fields at offsets `0x68` and `0x6c` into native stack floats.

Pattern auto-initialization is not a blanket exception. The retained i386
objects exposed seven additional `memset`/caller pairs (ten call sites), all
with fill byte `0xff` and EBP-relative native stack destinations:

| Adapter caller | Proven native destination | Subsequent use |
| --- | --- | --- |
| `ply_open_stream` | `alt[300]` | bounded native path formatting |
| `p4gl_context_present` | inlined capture paths of 512, 512 and 600 bytes | bounded formatting, host file/PNG output |
| `p4gl_gl_compile_shader` | `GLchar log[1024]` | bounded `GetShaderInfoLog` |
| `func_003fd940` | `palette[256*4]` | native texture decode |
| `p4gl_drawlog` | `line[1024]` | bounded formatting and native draw logging |
| `p4tmx_decode` | `pal[256*4]` | indexed native palette reads/writes |
| loader `main` | `elf[1100]`, two mutually exclusive argument branches | bounded native executable-path preparation and image loading |

Their exact pairs and buffer-domain reasons are reviewed in the same policy,
not accepted through a profile switch. Disassembly, DWARF attribution, the ten
relocations and seven object hashes are retained in
`<scratch>/studies/PortCRT/pattern-memset-proof.json`. New pairs still fail;
the targeted guest-write check remains enabled.

The adapter check also runs targeted Clang-AST result-flow checks with the actual
compile command. Host allocations/results may not cross P4_H2G/function-pointer
registration or guest-memory stores/copies. It follows project/inline helpers,
local arrays and native caches, and rejects host scanner writes/end pointers into
guest destinations. This is a focused boundary check, not a claim to solve arbitrary
C alias analysis; the pair review remains responsible for each adapter's semantics.

The original-texture dump is a native platform service: its host-owned queue,
thread, filesystem streams and provenance metadata never become guest addresses.
Resume verification retains `ferror`, so equal short reads cannot hide I/O
failure. On Windows SDL's public `SDL_CreateThread` macro deliberately passes
the CRT's `_beginthreadex` and `_endthreadex` as `SDL_FunctionPointer` to
`SDL_CreateThreadRuntime`; the worker itself has the `SDL_ThreadFunction`
signature. Raster pixel writes remain in the guarded renderer and use the
canonical EE byte operations.

Bionic's fortified byte/string aliases are allowed only for the same reviewed
valid-span contracts: CRI's RPC wrapper bounds both transfers by its actual
buffer, and the error formatter's fixed literals fit its 64-byte destination.
The result-flow checker classifies fortified/builtin byte copies and formatted
output just like their ordinary spellings; they do not bypass host-pointer or
parser-result checks. Macro-renamed Android entry functions retain source
ownership rather than inheriting the preceding included header's filename.

Viewer numeric conversions that set guest field/date/player/camera data or guest
frame counts use retail parsers, even when their text came from argv/getenv.
Authored input is not a parser exemption. Host-only draw-window/MAD controls,
capture paths, diagnostics, native scratch and operating-system/device I/O remain
host operations. Viewer camera radius uses the retail float square root.

The renderer's resolution-value callback uses `p4pc_ee_snprintf`, a bounded
native-register frontend to that same retail formatter, rather than host
snprintf; its output can be consumed by native Config. A zero/one-byte capacity
and truncated output retain the shared sink's count/NUL rules.

The caller ABI is part of the formatter seam. Both mirror rewriters normalize
every masked integer argument to an **unsigned long long** EE GP word before
the call, preserving the promoted expression's sign or zero extension. Width,
precision and `%c` operands follow the same rule; native pointers and native
double-register arguments retain their own mask classes. The i386 compiler
launcher runs the narrow printf-AST phase even with `P4_EE_FLOAT=OFF`, before
any optional arithmetic rewrite. It preserves the original language/frontend
flags and source/header depfile rather than depending on deleted `.i`/`.ll`
files. The 64-bit fixture uses the production full rewriter, not a cloned
narrow launcher.

This is not a generic indirect-varargs bridge. The shipped mirror's RW string
vectors are real installations, but all seven selected member/macro consumer
units have zero active consumers in both actual i386/x64 preprocessing
(`RWDEBUG` is absent). Retail's six numeric sprintf-slot calls are separately
accounted for: five `%s`/literal calls belong to the bypassed PS2 VU layout
routine `003d2c40`, and the heap-growth `%d/%d` call formats an unread stack
buffer omitted by the native `00414a30` body. They are not absent from retail.
Controller draw and CRI error callbacks supply fixed pointer/text contracts.
No active width-sensitive or scalar-encoded-pointer indirect formatter consumer
was found in this generated corpus; future enabled consumers require actual
adaptation, not an assumption that the existing register-class bridge carries
integer widths or a GP mask.

`tests/crt_guard_test.py` compiles actual objects for Linux i386/x86-64, Win32/Win64,
Linux AArch64 and Android arm64/x86-64. Its deliberate puts call, renamed strcmp
bypass, metadata-only time permission and unknown host entry must fail. An approved
malloc/caller pair still fails when its result is stored into guest memory,
including through an inline helper/native cache. Harmless source/header edits pass;
a new calling function does not inherit an old pair's permission. Macro-renamed
adapter bodies are checked too. Actual native definition objects are accepted
without optional mirror sidecars; compile-only audits still reject missing
sidecars rather than silently discarding their required ownership metadata.

The bounded seven-test suite passed with the actual Linux compiler in 7.653 s
and actual native Windows Clang in 96.112 s. Both compile the seven target
ABIs above, including bad callback casts/stores/erased arguments, a valid
runtime-loaded driver pointer, and real inherited-target/width overrides in
both directions. The checker honors the compiler's last target and `-m32`/`-m64`
flags, rather than mistaking inherited i386 flags for the effective Win64 ABI.
Native command/source hashes and the rc=0 log are retained in
`<scratch>/studies/PortCRT/final-native-guard-precedence/`.

The production Win32 driver selects its default target through its executable
name, `/opt/llvm-mingw/bin/i686-w64-mingw32-clang`, without `--target` or `-m32`.
Its actual target is `i686-w64-windows-gnu`. The first Win32 package qualification
after the cutover exposed a checker error: the builtin callback spelling
`__size_t` was interpreted using the 64-bit Linux checker's pointer width even
though the selected compiler's real `memcpy` parameter desugared to 32-bit
`unsigned int`. The declarations and callbacks were already ABI-compatible;
changing them or suppressing the diagnostic would hide the cause.

The checker now derives the default target from Clang's target-prefixed driver
before applying the last explicit target and independent `-m32`/`-m64` overrides.
An eighth real-object regression uses both i686 and x86-64 target-prefixed
drivers without target flags, then overrides them in both directions. Valid
pointer-word callbacks and byte copies pass; opposite-width callbacks still
fail. No AST mock, callback exception or new global CRT allowance is used.
The expanded eight-test suite passed on actual Linux Clang 19 in 28.948 s and
actual native Windows Clang in 87.095 s (not Wine); both retain the bad-callback
and forbidden-host-call rejection witnesses. The fresh native receipt and
source hashes are in
`<scratch>/studies/PortCRT/win32-f370c1ab-fix/native-windows/`.

The Win32 objects also exposed seven missing native-adapter caller pairs, reviewed
individually in the same policy: standalone movie-viewer constructor/diagnostic
access and external frame-count parsing; the image loader's fatal diagnostic
stream and inlined fixed diagnostic write; the renderer's equality-only
resolution-string comparison; and the inlined standalone model-viewer file open.
None authorizes a guest numeric parser, host comparison magnitude, or a host
pointer escaping into guest state. Linux i386
scene coverage is not a production Win32 build: the landing cohort must build
all three O2 Win32 production executables with their unconditional strict guards.

The full Wine suite additionally exercises the selected LLVM-Mingw wrapper:
`i686-w64-mingw32-clang` links to `clang-target-wrapper.sh`, which obtains the
target from its invocation name and resolves its own directory separately.
The fixture must preserve that lexical driver path. Resolving the symlink first
invented a `...-clang.sh` fixture name and lost the target-selection contract.
Keeping the original driver name covers the real wrapper without guessing a
shell script's ABI, changing production callback declarations, or skipping
either the valid- or invalid-width cases.

The two i386 image-loading CRT oracles (`ee_decimal_oracle_test` and
`ee_printf_abi_oracle_test`) require `p4_ps2_exe`, just like production and the
other fixed-address fixtures. Without it their PE preferred image base was
`0x00400000`, their own `.text` began at `0x00401000`, and no `.ps2ram` section
reserved the requested `0x00100000..0x02000000` before native startup. A runtime
fixed allocation can therefore collide with the executable or startup state;
this is a missing executable reservation, not permission to move guest memory.
The corrected PE image base is `0x000f0000`, `.ps2ram` owns exactly
`0x00100000..0x02000000`, and native `.text` begins at `0x02000000`.
The existing OS mapping seam verifies image ownership and makes those pages
executable; its exact-address `VirtualAlloc` fallback and failure checks remain
unchanged. Returning arbitrary host memory would invalidate i386's direct guest
pointers and is not a valid fallback.

Both fixed PE oracles and all eight guard regressions passed through the actual
Wine profile's selected wrapper (three CTest entries, no skips). PE layouts,
binary hashes and native qualification receipts are retained separately in
`<scratch>/studies/PortCRT/win32-wine-58335947/`. The mandatory Win32 shard
must run the existing non-GL Wine suite as well as build it; build-only coverage
cannot detect these runtime reservation or Python child-launch failures.
The combined CRT/PE and configured-emulator fixes passed the entire existing
Wine selection (`ctest -LE gl`): 126 selected tests, zero failures and five
existing skips, in 30.35 s. This includes `mod_files`, `vfs_origin`, both
fixed-address oracles and the actual-wrapper `crt_guard`, not only a focused
rerun. Full logs and generated CTest registrations are in the study's
`full-wine/` subdirectory. The GL-labelled game gates retain their separate
native Windows qualification; they are not silently claimed as Wine coverage.

This reservation failure is not Wine-only. Both original Win32 executables
also exited 2 with the fixed-address `VirtualAlloc` error as native 32-bit
Windows processes on the x64/WOW64 host. Both corrected executables exited 0:
dtoa matched 22,960 cases, integer conversion matched 2,552, and the rewritten
formatter ABI matched 5,888 with 32-bit native pointers. The original and
corrected native receipts/logs are retained as `native-unfixed/` and
`native-fixed/`, alongside hashes of both binary sets. These are real Win32
process observations, not a claim that a separate 32-bit Windows OS VM was run.

Failure-retention integration tests invoke the real CI entry point with
`P4PC_SOURCE_DIR` selecting a private CMake project (the production default
remains `/lane/src`). They do not match the spelling of a shell scope condition.
Actual CMake/Ninja/CTest processes prove warm nested changes, a concurrent edit
that causes Ninja regeneration to fail, and byte-exact retained logs after build
cleanup. Scope witnesses verify `win` builds without Wine, `win-tests` runs the
complete existing Wine selection, and Windows/package configure errors retain
their real output. All 31 `remote_check_host` behavioral tests passed in 45.779 s.

The real Linux CMake `p4crt_mirror64` step was also exercised with an actual
compiled `puts` call deliberately merged into a game object. It failed closed:

```text
CRT guard rejected game references:
  .../units/src/calendar.c.64.o: p4_crt_guard_deliberate_host_call -> puts: host/unknown external absent from reviewed allowlist
```

The original object was restored byte-exact and that same CMake target passed
again: 1,231 game objects and 2,297 caller/binding rows. The bad/restored logs
and failing inventory are retained under
`<host>:<p4pc root>/lanes/PortCRT/crt-probe/guard-witness-*`.

## Indirect callback ABI boundary

An indirect cast can conceal the same host-width bug as a CRT call. The 30 AI
forwarders at `001dc5c0..001dd090` declared their first guest action word as
`u64` and returned `void`, although their dispatch table calls `s32(u8 *,u32)`.
R5900 gives each scalar an argument register; i386 splits that first value
across two stack words. The intended query consequently became the high half
of `formation`, and `flags` read stale caller data. All 30 original wrappers
forward a0/a1 (or substitute DOWN) to `001dbba0` and preserve v0. Their mirrored
native contracts now express that pointer/query/result directly.

Another 23 conditional predicate wrappers discarded the same search result in
their C declarations, although their original epilogues leave the helper's v0
live. Their definitions and fifteen public header declarations now return that
unchanged signed word; the thirteen simple wrappers are already covered by the
existing tail-return transform. The upstream source-family repair
`614dbf3f0419107b6dbbefec2a7f74528cc2e5fb` matched all 320 owner functions.
No side mask, query, search order or integer result is normalized.

The six plugin identity callbacks at `003b7a80`, `003b82b0`, `003bf1c0`,
`003bf1f0`, `003cad80` and `003caee0` are not `u64(u64)` conversions: each
original body is `jr ra` with `move v0,a0` in its delay slot. Their registrations
and actual dispatchers establish pointer-returning copy (four parameters),
destructor (three), or stream-writer (five) contracts. The mirrored definitions
and all 38 selected declaration occurrences use those contracts, retaining
the ignored trailing arguments and the unchanged first-pointer result.

`00100e00` similarly receives the SVM error callback's object/message pointers,
not two 64-bit integers; its original `move a0,a1` forwards the message to
`00440b68`. Both formatter declarations in `runtime_callback.c` use the actual
integer-returning variadic entry prototypes. The existing `00100dd0` sibling
pointer repair remains in place. Neither callback is sanitized into
`printf("%s", message)`: message-as-format behavior remains exactly original.

Two model-effect callback storage views also invented status results:
`mdlEffectDestroyPrimitiveRecord` (`004846d0`, table column `00713490`) and
`mdlEffectSetChildRotations` (`00486330`, column `007134a4`). Their twenty and
thirteen distinct resolved targets respectively return void. GNU disassembly
shows the destructor's `0048473c` call followed by source release without a v0
read, and the rotation's `004863c0` call followed by child traversal at
`004863c8`, likewise without a v0 read. The two destructor declarations and the
rotation local/load cast now declare those actual void results. Payload words,
argument registers, VU operations, iteration and cleanup order are unchanged.

The build combines the actual-object CRT symbol guard with bounded checks over
the reviewed host-adapter sources: indirect callback ABI contracts and the
targeted guest-memory result-write boundaries. The callback check reuses that
source's single AST from its actual profile compile command. Known targets at
casts, stores/initializers, calls and callback arguments must agree on parameter
and return width/class, argument count, variadic status and effective calling
convention. Same-source helper requirements retain erased callback arguments.
Only conventions the selected ABI actually ignores are normalized; runtime-loaded
host-driver pointers remain governed by their reviewed API contracts rather than
invented retail targets. It does not collect or solve
whole-program game/mirror pointer flow. The experimental byte-precise global
solver is not part of this cutover or its build/land path; any separately
proposed nightly study would need explicit hard time and memory bounds.

The first mismatches were watched, not inferred from failing walk end states:
9F read stale skill `0x7f` instead of DOWN `0x100000`; 11F read stale `2`
instead of DOWN; guardian read stale skill `0x95` instead of excluded ID `1`.
The changed candidate filter/target preceded the later RNG and HP divergence.
A private exact-`9ea9c363` i386 relink replacing **only** that AI object passed
all three unchanged registered gates, with no tape/card/limit changes:
`bathhouse_guardian_0526`, `bathhouse_9f_0530`, `bathhouse_11f_0602`.
Its binary SHA-256 is
`ef567983695184b43a6051c261ee56289556969fe97ed94d2f3bec15beda21a6`;
the captured compile/link argv, 1,368 original input hashes, replacement-object
hash and gate/RNG logs are retained under
`<host>:<p4pc root>/lanes/PortStory3/diagnostics/divergence-9ea9c363/selector-30-proof/`.
This isolated causal relink did not rerun the final pre-link guard; full build
qualification remains a separate requirement.

The same review recovered the packet trampoline's consumed result:
`00194c74` calls the installed callback and the epilogue leaves v0 unchanged.
`BtlPacketCallback` and `btlPacketCallbackRun` therefore return `s32`, not void.
The tint packet callback's original epilogues likewise return full-register
zero/one, so its native return is `u32`, not an unspecified upper-byte result.

The action-event slot at `+0x440` likewise stores `s32(void)`: its actual target
`btlTaskSlotLinkedSlot3Callback` (`001fad10`) returns the existing completion
word, and `001ac6b4` calls that slot before `001ac6bc` branches on v0.
Both copied declarations and the mirrored storage cast now agree with the consumer
and actual definition.

CRI's `005038c0` preserves a1 as its error code across the callback jalr; both
library and per-handle error callbacks now receive `(object,code)`.

## Allocator boundary and checked headers

The original RW table at `0044f510` is restored, not redirected to a plain
dlmalloc mspace. Its guest table at `008be290` contains
`0044ec60`, `0044ee70`, `0044f140`, `0044f3a0`. Early platform initialization
copies those same words to `008873e8..008873f4`; the matched retail initializer
also retains its pool-owner callback at `pool+0x30`. There is no second checked
allocator implementation: the matched `Kernel/sdkChkmem.c` bodies are used.

Retail `chkmemAllocate` obtains `(size + 0x37) & ~15` raw bytes from EE newlib,
writes its `0x18` tracking header, returns raw+32 aligned to sixteen bytes, and
stores the raw backpointer at payload-4. Checked free unlinks raw+0x0c, clears
the class word and immediately calls `0043ed08` → `0043e170`. The underlying
newlib uses **four-byte** chunk/bin words on every host, immediately coalescing
and reusing blocks. `ee_heap.c` retains the original instructions/branches;
the mapped-RAM sbrk/EndOfHeap and existing IRQ primitives are OS boundaries.
There is no host malloc, pointer-sized metadata, usable-size query, padding workaround or
quarantine in the shipping allocation path.

The realloc copies at `0044181c`, `00441968` and `00441a7c` call EE memcpy
`0043f810`. Predecessor merges can overlap with destination below source;
the original memcpy's forward `lq/sq`, `ld/sd` and byte loops preserve the
source bytes in that direction. `heap_copy` therefore uses the allowlisted
host **memmove**, not undefined overlapping host memcpy. The other copy
case obtains a distinct live allocation. The fixture includes a smaller
free predecessor and a live successor, forcing this overlapping merge.

The causal counterexample is `btlUnitReleaseModel` (`0019d3c0`): the original
code destroys/frees the model then writes its colour at +208 decimal (`0xd0`)
and layer-zero weight at `0xf4` / `0x244` through the original model setters.
With plain host-width dlmalloc that write can hit live allocator tree metadata;
Story's watched Marukyu transition subsequently crashed during TMX decode/free.
The lifetime is retained, not reordered: restoring the original checked/raw
layout and reuse rules is the compatibility seam. This supersedes the earlier
negative-offset-only audit's insufficient conclusion that no seam was needed.

The castle route exposes another observable heap remnant. A hardware watch in
the pre-cutover binary identified native `mspace_free` as the writer of three
eight-byte fd/bk links into reused guest storage (`RingInstanceDestroy` and
`RingDestroyHandleAlt`, flip 7,008). The four `0x30`-byte ThunderOrbit records
later inherit those host-pointer halves; this is allocator metadata, not an
unrelated typed game-field pointer leak. `004968a0`'s original age/color branch
can read an uninitialized color for age -6, changing its local RNG consumption
and later the shared battle-camera draws. The allocator must retain retail
four-byte metadata, not reproduce old ASLR-dependent host pointer garbage.
No zero-initialization, RNG normalization, or old host-pointer reproduction is
applied. At the matched flip 7,019 entry, bolt ages `[-5,-8,-1,-6]`, parameters,
radius and effect RNG agree; bolt 3's old color `a5088852` versus retail-heap
`015f9e38` selects active geometry versus respawn. The original code's
`age == -1` test permits this read before the new bolt initializes its color.

The approved castle controller re-plan changes only the four attack directives
to explicitly reopen Attack (`Circle/Cross/Cross`). It retains the same card,
eight battles, Dia between pairs, every live 4F predicate, Goho-M, native Save
OK and original 50,000-flip checkpoint. On the unchanged `9ea9c363` i386 and x64
binaries, both complete all 240 commands at flip 40,648 and pass at 50,000;
final RNG front/state agrees at `22/350ee6cd`. The tape SHA-256 is
`88883d1cab01c7c85672aae4a9c82dfc2f42758c0b721e2657013bc7da83fe7a`.
Paired binary hashes, logs and command are retained in
`<host>:<p4pc root>/lanes/PortStory3/diagnostics/divergence-9ea9c363/castle-replan-attack-v1/`.

**Oracle limitation:** this proves the revised route on both host ABIs, not the
PS2's bolt-3 branch. The immutable mapped-PAD replay first diverges at 157;
its capture at 7,019 never calls the thunder updater. The no-resync,
state-aligned replay has an earlier failed anchor and a separate Castle-4F
generation/path gap. Lockstep isolated that gap to the first differing RNG
entry, 65,868: `FldBoardGenerate` legitimately reseeds from the current
game-frame counter, native 3,552 versus retail 6,534 after variable-length loads.
The preceding 65,867 entries match all 31 state words and both cursors. Both
31-word initializations and complete 310-step seed warmups reproduce exactly,
with no later non-step transition in either load. This is not an unexplained
draw or a CRT/RNG arithmetic regression. The precise later missed-battle path
remains unproved; retail never reaches the owned-party COMMAND barrier and
there are zero golden thunder calls. The failed 4,096-draw reachability bound
does not justify a wider search, elapsed-draw resync, altered seed or cleared
guest field. The retail-uninitialized-read variant and oracle limitation were
explicitly accepted for the allocator cutover. The seed-cause finding above is
historical observed evidence, not a claim of a currently revalidatable paired
capture: the native tape/RAM and cause receipt subsequently became unavailable,
with no alternate paired native copy established. The retail half (on a private host) remains;
the cause of the missing native material is unproved. The separate two-ABI
castle re-plan/save proof cited above remains retained. No missing capture or
raw receipt has been reconstructed or represented as surviving.

Restoring the original callback words exposed a separate generic host ABI
error in afterimage cleanup. `AfPackRunList` casts a `void **` slot loaded
through a `void ***` table to a function pointer. The original executes
`jalr` through RW's free word at `004b708c` and `004b709c`; data-window
projection instead tried to execute `guest_base + 0044ee70`. The rewriter's
existing data-word-to-code rule now accepts every data-pointer depth,
while an already typed function pointer retains its own signature handling.
The allocator oracle also exercises the real afterimage cleanup and compares
its head/tail globals and both checked frees against unchanged instructions.
This is not an allocator-address exception or a changed game lifetime.

Other negative-offset reads still require allocation provenance:

| Consumer | Evidence / policy |
| --- | --- |
| Closed spline `003b9a40` | Zero-length recurrence for n=0 or n=-3 reads payload-4 as a divisor (`003ba1e8` / `003b9ea8`), but no point accumulation uses its inverse when n<=0. Keep exact requests/seeds and the established allocator policy; do not enlarge requests to hide the read. |
| Font ring `0026e170` | A real payload-4 reader, but `rbRingBufCreate` (`0026e010`) explicitly writes the original allocation word inside its **own** requested block and returns base+4. It is not a chkmem header. |
| Font list `0026e120`, script pool `0029d1c0` | Node/link prefixes owned and initialized by their respective ring/pool constructors, not words preceding the underlying allocation. |
| RW resource alignment | `baresour.c`'s PSP-only alignment path explicitly stores/restores its own backpointer inside the requested block; it is not an assumption about the outer allocator. |
| Newlib bigint and math predecessor loads | Previous limbs/table elements within their own objects, not checked-allocation headers. |

The ELF negative-load census found 8,299 non-stack/non-gp negative loads, including
78 literal -4 loads in 36 symbol-start intervals. These counts are triage, not a
proof that every computed negative address is a header: the odd spline path, for
example, computes its predecessor address before loading. Source/callback
provenance and self-written prefixes supply the relevant distinction.

The compiled sort callers do not order allocator payload addresses: scene/resource
comparators use camera-distance differences, item/persona comparators use item
prices or explicit numeric keys, and RW triangle/vertex comparators use material
identity plus stored raster/pipeline/texture/material/vertex indices. In
`bameshop.c::SortPolygons`, the old comment about memory order is not the
implementation: only material equality is tested before numeric indices. The
promoted comparator `003c6080` likewise compares its four stored halfword keys.
Changing the host sort's equal-key permutation was observable; pointer numeric
ordering in these compiled callers was not evidenced.

The modeled RNGs keep explicit guest state (`RpRandomSeed` / `RpRandomSeedMT`),
not host rand state or allocation-derived seeds. Startup seeds from the modeled
counter at `0x10000000`; model/floor generation selects the explicit argument,
`iGpffffb1a0` counter or `iGpffffb214` override. No host usable-size query or
host-allocation-address seed was found in these consumers. This audit does not
claim that different guest heap layouts reproduce original retail pointer bytes.

The old assertion that nothing reads chkmem globals was false. `chkmemGetBytes`
is called at `0014f2e0` and `0014f6cc`; the field-load metric at `007642f8` has four
gp references (`0014f2e8`, `0014f6d4`, `0014f6dc`, `0014f700`) in the queue/load
accounting paths. `chkmemGetClasses` at `00454b98` captures the HCdvd class tag.
The restored matched checked layer now maintains these original accounting
values rather than presenting mspace statistics as checked allocation-byte
parity. The address/header audit alone was not sufficient to justify the former
cutover: allocator metadata can be observed through retail's own stale writes,
not only explicit header readers. The Marukyu lifetime and original-ELF complete
heap/bin/header/accounting comparisons cover that distinction.

## Inactive software movie decoder

Every production profile requires FFmpeg. The actual ingress is
`H_Cdvd_StartPlayback -> p4pc_movie_start -> native SFD demux -> libavcodec`;
it creates neither the reconstructed RE4 MPV decoder nor its SFD player object.
The reverse call graph closes over 25 `src/cri/re4/mpv*` units (143 definitions)
and 50 cold caller definitions in seven mixed Sofdec units. Analysis of the
completed `9ea9c363` objects found 36 cross-object incoming pairs / 73
relocations, all owned by those removed functions or their five constant-table
initializers, not a retained production entry.

[`config/inactive_movie_decoder.json`](../../config/inactive_movie_decoder.json)
is the exact cut: the generator removes those 25 generated source copies,
50 mixed-unit bodies, ten registrations and their eight retail markers. Each
body/initializer/marker must match once or generation fails. The immutable
decomp submodule is not edited. Live ABI inputs are explicitly listed by CMake,
so stale removed-unit metadata cannot recreate a registry entry.

The two obsolete source-bound records for `MWSFCRE_DestroySfd` and
`mwPlyStartFnameRange`, and nineteen now-unreferenced addressless call-trap
records, are removed too. They are not retargeted to fake replacements. The
table generator continues checking the exact surviving registrations against
the actual profile objects.

The generated retail patch map also retires exactly `00760450`, `00760454`
and `00760458` (software-model Init/Finish/ExecServer), checking their original
code words before omitting them. It does not synthesize traps or replacement
success stubs for dangling registrations. The other eleven original IPU
interface words, separate original PS2 IPU implementation, all passive frame
helpers, six native MWPLY forwarders and native create/status/destroy remain.
This is a production-reachability deletion, not a reconstruction of the
software decoder's `skip_func` contract and not a claimed PS2-verified n-fix.
The source/object graph receipt is `<scratch>/studies/PortCRT/mpv-production-cut.json`.

The post-cut `movie_start` witness passed on Linux i386, Linux x64 and actual
native Windows: each decoded 41 pictures and 66,048 audio frames in 60 updates,
with traps forced fatal. The native command, PE/DLL hashes and rc=0 log are
retained in `<scratch>/studies/PortCRT/final-native-movie/`.

## Differential witnesses

The standalone exact-source closures were exercised on Linux x86-64 and native
Windows x86-64 with the shipping UCRT, in ordinary and shipping EE FPU modes:

| Closure | Original-ELF comparisons on each platform |
| --- | --- |
| Double libm plus native-register adapters | 22,584 exact return-bit comparisons; digest `b93c20db0248d73f` |
| dtoa | 22,960 exact cases |
| Signed/unsigned 64-bit conversion helpers | 2,552 exact cases |
| Formatter | 50,924 exact cases, 639 inventoried formats; named and masked families included |
| Actually rewritten Sint32/Uint32 formatter calls | 5,888 exact cases; also exercised on Linux i386, including `%ld/%lld`, negative values, native/guest pointers and mixed real/soft-double arguments |
| strtod / scanf / tokenizer | 23,196 exact cases in normal and EE FPU modes |
| SPU Note2Pitch | 65,542 exact original-IRX comparisons; digest `c63bc828366831b6` |
| EE newlib and checked RW heap | 2,856 complete pointer/heap/bin/header/errno/checked-state comparisons, digest `93f1867bca19fe1d` |

These interpreter runs execute the unchanged user-supplied ELF/IRX with no native
algorithm hooks. They compare bits, lengths, destination bytes/canaries, end/count
slots and guest errno, not a host-library approximation. No retail assets or
extracted module bytes are committed. Optional dtoa modes not reached by the
inventoried formatter are not claimed by the formatter's corpus.

After separating the unchanged byte implementations into `ee_bytes.c`, the
Linux suite passed all twelve selected CRT/animation tests and the eleven
actual native Windows/UCRT CRT executables passed again from source
`8b9a9e330fc13ca44a821057b56bceb45ee1c923`. The rebuilt Linux/Win64 full
aggregates also linked every isolated fixture; Clang 19 separately passed the
five affected Linux byte/heap/animation fixtures. The fresh Windows package
hash manifest is `<scratch>/studies/PortCRT/byte-split-win64/manifest.json`;
its individual logs and result receipt are retained in
`<scratch>/studies/PortCRT/byte-split-native-results/`. This is a fresh
qualification, not recovery of the older raw files described below.

The native Windows `59f4d485` qualification ran the eleven actual UCRT
executables, not Wine, using the canonical CPU lease. All passed, including
the scanner/tokenizer and complete heap oracles. The eleven original rc=0
stdout sections were recovered from the retained tool-result transcript to
`<scratch>/studies/PortCRT/recovery-transcripts/native-crt-59f4d485.bg_1110.stdout.txt`
(SHA256 `07f885b3befe51a64761a6c99784240b9a6a49b47d11559333704da7f32d53ca`).
This is newline-normalized retained stdout, not a rerun or a claim that the
lost original per-test log bytes were recovered.

A concrete former UCRT difference is `%4.2f` at 1.375: host 1.37 versus retail
1.38 (123.456f similarly produced 123.45 versus 123.46). Equal-key qsort order and
signed/unsigned 64-bit saturation/truncation are likewise intentional retail
bindings, not golden normalization. PTank rotations use the retail single sin/cos
calls seen at `003abdac/003abdb8`. SPU pitch uses the original integer Note2Pitch
lookup/rounding after combining tone and bend, not host exp2; see [audio](../audio.md).

Arithmetic oracles do not replace gameplay qualification. Keep state-word,
boot/newgame RNG, floor seed/grid and 22-row walk witnesses unchanged; compare
semantic bytes without retiming, dropping rows or rewriting goldens. Follow
[numeric state diagnosis](numeric_determinism.md#state-diagnosis-and-retained-oracles)
and save-anchored lockstep for any newly observed gameplay difference.

Allocator candidate `50ea2482` passed 2,856 complete original-ELF pointer,
heap/bin/header/errno/checked-state comparisons on Linux x86-64, digest
`93f1867bca19fe1d`, including the real afterimage cleanup callbacks. Its pristine
Marukyu replay reached the post-Rise Teddie COMMAND, completed the unchanged
tape at flip 44,098, and passed the 60,000-flip gate at arena `242:3`, date 85.
The gate's expected location is the post-event battle arena, not its `25:2`
load seed.

The same Linux candidate retained all 176 exact-at-goal state words for both
registered newgame and April story routes, all 312 boot RNG rows, all 322
newgame RNG rows, the complete 22-row walk digest, and the retail `19c3` floor
seed/checksum against the accepted `1057e23b` records. No row, state byte or
golden was normalized. The external retained proof is
`<host>:<p4pc root>/lanes/PortStory3/diagnostics/semantic-50ea2482/results-qualified/`;
the corresponding Marukyu proof is in
`diagnostics/allocator-50ea2482/qualified/boot.log` under the same lane.

Native UCRT candidate `59f4d485` independently passed the same six registered
semantic cases and matched every accepted raw hash: both 704-byte state
snapshots, 312 boot RNG rows, 322 newgame RNG rows, the 22-row walk and floor
seed/grid. The original six-case completion stdout, including those recorded
hashes and verdicts, is retained at
`<scratch>/studies/PortCRT/recovery-transcripts/native-semantic-59f4d485-qualified.bg_1134.stdout.txt`
(SHA256 `7f690bc61e4fada9a6f94dab0d7d879f26cd1d009cf3f0ea79b76e0973a35879`).
This is newline-normalized original tool stdout; it does not purport to recover
the original per-case raw files lost from the inactive study scratch directory.

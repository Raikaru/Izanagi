# Decomp interfaces and remaining port ABI adaptations

This records qualified upstream cutovers and the machine-ABI views that remain
in the port. `config/mirror_fixes.txt` is the authoritative per-site list: each
remaining row has a root category, a concrete reason and an exact match count.
The source-side contracts and verified symbol roles are documented in
`decomp/docs/port/retail_argument_repairs.md` and
`decomp/docs/port/runtime_symbol_roles.md`.

## Floating-point, pointer and variadic ABI

EE binary64 helpers use the guest register/slot ABI. Native bindings in
`src/platform/ps2_fpbit.c` and `src/platform/ps2_libc.c` use explicit 64-bit bit
containers where a host `double` would use a different register class. That
container spelling is not a replacement type to copy into every PS2 caller.
Likewise, guest address words, real host pointers and PS2 by-value aggregates
must not be conflated merely because they fit the same EE GP register.

The formatter/scanner entry roles are established upstream. `vprintf` returns
an integer; `vsprintf` and `_vsprintf_r` are not the digit generator `_dtoa_r`.
The port still owns the bridge between eight-byte EE vararg slots and native
`va_list`, including the bank manager's register-save-area construction.
Native colored variadic forwarding and MWCC's `__sqrt` intrinsic lowering are
host ABI/backend adaptations, not new PS2 C interfaces.

Matrix/native dispatch still needs the host's SIMD alignment and address
translation. Source-only ASM owners are not made into matching C by a native
override. Matching-source codegen exceptions keep their measured PS2 argument
views, while the port supplies the required host argument order or width.
No generic ABI pass, checksum, mirror contract or census guard is disabled.

## Retail argument audit upstream cutover

The gitlink consumes the matching-source repairs described per call site in
`decomp/docs/port/retail_argument_repairs.md`:

| Cohort | Decomp commit |
|---|---|
| GP/FP signatures and class-only declaration | `700bdc286432e17cf71dd4545aff3b785819ab05` |
| Full-width file-size declarations and forwarding | `c01602a24a3e0ef1c4c9e2c0dadecb725aad49d8` |
| Live incoming arguments and style-kind width | `b0960238c391d19578712acc73c4d69e1b8f315c` |
| Smap aggregate declarations and callers | `854e43d25d5b511503d919cd1d2841c85a64dece` |

The forty omitted-argument findings comprise 38 active upstream C sites and
two unmarked SDK reconstructions active only in the port. Thirty-seven sites
are repaired upstream. The write wrapper `003de6a0` stays documented-only:
passing its 64-bit count to the actual 32-bit syscall formal adds conversion
instructions not present in retail. Its two host mirror rows remain.
At that first-round pin, `SFD_Init` and `mwSfdExecDecSvrHndl` retained their
port rows because their authoritative upstream retail owners were still ASM.

Of the nine positional findings, six concern upstream C: the clip-plane and
screen-quad fixes move upstream, while both shadow-spawn calls and both
`shdPersonaDrawByState` background calls retain their host repairs. Direct
canonical source spellings change the matched PS2 instructions; upstream now
documents the actual GP/FP inputs. The other three are port-only matrix
overrides, not missing upstream C fixes. `FldNpcControlUpdate` already uses
the canonical `RwMatrixRotate(matrix, axis, heading, POSTCONCAT)` interface
at the old pin. Generic alias/prototype reconciliation remains necessary for
the other port overrides; no function-specific generator bypass is added.

Smap keeps the matched by-value `RwV3d` definitions and aggregate-copy
prologues upstream. The host still needs pointer parameters, so its three
`y_misc_inferred.c` adaptation rows are retargeted to the corrected aggregate
calls rather than removed. All other Smap host adaptations remain.

The cutover removes 90 literal mirror rows and the two `004e7798` passthrough
rows that upstream makes redundant. Remaining rows stay fail-closed, as do
the reviewed per-pass mirror contracts and per-target guest-rewrite censuses.
The audit's four intentional exclusion classes are unchanged.

Source-ownership counts are disjoint: omitted arguments have 38 class-(a)
active upstream C sites and two class-(c) authoritative ASM owners; positional
findings have six class-(a) sites and three class-(b) port overrides. The
class-only declaration is also class (a).

The strict x64 census identified eleven changed rule counts in five units,
all corresponding to these source repairs: the name pointer no longer needs
call adaptation, the two CRI handle loads now have pointer types, and style
kind no longer needs narrow-argument adaptation. The common counts are
updated in all four target profiles without replacing profile-specific
counts or Clang-version patches. No census check or mirror-contract guard
is disabled.

### Qualification

All four source-cohort remote gates passed: 8,101 C functions linked,
both retail SHA1 checks passed, 6,860/6,860 first-party functions matched,
2,674 other third-party functions matched, and there were no rows outside
MATCH/ASM. Each gate ran 857 unit tests with twelve skips. Published head
`854e43d2` also passed [CI run 40048441982](https://github.com/Raikaru/Persona4-Decompilation/actions/runs/40048441982),
including proprietary build and verify. The affected guarded CVFS recovery
unit separately compiled with EE-GCC and `-DNON_MATCHING`; production keeps
the existing assembly fallback.

The isolated port comparison used baseline `7b17dc0e` / decomp `d19f9e22`
and candidate `78b71f03` / decomp `854e43d2`. On `a Linux host`, both before and
after passed `tools/remote_check.sh` with `P4PC_SCOPE=main` (119 Linux i386
tests, 97 Windows i386 tests, two skips each) and `P4PC_SCOPE=x64` (123 tests,
no skips). The first candidate's strict x64 census rejected the eleven
expected rule-count changes; only the reviewed counts above were updated.

Registered NewGame, April-15 story, RNG boot/floor and walk-digest commands
were replayed with identical game arguments and controlled environments
before and after. Input was script-only; both runs used the same disc implementation.
Read-only normal-exit GDB snapshots kept ASLR enabled. No RAM or clock was written. Both i386 and
x64 had identical before/after results:

- All 176 flag words (704 bytes) at normal NewGame exit, and all 176 words at
  April-15 story exit: zero changed words.
- Boot seeds/draws: all 312 recorded rows identical, and the seeded floor grid
  still equalled retail's.
- NewGame RNG stream: all 322 seed/draw rows identical.
- Walk/battle digest: all 22 rows identical.

Raw private snapshots and child/gate logs remain in the four owned
`DecompUpstreamBefore32`, `DecompUpstreamBefore64`, `DecompUpstreamAfter32`
and `DecompUpstreamAfter64` remote lane proof directories; the after-x64
directory also contains `upstream-comparison.json` with hashes, commands'
manifest checks and per-snapshot changed-word indexes. No retail bytes or
temporary proof launchers are committed.

## Complete mirror and prototype cutover, second round

The source audit starts at decomp `854e43d2` and the port's frozen 317 literal
rows, 156 explicit passthrough records and 567 generated prototype diagnostics.
The published source commits are:

| Cohort | Decomp commit | Full gate |
|---|---|---|
| Retail interfaces, SDK/data ownership, Config/newlib/CRI names, complete input chains and aggregate aliases | `2bbc2a90b5f4a2cf1a95d124d65615d5553773a6` | 8,103 linked C; both SHA1 OK; 6,860/6,860 first-party and 2,674 other-third-party matches; 859 tests, 12 skips |
| Allocator names, pointer-returning forwarding callbacks and ignored normalization results | `7d64e64ff972d64fd316d2f934b7a89028869e42` | Same complete gate counts and checksums |
| Complete AHX decoder input and shared debug formatter result contract | `d94be54b449497110c9ae0f03fb70d83346cda6c` | Same complete gate counts and checksums |
| Original-compiler word-packing C and descriptor/process-status inputs | `deabd5be6a5558ec7c2e24c6039eecb7dc3b25e3` | 8,104 linked C; both SHA1 OK; 6,860/6,860 first-party and 2,674 other-third-party matches; 859 tests, 12 skips |
| Full raw soft-double results in direct and colored listbox formatting | `151a08e2088599ec94029a31573cb42431293595` | 8,104 linked C; both SHA1 OK; 6,860/6,860 first-party and 2,674 other-third-party matches; 859 tests, 12 skips |
| Thirty weighted-target callbacks: pointer/query slots and selector results | `4ef90cefbd95a320d8b3de993fa262ff5072b2ff` | 8,104 linked C; both SHA1 OK; 6,860/6,860 first-party and 2,674 other-third-party matches; 859 tests, 12 skips |
| Defined unsigned error-word shift; measured variadic input view retained | `1aec1d67c8fdbfd62c7f10efe3ebdb7d79104f64` | Same complete 8,104-linked gate |
| Full tint completion word and packet callback result forwarding | `ef9ec11ba70bc775fe6890d46d411775773cb8c7` | Same complete 8,104-linked gate |
| Live MPS error code and Persona node/work stored callback contracts | `d40c7c03d8e39b4b424d213ff9293fa83e5adbc4` | Same complete 8,104-linked gate |
| Thirty-six actual condition-table wrappers return their search status | `614dbf3f0419107b6dbbefec2a7f74528cc2e5fb` | Same complete 8,104-linked gate |

The first nine functional cohorts also passed CI: [40315017720](https://github.com/Raikaru/Persona4-Decompilation/actions/runs/40315017720),
[40321727805](https://github.com/Raikaru/Persona4-Decompilation/actions/runs/40321727805),
[40329142344](https://github.com/Raikaru/Persona4-Decompilation/actions/runs/40329142344),
[40335055659](https://github.com/Raikaru/Persona4-Decompilation/actions/runs/40335055659),
[40371834205](https://github.com/Raikaru/Persona4-Decompilation/actions/runs/40371834205),
[40380957384](https://github.com/Raikaru/Persona4-Decompilation/actions/runs/40380957384),
[40387475280](https://github.com/Raikaru/Persona4-Decompilation/actions/runs/40387475280),
[40388873416](https://github.com/Raikaru/Persona4-Decompilation/actions/runs/40388873416), and
[40390118462](https://github.com/Raikaru/Persona4-Decompilation/actions/runs/40390118462).

Documentation-only `795a8e98774a7939c8c58a135cb12bbe990018fb` clarifies verifier
statuses versus real-C linkage without changing source or headers. Its required
proprietary build/verify and free checks passed
[CI run 40352310199](https://github.com/Raikaru/Persona4-Decompilation/actions/runs/40352310199).
The strict link floor remains 8,101. An intermediate 8,099-link candidate
failed despite clean normalized verify results: entirely third-party gapped
objects cannot be placed just because each marked function matches. The
corrected SJD getters use contiguous existing SDK owner groups; the accepted
link contains 8,103 real C functions. No floor or fallback hash was changed.

The frozen literal set falls from 317 to 115: 202 rows become redundant.
All 156 explicit passthrough records become source-redundant. The two guarded
SFADXT input fixes and other optional SDK reconstruction corrections retain
their authoritative ASM; they are not counted as new matching C functions.
The separate CRT scan-slot cohort also moves upstream: `%lx` keeps its eight-
byte EE destination, while actual data/size/offset consumers use the correct
low-word width and signedness.

At this cutover there are 96 genuine host adaptations and 19 measured or
inherited matching-C exceptions in the original literal set. Each row retains
its own concrete rationale. Host adaptations include native SIMD alignment,
guest/host pointer conversion, low-memory/MMIO behavior, native video and
widescreen seams, allocator policy and native-only registration/export seams.
Matching-C exceptions cover the 175-byte record-query and 911-byte packed
digit-color residuals, the documented syscall/shadow-call scheduling views,
and explicit host models of EE reused/uninitialized register or stack state.
They are not arbitrary missing-argument padding.

The three prototype aggregate-alias diagnostics are resolved upstream, as
are uniform caller-supplied unused inputs. Removing existing caller operands
was the wrong hypothesis for several interfaces: declaring the complete
callee contract preserved the original instruction setup and eliminated those
private repairs. True mixed-arity four-register SDK forwarders, genuine EE
aggregates/128-bit values, variadic interfaces, native definitions and measured
width/order exceptions remain conservatively classified rather than guessed.
An MWCC micro-witness shows that adding an unread ellipsis still introduces
an 80-byte frame and seven GP-register home stores, so it is not a matching
substitute for these views.

The native word-packing override at `004f5860` is removed: upstream now states
the real `u32` high/low inputs, full `s64` result and an active matching C owner
under the original EE-GCC 2.96 compiler. Its five live instructions and complete
linked retail image match; this raises the real-C link count from 8,103 to 8,104.
Geometry and boot-heap overrides use the owning source contracts instead of stale local declarations.
The generic tail-result pass also emits built-in `int`/`unsigned int` in a
header with no visible project/CRI integer namespace; it no longer invents a
dependency on `Sint32`/`Uint32` for an otherwise self-contained callback header.
Two parser/transform regression fixtures cover that namespace choice and
idempotent unsigned-result propagation.

The complete candidate prototype report remains a conservative lexical ABI
audit, not a list of PS2 execution failures. In particular, the three aggregate
diagnostics remain even though their concrete source and mirrored calls now
use the canonical types. The apparent zero-argument `strlen()` introduced in
that report is an explanatory SDK comment, not a real zero-argument call.
The corrected `SVM_TestAndSet` name exposes an existing four-GP SDK bank view,
while the real lock consumes one pointer. Those are classified separately
from the newly repaired live AHX input at `004cdca0` → `004c1c50`.


The newly found descriptor/process interfaces also move upstream. `fstat`
ignores a signed 32-bit descriptor, not a host path pointer; `kill` forwards a
signed status word through the runtime-exit chain. Their owning declarations
cover the actual SDK syscall and `kwlnExitRuntime`. The CRT's host namespace
aliases remain necessary; its three local width/prototype rows become redundant.
The complete six-owner witness reports 206 `MATCH` and 323 `ASM` results,
with no failures. Those are verifier statuses: existing assembly-bodied
syscalls can report `MATCH`, and are not newly recovered C. The real-C link
increase and complete-image check for the new word-packing owner are separate.

The generic generator regressions pass all 48 fixtures under the managed Linux
toolchain. Expected fail-closed contract-negative fixtures emit diagnostics;
the suite itself exits zero. This is generator evidence, not the paired runtime
acceptance or a substitute for the full port landing queue.


The listbox cohort `151a08e2088599ec94029a31573cb42431293595` also keeps
`fptodp`'s complete raw 64-bit result in both listbox formatting owners.
The direct `sprintf` branch takes raw bits, not a native `double`; the native
colored-text `va_list` branch still needs its bit-copy adapter; its source
selector after this pin is `u64 r`. A separate raw-bit temporary keeps the
unrelated integer cell-width calculations at their retail width.
Both complete owners verify 19/19 first-party `MATCH`. The prior generic
soft-double pass already repaired these source-width defects in the host mirror;
the source change removes that private work rather than claiming a new host fix.
Complete mirror generation reduces the generic soft-double declaration repairs
from 44 to 43 and widened result locals from nine to six. Literal rows remain
115 at this source-only stage; no redundant literal was added for a repair the
generic pass already supplied.


The callback cohort `4ef90cefbd95a320d8b3de993fa262ff5072b2ff` expresses the
thirty weighted-target callback contracts directly: `s32(u8 *, u32)`, not
`void(u64, u32)`. The wide first argument consumes an extra i386 stack slot,
misplacing the query; x64's one-register-per-argument ABI hid that mismatch.
Each wrapper now returns the selector result that retail leaves in `v0`.
The two fixed-DOWN callbacks expose but ignore the dispatcher's query.
The complete command owner retains all 263 first-party `MATCH` results.

`btlAICommandNomalStatusCheck` at `001db5e0` remains a separate measured
wide-input source view: its original 64-bit shifts consume only the signed
low halfword. A host-only trailing-query normalization must retain that
meaning and update the owning mirrored header consistently. It is not a
reason to preserve the thirty incorrect wide-first-argument source interfaces.


The stored-callback cohort `d40c7c03d8e39b4b424d213ff9293fa83e5adbc4` also preserves
the scheduler's full callback results: the tint interpolator returns `u32`,
and the packet callback dispatcher returns its installed callback's `s32`
result. The complete owners retain 27 and three `MATCH` results respectively.
MPS's library/handle error slots now carry the existing code argument, while
Persona draw-command storage retains both node and work inputs without an
erasing cast. The two MPS owners retain 14 marked SDK matches and the complete
Persona owner retains 104 first-party matches. Unmarked MPS setter/error
reconstructions remain assembly-backed, not newly matched C.

Three further interface normalizations remain host-side. Narrowing the two
target-dispatcher query views changes 82 and 90 normalized bytes; their low
24-bit query meaning is nevertheless a single host word. The variadic error
callback at `003df590` retains a measured wide formal to reproduce its two
low-word sign-extension shifts. Its signed-negative left shift is removed
upstream using an unsigned left shift, but its host indirect call still needs
the real `s32` first slot. A narrow direct return loses the two shifts; a
narrow formal plus explicit widening adds an unwanted pair. The complete
error owner reports 60 `MATCH` and 71 `ASM` results.


The condition-table source pin `4776c14ebc63d7cb4ce1b62b72684fc0c01e0e14` includes
`614dbf3f`'s complete condition-table result recovery and a documentation-only
clarification of the table census. Retail `00609850` has 78 twelve-byte rows:
one null entry and 77 distinct non-null callbacks. Thirty-six source targets
discarded the helper's status: 22 enemy-side and 14 allied-side wrappers.
All now return `s32`; their helper call at `001d94d0` leaves that exact result
in `v0` through the original restoration and return. The complete two owners
retain all 320 first-party matches, with shared declarations for every wrapper.

`btlCond_ENGROUP` forwards its existing word query unchanged. Only the side mask
is narrowed to 16 bits; the MYGROUP predicate ignores the query, classifies the
action's skill halfword and returns a full one/zero. No query-width correction
is inferred from those unrelated narrowing operations.
The final documentation-only child and identical functional source/header bytes
passed proprietary build/verify and free checks in
[CI run 40395743387](https://github.com/Raikaru/Persona4-Decompilation/actions/runs/40395743387).

Complete mirror generation at this pin reduces generic tail-result repairs
from 59 to 46. All six additional contract-stage deltas are confined to the
two matched owners and their existing condition header. The 276 prototype
diagnostic records remain conservative and unchanged; no guard is disabled.


The registered-plugin source pin
`640e9e4506e17668f56c6fc684de230ed2c124a5` additionally recovers six
RenderWare identity callbacks as pointers with their complete registered
copy, destructor or stream-writer interfaces. Forty-two copied plugin
declarations, including six unused SkinSky declarations, are removed.
`kwlnPrintStreamError` and `kwlnLogStreamError` share their actual two-input
interfaces; the variadic stdio owner keeps integer results and the original
message-as-format behavior. The twelve complete owners retain 179 `MATCH`
and 140 `ASM` statuses with no failures. The full pre-push gate preserves
8,104 linked C functions, both retail SHA1 checks, all 6,860 first-party
matches, 2,674 other-third-party matches and 859 passing tests (12 skipped).
This is not a claim that existing assembly-backed functions became C.

The registered-plugin pin also passes both jobs in
[CI run 40432228122](https://github.com/Raikaru/Persona4-Decompilation/actions/runs/40432228122).
Complete source-mirror generation preserves the 115 literal rows, zero
explicit passthrough records and byte-identical 276-record prototype diagnostic
census. Its reviewed 56 contract records in eleven stages touch only the
fifteen committed source/header paths. Every existing transformation keeps
its exact removed/added hunk shape; the only new path is the shared plugin
header. No strict contract guard is relaxed.


The model-callback source pin `342ed92597942ccbcaeaeed813eada0c80cffa10`
also includes `015c4d98`'s release slot result correction. Retail primitive
dispatch has twenty distinct non-null release targets and thirteen rotation
targets, all returning void. The stored/local release views and local/loaded
rotation views now declare void results. Both original indirect calls ignore
`v0`; arguments, bodies, VU operations and iteration remain unchanged.
Complete owner comparisons retain two and ten first-party matches.
Each of the two commits separately passes the full pre-push gate with 8,104
linked functions, both SHA1 checks, all 6,860 first-party matches, 2,674 other
third-party matches and 859 passing tests (12 skipped). The corresponding
incoming old-pin port return-type rows are redundant after this pin.

The final model-callback child passes free checks and proprietary build/verify in
[CI run 40438290126](https://github.com/Raikaru/Persona4-Decompilation/actions/runs/40438290126).
Complete source-mirror generation leaves all aggregate counters and the raw
prototype report unchanged. Its seven reviewed fingerprint deltas, across
five stages, affect only the two model owners; all transformation hunk shapes
are identical to the prior reviewed baseline.


The linked-slot source pin `0b214e7ed65f7ffaf42b964bafe3debef932de67`
recovers the existing `s32(void)` completion callback in the shared sound
owner, removes two copied void declarations and preserves its signed result
when storing the event slot. The real callee already returns status; the
actual action event consumer branches on that result before resuming state.
Neither body changes. The four complete owners retain 102 first-party
matches, and the full pre-push gate again preserves 8,104 linked functions,
both SHA1 checks, all 6,860 first-party matches, 2,674 other-third-party
matches and 859 passing tests (12 skipped).

The linked-slot pin passes free checks and proprietary build/verify in
[CI run 40445876758](https://github.com/Raikaru/Persona4-Decompilation/actions/runs/40445876758).
Complete source-mirror generation keeps the 115 frozen remaining literal
rows, zero explicit passthrough records and byte-identical 276-record
prototype report. Eleven reviewed fingerprint deltas across seven stages
touch only the sound header and two migrated caller files; existing
transformation hunk shapes do not change.


### Accepted CRT baseline cutover

The control is accepted port `4f6586fbe6e768a343dd3014d1f3e3685b653f62`
(submission 000292, batch 80575), with the unchanged old decomp pin
`854e43d25d5b511503d919cd1d2841c85a64dece`. Its immutable land receipt is
`<scratch>/land/check-4f6586fbe6e7.52bcf59ccb/result.json`: result zero,
all six phases complete. The accepted inventory contains 479 literal rows,
including five read-only host provenance hooks absent from the earlier
unaccepted snapshot and two later copied completion-callback declarations.

The source cutover removes exactly 345 accepted rows. It keeps all 134
remaining accepted adaptations and makes two previously implicit native
normal-status query views explicit, for 136 final rows: 111 genuine host-only
adaptations and 25 measured matching-C exceptions. Each row states its reason
category. The two new views normalize both the definition and owning header
of `btlAICommandNomalStatusCheck`; its signed-low16 body is unchanged. The
matched PS2 source retains its measured wide formal and shift sequence.

The existing colored-debug native `va_list` conversion now selects the
upstream raw `u64` local, and the existing narrow error callback adapter now
selects the upstream unsigned-left-shift expression. Both keep their accepted
native replacements unchanged. No accepted presentation, read-only
provenance, CRT algorithm, input schedule or game-clock policy is removed.

Complete generation against the accepted CRT baseline applies all 136 literal
rows and zero explicit passthrough records. The 1,207 active units preserve
the accepted decoder cut; its 25 inactive units are not removed by this
source-contract change. All 1,989 changed fingerprint records across 32
stages are accounted for: 527 paths changed in the gated decomp history and
five induced paths were reviewed against their complete generated source.
No strict pattern or counter guard is relaxed.

The accepted control has 574 prototype diagnostic records (572 unique);
the candidate has 277 (275 unique). These are diagnostics, not raw report
line counts or new C-match counts. The two candidate-only records remain
conservative: the SVM call set includes a four-register native view, and
the `strlen` zero-argument appearance comes from the SDK comment/parser
view rather than a real zero-argument retail call. The extra `func_00442a80`
type diagnostic is the native fixed-width `strncat` binding; it already exists
in the accepted CRT baseline.

Actual Windows compilation exposed a separate generator error: return
alignment copied the MWCC function modifier `asm` into otherwise correct C
declarations. Return-type extraction now strips that vendor modifier along
with storage qualifiers. The entire generated change is removal of `asm`
from two declarations in `code1_0042.c`, not a changed syscall body or a
per-site text repair. Four follow-on fingerprint records were reviewed.
All 52 generator/parser fixtures pass, including the new synthetic
already-correct/incorrect-return and idempotence regression.

All four actual census profiles retain 1,207 active units and zero unsupported
constructs: Linux x64/clang23, Windows x64/clang23, Linux AArch64/clang19 and
Android arm64/clang18. The existing Linux x64 clang19 compatibility section
was also regenerated from its complete active set; its two compiler-specific
rows are retained, not discarded or copied stale.

Each primary profile changes 85 unit records. The sole new owner is the
matched `rfmai_wordpack.c`, replacing its removed private override; no active
unit is dropped. Existing-unit signed rule deltas agree between Linux x64
and AArch64. Android's additional changes are its compiler18 layout-oracle
accounting. All changed paths belong to the gated source/fingerprint history
except one unchanged `g_data` mirror: its newly included shared
`datPanelDrawByKind` declaration adds one existing packed-float-pair ABI
adaptation. Complete generated-source comparison shows only header order
and declarations there, not a changed function body.

All collections kept census checking enabled and stopped on the old baseline
until their complete deltas were reviewed. Normal checks use these explicit
per-unit baselines with the same strict guards.


### Canonical callable ownership and duplicate placements

The source pin is `363eab8a966d60a9773b45f330f2344329ec4f0a`. Its full
pre-push gate retained 8,104 linked C functions, both original SHA1 values,
6,860/6,860 first-party matches and 2,674 other third-party matches, with
859 tests passing (12 skips). The gate took 484.60 seconds; public CI
40478998576 also passed free checks and proprietary build/verify. This is
the fifteenth gated functional/configuration commit in this round, not an
additional C-function match.

Normal i386 and x64 table generation exposed thirteen now-redundant private
placements. Eleven data identities already have source-owned addresses and
kinds. `RpClumpForAllAtomics` (`003bff30`) and
`_rpWorldSectorNativeRead` (`003cebb0`) now have authoritative function-kind
entries from the maintained canonical-name overlay. The generated generic
relocation file is not hand-edited. All thirteen private rows are removed,
reducing manual placements from 27 to 14 without weakening the existing
duplicate-placement guard.

The two clump override linkages, the reader's exported identifier and its
MIPS differential entrypoint records use the canonical names. No compatibility
shim, address, parameter, result, algorithm or storage change is added.
The full symbol-map difference is limited to obsolete address-form spelling
keys; all thirteen retained identities keep their exact addresses and kinds.

Complete mirror regeneration changes eight further fingerprint records in
only `code1_003c.c` and its excluded `code1_003c_cw119.c` alternate. Review of
both complete generated differences shows only those linkages, identifier
spellings and comments, plus the excluded declaration following its own
unchanged pointer interface. The 277 diagnostic records (275 unique) are
unchanged. Literal rows remain 136 and explicit passthrough records remain
zero.

### Final source-owner and getter-witness cutover

The normal links exposed a remaining duplicate private `criSetVoiceCounts`
forwarder. Its real matched source owner now supplies the entry at
`004cd470`; the native `criConfigureVoiceCounts` implementation is unchanged.
The standalone native-helper fixture keeps its configuration/error cases.
Its two-argument forwarding assertion moves to the already fully linked CRI
CPU fixture, which resolves the actual generated entry, checks its ABI and
all five resulting configuration words, then restores fixture state. No test
clones the removed body or masks duplicate definitions.

The waveform witness now names the actual inner `adxt_GetFmtBps` C body.
Its channel/format assertion is retained, and retail-address coverage now
checks both public/inner channel entries (`004d4c58`, `004d4c90`) and both
public/inner format entries (`004d4d68`, `004d4da0`). No decoder, renderer,
native getter algorithm, reference, schedule or limit changes.

### Paired i386 and x64 behaviour evidence

The immutable control is accepted port `4f6586fbe6e768a343dd3014d1f3e3685b653f62`
with source pin `854e43d25d5b511503d919cd1d2841c85a64dece`. The source-cutover
runtime payload is port `08ff6d429eaa6cae8cfa2fbf5e758e52f8f7dbde` with
source pin `363eab8a966d60a9773b45f330f2344329ec4f0a`.

All six cases pass on both sides of both ABI pairs. The five registered gate
commands/properties are unchanged; the sixth is the identical New Game gate
with the same `P4PC_LOG_RANDOM=630` on both sides. Every game environment
setting matches within its pair. Input is script-only; ASLR stays enabled.
The state snapshots only read 176 little-endian words at guest `0079b1cc`
at the unique normal successful exit. No guest-state, RNG or clock writes,
changed limits, schedule overrides or quarantine exceptions are used.

| Witness | i386 and x64 result | Raw SHA-256 |
|---|---|---|
| New Game field entry, flip 30,500 | All 176 state words identical; no changed indexes | `5e274ce90c1835d17966191013ae91e36b3147e5ee0ca345a09f27fb76d5b342` |
| April 15, flip 140,000 | All 176 state words identical; no changed indexes | `1d1fc6a0ea9cbc0350153dd25ee323bce7fc669fb275e9ffd8db8856b0fdd4e7` |
| Boot RNG | All 312 seed/draw rows identical (10,656 bytes) | `8509cea43d0f4ba5837b836294c655731a7ab89c285d9df69cb5e1557e7b69a9` |
| New Game RNG | All 322 seed/draw rows identical (11,036 bytes) | `8f05a738109b8f69c63605552e09f9891ce367731b118313ba49ca6d8b7cd391` |
| First complete floor grid, seed `19c3` | All 2,512 bytes identical; retail POSIX checksum `4034924771 2512` | `860674e2df82f12fd170e0683835d1d9ba496668746a5b97737b9b17104a2af0` |
| Walk/battle digest | All 22 rows identical (1,133 bytes) | `b523cb942a57b5a38d2c8f785bb0dd54d92d834c260ff6ef81e23c66dd45f385` |

New Game reaches field `7:2`, date index 10, with unchanged frame MAD:
0.8763% on i386 and 0.8790% on x64 (ordinary 5% limit). April 15 reaches
field `7:2`, date index 14, under the unchanged registered no-MAD-limit gate.
The comparator checks raw bytes, all state-word indexes, child exits,
registered manifests, commands and environments, not only these hashes.
Its complete JSON SHA-256 is
`141af51e1c32cb8082978d481102998818f2ad365bc9cb1b66e321b9dbf19fd4`.

Private raw evidence is retained under the managed
`<scratch>/studies/DecompUpstream2-4f6586fb` tree and the four corresponding
`DecompUpstream2Before32/64` and `DecompUpstream2After32/64` host proof trees.
Retail bytes and captures are not committed. This proves equality of the
requested witnesses, not byte identity of the PC executables.

### Integrated accepted-parent comparison and qualification

The checker-only CRT prerequisite landed as
`583359470ec98c7bf2dfe523acc8075e5ac0b17b` (submission 000300, batch 84470).
Its immutable receipt
`<scratch>/land/check-583359470ec9.ae839c08e8/result.json` has result zero
and all seven mandatory phases complete. Its source pin remains the old
`854e43d25d5b511503d919cd1d2841c85a64dece`; its 479-row literal file and
source contract file are byte-identical to the original accepted CRT control.
The cutover rebases onto this actual accepted parent, not its earlier private
candidate.

Other already accepted Mods runtime files had changed after the original
control. Therefore a second independent comparison uses fresh
`DecompUpstream2Control58332/64` old-pin controls and the integrated candidate
`d6c8b7a13d00943bd76bbe43b6fe200881f129b1`, preserving the original proof.
All six cases again pass on both sides of both ABI pairs. All 176 state words
at both checkpoints, both complete RNG streams, the retail floor grid and
the walk/battle digest are byte-identical; all six raw hashes in the table
above remain unchanged. Commands, properties, environments and ordinary
limits match exactly, with the same read-only snapshot method.

The new complete comparison JSON SHA-256 is
`2bec296ed1d895d1fb92c42443cd0c4c35eafd798b3a5f76f79b17a1f660df78`.
Its candidate evidence is in the separate `proof583` host directories,
and local `control58332/64` and `integrated32/64` directories retain the raw
captures. The original `proof` directories and control captures are untouched.

Normal integrated verification records Linux i386 152 tests, zero failures,
four skips, and Linux x64 156 tests, zero failures, one skip. The actual
Win32 ALL build also passes at the intended `O2/g1` settings with census
checking enabled. The additional full Wine reference suite is **not green**:
126 tests include five failures and five skips. Two EE oracles report failed
fixed guest-RAM reservation/missing `.ps2ram`; two independent Mods Python
fixtures execute Win32 children without the configured Wine prefix; and
the new CRT target-driver fixture has a real `.sh`-wrapper coverage regression
confirmed by its owner. These are recorded in
`check.H0uKQSUmpD/logs/check/ctest-win32.gUdGEf`; none is relabelled as a pass,
waived, quarantined or rerun unchanged by this cutover. The owning lanes are
handling their causal repairs separately.

Final requested symbol-list reconciliation with PortCRT, PortScanf, PortPrintf
and PortSettingsMenu finds no additional recorded correction. In particular,
`00443068` remains correctly curated as `_strtod_r`.


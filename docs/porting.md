# Porting notes

## How the decomp is compiled for PC

- The game's C and the RenderWare 3.7 core come from `decomp/src` unchanged
  and are compiled with clang for 32-bit x86 (`-m32`). The mirror target (`p4decomp`) adds
  `-malign-double`, which reproduces MWCC's struct layout. Port code is built without it.
- **Overrides.** Every decomp function has a `// FUN_XXXXXXXX` marker above its
  definition. A file `overrides/xxxxxxxx.c` (lowercase address) holds one
  replacement definition. At build time the generator copies each decomp
  source file into the build tree and splices the override's text in place of
  the original definition, so static helpers and file-local types still
  resolve. Overrides are for code that cannot run on x86 as written (PS2 vector
  unit asm, hardware access) or that must call the port's platform layer.
- **Data.** The retail image is loaded at its link address. The whole PS2 RAM range
  0x00100000..0x02000000 is mapped read/write/exec once, so the heap that kwlnBoot
  computes just below 0x02000000 is valid, and pointers inside game data stay valid.
  `D_XXXXXXXX` symbols are linker-defined absolute addresses. Words in data
  that point at functions are redirected to the native functions at load.
- **Missing pieces.** Any function with no native body links to a generated
  trap that logs its name and PS2 address.
- **Shared native address reads.** Use `P4_G2H` from `p4pc/guest.h`, not the
  64-bit-only `p4_g2h` helper. The macro is a guest-base translation on 64-bit
  builds and an identity cast on i386; frame-counter readers such as libpad's
  request scheduler must compile and read the same guest word on both.
- **Marker overrides may replace an INCLUDE_ASM.** When the `// FUN_` marker is followed by
  `INCLUDE_ASM(...)` (no C body in the decomp), the override replaces that line.
- **An override may open with includes, data, types or helpers.** After splicing, the
  generator moves the `// FUN_` marker line (a comment; the compiled text is unchanged)
  to just before the override's own body: the definition it replaced (its callers' link
  name, or `func_<addr8>`), else its first non-static definition when the override
  renames the body. Function-pointer tables, retail names and the mod hook catalog all
  read the definition after that marker, so a static helper or a data line first can no
  longer bind the address to the wrong function or none. An override with no non-static
  definition fails the mirror.
- **Marker functions link by name.** The mirror copy drops `static` from a prototype whose name
  a `// FUN_` marker introduces (`static T name(` before a later definition). Otherwise the
  retail-address alias cannot link to it (for example `FreeListCreate` at `0x003e1030`).
- **A marker or symbol can name the wrong body.** `config/unbound_natives.txt` lists compiled functions 
  (RE4 bodies that differ from the retail function, a marker of another function) whose retail address must not
  resolve to them: the address becomes a trap, or the port translation named `func_<addr8>`. The name still links
  for the decomp's own C callers. `docs/asm_worklist.md` lists the rows with what retail does with each.
  Census both ABIs from their own compiled objects and generated tables:
  `asm_worklist.py --gen build/gen --tables build/gen/p4pc_tables64.c --objects-file build/gen/objects64.txt`
  selects the x64 pair; omit `--tables` and use `objects.txt` for i386.
  Supply the same `--decomp`, `--retail` and repeatable retail `--coverage` inputs
  to both. The remote wrapper accepts the x64 pair through
  `P4PC_EXTRA="--tables /lane/build/gen/p4pc_tables64.c --objects-file /lane/build/gen/objects64.txt"`.
  Patch rows are parsed only inside their declared array and must agree with
  `p4pc_patch_count`; unknown schemas and missing/duplicate rows fail. The graph
  reads both ELF32 and ELF64 text relocations, including signed addends and weak
  or indirect-function bodies, instead of silently discarding x64 edges.
- **RenderWare globals sit at their retail addresses.** `ourGlobals` is at `0x008872E0`; the
  RW statics with no decodable retail address are port-owned (`config/port_owned.txt`).
- **Unit-local varargs and duplicate definitions** are removed in the mirror copy: `va_list`/
  `va_start` lines in units that do not use them, and a promoted definition already owned by a
  non-promoted unit (macro aliases honoured).
- **Battle script selector ABI.** The mirror narrows the script's `s64` query to
  the selector's `s32` parameter before its unprototyped calls. Without that
  conversion i386 passes two stack words and shifts the status, flags and
  predicate arguments, potentially calling a null predicate during battle.
  The selector ABI witness exercises all affected call expressions on i386.

Changes that are correct for both the PS2 and PC builds (for example a wrong
prototype) belong in the decomp, through its own matching checks, and arrive
here by bumping the submodule.

## Configure (`tools/gen_port.py`)

`tools/gen_port.py` configures the build: it compiles the units in
`config/units.txt` minus `config/excluded_units.txt` (each exclusion has a
reason). It also drops redundant prototypes (rule below). Undefined function
symbols with a retail address become trap stubs. Names prefixed `p4pc_` or `p4_`
are port-owned and never stubbed. Overrides whose unit is excluded are skipped
with a stderr note.

### Decomp text contracts and pin updates

`config/mirror_fixes.txt` requires exactly one literal match per row unless its optional
fifth tab-separated field gives another positive count. Missing and extra matches are errors.
An empty replacement deletes the matched text; the old text, reason and exact positive
cardinality remain required, so removing an obsolete local cannot silently become a no-op.
`config/mirror_contracts.json` additionally records each edited unit's full input SHA-256
and exact edit hunks (line counts and old/new text SHA-256) for every mirror pass.
The initial copy also fingerprints every raw C/header input, including headers no pass
edits: a changed inherited declaration or typedef cannot silently change an override's
pointer stride while leaving the repaired function text untouched.
The input fingerprint includes caller signatures and declarations: a parameter rename cannot
silently leave stale names in an otherwise unchanged call repair. Losing a site, acquiring
a site, or targeting different text fails the build before the staged mirror is published.
These are expectations, not a cache: builds and nightly jobs never
regenerate them. (Each unit's computed facts are cached by its before/after bytes and `tools/gen_port.py`; a changed pass, pin
or generator misses every unit it edits, and those line diffs run on every core, results taken in unit order.) Address claims must still have a decomp marker/function symbol, unbound
natives must still bind the specified address, named natives must be compiled functions, and every
`libc_rename.h` target must be a compiled function, not a same-named data object or a trap. Object-like
alias directives are parsed explicitly: duplicate names and nonsimple targets fail,
and directive whitespace or comments cannot silently remove a checked binding.
The tables also require the exact reviewed addressless SDK-call set in
`config/call_traps.txt` (67 names, identical in ELF32/64 and COFF32/64). A missing
private helper can no longer become a new zero-returning trap at address 0.
Adding a trap or losing a reviewed trap is a build error, not an automatic refresh.

The readability pin also spells `btlFormationHasEntry` and `btlCond_ANALYZE` as `u8`
instead of `bool`. They remain logical predicates used through full-register return slots:
the mirror explicitly widens both and requires one definition each. Shared matrix unions
are brace-scanned so their new word views cannot hide the required 16-byte alignment.
Effect-kind dispatch similarly follows the typed instance/kind expression, preserving all
eight formerly raw-offset argument repairs.
Anonymous typedef members such as `ResrcHeader;` use clang's `-fms-extensions` mode,
which reproduces MWCC's anonymous base-field promotion without changing their layout.
Forty-two offset assertions in twelve headers (including multiline bounds) use `__builtin_offsetof`, retaining
their `u32` value and comparisons instead of nonconstant pointer-to-integer casts on
64-bit targets. The checks stay active; runtime pointer expressions are not rewritten.
Unmarked local helpers use `mdlEffectColorOpacity`, `mdlEffectCameraMatrix`, and
`mdlEffectTrackIndex`; address markers alone do not inventory these dependencies.
Removed inline helpers (`p4_add_u32`, `novaAdd`, `novaMultiply`, `windAdd`, and
`particleProjectedFrameIndex`) are spelled directly in their port overrides, preserving
unsigned wraparound, f32 evaluation order, and the unsigned frame-word load.

The `d19f9e22` data-type audit covers all global object declarations/definitions and
block-scope externs: 114 symbols changed declared type, and all were individually
searched throughout the port. Sixteen have port references. The Flash/Wind allocator
overrides now read `.parameterBytes` from typed 0x1C-byte rows instead of multiplying
that stride twice. Twenty-four effect-table row/slot accesses explicitly cast the base
to `u8 *`; the battle-context curve offset does the same. The eight distortion UV
aliases already use explicit byte casts. Other referenced changes are declaration
aliases, fixed guest-address accesses, test strings, or documentation, not inherited
element-stride arithmetic. The changed/added/removed pointer-valued macros were also
checked; the font-cache assignment uses its underlying pointer storage.

The return-declaration audit additionally covers eleven marker-bound functions. Seven
changes only spell `int`/`float` as `s32`/`f32`; the two `bool` predicates are handled
above. `FldResource_CloneRotated` changes only its struct tag, with no port caller.
`chkmemInitRenderWareMemoryFunctions` gives its existing function-pointer return a
typedef name; the port-owned guest-address allocator table remains unchanged.

One upstream change intentionally improves host behavior while preserving matched PS2
instructions. `FldNpcControlUpdate` (`0018a200`) formerly called
`func_003e0870(handle, D_00756510, 2, initialHeading)`; its alias prototype placed the
integer combine mode before the floating-point angle. The new C calls
`RwMatrixRotate(matrix, axis, initialHeading, rwCOMBINEPOSTCONCAT)` (mode 2).
Both calls supply four arguments and use the same PS2 EABI GP/FP register assignment.
On i386 and Windows x64, the old alias crossed a positional native ABI without
adapting that class order, leaving newly placed NPCs with the wrong heading. The
canonical typed call fixes that host-only ABI mismatch; no retail argument was missing.
Linux i386 llvmpipe MADs consequently move from 2.8185% to 2.8212% for
`scene_save_butterfly_glow` and 2.2907% to 2.2906% for `scene_sky_black_tint`.
GDB traced the first difference to the NPC orientation matrix at game frame 195;
initializing locals with clang's `pattern` leaves both per-pin results unchanged.
Against the pre-renderer-change control, all other i386 scene facts, the full New Game
RNG trace and all 1319 golden replay verdicts/counts are identical. This is a
retail-justified ABI correction, not numerical tolerance or a changed reference image.

All 42 Linux x86_64 scene verdicts and recorded MAD/state/walk/RNG facts are identical
with the intended virtual clock. Apparent x64 MAD deltas were a fixture bug: the
`scenes64` label was attached after the shared clock-default loop, so those twins
ran on wall time. The late twin-registration loop now supplies the clock default,
without changing references or tolerances. The sky control additionally has an
identical full-history draw stream and byte-identical PNG at the two pins.
Both architectures retain all 322 New Game RNG rows (SHA256
`8f05a738109b8f69c63605552e09f9891ce367731b118313ba49ca6d8b7cd391`),
all 176 story-flag words and the camera/field/calendar/procedure words at the actual
New Game/story gates. All 1319 golden replay verdicts and numerical counts are unchanged;
the existing six i386 and five x64 `MISMATCH` verdicts remain, not newly introduced failures.

Replay must read a 64-bit integer result from the full EE `$v0`, not a
`$v0:$v1` pair. This applies both when injecting a native callee's result into
the interpreter and when extracting the interpreted result. Retail `__pack_d`
at `0x0044dd18` builds all 64 bits in `$v0`; `$v1` is unrelated scratch state.
Its legacy golden records omit `v0h`, so using their stale `$v1` as the upper
word creates a false mismatch. Such an incomplete record still checks its low
word and memory; absent a real mismatch it is explicitly inconclusive, never
an invented full-width match. Fresh oracles must record `v0h`.
`ovdiff --selftest-abi` checks register injection/extraction, and the
`mipsdiff_replay_oracle[64]` tests exercise complete, incomplete, wrong-upper,
wrong-lower, wrong-memory and caller-stack synthetic records through both the
native and interpreted-retail reader/comparator. Recorded caller-stack bytes
are inputs too: `__pack_d` can receive `a0 == entry $sp`, and snapshot
deduplication then leaves its unpacked value only in a `k:s` region. Restore
that input while keeping unlabelled outgoing stack words out of output
comparison. Neither test copies retail memory into the repository.
Random ovdiff scratch memory is untyped: any word can become a pointer when a
native callee follows a synthetic structure. Scratch words in the reserved
executable-image guest-address band are deterministically replaced with null,
so a random value cannot alias the port executable as its image size changes.
Known callback words are installed separately; recorded replay inputs and game
address translation are unchanged. `ovdiff_synthetic_words[64]` covers the
band boundaries and the observed image-alias value.

Global-window replay follows the two `[address,length]` descriptors in each
record, including the 4640-byte GP/narrow-RW snapshots and 8192-byte full-page
snapshots. The old fixed-size reader silently discarded every narrow snapshot;
for example PTank's plugin offset at `0x00764700` replayed as zero instead of
the recorded `0x94`. Malformed ranges, wrong byte counts, duplicate IDs and
missing referenced windows now fail loudly. The allocator slots remain pinned.
`remote_replay.sh` always restores the supplied global windows (`-G`).
`mipsdiff_windows[64]` exercises both real layouts, descriptor order,
explicit fragments, JSON whitespace and malformed-window failures against a
synthetic PTank lock, through native code and interpreted retail.

Interpreted replays use the recorded EE entry `$sp`, not the random-case
arena stack. `-E` interprets the selected body with native callees; `-E -F`
also interprets retail callees. Mixed GP/FP native calls require their actual
positional signature: `RwMatrixRotate` at `0x003e0870` is the callee-only
`ppfi` entry, not the fallback GP-first `ppif` order. The ABI self-test reads
that real registry entry and checks the GP/FP slots and recorded stack.

Before changing the gitlink, gate the exact upstream decomp revision (both retail SHA1s and
all first-party matches). Review every failed substitution/pass against the old pin, retarget
the same semantic fix, and migrate upstream identifier renames throughout the port.
Only after that review, explicitly regenerate the edit census with
`python tools/gen_port.py mirror --decomp decomp --build build/contracts --refresh-contracts`.
Review the contract diff along with the source changes; refreshing is not evidence of
behavioral equivalence. The pin update still needs before/after i386 and x86_64 game gates,
MAD/state-word/RNG/walk-digest comparisons, ovdiff, golden replay, and the full land check.
Measure state words from the actual registered gate commands with a read-only debugger
snapshot at their normal exit, preserving their input, card and capture environments.
A standalone launch with different observer/startup flags is not a gate-equivalent control.

The Challenge/Fusion integration combines the three Challenge leader overrides
(`001e6d20`, `001e1530`, `001e1800`) with the newer AI metadata-decode mirror
fixes and the inherited weapon-icon changes. The original Challenge contract
inputs below are historical, not the combined-source expectations. Their
complete files remain available at
`2a4420dfaee9bfef76b34602272c693cfa0d04d7:config/mirror_contracts.json` and
`8c626d726726925bd37d61eee110796722dd22c2:config/mirror_contracts.json`;
`a9851d7c981b0221064f99c3ddb1ed58d93f6379` preserves the intermediate HP-only
contract.

| Pass | Retained newer input | HP-only input | HP plus ailment input |
| --- | --- | --- | --- |
| `align_narrow_returns` | `c81098fa294bc1814f536a9ae6117a6d0c3cf098c191a760fc09c2c5bfa1fc64` | `b340decb747cec8f1dca13f2dcab10d999ecaf37d22b2420af9f1a0c186ecebf` | `83c4aee78aff53d1356e2fda07881d6455df1a1504e1fb1b497f63891b56422e` |
| `prototype_unprototyped_declarations` | `b7c1de1d115d11dc0de709c29ba28a8b9c1d4ec64a1e92ab9629ca160011c298` | `0acca30638bb0011245375be6abc377a410bbfdc5e0b0b5bd272420b4062d48d` | `37d3eaf7d37cae47ba3ea62c5ee503b7946e4e43a8dd5332513429b0644ee1f3` |
| `widen_narrow_returns` | `c5e9d84b47d473fb3fee029105caeec6a9b715741db5b9a0cb2e20749fc0899d` | `fa195e252f2ca3bb970daa36d4577ecb0065030b436381a14d54106c3cbb251c` | `e52f4f7d219e8ec6933b3ab13478187c704e05485bfb3266ec9255dd85218bcd` |

The combined candidate retains `833d5108`'s ten icon-related contract entries
and `10f58d46`'s four `btlAICommand.c` input-hash updates. The icon entries
cover `y_fclShopDraw.c`, `code1_0012.c` and
`unplaced_inferred_00111bc0.c`: three `apply_mirror_fixes` records change
hunks, while the other seven retain their hunks and change input hashes.
The four AI records retain their hunks and change only inputs. These are
distinct pass/unit records; the Challenge merge did not replace the icon
records. Prior pinned-decomp mirror runs were reported successful for the
icon and combined candidates, but an independent exact-tip contract review
remains pending; this integration did not regenerate or validate them.

For that review, use the pinned decomp
`18016afe60c6c73bdc5d0c72177a29c6f25878c6` and a unique scratch build:
`python tools/gen_port.py mirror --decomp <decomp> --build <scratch> --mod-hooks --refresh-contracts`.
Compare every affected input and hunk to the actual transformed source and
the preserved contracts before accepting any generated change; then run the
mirror without refresh. The focused AI and Fusion/Challenge regressions, all
required target censuses, final-tip milestone scopes and real-game hardware
witnesses remain separate checks. No qualification is implied here.

The post-pin argument audit starts with the actual preprocessed compilation units:
`python tools/audit_prototypes.py --gen build/gen --classes --abi build/gen/obj64/abi.json --json build/call-classes.json`.
It compares effective argument classes with compiled definitions, resolving generic
address names, native-name bindings, asm labels and immutable function-pointer aliases.
The source AST uses the i386 layout oracle, not an unrewritten 64-bit layout whose retail
size assertions fail. Existing small-record packing adapters are accounted for explicitly.
Opaque aggregate slots are resolved from the actual callee definition, across units and
retail-address aliases. Concrete byte sizes, compiler field offsets, eightbyte register
classes and homogeneous-float widths are compared; a record is never treated as a
single scalar GP argument. The layout reader retains anonymous typedef labels such
as `FclDrawColor` and named fields of anonymous nested records. Every call is retained
in the report; dynamic targets, unavailable definitions, genuinely unsupported layouts
and parse errors remain visible rather than being counted as successful comparisons.

Then run `python tools/audit_live_args.py --src --mirror build/gen --compiled-calls build/call-classes.json --all`.
The inventory must contain every call from exactly these active units with no parse errors.
Spliced overrides are included once; inactive `NON_MATCHING` branches and excluded alternate
C copies are not callers. Named/static marker functions and renamed native definitions are
included, and both callers and callees may use the full retail disassembly (`build/all.s`
or `P4_ALL_ASM`). Reports retain C source lines, retail call PCs and live register slots.
An aggregate result buffer in a0 is not a missing source argument. Raw EE square-root and
accumulator instructions must have their real register effects, not phantom incoming FPRs.

Prototype arity disagreements alone do not prove a missing host-live argument. The native
skin no-op does not consume retail's ignored identity-result input, and the RE4 SVM model
has no retail debug-name field. The random seeder's second formal, the file-open model's
host-only mode, and the unidentified kernel-call model's argument are explicitly unused.
Sound packets likewise have opcode-specific live payloads: sndcom control 8f/82/81
needs three words, 80/84 four, 83 five; handle release op 2 needs four and start ops 0/7
five. HSYNTH setup command 3 needs six words, but commands 1/2/5 need four; SESQ
registration command 0 needs four, but player-create command 5 needs three.
Prologue spills and unused packet tails are not extra arguments.
`effMiscProgressResetWork`'s fallback vertex-count formal is also not a caller input
for valid progress kinds 0/1/2: each kind replaces it with its derived count, just as
the preceding ribbon constructor does. Invalid kinds already leave the ribbon
counts uninitialized; inventing a default argument would not restore a retail value.
Inspect the retail site and actual host consumer; repair genuine omissions with exact mirror rows,
not ignored padding. In particular, `SFD_Init` must pass its existing `p1` refresh-rate
word to `SFTIM_Init`: retail loads `prm->prm1` into a1 in the call's delay slot, and the
callee stores it at work+8. Typed alias declarations and their calls are reordered
together while preserving order within each GP/FP class; whole aggregate arguments
remain intact. New alias coverage changes order only, not unrelated widths or returns.
Native hook declarations must match the restored mirror even when their only use is
storing a callback address. `seam_abi_mirror` checks those source-side declarations
against the compiled mirror ABI; a stale two-argument Dolby encoder hook must not
survive a four-argument mirror repair.

The aggregate audit also covers the EE's by-reference aggregate convention.
`smapWorldToColumn` (`002b11c0`) and `smapWorldToRow` (`002b1210`) load the position's
three float lanes through a0, but their decomp definitions take a by-value `RwV3d`.
Two `mapSymbolTaskUpdate` calls declare/pass a byte pointer, which happens to agree
on the EE but not on host stack/SSE aggregate conventions. Exact mirror rows give
both converters a `const RwV3d *` interface and migrate all 24 callers, including the
22 formerly by-value calls. The existing positions are passed by address without
new vector copies; the x/z-to-cell arithmetic is unchanged. Do not repair only the
two pointer callers while leaving conflicting converter interfaces elsewhere.

The same dump-parser repair removes ambiguous-record skips instead of changing
runtime rewrites. Full 1231-unit measurements add 15,647 layout assertions on Linux
and Windows clang 23 (53,435 to 69,082), and on Linux/AArch64 clang 19
(53,445 to 69,092); Android NDK clang 18 adds 15,321 (52,109 to 67,430).
These are 383 formerly ambiguous record observations across 299 native units,
or 369 across 293 Android units. Every new assertion compiles. The only
non-layout census change is the two existing SMap local positions now taken by
address (`addr-of-local` 26 to 28). Compiler patches and platform tables come from
their actual complete builds, not from another platform's counts.

The Windows x64 census includes the Analyze ailment-page overlay's three added
pointer reads in each of the action and panel units. In `datCalc.c`, the
skill-delivery action-frame routing adds one pointer read and one address-of-local
rewrite; the separate ailment-knowledge observation already accounted for its own
local address. These counts come from the affected unit's Windows rewrite sites,
not from a Linux baseline.

`tools/nightly_decomp.sh`, called by the nightly driver, builds both architectures against
the decomp's latest `origin/main` in a disposable port checkout with its own submodule configuration. Its local gitlink-only
commit is never pushed. The usual strict build diagnostics identify failed rows and passes;
the original pin, source checkout, and reviewed contracts are left unchanged.


A bare prototype is dropped when its name is already declared in a transitively included header or earlier in the same file. Three guards keep a line: a function-pointer declarator, a name starting with `__` (attributed declarations such as `s128 quat __attribute__((aligned(16)));`), and a line that does not start a statement or has unbalanced parentheses (a continuation line). `RWALIGN(name, 16)` declarators and `__asm__ volatile(...)` statements are never read as prototypes (they were once dropped as "redundant"). The first declaration wins.

The mirror also repairs the decomp's declaration mistakes that MWCC accepts and clang rejects (`tools/gen_port.py`, in this order):
- Prototypes that wrap their parameter list are joined onto one line and a trailing `/* ... */` comment is dropped, so the later rules see them.
- A prototype of a function the same file defines, whose parameter or return types differ from the definition's (struct tag and typedef are the same type), takes the definition's signature.
- A block-scope prototype whose name is called earlier in the file (implicitly declared `int f()` there) is hoisted to file scope, directly above the first function that uses the name. Types visible to that function are visible to the prototype.
- A `struct Tag` that first appears in a prototype's parameter list gets a forward declaration above it (the tag would otherwise be scoped to the prototype).
- Absolute data symbols (`D_xxxxxxxx`, `iGpxxxxxxxx`) declared with disagreeing types in different functions: the declarations that disagree with the file-scope one get a unique C name bound to the same symbol with an asm label (`__asm__("D_00887300")`), and their function's uses are renamed.
- EE `sync` barrier statements become compiler barriers.
- `config/mirror_fixes.txt` holds literal substitutions for what no rule can see: `<unit>TAB<old>TAB<new>TAB<reason>`. A call that omits an argument the callee reads from the caller's own register is the common case (the retail asm leaves a0..a3/f12 untouched across the call); each row cites the asm. A row whose old text has gone stops the mirror, so rows do not outlive the code they repair.

A override's `func_<addr>` prototypes (with or without `extern`) in every unit take the override's signature, so an override that is spliced into one unit and called from another stays consistent. An override used by units that lack the decomp's `u32`/`s32` typedefs (the re4 CRI units) uses plain C types in that signature.

An undefined `func_XXXXXXXX` whose address has a compiled function (found by marker or symbol name) becomes a `jmp` thunk to that function instead of a trap. This covers m2c-named calls such as `func_003e84a0` to `RwCameraCreate`. A marker whose return type sits on its own line (`RwCamera *`, then `RwCameraCreate(void)`) now resolves to its name.

A unit is un-excluded only after a fresh build passes (compile with `-c`, not `-fsyntax-only`: syntax-only misses invalid inline asm). Functions in a compiled unit that contain MWCC `asm` bodies, hardware access (DMAC/SPR copies) or VU0 asm are replaced by an `overrides/<addr>.c`; the remaining exclusions in `config/excluded_units.txt` are the PS2 sky2 driver, duplicates of an owning unit, and CRI units superseded by their re4 versions, each with its reason there.

`P4_Q128` (`include/p4pc/compat.h`) stores the decomp's `__int128` and
`mode(TI)` types as 16-byte structs. Arithmetic on them does not compile, which
marks the units that need overrides (`src/Event/Fcl/fclCombineMisc.c` today).

Undefined names that the decomp's symbol lists do not place need an address in `config/port_symbols.txt`. RenderWare extern storage that the port defines goes in `config/port_owned.txt` (one name per line; its comment gives the decomp's declaration). Configure checks that list against the decomp and fails on a name the decomp now defines or no longer declares. `gen_port.py tables` fails on any undefined name that is in neither list (listed in `build/unplaced_names.txt`); nothing is trapped as data.

Port decisions in the generated tables (`tools/gen_port.py`):
- A decomp `#define NAME func_XXXXXXXX` (for example `RwImageCopy`) becomes a jmp
  thunk to that function, native or trap. A `.set` alias would leave the name
  undefined when the target lives in another object.
- A name that an object calls is code. The decomp's symbol lists type some code as
  data (`btlPacketFindFirstByActionUID`, 0x00193bf0 in `symbols_recovered.txt`), so
  a call relocation overrides a `data` tag and the name becomes a trap.
- RenderWare freelist and module globals are port-owned storage listed in `config/port_owned.txt` and defined in their unit. No retail address is known for them and only RenderWare code uses them; the file records the evidence.
- The RenderWare globals block (`ourGlobals`, which `RwEngineInstance` names) is retail data at its retail address `0x008872E0`. The decomp's symbol lists already place it, and `config/port_symbols.txt` must not repeat it (configure's clash check). `include/p4pc/compat.h` defines `RWGLOBALSIZE` for every unit: `baresour.h` includes `balibtyp.h` before `batypes.h` defines it, so some units otherwise saw the pointer form of `RwEngineInstance` and referenced an undefined symbol. `port_source` (`tools/gen_port.py`) turns the `RWGLOBALSIZE` definition in `plcore/baplcore.c` and `badevice.c` into an extern, so RenderWare and the game share one copy; the allocator slots at `0x008873E8` are fields of this block.
- `_rwBuildDateTime` is port-owned with the text `"Persona4-PC"`, not the retail
  banner.

### CPU-only SRD/CVFS adapters

These entry points implement the retail CPU contract rather than leaving an
asm trap or trusting an incomplete `NON_MATCHING` declaration:

| Entry | Contract |
|---|---|
| `004f13c0` | Forwards the handle, sector position and origin `0..2` to `cvFsSeek`; invalid origins take the real RFTGT error path. Both results are preserved. |
| `004f1460` | Passes the live handle to `cvFsGetStat`; states `0..2` pass through and all other states become `3`. |
| `004e3448` | Reads the CVFS option slot `+0x60`, requests high word `200` then low word `201`, and combines their low 32 bits into the full 64-bit count. Reloads the interface and handle between callbacks. |
| `004f14e0` | Returns a nonzero **64-bit** extended count, including a high-only value such as `0x100000000`; otherwise sign-extends the legacy `004f14c8` count. |
| `004f14c8` | The existing mirror repair preserves both the live handle and `cvFsGetNumTr`'s signed result; the three original semantic-fix rows remain, with corrected return types and `return`. |
| `004f53c0` | Returns/stores the error code, or `-203` when uninitialised. The optional detail is a string pointer, the line is sign-extended from 16 bits, and `RofsErrFunc` is `void(void *, char *, int32_t)`. |
| `004f9a58` | Reuses the existing `SVM_Unlock` native through `native_names`; the retail body is only its no-argument tail. |
| `004f9a70` | Reuses the existing `004e4190` SRD request pump through the same explicit address-binding mechanism. |

`cri_cpu_adapters` calls the actual generated native-table entries, including
the canonical aliases and the generated legacy tail, and compares them with
the private retail instructions in the R5900 interpreter. It checks the
registered GP/pointer/FP classes too; void and integer results share the
ordinary register class, as in the runtime adapter. Full-width integer
results are still compared bit for bit.

Only the named external CVFS delegates, CVFS error dispatcher, SVM primitive
and request-pump seams are wrapped. Application/option callbacks record their
arguments and read their context, including a callback which changes both the
CVFS interface and handle between the high/low requests. The extended-query
body, error formatter, generated tail and alias bindings are not substituted.
The interpreter delegates retail `sprintf` to the host formatter using the
retail format pointer and decoded argument sequence, not a replacement error
message or expected-output string.
The ELF supplies all instructions, format strings and other retail data; the
test commits none of those bytes.

The 64-bit rewriter adapts both formatter calls to `p4pf_sprintf`, with integer
width masks `0x11` (with detail) and `0x9` (without detail): only the error code
and signed line are integer arguments; message, detail and file remain host
pointers. The application callback likewise retains two host pointers and an
integer code. Both 64-bit censuses explicitly record the two call adaptations
and two printf-mask rewrites in `src/sce/rofs_mai.c`.

ADXF history address `004c7260` must resolve to `func_004c7260`, not the RE4
`adxf_SetCmdHstry`: the SDK body uses private host-static history and counters,
whereas retail writes the guest ring at `007165f8`, counters at `007166f8` and
index at `00716718`. `native_names.txt` pins this address to that port provider,
without redefining the SDK name or requiring its object to remain compiled.
The generated-table witness compares the complete guest state and surrounding
guards against actual retail instructions, including index wrapping and
16-bit counter overflow.

Partition teardown `004c7e28` has no return value. Retail leaves `v0` holding
the scratch store base `00710000`, not a status result; the port's update
paths call cleanup and then read the load-state word. Its C declaration and
oracle signature are therefore both `void`. Replay still compares the handle,
count and chunk stores and the real ADXF close effects: it must not mistake
an unused scratch register for a return-value mismatch.

Readiness address `00513380` reuses the existing ADXT-only
`sfadxt_IsDecoded` getter. One exact mirror row exports it with a full 32-bit
boolean return, and `native_names.txt` pins the compiled wrapper. This avoids
enabling an entire promoted unit or a global `NON_MATCHING` switch. It calls
`ADXT_GetStat` once and tests neither `0` nor `1`, equivalent to retail's
**unsigned** status `>= 2`, including negative statuses and the NULL-handle
error. The generated-entry witness covers every status byte at two ADXT
addresses and NULL, executes the real retail status getter/empty locks, and
checks that the input objects remain unchanged. No RE4 SFD structure layout
is used.

The SFD silence callback `00512d90` instead uses the retail raw layout in
`src/cri/sfd_port.c`: the 64-bit audio-byte counter is at `+0x9c8`, the
driver work pointer at `+0x2004`, and SEE work at `+0x36b0`. Its mux-version,
audio-header and audio-info helpers (`00517840`, `00513dc8`, `00512ea8`)
read those same offsets. For old muxes the strict `offset < len - 18`
scan leaves the final record; newer muxes transition without consuming input.
The next callback has the actual `void(pointer,pointer,int,pointer)` ABI.
The CPU witness compares the entire guarded guest fixture, including full
counter carry/wrap, signed seek flags, sample-offset wrap and output aliases;
only the explicit next-callback code-pointer correspondence is normalized.

The remaining callback chain is raw-layout too: header exclusion `00512bb0`,
audio alignment `005128a0`, and terminal copy `005127e8`, with the actual
SJ chunk wrappers and three-argument ADXT silence-insertion delegate.
`cri_sfd_callback_test` follows the stored callbacks through all four stages
using the production function-pointer bridge, and checks every guest byte,
external-call argument, timer diagnostic store and consumed-byte count.
Canonical SDK timer helpers are reused on the verified relative timer layout;
four exact mirror rows put their diagnostic variables at guest `745b10`
through `745b20`, preserving the conditional stores rather than leaving
private host BSS copies. Canonical `SFSET_GetCond` receives a host pointer.
The error pair `00517ad0`/`00517b40` is raw: the i386 SDK's canonical
`SFLIB_SetErr` writes a private library-wide error instead of guest `745a5c`.
The raw pair keeps the first stored code, passes the current code and live
context to the callback, and only then negates a positive handle state.
All CPU/metadata/callback fixtures enter an explicit
`p4t_on_guest_stack` wrapper on 64-bit hosts; they do not depend on a
CMake-generated main rename or on truncating addresses of host-stack locals.

### PC ROFS metadata boundary

`rofs_host.c` supplies the PC device's option entry `004f8848` directly.
Option100 returns1 to request the `DEV:path` name form; volume options
return0 without constructing PS2 mount metadata or modifying a packet,
output buffer or last-error word. There is no alternate dispatcher or
mode-dependent mount-name allocation.

The registration entry `004c6eb0` takes no argument; the native header, boot
mirror and audio/file fixtures call that one signature directly. Native-name
aliases for the deleted model-only ReadSj/read32/seek/progress bodies are
retired, rather than retained as compatibility forwarders. The SDK server,
audio joints and file-window entries still used by the PC path remain bound.

`cri_rofs_metadata` checks the actual generated entry and its pointer/integer
register signature, including unchanged packet/output/error bytes for present,
missing and null name/output combinations. The disc's file-name lookup and
deferred read completion are exercised separately by `rofs_open`; neither
metadata queries nor status polling may bypass the next-frame read fence.

## Build pitfalls

Build pitfalls, fixed here and worth knowing for any new file-layer or decomp code:
- `_FILE_OFFSET_BITS=64` is set globally in `CMakeLists.txt`, not per file. The forced `-include p4pc/compat.h` pulls in `stdint.h`, which fixes `off_t` at 32 bits, so a per-file define arrives too late. Without it, CD offsets above 2 GiB (`ZZZZZ.BIN` sits at 3607136256) truncate in `fseeko`.
- Port code is built without `-malign-double`, so glibc's `struct stat` matches the port's layout, and `host_size` reads `st_size` directly. The flag is set only on the decomp mirror (`p4decomp`), which reproduces MWCC's layout. A port struct that mirrors a game struct with a 64-bit member uses `P4_PS2_ALIGN64` (`include/p4pc/align.h`, force-included by `compat.h`) and asserts its offsets with `_Static_assert`, as `src/platform/ps2_fpbit.h` does.
- Game-facing libc names use the canonical retail/guest-heap boundary (`include/p4pc/libc_rename.h` for generated units; explicit canonical calls in native CRI/game-facing code). Host libc is restricted to reviewed byte operations and per-source host-symbol/calling-function adapter contracts, with targeted guest-result flow checks. CMake's fail-closed object guard covers the generated mirror, native callers and executable alias tables; see the [CRT binding and caller-inventory contract](platform/crt_boundary.md). A metadata name or unrelated header edit is not an approval.
- `qsort` also binds to retail `00440bb8`, not the host CRT. Equal-key permutations are observable: `resrcMngRenderTask` sorts resources before their effects consume the shared RNG. In `castle_training_042164`, identical six-resource inputs first produced different Windows/glibc orders at frame 10705, followed by different particle counts, then a battle-camera RNG draw at 11893. `qsort_retail_order` checks retail's fixed eight-record equal-key permutation on aligned and unaligned storage, including its Linux/Windows host-test twins.
- The separate keyed registry sort (`004156e0`) has a different input contract from general `qsort`. Retail has exactly one direct caller (`00414348`, in `004142b0`); its cell identifiers are unique because `00413350` finds/reuses an existing ID rather than appending another. Its unsigned-key comparator therefore has one possible final order on the game's inputs. The native helper uses the retail `00440bb8` binding as well, leaving no host CRT sort in this path; this does not claim the registry's radix implementation has retail qsort's equal-key permutation on arbitrary records.
- Do not infer a retail conversion's width from the host's `long` (or from the EE's 32-bit pointers). Retail `00444458` accumulates an **unsigned 64-bit** value; `00444210` is its **signed 64-bit** counterpart, and `atoi` (`0043c6b0`) keeps and sign-interprets the low 32 bits of that signed result. `src/platform/ps2_libc.c` implements those widths explicitly, including the retail ASCII/base-prefix scan, saturated overflow, `ERANGE` value 34 in the guest reentrancy block, and **four-byte guest end-pointer slots**. The CRI `004e9e98` tail wrapper preserves all 64 result bits; address/length consumers explicitly narrow at their own 32-bit stores. `ps2_libc` covers the 32/64-bit boundaries, scan endpoints and adjacent-slot canaries.
- `strcmp`, `strncmp` and `memcmp` bind to `004426e8`, `00442c30` and `0043f778`: they return the exact first differing **unsigned-byte subtraction**, not merely a host-dependent negative/positive value. The caller audit found 81 comparisons: 66 test equality with zero, 12 inequality with zero, and three nonpositive ordering tests; none depends on a nonzero magnitude. The CRT fixture nevertheless checks exact magnitude as well as sign, high-bit bytes, bounded comparisons and zero-length reads.
- The two retail `sscanf` calls are the CRI `SFXZ_MakeCnvZTbl` hexadecimal scans (`0052c1f8`, `0052c25c`). Their EE `%lx` destinations occupy eight-byte stack slots; later Z values use only the low words. Mirror fixes use `Uint64` destinations and `%llx` on both host ABIs, then explicitly narrow at the original 32-bit uses. This avoids both LP64 overwrites of adjacent `Uint32` locals and LLP64 truncation during the scan; do not change these to host-width `%lx`.
- Soft-double **argument class** matters independently of formatting digits. The old 64-bit `p4pf_*` path read masked integer-class double bits from stale FPR save slots on SysV: an unrelated preceding `-999.0` print replaced every supplied value. The shared retail formatter now reads masked values directly as 64-bit EE payloads, and unmasked native doubles by bit-copy. `ps2_libc` retains poisoned-FPR and mixed guest-string/integer/native-double tests. The old eighteen-case Linux host comparison was not a portable formatter proof: shipping UCRT rounding produced `1.37` for `%4.2f`/1.375 where retail produces `1.38`. No host digit generation remains in the replacement engine.

### Game CRT format inventory

The AST inventory covers 1,231 selected decomp units (37 excluded); raw-source counts also include inactive preprocessor branches. It finds 291 `sprintf`, two `sscanf`, 376 fixed-address console `printf`, nine `vsprintf` forwards (four named and five fixed-address), one `vprintf` forward, and five CRI printf-adapter calls. The callers of the ten format-forwarding wrappers were also inspected. The retail ELF itself contains 375 direct console `printf`, 297 `sprintf` and two `sscanf` calls; these are instruction counts, not source-token counts.

| Conversion | Statically resolved game forms |
| --- | --- |
| Strings/characters | `%s`, `%c` |
| Signed decimal | `%d`, `%2d`, `%3d`, `%8d`, `%02d`, `%03d`, `%04d`, `%05d` |
| Hexadecimal | `%x`, `%X`, `%02x`, `%03x`, `%03X`, `%06X`, `%08x`, `%08X` |
| Floating output | `%f`, `%4.2f`; the space/underscore prefixes are literal text. No `%e`, `%g`, `%a` or long-double output was found. |
| Hexadecimal input | Original `%lx` and `%lx %lx %lx`; mirrored as `%llx` and `%llx %llx %llx` with matching 64-bit destinations. |

The remaining named error formats are plain text, `%s %d`, or `%08x.%08x`; message-buffer forwards receive already-constructed error text. The compiled but disabled SFTST diagnostic buffer has a separate `%p`/`%ld`/`%08lX` format. `SFTST_SetDebugOut` has no game caller; that diagnostic-only pointer/long representation is **not** part of the active-format equivalence claim. Shipping-disabled RenderWare monitor branches are likewise not evidence of an active game float format.

No selected game-source call to host `bsearch`, `rand`/`srand`, `strtod`/`strtof`/`atof`, `time`, `localtime`, `gmtime`, `mktime`, `clock`, `strftime`, `difftime`, `asctime` or `ctime` was found. Host/tool configuration and OS diagnostics are outside the forced game-libc binding map.

### Retail decimal conversion

`src/platform/ee_dtoa.c` translates `_dtoa_r` at `0043ca70`; `ee_decimal.c`
contains its bigint closure. The source reference is Gay's implementation in
[newlib-1_9_0](https://github.com/mirror/newlib-cygwin/tree/newlib-1_9_0/newlib/libc/stdlib),
with the IEEE little-endian, packed-32-bit-limb branches compared against the
retail instructions. This identifies the source variant, not an independently
proven embedded newlib release string. Bigint links and the reentrancy block's
result/free-list/powers-of-five pointers retain their four-byte guest layout.
The result uses the existing game heap, not a host formatting library.
Dtoa and strtod share the original guest power tables at `00754f38`,
`00755000` and `00755028`. The ELF-free binding fixture supplies only their
mathematical values; the instruction differentials read the original image.

All rounded arithmetic and floating comparisons use the retail soft-double
helpers, including the initial zero comparison: double subnormals compare equal
to zero on the EE. A generic IEEE Gay dtoa would instead print nonzero digits.
`ee_decimal_retail_oracle` executes the original ELF instructions, including
malloc/sbrk/IRQ brackets, without native algorithm hooks. Its generated cases
compare digits, decimal position, sign and end-pointer length in ordinary and
shipping EE FPU modes. The optional `R5900KernelEnv` supplies only CP0 Status,
EI/DI and the EndOfHeap syscall; absent that record, the interpreter remains
strict. Printf uses dtoa modes 2/3; shortest modes 0/1 are also tested. The
original debug mode 5 at DBL_MIN/precision -8 reaches the interpreter step limit,
so the byte-output corpus does not claim termination for optional modes 4..9.

The same fixture also checks the signed/unsigned 64-bit conversion closure:
`__floatdidf` (`0044ba00`), `__fixunsdfdi` (`0044bb60`), `__fixdfdi`
(`0044bc68`) and the unsigned-word helper (`0044e830`). Their canonical
raw-bit APIs live in `ps2_fpbit.c/h`. The EE's split-word conversion and
wrapping out-of-range behavior are retained, not replaced with host casts;
signed integer rounding boundaries, infinities, NaNs and raw patterns run
against the original instructions in both FPU modes.


### Retail output formatting

`ee_printf.c` shares one conversion engine between the ordinary libc bindings
and `p4pf_*` guest-mask entry points. It follows the retail `00444870`/`00445dd0`/
`00445f78` control flow and the corresponding **newlib-1_10_0** source variant:
in particular, fixed-point fractional output tests `expt || ndig`, and alternate
octal zero with zero precision still emits `0`. The older 1.9 formatter lacks
those two branches. This is source-variant identification, not a claim to have
found a release string in the ELF.

Signed/unsigned integer, string, character, pointer and `%n` conversions, flags,
widths, precisions and e/E/f/g/G output are implemented without host formatting.
Float digits use the raw-bit retail dtoa above; no host floating arithmetic
chooses digits or rounding. Console/file transport writes bytes with `fwrite`;
strings and padding do not allocate temporary output buffers. Guest-mask
wrappers live beside this engine, not in `guest.c`, so unrelated memory-model
fixtures do not acquire formatter/allocator dependencies.

Masked arguments have one ABI on every profile: the rewriter casts each
integer-class variadic expression to `unsigned long long`, preserving signed32
sign extension and unsigned32 zero extension. Each set mask bit then consumes
one 64-bit GP word, including `%d`, `%c`, `*` width/precision and guest pointers
on i386. The conversion's retail width is applied after reading that word;
`%ld`/`%lld` remain 64-bit, never host `long`. Unmasked native pointers and
real doubles retain their native C varargs classes and widths.

The full x64 `adapt_call` and the narrow i386
`tools/port64/printf_rewrite.py` share this normalization operation. The latter
only adapts printf-family calls, not layouts or pointer storage:
`--root MIRROR --out UNIT.printf.i [--depfile UNIT.d] UNIT.c -- FLAGS`.
For i386 COFF, also pass `--symbol-prefix _`.
`ee_printf_abi_oracle_test.c` links the actually rewritten typed calls in
`tests/data/ee_printf_abi_calls.c`; its Sint32 `%ld` operands are not
pre-widened oracle slots. Signed/unsigned boundaries, native and guest pointers,
native and soft doubles, starred fields and characters are compared against
unchanged retail `sprintf`, with whole-buffer canaries and return counts.
This rewritten-caller witness passed 5,888 byte-identical cases per profile on
Linux x86-64, Linux i386 and native Windows x64 (shipping UCRT), including both
ordinary and EE FPU modes. The AST adapter suite additionally checks all four
families, alias spellings, i386 COFF labels and idempotent narrow rewriting.

`ee_printf_retail_oracle` compares whole canary-filled output buffers, return
counts and `%n` destination canaries against unchanged retail `sprintf`.
`tools/ee_printf_corpus.py` derives its 639 inventoried forms from the CRT AST
inventory: 612 are runtime addresses into the user's ELF; 27 are source-only
test inputs not present in that ELF. No extracted retail strings are committed.
Synthetic cases additionally exercise flag combinations, variable width and
precision, mixed pointers/integers/doubles, negative zero, subnormals, ties,
extreme finite values, infinity and NaNs. Both ordinary and shipping EE FPU
modes are included.

Standalone source-closure qualification produced the same byte-identical results
on Linux x86-64 and native Windows x86-64 using the shipping UCRT toolchain:

| Original-ELF differential | Cases per OS, including both FPU modes |
| --- | ---: |
| Dtoa modes 0..3 | 22,960 |
| Signed/unsigned conversion closure | 2,552 |
| Formatter, including all 639 inventoried forms | 50,924 |

The named printf-family and masked-varargs fixture also passed on both systems.
These algorithm witnesses do not replace normal integrated port qualification.



## EE float mode

The EE's float unit (and VU0) rounds toward zero (PCSX2 "Chop") and flushes denormals; it has no Inf/NaN (results clamp to +-Fmax). The PC build compiles
all game code with `-mfpmath=sse -msse2 -ffp-contract=off`, so single-precision operations are SSE operations and obey MXCSR. `p4pc_kernel_pre_init`
(`src/crt.c`) calls `p4pc_fpu_ee_mode` (`include/p4pc/hostmath.h`): MXCSR round-toward-zero, flush-to-zero, denormals-are-zero on the game thread
(`P4PC_EE_ROUNDING=near` turns MXCSR off for A/B runs only: the VU0 helpers and the boss fight's 180-degree quaternion depend on the truncation, so the Yosuke battle gate goes from 15.5% to 28.7% with it). The coroutine kernel threads share the OS thread and are created after the call, so every context holds the mode. Threads created later (SDL audio, Mesa's llvmpipe workers) inherit it.

- **Do not hand-truncate.** A plain `a * b` truncates already (the old per-function emulations of RtQuatRotate's half angle and the ACS fader dB multiply
  and divide are gone). The VU0 helpers (`include/p4pc/vu0.h`) compute in double and narrow, which truncates the exact result once.
- **Add and subtract are not truncated exact sums.** The EE adder aligns the smaller operand by truncation with one guard bit and truncates the result
  (`p4pc_ee_add_bits`, `include/p4pc/eefloat.h`): `func_003dc490` (RtQuatConvertFromMatrix, x-dominant branch) matches 12 of 12 golden calls, against 3 of
  12 with a truncated exact sum and 10 of 12 with round-to-nearest. **Division and square root are nearest** (`func_003d6d50`: 12 of 12 with a nearest
  division, 7 of 12 truncating). Neither can be had from MXCSR, so the decomp mirror is compiled through `tools/ee_float_cc.py` (CMake option `P4_EE_FLOAT`,
  list `P4_EE_FLOAT_OPS`): each unit goes to unoptimised LLVM IR, every float `fadd`/`fsub` becomes a call to `p4pc_ee_add`/`p4pc_ee_sub`
  (`src/platform/ee_float.c`), every float `fdiv` and every `sqrtf` / `llvm.sqrt.f32` the x87 extended operation narrowed to float (the x87 unit is not under
  MXCSR control; `p4pc_fpu_ee_mode` sets its control word to 64-bit precision, nearest). Overrides are decomp units and get it too. Port code that ports game
  code by hand calls `p4pc_ee_add/sub/div/sqrt` itself (`rw_normalize2.c`, `rw_normalize3.c`). `P4_EE_FLOAT_REPORT=<file>` collects "unit function div sqrt
  add" per function and `tools/ee_float_sites.py` compares it with the retail div.s/sqrt.s counts (on the build tree of this commit 92 of the 106 mirror
  functions with a rewritten div/sqrt match the retail counts; the others are overrides written differently from the retail code, or sqrt.s that objdump
  prints as a coprocessor instruction).
- **mul.s truncates and madd.s/msub.s are not fused.** The truncation comes from MXCSR. Fusing `c + a * b` into one truncation (the exact product, as the
  EE's accumulator suggests) was measured and loses golden exactness (six functions lose 35 cases, one gains 1; `func_00415bc0` 9/12 -> 0/12), so a
  plain C multiply and add, each step through the machinery above, is right. The VU0 helpers (`include/p4pc/vu0.h`) keep the truncated exact sum: the
  guard-bit adder matches slightly fewer golden calls for VU0 code (2600 against 2605 of 3355).
- **Sum in retail's accumulator order.** The guard-bit adder is not associative, so a hand-written override adds the terms of a `mula.s`/`madda.s`/`madd.s`
  chain in the chain's order. `func_003dcc70` (slerp setup) summed its dot product w, z, x, y where retail accumulates y, x, z, w: one ulp of a dot near 1
  became 161 ulps of the acos angle, and the battle opening camera's third curve key came out 1-2 ulp off retail (collision 900, `docs/lockstep.md`). The
  `battle_camera_curve` test checks the four keys bit for bit from the production mirror and override; the golden replay of `003dcc70` went from 6 exact,
  2 within tolerance and 4 mismatching recorded calls to 12 of 12 exact. The RtAnim keyframe callbacks had the same reversed chains: the slerp dot of
  `00415bc0` (interpolate) and `004165a0` (blend), the squared length and every product sum of `00416e10` (multiply by reciprocal) and the scalar part of
  `00417310` (add). `anim_accumulator_order` runs these five overrides against the retail instructions in the R5900 interpreter on 4096 cases each; before
  the fix 215, 418, 282 and 954 cases differed. The golden replay of `004165a0` went from 314 exact and 10 within tolerance to 324 of 324 exact
  (`00416e10` and `00417310` have no recorded calls). The other float-math overrides that `tools/mipsdiff/replay_expected.txt` excuses write the same
  bytes as the retail body in the paired replay (`ovdiff --paired`: both from the same recorded snapshot) wherever both run, apart from `00393900`'s
  documented stale return register, so their remaining recorded-call differences are recording or harness state, not arithmetic. The three
  float-semantics excuses (`00415ad0`, `0048d8c0`, `003e05f0`) no longer match any call of the 27 recorded sets and are removed.
- **Clamp: left alone.** EE results saturate to +-Fmax and float-to-int conversions saturate; the interpreter counts both over the whole golden replay
  (about 3600 recorded calls): 24 clamped results and no saturating conversion. A port-wide treatment would cost a compare and select on every operation for
  24 events; the functions that do clamp (random cases in ovdiff) stay in the "special" class.
- **The libm cores are the retail code.** sinf, cosf, tanf, atanf, acosf, asinf, atan2f, logf, powf, their kernels (`__kernel_sin/cos/tanf`,
  `__ieee754_rem_pio2f`, `__kernel_rem_pio2f`), sqrtf (a bitwise integer routine, not `sqrt.s`), floorf, fabsf, scalbnf and copysignf are
  translated from the retail asm by `tools/mips2c.py` into `src/platform/ee_libm.c` (spec: `tools/mips2c/libm.spec`; the output is generated, do not edit): one C
  function per retail function, instruction for instruction, registers as locals, branches as gotos with their delay slots, the constants and tables copied from the
  image as static arrays, float operations as plain C that the launcher gives the EE's arithmetic. They are exact against the interpreter in every random case
  (the only exception, `__kernel_rem_pio2f`, differs on garbage `nx` values that overrun the local frame) and against the golden calls (sinf, cosf, acosf, atan2f,
  atanf 12 of 12 each; the host-libm bindings they replace matched 12, 9, 3, 12 and 8).
  **Double libm is retail too.** `acos` (`004470f0`), `log` (`004476b0`),
  `log10` (`00447b88`), `pow` (`00447d00`) and `sqrt` (`00448a00`) live in
  `src/platform/ee_dlibm.c`. `tools/mips2c_double.py` translates all 1,924
  instructions of these routines and their sign/scaling callees, preserving
  branches, delay slots, word sign extension and exact coefficient bits.
  Every floating operation calls the integer fp-bit software-double helpers in
  `ps2_fpbit.c`; no host libm or native-double arithmetic appears in the cores.
  `ee_dfloat.c` adapts native double arguments/results by copying their bits only.
  This replaces the former host-libm approximation: sampled agreement after
  truncation to float was not proof of double equivalence, and depended on the host.
  `ee_dlibm_retail` runs the unchanged ELF instructions with no hooked arithmetic
  against the native cores, including NaN payloads/signs, signed zero, denormals,
  overflow/underflow, integer boundaries, every pair of 44 edge values and a
  deterministic raw-bit spread and dense ordinary-input cases. It also checks the
  native-double register adapters, in default and shipping EE FPU modes.
  Native Linux and Windows UCRT: 22,584 byte-exact comparisons, zero failures, digest
  `b93c20db0248d73f`. The ordinary i386/x64/Win64 fixture factories register
  the same test, using `P4PC_ELF` for the retail executable.
  The surrounding decomp double arithmetic is unchanged: its native add/subtract
  can still differ from retail's guard-bit behaviour; the translated cores never
  substitute it. `ADX_GetCoefficient` (`cri_glue.c`) takes retail's cosf
  (`func_0044b610`; 18 of 20000 pairs differed with the host cosf) and retail's
  constant 0x40c90fda for 2 pi (the source's 6.2831855f is 0x40c90fdb:
  1 pair in 20000 differed): 0 of 100000 differ now.
  **RenderWare's double `sin`/`cos`/`atan2`** (`RwSin`, `RwCos`, `RwATan2` expand to the double functions in the mirror build) are renamed by `libc_rename.h` to
  `p4pc_rw_sin/cos/atan2` (`ps2_libc.c`), which call the retail float cores: retail has no double versions (its `RwMatrixRotate` calls sinf 0x44b7b0 and cosf 0x44b610).
  glibc's double `cos(98 deg)` under round-toward-zero returned a NaN, the EE add read it as FLT_MAX, `RwMatrixRotate` built a matrix with -FLT_MAX on its diagonal, and the
  look-at direction (0.99, 0, -FLT_MAX) it produced made the retail acosf return +FLT_MAX and `mdlUpdateHierarchyPose` wrap the pitch forever (new-game walk, flip ~29000).
  `nm -u build/p4pc` lists what still binds to glibc.
  **Class guard.** `tools/gen_port.py` fails the build when a decomp unit (mirror or override) calls a host library function that `config/host_symbols_allow.txt` does
  not list; each entry carries a reason. Memory, string and parsing seams and compiler runtime are listed; the maths entries (`log`, `pow`, `sqrt`, `sqrtf`) are marked TEMPORARY
  and the aim is zero of them (PortGLES binds the double cores). The list the build found is printed as `gen_port: host library functions called by decomp units`.
  `fabsf` is renamed to the retail `func_0044b6c0` in `libc_rename.h`.
  **Soft-double helpers.** `fptodp`, `dptofp`, `dpsub`, `func_0044e168`, `func_0044b310` and the double libm cores (`func_004470f0`, `func_00447b88`, `func_00447d00`,
  `func_0044b8d8`...) take and return the 64-bit double bit pattern; the decomp declares them per unit with a 32-bit or `f64` type, which on i386 read half a result and
  passed half an argument. `tools/gen_port.py` (`pass_soft_double_types`) gives every declaration the port's signature, widens the locals that receive a 64-bit
  result, and dies if one is still narrower; the hand rows of `config/mirror_fixes.txt` are gone. The class made every moving scene resource (event walkers, crowd NPCs,
  the car) face +Z: `MT_Scene_GetDirectionYaw`'s acos read half a double. **RenderWare's sqrt macros** (`rwSqrtMacro`, `rwInvSqrtMacro`, `rwSqrtInvSqrtMacro`) are the EE's
  `sqrt.s` in `libc_rename.h`, as in retail (`RwV3dNormalize` 0x003e40b0); the generic table-driven default left "unit" vectors 1.00007 long, which sent the heading's dot
  product past 1 and the double acos to NaN (a FLT_MAX rotation, an endless `MT_Scene_WrapAngles`). Gate: `P4PC_FACING_GATE=1` (`src/platform/newgame_gate.c`, on in
  `newgame_gate.sh` and `story_gate.sh`): a scene model that moved must have its matrix `at` along the move (state words), and the Inaba greeting's three poses must
  match retail (PortTrace3's ng-handsome: Yu faces -Z toward Dojima and Nanako).
  Port code that wants libm in nearest keeps the macros of `hostmath.h`.
- **Rewritten-operation counts versus the retail asm** (`tools/ee_float_sites.py`, raw instruction words, so `sqrt.s` is counted too): 107 of the 115 mirror
  functions with a rewritten div/sqrt match exactly. The other eight: `003955a0` and `00395bd0` call the retail acosf (`func_00448d10`) where retail inlines a copy
  of it (the operations are the same, counted in the callee); `003dcc70` called host acosf/sinf where retail inlines its own polynomials, now `p4_anim_acos` and
  `p4_anim_sin` (`rtanim_half.h`) like its siblings: ovdiff MISMATCH -> exact in 96 of 100 cases and the rest proven; `00390680`, `00392bf0`, `003b90b0`,
  `003b9a40` and `003d9760` are C loops over what retail unrolls or shares (ovdiff exact in every valid case, the retail division and square-root sites
  exist once per loop in the C).
- **Hand-written natives go through the launcher too.** `p4portee` (CMake) holds the port natives that translate retail float code (CRI aixp/acs/glue, rw_skin_callees,
  rw_rtanim, rw_rtanim_record, rw_normalize2/3, ee_libm): same options as `p4port` plus the `tools/ee_float_cc.py` launcher. `ee_float.c` itself stays in `p4port`
  (a rewritten helper would call itself) and so do the host seams.
- **Mesa llvmpipe runs in the EE mode on purpose.** Its worker threads inherit MXCSR; with them (and the draw calls) in nearest the scene gates' MAD against
  the retail captures rises (title 1.14 -> 1.20, field 1.78 -> 2.00, near-fog and sky-black-tint worse), because the GS truncates colours and
  coordinates where llvmpipe rounds. A real GPU driver is unaffected (no MXCSR dependence in the rasteriser).
- **Not covered by MXCSR:** the EE's clamp to +-Fmax instead of Inf, no NaN, float-to-int saturation, and the fused `madd` above.

Measured when the mode went in (MAD against the retail captures; the same binary, `P4PC_EE_ROUNDING=near` against the default):

| scene | nearest | EE mode |
|---|---|---|
| title | 1.2131 | 1.1367 |
| Dojima (field_mad) | 1.7187 | 1.7773 |
| sky-black-tint | 3.0618 | 3.0261 |
| near-fog | 15.7918 | 15.5125 |
| save-butterfly-glow | 3.6845 | 3.3727 |
| rain | 13.6674 | 12.3330 |
| riverbank-vignette | 8.8439 | 8.4630 |
| arcana-rankup-sparkles | 13.6633 | 12.9090 |

ovdiff random sweep (`-n 20`, 1003 functions with valid cases): exact cases 16912 -> 17120 of 19035, functions exact in every case 828 -> 843, 51 functions gain
exact cases and 4 lose one (NaN/Inf/clamp cases). Golden replay (`-R`, retail data from PCSX2): exact 2668 -> 2704 of about 3610, mismatches 775 -> 745,
float-tolerance 176 -> 158.

## Oracle lane correctness

The differential oracle must preserve EE lane order before an `exact`,
float-only or benign replay verdict is evidence. In particular:

- `PCPYUD` takes `rs.upper` into the low doubleword and `rt.upper` into the
  high doubleword; its operand order is intentionally different from `PCPYLD`.
  Reversing it makes the retail `__muldi3` extraction `pcpyud at,at,zero`
  return zero for nonzero low products.
- `PMFHI` and `PMFLO` read both banks of the single 128-bit HI/LO pair.
  Their upper doublewords are the `hi1`/`lo1` banks written by `MULT1`,
  `MTHI1` and the related operations, not separate unused state.
- SA is represented by its hardware-visible byte selector. `MTSAB`,
  `MTSAH`, `MFSA` and `MTSA` must agree, and `QFSRV` converts the byte
  selector into its bit shift. The Sony manual treats an SA save value as
  opaque; concrete MFSA encoding and MTSA masking also follow the
  [hardware-generated funnel expectations](https://raw.githubusercontent.com/unknownbrackets/ps2autotests/master/tests/cpu/ee_simd/funnel.expected),
  rather than an assumption about an emulator's internal units.

The pack/extract/interleave/shuffle maps and packed shifts are audited against
[the PS2 implementation](https://raw.githubusercontent.com/ps2homebrew/ps2-unpacker/main/MMI.c),
the Sony EE instruction definitions and the hardware tests. Unsupported lane
instructions still stop as unsupported; they never return approximate values.
`ovdiff --selftest-lanes` exercises nonzero sources, aliased destinations,
zero registers, every modeled shuffle map, shift boundaries, accumulator
transfers, hardware SA save/restore and the actual retail `__muldi3` over
zero, signed-edge and high-word operands. It requires `P4PC_ELF` for that
real-code witness and is registered as `ovdiff_ee_lanes`/`ovdiff_ee_lanes64`.

After a lane-oracle change, enumerate every retail function containing the
affected operation families, including compiler/libc helpers and unnamed
executable ranges, and re-run the corresponding ovdiff and recorded replays
on both ABIs. Follow the complete static caller closure and also replay the
available runtime corpus, since indirect calls are absent from a static
`jal`/`j` closure. Earlier `exact` or benign labels are not carried forward
without that revalidation; missing recordings or unimplemented native seams
are stated explicitly rather than counted as exact.


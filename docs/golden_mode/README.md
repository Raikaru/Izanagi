# Golden mode

Golden mode is an in-development alternative to the existing PS2 mode. It is intended to derive its content at runtime from the player's own Steam installation; no game content is distributed with the port. The mode switch and ID mapping described here do not yet make Golden mode playable.

`[game] mode = ps2` is the default; `[game] mode = golden` selects Golden mode. `P4PC_MODE` overrides the setting. The mode is resolved once and remains fixed for the process. An unknown value is invalid.
Golden name accessors use the published MSG bank extents and return guest addresses only in Golden mode. Skill and persona name rows have distinct 23-byte and 21-byte Golden widths; PS2 retains its existing 19-byte and 17-byte layouts.


Golden startup fails closed if native package preparation fails or the table boot has not published all nine inputs from the same verified Golden cache. It never offers the PS2 optional-package fallback in that mode. PS2 retains its existing recovery behavior.

The Golden-only VFS source opens `data_e.cpk`, `data.cpk`, `data_movie.cpk`
and `sysdat.cpk` read-only. English takes precedence over common archives. It
resolves case-folded keys, PS2 container-label aliases and nested PAK members;
unmapped requests are logged and refused. Each selected object records its
archive chain and SHA-256 in VFS origin metadata. Compressed objects use a
bounded 64 MiB, eight-entry LRU; open objects larger than the cache are not
retained. File identity is rechecked before reads. The source is registered
only after Golden table admission, and PS2 does not register it.

`P4PC_GOLDEN_HYBRID=1` permits a labelled development-build disc fallback;
release builds refuse that option. This is not a release Golden boot. Native
asset formats, Golden scripts and other capability refusals remain unresolved.

The local metadata census uses `tools/golden_mode/inventory.py` followed by
`tools/golden_mode/census_scripts.py --install DIR --inventory inventory.jsonl
--elf SLUS_217.82 --out DIR`. Outputs must be outside the repository and game
install; they contain paths, hashes, sizes, script command counts and media
header classifications, not payloads. Procedure command association is a
linear instruction-range census, not a control-flow or reachability claim.
USM encryption can be indeterminate because its container has no universal
encryption flag; an unknown classification is not proof of clear content.

Embedded ACB wave banks, including numeric-ID ITOC CPKs, are inspected for HCA
cipher headers, and AMD chunk members are counted separately (including GMO
signatures). Any bytes beyond an AMD's declared members are recorded as
uninterpreted trailer metadata, not silently classified as members. Their bytes
are not written to the census.

For the PS2 noninterference witness only, a non-release native build may set
`P4_GOLDEN_VFS_WITNESS=ON` and launch PS2 mode with
`P4PC_GOLDEN_VFS_DORMANT=1`. The archives are registered in a dormant slot
that VFS lookup never consults; this does not admit Golden mode or enable a
PS2 fallback in release builds.
The boot capability ledger also refuses unimplemented Golden BF commands,
Golden script features and native asset families. Their presence in a static
census is not evidence of dynamic reachability or implemented behaviour.

## Native textures and atlas data

The Golden-only texture layer decodes DDS BC1/2/3 and RGBA/BGRA8 (including
mips and DX10 sRGB metadata), TMX, PNG and TGA. The new GL upload seam refuses
in PS2 mode before touching GL; the PS2 texture, raster and TMX paths are unchanged.
See [format, license and qualification records](textures.md).

The compiled icon adapter verifies the English `c_main01x2.spr` member,
resolves actual sprite records for all 22 ordered item flag selections and
arcana 24–27, and stores natural-source crops and rectangle metadata through
the M5 per-user hash-addressed cache. No-match item flags produce an explicit
transparent no-draw resource, not a borrowed icon. The table cache identity
also includes the atlas source hash and converter version. Resources200 loads
these verified objects only in Golden mode and publishes them through the
normal owned image generation, pinning and retirement rules. The source key
contains both cache-manifest and object SHA-256, with a distinct non-VFS
provenance sentinel.

This supplies the R6/P9 **data** prerequisite; M9's UI call-site routing,
Steam special-state binding and text rendering are not implied. All other
Golden boot refusals remain enforced.

## ID domains

The map preserves PS2 IDs and places Golden-only entities in the existing expansion families. For the measured Golden table shape, let `A = active_skills - 440` (32).

| Domain | Golden ID | Port ID |
| --- | --- | --- |
| Skill | below 440 | unchanged |
| Skill | 440 through `active_skills - 1` | `576 + id - 440` |
| Skill passive | `id - A < 576` | `id - A` |
| Skill passive | `id - A >= 576` | unchanged |
| Enemy, item, persona | any | unchanged |

IDs at or above the retail enemy, item and persona counts (336, 1536 and 256 respectively) are declared as expanded entities. The measured shape has 48 expanded skills, 32 enemies, 1024 items and no personas. Declarations are permitted only in Golden mode and must cover every expanded ID exactly once before registration begins.

Golden-only skill effects and enemy or persona models need presentation sources. Native Golden asset support is planned after the table milestones; an entity without an acceptable source must not silently use an unrelated effect or model. Converters must map IDs inside table rows as well as row indices.

The Golden declaration assembler consumes already-converted rows, an explicitly approved retail model source for every expanded enemy or persona, and an approved icon reference for every expanded item. It does not infer an asset from a model-table row or invent an icon. Golden-only skills carry no effect source, so casting them is refused until an approved presentation source exists.

The in-development readers validate PAK v2, FTD and DAT framing without treating padding as payload. The EXP reader accepts only the measured Steam executable and EXP table hashes, locates the table structurally in its file-backed `.sdata`, requires that section to be initialized non-executable data, and never executes or unpacks the executable. It hashes the entire executable for build identity. Golden boot admits only registered compiled adapters and reports each missing required capability before reading or publishing tables.

The capability ledger keeps EFFECT0 Golden rows (472×12) and MODEL0 Golden rows
(11×382) distinct from unsafe PS2 truncation. Expanded item categories/actions
and expanded item models have separate refusals and owners. The image supplier
ledger is a required capability: every M2 `golden_check` object must have one
supplied value or an explicit per-object retention decision.
The data-driven AICALC9/10 admission validates its six raw AI dependencies and
programs and requires the M7a SKILL1 projection. Its registration does not
satisfy the independent M6b image-supplier ledger or other Golden refusals.

The measured Steam install cannot boot in Golden mode at this stage. Required adapters remain unavailable, including complete EFFECT0 action-packet behavior (472×12 versus 440×4), approved expanded asset sources and text bindings. The M6 image binder publishes the twelve-material recipes, 28-arcana charts, gates, EXP and inheritance only after the complete table cohort is admitted and sealed; it does not bypass any remaining refusal. AICALC9/10 readiness is data-driven, subject to the verified AI cohort and M7a SKILL1 projection.

The Golden icon adapter supplies verified atlas-backed icon references.

The approved-output assembler prepares the retail prefixes of ITEM, SKILL and UNIT, the converted PERSONA and ENCOUNT banks, raw MODEL banks, and declaration metadata, affinity, AI-row, skill-auxiliary, semantic-element, inheritance-group and inheritance-rate inputs. It leaves unresolved cache entries absent for their responsible adapters; it does not synthesize EFFECT, AI programs, icons or text.

MODEL0 remains one byte-identical 11×382 backing block through the cache and packed guest loader. Golden-only consumers select the widened stride, 32 motion records and the kind byte at `+0x17C`; PS2 keeps its original layout. Hit-ordinal reads remain unchecked against row boundaries, retaining motion, tail and next-row aliases in the actual guest backing. Motion timing retains binary32 arithmetic and defined AArch64 float-to-integer behavior. Extra animation assets remain M8c's responsibility.

Golden passive SKILL0 `FF` is signed NONE, transported as `FFFFFFFF` and serialized back to `FF`; its auxiliary byte is preserved. Inheritance bucket 18 is a separate namespace, not semantic element 18. Only an inactive `golden-mode` source-owned declaration receives this explicit sentinel admission; arbitrary authored and non-Golden declarations still require a registered element. Inactive Golden semantic definitions expose no active metadata row.

The full EFFECT0 and SKILL1 rows are copied from verified cache extras before cache buffers are released. The native completion fence publishes their sidecar only after checking the PS2-shaped prefixes; EFFECT accessors use an allocated contiguous native guest block, retaining unchecked surrounding-memory reads. Result markers have action-address/UID lifetimes, distinct from PS2 reaction, reflection and message fields. Packet delay sidecars preserve values above32767 through native allocation, stepping and release; ordinary signed16 delays need no sidecar allocation and the non-Golden branch is unchanged.

Golden selected-skill resolution clears ordinary result markers, uses the live flag38/community-selector5 admission, rewrites every selected party result without clearing unrelated hit-row fields, and preserves the battle-bit31 one-shot latch. Presentation consumes the header0100 genus override, explicit residual EFFECT1000 dependencies, EFFECT20000 RNG ordering, upper motion/effect options, signed opening lead, and the exact two-stage fade-context lookup. Guest-pointer helpers live in the mirror source, not port headers, so all native profiles rewrite their guest field accesses. The EFFECT800 path constructs real type301 packets with mode1/options0, the specified P0/Pi dependencies and sibling902/A02/506 followers; ordinary and reflected primary delays remain separate. The full EFFECT0 and R2 timing adapters are installed. Actual BED400 playback remains refused under `M8f-res:battle-common-14A8-1490`: M8f must install the typed, ready49-entry common-resource owner; no PS2 formation slot or empty effect substitutes for it.

`p4_golden_live_frame_step()` is the shared Golden binary32, nominal60Hz frame publication. Golden application startup primes it from the native kernel's paced scheduler clock, and frame boundaries publish before native callbacks. Virtual/headless runs use the same scheduler's virtual clock; rendering and packet updates never independently sample wall time. The producer preserves the Steam counter-narrowing order and [.01,3] clamp. Practice pacing is already represented by scheduler time, not reapplied as a consumer multiplier. The sound lifecycle owns a separate16-row float-age cache: reset3, changed cue0, half-step advance, expiry>=3; the rotating signed8 timer remains unchanged. Type506 independently starts at0, invokes the real target message panel once, advances by the same half-step and completes strictly after32. Clock, cache, action and packet reset domains are independent.

`M11-dep:yell-producer` remains separate from R2: this branch consumes the shared header bit but does not fabricate a Golden YELL action, updater or emitter. Its absent producer is inert until M11 installs the assist/message feature.

The private table cache lives under the user's SDL preference directory in `golden-cache`, outside the game install, program directory and source checkout. Its key covers source hashes and converter, specification and schema versions. A complete entry contains a manifest, a verified table index, `image-index.json`, and SHA-256-addressed objects. The table index authenticates the image index as an object, and the separate image index file must be byte-identical. Damaged entries are treated as misses for rebuilding rather than trusted. Cache payloads are user-derived and must not be added to source control or distributed.

The cache refuses unsafe paths and old keyed entries containing unexpected files it cannot prune; it does not recursively delete unknown content.

Synthetic cache tests also keep their valid preference fixtures outside the checkout:
the shared `golden_cache_fixture.h` helper selects Windows `TEMP` or POSIX `TMPDIR`
(default `/tmp`). Missing or empty Windows `TEMP` fails the test, with no checkout
fallback. A build directory below a checkout is not a valid preference root.
The cache test separately verifies that an unreported checkout ancestor is
rejected; this safety rule applies on both 32-bit and 64-bit Windows as well as POSIX.

The source identity collector records the verified executable, English battle and extra tables, fusion members, inheritance rates and rank-up member under their full inventory chains. It includes the pinned Steam build and the converter, specification and schema versions. The identity remains valid only for the exact checked inputs; it does not itself publish the tables.

The inheritance-rate reader verifies the measured English archive member hash. Its converter uses the skill ID map for every rate, including Golden's shifted passive skills, and requires a one-byte rate for each skill in the supplied domain.

The Social Link rank-up reader verifies the measured English archive member hash. Its converter maps both the taught and replacement skill IDs and refuses malformed table shapes and out-of-domain skills.

The skill table projection maps element rows and projects 44-byte Golden active metadata to 40-byte PS2-shaped rows without discarding the original flags or padding: all 472 original rows survive in a cache-verified sidecar. Golden-only consumers implement the `0x100` intermediate-sprite bypass, `0x200` threshold shortcut, `0x400` target gate, qualified `0x800` counter suppression and the combined four-bit `(flags >> 9) & 15` calculation-source selector, including `0x1000`. Bits 13–31 and opaque byte 7 remain unmodified. The battle-completion fence checks native retail-prefix compatibility before publishing the sidecar; session teardown releases its native EFFECT backing. PS2-shaped banks and non-Golden routes retain their original fields and conditions.

Generated battle and calculation hooks use only `p4pc/guest.h` for `P4_G2H`/`P4_H2G`. Including the broader `gmem.h` there also imports host `string.h`, which conflicts with the mirrored units' existing guest CRT declarations.

The added camera/calculation reads and unchecked source accessor have measured census entries for every supported native profile. Refresh each profile from its own layout oracle: array, macro and variadic counts differ by ABI. An increase in missing-oracle records is a build defect, not an acceptable census refresh.

Extracted guest-body witnesses use the layout oracle's `-m32 -malign-double -mfpmath=sse -msse2` ABI flags, including Linux i386's explicit eight-byte double/u64 alignment. Keep the shared action-size assertions enabled rather than accepting the host compiler's default layout.

The M7a extracted-body witnesses apply only their milestone's mirror section, including EFFECT0 and sidecar subheadings, and stop at the next `# Golden M...:` milestone header. A synthetic appended-section regression retains strict replacement-count drift failures. The full generated-unit builds still apply every production mirror fix.

The whole-bank projection assembles the skill, enemy, item, name, persona and encounter converters without using PS2 table bytes. It retains expanded-ID rows in the projected banks for later declaration and refuses malformed names rather than returning a partial projection.

The ITEMTBL converter checks both section headers, name encoding and every granted-skill ID before writing the PS2-shaped metadata and names. Empty name slots remain empty and cannot be used as declaration names.

M7d's RE-3 adapter admits the full 2,560-row ITEM table, with kinds 0–9
mapped to eight logical categories. No pinned row has kind above31; rejecting
an unspecified kind is still required. Camp lists retain the verified CTD
group/range ordering and exact flag equality, not icon-bit membership.
The per-user cache retains `items/cmp-tables` for native list lifetime.
Skill-card transactions teach the already-converted granted skill into the
first free slot or replace the first selected old skill, consuming once only
on completed success. The qualification fixture supplies admitted task
results; it does not invent a PS2 persona-selection UI, duplicate rule,
locked-slot policy or registration/save behavior.

The observed routing capability R7a is separate from required sub-refusals
`R7a-dep:M11-skill-card-UI-U1-U4`,
`R7a-dep:M11-return-field-flow` and
`R7a-dep:shop-permissions-context-flags-U2-U3-U5`.
The A3 return transaction/refund envelope is fixture-qualified; its later
field/message continuation has no approved PS2 binding, so live Golden use
refuses instead of setting the unrelated PS2 flag5121.
Shop selector mappings are not facility-specific visible tab layouts.
Contextual book/fishing/gardening/exchange actions, unknown flag effects and
Golden UI stay refused, alongside unchanged R6/R8 and model requirements.

P10's matrix checks all68 locally converted weighted groups:6,528 Golden
cells (retry false/true) and3,264 projected-native cells, including normal-only,
boost and field suppression. The native four-argument selector still takes
a PS2 level, never a Golden forced-mode enum. Its Golden signed-rate total
bridge preserves that projection; its PS2 formula and zero-total assertion
are unchanged. Empty eligible formation weight returns-1 before the second
RNG modulo. `P10-dep:E2-forced-retry-liveness` remains refused: positive
category totals do not prove forced/retry termination.

Golden skill and Persona name banks retain their 23- and 21-byte widths; their Golden-only consumers use the widened layout. PS2 retains its 19- and 17-byte layouts. Names that do not fit their selected destination are refused, never silently truncated.

Encounter projection maps formation enemies and record items, and repacks weighted-group rate headers. The consumer returns no encounter when a selected category has zero eligible weight; the converter preserves those valid empty categories.

Persona growth, party learn-event and skill-replacement projections map active skill references while preserving non-skill event payloads and the unused tail after a terminator.

Enemy metadata projection maps learned skills and item drops separately; the flagged special drop is an item ID, despite misleading legacy skill-oriented field names.

The fusion source reader pins the measured English members. Projection checks tier, affinity, gate and chart shapes, maps gate Persona IDs and bounds chart arcana indices. The M6 registry publishes the three 28×28 charts, 26 gates and all 17 recipes, including the twelve-material row, through the existing Golden-only runtime installers.

Additional pinned English inputs cover EFFECT, MODEL, AICALC and the camp table pack. Their presence does not establish compatibility of opaque Golden AI programs or provide native Golden asset support.

Twelve-material fusion recipes use a Golden-only explicit-count representation; PS2 fusion and save layouts remain unchanged. Cached recipes have a portable 27-byte wire row (little-endian result, twelve material IDs, explicit count), never a host struct or guest pointer.

## Image publication

`golden_mode_image.c` owns the data-only registry: name, converter `id@version`,
exact byte count, retail address and bounded retail prefix. EXP (99×u32), nine
pool tiers and the 14×19 weight matrix overwrite only their original image
extents. The 624 port-ID-indexed inheritance bytes retain the 576-byte retail
prefix at its fixed address; expanded skill declarations carry the matching tail.
Charts, gates and recipes use the M5b widened installers.

The production cache callback captures only reloaded, verified cache objects.
`golden_mode_image_bind.c` retains an independent session-owned copy and issues
an image-manifest/cache-key digest in the actual builder's native source ledger
before seal. Binding requires that exact sealed builder and its native-use
context, rehashes every object, validates destination bounds and overlap, installs
widened banks, backs up and writes fixed guest ranges, and authenticates readback
before marking the exact issued native origin used. It runs after ELF load and
table seal but before exposure or any guest entry. A second bind is refused.
Failure or pre-exposure rollback restores the image and clears widened banks;
explicit post-drain teardown releases retained bytes. PS2 does not capture or
register an image origin, and binding in PS2 mode is a no-op.

M6b freezes source specifications at `918c10038d16699237d73dba9dd787b738b4596f`.
Its supplier ledger covers all **1,043 objects / 249,428 bytes**: **24 bound,
0 D10a, 0 D10b, 0 D10c, 1,019 OPEN-retain**. The 21 PROVEN and three
PROVEN-layout rows at that cut are supplied; later source batches, including
PROVEN-code authoring, belong to M6c. L1 is lifted only for the qualified rows.
**L1-open = 1,019** remains tracked and must reach zero before M12; this is not a
cosmetic exemption or proof that the retained PS2 values match Golden.

`image/l1-ledger` is metadata only, authenticated by the same cache/image
transaction. Its digest is part of the cache specification identity and the
issued image origin, so changed decisions invalidate stale cache/save identities.
Each open object is named as `L1-open` in the boot manifest and admission log,
with its address, retained extent and per-object proof path. The canonical
inventory, primary decisions and archive supplement are joined by address;
unknown, duplicate, missing or size-mismatched decisions refuse. Runtime bindings
also pin each source row's required-seam contract. Source-only retention history
is recorded separately from the qualified runtime status. The layout-qualified
16-byte object with no identified direct reader explicitly records
`consumer_witness=none`; binding/readback is not a fabricated behavioural witness.

PE spans are read only from allowlisted initialized, non-executable data sections
during the existing EXP/image hash pass. Archive spans pin the complete FTD member
and validate its logical row/stride extent. Prefixes retain their PS2 tails.
The expanded 192-row key bank stays native; the 84-byte renderer-reference table
normalizes three banks of seven 64-bit Golden identities to existing 32-bit guest
queue identities, without reading or reallocating virtual queue objects.

Six PS2 weather-table fragments share one complete 128-by-28-byte Golden source,
not six stitched or oversized guest writes. Golden date lookup, eligibility,
priority/display and gameplay outcome reads use that same descriptor. Full
four-byte chosen effects live in a Golden-only companion registered/freed with
the fusion work lifetime; the original PS2 work and save layouts do not change.
The weighted draw consumes RNG only for a positive total, preserving unsigned
weights, binary32 scaling and full effect arguments. Missing dated rows retain
the separately unresolved PS2 calendar-phase fallback, which remains L1-open.

Synthetic and owner-install qualification exercise the real binder and extracted
consumer read sites in both modes without substitute production adapters or
starting the game. The existing seven M6 objects retain Steam-read equality
checks; all new supplied spans are independently re-read, including the shared
weather bank, native key expansion, normalized references and retained tails.
The companion lifetime witness covers simultaneous work objects and address reuse.

Isolated host fixtures translate guest-address results through `P4_G2H` before
host service calls, including fusion message-window acquisition, rather than
casting a 32-bit result directly to a host pointer. Production decomp sources
remain untouched. The six compiler/platform census baselines account for the
additional Golden community-rank ABI call and narrowing; both actual M6 and
M6b Mac measurements also establish the inherited EXP SDK-declaration count.
Unsupported-construct and layout-oracle guards remain enabled and unchanged.

The issued image origin uses the existing native-mode save record format;
missing or changed Golden image identities are fatal on load, not ordinary
historical-native warnings. Ordinary PS2 ledger and sidecar bytes are unchanged.
R11 (widened skill/persona names) is registered only when both correctly sized
projected banks exist. The level-from-EXP consumer selects the bound guest curve
in Golden mode; its PS2 branch retains the original static-table expression.
The Golden AI semantic engine implements the twelve missing command transitions
from the pinned clean-room `ai-commands.md` contract. Its input views carry
Golden IDs and unprojected 472×44 skill rows, not the PS2-shaped ID/row banks.
The engine is mode-gated; PS2 calls return unhandled without reading or writing
the supplied state. Separate real-native qualification below establishes the
VM/lifecycle path; semantic vectors alone are not that receipt. R4 readiness
requires the verified AI cohort and M7a SKILL1 projection. Mapped P11 uses the
verified eleven-block program backing.

AI draws use a separate 31-word additive stream (310 discarded updates after
seeding); common 148 uses SFMT-19937 with period certification. Commands consume
session streams and never seed them locally. RNG2 preserves separate binary32
rounding and uses the Switch saturating float-to-s64 conversion profile; invalid
conversions increment the session SFMT anomaly counter instead of adopting
Steam's indefinite value. Assist presentation always consumes two SFMT draws,
discards the first, and scales the second in binary32 (the result can be 3).
The scalar SFMT implementation is independently written from the contract and
published algorithm; no reference source is incorporated. Saito and Matsumoto's
[SFMT reference implementation](https://www.math.sci.hiroshima-u.ac.jp/m-mat/MT/SFMT/)
uses a [BSD-3-Clause license](https://www.math.sci.hiroshima-u.ac.jp/m-mat/MT/SFMT/LICENSE.txt).
Tests include the contract vectors and public SFMT seed-1234 output vectors.

The `battle-state-lifecycle.md` contract at `d51b462f4` defines the native
sidecar writers. Real hooks cover initialization/release, all-actor eligible
admission, continuous entry, end-state production, bit29, `clear187AC` and
knowledge publication/save. Command8479 is an assist choreography selector,
not a generic message parameter. `R4-dep:M11-assist` and
`R4-dep:M11-message-follow-up` remain inert: qualification verifies that they
do not emit or alter a packet; presentation is not claimed.

The separate `golden_vm` interpreter implements the 35 specified opcodes with
eight-byte integer carriers/banks and four-byte float banks. Binary integer
operators and IF narrow to s32; MINUS and NOT remain wide. It preserves result
slot lifetime, float payload upper bits, CALL/END semantics and native COMM
yield/argument cleanup. Unsupported backing and stack corruption are rejected,
not emulated with fabricated numeric values. Division and invalid float conversion
use the Switch profile and increment an anomaly counter. Synthetic BF tests
exercise the interpreter and all twelve added command handlers. This is not yet
a live native-work-pool binding or cache-backed AICALC execution receipt; the
refusals above remain enforced.

The native bridge now has mirror hooks for separate wide-VM dispatch, native
COMM arguments/results, successful work-pool release, action/battle lifetimes,
eligible-start/continuous-entry writers, explicit bit29 transitions and nonzero
message-latch consumption. Golden RNG routing and persistent knowledge query/
mutation hooks are also installed. The hybrid receipt below qualifies their
native script path, not gameplay or whole-game startup. Synthetic level-source
admission and knowledge lifecycle tests cover the backing helpers independently.
M7a extraction fixtures retain all of their MODEL/EFFECT/SKILL/result/packet
splices and stop at the next milestone heading. Expected replacement-count
drift remains fatal; production still installs both M7a and M7b hook families.
Mirrored units include only the scalar `golden_ai_native.h` boundary. The
cache/provider header is host-only: importing it into translated units leaks
standard-I/O declarations into the game's distinct formatted-output ABI.
For address conversion alone, translated seams include `guest.h`, not
`gmem.h`: the latter imports CRT string declarations that can collide with
the retail memory-function aliases. `P4_GPTR(T,x)` is exactly
`(T *)P4_G2H(x)` at these boundaries.
Command8485 publishes its rebuilt native candidate work even on failure, but
only a non-`-1` result writes a selected target; a previous successful selection
does not authorize a stale target write.

The explicit `P4_GOLDEN_AI_QUALIFICATION=ON` developer configuration adds
`golden_ai_native_install64` and `golden_ai_native_install64w` targets, excluded
from the default build and packages. Release/mobile configurations refuse
this option, and this configuration's player refuses normal boot. Invoke the
test executable with `ELF PS2_ISO STEAM_INSTALL [BLOCK PROCEDURE]`; it validates
and caches the owner's six raw Golden AI dependencies, loads/patches the real
game image, and admits nine hash-pinned native PS2 banks from the owned disc.
The original EE heap and matched chkmem/RW allocator table are initialized
below the synthetic arena; native packet allocation uses no replacement.
It runs unchanged BF procedures over synthetic guest battle/task/HUD inputs
through actual native commands and wide accessors, labelled **hybrid qualification
(PS2 native tables + Golden raw banks)**, not a gameplay receipt. No native
command result is mocked, and widened Golden skill flags are not truncated.
Task/HUD storage is a valid native-layout input, not a task-creation,
scheduler or rendering receipt; native getters and HUD writers are unchanged.
Script work retains the native `-1` message-manager default while no manager
is attached; this does not qualify message-window rendering.
**R4 production lift requires M7a SKILL1 projection.** The dev-only hybrid
qualification does not remove that production prerequisite.
R4 admission reads the prepared table cohort; cache extra descriptors acquire
their backing only during publication. Extra names remain strictly sorted,
as required by the verified cache loader.
The entry also runs the real M7a projection/output assembler and R4 admission
over the verified Steam data, comparing serialized BF9/10 with the cache
product. That independent conversion check does not turn the hybrid runtime
counts into a gameplay or whole-boot receipt.
Coverage distinguishes `real-caller` from `synthetic-locals`. Native entries
are selected by the existing procedure-name lookup or Golden AI records;
actual BF CALL/JUMP destinations are recorded only for completed executions.
Unreached procedures use an explicit per-procedure vector: declared integer
locals are `1` (entity ID1, skill ID1 and index1); float locals are `1.0`.
These caller-owned banks are initialized before execution, never on CALL/PROC.
Real callees reached from such a vector retain a separate
`caller_from_synthetic` count/provenance. Neither category is gameplay evidence.

The actual-install hybrid run completed all137 AICALC9 and185 AICALC10
procedures through7537 real native calls, with no failures:

| Bank | Primary real-caller provenance | Real callees from synthetic parents | Direct synthetic-locals | Total |
| --- | ---: | ---: | ---: | ---: |
| AICALC9 | 84 | 12 | 41 | 137 |
| AICALC10 | 104 | 5 | 76 | 185 |

The counters' `real_caller` aggregates are96/109; the middle column is a
subset, not additional coverage. All procedures completed; M11-dependent
presentation branches remain explicitly inert as described above.

### Wide VM spec conformance

| Contract | Implemented profile | Evidence / qualification |
| --- | --- | --- |
| Integer carrier and banks; binary operators/IF narrow | s64 storage, s32 binary domain, full64 MINUS/NOT | Synthetic `golden_vm_test` vectors |
| Result, COMM and calls | Persistent reserved result; topmost-first arguments; no pop/advance on yield | Synthetic VM/AI vectors and all322 procedures through7537 real native calls in hybrid qualification |
| Integer DIV zero / INT32_MIN divided by -1 | Switch 0 / INT32_MIN; anomaly counter increment | Steam faults instead; Switch chosen by project ruling |
| Invalid f32-to-s64 conversion | Switch saturation, NaN zero; anomaly counter increment | Steam indefinite value deliberately not adopted |
| Stack corruption and unbacked indices | Explicit malformed-input rejection | No fabricated retail numeric behavior |
| Assist presentation RNG | Two unconditional SFMT draws; binary32 scale can yield 3 | Standalone edge/draw-count vectors; presentation wiring remains open |

## Native message and font formats

[M8b text and fonts](text.md) provides borrowed, bounded MSG1 message/dialog,
selection, speaker and control-token views, native FNT glyph decoding and
metrics, and a Golden-only RGBA page-rendering seam. The English path does not
reuse the PS2 RAW space/glyph mapping. No payload or system font is shipped.
M9 still owns UI consumer routing, dynamic controls and R8 admission.

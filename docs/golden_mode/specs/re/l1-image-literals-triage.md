# L1 consumer-module and gameplay-impact triage

## Scope and classification contract

The [triage table](l1-image-literals-triage.jsonl) has one address-keyed row for
all **1,043 selected objects / 249,428 PS2 bytes**, including every one of the
**1,026 OPEN-retain rows at `8c75aca2b`**. `decision_before` uses that exact
baseline; `decision_after` includes every merged cohort publication. Cohorts
1–2 add thirty-seven PROVEN, three PROVEN-code and two D10c rows; cohorts3–6
retain their own proof attribution. Totals use the restored merged ledger.
No addresses or bytes were dropped. The current table's canonical LF SHA-256
(Git representation, not Windows CRLF) is
`3b1bf8a6137dc7bf6ac151d86c6d4e4ad87cbbf1fa6f58bad2147e52e91c30f6`.

Each row records its impact tier, rule, confidence/basis, consumer modules,
PS2 consumer names, source files/marker lines, and unresolved indirect leads.
This is **triage, not a liveness or behavioral-equivalence certificate**:

- `balance-game-logic` includes gameplay-resource routing and **conservatively
  assigned unknowns**, not just arithmetic balance coefficients. An unidentified
  consumer is never silently assumed cosmetic. The measured fraction below is
  the fraction assigned to the gameplay-risk queue, **not a proven fraction of
  gameplay-changing numeric constants**.
- `ui-layout-cosmetic` covers observed presentation modules/readers, sprite/model/
  effect/audio paths, source-file assertion identities and identified display-text
  namespaces. Diagnostic and resource-path rules precede broad module rules:
  `datCalc.c` is diagnostic even though its consumers compute damage/HP.
  Presentation classification does not authorize dropping a resource or retaining
  PS2 permanently; asset-loading safety and coherent displays still matter.
- `dead-unused` requires positive non-use evidence. **Zero rows qualify.** No
  direct xref, a dummy-looking label, a PS2-only SDK name, or a failed Golden
  search is not sufficient.
- Multiple observed modules remain multiple modules, not whichever seems easiest.
  The source snapshot is PS2 decomp `18016afe6`; the existing matcher supplies
  recovered function-envelope MIPS xrefs and the ledger supplies M2 source leads.
  Inferred TUs may contain unrelated functions, so recovered behavior names also
  inform presentation triage. Marker lines identify source locations, not a new
  function match.
- The exploratory aligned-word parent scan has **false pointer candidates**:
  floating-point/packed data can numerically resemble addresses. It is retained
  only as `indirect_pointer_leads` / `indirect_module_candidates`, never promoted
  to a proved consumer. Directory-family attribution additionally uses PS2
  directory-list content and `H_Cdvd_SelectDirectory`'s directory-command tables
  (`h_cdvd.c:412-465`); interior partitions are explicitly not direct references.
- **15 remaining OPEN rows have unresolved direct consumer-module ownership**.
  Their table bucket is `unresolved-indirect`, with candidate owners and source
  evidence preserved. That is an explicit unresolved triage result, not invented
  module identification. Six weather fragments now have whole-table interval
  ownership despite several lacking direct address references. Do not generalize
  their proven interval to other arrays.

Using the merged classifications consistently on both sides, baseline OPEN
evidence bases are:314 conservative,57 module-risk,26 observed-role,
235 module-role,31 consumer-role,92 observed-diagnostic,181 resource-role,
46 content-and-namespace,8 typed-presentation-reader,4 observed-reader-role,
7 observed-task-registration-identity,1 observed-named-resource-registration,
2 observed-field-archive-loader,4 observed-dungeon-task-resource-family,
5 observed-paired-typed-selector-schema,1 observed-fishing-help-controller-identity
and4 observed-event-reader-constructor-identity;4 observed-resource-or-constructor-role,
2 positively-retired-unused-device-or-explicit-source-trace,
1 paired-complete-code-constructor-schema and1 observed-active-player-name-format.
The explicit `rule` and `impact_basis` fields make these distinctions filterable.
No claim is made that broad module/content triage equals exhaustive manual
semantic proof for every object.
Peer proofs reclassify BF diagnostics, panel message banks and typed camera/
analysis-bank readers by their actual presentation role. This batch also
classifies the obsolete missing-procedure format637320 as diagnostic/UI.
The same updated labels apply before/after; baseline source classes and
inventory denominators are unchanged. D10c representations are not dead assets.


## Cohort 5 typed-enclosure successor

[Typed-bank receipt](l1-c5-typed-enclosing-banks-proof.json),
[weapon-bank receipt](l1-c5-weapon-message-bank-proof.json),
[rank-up receipt](l1-c5-rankup-message-bank-proof.json),
[camera-name receipt](l1-c5-camera-state-labels-proof.json),
[action/encounter receipt](l1-c5-action-encounter-labels-proof.json),
[AI namespace receipt](l1-c5-ai-condition-target-labels-proof.json),
[result/shuffle receipt](l1-c5-result-shuffle-resource-identities-proof.json) and
[native cue-bank receipt](l1-c5-model-sound-cue-bank-proof.json):
**61 PROVEN / 57,223 B**, **16 OPEN / 2,043 B**; no canonical layout/code/D10
admissions. The [per-row review](l1-c5-indirect-review.json) covers the current
77 / 59,266 B assignment: two outgoing panel-message rows (910 B), incoming
analysis prefix (623 B), and two incoming weapon-bank rows (4,679 B).
All transfers are Main-approved; never count them twice.

Actual enclosing camera and analysis-bank readers replace false talk/movie/
item-name attributions. Forty-one apparent pointer words across fourteen
selected targets are positively identified as string interiors; the
`649ffc` message-glyph false pointer is additionally explained in the source
receipt. These rejections do not prove non-use of the selected target objects.

Source-qualified groups are five camera-preset slices, one HP/SP bank with
retained head/tail, two expanded analysis-model bank slices, one effect-camera
script slice, nine help member names, six battle-table filenames and ten
whole-resource weapon-message bank partitions.
Typed camera/analysis consumers are now presentation-classified; other source
groups preserve gameplay-risk scheduling. All real actor/level/mode/request/
message-handle producers and native/full-bank obligations are explicit
`required_seams`, not fabricated defaults.

The weapon bank now has actual Golden request/readiness/backing-copy/slot
publication, 32 unique label/page joins to the 53-entry expanded file, and
coherent dated/flag/gift/material selector namespaces. It requires semantic
whole-bank binding, not a changed-size fragment copy. The apparent SDK
`64732e` reference is an ASCII `.sd` integer comparison, not a pointer.
Unproven AI/action/camera label fields stay OPEN even when their enclosing
callback descriptor is known. Global counts/prose are maintained only by
GoldenRE-L1-c12; this cohort subsection does not update them.
Rank-up `63696e` now has the actual nine-locale source/task/state contract;
its `636850` prefix is already an identical supplier outside the L1 selection,
not a new canonical row. The two large `61646f`/`617461` fragments now have
real model sound-cue grid enclosure and consumers, rejecting the former
geometry interpretation. Their Golden actor/season/costume and 27-to-32-row
mapping remain OPEN; disabled-cell similarity is not source proof.

Five camera-name fields / 80 B are qualified by actual 20-to-32-byte
descriptor correspondence and override-aware state/frame loaders, not
duplicate-name similarity. Golden has 55 records, not a widened 51-record
copy: inserted bike state 49 shifts old down/defense states. Selected
IDs 5/9/32/39/43 remain structurally corresponding; runtime state/action/
override consumers still require binding.

Seven action/encounter fields / 128 B now have actual independent table
boundaries, complete old-to-Golden identity maps and live action/phase
producers. Action records are 38-to-42, encounter 20-to-21, both 12-to-24
bytes; inserted Golden states invalidate equal-index copy assumptions.
Actual float clock, pending transitions and list/task lifetimes remain
required runtime seams, not source-proof shortcuts.

Eight AI names / 136 B now use actual condition/target namespace boundaries:
condition `609850` is 78-to-82 records, 12-to-24 bytes, not a guessed
flag-first table at callback slot `6098bc`; target `609ce0` is 40 records,
8-to-16 bytes. The adjacent NULL condition entry must not extend the target
bank. Actual packed-opcode readers and real AI/formation/status/RNG/target
state contracts are proved, with runtime binding still pending.

Five result/shuffle identities / 160 B now have actual calendar/GP/native
member-loader and lifecycle proof. Four are native ARC path values;
`629150` is specifically an ordered two-BMD native message-bank value,
not a scalar replacement for `br_msg.bin`. Actual owned buffers +948/+950,
paired persona arguments and parent cleanup establish its role. Golden
notice 24 replaces observed old 23; no unchanged-ID/payload or raw filename
copy is admitted. All real recipient/producer/lifetime seams remain pending.

Cohort proof-document receipts use committed UTF-8/LF Git blob bytes;
private CRLF copies are not interchangeable hashes. The publication gate
checks those receipts alongside row-count, class and ownership invariants.

Main-authorized [census tooling repair](l1-c5-model-sound-census-trailer-receipt.json)
recognizes valid little-endian PAK-v2 members before a bounded 16-byte opaque
trailer and records the trailer bytes/hash. Fresh cue census recovers three
containers/nine children; 15 parser and 7 inventory tests pass. This changes
no canonical classes by itself. The separate typed actor/model/packed-resource
proof now admits actor-7 `0061646f` / 754 B to its actual complete native cue
bank, not a guessed `bc008` byte window. Actual 3 × 34 motion mapping and
32-row cell readers require native input/consumer cutover; no old27/new32
ordinal equality is assumed. The cross-actor8/9 partition remains OPEN.
Old cached census parts must not stand in for this fresh scan.

No runtime L1 reduction is claimed.

## Cohort 6 structural source batches

[Structural source receipt](l1-camera-banks-proof.json): **9 OPEN -> PROVEN /
38,152 PS2 bytes**, all runtime pending-seam. Eight complete script banks have
30,256 unchanged Golden live bytes and 64 retained PS2 tail bytes; the remaining
partition has 7,832 live preset bytes, including a partial final record.
Native preset readers require the full qualified 39,936-byte source bank.
Corresponding typed consumers and selectors, not byte/visual similarity, prove
the source. Four camera scalar objects remain OPEN, not silently inherited.

The [persona-voice source receipt](l1-persona-voice-proof.json) additionally
upgrades `0x00624B22` / 1,022 B: both primary/secondary readers, complete bank
boundaries, independent Switch source, Golden persona-kind normalization and
the removed character-4 override are explicit. Audio namespace binding remains
pending; source correspondence is not playback equivalence.

The [complete panel MSG1 bank](l1-ui-panel-bank-proof.json) adds three resource
partitions / 4,394 B and the manager diagnostic / 16 B. Two interior rows were
explicitly transferred from c5; `00626FE0` was transferred out to c5.
The [typed diagnostic source proof](l1-diagnostic-source-proof.json) adds
64 source-file resources / 1,448 B, with full Golden paths and actual
file/line sink roles; 19 reviewed cases remain OPEN, with no absence shortcut.
The [named-task/sequence registry proof](l1-sequence-presentation-proof.json)
adds three objects / 2,452 B. The registry changes 31x80 to 22x96 with explicit
callable, ID-namespace, work-layout and floating-delay runtime seams.

After the split, cohort 6A qualifies the
[complete persona-channel preset bank](l1-persona-panel-presets-proof.json):
four more PROVEN objects / 832 B; seven Golden ten-record presets with
explicit initializer, reset, native-copy and tail-preservation seams.
The [complete behavior descriptor bank](l1-effect-behavior-bank-proof.json)
adds one PROVEN / 180 B: twelve update/scale/size records widen16->24 bytes;
selected `00713D5C` is the +12 alias partition and is entirely live.
Callback normalization and the changed sample/emitter/update implementation
remain explicit runtime seams. Canonical, supplement and triage agree.
The [complete flash factory/topology proof](l1-flash-factory-topology-proof.json)
adds four PROVEN / 304 B: full ten-kind Golden callback factory, two typed
triangle-index templates and the positively paired unit-scale quad.
Preserve 30 PS2 padding bytes across those selections; all consumers and
floating-age/callback lifecycle remain runtime pending-seam.
The [thunder/ring/wind factory proof](l1-thunder-ring-wind-factories-proof.json)
adds four PROVEN / 320 B: complete five/four/four-kind banks and their shared
four-column strip topology. All sizes and callback roles are source-proven;
per-family ABIs, integer versus float age and all native readers need binding.
The second split assigned A2's98 OPEN /2,816 B to c6b. Main's third split
transferred44 rows /1,000 B from the87-row checkpoint to c6c; retained A1R
now has **41 OPEN /996 B**. Other writers' totals remain unchanged here.

The [item-mode code proof](l1-item-menu-native-code-proof.json) adds one
PROVEN-code / 848 B: the old thirty-record layout has an actual native
item-mode/controller/renderer replacement, not a new guessed image array.
Twelve modes pair 46 ordered operations and all direct effects. Full real
Golden views, grouped catalog, guard/history and new-flow binding remain
required; no PS2 positions or blank renderer count as runtime closure.

The [persona named-task bank proof](l1-persona-named-task-bank-proof.json)
adds two PROVEN / 64 B through actual model-load and level-up scheduler
producers plus their registered callback/resource lifetimes. Only 22/18 live
name bytes replace old data; ten/fourteen old zero-tail bytes remain.
Expanded work layouts, platform priority differences and every runtime
create/lookup/destroy alias remain explicit seams. The different deferred
free task stays OPEN; missing spelling is not subsystem-removal proof.

The [shared flash math/state proof](l1-flash-shared-math-state-proof.json)
adds two PROVEN / 12 B through corresponding actual staging-store/vertex-read
and random-birth-angle operations. `00713D18` is mutable workspace initial
z/w, not an immutable UV pair; initialize once and preserve real writes.
`007611C0` is the exact full-turn multiply operand, with resolved GP alias.
All 23/11 shared consumer routes and actual lifetime binding remain pending.

The [UI/render task-bank proof](l1-ui-render-task-names-proof.json) adds
three PROVEN / 80 B: singleton player-icon, allocated field-noise and the
complete name-entry scheduler identity bank. Real producer/callback lifetime,
not label equality, qualifies each value. Name-entry constructor, existence
and destruction all share the actual signed-selector three-name dispatch;
default-only aliases are not permitted. Actual work, draw, timing and cleanup
differences remain pending runtime seams.

The [effect-header source proof](l1-effect-misc-header-proof.json) adds
two PROVEN / 40 B, closing two earlier include-spelling exclusions.
All seven old uses are typed allocation-site metadata. Actual corresponding
particle/object producers select Golden `60AF99`, not an arbitrary normalized
alias. Full-path namespace staging, real Golden line and all native/inlined
routes remain pending; the release sink is genuinely a RET.

The [complete camp-root key proof](l1-camp-root-sprite-key-bank-proof.json)
adds one PROVEN /88 B. Actual three-role root rendering, flag-1F option
production and parent dispatch establish the new three eight-u16 key
groups; all24 values are independently read by ordinary Steam getters.
Physical/logical group order and old/new dimensions differ. Full root
atlas/index/renderer binding remains pending, without an old7-option shim.

The [narrow-frame native-code proof](l1-narrow-rounded-frame-code-proof.json)
adds one PROVEN-code /144 B. Full actual constructor field reconstruction
pairs36 ordered local coordinates,54 actual native XYZ stores and18 color
expressions. Author Golden guards, ordering, packet/pipeline and actual
caller bindings; no old template or generic ABI shim. Wide-frame and UV
banks remain OPEN, with no inference from similar gradient geometry.

Cohort 6 split baseline: **81 PROVEN / 47,484 B; 0 code/layout; D10a/b/c zero**.
Remaining **502 OPEN / 17,561 B**, current ownership **583 / 65,045 B**.
The four sequence/debug rows moved in and analysis moved out to c5 per Main.
Weapon-BMD rows were c3-owned and are not subtracted from this denominator.
Global totals belong to the global owner.
No runtime binding or measured reduction in runtime L1-open is claimed.

## Before/after source classes

| Class | Before objects | Before PS2 bytes | After objects | After PS2 bytes |
|---|---:|---:|---:|---:|
| PROVEN | 15 | 2,972 | 541 | 144,033 |
| PROVEN-layout | 2 | 112 | 8 | 240 |
| PROVEN-code | 0 | 0 | 38 | 12,053 |
| D10a | 0 | 0 | 0 | 0 |
| D10b | 0 | 0 | 0 | 0 |
| D10c | 0 | 0 | 211 | 78,039 |
| OPEN-retain | 1,026 | 246,344 | 245 | 15,063 |
| Total | 1,043 | 249,428 | 1,043 | 249,428 |

The781 source-OPEN reductions comprise L1b axis1/16 B; prior L1c
fusion6/5,269 B and code1/32 B; this c12 run37 PROVEN/13,964 B plus
3code/12B and2D10c/48B; cohort3 78 PROVEN/7,327 B plus10code/10,372B and30D10c/2,021B;
cohort4 56 D10c/72,730 B; cohort5 60 PROVEN/56,469 B;
cohort6 345 PROVEN/58,032 B plus24code/1,637B,5layout/112B and123D10c/3,240B.
Ownership transfers are reflected without duplicating rows or crediting peer
work to c12. Every source mapping/replacement route remains pending-seam.
The publication gate observed1,043 canonical and triage rows and no parent-class regression.

## Before/after by impact tier

### OPEN-retain only

| Tier | Before objects | Before bytes | After objects | After bytes |
|---|---:|---:|---:|---:|
| Balance/game-logic, including conservative unknowns | 425 | 141,018 | 158 | 12,907 |
| UI/layout/cosmetic | 601 | 105,326 | 87 | 2,156 |
| Positively dead/unused | 0 | 0 | 0 | 0 |

The assigned gameplay-risk fraction is **425/1,026 = 41.42% of OPEN objects**
and **57.24%** of their bytes before; afterwards **158/245 = 64.49%**
and **85.69%**. These use merged classifications consistently on both sides,
including conservative unknowns; this is not a changed-coefficient census.

### All selected objects, including source-qualified rows

| Tier | Total objects/bytes | Before PROVEN / code / layout / D10c / OPEN | After PROVEN / code / layout / D10c / OPEN |
|---|---:|---:|---:|
| Balance/game-logic | 437 / 142,578 | 11 / 0 / 1 / 0 / 425 | 179 / 14 / 1 / 85 / 158 |
| UI/layout/cosmetic | 606 / 106,850 | 4 / 0 / 1 / 0 / 601 | 362 / 24 / 7 / 126 / 87 |
| Dead/unused | 0 / 0 | 0 / 0 / 0 / 0 / 0 | 0 / 0 / 0 / 0 / 0 |

All newly source-qualified rows are **runtime pending-seam**, not bound.
This task makes **no measured reduction in runtime L1-open**: M6b must bind
and test all direct/inlined/native routes before claiming that reduction.

## Balance-first structural batch: fusion weather

[The proof receipt](l1-fusion-weather-proof.json) upgrades the six partitions
`0063fca0`, `0063fff6`, `0064006e`, `00640131`, `006432b0`, `00643825`.
It recovers the PS2 arrays' actual sentinel extents, establishes duplicate
logic/display ownership, follows the Golden facility loader and named member
into the ordinary Switch date/weighted-effect consumer and its fusion callers,
and qualifies the complete Steam FTD payload. There is no per-function
signature threshold relaxation, guessed byte-window mapping or `.arch` decoding.

The result is a **single shared expanded 128 x 28 source**, not six image copies.
All 94 PS2 keys join uniquely; 34 dates are added, 26 existing records change,
and 68 whole records are unchanged. Both original 94-record arrays include a
dummy and sentinel; eight-byte alignment tails remain PS2. Every admission
records the same group-wide logic/date/display seams. The phase-indexed fallback
source is distinct and remains unresolved; no whole-calendar equivalence is
claimed.

## Concrete remainder plan and effort estimate

The following queues are disjoint and sum to **245 objects / 15,063 B**.
Estimates are **INFERENCE: engineering planning ranges**, not measured throughput
or a guarantee of proof closure. They cover static source research/spec writing;
M6b binding and runtime qualification are additional. Re-estimate after two more
module batches; a failed structural lead stays OPEN rather than being forced
through D10.

| Priority / cohort | OPEN objects / PS2 bytes | Concrete next operation and closure criterion | Estimated engineer-hours |
|---|---:|---|---:|
| 1. Calendar and fusion fallback | 7 / 4,448 | Three task/path identities and one mutable month cache are PROVEN, one day-flag mask is PROVEN-code, two replaced local scheduler diagnostics are D10c. Core signed weather-stack/bypass/saved-real-state operations now have exact source subcontracts; full calendar extent/raw-day domain and actual caller qualification stay OPEN. Five-animation and11->20message-map types are recovered, not admitted sources. | 12–24 |
| 2. Battle, AI and reward logic | 36 / 172 | Thirty-three data sources and three code sources are qualified, including typed degree13 sine coefficients and actual loaded boss34-byte motion map. Real callback/state/model/formation/pose/math/float-clock producers remain runtime seams; continue shared axes and remaining GP values. A module/program owner alone does not prove an interior literal. | 24–48 |
| 3. Field, event, shop and progression | 98 / 5,490 | 78 data sources,10 code sources and30 D10c are qualified. Continue typed readers in Main's source-disjoint A1/A2/B splits; writer transfers do not change cohort attribution.00750008 moved to c6b; weapons00645240/0064732e belong to c5. Non-census005edea0 is NOT an extra row. | 48–96 |
| 4. PS2 filesystem directory migration | 0 / 0 | All56 directory representations have D10c route proofs/72,730B; runtime M10 binding remains pending. All four transferred non-directory sequence/kernel objects2476B are now PROVEN in c6. | 0 static-row research |
| 5. Unresolved indirect consumers | 17 / 2,797 | Sixty mappings are qualified. Continue actual remaining selectors/enclosures; numeric pointers and module labels alone do not close them. Reservation00636850 is outside this1043-row ledger. Honor weapon/panel/analysis transfers. | 24–64 |
| 6. Presentation and diagnostics | 87 / 2,156 | 345 data sources,24 code sources,5 layouts and123 D10c are qualified; original setsB/C are source-decided. A1 retained42/1140B and transferred44/1000B, plusA2 sole710500/16B remain.00750008 is source-qualified in c6b;00640e70 stays OPEN in cohort1, outside nLine. No cosmetic exemption. | 40–96 |

The prior planning range **[INFERENCE]:164–360 static-RE engineer-hours**
and its per-cohort ranges are retained as historical planning inputs, not a
freshly measured estimate after source reductions. Runtime binding/qualification
and review remain additional; reader domains dominate uncertainty.
Main rejected cosmetic retention; all remaining objects require Golden values.
No M12 completion date is implied. Module batching amortizes loader
and namespace discovery; the failed reader/layout scorers are not a demonstrated
way to close the remainder mechanically.

## Owner decisions

Main's 2026-11-13 ruling **rejects `retain-PS2-cosmetic`**. Golden values are
required for **all 245 remaining OPEN objects**, cosmetic included. The
earlier proposal was not applied and supplies no exemption or M12 closure.
Continue priority 1 (calendar/fusion fallback), then 2 (battle/AI/reward),
then the remaining cohorts; presentation classification only affects order.

D10c still requires positive subsystem/object absence evidence under the
spec's existing admission rule. No failed search, diagnostic-looking label
or unresolved owner establishes absence. Cohort4 admits56 positive directory
representation replacements; this c12 run additionally admits two paired
scheduler diagnostic replacements; cohort3 adds30 Main-admitted2,021B positive
constructor/device/transport/codec/diagnostic replacements; cohort6 adds123 source-owned positive replacement contracts.
Every native replacement route still requires binding/qualification.
Never rename OPEN merely to lower counts or bypass runtime binding.

## Reproduction and evidence boundary

Private research scripts/receipts are under
`<scratch>/GoldenRE-notes/l1/l1c_*` and `l1c-*`:
`l1c_census.py`, `l1c_impact_inventory.py`, `l1c_review_groups.py`,
`l1c_triage.py`, `l1c_weather.py`, `l1c_weather_follow.py`,
`l1c_constraint_reader.py`, `l1c_weather_proof.py`, `l1c_install_decisions.py`.
Run only through the required Git Bash and fixed Python 3.12; no WSL.
Generator assertions exercised canonical unique-address coverage, duplicate
PS2 table equality, sentinel/header shape, unique/complete date-key joins and
six-partition byte totals. A Romwright worker extraction of `710016d0e0` returned
0 and showed that helper is a separate integer table getter, **not** the date
constraint interpreter; it was not misused as an extra proof.

No build, unit suite, objdiff, BBV, game, emulator, key access or `.arch` decode
was run. Metadata/source correspondence is the exercised evidence; game behavior,
constraint-interpreter equivalence and runtime L1 closure remain unverified.

## Priority 1 successor: day-flag code source

`0x005e4610` is Main-approved **PROVEN-code** from the ordinary NSO's
default/read/write/capture/restore module (`275700` through `275A10`).
The [proof](l1-day-flags-code-proof.json) supplies exact ordered Golden IDs,
defaults, ten-bit mask semantics, bit31 validity and all consumer seams.
It is not a Steam table binding: the `90C578` candidate is rejected.
Main authorizes an authored Golden-mode constant under D1 with missing Steam
corroboration explicitly recorded.

The post-write helper `2A8090` overwrites flag `1474` from nonzero live s32
at NSO `4A70C38`; final mask bit7 may therefore differ from the supplied
mask or initial defaults. Its writer lifecycle is now established in
[the sidecar receipt](l1-network-sidecar-lifecycle.json): zero-initialized
Golden-only session state, config commit and explicit load rehydration,
network-dialog transitions and operation-disable completion. No PS2 field
counterpart is identified. Native writer/reader lifecycle seams still require
implementation and runtime qualification; no zero fallback is authorized.


## Cohorts 1–2 publication

Thirty-seven PROVEN /13,964 B, three PROVEN-code /12 B and two D10c /48 B:
[boss banks](l1-boss-camera-banks-proof.json),
[role-mapped strings](l1-boss-scheduler-strings-proof.json),
[shuffle views](l1-shuffle-count-roster-proof.json),
[variant weights and reward callbacks](l1-shuffle-variants-reward-proof.json),
[task identities](l1-calendar-result-tasknames-proof.json),
[movement speed](l1-battle-movement-speed-proof.json),
[fixed status-AI command](l1-ai-status-fixed-command-proof.json),
[named battle resources](l1-battle-named-resources-proof.json),
[sky index topology](l1-battle-sky-indices-proof.json),
[localized revival bank](l1-battle-revival-message-bank-proof.json),
[expanded action descriptors](l1-battle-action-state-table-proof.json),
[expanded genus-motion map](l1-battle-motion-id-map-proof.json),
[living-party grid vectors](l1-battle-formation-grid-proof.json),
[loaded boss motion map](l1-battle-boss-motion-map-proof.json),
[formation paths](l1-battle-formation-paths-proof.json) and the
[scheduler failure replacement](l1-scheduler-missing-procedure-proof.json) and
[mutable monthly cache/local tag replacement](l1-scheduler-month-cache-proof.json),
and [signed intrinsic sine coefficients](l1-shared-sine-polynomial-proof.json).
All remain pending-seam. Calendar extent/producer gaps stay explicit in
[calendar research](l1-calendar-extent-research.json); the
[AI ownership receipt](l1-aicalc-program-ownership-research.json) is research,
not additional row admissions.


[Calendar producer research](l1-calendar-weather-producers-research.json) adds
exact reset/push/pop/signed-depth/ff and saved-real-context subcontracts,
with no new source-class admission. The3114B equal prefix and following6B
difference are not a519/520-row extent proof. Old5-record animation and11-ID
message-map roles are now explicit; their actual Golden mappings stay OPEN.


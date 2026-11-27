# L1 image literals: per-object source and retention decisions

## Scope and evidence boundary

**OBSERVED metadata:** the authoritative M2 selection is exactly
`golden_check == true`, not the report's all-object variant headline.
The accompanying [JSONL ledger](l1-image-literals.jsonl) gives one record for
every selected PS2 object, including its address, extent, known symbol,
consumer names, candidate location, layout-proof status and cheapest proof
path. It contains metadata only, not PS2 or Golden payloads or instruction
listings.

| Selection | Objects | PS2 bytes |
| --- | ---: | ---: |
| data1 | 786 | 241,555 |
| data2 | 257 | 7,873 |
| Total | **1,043** | **249,428** |
| Executable variant leads | 585 | 177,465 |
| Archive variant leads | 123 | 63,003 |
| No variant lead: 327 absent candidates and 8 partial candidates | 335 | 8,960 |

The report's 586 executable and 128 archive variants also include six
objects outside this selection, already assigned a different supplier.
Do not add them to this ledger or omit the 335 no-variant objects.

The immutable selection was supplied by `tools/golden_mode/l1-inventory.json`
in the M6b worktree. Its source `typed.jsonl` SHA-256 is
`19a89b5d028da3fac821ef01d61f2f1fda81510cf392714fe89024a30f0c60d1`;
the inventory file measured for this spec has SHA-256
`204564fcf3633e56a248cb70311d954ccb9a6e9f2d9fd61e778e0feb496a181c`.
The PS2 names, accesses and inferred kinds are provenance evidence, not
proof that every partition boundary is a real C object boundary.

## Decision meanings

The owner's per-object ruling distinguishes these decisions:

| `decision` | Meaning / permitted action |
| --- | --- |
| `PROVEN` | Structural source correspondence is established, with exact source spans, layouts and `required_seams`. This is not a runtime-binding claim. |
| `PROVEN-layout` | Main-authorized local layout correspondence: identical/proved flanking anchors, contiguous same-count object order, matched size or documented widening, and consistent alignment. Record both anchors and interval coordinates; this is not a reader-function or runtime-binding claim. |
| `PROVEN-code` | Values embedded in code, proven from Steam code and/or the NSO; the spec carries exact constants; the runtime authors them and never reads the NSO. |
| `D10a` | **INFERRED from BBV:** record the actual observation separately from the hypothesis and proposed independent check. An unperformed BBV check is not evidence. |
| `D10b` | Author from cited published facts; state the authored field mapping and observable consequences. |
| `D10c` | Positively demonstrated absence of a Golden counterpart; retain PS2 or drop, explicitly as a balance deviation. Failure of a byte search does not qualify. |
| `OPEN-retain` | Source correspondence is not proved. Preserve the unproven candidate, reason and per-object `proof_path`; temporarily retain PS2. This is not a D10c absence claim. |

The owner's final rule separates **spec class** from **runtime status**.
A `PROVEN`, `PROVEN-code` or `PROVEN-layout` row remains `runtime_status=pending-seam`, still supplies PS2,
and stays in `L1-open` until GoldenM6b supplies and tests every required
direct/inlined read seam. Only then is its runtime status `bound`.
Do not downgrade a proved source to OPEN merely because implementation
is pending, or promote a heuristic candidate because guest writes succeed.

| Scope | PROVEN | PROVEN-code | PROVEN-layout | D10a | D10b | D10c | OPEN-retain | Objects |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| Executable: 585 variants + 2 partials | 392 | 14 | 5 | 0 | 0 | 82 | 94 | 587 |
| Archive/no-lead supplement | 149 | 24 | 3 | 0 | 0 | 129 | 151 | 456 |
| Canonical merged ledger | **541** | **38** | **8** | **0** | **0** | **211** | **245** | **1,043** |

Executable scope is 179,901 PS2 bytes. The canonical merged ledger contains
541 PROVEN / 144,033 B, 38 PROVEN-code / 12,053 B, eight PROVEN-layout / 240 B,
211 D10c / 78,039 B and 245 OPEN-retain / 15,063 B. These totals are
recomputed from the restored merged JSONL. Canonical and triage each contain
1,043 unique rows; no parent source class regresses. Source-qualified rows
and D10c replacement obligations remain pending-seam, not runtime bound.

Cohorts 1–2 add thirty-seven PROVEN / 13,964 B, three PROVEN-code / 12 B
and two D10c / 48 B this run: six mixed boss-camera banks, thirty-one typed
string/table/field rows, three signed code coefficients and two replaced diagnostics.
The 456-object supplement is merged by address, not concatenated.
Scope membership follows that456-address inventory, not mutable candidate-container labels.
See [impact triage](l1-image-literals-triage.md) for complete before/after
counts and the disjoint remainders. These source counts do not close runtime L1.

A native consumer may read a compiled PS2 static array rather than guest
memory. Installing bytes at its guest address alone then does not change
that consumer. A source proof and a functioning consumer route are distinct
obligations: record any required Golden-only seam explicitly; do not claim
that a successful guest-memory copy proves changed gameplay.

## Cohort 5: typed enclosing objects and actual pointer loaders

[Typed-bank proof](l1-c5-typed-enclosing-banks-proof.json),
[weapon-bank proof](l1-c5-weapon-message-bank-proof.json),
[rank-up proof](l1-c5-rankup-message-bank-proof.json),
[camera-name proof](l1-c5-camera-state-labels-proof.json),
[action/encounter proof](l1-c5-action-encounter-labels-proof.json),
[AI namespace proof](l1-c5-ai-condition-target-labels-proof.json),
[result/shuffle proof](l1-c5-result-shuffle-resource-identities-proof.json),
[native cue-bank proof](l1-c5-model-sound-cue-bank-proof.json) and
[77-row review](l1-c5-indirect-review.json): **61 PROVEN /
57,223 PS2 partition bytes**; **16 OPEN / 2,043 B** remain.
PROVEN-code, PROVEN-layout and D10a/b/c additions are zero.
All admissions are **runtime pending-seam**, not runtime L1 closure.
Cohort-5 proof-document `source_sha256` receipts hash **committed Git blob
bytes (UTF-8/LF)**, not private Windows CRLF copies. Publication checks these
receipts as well as 1,043 unique canonical rows, nonregression and ownership.
The archived `0061646f` duplicate carries the same admitted native source.
Publication also requires canonical/triage/archive class coherence and
identical Golden source values for owned archive duplicates.

Main transferred `00626172` / 512 B and `00626372` / 398 B to cohort 6,
and `00626fe0` / 623 B from cohort 6 to this writer. Main subsequently
transferred weapon-bank rows `00645240` / 3,877 B and `0064732e` / 802 B.
Current scope is **77 / 59,266 B**, rather than the original 76 / 54,874 B.
Other writers' proofs and shared summary totals are not changed here.

The largest partitions are not message/AI tables inferred from numeric words:

- Five partitions `00600068`, `00600114`, `00600194`, `0060046c`,
  `0060078a` / **31,966 B** are interior views of the already independently
  qualified 768 x 52-byte camera-preset bank. Use bank-relative positions,
  including partial records; native readers need the complete 39,936-byte bank.
  The apparent talk pointer at `649ffc` is encoded message payload, not a
  pointer field.
- `005dd86c` / **4,364 B** encloses the 99 x 44-byte party HP/SP table:
  **retain four leading bytes, copy 4,356 source bytes, retain four trailing
  bytes**. Actual physical PS2 base is `5dd870`, Golden Steam base `90cae0`.
  Both logical readers use a one-row negative bias. Do not copy candidate
  `90cadc` or interpret preceding experience-table storage as an HP/SP row.
- `00626fe0` and `0062724f` / **8,064 B** form the PS2 analysis-model
  placement bank, not message data. Golden expands 336 to **368 x 24-byte**
  records at Steam `a44710`; a separately qualified 24-byte fallback is at
  `afd758`. The split is byte 623, inside row 25's padding. Native readers
  require all 8,832 Golden bytes and the actual genus/actor selector and
  Golden projection transform, not PS2 pixel equations on HD coordinates.
- `00632e79` / **2,247 B** is inside the 22-script effect-camera bank:
  copy **2,239 B**, retain eight PS2 alignment bytes. Real PS2 base is
  `632240`; Steam `aeb6e0`, Switch `6fe540`. The next independent Golden
  object is a byte-remap table, not padding. Apparent movie/effect references
  are the literal suffix `y.c` in two source filenames.

Fifteen more admissions are complete loader-consumed filename literals:
nine persistent help member names, with the actual nine-slot installer and
the nested `data_e.cpk::init.bin::init/datMsg.pak` member receipts; and six
battle-table paths, whose filename pointers are proved in **all four**
Golden eight-descriptor variants. Filename proofs do not prove message
handle normalization, AI program-block equivalence or packed payload layouts.
`required_seams` records these distinctions and real producer/state contracts.

The weapon-bank successor upgrades **ten partitions / 8,650 B** by actual
resource publication, not a byte-window fit. Descriptor `764fe0` supplies
`facility/msg_weapon.bmd` within `facility/weapon.arc`; the common shop task
requests/waits/copies it and publishes the real parsed message slot. The
Steam English file has **53 entries / 15,057 declared bytes**, plus fifteen
archive-padding bytes; the PS2 bank has **32 entries / 11,421 bytes**.
Every old label joins uniquely with matching type/page count:
old IDs 0–26 map to 19–45, 27/28 to 51/52, and 29–31 to 15–17.
Golden FTD selectors already contain Golden IDs and must not be translated
twice. Bind the complete relocation-aware bank and its actual actor/date/
flag/inventory/material/parameter/slot lifetime contracts; never copy
expanded fragments over PS2 image storage.

`004c26a0`'s apparent `64732e` reference is positively rejected: it compares
the packed ASCII integer `.sd` against input bytes, not a BMD pointer.
The earlier review additionally rejected **41 address-looking words across
14 objects** as real string interiors, plus the `649ffc` glyph false pointer.
The rank-up successor additionally qualifies `0063696e` / **194 B**:
179 file bytes and fifteen retained PS2 alignment bytes. The actual category
task selects a complete four-message MSG1 bank through **nine** positional
locale pointers at Steam `914910` / Switch `796598`; every full bank and
ordered message identity is qualified. English slot 1 is the unchanged
465-byte source at `aab730`. Preserve actual locale, category/increment,
score/rank state, three name substitutions, slot lifecycle and Golden
notifications; no arbitrary localized offset copy. Code-derived reader
constants are explicitly marked PROVEN-code components, not extra canonical
row admissions. The 286-byte `636850` prefix is already an identical
`golden-exe` supplier outside the L1 selection and is not counted again.

The two largest remaining alleged geometry fragments, `0061646f` / 754 B
and `00617461` / 1,293 B, are now positively enclosed in **model sound-cue
grids**, not item-name or mesh data. The actual PS2 bank is
`60ac00..618ac0`: two seasons, eleven actors, 27 x 8 x 12-byte cells per
actor. Actual pointer publication and the typed `mdlSE` consumer are
recovered. Golden's actual cell consumer uses 32 rows and gets the bank
from packed model-resource entry 1. Actor/season/costume identity and motion
row correspondence remain unproved, so these stay OPEN rather than copying
repeated disabled cells from an arbitrary `bc008` file.

Five camera labels / **80 B** now have their actual typed enclosing object:
PS2 `5f74c0`, 51 x 20-byte descriptors; Golden Steam `afa280` /
Switch `70a8f0`, 55 x 32-byte descriptors. Actual state setters and per-frame
readers select encounter overrides or index that table, then consume its
flags and initialization/update callbacks. Name fields widen from +16 to
+24. Selected states 5/9/32/39/43 establish the five exact Golden names.
`ESCAPE_MES` also has an action-descriptor reference: literal identity alone
would not distinguish its role. Do not widen/copy all 51 old records:
Golden inserts bike pursuit at 49, shifts down/defense to 50/51, and adds
three more states. This is source proof for label fields, not callback-code
equivalence or runtime closure; real state/action/override producers remain
in the required seams.

Seven action/encounter names / **128 B** now have separate typed domains.
Action records are PS2 `5f6e20` / **38 x 12**, Golden Steam `a24c10` /
Switch `709a40` / **42 x 24**; the old bank continues through DEAD/EXIT/TEST,
not just END HOME. Golden inserts UNION/BIKE_PURSUIT and ANALYZESKILL/YELL.
The selected old states 3/17/28/34 map to 3/19/32/38. Encounter records are
PS2 `5f7180` / **20 x 12**, Golden Steam `a303f0` / Switch `71f8a0` /
**21 x 24**; inserted REVIVALRISE shifts old 12/14 to 13/15 while 6 stays 6.
Actual native allocation, immediate/pending action transition, action-list
update/release, encounter setter/update/reset and real float frame clock
establish the producer/consumer contracts. Names widen +8 to +16; callbacks
are not byte-copied or admitted as behaviorally equivalent by their labels.

Eight AI names / **136 B** now have their correct independent namespaces.
The condition bank starts at PS2 `609850`, **78 x 12** callback/name/metadata
records; `6098bc` is only callback slot 9, not a bank base. Golden has
**82 x 24** at Steam `af97b0` / Switch `70a0c8`, appending four operators.
The target-selector bank is PS2 `609ce0`, **40 x 8**, versus Golden
**40 x 16** at Steam `af9f60` / Switch `709e48`. The following NULL triple is
condition opcode 0, not target entry 40 or padding. Banks reverse physical
order between the two Golden images. Actual ordinary Steam loaders and the
native AI core consume top-byte opcodes and low-24-bit operands with the
correct distinct strides. Real unit-indexed AI records, condition rows,
candidate/status/element state, RNG and published target handles/count are
required producers/consumers. This is not a speculative AI module assignment
or callback-code admission based on names.

Five result/shuffle resource identities / **160 B** now follow actual GP
pointer loaders, calendar-derived variants and native package/member
consumers. `simple_w/s.bin` become `simple_w/s.arc`; the two old shuffle
paths become one `shuffle.arc`, while the actual date variant still selects
three sprite-member names. Complete four-member simple and eighteen-member
shuffle PAK-v2 receipts are captured for English and locale-neutral sources.
No filename change authorizes reinterpreting native assets as PS2 blobs.

`629150` is explicitly a **native ordered message-bank** admission, **not a
scalar filename replacement**: old `br_msg.bin` entries 0/1 correspond to
`sysmsg0_br.bmd` / `sysmsg1_br.bmd` in the selected simple ARC, copied into
actual parent buffers +948/+950, passed in the same order to persona
level-up, and freed with the parent. Paired call arguments plus the uncommon
party-5/RNG-3/+468 branch independently establish that consumer role.
The complete Golden directories have 29/11 entries; do not assume old IDs
remain unchanged: the observed old notice 23 is Golden 24. Real calendar,
locale, readiness, copy/adoption, message/child state and lifetime bindings
are required; no invented `br_msg` path or unused shadow bank is accepted.

[Native sound census repair](l1-c5-model-sound-census-trailer-receipt.json):
the three declared members of `bc007.se`, `bc007_c1.se` and `bc008.se` were
hidden by a 16-byte nonzero opaque trailer. The Main-authorized RE parser fix
accepts at most sixteen opaque bytes after otherwise valid little-endian
PAK-v2 members and records their bytes/hash. Fresh targeted census recognizes
three more containers and nine children, including the actual 3,072-byte
`bc007_c1.bin`; `bc008_c1.se` remains three children. Fifteen parser and seven
inventory fixture tests pass. Reuse a fresh census/checkpoint output for this
parser revision. This repairs candidate coverage, not actor/row identity:
neither repeated disabled cells nor equal active-cell counts admit a row.

[Native actor-7 cue-bank proof](l1-c5-model-sound-cue-bank-proof.json)
admits `0061646f` / 754 B. The real unit packet supplies the character ID
without a +1 bias; the only model-ID remap is positively enemy-specific.
Actual packed-model filename selection, PAC entry 0 (SE), then SE entry 1
(cue grid) identify `bc007_c1.bin` for the default season-1 case.
The complete native bank is **32 × 8 × 12**, not the old 27-row bank:
ordinary Steam `.shared` `c6a30` and NSO `6a3f0` establish the real
3 × 34 generic-motion map and animation-row producer. This admission
requires a **whole native logical-bank and actual consumer cutover**,
not a 754-byte offset projection or guessed row-number correspondence.
Real equipment/date/mode, selected model/cache/request, native animation,
audio/RNG/frame state, buffer sharing and teardown remain required seams.
`00617461` crosses actor 8/9 and remains OPEN; missing `bc009` paths
and repeated disabled cells do not prove its actor-9 part absent.

Unproved label/contour mappings also remain OPEN; no no-reference result
is an absence proof.

## Presentation cohort 6: battle-camera banks

[The camera source receipt](l1-camera-banks-proof.json) upgrades nine objects /
**38,152 PS2 bytes**: eight complete script banks and the child-unit preset-bank
partition. Ordinary Steam selectors and independent Switch selectors establish
character-ID/random-bit/mode roles and common typed consumers, not visual or
byte similarity alone. Eight script banks have unchanged live Golden values:
copy 244-byte scripts, retain each eight-byte PS2 partition tail. Golden bytes
after those banks are unrelated strings, counters or floats, not camera data.

The preset selector uses three 52-byte records per character, fixed slot 2 or
random slot 0/1. The qualified complete 256-ID interval is 39,936 bytes at Steam
`.xpdata` RVA `0xA25000`, independently equal to Switch `0x710070AFD0`.
The selected 7,832-byte partition ends **inside** record 150; its last 32 bytes
are live, not padding. Native whole-bank readers must use the complete source;
guest installation must not write outside the selected partition. All direct,
inlined and native selectors/consumers are listed in `required_seams`.

This batch is **9 PROVEN / 0 PROVEN-layout / 0 D10a/b/c**, runtime pending-seam.
Current cohort totals and ownership transfers are recorded below. Four camera scalar rows
remain explicitly OPEN; the bank proofs do not close the whole camera module.
No PS2-cosmetic retention policy or runtime L1-open reduction is applied.

The ninth bank `0x00633740` is camera data in the inferred `btlVoice` translation
unit, not audio data. `btlCameraSetRandomPartyPose` (`0x00228A80`), Steam
`0x3B880` and Switch `0x7100007030` agree on the no-target gate, ID-minus-two,
two-script random selection, option 8 and geometry 3. Its alternate boss-preset
branch is not qualified by this bank proof. Copy 4,392 bytes; retain eight.

## Presentation cohort 6: persona-voice selector bank

[The persona-voice receipt](l1-persona-voice-proof.json) upgrades `0x00624B22`,
**1,022 PS2 bytes**, as the +2 partition of the complete 256 x 4 primary/secondary
selector bank at `0x00624B20`. Steam primary/secondary readers `0xBC830` /
`0xBC910` and Switch `0x7100062FD0` / `0x7100063060` identify the same halfword
fields and persona/character context. The complete source is Steam `.xpdata`
RVA `0xA305F0`, 1,024 bytes, equal to Switch `0x710071FA98`; 22 records change.
Selected guest installation is **1,022 bytes from RVA `0xA305F2`**, without tail.

Golden first normalizes persona IDs through the loaded kind table (stride 68).
Kinds 7 and 8 both subtract `0x700`; kind 9 subtracts `0x800`. Do not replace
this with `kind*256` or an unchecked low-byte mask. Golden secondary control
also removes PS2's character-4 special override. The kind-data dependency,
both consumers/inlines/native copies, and sound-bank/slot namespace loading
are explicit runtime seams; this source proof does not certify playback.

Cohort 6 split-baseline upgrades: **81 PROVEN / 47,484 PS2 B**; **0 PROVEN-code /
0 PROVEN-layout / 0 D10a/b/c**. Remaining: **502 OPEN / 17,561 B**.
Current ownership is **583 objects / 65,045 B**. Main transferred the two
panel-bank interior rows and four sequence/debug rows into c6; `00626FE0`
transferred out to c5. Weapon-BMD rows belonged to c3, not this denominator.
All source proofs remain pending-seam. Global totals belong to the global owner.

## Presentation cohort 6A: complete persona-channel preset bank

[The persona-panel preset proof](l1-persona-panel-presets-proof.json) adds
four PROVEN objects / 832 PS2 B after the split: `005E4880`, `005E4950`,
`005E4A20` and `005E4AF0`. The complete named bank has six PS2 presets and
seven Golden presets, each ten 20-byte channel records. Both Golden
architectures agree on every complete 200-byte source and on actual
selector arms, field loads and duration scaling. Preserve eight zero tail
bytes per old selected object.

The first two corrected sources are byte-identical to PS2 but positively
qualified by typed consumers; cosmetic retention was not used as proof.
Earlier M2 candidates for three objects were wrong windows. Setup, initial
close-preset reads, comparison-reset open-preset reads and constant-folded/
native copies are explicit runtime seams. Untyped decompiler casts do not
change the raw float timing fields established by LDP/FMUL instructions.
The second-split baseline was **199 OPEN / 6,408 B**. Main assigned 98 /
2,816 B to c6b as A2; retained A1 now has **96 OPEN / 2,424 B**.
Sets B/C and global source totals are not updated in this subsection.

## Presentation cohort 6A: complete effect behavior descriptor bank

[The effect-behavior proof](l1-effect-behavior-bank-proof.json) adds one
PROVEN object / 180 PS2 B: `00713D5C` is +12 into the complete twelve-kind
descriptor bank `00713D50`, not 45 independent integer coefficients.
Three independent typed readers establish update callback, scale callback,
per-primary state bytes and parameter bytes. PS2 records are 16 bytes;
Golden records are 24 bytes at NSO `86F808` and Steam `.xpdata` `AE98F0`.
All twelve ordered size pairs agree; positive allocator/update/scale
control flow establishes identity rather than the source filename alone.

The selected 180 bytes are all live, with no retained tail. Bind the whole
normalized Golden bank, not a raw 288-byte guest overwrite. Callback code,
88-byte samples versus PS2's 32-byte samples, 80-byte versus 48-byte
emitter headers and changed update timing/state are explicit runtime seams.
This is source proof, not a claim that old effect callbacks remain valid.

## Presentation cohort 6A: flash factory and named geometry banks

[The flash resource proof](l1-flash-factory-topology-proof.json) adds four
PROVEN objects / 304 PS2 B: `00714028`, `007133A0`, `00713408`, `00713CE0`.
The complete Golden factory has ten 56-byte records: six native callbacks
and parameter size. Direct/source/optional-parent constructors, destroy,
advance and post-age update independently establish the field roles.
Golden kind 9 supplies real callbacks and 264 parameter bytes; do not
invent an old descriptor from the following `effPolygonWind.c` filename.
The old guard still admits kind 9; this proof does not change PS2 behavior.

The two paired geometry constructors supply exactly nine and six u16
indices for three/five and two/four triangle/vertex templates. Both common
geometry builders establish `triangles * 3` reads, not a guessed padding
window. The unit-scale quad is independently selected by corresponding
instance initializers. Preserve 4/14/12/0 old trailing zero bytes,
respectively: 274 selected bytes are live and 30 are padding.

Callable normalization, all shared topology/scale readers and native copies,
the new factory kind and Golden floating-age lifecycle remain runtime seams.
The homogeneous scalar pair `00713D18` and angle constant `007611C0` remain
OPEN: they are not qualified merely by these neighboring resources.

## Presentation cohort 6A1: thunder, ring and wind factory banks

[The three-factory proof](l1-thunder-ring-wind-factories-proof.json) adds
four PROVEN objects / 320 PS2 B: `00713E84`, `00713F24`, `00714148`,
and shared strip topology `00713330`. Exact kind bounds, parameter aliases,
allocation/init/copy flow and independent lifecycle readers establish
five/four/four-kind banks. Five-callback descriptors widen 24 to 48 bytes;
six-callback wind descriptors widen 28 to 56 bytes. Steam corroborates
complete ordered size vectors; ISA-specific callback values are normalized,
not compared as bytes.

The common four-column strip builders establish exactly eighteen u16
indices, shared by two ring and three wind producers. Preserve twelve old
topology padding bytes and eight thunder alignment bytes. Ring/wind selected
descriptor partitions are entirely live.

Per-family lifecycle is not interchangeable: Golden thunder still increments
integer age; ring/wind use floating age with actual timing getter times 0.5.
Thunder destruction receives the instance; ring destruction receives its
child. All real callbacks, layout/ABI differences, shared readers and native
copies remain runtime seams. No generic callback cast or timing shim is
authorized by these source proofs.

## Presentation cohort 6A1: item layouts replaced by native mode code

[The item-menu native-code proof](l1-item-menu-native-code-proof.json)
upgrades `005E5BF0` / 848 B to **PROVEN-code**. The actual old subsystem is
`cmpItem.c`, not the inferred title/radar name. Initialization, the held-item
id/quantity list, 84-cell 7x12 grid, nLine 11–13 and the single/all-recipient
use graph identify the Golden item controller. Camp row zero was a rejected
skill-menu lead; the qualified item controller is independently row one.

Golden replaces the thirty 28-byte templates with twelve native mode entries.
All 46 ordered channel operations, 122 direct written bytes and external
effect order/input snapshots agree across ordinary Steam `.shared` and NSO.
The proof explicitly handles the specialized Steam tab helper and selection
snapshot wrapper; changed/no-op guards are not ignored. The full real
renderer source, grouped `cmpItemSortTable.ctd` dependency, helper contracts
and new kind-7 flow remain sealed source inputs/runtime requirements.

Neither old layout coordinates/masks nor an empty mode-only renderer are
authorized. All mode/controller/draw/inlined paths must consume actual
Golden values. Source-only contribution so far: **94 PROVEN / 49,120 B plus
one PROVEN-code / 848 B**; layout/D10 classes zero. This subsection does not
update any sibling or global totals, and runtime remains pending-seam.

## Presentation cohort 6A1: persona named-task banks

[The named-task bank proof](l1-persona-named-task-bank-proof.json) adds
`005E4EB0` and `005E4F30`: two PROVEN objects / 64 PS2 B. Actual model-load
and level-up task producers, priority/parent setup, registered update and
destructor callbacks, and owned-resource cleanup independently establish
the named identity fields. Matching strings or source-module labels alone
would not qualify these rows.

Copy 22 and 18 live Golden bytes respectively; retain ten and fourteen old
zero-tail bytes. Steam's adjacent model-format string is not part of the
display-task name. Expanded work/payload layouts and platform-specific
constructor priorities are explicitly not equated with PS2. The separate
deferred-free task remains OPEN: this proof makes no removal claim.

Source-only contribution: **96 PROVEN / 49,184 B plus one PROVEN-code /
848 B**; layout/D10 classes zero. Retained A1 has 94 OPEN / 2,360 B.
All actual create/lookup/destroy/inlined paths and lifecycle binding remain
runtime pending-seam; no sibling or global totals are edited here.

## Presentation cohort 6A1: shared flash math and mutable staging

[The shared-math/state proof](l1-flash-shared-math-state-proof.json)
adds two PROVEN objects / 12 B: `00713D18` and `007611C0`. The already
qualified rotating-flash factory and three-triangle/five-vertex producer
identify actual corresponding advance callbacks. Their shared staging
stores/vertex reads and random-birth-angle multiply establish source
fields independently of common zero/one/full-turn byte values.

`00713D18` is the +8 suffix of a MUTABLE four-float staging workspace,
not an immutable UV pair. Native `86FAB0` and Steam `AD9980` positively
implement the corresponding working buffer; all three images initialize
it to `(0,0,0,1)`. Runtime must initialize once and preserve actual writes,
not freeze/reset z/w at reads. VU-emulation macros still exist in Golden.

The full-turn scalar is exact `40C90FDB` at Steam `965054`, corroborated
by the NSO immediate and actual angle publication. The old GP alias is
resolved, not guessed. All 23 staging and 11 angle consumers/native copies
remain explicit runtime seams; neither RNG nor timing equivalence is claimed.

Source-only contribution: **98 PROVEN / 49,196 B plus one PROVEN-code /
848 B**; layout/D10 classes zero. Retained A1 has 92 OPEN / 2,348 B.
Runtime remains pending-seam; no sibling or global counts are changed.

## Presentation cohort 6A1: UI and field-render task identity banks

[The task-bank proof](l1-ui-render-task-names-proof.json) adds three
PROVEN objects / 80 B: `005F1950`, `005F1E08` and `0063E918`.
Each bank has an actual scheduler producer and registered lifetime:
singleton player-icon rendering, allocated field-noise rendering, and
name-entry create/existence/hierarchy-destroy. No missing Steam reader or
unchanged label is treated as a proof.

The complete name-entry identity bank has three Golden variants. Signed
selector equal to four uses `nameEntry_ck`; greater than or equal to five
uses `nameEntry_figs`; otherwise it uses `nameEntry`. Actual NSO constructors
and both images' existence/destruction paths share this dispatch.
Alternate native names do not authorize overwriting old literal padding.

The other two render tasks preserve actual singleton-versus-allocated
ownership. Their Golden work sizes and float timing differ from PS2;
the player-icon destructor really is a RET. The proof does not invent
cleanup or authorize old rendering as a name-only fallback.

Source-only contribution: **101 PROVEN / 49,276 B plus one PROVEN-code /
848 B**; layout/D10 classes zero. Retained A1 has 89 OPEN / 2,268 B.
Every actual callback/draw/native-copy route remains runtime pending-seam.

## Presentation cohort 6A1: complete effect allocation-header bank

[The effect-header proof](l1-effect-misc-header-proof.json) adds two
PROVEN objects / 40 B: `00713CC0` and `00714508`. All seven actual old uses
are allocation-site file/line metadata, not arbitrary text resources.
The corresponding particle/object constructors, count/default/copy graph
and actual owning-TU diagnostics select the real full Golden header value
at NSO `60AF99`. Five old particle sites and two object sites are covered;
two object producers now share a real allocator.

There are two raw Golden include spellings. Both positively normalize to
the same header, but that alone does not select a raw value: the actual
corresponding producers use `60AF99`, not the other spelling at `5FACFD`.
The source-site line is Golden `19F`, not old `171`. Stage the full long
path through the diagnostic namespace without overrunning short guest
objects or their twenty combined padding bytes. The release metadata sink
really is a RET; no tracking/logging is invented.

This independently closes two earlier diagnostic-review exclusions.
Source-only contribution: **103 PROVEN / 49,316 B plus one PROVEN-code /
848 B**; layout/D10 classes zero. Retained A1 has 87 OPEN / 2,228 B.
All native/inlined allocation-source routes remain runtime pending-seam;
particle work, emission and rendering are not qualified by a header value.

## Presentation cohort 6A1R: complete camp-root sprite-key bank

[The root-key proof](l1-camp-root-sprite-key-bank-proof.json) upgrades
`005E5B60` / 88 B to PROVEN. The actual subsystem is the camp root,
despite old inferred title names. Its initializer caches three groups of
seven sprites; the indirect renderer separately selects unselected,
selected and supplementary selected-option groups. Those actual roles,
the flag-1F visibility gate and parent camp dispatcher identify the bank.

Golden has three eight-entry u16 key tables, not the old 21-u32 format.
NSO sources are `62A840`, `62A940`, `62A910`; ordinary Steam indexed
getters corroborate every key at `90C858`, `90C868`, `90C848`.
Steam physical order is detail/unselected/selected, not logical group order.
The actual registered root atlas is `init/c_main01x2.spr`.

Use all 24 real keys and the actual eight-option producer/renderer.
Do not memcpy the 48 source bytes over old u32s, truncate the extra option,
use visible-row indices as logical options, or overwrite four old tail
bytes. This root dependency does not close shared pathname `005E5810`
for its other consumers. Real resource, work, timing and draw bindings
remain explicit pending seams; old seven-option rendering is not a fallback.

After Main's third split, c6c owns the frozen 44-row / 1,000-B A1C packet.
This writer retains **42 OPEN / 1,140 B** in A1R. Own cumulative source
contribution is **104 PROVEN / 49,404 B plus one PROVEN-code / 848 B**;
layout/D10 classes zero. No sibling/global totals or runtime reduction
are claimed.

## Presentation cohort 6A1R: narrow rounded-frame native construction

[The narrow-frame code proof](l1-narrow-rounded-frame-code-proof.json)
upgrades `0064E320` / 144 B to PROVEN-code. The complete old eighteen-float2
seed bank is replaced by actual native constructor `219960`, not an
unproved equal-looking point cloud or an old template retained as cosmetic data.

Static source reconstruction pairs all 36 ordered local-coordinate fields;
an independent typed def-use trace of the actual bounded A64 stack writes
pairs all 54 emitted XYZ fields and eighteen uniform-color field expressions.
Every field's actual writer PC is recorded. These are parameterized producer
and packet roles, not visual similarity. Rational normalization establishes
field correspondence, not universal IEEE or whole-function equivalence.

Author actual Golden construction, signed dimension/alpha guards, f32
ordering, origin/depth/color and real 24-byte vertex packet/pipeline.
Do not cast the old scale/rotation/queue API to the new ABI or preserve it
as a fallback. All real caller arguments and native/translated/inlined
routes need binding. The different gradient producer `219C70` is not
accepted as the wide-frame counterpart; `0064E3B0` and UV `0064E440`
remain OPEN.

Own cumulative source contribution: **104 PROVEN / 49,404 B plus two
PROVEN-code / 992 B**; layout/D10 classes zero. Retained A1R has
**41 OPEN / 996 B**. Every new route remains runtime pending-seam.












## Presentation cohort 6: named tasks and sequence registry

[The sequence presentation proof](l1-sequence-presentation-proof.json) qualifies
the clear-OT and debug-menu task names through actual scheduler producer and
lookup/teardown roles, plus the complete logical callback registry containing
the selected `005DCC6C` partition. The actual PS2 base is `005DCC20`: 31
80-byte records, with the selected partition starting at row-zero destroy.
Golden registers 22 96-byte records; all complete name/flag/padding prefixes
agree independently between Steam and Switch. Registered counts, indexed
getters, query/destroy/start dispatch and pending-payload lifecycle establish
the source correspondence, not module labels or pointer-list appearance.

This is a semantic registry migration, not an offset-preserving guest copy.
Golden IDs are reordered; no positional or label-only ID translation is
authorized. Function pointers require callable normalization and qualified
controller implementations. Golden work layout, float delay/timing and
zero-size delayed-allocation behavior also differ. These are explicit
runtime seams, not claims that a PS2 callback or integer countdown is valid.


## Presentation cohort 6: typed source-file diagnostic resources

[The diagnostic namespace proof](l1-diagnostic-source-proof.json) reviews
83 owned candidates (excluding the earlier manager proof), qualifying
**64 / 1,448 PS2 B** and leaving **19 OPEN**. This includes Main's newly
assigned `005DCB28` / `sequence.c`; the other three transferred sequence/debug
objects are not inherited by its filename proof.

These are full **source-file value** correspondences, not generic module labels.
Every enumerated PS2 filename use has a typed file/line argument role; each
qualified basename has exactly one full Golden source path, actually loaded
into argument zero and passed without intervening clobber/branch/call to a
positively identified diagnostic/allocation-site sink. All complete NUL spans
and hashes are checked. The proof pins 4,207 Golden bytes from Switch 1.2;
it does not pretend those are Steam PC path strings or gameplay-function matches.

Pass each **entire Golden path** through all direct/inlined/native filename
routes; do not retain the PS2 basename as a cosmetic policy or overwrite a
short guest literal with the expanded path. The 458 trailing zero PS2 bytes
are outside the typed NUL-terminated filename value and remain unchanged.
The advantage-panel inline helper uses and `eff2D.c`'s exact GP-relative alias
are recovered explicitly rather than mistaken for missing consumers.

Actual release behavior is also pinned: `7100349960` and `710039D420` are
single-RET sinks, not logging or aborting implementations. `710039E630`
records a 64-bit file pointer and 32-bit line. Do not invent those side
effects for the no-ops. Ambiguous header include paths and unresolved
source/sink cases stay OPEN; no failed search becomes D10c.

## Presentation cohort 6: complete battle-panel message bank

[The panel-bank proof](l1-ui-panel-bank-proof.json) qualifies the complete
PS2 MSG1 resource `006253B0..006264FB`, **4,427 bytes / 47 entries**.
Its selected partitions `006253B0`, `00626172`, `00626372` account for
**4,394 PS2 bytes**, including five alignment bytes after the logical file.
Main authorized the latter two rows' transfer from c5; `00626FE0` stays with c5.

PS2 `btlUiInitComponents` (`00200FD0`), ordinary Steam `DB800` and Switch
`71000834A0` wire the component/owner graph and install this message bank.
Golden reads an eight-locale pointer table (Steam `.sdata` RVA `90B240`,
Switch `7100733878`). All eight complete MSG1 files independently match
between the two Golden images. English slot 1 is **8,123 bytes / 89 entries**
at Steam `.xpdata` RVA `A38400`, SHA-256
`980fd69e3318c05ff77c538b04460a9c302b081f26933ea0c53b11a11afff88b`.
All first 47 ordered kind/name directory identities remain; 42 entries append.
Repeated/truncated names are corroboration, not independently unique keys.

This is **whole-resource semantic replacement**, not three image copies.
M2's `A38534` candidate starts `0x134` bytes inside the real English file.
Use the complete selected-locale file, its 89-entry namespace, encoding and
relocation domain; qualify native pointer widening and every panel reader.
Never overwrite the old guest partitions with the expanded file. Retain the
five PS2 tail bytes of `00626372`; its remaining 393 bytes are in the file.

The same structurally paired MSG validator/installer/constructor proves
`0063BE10` / 16 B as the manager filename diagnostic. Golden Switch passes
the **entire 69-byte path** at `71005F4F74` to assertion/allocation-site sinks.
Resolve every filename argument semantically; do not truncate the Golden path
or write 69 bytes over the 16-byte PS2 object. This is an explicitly qualified
Switch source, not a claimed Steam diagnostic string. The archive supplement
is synchronized for this row. These four upgrades are source-only PROVEN.

## Fusion weather: module-level structural proof

Six partitions totaling **5,269 PS2 bytes** are now PROVEN as parts of
two duplicate dated fusion-rule arrays: `0x0063FCA0`, `0x0063FFF6`,
`0x0064006E`, `0x00640131`, `0x006432B0`, `0x00643825`.
The [structural receipt](l1-fusion-weather-proof.json) records the complete
source, row-index join, field layout, consumers and required semantic seams.

The PS2 inline date lookup starts at row 1 and stops at signed `-1/-1`.
Both complete arrays have 94 stride-28 records (dummy, 92 dates, sentinel)
and identical 2,632-byte hashes, followed by eight alignment bytes each.
This resolves the former mid-record partition/extent blocker.

Switch `0x7100173720` resolves `WeatherBonusTbl.ftd` from the facility table
bank and returns payload+16 plus the header count. Its fusion consumer
`0x7100169790` splits the date, scans stride-28 month/day records, returns
the constraint header at +2 and consumes five four-byte effects at +8.
Kind zero is absent; weight 100 is unconditional; other weights enter
weighted candidates, including weight zero. RNG is called **only for positive
total weight**, not merely when such candidates exist: `169BD4/169BD8`
guards the actual call at `169BE0`. A zero total sets roll zero at `169C08`;
strict roll < cumulative never selects a zero-weight entry. Return only the
unconditional effects (or zero), without consuming RNG. Positive draws use
FCVTZU then SXTH at `169BFC/169C00`. This rule applies to both dated and
phase-fallback branches. Ordinary fusion callers provide today/tomorrow
and result-growth contexts. This module/loader/typed-consumer chain,
not a per-function signature or a numeric byte vote, establishes identity.

Steam's complete payload is **128 x 28 = 3,584 bytes**, excluding the
16-byte FTD header and 16 trailing member bytes. All 94 PS2 keys join
uniquely, 34 dates are added, and 26 existing records differ. Bind this
**one expanded logical source** for both logic and display; never stitch
the six candidate windows, overwrite guest arrays or reuse PS2 row indices.
The full required seam set includes `00303610`, `00303DE0`, `0030C3C0`,
`00313B50`, `00313D20`, `0032E570` and their inlines/native copies.
The phase-indexed 20-byte fallback table remains a distinct unproved source.
No game execution, behavioral equivalence or runtime binding is claimed.


## Executable structural proofs and required seams

Reader addresses below are RVAs for the pinned Steam executable; Switch
addresses include the `0x7100000000` image base. Only ordinary `.shared`
was decoded. Source hashes and field-level layouts are in the JSONL.

| PS2 object | Corresponding consumers and source | Installation contract |
| --- | --- | --- |
| `0x005E45E0`, 48B, twelve month lengths | PS2 `calGetMonthLength` (`0x001104A0`) and Steam `0x13BA90` both read signed `(month-1)%12` at stride 4. Switch `0x71000C9F70` independently reads `0x710062F12C`. Steam source `.sdata` RVA `0x90C078`, 48B. | Twelve little-endian s32 values; February becomes 29. Cover all five recorded PS2 calendar readers and their inlines. Golden year normalization must agree with 366 days: Switch split reader `0x71000C9FB0` uses `0x16E`, unlike PS2's 365. A month-only write does not prove complete calendar behavior. |
| `0x0063CB50`, 1,072B, packed-key action table | PS2 `mtMiscDispatchKeyAction` (`0x00298250`), Steam `0x20F540`, and Switch `0x710014D5A0` pack `(high<<16)|low`, compare ordered keys, read action at +4, and default to action 3. Steam has **192** stride-8 records at `.xpdata` RVA **`0xB089C0`**; PS2 has 134. Steam helper `0x20F4F0` and inline `0x1F0632` corroborate the source. | **Keyed expansion, not image copy.** Bind the full sealed 1,536B table through the PS2 owner's Golden-only lookup; first match wins, missing action is 3. Preserve the PS2-mode 134-row path. The Golden inline is a witness, not a separately claimed PS2 read. M2's `0xB08A10` probe starts ten rows late and is not an admitted coordinate. |
| `0x0060A410`, 1,072B, sky xyz vertices | PS2 sky setup allocates grid `(1,118,89)`, copies 89 xyz triples, scales UV y by 8 and fills white color. Steam `0xB8430..0xB86D7` repeats those operations with 11 unrolled groups of eight vertices plus one; source `.xpdata` RVA `0xA2EED0`. The loaded pointer is base+4, with the first x read at pointer-4. Switch `0x7100060238..0x710006029C` corroborates xyz reads. | Prefix-copy **1,068B**, 89 stride-12 f32 triples; retain four PS2 partition-tail bytes. Cover `btlMainSetupCameraAndEffects`/legacy `func_001B6120` translated and inlined reads. Do not confuse the adjacent UV source with vertices. |
| `0x0064EFB0`, 224B, shuffle sprite cues | PS2 `btlShuffleDrawSpriteCues` (`0x0038B1C0`) and Steam `0x10C3B1..0x10C4D6` consume eleven stride-20 records: f32 x/y at +0/+4, s32 start/end/sprite at +8/+12/+16, wave staging and palette split at index 5. Steam source `.xpdata` RVA `0xB05C90`; Switch source `0x710062E944` corroborates the fields. | Prefix-copy **220B**, retain four PS2 partition-tail bytes; cover direct/inlined cue reads. The source correspondence does not claim Golden palette constants or timing control code are identical. |
| `0x0063E4F0`, 48B, procedure-entry diagnostic | Matcher pairs PS2 `scrCreateProcedureWork` (`0x0029D660`) with Switch `0x71002DFF50`, score 0.88 / margin 0.88. Hand inspection confirms the four `FLW0` signature tests, stride-16 entry walk, five content types, header-relative offsets and default assertion argument 0 at `0x71002E00B0`. The independently reader-identified Switch string at `0x71005D706A` has the same complete live-span hash as the unique Steam `.sdata` source at RVA `0x947D88`. | Prefix-copy **36B** including NUL, retain twelve PS2 tail bytes. Cover the default unsupported-content assertion read in `scrCreateProcedureWork`, including translations/inlines. This proves source correspondence for an unchanged diagnostic, not a gameplay-balance improvement. |

The widened action table has SHA-256
`99771c8de168b56fcaddf837ebd48eec0bd099e9139d5fbe2d93bc8160df51b8`
over its entire 1,536B Steam span and the independently reader-identified
Switch span at `0x7100632CCC`. Equality corroborates the structural
identification; it is not its sole basis.

### Calendar seam rules: not a blanket 365-to-366 substitution

These additional static observations constrain the month-table binding.
Signed remainder means truncation toward zero, not floor modulo; no
validation or positive-remainder normalization is inferred.

| PS2 role | Observed Golden rule | Structural witness |
| --- | --- | --- |
| `calSplitDayOfYear` | Signed `dateIndex % 366`, then April-first month/day decomposition; February has 29 days. | Switch `0x71000C9FB0..0x71000CA0D8`; the year bound is explicit at `0x71000C9FCC`. |
| `calDayOfYearFromMonthDay` | Sum month lengths from April through the preceding month, plus day minus one; no year modulo. | Steam `0x13BC80..0x13BD4A`; Switch `0x71000CA0E0..0x71000CA168`. |
| `calIsDayInRange` | Inclusive comparison of the raw date index against two month/day sums. No wraparound-year normalization. | Steam `0x13BE20..0x13BE85`; Switch `0x71000CA2A0..0x71000CA3BC`. |
| `calGetWeekdayOfDay` | Signed `((dateIndex % 7) + 5) % 7`; **no preceding year modulo**. This differs from the PS2 source, which first takes `%365`. | Steam `0x13BB80..0x13BBD1`, two signed divisions by seven and addition of five. |
| `calGetDayEntryForMonthDay` / override fallback | Month summation and day-minus-one select the day-table flag and phase 3/4. The downstream field semantics are **not** proved by the month-table source proof. | Switch `0x71000CA7D0..0x71000CA938` chooses phase; `0x71000CA640..0x71000CA79C` and `0x71000CA7A0..0x71000CA7C4` read **u16** at row +2/+4, unlike the PS2 signed-byte fallback. |

The month row's required seams cover its table reads and demonstrated
normalization rules, not an invented conversion for the still-OPEN calendar
column object. Keep that distinction visible in `L1-open`; do not label
phase/override semantics Golden-correct merely because the month row binds.

### Exhaustive decision coverage, bounded reader census

All 587 executable candidates were checked against 35,579 independent PE
exception-directory ranges inside `.shared`: 1,900,110 decoded instructions,
2,155 direct RIP-relative references, and 275 candidate windows with at
least one such reference. A per-row evidence record identifies reference
sites, widths, target offsets and containing ranges. **This is not an
exhaustive consumer census**: non-unwind leaf functions, indirect roots and
bases preceding a candidate are not covered. The other 312 candidates do
not thereby become D10c. Large/overlapping windows can include references
to unrelated objects.

Specific retained decisions prevent known unsafe promotions:

- Calendar `0x005E3A04`: Steam packed reader `0x13C2B0` does match the PS2
  day/reference-day, six-byte stride and alternate-value selection. However,
  the probe starts at column +4 of base `0xA4F7E0`; its 2,196B partition
  extent is not an independently proved array end. Resolve aliases,
  the 365/366-day distinction and tail boundaries before admitting a span.
- Rank sound rows `0x00635C00/20/40/60`: all nominate one numeric-format
  string at `0x92D040`. Its Steam reader `0x20F3D0` takes an arbitrary event
  id, not the PS2 rank-selector switch. Installing a formatter into the
  fixed-path PS2 callsites is not a conversion.
- Shop `0x0063F550`: Steam's nominated path is read by stateful loader
  `0x248810`; the PS2 owner loads nine shop message handles. Matching an
  archive basename does not pair those consumers.
- Trail alias `0x0064EB84`: a Switch reference inside the byte probe is to
  a jump table at +0xA4, not demonstrated trail-record access.

No BBV was performed, no published fact substitutes for a missing reader,
and no failed search is positive absence evidence. Thus D10a/b/c remain
zero rather than being used to cosmetically close unresolved rows.

## Source coordinates and identity

The approved executable is the owner's Steam build **10836033**, install
member `P4G.exe`, SHA-256
`0f327c7669e6cac72aaf355181b1e51edd12fee4d71709fc1802dbf0da70546e`.
That identity is the existing pinned source in
[level-table-source.md](level-table-source.md), not a newly qualified build.
Only the plaintext file-backed data sections below are used for candidate
measurements. Ordinary `.shared` and the pinned Switch NSO may supply reader
proof. Protected `.arch` code must never be decoded or followed.

| Section | RVA base | Raw file offset | File-backed size | Virtual size |
| --- | --- | --- | ---: | ---: |
| `.sdata` | `0x887000` | `0x885C00` | 1,674,240 | 1,675,264 |
| `.xpdata` | `0xA20000` | `0xA1E800` | 1,810,944 | 497,131,520 |

For an executable candidate, M2's offset is **section-relative**, not a
file offset or RVA. The JSONL records both derived coordinates and a
`probe_sha256` when the complete probe lies inside both the section's raw
and virtual extent. Its `probe_size` is the PS2 object's size used by the
heuristic search, **not a proven Golden object size**. Never read the
virtual-only tail as a fabricated zero-filled source.

For an archive candidate, `member_chain` is the complete CPK/nested-member
chain, `offset` is relative to the extracted final member, and
`member_sha256` pins that whole member where inventory evidence is available.
This hash does not turn a heuristic window into a table mapping. A
`PROVEN` row identifies and hashes its actual source span separately.

A missing PS2 symbol is represented by null plus the real PS2 address;
no invented symbol is used. `layout.ps2_kind`, stride and rows inherit M2's
stated evidence. Content-derived periodicity is not a declared struct
layout. `layout.kind = unproven` and null Golden fields explicitly mean
that identical, widened and reordered mappings have **not** been established.

## Why the byte leads cannot be bound directly

M2 votes for placements using eight-byte windows, then reports the fraction
of equal bytes in a PS2-sized window. It does not prove semantic identity,
unique placement, a complete Golden object, a row count or pointer width.
In particular:

- It deliberately does not generate leads for objects smaller than 16 bytes.
  Many small `golden_check` constants therefore have no lead by construction;
  this is not evidence that Golden changed or removed them.
- The six elemental-stage lists and some facility tables have a logical FTD
  payload smaller than the recorded PS2 extent. Bytes beyond the FTD's
  advertised payload can be alignment/storage tail, not additional rows.
- A BMD text match does not authorize copying relocated or differently
  encoded message structures over a PS2 image object.
- Several unrelated PS2 objects can nominate the same Golden window. Their
  candidate offsets must not be mistaken for independent structural proofs.
- A matching constant can have many occurrences; choose it through a typed
  reader and semantic role, not by numeric equality alone.

The per-object `proof_path` names the available PS2 consumer(s), or records
that M2 did not identify one, and chooses structural RE, Switch corroboration
or a concrete message-display BBV check as appropriate. No BBV run is claimed.

## Synthetic decision-reader vectors

These are fabricated metadata cases, not retail data:

| Input | Expected decision-reader behavior |
| --- | --- |
| An eight-byte PS2 float-vector object has no M2 lead | Retain PS2 as OPEN-retain; do not infer absence, zero it or promote it to D10c. |
| A 32-byte probe matches 30 bytes at an archive offset, but no typed reader is known | OPEN-retain; retain the candidate metadata, not an admitted source. |
| A candidate offset plus probe size exceeds its member extent | Do not bind; preserve the failing coordinate as research evidence. |
| A proven source exists but the consumer still reads a native PS2 static | Do not claim the guest write changes behavior; require a mode-gated consumer route. |
| Two ledger records repeat one PS2 address or omit a selected object | Reject the ledger as incomplete/ambiguous; coverage is by address and size, not count alone. |
| A structurally proved row has a wrong source-span hash | Refuse that source; never silently convert it into PS2 retention. |

## Research receipts

Raw scripts and metadata receipts are under
`<scratch>/GoldenRE-notes/l1/`: `l1_inventory4.py`,
`inventory-summary.json`, `l1_decisions4.py`, `decisions-summary4.json`,
`facility_sources4.py`, `facility-sources4.json`, `facility_xrefs4.py` and
`facility-xrefs4.txt`. Executable follow-on receipts are
`exe_readers_l1a2.py` / `exe-readers-l1a2.json`,
`exe_focus_l1a2.py`, `exe_boundaries_l1a2.py`,
`exe_switch_l1a2.py`, `exe_switch_readers_l1a2.py`,
`exe_near_l1a2.py`, `exe_exact_l1a2.py`, `exe_functions_l1a2.py`,
`exe_social_l1a2.py`, `exe_decisions_l1a2.py`, and
`calendar_rules_l1a2.py`; their text/JSON outputs
are private static-analysis research. The earlier linear-sweep census
was superseded by the boundary-anchored census quoted above.
Committed evidence is metadata/prose only, never game bytes or instruction
listings. Generator assertions checked unique canonical coverage and the
587/456 disjoint partition; no build, test suite, BBV, game, emulator,
key access or `.arch` decoding was performed. This specification neither
changes PS2 mode nor implements the M6b binder.

## Systematic cross-ISA reader matching

`tools/golden/reader_match.py` is an offline, metadata-only research tool.
Its required arguments are `--ledger`, `--elf`, `--decomp`, `--steam-exe`,
`--switch-image`, `--switch-cache` and an external `--out` directory.
It reuses the provenance tool's MIPS LUI/%lo and GP-relative reference
extraction and recovered function boundaries. Steam decoding is restricted
to independent exception-directory ranges inside ordinary `.shared`.
Switch input is the pinned mapped image and the private CFG materialization
cache described in the research receipts.

The matcher indexes exact referenced-string hashes, then scores function
correspondence using independent anchors, nontrivial immediates,
increment/scale features and already matched call neighbours. It reports
competing candidates, score margins and per-object access sites. An object
cannot establish its own function match: string-span proposals require a
rare anchor independent of that object, non-string structural features,
an unambiguous ordered access bracket and the same ABI argument position.
Object-byte similarity is never part of the function score. Admission uses
score at least 0.84 and a runner-up margin of at least 0.10, with independent
anchor/constant/stride/call-neighbour gates recorded in each row report.

Outputs are `functions.json`, `matches.json`, `rows.jsonl` and `summary.json`, outside the
repository. These contain hashes, coordinates, counts and feature metadata,
not string contents, instruction listings or game payloads. Scores are
research confidence, not byte-exact decompilation matches. A function match
alone does not establish table extent, field conversion or runtime binding;
non-string partitions and ambiguous alignments remain OPEN pending those
proofs. String-span proposals require hand-sampled call-role confirmation
before promotion. Protected code, indirect references and incomplete
function boundaries are explicit limitations, not positive absence evidence.

`--reuse-functions` accepts an earlier metadata extraction so scoring does
not repeat disassembly. `--seed-matches` accepts explicitly hand-proved
function pairs; seed and newly inferred counts must be reported separately.
The initial symmetric string-coverage scorer admitted no functions; using
rare anchor islands accommodates changed/inlined diagnostics without lowering
the 0.84/0.10 gate. This remains a measured research method, not a guarantee
that every PS2 function has enough surviving plaintext anchors.

### Exercised systematic batch and measured limitations

All **587 executable rows**, including all 583 then-OPEN rows, passed through
the matcher. Extraction covered 13,102 recovered PS2 function ranges /
15,240 data references, 35,579 Steam `.shared` ranges / 59,994 data
references, and 15,861 Switch ranges / 22,506 materialized references.
The report also includes the 456 non-executable rows for the other owner;
this tool does not change their decisions.

The PS2 functions expose 2,065 distinct exact ASCII string hashes; only
285 occur among the extracted Steam readers and 341 among Switch readers.
Seven already hand-proved function pairs were explicit seeds (four Steam,
three Switch). The final scorer added **two** function identities, not nine:
PS2 `cmmRankUpSequenceUpdate` (`0x0025B240`) to Steam `0x1C14B0`
(score 0.86087, margin 0.21444), and PS2 `scrCreateProcedureWork`
(`0x0029D660`) to Switch `0x71002DFF50` (0.88 / 0.88).
The latter supplied the one hand-checked new executable source proof above.
Call-neighbour propagation did not produce a broader matching closure.
The matcher is therefore useful triage, **not a demonstrated scalable
solution for the remaining source mappings**; the proof gate was not lowered
to turn poor matches into PROVEN rows.

At the reader-matcher checkpoint, the then-582 OPEN executable rows had
concrete per-row features/reasons and competing candidates, categorized as:

| Retained blocker | Rows |
| --- | ---: |
| No direct PS2 formed-address reader under recovered boundaries; follow indirect/computed/data-pointer owners | 147 |
| Reader exists, but no independent unique cross-ISA function match meets the structural gate | 431 |
| Matched reader, but object access/argument bracket or fitting string span is ambiguous | 3 |
| Matched reader, but non-string object layout/extent remains unproved | 1 |

The reviewed receipt is
`<scratch>/GoldenRE-notes/l1/matcher-v3-l1a2/rows-reviewed.jsonl`,
SHA-256
`529333632c2b36eca8c0e3199bfa0e0ea236a76b2a8bc8d00fb988648e377dbf`.
Raw extraction, candidate matches, calibration and the hand sample are in
the sibling `functions.json`, `matches.json`, `summary.json`,
`matcher_calibrate_l1a2.py`, `matcher-sample-l1a2.txt`, and
`matcher_decisions_l1a2.py`. Generator assertions checked all 1,043 unique
addresses and disjoint 587/456 ownership. No build, unit suite, BBV or game
execution was performed.

## Local layout anchoring

`tools/golden/layout_anchor.py` implements the owner's additional
`PROVEN-layout` class. Required arguments are `--typed` (the complete M2
object inventory, **not just L1**), `--ledger`, `--elf`, `--steam-exe`,
`--switch-image` and an external `--out`. It searches every positive-sized
PS2 object for full byte-identical spans in plaintext `.sdata`, `.xpdata`
and mapped NSO data, including objects below eight bytes and zero-only
objects. More than 32 identical occurrences in one image remain ambiguous,
not silently selected. Existing source proofs also pin anchor starts;
prefix-only proofs do not invent an unproved left-anchor end.

A variant B is admitted only within a locally checked A→B→C interval:

1. Both flanks are full identical anchors or previously proved source
   coordinates, on both PS2 and the qualified Golden image.
2. The intervening PS2 objects are contiguous, with the same ordered count
   of Golden slots. For a single B, the two flanks fix its entire extent.
   A multi-object interval additionally requires nominated starts to tile
   the whole interval in order; otherwise it is an **ambiguous gap**.
3. Each size is equal, or an address-keyed `--widenings` entry supplies
   `golden_size` and a cited prior layout proof. No widening/shrinkage is
   guessed from a nearby byte window.
4. Minimum element alignment agrees and precisely one complete source
   placement survives within the selected platform. Source hashes, both
   anchor sizes/offsets and all interval slots are recorded.

This is a **local correspondence model**, not a claim that compilers,
constant merging, linkers or whole translation units preserve one global
data order across architectures. Reordered anchors, incompatible gap sizes,
missing flanks and unpaired object counts remain OPEN. The tool emits
`anchors.jsonl`, `rows.jsonl`, `summary.json` and a deterministic random
20-row `sample.json` (seed 20261112). The sample is initially marked pending;
actual hand-review evidence is recorded separately. No code is decoded or
executed by this method, and no original payload bytes are emitted.

### Layout batch, qualified sources and twenty-row hand sample

The final run processed **all 587 executable rows** against the complete
9,415-object M2 inventory, including short and zero-only objects. It found
1,344 usable anchor objects: 1,104 have `.sdata` occurrences, 341 `.xpdata`
occurrences and 513 NSO-data occurrences (these groups overlap).
4,700 object/image occurrence lists exceeded the 32-placement limit and
were kept ambiguous. Six previously proved inline-image endpoints were
qualified, including the supplement's newly proved 84→168B pointer table.
No undocumented widening was supplied or inferred.

Exactly two new local placements satisfy the rule:

| Object B | PS2 A→B→C | Qualified Golden A→B→C | Binding source |
| --- | --- | --- | --- |
| `0x00609C38`, 16B | A `0x609C28`/16B, B `0x609C38`/16B, C `0x609C48`/16B | Steam `.sdata` offsets 592440/16B → 592456/16B → 592472/16B; one contiguous slot, alignment 1. Both complete flanks are byte-identical. | Steam RVA `0x917A48`, raw 9528904, 16B, SHA-256 `18fc8d436b4db9cd8b04303d5fec52bf584542d7dd5ffd474c9b3f2efc8ef9fd`. No direct PS2 reader was identified; runtime coverage is not invented. |
| `D_00626F00`, 96B | A `0x626DE0`/288B, B `0x626F00`/96B, C `0x626F60`/96B | NSO mapped offsets 6480644/288B → 6480932/96B → 6481028/96B; one contiguous slot, alignment 4. Both complete flanks are byte-identical. | Layout-identifying NSO address `0x710062E424`. The entire 96B payload occurs once in Steam `.sdata`, RVA `0x90B890`, raw 9479312, SHA-256 `92ba4ccba39c6baaea2f7f909d8553183c296c16f3bcdbb4460970482e22f3d6`. Bind that qualified complete copy; **Steam anchor order itself is not proved**. |

The second object's 24-f32 PS2 declaration and consumer `func_002142B0`
are preserved as existing type/route evidence; the layout method does not
invent field names or control semantics. Both new rows remain
`runtime_status=pending-seam` with explicit installation/read requirements.
Neither shifted M2 probe is used as the admitted source.

The random sample is **20 distinct rows drawn uniformly from all 587**
using seed 20261112, not selected from successful placements. All twenty
sampled rows remain OPEN. Independent full-anchor byte/hash checks and gap
arithmetic were followed by manual review of every interval summary.
Both successful placements were additionally checked by hand. Adding the
latest pointer-table proof changed one sampled interval; that new endpoint
and its gap were checked separately, while nineteen reports were unchanged.
The sample records missing/reversed flanks, unequal gap sizes and unknown
multi-object order instead of assuming global compiler data order.

The final receipt directory is
`<scratch>/GoldenRE-notes/l1/layout-current-l1a2/`:

- `rows.jsonl`, SHA-256
  `7b6c19ff883ba46809e443fcba1706c3d80c8ec6b08b5bb4ebd34b0922179bd3`;
- `sample-hand-review.json`, SHA-256
  `606d9f2ba82bcb8bc3982604ad6b2352716caf62a4911fc0b94fcc64678acfe2`;
- `anchors.jsonl`, `summary.json`, `sample.json` and
  `sample-new-anchor-check.json`.

The prior `layout-v2-l1a2/sample-independent-checks.json` contains the
twenty independently computed interval summaries; the final hand receipt
records the one updated interval. These are metadata/prose only.
All 580 remaining executable OPEN rows explicitly record **ambiguous gap**
with their actual flanks, candidate offsets/sizes, object count and failed
order/extent/alignment checks. The local rule therefore improves two rows,
but does not demonstrate global source-order preservation or broad closure.

The committed [twenty-row hand-review record](l1-layout-sample.json)
preserves the sampled addresses, decisions and checked failure geometry,
plus the two additional admission checks, without shipping payload bytes.

## L1c priority 1: code-derived day-flag mask

Main approved `PROVEN-code` for `0x005E4610` under D1. The PS2 partition
contains six live s32 IDs and eight unused trailing bytes. The matching NSO
module unrolls defaults at `275700`, reads at `2757B0`, writes at `275890`,
captures at `2759B0` and restores at `2759D0`. The ordered Golden list is
`39,3A,3B,3C,3D,3E,1003,1474,1477,1476` (hex), yielding ten mask bits.
The [code proof](l1-day-flags-code-proof.json) records exact defaults,
bit31 validity, all seams, and synthetic vectors.

This is an authored Golden-mode constant, not a Steam literal-table copy.
Steam `.sdata` `90C578` contains `39..40` (hex), not this ten-ID source;
Steam corroboration remains missing. No `.arch` code was decoded.
PS2 six-bit behavior and the eight tail bytes remain unchanged in PS2 mode.

Post-update helper `2A8090` is a four-instruction leaf: set flag `1474`
from nonzero live s32 at NSO `4A70C38`. It runs after defaults and mask
writes, so final bit7 is not generally the input/default mask bit7.
The [sidecar lifecycle](l1-network-sidecar-lifecycle.json) now identifies this
as a Golden network-enable/session latch (`shdNetWork.c` / `NetworkDialog`).
It is process-owned, initially zero in NSO BSS, and updated by config commits,
explicit post-load initialization, dialog enable/disable completions and
network-operation state27/result1 disabling. No PS2 field counterpart is
identified: author a Golden-only sidecar and all its observed writer seams.
Generic day-mask restore reads the sidecar into flag1474; it must not overwrite
the sidecar from saved maskbit7. Runtime implementation/qualification remains
pending; never replace the live state by fixed zero or by the input mask bit.


### Calendar extent followthrough, still OPEN

[The extent receipt](l1-calendar-extent-research.json) resolves the PS2 real
base `5E3A00` and aliases +0/+2/+4. Its 366 x 6 logical view is 2,196 bytes;
selected column partition `5E3A04` contains 2,192 live bytes plus four padding
bytes. NSO relocation `8808D8 -> 73F0D8` and Steam `.xpdata` `A4F7E0`
have equal first-2,196-byte view hashes. The nearer typed next object is
NSO `73FD08`: six relocated phase-label pointers, read at `C9A68`.
Its 3,120-byte distance supplies only a 520-unit upper bound, not a row count.
Steam and NSO first differ at byte 3,117; 519 rows plus padding is a concrete
lead, not an admitted extent. Explicit u16 phase/packed readers, current-day
override flags, raw signed16 day indexing and the separate 366-day splitter
are now recorded. Override-stack producers and physical/logical extent remain
unproved. Do not copy the historical 2,196-byte column4 probe.

The same receipt identifies companion `WeatherUnconditionalBonus.ftd`:
8 x 20 = 160 payload bytes at +16, not PS2's five observed live rows in each
112-byte partition. These bases (`6406F0`, `643D00`) are **outside the selected
1,043-row L1 inventory**; this is a coherence lead, not two newly added rows
or a runtime-qualified fallback conversion.

## Cohort 4: PS2 filesystem directory migration

[Initial architecture proof](l1-directory-migration-proof.json),
[followup proof](l1-directory-migration-followup-proof.json), and
[M10 resource-route contract](l1-directory-resource-routes.md):
**56 D10c / 72,730 PS2 B; zero OPEN-retain** in the actual directory scope.
PROVEN / PROVEN-layout / PROVEN-code / D10a / D10b are zero.
The original assignment was60 /75,206 B; Main transferred the four
positively non-directory objects /2,476 B to cohort6. Their current decisions
are not counted or modified by this followup.

Main's 2026-11-13 direction admits positive representation-replacement D10c
as `runtime_status=pending-seam`, with
`required_seams=["golden-resource-route:<class>"]`. This does not claim that
the current M6b typed-table loader routes general assets or that runtime
L1-open is reduced. M10 must implement/bind the named contract without a PS2
ELF or static directory lists. Drop the old representation, not resources.

Actual H_Cdvd paired pointers and four ROFS registrations establish the
56 directory partitions. Golden's positively traced CRI CPK bind/name/size/read
graph replaces the PS2 disc-specific list along eighteen route classes.
The final thirteen admissions prove effect pack/texture consumers, registered
AMD/PAC model callbacks and NPC variant producer, parent-qualified facility
ARC loading, BTL-root routing union, and the ACB/AWB bank/cue architecture.
Audio contracts include actual bank readiness/lifetime, persisted SYSTEM/Voice,
and numeric cue producers; CueId is not an AWB member index or a parsed label.
No guessed Golden file offsets, sectors, byte-window matching or no-reference
absence claims are used. Golden producers, not old disc names, choose assets.

E820 is representation-only: no Golden E826 PM asset or inferred rename.
Missing assets fail closed. M11 must confirm scripts never request
E826_021/022 and reopen its row if they do. All56 remain runtime pending-seam.
The four transferred rows are `005dcaf8`, `005dcb28`, `005dcb98`, `005dcc6c`.

This cohort subsection updates only its own decisions. Shared summary
counts/prose remain the global-count owner's responsibility.


## Cohorts 1–2: scheduler, boss banks and shuffle source batch

The [boss-bank proof](l1-boss-camera-banks-proof.json) admits six 1,872-byte
PS2 partitions. Each has a complete **1,868-byte mixed source plus four PS2
tail bytes**, not a 36-record homogeneous table. Encounter IDs, unit IDs and
packet-triggered reselection establish ownership through the shared boss-bank
pointer. Typed camera readers establish frame/30 timing, vector pairs, party
order/random variants and the final pose at `718..74B`. Native alternate
boss branches were not covered by the previous non-boss camera-script proof.

The [string proof](l1-boss-scheduler-strings-proof.json) admits seven rows.
Material correspondence is by HP-threshold **role**, not suffix similarity:
PS2 c/b/a map to Golden `petal:petal_a_Material`,
`petal:petal_b_Material`, `petal:petal_c_Material` respectively. Install each
23-byte complete path and retain nine PS2 tail bytes. The actual resource
2804 model namespace and live HP/maxHP producers remain binding obligations.
Both transformation procedure names and scheduler create/query/load paths
have independently corresponding readers.

The [shuffle proof](l1-shuffle-count-roster-proof.json) admits `64E5F1`
and `64E76F`. The former is the +1 partition of a ten-pair min/max table:
copy 19 bytes, retain twelve; native readers need the complete 20-byte source.
The latter has a retained dummy at +0, **nine** live values at +1..9 and
seven retained tail bytes. Golden indexes `playerLevel-1` only for levels1..9.
Do not copy the unrelated tenth NSO byte or a 17-byte heuristic prefix.
The [variant/reward proof](l1-shuffle-variants-reward-proof.json) now admits
`64E670` and `64E7B8`: forty live four-column weights plus eight nonlogical
PS2 tail bytes, and a complete44-record callback-column view respectively.
Golden stride16/pointer64 descriptors require semantic normalization into the
520-byte selected guest view, never a704-byte overwrite or foreign code pointer.
Five callback roles, real PERSONA.TBL blocks4/5, live party/roster state and
RNG are explicit seams. Normal Golden production selects pattern5 and negative
level-byte difficulty while still consuming its flagged-selector RNG draw.

First publication: **15 PROVEN / 11,496 PS2 B**, no new code/layout/D10.
Current cohort 1 cumulative: 4 PROVEN / 96 B,
1 PROVEN-code / 32 B, 2 D10c / 48 B, 7 OPEN / 4,448 B.
Cohort 2: 33 PROVEN / 13,868 B, 3 PROVEN-code / 12 B, 36 OPEN / 172 B.
All admissions remain runtime pending-seam.

The [calendar producer research](l1-calendar-weather-producers-research.json)
seals exact signed-depth reset/push/pop, ff sentinel, live759454 bypass and
saved-real date/phase/weather/flag restoration, without a row promotion.
Push/query have different actual signed bounds; never clamp them to one limit.
The corrected typed next anchor is73FD08. Equal3114B and differing following6B
do not prove519/520calendar rows; physical extent and raw-day domains stay OPEN.
`5E50D0` is five mixed calendar-animation initializers, not proven weapon data;
`641AD0` is an old11-ID message map whose Golden child copies20words.
Neither observed PS2 type alone admits a Golden supplier or guessed prefix.

The [shared sine polynomial proof](l1-shared-sine-polynomial-proof.json)
adds six4-byte GP coefficients. Original1BA790 actual loads and squared-angle
Horner weights pair the degree13 intrinsic with ordinary Steam4FFF10's
quaternion-cache sine/reciprocal scaling. Positive x13/x9/x5 coefficients
are PROVEN data operands; negative x11/x7/x3 are PROVEN-code because SUBSS
supplies their signs. Author exact signed bits, never copy positive magnitudes
into negative slots. Source values do not prove unchanged quaternion/cache,
endpoint/linear, timing or evaluation-order semantics; every real producer
and direct/native/inlined polynomial reader remains a qualification seam.

The [monthly cache/source-tag proof](l1-scheduler-month-cache-proof.json)
adds `7637A0` PROVEN/8B and `7637A8` D10c/8B. The former is a mutable
signed loaded-month word initialized-1, not f64:4live+4tail and actual
SteamAF321C/NSO758A38 sentinel/compare/write/path/script-header consumers.
Write real current month before asynchronous acquisition, never freeze or
reapply the initializer per task. The latter is the old6-byte `%s %d` local
source/line tag plus2tail; both complete Golden monthly acquisition branches
format the real path then directly acquire, without that local diagnostic.
Preserve native outstanding-handle assertions, other allocation-site metadata
and actual31000hex script-buffer/resource/procedure lifetimes.

The [action descriptor proof](l1-battle-action-state-table-proof.json)
adds `5F6E20`:38 original12B records project from a complete42x24B Golden
source. The established c5 typed bank and independent ordinary dispatch
readers prove four inserted states15/16/24/25 and shifted role identities.
Never copy1008B over456B, copy host pointers, or use the38-slot projection
as the actual42-state Golden dispatch bank. Real84 callback implementations,
all state producers/aliases and528hex work/list lifetime remain pending-seam.
Golden delay/elapsed are f32 driven by the actual shared clock, not old
u16 countdown/u32elapsed+=2; its deferred helper lacks the PS2 zero-delay
immediate-init branch. Matching descriptors do not prove callback bodies.

The [motion ID map proof](l1-battle-motion-id-map-proof.json) adds `5F6CA0`:
old3x30u8/90B plus6 retained tail bytes maps from Golden3x34u8/102B.
Copy30B per source row34 into destination row30, never a flat96B probe.
Typed Steam/NSO lookup and actual packet domain0..33 fix the shape; genus2
motions5/6/7 genuinely change. Full native sources retain new motions30..33,
real formation overrides, unit/model/rate producers and separate forced
slot27 state; an unmapped static slot is not proof of unreachable animation.

The [loaded boss map proof](l1-battle-boss-motion-map-proof.json) adds
`6355A0`:30 old live motion-map bytes plus2 retained tail bytes, from the
actual34-byte Golden map. Logical27 changes0->18. Paired real encounter
publishers and dynamic motion consumers prove battle11C8/NSO11E0 identity;
common initialization resets34 signed override bytes toff, not30. Preserve
actual branch-conditioned publication, later phase/override writers, special
unit conditions and separate generic fallback; never force an always-on map.

The [living-party grid proof](l1-battle-formation-grid-proof.json) adds
`609430` and `609458`:three XYZ positions/36B and radial scales/12B,
retaining4/12 PS2 tail bytes. Actual B06 packet and complete NSO grid update,
paired original operands and unique hash-bound Steam spans establish role,
not float equality. Preserve real eligible roster/nonactor index, selected
action, live default pose9857e0, grid/normalize and unit/model writes; do not
invent a zero actor pose, force index bounds or copy the40/24B probes.

The [named battle resource proof](l1-battle-named-resources-proof.json)
adds the17-byte formation/variant sky key,18-byte NPC-definition path and
22-byte shuffle-result task name. Actual resource/formatter/task readers pair
Steam and NSO; copy only those live spans and retain15/14/10-byte PS2 tails.
Native sky geometry, real NPC/calendar variants and widened reward-task work
plus the additional live fifth input remain explicit binding obligations.

The [sky index proof](l1-battle-sky-indices-proof.json) adds `60A140`:
118 actual triangle triples /354u16 /708B, with12 nonlogical PS2 tail bytes.
Original and independent NSO bounded readers plus the ordinary Steam call
prove one row,89 vertices and flags4c; the complete sources agree independently.
Do not copy the old720-byte probe or conflate PS2 eight-byte output triangles
with Golden six-byte output triangles. Actual geometry/resource/native
producers remain pending-seam; the index proof does not qualify every sky field.

The [revival bank proof](l1-battle-revival-message-bank-proof.json) adds
`5F7000`:271 old file bytes plus one retained alignment byte, not272B of
scalar logic. Nine complete localized Golden banks match independent NSO
sources in the actual table order. Golden expands two records to three:
real flag100d selects record1 when clear,2 when set. Preserve actual locale,
flag producers/store/lifecycle, window selection, action/packet/camera graph
and release/reset; never force PS2record1 or overwrite272B with349..405B.
No ordinary Steam reader is claimed; use pinned image/full-span hashes/refuse,
the complete NSO typed reader and actual native/full-bank resource routing.

The [fixed status-AI command proof](l1-ai-status-fixed-command-proof.json)
adds `7635C0`: an8-byte command record, not f64. The real low20 status2 branch
selects command8000/target0 directly without RNG, with paired PS2/Steam/NSO
typed consumers. Real command-work projection and target callback/fallback
remain explicit native binding obligations.


### Calendar/result identity and scheduler failure successor

[The task-identity proof](l1-calendar-result-tasknames-proof.json) additionally
qualifies `638EF0` (`cldDayChangeMonth`, 18-byte NUL span) and `6290F0`
(`battle_result_simple`, 21-byte NUL span). Corresponding constructors,
priority15, lifecycle callbacks, parent work and identity queries establish
the sources; unchanged names alone are not the proof. Retain14/11 tail bytes
respectively. Wider Golden task-work layouts are not image-copy permissions.

[The missing-procedure proof](l1-scheduler-missing-procedure-proof.json)
positively assigns `637320` to D10c. Both ordinary Steam and NSO replace the
PS2 diagnostic printf/assertion branch with stored task handle0, sequence10
with zero auxiliary arguments, and return-1. This is a complete paired error
branch, not a failed search or cosmetic exemption. Bind and exercise that
Golden failure route before dropping the obsolete diagnostic supplier.

[AICALC program ownership](l1-aicalc-program-ownership-research.json) now
traces the actual length-prefixed loader and all eleven destination slots.
Block8 supplies stride164 enemy metadata and its u16+2 procedure identifier;
block9 is the player-character FLW namespace; block10 is the enemy FLW
namespace. The ordinary AI dispatch and named procedure packet distinguish
them. Packet kind3 instead selects a separate current boss program. This
research does not admit interior literal candidates without their PS2
procedure/operand correspondence, and edits no c5-owned weapon-bank rows.


### Battle movement coefficients and formation resources

[The movement proof](l1-battle-movement-speed-proof.json) qualifies
`5F6D20`: five f32 coefficients, **20 live bytes plus four retained tail bytes**.
The Steam24-byte heuristic probe includes an unrelated scalar. Actual readers
select default index2 or a u16 from the live character-row/skill-side producer.
Neither a default-for-every-unit shortcut nor a new clamp is authorized.
Native movement packets must receive the qualified value; equal coefficients
do not prove identical timing or qualify separate caller multipliers.

[The formation-path proof](l1-battle-formation-paths-proof.json) qualifies
`609570` and `609588` through the same paired formatter: real resource-mode
bit0 selects `battle/BTL_TEST.BED`; otherwise fixed `skill/` plus `%03X` of
the low16-bit identifier. Three hexadecimal digits are a minimum, not a cap.
Copy20/11 bytes including NUL, retain4/13 tail bytes. Actual Golden BED asset
availability/name routing remains an explicit runtime obligation.


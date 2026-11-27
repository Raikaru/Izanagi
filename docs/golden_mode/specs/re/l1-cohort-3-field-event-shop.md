# L1 cohort 3: field, event, shop and progression

## Scope and source counts

The immutable historical L1c cohort contains **219 objects / 30,021 PS2 bytes**.
Main transferred weapon-bank rows `00645240` (3,877 B) and `0064732e` (802 B)
to cohort 5; neither row was edited here. Current ownership is therefore
**217 objects / 25,342 B**. Four batches establish **29 PROVEN / 996 B**,
leaving **188 OPEN-retain / 24,346 B**. PROVEN-layout and D10a/b/c remain zero.
These are source decisions, **not runtime closure**; all 29 admissions are
`pending-seam`. This is a checkpoint, not completion of the remaining cohort.
Global totals are maintained separately by GoldenRE-L1c.

The [fishing/BF/shop receipt](l1-cohort-3-field-event-shop-proof.json),
[field/bank receipt](l1-cohort-3-field-bank-proof.json),
[VM diagnostic receipt](l1-cohort-3-vm-diagnostic-proof.json), and
[named-bank receipt](l1-cohort-3-named-bank-table-proof.json) contain source
identities, exact Steam coordinates/hashes, NSO segment bounds, complete
logical objects, corresponding readers, tails and required seams.

| Owned PS2 row | Bytes | Proven role / installation |
|---|---:|---|
| `00635a40` | 32 | Fishing archive loader; copy the 18-byte NUL-terminated Golden path, retain 14 tail bytes. |
| `00635bb8` | 24 | Fishing indexed script return; six old entries become eleven Golden entries plus an optional s32 override. Semantic seam, never a 24-byte image copy. |
| `0063e3f0` | 48 | Four-null-input guard diagnostic in `scrCreateWork`; copy 37 live bytes, retain 11. |
| `0063e4d0` | 32 | FLW0 signature guard diagnostic in `scrCreateProcedureWork`; copy 23 live bytes, retain 9. |
| `0063e618` | 24 | Script child-task registration name; copy 13 live bytes, retain 11. The Steam occurrence is reader-selected, not chosen among duplicate strings by order. |
| `0063f550` | 16 | Persistent help archive moves from `help/datMsg.pak` to `init/datMsg.pak`; install all 16 Golden bytes. |

The two BF guard strings are diagnostics, not coefficient/state data. Their
source proofs do not authorize permanent PS2-cosmetic retention. Task names
and archive paths also require coherent registration/loading; text equality
alone does not establish that coherence.

## Complete fishing indexed-value source

PS2 `00250870` copies exactly six signed halfwords from `00635bb8` and indexes
by the fishing data task's choice. Twelve additional partition bytes are
alignment, not six more values. The sole identified table consumer is this
script getter; translated/inlined/native copies are explicit required seams.

Golden Switch `710012e840` accesses `cmmFishingDATA`, reads choice at work+80,
loads a signed halfword from the eleven-entry table at `7100632574`, and then
uses work+a8 as a full signed-word override when it is not -1. Ordinary Steam
`1cebf0` has the same choice, eleven-entry bound and override rule.

The complete Golden source is a **Steam-only composite**:

- First eight entries: 16-byte data span `.sdata` RVA `9677c0`, SHA-256
  `25cb33b6d11b36eb7f10056bf09ace7670eba947b291be7f50e833d614dea8e8`.
- Last three entries: **PROVEN-code constants** `1007, 1014, 1006`, authored
  only in Golden mode. Steam `1cec1c` materializes the first pair as one
  immediate; `1cec2f` materializes the third. The receipt records exact
  instruction bounds/hashes and independent NSO data corroboration.
- Full composed 22-byte SHA-256:
  `5232d18ecec4dff7a4f70ce1db05b6f27824bfa53ba7e52a97c0de49ca9f5a0b`.

Main explicitly admitted the composite source on 2026-11-13. `PROVEN-code`
classifies the authored constant component, not a new ledger-row decision.
The runtime must **not** read instruction bytes or depend on the NSO/private
research files. Steam unsigned indices >=11 return -1; there is no invented
clamp. A non--1 override replaces the result without halfword truncation.
Golden input-choice/override state transport must be implemented explicitly;
PS2 work offsets must not be guessed from Golden offsets. This does not prove
that the rest of the expanded Golden fishing minigame is implemented.

## Field/BF constructor bootstrap

The existing independently established `scrCreateProcedureWork` pairing
(PS2 `0029d660` / Switch `71002dff50`) identifies the FLW0 signature guard,
five content kinds, stride-16 entry walk and header-relative pointers. Its
call into Switch `71002e0370` leads to the four-input null guard and
stride-32 procedure-name copy in PS2 `scrCreateWork` (`0029d2f0`). These
structural conditions identify both diagnostic sources, not their spelling.

For the `ScriptViewer` name, PS2 `0029e8d0` allocates 0x228 and registers
priority c9 with script start/destroy callbacks. Switch `71002e0be0` and
Steam `468e40` allocate 0x240, register the corresponding callbacks/priority,
and attach the optional parent. The actual Steam reader selects RVA `947d78`;
the same-text occurrence `93f183` is not admitted.

## Facility-bank bootstrap and unresolved selectors

The former `0063f550` blocker was that its candidate only had a generic
stateful-loader reference. Following Switch `7100174430` through named
resource lookup and `710018b250` resolves the missing role: it loads
`init/datMsg.pak` beside `fclTable.bin`, waits for readiness and registers the
same nine PS2 help-member names at indices 0..8, followed by four additions.
The qualified Steam path is `.sdata` RVA `92de28`, 16 bytes, SHA-256
`67481579650a2c63a9dd096fe302f061e4af763a2cd194645abbeb02280dd45a`.
Changing the path alone does not certify BMD parsing or runtime binding.

The bank investigation also establishes why the remaining rows cannot be
closed by copying candidates:

- PS2 `0063f2b0` is the start of **13 mutable s32 handles**, initially -1,
  not a numeric gameplay table. `002bc010` installs and `002bc060` releases
  a slot, restoring -1. All seven whole-table lifecycle/draw owners are recorded,
  including the slot-6 alias read at `002bc860`.
- PS2 `0063f2f0` has six selector words (24 live bytes, eight tail bytes),
  and `0063f310` has nine (36 live, twelve tail). These are distinct complete
  logical selector tables, not the inventory's padded extents.
- Golden has **15 handles** at `7100764b84`, install/release at
  `710018b250/710018b280`, and generic/category/direct readers at
  `710018b2c0/710018b390/710018b510`. The selector at `7100633830` has exactly
  seven live words: a positive kind-6 read establishes its last word, and an
  independently read 32-u16 branch table starts immediately afterward.
- All four identified Golden installation callers and three release callers
  are recorded. Slots 0..12 have named help-member identities. Bank 13 is
  `facility/datMyth.bmd` in both contextual producers; bank 14 is
  `event/cmm.bin` member 9, matching PS2 community auxiliary bank 11.
- Golden weapon constructor `71001b15d0` allocates 0x170 and passes the
  descriptor at `7100764fe0` into `71001987f0`. Ordinary instructions prove
  the third argument omitted by the worker's inferred prototype. The base
  initializer allocates 0x8c0 plus count-derived handle/work areas.

The selected first-word row `0063f2b0` is now PROVEN as an authored
**PROVEN-code initial/reset sentinel -1**, corroborated by both complete
initializers and the release/install/weapon-help role. Initialize only before
registration, then preserve guest-owned live handles and their lifetime.
Never copy fifteen live Golden IDs over thirteen guest slots.

Selector caller adaptation remains **OPEN**, not an initialization problem.
Retail `002bc4b0` calls `datEntryGetKind` (`00106850`, item word +0), despite
the inferred C spelling `datEntryGetFlags`; `002bc0e0` really uses flags +4
for its separate six-word selector. Golden `00111da0` also reads kind +0.
Flags and Kind are not interchangeable. The complete ten-case Golden kind
map and seven-word generic-kind map are recorded, but their changed domains
must not be prefix-copied or equated by ordinal. Adjacent selector/alias rows
belong to cohort 6 and were not edited here.
No module label, -1 byte run, missing reference, generic loader, or float
similarity was used as a proof of correspondence or absence.

## Field/BF resource and bank follow-on batch

The second batch proves **16 objects / 484 B**:

| Owned rows | Complete source family / seam |
|---|---|
| `005f0aa0`, `005f0ac0`, `005f0b90` | Field startup BF, dungeon table and support BF. The constructor/load graph pairs model kind 4/id 2000, owned pointer/size copies, the dungeon size `<0xc00` guard and common NPC/units staging. |
| `005f0830` | Common NPC table: owned full-file copy, 0x84-byte records and s16 -1 marker count. |
| `005f5470`, `005f5490` | Food court: qualify the entire three-script seasonal family, field 9/room 1, and both BF procedure uses at priority 0xf. |
| `005f1ff0`, `005f11b0` | Local-map startup BF and per-field loose BF request. Pair scene/request/readiness and script-entry roles rather than text spelling. |
| `005f07d0`, `005f0810` | Complete normal/night NPC table and secondary-BF filename families. Night is time-slot 5; only the scene-bin reader has the normalized-member retry. |
| `005f10b0`, `005f1150`, `005f1190` | FBN parts/placements, HBN owned light/point data and counted 44-byte event records identify three distinct member-name families. |
| `005efeb0`, `005efef0` | Versioned ENV and CMR source keys, identified by typed environment/camera consumers, not guessed extension changes. |
| `0063f2b0` | Bank-0 initialization/reset sentinel and guest-owned handle lifetime, as described above. |

Eight ordinary strings permit exact prefix copying with recorded retained
tails. The seven expanded/renamed resource-key families require semantic
routing instead: loose `field/data`, `field/table`, `field/script` or
`field/env` namespaces versus named members in normalized
`field/pack/fd%03d_%03d.arc` archives. The receipt records complete
January/February room-normalization maps, all special field-band predicates,
normal/night choices, actual missing-member retries, and ENV weather/date
variant precedence. These are authored **PROVEN-code** selection constants,
not runtime reads of NSO instructions.

The full logical objects and every identified source reader are accounted
for in each admission. Source qualification does **not** assert that newer
Golden FBN/HBN/ENV data, model pointers or graphics APIs have the PS2 layout.
Resource providers, full-file ownership, version-aware decoders and native/
inlined users must bind coherently before runtime closure. No arbitrary
archive offsets, guessed fallback, or source-only filename patch is sufficient.

## Complete BF VM diagnostic family

The third batch proves **six diagnostics / 288 B**, not state or balance
tables: `0063e0a0`, `0063e0d0`, `0063e100`, `0063e130`, `0063e340`, and
`0063e3a0`. Each owns a 48-byte PS2 partition. The complete NUL-terminated
Golden strings are respectively 33, 40, 38, 33, 40 and 40 bytes; the
remaining partition tails are retained, not mistaken for diagnostic data.

All **35 opcode slots** are decoded from PS2 registration `0063e2b0`,
Switch relocation-backed `71007af218`, and Steam `.xpdata` RVA `b25710`.
The ordinary Switch executor independently rejects negative opcodes and
opcodes >=35 before loading an 8-byte function pointer and calling it.
This bounded registration graph identifies global/local integer and float
stores, return-PC restoration, NOT, IF and the binary-operation helper;
matching text or a relaxed automated score is not the proof.

The integer-pop diagnostic covers all six paired PS2 logical users and both
operand sites in the shared binary helper. The two float-pop diagnostics
cover the complete five-user family, centralized in Golden `71002e2940`:
tag 4 takes the specific return/address error; unsupported tags take the
other error. Command integer/string getters are separately identified by
the parameter-stack indexing, accepted tag sets and global-reference reads.
The local-integer bound format is **argument 2**, with operand `LI` in
**argument 3**. Ordinary instructions prove the variadic argument omitted
by the worker's inferred prototype.

Golden value slots/global integers widen while float globals remain 4 bytes.
These diagnostic proofs do **not** claim VM arithmetic or pointer-layout
equivalence. Runtime inputs remain hash-qualified Steam strings; the NSO is
static corroboration only. Every listed user, translated/inlined copy and
compiled native literal remains an explicit seam before runtime closure.

## Complete named shop-help registration table

The fourth batch proves `0063f520` (**48 B**): nine 4-byte filename pointers
occupy 36 live bytes, followed by 12 alignment bytes. These are **not resource
IDs** despite the inferred C variable name. Their sole identified PS2
consumer `002bdf20` supplies each address to `00455f70`, which normalizes and
compares the requested pathname with registered archive entry names, then
installs the returned member at the same bank index.

The full Golden source is a **13-pointer / 104-byte** Steam `.sdata` table at
RVA `90e6c0`, SHA-256
`541f37cc951bf13c6e4e0e7cc708b25928306657f7c4d29308174526d3765ea0`.
Every pointer resolves to an individually hash-qualified Steam string.
Standalone reader `248740` and the inlined startup region
`2492a7..24945c` both walk by 8 bytes, perform named archive lookup, create a
BMD handle and store it at the ordered index; both independently bound the
loop at 13. Switch `7100174430` corroborates all thirteen named lookups and
explicit bank 0..12 registrations. The first nine preserve the PS2 member
roles under the new archive-relative namespace, followed by four additions.

This source is distinct from the mutable handle initializer and the
Flags/Kind selector maps. Binding must replace the **complete registration
family**, not write 104 bytes over the 48-byte guest partition or thirteen
pointers into its nine-element stack vector. Decode sealed file pointers
to bounded source RVAs, never guest/host live pointers. Preserve the
`datLoadEntryTable` continuation, guest-owned bank lifetimes and contextual
banks 13/14. The archive/BMD provider and every translated/native user remain
pending seams; no runtime NSO input or filename-only closure is authorized.

## Exercised evidence

Private reproducible helpers: `<scratch>/GoldenRE-notes/l1/l1c3_*.py`.
They were run through the required Git Bash wrapper and fixed Python 3.12.
Static extraction recovered complete table widths, live string spans, PE/NSO
image/segment identities, Steam data and instruction hashes, and equality of
the full 22-byte composite with the independently reader-identified NSO span.
Romwright analyse-function and worker returned zero for 45 extracted
readers/constructors listed in the private receipts. Two earlier float-store
receipts were reused, not counted as new extractions. Pivotal ordinary ARM64
and `.shared` instructions were inspected independently of worker prototypes.

No build, verify, objdiff, unit suite, BBV, CI, game/emulator execution,
key access, or `.arch` decoding was performed. Runtime tests remain M6b's
separate responsibility.

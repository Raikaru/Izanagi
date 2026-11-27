# L1 cohort 4: directory replacement and Golden resource routes

## Decision and evidence boundary

This is a source/architecture specification for M10, **not an implemented loader**.
Main's 2026-11-13 direction permits D10c resource-representation replacement with
`runtime_status=pending-seam`, just as source-qualified tables await binding.
The [initial receipt](l1-directory-migration-proof.json) and
[followup receipt](l1-directory-migration-followup-proof.json) now decide all
**56 actual directory objects / 72,730 B as D10c**, runtime pending-seam.
The other **4 objects / 2,476 B are positively not directory records** and Main
transferred them to cohort 6; this followup does not edit their rows.
PROVEN, PROVEN-code, PROVEN-layout, D10a, D10b and OPEN-retain are zero in the
current 56-object directory scope. No runtime L1-open reduction is claimed.
Global counts belong to the global-count owner.

The D10c decision is **drop the PS2 physical directory representation, replace its
resource-routing role**, not drop resources, retain PS2 cosmetics, author guessed
Golden sectors, or claim that Golden lacks directories or file metadata.
Golden's functional successor is runtime archive metadata; it is not a Golden
literal corresponding to the PS2 disc-build-specific record. No old image bytes,
including alignment tails, are required by the specified Golden routes.

## Positive architecture-level proof

1. PS2 `H_Cdvd_SelectDirectory` at `00455230`, in pinned decomp
   `18016afe6:src/h_cdvd/h_cdvd.c:412-466`, normalizes the path, compares
   `normalized+4`, and selects one paired pointer from:
   - BTL: names `007107b0`, lists `00710800`, 19 nonempty names;
   - VOL/DATA: names `00711190`, lists `007113e0`, 146 nonempty names.
   The selected list is passed to `004f2798` using the corresponding volume work
   `00763e40` or `00763e44`. These are file-list records, not script instructions.
2. PS2 initialization `kwlnInitSubsystems` at `00100e60`,
   `src/Kernel/Kwln/kwln_inferred_00100e30.c:258-274`, opens four volumes in order:
   `CDV:/data.cvm`, `CDV:/bgm.cvm`, `CDV:/btl.cvm`, `CDV:/env.cvm`, and registers
   roots `00559840`, `005c4ec0`, `005c6350`, `005db850`, respectively. **BGM and
   ENV must not be swapped.** Their roots are not extra H_Cdvd name-table entries.
   Original `004f2798` dispatches through the CVFS interface word at `00743a48`,
   method `+40`; `004f8740` registers the per-volume getter `004f8548`.
3. Each of the 56 actual directory partitions is contained in a typed enclosing
   list: repeated count words at `+0/+4`, directory identifier `+8`, `#DirLst#`
   at `+12`, 24-byte header, `count` 48-byte entries, then 16-byte allocation
   alignment. Each entry has its name at `+14`, flags at `+12`; the first two
   are `.` and `..`. The receipt records actual roots, offsets, extents, hashes
   and pointer-table ownership, not nearest-address/module guesses. The selected
   partitions can begin inside a name or record. Their arbitrary M2 boundaries
   are **not** Golden object boundaries. The unexported entry prefix contains
   disc metadata; it must never be repurposed as Golden offsets.
4. Ordinary Switch 1.2 NSO `71003974a0` initializes the Golden SDK filesystem and
   CRI binder. At `39779c/3977ac/3977bc/3977cc`, the selected locale archive,
   `data.cpk`, `data_movie.cpk`, and `sysdat.cpk` are handed to `397850` (the
   locale-zero path omits the first). English selection uses `data_e.cpk`.
   `397850` asks for archive work sizes, allocates them, and calls `42d7b0`
   (`b 42d7c0`), then polls bind status: 2 completes, 6 is an error. The latter
   function's own `CriFsBinder_GetWorkSizeBindCpk` diagnostic identifies the
   CPK-binding operation. This is positive replacement architecture, not a
   failed search for `#DirLst#` in Golden.
5. The Golden request descriptor stores the **logical path** at `+10`.
   `397d20`/`397d38` copy that path and reference-count the request. Independent
   instruction inspection of the request pump (the worker decompilation loses
   jump-table successors) shows `397a78..397a88` passing binder `*[8815d0]`,
   descriptor `+10`, and output-size storage to `42f7b0`. Then
   `397b38..397b50` passes loader, binder, the same path, zero read offset and
   the **queried** byte size to `436b40`. This path carries no PS2 directory-list
   pointer or PS2 sector. Its 2048-byte allocation rounding is not evidence for
   static PS2 directory equivalence.
6. Fresh static parsing of Steam build 10836033 CPK header/TOC confirms archive
   filenames and named members. The existing recursive M1 inventory additionally
   locates startup-pack members. CPK `@UTF` columns `DirName`, `FileName`,
   `FileOffset`, `FileSize`, `ExtractSize` supply per-install metadata. The port's
   `tools/golden_mode/formats.py:198-221` and
   `src/platform/golden/golden_formats.c:276-365` implement the same named TOC
   framing. No file offsets or sectors are copied into this specification.
7. The class-specific Golden consumers below actually produce/use the matching
   logical names. Therefore, **along those resource paths**, the PS2 static
   CVM list has no corresponding Golden constant object: the bind/read graph
   obtains the routing facts from the player's Golden archive instead. This is
   representation absence with a positive functional successor, not exhaustive
   binary non-use, content identity, or a claim that every PS2 asset survives.

Private instruction/decompiler receipts and reproduction scripts are indexed in
`l1-directory-migration-proof.json`. They are not shipped game bytes. No `.arch`
code, game, emulator, or guessed archive window was used.

## M10 common route contract

Every admitted row names this contract and a seam
`golden-resource-route:<class>`. Mark a row bound only after **all** its class
routes, including native/inlined/translated requests, use the composed Golden
image without a PS2 ELF or CVM-list dependency. Until then it is pending-seam;
the existing port still routes general assets through its PS2 VFS namespace.
`src/platform/golden/golden_source.c`'s typed-table loader is not that binding.

- Logical source names, not PS2 file indices, sectors, directory ids, or M2
  candidate offsets, are authoritative. Sources below are Steam archive-member
  chains; the NSO is research evidence only, never a runtime input.
- For the supported English composition, resolve an exact logical path from the
  English archive when that member exists, otherwise the base archive. This is
  an explicit port composition rule, not an inferred CRI bind-priority rule.
  A source in `init_free.bin` must be resolved by its full pack-member name,
  after bounded pack parsing; do not mistake the pack offset for a CPK offset.
- Read CPK/pack extents, compression and lengths from that install's validated
  archive. The existing CPK/CRILAYLA/pack readers are the starting implementation.
  Never supply an old PS2 table as a fallback. A missing or incompatible required
  Golden member refuses that resource/qualification; it does not fall back to
  DATA.CVM, an adjacent member, a guessed sector, or zero-filled assets.
- Preserve full logical path identity (including directory) and the consumer's
  format. Case normalization may compare ASCII-insensitively as the existing
  CPK reader does, but must not collapse same basenames in different directories.
  For Golden AMD, PAC, SPR and archive members, use the corresponding Golden
  parser/translation route; replacing a directory does not prove asset-format
  compatibility with a PS2 parser.
- Producers publish a Golden-domain request identity when the owning event,
  field, battle or UI work is created. Capture that identity through pending
  reads; do not re-sample changing global state midway. Own cached pack bytes
  until the last consumer releases them, keep simultaneous requests independent,
  and publish completion through the port's scheduler boundary. Preserve the
  Golden consumer's load/poll/release dependencies; no synchronous success
  fabricated in a submitted asynchronous request.
- Required qualification: each class has a successful real composed-source read,
  an absent-member refusal, complete path/overlay identity, bounds/decompression,
  producer changes while a prior request is pending, consumer release/lifetime,
  and proof no PS2 ELF/list bytes are read. Tests must exercise direct and native
  routes, not only a standalone CPK parser. These checks remain M10's work.

## Admitted class routes and producer/state contracts

### `event-polymovie`

VOL `\\EVENT\\Eddd\\` -> `data.cpk::event/eddd/Eddd_nnn.PM1`, `.PM2`, `.PM3`
(and `.PAK` when the Golden packed branch requests it). Locale override uses
exact matching paths, not PS2 text or sector metadata. Golden `14bc60` formats
all four names, grouping `(eventNo/10)*10`, retaining the original event number
and subscene number in the basename. `14ac10` gets its owning task work through
`39fbb0`; s32 work `+0c/+10` are passed to that formatter. Work `+8` selects the
pack branch; `+4` selects the cached PM2/PM3 branch. Its state machine loads
PM1/PM2/PM3 separately or loads PAK then resolves the cached names, polls before
publishing, and retains handles at `+20/+28/+30` with data at `+50/+58/+60`.
M10 must reproduce that Golden request identity and branch/lifetime contract;
reusing PS2 scene allocation or directory order is not a producer.

The receipt gives each admitted directory's actual named-member join, including
Golden additions/deletions. It does not require byte equality or promise every
old filename survives. Main additionally admitted `\\EVENT\\E820\\` as
representation-only after pairing PS2 `290710`/`2938c0` with Golden
`14bc60`/`14ac10`. Its actual two names are **E826_021.PM1/E826_022.PM1**,
not E862. Golden has no E826 PM asset in the qualified install; **no rename
was inferred**. Missing asset lookup must fail closed. **M11 must confirm
Golden event scripts never request E826_021/022; reopen this row if they do.**

### `event-script`

VOL `\\EVENT_DATA\\SCRIPT\\` -> `data.cpk::event_data/script/Eddd_nnnL.bf`.
Golden `144260` gates on interpreter operation 2 and matching record/key, then
formats at `1442d0`: work u32 `+ad8/+adc` supply event/subscene; record s16
`+14` supplies `'A'+value`. It creates/reuses work `+a90` and dispatches
`1480a0(handle, 0x0f, path, record.s16(+16))`.
M10's producer is the Golden event interpreter record plus its owning scene
work, not an old BF byte offset. Preserve this gate, script variant and dispatch
argument; hold scene identity until completion. All 473 PS2 directory basenames
join the 683-member Golden subtree, but only the full Golden script source is
read. This proves routing, not BF instruction equivalence.

### `field-script`

VOL `\\FIELD\\SCRIPT\\` -> `data.cpk::field/script/comuNN.bf` for this selected
community-script family. Golden `1e6c60` formats `comu%02d.bf` and submits
`397d20` only for an unsigned id `<0x83`; it stores the handle in request `+18`.
Caller `1d78a0` derives the id from a loaded field record's first u16 (observed
`1d78a0` worker lines 184-217), stores it in request `+20`, and calls `1e6c60`.
Use that Golden field-record identity, request creation/release and `<131` gate.
Do not carry over the PS2 directory's 137-entry enumeration or invent a clamp
for absent ids. Other field scripts are not an authorization to alias this
community route; their own Golden producers select their full logical names.

### `field-grmd`

VOL `\\FIELD\\GRMD\\` -> `data.cpk::field/grmd/dobjNNN.AMD` / `fobjNNN.AMD`.
Golden `1d2cd0` formats `field/grmd/dobj%03d.AMD` at `1d3300` and passes the name
to model loader `387010` (kind 4, identity `fffd`, flags `20`). `1f8ba0` obtains
its signed group and numeric object id from `2e11c0(0/1)`; negative group builds
`field/grmd/fobj%03d.AMD`, nonnegative group builds `field/rmd/m%03d_%03d.AMD`.
Those live selectors come from the Golden field-script context, must be captured
with the model request and must not be translated from PS2 directory positions.
The latter branch is a distinct path, not a fallback search of GRMD. Golden's
AMD reader/model namespace is mandatory; renaming old RMD bytes is forbidden.

### `battle-boss`

BTL `\\BATTLE\\BOSS\\` -> `data.cpk::battle/boss/eHHH.bin`.
Golden `005d80` obtains the current boss/resource id from `046d80`, formats
`e%03X.bin`, submits `397d20(path,1)` and stores the handle in battle work
`*[880528]+1108`. Producer is Golden battle state/current-boss selection, not a
PS2 file index. Capture that id when loading the battle; retain/poll/release the
handle with battle work. Sixteen original basenames join the Golden directory,
which has eighteen named members. No difficulty or boss-script identity is
inferred from that count.

### `skill-effects`

BTL `\\SKILL\\` -> `data.cpk::skill/HHH.pac` or `skill/HHH.ACB`.
Golden `063340` reads u16 skill id at request `+4` and request flags at `+10`.
For the observed enabled path, battle-work byte `+14` bit0 selects ACB versus
PAC. PAC uses the shared handle at battle work `+14c0` and increments `+14c8`;
ACB stores its own handle in request `+8`. The producer must supply the Golden
skill identity and these Golden mode/state bits, not map PS2 BED/SE file indices
onto PAC offsets. Preserve the disabled branch (request `+10` bit0 clears its
handle), cache references and release lifecycle. Numeric spelling is uppercase
hex `%03X`, not decimal `%03d`; do not impose a PS2 roster limit.

### `minimap`

VOL `\\SMAP\\` / `\\SMAP\\SYMBOL\\` ->
`data.cpk::init_free.bin::smap/i_mini_map01x2.spr`,
`data.cpk::init_free.bin::smap/symbol/smap_p.tmx`, plus the owning Golden map
bank `data.cpk::field/fld_smap.bin`. Golden `2b8d80` state 0 first probes cached
SPR identity and otherwise loads it; states 2-4 load/parse/release the symbol;
states 5-9 load the map bank, resolve thirty named members, parse, and release.
The live producer is this Golden UI/task lifecycle and bank-member selector,
not the old `FLD_MAP_01.SPR` list position. Use complete named startup members,
whose sizes/hashes are in the receipt. A directory gate must not prevent cached
startup assets being found. This is routing/ownership proof, not visual identity.

### `staff`

VOL `\\OPED\\` -> `data.cpk::oped/p4gstaff.bin`.
Golden `2c1ab0` owns staff work at `4a70d78`, checks it exists and its `+4` loaded
bit is clear, submits `397d20(path,1)`, stores the handle at `+10`, and sets bit0
of `+4`. The producer is the Golden ending task and its load lifecycle; retain
until its Golden staff consumers finish. Do not retain PS2 `P4STAFF.BIN`,
`STAFF.BIN` or font assets as supposed Golden substitutes.

## Completed directory scope and transferred exclusions

The initial 43 admissions are supplemented by 13 representation-only D10c
decisions below. These prove the resource-routing replacement, not identity
of every asset formerly enumerated on the PS2 disc. Golden scripts and native
producers choose the Golden resource names/cues. There is no old-name fallback.

Main transferred four positively non-directory objects to cohort 6:
`005dcaf8` (`clear ot`), `005dcb28` (`sequence.c`), `005dcb98` (`debug menu`),
and `005dcc6c` (a 2,404-byte sequence-owned object). All are outside the last
ENV directory allocation. Their rows are untouched by this followup; this
contract makes no D10c or dead-code claim about them.

## Exercised evidence

Static PS2 list/header/extent decoding; original pointer and volume registration;
Steam CPK TOC parsing; recursive inventory member identity; ordinary NSO worker
and independent pivotal instruction inspection. Scripts use the fixed Python
3.12 under Git Bash, never WSL. No build, unit suite, objdiff, runtime binding,
BBV, game/emulator execution or protected Steam `.arch` decoding was performed.

## Followup class routes: all remaining directory records

The following contracts close the initial thirteen directory blockers.
Their sources, cue identities, actual producer instructions and receipt hashes
are in the [followup proof](l1-directory-migration-followup-proof.json).
The common fail-closed, lifetime, full-path and no-PS2-image requirements above
apply to every class. These are M10 implementation contracts, not a claim that
the existing asset backend already implements them.

### `effects`

VOL `\\EFFECT\\` routes both enumerated resource roles through the Golden
startup pack:

- `data.cpk::init_free.bin::system/effect/effpack.bin`;
- `data.cpk::init_free.bin::battle/effect/z0.tmx`.

This is not a guessed mapping to the high-resolution
`tex2/battle/effect/z0.tmx` probe. The named startup TMX member is 8,320 B;
the separate tex2 source is 262,208 B and has a separate texture-namespace role.
Golden `385090` probes/loads the exact EFFPACK path, waits for completion,
resolves 22 `GENERAL_%02d.tmx` members and releases its temporary file handle.
Golden `34dc40` independently probes/loads the exact Z0 name, waits, creates
the texture through `33b3a0`, stores it at `51c8ad8`, and releases the request.
The producer is renderer/effect initialization, not a PS2 list index. Preserve
the pack's cache lifetime, texture ownership and actual Golden texture-loader
conversion; do not put a tex2 byte window into the startup member or reuse
PS2 EFFPACK/Z0 data.

### `facility`

VOL `\\FACILITY\\` becomes explicit Golden facility ARC/member lookup, not
a global directory selection or twenty aliases for PS2 loose assets.
This is supported by actual reader/state/parent-archive correspondence:

- PS2 `330f20` waits for directory loads, requests `velvet01.spr`,
  `bonus_exp01.spr`, `dot.tmx`, `num01.tmx` and its other shared resources,
  installs the message window and releases completed directories in stages
  (`y_fclCombineDraw.c:6010-6088`).
- Golden `154bf0`, work state u16 `+f2`, state 2, issues fourteen
  `2e4e30(logical_name, 0, "facility/cmbroot.arc")` requests. They include
  `velvet_01x2.spr`, `velvet_arrowx2.spr`, `chara_magax2.spr`,
  `bonus_expx2.spr`, `v_icon.spr`, message BMDs, BF/CTB/search/plugin members.
  It retains handles at work `+f8` through `+160`, waits for readiness before
  using data, and owns the resulting sprite/message resources. The distinct
  child-list constructor `153150` uses parent `facility/cmbpslist.arc`;
  do not substitute CMBROOT just because the child names overlap.
- Golden shop-base constructor `1987f0` stores its actual descriptor at work
  `+860`. Its update `198910` reads the name-array pointer at descriptor `+8`,
  the parent ARC name at `+10`, and requests exactly
  `4 + u16(+18) + u16(+1c)` entries before publishing parsed resources.
  For the observed weapon descriptor `764fe0`, the parent is
  `facility/weapon.arc`; the five names are `msg_weapon.bmd`, `weapon.bf`,
  `weapon.ctb`, `weaponx2.plg`, and `shop_daidarax2.spr`, each under `facility/`.
  The observed additional sprite-count field at `+1a` is not the load-loop
  count. These descriptor inputs come from the owning Golden constructor,
  not the old loose-file order or the PS2 shop's nine-bank selectors.
- Shared Golden loader `2e4e30` first probes the full logical-name cache.
  If an explicit parent ARC is supplied and cached, it strips only the
  logical path's directory and resolves that basename **within that parent**
  through `397040`. Otherwise it starts `397d20` on the explicit parent ARC;
  its poll/take/release path completes the member lookup. Thus global duplicate
  basenames cannot select a member from an unrelated archive.

Steam framing positively locates the shared sprites and `ps_model.bin` in
`data.cpk::facility/cmbroot.arc`, and the weapon assets in
`data.cpk::facility/weapon.arc`. Golden scripts/native constructors choose
the new resources. **This does not admit any PS2 UI constants as Golden values,
or assert that old SHOP_SPR/JMES/METAL sprites match the new UI.** M10 must port
the Golden constructor/descriptor producer and complete parent-qualified route;
old UI opens must not survive as silent PS2 fallbacks. M11's resource coverage
must exercise all live facility/child/shop constructors and reject an unqualified
old-name request, rather than declaring the directory bound from one sprite.

### `model-field` and `model-npc`

The paired generic model callback is now positive evidence, not just an AMD
filename search. PS2 `2312e0` registers model-path callback `2308a0`; Golden
`280d10` passes `280d60` to `385e60`, installing slot `51c8f40`, which
`385f30` dispatches. Both callbacks distinguish field kind 9 and NPC kind 5.

- `model-field`: VOL `\\MODEL\\FIELD\\` -> Golden logical
  `model/field/fc_%02d_%02d.AMD`, with the high and low bytes of the u16 model
  identity as the two decimal arguments. The Golden callback appends `.AMD`,
  not PS2 `.RMD`. The source includes complete named CPK members such as
  `data.cpk::model/field/fc_01_00.AMD`.
- `model-npc`: VOL `\\MODEL\\NPC\\` -> Golden
  `model/npc/n<id>_<variant>.AMD`; ids greater than `0xc4f` use `model/npc2`.
  The kind-5 callback calls `281430` with the actual u16 NPC id. That helper
  uses cached `event/npc_def.dat` from `data.cpk::init.bin`, count at `+4`,
  four-byte id/mask records at `+8`, first matching id, and the live selectors
  obtained from `c9d40`/`caba0` and `1f0520` to choose the numeric variant.
  A missing/ nonpositive definition takes the actual Golden variant-0 path.
  Do not treat the zero-variant *algorithm branch* as permission to invent
  zero-filled resource bytes. Capture the Golden calendar/context and NPC id
  with the request; preserve the selector bit tests in `281430`, not the
  old PS2 RMD variant table. The 1,508-byte named NPC definition and complete
  AMD members, not the NSO, supply runtime data.

Model producers must publish Golden `(kind, id, variant/context)` identities
before starting reads. Keep model cache/refcount/attach/release ownership and
the registered PAC-path callback `280f30`: a packed model changes its resolved
extension to PAC and uses the corresponding `model/pack` basename. Some enemy/
player AMDs therefore exist inside PAC/cache sources, not as direct CPK files.
A zero top-level filename count is not absence and must not defeat this route.
Actual Golden AMD/PAC parsers and constructors are required; changing an old
RMD extension is not conversion.

### `battle-root`

The BTL root `005c6350` has **no regular files**: only `.`, `..`, `BATTLE`,
`MODEL_BTL`, and `SKILL`. Its routing union is now closed by the positive
registered model callback above plus `battle-boss`, `skill-effects`, effects
and battle-audio consumers. PS2 `2308a0` uses `model_btl/` for enemy/persona;
Golden `280d60` selects the `model/` logical namespaces, with PAC handling
through `280f30`. Battle/skill code already submits explicit Golden names to
the SDK binder/cache. M10 must route this **whole union** before binding the
root seam; do not fabricate a root table, keep MODEL_BTL on a PS2 fallback,
or confuse virtual model/PAC namespaces with direct CPK members.

### Shared Golden audio architecture and live state

The five audio classes below replace ADX/AHX loose-file and PS2 SPU-triplet
routing with actual Golden CRI ACB/AWB bank and cue routing. They are not
assertions of acoustic equality or a guessed cue-to-wave/AFS offset map.

**Sources and bank installation.** Golden `276da0` registers:

| Bank | Actual source in `data.cpk` |
|---|---|
| 0 | `sound/adx2/bgm/snd00_bgm.acb` + `.awb` |
| 21 | `sound/adx2/bgm_en/snd00_bgm.acb` + `.awb` |
| 4 | `sound/adx2/comse/snd04_comse.acb` (no separate AWB path supplied) |
| 13/14/15 | `sound/adx2/btlmem.awb`, `btlhlp_r.awb`, `btlhlp_k.awb` |
| 17/18/19 | corresponding `sound/adx2/en/` AWB paths |

`3a1610` registers `sound/adx2/common.acf` against the same CRI filesystem
binder and creates sixteen channel/player records. `276c30` waits for bank
readiness, then publishes bank 0 to channels 0/2/3, bank 4 to channel 4, and
the three battle-voice banks to channel-1 multiplex slots. Later binding goes
through `3a23f0`/`3a25c0`; never assume a channel's bank is permanently fixed.

**Load and release state.** `3a1ec0` queues ACB paths; `3a2330` queues AWB-only
paths. A bank record is `238` bytes: used/state/type at `+0/+4/+8`, ACB path
`+c`, AWB path `+10c`, handles `+220/+228`, lock `+230`. Stored paths have the
`app0:/data/` prefix. The `3a3dc0` scheduler, not a PS2 directory selector,
passes binder `*[8815d0]` and paths at `+16/+116` (the logical names after
that prefix) to actual CRI ACB/AWB loaders. Queued used=1/state=1 becomes
state=3 only after a nonnull loader result for the submitted bank types 0/1/2.
`3a1370` checks readiness; no claim is made for an invalid, unauthored type.
`3a1fc0` first detaches referring channels/multiplex slots, then releases
ACB/AWB handles and clears used/state/handles. M10 must preserve those ownership,
readiness and language-partner dependencies, not report an allocated bank ready.

**Persisted voice selector.** Getter/setter `3112c0`/`3112b0` access raw u32
state `050a7300` through relocation slot `881290`. It is process-zero BSS.
The complete configuration reader starts at **1136e0**, not the cached
mid-prologue label 1136f4. At `113714` it formats default decimal **0**; it
reads `SYSTEM/Voice`, parses the resulting string, and passes the actual w0
value to the setter at `113748`. M10 must hydrate this state through the
Golden configuration lifecycle, preserve the raw setting across reads, and
publish setting changes through the same state contract. Do not derive it
from text-language selection or invent an always-English/always-zero fallback.

Normalizer `3a11f0(bank,cue)` preserves the bank when Voice is zero. For
nonzero Voice it considers `13/14/15/16/0 -> 17/18/19/20/21`, from the actual
five-word map at `63c9f4`, only after the base and alternate are ready.
AWB-only type 2 uses the alternate; an ACB alternate is used only when
`3ee8a8` confirms that cue exists, otherwise the base remains selected.
Preserve this per-cue fallback **between Golden banks**; it is not a PS2
fallback. `3a27d0` takes `(channel,cue)`, checks bank readiness and current
channel state, normalizes the bank and starts the actual Atom cue/AWB selection.
Keep channel-0 transition/fade state, channel-1 pending state and bank releases
coherent when settings or requests change.

**Source-reader limitation.** The research receipt decodes the complete
900-entry CueTable/CueNameTable in each BGM ACB and the 249-entry common table.
The outer ACB UTF variant declares a 415-byte row but contains 439 bytes of
column storage; the extra 24 bytes are three appended columns. The bounded
research reader explicitly does not parse those appended columns. Existing
CPK UTF code rejects this ACB variant; it was not weakened or changed.
M10 must supply a real compatible ACB/playback reader (or the appropriate
native backend), with the full cue/sequence/track/waveform graph and lifetime.
**CueId is not an AWB member index.** These metadata proofs do not authorize
skipping bad fields, synthesizing silent clips, or claiming playback support.

### `audio-bgm`

BGM physical root -> banks 0/21 above, using Golden producer cue ids through
`3a27d0(0,cue)`, not PS2's `7118b4` twelve-byte track-to-file table.
The old main-channel request `459ad0` selected `BGM:%s`; the positive Golden
replacement loads named ACB/AWB banks and selects a cue. Source cue names
include `BGM_001`/id1 and `BGM060_ENV380`/id60. Only 56 of the 107 old disc
basenames have that simple name join; **do not extrapolate missing 100..115
filenames to CueId100..115**, or retain old music for them. Golden battle,
calendar/event and menu producers supply their actual Golden cues and
transition requests. M11 must cover their live requests against the source
bank's actual CueTable, not reconstruct them from the old disc enumeration.

### `audio-env`

ENV physical root -> the same Golden bank family, with environmental cues.
All 66 old `ENV_301` through `ENV_366` names have actual Golden CueIds301..366;
the source adds environmental identities through405. PS2 `45b2e0` instead
indexed `711f24` and opened an ENV filename on its channel-3 request.
Golden startup explicitly binds channel3 to bank0, and the new cue API chooses
the actual Golden environment identity. Do not carry over the PS2 track-index
domain `0..65`, re-index an AWB by cue number, or drop added environment cues.
The producer is the owning Golden field/event environment state; capture that
cue and bank/channel with pending/fade requests and preserve normalizer state.

### `audio-event`

VOL `\\SOUND\\EVENT\\` (all three selected partitions of one actual list)
-> Golden bank0/21 ACB/AWB EVE cue identities. The paired event-SFX wrapper
is explicit: PS2 path readers `298130`/`299920` name `EVE_%03d.ADX`;
Golden `14d4c0` still formats that string locally but **does not open it**.
It submits `3a27d0(channel, eventSoundId + 1000)`. The native event command's
channel and id are the live producer; do not treat the formatted old path as
a Golden filesystem request.

The old list has307 files;305 names join the Golden EVE names. The remaining
two do not need a guessed file mapping: actual source CueId1125 is named
`dammy_3` and CueId1525 is `dammy_20`, exactly the ids produced for125/525.
These are real Golden cue programs, **not an authored silent fallback**;
their names/lengths alone do not prove silence. Play the actual source graph.
Separate voiced-scene prefetch `148680` installs AWB bank1 from Golden scene
work `+ad8/+adc` and Voice, selecting the literal `v%03d%03d.awb` or the
communicated-scene bank as its own branch. Do not mix that AWB wave-id namespace
with EVE ACB CueIds. M10 must port both native/event routes coherently.

### `audio-battle`

VOL `\\SOUND\\BATTLE\\` -> Golden battle-sound producers using the same
bank/cue API, plus the explicit battle-voice/skill/model bank routes above.
Seventeen old names have actual source cue identities: `SK_100/101/102`
are1900/1901/1902; boss ending names such as `EM100_END` are present.
Golden `005d80` loads the boss resource by the current boss id; its ending
sound consumer `0060a0` gets that id from `046d80`, checks the actual
`0x200def` mask over `(id-0x200)<22`, then submits channel2's cue from
`629fcc + 4*(id-0x200)`. Independent instructions at `063810` show the
generic battle packet adding1000 to its s16 cue field before calling
`3a27d0` with its s16 channel.

`BOKO.ADX` and `EM109_END.ADX` have no admitted same-name asset mapping.
**Do not map them by adjacency to `dammy_0`/1907 or `dammy_5`/1912.**
The positive replacement is the Golden battle producer and actual cue/packed
resource graph, not an assumed one-for-one old-file map. All-out, skill,
boss-end and model-linked sound requests must use their Golden producers;
any surviving legacy file open keeps the seam unbound. M11 must cover those
branches and real Golden cue validity before runtime qualification.

### `audio-common`

VOL `\\SPU\\` -> Golden bank4 `sound/adx2/comse/snd04_comse.acb`.
The PS2 registered triplet table `712390` has an empty entry followed by
`spu/comse.hd/.bd/.sq` and `spu/bse.hd/.bd/.sq`.
`sdkSndUpdateDecodeSlots` loads those three physical files in stages, transfers
the data to IOP allocations, and creates SDK queues/decoders. This specific
representation is replaced by Golden's binder/Atom bank lifecycle, not merely
declared absent because the PS2 SPU differs from PC hardware.

The Golden ACB contains132 `comse` and117 `bse` cue identities.
Native producer `3a36d0` uses family0 ->10000, family1 ->20000,
then `100*sequence + cue`; other families return without playback.
It submits on channel4's bound common bank. **Use actual CueTable.CueId,
not a number parsed from CueName:** source label `bse05_07` has id20508 and
`bse05_08` has id20507. Preserve that real label/id disagreement; do not
“correct” the source. The producer arithmetic still selects20507 for
family1/sequence5/cue7 and thereby plays the actual Golden20507 program.

The TEST triplet and BSE.SE directory entries are not declared dead, silently
replaced by COMSE data, or given fabricated cues. This D10c removes static
directory metadata along the positively established common-bank route.
Golden debug/other live producers must request actual Golden bank/cue resources;
an old triplet file open is not authorized and cannot qualify this seam.


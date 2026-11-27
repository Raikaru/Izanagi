# M8f battle effects: specification, shared owner and acceptance

**Census complete; BED400/EPL/EP framing specified; full binary-independent playback/render semantics remain OPEN.** This document does not close M8f or lift M7aR2’s resource refusal. In particular O03/O04 are substantive format/renderer gaps, not merely a request for screenshots.

## Deliverables and source boundary

- [Every named BED/EPL/EP/PB asset, counts and hashes per CPK/containing archive](m8f-census.md).
- [BED400 header, banks, slots, attachment fields and bounded decoder contract](m8f-bed400.md).
- [EPL roots/parts, primitive descriptors, versions, sharing and other families](m8f-epl-ep.md).
- [Measured kind/version/size census and static code receipts](m8f-effect-grammar.md).
- [Adjacent TRCK/BVP/common-texture-pack discovery and identities](m8f-discovery-boundaries.md).
- [Common particle parameters, exact effect RNG/keyframes, Golden sample interpolation and EPL render state](m8f-emitter-render-state.md).
- [Helix/orbit/cone/angular-cone behavior3..6 suffix, state and playback equations](m8f-emitter-behaviors-3-6.md).
- [P2A/TLP0 byte grammars, HFLC/MD5/tex2 binding and image/request ownership](m8f-texture-bindings.md).
- [Embedded P2A/TLP0 counts per CPK/parent archive and complete identity hashes](m8f-adjunct-census.md).
- [Sway behavior11, including its distinct preroll and signed floor-bounce recurrence](m8f-emitter-sway.md).
- [BattleOnly primitive31 callback phases, clone/destruction owners and R_BLOCK subtype6 dependency](m8f-battleonly-lifecycle.md).
- [Swirl behavior10, indexed direction alternation and separate preroll/active equations](m8f-emitter-swirl.md).
- [Floor-bounce8 and sphere-boundary9, compact state indexing versus allocation, and noisy reflection](m8f-emitter-floor-sphere.md).
- [Bezier7 control points, monotonic progress, Rodrigues rotation and preroll differences](m8f-emitter-bezier.md).
- [Ordinary trail history/fade, linear/cubic interpolation and legal low-count edges](m8f-particle-trails.md).
- [Concrete BattleOnly1..12 primary schemas, actual registration and R_BLOCK animated-light controller](m8f-battleonly-subtypes.md).
- [Model26/helper27/drawable32 primary layouts, clocks, clone ownership and logical projection](m8f-model-helper-drawable.md).
- [All2403 adjunct3 and104 helper-inline model spans, per-archive counts and537 identity hashes](m8f-model-adjunct-census.md).

Steam build10836033 ordinary `.shared` code/data and CPKs, authorized Switch1.2 NSO, private GoldenRE notes and the PS2 decomp are the only sources. No `.arch` decoding, game execution, emulator or proprietary source import. Static parser facts are distinguished from remaining semantic and runtime qualification gates.

The named census is **4400 logical objects,881 distinct extracted SHA identities**:2397 BED400,747 eight-byte dummy BED,1107 EPL,148 EP and one PB tool object. It retains locale/nested duplicates. Recursive EPL/EP traversal gives1231 EPL roots,10968 parts and10199 EP descriptor references per distinct outer identity. These are not claims of10968 unique GPU effects or4400 legally reachable battle effects.

The embedded-family annex adds702 physical-descriptor P2A occurrences/43 identities and188 TLP0 occurrences/23 identities; these are spans within the4400 roots, not additional root files. Four HFLC registry files are separately counted/hashed in the binding spec. R_BLOCK's real external image is proved through raw-payload MD5→HFLC nameIndex565→`tex2/battle/effect/R_BLOCK/000.tmx`, not a guessed filename association.
## Shared common-resource installation and ownership

The authoritative [R2 common-resource/helper addendum](effect-interception-resources.md), committed in5ca8dc8774a03eb83ad687c4e7710167e17b98ba, is shared with [M7aR2 packet binding](effect-fields.md#0x800-interception-target-packet-binding-addendum). Reuse that owner; do not create independent interception copies.

| Property | Binding |
|---|---|
| Source | `data.cpk::battle/btleff.bin` |
| Container | PAK-v2,49 ordered members |
| Archive bytes/SHA |398888 decimal; `96f82ee716f7f947c82a980b480bec1dbf08c4dd4958e3fd9105d56c97ca53d7` |
| Extra common member | ordinal48, `R_BLOCK.BED`,128448 decimal bytes |
| Member SHA | `8201b318b89d76c5a67954d5c65af81838aec40045d9ac4a1d6c8633836bcb86` |
| Source byte offset |270440 decimal within extracted archive |
| Common handle slot | Steam battle+0x1490 / NSO battle+0x14A8 |
| Ready publication | battle flags OR0x40000 only after all49 installations |

The slot holds a **formation-work handle**, not a BED byte pointer. PS2 `formations[48]` has legal indices0..47 only. Native Golden mode must expose a typed Golden sidecar for logical common index48 without changing PS2 guest layout, writing past the native48-slot array or overwriting reflection/absorption slots9/10. The same Golden common owner supplies coherent entries0..47.

Installation sequence: obtain the real archive through the normal loaded-data/asynchronous request path; retain a complete owned decompressed archive copy; resolve every member by its ordered archive index; decode/construct the real Golden formation resource using BED400/EPL/EP decoding; install all49 handles; release/null the request; publish readiness. No pointer into an extraction temporary, fake non-null handle, synthetic blank formation, delay-only effect or PS2 BED fallback implements this sequence. A merely parsed graph with unsupported live primitive semantics cannot be published as render-ready.

This is one battle common-resource **generation**, not per-action/per-turn state. Handles borrow from the retained whole-archive allocation under normal bit26-clear ownership. Common clear/reload destroys every live formation handle, nulls all49 slots, clears0x40000, releases/nulls any request, then frees/nulls the single backing copy. In bit26-set per-entry ownership mode, destroy/free according to each real individual data owner rather than freeing borrowed subspans as a whole archive. All outstanding type301/EPL/model/texture borrowers must finish or be cancelled through their normal teardown before backing is freed. Invalidate generation-scoped decoded/GPU caches with this same event.

M7aR2 consumes the real Golden common-interception handle through this owner. Its Ii constructor receives native source/target units,mode1/options0 and the packet dependencies/queues specified in the R2 addendum. `M8f-res:battle-common-14A8-1490` remains until decoder **and** real semantic owner exist. Do not clear the extra handle when184/lead is reset, or construct an independent M7a cache.

UI/audio followers keep their independent902/A02/506 lifecycles: real channel cue20211, immediate bank0x0D/channel3 voice request0x6AC/0x6AD after the source’s one RNG draw, and target message0xBB with floating timer. A02 is not the nativeA03 multi-step packet. This spec does not replace these operations with completion acknowledgements or claim to qualify audio.

Reuse R2's shared [**`golden-live-frame-step`** native facade](effect-interception-resources.md#shared-golden-native-live-clock-facade)
for floating presentation timers. The addendum records the live producer, units
and start/reset points; it does not name a nonexistent PS2 frame-step getter.
The902 cache has16 float-age rows, expires at>=3, and remains distinct from the
separate signed8 rotating-queue timer. The real presentation reset is
`btlSoundCancelPlayback`, followed separately by the bank3 fade.

### Exact common entry order

| Ordinal | Member |
|---:|---|
| 0 | `ATC_D.BED` |
| 1 | `ATC_Z.BED` |
| 2 | `ATC_J.BED` |
| 3 | `ATC_D_J.BED` |
| 4 | `ATC_Z_J.BED` |
| 5 | `ATC_J_J.BED` |
| 6 | `ATC_CA.BED` |
| 7 | `COMB_F.BED` |
| 8 | `AT_JIMEN.BED` |
| 9 | `HANSYA.BED` |
| 10 | `KYUSYU.BED` |
| 11 | `HATUDO_S.BED` |
| 12 | `HATUDO_H.BED` |
| 13 | `HATUDO_P.BED` |
| 14 | `TUKEKAE.BED` |
| 15 | `HATUDO_E.BED` |
| 16 | `SHADOW_S.BED` |
| 17 | `SHADOW_D.BED` |
| 18 | `GEKIKOU.BED` |
| 19 | `KYOUHU.BED` |
| 20 | `CLOSE.BED` |
| 21 | `PANIC.BED` |
| 22 | `KIZETU.BED` |
| 23 | `POISON.BED` |
| 24 | `SUIZYAKU.BED` |
| 25 | `ROUKA.BED` |
| 26 | `AYATURU.BED` |
| 27 | `PER.BED` |
| 28 | `SLIP.BED` |
| 29 | `DOWN_H.BED` |
| 30 | `DOWN_J.BED` |
| 31 | `DOWN_S.BED` |
| 32 | `DOWN_Y.BED` |
| 33 | `ITEM.BED` |
| 34 | `SYUCYU.BED` |
| 35 | `TUIGEKI.BED` |
| 36 | `AUTO_DK.BED` |
| 37 | `AUTO_SA.BED` |
| 38 | `AUTO_SR.BED` |
| 39 | `AUTO_HJ.BED` |
| 40 | `BST_KH.BED` |
| 41 | `AGI_CRT.BED` |
| 42 | `ZIO_CRT.BED` |
| 43 | `BUH_CRT.BED` |
| 44 | `GAL_CRT.BED` |
| 45 | `SP_ATTK.BED` |
| 46 | `DEATH.BED` |
| 47 | `SPOT.BED` |
| 48 | `R_BLOCK.BED` |

Archive-order indices are ABI inputs. Do not sort these filenames. Other copies such as battle_pack.bin/btleff.bin and localized archives remain separately hash-identified in the census; do not assume that an identical basename authorizes replacing the canonical owner source.

## Viewer witness targets (not yet exercised)

The owner’s viewer must consume the installed Steam asset through the native Golden decoder, with real model/texture/audio owners. Use an explicit seed, fixed source/target poses and frameStep trace; preserve the source’s effect RNG consumption independently of gameplay RNG. Capture render/decode diagnostics, effect-instance counts and resource-generation references alongside images, not screenshots alone.

Mandatory named inputs below cover every19 primitive kind reached from the census’s named battle-root BED/EPL graphs (including recursive adjuncts). This is a structural set cover, not proof of legal trigger reachability. The primary/common targets are mandatory even when a smaller set could cover the same primitive kinds.

| Witness | Exact input | Extracted SHA-256 | Primitive kinds reached |
|---|---|---|---|
| W01 | `data.cpk::battle/btleff.bin::R_BLOCK.BED` | `8201b318b89d76c5a67954d5c65af81838aec40045d9ac4a1d6c8633836bcb86` | 1, 4, 5, 10, 12, 26, 31 |
| W02 | `data.cpk::battle/btleff.bin::HANSYA.BED` | `53ae6478c20a1b15e53565901e90395c04ca6b92d8cc0674d523adc6c3eb4d18` | 10, 12, 26, 31 |
| W03 | `data.cpk::battle/btleff.bin::KYUSYU.BED` | `650969c19565418e23e9ea3e01981b0cd620bc7042508874da608c09f6403b8e` | 1, 12, 26, 31 |
| W04 | `data.cpk::battle/btleff.bin::AGI_CRT.BED` | `1ae395a2d246f22174626397e2c3edebc798f74f6543661a5c9f98da5bafe336` | 1, 31 |
| W05 | `data.cpk::battle/btleff.bin::ZIO_CRT.BED` | `777a117e709a560677df0d6352015f00d13693ad54709c981ae5ba1bd959dfbf` | 28, 31 |
| W06 | `data.cpk::battle/cutin/boko/koma2/2ICON.EPL` | `e8799a8ee97aba9524fcaf42838064d11cf6b4805c36cc1ecd960d876fdd6804` | 5, 32 |
| W07 | `data.cpk::battle/assist/ANALYZE.BED` | `69d0bd59a2196ef02335dcc731617df0a4fa2f374e8e4e891fce622e7cfd22aa` | 1, 2, 5, 7, 12, 26, 28, 31 |
| W08 | `data.cpk::battle/bike/B118_W.BED` | `7fd0c67f33cbcf875e9781c548b03b428312f270c95fa0d95fe928e860b11d81` | 1, 5, 10, 12, 16, 21, 26, 30, 31 |
| W09 | `data.cpk::battle/boss/e20B.bin::BED/e20B_END1.BED` | `8069c2bcc904b6fa9ed727768508d39a56ecc95f3ce509fbb4d92d5c9c1073fb` | 1, 4, 5, 10, 12, 13, 21, 27, 31 |
| W10 | `data.cpk::battle/assist/ASSIST_EF_S.BED` | `c568b47df3880ecbea1b2f36ae91421720851efb789d5c063966a0171a9ae86a` | 1, 2, 4, 10, 12, 15, 31 |
| W11 | `data.cpk::battle/boss/e206.bin::BED/e206_ENCOUNT.BED` | `5b9ca9679564c387f222fcd3aefbba7ca6b0719893a33f2f14f35e1d09b064ce` | 4, 9, 31 |
| W12 | `data.cpk::battle/boss/e20C.bin::BED/e20C_END.BED` | `98e314cd795aabd6134413891b58afcd5b06a5fa4c2668ab5014bb7c9e19ec9f` | 1, 21, 29, 31 |

### Required observable checks

1. W01 R_BLOCK: source/target units separated, rotated and unequal in size. Render common48 in mode1/options0; show its actual particles, model adjunct and filters. Run two simultaneous instances on different targets; clocks/positions must be independent although templates/resources may share ownership. Compare with W02 reflection9 and W03 absorption10: those are distinct assets/instances, not accepted R_BLOCK stand-ins.
2. W04 fire critical and W05 electric critical: establish particle versus object-particle animation, blend/depth and temporal evolution, not one static quad. Exercise start boundary0,1,2 and frame jumps across a slot trigger; preserve strict BED start comparison versus non-strict EPL part comparison.
3. W06 2ICON: standalone EPL screen/cutin input, with real kind32 animated Golden drawable and filter. It must not be silently parsed as a BED prefix.
4. W07 ANALYZE: track, textured polygon, model and particle dependencies. W08..W12 add wind/environment/line nova/thunder/helper/alternate track/distortion paths; both model and postprocess owners must participate. A fallback bounding box, white sprite or missing distortion is a failure.
5. For each, capture frame0,1,2,15,30,60,120,240 and its actual completion boundary; long-running effects additionally need a loop boundary and explicit cancellation. Frame values refer to the recorded simulation clock, not wall-clock elapsed time. Do not force a fixed240-frame completion on long effects.
6. Exercise an EPL clone and shared adjunct: distinct live transforms/clocks with correctly retained backing; teardown one borrower while the other still renders. Record exact live instance/refcount changes and final zero retained-generation references.
7. Cold load, request-pending, ready load, normal clear, reload into a new generation and cancellation during active effect. All49 handles must install/clear coherently and common48 may never point into freed archive bytes. Exercise bit26 ownership separately with a real producer, not an invented test-only flag toggle.
8. Integrate W01 with real M7aR2 Ii dependencies and902/A02/506 followers. Record constructor inputs, ready owner generation, cue/voice/message dispatch and completion order. Rendering, sound duration and packet completion are independent assertions.
9. Maintain separate PS2-mode regression witnesses. Golden decode/upload must not alter guest layouts, PS2 parser behavior, input/save formats or scene reference checksums.

Numeric framing/hash success alone cannot accept W01..W12. No viewer command or scene runner is invented here: the implementation owner must register these exact asset/pose/time targets in the existing viewer and run its real acceptance tooling.

## OPEN list and disposition

| ID | Unresolved item | Required closure / gate |
|---|---|---|
| O01 | Unusual packet completion/repeat option combinations and player-facing names of timing controls | Core source comparisons are now specified in the BED addendum: side0/side1 fade selectors, doubled duration/minimum2 completion, separate packet versus entry clocks. Qualify remaining caller-option combinations with deterministic witnesses. |
| O02 | Position/height edge combinations and unusual attachment13 cache/displacement cases | Attachment8/10 companion+F00 and9/11 companion+EE8, original-model/null-companion fallback, bone-failure transformed-cache fallback and common EPL transform flags are now specified. Qualify remaining legal poses without replacing unknowns with zero positions. |
| O03 | Primitive-specific geometry/postprocess schemas and exact BattleOnly unit/camera/render facade binding | Ordinary behavior1..11 and trail equations are specified. BattleOnly registration/table/primary schemas/envelopes, R_BLOCK helper-driven lighting and Model26/Helper27/Drawable32 primary/phase behavior are source-specified; remaining unit action/pose/camera/layer facade details and primitive/backend closure still block complete binary-independent rendering. |
| O04 | P2A group/paired draw composition, universal frame+14 semantics, TLP17D preserved trailer and exact camera/matrix/postprocess/render-backend adapters | P2A/TLP0 framing, ordinary playback/timing/UV controls, ownership and model/helper/drawable primary fields are specified and measured. Drawable960x540 projection constants and source operations are explicit; established engine composition/layer binding and remaining filter semantics still block complete other-family playback support. |
| O05 — source mechanism resolved | Raw-payload MD5, native strcmp/first-match semantics, selected HFLC name-table indices, exact tex2 path rewrite and request/image owners | Static R_BLOCK binding reaches an actual hashed installed image. Existing locale/mount resolver selects one registry, not merged tables. Runtime per-locale/cache/async qualification is part of O08; filename-stem guessing is no longer an OPEN source question. |
| O06 | Tool PB/PIB0 and ancillary TRCK full schemas/linkage, historical200 null-callback kinds8/11/14/17/19/20 reachability, AMD/GMO opaque effect adjunct links, possible unnamed effects in unrecognized containers | Keep counted and explicitly unsupported unless proven; do not infer legal battle use from editor sample presence or claim a universal no-other-format proof from named inventory alone. |
| O07 | Exact eight-byte dummy disposition through the legal formation/archive request route | Specify absent/sentinel result and all relevant callers; never feed the8-byte dummy to BED400. |
| O08 | Implementer-owned viewer/lifetime/audio/PS2 regression qualification | W01..W12 and common reload/clear are defined but unexercised. These runtime checks belong to implementation, not a prerequisite for this static source-spec lane. |

**Acceptance status:** requested census and source-grounded format/owner/witness documentation are delivered. The stronger request for complete field semantics sufficient for every effect to render without the binaries is **not yet satisfied**, specifically O01–O04. These are material OPEN work, not a claim that the assignment’s full format/playback acceptance passed.

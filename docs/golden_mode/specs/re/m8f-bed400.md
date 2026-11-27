# M8f BED400 wire format and decoder contract

Status: **wire framing and resource ownership specified; rendering semantics have explicit OPEN gates**. This is not an assertion that a Golden renderer exists. Read the [census](m8f-census.md), [EPL/EP specification](m8f-epl-ep.md), and [acceptance/OPEN list](m8f-battle-effects.md) together.

Notation: addresses, offsets, structural extents and strides are **hexadecimal** (for example630 bytes means0x630=1584 decimal). Counts, versions, frame values, scalar values and prose census byte totals are decimal unless marked hex. Kind selectors0..14 in the attachment table are decimal.

## Evidence and terminology

Sources are Steam build10836033 ordinary `.shared` instructions/data, its installed CPKs, Switch1.2 ordinary NSO, and the PS2 decomp at18016afe6. No game execution or `.arch` decoding. Addresses below are Steam RVAs or Switch virtual addresses with base7100000000. Decoder requirements below are native safety/ownership requirements; they do not claim the retail parser validates hostile inputs.

Steam79F50..7A144 and NSO7100032AB8..32D20 independently establish the same file-to-work conversion. The Steam loop has two banks, file stride314, work stride418,32 slots per bank, file slot stride18 and work slot stride20. Its7A0ED..7A0F0 resolves a direct slot as originalFileBase+u32Offset and calls507870. NSO32AFC/32B70 calls363B80 equivalently. Their FE skip / FF indexed clone / instantiated-bit2 operations agree. Steam's507870 EPL constructor is ordinary `.shared`; its50A030 conversion helper jumps into protected code. That jump was **not followed**. NSO363B80 supplies the ordinary-code field projection behind that boundary. No claim that all Steam helper internals were independently recovered.

The PS2 `btlFormationCopyRecord` at001D3EA0 similarly copies630 bytes then resolves offsets; this is contextual correspondence, **not authorization to call the PS2 parser on BED400**. Golden work contains widened native handles and Golden primitives/models/textures. Reuse operations only through a Golden decoder and owner.

All wire integers are little endian. f32 is IEEE754 binary32. Offsets have the base explicitly stated below. Wire objects never contain host pointers. Do not map a C struct containing native pointers over these bytes.

## Overall layout

| File offset | Bytes | Meaning |
|---|---:|---|
| 0 | 8 | Exact signature `BED400\0\0` |
| 8 | 314 hex | Bank0:14-byte header +32 slots of18 bytes |
| 31C | 314 hex | Bank1, same grammar |
| 630 onward | variable | Embedded EPL graphs/resources and alignment padding |

Both banks are physically present even when their live count is zero. No section directory or file-size word exists in this630-byte prefix. Slot offsets locate the embedded objects; the decoder must not treat every630-byte suffix as one self-contained EPL, nor use filename extension to decide a suffix's format.

All458 distinct BED identities measured have this400 signature. The3144 named `.BED` occurrences are **2397 BED400 plus747 eight-byte dummy objects**. All747 dummies have one distinct SHA and exact bytes `01 00 77 76 78 77 33 72`; they are not another BED version. Their known absence/sentinel disposition must be retained by the archive/resource resolver; treating them as a zero-duration BED, reading630 bytes from them, or emitting a fake successful formation is forbidden. Exact sentinel-to-loader behavior is OPEN O07.

No BED100/200/300/other400 subversion was observed. Do not confuse the embedded EP version values200/210/220 with BED's400 signature.

## Bank header:14 bytes, relative to bank base H

| Relative offset | Wire type | Operational meaning |
|---|---|---|
| 00 | u32 | Timing selector returned for side0 by32D70; file08 in bank0 |
| 04 | u32 | Duration/repeat-refresh timing word; compared with integral playback time by33760; corresponding PS2 duration getter001D4190 |
| 08 | u16 | Live slot count, legal range0..32 |
| 0A | 2 bytes | Preserved; converter does not assign semantic fields from these bytes |
| 0C | u32 | Other timing selector; bank1 value returned for side1 by32D70 (file328) |
| 10 | u8 | Unit-dependent effect scaling selector;0 none,1 source, other nonzero target in33760 |
| 11 | 3 bytes | Preserved; not interpreted by the conversion loop |

The first, second and fourth words are not interchangeable, and the source does not establish a user-facing name such as 'windup' for all three. Their complete packet scheduling interpretation is O01. Preserve them separately rather than normalize to one duration. Unsupported side queries return0. A bank with zero slots may still carry nonzero timing controls.

## Slot:18 bytes, S=H+14+index*18

| Relative offset | Wire type | Meaning |
|---|---|---|
| 00 | u8 | State/kind;07 direct EPL in every measured live slot;FE absent;FF indexed clone in parser capability;FD non-owning runtime state |
| 01 | 3 bytes | Preserved padding |
| 04 | u32 | BED-base-relative EPL offset for direct entries; packed bank/slot index forFF |
| 08 | u8 | Attachment type, dispatch table below |
| 09 | u8 | Height mode |
| 0A | u8 | Position mode;08 center,09/0A view-relative variants; full selector tables below |
| 0B | u8 | Preserved padding |
| 0C | u16 | Matrix/bone index for bone attachment; explicit radius override for radial modes |
| 0E | u16 | Camera-direction displacement in special attachment13; type-dependent, not universally a bone index |
| 10 | u32 | Start time; activation uses **start < trunc(playbackTime)**, not <= |
| 14 | u8 | Runtime auxiliary selector copied to work+3C; all measured live values0 |
| 15 | 3 bytes | Preserved padding |

There are797 live slots across458 distinct BEDs: all kind07;28515 physical slots areFE. No live slot has zero offset. These are identity-deduplicated observations, not logical occurrence counts. FE slots must not be dereferenced, even if their remaining bytes are nonzero. Count only the live prefix indicated by the bank, not all32 physical slots as effects.

ForFF, split `bank=resource>>5`, `slot=resource&31`; resolve the referenced already-instantiated formation entry and clone its EPL instance, inheriting its kind. Both Steam7A0BE..7A0E9 and NSO32B98..32BC0 prove this. Reject out-of-range, absent, cyclic or unavailable references in a safe native decoder; do not guess thatFF is a file offset. FF occurs in parser code, not in the measured live BED set. FD is an instance-ownership state: playback may reuse a template when formation flag4 allows it, marking the entryFD; do not serialize that runtime mutation into an immutable file image.

### Attachment dispatch and numeric controls

NSO relocated table710070A878 has15 entries. Its offsets below omit7100000000:

| Type | Callback | Operation established by ordinary code |
|---:|---|---|
| 0 | 32FE0 | Source bounds/radius, local offset rotated into cached source orientation, then radial placement43C0 |
| 1,7 | 330D0 | Source model matrix/bone position; fallback to cached transformed source offset |
| 2 | 331A0 | Group bounds selector1 for source genus0, otherwise2; radial placement |
| 3 | 33230 | Complementary group-bounds selector2 for genus0, otherwise1; radial placement |
| 4 | 332C0 | Combined group bounds selector3; radial placement |
| 5,6 | null | Not legal to invoke; neither appears in live measured slots |
| 8,10 | 33340 | Alternate model/companion attachment path |
| 9,11 | 33440 | Alternate model/companion attachment path |
| 12 | 33540 | Zero XYZ output |
| 13 | 33550 | Bone/pose attachment with optional cached position and view-direction displacement |
| 14 | 33740 | Copy the supplied work vector at+84 (not a fixed zero position) |

For8/10,33340 checks the supplied unit byte98 bit1. When clear,067E00 supplies the unit's world anchor. Otherwise choose the unit's full-width model pointer atC60; prefer that model's companion pointer+F00 when nonnull. For9/11,33440 is identical except it prefers companion+EE8. A null companion falls back to the original model, not a null attachment. Refresh the selected model through388EB0 and38AA30, then query38AB40 with the native widened slot's u16 bone selector+14 (wire selector+C). On bone-query failure, scale the attachment-work cached local XYZ+38/+3C/+40 by f32+68, rotate by its quaternion+58, then add translation+4C/+50/+54. These native byte offsets are evidence, not a host layout requirement. No all-zero or identity-bone replacement is equivalent.

Attachment13 caches the computed pose when its position-mode byte==1 at start+2 and reuses that cached pose according to the source's integral-frame change condition. Its extra u16 displacement moves away from the normalized camera-to-anchor vector. Bone lookup failure follows the source fallback. O02 now concerns the remaining position/height edge combinations and unusual attachment13 cache/displacement cases, not unknown8..11 companion selection. Common EPL transform flags are specified in [the emitter/render-state continuation](m8f-emitter-render-state.md).

43C0 uses height-mode values0..4. For height-mode4 the default Y is2; otherwise `Y=anchorY+heightFactors[heightMode]*halfHeight`. Position-mode8 returns the anchor's X/Z and selected Y; nonzero override adds the supplied radius in its relevant branch. Position9/10 use view-relative normalized vectors and negative max(radius,halfHeight);10 zeroes the vertical direction component. Other position modes rotate the source direction by the indexed angle then displace by radius. The exact constant tables are recorded in the evidence/grammar annex; preserve f32 operations and zero-vector behavior. Do not introduce a different engine-unit scale as a decoder heuristic.

## Immutable decoded representation

A binary-free implementation can decode the complete BED container with this algorithm:

1. Require exact signature and length>=630. Retain an immutable owner-backed span. Read both headers at8 and31C; reject counts>32 before indexing.
2. Read all64 physical slots as fixed-width wire fields, preserving uninterpreted bytes. Separately build each live prefix; skipFE without resolving its resource word.
3. Direct kind07: checked-add its u32 offset to the **BED base**, require a bounded EPL header and decode using the companion grammar. Multiple slots may reference one immutable decoded EPL but must not share mutable playback clocks.
4. FF: record an explicit typed reference to the decoded bank/slot; check graph validity and resolve in construction order. FD is never interpreted as an on-disk pointer. Unknown direct kinds or unimplemented semantic selectors are reported as unsupported with full logical path and offset.
5. EPL offsets inside the embedded root use that **EPL root** as base. Primitive descriptor parameter/adjunct offsets use the **descriptor** base. Mixing these three bases is a hard error.
6. Publish a Golden formation template only after the complete needed graph is decoded and dependencies are available. Do not publish successful render support just because the outer container parses. O03/O04 block that claim.

For every offset/count/size, use checked additions and bounded spans before dereference; reject negative serialized counts, integer overflow, out-of-range indices and unterminated label use. Bound names to their fixed16-byte field. Never execute serialized words as pointers/callbacks. File padding is not a GPU program, relocation stream or native work tail.

Decoded objects need explicit distinctions between immutable templates, live formation handles, live EPL instances and shared/borrowed resources. Golden's formation work size848, bank stride418 and slot stride20 are useful **evidence**, not a public host ABI. A native implementation may use ordinary typed owners without recreating those byte layouts. Do not copy the Golden work into the PS2 guest image or place native pointers in its guest fields.

## Playback behavior established here

33760 activates each slot only after its strict start threshold. A non-owning shared source is reused only under its recorded formation/control conditions; otherwise363B80/365980 construct or clone real EPL trees.33760 applies live source/target anchor selection, color, scale, orientation and per-slot start semantics. It advances the formation clock by **0.5*liveFrameStep**. The EPL update364F80 independently advances its own clock by the same factor; these are distinct clocks and must not be collapsed into a packet's integer age.

Bank scaling is applied only for attachment types in mask20C3 (0,1,6,7,13) with the source predicate `<14`; observed6 is not a callable attachment. Source/target sphere geometry affects the scale; normal bound is0.8..2.5, the alternate-source path2..5 with a4.25 default. This must remain geometry-dependent, not a constant scale chosen to make one image plausible.

Packed color multiplication in364DC0 extracts bytes high-to-low at shifts24,16,8,0, divides each by255, multiplies corresponding parent and child channels and truncates each result back to an integer channel. The source assigns no universal RGBA naming here: channel-to-render-backend interpretation is O03. Do not silently apply PS2 half-alpha conversion to all four channels.

The decoder must preserve these semantics for playback qualification, but this document does not establish every emitter distribution, primitive-specific parameter or blend/depth operation. Those gaps are enumerated, not hidden by opaque 'supported' status.

## Installation and teardown

The [5ca8dc8774 common-resource contract](effect-interception-resources.md) remains authoritative, including all49 ordered entries, ready flag40000, request release, retained archive copy, individual-data ownership mode bit26 and handle-first destruction. M8f must attach its decoded templates and native instances to that same resource generation. See the [integration page](m8f-battle-effects.md), not a second independently allocated R_BLOCK cache.

## Additional packet timing proof

NSO489A0 is the ordinary type301 step consumer of32D70. It first requires the formation's instantiated flag; an inactive effect-node byte at+7 completes immediately. An optional nonzero signed packet limit at work+54 completes when its doubled value is <=packet elapsed at+30. Otherwise the side-specific timing selector triggers fade preparation at `packetElapsed >= 2*timing - 13`. Without work flags bit18 it marks the effect node0x100 immediately; with that bit it waits for `packetElapsed >= 2*timing`, resets the node's color-control fields and then marks0x100. A node flag0x40 stalls this path without clock advance.

At elapsed0, unless packet work flags include both0x400 and0x800, it constructs the live formation entry. Side0 uses(source,target), side1 reverses the pair; it does not substitute the same source for both. Work flagbit19 publishes the packet's explicit position/orientation fields into the live entry. On the ordinary running path, **packet elapsed increases by two separately evaluated additions of `0.5*frameStep`** (489A0..48B50); this is not the entry/EPL half-step clock and not the PS2 integer increment2.

The independent completion predicate48B50 first checks instantiated state, then completes at `packetElapsed >= (duration>0 ? 2*duration : 2)`. Its duration getter32DA0 reads bank-relative04; thus the three wire timing words have separate proven consumers: bank0-relative00 for side0 fade threshold, bank1-relative0C for side1 fade threshold, and each selected bank's04 for completion. Keep the minimum2 completion threshold. Exact player-facing names and unusual caller-option combinations remain O01; these operational comparisons do not depend on those names.


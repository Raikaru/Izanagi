# M8f EPL, EP and effect adjunct families

Status: **container decoder specified; primitive rendering coverage incomplete (O03/O04)**. The census includes ordinary battle effects, cutins, field effects and editor samples deliberately: directory/name alone is not evidence of legal battle reachability. Exact counts/hashes are in [census](m8f-census.md), per-kind/version counts in [grammar](m8f-effect-grammar.md).

Notation: addresses, offsets, structural extents and strides are **hexadecimal** (root90=144 decimal, partC0=192 decimal, descriptor2C=44 decimal). Counts, versions, primitive-kind/subtype selectors and census byte totals are decimal. The PB size900 is decimal.

## Relationship between families

BED400 is a two-bank formation program whose kind7 slots point to EPL roots. An EPL root is an untagged effect tree, not an archive with an EPL magic. Its parts reference EP descriptors. Standalone `.EP` files contain the same descriptor at offset0. EP adjunct kind5 contains a nested EP; kind6 contains another EPL; kind3 contains model resources, not another EP. The recursive census walks5/6 as well as BED-owned and standalone roots. EP versions are decimal200,210,220 (hexC8,D2,DC).

The named catalog contains1107 standalone EPL occurrences (273 distinct SHA identities),148 EP occurrences (148 distinct SHA identities), and one900-byte tool PB. BED400 has458 distinct identities; dummy BED has one. All881 selected identities were re-extracted from the pinned install and matched their frozen inventory size/SHA. A duplicate logical path's identity comes from the full M7c inventory; it was not independently re-extracted in this deduplicated pass.

Wire integers/floats are little endian. The owning BED/CPK span outlives every decoded borrowed range. There are **three offset bases**: BED slot offsets use BED base; EPL part-resource offsets use EPL root base; EP data offsets use EP descriptor base. A label is not an external asset path unless a proven resource reader says so.

## EPL root:90 bytes

| Offset | Wire shape | Meaning / interpretation limit |
|---|---|---|
| 00 | f32[4] | Initial local position vector; retail construction uses this same quad for two initial vectors |
| 10 | f32[4] | Serialized rotation vector; retained; do not override the constructor's observed first-quad initialization |
| 20 | f32[4] | Scale-vector state |
| 30 | 16 bytes | Preserved transform/state words; not assigned a new semantic interpretation here |
| 40 | f32[4] | Offset/translation state |
| 50 | f32[4] | Orientation state |
| 60 | f32 | Scale |
| 64 | u32 | Packed four-channel color |
| 68 | u32 | State flags; distinct from part sourceFlags at98 |
| 6C | u32 | Preserved auxiliary state |
| 70 | u32 | Preserved auxiliary state / pointer-bearing runtime counterpart |
| 74 | f32 | Multiplier; constructor publishes this into native root+78 |
| 78 | 8 bytes | Preserved state |
| 80 | s32 | Part count (reject negative serialized values) |
| 84 | u32 | Frame/state field; runtime constructor initializes new clock independently |
| 88 | u32 | Root-relative offset to contiguous part array in file form |
| 8C | u32 | Linked-list first-part field in runtime form; **zero in all measured serialized roots** |

The full `90 + count*C0` is not necessarily the file size: part array location is the88 word, and descriptors/payloads follow. Validate `arrayOffset + count*C0 <= rootSpanLength`; the count and offset are not native pointers. No additional EPL magic or independent version was found. A nonzero serialized8C must not be executed as a host linked list. The linked in-memory branch of363B80 is a separate native object input form, not accepted disk data.

Evidence: NSO363B80 copies90 bytes, projects32-bit file fields into a widened nativeA0 root, then either traverses a native list or uses the serialized count/offset. Steam507870 makes the same90-byte input copy and allocatesA0. Its helper50A030 is protected beyond its ordinary-code jump; no protected instruction was decoded. PS2 `mdlEffectBuildFromResource` at00484BB0 independently explains the first-quad duplication and corresponding wire fields; native PS2 root allocation90 must not be confused with Golden'sA0 native allocation.

## EPL part:C0 bytes

Offsets00..7F use the same serialized state block as the root. In particular60 scale,64 color,68 transform flags and74 multiplier are not the source-sharing flags below.

| Offset | Type | Meaning |
|---|---|---|
| 80 | u32 | Part auxiliary state |
| 84 | u32 | Start frame; native+88, eligible when start<=EPL clock |
| 88 | u8 | Part state/flags; native+8C; update skips flagbit1 |
| 89 | u8 | Attribute selector |
| 8A | u16 | Attribute selector / preserved halfword |
| 8C | u32 | Auxiliary state |
| 90 | u32 | EPL-root-relative descriptor offset, **or earlier-part index if sourceFlags bit0** |
| 94 | u32 | Source-part index for adjunct sharing when sourceFlags bit1 and not bit0 |
| 98 | u32 | sourceFlags: bit0 clone prior primitive, bit1 share adjunct data |
| 9C | char[16] | Bounded part label; preserve raw bytes |
| AC | u32 | Serialized next-link field, not traversed in indexed file form |
| B0 | u32 | Serialized previous-link field |
| B4 | 8 bytes | Auxiliary state |
| BC | u32 | Auxiliary state |

Do not widen the file strideC0 when decoding on64-bit hosts. Native Golden parts allocateE0 and use primitive pointer+98, sourceIndex+A0, sourceFlags+A8, label+AC, next+C0 and previous+C8. The parser's stack projections explicitly widen the file's90/94/AC/B0 fields. This is why copying wireC0 bytes directly into a native part is wrong.

### Sharing rules, in file order

- `sourceFlags&1`:90 is a zero-based **already-created part index**, not an offset. Clone its primitive instance using the primitive-specific clone operation. Do not duplicate the primitive merely by copying its host pointer. Bit0 wins when both bits0/1 are set.
- Otherwise90 locates an EP descriptor relative to this EPL root. If bit1 is clear, construct from it normally.
- If only bit1 is set, current descriptor's auxiliary offset/count must be empty.94 selects the source part's descriptor. Borrow its real auxiliary data span/count for construction of the new primitive, using descriptor-relative addressing of the source descriptor. The indexed-file branch of NSO363B80 establishes separate constructed primitive versus borrowed adjunct semantics. The original uses temporary descriptor field substitution; a clean native implementation should pass a typed borrowed span without modifying immutable input.
- Retain resources across all clones/borrowers. Sharing is not permission to free the source part first or to resolve a reference to a later unconstructed part.

All flag values0,1,2,3 occur. The exact histograms and invalid-reference measurements belong to the grammar annex; a successful bounds pass is not proof that every legacy tool sample has a supported primitive callback.

## EP descriptor:2C bytes

| Offset | Type | Meaning |
|---|---|---|
| 00 | u32 | Version, decimal200/210/220 observed |
| 04 | u16 | Primitive family kind; native table bound0..33 inclusive |
| 06 | u16 | Preserved padding/high halfword |
| 08 | u32 | Runtime primitive instance/handle field;0 in measured file descriptors |
| 0C | u16 | Kind-specific subtype/id |
| 0E | u16 | Preserved padding/high halfword |
| 10 | u32 | Descriptor-relative primary parameter offset |
| 14 | u32 | Primary parameter byte length |
| 18 | u32 | Primary allocation/direct-pointer marker;0 in measured file descriptors |
| 1C | u16 | Adjunct/source kind |
| 1E | u16 | Preserved padding/high halfword |
| 20 | u32 | Descriptor-relative adjunct offset;0 means absent |
| 24 | u32 | Adjunct byte length |
| 28 | u32 | Adjunct allocation/direct-pointer marker;0 in measured file descriptors |

Use checked `descriptorBase+offset+byteLength` bounds before reading either span. Offsets may include alignment padding; do not force `auxOffset=2C+primaryLength`. Zero offset is absence, not a request to read bytes at the descriptor header. Do not treat24 as an element count with an implicit record stride: observed values bound complete adjunct blobs, including models.

NSO363660/363690 resolve primary/adjunct respectively. A nonzero allocation marker selects an already-resolved native pointer; otherwise a nonzero offset is relative to descriptor base. Disk decoding must not accept those pointer forms. NSO3636C0 validates maximum versionDC, reads type4 and idC, creates a native50-byte wrapper, and dispatches type-specific constructors through710086FAD0 with stride80. The new runtime wrapper version is inputVersion+10000; **do not rewrite the on-disk version to this runtime tag**. The serialized descriptor remains2C bytes.

The version numbers alone do not determine parameter size. The census records different lengths for the same kind/subtype/version. Subtype-specific tails and historical versions must be interpreted by their actual schema; copying a modern parameter struct length from an old200 blob can overread its declared data. No global 'skip ten bytes for version220' rule was established.

## Adjunct kinds and native installation

| Kind | Family / native route | Decoder/owner requirement |
|---:|---|---|
| 1 | Texture resource stream; particle and textured-polygon loaders | Resolve actual texture payload and scoped external `tex2` override policy; do not assume every stream starts with TMX/DDS magic |
| 2 | Texture/track resource,362E60 | Decode its independent frame/stream structure; not an EPL part array |
| 3 | Model resource | Observed `OMG.` GMO, `CHNK` AMD and legacy/tool variants; use M8c Golden model owner, never PS2 RMD parser |
| 4 | u32 external/native resource id |4-byte payload for ordinary entries; preserve selector and resolve through the owning resource namespace, not as a pointer |
| 5 | Nested EP descriptor | Recursively decode, retain parent/child resources, bound recursion |
| 6 | Nested EPL root | Recursively decode; helper27 can instance one tree for every model anchor and clone the first tree |
| 7 | Texture-list/palette family,383960 | Includes observed `TLP0` payloads; no blanket DDS reinterpretation |

For kind1, NSO36BB70/3794C0/366090 call the resource-path helper32A080. A successful path can be rewritten at `.EPL` or `.BED`, prefixed `tex2`, queried through398750, and used in preference to embedded texture state if available; a failed lookup returns to embedded handling. The [binding contract](m8f-texture-bindings.md) now specifies raw MD5/HFLC lookup, exact path construction and owners. Locale/cache/async runtime qualification remains O08; filename-stem guessing is not the source mechanism. DDS decode support alone is not proof these streams bind correctly.

For model kind26, NSO361D70 copies68 primary bytes and feeds its kind3 adjunct to361BD0. Kind32 copiesA4 primary bytes, creates drawable mode4 and sets real animation/model callback state; preserving only a static GMO draw is not full support. Kind27 copies68 primary bytes, skips70 before loading the inline model image, then instantiates/clones adjunct EPL per anchor count returned by38ABB0, not automatically bone count. The [model/helper/drawable contract](m8f-model-helper-drawable.md) specifies these fields, clocks, phases, projection and clone ownership; the [dependency census](m8f-model-adjunct-census.md) pins2507 logical model spans. M8c-1 and M8c-2 are required together.

## Primitive families observed

The [grammar annex](m8f-effect-grammar.md) is the exhaustive measured kind/version/subtype/byte-size table. Source names below are recovered from ordinary NSO assertion strings, not guessed from an effect's appearance.

| Kind | Family | NSO constructor offset | Existing PS2 semantic context (not Golden ABI) |
|---:|---|---|---|
| 1 | Particle |36BB70| `Graphics/Effect/effParticle.c`, `particle_spawn_internal.h` |
| 2 | PolygonTrack |37E6F0| `effPolygonTrack.c` |
| 4,5 | BlurFilter copied/direct |352DA0/3537F0| `effBlurFilter.c` |
| 7 | PolygonTex |3794C0| model/effect textured-polygon counterparts |
| 9 | Environment |35D060| environment state, not a particle emitter |
| 10 | PolygonFlash |377570| `effPolygonFlash.c` subtype table |
| 12 | PolygonRing |379190| `effPolygonRing.c` |
| 13 | PolygonThunder |37E400| `effPolygonThunder.c` |
| 15 | PolygonTrack alternate |37EF60| shared track handlers, distinct constructor |
| 16 | PolygonWind |382E80| `effPolygonWind.c` subtype table |
| 21 | Environment control |35DC40| four-word payload copy |
| 23 |2D|366090| screen-space effect, not source-bone billboarding |
| 26 | Model |361D70| Golden model bridge required |
| 27 | Helper |367FE0| model + EPL per anchor |
| 28 | ObjectParticle |36A420| `effObjectParticle.c` |
| 29 | DistortParticle |366AE0| `effDistortParticle.c` |
| 30 | LineNova |3691C0| `effLineNova.c` |
| 31 | BattleOnly |34F890| battle-owned subtype callbacks |
| 32 | Model drawable |362440| Golden animated drawable bridge required |
| 33 | FovyScale |367DE0| Golden camera/FOV owner; absent from PS2 type0..32 bound |

Kinds8,11,14,17,19,20 also occur in **version200 tool samples**; their Switch constructor table entries resolve to the generic365E40 null constructor and365E50 empty handlers. This is observed upstream table state, not permission to pretend to render them. Mark unsupported/editor-only until O06 resolves reachability and intended semantics. No other measured family is silently grouped with particles. Table slots0,3,6,18,22,24,25 are unobserved; null-table membership is not positive format support.

### Emitter facts cross-checked with PS2 semantics

For ordinary particles, NSO36BB70 computes capacity from primary s32 emissionDuration at20 (or lifetime atB8 when duration==0) times emissionRate at24, capped at300. These exact offsets agree with PS2 `WaveSpawnParameters` and `ParticleCreateFromSource`. Trail capacity in PolygonTrack is capped100, ObjectParticle100 and DistortParticle100, but their parameter headers differ: never use the particle header globally.

NSO35B590 allocates the behavior state for id0..11, including trailCount/trailWidth expansion from primary+C0/C4. It copies subtype parameters into current/original storage and creates an independent clock/state bank. Ordinary behaviors1..11 consume the global effect RNG through a null-state argument; initialization385090 seeds that global stream with0x12345678. Do not invent per-emitter seeds or borrow battle gameplay RNG. Exact distribution/birth/active/preroll equations are now specified in the linked behavior addenda; [particle trails](m8f-particle-trails.md) closes history/interpolation and observed bank strides. Historical version-tail and remaining primitive/backend questions are separate O03/O04.

EPL364F80 applies per-part start threshold, position/orientation, scale and real type-specific update callbacks; its clock advances0.5*frameStep. Draw and update are distinct dispatch slots. Do not use a render-only animation loop that never runs the source's effect counters/teardown callbacks.

## PB tool object

The only `.PB` is `data.cpk::system/tool/effect/hlp/TEST.PB`,900 bytes. Header words at0/4 are1/36 and bytes8..B are `PIB0`. It is not a BED, EP, EPL, CPK or ordinary named PAK by the existing recursive inventory. There is no observed battle-root `.PB` path. Its complete field semantics and loader reachability are **OPEN O06**, not asserted from the suffix. It remains fully counted and hash-identified. This record must not be fed to an EPL decoder or claimed rendered merely because a tool file exists.

## Decoder result and implementation boundary

A decoder can implement all framing above without either Golden binary. It must return a typed graph containing both banks/slot fields, EPL root/part state, sharing references, EP version/kind/subtype, parameter spans and typed adjunct spans, together with logical-path/offset diagnostics. Preserve raw uninterpreted state without claiming semantic support. Decode success and rendering support are separate statuses.

This is **not yet a complete binary-independent primitive renderer specification**: remaining primitive geometry/filter and blend/depth/postprocess operations, historical layout qualification, P2A paired composition, the preserved TLP17D trailer, AMD model-attached effect payloads and exact unit/camera/render facade binding remain O03/O04/O06. Ordinary behavior1..11 distributions, trail equations, model/helper/drawable primary fields, BattleOnly subtype primary layouts and R_BLOCK helper-driven lighting are source-specified in the addenda. PS2 source is reusable context, not proof of identical Golden fields/backend/pointer widths. Do not remove M8f's runtime refusal on the basis of this census.

## Reused, independently proven behavior-bank contract

The existing [twelve-kind behavior bank proof](l1-effect-behavior-bank-proof.json) fixes allocator sizes and both callback phases across Steam/NSO/PS2. Its callbacks are **behavior ids**, not the EP primitive-kind enum. M8f reuses that qualified bank identity rather than inventing another state-size table. That proof does not supply complete update/render semantics and does not lift O03.

| Behavior id | State bytes per primary (decimal) | Parameter bytes (decimal) | NSO update | NSO scale |
|---:|---:|---:|---|---|
| 0 | 0 | 0 | `None` | `None` |
| 1 | 60 | 240 | `0x7100354330` | `0x7100354d10` |
| 2 | 28 | 224 | `0x7100354d90` | `0x7100355580` |
| 3 | 64 | 244 | `0x7100355600` | `0x7100356040` |
| 4 | 52 | 232 | `0x71003560c0` | `0x7100356a10` |
| 5 | 28 | 232 | `0x7100356a80` | `0x7100357350` |
| 6 | 28 | 236 | `0x71003573d0` | `0x7100357cd0` |
| 7 | 76 | 292 | `0x7100357d50` | `0x7100358d50` |
| 8 | 248 | 248 | `0x7100358da0` | `0x7100359550` |
| 9 | 232 | 232 | `0x71003595d0` | `0x710035a100` |
| 10 | 36 | 248 | `0x710035a180` | `0x710035ab60` |
| 11 | 64 | 272 | `0x710035abd0` | `0x710035b510` |

Golden bank has12 records,24 decimal bytes each: native update pointer+0,scale pointer+8,state bytes+16,parameter bytes+20. Its source bank is NSO710086F808 (RELA-resolved), Steam RVA0xAE98F0; raw pointer bytes are not portable callback identities. The PS2 bank uses16 decimal bytes per record. Normalize by behavior id and phase, never copy Golden288 host bytes into the old180-byte selected PS2 partition.

Golden sample stride0x58 and emitter header0x50 are materially different from PS2 sample0x20/header0x30. First-tick inactive marker is **float-1 at sample+0x30**, not the old integer sample mode. The Golden advance reader35B720 has substep/update/copy behavior not established by merely mapping the old callback symbol. Two parameter copies must preserve current versus original state, and every allocator/update/scale reader must use the same native bank and correctly widened pointers.

The primary-count trail expansion is `totalCount=primaryCount*(trailWidth*trailCount+1)`. The allocation consists of the native emitter header, totalCount samples, primaryCount behavior-state records and two parameter blocks. The paired-source proof independently establishes these sizes, but a version200 file with a shorter declared primary blob is **not** proof that reading a modern full-size block is safe. Historical-tail conversion is still O03.


## Common parameter/render continuation

The [emitter/render-state continuation](m8f-emitter-render-state.md) now specifies the ordinary particle common wire prefix, exact effect RNG and seeding, scalar/color/opacity curves, discrete/interpolated native samples, wave/motion semantics and shared EPL transform/update/draw rules. In particular version200 occurs in real battle assets, not solely editor samples. The continuation narrows O02/O03; unqualified remaining subtypes must still not be marked rendered.


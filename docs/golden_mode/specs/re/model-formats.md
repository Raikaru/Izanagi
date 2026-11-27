# Golden AMD, GMO and model PAC formats — M8c

## Status, evidence and limits

This is a **data-format specification**, not a Golden executable reconstruction.
The companion [census and field tables](model-formats-census.md) give the exact
measured populations, chunk occurrences, field ranges and exceptions. Framing
coverage and rendering-semantic coverage are deliberately different: an opaque,
length-delimited effect member is classified, but is **not** thereby implemented.
The open-question register below is part of this specification, not permission to
substitute a PS2 asset, discard a feature, or advertise a supported model.

Evidence labels used throughout:

* **OBSERVED / O**: direct measurement of the owner's Steam app 1113000, build
  10836033; the M7c inventory identifies objects by full container chain and
  SHA-256. Unless qualified as a witness, aggregate O claims cover every selected
  census object, including locale copies and nested pack copies.
* **OBSERVED / C**: an already committed project contract or PS2 port/decomp
  description; cited with its file and revision. It is not a new retail runtime
  observation.
* **PUBLIC / P**: a published format fact, not implementation code. It does not
  establish that a particular Golden file uses that feature.
* **INFERRED / I**: a semantic interpretation or proposed conversion. Do not
  silently promote it to O; use the corresponding BBV card.

No P4G executable code, `.arch` protector material, RE notes, proprietary SDK
implementation, Noesis plugin, Amicitia implementation, or other unlicensed
model-reader code was used. Census tools use original Python, the existing
project CPK/CRILAYLA/PAK readers and the standard library. The numerical witness
additionally uses installed Pillow as a measurement-only DDS reader.
No third-party runtime decoder is adopted; no new shipped dependency or notice
obligation is created. Public descriptions are paraphrased, not copied.

### Public references

1. [Amicitia AMD](https://amicitia.miraheze.org/wiki/AMD?oldid=6206): AMD roles,
   MODEL_DATA, effect adjuncts, NECK_BENDER.
2. [Amicitia GMO](https://amicitia.miraheze.org/wiki/GMO?oldid=4974): Golden
   restrictions, custom-data identifiers, attachment and material roles.
3. [GMO structural overview](https://wiki.ffrtt.ru/index.php?title=PSP/GMO_Format&oldid=5237):
   signature and early chunk identifications. Its abbreviated header description
   is insufficient for Golden; use the measured offsets below. In particular,
   a block's total size includes its header, and type 0x000B in this corpus
   contains motion commands and FCurves, not merely shared vertex data.
4. [Published MDX format description](https://psmreborn.com/psm-sdk-doc/en/mdx_format_en.html):
   corroborating concepts only: scoped references, transforms, mesh attributes,
   animation curves. **MDX is not GMO. Its binary header, type numbers and vertex
   flags must not be substituted for the Golden layouts.**
5. [Amicitia model directory](https://amicitia.miraheze.org/wiki/Persona_4_Golden/data.cpk/model?oldid=6048):
   directory roles. This is not evidence for the pinned build's object counts.

## 1. Corpus and reproducibility

The selection is broader than battle models: every M7c inventory row whose
signature is CHNK or OMG., plus every `.pac` row, regardless of directory or
locale. This includes field and tool models and skill PAC containers so that
extension-based exclusions cannot hide a grammar variant. See the annex for
nested NECK_BENDER GMO accounting, which is additional to inventory rows.

| Inventory class | Objects | Unique extracted SHA-256 identities |
| --- | ---: | ---: |
| AMD / CHNK | 4,679 | 4,118 |
| Standalone or ordinary-pack GMO / OMG. | 55 | 42 |
| Counted PAK v2 PAC | 1,041 | included in total below |
| Sequential PAK v1 PAC | 3 | included in total below |
| **Total inventory objects** | **5,778** | **5,191** |

There are **494** `data.cpk::model/pack/*.pac` roots and **494** AMD children
in those packs. All 494 model-pack roots use PAK v2. Do not extrapolate that to
all PACs: the other 550 PAC objects include three PAK v1 files. Nested objects
are counted as objects, not as additional disk bytes. The census is static;
this does not claim that every file is reached in normal gameplay.

Pinned inventory SHA-256:
`150454e1b60d50ccdd58346b6da4e18e363860bf49bb22a679db379dee3771f8`.

Run `tools/golden/model_measure.py` under the PC CPU governor. Required arguments
are `--install`, `--inventory`, and an owner-local `--out`. `--install` can also
come from `P4PC_GOLDEN_STEAM`. Detection follows
`tests/golden_cpk_vfs_steam_test.py`: check the adjacent
`appmanifest_1113000.acf`, app id, install-directory identity, and additionally
build id 10836033. Each selected object's extracted length and SHA-256 must equal
its inventory record. A mismatch aborts, not a skip. The tool never reads the
P4G executable or writes extracted asset files.

Optional acceleration: compile the existing, unchanged
`src/platform/golden/golden_formats.c` as an owner-local shared library and pass
`--native <DLL>`. Only its bounded CRILAYLA function is called. It is not a
second model parser, does not weaken the hash checks, and must not be packaged.
Measurement outputs, captures and any temporary DLL remain on the owner PC.

## 2. Byte conventions and containing archives

All integers below are little-endian. `u16`, `u32`, `s16`, `s32`, `f32` have
2/4/2/4/4 bytes. Floating-point fields are IEEE binary32 unless stated otherwise.
Offsets are relative to the indicated record, **never host pointers**.
Overflow-safe containment is required before multiplying counts or advancing.

### 2.1 Model PAC: PAK v2 (O)

| Offset | Type | Meaning |
| ---: | --- | --- |
| 0 | u32 | entry count |
| entry +0 | byte[32] | ASCII member name, NUL terminated and NUL padded |
| entry +32 | u32 | payload byte length |
| entry +36 | byte[length] | payload |

The first entry is at +4. The next entry begins immediately after this payload;
there is **no** 64/128-byte inter-entry alignment. Names are keys, not extensions
to trust as a decoder selector. AMD and dependent `.se` resources coexist.
Route the container through the existing PAK v2 reader, then dispatch the
member by its bounded contents. An `.se` member is not a motion or a model.
Ordinary PAK validation requires any bytes after the declared members to be
zero. The annex supplies entry-count and size ranges.

The PS2 PAK v1 analogue instead has name[252], u32 length, payload, and
64-byte alignment; some field packs align to 128. It has no initial count.
Those formats share the concept of named members, **not** field offsets.

### 2.2 AMD / CHNK (O)

| Offset | Type | Meaning |
| ---: | --- | --- |
| 0 | byte[4] | `CHNK` |
| 4 | u32 | member count |
| member +0 | byte[16] | member name, ASCII through first NUL |
| member +16 | u32 | type tag; not a byte length |
| member +20 | u32 | payload length |
| member +24 | byte[length] | payload |

Members are consecutive without alignment insertion. The first is at +8.
A member's next position is `position + 24 + length`. Finish the declared count;
record any remaining suffix separately. Do not scan for another `CHNK` or
`OMG.` signature inside payload or suffix. Such signatures also occur in
exporter residue. Nonzero suffix bytes exist and are not a reason to reinterpret
an otherwise bounded member as a different format.

| Name / type | Grammar and role | Evidence |
| --- | --- | --- |
| MODEL_DATA / 0x101 | one bounded GMO file | O; P model role |
| NECK_BENDER / 0x2D0 | counted name[32]/length/member archive described below | O; P head-animation role |
| EFFECT_EPL / 0x2E0 or 0x2E2 | length-delimited adjunct; distinct tag variants must survive | O; P attached effects |
| EFFECT_TRACK / 0x2E3 | length-delimited track-effect adjunct | O/P |
| EFFECT_ADDMOT / 0x2E4 | length-delimited animation/effect adjunct | O/P |
| EFFECT_EDGE / 0x2E5 | length-delimited edge-light adjunct | O/P |

The exact name/tag occurrence matrix is authoritative in the annex. A type tag
alone is not proof that two adjunct variants have identical internal records.
The full unknown payload must retain its identity in the Golden resource graph;
M8f owns the effect semantics, with the animation coupling shared with M8c-2.

### 2.3 NECK_BENDER nested archive (O)

The prefix is a u32 count, then the same name[32], u32 length, immediate payload
records as PAK v2. Its declared members include `.dat` parameters and GMO data.
**Its trailing region is not subject to the ordinary PAK zero-tail rule.**
A measured witness has two members, a 32-byte `.dat` and a GMO, followed by
20 nonzero/residual bytes; the complete distributions are in the annex.
Therefore simply calling the ordinary PAK reader and treating refusal as an
absent neck animation would lose real content. Parse the bounded counted prefix,
record its suffix, and retain both parameter and motion resources. Do not hunt
for GMO magic: copied header bytes can also appear in the suffix.

## 3. GMO chunk grammar (O)

The 16-byte signature is `OMG.00.1PSP\0\0\0\0\0` (see the annex's exact
hex signature). At +16 is the type-2 root. The root's declared span, not the
outer member length, bounds the hierarchy. Bytes after that span are an opaque
file suffix. This distinction matters on thousands of corpus objects.

### 3.1 Block and command headers

A block begins:

| Offset | Type | Meaning |
| ---: | --- | --- |
| +0 | u16 | block type |
| +2 | u16 `H` | end of block name / start of arguments |
| +4 | u32 `S` | complete block size, including header and descendants |
| +8 | u32 `C` | end of block data / start of child chunks |
| +12 | u32 `A` | start of block data / end of arguments |
| +16..H | bytes | name area |
| H..A | bytes | typed arguments |
| A..C | bytes | block data, e.g. vertex buffer or key array |
| C..S | chunks | child commands and blocks |

The measured inequality is `16 <= H <= A <= C <= S`. The order of +8 and +12
is easy to reverse: **children start at +8's value, data at +12's value**.
For many ordinary blocks H=A=C; vertex and FCurve blocks have data and C=S.
Names and numeric regions are four-byte aligned; no alignment may move the
next sibling past `current + S`. The annex counts alignment checks.

A command has type:u16 at +0, **zero** at +2, total size:u32 at +4, then
arguments at +8 through its end. Do not apply the 16-byte block header to a
command. The high bit marks the observed command family. Sibling iteration
advances by total size; child traversal is bounded by its containing block.
The annex enumerates **every observed type and parent→child edge**; a parser
must not assume the common hierarchy is the only grammar.

Typical structure:

```
file(0002)
  model(0003)
    bone(0004) + parent/transform/blend/custom-data commands
    part(0005)
      mesh(0006) + material/subset/draw commands
      arrays(0007) + packed vertex data
    material(0008)
      layer(0009) + texture/render-state commands
    texture(000A) + filename/file-image commands
    motion(000B)
      animate commands + fcurve(000C)
```

### 3.2 Scoped references (I, corroborated by measured command shapes)

A reference word has block type in bits 16..31, ancestor level in bits 12..15,
and same-type sibling index in bits 0..11. The scope is the block hosting the
command; level 0 uses that block's children, level 1 its parent's children,
and so on. Count only siblings of the referenced type, not all children.

Examples of *synthetic* references: `0x00041002` means bone index 2 one scope
up; `0x00082001` means material index 1 two scopes up. **OBSERVED:** this shape
resolves all 295,220 parent refs, 155,270 blend-bone refs, 92,393 material refs,
493,569 arrays refs and 38,685 texture refs against the same-type scoped tables.
It also resolves 92,393 ordinary DrawPart refs and 3,965,908 animation-reference
words. It is not a total standalone-GMO resolver:

* NECK_BENDER GMO files retain **16,233** DrawPart and **638** material-animation
  references absent from their own tables. Testing the enclosing AMD's sole
  MODEL_DATA scope resolves **16,000** of those part refs and **all 638** material
  refs; **233 part refs remain outside that candidate owner table**. Preserve
  the supplemental resource relationship; do not reject it as an isolated model,
  clamp indices, or claim the owner-binding hypothesis completely solves it.
* **3,345** animation target words are 0xFFFFFFFF. They are a separate sentinel
  case, not a type-FFFF reference to ancestor 15. Its target-dependent meaning
  remains OPEN; target command counts are in the annex.
* **Nine** other animation bone-target words fail the local-scope hypothesis,
  in five persona models. Their exact paths/values are listed in the annex.

Resolve after indexing the complete owning resource graph. Wrong types and
out-of-range indices become explicit failures only after the required context
and sentinel semantics are established; do not silently skip the exceptions.
Reference words are not byte offsets. Exact binding remains U02/U07/U10.

## 4. Skeleton, geometry and skin records

Offsets in command rows are relative to the **payload at command +8**.
All block arguments below begin at H, not at a hardcoded name length.

| Type | Layout / interpretation | Evidence |
| --- | --- | --- |
| 0004 | named bone, normally no argument/data region; children describe state | O |
| 8041 | +0 reference to parent bone | I; 4-byte payload O |
| 8048 | +0/+4/+8 f32 local translation XYZ | I, field shapes O |
| 8049 | three f32 Euler rotation components; exact Euler order is U01 | I |
| 804B | four f32, candidate quaternion XYZW; all 44 squared norms are 0.9999999554546122..1.0000000548678565 | O shape/norms, I convention, U01 |
| 804C | three f32 scale components | I |
| 8044 | +0 u32 count, then count reference words: blend-bone palette | I |
| 8045 | +0 u32 count, then count 64-byte/16-f32 bind-offset matrices | I |
| 804E | +0 part reference: draw this part on this bone | I |
| 8014 | six f32: min XYZ then max XYZ bounding box | I; witness reconstructed below |
| 8015 | +0 attribute mask; then offset vector, then scale vector | I; payload lengths and masks O |
| 8061 | +0 material reference for this mesh | I |
| 8062 | +0 count, then count u32 blend-palette subset indices | I |
| 8066 | +0 arrays reference; +4 primitive flags; +8 vertices per primitive; +12 primitive count; +16 sequential start or index data | I; exact forms measured in annex, U03 |

Bone nodes are flat siblings with parent references. Their traversal order is
not an HAnim id. Preserve names, scope and custom tags independently. A model
may contain multiple models/scopes; do not merge their id spaces.

### 4.1 Packed arrays (0007)

Observed argument prefix: +0 u32 format; +4 u32 vertex count; +8 u32 auxiliary
count (always 1); +12 u32 **additional UV-set count** (interpretation I,
exhaustive length corroboration O). Data is [A,C). Keep +8 verbatim: its constant
value is not proof of a generic morph-count interpretation.

Candidate packing, measured against every array's byte length in the annex:

| Format bits | Attribute | Width selector |
| --- | --- | --- |
| 0..1 | UV, two components | 0 absent; 1 byte; 2 halfword; 3 f32 |
| 2..4 | packed colour | 0 absent; 4/5/6 packed 16-bit; 7 packed 32-bit |
| 5..6 | normal, three components | 0 absent; 1 byte; 2 halfword; 3 f32 |
| 7..8 | position, three components | same |
| 9..10 | weights | 0 absent; 1 byte; 2 halfword; 3 f32 |
| 14..16 | weight count minus one, only if weights are present | 1..8 representable |
| all remaining bits | exporter/format options | retain; U03 |

**INFERRED:** order is weights, all UV sets, colour, normal, position. There are
`1 + additional_uv_sets` UV pairs when UVs are present. Align each attribute to
its scalar width, then vertex stride to the largest width. Round the complete
vertex buffer up to four bytes, **not each vertex**.

**OBSERVED:** one-UV packing fits 90,509 arrays but fails 1,884. Accounting for
+12's 1/2/3 additional sets fits **all 92,393** arrays: 90,509 have one UV set,
1,531 two, 342 three, and 11 four. A two-UV field witness's decoded position
bounds agree within 0.020 game units when the additional UV pair precedes
normal/position, versus over 2,500 units error if it is placed after position.
The two-UV maximum of the existing cooked mesh is therefore not a Golden limit.

Signed normal/position fixed components are normalized by 128 or 32768.
UV fixed components and weights are **unsigned**; candidate denominators are
128 and 32768 respectively for byte and halfword scalars. This distinction is
corroborated by the [public GE vertex-format facts](https://www.ppsspp.org/docs/psp-hardware/gpu/ge-vertex-pipeline/),
not by importing emulator implementation code. GE upper-bit morph semantics
must not be copied into GMO. All measured weighted arrays use byte weights:
56,527 arrays have minimum vertex sum 128, 75 have minimum 127, and every
weighted array's maximum is 128. Preserve the resulting 127/128 sums; do not
silently renormalize them to one. Packed colour component order and currently
unused scalar forms remain U03/U04. Golden's public omitted-single-weight
warning is not permission to supply an invented implicit bone.

Quantization command 8015 is scoped state. For mask 0x180 (position), its
payload is mask + offset[3] + scale[3]; for mask 3 (UV), mask + offset[2] +
scale[2]. **INFERRED:** decoded component is `offset + scale * normalized`.
One character witness's 16-bit positions reconstructed its independently stored
part bounds to approximately 0.0011 game units using this rule. This is a
quantization-error witness, not a claim that all model bounds have been proved.
The generic command word ranges and all array packing counts are exhaustive;
position semantics and state inheritance still require U01/U03.

Draw flag 0x100 is the **INFERRED** sequential form: repeat vertices per
primitive, incrementing from the supplied start. **OBSERVED:** all 493,569 draw
commands fit exactly: payload length 20 for this flag, otherwise
`align4(16 + 2 * vertices_per_primitive * primitive_count)` with u16 indices.
The observed mode/counts are 0x101:1, 0x103:70,999, 0x104:417,283,
0x003:917 and 0x005:4,369. Low modes 1/3/4/5 are candidate line/triangle/
triangle-strip/triangle-fan roles (P/I); preserve winding alternation and
degenerates until triangulation. The sequential start is u32 at +16; the
indexed array starts there instead. No variable-length list may be read merely
because the common sequential payload happens to contain five words.
Primitive semantics still require U03; rare 806B is separate (U14).

### 4.2 Skinning (I unless explicitly O)

8062 maps vertex weight lanes to the 8044 palette, which references actual
bone blocks. The corresponding 8045 matrix maps bind-space vertices into that
bone's space. The 16-f32 witness stores affine translation in elements 12..14
and affine padding in 3/7/11/15, like the PS2 RwMatrix row-vector form.
The proposed skin result is the weighted sum of `position * offset * boneWorld`.
Confirm matrix multiplication order, nonuniform scale and normal treatment in U04.

Do not collapse six lanes to the port's four-lane cooked-skin representation.
The existing `P4ModelCookedSkin.maxWeights` is 1..4. Public Golden descriptions
allow six weights and the annex measures the actual corpus; the new Golden
host path must preserve every measured lane. Do not renormalize away a material
or palette disagreement to make a model appear to work.

## 5. Materials and textures

| Type | Payload / role | Evidence |
| --- | --- | --- |
| 0008 / 0009 | named material / named layer; each can have state commands | O |
| 8081 | two u32 state words: blend/render policy, exact enumerations unresolved | O / U05 |
| 8082 | four f32, candidate diffuse RGBA | I; P colour role |
| 8083 / 8084 | material colour/property command variants | O / U05 |
| 8085 | four f32, candidate ambient/property vector | I / U05 |
| 8086 / 8087 / 8088 | scalar material-state commands; preserve distinctly | O / U05 |
| 8091 | texture reference | I |
| 8092 / 8093 | one u32 state each; sampler/environment options unresolved | O / U05 |
| 8094 | three u32 layer-state words | O / U05 |
| 8097 / 8098 / 8099 | optional layer state | O / U05 |
| 809A | optional 64-byte matrix, candidate UV transform | I / U05 |
| 8012 | NUL-terminated source filename, padded to four bytes | O |
| 8013 | +0 u32 image length, then embedded image bytes | O; image kinds in annex |

The texture block name, filename and embedded image identity are different keys.
Do not require that a historical `.tm2` name imply the bytes are a standalone
TM2 file on disk. In addition, the census has **3,790** DDS objects in
`model/hrestex/dummy/`, with **zero case-folded basename collisions**.
A character witness has a matching high-resolution DDS basename for its embedded
low-resolution texture. The priority rule, path fallback and alpha policy are
**not established** by that coincidence (U06).

Embedded images are **25,742 TIM2 plus 5 BMP**, outside M8a's standalone
DDS/TMX census. [The full embedded-image specification](model-formats-textures.md)
defines their container fields, measured variants, palette/alpha delta against
the actual existing **TMX** decoder, and exhaustive high-resolution name
coverage. There is no existing TIM2 container decoder in this baseline.
High-resolution binding remains **OPEN**; Main's ruling is to default to embedded
textures while unproven. Never feed TIM2 bytes into `p4gl_native.c` as a PS2
native chunk or apply TMX's alpha expansion to full-range Golden palettes.

## 6. Animation and the 32 motion slots

### 6.1 GMO motion and FCurve layout

| Type | Layout | Evidence |
| --- | --- | --- |
| 000B | named motion containing commands and 000C curve blocks | O |
| 80B1 | two f32, candidate loop start/end in frames | I |
| 80B2 | one f32, frames per second | I; witnesses include 30 |
| 80B3 | four u32: target reference, target command, option/component, curve reference | I |
| 000C args +0 | u32 format (interpolation/precision/extrapolation bits) | O |
| 000C args +4 | u32 dimension count | O; meaning I |
| 000C args +8 | u32 key count | O; meaning I |
| 000C args +12 | u32 option/reserved | O |
| 000C data | packed key rows, rounded as one buffer to four bytes | O / I interpretation |

Candidate curve format: bit 7 selects IEEE binary16 instead of binary32;
low nibble 0 constant, 1 linear, 2 Hermite, 3 cubic, 4 spherical. Higher bits
are retained (U07); they are not all known interpolation flags. A simple key row
is time then `dimensions` values, **all in the selected precision**, including
time. Thus half-precision time 1 is 0x3C00, not the first half of a binary32.
The sampled constant quaternion has 5 half scalars plus final buffer padding.
A three-component, 27-key half-precision linear curve has 216 data bytes.

**OBSERVED:** all 1,984,942 curves fit the selected-precision row-size formula,
with formats 0x80 (1,002,081), 0x81 (71,470), and 0x84 (911,391) only.
All therefore use the proposed half-precision storage. Dimensions are 1, 3 or 4;
key counts are 1..1,098. **Hermite, cubic, binary32 curves and additional format
bits are not observed**, so they are outside this build's required decoder set,
not silently extrapolated from MDX. Every present frame-rate command is 30 f32.
Keep target command and option: observed targets include 0x48 translation,
0x4B rotation, 0x4C scale, 0x82/0x85 material and 0x93/0x98 layer channels.
No bone-only animation conversion covers this corpus.

For conversion, frame time / frame rate gives seconds (I). Translation, scale,
quaternion, visibility, material and UV animation must remain separate channels.
Spherical rotation interpolation needs shortest-arc handling and normalization;
nonlinear/extrapolated formats need their own qualified evaluator, not LINEAR
resampling by fiat. Empty motion blocks are real observed objects, not proof of
an alias to slot zero. EFFECT_ADDMOT can affect curve lifetime (P); see U07/U11.

### 6.2 Table slots, logical motion ids and physical clips are distinct

**OBSERVED/C:** `model-fields.md` at committed revision `d51b462f4` specifies
MODEL0 as 11 x 0x17E, motion records at +0x3C, 32 records of 10 bytes, followed
by tail bytes +0x17C/+0x17D. First 27 records are structurally analogous to PS2.
All five halfwords must survive. The first three enter timing/state calculations;
the fourth participates in position calculations; the fifth is a presentation
parameter. Their physical units are not all established. Never truncate the row
or route only a guessed animation id to the renderer.
The M7a contract retains the original contiguous 11 x 382-byte bank, including
documented cross-row/native-backing reads. M8c must not repack it into independent
rows/slots, clamp those consumers, or change packet completion ordering while
forwarding presentation values into the model layer.

| Record-relative offset | Storage | Required preservation |
| ---: | --- | --- |
| +0 | s16 | timing numerator / duration-related value |
| +2 | s16 | percentage-like playback denominator |
| +4 | s16 | secondary timing/state value |
| +6 | s16 | spatial presentation value |
| +8 | u16 | native presentation parameter; not necessarily a motion id |

For physical slot `s`, the row offset is `0x3C + 10*s`. Slots 0..26 occupy
+0x3C..+0x149; slot 27 starts +0x14A (330), 28 +0x154 (340),
29 +0x15E (350), 30 +0x168 (360), 31 +0x172 (370), ending at +0x17B.
Slot zero is zero-based; the row's two tail bytes are not a 33rd motion.

The committed logical mapping specifies genus-0 logical ids **30,31,32,33 →
slots 28,29,30,31**. Logical id **34 has no slot-27 mapping**. A separate
state bit 0x40 at the unit motion-state field selects slot **27**. The existing
protagonist special-state mapping of logical motion 1 selects slot **28** when
unit state bit 0x100 and party id 1 apply. Preserve these two distinct overrides;
do not invent a logical id 34→27 convenience alias. Exact gameplay names for the
added motions are not established by the table spec and remain U08.

**OBSERVED witness / INFERRED mapping:** one ordinary party GMO has 28 motion
blocks: an unsuffixed base motion followed by `-0000` through `-0026`. Across
the corpus all 4,281 models with motions start with an unsuffixed motion;
42,393 suffixed entries agree with `suffix = file_index - 1`, but **41 do not**,
and **81** additional unsuffixed entries occur after index zero. Thus a suffix
parser is not a total slot mapping. Preserve **file order and every motion**;
the base/slot interpretation and exceptions require U08. The annex supplies
counts and the semantic receipt supplies exception identities. Models have up
to **35** physical motion blocks; 32 is the party MODEL0 record count, not a
universal clip-list capacity. Empty-curve entries are not proven aliases.
Never fabricate clips for a short file. The full 32-slot record bank still loads
unchanged under M7a.

### 6.3 MODEL1, explicit model ids and attachments

The committed `model-fields.md` contract specifies standard enemy paths
`model/enemy/em%03X.AMD`, and the registered pack path
`model/pack/em%03X.pac`, with unsigned low-16-bit ids. Equal MODEL1 parameter
rows do **not** alias model identities. The battle-load constructor's documented
0x111→0x110 exception applies only to its stated flags/genus/stored-id condition;
the generic model manager forwards caller-supplied type/id pairs. Explicit
summon ids and dependent `.se` requests are separate inputs. This format spec
does not rederive any executable selector or substitute a PS2 retail row.

## 7. Attachments and custom data (80F1)

80F1 carries a NUL-terminated tag and tag-specific data. Tag payload lengths and
counts are exhaustive in the annex. String alignment is tag-specific: in
particular, UserProperty's second string follows its NUL immediately, whereas
the integer-bearing tags align the following integer to four bytes.

| Tag | Measured representation / interpretation | Evidence |
| --- | --- | --- |
| per3Helper | tag[12], s32 id, then f32 fields; common 52-byte payload described below | O; attachment role P |
| UserProperty | `UserProperty\0` then `tag=<decimal-id>\0`, then padding | O/P; node semantic tag, not skeleton ordinal |
| transAlgo | padded tag then s32 value | O/P rendering order; enumeration U05 |
| EdgeLight | padded tag then s32 value | O/P persona edge-light control |
| ShadowMap | padded tag then s32 value | O/P field-shadow layer marker |
| other observed tags | retain separately; exact list/count/length in annex | O; U14 |

All **12,295** per3Helper payloads are **52 bytes**: id at +12, translation XYZ
at +16..+24, candidate quaternion XYZW at +28..+40, then eight **zero** bytes.
The exhaustive squared quaternion norms are
0.9999983609779131..1.0000015399784843. This **does not match blindly reading
nine floats as a 3x3 matrix**, despite the abbreviated public prose. The TRQ
semantics/convention still require U09; the trailing zeros are retained, not
newly assigned a scale meaning.

**INFERRED conversion:** make an attachment offset transform from those TR/Q
values and multiply it by the owning bone's world transform. Keep helper ids
such as 500 in a distinct namespace from UserProperty ids such as 5001.
UserProperty id 5001 is publicly associated with the head/neck feature; it is
not a guarantee that bone index 5001 exists. Attach a separate weapon root by
helper semantics, not by finding a similarly named bone.
NECK_BENDER parameter interpretation and its interaction with this head tag
are U10. The effect adjuncts may also refer to helpers (U11).

## 8. Field-by-field PS2 / port correspondence

**OBSERVED/C:** baseline port `ef7b5b7ed`, `docs/modding/models.md`,
`src/platform/models/p4model.h`, `p4model_stream.c`, `p4model_build.c`, and
`src/render/p4gl_native.c`. The following Golden translations are proposals,
not an assertion of binary compatibility.

| Golden field / concept | PS2 RMD / existing port analogue | Required distinction |
| --- | --- | --- |
| AMD member name/type/length | top-level RW/Atlus chunk id/length/library id | AMD has no RW library id; lengths include different headers |
| PAC count/name[32]/length | PAK v1 name[252]/length/alignment | existing v2 container reader, not the PS2 layout |
| GMO type-3 model | RW 0x10 clump | new hierarchy must be built; glTF overrides retain a PS2 clump and cannot supply this by themselves |
| bone name, parent reference, TRS | RW frame parent index, rotation rows, position | resolve scopes; convert matrix convention once, never copy reference words as frame indices |
| UserProperty tag id | HAnim node id / frame user data | id is not traversal index; preserve helper and bone namespaces |
| DrawPart plus mesh | RW atomic frame index and geometry index | one part can have several drawing relationships; do not weld solely by material |
| array vertex count/format/data | geometry vertex count/format and separate attribute arrays | dequantize attributes; preserve normals/UV/colours |
| DrawArrays primitive/start/count | RW triangle list and bin-mesh strips | explicit triangulation and winding, with material slot per triangle |
| layer/material texture reference | P4ModelCookedMaterial.texture and RW material/texture | resolve scoped layer references; multiple layers cannot become one silently |
| RGBA/property/state commands | material color, ambient/specular/diffuse, MatFX and drawCategory | unknown enum is not PS2 per3MatAlpha by numerical coincidence |
| filename/image/high-res DDS | native texture dictionary and PS2 GS texture stream | M8a DDS upload or clean TIM2 handling; no GS alpha/palette assumptions |
| palette refs + subset indices | RpSkin hierarchy indices and used-bone list | map through scopes/palette; do not use helper ids as skin indices |
| bind offsets | RpSkin inverse matrices / P4ModelCookedSkin.matricesOffset | preserve matrix/scale; verify multiplication order |
| up to measured weight lanes | four u8 joints / four f32 weights per PS2 vertex | Golden host representation may need more than four; no lossy truncation |
| motion + curve targets + times | RW 0x1B animations, RtAnim tracks, compressed keys | generic GMO curves include scale/material/UV; no automatic compressed-key equivalence |
| base motion / slot candidate | RMD 0xF0F00009 base, 0xF0F00004 next-slot, 0xF0F00003 aliases | require a proved mapping; empty GMO motion is not an RW alias opcode |
| 32 MODEL0 records | PS2 27-record bank | all 32 retained; format and gameplay mapping are different layers |
| per3Helper id/TRQ | RMD 0xF0F00006 id/node/matrix; per3HelperId frame fallback | common semantics of an attachment; different on-disk fields |
| neck archive | RMD blend groups / head-specific model work | no proved one-to-one binary translation |
| effect adjuncts | RMD 0xF0F000E0/E1 and material/UV/effect slot chunks | M8f coupling; retain timing and helper links |

The existing cooked header is version 3. Mesh positions/normals/UV are f32,
triangle indices u16, material slots explicit. Skin indices are hierarchy indices,
not host pointers. Existing animations carry layer/slot, duration, node-id tracks,
packed quaternion/translation and translation scale/offset. None of that creates
missing Golden skeletons, supports arbitrary curve targets, or increases its
four-weight limit. `p4gl_native.c` is a **texture** handler, not the GMO mesh
reader. Reuse owned runtime concepts, not its on-disk PS2 native framing.

## 9. Open questions and preregistered BBV cards

BBV is proposed, **not run**. All retail runs are owner-operated unmodified
Steam P4G: no injection, debugger, memory reads, mod loader or protector work.
Freeze the transcribed observations before the implementer compares them with
this spec. Captures stay on the owner PC; report hashes and structured facts.
For each deterministic card, require zero invariant violations in at least 30
repetitions/frames at the named transitions; for stochastic comparisons use the
roadmap's preregistered chi-square/Fisher rule (alpha .05, N>=30). Include
camera, resolution, costume, equipment, date, save and selected source identity.
Do not tune constants to the captures after seeing the result.

| ID | Open question / affected fields | Proposed owner BBV and deterministic invariant |
| --- | --- | --- |
| U01 | axes, winding, Euler order (8049/804B), local/world matrix order | Freeze an asymmetric weapon/body pose in front/side/rear views, then a turning animation. Same hand, front face and signed turn; no mirrored geometry or alternating missing strip faces. Nonuniform-scale model checks separate scale from rotation. |
| U02 | scoped references, parent resolution, multi-model scopes | Use a character with distinct hand/head attachments and a field model with several parts. Freeze each linked part's motion; every child follows the intended parent, no cross-model palette/material leakage. Data-side index-bound checks supplement BBV. |
| U03 | upper vertex flags, packed colour, primitive enums/indexed forms, quantization inheritance | Compare silhouette, UV seams and colour on one witness for every measured array/draw variant, including rare tool/field files if reachable. Static bounds must enclose decoded geometry within declared quantization tolerance; strips keep winding. Unreachable variants are not BBV-passed. |
| U04 | palette/subset resolution, weight normalization, bind offsets, scale normals | Select every measured weight-count class and a visibly bending joint. Rest and extreme flexion must keep mesh attached, silhouette continuous, normals stable; compare a six-lane witness without dropping lanes. Validate offset*world invariance at bind pose numerically before capture. |
| U05 | all unresolved 808x/809x enums and transAlgo/EdgeLight/ShadowMap values | For each observed enum variant, compare opaque/cutout/translucent overlap, persona edge glow and field shadow under fixed lighting. Freeze draw ordering and whether background shows through each layer. No arbitrary PS2 enum substitution. |
| U06 | embedded TIM2 vs external DDS selection, name case/extension fallback, alpha and palette | Capture a texture with visibly different low/high-res detail, a cutout, and a partially translucent surface. Compare at close and distant views; same texel orientation, correct alpha and mip transitions. Asset-name correspondence is a data measurement, not a way to prove retail search order; ambiguous priority remains open. |
| U07 | curve precision/interpolation/extrapolation bits, frame endpoints, material/UV/scale target options | One witness per measured curve format. Capture initial/middle/final keys plus two loop boundaries and fractional playback positions. Pose continuity, endpoint timing, shortest quaternion arc, and UV/material periodicity must agree; no forced LINEAR replacement. |
| U08 | physical motion/base/suffix mapping; empty motions; extra slots 27..31; exact gameplay names | Freeze clips for logical ids 30..33, state-selected 27, and protagonist special-state 28 using owner saves that naturally reach those presentations. Record clip source/slot separately from logical request and the full MODEL0 record; verify no one-slot base-motion shift and no id34→27 alias. If a state cannot be reached without prohibited tools, record unvalidated and seek the owner's ruling, not a manufactured pass. |
| U09 | per3Helper TRQ vs other lengths, quaternion convention, trailing fields | Equip distinguishable right-/left-hand weapons and a character whose grip is offset. Compare idle and attack grip at several rotations: attachment does not orbit, mirror or drift. Repeated helper ids on distinct owners stay isolated. Noncommon lengths need a separate witness. |
| U10 | NECK_BENDER .dat fields, head tag 5001, blend and attached-part interaction | Capture a long-haired character looking left/right/up/down with glasses or head accessory. Freeze head angles, hair continuation and accessory parent. Compare animation with and without active look-at in ordinary scenes; no detached hair/accessory jump. |
| U11 | EFFECT_EPL variants, TRACK, ADDMOT and EDGE internals and motion lifetime coupling | M8f and M8c-2 share a card per adjunct variant: fixed camera, start/end/loop of its action. Freeze start/end frame, attachment bone and trail/edge persistence. A disabled effect is not an acceptable model witness. |
| U12 | generic type/id reachability and dependent-resource ordering beyond committed selector contracts | Log the port's model and dependent-resource requests for the agreed normal, boss-transform and explicit-summon scenes; compare owner-visible identities and completion order. Apply only documented constructor override, never equality of MODEL1 rows. This validates dynamic reachability, not new selector RE. |
| U13 | AMD/GMO/neck suffix semantics, retained option/reserved fields | Repeat relevant load/pose captures across models representing every suffix-length/option variant. Require the same result without consuming suffix bytes as children. If the suffix's semantic effect is not observable, keep it opaque/unknown; BBV cannot establish that exporter residue is always meaningless. |
| U14 | remaining chunk types/custom tags, including rare 806B/80E2 and any annex-only type | Use the annex's local example path to identify a reachable scene and freeze a per-type observable before decoding it. Tool/editor-only objects with no legal retail route get N=0, unreachable/unvalidated. Require independent public facts/data constraints or an explicit owner scope decision; never label silent skipping as support. |
| U15 | numeric rounding, binary16 handling, key normalization | Long looping animation and frame-stepped start/end captures, compared with double-precision synthetic reference math and the chosen binary32 evaluator. No accumulating drift or discontinuity; identical cache rebuild gives identical decoded numerical data. Captures alone do not prove bit-exact floating-point internals. |

All opaque AMD adjunct payloads, generic command-word views without a semantic
name in the annex, unassigned upper flag bits, suffix bytes, and retained
reserved/option fields are explicitly covered by U03/U05/U07/U11/U13/U14.
There is no implied default interpretation for them.

## 10. Implementation split and acceptance boundaries

### M8c-1: static models, hierarchy, materials and attachments

Implement Golden-only bounded PAC/AMD/GMO framing, a scope/reference index,
static bone transforms and parts, measured vertex/draw formats, complete
material/layer and texture binding, helper metadata, and a new owned Golden
model resource. Use M7c VFS and M8a DDS decoding; resolve embedded TIM2 handling
explicitly. Resource identity contains the outer chain, all dependent texture
identities and a converter id/version. Do not store host pointers in guest slots.
Build a genuine Golden hierarchy: the mod glTF replacement path's PS2 skeleton
fallback is not a native loader.

Required synthetic vectors: nested scopes, truncated/overflowed lengths, invalid
references/cycles, exact name boundaries, root-vs-member suffix, PAC variants,
negative fixed components, final buffer padding, all measured primitive and
material variants, helper offsets and texture-alpha cases. Census qualification
must classify all objects and decode every static-capable class or keep a named
refusal. An unsupported material is not made opaque merely to lift R5/R7b/P5.
Owner-local static visual witnesses cover every measured static variant and U01,
U02, U03, U05, U06, U09. Do not advertise animated or skinned readiness yet.

### M8c-2: skinning, 32-slot motion integration and adjunct coupling

Preserve all measured weight lanes; implement palettes, matrices and complete
skeleton binding, then curve evaluators, scale/material/UV channels, base/slot
selection, 32-record forwarding, empty-entry semantics, NECK_BENDER and animated
attachments. Share effect-lifetime semantics with M8f; do not drop them to show
an apparently animated model. Resolve U04/U07/U08/U10/U11/U15 and any rare
variant discovered in the annex before claiming its runtime support.

Synthetic fixtures must distinguish bone id vs palette index vs helper id, use
more than four weights where measured, nonuniform scale, quaternion antipodes,
constant/linear/nonlinear curves, half-time endpoints, empty motions, base-vs-slot
off-by-one, all five extra slots, and dependent-resource completion ordering.

At the implementation milestone, collect the roadmap's PS2-unchanged x64,
x64w, win-tests and a64 exact-tip receipts serially per agent, plus unchanged
PS2 scenes64 hashes and Golden-only registration/isolation tests. This spec lane
changes no runtime code and does not claim those receipts. R5, R7b, P5, P12 and
the render half of R3 lift only after the relevant complete native paths and
owner witnesses pass, never merely because this framing census is complete.

## 11. Synthetic framing vectors

No game bytes are needed for these fixtures:

1. Empty GMO: 16-byte fixed signature; a type-2 block at +16 with H=16,
   S=16, C=16, A=16. Total 32 bytes. Appending eight nonzero synthetic bytes
   changes only the opaque suffix, not the root children.
2. AMD: `CHNK`, count 1; name `MODEL_DATA` in name[16], type 0x101,
   length 32; the empty GMO above. Total 64 bytes; no inter-member padding.
3. PAC v2: count 1; `toy.AMD` in name[32], length 64; the synthetic AMD.
   Total 104 bytes. The PS2 name[252] parser must not be used.
4. Array layout: six byte weights, UV2 signed halfwords, normal3 signed
   halfwords, position3 signed halfwords occupy 22 bytes per vertex. Three
   vertices occupy 66 bytes plus 2 final pad bytes, not 24*3.
5. Half-precision linear XYZ curve: two rows of (time, x, y, z), all binary16,
   times 0x0000 and 0x3C00. Total data 16 bytes. A constant quaternion row
   occupies 10 bytes plus 2 pad bytes. These are not PS2 compressed floats.
6. Helper: id 500, translation (1,2,3), quaternion (0,0,0,1), eight zero
   suffix bytes, inside a padded `per3Helper` tag payload. Candidate transform
   is a pure offset, not the nonsensical matrix obtained by reading nine floats.
7. Motion records: slot31 begins at +370, its last halfword at +378; +380 is
   tail, not slot32. Logical30→28 and logical33→31 do not consume the base clip.

These specify expected construction/interpretation; no synthetic test execution
or retail runtime BBV is claimed by this document.

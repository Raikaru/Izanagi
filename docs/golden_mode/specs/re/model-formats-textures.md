# GMO embedded textures: TIM2, BMP and optional high-resolution DDS

Part of [M8c](model-formats.md). Evidence labels and clean-room rules are those
of the parent document. The [census annex](model-formats-census.md) supplies
**exhaustive measured counts and value ranges**, including fields that are zero
in this build. No proprietary image decoder or converter implementation was
read. Container facts are independently described by
[OpenKh's TIM2 format record](https://openkh.dev/common/tm2.html); that page's
code example is not used. The BMP description is the ordinary
[Microsoft BITMAPFILEHEADER](https://learn.microsoft.com/en-us/windows/win32/api/wingdi/ns-wingdi-bitmapfileheader)
and [BITMAPINFOHEADER](https://learn.microsoft.com/en-us/windows/win32/api/wingdi/ns-wingdi-bitmapinfoheader)
format. Published facts do not establish Golden's actual variants; measurement
does.

## 1. GMO image boundary

Texture block 000A has a name, a source-filename command 8012, and optionally a
file-image command 8013. The latter's payload begins with a **u32 byte length**,
then exactly that many image bytes; remaining command bytes are padding. Do not
mistake the length word for the TIM2/BMP header. Three texture blocks have no
file-image command; do not fabricate their pixels. The whole-corpus image count
is **25,747: 25,742 TIM2 and 5 BMP**. Duplicated textures across models remain
separate occurrences in these counts.

A historical filename is not an external-file guarantee. Embedded images are
real source resources and must remain usable without high-resolution DDS.
Image dimensions, CLUT sizes and image lengths are checked before allocation.
No GS VRAM address or serialized numeric word may become a host pointer.

## 2. TIM2 file framing

The measured Golden framing is the standard TIM2 container. The full common
header, relative to the embedded image, is:

| Offset | Type | Field |
| ---: | --- | --- |
| +00 | byte[4] | ASCII `TIM2` |
| +04 | u8 | revision |
| +05 | u8 | alignment/style flag |
| +06 | u16 | picture count |
| +08 | u32 | reserved |
| +0C | u32 | reserved |

**OBSERVED:** all 25,742 TIM2 files have revision **3**, style **0**, one
picture, one mip, a **48-byte** picture header, and picture format **0**.
The first picture is at +0x10. File reserved words, all GS metadata words,
picture padding and file trailing lengths are zero. Width and height are
powers of two in 8..512. Do not reject revision 3 because public descriptions
often illustrate revision 4. Other versions/styles/mips are outside the measured
build contract, not implicitly supported.

### 2.1 Picture header

Offsets are relative to the start of each picture:

| Offset | Type | Meaning |
| ---: | --- | --- |
| +00 | u32 | total picture span, including header, image, CLUT and picture padding |
| +04 | u32 | CLUT byte count |
| +08 | u32 | image byte count |
| +0C | u16 | picture-header byte count |
| +0E | u16 | CLUT colour count |
| +10 | u8 | picture format/options |
| +11 | u8 | mip count |
| +12 | u8 | CLUT colour type and layout flags |
| +13 | u8 | image colour type |
| +14 | u16 | width |
| +16 | u16 | height |
| +18 | u64 | GS TEX0 metadata |
| +20 | u64 | GS TEX1 metadata |
| +28 | u32 | GS register metadata |
| +2C | u32 | GS CLUT metadata |

Require `48 <= header_bytes <= total <= remaining_file_bytes` and that
`image_bytes + clut_bytes <= total - header_bytes`, using overflow-safe
arithmetic. Pixel data starts at picture + header_bytes. **The CLUT follows the
pixel data**, at picture + header_bytes + image_bytes. Advance to the next
picture by total, not width*height or the CLUT end. Record any file suffix after
the declared pictures separately. The corpus's mip/header-size/padding
measurements, not a guessed generic extension layout, define the admitted
variant set. If a future file has multiple mips or an extended header, refuse
until its explicit framing and mip byte boundaries are specified.

The GS words are serialized metadata. Even where public TEX0 bit fields are
known (PSM at bits 20..25, CPSM at 51..54, CSM at 55, CSA at 56..60), zero or
legacy metadata does not independently prove how the embedded CPU pixels are
arranged. Do not reconstruct PS2 VRAM uploads from these files.

### 2.2 Pixel and CLUT encodings

Public TIM2 image colour-type ids are not TMX's GS PSM ids:

| TIM2 image type | Meaning | Corresponding TMX pixel family, not header value |
| ---: | --- | --- |
| 1 | 16-bit A1B5G5R5 | 16-bit RGB555 family |
| 2 | direct RGB24 family | 24-bit family |
| 3 | direct RGBA32 | 32-bit family |
| 4 | 4-bit indexed | PSMT4 / TMX 0x14 |
| 5 | 8-bit indexed | PSMT8 / TMX 0x13 |

Only **measured** rows are required for this build; the annex reports exact
variant tuples. For indexed images, palette entry count and palette byte count
must agree with the measured CLUT encoding. CLUT base type 3 is four bytes per
entry in R,G,B,A byte order. Preserve alpha as data; it is not implicitly a
PS2 GS 0..0x80 value. Direct data and palette layout are separate questions.

| Measured image type | CLUT type | Colours / CLUT bytes | Objects |
| ---: | ---: | --- | ---: |
| 2: RGB, **three bytes per pixel** | 0 | 0 / 0 | 7 |
| 3: RGBA, four bytes per pixel | 0 | 0 / 0 | 18 |
| 4: indexed4 | 3: RGBA32 | 16 / 64 | 2,066 |
| 5: indexed8 | 3: RGBA32 | 256 / 1,024 | 23,651 |

Every picture's image byte count equals width*height times the measured depth;
all picture spans equal header + image + CLUT, without additional picture
padding. None has a linear-CLUT flag or a 16-bit CLUT. Among 25,717 palettes,
25,602 have a maximum alpha above 128; **1,861** contain intermediate 1..254
values. The complete alpha extrema are 0..255, not just binary transparency.
These observations preclude blindly reusing TMX's GS alpha expansion.

**INFERRED, publicly corroborated:** 8-bit CLUTs without the linear-layout flag
use the CSM1 palette permutation: within each 32-entry group interchange
entries 8..15 with 16..23. Equivalently, exchange index bits 3 and 4, leaving
all other bits unchanged. A 4-bit palette has only 16 entries and must not be
run through the 256-entry permutation. The census records the CLUT type/layout
flags, so a later linear CLUT variant cannot accidentally inherit this rule.
Apply a permutation once, not both to palette and pixel indices.

**Measured discriminator, not a retail execution:** a hash-checked enemy
texture's embedded TIM2 and its matching high-resolution DDS were compared
at the embedded resolution. With straight RGB palettes and row-major indices,
CSM1 palette order gave RGB mean absolute error **2.1588948568**, versus
**6.8479003906** for an identity palette. The DDS was decoded with Pillow
10.4.0 in the reproducible native-Python witness (an independent 12.3.0
measurement gave the same errors) and BOX-resized. These are numerical
comparisons of the owner's data, not imported images or a shipped decoder.
BCn compression/high-resolution art prevent treating the errors as byte equality.
U06 still qualifies the complete palette and pixel-layout rule for every variant.

Image length alone cannot prove pixel swizzle. The provisional CPU path is
row-major indices; 4-bit indices use the low nibble for the even pixel and the
high nibble for the odd pixel (P/I). Preserve any picture-format differences
rather than treating a width*height match as proof of identical swizzle.
U06 compares texture orientation, edge/seam structure and palette colours on
all measured variants. Any unmatched swizzled class remains refused rather
than decoded with a plausible-looking wrong palette.

### 2.3 Delta against the actual existing decoder: TMX, not TIM2

**OBSERVED/C:** there is no TIM2 container decoder in M7c `ef7b5b7ed` or
M8a `12fcaafe3`. The reusable code is `src/render/p4tmx.c`'s
`p4tmx_decode`, a **TMX0** reader. Its current interface consumes a complete
TMX file; passing TIM2 bytes to it will correctly fail. Share or factor pixel
primitives behind Golden-only container handling only after reviewing the
following differences:

| Field / behaviour | Existing TMX decoder | Golden embedded TIM2 requirement |
| --- | --- | --- |
| Signature | initial 2,0 and TMX0 at +8 | TIM2 at +0 |
| Header | fixed 0x40 bytes | 16-byte file header plus declared picture headers |
| Width/height | u16 +0x12/+0x14 in TMX header | u16 picture +0x14/+0x16 |
| Pixel selector | GS-style PSM at +0x16 | TIM2 image type at picture +0x13 |
| Palette count/type | +0x10/+0x11 | picture colour count +0x0E and CLUT type +0x12 |
| Data order | palette first, then pixels | pixels first, then CLUT |
| Bounds | one image inferred from dimensions | explicit picture/image/CLUT byte spans |
| 8-bit palette | swap middle eight entries per 32 | reusable permutation only for the measured non-linear CLUT class |
| 4-bit packing | low nibble first | candidate same packing; qualify with U06 |
| RGB555 palette transparency | TMX forces entry 0 transparent | not a TIM2 rule; cannot inherit this shortcut |
| RGBA alpha | `a>=0x7F ? 255 : (a*255)>>7` | full-range alpha exists; do **not** apply TMX expansion blindly |
| Mips/pictures | one returned image | preserve every admitted picture/mip, or refuse unmeasured variants |
| GS image swizzle | not a generic TIM2 decision | use the measured picture/CLUT class and U06, not GS address fields |

The measured discriminator's palette alpha is 255. Thus these are not all
unchanged PS2-alpha palettes even though the container says TIM2. The census
also measures alpha minima/maxima and whether intermediate values exist; those
observations alone do not prove the transfer function for every intermediate
value. Proposed Golden TIM2 output is straight RGBA8, retaining full-range alpha;
compare partially translucent witnesses in U06 before marking the path supported.
DDS alpha remains M8a's DDS contract, independently of TIM2.

## 3. Minimal BMP contract for the five embedded files

Measure and admit only the observed variant tuple in the annex. The relevant
common file fields are:

| Offset | Type | Meaning |
| ---: | --- | --- |
| +00 | byte[2] | `BM` |
| +02 | u32 | declared file byte size |
| +06 | u32 | two reserved u16 fields |
| +0A | u32 | absolute pixel-data byte offset |
| +0E | u32 | DIB-header size |
| +12 | s32 | width |
| +16 | s32 | signed height; positive is bottom-up |
| +1A | u16 | plane count |
| +1C | u16 | bits per pixel |
| +1E | u32 | compression |
| +22 | u32 | image byte count, possibly zero for uncompressed data |
| +26/+2A | s32 | horizontal/vertical pixels per metre |
| +2E/+32 | u32 | colours-used / important-colours |

For an observed uncompressed 24-bit BITMAPINFOHEADER image, pixels are B,G,R,
row byte stride is `(3*width + 3) & ~3`, positive heights reverse row order,
and output alpha is 255. Bounds cover every padded row inside the declared
file span. No indexed/RLE/bitfield BMP support is implied unless measured.
Do not confuse BGR with TIM2's RGB byte order. A malformed signed dimension,
overflowed row count, palette overlap or missing row is a refusal.

**OBSERVED:** all five have DIB size 40, one plane, 24 bpp, compression 0,
pixel offset 54, declared image-byte field 0, no palette, zero reserved/important
fields and zero file suffix. Width is 64 throughout; one has positive height 32
and file size 6,198, four positive height 64 and file size 12,342. Each GMO
image command has two final pad bytes, outside its declared BMP length.
Thus this minimal uncompressed BGR decoder covers all five; no RLE, bitfields,
indexed BMP or alpha-bearing BMP is required by the census.

## 4. High-resolution DDS binding rule and qualification

The measured candidate rule is exact case-folded basename/texture-id matching
against 3,790 `model/hrestex/dummy/*.dds` objects. Candidate A uses the GMO texture
block id; candidate B strips the source filename's final extension and directory.
Known image extensions on block ids are stripped as well. No namespace/id/suffix
rewrites are introduced.

Names are **8-bit byte strings**, not guaranteed ASCII: 20 texture occurrences
have non-ASCII names/filenames. Nine DDS TOC names were lossily represented with
U+FFFD by M7c's UTF-8 replacement decoder. The measurement tool reads the raw
TOC string cells by TOC index, checks their decoded inventory identity, and uses
strict CP932 as a **candidate** interpretation of non-ASCII artist names.
This recovers all nine DDS names without modifying the shared archive reader.
It is not proof of retail filename-encoding semantics; retain original bytes,
not the census's replacement characters. U06 covers this encoding boundary.

**OBSERVED:** candidate B matches **19,590 / 25,750 texture blocks (76.077670%)**;
candidate A matches **19,516 / 25,750 (75.790291%)**. Every A match is also a B
match, and **zero** cases select two different DDS objects. There are **6,157**
embedded blocks and **three external-only blocks** without either association.
Of the 3,790 distinct DDS stems, **3,789** are reached (99.973615%); one is not.
There are no case-folded stem collisions. Candidate ids differ in **83** texture
occurrences, so treating the two fields as interchangeable would lose coverage.
The annex gives:

* occurrences matched and unmatched by each candidate;
* equality/disagreement of the two candidate ids;
* conflicting cases where both names select different DDS objects;
* distinct DDS identities reached and unused;
* separate results for embedded and external-only textures.

**Status: OPEN.** A deterministic name match is a source association, not proof
of the retail loader's priority, fallback, per-model overrides or texture-id
selection. Even 100% name coverage would not prove that behavioural rule.
Main's 2026-11-12 ruling is to **default to embedded textures while DDS binding
is unproven**. For the three external-only texture blocks, absence of an
established source is a named refusal, not a manufactured embedded image.

U06 must freeze a paired owner capture of embedded-detail vs high-resolution
appearance, plus a cutout and partial-alpha case, at near/far distances. Data
qualification checks every measured name association and conflict; runtime
qualification checks both explicitly selected embedded and approved DDS paths.
Only after an unambiguous total mapping and the priority contract are established
may the implementation advertise the high-resolution path as proved. No option
may fall back to a PS2 disc texture in Golden mode.

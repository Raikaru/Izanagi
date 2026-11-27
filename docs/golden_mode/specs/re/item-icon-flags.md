# Item icon flags: retail selector and Golden differences

This specification establishes one Golden flag-icon selector and the PS2
counterpart. A Steam default atlas member is identified below; native SPR
support and the icon-resource adapter remain necessary.

## Golden flag-icon selector

For Steam build 10836033, `0x418930` sign-extends the item ID as a 16-bit
integer and reads the `u32` at offset 4 of its 68-byte row. It tests the
following masks in order and returns the sprite index of the **first** match,
or -1 when none matches. The drawing wrapper at `0x418860` skips the draw
when the selector returns -1; otherwise it draws that index using a texture
supplied by its caller. The selector itself performs no item-ID bounds check.
These Golden sprite indices are not PS2 icon modes.

### Steam 10836033 atlas ownership — OBSERVED

The Steam drawing path at `0x417D00`–`0x417DAA` takes an optional
texture/resource pointer from the item display record at `+0x20`. If it is
null, it calls `0x533480` with the name `init/c_main01x2.spr` and passes
the resulting pointer as the fifth argument to `0x418860`; a non-null
record pointer overrides this default. A second caller at `0x105570`–
`0x10563D` searches its resource list for the same name and passes that
resource to the same wrapper. These call paths therefore identify the
default item-icon sprite resource that supplies this selector's sprite
indices; they do not establish that every caller uses the default or that
every index in the numeric interval is selected.

The archive census identifies the English Steam member as
`data_e.cpk::init_free.bin::init/camp.arc::c_main01x2.spr`. This is an
archive-member ownership result, not a specification of the SPR's internal
texture layout or a completed asset decoder.

**CORROBORATED, Switch patch 131072 vs Steam 10836033:** Switch
`0x71002A6CD4`–`0x71002A6D28` likewise uses a record pointer at `+0x20`
when present; otherwise it resolves `init/c_main01x2.spr` via the name
pointer at `0x7100880800`, then invokes selector `0x71002A74B0`. Its
selector uses the same complete ordered mask/index table as Steam
`0x418930` (**CORROBORATED**, Switch `0x71002A74B0`). This removes the
default-resource ownership unknown in R6 but does not implement the
resource adapter or lift the boot refusal.

| Priority | Mask | Golden sprite index |
| ---: | ---: | ---: |
| 1 | `0x00000081` | `0x33` |
| 2 | `0x00000102` | `0x34` |
| 3 | `0x00000204` | `0x35` |
| 4 | `0x00000408` | `0x36` |
| 5 | `0x00000810` | `0x37` |
| 6 | `0x00001020` | `0x38` |
| 7 | `0x00002040` | `0x39` |
| 8 | `0x00004000` | `0x3B` |
| 9 | `0x00008000` | `0x3D` |
| 10 | `0x00010000` | `0x3F` |
| 11 | `0x00020000` | `0x40` |
| 12 | `0x00040000` | `0x3E` |
| 13 | `0x00080000` | `0x50` |
| 14 | `0x00100000` | `0x51` |
| 15 | `0x00400000` | `0x53` |
| 16 | `0x00800000` | `0x4F` |
| 17 | `0x01000000` | `0x52` |
| 18 | `0x02000000` | `0x54` |
| 19 | `0x00200000` | `0x4D` |
| 20 | `0x04000000` | `0x3C` |
| 21 | `0x08000000` | `0x4C` |
| 22 | `0x10000000` | `0x41` |

Synthetic Golden vectors:

| Flags | Result | Reason |
| ---: | ---: | --- |
| `0` | -1 | no matching flag |
| `0x00000104` | `0x34` | priority 2 precedes priority 3 |
| `0x00080000` | `0x50` | expanded bit 19 |
| `0x00200000` | `0x4D` | expanded bit 21 |
| `0x00600000` | `0x53` | bit 22 has priority over bit 21 |
| `0x02000000` | `0x54` | expanded bit 25 |
| `0x20000000` | -1 | bit 29 is not tested |

## Sprite-rectangle adapter

**CORROBORATED runtime lookup**, Switch `0x71002A7430` →
`0x710034C390` and `0x710034DC10`: a sprite resource's signed sprite
count is at `+0x326`; its entry-array pointer is at `+0x308`. Select entry
`array + 0x80 * sprite_index`. The signed 32-bit texture index at entry
`+0x14` selects a texture handle at resource `+0x108 + 8 * texture_index`.
The signed 32-bit coordinates at entry `+0x54`, `+0x58`, `+0x5C`,
`+0x60` are corners `(x0,y0)` and `(x1,y1)` in **logical texture
coordinates**, not necessarily physical TMX pixels. Their difference is the
unscaled logical destination size. Nonzero signed halfwords at `+0x74`,
`+0x76` override the respective destination dimensions, not the source
rectangle. Entry `+0x44/+0x48` contributes position offsets; entry `+0x18`
flag bits 0 and 1 swap texture coordinates (**SWITCH-ONLY**). A non-null
caller resource is an override, so always resolve its entry, not the default's.

**Steam data observation:** The English default member is an `SPR0` file,
with 32 textures and 844 sprites. All 22 selector results have distinct,
in-range sprite records, valid texture indices, increasing rectangle corners,
and neither of the two flip bits set. These counts are not layout assumptions
for caller overrides. On-disk, the 0x20-byte little-endian header contains
signed 16-bit texture and sprite counts at `+0x14` and `+0x16`, and 32-bit
offsets to their respective pointer tables at `+0x18` and `+0x1C`. Each
table element is 8 bytes; its second 32-bit word is a file-relative offset
to a texture or a 0x80-byte sprite entry. Entry fields above use the same
offsets as the runtime entries. This layout is consistent with the public
[PersonaSpriteTools SPR reader](https://github.com/Secre-C/PersonaSpriteTools/tree/main/Sprite/Spr).
The file's texture payloads still require the appropriate texture decoder.
Steam `0x41890F` → `0x253400` → `0x534000` reads the same `+0x308`
array, `0x80` stride, texture index and source corners; `0x533EF0` and
`0x533F60` corroborate the width and height overrides. Do not infer that
every renderer detail is identical.

### Logical coordinates and HD TMX scale

**CORROBORATED:** A TMX has physical width and height as little-endian
halfwords at `+0x12` and `+0x14`. Its little-endian halfword at `+0x02`
selects the HD coordinate rule: value 1 means scale `m = 2`; otherwise
`m = 1`. Switch TMX loader `0x710039A530` copies the physical dimensions
to the runtime texture and sets bit 0 of its `+0x28` flags for value 1.
Texture binding `0x710033B5A0` propagates that bit; shader-uniform setup
`0x7100340F50`–`0x7100340F98` uses logical dimensions `(W/m,H/m)` and
reciprocals `(m/W,m/H)`. Steam `0x537FE0`–`0x538067` propagates the same
runtime flag, and ordinary routines `0x52E750`–`0x52E7C0` and
`0x58FC50`–`0x58FC8F` report the dimensions multiplied by `0.5` when it
is set; the float constant at Steam RVA `0x964C38` is `0.5`.

For a decoded physical `W × H` TMX, a sprite rectangle therefore covers
physical pixel corners `(m*x0,m*y0)`–`(m*x1,m*y1)`, with normalized UV
corners `(m*x0/W,m*y0/H)`–`(m*x1/W,m*y1/H)`. This is a texture-wide
scale, not a change to the destination width/height override. No
per-texture coordinate offset is applied by the cited sprite/texture path.
In particular, cropping an HD texture directly at the unscaled record
coordinates extracts the wrong fragment. The public
[PersonaSpriteTools SPR disassembler](https://github.com/Secre-C/PersonaSpriteTools/blob/main/SprDisassembler.py)
independently calls for multiplier 2 on P4G PC.

The sprite index is zero-based and directly selects the sprite-directory
entry: there is no `0x33` subtraction or one-based adjustment. The signed
texture index at entry `+0x14` is likewise zero-based and directly selects
the texture-directory entry; it is not the sprite index. Switch
`0x710034BC70`–`0x710034BCC0` copies each 0x80-byte sprite entry in directory
order; `0x710034BD14`–`0x710034BD94` populates texture slots in texture
directory order. Steam's indexed lookups are cited above.

**Steam data observation:** All 22 item-selector sprites and the four arcana
sprites `0x6C`, `0x251`, `0x252`, `0x253` select TMXs with HD field 1 and
have in-bounds source rectangles after multiplication by 2. Across these 26
records, 24 select texture-directory entry 2 and two select entry 18. These
are measurements for this exact English atlas, not a rule for overrides.

| Synthetic texture | Logical sprite corners | Physical crop corners | Logical destination size |
| --- | --- | --- | --- |
| `128×128`, TMX `+0x02 = 1` | `(8,16)`–`(24,40)` | `(16,32)`–`(48,80)` | `16×24` |
| `128×128`, TMX `+0x02 = 0` | `(8,16)`–`(24,40)` | `(8,16)`–`(24,40)` | `16×24` |

Apply flips to the selected rectangle, validate the **scaled** pixel corners
against the selected physical texture, and retain logical positioning and
destination dimensions separately.

For a bounded adapter, validate the selected sprite index, pointer-table and
entry ranges, texture index and source rectangle against the decoded texture.
Preserve the actual resource, texture selection, source corners and flip and
position metadata; do not substitute a fixed-size grid or assume the selector
index is a texture number. Synthetic vectors below use a texture with
TMX `+0x02 = 0` (scale 1):

| Selector index | Sprite count | Entry texture index | Source corners | Result |
| ---: | ---: | ---: | --- | --- |
| `-1` | 100 | — | — | no draw |
| `0x33` | 100 | 2 | `(8,16)`–`(24,40)` | texture 2, source 16×24 |
| `0x33` | 51 | — | — | reject: index equals count |
| `0x34` | 100 | 4, with 4 textures | `(0,0)`–`(8,8)` | reject: texture index equals count |
| `0x33` | 100 | 2 | `(8,16)`–`(24,40)`, destination overrides 32×12 | same source 16×24, destination 32×12 |

## Arcana badge sprites, including 24–27

**CORROBORATED selector:** Switch `0x71002A7B40` and Steam `0x4190B0`
select the sprite from an unsigned arcana ID. On Switch the caller asserts
the valid range is 1–31. First, if the platform's special state is active,
replace ID 25 with 26; otherwise leave it unchanged. Switch obtains that
state by querying game flag `0x815` at `0x710012EA50`; Steam tests bit
`0x200000` at offset `0x100` of its referenced state object
(`0x4190C5`–`0x4190E6`). The equivalence of these state-storage
representations is not assumed. Then, for the resulting ID `a`, select
sprite `0x54 + a` when `a <= 24`, or `0x238 + a` when `a >= 25`.
This is not the PS2 `0x20 + a` index.

**SWITCH-ONLY ownership and drawing:** The persona screen initializes its
resource from `init/c_main01x2.spr` at `0x71002A8730`–`0x71002A8744`.
It forwards that resource through `0x71002B0804` → `0x71002ADB80` →
`0x710017C1B0`. The latter obtains the sprite index from the selector,
then uses the same `0x710034C390`/`0x710034DC10` rectangle and texture
lookup described above. Thus the adapter must resolve the selected record,
not infer a grid position. For the English Steam default atlas, all four
records in the table below exist, have distinct entry pointers, in-range
texture indices and increasing source corners. These are data observations,
not copies of the atlas.

| Synthetic arcana | Special state | Effective ID | Sprite index |
| ---: | --- | ---: | ---: |
| 24 | either | 24 | `0x6C` |
| 25 | off | 25 | `0x251` |
| 25 | on | 26 | `0x252` |
| 26 | either | 26 | `0x252` |
| 27 | either | 27 | `0x253` |

P9's index and default-atlas lookup are specified; rendering still depends on
the atlas decoder, state binding and resource adapter.

## PS2 behaviour

The PS2 item flag accessor at `0x00106880` reads the `u32` at byte offset 4
of the item's 68-byte metadata row. The icon renderer at `0x00113280` tests
the following masks in order and selects the first matching row. A zero mask
result draws no flag icon. `item` and `badge` denote two distinct caller-owned
textures; mode is the sprite index within the selected texture.

| Priority | Flag mask | Texture | Mode |
| ---: | ---: | --- | ---: |
| 1 | `0x000081` | item | `0x14` |
| 2 | `0x000102` | item | `0x15` |
| 3 | `0x000204` | item | `0x16` |
| 4 | `0x000408` | item | `0x17` |
| 5 | `0x000810` | item | `0x18` |
| 6 | `0x001020` | item | `0x19` |
| 7 | `0x002040` | item | `0x1A` |
| 8 | `0x004000` | item | `0x1B` |
| 9 | `0x008000` | item | `0x1C` |
| 10 | `0x010000` | badge | `0x1B` |
| 11 | `0x020000` | badge | `0x1C` |
| 12 | `0x040000` | badge | `0x1A` |

This is one icon layer. The separate category-background layer has its own
mask tests (`0x00112610`); neither layer provides an owned standalone image
resource suitable for the M5 registry by itself.

## Steam data measurement

The Golden table has 2,560 rows of 68 bytes; retail has 1,536. Comparing the
little-endian word at offset 4 by the same row index, 1,429 of 1,536 pairs
are identical. Of the other 107, retail `0x20000` becomes Golden `0x2000000`
in 55 rows; retail `0x10000` becomes Golden `0x200000`, `0x400000`,
`0x4000000`, or `0x8000000` in 32, 12, 4, or 4 rows respectively. These
are data differences, not a proven translation of icon semantics.

Applying the PS2 mask sequence mechanically to all 1,024 additional row
indices finds no matching mask in 589 rows. The count includes all rows,
including unused/reserved rows; it is not a count of playable items. Thus
copying the PS2 selector over the Golden flag word cannot be assumed to
provide a flag icon for every expanded row. The Golden selector above handles
these extra flag bits; decoding and binding the atlas remain required.

## Synthetic vectors for the PS2 selector only

| Flags | Expected texture | Expected mode | Reason |
| ---: | --- | ---: | --- |
| `0` | none | none | no matching flag |
| `0x80` | item | `0x14` | first mask |
| `0x104` | item | `0x15` | second mask wins over third |
| `0x10000` | badge | `0x1B` | tenth mask |
| `0x40000` | badge | `0x1A` | twelfth mask |
| `0x200000` | none | none | not a recognized PS2 mask; no Golden conclusion |

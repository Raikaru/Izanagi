# Golden native textures and sprite atlases (M8a)

## Provenance and license record

Implementation `golden_texture.c` / `golden_atlas.c` is original project code
(MIT, repository LICENSE). BC1/2/3 are original; measured BC7 uses
[bcdec 0.985](https://github.com/iOrange/bcdec), pinned to
`80859ed3b7afb1c527a2a99d70c61457bea72d0c`, under its MIT option.
Only its BC7 function is referenced, through static-inline inclusion; it
allocates nothing and has no dependencies, I/O or configure flags. The local
endpoint zero-initialization patch avoids indeterminate implicit-alpha reads;
the upstream copyright/license and selected MIT notice are retained in
`third_party/licenses/bcdec-MIT.txt` and every package notice/copy list.
DirectXTex is not used. TMX and bounded DDS framing reuse existing code.
PNG uses reviewed LodePNG 20261006, commit
`e584a8490db1b76b81bf298c11c09d3b55d9c270`, under zlib, with its existing
notice and bounded allocator. No new shared library or configure enable set.
The GL seam uses RGBA8 CPU fallback on every platform, including GLES: it does
not require S3TC driver availability. DDS sRGB is retained as storage metadata;
no CPU gamma conversion is applied. Mip levels and straight alpha are retained.

Public format sources (facts only; no prose or decoder code copied):

- [Microsoft DDS_HEADER](https://learn.microsoft.com/en-us/windows/win32/direct3ddds/dds-header)
  and [DDS pixel format](https://learn.microsoft.com/en-us/windows/win32/direct3ddds/dds-pixelformat).
- [Khronos EXT_texture_compression_s3tc 1.6](https://registry.khronos.org/OpenGL/extensions/EXT/EXT_texture_compression_s3tc.txt):
  RGB565 endpoints, BC1 transparent mode, BC2 four-bit alpha, BC3 interpolated
  alpha. BC2/3 always use four-color RGB, regardless of endpoint order.
- [Amicitia TMX](https://amicitia.miraheze.org/wiki/TMX), revision 5340:
  64-byte TMX0 framing, direct and palette formats. Existing `p4tmx_decode`
  supplies the PS2-compatible palette permutation, nibble order and alpha
  convention from the clean port/decomp; it is unchanged by this milestone.
- [Amicitia SPR](https://amicitia.miraheze.org/wiki/SPR), revision 4933:
  TMX-containing atlas family. Exact Golden SPR0 record framing and rectangle
  semantics come only from committed `specs/re/item-icon-flags.md` at
  `7e355dda7` (including arcana badges and HD coordinate scaling); no RE notes are implementation inputs.
- [PNG specification](https://www.w3.org/TR/png-3/): existing LodePNG decoder.
- [Truevision TGA 2.0](https://www.dca.fee.unicamp.br/~martino/disciplinas/ea978/tgaffs.pdf):
  uncompressed/RLE truecolor and grayscale, BGR(A) channels and origin bits.

## Census and framing

The M7c census identifies DDS (31,562 objects), TMX (7,757), SPR (1,904),
PNG (5) and TGA (1). Counts include nested duplicates and locales, not unique
images. No TXN, SPD or GXT signature was reported. `texture_census.py` performs
a hash-checked subformat census of those objects, including SPR's embedded
textures; its local-only `texture-variants.json` records exact variants and
example chains. Never commit its image payloads or contact sheet.

The hash-checked variant census measures 41,229 standalone objects:
3,097 DXT1, 5,977 DXT5, 22,486 DDS DX10 BC7 (`DXGI=98`, 2D,
array size 1, straight alpha), two masked BGRA32 DDS, 7,757 TMX,
1,904 SPR0, four indexed8 PNG, one RGBA8 PNG and one raw BGRA32 TGA
(descriptor 8). TMX, standalone and SPR-embedded, uses only pixel formats
0/2/0x13/0x14, palette format 0, at most one palette and zero mip count.
DXGI extension fields are included in the census identity, not collapsed
under the DX10 FourCC. The metadata-only local receipt is
`GoldenM8a-evidence/texture-variants.json`; no game pixels belong in exports.


DDS uses the shared bounded 2D framing reader. Arrays, volumes, cubemaps,
unsupported compression and truncated mip chains refuse explicitly. TMX uses
the 64-byte header and declared total length; palette count, pixel format and
mip count are validated before decoding. SPR0 has its magic at +8, signed
16-bit counts at +0x14/+0x16 and u32 pointer-table offsets at +0x18/+0x1c.
Each pointer table has eight-byte entries, with the file-relative record offset
in the second word. Texture entries are decoded by their magic, never by their
index. Sprite entries are 0x80 bytes. Source corners, texture index, position
and size overrides and flip metadata are preserved. Reject empty/reversed or
out-of-texture source rectangles, negative/out-of-range indices and truncated
tables or records. Caller-supplied atlases override the default completely.

TMX halfword +0x02 equal to 1 scales logical SPR source corners by 2.
Lookup returns physical pixel corners while retaining logical destination sizes
and positioning. Flip bits come from entry +0x18. Sprite and texture indices
are zero-based. The atlas converter identity is `spr0-tmx-dds-bc1237@3`;
older unscaled crops cannot be reused. Synthetic 128x128 vectors cover both
scale rules, exact crops, flips and scaled bounds.

### Codec variants (support, not an install-usage claim)

| Container | Native variants | Preserved or refused |
|---|---|---|
| DDS | DXT1/BC1, DXT2/3/BC2, DXT4/5/BC3; 32-bit RGBA/BGRA masks | Full provided mip chain; BC1 transparent selector; both BC3 alpha endpoint orders; DXT2/4 unpremultiplication |
| DDS DX10 | RGBA8/BGRA8/BGRX8, BC1/2/3/7 and their sRGB variants | BC7 all eight modes; sRGB metadata retained; non-2D/array/cube/unknown formats refused |
| TMX0 | Direct pixel formats 0/1/2/0x0a; indexed 0x13/0x1b and 0x14/0x24/0x2c | One palette, palette formats 0/2/0x0a; 8-bit palette permutation and low-first nibbles; nonzero mip count/multiple palettes refused |
| PNG | Existing bounded LodePNG static-image decoder | Existing common PNG framing/CRC checks and RGBA output; animation refused |
| TGA | Types 2/3/10/11; truecolor BGR24/BGRA32 or gray8 | Raw and RLE packets; top/bottom and left/right origin; color maps/interleaving refused |
| SPR0 | Signed header counts, eight-byte pointer entries, 0x80-byte sprite entries | Selected-record validation; caller-owned atlas override; original destination/position/flip metadata |

Atlas image cache records contain little-endian u32 fields: natural width,
natural height, sprite index, texture index, source x0/y0, position x/y,
destination width/height and flip bits, followed by packed natural-source RGBA.
The 44-byte prefix is portable; signed values retain their two's-complement
representation. A separate cache-purpose version keeps this complete image
object set distinct from the table cache, even with the same source list.


## Synthetic vectors and mode boundary

`tests/golden_texture_test.c` constructs DDS BC1/2/3 blocks, raw DDS, DX10 sRGB
and mip chains, TMX direct/indexed variants and malformed streams, and SPR0
pointer/rectangle fixtures. Color endpoint and alpha interpolation assertions
are numeric, not source-text checks. GL seam tests check exact uploaded mip
bytes, sRGB internal format, rejected input and no GL work in PS2 mode.

The item selector uses the spec's ordered 22 masks (not PS2 modes). Arcana
24/25/26/27 select 0x6c/0x251/0x252/0x253; the caller supplies the independently
bound special state, which maps 25 to 26. Data lookup does not infer that
Switch flag 0x815 and Steam state bit storage are interchangeable. Text and
skill-help rendering remain M8b/M9; no ID-608 out-of-bounds behavior is emulated.

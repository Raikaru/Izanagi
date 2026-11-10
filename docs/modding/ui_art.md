# Native UI art for mods

Mod art should be indistinguishable from the game's own. This page records how
Persona 4's UI art is built, measured from the decomp (`Kernel/sdkSpr.c`,
`Event/Fcl`, `shdSkill.c`, `Camp/`) and the player's own disc, and how
[`tools/ui_art/ui_art.py`](../../tools/ui_art/ui_art.py) produces original art
in those families. Retail images are only measured, in memory: nothing is
extracted, traced, resampled or committed.

## Formats

**Sprite packs (`SPR0`).** A 0x20-byte header (magic `SPR0` at +8, u16 raster
count at +0x14, u16 record count at +0x16) is followed by 8-byte directory
entries (offset in word 1) for the rasters and then the records. Records are
0x80 bytes:

| Offset | Field |
| --- | --- |
| +0x04..+0x13 | Shift-JIS comment |
| +0x14 | raster index |
| +0x18 | flags (4 in UI packs; bits 0/1 flip U/V) |
| +0x2c | render flags |
| +0x44/+0x48 | origin |
| +0x54/+0x58/+0x5c/+0x60 | texel rectangle left/top/right/bottom |
| +0x64 | four RGBA vertex colours, 0x80 = 1.0 |
| +0x74/+0x76 | width/height override (0 = the rectangle) |

At run time a pack is a 0x240-byte `SdkSpritePack` (`rasters[32]` at +0x104,
`recordsAddress` at +0x204, `recordCount` at +0x216). `sdkSpriteDraw`
(0x46b380) maps a record's rectangle divided by its raster's width and height,
so atlases are power-of-two, 32 to 512 texels a side.

**Textures (`TMX0`).** A 0x40-byte header: u16 2, size, `TMX0` at +8, palette
count at +0x10, palette format at +0x11 (0 = 32-bit), width and height at
+0x12/+0x14, pixel format at +0x16 (0x14 4-bit, 0x13 8-bit, 0x00 32-bit).
Pixels are linear; 8-bit palettes use the CSM1 entry order (entries 8..15 and
16..23 of every 32 swapped). Alpha is 0..0x80, doubled by the renderer.

**Fonts.** `font/font0.fnt` (504 glyphs) and `font/font1.fnt` (220 glyphs) hold
32x32 4-bit compressed glyphs with a 16-step grey palette. Mod text uses the
native font calls (`p4.ui.page`, `p4.ui.status`, `p4.ui.overlay`), never
rendered images.

## Families

UI art is white on transparent, quantized to a short alpha ramp, and coloured
by the vertex tint where it is drawn (Analyze's known header (0x24,0x4c,0x00),
unknown (0x70,0x70,0x70), the skill selection bar (0xff,0xa0,0x0b)). Coloured
art therefore comes out dark under a tint. The exception is the Camp list's
two-tone labels, which are drawn untinted.

| Family | Native source | Cell | Alpha ramp | Coverage, stroke |
| --- | --- | --- | --- | --- |
| `element-header` | `init/camp.bin` › `c_equip_help_02.spr` 0x2e..0x34 | 30x23 | 0, 56, 116, 158, 196, 230, 255 | 0.157..0.310, 3..13 px |
| `skill-icon` | same pack, 0xb8..0xc2 | 34..38 x 27 | 16 steps (0, 14, 30 ... 255) | 0.153..0.488, 2..15 px |
| `velvet-symbol` | `facility/velvet01.spr` 0xdf..0xef | 72x77 | 16 steps | 0.067..0.310, 5..13.5 px |
| `velvet-label` | same, 0xfc..0x105 | 72..99 x 22..31 | 16 steps | 0.130..0.242, 3..4 px |
| `velvet-label-small` | same, 0xf2..0xfa | 38..72 x 18 | 16 steps | 0.235..0.322, 3..4 px |
| `camp-label` | `init/camp.bin` › `c_top_01.spr` 0..6 | width x 21..22 | dark-to-white greys 45..255 opaque, white 0..255 outer edge | 0.468..0.608 |
| `element-marker` | none (new) | 12..20 x 9..12 | header ramp | — |
| `battle-prompt` | the battle help bar's "△RUSH" (measured on a render; no record located) | width x 21 | dark greys 24..255 opaque plus the Square pink, dark 0..255 outer edge | 0.70..0.85, 12..21 px |

Battle prompts (`prompt-ailments`, `prompt-affinities`, the Analyze page-flip
hints hard mode declares as `[element-prompt]` images) are a 17-pixel dark
button disc with the Square symbol, then condensed caps (Archivo, weight 600,
width 90, 17 px) shading from white at the cap top to grey 208 at the base,
merged into one dark outline band as the native prompt's letters are.

Further layout facts used by the services:

- The Velvet fusion command cards are an 82x105 card base, a 72x77 symbol, an
  18-pixel small-caps label and a 22..31-pixel large label. They slide in from
  x = -200 or 700 and fade in over 10 updates (`fclSpriteStartAlphaAt(..., 0xA, ...)`).
- The Velvet root list rows sit at x 26, y = 87 + 34n.
- The native system cues are `sdkSndStartStream(0, 0, 0, n)` with n = 0 for
  cursor, 1 for confirm, 2 for cancel and 8 for refuse.

## Making art

`python tools/ui_art/ui_art.py build` writes every asset in its `ASSETS` table:

- **Vector art** (icons, symbols, markers) is deterministic, dependency-free
  Python. A mod's own generator may call `vector_png(name)` so CTest can prove
  that the committed bytes are the generated ones.
- **Text labels** are rendered with Pillow from pinned SIL OFL 1.1 fonts, which
  are fetched once, checked by SHA-256 and never committed (images rendered with
  them are not Font Software under the OFL). They are Liberation Sans Bold for
  Velvet card labels and Archivo at weight 900 and width 125 for Camp labels.
- **Art for native sprites** (`p4.ui.sprite`) is placed in a transparent
  power-of-two atlas at the rectangle that `ASSETS` declares. Physical Split's
  element icons are standalone family cells, drawn by element-ui.

`python tools/ui_art/ui_art.py check [--iso PATH]` checks every asset present
(atlas members separately):

- **Size:** the cell is within its family.
- **Colour:** white at a ramp alpha, or Camp greys.
- **Atlas:** no ink lies outside the declared rectangles.
- **Weight:** coverage is within ±25 % and the median stroke within ±1 px of the
  native family.

With `--iso`, the native ranges are measured from the player's disc instead of
the recorded ones, and `measure --iso` prints them. Without `--iso`, `measure`
uses `P4PC_LOCAL_ISO`, then `P4PC_ISO`, then `$P4PC_LOCAL_DIR/retail/p4.iso`
(only when `P4PC_LOCAL_DIR` is set and that file exists); with none of them it
stops and asks for `--iso`. There is no built-in disc location.

## Acceptance

A new asset is accepted when two things hold. First, `check` passes. Second, an
in-game mod-witness capture shows it beside the native members of its family,
on the same screen and under the same tint.

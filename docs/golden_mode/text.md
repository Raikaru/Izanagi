# Golden native messages and fonts (M8b)

## Sources and licence

This module is original project code under the project licence. No third-party
message/font implementation, encoding table, font software or game payload is
included. No new dependency, configure flag or packaging obligation is added.
M8a's separately reviewed texture dependency is unchanged.

Format facts: [Amicitia BMD](https://amicitia.miraheze.org/wiki/BMD),
[Amicitia FNT](https://amicitia.miraheze.org/wiki/FNT), committed M3b framing at
`ba7d6de20:docs/golden_mode/specs/message_tables.md`, and the PS2 decomp's
`frFontSlotLoad`, `frFontCreateGlyphNode`, `fmGslContSetupSubBlocks` and
`fmGslContDecodeRun`. Implementation reads only these facts and measurements of
the owner's Steam build 10836033, not Golden executable code or RE notes.

## Message container and typed text

MSG1 has a 32-byte little-endian header: type 7 at byte 0, declared file size
at +4, magic at +8, relocation offset/size at +16/+20, dialog count at +24,
relocated byte at +28, version at +30. On disk pointers are relative to +32.
The directory contains `(kind, pointer)` pairs. A message (kind 0) has a
24-byte label, u16 page count, u16 speaker, page pointers, u32 buffer length,
then text. A selection (kind 1) has a 24-byte label, u16 reserved, u16 choice
count, u32 pattern, choice pointers, u32 buffer length and text. Labels and
pages may alias; directory order does not imply monotonically increasing data.

The speaker header after the dialog directory has a relative pointer, count
and two reserved words. Its table contains relative pointers to encoded names.
Page views stop at the next greater page pointer (or buffer end), including
out-of-order aliases. Measured multi-page dialogs end non-final pages with the
two-byte control `F1 21`, **not NUL**; only the whole buffer is NUL-terminated.
The iterator reports the bounded page end separately from its retained control
token, so rendering one page never spills into the following page.
Zero-page dialogs are exactly their fixed header: they have no page table,
buffer-length word or text. This is measured in all 90 title lesson banks;
reading the following dialog's label as a length is incorrect.

Tokens distinguish end, newline, glyph and control. A control begins with
0xF1..0xFF: its low nibble is its total size in 16-bit words, the following
byte is the group/index selector, and remaining words are preserved arguments.
An argument containing zero or a command-looking byte is never scanned as text.
English printable ASCII retains its scalar. Extended pairs retain their
encoded value and glyph index; they are not guessed as Shift-JIS or UTF-8.
Runtime substitutions, timers, choices and other control execution belong to
M9, not this format module. Unsupported presentation must refuse explicitly,
never silently drop dynamic text or invent a replacement.

## Font container

The measured English font is FNT, not DDS. Its header describes glyph count
(+14), cell width/height (+16/+18), packed bytes per glyph (+20), palette block
count (+10), metrics flag (+22), and data offset (+0). Each palette is 64 bytes.
Optional length-prefixed metrics and auxiliary chunks precede a four-byte per
glyph reserved map. Metrics are signed left/right edge pairs, not advances.

The compressed section has a 32-byte header, a six-byte-record binary tree,
a u32 bit-offset table with a final sentinel, and an LSB-first u16 bitstream.
Tree record 0 is the root; each branch selects another record; a zero first
branch marks a leaf whose second branch's low byte is emitted. Every glyph
must end at the root and emit exactly width*height/2 bytes. Packed pixels use
low nibble first. Palette bytes supply RGBA with the retained PS2 alpha
convention (0x80 is opaque), not a replacement system font.
The native glyph code is an index encoding, not Unicode: printable ASCII
selects `code-0x20`; multibyte codes select successive 128-cell pages.
English space selects index zero's metrics and advances without ink. The PS2
RAW renderer's special index 0x1F6 is **not** a Golden space: the viewed
witness exposed a CJK glyph there. The Golden path never uses that mapping.

## Isolation and qualification

The parsers are CPU-only borrowed views, with no mode, filesystem, guest writes,
registration or global cache. Rendering is a separate Golden-mode-only RGBA
seam: PS2 mode returns before touching input or output. It consumes the parsed
font's own glyphs and metrics. Existing PS2 RAW/message validation and renderer
paths are unchanged. No host pointer enters a guest word. R8 and consumer P8
remain refused pending M9.

The static witness selects pages containing only the default-style envelope
`F2 05 FF FF`, `F1 41`, and the completed-page fence `F1 21`. Its callback
explicitly accepts these; every other presentation control refuses. This is a
static font/page witness, not a timer, colour, substitution or M9 UI-runtime
implementation. The native CLI also exposes M7c's unchanged bounded CRILAYLA
codec on a private pipe, keeping the full census practical without temporary
decoder libraries or weakening a single inventory hash.

Synthetic tests cover framing, aliases, selections, malformed spans, control
arguments, glyph indices, tree traversal and rendering isolation. Local Steam
qualification checks inventory hashes and writes only aggregate metadata and
owner-local images. Counts, viewed witness hashes and exact-SHA proof receipts
are recorded in the milestone report outside the repository.

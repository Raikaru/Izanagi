# Skill help: observed labels and bank use

Scope: the English Steam build 10836033. Addresses below are PE RVAs, not
file offsets. This is a **partial specification**: it does not establish the
skill-ID-to-dialog mapping for every call site, nor the treatment of skills
without a corresponding dialog. It does not authorize dropping M5 refusals.

## Evidence

The message-bank filename pointer at `0x90E6F0` points to the skill-help BMD
filename at `0x92DDB0`. It occupies entry 6 of the filename pointer list at
`0x90E6C0`. The loader at `0x248740` iterates 13 names, finds a matching
archive member by name, and places its loaded handle in the corresponding
slot; the array begins at `0xB09490`, making the skill-help slot `0xB094A8`
(`0x248769`–`0x248800`).

The consumers at `0x191431`–`0x1914C3` and `0x191E30`–`0x191ECC` load
that slot, extract a 16-bit value from a selected UI record, compare it to the
message bank's dialog count, and pass it as the dialog index. The same record
field is passed through the skill-list renderer at `0x428800`, which calls the
skill-name accessor `0xD59B0` at `0x4289A4`–`0x4289BB`. In the English
layout, that accessor indexes the 23-byte skill-name table at `0x1E4390C8`;
the MSG.TBL descriptor at `0xB04390` identifies it as block 1. Thus these UI
paths use the same skill ID for name and help dialog, rather than applying a
passive-skill offset or looking up the label. This does not prove every
other caller uses the same path.

The observed bank has 608 dialogs, indexed 0 through 607. These consumers
branch away when the dialog count is strictly less than the requested ID,
so IDs 609–623 are skipped there. They pass ID 608 onward because equality
is not excluded. Both downstream paths perform an out-of-range record access,
as described below; its outcome is not determined. Do not invent descriptions.

In the observed BMD, all 608 directory records are message dialogs with
nonempty, distinct 24-byte name fields. Every record at directory index `i`
has the ASCII label `skill_` followed by `i` in exactly three uppercase
hexadecimal digits, for `0 <= i < 608`; the last is `skill_25F`. There are
624 skill-name rows, so no dialog label exists for the final 16 numeric values
`0x260`–`0x26F`. Label names are observations, not evidence that the runtime
looks up labels or what it displays for those skills. Do not synthesize blank
or borrowed descriptions.

## ID 608 boundary path — CORROBORATED

Switch `0x71001748E4`–`0x7100174900` loads `datSkillHelp.bmd` into
bank slot 6, matching Steam `0x248740` (**CORROBORATED** for bank identity).
Switch `0x710018B2C0` compares the requested dialog index with the bank's
count and skips only when the count is *less* than the index. Equality
therefore passes, matching the Steam outer-check observation.

The Switch downstream path `0x71002DB330` → `0x71002DB1C0` does not
perform another dialog-count check before indexing the bank's array of
16-byte dialog records. It reads a pointer at array base `+0x38 + 16*index`,
then checks that pointer and a line-count field. Thus index 608 is not a
valid in-bounds dialog for a 608-entry array, but the eventual read value
and outcome depend on adjacent memory and assertion handling. Do not infer
that it renders blank, crashes, or safely fails.

**CORROBORATED:** Steam `0x1914C3` calls `0x45E5B0`, which calls
`0x45E3E0`. At `0x45E41E`–`0x45E43C`, the latter reads a pointer from
the same `bank + 0x38 + 16*index` record location without a count or
null check, and then reads the signed halfword at pointer `+0x18`. A
positive count causes it to take the pointer at `+0x20`; otherwise it
passes null onward. At equality this is an out-of-bounds record read, not
a proven blank or failure. Unlike Switch, Steam dereferences the retrieved
pointer without a preceding null check in this path. P7's boundary mechanism
is specified, but reproducing an arbitrary adjacent-memory result is unsafe.

| Synthetic count | Requested index | Outer Switch check |
| ---: | ---: | --- |
| 608 | 607 | passes |
| 608 | 608 | passes; downstream index is outside 608 entries |
| 608 | 609 | skips |

## Observed skill-help controls

**Steam 10836033 data observation:** In the English member
`data_e.cpk::init.bin::init/datMsg.pak::datSkillHelp.bmd`, all 608 dialogs
have one page. A bounded token census found exactly two function controls
in each page, in this order: `F2 05 FF FF`, then `F1 41`. It found
1,032 newline bytes `0A` across the pages. This is a census of the pinned
English Steam bank, not a claim about other message banks, languages or
versions. The 15,952 non-control glyph tokens are all single-byte printable
ASCII (`0x20`–`0x7E`); no two-byte glyph tokens occur. No dialog text is
reproduced here.

**SWITCH-ONLY handler behavior:** The Switch interpreter dispatch at
`0x71002D42C0`–`0x71002D4348` treats the low nibble of the first function
byte as one plus the number of two-byte argument pairs, and splits the
second byte into a three-bit table index and five-bit function index.
For `F2 05 FF FF`, this is table 0, function 5, one argument pair; its
handler `0x71002C9640` returns zero without side effects. For `F1 41`,
this is table 2, function 1, no arguments; handler `0x71002C9C20`
returns zero and, when global mode `0x7104A82878` is zero, sets interpreter
byte `+0x0F` to `0x28`, halfword `+0x26` to `0x20`, and global
`0x7104A82868` to zero. Otherwise it makes no such writes. These numeric
effects are observed, not a complete text-rendering or glyph specification.
The independently published
[TGE message metadata](https://github.com/tge-was-taken/Atlus-Script-Tools/blob/master/Source/AtlusScriptLibrary/Libraries/Persona4Golden/MessageScriptLibrary.json)
labels table 2/function 1 as a line start and table 0/function 5 unused;
the metadata is not proof of Steam's handler internals.

| Synthetic bytes | Function classification | Switch observation |
| --- | --- | --- |
| `F2 05 FF FF` | table 0, function 5, one argument pair | returns 0, no side effects |
| `F1 41` | table 2, function 1, no arguments | mode-dependent writes above, returns 0 |
| `0A` | newline token | when interpreter global flag bit 2 is set, add global line spacing times 8 to y and set active and pending-position bytes to 1; otherwise do not move y |

## Text-to-glyph path

**SWITCH-ONLY:** The message interpreter `0x71002D4200` stops at byte
zero. It interprets bytes `0xF0`–`0xFF` as function controls before
attempting glyph decoding. For a non-control byte at least `0x80`, it
consumes a second byte and treats the pair as a big-endian 16-bit value.
Values below `0xC080` pass to the glyph creator unchanged in the current
font style. Values at or above `0xC080` are reduced by `0x4000` and use
style `0x10`. A printable non-space ASCII byte is passed through unchanged.
Space is replaced with a language-dependent two-byte font code selected from
the table referenced at `0x7100881028`; do not assume one universal code.
Newline and controls are not glyphs. This is the interpreter's representation,
not Unicode, UTF-8, or Shift-JIS.

**SWITCH-ONLY:** Glyph creator `0x71002CE300` turns each resulting single
byte below `0x80` into font index `byte - 0x20`, except space, for which it
uses the language-selected code. For a two-byte value `v`, let
`d = (v - 0x8080) mod 2^32`; the font index is
`((d >> 1) & 0x7F80) | (d & 0x7F)`. If the index is at least the selected
font's unsigned 16-bit glyph count, it substitutes index `0x147`. It passes
the index to the cache/raster lookup `0x71002D3A80` and metric setup
`0x71002CDFA0`. Style, language, font count, glyph metrics and font assets
remain relevant; the index alone does not describe a rendered image.

The PS2 interpreter `frFont.c` has the same printable-ASCII pass-through,
two-byte threshold and `0x4000` adjustment, but its adjusted-pair style is
8 rather than Switch's 16, and its space code is `83 F6`. PS2 glyph-node
creation `0x00272170` maps non-space ASCII to `byte - 0x20`, space to
`0x1F6`, and other pairs by the same index arithmetic and `0x147` fallback.
For this pinned English bank, printable bytes and `0A` need no transcoding
for the PS2 message parser. Controls and layout still require handling;
this does not prove identical artwork or metrics for the two font assets.
Steam text creation at `0x453880` immediately transfers outside ordinary
`.shared` code, so its parser and glyph internals remain **OPEN**; do not
claim Steam/Switch renderer equivalence from the data census.

| Synthetic input | Interpreter output or action | Font index |
| --- | --- | ---: |
| `41 00` | glyph `41`, end | `0x21` |
| `80 80 00` | glyph `80 80`, current style | `0` |
| `C0 80 00` | glyph `80 80`, Switch style `0x10` | `0` |
| `F2 05 FF FF 41 00` | consume one argument pair; glyph `41` | `0x21` |
| `F1 41 0A 00` | line-start control, conditional newline, end | none |

The vectors assume the selected font has enough glyphs. Malformed or truncated
controls and pairs are not covered by this valid-input behavior; adapters should
validate their source bounds rather than emulate out-of-bounds reads.

## Synthetic vectors

These exercise only the observed label convention, not message rendering:

| Directory index | Synthetic label bytes before NUL | Convention result |
| --- | --- | --- |
| 0 | `skill_000` | matches |
| 10 | `skill_00A` | matches; hexadecimal, not decimal |
| 607 | `skill_25F` | matches |
| 608 | `skill_260` | convention matches, but outside the observed 608-record bank |
| 10 | `skill_010` | does not match index 10 |
| 10 | `skill_00a` | does not match observed uppercase convention |

For a synthetic selected-record ID of 607, those UI paths use name row 607
and help dialog 607. For ID 609, the outer help comparison skips rendering.
For ID 608, the outer comparison permits the call but no final rendering
result is specified.

A complete general adapter still needs other caller rules, Steam renderer
internals, font-asset equivalence and resource lifetime. The pinned English
bank's glyph repertoire and both platforms' ID-608 access are specified above;
these observations alone do not lift the boot refusal.

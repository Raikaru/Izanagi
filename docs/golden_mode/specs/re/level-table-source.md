# Non-enemy base HP/SP level table: Steam source

**Status: SPECIFIED** for Steam build10836033 and the pinned Switch patch131072
from [ai-commands.md](ai-commands.md). This is a read-only source/layout
specification, not a claim that the production cache builder has already
registered this source. No game, emulator, Steam executable execution,
protector decoding or executable patching was used.

## Corrected Switch address

The earlier AI specification's `0x710075ADCC` is a transcription error.
The correct base is **`0x710075ACCC`**, exactly `0x100` bytes earlier.
The relative relocation for GOT slot `0x7100880980` resolves to that base.
The old address lies inside the actual table and must not be accepted as
its beginning or hashed as a complete table.

Switch HP accessor `0x71001125C0`, especially `0x71001125EC..0x7100112608`,
loads that GOT slot, masks level to eight bits, multiplies by `0x2C`, adds
the sign-extended16 character ID multiplied by4, and loads an unsigned16
halfword at displacement `-0x2C`. SP accessor `0x7100112640`, especially
`0x710011266C..0x7100112688`, uses the same calculation and displacement
`-0x2A`. Hence the two addresses are:

| Value | Address |
| --- | --- |
| Base HP | `0x710075ACCC + 44*((level & 255)-1) + 4*signed16(id)` |
| Base SP | The HP address plus2 |

The lookup level comes from `0x7100071690`: unit byte6 for enemies or
character ID1; otherwise active-persona byte4 through `0x710007B9F0`.
Level zero takes the existing assertion path. These are base values before
membership/equipment bonuses and the separate maximum-resource clamps.

## Steam install locator and measured identity

The approved source is plaintext **`P4G.exe` data**, not an archive member.
No archive extraction is required. The observed install-relative path is
`P4G.exe`, under the registered Steam app1113000 install (on the measuring
machine, `<steam-install>`).
Do not use another executable, the forbidden repack or a running process.

| Identity / location | Measured value |
| --- | --- |
| Steam build | `10836033` |
| Executable length | `436027328` bytes |
| Executable SHA-256 | `0f327c7669e6cac72aaf355181b1e51edd12fee4d71709fc1802dbf0da70546e` |
| File-backed section | `.sdata`, flags `0x40000040` (initialized data, readable, not executable) |
| Section RVA | `0x887000` |
| Offset within section | `0x85AE0` |
| Table RVA | **`0x90CAE0`** |
| Table raw file offset | **`0x90B6E0`** |
| Payload length | **`4356` bytes (`0x1104`)** |
| Payload SHA-256 | **`88795bd395dc2bc08bed4ac39607d1359de8a8091d0f9234367108e838eff677`** |
| Exclusive RVA end | `0x90DBE4` |
| Exclusive raw-file end | `0x90C7E4` |

Offsets are reproducibility citations, **not** permission to skip the Gate B
structural locator or hash allowlist. This measured executable/table identity
pair is the build10836033 allowlist entry for an implementer to register.
Refuse unknown executable identities, absent/non-file-backed candidates,
executable sections, ambiguous candidates and wrong payload hashes; do not
fall back to PS2 bytes or a zero-filled table on a source failure.

## Shape and structural location

The payload contains **99 rows**, one for each level1 through99. Each row
is **44 bytes**: eleven consecutive four-byte pairs, indexed by character ID
0 through10. Each pair is little-endian `u16 baseHP`, then `u16 baseSP`.
There is no header, row count, byte swapping, pointer relocation or alignment
padding inside the payload. The first row is level1, not level0.

For the pinned data, a structural locator can scan even-aligned candidates
in named, non-executable file-backed data sections and require all of:

* Exactly99 complete rows of44 bytes remain inside that section's raw and
  virtual file-backed extent.
* Each row's ID0 pair is the same `(110,100)` sentinel pair.
* Each row's ID9 and ID10 pairs are both zero.
* The HP and SP columns for IDs1 through8 are positive, at most999,
  nondecreasing with level, and each has a higher last value than first.
* The complete4356-byte candidate passes the measured SHA-256 allowlist.

These are measured source-shape conditions, not an invitation to synthesize
missing game rows or an engine rule accepting arbitrary ID0/9/10 actors.
The underlying accessors perform signed-ID arithmetic, not a clamped array
index. This source specifies the valid99-level, eleven-pair payload; it
does not define new behavior for malformed unit IDs or out-of-domain levels.
The row immediately preceding and following the Switch payload do not
satisfy this table signature; do not prepend a level0 row or append level100.

The read-only measurement scanned `.sdata`, `.xpdata` and `.rdata` and found
one exact4356-byte occurrence, in `.sdata`. The independent structural scan
also found exactly one candidate, at that same address; those sections are
read as data only. Neither `.shared` nor `.arch` is needed for this locator.

## Cross-platform corroboration and PS2 comparison

| Source | Comparable level1 payload | Comparison |
| --- | --- | --- |
| Switch | `0x710075ACCC`, length`0x1104` | Same SHA-256 as the Steam payload |
| Steam | `.sdata` RVA`0x90CAE0`, length`0x1104` | All4356 bytes equal Switch |
| PS2 | `D_005DD844 + 44`, i.e. `0x005DD870`, length`0x1104` | All4356 bytes equal both Golden versions:2178/2178 halfwords,1089/1089 pairs |

Thus **there is no base HP/SP value difference from PS2 for levels1..99
and these eleven slots**. The indexing origins differ. PS2
`datGetLevelTableFirstValue` (`0x001053B0`) indexes `D_005DD844` by
`44*(level & 255) + 4*id`; second-value accessor (`0x00105460`) uses
`D_005DD846` with the same index. See `src/g_data/g_data.c` in the PS2 decomp.
Switch's relocated base instead already names the first actual level1 row,
so it subtracts44. Comparing Golden's first row against PS2 `D_005DD844`
itself would compare against preceding non-table bytes and produce a false
balance difference. No copied prefix, leading fake row, Golden scaling or
rounding correction belongs in the conversion.

This equality concerns the base table only. It does not establish equality
of final HP/SP calculations, active-persona level selection, skill effects,
equipment bonuses or Golden-only skill IDs; those readers remain governed
by [ai-commands.md](ai-commands.md).

## Synthetic implementer vectors

The following values are fabricated, not retail table entries. They exercise
a layout reader independently of production source/hash admission.

| Fabricated input / operation | Expected result |
| --- | --- |
| Payload base `T`; level1, ID1 | HP offset`4`, SP offset`6` |
| Level12, ID3; place HP`0x0123`, SP`0x0456` in that pair | HP offset`0x1F0`, SP offset`0x1F2`; read unsigned291 and1110, without a999 clamp in the base-table reader |
| Level99, ID10 | HP offset`0x1100`, SP offset`0x1102`; both halfwords remain within the4356-byte payload |
| Compare a synthetic PS2 view with one44-byte prefix to its Golden view without that prefix | PS2 base`P + 44*L + 4*id` and Golden base`(P+44) + 44*(L-1) + 4*id` select the same HP pair |
| Candidate has correct structure but one changed byte | Fail payload-hash admission |
| Correct payload lies only in a non-file-backed virtual tail or an executable section | Refuse source, even if a supplied synthetic hash matches |
| Two source-shape candidates are found | Refuse ambiguous source; a fixed recorded RVA must not silently choose one |
| Executable hash is not the pinned identity | Refuse before payload admission |

## Evidence and verification boundary

External raw measurement scripts and metadata-only receipts are under
`<scratch>/GoldenRE-notes/switch/`:
`level_table_source.py`, `level-table-source.json`,
`level_table_bounds.py`, `level_table_shape.py`, `level-table-shape.json`.
The original accessor decompilation notes are `71001125c0.txt` and
`7100112640.txt`; address arithmetic was independently read from their
AArch64 instructions. The bounds script resolves the GOT relocation from
the NSO's relative-relocation records, rather than trusting the old prose
address. Only metadata, address formulas, hashes and synthetic vectors are
published here; retail payloads and game instruction listings are not.

The measurement commands run were the local Python3.12 interpreter on
`level_table_source.py`, `level_table_bounds.py` and `level_table_shape.py`,
plus `ai_inspect.py 1125c0:80 112640:80`. These are static data/instruction
measurements, not production cache qualification, unit tests, a build,
verify/objdiff, gameplay, an emulator or CI.

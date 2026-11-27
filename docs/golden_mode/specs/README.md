# Golden mode format specs

Golden mode builds its content cache at run time from the player's own Persona 4 Golden (Steam) install
(the plan is in the project's Golden mode plan; milestone 3 is this directory). Converters are written
from these specs alone: a spec states the container, every record layout a converter needs, ID domains,
invariants, refusal rules and the PS2 differences, and carries synthetic test vectors. No spec contains
game data: layouts, row counts, sizes and value-range buckets only.

| Spec | Covers | Status |
|---|---|---|
| [containers.md](containers.md) | CPK/@UTF/CRILAYLA, Atlus PAK v1/v2, FTD/CTD, cmm DAT, packed-bank TBL, ITEMTBL sections | complete for wave 1 |
| [battle_tables.md](battle_tables.md) | `init_free.bin` `battle/*.TBL` (8 tables) and `init/itemtbl.bin` | row layouts for every block a converter reads; AICALC 9/10 and MSG 4 opaque |
| [fcl_tables.md](fcl_tables.md) | `init_free.bin` `init/fclTable.bin` (105 FTD tables) | fusion and inheritance tables specified field by field; the rest catalogued |
| [cmp_tables.md](cmp_tables.md) | `init_free.bin` `init/cmpTable.bin` (37 CTD tables, 1 BMD) | catalogued |
| [cmm_tables.md](cmm_tables.md) | `init.bin` `event/cmm.bin` (35 DAT tables, 1 BMD) | rank-up skill table specified; the rest catalogued |

Message files (BMD) inside these packs and asset families (textures, models, audio, movies) are not
covered by these specifications. Native Golden asset formats require further work.

## Rules

- **Sources.** Measurements of the player's own install with `tools/golden_mode/` (metadata only), the
  port's and decomp's own sources (PS2 layouts), and public documentation, cited. Never Golden's executable
  code. The one Golden executable table Golden mode reads (the protagonist EXP curve) is specified in the
  Golden mode P4G.exe study, not here.
- **Catalogue vs specification.** A *catalogued* table has its container, rows, row size and an
  *inferred* field split measured by `tools/golden_mode/layouts.py`. A *specified* table has fields with
  meanings and is what converters may interpret. A converter may only copy catalogued tables whole.
- **Inferred fields.** `layouts.py` reads a byte column as `u8` unless a higher byte varies, so a 16-bit
  field whose high byte is zero in this build appears as `u8` followed by `zero`. Treat `u8` + `zero` as
  "u16 or u8 + pad, not decided"; only a specified layout decides.
- **Build.** Measured on Steam build 10836033, English locale (`data_e.cpk`), which overrides the common
  `data.cpk` member of the same path. `Locale variants` counts the distinct SHA-256 of a member across
  `data.cpk` and the eight locale CPKs: 1 means locale-independent.
- **Refusal.** A converter refuses (with the table name and reason) when a container, row count or row
  size differs from its spec. It never guesses.

## Required spec sections

Every spec has `## Sources`, `## Container`, `## Records`, `## ID domains`, `## Invariants and refusals`,
`## PS2 differences`, `## Test vectors` and `## Status`. `tests/golden_mode_specs_test.py` checks them and
decodes every vector.

## Test vector format

A fenced block tagged `golden-vector`:

````
```golden-vector
name: <unique name>
container: ftd | dat | tbl | itemtbl | pak
hex: <hex bytes; whitespace ignored; XX*N repeats byte XX N times>
expect: <one-line JSON>
```
````

`expect` keys: `ftd`/`dat`: `rows`, `payload` (hex); `tbl`: `spans` (hex list); `itemtbl`: `sections`
(`[rows, row size, payload hex]` list, row sizes 68 then 24); `pak`: `variant` and `entries`
(`[name, payload hex]` list). A vector whose `expect` is `null` must be refused by the container reader.

## Workstation provenance

Source receipts use the following stable placeholders, not a researcher's
workstation paths. Match roots case-insensitively, accepting slash and escaped
backslash spellings; apply the more specific installation roots first.

| Workstation root shape | Neutral root |
|---|---|
| `<drive>:/Users/<user>/Projects/Persona 3 Decomp/source/Persona4-Decompilation` | `<decomp>` |
| `<drive>:/Users/<user>/Projects/_scratch` | `<scratch>` |
| `<drive>:/Users/<user>/AppData/Local/Programs/Python/Python312` | `<python>` |
| `<drive>:/Program Files [(x86)]/Steam/steamapps/common/Persona 4 Golden` | `<steam-install>` |
| `<drive>:/Program Files [(x86)]/Git` | `<git-install>` |
| `<drive>:/p4pc-{scratch,local,test}` | `<scratch>` |
| `<drive>:/p4pc-work` | `<worktree>` |
| `<drive>:/Users/<user>`, `/home/<user>`, `/Users/<user>` | `<home>` |
| `/mnt/<drive-letter>` | `<drive>` |

Only the root and its immediately following separator are normalized; historical
suffix spelling is retained. These are provenance locations, never runtime
dependencies. Do not change source values, addresses, classes or game-image
hashes. A receipt digest that hashes a changed provenance document must be
recomputed from that document, without weakening the hash check.

Embedded game/build strings, including `D:/Projects/P4G-NSW/...` and SDK build
paths, are evidence and remain verbatim. Both L0 and L1 publication gates reject
workstation-local provenance anywhere in the complete committed spec tree,
including unchanged files; the rules live in `tools/golden_mode/re_spec_provenance.py`.


# Golden mode measurement tools (milestone 1)

Golden mode is the planned Golden-only install: Izanagi plays with Persona 4 Golden's data, built at run
time from the player's own Golden (Steam) install, with nothing derived from game content shipped. These
tools measure what that needs. They read the player's files in memory and write **metadata only** (names,
chains, sizes, SHA-256, signature labels, addresses, counts); every tool refuses an output directory
inside this repository. They never open `P4G.exe` beyond hashing it as a loose file.

| Tool | Reads | Writes |
|---|---|---|
| `inventory.py` | the Golden install: every loose file and CPK member, recursing through nested CPK, PAK v1/v2/v3 and AFS2 | `inventory.jsonl` (one row per object), `summary.json` |
| `provenance.py` | `SLUS_217.82`, a decomp linker map, the decomp and port symbol lists and sources | `provenance.jsonl` (one row per data1/data2 object), `provenance-summary.json` |
| `sources.py` | the two outputs above, `config/game_tables.json`, the PS2 disc and the Golden install | `sources.json` (table inputs, banks, image tables, disc coverage), `disc.jsonl` |

```sh
python tools/golden_mode/inventory.py --install "<Steam>/steamapps/common/Persona 4 Golden" --out <dir>/inventory --jobs 8
python tools/golden_mode/provenance.py --elf <SLUS_217.82> --map <decomp build>/slus21782.map \
    --decomp <decomp checkout> --out <dir>/provenance
python tools/golden_mode/sources.py --inventory <dir>/inventory --provenance <dir>/provenance \
    --install "<Golden folder>" --iso <PS2 disc image> --out <dir>/sources
```

`formats.py` holds the container readers (CPK with masked @UTF, CRILAYLA, PAK families, signatures);
they mirror `src/platform/golden/golden_formats.c`. `image_table_candidates.json` is the reviewed list
of Golden name globs considered for each image-resident PS2 table; a table without a matching object
is reported missing. Provenance classes and their meaning are in `provenance.py`'s docstring.

Limits: the provenance map takes object boundaries from evidence (map, symbols, code-formed addresses,
data pointers, catalogue) and runs each object to the next boundary, so `mixed-unknown` objects can
span several real objects. Its map input is whatever build produced it; give it a current decomp
build's map for current C-ownership. Name matches in `sources.py` are candidates, not proof of
equivalent layout or content.

Tests: `golden_mode_formats`, `golden_mode_inventory`, `golden_mode_provenance` and
`golden_mode_sources` (`tests/cmake/255-golden-mode-tools.cmake`) run on synthetic fixtures from
`tests/golden_mode_fixtures.py`; no Golden or retail bytes.

## Image typing (milestone 2)

`typing_pass.py` types every object of milestone 1's provenance map (element kind, width, stride, rows,
and the evidence for each: decomp declarations, the retail code's access widths, measured content) and
assigns the supplier a Golden-mode image builder would use: zero, decomp-c, symbols, golden-exe,
golden-archive, golden-convert or decomp-literal (GATE C), with `golden_check` on decomp literals that
game-logic code reads or that have a Golden variant. Golden leads come from an 8-byte window index matched
against the Steam `P4G.exe`'s file-backed data sections (read as plain file bytes; nothing is decrypted,
unpacked or executed) and, optionally, small Golden archive objects:

```sh
python tools/golden_mode/typing_pass.py --provenance <dir>/provenance --elf <SLUS_217.82> \
    --decomp <decomp checkout> --golden-exe "<Golden folder>/P4G.exe" \
    --inventory <dir>/inventory --install "<Golden folder>" --out <dir>/typed
```

It writes `typed.jsonl` (one row per object) and `typed-summary.json`. Kinds from content alone are
heuristics: a `struct` has a size and no element evidence, and a stride from the nonzero-byte mask can be
a multiple of the real row size. Test: `golden_mode_typing` (`tests/cmake/255-golden-mode-typing.cmake`).
## Format specs (milestone 3)

`layouts.py` measures the record layouts of Golden's table families (fclTable.bin FTD, cmpTable.bin
CTD, cmm.bin DAT, the battle TBLs and ITEMTBL) and renders the catalogue tables of the format specs in
[docs/golden_mode/specs](../../docs/golden_mode/specs/README.md):

```sh
python tools/golden_mode/layouts.py --inventory <dir>/inventory --install "<Golden folder>" --out <dir>/layouts
python tools/golden_mode/layouts.py --render <dir>/layouts/layouts.json
```

Tests: `golden_mode_layouts` (synthetic tables) and `golden_mode_specs` (every spec's sections and
synthetic vectors), in `tests/cmake/255-golden-mode-specs.cmake`.

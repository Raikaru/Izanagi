# Golden mode measurement tools (milestone 1)

Golden mode is an in-development alternative that derives data at run time from
the player's own Persona 4 Golden Steam installation; it is not yet playable.
These tools inspect local files and write metadata only (such as names, sizes,
hashes, signatures, addresses and counts). They refuse output directories
inside this repository. Some inspect executable data as file bytes; they do not
execute or unpack it.

| Tool | Reads | Writes |
|---|---|---|
| `inventory.py` | the Golden install: every loose file and CPK member, recursing through nested CPK, PAK v1/v2/v3 and AFS2 | `inventory.jsonl` (one row per object), `summary.json` |
| `provenance.py` | `SLUS_217.82`, a decomp linker map, the decomp and port symbol lists and sources | `provenance.jsonl` (one row per data1/data2 object), `provenance-summary.json`, `provenance-surviving-aliases.jsonl` |
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

`formed_addresses(..., surviving_aliases=records)` preserves the historical
reference triples and separately reports data addresses and loaded-value source
addresses live at JAL/JALR or JR-RA boundaries, after their delay slots.
`call_surviving_aliases(read, gp)` exposes the same candidate stream directly.
Calls do not erase those candidates merely because the ABI allows a register
clobber; an omitted collector warns when review is required. The CLI always
writes the separate metadata-only `provenance-surviving-aliases.jsonl`.
This linear scan follows address copies/indexed bases and has no lifetime cutoff,
but is not a CFG, callee-effect, spill/alias or caller-continuation proof.
A no-op/dead-load/dead-store admission still needs complete path and target-set
closure; a value live at return requires review of every caller continuation.

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

## Image-literal ledger (milestone 6b)

`literal_inventory.py --typed <M2>/typed.jsonl --out <inventory.json>` freezes
exactly the M2 `golden_check == true` set, retaining metadata only. The committed
`l1-inventory.json` contains 1,043 objects / 249,428 bytes: data1 786 / 241,555
bytes and data2 257 / 7,873 bytes. Inside this set there are 585 executable
variant leads, 123 archive variant leads and 335 objects without a variant lead
(8 partial leads and 327 with no lead), totaling 8,960 bytes. M2's headline
586 + 128 variant count includes six objects outside `golden_check`; it is not
the L1 denominator. A heuristic lead is never binding authority.

`literal_ledger.py --inventory <inventory.json> --decisions <spec.jsonl>`
checks exact address/size coverage, one reasoned decision per object and
source/layout metadata for bound objects. `--registry-out <rows.inc>` emits
same-shape copy rows for the existing image binder; unsupported conversions
refuse rather than truncating or guessing. Missing, duplicate, unknown and
unclassified objects refuse.

Main's revised M6b acceptance permits the separate `OPEN-retain` class with a
per-object proof path. It is **not** D10c or proof of Golden balance: the PS2
value remains, must be named as `L1-open` at boot and must be resolved before
M12. The validator reports all six runtime classes separately. `golden_mode_literal_ledger`
tests synthetic decision/refusal cases and the complete immutable M2 inventory.

The M6c-1 intake is frozen at specification commit
`c8987ecc75e843c875bd1d5c3db19bc4c7d23ba7`, recorded in `l1-cut.json`.
The source author's separate provenance-only sanitation commit
`2dd82cb48cc7081971cac284dac450ff8c8e19c9` neutralizes private local paths and
reseals affected proof references without admitting later rows or changing values.
The canonical primary plus address-keyed archive supplement cover all 1,043
objects. Runtime classification is 26 bound, 585 pending-seam, 219 D10c
non-supplies and 213 OPEN-retain. A source admission alone never creates a
runtime supplier. Missing implementations retain their exact required-seam
contracts and named `pending:<owner>` blockers; D10c emits no runtime object.
Regenerate authenticated metadata and the address-complete blocker census with:

```sh
python tools/golden_mode/literal_ledger.py \
  --inventory tools/golden_mode/l1-inventory.json \
  --decisions docs/golden_mode/specs/re/l1-image-literals.jsonl \
  --supplement docs/golden_mode/specs/re/l1-image-literals-archive.jsonl \
  --bindings tools/golden_mode/l1-bindings.json \
  --registry-out src/platform/golden/golden_mode_literal_rows.inc \
  --pending-out tools/golden_mode/l1-pending.json \
  --manifest-c-out src/platform/golden/golden_mode_literal_manifest.inc \
  --identity-out src/platform/golden/golden_mode_literal_identity.h
```

The generated manifest carries every reason, provenance record, candidate and
proof path as metadata. The identity header pins its digest in the cache key.
Completeness tests compare both generated files against the committed specs.

The PE reader's `p4_golden_exp_image_read` reads EXP and explicitly specified
`.sdata`/`.xpdata` spans in one allowlisted executable-hash pass. Each span has
its own digest and must fit both raw and virtual section extents; executable
sections, virtual-only tails, overlapping output ranges and wrong hashes refuse.
The original EXP-only entry point remains unchanged for its existing callers.

M6c-1 adds the `spec-constant` path: binding records identify a public committed
JSON proof, its canonical LF SHA-256 and exact JSON-pointer evidence fields.
Only explicitly code-authored values become little-endian constant suppliers;
no executable instructions, NSO or private research files are runtime inputs.
The shared positive path epsilon is installed at its guest address, so both
actual direct and GP-relative readers see it. The type-3 event filter instead
uses seven fixed cells plus four live record-derived cells through the sealed
native constructor. It never reads or overwrites the legacy 48-byte partition
in Golden mode. The PS2 branch and record/reference/task ownership are retained.

The `spec-layout` path reads only an explicitly referenced, hash-pinned data
span, with bounded guest-prefix or native-bank destinations. Synthetic tests
exercise both policies and bad-proof refusal. No newly admitted semantic
layout bank is declared runtime-bound by M6c-1: their full voice/name/location
provider and consumer contracts remain pending for M6c-2 or the named owner.
`l1-pending.json` reports non-disjoint blocker counts and exact address sets
separately for the 585 pending supplies and the 219 non-supply obligations.
It is not permission to treat a Steam-span dependency as consumer closure.
`golden_exp` exercises both data sections using synthetic PE fixtures.

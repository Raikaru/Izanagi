# Contributing to Izanagi

Thanks for helping. Read [BUILDING.md](BUILDING.md) first: every change is built and
tested in the `p4pc-ci` container, and a pull request runs the same build and tests in
GitHub Actions (`.github/workflows/build.yml`).

## Never commit game data

No disc images, executables, extracted game files, memory cards, saves, savestates,
screenshots or captures of the game, and no signing keys. The `.gitignore` covers the
usual extensions; it is not a substitute for checking `git status` before you commit.
Test references (captures, RAM dumps) are pinned by name and SHA-256 in
`tests/refs.sha256` (`tools/ref_guard.py`); the bytes themselves stay outside the
repository.

## How the port is put together

- **The decomp is never edited from here.** It is the `decomp/` submodule. Fixes that are
  correct for both the PS2 build and the port go to
  [Persona4-Decompilation](https://github.com/Raikaru/Persona4-Decompilation); the port
  then moves its pin. No `#ifdef P4_PC` in the decomp. The port's adjustments to the
  decomp's text are `overrides/<addr>.c` files and the reviewed rewrites in
  `config/mirror_fixes.txt`. `config/mirror_contracts.json` pins every edit the mirror
  step makes; `python3 tools/gen_port.py mirror --refresh-contracts` updates it, and the
  diff is part of the review.
- **One owner per address.** Claim the address in `overrides/CLAIMS.md` before writing
  `overrides/<addr>.c`. The `// FUN_<addr>` marker binds its address to the first function
  defined after it, so `overrides/<addr>.c` defines the retail-named function first, with
  external linkage. Helpers go after it or in a header.
- **Data stays at its retail address.** If retail code references a symbol at a fixed
  address, place it there (`config/port_symbols.txt`, with the evidence). Only statics with
  no retail reference are port-owned (`config/port_owned.txt`).
- **Asm callees are in scope.** When you translate a function, translate its asm-only
  callees too. A function counts as done only when its callees are native, not trap stubs.
  Machine drafts in `drafts/` are a starting point, not a match: check them against the asm.
- **Replace at the game's own seam:** wrapper layers (H_Cdvd_*, sdkPad*, h_memcard), and
  RenderWare device/pipeline nodes for graphics. Use raw SCE calls only when no seam exists.
  GS/DMA/VIF/VU1 code is never transliterated; it is replaced at the sky2/pipeline-node seam.
- **64-bit first ([docs/port64.md](docs/port64.md)).** The 64-bit build is the default on
  every platform. New port code (`src/`, `overrides/`, `tests/`, `tools/`) does not assume
  `sizeof(void *) == 4`. Convert between guest addresses and host pointers with `P4_G2H` /
  `P4_H2G` (`p4pc/guest.h`), never store a host pointer in guest memory, and keep function
  pointers in guest memory as guest code addresses (`P4_FN_G2H` / `P4_FN_H2G`). New seams
  take and return guest memory as `u32` guest addresses and say so in their declaration.
- **PC disc contract.** File I/O runs at PC speed with one implementation, not a PS2 pacing
  model. A successful asynchronous disc/ADXF read never completes within its submission
  update: it is published at the next scheduler frame boundary and the game's own
  CRI/filesystem/task steps consume it ([disc contract](docs/platform/disc_cri.md#pc-disc-contract)).
- **Code that reproduces the retail game is not MIT-licensed.** New files in `overrides/`,
  `drafts/` or `src/cri/`, and any other translation of retail code, fall under the
  `LicenseRef-reconstructed` carve-out in [NOTICE](NOTICE). Mark new translations outside
  those paths with `/* SPDX-License-Identifier: LicenseRef-reconstructed */` and list them
  in NOTICE and `REUSE.toml`.

## Tests

- **Verify the changed path.** "It builds" is not "it works". Run the program or a ctest
  that exercises your change, and look at any image you produce.
- **Host tests get a 64-bit twin automatically.** A test executable added under `tests/`
  is also built for x86_64 as `<name>64` ([docs/port64.md](docs/port64.md) 6.8). Write it
  guest-safe (`tests/guest_test.h`: `p4t_guest_map`, `p4t_galloc`, `P4_G2H`/`P4_H2G`/`P4_FN_H2G`):
  no raw retail address cast to a pointer, no host pointer stored in a guest word.
- **Tests that need the game skip without it** (exit code 77, `SKIP_RETURN_CODE 77`) when
  `P4PC_ISO` / `P4PC_ELF` are unset or a reference is missing. CI has no game data, so every
  other test must pass there.
- **Automated input is script-only.** Gates and other automated runs set
  `P4PC_INPUT=script`, so they never read the user's gamepad or keyboard.
- **Tests run with mods and engine packs disabled** (`P4PC_MODS=0`, `P4PC_PACKS=0`); only the
  explicitly separate mod tests enable them ([docs/modding/](docs/modding/)).
- `tools/check_doc_links.py` (the `doc_links` test) checks the local links in `docs/`,
  `README.md` and this file.

## Pull requests

Keep a pull request to one change, describe what you verified and how, and make sure the
`build` workflow passes. By contributing you agree that your original code is released
under the [MIT licence](LICENSE).

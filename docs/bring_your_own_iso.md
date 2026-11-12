# Bring your own ISO

Izanagi contains no game data and never downloads any. Everything the game
shows and plays (its code, models, textures, music, voices, movies and text) is read
from a disc image you make from your own copy of the game.

## Which disc

**Shin Megami Tensei: Persona 4**, PlayStation 2, **USA (NTSC-U), `SLUS-21782`**.
Other regions (Japan, Europe) are different executables and are not supported.

## 1. Make an ISO from your disc

Copy the whole DVD to a single `.iso` file with any tool that reads data DVDs
sector by sector, for example:

- **Windows:** ImgBurn → *Create image file from disc*, output format ISO.
- **Linux:** `dd if=/dev/sr0 of=persona4.iso bs=2048 status=progress`
  (replace `/dev/sr0` with your drive).
- **macOS:** `dd` on the raw disk device (`diskutil list` names it), after
  `diskutil unmountDisk`.

Only plain `.iso` images (2048-byte sectors) are read. Convert `.bin/.cue`, `.chd`,
`.cso` or `.mdf` images to `.iso` first.

## 2. Check it (optional)

The image must be an ISO 9660 disc with `SLUS_217.82` in its root. The port is
built against this retail executable:

| File | Size | SHA-256 |
|---|---|---|
| `SLUS_217.82` | 8,621,084 bytes | `f90675b8a38138ddb5dc5cdf641550f41800fcb86bb499c586073e964fb41836` |

To check it, extract the one file and hash it:

```sh
# Linux / macOS (bsdtar reads ISO 9660; on macOS use `shasum -a 256`)
bsdtar -xf persona4.iso SLUS_217.82 && sha256sum SLUS_217.82
```

```bat
:: Windows (7-Zip)
7z e persona4.iso SLUS_217.82
certutil -hashfile SLUS_217.82 SHA256
```

A different hash means a different release or a damaged dump; the port expects the
retail USA executable above.

## 3. First launch

Start the player with no arguments. It looks for the disc image in this order:

1. the `P4PC_ISO` environment variable,
2. `iso` under `[game]` in `p4pc.ini`,
3. a file picker (Android: the system document picker).

A file that is not the USA disc shows an error and asks again; cancelling quits.
Once accepted, the path is saved in `p4pc.ini` and the executable is extracted once
beside it. **Keep the ISO where it is**: the port reads game files from it on every
run. To switch images, edit `[game] iso` or delete that line to be asked again.

`p4pc.ini`, the extracted executable, saves (`save/`) and logs (`p4pc.log`) live in
the per-user data folder:

| Platform | Folder |
|---|---|
| Windows | `%APPDATA%\p4pc\Persona4\` |
| Linux | `~/.local/share/p4pc/Persona4/` |
| Android | the app's private storage |

## Existing PS2 saves

Saves from a PS2 memory card or an emulator can be imported; see
[save import](platform/saves_memcard.md#importing-ps2-saves).

## For developers

Building does not need the executable: the build reads the game's function-pointer
words from `config/retail_fnptr_words.txt` (see
[BUILDING.md](../BUILDING.md#the-retail-executable-is-optional)). Pass your
extracted `SLUS_217.82` from step 2 (or the copy the player left in the data folder)
as `-DP4_RETAIL` to check that table against it and to enable the few tests that read
it; set `P4PC_ISO` and `P4PC_ELF` to run the game tests. Keep retail files, saves and
captures outside the checkout and never attach them to issues or pull requests.

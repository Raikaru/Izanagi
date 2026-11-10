#!/usr/bin/env python3
"""Build private save-container fixtures and exercise the native import surface.

--source accepts a port-made BASLUS-21782SaveN directory; data remains in the
chosen --out tree. With no source, constructs a checksum-valid synthetic image.
No game bytes are checked in. Existing tools/ps2save is used only as a writer.
"""
import argparse

import os
from pathlib import Path
import shutil
import struct
import subprocess
import sys
import tempfile

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from ps2save import build_card, pack_dir, tod
from cbs_fixture_encoder import encode_cbs


def word(b, off, value):
    struct.pack_into("<I", b, off, value)


def checksum(b):
    h = 0
    for value in b:
        h = (h + (value << (h % 24))) & 0xffffffff
    return h


def ecc(chunk):
    cp, l0, l1 = 0x77, 127, 127
    masks = (0x55, 0x33, 15, 0, 0xaa, 0xcc, 0xf0)
    for i, value in enumerate(chunk):
        for bit, mask in enumerate(masks):
            cp ^= ((value & mask).bit_count() & 1) << bit
        if value.bit_count() & 1:
            l0 ^= ~i
            l1 ^= i
    return bytes((cp, l0 & 127, l1))


def psv(folder, files):
    data = bytearray(160 + 60 * len(files))
    data[:4] = b"\0VSP"
    word(data, 56, 44)
    word(data, 60, 2)
    word(data, 100, len(files))
    data[104:120] = tod(1700000000) * 2
    word(data, 120, len(files))
    word(data, 124, 0x8427)
    data[128:128 + len(folder)] = folder.encode()
    for i, (name, blob) in enumerate(files):
        off = 160 + i * 60
        data[off:off + 16] = tod(1700000000) * 2
        word(data, off + 16, len(blob))
        word(data, off + 20, 0x8497)
        data[off + 24:off + 24 + len(name)] = name.encode()
        word(data, off + 56, len(data))
        data.extend(blob)
    return bytes(data)


def xps(folder, files):
    def header(name, size, mode):
        b = bytearray(250)
        struct.pack_into("<H", b, 0, 250)
        b[2:2 + len(name)] = name.encode()
        word(b, 66, size)
        struct.pack_into(">H", b, 78, mode)
        b[82:98] = tod(1700000000) * 2
        return b
    body = header(folder, len(files) + 2, 0x8427)
    for name, blob in files:
        body.extend(header(name, len(blob), 0x8497))
        body.extend(blob)
    return b"\x0d\0\0\0SharkPortSave" + bytes(16) + struct.pack("<I", len(body)) + body + struct.pack("<I", checksum(body))


def generate(root, source=None, max_encoder=None):
    root.mkdir(parents=True, exist_ok=True)
    folder = root / "input" / (source.name if source else "BASLUS-21782Save0")
    folder.mkdir(parents=True)
    if source:
        for p in source.iterdir():
            if p.is_file() and not p.name.startswith("."):
                shutil.copy2(p, folder / p.name)
    else:
        records = struct.pack("<II", 1, 4096) + bytes(range(256)) * 16
        summary = bytes(range(50))
        blob = struct.pack("<I", 1) + summary + struct.pack("<H", sum(summary) & 255) + records + struct.pack("<IIBI", 0x2000, 1, sum(records) & 255, 0xffffffff)
        (folder / folder.name).write_bytes(blob)
        (folder / "icon.sys").write_bytes(b"PS2D" + bytes(960))
        (folder / "icon.ico").write_bytes(bytes(range(256)) * 5)
    # PS2 name fields are fixed-width: a full 32-byte filename has no NUL.
    # Empty files also exercise the card's no-allocation-cluster encoding.
    (folder / ("X" * 32)).write_bytes(b"")
    files = sorted((p.name, p.read_bytes()) for p in folder.iterdir())
    paths = []
    for ext, blob in (("psu", pack_dir(str(folder))), ("psv", psv(folder.name, files)), ("xps", xps(folder.name, files)), ("sps", xps(folder.name, files))):
        p = root / ("fixture." + ext)
        p.write_bytes(blob)
        paths.append(p)
    card = build_card([(folder.name, files)], ecc=False)
    (root / "fixture.ps2").write_bytes(card)
    raw_ecc = bytearray()
    for off in range(0, len(card), 512):
        page = card[off:off + 512]
        raw_ecc.extend(page)
        raw_ecc.extend(b"".join(ecc(page[i:i + 128]) for i in range(0, 512, 128)) + bytes(4))
    (root / "ecc.ps2").write_bytes(raw_ecc)
    paths.extend((root / "fixture.ps2", root / "ecc.ps2", folder.parent, folder))
    indexed = root / "indexed" / folder.name
    indexed.mkdir(parents=True)
    for name, blob in files:
        (indexed / name).write_bytes(blob)
    index = '"%ROOT":\n  timeCreated: 1700000000\n  timeModified: 1700000060\n'
    for number, (name, _) in enumerate(files):
        index += f"{name}:\n  order: {number}\n  timeCreated: 1700000000\n  timeModified: 1700000060\n"
    (indexed / "_pcsx2_index").write_text(index, encoding="utf-8")
    (indexed / "_pcsx2_meta").mkdir()
    paths.append(indexed.parent)
    import json
    current = {"$ROOT": {"timeCreated": 1700000000, "timeModified": 1700000060}}
    current.update({name: {"order": number, "timeCreated": 1700000000, "timeModified": 1700000060}
                    for number, (name, _) in enumerate(files)})
    for style in ("block", "flow"):
        indexed_current = root / ("indexed-current-" + style) / folder.name
        indexed_current.mkdir(parents=True)
        for name, blob in files:
            (indexed_current / name).write_bytes(blob)
        text = index.replace("%ROOT", "$ROOT") if style == "block" else json.dumps(current)
        (indexed_current / "_pcsx2_index").write_text(text, encoding="utf-8")
        (indexed_current / "_pcsx2_index.invalid.bak").write_text("private metadata")
        paths.append(indexed_current.parent)
    encode_cbs(folder, root / "fixture.cbs")
    paths.append(root / "fixture.cbs")
    if max_encoder:
        encoder = [str(max_encoder)] if isinstance(max_encoder, (str, os.PathLike)) else list(max_encoder)
        subprocess.run(encoder + ["--encode", str(folder), str(root / "fixture.max")], check=True)
        paths.append(root / "fixture.max")
    return folder, paths


def exercise(driver, root, source=None, max_encoder=None):
    driver = [driver] if isinstance(driver, str) else driver
    if max_encoder is None:
        binary = Path(driver[-1])
        if not binary.name.startswith("save_import_test"):
            raise ValueError("--max-encoder is required with a nonstandard driver name")
        # CTest's ABI cloner rewrites the driver and inserts Wine when needed.
        # Select its same-ABI sibling, not an unbuilt 32-bit encoder target.
        encoder_name = "import_max_test" + binary.name[len("save_import_test"):]
        max_encoder = list(driver[:-1]) + [str(binary.with_name(encoder_name))]
    folder, paths = generate(root, source, max_encoder)
    original = {p.name: p.read_bytes() for p in folder.iterdir()}
    def run(src, dest, success=True, slot=None):
        cmd = list(driver) + ["--import", str(src), str(dest)] + ([str(slot)] if slot is not None else [])
        result = subprocess.run(cmd, text=True, capture_output=True)
        if (result.returncode == 0) != success:
            raise AssertionError((cmd, result.returncode, result.stdout, result.stderr))
    for index, src in enumerate(paths):
        dest = root / ("card%d" % index)
        run(src, dest)
        imported = dest / folder.name
        for name, blob in original.items():
            assert (imported / name).read_bytes() == blob, (src, name)
        assert (imported / ".p4save-meta").is_file()
        if src.name.startswith("indexed"):
            assert not any(p.name.startswith("_pcsx2_") for p in imported.iterdir())
            metadata = (imported / ".p4save-meta").read_bytes()
            timezone = 32400 if src.name == "indexed" else 0
            assert metadata[8:16] == tod(1700000000 + timezone)
            assert metadata[16:24] == tod(1700000060 + timezone)
            for offset in range(24, len(metadata), 49):
                assert metadata[offset + 33:offset + 41] == tod(1700000000 + timezone)
                assert metadata[offset + 41:offset + 49] == tod(1700000060 + timezone)
        # Second import must choose an empty slot, never replace the first.
        run(src, dest)
        assert len(list(dest.glob("BASLUS-21782Save*"))) == 2
        run(src, dest, slot=0)
        assert not list(dest.glob(".backup-*")) and not list(dest.glob(".import-*"))
        print(src.name, "roundtrip/collision/explicit replacement PASS")
    multi = root / "multi"
    for slot in range(2):
        name = "BASLUS-21782Save%d" % slot
        target = multi / name
        target.mkdir(parents=True)
        for filename, blob in original.items():
            if filename == folder.name:
                if slot:
                    changed = bytearray(blob)
                    changed[12] = (changed[12] + 1) % 100
                    changed[54] = sum(changed[4:54]) & 255
                    blob = bytes(changed)
                filename = name
            (target / filename).write_bytes(blob)
    destination = root / "collision-card"
    occupied = destination / "BASLUS-21782Save0"
    occupied.mkdir(parents=True)
    (occupied / "marker").write_bytes(b"existing data")
    run(multi, destination)
    assert (occupied / "marker").read_bytes() == b"existing data"
    assert (destination / "BASLUS-21782Save2" / "BASLUS-21782Save2").read_bytes() == original[folder.name]
    assert (destination / "BASLUS-21782Save1" / "BASLUS-21782Save1").read_bytes()[12] == (original[folder.name][12] + 1) % 100
    run(multi, root / "explicit-multi-rejected", success=False, slot=0)
    assert not (root / "explicit-multi-rejected").exists()
    badmulti = root / "bad-multi"
    shutil.copytree(multi, badmulti)
    damaged = badmulti / "BASLUS-21782Save1" / "BASLUS-21782Save1"
    data = bytearray(damaged.read_bytes())
    data[60] ^= 1
    damaged.write_bytes(data)
    run(badmulti, root / "bad-multi-rejected", success=False)
    assert not (root / "bad-multi-rejected").exists()
    file_card = root / "file-slot-card"
    file_card.mkdir()
    (file_card / "BASLUS-21782Save0").write_bytes(b"not a save directory")
    run(root / "fixture.psv", file_card, success=False, slot=0)
    assert (file_card / "BASLUS-21782Save0").read_bytes() == b"not a save directory"
    # A directory link is never a replacement target: backup cleanup must not
    # enumerate its external target. Linux CI always exercises this boundary.
    external = root / "external"
    external.mkdir()
    (external / "marker").write_bytes(b"protected")
    link_card = root / "linked-slot-card"
    link_card.mkdir()
    try:
        (link_card / "BASLUS-21782Save0").symlink_to(external, target_is_directory=True)
    except OSError:
        if os.name != "nt":
            raise
    else:
        run(root / "fixture.psv", link_card, success=False, slot=0)
        assert (external / "marker").read_bytes() == b"protected"
    # All malformed inputs leave their destination untouched.
    for kind in ("psu", "psv", "xps", "cbs"):
        good = (root / ("fixture." + kind)).read_bytes()
        bad = root / ("bad." + kind)
        bad.write_bytes(good[:-1])
        dest = root / ("reject-" + kind)
        run(bad, dest, success=False)
        assert not dest.exists()
    duplicate = psv(folder.name, sorted(original.items()) + [("ICON.SYS", b"collision")])
    (root / "case-alias.psv").write_bytes(duplicate)
    run(root / "case-alias.psv", root / "case-alias-rejected", success=False)
    numeric = psv("BASLUS-21782Save9999999999999", sorted(original.items()))
    (root / "bad-slot.psv").write_bytes(numeric)
    run(root / "bad-slot.psv", root / "bad-slot-rejected", success=False)
    oversized = bytearray((root / "fixture.psu").read_bytes())
    word(oversized, 1540, 0xffffffff)
    (root / "overflow.psu").write_bytes(oversized)
    run(root / "overflow.psu", root / "overflow-rejected", success=False)
    bad = bytearray((root / "fixture.psv").read_bytes())
    bad[128:160] = b"../BASLUS-21782Save0".ljust(32, b"\0")
    (root / "traversal.psv").write_bytes(bad)
    run(root / "traversal.psv", root / "rejected", success=False)
    bad = bytearray((root / "ecc.ps2").read_bytes())
    bad[512] ^= 1
    (root / "bad-ecc.ps2").write_bytes(bad)
    run(root / "bad-ecc.ps2", root / "rejected", success=False)
    # P4's checksum, not the PS3 signature, rejects corrupted game payload.
    bad = bytearray((root / "fixture.psv").read_bytes())
    offset = struct.unpack_from("<I", bad, 160 + 56)[0]
    bad[offset + 60] ^= 1
    (root / "bad-game.psv").write_bytes(bad)
    run(root / "bad-game.psv", root / "rejected", success=False)
    deep = root.joinpath(*(["extended-path-boundary-0123456789"] * 8))
    deep.mkdir(parents=True)
    shutil.copyfile(root / "fixture.psv", deep / "save.psv")
    run(deep / "save.psv", deep / "card")
    assert (deep / "card" / folder.name / folder.name).read_bytes() == original[folder.name]
    run(str(folder) + "/", root / "trailing-slash-card")
    # Extended-length Windows paths must not retain duplicate separators at the
    # card-root/staging boundary. Exercise fresh, collision and replacement.
    windows_driver = Path(driver[-1]).suffix.lower() == ".exe"
    for index, separator in enumerate(("/", "\\") if windows_driver else ("/",)):
        card = root / ("destination-separator-" + str(index))
        destination = str(card) + separator
        run(str(folder) + separator, destination)
        run(root / "fixture.psv", destination)
        run(root / "fixture.psv", destination, slot=0)
        assert (card / folder.name / folder.name).read_bytes() == original[folder.name]
        assert (card / "BASLUS-21782Save1" / "BASLUS-21782Save1").read_bytes() == original[folder.name]
    if windows_driver:
        result = subprocess.run(driver + ["--path-retry", str(root / "windows-path-retry")],
                                capture_output=True, text=True)
        assert result.returncode == 0, (result.returncode, result.stdout, result.stderr)
        print(result.stderr.strip())
    print("bounds, checksums, path confinement PASS")


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--source", type=Path)
    p.add_argument("--out", type=Path)
    p.add_argument("--driver", nargs="+", help="native executable, or an emulator followed by the executable")
    p.add_argument("--max-encoder", nargs="+", help="encoder command prefix (defaults to the driver's same-ABI sibling)")
    a = p.parse_args()
    if a.out:
        if a.driver:
            exercise(a.driver, a.out, a.source, a.max_encoder)
        else:
            generate(a.out, a.source, a.max_encoder)
    else:
        with tempfile.TemporaryDirectory(prefix="p4save-test-") as tmp:
            if not a.driver:
                p.error("--driver or --out required")
            exercise(a.driver, Path(tmp), a.source, a.max_encoder)
    return 0

if __name__ == "__main__":
    raise SystemExit(main())

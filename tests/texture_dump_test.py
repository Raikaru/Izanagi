#!/usr/bin/env python3
"""Asset-free production dump worker: decode PNGs and independently recompute keys."""
import hashlib
import json
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import zlib


def png_rgba(path):
    data = path.read_bytes()
    assert data[:8] == b"\x89PNG\r\n\x1a\n"
    offset = 8
    compressed = bytearray()
    dimensions = None
    while offset < len(data):
        size = struct.unpack_from(">I", data, offset)[0]
        tag = data[offset + 4:offset + 8]
        body = data[offset + 8:offset + 8 + size]
        crc = struct.unpack_from(">I", data, offset + 8 + size)[0]
        assert zlib.crc32(tag + body) & 0xffffffff == crc
        if tag == b"IHDR":
            dimensions = struct.unpack(">II5B", body)
            assert dimensions[2:] == (8, 6, 0, 0, 0)
        elif tag == b"IDAT":
            compressed.extend(body)
        elif tag == b"IEND":
            assert offset + size + 12 == len(data)
        offset += size + 12
    width, height = dimensions[:2]
    raw = zlib.decompress(compressed)
    stride = width * 4 + 1
    assert len(raw) == stride * height
    # The project's writer emits filter-0 rows; preserve invisible RGB as well.
    assert all(raw[y * stride] == 0 for y in range(height))
    return width, height, b"".join(raw[y * stride + 1:(y + 1) * stride] for y in range(height))


def main():
    command = sys.argv[1:]
    # Relative output paths work unchanged for native targets and Wine children.
    with tempfile.TemporaryDirectory(prefix="p4-texture-dump-") as directory:
        root = Path(directory)
        def run(*extra):
            subprocess.run([*command, "dump", *extra], cwd=root, check=True)
        run()
        inventory = root / "dump" / "inventory.jsonl"
        rows = [json.loads(line) for line in inventory.read_text().splitlines()]
        images = [row for row in rows if "hash" in row]
        assert len(images) == 2
        assert {row["excluded_origin"] for row in rows if "excluded_origin" in row} == {"unknown", "framebuffer"}
        assert images[0]["source"] == 'DATA/TEST".TMX'
        expected_rgba = bytes((i * 37 + 11) % 256 for i in range(24))
        image_files = sorted((root / "dump" / "textures" / "p4tex-v1").glob("*.png"))
        assert len(image_files) == 2
        before = {path.name: path.read_bytes() for path in image_files}
        concurrent = root / "dump" / "concurrent.png"
        assert png_rgba(concurrent) == (3, 2, expected_rgba)
        assert all(path.read_bytes() == concurrent.read_bytes() for path in image_files)
        for row in images:
            image = root / "dump" / "textures" / row["schema"] / (row["hash"] + ".png")
            w, h, rgba = png_rgba(image)
            assert (w, h, rgba) == (3, 2, expected_rgba)
            palette = bytes.fromhex(row["palette_rgba_hex"])
            wire = b"P4TEX\x01" + struct.pack("<5I", w, h, row["format"], row["palette_format"], row["palette_entries"])
            assert hashlib.sha256(wire + rgba + palette).hexdigest() == row["hash"]
        run()  # Resume verifies and preserves existing PNGs rather than overwrite.
        assert before == {path.name: path.read_bytes() for path in image_files}
        assert not list((root / "dump").rglob("*.tmp"))
        # A corrupt existing original is diagnosed, retained and never overwritten.
        first = root / "dump" / "textures" / "p4tex-v1" / (images[0]["hash"] + ".png")
        first.write_bytes(b"corrupt existing dump")
        run("fail")
        assert first.read_bytes() == b"corrupt existing dump"
        assert not list((root / "dump").rglob("*.tmp"))
    print("texture_dump: independent PNG/key checks, resume and failure preservation passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

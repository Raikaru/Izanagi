#!/usr/bin/env python3
"""Compare captured frames (PNG or lossless JXL), reporting RGB mean absolute difference,
largest channel difference and differing pixels. PNG decoding is dependency-free;
JXL is decoded to the same PNG path by djxl (P4PC_JXL_BIN_DIR or PATH).

  python tools/png_diff.py DIR_A DIR_B          # every *.png both have
  python tools/png_diff.py a.png b.png
  python tools/png_diff.py DIR_A DIR_B --write-diff OUT_DIR   # also writes <name>.diff.png (|a-b| x 8, saturated) for frames that differ
  python tools/png_diff.py --decode archived.png [--root DIR]  # PNG for an external viewer

Used to say how far one GL path's frame is from another's (docs/renderer_port.md, "OpenGL ES path"): 0.0000% and 0 pixels is
identical. Exit status 0 always; this reports, the gates judge.
"""
import os
import io
import shutil
import subprocess
import struct
import sys
import zlib
from pathlib import Path

try:  # a repository's capture store: cold residency, junction-aware link checks, managed decode scratch
    import capture_store
except ImportError:
    capture_store = None


def capture_path(path):
    """A recorded .png name also finds its lossless .jxl archive (and, with a capture store, its cold copy)."""
    path = os.fspath(path)
    if capture_store is not None:
        return capture_store.capture_path(path)
    if not os.path.exists(path) and path.lower().endswith(".png") and os.path.isfile(path[:-4] + ".jxl"):
        return path[:-4] + ".jxl"
    return path


def capture_files(directory):
    """Index PNG/JXL pairs once; prefer an existing PNG during atomic archival."""
    directory = os.fspath(directory)
    physical = capture_path(directory)
    names = set()
    files = {}
    for name in os.listdir(physical):
        if name.endswith(".ARCHIVED.json"):
            name = name[:-len(".ARCHIVED.json")]
        if name.lower().endswith((".png", ".jxl")) and not name[:-4].endswith(".diff"):
            names.add(name)
    for name in sorted(names):
        if name[:-4] not in files or name.lower().endswith(".png"):
            files[name[:-4]] = capture_path(os.path.join(directory, name))
    return files


def jxl_codec(name):
    directory = os.environ.get("P4PC_JXL_BIN_DIR")
    binary = os.path.join(directory, name + (".exe" if os.name == "nt" else "")) if directory else shutil.which(name)
    if not binary or not os.path.isfile(binary):
        raise FileNotFoundError("%s is required for lossless capture archives (set P4PC_JXL_BIN_DIR or PATH)" % name)
    return binary


def capture_png_bytes(path):
    path = capture_path(path)
    if path.lower().endswith(".jxl"):
        command = [jxl_codec("djxl"), path, "-", "--output_format=png",
                   "--bits_per_sample=8", "--num_threads=1", "--quiet"]
        if os.name != "nt":
            command = ["nice", "-n", "15", *command]
        result = subprocess.run(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                                creationflags=getattr(subprocess, "CREATE_NO_WINDOW", 0) |
                                getattr(subprocess, "BELOW_NORMAL_PRIORITY_CLASS", 0), timeout=120)
        if result.returncode:
            raise RuntimeError("capture decode exit %d; argv=%r; stderr=%r; stdout_bytes=%d" %
                               (result.returncode, command, result.stderr, len(result.stdout)))
        return result.stdout
    try:
        with open(path, "rb") as stream:
            return stream.read()
    except FileNotFoundError:
        archived = capture_path(path)
        if archived == path:
            raise
        return capture_png_bytes(archived)


def open_capture(path):
    """Pillow consumers share the same JXL decoder, including old recorded PNG paths."""
    from PIL import Image
    path = capture_path(path)
    if path.lower().endswith(".jxl"):
        with io.BytesIO(capture_png_bytes(path)) as stream:
            image = Image.open(stream)
            image.load()
        return image
    try:
        return Image.open(path)
    except FileNotFoundError:
        archived = capture_path(path)
        if archived == path:
            raise
        return open_capture(archived)


def copy_capture(source, destination):
    """Copy a logical PNG to its actual format plus the same verification sidecar."""
    source = Path(capture_path(source))
    logical = Path(destination)
    suffix = logical.suffix if source.suffix.lower() == ".png" and logical.suffix.lower() == ".png" else source.suffix.lower()
    destination = logical.with_suffix(suffix)
    link = capture_store.is_link if capture_store is not None else (lambda path: Path(path).is_symlink())
    if any(link(path) for path in (logical, destination, *destination.parents)) or \
            (capture_store is not None and capture_store.read_only(destination)):
        raise ValueError("retained capture copies must not modify retail/link inputs")
    if source.suffix.lower() not in (".png", ".jxl"):
        raise ValueError("not a PNG/JXL capture: %s" % source)
    metadata, target_metadata = Path(str(source) + ".json"), Path(str(destination) + ".json")
    if link(metadata) or link(target_metadata):
        raise ValueError("capture verification metadata must not be a link")
    destination.parent.mkdir(parents=True, exist_ok=True)
    try:
        shutil.copy2(source, destination)
    except FileNotFoundError:
        archived = Path(capture_path(source))
        if archived == source:
            raise
        return copy_capture(archived, destination)
    from mod_witness import copy_output_mode
    copy_output_mode(source, destination)
    if metadata.is_file():
        shutil.copy2(metadata, target_metadata)
    else:
        target_metadata.unlink(missing_ok=True)
    # An old alternate representation must not shadow this explicit replacement.
    alternate = destination.with_suffix(".png" if destination.suffix == ".jxl" else ".jxl")
    obsolete = {alternate}
    if logical != destination:
        obsolete.add(logical)
    for old in obsolete:
        old.unlink(missing_ok=True)
        Path(str(old) + ".json").unlink(missing_ok=True)
    return str(destination)


def materialize_capture(path, root=None):
    """Return a viewer-readable PNG; a JXL archive is decoded below root (default: a new temporary directory). A capture
    store decodes into its managed scratch instead, and root may then be its scratch policy."""
    source = capture_path(path)
    if not source.lower().endswith((".png", ".jxl")):
        raise ValueError("not a PNG/JXL capture: %s" % source)
    if not os.path.isfile(source):
        raise FileNotFoundError(source)
    if not source.lower().endswith(".jxl"):
        return os.path.abspath(source)
    if capture_store is not None:
        return capture_store.materialize(source, root, capture_png_bytes)
    import tempfile
    data = capture_png_bytes(source)
    if root is not None:
        Path(root).mkdir(parents=True, exist_ok=True)
    directory = Path(tempfile.mkdtemp(prefix="decoded-", dir=root))
    output = directory / (Path(source).stem + ".png")
    output.write_bytes(data)
    from mod_witness import copy_output_mode
    copy_output_mode(source, output)
    return str(output)


def read_png(path):
    data = capture_png_bytes(path)
    assert data[:8] == b"\x89PNG\r\n\x1a\n", path
    pos = 8
    idat = []
    width = height = depth = ctype = 0
    while pos < len(data):
        n, = struct.unpack(">I", data[pos:pos + 4])
        kind = data[pos + 4:pos + 8]
        body = data[pos + 8:pos + 8 + n]
        if kind == b"IHDR":
            width, height, depth, ctype, _, _, interlace = struct.unpack(">IIBBBBB", body)
            assert depth == 8 and interlace == 0 and ctype in (2, 6), (path, depth, ctype, interlace)
        elif kind == b"IDAT":
            idat.append(body)
        pos += 12 + n
    channels = 3 if ctype == 2 else 4
    raw = zlib.decompress(b"".join(idat))
    stride = width * channels
    out = bytearray()
    prev = bytearray(stride)
    p = 0
    for _ in range(height):
        ft = raw[p]
        row = bytearray(raw[p + 1:p + 1 + stride])
        p += 1 + stride
        if ft == 1:
            for i in range(channels, stride):
                row[i] = (row[i] + row[i - channels]) & 255
        elif ft == 2:
            for i in range(stride):
                row[i] = (row[i] + prev[i]) & 255
        elif ft == 3:
            for i in range(stride):
                left = row[i - channels] if i >= channels else 0
                row[i] = (row[i] + ((left + prev[i]) >> 1)) & 255
        elif ft == 4:
            for i in range(stride):
                a = row[i - channels] if i >= channels else 0
                b = prev[i]
                c = prev[i - channels] if i >= channels else 0
                pa, pb, pc = abs(b - c), abs(a - c), abs(a + b - 2 * c)
                pred = a if pa <= pb and pa <= pc else b if pb <= pc else c
                row[i] = (row[i] + pred) & 255
        out += row
        prev = row
    return width, height, channels, bytes(out)


def write_png(path, width, height, rgb):
    def chunk(kind, body):
        return struct.pack(">I", len(body)) + kind + body + struct.pack(">I", zlib.crc32(kind + body) & 0xFFFFFFFF)
    raw = b"".join(b"\x00" + rgb[y * width * 3:(y + 1) * width * 3] for y in range(height))
    with open(path, "wb") as stream:
        stream.write(b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", width, height, 8, 2, 0, 0, 0)) +
                     chunk(b"IDAT", zlib.compress(raw, 6)) + chunk(b"IEND", b""))


def compare(a, b, diff_out=None):
    wa, ha, ca, pa = read_png(a)
    wb, hb, cb, pb = read_png(b)
    from mod_witness import receipt_fields, tag_output
    mode = receipt_fields(sources=(capture_path(a), capture_path(b)))
    if (wa, ha) != (wb, hb):
        prefix = "mod_witness=true reference_eligible=false " if mode["mod_witness"] else ""
        return prefix + "size differs: %dx%d vs %dx%d" % (wa, ha, wb, hb)
    total = 0
    worst = 0
    pixels = 0
    diff = bytearray(wa * ha * 3) if diff_out else None
    for i in range(wa * ha):
        ia, ib = i * ca, i * cb
        d = [abs(pa[ia + c] - pb[ib + c]) for c in range(3)]
        s = d[0] + d[1] + d[2]
        if s:
            pixels += 1
            total += s
            worst = max(worst, max(d))
            if diff is not None:
                for c in range(3):
                    diff[i * 3 + c] = min(255, d[c] * 8)
    if diff is not None and pixels:
        write_png(diff_out, wa, ha, bytes(diff))
        if mode["mod_witness"]:
            tag_output(diff_out)
    message = "MAD %.4f%%  max channel diff %d  differing pixels %d of %d" % (total / (wa * ha * 3 * 255.0) * 100.0, worst, pixels, wa * ha)
    return ("mod_witness=true reference_eligible=false " + message) if mode["mod_witness"] else message


def main(argv):
    if argv and argv[0] == "--decode":
        import argparse
        parser = argparse.ArgumentParser(description="Decode a retained capture for an external PNG viewer")
        parser.add_argument("capture")
        parser.add_argument("--root", type=Path)
        options = parser.parse_args(argv[1:])
        print(materialize_capture(options.capture, options.root))
        return 0
    args = [a for a in argv if not a.startswith("--")]
    diff_dir = None
    if "--write-diff" in argv:
        diff_dir = argv[argv.index("--write-diff") + 1]
        args.remove(diff_dir)
        os.makedirs(diff_dir, exist_ok=True)
    if len(args) != 2:
        print(__doc__)
        return 2
    a, b = args
    if os.path.isdir(capture_path(a)) and os.path.isdir(capture_path(b)):
        left, right = capture_files(a), capture_files(b)
        for stem in sorted(left.keys() & right.keys()):
            out = os.path.join(diff_dir, stem + ".diff.png") if diff_dir else None
            print("%-40s %s" % (stem + ".png", compare(left[stem], right[stem], out)))
    else:
        print(compare(a, b, os.path.join(diff_dir, "diff.png") if diff_dir else None))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))

"""Minimal glTF 2.0 writer/reader helpers (stdlib only): one binary buffer, accessors, PNG encoding,
.gltf + .bin or a single .glb."""
import json
import math
import struct
import zlib

BYTE, UNSIGNED_BYTE, SHORT, UNSIGNED_SHORT, UNSIGNED_INT, FLOAT = 5120, 5121, 5122, 5123, 5125, 5126
ARRAY_BUFFER, ELEMENT_ARRAY_BUFFER = 34962, 34963
COMPONENT = {BYTE: ("b", 1), UNSIGNED_BYTE: ("B", 1), SHORT: ("h", 2), UNSIGNED_SHORT: ("H", 2),
             UNSIGNED_INT: ("I", 4), FLOAT: ("f", 4)}
WIDTH = {"SCALAR": 1, "VEC2": 2, "VEC3": 3, "VEC4": 4, "MAT4": 16}
GLB_MAGIC, GLB_JSON, GLB_BIN = 0x46546C67, 0x4E4F534A, 0x004E4942


def png_rgba(width, height, rgba):
    """PNG bytes of straight RGBA8 rows (top row first)."""
    def chunk(kind, payload):
        return struct.pack(">I", len(payload)) + kind + payload + struct.pack(">I", zlib.crc32(kind + payload) & 0xFFFFFFFF)
    stride = width * 4
    raw = b"".join(b"\0" + rgba[y * stride:(y + 1) * stride] for y in range(height))
    return (b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", width, height, 8, 6, 0, 0, 0))
            + chunk(b"IDAT", zlib.compress(raw, 9)) + chunk(b"IEND", b""))


def png_decode_rgba(data):
    """(width, height, RGBA bytes) of an 8-bit RGBA/RGB/gray/palette PNG without interlacing."""
    if data[:8] != b"\x89PNG\r\n\x1a\n":
        raise ValueError("not a PNG")
    offset, idat, palette, trns = 8, [], None, None
    while offset < len(data):
        length, kind = struct.unpack_from(">I4s", data, offset)
        payload = data[offset + 8:offset + 8 + length]
        offset += 12 + length
        if kind == b"IHDR":
            width, height, depth, ctype, _c, _f, interlace = struct.unpack(">IIBBBBB", payload)
        elif kind == b"PLTE":
            palette = payload
        elif kind == b"tRNS":
            trns = payload
        elif kind == b"IDAT":
            idat.append(payload)
        elif kind == b"IEND":
            break
    if depth != 8 or interlace:
        raise ValueError("PNG must be 8-bit, not interlaced")
    channels = {0: 1, 2: 3, 3: 1, 4: 2, 6: 4}[ctype]
    raw = zlib.decompress(b"".join(idat))
    stride = width * channels
    rows, prev = [], bytearray(stride)
    pos = 0
    for _ in range(height):
        f = raw[pos]
        line = bytearray(raw[pos + 1:pos + 1 + stride])
        pos += 1 + stride
        for i in range(stride):
            a = line[i - channels] if i >= channels else 0
            b = prev[i]
            c = prev[i - channels] if i >= channels else 0
            if f == 1:
                line[i] = (line[i] + a) & 0xFF
            elif f == 2:
                line[i] = (line[i] + b) & 0xFF
            elif f == 3:
                line[i] = (line[i] + ((a + b) >> 1)) & 0xFF
            elif f == 4:
                p = a + b - c
                pa, pb, pc = abs(p - a), abs(p - b), abs(p - c)
                line[i] = (line[i] + (a if pa <= pb and pa <= pc else b if pb <= pc else c)) & 0xFF
        rows.append(bytes(line))
        prev = line
    out = bytearray()
    for line in rows:
        for x in range(width):
            px = line[x * channels:(x + 1) * channels]
            if ctype == 6:
                out += px
            elif ctype == 2:
                out += px + b"\xff"
            elif ctype == 0:
                out += bytes((px[0], px[0], px[0], 255))
            elif ctype == 4:
                out += bytes((px[0], px[0], px[0], px[1]))
            else:
                i = px[0]
                alpha = trns[i] if trns is not None and i < len(trns) else 255
                out += palette[3 * i:3 * i + 3] + bytes((alpha,))
    return width, height, bytes(out)


def finite(value):
    """value with every non-finite float (JSON has none) replaced by the string of its float32 bits."""
    if isinstance(value, float):
        if math.isfinite(value):
            return value
        return "f32:0x%08x" % struct.unpack("<I", struct.pack("<f", value))[0]
    if isinstance(value, dict):
        return {k: finite(v) for k, v in value.items()}
    if isinstance(value, list):
        return [finite(v) for v in value]
    return value


class Builder:
    def __init__(self):
        self.doc = {"asset": {"version": "2.0", "generator": "Persona4-PC tools/models/rw_to_gltf.py"},
                    "scenes": [], "nodes": [], "meshes": [], "materials": [], "textures": [], "images": [],
                    "samplers": [], "skins": [], "animations": [], "accessors": [], "bufferViews": [],
                    "buffers": []}
        self.bin = bytearray()

    def view(self, payload, target=None, align=4):
        while len(self.bin) % align:
            self.bin += b"\0"
        view = {"buffer": 0, "byteOffset": len(self.bin), "byteLength": len(payload)}
        if target:
            view["target"] = target
        self.bin += payload
        self.doc["bufferViews"].append(view)
        return len(self.doc["bufferViews"]) - 1

    def accessor(self, rows, component, kind, target=None, normalized=False, minmax=False):
        """rows: list of tuples (or scalars for SCALAR)."""
        code, size = COMPONENT[component]
        width = WIDTH[kind]
        flat = [v for row in rows for v in row] if width > 1 else list(rows)
        payload = struct.pack("<%d%s" % (len(flat), code), *flat)
        acc = {"bufferView": self.view(payload, target, max(4, size)), "componentType": component,
               "count": len(rows), "type": kind}
        if normalized:
            acc["normalized"] = True
        if minmax:
            cols = [flat[i::width] for i in range(width)]
            acc["min"] = [min(c) for c in cols]
            acc["max"] = [max(c) for c in cols]
        self.doc["accessors"].append(acc)
        return len(self.doc["accessors"]) - 1

    def add(self, key, item):
        self.doc[key].append(item)
        return len(self.doc[key]) - 1

    def finish(self):
        doc = {k: v for k, v in self.doc.items() if v != []}
        if self.bin:
            doc["buffers"] = [{"byteLength": len(self.bin)}]
        return finite(doc)

    def write_gltf(self, path, bin_name):
        doc = self.finish()
        if self.bin:
            doc["buffers"][0]["uri"] = bin_name
        with open(path, "w", encoding="utf-8", newline="\n") as stream:
            json.dump(doc, stream, indent=1, sort_keys=False, allow_nan=False)
            stream.write("\n")
        return bytes(self.bin)

    def glb(self):
        doc = self.finish()
        text = json.dumps(doc, separators=(",", ":"), allow_nan=False).encode("utf-8")
        text += b" " * (-len(text) % 4)
        payload = bytes(self.bin) + b"\0" * (-len(self.bin) % 4)
        total = 12 + 8 + len(text) + (8 + len(payload) if payload else 0)
        out = struct.pack("<III", GLB_MAGIC, 2, total) + struct.pack("<II", len(text), GLB_JSON) + text
        if payload:
            out += struct.pack("<II", len(payload), GLB_BIN) + payload
        return out


def load(path):
    """(document, binary buffer bytes or None, base directory) for a .gltf or .glb file."""
    import os
    with open(path, "rb") as stream:
        data = stream.read()
    base = os.path.dirname(os.path.abspath(path))
    if data[:4] == b"glTF":
        _magic, _version, total = struct.unpack_from("<III", data, 0)
        offset, doc, binary = 12, None, None
        while offset < total:
            length, kind = struct.unpack_from("<II", data, offset)
            payload = data[offset + 8:offset + 8 + length]
            offset += 8 + length
            if kind == GLB_JSON:
                doc = json.loads(payload.decode("utf-8"))
            elif kind == GLB_BIN:
                binary = payload
        return doc, binary, base
    doc = json.loads(data.decode("utf-8"))
    binary = None
    if doc.get("buffers"):
        uri = doc["buffers"][0].get("uri")
        if uri:
            with open(os.path.join(base, uri), "rb") as stream:
                binary = stream.read()
    return doc, binary, base


def read_accessor(doc, binary, index):
    """Rows of accessor index (tuples for vector types, scalars for SCALAR)."""
    acc = doc["accessors"][index]
    view = doc["bufferViews"][acc["bufferView"]]
    code, size = COMPONENT[acc["componentType"]]
    width = WIDTH[acc["type"]]
    stride = view.get("byteStride", size * width)
    start = view.get("byteOffset", 0) + acc.get("byteOffset", 0)
    rows = []
    fmt = "<%d%s" % (width, code)
    for i in range(acc["count"]):
        values = struct.unpack_from(fmt, binary, start + i * stride)
        rows.append(values if width > 1 else values[0])
    return rows


def quat_from_rows(r):
    """Unit quaternion (x, y, z, w) of a RenderWare rotation (rows right, up, at = columns of the
    column-vector rotation matrix)."""
    # Column-vector matrix m[i][j] = r[j][i].
    m00, m01, m02 = r[0][0], r[1][0], r[2][0]
    m10, m11, m12 = r[0][1], r[1][1], r[2][1]
    m20, m21, m22 = r[0][2], r[1][2], r[2][2]
    trace = m00 + m11 + m22
    if trace > 0:
        s = math.sqrt(trace + 1.0) * 2
        w, x, y, z = 0.25 * s, (m21 - m12) / s, (m02 - m20) / s, (m10 - m01) / s
    elif m00 > m11 and m00 > m22:
        s = math.sqrt(1.0 + m00 - m11 - m22) * 2
        w, x, y, z = (m21 - m12) / s, 0.25 * s, (m01 + m10) / s, (m02 + m20) / s
    elif m11 > m22:
        s = math.sqrt(1.0 + m11 - m00 - m22) * 2
        w, x, y, z = (m02 - m20) / s, (m01 + m10) / s, 0.25 * s, (m12 + m21) / s
    else:
        s = math.sqrt(1.0 + m22 - m00 - m11) * 2
        w, x, y, z = (m10 - m01) / s, (m02 + m20) / s, (m12 + m21) / s, 0.25 * s
    n = math.sqrt(x * x + y * y + z * z + w * w)
    return (x / n, y / n, z / n, w / n)


def rows_from_quat(q):
    x, y, z, w = q
    # Column-vector rotation R; RenderWare rows are its columns.
    return ((1 - 2 * (y * y + z * z), 2 * (x * y + w * z), 2 * (x * z - w * y)),
            (2 * (x * y - w * z), 1 - 2 * (x * x + z * z), 2 * (y * z + w * x)),
            (2 * (x * z + w * y), 2 * (y * z - w * x), 1 - 2 * (x * x + y * y)))

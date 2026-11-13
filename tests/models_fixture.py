"""Synthetic Persona 4 RMD files for the model pipeline tests (no retail bytes).

build_rmd() writes the chunk layout docs/modding/models.md documents (slot count, texture dictionary
of PS2 native textures, clump with frames/HAnim/user data, geometry with bin mesh, optional skin,
attach table, slot matrix, compressed HAnim animation, slot aliases) from a small description, so tests
know every value the converter must reproduce.
"""
import os
import struct
import sys

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "tools", "models"))
import gltfio  # noqa: E402
import ps2tex  # noqa: E402

LIBID = 0x1C020037
P4_LIBID = 0x40000000
IDENTITY = (1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0)


def chunk(kind, payload, libid=LIBID):
    return struct.pack("<III", kind, len(payload), libid) + payload


def struct_chunk(payload):
    return chunk(0x01, payload)


def string_chunk(text):
    raw = text.encode("latin-1") + b"\0"
    raw += b"\0" * (-len(raw) % 4)
    return chunk(0x02, raw)


def extension(*plugins):
    return chunk(0x03, b"".join(chunk(pid, payload) for pid, payload in plugins))


def user_data(arrays):
    out = struct.pack("<i", len(arrays))
    for name, fmt, values in arrays:
        raw = name.encode("latin-1") + b"\0"
        out += struct.pack("<i", len(raw)) + raw + struct.pack("<ii", fmt, len(values))
        for value in values:
            if fmt == 1:
                out += struct.pack("<i", value)
            elif fmt == 2:
                out += struct.pack("<f", value)
            else:
                raw = value.encode("latin-1") + b"\0"
                out += struct.pack("<i", len(raw)) + raw
    return out


def native_texture(name, width, height, depth, pixels, palette=None, filter_addressing=0x1102):
    """pixels: indices (PAL4/PAL8) or RGBA bytes (32-bit); palette: straight RGBA tuples."""
    if depth == 32:
        data = ps2tex.encode_rgba32(pixels, width, height)
        flags = 0x0504
    else:
        data = ps2tex.encode_paletted(pixels, palette, width, height, depth)
        flags = 0x2504 if depth == 8 else 0x4504
    header = struct.pack("<16I", width, height, depth, (2 << 16) | flags, *([0] * 12))
    raster = struct_chunk(header) + struct_chunk(data)
    return chunk(0x15, struct_chunk(struct.pack("<II", 0x00325350, filter_addressing)) + string_chunk(name)
                 + string_chunk("") + struct_chunk(raster)
                 + extension((0x110, struct.pack("<I", 0xFC0))))


def texdictionary(textures):
    return chunk(0x16, struct_chunk(struct.pack("<HH", len(textures), 6)) + b"".join(textures) + extension())


def material(color, texture_name=None, arrays=(), filter_addressing=0x1102, surface=(1.0, 1.0, 1.0)):
    body = struct_chunk(struct.pack("<i4Bii3f", 0, *color, 0, 1 if texture_name else 0, *surface))
    if texture_name:
        body += chunk(0x06, struct_chunk(struct.pack("<I", filter_addressing)) + string_chunk(texture_name)
                      + string_chunk("") + extension((0x110, struct.pack("<I", 0xFC0))))
    plugins = [(0x11F, user_data(arrays))] if arrays else []
    return chunk(0x07, body + extension(*plugins))


def geometry(positions, normals, uvs, prelit, triangles, materials, skin=None, fmt=None, sphere=(0.0, 0.0, 0.0, 10.0)):
    nv = len(positions)
    if fmt is None:
        fmt = 0x1 | 0x2 | 0x10 | 0x20 | (0x4 if uvs else 0) | (0x8 if prelit else 0) | (0x10000 if uvs else 0)
    body = struct.pack("<Iiii", fmt, len(triangles), nv, 1)
    if prelit:
        body += b"".join(struct.pack("<4B", *c) for c in prelit)
    if uvs:
        body += b"".join(struct.pack("<2f", *uv) for uv in uvs)
    for v0, v1, v2, mat in triangles:
        body += struct.pack("<II", (v0 << 16) | v1, (v2 << 16) | mat)
    body += struct.pack("<4fii", *sphere, 1, 1 if normals else 0)
    body += b"".join(struct.pack("<3f", *p) for p in positions)
    if normals:
        body += b"".join(struct.pack("<3f", *n) for n in normals)
    matlist = chunk(0x08, struct_chunk(struct.pack("<i", len(materials)) + struct.pack("<%di" % len(materials), *([-1] * len(materials))))
                    + b"".join(materials))
    # Bin mesh: one strip per material, built naively from the triangle list (degenerate joins).
    meshes = []
    for mi in range(len(materials)):
        strip = []
        for v0, v1, v2, mat in triangles:
            if mat != mi:
                continue
            if strip:
                strip += [strip[-1], v0]
                if len(strip) % 2:
                    strip.append(v0)
            strip += [v0, v1, v2]
        meshes.append((mi, strip))
    binmesh = struct.pack("<III", 1, len(meshes), sum(len(s) for _m, s in meshes))
    for mi, strip in meshes:
        binmesh += struct.pack("<II", len(strip), mi) + struct.pack("<%dI" % len(strip), *strip)
    plugins = [(0x50E, binmesh)]
    if skin is not None:
        indices, weights, matrices = skin
        used = sorted({j for js, ws in zip(indices, weights) for j, w in zip(js, ws) if w > 0})
        payload = struct.pack("<4B", len(matrices), len(used), max(sum(1 for w in ws if w > 0) for ws in weights), 0)
        payload += bytes(used)
        payload += b"".join(bytes(js) for js in indices)
        payload += b"".join(struct.pack("<4f", *ws) for ws in weights)
        payload += b"".join(struct.pack("<16f", *m) for m in matrices)
        payload += struct.pack("<3i", 0, 0, 0)
        plugins.append((0x116, payload))
    plugins.append((0x11F, user_data([("FVF.UserData", 1, [0])])))
    return chunk(0x0F, struct_chunk(body) + matlist + extension(*plugins))


def hanim(node_id, nodes=None):
    if not nodes:
        return struct.pack("<iii", 0x100, node_id, 0)
    out = struct.pack("<iiiii", 0x100, node_id, len(nodes), 0, 36)
    return out + b"".join(struct.pack("<iii", *n) for n in nodes)


def clump(frames, geometries, atomics):
    """frames: [(rotation9, position3, parent, plugins list)]; atomics: [(frame, geometry)]."""
    framelist = struct.pack("<i", len(frames))
    for rotation, position, parent, _plugins in frames:
        framelist += struct.pack("<12fiI", *rotation, *position, parent, 0)
    body = struct_chunk(struct.pack("<iii", len(atomics), 0, 0))
    body += chunk(0x0E, struct_chunk(framelist) + b"".join(extension(*plugins) for _r, _p, _pa, plugins in frames))
    body += chunk(0x1A, struct_chunk(struct.pack("<i", len(geometries))) + b"".join(geometries))
    for frame, geo in atomics:
        body += chunk(0x14, struct_chunk(struct.pack("<iiIi", frame, geo, 5, 0)) + extension((0x1F, struct.pack("<II", 0x116, 1))))
    return chunk(0x10, body + extension())


def compress_float(value):
    """Inverse of rwmodel.decompress_float for values it represents exactly."""
    bits = struct.unpack("<I", struct.pack("<f", value))[0]
    if value == 0.0:
        return (bits >> 16) & 0x8000
    sign = (bits >> 16) & 0x8000
    exp = ((bits >> 23) & 0xFF) - 112
    if not 0 <= exp <= 15 or bits & 0xFFF:
        raise ValueError("%r is not exactly representable" % value)
    return sign | (exp << 11) | ((bits >> 12) & 0x7FF)


def animation(keys, duration, offset=(0.0, 0.0, 0.0), scale=(1.0, 1.0, 1.0)):
    """keys: [(time, quat4, translation3 already in packed units 0..1.99, previous index or None)]."""
    body = struct.pack("<iiiif", 0x100, 2, len(keys), 0, duration)
    for t, q, tr, prev in keys:
        body += struct.pack("<f7Hi", t, *(compress_float(c) for c in q), *(compress_float(c) for c in tr),
                            -1 if prev is None else prev * 24)
    body += struct.pack("<3f3f", *offset, *scale)
    return chunk(0x1B, body)


def p4(kind, payload=b""):
    return chunk(kind, payload, P4_LIBID)


def build_rmd(parts):
    return b"".join(parts)


# ---------------------------------------------------------------------------------------------------
# The two standard fixtures

def checker_texture(name, size=16, depth=8):
    palette = [(255, 0, 0, 255), (0, 0, 255, 255), (0, 255, 0, 128), (255, 255, 255, 0)] + [(i, i, i, 255) for i in range(4, 256 if depth == 8 else 16)]
    indices = bytes(((x // 4 + y // 4) % 4) if (x + y) % 7 else (x * 3 + y) % len(palette) for y in range(size) for x in range(size))
    return native_texture(name, size, size, depth, indices, palette), indices, palette


def static_weapon():
    """A weapon-shaped model: root frame + HAnim bone frame with the atomic, one textured material, the
    weapon slot layout (slot count 3: animation, alias, animation)."""
    tex, indices, palette = checker_texture("blade", 32, 8)
    positions = [(0.0, 0.0, 0.0), (1.0, 0.0, 0.0), (1.0, 10.0, 0.0), (0.0, 10.0, 0.0), (0.5, 12.0, 0.25)]
    normals = [(0.0, 0.0, 1.0)] * 4 + [(0.0, 0.6, 0.8)]
    uvs = [(0.0, 1.0), (1.0, 1.0), (1.0, 0.0), (0.0, 0.0), (0.5, -0.25)]
    prelit = [(255, 255, 255, 255), (128, 128, 128, 255), (64, 32, 16, 255), (0, 0, 0, 255), (10, 20, 30, 40)]
    triangles = [(0, 1, 2, 0), (0, 2, 3, 0), (3, 2, 4, 0), (1, 1, 2, 0)]  # the last one is degenerate
    mat = material((200, 150, 100, 255), "blade", [("name", 3, ["blade_mat"]), ("per3MatAlpha", 1, [2])])
    geo = geometry(positions, normals, uvs, prelit, triangles, [mat])
    frames = [(IDENTITY, (0.5, -1.0, 2.0), -1, []),
              (IDENTITY, (0.0, 0.0, 0.0), 0, [(0x11E, hanim(0, [(0, 0, 1)]))])]
    anim = animation([(0.0, (0.0, 0.0, 0.0, 1.0), (0.0, 0.0, 0.0), None),
                      (1.0, (0.0, 0.0, 0.0, 1.0), (0.0, 0.0, 0.0), 0)], 1.0)
    matrix = struct.pack("<16f", *IDENTITY[0:3], 0.0, *IDENTITY[3:6], 0.0, *IDENTITY[6:9], 0.0, 0.0, 0.0, 0.0, 1.0)
    attach = struct.pack("<H", 1) + struct.pack("<ii", 100, 0) + matrix
    data = build_rmd([p4(0xF0F000F0, struct.pack("<H", 3)), texdictionary([tex]), clump(frames, [geo], [(1, 0)]),
                      p4(0xF0F00006, attach), p4(0xF0F00005, matrix), anim, p4(0xF0F00004), p4(0xF0F00003, struct.pack("<H", 0)),
                      p4(0xF0F00004), anim])
    return data, {"positions": positions, "normals": normals, "uvs": uvs, "prelit": prelit, "triangles": triangles,
                  "indices": indices, "palette": palette, "texture_size": 32}


def skinned_actor():
    """Three bones in a chain, a skinned quad strip, a two-keyframe compressed animation per bone."""
    positions = [(0.0, 0.0, 0.0), (1.0, 0.0, 0.0), (0.0, 1.0, 0.0), (1.0, 1.0, 0.0), (0.0, 2.0, 0.0), (1.0, 2.0, 0.0)]
    normals = [(0.0, 0.0, 1.0)] * 6
    triangles = [(0, 1, 2, 0), (2, 1, 3, 0), (2, 3, 4, 0), (4, 3, 5, 0)]
    indices = [(0, 0, 0, 0), (0, 0, 0, 0), (0, 1, 0, 0), (1, 0, 0, 0), (2, 0, 0, 0), (2, 1, 0, 0)]
    weights = [(1.0, 0.0, 0.0, 0.0), (1.0, 0.0, 0.0, 0.0), (0.5, 0.5, 0.0, 0.0), (0.75, 0.25, 0.0, 0.0),
               (1.0, 0.0, 0.0, 0.0), (0.5, 0.5, 0.0, 0.0)]
    def inverse(y):
        return (1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, -y, 0.0, 1.0)
    matrices = [inverse(0.0), inverse(1.0), inverse(2.0)]
    mat = material((255, 255, 255, 255), None, [("name", 3, ["skin_mat"])])
    geo = geometry(positions, normals, None, None, triangles, [mat], skin=(indices, weights, matrices))
    nodes = [(10, 0, 2), (11, 1, 2), (12, 2, 1)]
    frames = [(IDENTITY, (0.0, 0.0, 0.0), -1, []),
              (IDENTITY, (0.0, 0.0, 0.0), 0, [(0x11E, hanim(10, nodes))]),
              (IDENTITY, (0.0, 1.0, 0.0), 1, [(0x11E, hanim(11))]),
              (IDENTITY, (0.0, 1.0, 0.0), 2, [(0x11E, hanim(12)), (0x11F, user_data([("per3HelperId", 1, [500])]))])]
    half = 0.70703125  # exactly representable in the 11-bit-mantissa packed float
    keys = [(0.0, (0.0, 0.0, 0.0, 1.0), (0.0, 0.0, 0.0), None),
            (0.0, (0.0, 0.0, 0.0, 1.0), (0.0, 1.0, 0.0), None),
            (0.0, (0.0, 0.0, 0.0, 1.0), (0.0, 1.0, 0.0), None),
            (0.5, (0.0, 0.0, 0.0, 1.0), (0.0, 0.0, 0.0), 0),
            (1.0, (0.0, 0.0, half, half), (0.0, 1.0, 0.0), 1),
            (1.0, (0.0, 0.0, 0.0, 1.0), (0.0, 1.0, 0.0), 2),
            (1.0, (0.0, 0.0, 0.0, 1.0), (0.0, 0.0, 0.0), 3)]
    anim = animation(keys, 1.0, offset=(0.0, 0.0, 0.0), scale=(1.0, 1.0, 1.0))
    data = build_rmd([p4(0xF0F000F0, struct.pack("<H", 1)), clump(frames, [geo], [(1, 0)]), anim])
    return data, {"positions": positions, "triangles": triangles, "indices": indices, "weights": weights,
                  "matrices": matrices, "keys": keys, "nodes": nodes}


def skinned_gltf(joint_names=("bone_1", "bone_2"), skin=True, animations=()):
    """A synthetic skinned glTF (GLB bytes, positions, weights): a 4-column strip whose vertices blend from
    the first joint to the second, on a scene-root node with a skin of two joints (each with its own inverse
    bind matrix, the first with matrix pads in extras.p4.matrixPads). skin=False leaves JOINTS_0/WEIGHTS_0
    without a skin. animations: [(name, [(node, path, times, values, interpolation)])], nodes 1 and 2 being
    the joints (the second at translation (20, 0, 0) under the first)."""
    b = gltfio.Builder()
    positions, joints, weights, indices = [], [], [], []
    columns = 4
    for c in range(columns + 1):
        t = c / columns
        for y in (0.0, 1.0):
            positions.append((10.0 * c, y, 0.0))
            joints.append((0, 1, 0, 0) if 0 < c < columns else ((0, 0, 0, 0) if c == 0 else (1, 0, 0, 0)))
            weights.append((1.0 - t, t, 0.0, 0.0) if 0 < c < columns else (1.0, 0.0, 0.0, 0.0))
    for c in range(columns):
        v = 2 * c
        indices += [v, v + 2, v + 1, v + 1, v + 2, v + 3]
    attrs = {"POSITION": b.accessor(positions, gltfio.FLOAT, "VEC3", minmax=True),
             "NORMAL": b.accessor([(0.0, 0.0, 1.0)] * len(positions), gltfio.FLOAT, "VEC3"),
             "JOINTS_0": b.accessor(joints, gltfio.UNSIGNED_BYTE, "VEC4"),
             "WEIGHTS_0": b.accessor(weights, gltfio.FLOAT, "VEC4")}
    b.add("materials", {"name": "skin_mat", "pbrMetallicRoughness": {"baseColorFactor": [0.5, 0.25, 1.0, 1.0]}})
    b.add("meshes", {"name": "strip", "primitives": [{"attributes": attrs, "material": 0,
                                                      "indices": b.accessor(indices, gltfio.UNSIGNED_SHORT, "SCALAR")}]})
    ibms = [(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1), (1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, -20.0, 0, 0, 1)]
    node = {"name": "skinned_0", "mesh": 0}
    if skin:
        node["skin"] = 0
        b.add("skins", {"joints": [1, 2], "inverseBindMatrices": b.accessor(ibms, gltfio.FLOAT, "MAT4"),
                        "extras": {"p4": {"matrixPads": [["0x1", "0x2", "0x3", "0x4"], ["0x0", "0x0", "0x0", "0x0"]]}}})
    b.add("nodes", node)
    b.add("nodes", {"name": joint_names[0], "children": [2]})
    b.add("nodes", {"name": joint_names[1], "translation": [20.0, 0.0, 0.0]})
    for name, channels in animations:
        samplers, targets = [], []
        for node_index, path, times, values, interpolation in channels:
            kind = {"rotation": "VEC4", "translation": "VEC3", "scale": "VEC3"}[path]
            samplers.append({"input": b.accessor(list(times), gltfio.FLOAT, "SCALAR", minmax=True),
                             "output": b.accessor(list(values), gltfio.FLOAT, kind), "interpolation": interpolation})
            targets.append({"sampler": len(samplers) - 1, "target": {"node": node_index, "path": path}})
        b.add("animations", {"name": name, "channels": targets, "samplers": samplers})
    b.add("scenes", {"nodes": [0, 1]})
    b.doc["scene"] = 0
    return b.glb(), positions, weights

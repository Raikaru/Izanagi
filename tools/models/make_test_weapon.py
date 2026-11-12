#!/usr/bin/env python3
"""Write a synthetic glTF weapon and the mod that installs it with [resource-model] (no retail bytes).

    python tools/models/make_test_weapon.py OUT [--replaces DATA/MODEL/WEAPON/WP004.RMD] [--id example.model]

OUT/<id>/mod.ini and OUT/<id>/models/test_sword.glb: a 160-unit sword along +Y with its grip at the origin,
on the node bone_0 (the HAnim bone that carries a retail weapon's mesh), with a 32-pixel texture of large
blocks in the value range of retail weapon textures (paletted when cooked) and a lit material, like retail
weapons. Install OUT/<id> under the mods directory and enable <id>; it replaces the protagonist's default
weapon (item 4, model WP004) by default.
"""
import argparse
import math
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import gltfio  # noqa: E402

MOD_INI = """[mod]
id = {id}
name = Example glTF weapon
version = 1.0.0
api = 1.{minor}
priority = 0
order = 0

[resource-model {id}:sword]
file = models/test_sword.glb
format = gltf
schema = 1
revision = 1
replaces = {replaces}
"""


def box(x0, x1, y0, y1, z0, z1, u0, u1, v0, v1):
    """Positions, normals, UVs and triangle indices of an axis-aligned box (24 vertices, 12 triangles)."""
    faces = [((1, 0, 0), [(x1, y0, z0), (x1, y1, z0), (x1, y1, z1), (x1, y0, z1)]),
             ((-1, 0, 0), [(x0, y0, z1), (x0, y1, z1), (x0, y1, z0), (x0, y0, z0)]),
             ((0, 1, 0), [(x0, y1, z0), (x0, y1, z1), (x1, y1, z1), (x1, y1, z0)]),
             ((0, -1, 0), [(x0, y0, z1), (x0, y0, z0), (x1, y0, z0), (x1, y0, z1)]),
             ((0, 0, 1), [(x1, y0, z1), (x1, y1, z1), (x0, y1, z1), (x0, y0, z1)]),
             ((0, 0, -1), [(x0, y0, z0), (x0, y1, z0), (x1, y1, z0), (x1, y0, z0)])]
    positions, normals, uvs, indices = [], [], [], []
    for normal, quad in faces:
        base = len(positions)
        positions += quad
        normals += [normal] * 4
        uvs += [(u0, v1), (u0, v0), (u1, v0), (u1, v1)]
        indices += [base, base + 1, base + 2, base, base + 2, base + 3]
    return positions, normals, uvs, indices


def ccw(positions, normals, indices):
    out = []
    for i in range(0, len(indices), 3):
        a, b, c = (positions[k] for k in indices[i:i + 3])
        n = normals[indices[i]]
        e1 = [b[k] - a[k] for k in range(3)]
        e2 = [c[k] - a[k] for k in range(3)]
        cross = (e1[1] * e2[2] - e1[2] * e2[1], e1[2] * e2[0] - e1[0] * e2[2], e1[0] * e2[1] - e1[1] * e2[0])
        tri = indices[i:i + 3]
        out += tri if sum(cross[k] * n[k] for k in range(3)) > 0 else [tri[0], tri[2], tri[1]]
    return out


def sword():
    parts = [  # (x0, x1, y0, y1, z0, z1, u0, u1, v0, v1)
        (-1.8, 1.8, -14.0, 6.0, -1.8, 1.8, 0.0, 0.5, 0.5, 1.0),       # grip
        (-9.0, 9.0, 6.0, 9.0, -2.4, 2.4, 0.5, 1.0, 0.5, 1.0),         # guard
        (-4.0, 4.0, 9.0, 150.0, -1.2, 1.2, 0.0, 1.0, 0.0, 0.5),       # blade: thick enough to read at 640x448
        (-2.4, 2.4, 150.0, 160.0, -0.9, 0.9, 0.0, 1.0, 0.0, 0.5),     # tip
    ]
    positions, normals, uvs, indices = [], [], [], []
    for part in parts:
        p, n, t, i = box(*part)
        base = len(positions)
        positions += p
        normals += n
        uvs += t
        indices += [base + k for k in ccw(p, n, i)]
    return positions, normals, uvs, indices


def texture(size=32):
    """Large blocks inside retail weapon textures' value range (docs/modding/models.md, "Look"): a
    cool steel blade (luma about 130 and 157), a dark wrapped grip and a brass guard."""
    rgba = bytearray()
    for y in range(size):
        for x in range(size):
            if y < size // 2:
                colour = (104, 140, 150, 255) if (x // 8 + y // 8) % 2 else (132, 166, 174, 255)
            elif x < size // 2:
                colour = (88, 44, 76, 255) if (y // 4) % 2 else (64, 32, 56, 255)
            else:
                colour = (168, 128, 52, 255) if (y // 4) % 2 else (132, 98, 40, 255)
            rgba += bytes(colour)
    return size, size, bytes(rgba)


def build():
    positions, normals, uvs, indices = sword()
    b = gltfio.Builder()
    attrs = {"POSITION": b.accessor(positions, gltfio.FLOAT, "VEC3", gltfio.ARRAY_BUFFER, minmax=True),
             "NORMAL": b.accessor(normals, gltfio.FLOAT, "VEC3", gltfio.ARRAY_BUFFER),
             "TEXCOORD_0": b.accessor(uvs, gltfio.FLOAT, "VEC2", gltfio.ARRAY_BUFFER)}
    w, h, rgba = texture()
    b.add("images", {"name": "test_sword", "bufferView": b.view(gltfio.png_rgba(w, h, rgba)), "mimeType": "image/png"})
    b.add("samplers", {"magFilter": 9729, "minFilter": 9729, "wrapS": 10497, "wrapT": 10497})
    b.add("textures", {"source": 0, "sampler": 0})
    b.add("materials", {"name": "test_sword", "pbrMetallicRoughness": {"baseColorTexture": {"index": 0},
                                                                       "metallicFactor": 0.0, "roughnessFactor": 1.0}})
    b.add("meshes", {"name": "test_sword", "primitives": [{"attributes": attrs, "material": 0, "mode": 4,
        "indices": b.accessor(indices, gltfio.UNSIGNED_SHORT, "SCALAR", gltfio.ELEMENT_ARRAY_BUFFER)}]})
    b.add("nodes", {"name": "frame_0", "children": [1]})
    b.add("nodes", {"name": "bone_0", "mesh": 0})
    b.add("scenes", {"name": "test_sword", "nodes": [0]})
    b.doc["scene"] = 0
    return b.glb()


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    parser.add_argument("out")
    parser.add_argument("--replaces", default="DATA/MODEL/WEAPON/WP004.RMD")
    parser.add_argument("--id", default="example.model")
    parser.add_argument("--api-minor", type=int, default=16, help="the [mod] api minor the section requires")
    args = parser.parse_args(argv)
    root = os.path.join(args.out, args.id)
    os.makedirs(os.path.join(root, "models"), exist_ok=True)
    with open(os.path.join(root, "models", "test_sword.glb"), "wb") as stream:
        stream.write(build())
    with open(os.path.join(root, "mod.ini"), "w", encoding="ascii", newline="\n") as stream:
        stream.write(MOD_INI.format(id=args.id, replaces=args.replaces, minor=args.api_minor))
    print(root)
    return 0


if __name__ == "__main__":
    sys.exit(main())

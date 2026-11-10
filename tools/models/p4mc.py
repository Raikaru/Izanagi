"""Reader for cooked glTF models (.p4mc, src/platform/models/p4model.h), for tests and diagnostics.

    python tools/models/p4mc.py model.p4mc      # summary
"""
import struct
import sys
import zlib

MAGIC, VERSION = 0x434D3450, 3
HEADER = struct.Struct("<16I68s3I")
TEXTURE = struct.Struct("<32s8I")
MATERIAL = struct.Struct("<4B3fiiII32s")
MESH = struct.Struct("<2i14I")
SKIN = struct.Struct("<8I")
ANIMATION = struct.Struct("<2If3I6f4I")
TRACK = struct.Struct("<i3I")
KEY = struct.Struct("<f8H")
FRAME_KINDS = {0: "root", 1: "hanim", 2: "helper", 3: "skin"}


def _name(raw):
    return raw.split(b"\0", 1)[0].decode("latin-1")


def load(data):
    h = HEADER.unpack_from(data, 0)
    magic, version, size, flags, textures, materials, meshes, triangles, toff, moff, meoff, tbytes = h[:12]
    if magic != MAGIC or version != VERSION or size != len(data):
        raise ValueError("not a version %d cooked model" % VERSION)
    if h[12] != zlib.crc32(data[HEADER.size:]) & 0xFFFFFFFF:
        raise ValueError("cooked model body CRC mismatch")
    model = {"source": _name(h[16]), "triangles": triangles, "textureBytes": tbytes, "textures": [], "materials": [],
             "meshes": [], "animations": []}
    for i in range(textures):
        name, w, hgt, depth, fa, pal, pix, nbytes, _r = TEXTURE.unpack_from(data, toff + i * TEXTURE.size)
        tex = {"name": _name(name), "width": w, "height": hgt, "depth": depth, "filterAddressing": fa}
        pixels = data[pix:pix + nbytes]
        if depth == 8:
            palette = [tuple(data[pal + 4 * k:pal + 4 * k + 4]) for k in range(256)]
            tex["rgba"] = b"".join(bytes(palette[index]) for index in pixels)
        else:
            tex["rgba"] = bytes(pixels)
        model["textures"].append(tex)
    for i in range(materials):
        r, g, b, a, amb, spec, diff, texture, category, mflags, tint, name = MATERIAL.unpack_from(data, moff + i * MATERIAL.size)
        model["materials"].append({"color": (r, g, b, a), "surface": (amb, spec, diff), "texture": texture,
                                   "drawCategory": category, "flags": mflags, "tint": tint, "name": _name(name)})
    for i in range(meshes):
        (kind, frame, nv, nt, fmt, uvsets, poff, noff, uv0, uv1, coff, troff, nmat, matoff, skoff, _r) = MESH.unpack_from(
            data, meoff + i * MESH.size)

        def rows(offset, width, code="f"):
            flat = struct.unpack_from("<%d%s" % (nv * width, code), data, offset)
            return [flat[k:k + width] for k in range(0, len(flat), width)]
        mesh = {"frame": (FRAME_KINDS[kind], frame), "format": fmt, "positions": rows(poff, 3),
                "normals": rows(noff, 3) if noff else None, "uv0": rows(uv0, 2) if uvsets >= 1 else None,
                "uv1": rows(uv1, 2) if uvsets == 2 else None, "colors": rows(coff, 4, "B") if coff else None,
                "materials": list(struct.unpack_from("<%dI" % nmat, data, matoff))}
        tris = struct.unpack_from("<%dH" % (nt * 4), data, troff)
        mesh["triangles"] = [tris[k:k + 4] for k in range(0, len(tris), 4)]
        mesh["skin"] = None
        if skoff:
            joints, max_weights, ids, matrices, slots, weights, order_count, order = SKIN.unpack_from(data, skoff)
            words = struct.unpack_from("<%dI" % (16 * joints), data, matrices)
            mesh["skin"] = {"joints": list(struct.unpack_from("<%di" % joints, data, ids)), "maxWeights": max_weights,
                            "matrices": [words[16 * j:16 * j + 16] for j in range(joints)],
                            "slots": rows(slots, 4, "B"), "weights": rows(weights, 4),
                            "order": list(data[order:order + order_count]) if order_count else None}
        model["meshes"].append(mesh)
    animations, animations_offset = h[13], h[14]
    for i in range(animations):
        a = ANIMATION.unpack_from(data, animations_offset + i * ANIMATION.size)
        layer, slot, duration, flags, ntracks, tracks = a[0], a[1], a[2], a[3], a[4], a[5]
        anim = {"layer": layer, "slot": slot, "duration": duration, "flags": flags,
                "translationOffset": a[6:9], "translationScale": a[9:12], "tracks": {}}
        for t in range(ntracks):
            node, nkeys, keys, _r = TRACK.unpack_from(data, tracks + t * TRACK.size)
            # (time, packed rotation x y z w, packed translation x y z): RtCompressedKeyFrame halves
            anim["tracks"][node] = [(k[0], k[1:5], k[5:8]) for k in
                                    (KEY.unpack_from(data, keys + j * KEY.size) for j in range(nkeys))]
        model["animations"].append(anim)
    return model


def main(argv):
    with open(argv[1], "rb") as stream:
        model = load(stream.read())
    print("source %s: %d triangles, %d texture bytes" % (model["source"], model["triangles"], model["textureBytes"]))
    for t in model["textures"]:
        print("  texture %s %dx%d depth %d filter 0x%x" % (t["name"], t["width"], t["height"], t["depth"], t["filterAddressing"]))
    for m in model["materials"]:
        print("  material %s colour %s texture %d category %d flags %d" % (m["name"], m["color"], m["texture"], m["drawCategory"], m["flags"]))
    for m in model["meshes"]:
        print("  mesh on %s %d: %d vertices, %d triangles, format 0x%x, materials %s" % (
            m["frame"][0], m["frame"][1], len(m["positions"]), len(m["triangles"]), m["format"], m["materials"]))
        if m["skin"]:
            print("    skin: %d joints (HAnim ids %s), up to %d weights per vertex" % (
                len(m["skin"]["joints"]), m["skin"]["joints"][:8], m["skin"]["maxWeights"]))
    for a in model["animations"]:
        print("  animation L%dS%02d: %.4g s, %d tracks, %d keys" % (a["layer"], a["slot"], a["duration"], len(a["tracks"]),
                                                                 sum(len(k) for k in a["tracks"].values())))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))

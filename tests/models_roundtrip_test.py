"""Round trip through the runtime model cooker: RMD -> tools/models/rw_to_gltf.py -> model_cook_cli (the
src/platform/models cooker a [resource-model] declaration runs) -> cooked model, compared with the RMD it
came from: vertex positions in their frame's space, unit normals, UV sets, prelit colours, triangles per
material, textures (straight RGBA), material colours and names, and the frame each mesh hangs from.
Synthetic fixtures always; a sample of static weapons and field props when the user's disc is present
(P4PC_ISO). Also the refusals that name the exceeded bound, and the
content-hash cache. No retail bytes are stored.

    python tests/models_roundtrip_test.py <model_cook_cli> [unittest arguments]
    python tests/models_roundtrip_test.py --runner <runner arguments...> <model_cook_cli> -- [unittest arguments]
"""
import hashlib
import json
import math
import os
import struct
import subprocess
import sys
import tempfile
import unittest

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, "..", "tools", "models"))
sys.path.insert(0, HERE)
import gltfio  # noqa: E402
import models_fixture as fx  # noqa: E402
import p4disc  # noqa: E402
import p4mc  # noqa: E402
import ps2tex  # noqa: E402
import rw_to_gltf  # noqa: E402
import rwmodel  # noqa: E402

COOK = None
RUNNER = []
ISO = p4disc.default_iso()
WEAPON_KEY = "DATA/MODEL/WEAPON/WP001.RMD"
FIELD_KEY = "DATA/FIELD/GRMD/DOBJ044.RMD"


def cook(path, key, cache=None, env=None):
    """(exit code, stdout, stderr, cooked model or None)."""
    directory = tempfile.mkdtemp(prefix="p4-cook-")
    out = os.path.join(directory, "model.p4mc")
    args = RUNNER + [COOK, path, key, out] + ([cache] if cache else [])
    run = subprocess.run(args, capture_output=True, text=True, timeout=600, env=env)
    model = None
    if run.returncode == 0:
        with open(out, "rb") as stream:
            data = stream.read()
        model = p4mc.load(data)
        model["bytes"] = data
    return run.returncode, run.stdout, run.stderr, model


def export(data, name, glb=True):
    conv = rw_to_gltf.Converter(data, name, name, glb).run()
    directory = tempfile.mkdtemp(prefix="p4-rt-")
    return conv, conv.write(directory)[0]


def expected_meshes(rmd):
    """What the cooker must produce for each static atomic: frame, positions, normals, UVs, prelit,
    triangles (in rw_to_gltf's primitive order: by material, then file order), material slots."""
    clump = rmd.clump
    out = []
    for atomic in clump.atomics:
        geo = clump.geometries[atomic.geometry]
        frame = clump.frames[atomic.frame]
        if frame.hanim is not None:
            where = ("hanim", frame.hanim.node_id)
        elif frame.parent < 0:
            where = ("root", 0)
        else:
            return None  # a plain intermediate frame: the cooker bakes its transform (not an exact comparison)
        tris, slots = [], []
        for mi in range(len(geo.materials)):
            chosen = [t for t in geo.triangles if t[3] == mi and len(set(t[:3])) == 3]
            if chosen:
                slots.append(mi)
                tris.extend((t[0], t[1], t[2], len(slots) - 1) for t in chosen)
        target = geo.morph_targets[0]
        out.append({"frame": where, "positions": [tuple(p) for p in target.positions], "normals": target.normals,
                    "uvs": geo.uvs, "prelit": geo.prelit, "triangles": tris, "materials": [geo.materials[s] for s in slots]})
    return out


class Compare(unittest.TestCase):
    def compare(self, rmd, model, textures_by_name):
        want = expected_meshes(rmd)
        self.assertIsNotNone(want, "fixture choice: every atomic sits on a bone or the root")
        self.assertEqual(len(model["meshes"]), len(want))
        for got, exp in zip(model["meshes"], want):
            self.assertEqual(got["frame"], exp["frame"])
            self.assertEqual(got["positions"], exp["positions"])
            if exp["normals"] is not None:
                self.assertIsNotNone(got["normals"])
                for g, e in zip(got["normals"], exp["normals"]):
                    length = math.sqrt(sum(c * c for c in e))
                    unit = tuple(c / length for c in e) if length > 1e-12 else (0.0, 0.0, 1.0)
                    for a, b in zip(g, unit):
                        self.assertAlmostEqual(a, b, delta=1e-6)
            if exp["uvs"]:
                self.assertEqual(got["uv0"], [tuple(rwmodel.f32(c) for c in uv) for uv in exp["uvs"][0]])
            if exp["prelit"] is not None:
                self.assertEqual(got["colors"], [tuple(c) for c in exp["prelit"]])
            self.assertEqual(got["triangles"], exp["triangles"])
            self.assertEqual(len(got["materials"]), len(exp["materials"]))
            for index, mat in zip(got["materials"], exp["materials"]):
                cooked = model["materials"][index]
                self.assertEqual(cooked["color"], tuple(mat.color))
                self.assertEqual(cooked["name"], (mat.user_value("name") or (mat.texture.name if mat.texture else cooked["name"]))[:31])
                if mat.texture is not None and mat.texture.name.lower() in textures_by_name:
                    width, height, rgba = textures_by_name[mat.texture.name.lower()]
                    tex = model["textures"][cooked["texture"]]
                    self.assertEqual((tex["width"], tex["height"]), (width, height))
                    self.assertEqual(tex["rgba"], rgba)

    def compare_animations(self, rmd, model):
        """Every slot animation of the RMD, re-exported and cooked, streams back as the RtAnim chunk the
        builder feeds RtAnimAnimationStreamRead: the same keyframes in the same order with the same previous
        offsets, times, translations and original packed rotations, and the same duration and translation range."""
        ids = [node_id for _f, node_id, _fl in rmd.clump.hanim_frames()]
        retail = slot_animations(rmd)
        cooked = {"L%dS%02d" % (a["layer"], a["slot"]) for a in model["animations"]}
        self.assertEqual(cooked, set(retail))
        for name in sorted(retail):
            code, data, err = animation_stream(model, name, ids)
            self.assertEqual(code, 0, err)
            ours, want = parse_animation(data), retail[name]
            self.assertEqual((ours.version, ours.scheme, ours.flags, ours.duration), (want.version, want.scheme, want.flags, want.duration))
            self.assertEqual((ours.offset, ours.scale), (want.offset, want.scale))
            self.assertEqual(len(ours.keyframes), len(want.keyframes))
            for index, (got, exp) in enumerate(zip(ours.keyframes, want.keyframes)):
                self.assertEqual(got[0], exp[0])
                if index >= len(ids):  # a node's first keyframe has no previous one (retail leaves garbage)
                    self.assertEqual(got[3], exp[3])
                for axis in range(3):
                    if want.scale[axis] != 0.0:  # a zero scale makes the packed value unused
                        self.assertEqual(got[2][axis], exp[2][axis])
                if want.scheme == rwmodel.Animation.COMPRESSED:
                    self.assertEqual(ours.packed[index][1:5], want.packed[index][1:5])


class SyntheticRoundTrip(Compare):
    def test_static_weapon(self):
        data, _want = fx.static_weapon()
        rmd = rwmodel.Rmd(data)
        _conv, path = export(data, "weapon")
        code, out, err, model = cook(path, WEAPON_KEY)
        self.assertEqual(code, 0, err)
        textures = {}
        for tex in rmd.texdictionary.textures:
            w, h, rgba, _p = ps2tex.decode(tex)
            textures[tex.name.lower()] = (w, h, rgba)
        self.compare(rmd, model, textures)
        self.assertEqual(model["materials"][0]["drawCategory"], 2)
        tex = model["textures"][0]
        self.assertEqual(tex["depth"], 8)  # <= 256 colours: paletted, exactly
        self.assertEqual(tex["filterAddressing"], 0x1102)
        self.assertIn(" weapon ", out)

    def test_gltf_with_external_files_matches_glb(self):
        data, _want = fx.static_weapon()
        _c, glb = export(data, "weapon", glb=True)
        _c, gltf = export(data, "weapon", glb=False)
        a, b = cook(glb, WEAPON_KEY)[3], cook(gltf, WEAPON_KEY)[3]
        self.assertEqual(a["meshes"], b["meshes"])
        self.assertEqual([t["rgba"] for t in a["textures"]], [t["rgba"] for t in b["textures"]])

    def test_cache(self):
        data, _want = fx.static_weapon()
        _c, path = export(data, "weapon")
        cache = tempfile.mkdtemp(prefix="p4-cache-")
        first = cook(path, WEAPON_KEY, cache)
        second = cook(path, WEAPON_KEY, cache)
        self.assertTrue(first[1].strip().endswith("miss"), first)
        self.assertTrue(second[1].strip().endswith("hit"), second)
        self.assertEqual(first[3]["bytes"], second[3]["bytes"])
        source = first[3]["source"]
        self.assertTrue(os.path.isfile(os.path.join(cache, source + ".p4mc")))
        # A damaged cache file is a miss, never trusted.
        with open(os.path.join(cache, source + ".p4mc"), "r+b") as stream:
            stream.seek(200)
            stream.write(b"\xff" * 16)
        third = cook(path, WEAPON_KEY, cache)
        self.assertEqual(third[0], 0)
        self.assertTrue(third[1].strip().endswith("miss"), third)

    def test_skinned_mesh(self):
        directory = tempfile.mkdtemp(prefix="p4-skin-")
        path, positions, weights = gltf_skinned(directory)
        code, _out, err, model = cook(path, FIELD_KEY)
        self.assertEqual(code, 0, err)
        mesh = model["meshes"][0]
        skin = mesh["skin"]
        self.assertEqual(mesh["frame"], ("skin", 0))
        self.assertEqual(skin["joints"], [1, 2])
        self.assertEqual(skin["maxWeights"], 2)
        self.assertEqual([tuple(p) for p in mesh["positions"]], positions)  # skin space: no node transform baked
        self.assertEqual(skin["matrices"][0][3::4], (1, 2, 3, 4))  # extras.p4.matrixPads, the RwMatrix fourth column
        # The geometry chunk RpGeometryStreamRead reads, for a hierarchy whose nodes are HAnim ids 0, 2, 1.
        code, data, err = geometry_stream(model, 0, [0, 2, 1])
        self.assertEqual(code, 0, err)
        geo = parse_stream(data)
        s = geo.skin
        self.assertEqual((s.num_bones, s.used_bones, s.max_weights), (3, [1, 2], 2))
        node_of_joint = {0: 2, 1: 1}
        for v, (w_in, joints_in) in enumerate(zip(weights, [mesh["skin"]["slots"][v] for v in range(len(weights))])):
            got = sorted((s.indices[4 * v + k], s.weights[4 * v + k]) for k in range(4) if s.weights[4 * v + k] > 0)
            want = sorted((node_of_joint[joints_in[k]], struct.unpack("<f", struct.pack("<f", w_in[k]))[0])
                          for k in range(4) if w_in[k] > 0)
            self.assertEqual(got, want)
        self.assertEqual(s.matrices[0], tuple(float(x) for x in (1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0)))
        self.assertEqual(s.matrices[1][12], -20.0)  # node 1 is HAnim id 2: the second joint's inverse bind matrix
        self.assertEqual(s.matrix_pads[2], (1, 2, 3, 4))
        self.assertEqual(s.tail, b"\0" * 12)  # not split
        self.assertEqual(geo.extension.ids(), [0x50E, 0x116])
        self.assertEqual(len(geo.materials), 1)
        self.assertEqual(geo.materials[0].color, (128, 64, 255, 255))
        self.assertEqual([tuple(p) for p in geo.morph_targets[0].positions], positions)
        # A hierarchy without one of the weighted bones refuses the mesh: the model keeps its retail look.
        code, _data, err = geometry_stream(model, 0, [0, 1])
        self.assertEqual(code, 1)
        self.assertIn("HAnim node 2", err)

    def test_skinned_actor_matches_its_rmd(self):
        """A synthetic skinned RMD (models_fixture.skinned_actor) through rw_to_gltf and the cooker: the geometry
        chunk the builder reads carries the RMD's skin extension byte for byte except the split data (left
        unsplit) and zero weights' bone indices (unread), and the same vertices."""
        data, want = fx.skinned_actor()
        rmd = rwmodel.Rmd(data)
        geo = rmd.clump.geometries[0]
        _conv, path = export(data, "actor")
        code, _out, err, model = cook(path, "DATA/MODEL/NPC/N000.RMD")
        self.assertEqual(code, 0, err)
        skinned = [i for i, m in enumerate(model["meshes"]) if m["skin"]]
        self.assertEqual(len(skinned), 1)
        self.assertEqual(model["meshes"][skinned[0]]["frame"], ("skin", 0))
        code, stream, err = geometry_stream(model, skinned[0], [node for node, _i, _f in want["nodes"]])
        self.assertEqual(code, 0, err)
        got = parse_stream(stream)
        have, expected = bytearray(got.extension.raw(0x116)[:-12]), bytearray(geo.extension.raw(0x116)[:-len(geo.skin.tail)])
        start = 4 + len(geo.skin.used_bones)
        for slot, weight in enumerate(geo.skin.weights):
            if weight == 0.0:
                have[start + slot] = expected[start + slot] = 0
        self.assertEqual(have, expected)
        self.assertEqual(got.morph_targets[0].positions, geo.morph_targets[0].positions)
        self.assertEqual(sorted(t for t in got.triangles), sorted(t for t in geo.triangles))

    def test_actor_animation_matches_its_rmd(self):
        """The synthetic actor's slot animation through rw_to_gltf, the cooker and the RtAnim chunk writer; a
        hierarchy node the glTF has no bone for cannot be driven (the slot keeps the file's animation)."""
        data, want = fx.skinned_actor()
        rmd = rwmodel.Rmd(data)
        _conv, path = export(data, "actor")
        code, _out, err, model = cook(path, "DATA/MODEL/NPC/N000.RMD")
        self.assertEqual(code, 0, err)
        self.compare_animations(rmd, model)
        ids = [node for node, _i, _f in want["nodes"]]
        code, _data, err = animation_stream(model, "L0S00", ids + [13])
        self.assertEqual(code, 1)
        self.assertIn("no track for HAnim node 13", err)

    def test_edited_rotation_does_not_use_retail_snapshot(self):
        data, _want = fx.skinned_actor()
        _conv, path = export(data, "edited")
        doc, binary, _base = gltfio.load(path)
        animation = doc["animations"][0]
        channel = next(ch for ch in animation["channels"] if ch["target"]["path"] == "rotation")
        sampler = animation["samplers"][channel["sampler"]]
        accessor = doc["accessors"][sampler["output"]]
        view = doc["bufferViews"][accessor["bufferView"]]
        binary = bytearray(binary)
        struct.pack_into("<4f", binary, view.get("byteOffset", 0) + accessor.get("byteOffset", 0), 0.0, 0.0, 1.0, 0.0)
        builder = gltfio.Builder()
        builder.doc, builder.bin = doc, binary
        with open(path, "wb") as stream:
            stream.write(builder.glb())
        code, _out, err, model = cook(path, "DATA/MODEL/NPC/N000.RMD")
        self.assertEqual(code, 0, err)
        node = doc["nodes"][channel["target"]["node"]]["name"]
        track = model["animations"][0]["tracks"][int(node.split("_")[1])]
        self.assertEqual(tuple(rwmodel.decompress_float(v) for v in track[0][1]), (0.0, 0.0, 1.0, 0.0))

    def test_animation_extras_round_to_nearest_under_ee_mode(self):
        """Retail ranges survive decimal JSON parsing even when the engine uses round-toward-zero."""
        data, _want = fx.skinned_actor()
        _conv, path = export(data, "rounding")
        doc, binary, _base = gltfio.load(path)
        offset = [9.268978118896484, 47.12909698486328, 14.849189758300781]
        scale = [49.90690231323242, 72.12495422363281, 95.32593536376953]
        doc["animations"][0]["extras"]["p4"]["translationOffset"] = offset
        doc["animations"][0]["extras"]["p4"]["translationScale"] = scale
        builder = gltfio.Builder()
        builder.doc, builder.bin = doc, bytearray(binary)
        with open(path, "wb") as stream:
            stream.write(builder.glb())
        env = dict(os.environ, P4_MODEL_TEST_ROUND_ZERO="1")
        code, _out, err, model = cook(path, "DATA/MODEL/NPC/N000.RMD", env=env)
        self.assertEqual(code, 0, err)
        anim = model["animations"][0]
        for actual, expected in zip(anim["translationOffset"] + anim["translationScale"], offset + scale):
            self.assertEqual(struct.pack("<f", actual), struct.pack("<f", expected))

    def test_authored_animation(self):
        """A glTF animation as Blender writes one. L0S02 turns bone_2 (LINEAR rotation keys at 0, 0.5 and 1)
        and steps bone_1 (STEP translation keys at 0 and 0.25) with a unit scale channel; bone_1 keeps its
        rest rotation and bone_2 its rest translation, and every track spans 0..1. "Idle" is not a slot."""
        half = math.sqrt(0.5)
        turn = [(0.0, 0.0, 0.0, 1.0), (0.0, 0.0, half, half), (0.0, 0.0, 1.0, 0.0)]
        animations = [("L0S02", [(2, "rotation", (0.0, 0.5, 1.0), turn, "LINEAR"),
                                 (1, "translation", (0.0, 0.25), [(0.0, 0.0, 0.0), (0.0, 2.0, 0.0)], "STEP"),
                                 (1, "scale", (0.0, 1.0), [(1.0, 1.0, 1.0)] * 2, "LINEAR")]),
                      ("Idle", [(2, "rotation", (0.0, 1.0), turn[:2], "LINEAR")])]
        path, _positions, _weights = gltf_skinned(tempfile.mkdtemp(prefix="p4-anim-"), animations=animations)
        code, _out, err, model = cook(path, FIELD_KEY)
        self.assertEqual(code, 0, err)
        self.assertEqual(len(model["animations"]), 1)
        anim = model["animations"][0]
        self.assertEqual((anim["layer"], anim["slot"], anim["duration"], sorted(anim["tracks"])), (0, 2, 1.0, [1, 2]))
        unpack = rwmodel.decompress_float

        def decoded(track):
            return [(t, tuple(unpack(v) for v in rotation),
                     tuple(unpack(v) * s + o for v, s, o in zip(translation, anim["translationScale"], anim["translationOffset"])))
                    for t, rotation, translation in anim["tracks"][track]]
        bone_1, bone_2 = decoded(1), decoded(2)
        self.assertEqual([key[0] for key in bone_1], [0.0, 0.25, 1.0])
        self.assertEqual([key[0] for key in bone_2], [0.0, 0.5, 1.0])
        for (_t, rotation, translation), held in zip(bone_1, [(0.0, 0.0, 0.0), (0.0, 2.0, 0.0), (0.0, 2.0, 0.0)]):
            for got, exp in zip(rotation + translation, (0.0, 0.0, 0.0, 1.0) + held):
                self.assertAlmostEqual(got, exp, delta=0.01)
        for (_t, rotation, translation), exp_rotation in zip(bone_2, turn):
            for got, exp in zip(rotation + translation, exp_rotation + (20.0, 0.0, 0.0)):
                self.assertAlmostEqual(got, exp, delta=0.01)
        # RtAnim order for a hierarchy of HAnim ids 1, 2: both first keys, both second keys, then bone_1's last
        # (needed once 0.25 passes) before bone_2's (once 0.5 passes); previous offsets count 24-byte keyframes.
        code, data, err = animation_stream(model, "L0S02", [1, 2])
        self.assertEqual(code, 0, err)
        stream = parse_animation(data)
        self.assertEqual([key[0] for key in stream.keyframes], [0.0, 0.0, 0.25, 0.5, 1.0, 1.0])
        self.assertEqual([prev for _f, _f, _f, _f, _f, _f, _f, _f, prev in stream.packed][2:], [0, 24, 48, 72])
        self.assertEqual(stream.duration, 1.0)


def gltf_quad(directory, name="quad", texture_size=(16, 16), mime="image/png", triangles=2, image_file=None,
              extras=None):
    """A minimal static glTF: a quad grid on node bone_0, one textured material."""
    b = gltfio.Builder()
    columns = max(1, triangles // 2)
    positions, uvs, indices = [], [], []
    for c in range(columns + 1):
        positions += [(float(c), 0.0, 0.0), (float(c), 1.0, 0.0)]
        uvs += [(c / columns, 1.0), (c / columns, 0.0)]
    for c in range(columns):
        v = 2 * c
        indices += [v, v + 2, v + 1, v + 1, v + 2, v + 3]
    attrs = {"POSITION": b.accessor(positions, gltfio.FLOAT, "VEC3", minmax=True),
             "TEXCOORD_0": b.accessor(uvs, gltfio.FLOAT, "VEC2")}
    width, height = texture_size
    rgba = b"".join(bytes((x * 13 & 255, y * 7 & 255, 99, 255)) for y in range(height) for x in range(width))
    png = gltfio.png_rgba(width, height, rgba)
    image = {"mimeType": mime}
    if image_file:
        image = {"uri": image_file}
    else:
        image["bufferView"] = b.view(png)
    b.add("images", image)
    b.add("samplers", {"magFilter": 9729, "wrapS": 10497, "wrapT": 10497})
    b.add("textures", {"source": 0, "sampler": 0})
    material = {"name": "quad_mat", "pbrMetallicRoughness": {"baseColorTexture": {"index": 0}}}
    if extras:
        material["extras"] = extras
    b.add("materials", material)
    b.add("meshes", {"primitives": [{"attributes": attrs, "material": 0,
                                      "indices": b.accessor(indices, gltfio.UNSIGNED_INT, "SCALAR")}]})
    b.add("nodes", {"name": "bone_0", "mesh": 0})
    b.add("scenes", {"nodes": [0]})
    b.doc["scene"] = 0
    path = os.path.join(directory, name + ".glb")
    with open(path, "wb") as stream:
        stream.write(b.glb())
    return path, rgba


def gltf_skinned(directory, joint_names=("bone_1", "bone_2"), skin=True, name="skinned", animations=()):
    """models_fixture.skinned_gltf written to <directory>/<name>.glb: (path, positions, weights)."""
    glb, positions, weights = fx.skinned_gltf(joint_names, skin, animations)
    path = os.path.join(directory, name + ".glb")
    with open(path, "wb") as stream:
        stream.write(glb)
    return path, positions, weights


def chunk_stream(model, mode, which, node_ids):
    """(exit code, chunk bytes, stderr) of model_cook_cli --stream (skinned mesh `which`) or --anim-stream
    (slot animation `which`, "L<layer>S<slot>") for a cooked model."""
    directory = tempfile.mkdtemp(prefix="p4-stream-")
    cooked, out = os.path.join(directory, "model.p4mc"), os.path.join(directory, "chunk.bin")
    with open(cooked, "wb") as stream:
        stream.write(model["bytes"])
    run = subprocess.run(RUNNER + [COOK, mode, cooked, str(which), ",".join(map(str, node_ids)), out], capture_output=True,
                         text=True, timeout=600)
    data = None
    if run.returncode == 0:
        with open(out, "rb") as stream:
            data = stream.read()
    return run.returncode, data, run.stderr


def geometry_stream(model, mesh, node_ids):
    return chunk_stream(model, "--stream", mesh, node_ids)


def animation_stream(model, slot, node_ids):
    return chunk_stream(model, "--anim-stream", slot, node_ids)


def parse_stream(data):
    return rwmodel.parse_geometry(data, rwmodel.chunk_list(data, 0, len(data))[0], "geometry stream")


def parse_animation(data):
    kind, size, _stamp = struct.unpack_from("<3I", data, 0)
    assert kind == rwmodel.ANIMANIMATION and size == len(data) - 12
    return rwmodel.Animation(data, 12, len(data), "animation stream")


def slot_animations(rmd):
    """{"L<layer>S<slot>": the RMD's animation chunk for that slot} (rw_to_gltf's names)."""
    anims = [chunk.obj for chunk in rmd.chunks if chunk.kind == rwmodel.ANIMANIMATION]
    return {"L%dS%02d" % (e["layer"], e["slot"]): anims[e["animation"]] for e in rmd.slots() if e["animation"] is not None}


class Refusals(unittest.TestCase):
    def setUp(self):
        self.dir = tempfile.mkdtemp(prefix="p4-refuse-")

    def refused(self, path, key, *fragments):
        code, _out, err, _model = cook(path, key)
        self.assertEqual(code, 1, err)
        for fragment in fragments:
            self.assertIn(fragment, err)
        self.assertIn(os.path.basename(path), err)  # the message names the file
        return err

    def test_accepts_the_minimal_model(self):
        path, rgba = gltf_quad(self.dir, extras={"p4": {"drawCategory": 3, "fogDisable": True, "tint": "ff102030"}})
        code, _out, err, model = cook(path, WEAPON_KEY)
        self.assertEqual(code, 0, err)
        self.assertEqual(model["meshes"][0]["frame"], ("hanim", 0))
        self.assertEqual(model["textures"][0]["rgba"], rgba)
        m = model["materials"][0]
        self.assertEqual((m["drawCategory"], m["flags"] & 3, m["tint"]), (3, 3, 0xFF102030))

    def test_malformed(self):
        path, _ = gltf_quad(self.dir)
        with open(path, "rb") as stream:
            data = stream.read()
        truncated = os.path.join(self.dir, "truncated.glb")
        with open(truncated, "wb") as stream:
            stream.write(data[:len(data) // 2])
        self.refused(truncated, WEAPON_KEY, "cannot parse")

    def test_oversize_texture(self):
        path, _ = gltf_quad(self.dir, texture_size=(512, 512))
        self.refused(path, WEAPON_KEY, "512x512", "at most 256", "weapon")

    def test_texture_budget_follows_the_target(self):
        path, _ = gltf_quad(self.dir, texture_size=(512, 512))
        self.assertEqual(cook(path, FIELD_KEY)[0], 0)

    def test_too_many_triangles(self):
        path, _ = gltf_quad(self.dir, triangles=4200)
        self.refused(path, WEAPON_KEY, "4200 triangles", "at most 4096")

    def test_not_power_of_two(self):
        path, _ = gltf_quad(self.dir, texture_size=(24, 16))
        self.refused(path, WEAPON_KEY, "24x16", "powers of two")

    def test_jpeg(self):
        path, _ = gltf_quad(self.dir, mime="image/jpeg")
        self.refused(path, WEAPON_KEY, "image/jpeg", "only PNG")

    def test_skinned_joint_not_a_bone(self):
        path, _p, _w = gltf_skinned(self.dir, joint_names=("bone_1", "armature_end"))
        self.refused(path, FIELD_KEY, "armature_end", "not an HAnim bone")

    def test_weights_without_skin(self):
        path, _p, _w = gltf_skinned(self.dir, skin=False)
        self.refused(path, FIELD_KEY, "JOINTS_0/WEIGHTS_0", "no skin")

    def test_cubic_spline_animation(self):
        keys = [(0.0, 0.0, 0.0, 1.0)] * 6  # in-tangent, value, out-tangent per key
        path, _p, _w = gltf_skinned(self.dir, animations=[("L0S00", [(2, "rotation", (0.0, 1.0), keys, "CUBICSPLINE")])])
        self.refused(path, FIELD_KEY, "animation L0S00 uses CUBICSPLINE")

    def test_scaling_animation(self):
        scales = [(1.0, 1.0, 1.0), (2.0, 2.0, 2.0)]
        path, _p, _w = gltf_skinned(self.dir, animations=[("L0S00", [(2, "scale", (0.0, 1.0), scales, "LINEAR")])])
        self.refused(path, FIELD_KEY, "animation L0S00 scales bone_2", "rotation and translation only")

    def test_two_animations_for_one_slot(self):
        still = [(2, "rotation", (0.0, 1.0), [(0.0, 0.0, 0.0, 1.0)] * 2, "LINEAR")]
        path, _p, _w = gltf_skinned(self.dir, animations=[("L0S01", still), ("L0S01", still)])
        self.refused(path, FIELD_KEY, "two animations replace layer 0 slot 1")

    def test_missing_external_image(self):
        path, _ = gltf_quad(self.dir, image_file="absent.png")
        self.refused(path, WEAPON_KEY, "absent.png", "missing")


STATIC_SAMPLE = ["DATA/MODEL/WEAPON/WP001.RMD", "DATA/MODEL/WEAPON/WP00A.RMD", "DATA/MODEL/WEAPON/WP027_R.RMD",
                 "DATA/MODEL/WEAPON/WP0A0.RMD", FIELD_KEY, "DATA/FIELD/GRMD/FOBJ101.RMD"]
# A Persona (85 bones, split skin data), a facility person, a multi-atomic NPC and a Shadow symbol.
SKINNED_SAMPLE = ["BTL/MODEL_BTL/PERSONA/PS000.RMD", "DATA/MODEL/FACILITYP/FP0B4.RMD", "DATA/MODEL/NPC2/N10801_2.RMD",
                  "DATA/MODEL/SYMBOL/ES001.RMD"]


@unittest.skipUnless(ISO, "no disc image (P4PC_ISO)")
class DiscRoundTrip(Compare):
    @classmethod
    def setUpClass(cls):
        cls.disc = p4disc.Disc(ISO)

    @classmethod
    def tearDownClass(cls):
        cls.disc.close()

    def test_static_sample(self):
        for key in STATIC_SAMPLE:
            with self.subTest(key=key):
                data = self.disc.read(key)
                rmd = rwmodel.Rmd(data)
                _conv, path = export(data, rw_to_gltf.model_name(key))
                code, _out, err, model = cook(path, key)
                self.assertEqual(code, 0, err)
                textures = {}
                for tex in rmd.texdictionary.textures:
                    w, h, rgba, _p = ps2tex.decode(tex)
                    textures[tex.name.lower()] = (w, h, rgba)
                self.compare(rmd, model, textures)
                self.assertEqual(model["triangles"], sum(len(m["triangles"]) for m in model["meshes"]))
                if rmd.clump.hanim_frames():
                    self.compare_animations(rmd, model)

    def test_skinned_sample(self):
        """Each skinned atomic re-exported and cooked gives the geometry chunk whose skin extension is the
        retail one byte for byte (bone count, used bones in retail order, bone indices, weights, skin-to-bone
        matrices with their pads) up to the split data, which the stream leaves unsplit, and the bone index of
        a zero weight, which nothing reads (rw_to_gltf writes joint 0 there); and the same vertices. Their slot
        animations come back as compare_animations describes."""
        for key in SKINNED_SAMPLE:
            with self.subTest(key=key):
                data = self.disc.read(key)
                rmd = rwmodel.Rmd(data)
                clump = rmd.clump
                ids = [node_id for _f, node_id, _fl in clump.hanim_frames()]
                retail = [clump.geometries[a.geometry] for a in clump.atomics if clump.geometries[a.geometry].skin is not None]
                _conv, path = export(data, rw_to_gltf.model_name(key))
                code, _out, err, model = cook(path, key)
                self.assertEqual(code, 0, err)
                cooked = [i for i, m in enumerate(model["meshes"]) if m["skin"]]
                self.assertEqual(len(cooked), len(retail))
                for index, geo in zip(cooked, retail):
                    code, stream, err = geometry_stream(model, index, ids)
                    self.assertEqual(code, 0, err)
                    got = parse_stream(stream)
                    split = len(geo.skin.tail)
                    want, have = bytearray(geo.extension.raw(0x116)[:-split]), bytearray(got.extension.raw(0x116)[:-12])
                    start = 4 + len(geo.skin.used_bones)
                    for slot, weight in enumerate(geo.skin.weights):
                        if weight == 0.0:
                            want[start + slot] = have[start + slot] = 0
                    self.assertEqual(have, want)
                    self.assertEqual(got.morph_targets[0].positions, geo.morph_targets[0].positions)
                    self.assertEqual(got.num_vertices, geo.num_vertices)
                self.compare_animations(rmd, model)


if __name__ == "__main__":
    if len(sys.argv) < 2:
        sys.exit("usage: models_roundtrip_test.py <model_cook_cli> [unittest arguments]")
    if sys.argv[1] == "--runner":
        try:
            end = sys.argv.index("--", 2)
        except ValueError:
            sys.exit("--runner must end with --")
        command = sys.argv[2:end]
        if not command:
            sys.exit("--runner needs a model_cook_cli command")
        RUNNER, COOK = command[:-1], command[-1]
        del sys.argv[1:end + 1]
    else:
        COOK = sys.argv.pop(1)
    unittest.main()

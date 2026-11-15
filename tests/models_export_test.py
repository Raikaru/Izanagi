"""tools/models: PS2 texture codecs, the RMD parser and rw_to_gltf on synthetic fixtures, plus (when the
user's disc is present via P4PC_ISO) a sample of weapons, characters, enemies
and field props. No retail bytes are stored; disc checks compare against values the converter must
reproduce from the parsed source."""
import json
import math
import os
import struct
import sys
import tempfile
import unittest
import zlib
from unittest.mock import patch

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, "..", "tools", "models"))
sys.path.insert(0, HERE)
import gltfio  # noqa: E402
import models_fixture as fx  # noqa: E402
import p4disc  # noqa: E402
import ps2tex  # noqa: E402
import rw_to_gltf  # noqa: E402
import rwmodel  # noqa: E402


def export(data, name, glb=False):
    conv = rw_to_gltf.Converter(data, name, name, glb).run()
    directory = tempfile.mkdtemp(prefix="p4-models-")
    paths = conv.write(directory)
    doc, binary, base = gltfio.load(paths[0])
    with open(paths[1], encoding="utf-8") as stream:
        sidecar = json.load(stream)
    return conv, doc, binary, base, sidecar


def check_document(test, doc, binary):
    """Structural glTF checks: views inside the buffer, accessors inside views, indices inside their
    vertex count, min/max of POSITION, unit quaternions, increasing animation times."""
    test.assertEqual(doc["asset"]["version"], "2.0")
    for view in doc.get("bufferViews", []):
        test.assertLessEqual(view.get("byteOffset", 0) + view["byteLength"], len(binary))
    for acc in doc.get("accessors", []):
        view = doc["bufferViews"][acc["bufferView"]]
        size = gltfio.COMPONENT[acc["componentType"]][1] * gltfio.WIDTH[acc["type"]]
        test.assertLessEqual(acc.get("byteOffset", 0) + size * acc["count"], view["byteLength"])
    for mesh in doc.get("meshes", []):
        for prim in mesh["primitives"]:
            count = doc["accessors"][prim["attributes"]["POSITION"]]["count"]
            indices = gltfio.read_accessor(doc, binary, prim["indices"])
            test.assertEqual(len(indices) % 3, 0)
            test.assertLess(max(indices), count)
            pos = gltfio.read_accessor(doc, binary, prim["attributes"]["POSITION"])
            acc = doc["accessors"][prim["attributes"]["POSITION"]]
            for axis in range(3):
                test.assertEqual(acc["min"][axis], min(p[axis] for p in pos))
                test.assertEqual(acc["max"][axis], max(p[axis] for p in pos))
            if "WEIGHTS_0" in prim["attributes"]:
                for w in gltfio.read_accessor(doc, binary, prim["attributes"]["WEIGHTS_0"]):
                    test.assertAlmostEqual(sum(w), 1.0, places=5)
    for anim in doc.get("animations", []):
        for sampler in anim["samplers"]:
            times = gltfio.read_accessor(doc, binary, sampler["input"])
            test.assertTrue(all(a < b for a, b in zip(times, times[1:])), "times must increase")
        for channel in anim["channels"]:
            if channel["target"]["path"] == "rotation":
                for q in gltfio.read_accessor(doc, binary, anim["samplers"][channel["sampler"]]["output"]):
                    test.assertAlmostEqual(math.sqrt(sum(c * c for c in q)), 1.0, places=5)


def image_rgba(doc, binary, base, index):
    image = doc["images"][index]
    if "uri" in image:
        with open(os.path.join(base, image["uri"]), "rb") as stream:
            return gltfio.png_decode_rgba(stream.read())
    view = doc["bufferViews"][image["bufferView"]]
    start = view.get("byteOffset", 0)
    return gltfio.png_decode_rgba(binary[start:start + view["byteLength"]])


class DiscSelection(unittest.TestCase):
    def test_environment_selects_existing_disc(self):
        with tempfile.TemporaryDirectory() as directory:
            path = os.path.join(directory, "disc.iso")
            with patch.dict(os.environ, {"P4PC_ISO": path}):
                self.assertIsNone(p4disc.default_iso())
                with open(path, "wb"):
                    pass
                self.assertEqual(p4disc.default_iso(), path)
            with patch.dict(os.environ, {}, clear=True):
                self.assertIsNone(p4disc.default_iso())


class TextureCodecs(unittest.TestCase):
    def roundtrip(self, w, h, depth):
        entries = 256 if depth == 8 else 16
        palette = [((i * 37) & 255, (i * 91) & 255, (i * 13) & 255, 255 if i % 3 else 128) for i in range(entries)]
        indices = bytes(((x * 7 + y * 3) ^ (x >> 2)) % entries for y in range(h) for x in range(w))
        data = ps2tex.encode_paletted(indices, palette, w, h, depth)
        tex = rwmodel.NativeTexture()
        tex.name, tex.data = "t", data
        tex.header = (w, h, depth, (2 << 16) | (0x2504 if depth == 8 else 0x4504)) + (0,) * 12
        width, height, rgba, decoded = ps2tex.decode(tex)
        self.assertEqual((width, height), (w, h))
        for i, (r, g, b, a) in enumerate(palette):
            self.assertEqual(decoded[i], (r, g, b, 255 if a >= 255 else ps2tex.ps2_alpha(ps2tex.rgba_alpha_to_ps2(a))))
        for i, index in enumerate(indices):
            self.assertEqual(tuple(rgba[4 * i:4 * i + 3]), palette[index][:3])

    def test_psmt8(self):
        for w, h in ((16, 16), (32, 64), (64, 128), (128, 128), (256, 32)):
            self.roundtrip(w, h, 8)

    def test_psmt4(self):
        for w, h in ((32, 16), (64, 64), (128, 64), (128, 256)):
            self.roundtrip(w, h, 4)

    def test_rgba32_alpha(self):
        rgba = b"".join(bytes((x, y, 7, 255 if x % 2 else 128)) for y in range(8) for x in range(8))
        tex = rwmodel.NativeTexture()
        tex.name, tex.data = "t", ps2tex.encode_rgba32(rgba, 8, 8)
        tex.header = (8, 8, 32, (2 << 16) | 0x0504) + (0,) * 12
        _w, _h, out, _p = ps2tex.decode(tex)
        self.assertEqual(out[0:4], bytes((0, 0, 7, 128)))   # 128 -> PS2 64 -> doubled back to 128
        self.assertEqual(out[4:8], bytes((1, 0, 7, 255)))

    def test_float_decompression(self):
        self.assertEqual(rwmodel.decompress_float(0x7800), 1.0)
        self.assertEqual(rwmodel.decompress_float(0x0000), 0.0)
        self.assertEqual(rwmodel.decompress_float(0xF800), -1.0)
        for value in (0.5, 0.25, 1.5, 0.70703125, -0.125):
            self.assertEqual(rwmodel.decompress_float(fx.compress_float(value)), value)


class StaticWeapon(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.data, cls.want = fx.static_weapon()
        cls.conv, cls.doc, cls.bin, cls.base, cls.side = export(cls.data, "weapon")

    def test_structure(self):
        check_document(self, self.doc, self.bin)
        self.assertEqual(len(self.doc["nodes"]), 2)
        self.assertEqual(self.doc["scenes"][0]["nodes"], [0])
        self.assertEqual(self.doc["nodes"][0]["children"], [1])
        self.assertEqual(self.doc["nodes"][0]["translation"], [0.5, -1.0, 2.0])
        self.assertEqual(self.doc["nodes"][1]["name"], "bone_0")
        self.assertEqual(self.doc["nodes"][1]["mesh"], 0)

    def test_vertices_exact(self):
        prim = self.doc["meshes"][0]["primitives"][0]
        attrs = prim["attributes"]
        read = lambda name: gltfio.read_accessor(self.doc, self.bin, attrs[name])
        self.assertEqual(read("POSITION"), [tuple(p) for p in self.want["positions"]])
        self.assertEqual(read("TEXCOORD_0"), [tuple(rwmodel.f32(c) for c in uv) for uv in self.want["uvs"]])
        self.assertEqual(read("COLOR_0"), [tuple(c) for c in self.want["prelit"]])
        self.assertTrue(self.doc["accessors"][attrs["COLOR_0"]]["normalized"])
        for got, want in zip(read("NORMAL"), self.want["normals"]):
            for a, b in zip(got, want):
                self.assertAlmostEqual(a, b, places=6)
        indices = gltfio.read_accessor(self.doc, self.bin, prim["indices"])
        self.assertEqual(indices, [v for t in self.want["triangles"] if len(set(t[:3])) == 3 for v in t[:3]])

    def test_material_and_texture(self):
        mat = self.doc["materials"][0]
        self.assertEqual(mat["name"], "blade_mat")
        self.assertEqual(mat["pbrMetallicRoughness"]["baseColorFactor"], [200 / 255, 150 / 255, 100 / 255, 1.0])
        self.assertEqual(mat["alphaMode"], "BLEND")  # the palette has a half-transparent entry
        self.assertEqual(mat["extras"]["p4"]["drawCategory"], 2)
        self.assertEqual(mat["extras"]["p4"]["lighting"], "lit")
        self.assertNotIn("extensions", mat)
        tex = self.doc["textures"][mat["pbrMetallicRoughness"]["baseColorTexture"]["index"]]
        sampler = self.doc["samplers"][tex["sampler"]]
        self.assertEqual((sampler["magFilter"], sampler["wrapS"], sampler["wrapT"]), (9729, 10497, 10497))
        w, h, rgba = image_rgba(self.doc, self.bin, self.base, tex["source"])
        size = self.want["texture_size"]
        self.assertEqual((w, h), (size, size))
        for i, index in enumerate(self.want["indices"]):
            r, g, b, a = self.want["palette"][index]
            self.assertEqual(tuple(rgba[4 * i:4 * i + 4]), (r, g, b, ps2tex.ps2_alpha(ps2tex.rgba_alpha_to_ps2(a))))

    def test_sidecar(self):
        side = self.side
        self.assertEqual(side["format"], "p4model-sidecar")
        self.assertEqual([c["name"] for c in side["chunks"]][:4], ["rmd slot count", "texture dictionary", "clump", "rmd attach table"])
        self.assertEqual(side["chunks"][0]["slots"], 3)
        self.assertEqual(side["attach"], [{"id": 100, "hanimId": 0, "matrix": [1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0]}])
        slots = side["slots"]
        self.assertEqual([(s["slot"], s["animation"], s["alias"]) for s in slots], [(0, 0, None), (1, None, 0), (2, 1, None)])
        self.assertEqual([s["gltfAnimation"] for s in slots if s["animation"] is not None], [0, 1])
        self.assertEqual(side["source"]["sha256"], __import__("hashlib").sha256(self.data).hexdigest())
        self.assertEqual(side["textures"][0]["rasterFlags"], "0x2504")

    def test_glb_matches_gltf(self):
        _conv, doc, binary, _base, _side = export(self.data, "weapon", glb=True)
        prim = doc["meshes"][0]["primitives"][0]
        self.assertEqual(gltfio.read_accessor(doc, binary, prim["attributes"]["POSITION"]),
                         gltfio.read_accessor(self.doc, self.bin, self.doc["meshes"][0]["primitives"][0]["attributes"]["POSITION"]))
        self.assertEqual(doc["images"][0]["mimeType"], "image/png")


class SkinnedActor(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.data, cls.want = fx.skinned_actor()
        cls.conv, cls.doc, cls.bin, cls.base, cls.side = export(cls.data, "actor")

    def test_skin(self):
        check_document(self, self.doc, self.bin)
        skin = self.doc["skins"][0]
        names = [self.doc["nodes"][j]["name"] for j in skin["joints"]]
        self.assertEqual(names, ["bone_10", "bone_11", "bone_12"])
        ibm = gltfio.read_accessor(self.doc, self.bin, skin["inverseBindMatrices"])
        self.assertEqual([list(m) for m in ibm], [list(m) for m in self.want["matrices"]])
        node = [n for n in self.doc["nodes"] if "skin" in n][0]
        self.assertEqual(node["extras"]["p4"]["atomic"]["frame"], 1)
        self.assertIn(self.doc["nodes"].index(node), self.doc["scenes"][0]["nodes"])
        prim = self.doc["meshes"][node["mesh"]]["primitives"][0]
        joints = gltfio.read_accessor(self.doc, self.bin, prim["attributes"]["JOINTS_0"])
        weights = gltfio.read_accessor(self.doc, self.bin, prim["attributes"]["WEIGHTS_0"])
        self.assertEqual(weights, [tuple(w) for w in self.want["weights"]])
        for j, w, wj in zip(joints, weights, self.want["indices"]):
            self.assertEqual(j, tuple(a if b > 0 else 0 for a, b in zip(wj, w)))

    def test_helper_and_hierarchy(self):
        helper = self.doc["nodes"][3]["extras"]["p4"]
        self.assertEqual(helper["helperId"], 500)
        self.assertEqual(helper["hanimId"], 12)
        root = self.doc["nodes"][1]["extras"]["p4"]["hierarchy"]
        self.assertEqual(root["nodes"], [list(n) for n in self.want["nodes"]])

    def test_animation_tracks(self):
        anim = self.doc["animations"][0]
        by_node = {}
        for channel in anim["channels"]:
            sampler = anim["samplers"][channel["sampler"]]
            by_node.setdefault(self.doc["nodes"][channel["target"]["node"]]["name"], {})[channel["target"]["path"]] = (
                gltfio.read_accessor(self.doc, self.bin, sampler["input"]),
                gltfio.read_accessor(self.doc, self.bin, sampler["output"]))
        times, rots = by_node["bone_10"]["rotation"]
        self.assertEqual(times, [0.0, 0.5, 1.0])
        times, rots = by_node["bone_11"]["rotation"]
        self.assertEqual(times, [0.0, 1.0])
        n = math.sqrt(2 * 0.70703125 ** 2)
        self.assertAlmostEqual(rots[1][2], 0.70703125 / n, places=6)
        self.assertAlmostEqual(rots[1][3], 0.70703125 / n, places=6)
        _times, trans = by_node["bone_12"]["translation"]
        self.assertEqual(trans, [(0.0, 1.0, 0.0), (0.0, 1.0, 0.0)])
        self.assertEqual(anim["extras"]["p4"]["scheme"], 2)


ISO = p4disc.default_iso()
SAMPLE = {  # spec -> what the source must show (category)
    "DATA/MODEL/WEAPON/WP001.RMD": "weapon",
    "DATA/MODEL/WEAPON/WP027_R.RMD": "weapon",
    "DATA/MODEL/PACK/BC001.PAC#bc001.RMD": "party",
    "DATA/MODEL/PACK/WP070.PAC#wp070.RMD": "party",
    "BTL/MODEL_BTL/PACK/EM000.PAC#em000.RMD": "enemy",
    "BTL/MODEL_BTL/PERSONA/PS00F.RMD": "persona",
    "DATA/MODEL/NPC/N101_1.RMD": "npc",
    "DATA/FIELD/GRMD/DOBJ044.RMD": "field prop",
    "DATA/FIELD/RMD/M001_003.RMD": "field prop",
    "DATA/MODEL/FACILITYP/FP000.RMD": "field prop",
}


@unittest.skipUnless(ISO, "no disc image (P4PC_ISO)")
class DiscSample(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.disc = p4disc.Disc(ISO)

    @classmethod
    def tearDownClass(cls):
        cls.disc.close()

    def test_sample_exports(self):
        for spec in SAMPLE:
            with self.subTest(spec=spec):
                data = self.disc.read(spec)
                rmd = rwmodel.Rmd(data)
                conv, doc, binary, base, side = export(data, rw_to_gltf.model_name(spec))
                check_document(self, doc, binary)
                clump = rmd.clump
                want_tris = sum(len([t for t in clump.geometries[a.geometry].triangles if len(set(t[:3])) == 3])
                                for a in clump.atomics)
                got_tris = sum(doc["accessors"][p["indices"]]["count"] // 3 for m in doc["meshes"] for p in m["primitives"])
                self.assertEqual(got_tris, want_tris)
                bones = clump.hanim_frames()
                if doc.get("skins"):
                    self.assertEqual(len(doc["skins"][0]["joints"]), len(bones))
                self.assertEqual(len(doc.get("animations", [])), len(rmd.of(rwmodel.ANIMANIMATION)) if bones else 0)
                self.assertEqual(len(doc.get("images", [])), len(rmd.texdictionary.textures) if rmd.texdictionary else 0)
                self.assertEqual(side["source"]["bytes"], len(data))
                for geo_index, geo in enumerate(clump.geometries):
                    mesh = [m for m in doc["meshes"] if m["extras"]["p4"]["geometry"] == geo_index][0]
                    pos = gltfio.read_accessor(doc, binary, mesh["primitives"][0]["attributes"]["POSITION"])
                    self.assertEqual(pos, [tuple(p) for p in geo.morph_targets[0].positions])

    def test_texture_matches_port_decoder(self):
        # The CRC-32s tests/native_tex_test.c expects from src/render/p4gl_nativetex.c for DOBJ044.RMD.
        rmd = rwmodel.Rmd(self.disc.read("DATA/FIELD/GRMD/DOBJ044.RMD"))
        crcs = []
        for tex in rmd.texdictionary.textures:
            images = ps2tex.gif_images(tex.data)
            unswizzle = ps2tex.unswizzle8 if tex.depth == 8 else ps2tex.unswizzle4
            crcs.append(zlib.crc32(unswizzle(images[0], tex.width, tex.height)))
        self.assertEqual(crcs, [0x598A3B01, 0x5CF41033])

    def test_weapon_attach_point(self):
        # The protagonist's attach table binds weapon slot 500 to a hand bone (mdlAttachWeaponModelSlots).
        rmd = rwmodel.Rmd(self.disc.read("DATA/MODEL/PACK/BC001.PAC#bc001.RMD"))
        ids = {ident: node for ident, node, _m in rmd.attach_table()}
        self.assertIn(500, ids)
        frames = {f.hanim.node_id for f in rmd.clump.frames if f.hanim is not None}
        self.assertIn(ids[500], frames)


if __name__ == "__main__":
    unittest.main()

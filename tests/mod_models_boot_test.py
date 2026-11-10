"""[resource-model] admission through the real boot handler: synthetic packages (a glTF sword from
tools/models/make_test_weapon.py, a synthetic RMD in a developer tree standing in for the disc).

    python tests/mod_models_boot_test.py [emulator...] <mod_models_boot> --driver-end [unittest arguments]
"""
import os
import pathlib
import re
import subprocess
import sys
import tempfile
import unittest

HERE = pathlib.Path(__file__).resolve().parent
sys.path.insert(0, str(HERE / ".." / "tools" / "models"))
sys.path.insert(0, str(HERE))
import make_test_weapon  # noqa: E402
import models_fixture  # noqa: E402
import gltfio  # noqa: E402

separator = sys.argv.index("--driver-end")
DRIVER = sys.argv[1:separator]
sys.argv = [sys.argv[0], *sys.argv[separator + 1:]]
KEY = "DATA/MODEL/WEAPON/WP004.RMD"
MANIFEST = "[mod]\nid={id}\nname=Model fixture\nversion=1.0\napi=1.{minor}\npriority=0\norder=0\n"
# Use the runtime's minor; model schema support is expressed by a capability,
# not by raising the package API minor.
_HEADER = (HERE / ".." / "include" / "p4pc" / "packages.h").read_text(encoding="utf-8")
MINOR = int(re.search(r"P4_PACKAGE_API_MINOR (\d+)", _HEADER).group(1))
# A skinned glTF with one slot animation (L0S01: bone_2 turns a quarter) and one other animation.
ANIMATED = models_fixture.skinned_gltf(animations=[
    ("L0S01", [(2, "rotation", (0.0, 1.0), [(0.0, 0.0, 0.0, 1.0), (0.0, 0.0, 0.7071068, 0.7071068)], "LINEAR")]),
    ("Wave", [(1, "translation", (0.0, 1.0), [(0.0, 0.0, 0.0), (0.0, 1.0, 0.0)], "LINEAR")])])[0]
SECTION = ("[resource-model {owner}:sword]\nfile={file}\nformat=gltf\nschema={schema}\nrevision=1\nreplaces={replaces}\n")


class ModelBoot(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory(prefix="p4 model boot ")
        self.addCleanup(self.temporary.cleanup)
        self.root = pathlib.Path(self.temporary.name)
        self.packages = self.root / "packages"
        self.packages.mkdir()
        developer = self.root / "developer" / "DATA" / "MODEL" / "WEAPON"
        developer.mkdir(parents=True)
        (developer / "WP004.RMD").write_bytes(models_fixture.static_weapon()[0])
        self.cache = self.root / "cache"
        self.env = dict(os.environ, P4PC_OVERRIDE_DIR=str(self.root / "developer"), P4PC_MODEL_CACHE=str(self.cache))
        self.env.pop("P4PC_ISO", None)

    def package(self, ident, glb=None, replaces=KEY, minor=None, section=True, file="models/sword.glb", schema=1):
        minor = MINOR if minor is None else minor
        folder = self.packages / ident
        (folder / "models").mkdir(parents=True)
        (folder / "models" / "sword.glb").write_bytes(glb if glb is not None else make_test_weapon.build())
        manifest = MANIFEST.format(id=ident, minor=minor)
        if section:
            manifest += f"requires=manifest.resource-model@{min(schema, 3)}\n"
            manifest += SECTION.format(owner=ident, file=file, replaces=replaces, schema=schema)
        (folder / "mod.ini").write_text(manifest, encoding="ascii")

    def run_driver(self, mode, enabled, key=KEY):
        run = subprocess.run([*DRIVER, mode, str(self.packages), enabled, key], capture_output=True, text=True,
                             env=self.env, timeout=300)
        self.assertEqual(run.returncode, 0, run.stderr)
        return run.stdout, run.stderr

    def test_empty_table_is_inactive(self):
        self.package("boot.a", section=False)
        out, _err = self.run_driver("empty", "boot.a")
        self.assertIn("publish 0", out)
        self.assertIn("active 0", out)
        self.assertIn("override none", out)

    def test_valid_declaration_publishes_its_override(self):
        self.package("boot.a")
        out, err = self.run_driver("valid", "boot.a")
        self.assertIn("active 1", out)
        self.assertIn("override %s boot.a:sword boot.a 48" % KEY, out)
        self.assertIn("replaces %s (48 triangles, cooked)" % KEY, err)

    def test_second_start_uses_the_cache(self):
        self.package("boot.a")
        self.run_driver("valid", "boot.a")
        out, err = self.run_driver("valid", "boot.a")
        self.assertIn("cached cook", err)
        self.assertIn("active 1", out)

    def test_later_package_wins_and_is_reported(self):
        self.package("boot.a")
        self.package("boot.b")
        out, err = self.run_driver("override", "boot.a,boot.b")
        self.assertIn("override %s boot.b:sword boot.b 48" % KEY, out)
        self.assertIn("from provider boot.b overrides boot.a:sword", err)

    def test_raw_replacement_of_the_same_file_is_reported(self):
        self.package("boot.a")
        raw = self.packages / "boot.b" / "files" / "DATA" / "MODEL" / "WEAPON"
        raw.mkdir(parents=True)
        (raw / "WP004.RMD").write_bytes(models_fixture.static_weapon()[0])
        (self.packages / "boot.b" / "mod.ini").write_text(MANIFEST.format(id="boot.b", minor=MINOR), encoding="ascii")
        out, err = self.run_driver("raw", "boot.a,boot.b")
        self.assertIn("override %s boot.a:sword boot.a" % KEY, out)
        self.assertIn("wins over provider boot.b's raw replacement of %s" % KEY, err)

    def refused(self, *fragments, **package):
        self.package("boot.a", **package)
        out, err = self.run_driver("refused", "boot.a")
        self.assertIn("publish -1", out)
        for fragment in fragments:
            self.assertIn(fragment, err)

    def test_missing_target_refused(self):
        self.refused("replaces DATA/MODEL/WEAPON/WP999.RMD: not on the disc", replaces="DATA/MODEL/WEAPON/WP999.RMD")

    def test_malformed_glb_refused(self):
        self.refused("models/sword.glb", "cannot parse", glb=make_test_weapon.build()[:200])

    def test_oversize_glb_refused(self):
        b = gltfio.Builder()
        size = 512
        rgba = bytes(range(256)) * (size * size * 4 // 256)
        b.add("images", {"bufferView": b.view(gltfio.png_rgba(size, size, rgba)), "mimeType": "image/png"})
        b.add("textures", {"source": 0})
        b.add("materials", {"pbrMetallicRoughness": {"baseColorTexture": {"index": 0}}})
        positions = [(0.0, 0.0, 0.0), (1.0, 0.0, 0.0), (0.0, 1.0, 0.0)]
        b.add("meshes", {"primitives": [{"attributes": {
            "POSITION": b.accessor(positions, gltfio.FLOAT, "VEC3", minmax=True),
            "TEXCOORD_0": b.accessor([(0.0, 0.0)] * 3, gltfio.FLOAT, "VEC2")}, "material": 0}]})
        b.add("nodes", {"name": "bone_0", "mesh": 0})
        b.add("scenes", {"nodes": [0]})
        self.refused("is 512x512; at most 256 for a weapon", glb=b.glb())

    def test_api_1_0_rejects_extension_fields(self):
        self.refused("Native extension fields require api=1.1", minor=0)

    def test_old_api_can_use_model_capability(self):
        # Extension sections date to 1.1; the capability permits model schemas
        # without requiring one of the unreleased planned API minors.
        self.package("boot.a", minor=1)
        out, err = self.run_driver("valid", "boot.a")
        self.assertIn("active 1", out, err)

    def test_skinned_needs_schema_2(self):
        self.refused("mesh 0 is skinned; skinned meshes need schema=2", glb=models_fixture.skinned_gltf()[0])

    def test_skinned_schema_2_publishes(self):
        self.package("boot.a", glb=models_fixture.skinned_gltf()[0], schema=2, minor=1)
        out, err = self.run_driver("valid", "boot.a")
        self.assertIn("active 1", out)
        self.assertIn("override %s boot.a:sword boot.a 8" % KEY, out)
        self.assertIn("(8 triangles, cooked)", err)

    def test_unknown_schema_refused(self):
        self.refused("schema=1, 2 or 3", schema=4)

    def test_animations_below_schema_3_stay_retail(self):
        """rw_to_gltf exports carry every slot's animation; a schema=2 declaration cooks them but applies none."""
        self.package("boot.a", glb=ANIMATED, schema=2)
        out, err = self.run_driver("valid", "boot.a")
        self.assertIn("override %s boot.a:sword boot.a 8 schema 2 animations 1" % KEY, out)
        self.assertIn("(8 triangles, 1 slot animations left retail (schema=3 applies them), cooked)", err)

    def test_animated_schema_3_publishes(self):
        self.package("boot.a", glb=ANIMATED, schema=3, minor=1)
        out, err = self.run_driver("valid", "boot.a")
        self.assertIn("override %s boot.a:sword boot.a 8 schema 3 animations 1" % KEY, out)
        self.assertIn("(8 triangles, 1 slot animations, cooked)", err)


if __name__ == "__main__":
    unittest.main()

#!/usr/bin/env python3
"""Register-ABI discovery for the port natives sigs.py catalogs."""
import importlib.util
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location("p4_mipsdiff_sigs", ROOT / "tools/mipsdiff/sigs.py")
sigs = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(sigs)


class SkyNativeTests(unittest.TestCase):
    def test_split_named_pointer_definition(self):
        source = "Matrix *\nProbe(Matrix *out, const Axis *axis, float angle, int mode)\n{ return out; }"
        matches = list(sigs.DEF.finditer(source))
        self.assertEqual(len(matches), 1)
        self.assertEqual(sigs.parse_def(matches[0]), ("Probe", "p", "ppfi"))

    def test_static_definition_is_not_a_native(self):
        source = "static RwV3d *Probe(RwV3d *out, const RwV3d *in, const RwMatrix *matrix)\n{\n return out;\n}\n"
        matches = list(sigs.DEF.finditer(source))
        self.assertEqual(len(matches), 1)
        self.assertIsNone(sigs.parse_def(matches[0]))

    def test_sky_multiplies_are_address_named_natives(self):
        # The engine-slot multiplies the GL device installs (src/platform/rw_skytran.c) keep the register ABI of the retail calls.
        found = {name: (ret, args) for name, ret, args in sigs.definitions(str(ROOT / "src/platform/rw_skytran.c"))}
        self.assertEqual(found, {
            "func_00402390": ("v", "ppp"),
            "func_00402410": ("p", "ppp"),
            "func_00402470": ("p", "ppip"),
            "func_00402510": ("p", "ppp"),
            "func_00402570": ("p", "ppip"),
        })


class DirectMmiProviderTests(unittest.TestCase):
    def providers(self):
        return {addr: ("provider_" + addr, ret, args)
                for addr, (ret, args) in sigs._MMI_NATIVE_ABIS.items() if addr != "003f6440"}

    def render(self):
        return [("RpSkyRenderStateSet", "i", "ii")]

    def test_actual_provider_classes_and_void_result(self):
        rows = {}
        sigs._add_mmi_native_users(rows, self.providers(), self.render())
        self.assertEqual({addr: row[1:] for addr, row in rows.items()}, sigs._MMI_NATIVE_ABIS)
        self.assertEqual(rows["003f6440"], ("RpSkyRenderStateSet", "i", "ii"))
        self.assertEqual(rows["004adb50"][1:], ("v", "p"))

    def test_missing_direct_provider_fails_loudly(self):
        for addr in self.providers():
            with self.subTest(addr=addr):
                providers = self.providers()
                del providers[addr]
                with self.assertRaisesRegex(SystemExit, "MMI native " + addr):
                    sigs._add_mmi_native_users({}, providers, self.render())

    def test_render_definition_must_be_unique(self):
        for definitions in ([], self.render() * 2):
            with self.subTest(count=len(definitions)):
                with self.assertRaisesRegex(SystemExit, "expected one RpSkyRenderStateSet"):
                    sigs._add_mmi_native_users({}, self.providers(), definitions)

    def test_changed_direct_parameter_count_or_class_fails(self):
        for addr in self.providers():
            ret, args = sigs._MMI_NATIVE_ABIS[addr]
            for changed in ((ret, args[:-1]), (ret, args + "p"), (ret, "f" + args[1:])):
                with self.subTest(addr=addr, changed=changed):
                    providers = self.providers()
                    providers[addr] = ("provider_" + addr, *changed)
                    with self.assertRaisesRegex(SystemExit, "MMI native " + addr):
                        sigs._add_mmi_native_users({}, providers, self.render())
        with self.assertRaisesRegex(SystemExit, "MMI native 003f6440"):
            sigs._add_mmi_native_users({}, self.providers(), [("RpSkyRenderStateSet", "i", "ip")])


class RecordParameterTests(unittest.TestCase):
    def shape(self, size, chunks="G", hfa=None, unaligned=False):
        return {"kind": "record", "bytes": size, "chunks": chunks,
                "hfa": hfa, "unaligned": unaligned}

    def test_every_small_integer_record_width(self):
        for size in range(1, 9):
            with self.subTest(size=size):
                self.assertEqual(sigs.record_parameter_class(self.shape(size), None),
                                 "i" if size <= 4 else "q")

    def test_existing_float_record_integer_lowering(self):
        native = {"cls": "int", "ct": "unsigned long long", "sx": "ArbitraryRecordAlias"}
        for size, count in ((4, 1), (8, 2)):
            self.assertEqual(sigs.record_parameter_class(self.shape(size, "F", (4, count)), native),
                             "i" if size == 4 else "q")

    def test_unlowered_float_and_double_records(self):
        self.assertEqual(sigs.record_parameter_class(self.shape(4, "F", (4, 1)), None), "g")
        self.assertEqual(sigs.record_parameter_class(self.shape(8, "F", (4, 2)), None), "e")
        self.assertEqual(sigs.record_parameter_class(self.shape(8, "F", (8, 1)), None), "d")

    def test_unaligned_records_keep_memory_class(self):
        self.assertEqual(sigs.record_parameter_class(self.shape(3, unaligned=True), None), "n")
        self.assertEqual(sigs.record_parameter_class(self.shape(5, unaligned=True), None), "m")

    def test_unknown_or_large_records_are_not_scalar_guesses(self):
        self.assertIsNone(sigs.record_parameter_class(None, None))
        self.assertIsNone(sigs.record_parameter_class({"kind": "int", "bytes": 4}, None))
        for size in (None, 0, 9, 16, 32):
            self.assertEqual(sigs.record_parameter_class(self.shape(size), None), "?")


if __name__ == "__main__":
    unittest.main()

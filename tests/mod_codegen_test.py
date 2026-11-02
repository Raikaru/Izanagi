import pathlib
import sys
import tempfile
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import gen_port
import mod_codegen


class ModCodegen(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory(prefix="p4-mod-guards-")
        self.addCleanup(self.tmp.cleanup)
        self.root = pathlib.Path(self.tmp.name)
        self.mirror = self.root / "mirror"
        (self.mirror / "src").mkdir(parents=True)
        (self.mirror / "include").mkdir()

    def generate(self, text, enabled=True):
        path = self.mirror / "src/game.c"
        path.write_text(text, encoding="utf-8")
        count = mod_codegen.instrument(self.mirror, ["src/game.c"], self.root, gen_port, enabled)
        return count, path.read_text(encoding="utf-8"), (self.root / "mod_bridges.c").read_text(encoding="utf-8")

    def test_scalar_signedness_and_guest_pointer_marshalling(self):
        text = (ROOT / "tests/mod_hooks_game.c").read_text(encoding="utf-8")
        count, game, bridge = self.generate(text)
        self.assertEqual(count, 6)
        for signature in ('"II"', '"hbH"', '"fifpd"', '"pp"', '"ii"', '"vp"'):
            self.assertIn(signature, bridge)
        self.assertNotIn("P4ModValue", game)
        self.assertIn("P4_H2G(a2)", bridge)
        self.assertIn("P4_G2H((uint32_t)(a[2].u))", bridge)
        self.assertIn("P4_MOD_BYPASS", game)
        self.assertIn(".enabled = p4_mod_entry_00100140.restore", game)
        self.assertIn('#include "p4pc/mod_entry.h"', game)
        self.assertNotIn("P4ModFunction", game)
        self.assertNotIn("__attribute__((noinline))", game)
        self.assertEqual(bridge.count("__attribute__((noinline))"), 6)
        self.assertIn("p4_mod_f32_number(a1)", bridge)
        self.assertIn("p4_mod_f32_value(result.d)", bridge)
        self.assertIn("(int64_t)(int32_t)(a0)", bridge)

    def test_retail_crt_aliases_bind_compiled_game_symbols(self):
        text = ("// FUN_00421FB8\nint kill(int command, int status) { return command + status; }\n"
                "// FUN_00421F98\nint fstat(int fd, void *info) { return fd; }\n"
                "// FUN_00100100\nint ordinary(int value) { return value; }\n")
        count, _, bridge = self.generate(text)
        self.assertEqual(count, 3)
        self.assertIn('P4_ASM_STR(__USER_LABEL_PREFIX__) \"func_00421fb8\"', bridge)
        self.assertIn('P4_ASM_STR(__USER_LABEL_PREFIX__) \"func_00421f98\"', bridge)
        self.assertIn('P4_ASM_STR(__USER_LABEL_PREFIX__) \"ordinary\"', bridge)
        self.assertNotIn('P4_ASM_STR(__USER_LABEL_PREFIX__) \"kill\"', bridge)

    def test_guard_wraps_final_hand_override(self):
        text = "typedef int s32;\n// FUN_00100100\ns32 sample(s32 value) { return value + 1; }\n"
        game = self.mirror / "src/game.c"
        game.write_text(text, encoding="utf-8")
        overrides = self.root / "overrides"
        overrides.mkdir()
        (overrides / "00100100.c").write_text("s32 sample(s32 value) { return value + 7; }\n", encoding="utf-8")
        gen_port.apply_overrides(self.root, self.mirror, ["src/game.c"])
        mod_codegen.instrument(self.mirror, ["src/game.c"], self.root, gen_port)
        emitted = game.read_text(encoding="utf-8")
        self.assertIn("value + 7", emitted)
        self.assertNotIn("value + 1", emitted)
        self.assertLess(emitted.index("p4_mod_guard_00100100(value)"), emitted.index("value + 7"))

    def test_complex_and_unknown_signatures_are_not_guessed(self):
        text = """typedef struct { int a; int b; } Record;
// FUN_00100100
Record aggregate(Record value) { return value; }
// FUN_00100120
void callback(void (*fn)(int)) { fn(1); }
// FUN_00100140
int variadic(int value, ...) { return value; }
"""
        count, game, _ = self.generate(text)
        self.assertEqual(count, 0)
        self.assertNotIn("p4_mod_dispatch", game)
        self.assertEqual((self.root / "mod_hooks.tsv").read_text().count("unsupported signature"), 3)

    def test_target_dependent_scalar_widths_are_refused(self):
        text = """typedef unsigned int size_t;
typedef size_t Count;
// FUN_00100100
void narrow(size_t count) {}
// FUN_00100120
long result(void) { return 0; }
// FUN_00100140
void alias(Count count) {}
// FUN_00100160
void pointer(size_t *count) {}
"""
        count, game, _ = self.generate(text)
        self.assertEqual(count, 1)
        self.assertNotIn("p4_mod_guard_00100100", game)
        self.assertNotIn("p4_mod_guard_00100120", game)
        self.assertNotIn("p4_mod_guard_00100140", game)
        self.assertIn("p4_mod_guard_00100160", game)
        self.assertEqual((self.root / "mod_hooks.tsv").read_text().count("unsupported signature"), 3)

    def test_disabled_keeps_original_game_text(self):
        text = "// FUN_00100100\nint sample(int x) { return x; }\n"
        count, game, bridge = self.generate(text, enabled=False)
        self.assertEqual(count, 0)
        self.assertEqual(game, text)
        self.assertNotIn("p4_mod_function_00100100", bridge)


if __name__ == "__main__":
    unittest.main()

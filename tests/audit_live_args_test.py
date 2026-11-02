"""Retail argument discovery, call counting and immutable function-alias resolution need no private data."""
import pathlib
import sys
import tempfile
import unittest
from unittest import mock

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1] / "tools"))
import audit_live_args as audit
import audit_prototypes as prototypes


class AuditDiscoveryTest(unittest.TestCase):
    def test_named_marked_static_and_function_pointer_return_bodies(self):
        text = (
            'int func_00100100(void) { return 0; }\n'
            'int renamed(int value) { return value; }\n'
            '// FUN_00100300\n'
            'static int private_name(void) { puts("} fake(,)"); return 0; }\n'
            '// FUN_00100400\n'
            'int (**get_hooks(void))(unsigned) { return 0; }\n'
            'int unrelated(void) { return 0; }\n'
        )
        definitions, original = audit.functions_in(text, {"renamed": 0x100200})
        self.assertEqual(set(definitions), {0x100100, 0x100200, 0x100300, 0x100400})
        self.assertEqual(definitions[0x100300][2], "private_name")
        self.assertEqual(definitions[0x100400][2], "get_hooks")
        for start, end, name in definitions.values():
            self.assertEqual(original[start], "{")
            self.assertEqual(original[end], "}")
        self.assertEqual(original, text)

    def test_literal_syntax_does_not_create_calls_or_arguments(self):
        text = (
            'extern void target(const char *, int, char) __asm__("real_target");\n'
            'void f(void) {\n'
            '  /* fake(1,2) { */\n'
            '  target("fake(,) }", nested(1, 2), \'{\');\n'
            '  puts("/* // } ((");\n'
            '}\n'
        )
        calls, aliases = audit.c_calls(text)
        self.assertEqual(dict(calls), {"target": [(3, 4)], "nested": [(2, 4)], "puts": [(1, 5)]})
        self.assertEqual(aliases, {"target": "real_target"})

    def test_macro_before_asm_alias_is_not_mistaken_for_the_alias(self):
        text = (
            '#define PARENT(p) (*(void **)(p))\n'
            'extern void matrix_mul(void *, const void *, const void *) __asm__("func_003e05f0");\n'
            'void f(void) { matrix_mul(a, b, PARENT(c)); }\n'
        )
        calls, aliases = audit.c_calls(text)
        self.assertEqual(aliases, {"matrix_mul": "func_003e05f0"})
        self.assertEqual(calls["matrix_mul"], [(3, 3)])
        self.assertEqual(calls["PARENT"], [(1, 3)])

    def test_ee_accumulator_and_raw_sqrt_register_effects(self):
        for mnemonic in ("adda.s", "suba.s", "mula.s", "madda.s", "msuba.s"):
            with self.subTest(mnemonic=mnemonic):
                self.assertEqual(audit.effects(mnemonic, ["$f12", "$f13"]),
                                 ([("f", 12), ("f", 13)], []))
        self.assertEqual(audit.effects("c1", ["0xf0444"]), ([("f", 15)], [("f", 17)]))
        with self.assertRaisesRegex(ValueError, "unknown COP1"):
            audit.effects("c1", ["0xf0440"])

    def test_variadic_definitions_not_fixed_va_list_functions_are_excluded(self):
        with tempfile.TemporaryDirectory() as tmp:
            source = pathlib.Path(tmp) / "format.c"
            source.write_text(
                "void debug_text(int position, const char *format, ...) { }\n"
                "void fixed_vsprintf(char *dest, const char *format, void *arguments) { }\n",
                encoding="utf-8")
            result, _aggregate = audit.source_abi_metadata([(str(source), None, True)],
                                                          {"debug_text": 0x100100, "fixed_vsprintf": 0x446ed8})
            self.assertIn(0x100100, result)
            self.assertNotIn(0x446ed8, result)

    def test_aggregate_result_pointer_is_not_an_omitted_source_argument(self):
        registers = {("i", 4), ("f", 12), ("f", 13)}
        self.assertEqual(audit.slots_needed(registers, registers, True)[0], 2)
        self.assertEqual(audit.slots_needed(registers, registers, False)[0], 3)
        with tempfile.TemporaryDirectory() as tmp:
            source = pathlib.Path(tmp) / "pair.c"
            source.write_text("typedef struct { float x, y; } Pair;\n"
                              "Pair make_pair(float x, float y) { Pair p; return p; }\n", encoding="utf-8")
            _variadic, aggregate = audit.source_abi_metadata([(str(source), None, True)],
                                                            {"make_pair": 0x100100})
            self.assertEqual(aggregate, {0x100100})

    def test_cast_function_and_immutable_pointer_aliases_resolve_without_guessing(self):
        target = {"kind": "FunctionDecl", "id": "fn", "name": "matrix_alias"}
        direct = {"kind": "DeclRefExpr", "referencedDecl": target}
        self.assertEqual(prototypes.function_reference({"kind": "CStyleCastExpr", "inner": [direct]}, {}), target)
        declaration = {"kind": "VarDecl", "id": "pointer", "type": {"qualType": "void (*const)(int, float)"},
                       "inner": [direct]}
        pointer = {"kind": "DeclRefExpr", "referencedDecl": {"kind": "VarDecl", "id": "pointer"}}
        self.assertEqual(prototypes.function_reference(pointer, {"pointer": declaration}), target)
        declaration["type"]["qualType"] = "void (*)(int, float)"
        self.assertIsNone(prototypes.function_reference(pointer, {"pointer": declaration}))

    def test_immutable_pointer_cycles_remain_explicitly_unresolved(self):
        pointer = {"kind": "DeclRefExpr", "referencedDecl": {"kind": "VarDecl", "id": "pointer"}}
        declaration = {"kind": "VarDecl", "id": "pointer", "type": {"qualType": "void (*const)(int)"},
                       "inner": [pointer]}
        self.assertIsNone(prototypes.function_reference(pointer, {"pointer": declaration}))

    def test_sound_packet_profiles_count_semantic_values_not_spilled_tails(self):
        for opcode, required in ((0x8f, 3), (0x82, 3), (0x81, 3), (0x80, 4), (0x84, 4), (0x83, 5)):
            self.assertEqual(audit.model_required_args(0x43c0c0, [3, opcode]), required)
        self.assertEqual(audit.model_required_args(0x43c518, [1, 2]), 4)
        self.assertEqual(audit.model_required_args(0x43c518, [1, 0]), 5)
        self.assertEqual(audit.model_required_args(0x43c518, [1, 7]), 5)
        self.assertIsNone(audit.model_required_args(0x43c518, [1, None]))
        for opcode, required in ((1, 4), (2, 4), (3, 6), (5, 4)):
            self.assertEqual(audit.model_required_args(0x43c230, [opcode]), required)
        self.assertEqual(audit.model_required_args(0x43c3b0, [0]), 4)
        self.assertEqual(audit.model_required_args(0x43c3b0, [5]), 3)
        self.assertIsNone(audit.model_required_args(0x43c230, [None]))
        self.assertEqual(audit.model_required_args(0x3b88c0, []), 0)

    def test_native_profiles_keep_live_syscall_causes_but_not_unused_formals(self):
        for address in (0x421580, 0x421590, 0x4215a0, 0x4215b0):
            self.assertEqual(audit.model_required_args(address, []), 1)
        self.assertEqual(audit.model_required_args(0x426cf0, []), 2)
        self.assertEqual(audit.model_required_args(0x3b6f00, []), 1)
        self.assertEqual(audit.model_required_args(0x42b3e0, []), 0)

    def test_compiled_inventory_rejects_partial_or_different_unit_reports(self):
        files = [("/lane/build/gen/decomp/src/unit.c", None, True)]
        call = {"unit": "src/unit.c", "caller": "active"}
        report = {"active_units": ["src/unit.c"], "counts": {"calls": 1}, "calls": [call], "errors": []}
        self.assertEqual(audit.compiled_call_inventory(report, files)["src/unit.c"]["active"], [call])
        for replacement in ({"errors": [{"unit": "src/unit.c"}]}, {"active_units": []}, {"calls": []}):
            with self.subTest(replacement=replacement), self.assertRaises(ValueError):
                audit.compiled_call_inventory(dict(report, **replacement), files)

    def test_opcode_constants_do_not_guess_runtime_values(self):
        integer = {"kind": "IntegerLiteral", "value": "143"}
        self.assertEqual(prototypes.integer_argument({"kind": "ImplicitCastExpr", "inner": [integer]}), 0x8f)
        self.assertEqual(prototypes.integer_argument({"kind": "UnaryOperator", "opcode": "-", "inner": [integer]}), -143)
        self.assertIsNone(prototypes.integer_argument({"kind": "DeclRefExpr"}))

    def test_mirror_manifest_selects_only_active_units(self):
        with tempfile.TemporaryDirectory() as tmp:
            gen = pathlib.Path(tmp)
            sources = gen / "decomp/src"
            sources.mkdir(parents=True)
            active = sources / "active.c"
            active.write_text("int active(void) { return 1; }\n", encoding="utf-8")
            (sources / "excluded.c").write_text("int excluded(void) { return 0; }\n", encoding="utf-8")
            (gen / "units.txt").write_text(
                "/lane/build/gen/decomp/src/active.c|-I/lane/build/gen/decomp/include\n",
                encoding="utf-8")
            self.assertEqual(audit.mirror_files(str(gen)), [(str(active), None, True)])

    def test_mirror_manifest_rejects_wrong_root_traversal_and_missing_units(self):
        with tempfile.TemporaryDirectory() as tmp:
            gen = pathlib.Path(tmp)
            manifest = gen / "units.txt"
            for row in ("/lane/src/unit.c|flags\n", "/lane/decomp/src/../unit.c|flags\n"):
                manifest.write_text(row, encoding="utf-8")
                with self.subTest(row=row), self.assertRaises(ValueError):
                    audit.mirror_files(str(gen))
            manifest.write_text("/lane/decomp/src/missing.c|flags\n", encoding="utf-8")
            with self.assertRaises(FileNotFoundError):
                audit.mirror_files(str(gen))

    def test_matched_callers_can_use_flat_retail_asm(self):
        sentinel = object()
        with mock.patch.object(audit.rn, "find_asm", side_effect=KeyError), \
                mock.patch.object(audit, "Func", return_value=sentinel) as constructor:
            self.assertIs(audit.load_func(0x100100), sentinel)
            constructor.assert_called_once_with(0x100100)


if __name__ == "__main__":
    unittest.main()

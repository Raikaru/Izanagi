#!/usr/bin/env python3
"""Non-retail fixtures for generated-table and ELF32/64 relocation readers."""
from pathlib import Path
import sys
from types import SimpleNamespace
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
import asm_worklist as worklist


TABLE = '''int absent(int a0) { return p4pc_trap_args("absent", 0x00300000, a0, 0, 0, 0); }
const p4pc_patch_t p4pc_patch_table[] = {
    { 0x00700000, (void *)&native, "ip" },
    { 0x00700004, (void *)&other, 0 },
};
const size_t p4pc_patch_count = 2;
const p4pc_native_t p4pc_native_table[] = {
    { 0x00300000, (void *)&native, "native", "ip" },
};
'''


class GeneratedTables(unittest.TestCase):
    def test_typed_patch_rows_are_not_native_rows(self):
        traps, patches = worklist._parse_generated_tables(TABLE)
        self.assertEqual(traps, {"absent": 0x00300000})
        self.assertEqual(patches, {0x00700000: "native", 0x00700004: "other"})

    def test_zero_count_is_explicit_not_missing(self):
        self.assertEqual(worklist._parse_generated_tables(
            "const p4pc_patch_t p4pc_patch_table[] = {\n};\nconst size_t p4pc_patch_count = 0;\n"), ({}, {}))

    def test_missing_or_incomplete_table_fails(self):
        for text in ("", TABLE.replace("p4pc_patch_table[]", "renamed[]"),
                     TABLE.replace("const size_t p4pc_patch_count = 2;", ""),
                     TABLE.split("};", 1)[0]):
            with self.subTest(text=text), self.assertRaises(ValueError):
                worklist._parse_generated_tables(text)

    def test_count_drift_fails(self):
        for count in (0, 1, 3):
            with self.subTest(count=count), self.assertRaisesRegex(ValueError, "count mismatch"):
                worklist._parse_generated_tables(TABLE.replace("count = 2", f"count = {count}"))

    def test_duplicate_patch_fails(self):
        with self.assertRaisesRegex(ValueError, "duplicate patch location"):
            worklist._parse_generated_tables(TABLE.replace("0x00700004", "0x00700000"))

    def test_duplicate_declarations_fail(self):
        with self.assertRaisesRegex(ValueError, "duplicate p4pc_patch_count"):
            worklist._parse_generated_tables(TABLE + "const size_t p4pc_patch_count = 2;\n")
        with self.assertRaisesRegex(ValueError, "duplicate p4pc_patch_table"):
            worklist._parse_generated_tables(TABLE + "\n".join(TABLE.splitlines()[1:]))

    def test_duplicate_trap_fails(self):
        with self.assertRaisesRegex(ValueError, "duplicate trap"):
            worklist._parse_generated_tables(TABLE.splitlines()[0] + "\n" + TABLE)

    def test_changed_patch_schema_fails(self):
        for replacement in ('(void *)&native },', '(void *)&native, "new-type" },'):
            with self.subTest(replacement=replacement), self.assertRaisesRegex(ValueError, "unrecognized patch row"):
                worklist._parse_generated_tables(TABLE.replace('(void *)&native, "ip" },', replacement))


class ObjectRelocations(unittest.TestCase):
    def calls(self, rows, bits=64):
        nm = "u.o:\ncaller T 00000000 20\nweak W 00000020 10\nlocal t 00000030 10\nresolver i 00000040 10\n"
        objdump = f"u.o:     file format elf{bits}-x86-64\nRELOCATION RECORDS FOR [.text]:\n{rows}\n"
        with patch.object(worklist.subprocess, "run", side_effect=[SimpleNamespace(stdout=nm), SimpleNamespace(stdout=objdump)]):
            return worklist.object_calls([Path("u.o")])

    def test_elf32_calls_and_address_references(self):
        calls, _, _ = self.calls("00000001 R_386_PC32 target\n00000008 R_386_32 callback+0x00000004", 32)
        self.assertEqual(calls["caller"], {"target", "callback"})

    def test_elf64_offsets_and_signed_addends(self):
        calls, _, _ = self.calls("0000000000000001 R_X86_64_PLT32 target-0x0000000000000004\n"
                                "0000000000000008 R_X86_64_64 callback+0x0000000000000010")
        self.assertEqual(calls["caller"], {"target", "callback"})

    def test_weak_local_and_ifunc_bodies_keep_their_edges(self):
        calls, objects, owners = self.calls("0000000000000021 R_X86_64_PLT32 weak_target-0x4\n"
                                            "0000000000000031 R_X86_64_PC32 local_target-0x4\n"
                                            "0000000000000041 R_X86_64_PLT32 resolver_target-0x4")
        self.assertEqual(calls["weak"], {"weak_target"})
        self.assertEqual(calls["local"], {"local_target"})
        self.assertEqual(calls["resolver"], {"resolver_target"})
        self.assertEqual(set(objects["u.o"]), {"caller", "weak", "local", "resolver"})
        self.assertEqual(owners["local"], {"u.o"})

    def test_data_and_section_references_are_not_calls(self):
        calls, _, _ = self.calls("0000000000000001 R_X86_64_PC32 .rodata-0x4\n"
                                "RELOCATION RECORDS FOR [.data]:\n0000000000000008 R_X86_64_64 not_a_call")
        self.assertEqual(dict(calls), {})


if __name__ == "__main__":
    unittest.main()

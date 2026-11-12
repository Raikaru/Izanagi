#!/usr/bin/env python3
"""tools/campaign_battles.py on synthetic flowscripts, ELF and tables; no retail data."""
import importlib.util
import os
import struct
import sys
import tempfile
import types
import unittest
from pathlib import Path
from unittest import mock

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("campaign_battles", ROOT / "tools/campaign_battles.py")
battles = importlib.util.module_from_spec(spec)
spec.loader.exec_module(battles)

PARAMETERS = {0x0010: 1, 0x0038: 1, 0x1016: 2, 0x200E: 1, 0x2069: 2, 0x20DC: 1}


def elf_with_commands():
    """An ELF32 whose one PT_LOAD holds D_00748530: groups 0..2, then an empty record."""
    base = 0x00748000
    segment = bytearray(0x2000)
    rows = 0x600
    for group in range(3):
        count = max([command & 0xFFF for command in PARAMETERS if command >> 12 == group] + [0]) + 1
        struct.pack_into("<II", segment, 0x530 + group * 8, base + rows, count)
        for index in range(count):
            command = group << 12 | index
            struct.pack_into("<II", segment, rows + index * 8, 0x00100000 + command * 8, PARAMETERS.get(command, 0))
        rows += count * 8
    header = bytearray(0x54)
    header[:4] = b"\x7fELF"
    header[4:6] = b"\x01\x01"
    struct.pack_into("<I", header, 0x1C, 0x34)
    struct.pack_into("<HH", header, 0x2A, 0x20, 1)
    struct.pack_into("<8I", header, 0x34, 1, len(header), base, base, len(segment), len(segment), 5, 16)
    return bytes(header) + bytes(segment)


def flowscript(procedures):
    """A FLW0 block: procedures [(name, [(opcode, operand) | ("word", value)])]."""
    code, table = [], []
    for name, body in procedures:
        table.append((name, len(code)))
        for opcode, operand in body:
            code.append(operand if opcode == "word" else opcode | operand << 16)
    names = b"".join(name.encode().ljust(24, b"\0") + struct.pack("<II", start, 0) for name, start in table)
    code_offset = 0x20 + 2 * 16 + len(names)
    header = struct.pack("<II4sII", 0, code_offset + len(code) * 4, b"FLW0", 0, 2).ljust(0x20, b"\0")
    sections = struct.pack("<4I", 0, 32, len(table), 0x40) + struct.pack("<4I", 2, 4, len(code), code_offset)
    return header + sections + names + struct.pack("<%dI" % len(code), *code)


def pack(members):
    out = bytearray()
    for name, data in members:
        out += name.encode().ljust(252, b"\0") + struct.pack("<I", len(data)) + data
        out += bytes(-len(out) % 64)
    return bytes(out)


PUSHI, PROC, COMM, END, ADD, PUSHIS, PUSHLIX = 0, 7, 8, 9, 14, 29, 30
FIELD = flowscript([
    ("boss_room", [
        (PROC, 0),
        (PUSHIS, 513), (COMM, 0x20DC),
        (PUSHIS, 516), (PUSHIS, 10), (COMM, 0x0010), (COMM, 0x20DC),  # WAIT pops its own argument
        (PUSHIS, 1), (PUSHI, 0), ("word", 811), (COMM, 0x1016),
        (PUSHLIX, 3), (COMM, 0x20DC),
        (PUSHIS, 2), (COMM, 0x0038),
        (PUSHIS, 99),  # left on the stack: the next procedure starts empty
        (END, 0)]),
    ("later", [(PROC, 0), (COMM, 0x20DC), (END, 0)]),
])
BOSS = flowscript([
    ("turn", [
        (PROC, 0),
        (PUSHIS, 1), (PUSHIS, 0), (COMM, 0x2069),
        (PUSHIS, 7), (PUSHIS, 5), (ADD, 0), (COMM, 0x200E),
        (END, 0)]),
])


class FakeDisc:
    files = {"BTL/BATTLE/BOSS/E201.BIN": None, "DATA/FIELD/PACK/F020_001.P00": None, "DATA/SOUND/A.ADX": None}

    def read(self, key):
        return {"BTL/BATTLE/BOSS/E201.BIN": BOSS, "DATA/FIELD/PACK/F020_001.P00": pack([("f020.bf", FIELD)]),
                "DATA/SOUND/A.ADX": FIELD}[key]


def encounter_block():
    block = bytearray(0x18 * 812)
    for encounter, flags, enemy in ((513, 0x4A2, 3), (516, 0x4A3, 4), (811, 0x212, 5)):
        struct.pack_into("<I", block, 0x18 * encounter, flags)
        struct.pack_into("<5H", block, 0x18 * encounter + 8, enemy, 0, 0, 0, 0)
    struct.pack_into("<HH", block, 0x18 * 513 + 0x12, 240, 2)
    return bytes(block)


class CampaignBattles(unittest.TestCase):
    def test_command_table_reads_parameter_counts(self):
        table = battles.command_table(elf_with_commands())
        for command, count in PARAMETERS.items():
            self.assertEqual(table[command][1], count)
        self.assertNotIn(3 << 12, table)
        with self.assertRaises(ValueError):
            battles.command_table(b"\0" * 64)

    def test_flowscripts_found_in_packs_and_boss_files(self):
        found = [(key, member) for key, member, _ in battles.flowscripts(FakeDisc())]
        self.assertEqual(found, [("BTL/BATTLE/BOSS/E201.BIN", None), ("DATA/FIELD/PACK/F020_001.P00", "f020.bf")])

    def test_calls_take_their_own_arguments(self):
        table = battles.command_table(elf_with_commands())
        calls = battles.script_calls(battles.parse_flowscript(FIELD), table, {0x20DC, 0x1016, 0x0038})
        self.assertEqual([(name, command, arguments) for name, _, command, arguments in calls], [
            ("boss_room", 0x20DC, [513]),
            ("boss_room", 0x20DC, [516]),
            ("boss_room", 0x1016, [811, 1]),
            ("boss_room", 0x20DC, [{"kind": "variable", "scope": "local", "index": 3}]),
            ("boss_room", 0x0038, [2]),
            ("later", 0x20DC, [{"kind": "unknown"}]),
        ])

    def test_inventory(self):
        table = battles.command_table(elf_with_commands())
        record = battles.encounter_records(encounter_block(), ["", "", "", "Shadow A", "Shadow B", "Knight"])
        result = battles.inventory(battles.flowscripts(FakeDisc()), table, record)
        started = [(b["procedure"], b["name"], b["encounter"]) for b in result["battles"]]
        self.assertEqual(started, [("boss_room", "btlScriptStartEncounter", 513),
                                   ("boss_room", "btlScriptStartEncounter", 516),
                                   ("boss_room", "CALL_BATTLE", 811),
                                   ("boss_room", "btlScriptStartEncounter", None),
                                   ("later", "btlScriptStartEncounter", None)])
        self.assertEqual(result["battles"][0]["record"], {"flags": 0x4A2, "escapable": False, "field": 240,
                                                          "variant": 2, "enemies": [{"id": 3, "name": "Shadow A"}]})
        self.assertTrue(result["battles"][1]["record"]["escapable"])
        self.assertEqual(result["battles"][2]["arguments"], {"encounter": 811, "argument1": 1})
        self.assertEqual([e["encounter"] for e in result["encounters"]], [513, 516, 811])
        self.assertEqual(result["party_commands"][0]["party_member"], "YOSUKE")
        checks = [(c["name"], c["encounter"], c["arguments"]) for c in result["boss_unit_checks"]]
        self.assertEqual(checks, [("btlAICommandScriptUnitIsIdOnSide", 0x201, {"side": 0, "unit": 1}),
                                  ("btlAICommandScriptQueryFrId", 0x201, {"unit": {"kind": "expression"}})])
        text = battles.summary(result)
        self.assertIn("513   no-escape Shadow A", text)
        self.assertIn("516   escapable Shadow B", text)
        self.assertIn("811   no-escape Knight", text)
        self.assertIn("unit checks: UnitIsIdOnSide(0,1)", text)

    def test_unknown_commands_invalidate_stack_and_bad_arity_fails(self):
        table = battles.command_table(elf_with_commands())
        script = battles.parse_flowscript(flowscript([
            ("branch", [(PUSHIS, 513), (COMM, 0x3000), (COMM, 0x20DC), (END, 0)]),
        ]))
        self.assertEqual(battles.script_calls(script, table, {0x20DC})[0][3], [{"kind": "unknown"}])
        del table[0x20DC]
        with self.assertRaisesRegex(ValueError, "no parameter count"):
            battles.script_calls(script, table, {0x20DC})
        table[0x20DC] = (0, 0)
        with self.assertRaisesRegex(ValueError, "unexpected parameter count"):
            battles.script_calls(script, table, {0x20DC})

    def test_summary_handles_missing_encounter_record(self):
        result = battles.inventory(battles.flowscripts(FakeDisc()), battles.command_table(elf_with_commands()),
                                   lambda encounter: None)
        self.assertIn("513   unknown", battles.summary(result))

    def test_output_paths_and_exclusive_creation(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            retail = root / "retail"
            retail.mkdir()
            iso, elf = retail / "disc.iso", retail / "game.elf"
            iso.write_bytes(b"disc")
            elf.write_bytes(b"elf")
            local_work = types.SimpleNamespace(work_root=lambda: root, RETAIL_AREA="retail")
            with mock.patch.dict(sys.modules, {"local_work": local_work}):
                with self.assertRaisesRegex(ValueError, "repository"):
                    battles.output_path(ROOT / "private.json")
                with self.assertRaisesRegex(ValueError, "retail"):
                    battles.output_path(retail / "private.json")
                output = root / "private.json"
                args = ["campaign_battles.py", "--iso", str(iso), "--elf", str(elf), "--out", str(output)]
                with mock.patch.object(sys, "argv", args), \
                     mock.patch.object(battles, "command_table", return_value={}), \
                     mock.patch.object(battles, "Disc") as disc, \
                     mock.patch.object(battles, "size_blocks", return_value=[(0, b""), (0, b""), (0, b"")]), \
                     mock.patch.object(battles, "inventory", return_value={"battles": []}):
                    disc.return_value.read.return_value = b""
                    self.assertEqual(battles.main(), 0)
                    saved = output.read_bytes()
                    with self.assertRaises(FileExistsError):
                        battles.main()
                    self.assertEqual(output.read_bytes(), saved)
                self.assertEqual(iso.read_bytes(), b"disc")
                self.assertEqual(elf.read_bytes(), b"elf")
                if os.name != "nt":
                    self.assertEqual(output.stat().st_mode & 0o777, 0o600)


if __name__ == "__main__":
    unittest.main()

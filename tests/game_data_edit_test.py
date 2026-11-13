"""p4.data edit layers (data/edits.xml) over real package boot and a synthetic disc only."""
import hashlib
import os
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import unittest
from game_elements_inputs_test import disc, packed

separator = sys.argv.index("--driver-end")
DRIVER = sys.argv[1:separator]
sys.argv = [sys.argv[0], *sys.argv[separator + 1:]]

GOOD_ONE = """<p4-edits schema="1">
  <skill name="Bufu" rename="Frost"/>
  <skill name="Agilao" power="120" sp="10" accuracy="95"/>
  <skill name="Poisma" ailment="fear" ailment-chance="60"/>
  <skill key="p4:skill:bash" hp-percent="9" hits-min="2" hits-max="3"/>
  <enemy name="Hablerie" rename="Big Hablerie" hp="300" level="20" st="12" fire="weak" ice="null" poison="null">
    <drop slot="1" item="Medicine" chance="50"/>
    <skill slot="2" name="Bufu"/>
  </enemy>
  <persona name="Izanagi" level="3" ma="5">
    <learn skill="Agilao" level="7"/>
    <move skill="Bufu" level="4"/>
  </persona>
  <persona name="Jiraiya"><learn skill="Agilao" level="3"/><replace skill="Bufu" with="Agidyne"/></persona>
  <item name="Medicine" price="100" rename="Herbal Tonic" from="4/20"/>
  <item name="Golf Club" sold="true"/>
  <item name="5-Iron"><unlock slot="1" material="Supple Metal" count="3"/></item>
  <floor name="Yukiko's Castle 3F" spawn="6" chests="4"/>
  <encounters id="1" rare-rate="5"><normal slot="2" formation="17" weight="40"/></encounters>
  <formation id="17"><enemy slot="2" name="Hablerie"/></formation>
  <treasure id="1"><record slot="2" item="Medicine" weight="5" locked="true"/></treasure>
</p4-edits>
"""
GOOD_TWO = '<p4-edits schema="1">\n  <skill name="agi lao" power="140"/>\n</p4-edits>\n'
# One package each; the refusal text the mode must produce.
REFUSALS = {
    "field": ('<skill name="Agilao" speed="3"/>', 'has no attribute "speed"'),
    "range": ('<skill name="Agilao" power="10000"/>', "power: 10000 is not a whole number from 0 to 9999"),
    "typo": ('<skill name="Agilaoo" power="1"/>', 'Did you mean "Agilao" (p4:skill:agilao)'),
    "ambiguous": ('<enemy name="Slime" hp="1"/>', '2 enemies are named "Slime"; use a key instead: p4:enemy:slime'),
    "ailment": ('<skill name="Agi" ailment-chance="10"/>', "this skill inflicts no ailment"),
    "drops": ('<enemy name="Hablerie"><drop slot="1" item="Medicine" chance="80"/>'
              '<drop slot="2" item="Medicine" chance="30"/></enemy>', "drop chances add up to 110%"),
    "learn": ('<persona name="Izanagi"><learn skill="Agi" level="3"/></persona>', "it already learns skill \"Agi\""),
    "hits": ('<skill name="Bash" hits-min="4"/>', "hits.min 4 is above hits.max 1"),
    "element": ('<skill name="Poisma" element="fire"/>', "only attack skills (physical..dark) can change element"),
    "schema": (None, 'the root must be <p4-edits schema="1">'),
    "empty": ("", "the file edits nothing"),
    "luau": (None, "data.luau needs Luau scripting"),
    "gate": ('<item name="Medicine"><unlock slot="1" material="Supple Metal" count="3"/></item>',
             "only gear has unlock gates; consumables use shop.from"),
    "material": ('<item name="5-Iron"><unlock slot="1" material="Medicine" count="3"/></item>', "not a material"),
    "date": ('<item name="Medicine" from="13/1"/>', "shop.from.month: 13 is not a whole number from 1 to 12"),
    "spawn": ('<floor name="Yukiko\'s Castle 3F" spawn="16"/>', "spawn: 16 is not a whole number from 0 to 15"),
    "rates": ('<encounters id="0" normal-rate="0"/>', "the three rates cannot all be 0"),
    "slot": ('<encounters id="1"><special slot="6" formation="1"/></encounters>', "<special> needs a slot from 1 to 5"),
}
MODES = ("good", "none", "session", "original", *REFUSALS)


def encount_blocks():
    """ENCOUNT rows: 20 formations (17 holds enemy 1), 10 floors (floor 8, the
    named Yukiko's Castle 3F, uses encounter group 1 and treasure group 1), the
    200-byte block, 2 encounter groups (0 has no rates) and 2 treasure groups."""
    formations = bytearray(20 * 24)
    struct.pack_into("<H", formations, 17 * 24 + 8, 1)
    selectors = bytearray(10 * 10)
    struct.pack_into("<HBBBBHH", selectors, 8 * 10, 1, 2, 5, 3, 0, 1, 0)
    groups = bytearray(2 * 248)
    groups[248:248 + 6] = bytes((97, 97, 3, 3, 0, 0))
    struct.pack_into("<HH", groups, 248 + 8, 17, 20)
    records = bytearray(2 * 348)
    struct.pack_into("<HH", records, 348, 10, 0x302)
    return (bytes(formations), bytes(selectors), bytes(200), bytes(groups), bytes(records))


def names(width, rows, named):
    bank = bytearray(width * rows)
    for row, name in named.items():
        bank[row * width:row * width + len(name)] = name.encode("ascii")
    return bytes(bank)


def original_unit():
    unit0, unit1, unit2 = bytearray(336 * 60), bytearray(336 * 32), bytearray(256 * 32)
    for enemy in (1, 2, 3):
        unit0[enemy * 60 + 3] = 5
        struct.pack_into("<H", unit0, enemy * 60 + 4, 50)
        for column in range(16):
            struct.pack_into("<H", unit1, enemy * 32 + column * 2, 0x14)
    return packed((bytes(unit0), bytes(unit1), bytes(unit2)))


def synthetic_disc():
    """Full-size retail-layout banks with a few named rows (no retail data)."""
    elements = bytearray(576 * 2)
    skills = bytearray(440 * 40)
    for skill, element in ((1, 1), (2, 1), (3, 2), (4, 1), (5, 9), (10, 0)):
        elements[skill * 2] = element
    elements[450 * 2:450 * 2 + 2] = b"\xff\x01"
    for skill, kind, cost, power in ((1, 2, 4, 50), (2, 2, 8, 100), (3, 2, 4, 50), (4, 2, 12, 160), (5, 2, 5, 0),
                                     (10, 1, 6, 60)):
        row = 40 * skill
        skills[row + 3] = kind
        struct.pack_into("<H", skills, row + 4, cost)
        skills[row + 0x0E], skills[row + 0x0F], skills[row + 0x10] = 98, 1, 1
        struct.pack_into("<H", skills, row + 0x12, power)
    skills[5 * 40 + 0x18], skills[5 * 40 + 0x19] = 1, 40
    struct.pack_into("<I", skills, 5 * 40 + 0x1C, 0x20)
    unit = original_unit()
    persona0, persona1, party = bytearray(256 * 14), bytearray(256 * 70), bytearray(24 * 622)
    persona0[14 + 2], persona0[14 + 3] = 1, 1                        # Izanagi: Fool, level 1
    struct.pack_into("<BBH", persona1, 70 + 6, 0, 1, 1)              # Agi innate
    struct.pack_into("<BBH", persona1, 70 + 10, 2, 1, 3)             # Bufu at base + 2
    struct.pack_into("<H", party, 0, 2)                              # Jiraiya (0xC0): Yosuke's
    struct.pack_into("<BBH", party, 4, 0, 1, 1)                      # Agi at 0
    struct.pack_into("<BBH", party, 8, 5, 1, 3)                      # Bufu at 5
    msg = (names(21, 32, {1: "Fool", 2: "Magician"}),
           names(19, 576, {1: "Agi", 2: "Agilao", 3: "Bufu", 4: "Agidyne", 5: "Poisma", 6: "Blank", 10: "Bash",
                           450: "Fire Boost"}),
           names(21, 336, {1: "Slime", 2: "Slime", 3: "Hablerie"}),
           names(17, 256, {1: "Izanagi", 0xC0: "Jiraiya"}),
           bytes(16))
    banks = {
        "BATTLE/SKILL.TBL": packed((bytes(elements), bytes(skills))),
        "BATTLE/UNIT.TBL": unit,
        "BATTLE/MSG.TBL": packed(msg),
        "BATTLE/PERSONA.TBL": packed((bytes(persona0), bytes(persona1), bytes(party), bytes(16), bytes(164),
                                      bytes(256 * 4))),
        "BATTLE/EFFECT.TBL": packed((bytes(440 * 4),)),
        "BATTLE/ENCOUNT.TBL": packed(encount_blocks()),
        "BATTLE/MODEL.TBL": packed((bytes(16),) * 3),
        "BATTLE/AICALC.TBL": packed(tuple(bytes(164 if i == 8 else 16) for i in range(11))),
    }
    items = bytearray(16 + 1536 * 68)
    struct.pack_into("<4I", items, 0, 1536, 0, 0, 0)
    medicine = 16 + 0x302 * 68
    struct.pack_into("<I", items, medicine, 3)
    struct.pack_into("<I", items, medicine + 0x24, 1)
    struct.pack_into("<I", items, medicine + 0x34, 850)
    # Gear: Golf Club (1) is never sold (the Blank gate); 5-Iron (3) waits for
    # 10 Supple Metal (0x50D, a material) sold to the weapon store.
    struct.pack_into("<I", items, 16 + 1 * 68 + 0x3C, 0x500 << 8 | 1)
    struct.pack_into("<I", items, 16 + 3 * 68 + 0x3C, 0x50D << 8 | 10)
    struct.pack_into("<I", items, 16 + 0x50D * 68, 5)
    return disc({"DATA": {"INIT/ITEMTBL.BIN": bytes(items)}, "BTL": banks})


def package(root, name, xml=None, luau=None):
    directory = root / name
    (directory / "data").mkdir(parents=True)
    (directory / "mod.ini").write_text(f"[mod]\nid=edits.{name}\nname=Edits {name}\nversion=1.0.0\napi=1.1\n"
                                       "priority=0\norder=0\n", encoding="utf-8", newline="\n")
    if xml is not None:
        (directory / "data" / "edits.xml").write_text(xml, encoding="utf-8", newline="\n")
    if luau is not None:
        (directory / "data.luau").write_text(luau, encoding="utf-8", newline="\n")


class DataEditBoot(unittest.TestCase):
    def test_edit_layers(self):
        for mode in MODES:
            with self.subTest(mode=mode), tempfile.TemporaryDirectory(prefix="p4-data-edit-") as temporary:
                root = Path(temporary)
                enabled = "edits.one"
                if mode == "good":
                    package(root, "one", GOOD_ONE)
                    package(root, "two", GOOD_TWO)
                    enabled = "edits.one,edits.two"
                elif mode == "none":
                    package(root, "one")
                elif mode in ("session", "original"):
                    package(root, "one", GOOD_ONE)
                elif mode == "schema":
                    package(root, "one", '<p4-edits schema="2"><skill name="Agi" power="1"/></p4-edits>\n')
                elif mode == "luau":
                    package(root, "one", luau="p4.data.edit(function(d) d.skill('Agi').power = 1 end)\n")
                else:
                    package(root, "one", f'<p4-edits schema="1">{REFUSALS[mode][0]}</p4-edits>\n')
                image = root / "inputs.iso"
                image.write_bytes(synthetic_disc())
                environment = dict(os.environ, P4PC_MODS="0", P4PC_PACKS="0", P4PC_INPUT="script",
                                   P4PC_CONFIG="none", SDL_AUDIODRIVER="dummy", P4PC_ISO=str(image))
                environment["P4_TEST_ORIGINAL_UNIT_SHA"] = hashlib.sha256(original_unit()).hexdigest()
                if mode == "original":
                    unit0, unit1, unit2 = bytearray(336 * 60), bytearray(336 * 32), bytearray(256 * 32)
                    for enemy in (1, 2, 3):
                        unit0[enemy * 60 + 3] = 37
                    override = root / "override" / "BATTLE"
                    override.mkdir(parents=True)
                    (override / "UNIT.TBL").write_bytes(packed((bytes(unit0), bytes(unit1), bytes(unit2))))
                    environment["P4PC_OVERRIDE_DIR"] = str(override.parent)
                else:
                    environment.pop("P4PC_OVERRIDE_DIR", None)
                result = subprocess.run([*DRIVER, mode, str(root), enabled], env=environment,
                                        capture_output=True, text=True, check=False)
                if result.returncode == 77:
                    self.skipTest("fixed guest mapping unavailable")
                self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
                self.assertIn(f"game_data_edit: {mode} passed", result.stdout)
                if mode in REFUSALS:
                    self.assertIn(REFUSALS[mode][1], result.stdout + result.stderr)


if __name__ == "__main__":
    unittest.main()

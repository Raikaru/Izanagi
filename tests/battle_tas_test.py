#!/usr/bin/env python3
"""Game-free proofs for tools/battle_tas.py: tape text, native menu rules and candidate commands over a sparse fake RAM."""
from pathlib import Path
import json
import struct
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
import battle_tas as tas

SKILLS = 0x01000000
ITEMS = 0x01100000


class FakeRam:
    """Little-endian guest bytes at chosen addresses; everything else reads zero."""
    def __init__(self):
        self.bytes = {}

    def put(self, address, data):
        for i, value in enumerate(data):
            self.bytes[address + i] = value

    def u8(self, address):
        return self.bytes.get(address, 0)

    def u16(self, address):
        return self.u8(address) | self.u8(address + 1) << 8

    def u32(self, address):
        return self.u16(address) | self.u16(address + 2) << 16


def skill(ram, skill_id, listed=True, cost_kind=2, cost=4, everyone=False, side=2, target_filter=0, effect=1, power=40,
          cures=0, sp_effect=0):
    row = SKILLS + skill_id * 0x28
    ram.put(row + 1, [2 if listed else 0])
    ram.put(row + 3, [cost_kind])
    ram.put(row + 4, struct.pack("<H", cost))
    ram.put(row + 8, [1 if everyone else 0, side, target_filter])
    ram.put(row + 0x11, [effect])
    ram.put(row + 0x12, struct.pack("<h", power))
    ram.put(row + 0x14, [sp_effect])
    if cures:
        ram.put(row + 0x18, [2])
        ram.put(row + 0x1C, struct.pack("<I", cures))


def item(ram, item_id, count, skill_id):
    ram.put(tas.STOCK + item_id, [count])
    ram.put(ITEMS + item_id * 0x44 + 0x24, struct.pack("<H", skill_id))


def fixture():
    ram = FakeRam()
    ram.put(tas.SKILL_BANK_SLOT, struct.pack("<I", SKILLS))
    ram.put(tas.ITEM_BANK_SLOT, struct.pack("<I", ITEMS))
    ram.put(tas.PERSONA_SELECTED_SLOT, struct.pack("<H", 2))
    hero = tas.PERSONA_ROSTER + 2 * 0x30 + 0xC
    ram.put(hero, struct.pack("<8H", 10, 11, 12, 13, 14, 0, 0, 0))
    ram.put(tas.PARTY_SLOTS + 0x54 + 0xC, struct.pack("<8H", 20, 0, 0, 0, 0, 0, 0, 0))  # member 2
    skill(ram, 10)                                                  # single-target damage, 4 SP
    skill(ram, 11, cost=50)                                         # more SP than the actor has
    skill(ram, 12, side=1, effect=2, cost=3)                        # single-ally restore
    skill(ram, 13, listed=False)                                    # a passive: not in the Skill list
    skill(ram, 14, cost_kind=1, cost=100)                           # all of max HP: the actor cannot pay
    skill(ram, 20, everyone=True)                                   # every enemy
    skill(ram, 300, cost_kind=0, side=1, target_filter=4, effect=11, power=50)   # revive one
    skill(ram, 301, cost_kind=0, side=1, everyone=True, effect=0, power=1)       # buff every ally
    skill(ram, 302, cost_kind=0, side=1, effect=0, power=15, cures=0x30)         # cures ailments 0x10 and 0x20
    skill(ram, 303, listed=False, cost_kind=0)                                   # not a battle item
    skill(ram, 304, cost_kind=0, side=1, effect=0, power=0, sp_effect=5)         # restores SP only
    item(ram, 0x30F, 1, 300)
    item(ram, 0x316, 2, 301)
    item(ram, 0x319, 1, 302)
    item(ram, 0x31A, 3, 303)
    item(ram, 0x30B, 1, 304)
    item(ram, 0x320, 0, 301)                                        # out of stock
    return ram


LIMITS = {1: {"id": 1, "max_hp": 300, "max_sp": 100}, 2: {"id": 2, "max_hp": 200, "max_sp": 80}}


def state(hero_hp=300, ally_hp=200, ally_status=0, hero_status=0, hero_sp=20, ally_sp=30):
    return {"party": [{"id": 1, "hp": hero_hp, "sp": hero_sp, "status": hero_status},
                      {"id": 2, "hp": ally_hp, "sp": ally_sp, "status": ally_status}],
            "enemies": [{"id": 261, "hp": 5000, "sp": 0, "status": 0}, {"id": 262, "hp": 0, "sp": 0, "status": 0}]}


class BattleTasTest(unittest.TestCase):
    def setUp(self):
        self.party, self.items = tas.menu(fixture(), [1, 2])

    def lines(self, current, actor=1, used=None):
        return [line for _, line in tas.candidates(actor, current, self.party, self.items, LIMITS, used or {})]

    def test_menu_follows_the_native_list_rules(self):
        self.assertEqual([row["id"] for row in self.party[1]], [10, 11, 12, 14])
        self.assertEqual([row["id"] for row in self.party[2]], [20])
        self.assertEqual(sorted(self.items), [0x30B, 0x30F, 0x316, 0x319])

    def test_full_party_offers_attack_guard_affordable_skills_and_buffs(self):
        self.assertEqual(self.lines(state(hero_sp=100, ally_sp=80)), [
            "enemy:261=" + tas.KEYS_ATTACK,
            tas.KEYS_GUARD,
            "enemy:261=" + tas.KEYS_SKILL % 10,
            "enemy:261=" + tas.KEYS_SKILL % 11,
            tas.KEYS_ITEM % 0x316,
        ])

    def test_restores_cures_and_revives_target_only_members_in_need(self):
        lines = self.lines(state(hero_hp=290, ally_hp=0))
        self.assertIn("ally:1=" + tas.KEYS_SKILL % 12, lines)
        self.assertIn("dead:2=" + tas.KEYS_ITEM % 0x30F, lines)
        cured = self.lines(state(ally_status=0x10))
        self.assertIn("ally:2=" + tas.KEYS_ITEM % 0x319, cured)
        self.assertNotIn("ally:1=" + tas.KEYS_ITEM % 0x319, cured)
        self.assertNotIn("ally:2=" + tas.KEYS_ITEM % 0x319, self.lines(state(ally_status=0x8)))  # not its ailment

    def test_silenced_actor_is_offered_no_skills(self):
        lines = self.lines(state(hero_hp=290, hero_status=tas.SILENCE))
        self.assertFalse([line for line in lines if "skill:" in line])
        self.assertIn("enemy:261=" + tas.KEYS_ATTACK, lines)

    def test_weakest_member_by_fraction_receives_the_single_restore(self):
        lines = self.lines(state(hero_hp=200, ally_hp=100))
        self.assertIn("ally:2=" + tas.KEYS_SKILL % 12, lines)
        self.assertNotIn("ally:1=" + tas.KEYS_SKILL % 12, lines)

    def test_sp_restore_targets_only_members_short_of_sp(self):
        self.assertIn("ally:1=" + tas.KEYS_ITEM % 0x30B, self.lines(state(hero_sp=20, ally_sp=60)))
        self.assertIn("ally:2=" + tas.KEYS_ITEM % 0x30B, self.lines(state(hero_sp=90, ally_sp=30)))
        self.assertFalse([line for line in self.lines(state(hero_sp=100, ally_sp=80)) if "item:%d" % 0x30B in line])

    def test_every_enemy_skill_has_no_binding_and_spent_stock_is_not_offered(self):
        self.assertIn(tas.KEYS_SKILL % 20, self.lines(state(), actor=2))
        self.assertNotIn(tas.KEYS_ITEM % 0x316, self.lines(state(), used={0x316: 2}))

    def test_knocked_out_enemy_is_neither_a_target_nor_ranked(self):
        current = state()
        current["enemies"].append({"id": 271, "hp": 1, "sp": 0, "status": tas.KNOCKED_OUT})
        self.assertFalse([line for line in self.lines(current) if "enemy:271=" in line])
        self.assertEqual(tas.score(current, LIMITS), tas.score(state(), LIMITS))

    def test_absent_target_is_read_from_the_walker_failure(self):
        self.assertEqual(tas.bound_target("enemy:262=circle,cross,cross"), ("enemy", 262))
        self.assertEqual(tas.bound_target(tas.KEYS_GUARD), ())
        with tempfile.TemporaryDirectory() as directory:
            node = tas.Node("n1", None, "dead:3=" + tas.KEYS_ITEM % 0x30F, Path(directory), ("item", 0x30F, 3))
            (Path(directory) / "stderr.log").write_bytes(
                b"p4pc: WALK SCRIPT FAILED at command 28 'waittask:battle:off': battle turn 4's ally 3 is absent from TARGET\n")
            self.assertEqual(tas.absent_target(node), ("dead", 3))
            (Path(directory) / "stderr.log").write_bytes(b"p4pc: battle turn 1 for character 1\n")
            self.assertIsNone(tas.absent_target(node))

    def test_unknown_actor_is_an_error(self):
        with self.assertRaisesRegex(tas.SearchError, "acting member"):
            self.lines(state(), actor=7)

    def test_tape_walk_and_prefix(self):
        with tempfile.TemporaryDirectory() as directory:
            walk = Path(directory) / "boss.walk"
            walk.write_bytes(b"# boss\nwait:10\r\nskiptask:battle\nwaitbtl:menu:1\nbattle:off\n")  # a CRLF line too
            prefix = tas.walk_prefix(walk, "skiptask:battle")
            self.assertEqual(prefix, ["# boss", "wait:10", "skiptask:battle"])
            text = tas.tape_walk(prefix, "plan=circle,up,cross;delay=80", ["enemy:261=circle,cross,cross"], 200000)
            self.assertEqual(text.splitlines()[3:], ["timeout:200000", "battle:plan=circle,up,cross;delay=80",
                                                     "battleturn:enemy:261=circle,cross,cross", "waittask:battle:off"])
            self.assertEqual(tas.walk_prefix(walk, count=2), ["# boss", "wait:10"])  # a fight without skiptask:battle
            with self.assertRaisesRegex(tas.SearchError, "6-line prefix"):
                tas.walk_prefix(walk, count=6)
            walk.write_bytes(b"skiptask:battle\nskiptask:battle\n")
            with self.assertRaisesRegex(tas.SearchError, "exactly one"):
                tas.walk_prefix(walk, "skiptask:battle")
        with self.assertRaisesRegex(tas.SearchError, "single"):
            tas.tape_walk([], "plan=circle,up,cross", ["a\nb"], 10)

    def test_live_entry_is_a_fight_only_renderless_flip_clock_request(self):
        entry = tas.live_entry(1, 517, 1, 400000, 200000, 10000)
        self.assertEqual(len(entry), 64)
        fields = struct.unpack("<8s14I", entry)
        self.assertEqual(fields[:10], (b"P4BLIVE1", 64, 3, 1, 517, 1, 400000, 200000, 10000, 1))
        self.assertEqual(fields[10:], (0, 0, 0, 0, 0))

    def test_witness_excludes_only_the_request_ram_hash(self):
        a = b'{"kind":"natural_request","ram_sha256":"aa","flip":1}\n{"kind":"battle_update","update":2}\n'
        b = b'{"kind":"natural_request","ram_sha256":"bb","flip":1}\n{"kind":"battle_update","update":2}\n'
        c = b'{"kind":"natural_request","ram_sha256":"aa","flip":2}\n{"kind":"battle_update","update":2}\n'
        self.assertEqual(tas.without_request_ram_hash(a, "ram_sha256"), tas.without_request_ram_hash(b, "ram_sha256"))
        self.assertNotEqual(tas.without_request_ram_hash(a, "ram_sha256"), tas.without_request_ram_hash(c, "ram_sha256"))

    def test_checkpoint_units_skip_missing_records_and_score_ranks_damage(self):
        checkpoint = {"node": "n1", "linked_units": [
            [{"id": 1, "guest_record": 5, "hp": 300, "sp": 20, "status": 0},
             {"id": 2, "guest_record": None, "hp": None, "sp": None, "status": None}],
            [{"id": 261, "guest_record": 7, "hp": 4000, "sp": 0, "status": 0}]],
            "record_words": [[[0x200, 1, 0, 3, 4, 5], None], [[0, 6, tas.CHARGED, 8, 9, 10]]]}
        units = tas.units(checkpoint)
        self.assertEqual([u["id"] for u in units["party"]], [1])
        self.assertEqual(units["party"][0]["words"], [0x200, 1, 0, 3, 4, 5])
        self.assertEqual(units["enemies"][0]["words"], [0, 6, tas.CHARGED, 8, 9, 10])
        exposed = json.loads(json.dumps(units))
        exposed["party"][0]["words"][0] = 0
        self.assertGreater(tas.score(units, LIMITS), tas.score(exposed, LIMITS))  # guarding under a charge
        calm = json.loads(json.dumps(units))
        calm["enemies"][0]["words"][2] = 0
        self.assertGreater(tas.score(calm, LIMITS), tas.score(units, LIMITS))
        hurt = json.loads(json.dumps(units))
        hurt["enemies"][0]["hp"] = 3900
        self.assertGreater(tas.score(hurt, LIMITS), tas.score(units, LIMITS))
        with self.assertRaisesRegex(tas.SearchError, "linked units"):
            tas.units({"node": "n2", "linked_units": None})

    def test_later_phase_outranks_earlier_form_and_guard_reserve_keeps_guards(self):
        def node(name, enemy_hp, key, phases=()):
            n = tas.Node(name, None, "line", Path(name), key)
            n.state = {"party": [{"id": 1, "hp": 300, "sp": 20, "status": 0}],
                       "enemies": [{"id": 267, "hp": enemy_hp, "sp": 0, "status": 0}]}
            n.phases = frozenset(phases)
            return n
        early = node("n1", 1, ("attack", 267))
        late = node("n2", 8000, ("attack", 270), [270])
        self.assertGreater(tas.rank(late, LIMITS), tas.rank(early, LIMITS))
        ranked = [node("n%d" % i, 100 * i, ("attack", 267)) for i in range(1, 5)] + [node("n9", 5000, ("guard",))]
        self.assertEqual([n.name for n in tas.trim(ranked, 3, 0)], ["n1", "n2", "n3"])
        self.assertEqual([n.name for n in tas.trim(ranked, 3, 1)], ["n1", "n2", "n9"])


if __name__ == "__main__":
    unittest.main()

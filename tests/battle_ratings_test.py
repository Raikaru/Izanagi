import struct
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
import battle_ratings
import battle_sim


class BattleRatingsTest(unittest.TestCase):
    def test_catalog_to_policy_payload(self):
        persona = bytearray(256 * 14)
        growth = bytearray(256 * 70)
        elements = bytearray(0x1b8 * 2)
        skills = bytearray(0x1b8 * 40)
        persona[14 + 2:14 + 4] = bytes((1, 12))
        persona[0xc0 * 14 + 2:0xc0 * 14 + 4] = bytes((1, 12))
        persona[2 * 14:2 * 14 + 4] = bytes((1, 0, 1, 12))
        for slot, skill in enumerate((4, 4, 5)):
            struct.pack_into('<BBH', growth, 70 + 6 + slot * 4, 0, 1, skill)
        elements[4 * 2] = 1
        elements[5 * 2] = 2
        skills[4 * 40 + 0x11] = 1
        skills[5 * 40 + 0x11] = 1
        table = battle_ratings.derive(persona, growth, elements, skills)
        payload, digest = battle_sim.compile_ratings(table)
        self.assertEqual(struct.unpack('<H8b', payload[12:]), (1, 0, 1, 1, 0, 0, 0, 0, 0))
        self.assertEqual(digest, table['digest'])
        with self.assertRaises(ValueError):
            battle_ratings.derive(persona[:-1], growth, elements, skills)


if __name__ == '__main__':
    unittest.main()

#!/usr/bin/env python3
import unittest

from dungeon_digest_project import project


class DungeonDigestProjectionTest(unittest.TestCase):
    def setUp(self):
        self.row = (
            "dungeon digest flip 123 field 9:8 step 7 seed abcdef01 "
            "grid 01020304 rng 11111111 mt 22222222 player 33333333 "
            "units 2:44444444 battle 5 roll 55555555"
        )
        self.expected = (
            "dungeon digest grid 01020304 rng 11111111 mt 22222222 "
            "player 33333333 units 44444444 roll 55555555"
        )

    def test_only_component_hashes_are_public(self):
        self.assertEqual(project(self.row), self.expected)
        changed = self.row.replace("flip 123 field 9:8 step 7 seed abcdef01", "flip 456 field 3:4 step 2 seed deadbeef")
        changed = changed.replace("units 2:", "units 9:").replace("battle 5", "battle 6")
        self.assertEqual(project(changed), self.expected)

    def test_each_hash_is_compared(self):
        for value in ("01020304", "11111111", "22222222", "33333333", "44444444", "55555555"):
            with self.subTest(value=value):
                self.assertNotEqual(project(self.row.replace(value, "aaaaaaaa")), self.expected)

    def test_malformed_rows_fail(self):
        for row in (self.row.replace("grid", "other"), self.row.replace("2:44444444", "44444444"),
                    self.row.replace("01020304", "notahash"), self.row + " extra"):
            with self.subTest(row=row), self.assertRaises(ValueError):
                project(row)


if __name__ == "__main__":
    unittest.main()

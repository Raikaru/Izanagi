#!/usr/bin/env python3
"""Every real-play corpus gate has a state-relative capture, never a fixed-flip fallback."""
import json
from pathlib import Path
import unittest

import event_scene_state


class CaptureRegistrationTest(unittest.TestCase):
    metadata = Path(__file__).resolve().parents[1] / "docs" / "skygl_scenes.json"


    def test_all_seven_real_play_scenes_are_registered(self):
        data = json.loads(self.metadata.read_text())
        registered = {name for name, values in data.items() if "port_capture" in values}
        self.assertEqual(registered, set(event_scene_state.PROCEDURES))
        for scene in sorted(registered):
            with self.subTest(scene=scene):
                fields = event_scene_state.capture_spec(scene, self.metadata, 10000).split(",")
                self.assertEqual(fields[-2:], ["9400", "10600"])
                kind = "event" if "event" in data[scene]["port_capture"] else ("camera" if scene == "near-fog" else "proc")
                self.assertEqual(fields[0], kind)

    def test_nominal_flip_changes_only_sanity_bounds(self):
        for scene in event_scene_state.PROCEDURES:
            with self.subTest(scene=scene):
                first = event_scene_state.capture_spec(scene, self.metadata, 10000).split(",")
                shifted = event_scene_state.capture_spec(scene, self.metadata, 10001).split(",")
                self.assertEqual(first[:-2], shifted[:-2])
                self.assertEqual(shifted[-2:], ["9401", "10601"])

    def test_sensitive_dialogue_phases_are_exact(self):
        self.assertEqual(event_scene_state.capture_spec("tv-world-enter", self.metadata, 75594),
                         "proc,sdl04_14_PM_C,105,20323,118,74994,76194")
        self.assertEqual(event_scene_state.capture_spec("shadow-haze", self.metadata, 110560),
                         "event,sdl04_15_PM_C,111,20419,145,1,0,628,25,1,7,2294016,109960,111160")
        self.assertEqual(event_scene_state.capture_spec("velvet-windows-blurry", self.metadata, 3630),
                         "event,upstair_myroom,6,9068,154,1,0,190,16,0,10,2162944,3030,4230")
        self.assertEqual(event_scene_state.capture_spec("rain", self.metadata, 25100),
                         "event,sdl04_12_AM_A,89,20152,108,1,0,195,0,0,249,655619,24500,25700")

    def test_rankup_uses_registered_procedure_phase(self):
        self.assertEqual(event_scene_state.capture_spec("arcana-rankup-sparkles", self.metadata, 7604),
                         "proc,155_commu_open,0,6,407,7004,8204")


    def test_rankup_snapshot_reads_draw_clock_not_text_fade(self):
        words = {0x00764ADC: 0x01000000, 0x00764AF4: 0, 0x00764AE8: 0,
                 0x01000038: 0x01001000, 0x0100003c: 0,
                 0x01001000: 0x18, 0x01001004: 10, 0x01001008: 604,
                 0x0100100c: 80, 0x01001018: 7, 0x0100101c: 0, 0x01001020: 1}
        self.assertEqual(event_scene_state._rankup_phase(words.__getitem__,
                         lambda fmt, address: (b"cmmRankUpSequence",)), (0x18, 10, 604, 7, 0, 1))

    def test_corridor_uses_field_player_basis_then_offset(self):
        fields = event_scene_state.capture_spec("near-fog", self.metadata, 4600).split(",")
        self.assertEqual(fields[:7], ["camera", "68", "1", "8640", "0", "3930", "1"])
        self.assertEqual(len(fields[7:16]), 9)
        self.assertEqual(fields[16:], ["0.0001", "206", "4000", "5200"])

    def test_missing_registration_does_not_fall_back_to_flip(self):
        with self.assertRaisesRegex(ValueError, "no state-based capture registration"):
            event_scene_state.capture_spec("save-butterfly-glow", self.metadata, 1700)


if __name__ == "__main__":
    unittest.main()

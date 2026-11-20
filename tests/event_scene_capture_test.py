#!/usr/bin/env python3
"""Every real-play corpus gate has a state-relative capture, never a fixed-flip fallback."""
import json
from pathlib import Path
import unittest

import event_scene_state


class CaptureRegistrationTest(unittest.TestCase):
    metadata = Path(__file__).resolve().parent / "fixtures/event_scene_synthetic.json"


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
        self.assertEqual(event_scene_state.capture_spec("tv-world-enter", self.metadata, 10000),
                         "proc,sample_tv,6,60,7,9400,10600")
        self.assertEqual(event_scene_state.capture_spec("shadow-haze", self.metadata, 10000),
                         "event,sample_shadow,4,40,11,12,13,14,15,16,17,18,9400,10600")
        self.assertEqual(event_scene_state.capture_spec("velvet-windows-blurry", self.metadata, 10000),
                         "event,sample_velvet,5,50,21,22,23,24,25,26,27,28,9400,10600")
        self.assertEqual(event_scene_state.capture_spec("rain", self.metadata, 10000),
                         "event,sample_rain,1,10,3,4,5,6,7,8,9,10,9400,10600")

    def test_rankup_uses_registered_procedure_phase(self):
        self.assertEqual(event_scene_state.capture_spec("arcana-rankup-sparkles", self.metadata, 10000),
                         "proc,sample_rank,3,30,5,9400,10600")


    def test_rankup_snapshot_reads_draw_clock_not_text_fade(self):
        words = {0x00764ADC: 0x01000000, 0x00764AF4: 0, 0x00764AE8: 0,
                 0x01000038: 0x01001000, 0x0100003c: 0,
                 0x01001000: 0x18, 0x01001004: 10, 0x01001008: 77,
                 0x0100100c: 80, 0x01001018: 7, 0x0100101c: 0, 0x01001020: 1}
        self.assertEqual(event_scene_state._rankup_phase(words.__getitem__,
                         lambda fmt, address: (b"cmmRankUpSequence",)), (0x18, 10, 77, 7, 0, 1))

    def test_corridor_uses_field_player_basis_then_offset(self):
        fields = event_scene_state.capture_spec("near-fog", self.metadata, 10000).split(",")
        self.assertEqual(fields[:7], ["camera", "90", "2", "10", "20", "30", "40"])
        self.assertEqual(len(fields[7:16]), 9)
        self.assertEqual(fields[16:], ["0.1", "5", "9400", "10600"])

    def test_missing_registration_does_not_fall_back_to_flip(self):
        with self.assertRaisesRegex(ValueError, "no state-based capture registration"):
            event_scene_state.capture_spec("save-butterfly-glow", self.metadata, 10000)


if __name__ == "__main__":
    unittest.main()

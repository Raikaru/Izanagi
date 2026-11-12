import contextlib
import importlib.util
import io
import json
from pathlib import Path
import struct
import sys
import tempfile
import unittest
from unittest import mock

import event_scene_state as gate

METADATA = Path(__file__).resolve().parents[1] / "docs/skygl_scenes.json"


class EventSceneStateTest(unittest.TestCase):
    def setUp(self):
        self.directory = tempfile.TemporaryDirectory()
        self.addCleanup(self.directory.cleanup)
        self.root = Path(self.directory.name)
        self.ram = bytearray(gate.RAM_END - gate.RAM_START)
        self.snapshot = self.root / "state.bin"

    def put(self, address, fmt, *values):
        struct.pack_into(fmt, self.ram, address - gate.RAM_START, *values)

    def scene(self, name):
        expected = json.loads(METADATA.read_text())[name]
        self.put(0x762EA0, "<I", 0x1000000)
        self.put(0x1000000, "<2I", expected["field"], expected["room"])
        self.put(0x7642CC, "<I", 0x1000020)
        self.put(0x1000028, "<h", expected["weather"])
        self.put(0x797B7A, "<H", expected["date"])
        self.put(0x797B7C, "<B", expected["slot"])
        self.put(0x764B70, "<I", 0x1000100)
        self.put(0x1000104, "<I", 0x1000200)
        self.put(0x1000250, "<16f", *expected["ltm"])
        self.put(0x1000168, "<2f", *expected["vw"])
        self.put(0x1000180, "<2f", expected["near"], expected["far"])
        procedure = gate.PROCEDURES[name]
        if procedure:
            self.put(0x764608, "<I", 0x1000800)
            self.put(0x1000800, "<24s", procedure.encode())
            registration = expected["port_capture"]
            self.put(0x1000818, "<I", registration["pc"])
            self.put(0x100092c, "<I", registration["index"])
            self.put(0x1000938, "<I", registration.get("timer", 0))
            if "event" in registration:
                event = registration["event"]
                self.put(0x764ADC, "<I", 0x1001000)
                self.put(0x1001000, "<24s", b"evtEvent")
                self.put(0x1001038, "<I", 0x1001100)
                self.put(0x1001100, "<I", 4)
                self.put(0x100110C, "<I", 0x1001200)
                self.put(0x1001238, "<I", 0x1001300)
                self.put(0x1001A8C, "<3I", *event["id"])
                self.put(0x1001314, "<I", event["frame"])
                self.put(0x1001978, "<5I", 1, 0, 17, 2, event["message"])
                self.put(0x8817EC, "<I", 0x1002000)
                self.put(0x1002008, "<2I", 17, 0x1002100)
                self.put(0x1002100, "<I", 0x00200204)
                self.put(0x100212C, "<H", event["row"])
                self.put(0x1002130, "<I", event["elapsed"])
                self.put(0x764678, "<I", 0x1002300)
                self.put(0x1002324, "<I", 0x1002400)
                self.put(0x1002404, "<2I", 0x0003000D, event["portrait"])

    def check(self, name, nearby=False, *, state_only=False):
        self.snapshot.write_bytes(self.ram)
        with contextlib.redirect_stdout(io.StringIO()):
            gate.check(self.snapshot, name, METADATA, nearby, state_only=state_only)

    def test_exact_camera_and_wrong_weather(self):
        self.scene("rain")
        self.check("rain")
        self.put(0x1000028, "<h", 0)
        with self.assertRaisesRegex(ValueError, "weather"):
            self.check("rain")

    def test_exact_camera_rejects_other_heading(self):
        self.scene("rain")
        self.put(0x1000250, "<f", 0)
        with self.assertRaisesRegex(ValueError, "camera"):
            self.check("rain")

    def test_state_only_is_not_an_image_or_camera_pass(self):
        self.scene("rain")
        self.put(0x1000250, "<f", 0)
        self.check("rain", state_only=True)
        with self.assertRaisesRegex(ValueError, "camera"):
            self.check("rain")
        self.put(0x1000028, "<h", 0)
        with self.assertRaisesRegex(ValueError, "weather"):
            self.check("rain", state_only=True)

    def test_state_only_keeps_the_exact_script_phase(self):
        self.scene("rain")
        registration = json.loads(METADATA.read_text())["rain"]["port_capture"]
        self.put(0x1000818, "<I", registration["pc"] + 1)
        with self.assertRaisesRegex(ValueError, "capture procedure/index/PC"):
            self.check("rain", state_only=True)

    def test_inner_dialogue_phase_does_not_follow_bf_asset_wait_time(self):
        self.scene("velvet-windows-blurry")
        self.put(0x1000938, "<I", 9999)
        self.check("velvet-windows-blurry")

    def test_inner_event_text_and_settled_portrait_are_required(self):
        for address, value in ((0x1001A8C, 146), (0x1001314, 627),
                               (0x1001988, 24), (0x100212C, 0),
                               (0x1002130, 6), (0x1002100, 0x00200102),
                               (0x1002404, 0x00000203), (0x1002408, 0x00210100)):
            with self.subTest(address=hex(address)):
                self.scene("shadow-haze")
                self.put(address, "<I", value)
                for state_only in (False, True):
                    with self.assertRaisesRegex(ValueError, "inner-EVT/message/portrait"):
                        self.check("shadow-haze", state_only=state_only)

    def test_nearby_fog_heading_remains_bounded(self):
        self.scene("near-fog")
        self.put(0x1000250, "<16f", 1, 0, 0, 0, 0, .97562134, .21946083, 0,
                 0, -.21946083, .97562134, 0, 8640, 353.9617, 3221.10718, 0)
        self.check("near-fog", True)
        self.put(0x1000250, "<f", -1)
        with self.assertRaisesRegex(ValueError, "camera axis"):
            self.check("near-fog", True)

    def test_nearby_camera_does_not_relax_other_scenes(self):
        self.scene("rain")
        with self.assertRaisesRegex(ValueError, "only for the fog"):
            self.check("rain", True)

    def test_tv_requires_actual_screen_space_compositor(self):
        self.scene("tv-world-enter")
        self.put(0x1000250, "<f", 0)  # background camera is covered by the opaque compositor
        for state_only in (False, True):
            with self.assertRaisesRegex(ValueError, "compositor"):
                self.check("tv-world-enter", state_only=state_only)
        self.put(0x764ADC, "<I", 0x1000500)
        self.put(0x1000500, "<24s", b"cldDungeonIn")
        self.put(0x1000538, "<I", 0x1000600)
        self.put(0x1000600, "<2i", 1, 104)
        for state_only in (False, True):
            self.check("tv-world-enter", state_only=state_only)
        self.put(0x1000604, "<i", 150)
        for state_only in (False, True):
            with self.assertRaisesRegex(ValueError, "compositor"):
                self.check("tv-world-enter", state_only=state_only)

    def test_region_excludes_only_pixels_outside_its_bounds(self):
        reference, capture = self.root / "reference.rgb", self.root / "capture.rgb"
        reference.write_bytes(bytes(640 * 448 * 3))
        actual = bytearray(640 * 448 * 3)
        actual[(300 * 640) * 3] = 255
        capture.write_bytes(actual)
        with contextlib.redirect_stdout(io.StringIO()):
            gate.check_region(capture, reference, 0, "0,0,640,200")
        with self.assertRaisesRegex(ValueError, "comparison failed"):
            with contextlib.redirect_stdout(io.StringIO()):
                gate.check_region(capture, reference, 0, "0,300,1,1")

    def test_physical_png_region_uses_fractional_area_not_nearest(self):
        tools = str(Path(__file__).resolve().parents[1] / "tools")
        sys.path.insert(0, tools)
        self.addCleanup(sys.path.remove, tools)
        from png_diff import write_png
        reference, capture = self.root / "reference.rgb", self.root / "capture.png"
        image = bytes(c for y in range(2) for x in range(3) for c in (x * 60, y * 100, 200))
        write_png(capture, 3, 2, image)
        expected = bytearray(640 * 448 * 3)
        for y, green in [(223, 0), (224, 100)]:
            offset = (y * 640 + 213) * 3
            expected[offset:offset + 3] = bytes((40, green, 200))
        reference.write_bytes(expected)
        with contextlib.redirect_stdout(io.StringIO()):
            gate.check_region(capture, reference, 0, "213,223,1,2")
        with self.assertRaisesRegex(ValueError, "comparison failed"):
            with contextlib.redirect_stdout(io.StringIO()):
                gate.check_region(capture, reference, 0, "214,223,1,1")


if __name__ == "__main__":
    unittest.main()

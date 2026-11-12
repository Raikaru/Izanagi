"""Synthetic MIPS ELF only: production loader and readonly range capture."""
import pathlib
import struct
import subprocess
import sys
import tempfile
import unittest

SPLIT = sys.argv.index("--driver-end")
COMMAND = sys.argv[1:SPLIT]
sys.argv = [sys.argv[0], *sys.argv[SPLIT + 1:]]


class ImageRanges(unittest.TestCase):
    def image(self, *, count=4, entry_size=40, outside=False, writable_code=False):
        data = bytearray(0x4000)
        ident = b"\x7fELF\x01\x01\x01" + bytes(9)
        struct.pack_into("<16sHHIIIIIHHHHHH", data, 0, ident, 2, 8, 1,
                         0x100000, 52, 0x3000, 0, 52, 32, 1,
                         entry_size, count, 0)
        struct.pack_into("<IIIIIIII", data, 52, 1, 0x1000, 0x100000,
                         0x100000, 0x400, 0x4000, 7, 0x1000)
        data[0x1000:0x1400] = b"\xa5" * 0x400
        sections = [
            (0, 1, 7 if writable_code else 6, 0x100000, 0x1000, 0x100, 0, 0, 16, 0),
            (0, 1, 3, 0x100100, 0x1100, 0x100, 0, 0, 16, 0),
            (0, 8, 3, 0x104000 if outside else 0x101000, 0, 0x1000, 0, 0, 16, 0),
        ]
        for i, section in enumerate(sections, 1):
            struct.pack_into("<IIIIIIIIII", data, 0x3000 + i * 40, *section)
        return data

    def invoke(self, **kwargs):
        with tempfile.TemporaryDirectory(prefix="p4-image-ranges-") as directory:
            path = pathlib.Path(directory) / "synthetic.elf"
            path.write_bytes(self.image(**kwargs))
            result = subprocess.run([*COMMAND, str(path)], text=True,
                                    capture_output=True, check=True, timeout=20)
        self.assertIn("entry=00100000", result.stdout)
        self.assertIn("loaded=a5 bss=00", result.stdout)
        return result.stdout

    def test_actual_allocated_code_data_and_bss(self):
        out = self.invoke()
        self.assertIn("ranges=3", out)
        self.assertIn("range=00100000:00100100:2", out)
        self.assertIn("range=00100100:00100200:1", out)
        self.assertIn("range=00101000:00102000:1", out)

    def test_missing_malformed_or_unloaded_sections_do_not_change_boot(self):
        for kwargs in ({"count": 0}, {"count": 65535},
                       {"entry_size": 39}, {"outside": True}):
            with self.subTest(kwargs=kwargs):
                self.assertIn("ranges=0", self.invoke(**kwargs))

    def test_writable_executable_section_keeps_code_denial_evidence(self):
        self.assertIn("range=00100000:00100100:3", self.invoke(writable_code=True))

    def test_truncated_load_segment_reports_real_eof(self):
        with tempfile.TemporaryDirectory(prefix="p4-image-short-") as directory:
            path = pathlib.Path(directory) / "truncated.elf"
            path.write_bytes(self.image()[:0x1200])
            result = subprocess.run([*COMMAND, str(path)], text=True,
                                    capture_output=True, timeout=20)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("p4pc: read PT_LOAD: unexpected end of file", result.stderr)


if __name__ == "__main__":
    unittest.main()

"""Real cooperative kernel/OS counter fixture, not game performance evidence."""
import os
import re
import subprocess
import sys
import unittest

separator = sys.argv.index("--driver-end")
DRIVER = sys.argv[1:separator]
sys.argv = [sys.argv[0], *sys.argv[separator + 1:]]
SAMPLE = re.compile(
    r"^p4pc frame_cpu: first (\d+) last (\d+) frames (\d+) "
    r"cpu_ns (\d+) wall_ns (\d+) quantum_ns (\d+) virtual (\d+)$", re.MULTILINE
)


class FrameCpu(unittest.TestCase):
    def invoke(self, enabled):
        environment = dict(os.environ, P4PC_MODS="0", P4PC_PACKS="0",
                           P4PC_INPUT="script", P4PC_CONFIG="none")
        environment.pop("P4PC_FRAME_CPU_LOG", None)
        if enabled:
            environment["P4PC_FRAME_CPU_LOG"] = "1"
        result = subprocess.run([*DRIVER, "--frame-cpu"], env=environment,
                                capture_output=True, text=True, timeout=90, check=False)
        text = result.stdout + result.stderr
        self.assertEqual(result.returncode, 0, text)
        self.assertIn("frame_cpu_fixture synthetic=true reference_eligible=false", text)
        self.assertIn("result=PASS", text)
        return text

    def test_disabled_logger_emits_nothing(self):
        self.assertNotIn("p4pc frame_cpu:", self.invoke(False))

    def test_enabled_logger_uses_contiguous_real_counter_windows(self):
        text = self.invoke(True)
        samples = [tuple(map(int, match)) for match in SAMPLE.findall(text)]
        self.assertGreaterEqual(len(samples), 2, text)
        self.assertNotIn("unavailable", text)
        self.assertNotIn("non-monotonic", text)
        previous = None
        for first, last, frames, cpu, wall, quantum, virtual in samples:
            self.assertEqual(last - first, frames)
            self.assertGreaterEqual(frames, 1000)
            self.assertGreater(wall, 0)
            self.assertGreater(quantum, 0)
            self.assertEqual(virtual, 1)
            # A short fixture may fall within one Windows accounting quantum.
            self.assertGreaterEqual(cpu, 0)
            if previous is not None:
                self.assertEqual(first, previous)
            previous = last


if __name__ == "__main__":
    unittest.main()

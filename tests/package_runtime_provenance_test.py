#!/usr/bin/env python3
"""Exercise the package provenance recorder against the installed Debian toolchain."""
import hashlib
from pathlib import Path
import platform
import re
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]


class RuntimeProvenanceTest(unittest.TestCase):
    def test_debian_static_runtime_sources_and_hashes(self):
        machine = platform.machine()
        if (platform.system() != "Linux" or machine not in ("x86_64", "aarch64")
                or not Path("/usr/bin/clang++").is_file() or not shutil.which("dpkg-query")):
            self.skipTest("Debian clang toolchain is unavailable")
        with tempfile.TemporaryDirectory() as tmp:
            report = Path(tmp) / "runtime.txt"
            subprocess.run(["sh", str(ROOT / "tools/package_runtime_provenance.sh"),
                            "linux", machine + "-linux-gnu", str(report)], check=True)
            text = report.read_text(encoding="utf-8")
            self.assertIn("Debian source archives: https://snapshot.debian.org/package/", text)
            self.assertIn("Source: gcc-", text)
            archives = re.findall(r"^([0-9a-f]{64})  (/.+\.a)$", text, re.MULTILINE)
            names = {Path(path).name for _, path in archives}
            self.assertIn("libstdc++.a", names)
            self.assertIn("libgcc.a", names)
            for digest, path in archives:
                self.assertEqual(digest, hashlib.sha256(Path(path).read_bytes()).hexdigest())


if __name__ == "__main__":
    unittest.main()

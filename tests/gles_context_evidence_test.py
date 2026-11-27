#!/usr/bin/env python3
"""The context evidence writer is bounded and never overwrites prior evidence."""
import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]


class GLContextEvidenceTest(unittest.TestCase):
    def test_renderer_is_escaped_bounded_and_exclusive(self):
        compiler = shutil.which("cc")
        if not compiler:
            self.skipTest("C compiler unavailable")
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            source, binary = root / "probe.c", root / "probe"
            source.write_text('#include "gles_context_evidence.h"\n'
                              'int main(int argc, char **argv) { '
                              'if (argc != 2) return 2; p4pc_write_renderer_evidence(argv[1], 1, 0); return 0; }\n')
            subprocess.run([compiler, "-std=c11", "-Wall", "-Wextra", "-Werror", "-I", str(ROOT / "tests"),
                            str(source), "-o", str(binary)], check=True)
            evidence = root / "renderer.json"
            env = dict(os.environ, P4PC_RENDERER_EVIDENCE=str(evidence))
            subprocess.run([str(binary), 'llvmpipe (a "quoted" \\ path)'], env=env, check=True)
            original = evidence.read_bytes()
            self.assertEqual(json.loads(original), {"schema": 1, "kind": "gl_context",
                                                   "renderer": 'llvmpipe (a "quoted" \\ path)',
                                                   "is_es": True, "fallback": False})
            subprocess.run([str(binary), "different"], env=env, check=True)
            self.assertEqual(evidence.read_bytes(), original)
            for bad in ("", "a\nb", "a" * 513):
                absent = root / ("bad-%d.json" % len(bad))
                subprocess.run([str(binary), bad], env=dict(env, P4PC_RENDERER_EVIDENCE=str(absent)), check=True)
                self.assertFalse(absent.exists())


if __name__ == "__main__":
    unittest.main()

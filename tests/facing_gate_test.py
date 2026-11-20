#!/usr/bin/env python3
"""Compile the facing gate with synthetic and, when present, private references."""
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
PRIVATE = ROOT / "src/platform/newgame_gate_poses_private.inc"
SYNTHETIC = ROOT / "tests/fixtures/facing_synthetic.inc"
PRELUDE = r'''#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned sFacingSamples, sFacingBad;
static float sFacingWorst = 1.0f;
'''
WITNESS = r'''int main(void) {
    int i;
    unsetenv("P4PC_FACING_COMPONENT_MASK");
    sFacingSamples = 40;
    for (i = 0; i < P4_POSES; i++) sPoseSeen[i] = 1;
    assert(p4pc_facing_gate_check(14));
    for (i = 0; i < P4_POSES; i++) {
        sPoseSeen[i] = 0;
        assert(!p4pc_facing_gate_check(14));
        sPoseSeen[i] = 1;
    }
    sFacingSamples = 39;
    assert(!p4pc_facing_gate_check(14));
    sFacingSamples = 40;
    sFacingBad = 1;
    assert(!p4pc_facing_gate_check(14));
    sFacingBad = 0;
    for (i = 3; i < P4_POSES; i++) sPoseSeen[i] = 0;
    assert(p4pc_facing_gate_check(10));
    sPoseSeen[0] = 0;
    assert(!p4pc_facing_gate_check(10));
    memset(sPoseSeen, 0, sizeof sPoseSeen);
    for (i = 3; i < P4_POSES; i++) sPoseSeen[i] = 1;
    sFacingSamples = 0; /* The boot component owns the original global minimum. */
    setenv("P4PC_FACING_COMPONENT_MASK", "0x78", 1);
    assert(p4pc_facing_gate_check(11));
    for (i = 3; i < P4_POSES; i++) {
        sPoseSeen[i] = 0;
        assert(!p4pc_facing_gate_check(11));
        sPoseSeen[i] = 1;
    }
    setenv("P4PC_FACING_COMPONENT_MASK", "0", 1);
    memset(sPoseSeen, 0, sizeof sPoseSeen);
    assert(p4pc_facing_gate_check(14));
    sFacingBad = 1;
    assert(!p4pc_facing_gate_check(14));
    sFacingBad = 0;
    setenv("P4PC_FACING_COMPONENT_MASK", "0x80", 1);
    assert(!p4pc_facing_gate_check(14));
    setenv("P4PC_FACING_COMPONENT_MASK", "", 1);
    assert(!p4pc_facing_gate_check(14));
    return 0;
}
'''

class FacingGateTest(unittest.TestCase):
    def setUp(self):
        self.compiler = shutil.which("cc")
        if os.name == "nt" or not self.compiler:
            self.skipTest("compiled facing witness runs on the Linux check host")
        self.temporary = tempfile.TemporaryDirectory(prefix="p4-facing-")
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        source = (ROOT / "src/platform/newgame_gate.c").read_text(encoding="utf-8")
        start = source.index("int p4pc_facing_gate_check(int date)")
        end = source.index("\n}\n", start) + 3
        self.function = source[start:end]

    def compile_and_run(self, code):
        unit = self.root / "facing.c"
        binary = self.root / "facing"
        unit.write_text(code, encoding="utf-8")
        subprocess.run([self.compiler, "-std=c11", "-O2", "-Wall", "-Wextra", str(unit), "-o", str(binary)], check=True)
        subprocess.run([str(binary)], check=True)

    def reference(self, path):
        poses = path.read_text(encoding="utf-8")
        declarations = "\nenum { P4_POSES = sizeof sPoses / sizeof sPoses[0] };\nstatic int sPoseSeen[P4_POSES];\n"
        self.compile_and_run("#define P4_FACING_POSES 1\n" + PRELUDE + poses + declarations + self.function + WITNESS)

    def test_synthetic_reference(self):
        self.reference(SYNTHETIC)

    @unittest.skipUnless(PRIVATE.is_file(), "private reference not present")
    def test_private_reference(self):
        self.reference(PRIVATE)

    def test_public_movement_only_mode(self):
        witness = r'''int main(void) {
    unsetenv("P4PC_FACING_COMPONENT_MASK");
    sFacingSamples = 39;
    assert(!p4pc_facing_gate_check(14));
    sFacingSamples = 40;
    assert(p4pc_facing_gate_check(14));
    sFacingBad = 1;
    assert(!p4pc_facing_gate_check(14));
    sFacingBad = 0;
    setenv("P4PC_FACING_COMPONENT_MASK", "1", 1);
    assert(!p4pc_facing_gate_check(14));
    return 0;
}
'''
        self.compile_and_run("#define P4_FACING_POSES 0\n" + PRELUDE + self.function + witness)

if __name__ == "__main__":
    unittest.main()

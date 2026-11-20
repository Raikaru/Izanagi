#!/usr/bin/env python3
"""tools/wingl.py: the Windows GL host tests' Wine commands mapped to native ones, and everything else refused."""
from pathlib import Path
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
import wingl  # noqa: E402

TREE, SRC = "/lane/build", "/lane/src"
ROOTS = {"abi": "x64", "tree": TREE, "src": SRC}


def test(name, command, **properties):
    rows = [{"name": key, "value": value} for key, value in properties.items()]
    return {"name": name, "command": command, "properties": rows}


WINE64 = ["/usr/bin/cmake", "-E", "env", "WINEDEBUG=-all", "WINEPREFIX=/lane/build/tests/wine64prefix"]


class MappingTest(unittest.TestCase):
    def plan(self, entry, roots=ROOTS):
        return wingl.plan(entry, roots, "B:/bundle", "O:/out", "S:/src", python="PY")

    def test_a_wine_command_runs_the_bundled_exe_with_its_environment(self):
        entry = test("render_smoke_es3064w", WINE64 + ["/usr/bin/cmake", "-E", "env", "P4PC_GL_API=gles", "/usr/bin/wine",
                                                        "/lane/build/win64/render_smoke64w.exe", "/lane/build/64w_render_smoke_es30.png"],
                     ENVIRONMENT=["P4PC_SAVE_DIR=/lane/build/save/render_smoke_es3064w", "P4PC_CONFIG=none"],
                     WORKING_DIRECTORY="/lane/build/tests/host64w/render_smoke_es30", SKIP_RETURN_CODE=77)
        job = self.plan(entry)
        self.assertEqual(job["argv"], ["B:/bundle/tree/win64/render_smoke64w.exe", "O:/out/64w_render_smoke_es30.png"])
        self.assertEqual(job["env"], {"P4PC_GL_API": "gles", "P4PC_SAVE_DIR": "O:/out/save/render_smoke_es3064w",
                                      "P4PC_CONFIG": "none"})
        self.assertEqual(job["cwd"], "O:/out/tests/host64w/render_smoke_es30")
        self.assertEqual((job["timeout"], job["skip"]), (1500.0, 77))

    def test_an_exe_that_cmake_env_runs_without_an_emulator_is_its_own_command(self):
        roots = dict(ROOTS, abi="x32", tree="/lane/build/win32")
        entry = test("render_smoke_gl33", ["/usr/bin/cmake", "-E", "env", "P4PC_GL_API=gl", "/lane/build/win32/tests/render_smoke.exe",
                                           "/lane/build/win32/render_smoke_gl33.png"], TIMEOUT=120.0)
        job = self.plan(entry, roots)
        self.assertEqual(job["argv"], ["B:/bundle/tree/tests/render_smoke.exe", "O:/out/render_smoke_gl33.png"])
        self.assertEqual((job["env"], job["cwd"], job["timeout"], job["skip"]), ({"P4PC_GL_API": "gl"}, "O:/out", 120.0, None))

    def test_a_python_driver_loses_its_wine_launcher_word(self):
        entry = test("mod_resources_boot64w", WINE64 + ["/usr/bin/python3", "/lane/src/tests/mod_resources_boot_test.py",
                                                         "/usr/bin/wine", "/lane/build/win64/mod_resources_boot64w.exe", "--driver-end"])
        self.assertEqual(self.plan(entry)["argv"],
                         ["PY", "S:/src/tests/mod_resources_boot_test.py", "B:/bundle/tree/win64/mod_resources_boot64w.exe", "--driver-end"])
        bare = test("renderer_reference_scope", ["/usr/bin/python3", "/lane/src/tests/renderer_scope_test.py",
                                                 "/lane/build/tests/render_smoke.exe", "/lane/build/scope.png"])
        self.assertEqual(self.plan(bare)["argv"], ["PY", "S:/src/tests/renderer_scope_test.py", "B:/bundle/tree/tests/render_smoke.exe",
                                                   "O:/out/scope.png"])
        self.assertEqual(wingl.native(entry, TREE, SRC)[2], {"win64/mod_resources_boot64w.exe"})

    def test_anything_else_is_refused(self):
        refused = {
            "a script gate": ["/usr/bin/sh", "/lane/src/tests/menu_gate.sh", "/lane/build/win32/p4pc.exe"],
            "a host binary": ["/lane/build/tests/gl_cache_test"],
            "wine running a host path": ["/usr/bin/wine", "/usr/lib/x.exe"],
            "a path outside the tree": ["/usr/bin/wine", "/lane/build/win64/a.exe", "/usr/share/data"],
            "an option's path outside the tree": ["/usr/bin/wine", "/lane/build/win64/a.exe", "--data=/opt/p4/x"],
            "cmake env unsetting": ["/usr/bin/cmake", "-E", "env", "--unset=HOME", "/usr/bin/wine", "/lane/build/win64/a.exe"],
            "a driver outside the checkout": ["/usr/bin/python3", "/tmp/x.py", "/lane/build/win64/a.exe"],
            "a driver with two launchers": ["/usr/bin/python3", "/lane/src/t.py", "wine", "/lane/build/a.exe", "wine", "/lane/build/b.exe"],
            "a driver that runs no exe": ["/usr/bin/python3", "/lane/src/t.py", "wine", "--help"],
        }
        for why, command in refused.items():
            with self.subTest(why), self.assertRaises(wingl.Unmappable):
                self.plan(test("t", command))
        with self.assertRaises(wingl.Unmappable):   # an environment value outside the tree
            self.plan(test("t", ["/usr/bin/wine", "/lane/build/win64/a.exe"], ENVIRONMENT=["DATA=/usr/share/x"]))
        with self.assertRaises(wingl.Unmappable):   # a working directory in the checkout
            self.plan(test("t", ["/usr/bin/wine", "/lane/build/win64/a.exe"], WORKING_DIRECTORY="/lane/src/tests"))
        self.assertTrue(wingl.script_gate(test("t", refused["a script gate"])))
        self.assertFalse(wingl.script_gate(test("t", WINE64 + ["/usr/bin/wine", "/lane/build/win64/a.exe"])))

    def test_the_filters_are_the_documented_ctest_selections(self):
        self.assertEqual(wingl.FILTERS["x64"], ["-L", "^gl$", "-L", "^port64win$", "-FS", "^wine64_ready$"])
        self.assertEqual(wingl.FILTERS["x32"], ["-L", "^gl$", "-LE", "scenes"])


if __name__ == "__main__":
    unittest.main()

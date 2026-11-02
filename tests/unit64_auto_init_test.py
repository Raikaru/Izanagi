#!/usr/bin/env python3
"""Rewritten-mirror compiler flags, generation-only output and census contracts."""
from contextlib import redirect_stderr, redirect_stdout
import io
import os
from pathlib import Path
import sys
import tempfile
from types import SimpleNamespace
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
sys.path.insert(0, str(ROOT / "tools" / "port64"))
import gen_port
import unit64


def setUpModule():
    global _cache_temp, _cache_env
    _cache_temp = tempfile.TemporaryDirectory()
    _cache_env = patch.dict(os.environ, {
        "P4PC_BUILD_CACHE_DIR": str(Path(_cache_temp.name) / "cache"),
        "P4PC_BUILD_CACHE_DISABLE": "0",
        "P4PC_BUILD_TOOLCHAIN_ID": "sha256:" + "1" * 64,
        "P4PC_BUILD_TRACE_DIR": str(Path(_cache_temp.name) / "trace"),
    })
    _cache_env.start()


def tearDownModule():
    _cache_env.stop()
    _cache_temp.cleanup()


class AutoInitTest(unittest.TestCase):
    def compile_command(self, mode):
        """Exercise the mirror path with cmd_build's complete CLI argument shape."""
        with tempfile.TemporaryDirectory() as work:
            directory = Path(work)
            original = directory / "preprocessed.i"
            original.write_text("float f(void) { float yaw; return yaw; }\n", encoding="utf-8")
            args = SimpleNamespace(gen=work, root=str(ROOT), unit="sample.c", out=str(directory / "sample.o"),
                                   abi=None, check=True, launcher="", clang="clang", opt="O2", auto_init=mode, keep_i=False,
                                   src=None, cflag=[], generate_only=False)
            rewritten = {"unsupported": "", "census": "", "text": original.read_text()}
            diagnostic = "sample.c:1:35: warning: variable 'yaw' is uninitialized when used here [-Wuninitialized]\n"
            with patch.object(unit64, "unit_flags", return_value=(str(original), [])), \
                    patch.object(gen_port, "read_symbols", return_value={}), \
                    patch.object(gen_port, "read_port_symbols", return_value={}), \
                    patch.object(unit64, "rewrite_outputs", return_value=(str(original), rewritten)), \
                    patch.object(unit64.subprocess, "run", return_value=SimpleNamespace(returncode=0, stderr=diagnostic)) as compiler, \
                    redirect_stderr(io.StringIO()) as stderr:
                unit64.cmd_build(args)
            self.assertFalse(Path(args.out + ".i").exists())
            retained = Path(args.out + ".uninitialized")
            return compiler.call_args.args[0], stderr.getvalue(), retained.read_text() if retained.exists() else None

    def test_both_modes_reach_final_compiler(self):
        for mode in ("pattern", "zero"):
            with self.subTest(mode=mode):
                command, diagnostic, retained = self.compile_command(mode)
                flag = "-ftrivial-auto-var-init=" + mode
                self.assertIn(flag, command)
                self.assertGreater(command.index(flag), command.index("-Wno-everything"))
                self.assertIn("-Wuninitialized", command)
                self.assertIn("-Wsometimes-uninitialized", command)
                self.assertIn("sample.c:1:35", diagnostic)
                self.assertIn("sample.c:1:35", retained)

    def test_shipping_default_is_unchanged(self):
        command, diagnostic, retained = self.compile_command(None)
        self.assertFalse(any(word.startswith("-ftrivial-auto-var-init=") for word in command))
        self.assertNotIn("-Wuninitialized", command)
        self.assertNotIn("sample.c:1:35", diagnostic)
        self.assertIsNone(retained)

    def test_generation_only_retains_source_and_census_without_compiling(self):
        with tempfile.TemporaryDirectory() as work:
            directory = Path(work)
            original = directory / "preprocessed.i"
            text = "int sample(void) { return 0; }\n"
            original.write_text(text, encoding="utf-8")
            args = SimpleNamespace(gen=work, root=str(ROOT), unit="sample.c", out=str(directory / "sample.c.64.o"),
                                   abi=None, check=True, launcher="", clang="clang", opt="O2", auto_init=None, keep_i=False,
                                   src=None, cflag=[], generate_only=True)
            rewritten = {"unsupported": "", "census": "pointer%20load=2\n", "text": text}
            with patch.object(unit64, "unit_flags", return_value=(str(original), [])), \
                    patch.object(gen_port, "read_symbols", return_value={}), \
                    patch.object(gen_port, "read_port_symbols", return_value={}), \
                    patch.object(unit64, "rewrite_outputs", return_value=(str(original), rewritten)) as rewrite, \
                    patch.object(unit64.subprocess, "run") as compiler:
                unit64.cmd_build(args)
            compiler.assert_not_called()
            self.assertFalse(Path(args.out).exists())
            self.assertEqual(Path(args.out + ".i").read_text(), text)
            self.assertEqual(Path(args.out + ".census").read_text(), "pointer%20load=2\n")
            self.assertEqual(rewrite.call_args.args[-1], (args.out + ".d", args.out + ".i"))

    def test_generated_census_drift_is_rejected(self):
        with tempfile.TemporaryDirectory() as work:
            directory = Path(work)
            units = directory / "units"
            units.mkdir()
            (units / "sample.c.64.o.census").write_text("abi=2\n", encoding="utf-8")
            active = directory / "active_units.txt"
            active.write_text("sample.c\n", encoding="utf-8")
            baseline = directory / "reviewed.txt"
            unit64.write_census_file(baseline, [{"major": 23, "patch": False, "units": {"sample.c": {"abi": 3}}}])
            args = SimpleNamespace(dir=str(units), census_baseline=str(baseline), census_refresh=None,
                                   clang="fixture-clang", active_units=str(active), baseline=None)
            output = io.StringIO()
            with patch.object(unit64, "clang_major", return_value=23), redirect_stdout(output), \
                    self.assertRaises(SystemExit) as error:
                unit64.cmd_report(args)
            self.assertEqual(error.exception.code, 1)
            self.assertIn("sample.c: rule abi: baseline 3, now 2", output.getvalue())



if __name__ == "__main__":
    unittest.main()

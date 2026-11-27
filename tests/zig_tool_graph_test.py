"""Exercise representative real Zig tooling registrations in synthetic trees.

Set P4_ZIG to the pinned Zig nightly (or put it on PATH). No game inputs are used.
"""
import json
import os
from pathlib import Path
import shutil
import subprocess
import runpy
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
ZIG = os.environ.get("P4_ZIG") or shutil.which("zig")
FRAGMENTS = (
    "250-tools-private.cmake", "252-orchestrator-private.cmake", "260-platform-checks-private.cmake",
    "251-captures.cmake", "253-client-wait.cmake", "254-derived-artifacts.cmake",
    "258-cold-residency.cmake", "261-platform-checks.cmake", "262-land-result-reuse.cmake",
    "263-land-rebase-queue.cmake", "265-public-export.cmake", "266-mod-guard-timing.cmake",
)
# These are independent representative contracts, not a parse of the graph.
SCRIPTS = {
    "zig_abi": "zig_abi_test.py",                 # retained public script
    "gs_audit": "gs_audit_test.py",               # private tools fragment
    "land_model": "land_model_test.py",           # private orchestrator fragment
    "quarantine": "quarantine_test.py",           # private platform fragment
    "capture_archive": "capture_archive_test.py", # private standalone fragment
    "land_client_wait": "land_client_wait_test.py",
    "remote_check_host": "remote_check_test.py",   # independently selectable serial test
    "mod_guard_timing_contract": "mod_guard_timing_test.py",
}


@unittest.skipUnless(ZIG, "P4_ZIG or zig is required")
class ToolGraphTest(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix="zig-tool-graph-")
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        for directory in ("zigbuild", "tools", "tests/cmake"):
            (self.root / directory).mkdir(parents=True)
        shutil.copyfile(ROOT / "zigbuild/tool_tests.zig", self.root / "zigbuild/tool_tests.zig")
        shutil.copyfile(ROOT / "tools/zig_tool_tests.py", self.root / "tools/zig_tool_tests.py")
        for name, file in SCRIPTS.items():
            # A real child process verifies the runner's safety environment and
            # records completion in the isolated build working directory.
            script = '''import os
from pathlib import Path
assert os.environ.get("P4PC_MODS") == "0"
assert os.environ.get("P4PC_PACKS") == "0"
assert os.environ.get("P4PC_BUNDLED_MOD_DIR") == ""
'''
            if name == "mod_guard_timing_contract":
                script += 'assert os.environ.get("P4PC_INPUT") == "script"\n'
            script += f'Path({(name + ".completed")!r}).write_text("completed", encoding="ascii")\n'
            (self.root / "tests" / file).write_text(script, encoding="utf-8")
        python = json.dumps(sys.executable.replace('\\', '/'))
        (self.root / "build.zig").write_text('''
const std = @import("std");
const tools = @import("zigbuild/tool_tests.zig");
fn rootPath(b: *std.Build, p: []const u8) []const u8 {
    return b.pathJoin(&.{ b.root.toString(b.allocator) catch @panic("OOM"), p });
}
fn exists(b: *std.Build, p: []const u8) bool {
    std.Io.Dir.cwd().access(b.graph.io, rootPath(b, p), .{}) catch return false;
    return true;
}
pub fn build(b: *std.Build) void {
    const all = b.step("nonserial", "nonserial");
    const suite = b.step("suite", "suite");
    suite.dependOn(all);
    const graph = b.step("replacement", "replacement");
    tools.register(b, all, .{
        .python = PYTHON, .clang = "clang", .nm = "nm", .objdump = "objdump",
        .working_dir = rootPath(b, "work"), .lock_file = rootPath(b, "lock"),
        .private_tools = exists(b, "tests/cmake/250-tools-private.cmake"),
        .private_orchestrator = exists(b, "tests/cmake/252-orchestrator-private.cmake"),
        .private_platform = exists(b, "tests/cmake/260-platform-checks-private.cmake"),
        .native_linux_with_cxx = false, .cmake_sources_replacement = graph,
        .host_adapter_graph_replacement = null, .remote_check_host_serial = suite,
    });
}
'''.replace("PYTHON", python), encoding="utf-8")

    def fragments(self, names):
        for name in names:
            (self.root / "tests/cmake" / name).write_text("# synthetic fragment\n", encoding="utf-8")

    def public_export(self):
        (self.root / "zigbuild/public-export-mode.txt").write_text("izanagi-public-export-v1", encoding="utf-8")

    def build(self, name):
        return subprocess.run([ZIG, "build", "zig-tool-test-" + name, "-j1", "--summary", "none"], cwd=self.root,
                              env=dict(os.environ, ZIG_LOCAL_CACHE_DIR=str(self.root / ".zig-cache")),
                              stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, timeout=300)

    def assert_executed(self, name):
        result = self.build(name)
        self.assertEqual(result.returncode, 0, result.stdout)
        self.assertTrue((self.root / "work" / (name + ".completed")).is_file(), result.stdout)

    def assert_not_executed(self, name):
        result = self.build(name)
        self.assertNotEqual(result.returncode, 0, result.stdout)
        self.assertFalse((self.root / "work" / (name + ".completed")).exists(), result.stdout)
        return result.stdout

    def test_qualified_optimization_defaults_and_drift_census(self):
        census = runpy.run_path(str(ROOT / "tools/zig_build_graph_test.py"))
        shutil.copyfile(ROOT / "zigbuild/core.zig", self.root / "zigbuild/core.zig")
        shutil.copyfile(ROOT / "zigbuild/zig_cc.zig", self.root / "zigbuild/zig_cc.zig")
        (self.root / "build.zig").write_text(r'''
const std = @import("std");
const core = @import("zigbuild/core.zig");
pub fn build(b: *std.Build) void {
    const options = core.readOptions(b);
    std.debug.print("P4_OPTIMIZATION {s} {s} {s}\n", .{options.opt, options.port64_opt, options.port64w_opt});
}
''', encoding="utf-8")

        def probe(*flags):
            result = subprocess.run([ZIG, "build", *flags, "-j1", "--summary", "none"], cwd=self.root,
                                    env=dict(os.environ, ZIG_LOCAL_CACHE_DIR=str(self.root / ".zig-cache")),
                                    stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, timeout=300)
            self.assertEqual(result.returncode, 0, result.stdout)
            rows = [line.split()[1:] for line in result.stdout.splitlines() if line.startswith("P4_OPTIMIZATION ")]
            self.assertEqual(len(rows), 1, result.stdout)
            return dict(zip(("opt", "port64_opt", "port64w_opt"), rows[0], strict=True))

        expected = census["qualified_optimizations"]()
        census["compare_optimizations"](probe(), expected)
        flags = {"opt": "opt", "port64_opt": "port64-opt", "port64w_opt": "port64w-opt"}
        variables = {value: key for key, value in census["OPTIMIZATIONS"].items()}
        for name in expected:
            with self.subTest(profile=name):
                actual = probe("-D" + flags[name] + "=O0")
                with self.assertRaisesRegex(census["Failure"], "optimization drift for " + name):
                    census["compare_optimizations"](actual, expected)
                # An intentional diagnostic override must match the actual CMake
                # cache, not bypass or lower the census assertion.
                cache = self.root / "CMakeCache.txt"
                cache.write_text("".join(f"{variables[key]}:STRING={value}\n" for key, value in actual.items()),
                                 encoding="utf-8")
                census["compare_optimizations"](actual, census["cache_optimizations"](cache))

    def test_mod_game_cmake_source_drift_is_rejected(self):
        graph = runpy.run_path(str(ROOT / "tools/zig_build_graph_test.py"))
        text = (ROOT / "tools/mods/CMakeLists.txt").read_text(encoding="utf-8")
        for active in (False, True):
            expected = graph["cmake_mod_game_sources"](text, active)
            graph["compare_mod_game_sources"](expected, expected, "p4_mod_game64w")
            with self.assertRaises(graph["Failure"]):
                graph["compare_mod_game_sources"](expected[:-1], expected, "p4_mod_game64w")
            with self.assertRaises(graph["Failure"]):
                graph["compare_mod_game_sources"](expected + ["unexpected.c"], expected, "p4_mod_game64w")

    def test_private_named_checks_and_missing_enabled_script(self):
        self.fragments(FRAGMENTS)
        for name in ("zig_abi", "gs_audit", "land_model", "quarantine", "capture_archive",
                     "land_client_wait", "remote_check_host", "mod_guard_timing_contract"):
            with self.subTest(name=name):
                self.assert_executed(name)
        (self.root / "work/capture_archive.completed").unlink()
        (self.root / "tests/capture_archive_test.py").unlink()
        self.assertIn("capture_archive_test.py", self.assert_not_executed("capture_archive"))

    def test_public_named_checks_and_missing_public_script(self):
        self.public_export()
        self.assert_executed("zig_abi")
        for name in ("gs_audit", "land_model", "quarantine", "capture_archive",
                     "land_client_wait", "remote_check_host", "mod_guard_timing_contract"):
            with self.subTest(name=name):
                self.assert_not_executed(name)
        (self.root / "work/zig_abi.completed").unlink()
        (self.root / "tests/zig_abi_test.py").unlink()
        self.assertIn("zig_abi_test.py", self.assert_not_executed("zig_abi"))

    def test_private_fragments_are_independent_in_public_tree(self):
        self.public_export()
        self.fragments(("251-captures.cmake",))
        self.assert_executed("capture_archive")
        self.assert_not_executed("land_client_wait")

    def test_unmarked_private_tree_requires_fragments(self):
        self.fragments(FRAGMENTS)
        for fragment in ("250-tools-private.cmake", "251-captures.cmake"):
            with self.subTest(fragment=fragment):
                path = self.root / "tests/cmake" / fragment
                path.unlink()
                output = self.assert_not_executed("zig_abi")
                self.assertIn("missing private test fragment", output)
                self.assertIn(fragment, output)
                path.write_text("# synthetic fragment\n", encoding="utf-8")

    def test_invalid_or_conflicting_export_marker_fails(self):
        marker = self.root / "zigbuild/public-export-mode.txt"
        marker.write_text("invalid\n", encoding="utf-8")
        self.assertIn("invalid public export mode marker", self.assert_not_executed("zig_abi"))
        self.public_export()
        (self.root / "tools/public_export.py").write_text("# private exporter\n", encoding="utf-8")
        self.assertIn("public export marker conflicts", self.assert_not_executed("zig_abi"))

    def test_exporter_marker_drives_graph_mode(self):
        # Public lanes use the marker produced by their own real export. Private
        # checkouts exercise that same exporter on an isolated fixture repository.
        output = self.assert_not_executed("zig_abi")
        self.assertIn("missing private test fragment", output)
        self.assertIn("250-tools-private.cmake", output)
        marker = ROOT / "zigbuild/public-export-mode.txt"
        if not marker.is_file():
            fixture = self.root / "export-input"
            fixture.mkdir()
            files = {
                "README.md": "# fixture\n",
                "tools/package_win.sh": "#!/bin/sh\n",
                "tools/package_linux.sh": "#!/bin/sh\n",
                "tools/public_export/public_paths.txt": "README.md\ntools/package_*_local.sh\n",
                "tools/public_export/private_paths.txt": "tools/public_export/**\n",
            }
            for name, text in files.items():
                destination = fixture / name
                destination.parent.mkdir(parents=True, exist_ok=True)
                destination.write_text(text, encoding="utf-8")
            overlay = fixture / "tools/public_export/overlay/zigbuild/public-export-mode.txt"
            overlay.parent.mkdir(parents=True)
            shutil.copyfile(ROOT / "tools/public_export/overlay/zigbuild/public-export-mode.txt", overlay)
            for args in (["init", "-q"], ["add", "-A"], ["commit", "-q", "-m", "fixture"]):
                subprocess.run(["git", "-c", "user.name=fixture", "-c", "user.email=fixture@example.invalid",
                                "-c", "commit.gpgsign=false", "-c", "core.autocrlf=false", *args],
                               cwd=fixture, check=True, capture_output=True)
            stage = self.root / "export-output"
            exported = subprocess.run([sys.executable, str(ROOT / "tools/public_export.py"), "export", "HEAD",
                                       "--repo", str(fixture), "--no-patches", "--stage", str(stage)],
                                      capture_output=True, text=True, timeout=120)
            self.assertEqual(exported.returncode, 0, exported.stderr)
            marker = stage / "tree/zigbuild/public-export-mode.txt"
        self.assertTrue(marker.is_file(), "the real export must supply its mode marker")
        (self.root / "zigbuild/public-export-mode.txt").write_bytes(marker.read_bytes())
        self.assert_executed("zig_abi")
        self.assert_not_executed("gs_audit")


if __name__ == "__main__":
    unittest.main()

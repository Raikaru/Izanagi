"""Zig-native exports must contribute their declared calling convention to mirror rewrites."""
import json
import os
import platform
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]


@unittest.skipUnless(shutil.which("clang"), "requires clang's AST reader")
class ZigAbiTest(unittest.TestCase):
    def abi_targets(self):
        native = subprocess.run([shutil.which("clang"), "-dumpmachine"], capture_output=True,
                                text=True, check=True).stdout.strip()
        yield native
        cross = "x86_64-w64-windows-gnu" if sys.platform == "win32" else "x86_64-linux-gnu"
        if cross == native:
            return
        headers = subprocess.run([shutil.which("clang"), "--target=" + cross, "-E", "-x", "c", "-"],
                                 input="#include <stdint.h>\n", capture_output=True, text=True)
        if headers.returncode and "file not found" in headers.stderr:
            reason = "x86_64 cross headers unavailable for " + cross + ": " + headers.stderr.strip()
            print("SKIP: " + reason, file=sys.stderr)
            with self.subTest(target=cross):
                self.skipTest(reason)
            return
        self.assertEqual(headers.returncode, 0, headers.stderr)
        yield cross

    def run_abi(self, directory, header, *sources, target):
        out = directory / "abi.json"
        cmd = [sys.executable, str(ROOT / "tools/port64/unit64.py"), "abi",
               "--gen", str(directory), "--root", str(ROOT), "--zig-header", str(header),
               "--out", str(out), "--clang", shutil.which("clang")]
        for source in sources:
            cmd += ["--zig-source", str(source)]
        env = os.environ.copy()
        # Each case uses an explicit native or cross target with checked system headers.
        env["P4_PORT64_TARGET"] = "--target=" + target
        result = subprocess.run(cmd, capture_output=True, text=True, env=env)
        return result, out

    def test_exported_declarations_only_and_pointer_registers(self):
        for target in self.abi_targets():
            with self.subTest(target=target), tempfile.TemporaryDirectory() as work:
                directory = Path(work)
                header = directory / "exports.h"
                header.write_text("void *battle_test(unsigned int id, const void *payload);\nint unrelated(void);\n")
                source = directory / "exports.zig"
                source.write_text("export fn battle_test(id: u32, payload: ?*const anyopaque) ?*anyopaque { return payload; }\n")
                result, out = self.run_abi(directory, header, source, target=target)
                self.assertEqual(result.returncode, 0, result.stderr)
                table = json.loads(out.read_text())
                self.assertEqual(set(table), {"battle_test"})
                self.assertEqual(table["battle_test"]["ret"], "ptr")
                self.assertEqual([param["cls"] for param in table["battle_test"]["params"]], ["int", "ptr"])
                self.assertTrue(out.with_suffix(".json.d").exists())

    def test_missing_or_duplicate_export_is_rejected(self):
        for target in self.abi_targets():
            with self.subTest(target=target), tempfile.TemporaryDirectory() as work:
                directory = Path(work)
                header = directory / "exports.h"
                header.write_text("int declared(void);\n")
                source = directory / "exports.zig"
                source.write_text("export fn missing() c_int { return 0; }\n")
                result, _ = self.run_abi(directory, header, source, target=target)
                self.assertNotEqual(result.returncode, 0)
                self.assertIn("missing ABI declarations: missing", result.stderr)
                source.write_text("export fn declared() c_int { return 0; }\nexport fn declared() c_int { return 1; }\n")
                result, _ = self.run_abi(directory, header, source, target=target)
                self.assertNotEqual(result.returncode, 0)
                self.assertIn("duplicate Zig function exports", result.stderr)

    @unittest.skipUnless(os.environ.get("P4_ZIG") and os.environ.get("ANDROID_NDK_HOME"),
                         "requires pinned Zig and Android NDK")
    def test_android_translate_c_accepts_bionic_stdio(self):
        with tempfile.TemporaryDirectory() as work:
            directory = Path(work)
            (directory / "bindings").mkdir()
            (directory / "bindings" / "sample.h").write_text("#include <stdio.h>\n")
            source = directory / "sample.zig"
            source.write_text('const c = @import("c_bindings");\n'
                              'export fn sample(f: ?*c.FILE) ?*c.FILE { return f; }\n')
            ndk = Path(os.environ["ANDROID_NDK_HOME"])
            sysroot = ndk / "toolchains/llvm/prebuilt/linux-x86_64/sysroot"
            if not sysroot.is_dir():
                self.skipTest("Android NDK Linux sysroot unavailable")
            obj = directory / "sample.o"
            cmd = [sys.executable, str(ROOT / "cmake/zig/zig_object.py"),
                   "--zig", os.environ["P4_ZIG"], "--default-target", "aarch64-none-linux-android24",
                   "--optimize", "safe", "--cache-dir", str(directory / "cache"),
                   "--global-cache-dir", str(directory / "global-cache"), "--android-sysroot", str(sysroot),
                   "-o", str(obj), "-c", str(source), "--", "-D_FORTIFY_SOURCE=2"]
            result = subprocess.run(cmd, capture_output=True, text=True)
            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertTrue(obj.is_file())

    @unittest.skipUnless(os.environ.get("P4_ZIG"), "requires pinned Zig")
    def test_translated_bindings_reproduce_safe_objects_across_cold_output_directories(self):
        with tempfile.TemporaryDirectory() as work:
            directory = Path(work)
            (directory / "bindings").mkdir()
            (directory / "bindings/sample.h").write_text(
                "struct Sample { int value; };\n"
                "static inline int sample_value(const struct Sample *s) { return s->value + 7; }\n")
            source = directory / "sample.zig"
            source.write_text('const c = @import("c_bindings");\n'
                              'export fn sample(value: c_int) c_int {\n'
                              '    var s: c.struct_Sample = .{ .value = value };\n'
                              '    return c.sample_value(&s);\n'
                              '}\n')
            objects = []
            for output in ("first", "second"):
                for cache in ("cache", "global-cache"):
                    shutil.rmtree(directory / cache, ignore_errors=True)
                obj = directory / output / "sample.zig.o"
                obj.parent.mkdir()
                result = subprocess.run([
                    sys.executable, str(ROOT / "cmake/zig/zig_object.py"),
                    "--zig", os.environ["P4_ZIG"], "--default-target", "x86_64-unknown-linux-gnu",
                    "--optimize", "safe", "--cache-dir", str(directory / "cache"),
                    "--global-cache-dir", str(directory / "global-cache"),
                    "-o", str(obj), "-c", str(source), "--"],
                    capture_output=True, text=True)
                self.assertEqual(result.returncode, 0, result.stderr)
                objects.append(obj.read_bytes())
            self.assertGreater(len(objects[0]), 0)
            self.assertEqual(objects[0], objects[1])

    @unittest.skipUnless(os.environ.get("P4_ZIG") and sys.platform.startswith("linux") and
                         platform.machine() in ("x86_64", "AMD64"),
                         "requires pinned Zig on x86-64 Linux")
    def test_executable_dso_handle_uses_zero_storage(self):
        with tempfile.TemporaryDirectory() as work:
            directory = Path(work)
            source = directory / "probe.c"
            source.write_text(
                "extern void *__dso_handle;\n"
                "extern int __cxa_atexit(void (*)(void *), void *, void *);\n"
                "extern void __cxa_finalize(void *);\n"
                "static int count;\n"
                "static void mark(void *p) { (void)p; ++count; }\n"
                "int main(void) {\n"
                "  int tag;\n"
                "  if (__cxa_atexit(mark, 0, &tag)) return 2;\n"
                "  __cxa_finalize(__dso_handle);\n"
                "  return count == 1 ? 0 : 1;\n"
                "}\n", encoding="utf-8")
            for fixed in (False, True):
                exe = directory / ("fixed" if fixed else "original")
                command = [os.environ["P4_ZIG"], "cc", "-target", "x86_64-linux-gnu.2.41",
                           "-no-pie", str(source)]
                if fixed:
                    command.append(str(ROOT / "zigbuild/dso_handle.c"))
                result = subprocess.run([*command, "-o", str(exe)], capture_output=True, text=True)
                self.assertEqual(result.returncode, 0, result.stderr)
                result = subprocess.run([str(exe)], capture_output=True, text=True)
                self.assertEqual(result.returncode, 0 if fixed else 1, result.stderr)

    @unittest.skipUnless(os.environ.get("P4_ZIG"), "requires pinned Zig")
    def test_live_posix_signal_sentinel_compiles_for_arm64_and_x64(self):
        with tempfile.TemporaryDirectory() as work:
            directory = Path(work)
            for target in ("aarch64-unknown-linux-gnu", "x86_64-unknown-linux-gnu"):
                with self.subTest(target=target):
                    obj = directory / (target + ".o")
                    result = subprocess.run([
                        sys.executable, str(ROOT / "cmake/zig/zig_object.py"),
                        "--zig", os.environ["P4_ZIG"], "--default-target", target,
                        "--optimize", "safe", "--cache-dir", str(directory / "cache"),
                        "--global-cache-dir", str(directory / "global-cache"),
                        "-o", str(obj), "-c", str(ROOT / "src/platform/battle_sim/live.zig"),
                        "--", "-D_FILE_OFFSET_BITS=64", "-D_TIME_BITS=64"],
                        capture_output=True, text=True)
                    self.assertEqual(result.returncode, 0, result.stderr)
                    self.assertGreater(obj.stat().st_size, 0)

    def test_explicit_and_conditional_exports_validate_functions_and_signatures(self):
        for target in self.abi_targets():
            with self.subTest(target=target), tempfile.TemporaryDirectory() as work:
                directory = Path(work)
                header = directory / "exports.h"
                header.write_text("int direct(unsigned int id);\nvoid *conditional(void *value);\n")
                source = directory / "exports.zig"
                source.write_text(
                    'fn real(id: u32) callconv(.c) c_int { return 0; }\n'
                    'fn other(value: ?*anyopaque) callconv(.c) ?*anyopaque { return value; }\n'
                    'comptime { @export(&real, .{ .name = "direct" }); '
                    'if (true) @export(&other, .{ .name = "conditional" }); }\n')
                result, out = self.run_abi(directory, header, source, target=target)
                self.assertEqual(result.returncode, 0, result.stderr)
                self.assertEqual(set(json.loads(out.read_text())), {"direct", "conditional"})
                source.write_text('comptime { @export(&missing, .{ .name = "direct" }); }\n')
                result, _ = self.run_abi(directory, header, source, target=target)
                self.assertNotEqual(result.returncode, 0)
                self.assertIn("must reference one C-ABI function", result.stderr)
                source.write_text(
                    'fn real(id: ?*anyopaque) callconv(.c) c_int { return 0; }\n'
                    'comptime { @export(&real, .{ .name = "direct" }); }\n')
                result, _ = self.run_abi(directory, header, source, target=target)
                self.assertNotEqual(result.returncode, 0)
                self.assertIn("Zig export ABI signature mismatch: direct", result.stderr)
                source.write_text(
                    'fn real(id: u64) callconv(.c) c_int { return 0; }\n'
                    'comptime { @export(&real, .{ .name = "direct" }); }\n')
                result, _ = self.run_abi(directory, header, source, target=target)
                self.assertNotEqual(result.returncode, 0)
                self.assertIn("Zig export ABI signature mismatch: direct", result.stderr)

    @unittest.skipUnless(os.environ.get("P4_ZIG") and shutil.which("llvm-objdump"),
                         "requires pinned Zig and llvm-objdump")
    def test_explicit_zig_section_flags_are_forwarded(self):
        with tempfile.TemporaryDirectory() as work:
            directory = Path(work)
            source = directory / "sections.zig"
            source.write_text("export var one: u32 = 1;\nexport var two: u32 = 2;\n"
                              "export fn foo() u32 { return one; }\n"
                              "export fn bar() u32 { return two; }\n", encoding="utf-8")
            for flags, separated in ((["-ffunction-sections", "-fdata-sections"], True),
                                     (["-ffunction-sections", "-fdata-sections",
                                       "-fno-function-sections", "-fno-data-sections"], False)):
                obj = directory / ("split.o" if separated else "joined.o")
                result = subprocess.run([
                    sys.executable, str(ROOT / "cmake/zig/zig_object.py"),
                    "--zig", os.environ["P4_ZIG"], "--default-target", "x86_64-linux-gnu",
                    "--optimize", "safe", "--cache-dir", str(directory / "cache"),
                    "--global-cache-dir", str(directory / "global-cache"),
                    "-o", str(obj), "-c", str(source), "--", *flags],
                    capture_output=True, text=True)
                self.assertEqual(result.returncode, 0, result.stderr)
                sections = subprocess.run([shutil.which("llvm-objdump"), "-h", str(obj)],
                                          check=True, capture_output=True, text=True).stdout
                for name in (".text.sections.foo", ".text.sections.bar",
                             ".data.sections.one", ".data.sections.two"):
                    self.assertEqual(name in sections, separated, sections)

    @unittest.skipUnless(os.environ.get("P4_ZIG") and os.environ.get("P4_ANDROID_SYSROOT"),
                         "requires pinned Zig and Android NDK sysroot")
    def test_android_fortified_header_translation(self):
        with tempfile.TemporaryDirectory() as work:
            directory = Path(work)
            (directory / "bindings").mkdir()
            (directory / "bindings" / "fortify.h").write_text("#include <stdio.h>\n", encoding="utf-8")
            source = directory / "fortify.zig"
            source.write_text('const c = @import("c_bindings");\n'
                              'export fn call_puts(s: [*:0]const u8) c_int { return c.puts(s); }\n',
                              encoding="utf-8")
            result = subprocess.run([
                sys.executable, str(ROOT / "cmake/zig/zig_object.py"),
                "--zig", os.environ["P4_ZIG"], "--default-target", "aarch64-none-linux-android24",
                "--android-sysroot", os.environ["P4_ANDROID_SYSROOT"], "--optimize", "fast",
                "--cache-dir", str(directory / "cache"), "--global-cache-dir", str(directory / "global-cache"),
                "-o", str(directory / "fortify.o"), "-c", str(source), "--", "-D_FORTIFY_SOURCE=2"],
                capture_output=True, text=True)
            self.assertEqual(result.returncode, 0, result.stderr)


if __name__ == "__main__":
    unittest.main()

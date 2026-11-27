"""Exercise the pinned Zig compiler's preprocessed AST, ABI and mirror rewrite path.

P4PC_ZIG_CC_WRAPPER must name the build's actual zig cc wrapper, not a mock.
"""
import hashlib
import json
import os
import platform
from pathlib import Path
import runpy
import subprocess
import sys
import tempfile
import unittest
from unittest import mock

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
from port64 import gptr_rewrite as gr


class ZigLinkPolicyTest(unittest.TestCase):
    def setUp(self):
        self.translate = runpy.run_path(str(ROOT / "tools/zig_link_map.py"))["coff_gc_policy"]

    def test_only_declared_gc_adds_ref(self):
        raw = ["lld-link", "-DEBUG", "-OUT:main.exe", "main.obj"]
        unchanged, policy = self.translate(["compiler", "-Wl,/Brepro", "main.obj"], raw)
        self.assertEqual(unchanged, raw)
        self.assertEqual(policy["caller_requests"], [])
        self.assertEqual(policy["argv_added"], [])
        for flags in (["-Wl,--gc-sections"], ["-Xlinker", "--gc-sections"]):
            corrected, policy = self.translate(["compiler", *flags, "main.obj"], raw)
            self.assertEqual(corrected, raw + ["/OPT:REF"])
            self.assertEqual(policy["translation_table"], {"--gc-sections": "/OPT:REF"})
            self.assertEqual(policy["argv_added"], ["/OPT:REF"])
            self.assertEqual(policy["caller_requests"][0]["caller_token"], "--gc-sections")
        self.assertEqual(raw, ["lld-link", "-DEBUG", "-OUT:main.exe", "main.obj"])

    def test_unknown_linker_policies_fail_closed(self):
        for token in ("--no-gc-sections", "--gc-unknown", "--icf=all", "/OPT:ICF", "-opt:ref"):
            with self.subTest(token=token), self.assertRaisesRegex(SystemExit, "unsupported COFF linker policy"):
                self.translate(["compiler", "-Wl," + token], ["lld-link"])
        with self.assertRaisesRegex(SystemExit, "-Xlinker needs a value"):
            self.translate(["compiler", "-Xlinker"], ["lld-link"])


class ZigCcAstTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        compiler = os.environ.get("P4PC_ZIG_CC_WRAPPER")
        if not compiler or not Path(compiler).is_file():
            raise RuntimeError("P4PC_ZIG_CC_WRAPPER must name the real Zig cc wrapper")
        cls.compiler = str(Path(compiler).resolve())

    @unittest.skipUnless(os.environ.get("P4_ZIG"), "requires pinned Zig")
    def test_nopic_argument_policy_for_every_target_os(self):
        result = subprocess.run([os.environ["P4_ZIG"], "test", str(ROOT / "zigbuild/zig_cc.zig")],
                                capture_output=True, text=True)
        self.assertEqual(result.returncode, 0, result.stderr)

    def test_windows_omits_redundant_nopic_while_elf_retains_it(self):
        with tempfile.TemporaryDirectory(prefix="p4-zig-pic-") as directory:
            root = Path(directory)
            source = root / "probe.c"
            source.write_text("extern int value; int read_value(void) { return value; }\n", encoding="ascii")
            for arch in ("i686", "x86_64"):
                for os_name, relocation in (("w64-mingw32", "pic"), ("linux-gnu", "static")):
                    with self.subTest(arch=arch, os=os_name):
                        result = subprocess.run(
                            [self.compiler, "--target=" + arch + "-" + os_name, "-fno-pic",
                             "-###", "-c", str(source), "-o", str(root / "probe.o")],
                            capture_output=True, text=True)
                        self.assertEqual(result.returncode, 0, result.stderr)
                        expected = "static" if arch == "i686" else relocation
                        self.assertIn('"-mrelocation-model" "' + expected + '"', result.stderr)
                        if os_name == "linux-gnu":
                            # Establish that losing -fno-pic would change this actual driver's job.
                            control = subprocess.run(
                                [self.compiler, "--target=" + arch + "-" + os_name, "-###",
                                 "-c", str(source), "-o", str(root / "probe.o")],
                                capture_output=True, text=True)
                            self.assertEqual(control.returncode, 0, control.stderr)
                            self.assertIn('"-mrelocation-model" "pic"', control.stderr)

    def test_clang_stack_and_assertion_defaults_preserve_caller_policy(self):
        with tempfile.TemporaryDirectory(prefix="p4-zig-defaults-") as directory:
            root = Path(directory)
            source = root / "probe.c"
            source.write_text("extern int consume(char *); int probe(void) { char bytes[128]; "
                              "bytes[0] = 1; return consume(bytes); }\n", encoding="utf-8")
            for target in ("x86_64-linux-gnu", "x86_64-w64-mingw32"):
                for opt in ("-O0", "-O2"):
                    with self.subTest(target=target, opt=opt):
                        base = [self.compiler, "--target=" + target, opt]
                        for policy, guarded in (([], False), (["-fstack-protector-all"], True),
                                                (["-fstack-protector-all", "-fno-stack-protector"], False)):
                            job = subprocess.run([*base, *policy, "-###", "-c", str(source),
                                                  "-o", str(root / "probe.o")], capture_output=True, text=True)
                            self.assertEqual(job.returncode, 0, job.stderr)
                            self.assertEqual('"-stack-protector"' in job.stderr, guarded, job.stderr)
                        for policy, value in (([], None), (["-DNDEBUG=7"], "7"),
                                              (["-DNDEBUG=7", "-UNDEBUG"], None),
                                              (["-D", "NDEBUG=9"], "9")):
                            macros = subprocess.run([*base, *policy, "-dM", "-E", str(source)],
                                                    capture_output=True, text=True)
                            self.assertEqual(macros.returncode, 0, macros.stderr)
                            definitions = dict(line[len("#define "):].split(" ", 1)
                                               for line in macros.stdout.splitlines() if line.startswith("#define "))
                            self.assertEqual(definitions.get("NDEBUG"), value)
                        obj = root / "probe.o"
                        subprocess.run([*base, "-c", str(source), "-o", str(obj)], check=True, capture_output=True)
                        nm = os.environ.get("P4_NM", "llvm-nm")
                        refs = subprocess.run([nm, "--undefined-only", str(obj)], check=True,
                                              capture_output=True, text=True).stdout
                        self.assertIn("consume", refs)
                        self.assertNotIn("__stack_chk", refs)

    @unittest.skipUnless(sys.platform.startswith("linux") and
                         Path("/opt/llvm-mingw/bin/clang").is_file(), "requires the Linux CI MinGW headers")
    def test_windows_libc_census_matches_standalone_headers(self):
        with tempfile.TemporaryDirectory(prefix="p4-zig-windows-headers-") as directory:
            root = Path(directory)
            source = root / "headers.c"
            source.write_text("#include <stdio.h>\n", encoding="utf-8")
            flags = ["--target=x86_64-w64-mingw32", "-std=gnu89", "-fno-builtin", "-fms-extensions",
                     "-DP4_PC=1", "-DP4_DECOMP_UNIT=1", "-include", "p4pc/compat.h", "-I" + str(ROOT / "include")]
            counts = []
            for label, compiler in (("cmake", "/opt/llvm-mingw/bin/clang"), ("zig", self.compiler)):
                with mock.patch.dict(os.environ, {"P4PC_BUILD_CACHE_DISABLE": "1",
                                                 "P4PC_ZIG_CC": "1" if label == "zig" else "0"}):
                    rw, _, _ = gr.process(str(source), flags, str(root / label), str(root), clang=compiler)
                counts.append(dict(rw.census))
                self.assertFalse(rw.unsupported)
            self.assertGreater(counts[0].get("skipped:macro-or-foreign", 0), 0)
            self.assertEqual(counts[0], counts[1])
            # The selected default header directory must not displace caller -I paths.
            (root / "stdio.h").write_text("#define P4_CALLER_HEADER 17\n", encoding="utf-8")
            macros = subprocess.run([self.compiler, "--target=x86_64-w64-mingw32", "-I", str(root),
                                     "-dM", "-E", str(source)], capture_output=True, text=True)
            self.assertEqual(macros.returncode, 0, macros.stderr)
            self.assertIn("#define P4_CALLER_HEADER 17", macros.stdout)

    def test_windows_cpp_keeps_libcxx_header_wrappers_before_crt(self):
        with tempfile.TemporaryDirectory(prefix="p4-zig-cpp-headers-") as directory:
            root = Path(directory)
            source, obj = root / "headers.cpp", root / "headers.obj"
            source.write_text("#include <cstdio>\n#include <cwchar>\n#include <cwctype>\n"
                              "#include <cstddef>\n#include <cerrno>\n#include <cmath>\n#include <string>\n"
                              "static_assert(sizeof(void *) == 8);\n"
                              "extern \"C\" double headers(double value) {\n"
                              "  const std::string text(\"zig\");\n"
                              "  return std::sin(value) + text.size();\n}\n", encoding="utf-8")
            compiler = Path(self.compiler)
            cpp = compiler.with_name(compiler.stem + "++" + compiler.suffix)
            result = subprocess.run([str(cpp), "--target=x86_64-w64-mingw32", "-std=gnu++17",
                                     "-fno-exceptions", "-fno-rtti", "-O2", "-c", str(source), "-o", str(obj)],
                                    capture_output=True, text=True)
            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertGreater(obj.stat().st_size, 0)

    def test_preprocessed_tokens_locations_and_calling_convention(self):
        with tempfile.TemporaryDirectory(prefix="p4-zig-ast-") as directory:
            root = Path(directory)
            source = Path("virtual-source.c").resolve()
            unit = root / "expanded.i"
            text = ('# 73 "virtual-source.c"\n'
                    'int __LINE__ = 7;\n'
                    '__attribute__((ms_abi)) int tagged(int value);\n')
            unit.write_bytes(text.encode("ascii"))
            flags = ["--target=x86_64-linux-gnu"]
            ast = gr.load_ast(str(unit), flags, self.compiler)
            nodes = {n.get("name"): n for n in ast.get("inner", ())
                     if n.get("kind") in ("VarDecl", "FunctionDecl")}
            variable = nodes["__LINE__"]
            _, start, end = gr.name_span(variable)
            self.assertEqual(text[start:end], "__LINE__")
            self.assertEqual(gr.Origin(text).at(start)[:2], (str(source.resolve()), 73))
            self.assertIn("ms_abi", nodes["tagged"]["type"]["qualType"])

    @unittest.skipUnless(sys.platform.startswith("linux") and platform.machine().lower() in ("x86_64", "amd64"),
                         "Linux x86-64 native execution witness")
    def test_preprocessed_object_executes(self):
        with tempfile.TemporaryDirectory(prefix="p4-zig-exec-") as directory:
            root = Path(directory)
            unit = root / "expanded.i"
            unit.write_text('int __LINE__ = 7; int read_value(void) { return __LINE__; }\n', encoding="ascii")
            flags = ["--target=x86_64-linux-gnu"]
            driver = root / "driver.c"
            driver.write_text("int read_value(void); int main(void) { return read_value() != 7; }\n", encoding="ascii")
            for name, prefix in (
                ("direct", []),
                ("ee-float", [sys.executable, str(ROOT / "tools/ee_float_cc.py"), "--modes=div,sqrt,add"]),
            ):
                with self.subTest(path=name):
                    obj = root / (name + ".o")
                    exe = root / name
                    env = os.environ.copy()
                    env["CCACHE_DISABLE"] = "1"
                    subprocess.run([*prefix, self.compiler, *flags, "-x", "cpp-output", "-c", str(unit),
                                    "-o", str(obj)], check=True, capture_output=True, env=env)
                    subprocess.run([self.compiler, *flags, str(driver), str(obj), "-o", str(exe)],
                                   check=True, capture_output=True, env=env)
                    subprocess.run([str(exe)], check=True, capture_output=True)

    @unittest.skipUnless(sys.platform.startswith("linux") and os.environ.get("P4_ZIG"),
                         "requires the Linux Zig toolchain")
    def test_link_map_replays_actual_lld_output_without_changing_binary(self):
        with tempfile.TemporaryDirectory(prefix="p4-zig-link-map-") as directory:
            root = Path(directory)
            source = root / "main.c"
            source.write_text("int main(void) { return 0; }\n", encoding="utf-8")
            obj = root / "main.o"
            subprocess.run([self.compiler, "--target=x86_64-linux-gnu", "-c", str(source), "-o", str(obj)],
                           check=True, capture_output=True)
            binary, mapfile = root / "main", root / "main.map"
            subprocess.run([sys.executable, str(ROOT / "tools/zig_link_map.py"), os.environ["P4_ZIG"],
                            str(mapfile), "--", self.compiler, "--target=x86_64-linux-gnu",
                            str(obj), "-o", str(binary)], check=True, capture_output=True)
            self.assertIn(str(obj), mapfile.read_text())
            provenance = json.loads(Path(str(mapfile) + ".linker.json").read_text())
            self.assertEqual(provenance["command"], "ld.lld")
            self.assertEqual(provenance["output_sha256"], hashlib.sha256(binary.read_bytes()).hexdigest())
            self.assertEqual(provenance["sha256"], hashlib.sha256(Path(os.environ["P4_ZIG"]).read_bytes()).hexdigest())
            self.assertTrue(provenance["discovery_byte_equal"])
            self.assertTrue(provenance["final_links_byte_equal"])
            self.assertEqual(provenance["final_argv"], provenance["discovery_argv"] + provenance["argv_added"])
            self.assertEqual(provenance["product_sha256"]["map"], hashlib.sha256(mapfile.read_bytes()).hexdigest())

    @unittest.skipUnless(sys.platform.startswith("linux") and os.environ.get("P4_ZIG") and
                         Path("/opt/llvm-mingw/bin/clang").is_file(), "requires the Linux CI compilers")
    def test_link_map_symbol_origins_match_cmake_linker(self):
        with tempfile.TemporaryDirectory(prefix="p4-zig-link-origins-") as directory:
            root = Path(directory)
            source, obj = root / "entry.c", root / "entry.o"
            source.write_text("void _start(void) {}\n", encoding="utf-8")
            subprocess.run([self.compiler, "--target=x86_64-linux-gnu", "-c", str(source), "-o", str(obj)],
                           check=True, capture_output=True)
            cmake_map, zig_map = root / "cmake.map", root / "zig.map"
            subprocess.run(["/opt/llvm-mingw/bin/clang", "--target=x86_64-linux-gnu", "-fuse-ld=lld",
                            "-nostdlib", "-no-pie", str(obj), "-Wl,-Map," + str(cmake_map),
                            "-o", str(root / "cmake")], check=True, capture_output=True)
            subprocess.run([sys.executable, str(ROOT / "tools/zig_link_map.py"), os.environ["P4_ZIG"],
                            str(zig_map), "--", self.compiler, "--target=x86_64-linux-gnu", "-nostdlib",
                            "-no-pie", str(obj), "-o", str(root / "zig")], check=True, capture_output=True)
            def origins(path):
                found, owner = {}, None
                for line in path.read_text().splitlines():
                    fields = line.split()
                    if len(fields) < 5:
                        continue
                    value = fields[-1]
                    if ":(" in value:
                        owner = value.split(":(", 1)[0]
                    elif value == "_start":
                        found[value] = owner
                return found
            self.assertEqual(origins(cmake_map), {"_start": str(obj)})
            self.assertEqual(origins(cmake_map), origins(zig_map))

    @unittest.skipUnless(sys.platform.startswith("linux") and os.environ.get("P4_ZIG"),
                         "requires the Linux Zig toolchain")
    def test_coff_link_map_replays_actual_lld_output(self):
        with tempfile.TemporaryDirectory(prefix="p4-zig-coff-map-") as directory:
            root = Path(directory)
            source = root / "main.c"
            source.write_text("int main(void) { return 0; }\n", encoding="utf-8")
            obj, binary, mapfile = root / "main.obj", root / "main.exe", root / "main.map"
            target = "--target=x86_64-w64-mingw32"
            subprocess.run([self.compiler, target, "-c", str(source), "-o", str(obj)], check=True, capture_output=True)
            result = subprocess.run([sys.executable, str(ROOT / "tools/zig_link_map.py"), os.environ["P4_ZIG"],
                                     str(mapfile), "--", self.compiler, target, str(obj), "-Wl,/Brepro", "-o", str(binary)],
                                    capture_output=True, text=True)
            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertTrue(mapfile.stat().st_size)
            provenance = json.loads(Path(str(mapfile) + ".linker.json").read_text())
            self.assertEqual(provenance["command"], "lld-link")
            self.assertEqual(provenance["output_sha256"], hashlib.sha256(binary.read_bytes()).hexdigest())
            self.assertTrue(provenance["final_links_byte_equal"])
            self.assertEqual(provenance["final_argv"], provenance["discovery_argv"] +
                             ["/MAP:" + str(mapfile), "/lldmap:" + str(mapfile) + ".origins"])
            self.assertEqual(provenance["product_sha256"]["map"], hashlib.sha256(mapfile.read_bytes()).hexdigest())
            self.assertEqual(provenance["product_sha256"]["pdb"],
                             hashlib.sha256(binary.with_suffix(".pdb").read_bytes()).hexdigest())
            self.assertTrue(provenance["discovery_codeview"])
            self.assertTrue(provenance["final_codeview"])
            self.assertEqual(provenance["product_sha256"]["origins_map"],
                             hashlib.sha256(Path(str(mapfile) + ".origins").read_bytes()).hexdigest())
            self.assertIn("single_map_exe_pdb_byte_equal", provenance)

    @unittest.skipUnless(sys.platform.startswith("linux") and os.environ.get("P4_ZIG"),
                         "requires the pinned Linux Zig and MinGW toolchains")
    def test_coff_declared_gc_discards_unused_comdat(self):
        with tempfile.TemporaryDirectory(prefix="p4-zig-coff-gc-") as directory:
            root = Path(directory)
            source, obj = root / "main.c", root / "main.obj"
            source.write_text("__attribute__((used,noinline)) int p4_gc_unused(void) { return 7; }\n"
                              "int mainCRTStartup(void) { return 0; }\n", encoding="utf-8")
            target = "--target=x86_64-w64-mingw32"
            subprocess.run([self.compiler, target, "-O2", "-g1", "-ffunction-sections",
                            "-c", str(source), "-o", str(obj)], check=True, capture_output=True)
            for name, flags in (("gc", ["-Wl,--gc-sections"]), ("default", [])):
                binary, mapfile = root / (name + ".exe"), root / (name + ".map")
                argv = [sys.executable, str(ROOT / "tools/zig_link_map.py"), os.environ["P4_ZIG"],
                        str(mapfile), "--", self.compiler, target, "-nostdlib", "-g1",
                        str(obj), "-Wl,-e,mainCRTStartup", *flags, "-o", str(binary)]
                subprocess.run(argv, check=True, capture_output=True)
                mapped = Path(str(mapfile) + ".origins").read_text()
                self.assertIn("mainCRTStartup", mapped)
                if flags:
                    self.assertNotIn("p4_gc_unused", mapped)
                provenance = json.loads(Path(str(mapfile) + ".linker.json").read_text())
                added = ["/OPT:REF"] if flags else []
                self.assertEqual(provenance["declared_coff_gc_policy"]["argv_added"], added)
                self.assertEqual(provenance["discovery_argv"], provenance["raw_compiler_discovery_argv"] + added)
                self.assertEqual(provenance["final_argv"], provenance["discovery_argv"] +
                                 ["/MAP:" + str(mapfile), "/lldmap:" + str(mapfile) + ".origins"])
                self.assertTrue(provenance["final_links_byte_equal"])

    @unittest.skipUnless(sys.platform.startswith("linux") and os.environ.get("P4_ZIG"),
                         "requires the Linux Zig toolchain")
    def test_coff_final_link_rejects_any_product_byte_change(self):
        helper = runpy.run_path(str(ROOT / "tools/zig_link_map.py"))
        real_run = subprocess.run
        for product in ("main.exe", "main.pdb", "main.map", "main.map.origins"):
            with self.subTest(product=product), tempfile.TemporaryDirectory(prefix="p4-zig-link-reject-") as directory:
                root = Path(directory)
                source = root / "main.c"
                source.write_text("int main(void) { return 0; }\n", encoding="utf-8")
                calls = 0

                def altered_run(argv, *args, **kwargs):
                    nonlocal calls
                    result = real_run(argv, *args, **kwargs)
                    if len(argv) > 1 and argv[1] == "lld-link" and any(a.startswith("/lldmap:") for a in argv):
                        calls += 1
                        if calls == 2:
                            path = root / product
                            data = bytearray(path.read_bytes())
                            data[-1] ^= 1
                            path.write_bytes(data)
                    return result

                argv = ["zig_link_map.py", os.environ["P4_ZIG"], str(root / "main.map"), "--",
                        self.compiler, "--target=x86_64-w64-mingw32", str(source), "-o", str(root / "main.exe")]
                with mock.patch.object(sys, "argv", argv), mock.patch.object(subprocess, "run", altered_run):
                    with self.assertRaisesRegex(SystemExit, "final links are not byte-identical"):
                        helper["main"]()

    @unittest.skipUnless(sys.platform.startswith("linux") and platform.machine().lower() in ("x86_64", "amd64"),
                         "Linux x86-64 native execution witness")
    def test_ee_float_c_frontend_emits_real_object_with_depfile(self):
        with tempfile.TemporaryDirectory(prefix="p4-zig-ee-c-") as directory:
            root = Path(directory)
            source = root / "math.c"
            source.write_text("float plus_one(float x) { return x + 1.0f; }\n", encoding="ascii")
            driver = root / "driver.c"
            driver.write_text("float plus_one(float); int main(void) { return plus_one(2.0f) != 3.0f; }\n", encoding="ascii")
            obj, exe, dep = root / "math.o", root / "driver", root / "math.d"
            env = os.environ.copy()
            env["CCACHE_DISABLE"] = "1"
            subprocess.run([sys.executable, str(ROOT / "tools/ee_float_cc.py"), self.compiler,
                            "--target=x86_64-linux-gnu", "-O2", "-MD", "-MF", str(dep),
                            "-c", str(source), "-o", str(obj)], check=True, capture_output=True, env=env)
            subprocess.run([self.compiler, "--target=x86_64-linux-gnu", str(driver), str(obj),
                            str(ROOT / "src/platform/ee_float.c"), "-I" + str(ROOT / "include"), "-o", str(exe)],
                           check=True, capture_output=True, env=env)
            subprocess.run([str(exe)], check=True, capture_output=True)
            self.assertTrue(dep.is_file())

    def test_guest_layout_ast_acceptance_matches_cmake_driver(self):
        with tempfile.TemporaryDirectory(prefix="p4-zig-layout-") as directory:
            unit = Path(directory) / "guest.i"
            unit.write_text("typedef char GuestSize[sizeof(void *) == 4 ? 1 : -1];\nint native(void) { return 1; }\n", encoding="utf-8")
            flags = ["--target=x86_64-linux-gnu"]
            for compiler, zig in (("clang", "0"), (self.compiler, "1")):
                with self.subTest(compiler=compiler), mock.patch.dict(os.environ, {"P4PC_ZIG_CC": zig, "P4PC_BUILD_CACHE_DISABLE": "1"}):
                    ast = gr.load_ast(str(unit), flags, compiler)
                    self.assertTrue(any(node.get("name") == "native" for node in ast.get("inner", ())))

    @unittest.skipUnless(sys.platform.startswith("linux") and platform.machine().lower() in ("x86_64", "amd64"),
                         "Linux x86-64 ABI system headers")
    def test_native_abi_system_headers_match_cmake(self):
        with tempfile.TemporaryDirectory(prefix="p4-zig-abi-headers-") as directory:
            unit = Path(directory) / "native.c"
            unit.write_text("#include <SDL3/SDL.h>\n#include <libavcodec/avcodec.h>\n", encoding="utf-8")
            common = ["--target=x86_64-linux-gnu", "-I/usr/include/x86_64-linux-gnu"]
            for compiler, extra in (("clang", []), (self.compiler, ["-idirafter", "/usr/include"])):
                with self.subTest(compiler=compiler):
                    subprocess.run([compiler, *common, *extra, "-E", str(unit), "-o", os.devnull],
                                   check=True, capture_output=True)
            self.assertEqual(gr.stage2_flags(["--target=x86_64-linux-gnu", "-idirafter", "/usr/include"]),
                             ["--target=x86_64-linux-gnu"])

    def test_guest_layout_oracle_matches_cmake_driver(self):
        with tempfile.TemporaryDirectory(prefix="p4-zig-oracle-") as directory:
            unit = Path(directory) / "guest.c"
            unit.write_text("struct DebugOnly { int n; }; struct Guest { int n; struct DebugOnly *p; }; "
                            "struct Guest global;\n"
                            "int invalid_guest_conversion(void *p) { return p; }\n", encoding="utf-8")
            with mock.patch.dict(os.environ, {"P4PC_BUILD_CACHE_DISABLE": "1"}):
                c = gr.oracle_layouts(str(unit), ["--target=i386-linux-gnu", "-m32", "-std=gnu11"], "clang")
                zig = gr.oracle_layouts(str(unit), ["--target=x86-linux-gnu", "-m32", "-std=gnu11"], self.compiler)
            self.assertIn("struct Guest", c)
            self.assertEqual(c["struct Guest"], zig["struct Guest"])
            self.assertEqual(c, zig)

    def test_target_specific_abi_rewrites_and_cache(self):
        fixture = """\
typedef unsigned int u32;
extern u32 p4_h2g(void *);
extern void *p4_g2h(u32);
extern void use_address(void **);
#ifdef _WIN32
void *profile(void *p, double n) { return p; }
#else
int profile(int n, void *p) { return n; }
#endif
int guarded(void *task) {
    _Static_assert(__builtin_classify_type(task) == 5, "pointer parameter");
    use_address(&task);
    return 0;
}
"""
        with tempfile.TemporaryDirectory(prefix="p4-zig-rewrite-") as directory:
            root = Path(directory)
            source = root / "game.c"
            source.write_text(fixture, encoding="utf-8")
            cache = root / "cache"
            identity = hashlib.sha256(Path(self.compiler).read_bytes()).hexdigest()
            env = {"P4PC_BUILD_CACHE_DIR": str(cache), "P4PC_BUILD_CACHE_DISABLE": "0",
                   "P4PC_BUILD_TOOLCHAIN_ID": "sha256:" + identity, "P4PC_ZIG_CC": "1"}
            with mock.patch.dict(os.environ, env):
                for target, result, params in (
                    ("x86_64-linux-gnu", "int", ["int", "ptr"]),
                    ("x86_64-windows-gnu", "ptr", ["ptr", "float"]),
                ):
                    with self.subTest(target=target):
                        flags = ["--target=" + target, "-std=gnu11"]
                        work = root / target
                        work.mkdir()
                        abi = gr.collect_abi(str(source), flags, str(work), str(root), self.compiler)
                        self.assertEqual(abi["profile"]["ret"], result)
                        self.assertEqual([p["cls"] for p in abi["profile"]["params"]], params)
                        self.assertEqual(abi, gr.collect_abi(str(source), flags, str(work), str(root), self.compiler))
                        rw, _, rewritten = gr.process(str(source), flags, str(work), str(root), clang=self.compiler)
                        self.assertEqual(rw.census["addr-taken-param"], 1)
                        guarded_line = fixture.splitlines().index("int guarded(void *task) {") + 1
                        self.assertIn(("game.c", guarded_line, 1), rw.census.sites["addr-taken-param"])
                        self.assertIn("__builtin_classify_type(task_p4arg)", rewritten)
                        self.assertFalse(rw.unsupported)
                        output = work / "rewritten.i"
                        output.write_bytes(rewritten.encode("latin-1"))
                        subprocess.run([self.compiler, *flags, "-x", "cpp-output", "-c", str(output),
                                        "-o", str(work / "rewritten.o")], check=True, capture_output=True)

    def test_zig_export_declarations_use_the_real_frontend(self):
        with tempfile.TemporaryDirectory(prefix="p4-zig-exports-") as directory:
            root = Path(directory)
            header = root / "exports.h"
            header.write_text("void *battle_test(unsigned int id, const void *payload);\nint unrelated(void);\n")
            source = root / "exports.zig"
            source.write_text("export fn battle_test(id: u32, payload: ?*const anyopaque) ?*anyopaque { return payload; }\n")
            output = root / "abi.json"
            env = os.environ.copy()
            env.update({"P4PC_ZIG_CC": "1", "P4PC_BUILD_CACHE_DISABLE": "1",
                        "P4_PORT64_TARGET": "--target=x86_64-linux-gnu",
                        "P4_PORT32_TARGET": "--target=x86-linux-gnu"})
            subprocess.run([sys.executable, str(ROOT / "tools/port64/unit64.py"), "abi",
                            "--gen", str(root), "--root", str(ROOT), "--zig-header", str(header),
                            "--zig-source", str(source), "--out", str(output),
                            "--clang", self.compiler], env=env, check=True, capture_output=True)
            table = json.loads(output.read_text())
            self.assertEqual(set(table), {"battle_test"})
            self.assertEqual(table["battle_test"]["ret"], "ptr")
            self.assertEqual([p["cls"] for p in table["battle_test"]["params"]], ["int", "ptr"])
            self.assertTrue((root / "abi.json.d").is_file())

    def test_zig_macos_target_rewrite_semantics(self):
        flags = ["--target=aarch64-macos"]
        self.assertEqual(gr.symbol_prefix(flags), "_")
        self.assertTrue(gr.variadic_on_stack(flags))
        self.assertEqual(gr.symbol_prefix(["--target=x86_64-macos.13.0"]), "_")
        self.assertFalse(gr.variadic_on_stack(["--target=aarch64-linux-gnu"]))


if __name__ == "__main__":
    unittest.main()

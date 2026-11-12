#!/usr/bin/env python3
"""Exercise the boundary with actual ELF/COFF relocations, not mocked nm output."""
import argparse
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
import crt_guard
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools/port64"))
import gptr_rewrite


class GuardTest(unittest.TestCase):
    clang = "clang"
    nm = "llvm-nm"
    objdump = "llvm-objdump"

    def test_real_object_profiles(self):
        profiles = (("i386-linux-gnu", False), ("x86_64-linux-gnu", False),
                    ("i686-w64-mingw32", True), ("x86_64-w64-mingw32", False),
                    ("aarch64-linux-gnu", False), ("aarch64-linux-android", False),
                    ("x86_64-linux-android", False), ("arm64-apple-macos12.0", False), ("x86_64-apple-macos12.0", False))
        source = """
extern void *memcpy(void *, const void *, __SIZE_TYPE__);
extern int strcmp(const char *, const char *);
extern int puts(const char *);
extern long time(void *);
extern int surprise_host_entry(const char *);
extern int fstat(int, void *);
extern int kill(int, int);
extern int func_00421f98(int, void *);
extern int func_00421fb8(int, int);
extern int func_004426e8(const char *, const char *);
extern unsigned long long fptodp(float);
void *allowed(void *a, const void *b) { return memcpy(a, b, 3); }
int deliberate_host_call(const char *s) { return puts(s); }
int renamed_bypass(const char *s) { return strcmp(s, "x"); }
long metadata_is_not_permission(void) { return time((void *)0); }
int unknown_host_call(const char *s) { return surprise_host_entry(s); }
int posix_status_bypass(void *buffer) { return fstat(0, buffer); }
int posix_signal_bypass(void) { return kill(2, 9); }
int retail_device_status(void *buffer) { return func_00421f98(0, buffer); }
int retail_process_command(void) { return func_00421fb8(2, 0); }
int retail_compare(const char *s) { return func_004426e8(s, "x"); }
unsigned long long retail_runtime_alias(void) { return fptodp(1.0f); }
"""
        renames = {"strcmp": "func_004426e8", "fstat": "func_00421f98", "kill": "func_00421fb8",
                   "memcpy": "func_0043f810"}
        with tempfile.TemporaryDirectory(prefix="p4crt-guard-") as directory:
            root = Path(directory).resolve()   # (the guard reports resolved paths; macOS's temporary directory is under the /var symlink)
            src = root / "build/gen/p4pc_tables64.c"
            src.parent.mkdir(parents=True)
            src.write_text(source, encoding="utf-8")
            for triple, coff32 in profiles:
                # Mach-O (macOS, --macho): C names carry an underscore, and a section-start label can share a function's address
                crt_guard.MACHO = "-apple-" in triple
                self.addCleanup(setattr, crt_guard, "MACHO", False)
                with self.subTest(profile=triple):
                    obj = root / "build/CMakeFiles/p4pc64.dir/gen" / (triple + ".o")
                    obj.parent.mkdir(parents=True, exist_ok=True)
                    subprocess.run([self.clang, "--target=" + triple, "-c", "-O0", "-fno-builtin", str(src), "-o", str(obj)], check=True, capture_output=True)
                    commands = {str(obj.resolve()): {"directory": str(root), "file": str(src)}}
                    selected = crt_guard.compile_target_objects(commands, ["p4pc64"], root)
                    self.assertEqual(selected, [obj])
                    rows, failures = crt_guard.audit(
                        selected, [], self.nm, self.objdump, coff32, set(),
                        {"strcmp", "time", "func_004426e8", "_GLOBAL_OFFSET_TABLE_"},
                        renames,
                        {"memcpy": "non-overlapping byte-copy fixture"},
                        retail_names={"time": 0x00444010, "fptodp": 0x0044dcd8})
                    self.assertEqual(len(failures), 6, failures)
                    for name in ("puts", "strcmp", "time", "surprise_host_entry", "fstat", "kill"):
                        self.assertTrue(any(" -> " + name + ":" in failure for failure in failures), failures)
                    self.assertTrue(any(row["symbol"] == "memcpy" and row["caller"] == "allowed" and row["disposition"] == "reviewed host" for row in rows), rows)
                    self.assertTrue(any(row["binding"] == "func_004426e8" and row["caller"] == "retail_compare" and row["disposition"] == "retail binding" for row in rows), rows)
                    self.assertTrue(any(row["binding"] == "fptodp" and row["caller"] == "retail_runtime_alias"
                                        and row["disposition"] == "retail binding" for row in rows), rows)
                    self.assertTrue(any(row["binding"] == "func_00421f98" and row["disposition"] == "retail binding" for row in rows), rows)
                    self.assertTrue(any(row["binding"] == "func_00421fb8" and row["disposition"] == "retail binding" for row in rows), rows)
                    print(f"CRT rejection witness {triple}: puts, strcmp, metadata time, unknown external and host fstat/kill rejected; canonical EE SDK bindings selected; explicitly reviewed byte emission retained despite its EE-width source alias", flush=True)
                    stub_source, stub_obj = root / "trap.c", root / (triple + "-trap.o")
                    stub_source.write_text(
                        'extern int p4pc_trap_args(void);\n'
                        'int func_004426e8(const char *a, const char *b) { return p4pc_trap_args(); }\n',
                        encoding="utf-8")
                    subprocess.run([self.clang, "--target=" + triple, "-c", "-O0", str(stub_source), "-o", str(stub_obj)],
                                   check=True, capture_output=True)
                    _, trap_failures = crt_guard.audit(
                        selected, [stub_obj], self.nm, self.objdump, coff32, set(),
                        {"strcmp", "time", "func_004426e8", "_GLOBAL_OFFSET_TABLE_"},
                        renames, {"memcpy": "non-overlapping byte-copy fixture"},
                        retail_names={"time": 0x00444010, "fptodp": 0x0044dcd8})
                    self.assertTrue(any("retail_compare -> func_004426e8" in failure and "non-trap" in failure
                                        for failure in trap_failures), trap_failures)
                    self.assertTrue(any("retail_runtime_alias -> fptodp" in failure for failure in trap_failures), trap_failures)
                    core_source, core_obj = root / "core.c", root / (triple + "-core.o")
                    core_source.write_text(
                        'unsigned long long func_0044dcd8(float x) { return 0x3ff0000000000000ULL; }\n',
                        encoding="utf-8")
                    subprocess.run([self.clang, "--target=" + triple, "-c", "-O0", str(core_source), "-o", str(core_obj)],
                                   check=True, capture_output=True)
                    alias_rows, alias_failures = crt_guard.audit(
                        selected, [core_obj], self.nm, self.objdump, coff32, set(),
                        {"strcmp", "time", "func_004426e8", "_GLOBAL_OFFSET_TABLE_"},
                        renames, {"memcpy": "non-overlapping byte-copy fixture"},
                        retail_names={"time": 0x00444010, "fptodp": 0x0044dcd8})
                    self.assertFalse(any("retail_runtime_alias -> fptodp" in failure for failure in alias_failures), alias_failures)
                    self.assertTrue(any(row["binding"] == "fptodp" and row["disposition"] == "retail binding"
                                        for row in alias_rows), alias_rows)
                    table_obj = obj.parent / "p4pc_tables64.c.o"
                    shutil.copy2(core_obj, table_obj)
                    _, table_failures = crt_guard.audit(
                        selected, [table_obj], self.nm, self.objdump, coff32, set(),
                        {"strcmp", "time", "func_004426e8", "_GLOBAL_OFFSET_TABLE_"},
                        renames, {"memcpy": "non-overlapping byte-copy fixture"},
                        retail_names={"time": 0x00444010, "fptodp": 0x0044dcd8})
                    self.assertTrue(any("retail_runtime_alias -> fptodp" in failure for failure in table_failures), table_failures)

    def test_native_definitions_do_not_require_mirror_abi_sidecars(self):
        # A cold i386-only build has real native objects but no 64-bit ABI
        # products. A stale mirror manifest must not change that ownership.
        with tempfile.TemporaryDirectory(prefix="p4crt-native-defs-") as directory:
            root = Path(directory)
            decomp, build = root / "decomp", root / "gen"
            (decomp / "config").mkdir(parents=True)
            build.mkdir()
            for name in ("symbol_addrs.txt", "symbol_data_addrs.txt", "symbols_recovered.txt"):
                (decomp / "config" / name).write_text("", encoding="utf-8")
            (build / "units.txt").write_text("", encoding="utf-8")
            missing = build / "obj64/abi/port/src/crt.c.abi.json"
            (build / "crt-native-abi-files.txt").write_text(str(missing) + "\n", encoding="utf-8")
            game, native = root / "game.c", root / "native.c"
            game.write_text("extern int fixture_native(void);\nint fixture_game(void) { return fixture_native(); }\n", encoding="utf-8")
            native.write_text("int fixture_native(void) { return 7; }\n", encoding="utf-8")
            for triple in ("i386-linux-gnu", "x86_64-linux-gnu", "i686-w64-mingw32",
                           "x86_64-w64-mingw32", "aarch64-linux-gnu",
                           "aarch64-linux-android", "x86_64-linux-android"):
                with self.subTest(profile=triple):
                    objects = []
                    for source in (game, native):
                        obj = source.with_suffix(".o")
                        subprocess.run([self.clang, "--target=" + triple, "-c", "-O0",
                                        str(source), "-o", str(obj)], check=True, capture_output=True)
                        objects.append(obj)
                    game_list, native_list = root / "objects.txt", root / "definitions.txt"
                    game_list.write_text(str(objects[0]) + "\n", encoding="utf-8")
                    native_list.write_text(str(objects[1]) + "\n", encoding="utf-8")
                    command = [sys.executable, str(Path(crt_guard.__file__)), "--objects-file", str(game_list),
                               "--decomp", str(decomp), "--build", str(build), "--nm", self.nm,
                               "--objdump", self.objdump, "--report", str(build / "audit.json")]
                    if triple == "i686-w64-mingw32":
                        command.append("--coff32")
                    actual = subprocess.run(command + ["--definitions-file", str(native_list)],
                                            text=True, capture_output=True)
                    self.assertEqual(actual.returncode, 0, actual.stdout + actual.stderr)
                    mirror = subprocess.run(command, text=True, capture_output=True)
                    self.assertNotEqual(mirror.returncode, 0)
                    self.assertIn("crt.c.abi.json", mirror.stderr)
                    print(f"CRT native-definition witness {triple}: real objects own native names; compile-only mirror still requires its ABI sidecars", flush=True)

    def test_adapter_pairs_and_guest_writes(self):
        profiles = (("i386-linux-gnu", False), ("x86_64-linux-gnu", False),
                    ("i686-w64-mingw32", True), ("x86_64-w64-mingw32", False),
                    ("aarch64-linux-gnu", False), ("aarch64-linux-android", False),
                    ("x86_64-linux-android", False))
        with tempfile.TemporaryDirectory(prefix="p4crt-flow-") as directory:
            root = Path(directory)
            (root / "src").mkdir()
            source, header = root / "src/adapter.c", root / "src/adapter.h"
            original = ('extern void *malloc(__SIZE_TYPE__);\n#include "adapter.h"\n'
                        'void host_adapter(void) { keep_native(malloc(16)); }\n')
            boundary = (
                '#if __SIZEOF_POINTER__ == 4\n'
                '#define P4_H2G(p) ((unsigned)(__UINTPTR_TYPE__)(p))\n'
                '#define P4_G2H(a) ((void *)(__UINTPTR_TYPE__)(unsigned)(a))\n'
                '#else\nextern unsigned p4_h2g(const void *);\nextern void *p4_g2h(unsigned);\n'
                '#define P4_H2G(p) p4_h2g(p)\n#define P4_G2H(a) p4_g2h(a)\n#endif\n'
                '#define P4_G32(a) (*(unsigned *)P4_G2H(a))\n'
                'static void *native_cache;\n')
            safe = 'static void keep_native(void *p) { native_cache = p; }\n'
            obj = root / "CMakeFiles/p4port.dir/src/adapter.c.o"
            obj.parent.mkdir(parents=True)
            policy = {"src/adapter.c": {
                "calls": {"malloc": ["host_adapter"]},
                "reason": "address retained in native_cache only",
            }}
            for triple, coff32 in profiles:
                with self.subTest(profile=triple):
                    arguments = [self.clang, "--target=" + triple, "-c", "-O0",
                                 "-fno-builtin", str(source), "-o", str(obj)]
                    entry = {"directory": str(root), "file": str(source), "arguments": arguments}
                    commands = {os.path.normcase(str(obj.resolve())): entry}
                    def compile_and_audit():
                        subprocess.run(arguments, check=True, capture_output=True)
                        return crt_guard.audit([obj], [], self.nm, self.objdump, coff32,
                                               set(), {"_GLOBAL_OFFSET_TABLE_"}, {}, {}, policy,
                                               commands=commands)
                    source.write_text(original, encoding="utf-8")
                    header.write_text(boundary + safe, encoding="utf-8")
                    rows, failures = compile_and_audit()
                    self.assertFalse(failures, failures)
                    self.assertTrue(any(row["disposition"] == "reviewed host adapter" for row in rows), rows)
                    # No hashes: unrelated header/source edits retain the pair approval.
                    header.write_text(boundary + safe + "static const unsigned unrelated = 7;\n", encoding="utf-8")
                    source.write_text(original + "/* unrelated source edit */\n", encoding="utf-8")
                    self.assertFalse(compile_and_audit()[1])
                    # Same host symbol and caller, but a guest-memory write now escapes.
                    header.write_text(boundary + 'static void keep_native(void *p) { P4_G32(0x100000) = P4_H2G(p); }\n', encoding="utf-8")
                    _, failures = compile_and_audit()
                    self.assertTrue(any("result reaches guest-memory boundary" in row for row in failures), failures)
                    # A source-local function-name macro must not inherit
                    # ownership from the preceding included helper header.
                    source.write_text(original.replace(
                        "void host_adapter", "#define host_adapter macro_adapter\nvoid host_adapter"),
                        encoding="utf-8")
                    policy["src/adapter.c"]["calls"]["malloc"] = ["macro_adapter"]
                    header.write_text(boundary + safe, encoding="utf-8")
                    self.assertFalse(compile_and_audit()[1])
                    header.write_text(boundary + 'static void keep_native(void *p) { P4_G32(0x100000) = P4_H2G(p); }\n', encoding="utf-8")
                    self.assertTrue(any("result reaches guest-memory boundary" in row
                                        for row in compile_and_audit()[1]))
                    source.write_text(original, encoding="utf-8")
                    policy["src/adapter.c"]["calls"]["malloc"] = ["host_adapter"]
                    # A previously cached result escaping elsewhere is caught too.
                    header.write_text(boundary + safe, encoding="utf-8")
                    source.write_text(original + 'void escaped(void) { P4_G32(0x100000) = P4_H2G(native_cache); }\n', encoding="utf-8")
                    _, failures = compile_and_audit()
                    self.assertTrue(any("result reaches guest-memory boundary" in row for row in failures), failures)
                    # An ordinary new calling function needs explicit pair review.
                    source.write_text(original + 'void *new_caller(void) { return malloc(8); }\n', encoding="utf-8")
                    _, failures = compile_and_audit()
                    self.assertTrue(any("new_caller -> malloc" in row for row in failures), failures)
                    print(f"CRT flow witness {triple}: harmless edits pass; direct/inline cached guest escapes and new caller rejected", flush=True)


    def test_external_inputs_are_not_guest_parser_exemptions(self):
        prefix = """
extern long strtol(const char *, char **, int);
extern int sscanf(const char *, const char *, ...);
extern char *getenv(const char *);
extern void *malloc(__SIZE_TYPE__);
extern void *memcpy(void *, const void *, __SIZE_TYPE__);
extern void *memset(void *, int, __SIZE_TYPE__);
extern int snprintf(char *, __SIZE_TYPE__, const char *, ...);
extern void *__memcpy_chk(void *, const void *, __SIZE_TYPE__, __SIZE_TYPE__);
extern int __sprintf_chk(char *, int, __SIZE_TYPE__, const char *, ...);
extern char *strstr(const char *, const char *);
#if __SIZEOF_POINTER__ == 4
#define P4_H2G(p) ((unsigned)(__UINTPTR_TYPE__)(p))
#define P4_G2H(a) ((void *)(__UINTPTR_TYPE__)(unsigned)(a))
#else
extern unsigned p4_h2g(const void *);
extern void *p4_g2h(unsigned);
#define P4_H2G(p) p4_h2g(p)
#define P4_G2H(a) p4_g2h(a)
#endif
#define P4_G32(a) (*(unsigned *)P4_G2H(a))
"""
        body = """
static void frontend(const char *spec) {
    unsigned value = 0;
    sscanf(spec, "%u", &value);
    P4_G32(0x100000) = value;
    const char *text = getenv("P4PC_TEST");
    if (text) P4_G32(0x100004) = (unsigned)strtol(text, (char **)0, 10);
}
int main(int argc, char **argv) {
    if (argc > 1) frontend(argv[1]);
    return 0;
}
"""
        with tempfile.TemporaryDirectory(prefix="p4crt-input-") as directory:
            root = Path(directory)
            source = root / "frontend.c"
            for triple in ("i386-linux-gnu", "x86_64-linux-gnu", "i686-w64-mingw32",
                           "x86_64-w64-mingw32", "aarch64-linux-gnu",
                           "aarch64-linux-android", "x86_64-linux-android"):
                with self.subTest(profile=triple):
                    entry = {"directory": str(root), "file": str(source),
                             "arguments": [self.clang, "--target=" + triple, "-fno-builtin", str(source)]}
                    def analyze(text):
                        source.write_text(prefix + text, encoding="utf-8")
                        return crt_guard.crt_flow.check(entry, {"strtol", "sscanf", "getenv", "malloc", "snprintf"})
                    errors = analyze(body)
                    self.assertTrue(any("sscanf result reaches" in error for error in errors), errors)
                    self.assertTrue(any("strtol result reaches" in error for error in errors), errors)
                    guest_destination = body.replace('sscanf(spec, "%u", &value);',
                                                     'sscanf(spec, "%u", (unsigned *)P4_G2H(0x100000));')
                    self.assertTrue(any("writes directly into a guest destination" in error for error in analyze(guest_destination)))
                    addressed_guest = body.replace('sscanf(spec, "%u", &value);',
                                                   'sscanf(spec, "%u", &P4_G32(0x100000));')
                    self.assertTrue(any("writes directly into a guest destination" in error for error in analyze(addressed_guest)))
                    allocation = body.replace('P4_G32(0x100000) = value;',
                                              'P4_G32(0x100000) = P4_H2G(malloc(16));')
                    self.assertTrue(any("malloc result reaches" in error for error in analyze(allocation)))
                    end_pointer = body.replace('P4_G32(0x100000) = value;',
                                               'char *end; strtol(spec, &end, 10); P4_G32(0x100000) = P4_H2G(end);')
                    self.assertTrue(any("strtol:end" in error for error in analyze(end_pointer)))
                    parsed_array = """
static void frontend(const char *spec) {
    unsigned values[2] = {0};
    sscanf(spec, "%u", &values[0]);
    memcpy(P4_G2H(0x100000), values, sizeof values);
}
"""
                    self.assertTrue(any("sscanf result reaches guest-memory boundary memcpy" in error
                                        for error in analyze(parsed_array)))
                    native_scratch = """
void frontend(void) {
    char *scratch = malloc(4);
    memset(scratch, 42, 4);
    memcpy(P4_G2H(0x100000), scratch, 4);
}
"""
                    self.assertFalse(analyze(native_scratch))
                    self.assertFalse(analyze(native_scratch.replace('scratch, 4);', '&scratch[0], 4);')))
                    # Bionic/GNU fortified spellings must preserve the same
                    # pointer-vs-byte distinction, including builtin lowering.
                    for copy in ("__memcpy_chk", "__builtin___memcpy_chk"):
                        fortified_bytes = native_scratch.replace(
                            "memcpy(P4_G2H(0x100000), scratch, 4);",
                            copy + "(P4_G2H(0x100000), scratch, 4, 4);")
                        self.assertFalse(analyze(fortified_bytes))
                        fortified_pointer = (
                            "void frontend(void) { void *pointer = malloc(8); " +
                            copy + "(P4_G2H(0x100000), &pointer, sizeof pointer, sizeof pointer); }")
                        self.assertTrue(any("malloc result reaches guest-memory boundary memcpy" in error
                                            for error in analyze(fortified_pointer)))
                    fortified_format = """
void frontend(const char *spec) {
    char text[32];
    __sprintf_chk(text, 0, sizeof text, "%s", spec);
    __memcpy_chk(P4_G2H(0x100000), text, sizeof text, sizeof text);
}
"""
                    self.assertTrue(any("sprintf result reaches guest-memory boundary memcpy" in error
                                        for error in analyze(fortified_format)))
                    indexed_pointer = native_scratch.replace(
                        'memcpy(P4_G2H(0x100000), scratch, 4);',
                        'P4_G32(0x100000) = P4_H2G(&(scratch[0]));')
                    self.assertTrue(any("malloc result reaches" in error for error in analyze(indexed_pointer)))
                    for declaration, count in (("char text[32];", "sizeof text"), ("char *text = malloc(32);", "32")):
                        formatted = ('static void frontend(const char *spec) { ' + declaration +
                                     ' snprintf(text, ' + count + ', "%s", spec); ' +
                                     '__builtin_memcpy(P4_G2H(0x100000), text, ' + count + '); }')
                        self.assertTrue(any("snprintf result reaches guest-memory boundary memcpy" in error
                                            for error in analyze(formatted)))
                    formatted_member = """
struct Native { char *text; };
static void frontend(const char *spec) {
    struct Native buffer = { malloc(32) };
    snprintf(buffer.text, 32, "%s", spec);
    memcpy(P4_G2H(0x100000), buffer.text, 32);
}
"""
                    self.assertTrue(any("snprintf result reaches guest-memory boundary memcpy" in error
                                        for error in analyze(formatted_member)))
                    substring_pointer = native_scratch.replace(
                        'memcpy(P4_G2H(0x100000), scratch, 4);',
                        'P4_G32(0x100000) = P4_H2G(strstr(scratch, "*"));')
                    self.assertTrue(any("malloc result reaches" in error for error in analyze(substring_pointer)))
                    print(f"CRT input witness {triple}: argv/env parser results, guest destinations, end pointers and allocations rejected", flush=True)

    def test_real_project_archive_satisfies_bridge_references(self):
        with tempfile.TemporaryDirectory(prefix="p4-crt-bridge-") as directory:
            root = Path(directory)
            game, bridge = root / "game.c", root / "bridge.c"
            game.write_text("extern int entry; extern int guard(void); int game(void) { return entry + guard(); }\n",
                            encoding="utf-8")
            bridge.write_text("int entry; int guard(void) { return 1; }\n", encoding="utf-8")
            ar = str(Path(self.clang).with_name("llvm-ar" + Path(self.clang).suffix))
            for triple in ("x86_64-linux-gnu", "x86_64-w64-mingw32", "aarch64-linux-gnu"):
                with self.subTest(profile=triple):
                    objs = [root / (triple + suffix) for suffix in ("-game.o", "-bridge.o")]
                    for src, obj in zip((game, bridge), objs):
                        subprocess.run([self.clang, "--target=" + triple, "-c", str(src), "-o", str(obj)],
                                       check=True, capture_output=True)
                    archive = root / (triple + ".a")
                    subprocess.run([ar, "rcs", str(archive), str(objs[1])], check=True, capture_output=True)
                    project, _ = crt_guard.object_symbols([archive], self.nm)
                    host = crt_guard.host_exports([archive], self.nm, False)
                    _, failures = crt_guard.audit([objs[0]], [], self.nm, self.objdump, False,
                                                  host, set(), {}, {})
                    self.assertTrue(failures)
                    host.difference_update(project - crt_guard.C_FUNCTIONS)
                    _, failures = crt_guard.audit([objs[0]], [], self.nm, self.objdump, False,
                                                  host, project, {}, {})
                    self.assertFalse(failures, failures)

    def test_address_taken_parameter_preserves_guard_abi_assertions(self):
        source = """
typedef unsigned int u32;
extern u32 p4_h2g(void *);
extern void *p4_g2h(u32);
extern void use_address(void **);
int guarded(void *task) {
    _Static_assert(__builtin_classify_type(task) == 5, "pointer parameter");
    _Static_assert(sizeof(guarded(task)) == 4, "result type");
    use_address(&task);
    return 0;
}
"""
        with tempfile.TemporaryDirectory(prefix="p4-guard-param-") as directory:
            root = Path(directory)
            src = root / "game.c"
            src.write_text(source, encoding="utf-8")
            for triple in ("x86_64-linux-gnu", "x86_64-w64-mingw32", "aarch64-linux-gnu",
                           "aarch64-linux-android"):
                with self.subTest(profile=triple):
                    work = root / triple
                    rw, _, rewritten = gptr_rewrite.process(
                        str(src), ["--target=" + triple], str(work), str(root), clang=self.clang)
                    self.assertEqual(rw.census["addr-taken-param"], 1)
                    self.assertIn("__builtin_classify_type(task_p4arg)", rewritten)
                    out = work / "game.i"
                    out.write_text(rewritten, encoding="latin-1")
                    subprocess.run([self.clang, "--target=" + triple, "-c", str(out), "-o", str(work / "game.o")],
                                   check=True, capture_output=True)

    def test_renamed_printf_keeps_integer_masks(self):
        source = """
typedef unsigned int u32;
extern void *p4_g2h(u32);
extern int func_00442088(u32, u32, ...);
extern int func_00440b68(u32, ...);
int formatted(u32 dst, u32 fmt, u32 string, unsigned long long bits) {
    return func_00442088(dst, fmt, string, bits, 2.25) +
           func_00440b68(fmt, string, bits, 2.25);
}
"""
        ptr = lambda text: {"ct": text, "cls": "ptr"}
        abi = {
            "func_00442088": {"rt": "int", "ret": "int", "va": "other",
                              "params": [ptr("char *"), ptr("const char *")]},
            "func_00440b68": {"rt": "int", "ret": "int", "va": "other",
                              "params": [ptr("const char *")]},
        }
        with tempfile.TemporaryDirectory(prefix="p4crt-printf-") as directory:
            root = Path(directory)
            src = root / "game.c"
            src.write_text(source, encoding="utf-8")
            for triple in ("x86_64-linux-gnu", "x86_64-w64-mingw32", "aarch64-linux-gnu",
                           "aarch64-linux-android", "x86_64-linux-android"):
                with self.subTest(profile=triple):
                    work = root / triple
                    rw, _, rewritten = gptr_rewrite.process(
                        str(src), ["--target=" + triple], str(work), str(root),
                        clang=self.clang, abi=abi)
                    self.assertEqual(rw.census["printf-masked"], 2)
                    self.assertIn('__asm__("p4pf_sprintf")', rewritten)
                    self.assertIn('__asm__("p4pf_printf")', rewritten)
                    self.assertEqual(rewritten.count("0x3u,"), 2)
                    out = work / "game.i"
                    out.write_text(rewritten, encoding="latin-1")
                    obj = work / "game.o"
                    subprocess.run([self.clang, "--target=" + triple, "-c", str(out), "-o", str(obj)],
                                   check=True, capture_output=True)
                    _, references = crt_guard.object_symbols([obj], self.nm)
                    self.assertIn("p4pf_sprintf", references[str(obj)])
                    self.assertIn("p4pf_printf", references[str(obj)])
                    self.assertNotIn("func_00442088", references[str(obj)])
                    print(f"CRT printf mask witness {triple}: raw aliases bind p4pf and preserve GP mask 3", flush=True)

    def test_adapter_known_callback_prototypes_use_the_existing_compiler_ast(self):
        prefix = """
extern int puts(const char *);
extern void *SDL_GL_GetProcAddress(const char *);
typedef unsigned (*Callback)(void *, unsigned);
typedef float (*Floating)(float);
typedef unsigned (__attribute__((stdcall)) *StdCallback)(void *, unsigned);
unsigned good(void *object, unsigned mask) { return object != 0 || mask; }
void bad(unsigned long long object, unsigned mask) {}
unsigned wrong_class(unsigned value) { return value; }
void no_args(void) {}
static Callback saved;
static Callback factory(Callback value) { return value; }
static void install(Callback value) { saved = value; }
static void erased(void *value) { saved = (Callback)value; }
GLOBAL
void adapter(void) { puts("native diagnostics"); BODY }
"""
        variants = (
            ("", "Callback value = factory(good); install(value); (void)saved(0, 1);", None),
            ("", "((Callback)bad)(0, 1);", "return width/class"),
            ("", "saved = (Callback)bad;", "return width/class"),
            ("", "erased((void *)bad);", "argument via erased"),
            ("", "((Floating)wrong_class)(1.0f);", "parameter 0 width/class"),
            ("", "((Callback)no_args)(0, 1);", "parameter count"),
            ("Callback initial = (Callback)bad;", "", "return width/class"),
            ("", "Callback driver = (Callback)SDL_GL_GetProcAddress(\"fixture\"); driver(0, 1);", None),
        )
        with tempfile.TemporaryDirectory(prefix="p4crt-callback-") as directory:
            root = Path(directory)
            source = root / "src/adapter.c"
            obj = root / "CMakeFiles/p4port.dir/src/adapter.c.o"
            source.parent.mkdir()
            obj.parent.mkdir(parents=True)
            policy = {"src/adapter.c": {"calls": {"puts": ["adapter"]},
                                       "reason": "Native-only diagnostics; callback ABI checked in the same AST."}}
            for triple, coff32 in (("i386-linux-gnu", False), ("x86_64-linux-gnu", False),
                                   ("i686-w64-mingw32", True), ("x86_64-w64-mingw32", False),
                                   ("aarch64-linux-gnu", False), ("aarch64-linux-android", False),
                                   ("x86_64-linux-android", False)):
                convention = (("", "StdCallback value = (StdCallback)good; value(0, 1);",
                               "calling convention" if triple.startswith(("i386", "i686")) else None),)
                for global_, body, rejected in variants + convention:
                    with self.subTest(profile=triple, body=body, global_=global_):
                        source.write_text(prefix.replace("GLOBAL", global_).replace("BODY", body), encoding="utf-8")
                        arguments = [self.clang, "--target=" + triple, "-c", "-O0", "-Wno-everything",
                                     "-fno-builtin", str(source), "-o", str(obj)]
                        subprocess.run(arguments, check=True, capture_output=True)
                        entry = {"directory": str(root), "file": str(source), "arguments": arguments}
                        _, failures = crt_guard.audit(
                            [obj], [], self.nm, self.objdump, coff32, set(),
                            {"_GLOBAL_OFFSET_TABLE_", "SDL_GL_GetProcAddress"}, {}, {}, policy,
                            commands={os.path.normcase(str(obj.resolve())): entry})
                        if rejected is None:
                            self.assertFalse(failures, failures)
                        else:
                            self.assertTrue(any("indirect callback" in row and rejected in row
                                                for row in failures), failures)
            # Real CMake commands inherit the EE target/mode before the native
            # overrides. Width and target selection must follow Clang's last
            # applicable flags, including LLP64 versus LP64 long.
            word_declarations = """
typedef void *(*PointerWord)(void *);
__UINTPTR_TYPE__ native_word(__UINTPTR_TYPE__ value) { return value; }
#if __SIZEOF_POINTER__ == 8
unsigned wrong_word(unsigned value) { return value; }
#else
unsigned long long wrong_word(unsigned long long value) { return value; }
#endif
"""
            overrides = (
                (["--target=i386-linux-gnu", "-m32", "--target=x86_64-w64-mingw32", "-m64"], False),
                (["--target=x86_64-w64-mingw32", "-m64", "-target", "i686-w64-mingw32", "-m32"], True),
                (["--target=x86_64-w64-mingw32", "-m64", "-m32"], True),
                (["--target=x86_64-w64-mingw32", "-m32", "-target", "x86_64-linux-gnu", "-m64"], False),
            )
            for flags, coff32 in overrides:
                bodies = (
                    ("PointerWord value = (PointerWord)native_word; (void)value(0);", None),
                    ("PointerWord value = (PointerWord)wrong_word; (void)value(0);", "return width/class"),
                    ("StdCallback value = (StdCallback)good; value(0, 1);",
                     "calling convention" if coff32 else None),
                )
                for body, rejected in bodies:
                    with self.subTest(overrides=flags, body=body):
                        source.write_text(prefix.replace("GLOBAL", word_declarations).replace("BODY", body),
                                          encoding="utf-8")
                        arguments = [self.clang, *flags, "-c", "-O0", "-Wno-everything",
                                     "-fno-builtin", str(source), "-o", str(obj)]
                        subprocess.run(arguments, check=True, capture_output=True)
                        entry = {"directory": str(root), "file": str(source), "arguments": arguments}
                        _, failures = crt_guard.audit(
                            [obj], [], self.nm, self.objdump, coff32, set(),
                            {"_GLOBAL_OFFSET_TABLE_", "SDL_GL_GetProcAddress"}, {}, {}, policy,
                            commands={os.path.normcase(str(obj.resolve())): entry})
                        if rejected is None:
                            self.assertFalse(failures, failures)
                        else:
                            self.assertTrue(any("indirect callback" in row and rejected in row
                                                for row in failures), failures)

    def test_target_prefixed_compiler_driver_uses_its_actual_abi(self):
        source_text = """
typedef __SIZE_TYPE__ size_t;
extern int puts(const char *);
extern void *memcpy(void *, const void *, size_t);
typedef void *(*PointerWord)(void *);
__UINTPTR_TYPE__ native_word(__UINTPTR_TYPE__ value) { return value; }
#if __SIZEOF_POINTER__ == 8
unsigned wrong_word(unsigned value) { return value; }
#else
unsigned long long wrong_word(unsigned long long value) { return value; }
#endif
void adapter(void) {
    char destination[8], source[8] = {0};
    puts("native diagnostics");
    memcpy(destination, source, sizeof source);
    PointerWord callback = (PointerWord)TARGET;
    (void)callback(0);
}
"""
        with tempfile.TemporaryDirectory(prefix="p4crt-driver-") as directory:
            root = Path(directory)
            source = root / "src/adapter.c"
            obj = root / "CMakeFiles/p4port.dir/src/adapter.c.o"
            source.parent.mkdir()
            obj.parent.mkdir(parents=True)
            # Preserve the selected driver's name: LLVM-Mingw links it to
            # clang-target-wrapper.sh, whose argv[0] chooses the target. Resolving
            # that link would invent a clang.sh driver and lose its ABI contract.
            compiler = Path(shutil.which(self.clang) or self.clang).absolute()
            policy = {"src/adapter.c": {
                "calls": {"puts": ["adapter"], "memcpy": ["adapter"], "memset": ["adapter"]},
                "reason": "Native-only fixture buffers and diagnostics."}}
            for triple in ("i686-w64-mingw32", "x86_64-w64-mingw32"):
                driver = root / (triple + "-clang" + compiler.suffix)
                try:
                    driver.symlink_to(compiler)
                except OSError:
                    # Windows may disallow symlinks; a real driver hard link
                    # still makes Clang derive the target from argv[0].
                    os.link(compiler, driver)
                for flags, coff32 in (
                        ([], triple.startswith("i686")),
                        (["--target=x86_64-linux-gnu", "-m64"], False),
                        (["--target=x86_64-w64-mingw32", "-m64", "-m32"], True)):
                    for target, rejected in (("native_word", False), ("wrong_word", True)):
                        with self.subTest(driver=driver.name, flags=flags, target=target):
                            source.write_text(source_text.replace("TARGET", target), encoding="utf-8")
                            arguments = [str(driver), *flags, "-c", "-O0", "-Wno-everything",
                                         str(source), "-o", str(obj)]
                            subprocess.run(arguments, check=True, capture_output=True)
                            entry = {"directory": str(root), "file": str(source), "arguments": arguments}
                            _, failures = crt_guard.audit(
                                [obj], [], self.nm, self.objdump, coff32, set(),
                                {"_GLOBAL_OFFSET_TABLE_"}, {}, {
                                    "memcpy": "Non-overlapping fixture arrays.",
                                    "memset": "Compiler zero-initialization of the native fixture array."},
                                policy, commands={os.path.normcase(str(obj.resolve())): entry})
                            if rejected:
                                self.assertTrue(any("return width/class" in row for row in failures), failures)
                            else:
                                self.assertFalse(failures, failures)

    def test_import_and_version_normalization(self):
        self.assertEqual(crt_guard.normalize("__imp__printf", True), "printf")
        self.assertEqual(crt_guard.normalize("__imp_printf"), "printf")
        self.assertEqual(crt_guard.normalize("memcpy@@GLIBC_2.14"), "memcpy")
        self.assertEqual(crt_guard.normalize("___divdi3", True), "__divdi3")


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--clang", default=shutil.which("clang"))
    parser.add_argument("--nm", default=shutil.which("llvm-nm"))
    parser.add_argument("--objdump", default=shutil.which("llvm-objdump"))
    args, rest = parser.parse_known_args()
    for name in ("clang", "nm", "objdump"):
        if not getattr(args, name):
            parser.error(f"{name} is required (guard tests must not silently skip)")
        setattr(GuardTest, name, getattr(args, name))
    unittest.main(argv=[sys.argv[0], *rest])

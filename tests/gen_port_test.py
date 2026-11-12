"""The mirror must reject missing, extra and retargeted text, not silently skip it."""
import argparse
import json
import os
import pathlib
import platform
import struct
import shutil
import subprocess
import sys
import tempfile
import unittest
from unittest import mock

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1] / "tools"))
import gen_port
import audit_prototypes
from port64 import gptr_rewrite, unit64


def setUpModule():
    global _cache_temp, _cache_env
    _cache_temp = tempfile.TemporaryDirectory()
    _cache_env = mock.patch.dict(os.environ, {
        "P4PC_BUILD_CACHE_DIR": str(pathlib.Path(_cache_temp.name) / "cache"),
        "P4PC_BUILD_CACHE_DISABLE": "0",
        "P4PC_BUILD_TOOLCHAIN_ID": "sha256:" + "1" * 64,
        "P4PC_BUILD_TRACE_DIR": str(pathlib.Path(_cache_temp.name) / "trace"),
    })
    _cache_env.start()


def tearDownModule():
    _cache_env.stop()
    _cache_temp.cleanup()


class MatchContractsTest(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.root = pathlib.Path(self.tmp.name)
        (self.root / "config").mkdir()
        self.mirror = self.root / "mirror"
        (self.mirror / "src").mkdir(parents=True)
        self.unit = self.mirror / "src/unit.c"
        self.patch = mock.patch.object(gen_port, "ROOT", self.root)
        self.patch.start()
        self.addCleanup(self.patch.stop)

    def test_function_scanner_ignores_calls_in_for_conditions(self):
        text = ("int strcmp(const char *, const char *);\n"
                "void f(void) { for (int i = 0; i < count && strcmp(name, names[i]); i++) {} }\n")
        functions = gen_port._scan_functions(gen_port._strip_literals(text), {})
        self.assertEqual([f[0] for f in functions].count("strcmp"), 1)
        self.assertIn(("strcmp", False, "i", ["p", "p"], False), functions)

    def test_compiled_project_archive_does_not_authorize_system_symbols(self):
        cc, ar, nm = (shutil.which(name) for name in ("clang", "ar", "nm"))
        if not all((cc, ar, nm)):
            self.skipTest("host clang/ar/nm unavailable")
        source = self.root / "bridge.c"
        obj = self.root / "bridge.o"
        archive = self.root / "bridge.a"
        source.write_text("void p4_mod_entry_example(void) {}\nvoid host_collision(void) {}\n", encoding="utf-8")
        subprocess.run([cc, "-c", str(source), "-o", str(obj)], check=True)
        subprocess.run([ar, "rcs", str(archive), str(obj)], check=True)
        # the host's own object format: Mach-O on macOS (its C names carry `_`), ELF elsewhere
        with mock.patch.object(gen_port.Tools, "nm", nm), mock.patch.object(gen_port.Tools, "coff", False), \
                mock.patch.object(gen_port.Tools, "macho", sys.platform == "darwin"):
            self.assertEqual(gen_port.project_symbols([archive], {"host_collision"}), {"p4_mod_entry_example"})

    def test_retail_aliases_read_root_relative_manifest_entries(self):
        build = self.root / "build/gen"
        source = build / "decomp/src/unit.c"
        source.parent.mkdir(parents=True)
        source.write_text("// FUN_00100100\nvoid renamed(void) { }\n", encoding="utf-8")
        (build / "units.txt").write_text(source.relative_to(self.root).as_posix() + "|\n", encoding="utf-8")
        with mock.patch.object(gen_port, "read_symbols", return_value={}), \
                mock.patch.object(gen_port, "read_port_symbols", return_value={}), \
                mock.patch.object(gen_port, "read_native_names", return_value={}):
            self.assertEqual(gen_port.retail_function_names(self.root / "decomp", build),
                             {"renamed": 0x100100})

    def test_empty_marker_does_not_borrow_the_next_native_definition(self):
        text = ("// FUN_00100100\n// removed assembly body\n"
                "// FUN_00100200\nvoid real_body(void) { }\n")
        markers = list(gen_port.MARKER_RE.finditer(text))
        self.assertIsNone(gen_port.marker_definition_name(text, markers[0].end()))
        self.assertEqual(gen_port.marker_definition_name(text, markers[1].end()), "real_body")

    def test_override_marker_settles_before_the_body_it_replaced(self):
        (self.root / "overrides").mkdir()
        self.unit.write_text(
            "typedef int s32;\n"
            "// FUN_00100100\ns32 target(s32 v) { return v; }\n"
            "// FUN_00100200\nINCLUDE_ASM(\"asm/nonmatchings/unit\", func_00100200);\n"
            "// FUN_00100300\ns32 kept(s32 v) { return v; }\n"
            "// FUN_00100400\ns32 original(s32 v) { return v; }\n", encoding="utf-8")
        overrides = self.root / "overrides"
        # Data and a static helper before the body it replaces.
        (overrides / "00100100.c").write_text(
            "extern s32 D_00700000[];\nstatic s32 table[11];\n"
            "static s32 helper(s32 v) { return v + table[0]; }\n"
            "s32 target(s32 v) { return helper(v) + D_00700000[0]; }\n", encoding="utf-8")
        # An asm-only marker whose address-named body follows a struct type.
        (overrides / "00100200.c").write_text(
            "typedef struct { s32 a; } Pair;\ns32 func_00100200(Pair *p) { return p->a; }\n", encoding="utf-8")
        # Includes and comments already resolve: nothing moves.
        (overrides / "00100300.c").write_text(
            "#include <stddef.h>\n/* note */\ns32 kept(s32 v) { return v + 1; }\n", encoding="utf-8")
        # A renamed body (bound by its own port symbol) is left where it is.
        (overrides / "00100400.c").write_text(
            "extern s32 elsewhere(void);\ns32 renamed(s32 v) { return v + elsewhere(); }\n", encoding="utf-8")
        gen_port.apply_overrides(self.root, self.mirror, ["src/unit.c"])
        text = self.unit.read_text(encoding="utf-8")
        names = {int(m[1], 16): gen_port.marker_definition_name(text, m.end()) for m in gen_port.MARKER_RE.finditer(text)}
        self.assertEqual(names, {0x100100: "target", 0x100200: "func_00100200", 0x100300: "kept", 0x100400: "renamed"})
        self.assertIn("static s32 helper(s32 v) { return v + table[0]; }\n// FUN_00100100\ns32 target", text)
        self.assertIn("typedef struct { s32 a; } Pair;\n// FUN_00100200\ns32 func_00100200", text)
        self.assertIn("// FUN_00100300\n#include <stddef.h>", text)
        # Only the comment moved: removing every marker gives the plain splice.
        plain = gen_port.MARKER_RE.sub("", text)
        self.assertIn("extern s32 D_00700000[];\nstatic s32 table[11];\nstatic s32 helper", plain)

    def test_override_without_an_identifiable_body_fails_the_mirror(self):
        (self.root / "overrides").mkdir()
        self.unit.write_text("// FUN_00100100\nint target(int v) { return v; }\n", encoding="utf-8")
        (self.root / "overrides/00100100.c").write_text(
            "static int table[2];\nstatic int only_helpers(int v) { return table[v]; }\n", encoding="utf-8")
        with self.assertRaises(SystemExit):
            gen_port.apply_overrides(self.root, self.mirror, ["src/unit.c"])

    def test_inactive_movie_cut_removes_bodies_tables_and_markers_only(self):
        cold = self.mirror / "src/cold.c"
        cold.write_text("void software_decoder(void) {}\n", encoding="utf-8")
        self.unit.write_text(
            "typedef void (*Callback)(void);\n"
            "void cold(void);\n"
            "// FUN_00100100\n"
            "void cold(void) { const char *brace = \"}\"; if (brace) { } }\n"
            "static const Callback cold_table[1] = { cold };\n"
            "// FUN_00100200\n"
            "void warm(void) { }\n", encoding="utf-8")
        policy = {"units": ["src/cold.c"], "functions": {"src/unit.c": ["cold"]},
                  "registrations": {"src/unit.c": ["cold_table"]},
                  "markers": {"src/unit.c": ["00100100"]}}
        with mock.patch.object(gen_port, "read_inactive_movie", return_value=policy):
            self.assertEqual(gen_port.remove_inactive_movie(self.mirror), [1, 1, 1])
        text = self.unit.read_text(encoding="utf-8")
        self.assertFalse(cold.exists())
        self.assertIn("void cold(void);", text)
        self.assertNotIn("brace", text)
        self.assertNotIn("cold_table", text)
        self.assertNotIn("FUN_00100100", text)
        self.assertIn("// FUN_00100200\nvoid warm(void) { }", text)

    def test_inactive_movie_cut_rejects_a_stale_definition_review(self):
        self.unit.write_text("void warm(void) {}\n", encoding="utf-8")
        policy = {"units": [], "functions": {"src/unit.c": ["missing"]},
                  "registrations": {}, "markers": {}}
        with mock.patch.object(gen_port, "read_inactive_movie", return_value=policy):
            with self.assertRaisesRegex(SystemExit, "expected one definition, found 0"):
                gen_port.remove_inactive_movie(self.mirror)

    def test_abi_merge_reads_only_the_explicit_live_source_manifest(self):
        directory = self.root / "abi"
        directory.mkdir()
        live = directory / "live.abi.json"
        stale = directory / "removed_decoder.abi.json"
        row = {"ret": "int", "rt": "int", "params": [], "skip": False}
        live.write_text(json.dumps({"live": row}), encoding="utf-8")
        stale.write_text(json.dumps({"removed_decoder": row}), encoding="utf-8")
        manifest = self.root / "abi-files.txt"
        manifest.write_text(str(live) + "\n", encoding="utf-8")
        output = self.root / "merged.json"
        args = mock.Mock(abi_files=str(manifest), dir=str(directory), root=str(self.root),
                         gen=str(self.root / "build/gen"), out=str(output), clang="clang")
        with mock.patch.object(unit64.gr, "sigs_abi", return_value={}), \
                mock.patch.object(unit64.gr, "collect_host_abi", return_value={}), \
                mock.patch.object(gen_port, "retail_function_names", return_value={}):
            unit64.cmd_merge(args)
        merged = json.loads(output.read_text(encoding="utf-8"))
        self.assertEqual(set(merged), {"live"})
        self.assertTrue(stale.exists())

    def test_alias_alignment_reorders_generic_and_asm_label_calls(self):
        signature = ("p", ["p", "p", "f", "i"])
        definitions = {"Rotate": signature, "existing": ("i", ["i"])}
        names = {"Rotate": 0x3e0870, "func_003e0870": 0x3e0870, "existing": 0x3e0870}
        gen_port.add_alias_definition_signatures(definitions, names, {0x3e0870: "Rotate"}, {})
        self.assertEqual(definitions["func_003e0870"], signature)
        self.assertEqual(definitions["existing"], ("i", ["i"]))
        self.unit.write_text(
            'extern void func_003e0870(void *m, void *axis, int mode, float angle);\n'
            'extern void labeled(void *m, void *axis, int mode, float angle) __asm__("func_003e0870");\n'
            'void caller(void *m, void *axis, int mode, float angle) {\n'
            '    func_003e0870(m, axis, mode, angle);\n'
            '    labeled(m, axis, mode, angle);\n'
            '}\n', encoding="utf-8")
        result = gen_port.align_prototypes_with_definitions(self.mirror, definitions, {}, {})
        self.assertEqual(result[1:3], (2, 2))
        text = self.unit.read_text()
        self.assertIn('float angle, int mode) __asm__("func_003e0870");', text)
        self.assertIn("func_003e0870(m, axis, angle, mode);", text)
        self.assertIn("labeled(m, axis, angle, mode);", text)

    def test_alias_authority_excludes_unbound_bodies(self):
        definitions = {"wrong": ("i", ["i", "f"]), "right": ("i", ["f", "i"])}
        names = {"wrong": 0x100100, "right": 0x100100}
        gen_port.add_alias_definition_signatures(definitions, names, {0x100100: "wrong"}, {0x100100: "wrong"})
        self.assertEqual(definitions["func_00100100"], definitions["right"])

    def test_float_reorder_preserves_a_whole_aggregate_argument(self):
        self.unit.write_text(
            "extern void Draw(Vec position, float depth, int alpha, int style);\n"
            "void caller(Vec position, float depth, int alpha, int style) {\n"
            "    Draw(position, depth, alpha, style);\n"
            "}\n", encoding="utf-8")
        result = gen_port.align_prototypes_with_definitions(
            self.mirror, {"Draw": ("v", ["@agg", "i", "i", "f"])}, {"Vec": "@agg"}, {})
        self.assertEqual(result[1:3], (1, 1))
        text = self.unit.read_text()
        self.assertIn("Draw(Vec position, int alpha, int style, float depth);", text)
        self.assertIn("Draw(position, alpha, style, depth);", text)

    def test_literal_deletion_still_requires_exact_old_text(self):
        (self.root / "config/mirror_fixes.txt").write_text("src/unit.c\tunused declaration;\t\tremove obsolete local\n",
                                                        encoding="utf-8")
        self.unit.write_text("unused declaration;\n", encoding="utf-8")
        self.assertEqual(gen_port.apply_mirror_fixes(self.mirror), 1)
        self.assertEqual(self.unit.read_text(), "\n")
        with self.assertRaises(SystemExit):
            gen_port.apply_mirror_fixes(self.mirror)

    def test_new_alias_coverage_does_not_change_unrelated_widths_or_returns(self):
        original = ('extern long long labeled(int value) __asm__("func_00100100");\n'
                    'void caller(int value) { labeled(value); }\n')
        self.unit.write_text(original, encoding="utf-8")
        result = gen_port.align_prototypes_with_definitions(
            self.mirror, {"func_00100100": ("f", ["q"])}, {}, {})
        self.assertEqual(result[:5], (0, 0, 0, 0, 0))
        self.assertEqual(self.unit.read_text(), original)

    def test_literal_cardinality(self):
        path = self.root / "config/mirror_fixes.txt"
        path.write_text("src/unit.c\told\tnew\treason\n", encoding="utf-8")
        for text in ("none", "old old"):
            with self.subTest(text=text):
                self.unit.write_text(text, encoding="utf-8")
                with self.assertRaises(SystemExit):
                    gen_port.apply_mirror_fixes(self.mirror)
        path.write_text("src/unit.c\told\tnew\treason\t2\n", encoding="utf-8")
        self.unit.write_text("old old", encoding="utf-8")
        self.assertEqual(gen_port.apply_mirror_fixes(self.mirror), 1)
        self.assertEqual(self.unit.read_text(), "new new")

    def test_battle_selector_script_query_uses_one_i386_argument(self):
        repo = pathlib.Path(__file__).resolve().parents[1]
        import re
        units = {
            "src/Battle/btlEffect.c": 5,
            "src/Battle/btlEffect_inferred_001e6d20.c": 1,
            "src/Battle/btlEffect_inferred_001e7400.c": 15,
            "src/Battle/btlEffect_inferred_001e8d80.c": 3,
        }
        fixes = (repo / "config/mirror_fixes.txt").read_text(encoding="utf-8").splitlines()
        decomp = pathlib.Path(os.environ.get("P4PC_TEST_DECOMP", repo / "decomp"))
        calls = []
        for unit, expected in units.items():
            source = (decomp / unit).read_text(encoding="utf-8")
            rows = [line.split("\t") for line in fixes if line.startswith(unit + "\t")
                    and "btlAICommandSelectCandidates(" in line]
            self.assertTrue(rows, unit)
            for _, old, new, _, count in rows:
                self.assertEqual(source.count(old), int(count), unit)
                source = source.replace(old, new)
            # The expanded AI carrier wraps a site in P4_AI_SELECT, whose retail
            # fallback passes the same six arguments to the original selector.
            found = re.findall(r"((?:btlAICommandSelectCandidates|P4_AI_SELECT)\((?:action|work),[^\n]+?\)) == 1", source)
            self.assertEqual(len(found), expected, unit)
            self.assertTrue(all(re.match(r"(?:btlAICommandSelectCandidates|P4_AI_SELECT)\((?:action|work), \(s32\)", call)
                                for call in found), unit)
            found = [re.sub(r"^P4_AI_SELECT\((.*), P4_GAME_AI_TEST_\w+\)$", r"btlAICommandSelectCandidates(\1)", call)
                     for call in found]
            calls.extend(found)

        # Exercise the actual rewritten argument expressions on the i386 ABI.
        # The callee is a separate TU, as it is in the game, so the old-style
        # declaration cannot be corrected by seeing the definition here.
        if sys.platform != "linux" or platform.machine().lower() not in ("x86_64", "i386", "i686"):
            return
        cc = shutil.which("clang")
        if cc is None:
            self.fail("clang is required for the i386 selector ABI witness")
        calls = [re.sub(r"&btl(?:Cond|AICommand)\w+", "&predicate", call) for call in calls]
        caller = self.root / "caller.c"
        callee = self.root / "callee.c"
        exe = self.root / "selector-abi"
        caller.write_text(
            "typedef int s32; typedef long long s64; typedef unsigned char u8;\n"
            "extern s32 btlAICommandSelectCandidates();\n"
            "s64 scrGetParamInt(int n) { return 0x1234567812345678LL + n; }\n"
            "s32 predicate(u8 *p, short n) { return p != 0 && n != 0; }\n"
            "int main(void) { u8 byte = 1, *action = &byte, *work = &byte; "
            "s64 flags = scrGetParamInt(0);\n"
            + "".join("if (!" + call + ") return 1;\n" for call in calls)
            + "return 0; }\n", encoding="utf-8")
        callee.write_text(
            "typedef int s32; typedef unsigned char u8;\n"
            "extern s32 predicate(u8 *, short);\n"
            "s32 btlAICommandSelectCandidates(u8 *p, s32 value, s32 excluded, s32 mode, "
            "s32 rejected, s32 (*cb)(u8 *, short)) {\n"
            "return p && value == 0x12345678 && (excluded == 0 || excluded == 0x100000)"
            " && (mode == 0x2c || mode == 0x2d || mode == 0x2e || mode == 0x2f)"
            " && (rejected == 0 || rejected == 1) && cb == predicate && cb(p, 1); }\n",
            encoding="utf-8")
        subprocess.run([cc, "-m32", "-Wno-deprecated-non-prototype", str(caller), str(callee),
                        "-o", str(exe)], check=True)
        subprocess.run([str(exe)], check=True)

    def test_libc_aliases_parse_directives_without_comment_false_targets(self):
        header = self.root / "include/p4pc/libc_rename.h"
        header.parent.mkdir(parents=True)
        header.write_text(
            "/*\n#define ignored fake_native\n*/\n"
            "#ifndef GUARD\n#define GUARD\n"
            "# define sinf native_sinf /* evidence */\n"
            "#define __DECL_SIMD_native_sinf\n"
            "#define helper(x) ((x) + 1)\n"
            "#define cosf native_cosf // evidence\n#endif\n", encoding="utf-8")
        self.assertEqual(gen_port.read_libc_renames(), {"sinf": "native_sinf", "cosf": "native_cosf"})

    def test_libc_aliases_reject_duplicates_and_nonidentifier_targets(self):
        header = self.root / "include/p4pc/libc_rename.h"
        header.parent.mkdir(parents=True)
        for entry in (
                "#define sinf another_native\n",
                "#define cosf (native_cosf)\n",
                "#define cosf native_cosf + extra\n",
                '#define cosf "not_a_native"\n',
                "#define cosf 42\n"):
            header.write_text("#define sinf native_sinf\n" + entry, encoding="utf-8")
            with self.subTest(entry=entry), self.assertRaises(SystemExit):
                gen_port.read_libc_renames()

    def test_native_symbol_collector_rejects_data_as_callable_targets(self):
        kinds = {
            "native": "T", "weak_native": "W", "resolver": "i",
            "global_data": "D", "constant_data": "R", "weak_data": "V",
            "external": "U", "weak_external": "w",
        }
        for coff, prefix in ((False, ""), (True, "_"), (True, "")):
            output = "fixture.o:\n" + "\n".join(
                f"{prefix}{name} {kind} 0 4" for name, kind in kinds.items())
            if coff:
                output += "\n__imp_printf I 0 8\n"
            with self.subTest(coff=coff, prefix=prefix), \
                    mock.patch.object(gen_port.Tools, "coff", coff), \
                    mock.patch.object(gen_port.Tools, "us", prefix), \
                    mock.patch.object(gen_port.subprocess, "run", return_value=mock.Mock(stdout=output)):
                defined, undefined, functions = gen_port.nm_symbols([self.root / "fixture.o"])
            self.assertEqual(undefined, {"external", "weak_external"})
            self.assertEqual(functions, {"native", "weak_native", "resolver"})
            self.assertEqual(defined - functions, {"global_data", "constant_data", "weak_data"})

    def test_pass_census_rejects_missing_extra_and_retargeted_edits(self):
        def transform(mirror):
            path = mirror / "src/unit.c"
            path.write_text(path.read_text().replace("old", "new"), encoding="utf-8")

        original = "before\nold\nafter\n"
        self.unit.write_text(original, encoding="utf-8")
        contracts = gen_port.MirrorContracts(self.mirror, refresh=True)
        contracts.run(transform, self.mirror)
        contracts.finish()
        for text in (original, original.replace("\n", "\r\n"), "before\nafter\n", "old\nold\n", "before\nold changed\nafter\n", "renamed caller parameter\nold\nafter\n"):
            self.unit.write_bytes(text.encode("utf-8"))
            contracts = gen_port.MirrorContracts(self.mirror)
            if text.replace("\r\n", "\n") == original:
                contracts.run(transform, self.mirror)
                contracts.finish()
            else:
                contracts.run(transform, self.mirror)
                with self.subTest(text=text), self.assertRaises(SystemExit):
                    contracts.finish()

    def test_no_automatic_learning_and_removed_pass(self):
        with self.assertRaises(SystemExit):
            gen_port.MirrorContracts(self.mirror)
        (self.root / "config/mirror_contracts.json").write_text(json.dumps({"old_pass": {}}), encoding="utf-8")
        with self.assertRaises(SystemExit):
            gen_port.MirrorContracts(self.mirror).finish()

    def test_unedited_header_type_changes_fail_the_source_contract(self):
        source = self.root / "decomp"
        (source / "include").mkdir(parents=True)
        header = source / "include/row.h"
        header.write_text("typedef struct Row { int value; } Row;\n", encoding="utf-8")
        def copy_header():
            (self.mirror / "include").mkdir(exist_ok=True)
            (self.mirror / "include/row.h").write_bytes(header.read_bytes())
        contracts = gen_port.MirrorContracts(self.mirror, refresh=True)
        contracts.run(copy_header, before_root=source)
        contracts.finish()
        header.write_text("typedef struct Row { int value, added; } Row;\n", encoding="utf-8")
        contracts = gen_port.MirrorContracts(self.mirror)
        contracts.run(copy_header, before_root=source)
        with self.assertRaises(SystemExit):
            contracts.finish()

    def test_failed_warm_stage_never_becomes_a_later_mirror_input(self):
        source = self.root / "decomp"
        (source / "src").mkdir(parents=True)
        (source / "src/unit.c").write_text("int current(void) { return 1; }\n", encoding="utf-8")
        baseline = gen_port.MirrorContracts(self.mirror, refresh=True)
        baseline.run(gen_port.mirror_tree, source, self.mirror, before_root=source)
        baseline.finish()
        manifest = self.root / "config/mirror_contracts.json"
        expected = manifest.read_bytes()
        build = self.root / "build"
        published = build / "decomp/src/published.c"
        published.parent.mkdir(parents=True)
        published.write_text("previous successful mirror\n", encoding="utf-8")
        stage = build / "decomp.stage"
        observed = []
        factory = gen_port.MirrorContracts

        def contracts(*args):
            result = factory(*args)
            observed.append(result)
            return result

        def strip_local_varargs(_mirror):
            raise RuntimeError("after first pass")

        arguments = argparse.Namespace(decomp=source, build=build, refresh_contracts=False, mod_hooks=False)
        for attempt in range(2):
            (stage / "include").mkdir(parents=True, exist_ok=True)
            (stage / "include/later_pin_internal.h").write_text("void later_pin(void);\n", encoding="utf-8")
            (stage / "src").mkdir(exist_ok=True)
            (stage / "src/removed_in_current_pin.c").write_text("void obsolete(void) {}\n", encoding="utf-8")
            with self.subTest(attempt=attempt), \
                    mock.patch.object(gen_port, "read_units", return_value=([], [])), \
                    mock.patch.object(gen_port, "mirror_cache_inputs", return_value={"fixture": "failed-stage"}), \
                    mock.patch.object(gen_port, "check_decomp_targets"), \
                    mock.patch.object(gen_port, "MirrorContracts", side_effect=contracts), \
                    mock.patch.object(gen_port, "strip_local_varargs", strip_local_varargs):
                with self.assertRaisesRegex(RuntimeError, "after first pass"):
                    gen_port.cmd_mirror(arguments)
            self.assertEqual(observed[-1].errors, [])
            self.assertEqual(observed[-1].actual["mirror_tree"], baseline.actual["mirror_tree"])
            self.assertFalse((stage / "include/later_pin_internal.h").exists())
            self.assertFalse((stage / "src/removed_in_current_pin.c").exists())
            self.assertEqual(published.read_text(encoding="utf-8"), "previous successful mirror\n")
            self.assertEqual(manifest.read_bytes(), expected)

    def test_claims_and_bindings_reject_stale_or_malformed_targets(self):
        (self.root / "overrides").mkdir()
        claims = self.root / "overrides/CLAIMS.md"
        valid = "00123456 Lane native override\n"
        claims.write_text(valid, encoding="utf-8")
        self.unit.write_text("// FUN_00123456\nint named(void) { return 0; }\n",
                             encoding="utf-8")
        with mock.patch.object(gen_port, "read_symbols", return_value={}):
            gen_port.check_decomp_targets(self.mirror)
            for text in (valid + valid, "00123457 Lane missing\n",
                         "0012345g Lane malformed\n", "0x00123456 Lane malformed\n"):
                claims.write_text(text, encoding="utf-8")
                with self.subTest(claim=text), self.assertRaises(SystemExit):
                    gen_port.check_decomp_targets(self.mirror)
            claims.write_text(valid, encoding="utf-8")
            unbound = self.root / "config/unbound_natives.txt"
            unbound.write_text("0x00123456 named\n", encoding="utf-8")
            gen_port.check_decomp_targets(self.mirror)
            unbound.write_text("0x00123456 old_name\n", encoding="utf-8")
            with self.assertRaises(SystemExit):
                gen_port.check_decomp_targets(self.mirror)
            unbound.write_text("", encoding="utf-8")
            (self.root / "config/native_names.txt").write_text(
                "0x00123457 native\n", encoding="utf-8")
            with self.assertRaises(SystemExit):
                gen_port.check_decomp_targets(self.mirror)

    def test_address_signature_preserves_named_caller_abis(self):
        body = ("int helper(void) { return 0; }\n"
                "void *\nfunc_00123456(void *task)\n{ return task; }\n")
        self.assertEqual(gen_port.override_signature(body, "00123456"),
                         ("func_00123456", "void * func_00123456(void *task)"))
        self.assertIsNone(gen_port.override_signature(body.replace("func_00123456", "renamed"), "00123456"))
        body = "s32 (**func_00123456(void))(u32) { return 0; }\n"
        self.assertEqual(gen_port.override_signature(body, "00123456"),
                         ("func_00123456", "s32 (**func_00123456(void))(u32)"))
        marked = "// FUN_00123456\n" + body.replace("func_00123456", "renamed")
        marker = gen_port.MARKER_RE.search(marked)
        self.assertEqual(gen_port.marker_definition_name(marked, marker.end()), "renamed")

    def test_addressless_calls_cannot_hide_missing_helpers(self):
        path = self.root / "config/call_traps.txt"
        path.write_text("# Existing SDK seam\nSDKCall\n", encoding="utf-8")
        gen_port.check_addressless_traps({"SDKCall": 0, "KnownAddress": 0x100000})
        with self.assertRaises(SystemExit):
            gen_port.check_addressless_traps({"SDKCall": 0, "RemovedHelper": 0})
        with self.assertRaises(SystemExit):
            gen_port.check_addressless_traps({"KnownAddress": 0x100000})
        path.write_text("SDKCall\nSDKCall\n", encoding="utf-8")
        with self.assertRaises(SystemExit):
            gen_port.check_addressless_traps({"SDKCall": 0})
        path.write_text("SDKCall extra\n", encoding="utf-8")
        with self.assertRaises(SystemExit):
            gen_port.check_addressless_traps({"SDKCall": 0})

    def test_claim_and_unbinding_targets(self):
        decomp = self.root / "decomp"
        (decomp / "src").mkdir(parents=True)
        (decomp / "config").mkdir()
        (self.root / "overrides").mkdir()
        (self.root / "overrides/CLAIMS.md").write_text("00123456 Lane function\n", encoding="utf-8")
        for name in ("symbol_addrs.txt", "symbol_data_addrs.txt", "symbols_recovered.txt"):
            (decomp / "config" / name).write_text("", encoding="utf-8")
        (self.root / "config/unbound_natives.txt").write_text("0x00123456 original\n", encoding="utf-8")
        unit = decomp / "src/unit.c"
        unit.write_text("// FUN_00123456\nint original(void) { return 1; }\n", encoding="utf-8")
        gen_port.check_decomp_targets(decomp)
        unit.write_text("// FUN_00123456\nint renamed(void) { return 1; }\n", encoding="utf-8")
        with self.assertRaises(SystemExit):
            gen_port.check_decomp_targets(decomp)
        (self.root / "config/unbound_natives.txt").write_text("", encoding="utf-8")
        unit.write_text("", encoding="utf-8")
        with self.assertRaises(SystemExit):
            gen_port.check_decomp_targets(decomp)

    def test_typed_effect_dispatch_preserves_incoming_instance(self):
        unit = self.mirror / "src/mdlEffect.c"
        unit.write_text("(*(void (**)(void))(D_00714010 + "
                        "(((MdlEffectVectorInstance *)instance)->kind.value * 0x1C)))();\n",
                        encoding="utf-8")
        self.assertEqual(gen_port.pass_effect_kind_dispatch(self.mirror), 1)
        self.assertIn("(*(void (**)(u8 *))", unit.read_text())
        self.assertIn(")(instance);", unit.read_text())

    def test_shared_nested_matrix_and_parenthesized_array_alignment(self):
        self.unit.write_text(
            "typedef struct RwMatrixTag {\n"
            " union { Vec3 right; struct { int x, y, z; }; };\n"
            " int flags;\n"
            " union { Vec3 up; int words[3]; };\n"
            " int pad1;\n"
            " union { Vec3 at; int words2[3]; };\n"
            " int pad2;\n"
            " union { Vec3 pos; int words3[3]; };\n"
            " int pad3;\n"
            "} RwMatrixTag;\n"
            "void use(void) {\n"
            "    u8 matrix[0x40];\n"
            "    rotate((RwMatrixTag *)(matrix));\n"
            "}\n", encoding="utf-8")
        self.assertEqual(gen_port.align_matrix_structs(self.mirror), (2, 1))
        text = self.unit.read_text()
        self.assertIn("} __attribute__((aligned(16))) RwMatrixTag;", text)
        self.assertIn("u8 matrix[0x40] __attribute__((aligned(16)));", text)
        self.assertEqual(gen_port.align_matrix_structs(self.mirror), (0, 0))

    def test_offset_assertions_remain_constant_without_runtime_rewrites(self):
        runtime = "u32 keep(void) { return (u32)&((struct Row *)0)->link.next; }\n"
        self.unit.write_text(
            "typedef char LinkOffset[(u32)&((struct Row *)0)->link.next == 0x14 ? 1 : -1];\n"
            "typedef char ArrayOffset[(u32)&((Row *)0)->items[2].value == 0x20 ? 1 : -1];\n"
            "typedef char NestedOffset[\n"
            "    (u32)&((Row *)0)->recovery.cursors == 0x3F0 ? 1 : -1];\n" +
            runtime, encoding="utf-8")
        self.assertEqual(gen_port.portable_offset_asserts(self.mirror), (3, 1))
        text = self.unit.read_text()
        self.assertIn("((u32)__builtin_offsetof(struct Row, link.next)) == 0x14", text)
        self.assertIn("((u32)__builtin_offsetof(Row, items[2].value)) == 0x20", text)
        self.assertIn("((u32)__builtin_offsetof(Row, recovery.cursors)) == 0x3F0", text)
        self.assertIn(runtime, text)
        self.assertEqual(gen_port.portable_offset_asserts(self.mirror), (0, 0))

    def test_port_include_fragment_retains_real_typed_header_visibility(self):
        include = self.root / "include/p4pc"
        include.mkdir(parents=True)
        (include / "native.h").write_text(
            "typedef struct Frame Frame;\n"
            "int actual_frame_query(void *actor, const Frame **frames, unsigned int *count);\n",
            encoding="utf-8")
        (include / "adapter.inc").write_text('#include "p4pc/native.h"\n', encoding="utf-8")
        self.unit.write_text(
            '#include "p4pc/adapter.inc"\n'
            "int consumer(void *actor) {\n"
            "    const Frame *frames; unsigned int count;\n"
            "    return actual_frame_query(actor, &frames, &count);\n"
            "}\n", encoding="utf-8")
        before = self.unit.read_text()
        with mock.patch.object(gen_port, "retail_function_names", return_value={}):
            gen_port.prototype_unprototyped_declarations(self.mirror, self.root, self.root / "prototypes.txt")
        self.assertEqual(self.unit.read_text(), before)
        self.assertNotIn("extern int actual_frame_query", self.unit.read_text())

    def test_tail_return_headers_do_not_require_unrelated_cri_typedefs(self):
        include = self.mirror / "include"
        include.mkdir()
        header = include / "callbacks.h"
        header.write_text("void adapter(void *node, void *work);\n", encoding="utf-8")
        self.unit.write_text(
            "typedef unsigned int u32;\n"
            "u32 leaf(void *work) {\n    return 1;\n}\n"
            "void adapter(void *node, void *work)\n{\n    leaf(work);\n}\n",
            encoding="utf-8")
        self.assertEqual(gen_port.tail_call_returns(self.mirror)[0], 1)
        self.assertEqual(header.read_text(), "unsigned int adapter(void *node, void *work);\n")
        self.assertIn("return leaf(work);", self.unit.read_text())
        self.assertEqual(gen_port.tail_call_returns(self.mirror), (0, 0))

    def test_asm_definition_modifier_is_not_a_c_return_type(self):
        self.unit.write_text(
            "extern void syscall_stub(void);\n"
            "extern int repaired_stub(void);\n"
            "asm void syscall_stub(void)\n{\n}\n"
            "asm void repaired_stub(void)\n{\n}\n", encoding="utf-8")
        self.assertEqual(gen_port.align_prototype_returns(self.mirror), (1, 1))
        text = self.unit.read_text()
        self.assertIn("extern void syscall_stub(void);", text)
        self.assertIn("extern void repaired_stub(void);", text)
        self.assertNotIn("extern asm", text)
        self.assertIn("asm void syscall_stub(void)", text)
        self.assertIn("asm void repaired_stub(void)", text)
        self.assertEqual(gen_port.align_prototype_returns(self.mirror), (0, 0))

    def test_integer_return_spelling_uses_available_type_namespace(self):
        self.assertEqual(gen_port._spell_int("Uint32", "void adapter(void);"), "unsigned int")
        self.assertEqual(gen_port._spell_int("Sint32", "void adapter(void);"), "int")
        self.assertEqual(gen_port._spell_int("Uint32", "u8 *work;"), "u32")
        self.assertEqual(gen_port._spell_int("u32", '#include "cri_xpt.h"\n'), "Uint32")

    def test_u8_predicates_keep_full_register_returns(self):
        self.unit.write_text(
            "u8 btlFormationHasEntry(u32 *work) { return work[0] != 0; }\n"
            "u8 btlCond_ANALYZE(s32 action) { return action != 0; }\n",
            encoding="utf-8")
        self.assertEqual(gen_port.widen_narrow_returns(self.mirror), 2)
        text = self.unit.read_text()
        self.assertIn("u32 btlFormationHasEntry", text)
        self.assertIn("u32 btlCond_ANALYZE", text)
        self.assertIn("return (u8)(work[0] != 0);", text)
        self.unit.write_text("u8 btlCond_ANALYZE(s32 action) { return action != 0; }\n", encoding="utf-8")
        with self.assertRaises(SystemExit):
            gen_port.widen_narrow_returns(self.mirror)

    def test_narrow_return_declarations_precede_calls_before_override_includes(self):
        (self.mirror / "src/definitions.c").write_text(
            "u16 panel(void *object) { return 1; }\n"
            "u8 other(void *object) { return 1; }\n", encoding="utf-8")
        self.unit.write_text(
            '#include "types.h"\n'
            "int first(void *p) { return panel(p) + other(p); }\n"
            '#include "override.h"\n'
            "extern u16 panel(void *);\n"
            "int second(void) { return 0; }\n", encoding="utf-8")
        gen_port.align_narrow_returns(self.mirror)
        text = self.unit.read_text(encoding="utf-8")
        first = text.index("int first(")
        self.assertLess(text.index("extern u16 panel(void *);"), first)
        self.assertLess(text.index("extern u8 other();"), first)
        self.assertGreater(text.index('#include "override.h"'), first)
        self.assertEqual(gen_port.align_narrow_returns(self.mirror)[0], 0)


class MachOTest(unittest.TestCase):
    """macOS (--macho): C names carry `_`, asm labels get it from the rewriter's Mach-O pass, the i386 oracle follows the triple."""

    def setUp(self):
        saved = (gen_port.Tools.macho, gen_port.Tools.coff, set(gen_port.Tools.raw_undefined))
        def restore():
            gen_port.Tools.macho, gen_port.Tools.coff, gen_port.Tools.raw_undefined = saved
        self.addCleanup(restore)
        gen_port.Tools.macho, gen_port.Tools.coff = True, False
        gen_port.Tools.raw_undefined = set()

    def test_symbol_names_and_relocations(self):
        self.assertEqual(gen_port.target_name("_SDL_Init"), "SDL_Init")
        self.assertEqual(gen_port.target_name("___divdi3"), "__divdi3")
        self.assertEqual(gen_port.target_name("_ext@GOTPCREL"), "ext")
        self.assertEqual(gen_port.target_name("func_003df4d0"), "func_003df4d0")
        # `objdump --macho -r` rows (arm64 prints a pointer's length as `?( 3)`); only an external entry names a symbol
        for line, ext, kind, name in (("00000018 True  long   True   BR26    False     _ext_fn", "True", "BR26", "_ext_fn"),
                                      ("000695f0 False ?( 3)  True   UNSIGND False     _vfs_stat", "True", "UNSIGND", "_vfs_stat"),
                                      ("00000031 True  long   True   BRANCH  False     func_003df4d0", "True", "BRANCH", "func_003df4d0"),
                                      ("000695f8 False quad   False  UNSIGND False     2 (__TEXT,__cstring)", "False", "UNSIGND", "2")):
            m = gen_port.MACHO_RELOC_RE.match(line)
            self.assertIsNotNone(m, line)
            self.assertEqual((m[1], m[2], m[3]), (ext, kind, name))
        self.assertIn("ARM64_RELOC_BRANCH26", gen_port.CALL_RELOCS)
        self.assertIn("X86_64_RELOC_BRANCH", gen_port.CALL_RELOCS)
        self.assertNotIn("X86_64_RELOC_GOT_LOAD", gen_port.CALL_RELOCS)

    def test_real_objects_relocation_kinds(self):
        cc, objdump = shutil.which("clang"), shutil.which("llvm-objdump")
        if not cc or not objdump:
            self.skipTest("clang/llvm-objdump unavailable")
        saved = (gen_port.Tools.objdump, gen_port.Tools.arm64)
        self.addCleanup(lambda: (setattr(gen_port.Tools, "objdump", saved[0]), setattr(gen_port.Tools, "arm64", saved[1])))
        gen_port.Tools.objdump = objdump
        with tempfile.TemporaryDirectory() as tmp:
            src = pathlib.Path(tmp) / "table.c"
            src.write_text("void called(void); void listed(void); extern int datum;\n"
                           "void *const table[] = { (void *)&listed, (void *)&called, &datum };\n"
                           "void caller(void) { called(); }\n", encoding="utf-8")
            for triple, arm64, call, data in (("arm64-apple-macos12.0", True, "ARM64_RELOC_BRANCH26", "ARM64_RELOC_UNSIGNED"),
                                              ("x86_64-apple-macos12.0", False, "X86_64_RELOC_BRANCH", "X86_64_RELOC_UNSIGNED")):
                with self.subTest(triple=triple):
                    gen_port.Tools.arm64 = arm64
                    obj = pathlib.Path(tmp) / (triple + ".o")
                    subprocess.run([cc, "--target=" + triple, "-c", "-O1", str(src), "-o", str(obj)], check=True, capture_output=True)
                    self.assertEqual(gen_port.reloc_kinds([obj], {"called", "listed", "datum"}),
                                     {"called": {call, data}, "listed": {data}, "datum": {data}})

    def test_real_objects_report_unprefixed_labels(self):
        cc, nm = shutil.which("clang"), shutil.which("llvm-nm")
        if not cc or not nm:
            self.skipTest("clang/llvm-nm unavailable")
        saved_nm = gen_port.Tools.nm
        self.addCleanup(setattr, gen_port.Tools, "nm", saved_nm)
        gen_port.Tools.nm = nm
        with tempfile.TemporaryDirectory() as tmp:
            src = pathlib.Path(tmp) / "unit.c"
            src.write_text('extern int lab(int) __asm__("func_003df4d0");\nextern int ext(int);\n'
                           'int caller(int x) { return lab(x) + ext(x); }\n', encoding="utf-8")
            for triple in ("arm64-apple-macos12.0", "x86_64-apple-macos12.0"):
                with self.subTest(triple=triple):
                    obj = pathlib.Path(tmp) / (triple + ".o")
                    subprocess.run([cc, "--target=" + triple, "-c", "-O1", str(src), "-o", str(obj)], check=True, capture_output=True)
                    defined, undefined, functions = gen_port.nm_symbols([obj])
                    self.assertEqual(defined, {"caller"})
                    self.assertEqual(undefined, {"func_003df4d0", "ext"})
                    self.assertEqual(functions, {"caller"})
                    self.assertEqual(gen_port.Tools.raw_undefined, {"func_003df4d0"})

    def test_rewriter_prefix_and_i386_oracle(self):
        sdk = ["-isysroot", "/sdk"]
        self.assertEqual(gptr_rewrite.symbol_prefix(["--target=arm64-apple-macos12.0", *sdk]), "_")
        self.assertEqual(gptr_rewrite.symbol_prefix(["--target=x86_64-apple-macos12.0"]), "_")
        self.assertEqual(gptr_rewrite.symbol_prefix(["--target=x86_64-linux-gnu", "-m64"]), "")
        self.assertEqual(gptr_rewrite.symbol_prefix(["--target=x86_64-w64-mingw32"]), "")
        self.assertEqual(unit64.default_t32(["--target=arm64-apple-macos12.0", *sdk, "-fsigned-char"]),
                         ["--target=i686-apple-macos10.13", *sdk, "-fno-blocks"])
        self.assertEqual(unit64.default_t32(["--target=aarch64-linux-gnu"]), ["--target=i386-linux-gnu"])
        mach = gptr_rewrite.Rewriter({"inner": []}, "/root", sym_prefix="_")
        elf = gptr_rewrite.Rewriter({"inner": []}, "/root")
        self.assertEqual(mach.linked_name({"mangledName": "_RwMatrixCreate"}, "RwMatrixCreate"), "RwMatrixCreate")
        self.assertEqual(mach.linked_name({"mangledName": "func_003df4d0"}, "p4c_cos"), "func_003df4d0")
        self.assertEqual(mach.linked_name({}, "plain"), "plain")
        self.assertEqual(elf.linked_name({"mangledName": "_leading"}, "_leading"), "_leading")

    def test_macho_text_prefixes_declaration_labels_only(self):
        source = ('extern int lab(int) __asm__("func_003df4d0");\n'
                  'extern unsigned char d[] __asm__ ("D_008873E8");\n'
                  'extern int two(int)\n    __asm__("func_003df590");\n'
                  'extern int sdk(void) __asm("_" "fopen" "$DARWIN_EXTSN");\n'
                  'extern int done(void) __asm__("_already");\n'
                  'extern struct S { int a; } gp __attribute__((section(".sdata")));\n'
                  'void f(void) { __asm__ volatile("nop"); ; __asm__("sync"); }\n'
                  '__asm__(".globl x");\n')
        with tempfile.TemporaryDirectory() as tmp:
            path = pathlib.Path(tmp) / "unit.i"
            path.write_bytes(source.encode("latin-1"))
            gptr_rewrite.macho_text(str(path), "_")
            text = path.read_bytes().decode("latin-1")
        self.assertIn('__asm__("_func_003df4d0")', text)
        self.assertIn('__asm__ ("_D_008873E8")', text)
        self.assertIn('__asm__("_func_003df590")', text)
        self.assertIn('__asm("_" "fopen" "$DARWIN_EXTSN")', text)
        self.assertIn('__asm__("_already")', text)
        self.assertNotIn("section", text)
        self.assertIn('__asm__ volatile("nop")', text)
        self.assertIn('; __asm__("sync")', text)
        self.assertIn('__asm__(".globl x")', text)


class OracleLayoutTest(unittest.TestCase):
    def layouts_with_cold_and_warm_cache(self, dump):
        def fake_clang(argv, **kwargs):
            if "-E" in argv:
                pathlib.Path(argv[argv.index("-o") + 1]).write_text("preprocessed fixture\n")
            return mock.Mock(stdout=dump, stderr="", returncode=0)

        with mock.patch.object(gptr_rewrite.subprocess, "run", side_effect=fake_clang) as clang:
            cold = gptr_rewrite.oracle_layouts(self.id() + ".c", [])
            warm = gptr_rewrite.oracle_layouts(self.id() + ".c", [])
        self.assertEqual(cold, warm)
        self.assertEqual(sum("-E" not in call.args[0] for call in clang.call_args_list), 1)
        return cold

    def test_anonymous_typedef_roots_are_concrete_records(self):
        dump = """*** Dumping AST Record Layout
         0 | struct P4Q128
         0 |   uint32_t[4] w
           | [sizeof=16, align=16]
*** Dumping AST Record Layout
         0 | FclDrawColor
         0 |   u8 c0
         1 |   u8 c1
         2 |   u8 c2
         3 |   u8 c3
           | [sizeof=4, align=1]
*** Dumping AST Record Layout
         0 | FclVec2
         0 |   f32 x
         4 |   f32 y
           | [sizeof=8, align=4]
"""
        layouts = self.layouts_with_cold_and_warm_cache(dump)
        self.assertEqual(layouts["FclDrawColor"],
                         [{"size": 4, "fields": [(0, "c0"), (1, "c1"), (2, "c2"), (3, "c3")]}])
        self.assertEqual(layouts["FclVec2"],
                         [{"size": 8, "fields": [(0, "x"), (4, "y")]}])
        self.assertEqual(layouts["struct P4Q128"], [{"size": 16, "fields": [(0, "w")]}])

    def test_named_fields_of_anonymous_record_types_are_retained(self):
        dump = """*** Dumping AST Record Layout
         0 | FclBoundsPacket
         0 |   struct (unnamed at fixture.h:17:5) dimensions
         0 |     u8[8] reserved
         8 |     s32 width
        12 |     s32 height
         0 |   FclBoundsBytes representation
         0 |     u8[16] bytes
           | [sizeof=16, align=4]
"""
        layouts = self.layouts_with_cold_and_warm_cache(dump)
        self.assertEqual(layouts["FclBoundsPacket"],
                         [{"size": 16, "fields": [(0, "dimensions"), (0, "representation")]}])

    def test_explicit_union_tag_resolves_without_a_typedef(self):
        record = {"kind": "RecordDecl", "id": "color", "tagUsed": "union",
                  "name": "ColorWord", "completeDefinition": True}
        reader = object.__new__(gptr_rewrite.Rewriter)
        reader.decl_by_id = {"color": record}
        reader.typedef_ids = {}
        self.assertIs(reader.find_record({"type": {"qualType": "union ColorWord"}}), record)
        self.assertIsNone(reader.find_record({"type": {"qualType": "union ColorWord *"}}))


class AggregateCallShapesTest(unittest.TestCase):
    vector = {"kind": "record", "bytes": 12, "class": "G", "chunks": "FF", "hfa": (4, 3)}
    color = {"kind": "record", "bytes": 4, "class": "G", "chunks": "G", "hfa": None}
    integer = {"kind": "int", "bytes": 4, "class": "G"}
    pointer = {"kind": "ptr", "bytes": 4, "class": "G"}

    def check_call(self, arguments, shape=None, abi=None, definitions=None, local=None):
        row = {"unit": "src/unit.c", "linked": "target", "callee": "target"}
        shape = self.vector if shape is None else shape
        abi = {"params": [None]} if abi is None else abi
        definitions = {"target": [[shape]]} if definitions is None else definitions
        return audit_prototypes.finish_aggregate_calls(
            [(row, abi, arguments, [False] * len(arguments))], definitions, local or {})

    def test_pointer_is_not_a_by_value_float_vector(self):
        counts, findings, unresolved = self.check_call([self.pointer])
        self.assertEqual(counts["aggregate_calls_checked"], 1)
        self.assertEqual(findings[0]["kind"], "aggregate-abi")
        self.assertEqual(findings[0]["aggregate_positions"], [0])
        self.assertEqual(unresolved, [])

    def test_whole_matching_vector_is_preserved(self):
        counts, findings, unresolved = self.check_call([self.vector])
        self.assertEqual(counts["checked"], 1)
        self.assertEqual((findings, unresolved), ([], []))

    def test_packed_integer_color_has_the_same_gp_bits(self):
        counts, findings, unresolved = self.check_call([self.integer], shape=self.color)
        self.assertEqual(counts["checked"], 1)
        self.assertEqual((findings, unresolved), ([], []))

    def test_float_record_cannot_be_an_integer_color(self):
        floating = dict(self.color, chunks="F", hfa=(4, 1))
        self.assertFalse(audit_prototypes.aggregate_compatible(self.integer, floating))

    def test_different_hfa_register_widths_do_not_match(self):
        first = dict(self.vector, bytes=16, hfa=(4, 4))
        second = dict(first, hfa=(8, 2))
        self.assertFalse(audit_prototypes.aggregate_compatible(first, second))

    def test_unaligned_record_does_not_match_a_gp_integer(self):
        packed = dict(self.color, unaligned=True)
        self.assertFalse(audit_prototypes.aggregate_compatible(self.integer, packed))

    def test_unaligned_record_does_not_match_an_aligned_record(self):
        packed = dict(self.color, unaligned=True)
        self.assertFalse(audit_prototypes.aggregate_compatible(packed, self.color))

    def test_unknown_layout_is_reported_not_counted_clean(self):
        counts, findings, unresolved = self.check_call(
            [self.vector], definitions={"target": [[None]]})
        self.assertEqual(counts["checked"], 0)
        self.assertEqual(counts["parameter_class_unresolved"], 1)
        self.assertEqual(findings, [])
        self.assertEqual(len(unresolved), 1)

    def test_local_definition_is_authoritative(self):
        counts, findings, unresolved = self.check_call(
            [self.vector], definitions={"target": [[self.color]]},
            local={("src/unit.c", "target"): [[self.vector]]})
        self.assertEqual(counts["checked"], 1)
        self.assertEqual((findings, unresolved), ([], []))

    def test_conflicting_global_definitions_remain_explicit(self):
        counts, findings, unresolved = self.check_call(
            [self.vector], definitions={"target": [[self.vector], [self.color]]})
        self.assertEqual(counts["checked"], 0)
        self.assertEqual(findings, [])
        self.assertEqual(len(unresolved), 1)

    def test_variadic_tail_does_not_hide_a_fixed_record_check(self):
        counts, findings, unresolved = self.check_call(
            [self.vector, None], abi={"params": [None], "va": "other"})
        self.assertEqual(counts["variadic_fixed_parameters"], 1)
        self.assertEqual((findings, unresolved), ([], []))

    def test_missing_aggregate_argument_is_an_underpass(self):
        _, findings, unresolved = self.check_call([])
        self.assertEqual(findings[0]["kind"], "under-passing")
        self.assertEqual(findings[0]["defined"], 1)
        self.assertEqual(unresolved, [])


class CensusConfigureTest(unittest.TestCase):
    def configure_guard(self, enabled, has_baseline, android=False):
        project = pathlib.Path(__file__).resolve().parents[1]
        source = (project / "cmake/sources/080-port64-profile.cmake").read_text(encoding="utf-8")
        start = source.index("function(p4_census_check ")
        end = source.index("endfunction()", start) + len("endfunction()")
        with tempfile.TemporaryDirectory() as tmp:
            root = pathlib.Path(tmp)
            baseline = root / ("config/port64android_aarch64_census.txt" if android else "reviewed.txt")
            selection = 'set(P4_CENSUS_BASELINE "${CMAKE_CURRENT_SOURCE_DIR}/reviewed.txt")\n'
            if android:
                selection_start = source.index("set(P4_CENSUS_BASELINE)")
                selection_end = source.index("if(P4_PORT64 AND NOT P4_WIN32", selection_start)
                selection = (
                    "set(P4_ANDROID64 TRUE)\nset(ANDROID_ABI arm64-v8a)\n" +
                    source[selection_start:selection_end])
            if has_baseline:
                baseline.parent.mkdir(parents=True, exist_ok=True)
                baseline.write_text("# reviewed fixture\n", encoding="utf-8")
            (root / "CMakeLists.txt").write_text(
                "cmake_minimum_required(VERSION 3.20)\nproject(census_guard NONE)\n" +
                source[start:end] + "\n" +
                f"set(P4_CENSUS_CHECK {enabled})\nset(P4_PORT64_ARCH x86_64)\n" +
                selection +
                'p4_census_check(stamp "${CMAKE_CURRENT_SOURCE_DIR}/objects/units" '
                '"${P4_CENSUS_BASELINE}" "src/unit.c")\n' +
                'message(STATUS "census_guard_stamp=${stamp}")\n',
                encoding="utf-8")
            result = subprocess.run(
                ["cmake", "-S", str(root), "-B", str(root / "build")],
                capture_output=True, text=True)
            active = root / "objects/active_units.txt"
            return result, active.read_text(encoding="utf-8") if active.exists() else None

    def test_missing_baseline_is_fatal(self):
        result, active = self.configure_guard("ON", False)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("no reviewed census baseline", result.stderr)
        self.assertEqual(active, "src/unit.c\n")

    def test_experimental_switch_is_explicit_and_keeps_active_units(self):
        result, active = self.configure_guard("OFF", False)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertIn("P4_CENSUS_CHECK=OFF", result.stderr)
        self.assertEqual(active, "src/unit.c\n")
        self.assertIn("census_guard_stamp=\n", result.stdout)

    def test_reviewed_baseline_enables_the_build_check(self):
        result, active = self.configure_guard("ON", True)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertEqual(active, "src/unit.c\n")
        self.assertRegex(result.stdout, r"census_guard_stamp=.+/census\.ok")

    def test_android_uses_ndk_abi_despite_desktop_architecture_default(self):
        result, active = self.configure_guard("ON", True, android=True)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertEqual(active, "src/unit.c\n")
        self.assertRegex(result.stdout, r"census_guard_stamp=.+/census\.ok")
class FunctionPointerWordsTest(unittest.TestCase):
    def setUp(self):
        # Same alignment as the retail floor label at 0x005f41b9. Its final
        # aligned word is b" 6F\0", also the retail address of tmxConvertRgb24.
        self.data = b"\0Yukiko's Castle 6F\0" + struct.pack("<II", 0x00463620, 0x00123450)
        self.base = 0x1000
        self.starts = {0x00463620, 0x00123450}

    def test_32bit_keeps_literal_but_rewrites_real_callbacks(self):
        self.assertEqual(
            list(gen_port.function_pointer_words(gen_port.pointer_word_rows(self.data, self.base), self.starts, True)),
            [(0x1014, 0x00463620), (0x1018, 0x00123450)],
        )

    def test_64bit_registration_scan_remains_unchanged(self):
        self.assertEqual(
            list(gen_port.function_pointer_words(gen_port.pointer_word_rows(self.data, self.base), self.starts, False)),
            [(0x1010, 0x00463620), (0x1014, 0x00463620), (0x1018, 0x00123450)],
        )

    def test_short_printable_callback_is_not_a_literal(self):
        data = struct.pack("<III", 0x00463620, 0x00463620, 0x00123450)
        self.assertEqual(
            list(gen_port.function_pointer_words(gen_port.pointer_word_rows(data, self.base), self.starts, True)),
            [(0x1000, 0x00463620), (0x1004, 0x00463620), (0x1008, 0x00123450)],
        )

    def test_unterminated_printable_bytes_are_not_classified_as_a_string(self):
        data = b"ABCD" + struct.pack("<I", 0x00123450)
        self.assertEqual(
            list(gen_port.function_pointer_words(gen_port.pointer_word_rows(data, self.base), self.starts, True)),
            [(0x1004, 0x00123450)],
        )

    def test_android_layout_oracle_replaces_only_the_architecture_prefix(self):
        project = pathlib.Path(__file__).resolve().parents[1]
        source = (project / "cmake/sources/080-port64-profile.cmake").read_text(encoding="utf-8")
        derivation = next(line for line in source.splitlines()
                          if "REGEX REPLACE" in line and "P4_X32_TRIPLE" in line)
        statements = ["cmake_minimum_required(VERSION 3.20)"]
        for native, oracle in (
                ("aarch64-none-linux-android24", "i686-none-linux-android24"),
                ("x86_64-none-linux-android24", "i686-none-linux-android24"),
                ("aarch64-linux-android26", "i686-linux-android26")):
            statements.extend((
                f'set(P4_X64_TRIPLE "{native}")',
                derivation,
                f'if(NOT P4_X32_TRIPLE STREQUAL "{oracle}")',
                '  message(FATAL_ERROR "incorrect Android layout oracle: ${P4_X32_TRIPLE}")',
                "endif()"))
        with tempfile.TemporaryDirectory() as tmp:
            script = pathlib.Path(tmp) / "oracle_target.cmake"
            script.write_text("\n".join(statements) + "\n", encoding="utf-8")
            result = subprocess.run(["cmake", "-P", str(script)], capture_output=True, text=True)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)


class ObjectChunksTest(unittest.TestCase):
    """nm and objdump get the objects in groups that fit a Windows command line (32,767 characters)."""

    def test_groups_keep_order_and_stay_within_both_bounds(self):
        objs = [pathlib.PurePosixPath("E:/lanes/x/build/win32/CMakeFiles/p4decomp.dir/gen/decomp/src/cri/unit%04d.c.obj" % i)
                for i in range(1200)]
        groups = list(gen_port.object_chunks(objs))
        self.assertEqual([path for group in groups for path in group], [str(o) for o in objs])
        for group in groups:
            self.assertLessEqual(len(group), 400)
            self.assertLessEqual(sum(len(path) + 1 for path in group), 24000)
        self.assertGreater(len(groups), 3)      # 400 of these paths alone are over 32,000 characters

    def test_short_paths_still_group_by_count_and_empty_input_makes_no_command(self):
        self.assertEqual([len(g) for g in gen_port.object_chunks(["a.o"] * 1000)], [400, 400, 200])
        self.assertEqual(list(gen_port.object_chunks([])), [])

    def test_a_single_overlong_path_is_still_sent_alone(self):
        self.assertEqual(list(gen_port.object_chunks(["x" * 30000, "b.o"])), [["x" * 30000], ["b.o"]])


if __name__ == "__main__":
    unittest.main()

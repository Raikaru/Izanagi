#!/usr/bin/env python3
"""Exact build-output keys, immutable bytes, process locks and real Clang reuse."""
import contextlib
import hashlib
import io
import json
import multiprocessing
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tarfile
import tempfile
import unittest
from unittest import mock

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
sys.path.insert(0, str(ROOT / "tools/port64"))
import build_cache as cache
import gen_port
import gptr_rewrite as gr
import unit64


def cache_worker(counter, ready, release, result, arriving=None):
    """A real process owns the cache's OS lock, including during forced death."""
    def produce():
        with open(counter, "ab") as output:
            output.write(b"compute\n")
        ready.set()
        if not release.wait(30):
            raise RuntimeError("fixture producer was not released")
        return b"immutable artifact\x00\xff"
    if arriving is not None:
        arriving.set()
    data = cache.run_bytes("process-fixture", {"source": "same exact input"}, produce)
    Path(result).write_bytes(data)


class CacheFixture(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory(prefix="p4pc-build-cache-")
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        self.directory = self.root / "cache"
        self.environment = mock.patch.dict(os.environ, {
            "P4PC_BUILD_CACHE_DIR": str(self.directory),
            "P4PC_BUILD_CACHE_DISABLE": "0",
            "P4PC_BUILD_PROFILE": "fixture",
            "P4PC_BUILD_TOOLCHAIN_ID": "sha256:" + "1" * 64,
            "P4PC_BUILD_TRACE_DIR": "",
            "P4PC_BUILD_UNIT": "",
        })
        self.environment.start()
        self.addCleanup(self.environment.stop)

    def entries(self):
        return sorted(self.directory.glob("entries/*/*"))


class CacheTest(CacheFixture):
    def test_exact_inputs_hit_and_changed_inputs_miss(self):
        calls = []
        def produce():
            calls.append(1)
            return b"bytes\x00\xff"
        request = {"preprocessed": "source hash", "flags": ["-O2"],
                   "toolchain": "compiler hash", "generator": "version hash"}
        self.assertEqual(cache.run_bytes("unit", request, produce), b"bytes\x00\xff")
        self.assertEqual(cache.run_bytes("unit", dict(reversed(list(request.items()))), produce), b"bytes\x00\xff")
        self.assertEqual(len(calls), 1)
        for field in request:
            changed = dict(request)
            changed[field] = ["-O0"] if field == "flags" else "changed " + request[field]
            cache.run_bytes("unit", changed, produce)
        cache.run_bytes("another-pass", request, produce)
        with mock.patch.dict(os.environ, P4PC_BUILD_PROFILE="another-profile"):
            cache.run_bytes("unit", request, produce)
        self.assertEqual(len(calls), 7)
        self.assertEqual(len(self.entries()), 7)

    def test_disabled_path_does_not_evaluate_keys_or_create_cache(self):
        calls = []
        def key():
            raise AssertionError("disabled cache evaluated its key")
        def produce():
            calls.append(1)
            return b"reference bytes"
        with mock.patch.dict(os.environ, P4PC_BUILD_CACHE_DISABLE="1"):
            self.assertEqual(cache.run_bytes("unit", key, produce), b"reference bytes")
            self.assertEqual(cache.run_bytes("unit", key, produce), b"reference bytes")
        self.assertEqual(len(calls), 2)
        self.assertFalse(self.directory.exists())

    def test_failures_and_nonbytes_are_not_published(self):
        def failure():
            raise RuntimeError("actual producer failure")
        with self.assertRaisesRegex(RuntimeError, "actual producer failure"):
            cache.run_bytes("unit", {}, failure)
        with self.assertRaisesRegex(TypeError, "must return bytes"):
            cache.run_bytes("unit", {}, lambda: "not bytes")
        self.assertEqual(self.entries(), [])
        self.assertEqual(cache.run_bytes("unit", {}, lambda: b"recovered"), b"recovered")

    def test_every_mutation_fails_without_recomputation_or_repair(self):
        for mutation in ("payload", "manifest", "key", "schema", "missing", "extra", "directory"):
            with self.subTest(mutation=mutation):
                with mock.patch.dict(os.environ, P4PC_BUILD_PROFILE=mutation):
                    previous = set(self.entries())
                    cache.run_bytes("unit", {}, lambda: b"original bytes")
                    entry, = set(self.entries()) - previous
                    manifest = entry / "manifest.json"
                    payload = entry / "payload.z"
                    if mutation == "payload":
                        payload.write_bytes(b"changed compressed bytes")
                    elif mutation == "manifest":
                        manifest.write_bytes(b"{invalid JSON")
                    elif mutation in ("key", "schema"):
                        value = json.loads(manifest.read_bytes())
                        value[mutation] = "wrong" if mutation == "key" else 99
                        manifest.write_text(json.dumps(value), encoding="utf-8")
                    elif mutation == "missing":
                        payload.unlink()
                    elif mutation == "extra":
                        (entry / "unexpected").write_bytes(b"do not discard")
                    else:
                        payload.unlink()
                        payload.mkdir()
                    before = {p.name: p.read_bytes() if p.is_file() else None for p in entry.iterdir()}
                    def forbidden():
                        raise AssertionError("corruption was silently regenerated")
                    with self.assertRaisesRegex(cache.CacheCorruption, "entry retained, never regenerated"):
                        cache.run_bytes("unit", {}, forbidden)
                    after = {p.name: p.read_bytes() if p.is_file() else None for p in entry.iterdir()}
                    self.assertEqual(after, before)

    def test_entry_appearing_under_lock_is_not_overwritten(self):
        appeared = []
        def produce():
            key = next((self.directory / "locks").glob("*.lock")).stem
            entry = self.directory / "entries" / key[:2] / key
            entry.mkdir(parents=True)
            appeared.append(entry)
            return b"must not replace existing directory"
        with self.assertRaisesRegex(cache.CacheCorruption, "appeared under its exclusive lock"):
            cache.run_bytes("unit", {}, produce)
        self.assertTrue(appeared[0].is_dir())
        self.assertEqual(list(appeared[0].iterdir()), [])
        self.assertEqual(self.entries(), appeared)

    def test_compiler_identity_requires_an_immutable_toolchain(self):
        for identity in ("", "p4pc-ci:latest", "sha256:" + "x" * 64, "sha256:123"):
            with self.subTest(identity=identity), mock.patch.dict(os.environ, P4PC_BUILD_TOOLCHAIN_ID=identity):
                with self.assertRaisesRegex(RuntimeError, "immutable CI image"):
                    cache.compiler_identity(sys.executable)
        identity = cache.compiler_identity(sys.executable)
        self.assertEqual(identity["image"], "sha256:" + "1" * 64)
        self.assertEqual(identity["compiler"], str(Path(sys.executable).resolve()))

    def test_file_set_keys_cover_added_removed_and_changed_inputs(self):
        one, two = self.root / "one", self.root / "two"
        one.write_bytes(b"one")
        before = cache.input_files([one, two], root=self.root)
        self.assertIsNone(before["two"])
        two.write_bytes(b"two")
        self.assertNotEqual(cache.input_files([one, two], root=self.root), before)
        one.write_bytes(b"changed")
        self.assertNotEqual(cache.input_files([one], root=self.root)["one"], before["one"])
        one.unlink()
        self.assertIsNone(cache.input_files([one], root=self.root)["one"])

    def test_file_set_keys_preserve_distinct_names_for_the_same_link_target(self):
        target, first, renamed = (self.root / name for name in ("target.h", "first.h", "renamed.h"))
        target.write_bytes(b"typedef int GuestWord;\n")
        try:
            first.symlink_to(target)
            renamed.symlink_to(target)
        except (NotImplementedError, OSError) as error:
            self.skipTest("file symlinks are unavailable: " + str(error))
        before = cache.input_files([target, first], root=self.root)
        after = cache.input_files([target, renamed], root=self.root)
        self.assertEqual(set(before), {"target.h", "first.h"})
        self.assertEqual(set(after), {"target.h", "renamed.h"})
        self.assertEqual(before["first.h"], before["target.h"])
        self.assertEqual(after["renamed.h"], before["first.h"])
        self.assertNotEqual(before, after)

    def test_concurrent_processes_publish_one_producer(self):
        context = multiprocessing.get_context("spawn")
        first_ready, second_ready, release = context.Event(), context.Event(), context.Event()
        second_arriving = context.Event()
        counter = self.root / "producer-count"
        processes = [context.Process(target=cache_worker, args=(
            str(counter), ready, release, str(self.root / ("result-%d" % index)),
            second_arriving if index else None))
            for index, ready in enumerate((first_ready, second_ready))]
        try:
            processes[0].start()
            self.assertTrue(first_ready.wait(30), "first cache producer did not acquire its OS lock")
            processes[1].start()
            self.assertTrue(second_arriving.wait(30), "second process never reached the contended cache")
            release.set()
            for process in processes:
                process.join(30)
                self.assertEqual(process.exitcode, 0)
            self.assertEqual(counter.read_bytes(), b"compute\n")
            self.assertEqual((self.root / "result-0").read_bytes(), (self.root / "result-1").read_bytes())
            self.assertEqual(len(self.entries()), 1)
        finally:
            release.set()
            for process in processes:
                if process.pid and process.is_alive():
                    process.terminate()
                    process.join(30)

    def test_killed_producer_releases_its_os_lock_without_a_stale_pid_rule(self):
        context = multiprocessing.get_context("spawn")
        ready, release = context.Event(), context.Event()
        process = context.Process(target=cache_worker, args=(
            str(self.root / "producer-count"), ready, release, str(self.root / "unused-result")))
        try:
            process.start()
            self.assertTrue(ready.wait(30), "fixture producer did not own its OS lock")
            process.terminate()
            process.join(30)
            self.assertIsNotNone(process.exitcode)
            self.assertEqual(self.entries(), [])
            self.assertEqual(cache.run_bytes("process-fixture", {"source": "same exact input"},
                                              lambda: b"surviving producer"), b"surviving producer")
        finally:
            # A terminated worker may own the Event's non-robust condition lock.
            # Never signal synchronization objects belonging to a killed process.
            if process.pid and process.is_alive():
                process.terminate()
                process.join(30)

    def test_nested_trace_has_real_rusage_and_preserves_failed_cli_streams(self):
        trace_root = self.root / "trace"
        with mock.patch.dict(os.environ, P4PC_BUILD_TRACE_DIR=str(trace_root)):
            with cache.timed("outer"):
                with cache.timed("inner"):
                    sum(range(1000))
            command = [sys.executable, str(ROOT / "tools/build_cache.py"), "run", "fixture.child", "--",
                       sys.executable, "-c",
                       "import sys;sys.stdout.buffer.write(b'output\\x00\\n');sys.stderr.buffer.write(b'error\\n');sys.exit(7)"]
            result = subprocess.run(command, capture_output=True)
        self.assertEqual((result.returncode, result.stdout, result.stderr), (7, b"output\x00\n", b"error\n"))
        records = [json.loads(line) for path in trace_root.glob("*.jsonl") for line in path.read_text().splitlines()]
        by_name = {row["pass"]: row for row in records}
        self.assertEqual(by_name["inner"]["parent_interval"], by_name["outer"]["id"])
        self.assertIsNone(by_name["outer"]["parent_interval"])
        self.assertEqual(by_name["fixture.child"]["details"]["returncode"], 7)
        self.assertEqual(by_name["fixture.child"]["outcome"], "failed")
        for row in records:
            self.assertGreaterEqual(row["wall_seconds"], 0)
            self.assertIn("inclusive", row["accounting"])
            for value in row["rusage"].values():
                if value is not None:
                    self.assertGreaterEqual(value, 0)

    def test_real_profile_helper_records_shared_compiler_identity_and_statistics(self):
        shell = os.environ.get("P4PC_BASH") or shutil.which("bash")
        if not shell:
            self.skipTest("Bash is unavailable")
        script = ('. "$1"\np4pc_build_cache_profile windows-x86_64 "$2" || exit $?\n'
                  'printf "RESULT_profile=%s\\nRESULT_namespace=%s\\nRESULT_compiler=%s\\nRESULT_stats=%s\\nRESULT_epoch=%s\\n" '
                  '"$P4PC_BUILD_PROFILE" "$CCACHE_NAMESPACE" "$CCACHE_COMPILERCHECK" "$CCACHE_STATSLOG" "$SOURCE_DATE_EPOCH"\n')
        for epoch in ("", "123456789"):
            with self.subTest(epoch=epoch):
                trace = self.root / ("trace-" + (epoch or "empty"))
                env = dict(os.environ, P4PC_BUILD_TRACE_DIR="", SOURCE_DATE_EPOCH=epoch,
                           P4PC_BUILD_TOOLCHAIN_ID="fixture-exact-toolchain-identity")
                result = subprocess.run([shell, "-c", script, "--", (ROOT / "tools/build_cache.sh").as_posix(),
                                         trace.as_posix()], env=env, capture_output=True, text=True, timeout=20)
                self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
                values = dict(line[len("RESULT_"):].split("=", 1)
                              for line in result.stdout.splitlines() if line.startswith("RESULT_"))
                self.assertEqual(values["profile"], "windows-x86_64")
                self.assertEqual(values["namespace"], "p4pc-fixture-exact-toolchain-identity-windows-x86_64")
                self.assertEqual(values["compiler"], "content")
                self.assertEqual(values["epoch"], "0")
                self.assertEqual(Path(values["stats"]).parent, trace)
                self.assertRegex(Path(values["stats"]).name, r"^ccache-windows-x86_64-\d+\.log$")
                self.assertTrue(trace.is_dir())


class MirrorCacheTest(CacheFixture):
    def setUp(self):
        super().setUp()
        (self.root / "config").mkdir()
        self.contracts = self.root / "config/mirror_contracts.json"
        self.build = self.root / "generated"
        self.mirror = self.build / "decomp"
        (self.mirror / "src").mkdir(parents=True)
        (self.mirror / "include").mkdir()
        self.unit = self.mirror / "src/unit.c"
        self.header = self.mirror / "include/types.h"
        self.unit.write_bytes(b"// original byte \xff\r\nint one(void) { return 1; }\r\n")
        self.header.write_bytes(b"typedef unsigned int Word;\n")
        (self.build / "asm_blocks.txt").write_bytes(b"src/unit.c|fixture\n")
        (self.build / "prototype_report.txt").write_bytes(b"reviewed prototype report\n")
        (self.build / "mod_bridges.c").write_bytes(b"/* bridge output */\n")
        (self.build / "mod_hooks.tsv").write_bytes(b"address\tname\n")
        patch = mock.patch.object(gen_port, "ROOT", self.root)
        patch.start()
        self.addCleanup(patch.stop)

    def test_per_unit_contract_cache_reuses_exact_diff_but_checks_current_review(self):
        old = b"int one(void) { return 1; }\n"
        new = b"int one(void) { return 2; }\n"
        facts = {"reviewed-pass": {"src/unit.c": {
            "input_sha256": cache.digest(old),
            "hunks": [[1, 1, cache.digest(old), cache.digest(new)]]}}}
        self.contracts.write_text(json.dumps(facts))
        def transform():
            self.unit.write_bytes(new)
            return "actual transformation"
        self.unit.write_bytes(old)
        reviewed = gen_port.MirrorContracts(self.mirror)
        self.assertEqual(reviewed.run(transform, stage_name="reviewed-pass"), "actual transformation")
        reviewed.finish()
        self.assertEqual(reviewed.actual, facts)
        self.unit.write_bytes(old)
        reused = gen_port.MirrorContracts(self.mirror)
        with mock.patch.object(gen_port.MirrorContracts, "_edit_facts",
                               side_effect=AssertionError("unchanged unit's expensive diff recomputed")):
            reused.run(transform, stage_name="reviewed-pass")
            reused.finish()
        self.assertEqual(reused.actual, facts)
        self.contracts.write_text(json.dumps({"reviewed-pass": {}}))
        self.unit.write_bytes(old)
        drifted = gen_port.MirrorContracts(self.mirror)
        with contextlib.redirect_stderr(io.StringIO()) as errors:
            drifted.run(transform, stage_name="reviewed-pass")
            with self.assertRaisesRegex(SystemExit, "reviewed matches"):
                drifted.finish()
        self.assertIn("changed match census", errors.getvalue())
        self.assertEqual(json.loads(self.contracts.read_text()), {"reviewed-pass": {}})

    def test_cached_mirror_restores_exact_bytes_reports_diagnostics_and_deletions(self):
        facts = {"reviewed-pass": {}}
        self.contracts.write_text(json.dumps(facts))
        expected = {path.relative_to(self.build): path.read_bytes()
                    for path in self.build.rglob("*") if path.is_file()}
        header_mtime = self.header.stat().st_mtime_ns
        payload = cache.run_bytes("mirror", {"fixture": 1},
                                  lambda: gen_port.pack_mirror(self.build, facts, "original stdout\n", "original warning\n"))
        self.unit.write_bytes(b"wrong current bytes")
        (self.mirror / "src/stale.c").write_bytes(b"must be removed")
        (self.build / "asm_blocks.txt").write_bytes(b"wrong report")
        (self.build / "mod_bridges.c").unlink()
        (self.build / "mod_hooks.tsv").write_bytes(b"stale hooks")
        def forbidden():
            raise AssertionError("whole mirror recomputed on a cache hit")
        reused = cache.run_bytes("mirror", {"fixture": 1}, forbidden)
        self.assertEqual(reused, payload)
        with contextlib.redirect_stdout(io.StringIO()) as stdout, contextlib.redirect_stderr(io.StringIO()) as stderr:
            gen_port.restore_mirror(self.build, reused)
        actual = {path.relative_to(self.build): path.read_bytes()
                  for path in self.build.rglob("*") if path.is_file()}
        self.assertEqual(actual, expected)
        self.assertEqual(self.header.stat().st_mtime_ns, header_mtime)
        self.assertEqual((stdout.getvalue(), stderr.getvalue()), ("original stdout\n", "original warning\n"))

    def test_mirror_key_tracks_hook_mode_and_codegen_source(self):
        decomp = self.root / "decomp"
        decomp.mkdir()
        tools = self.root / "tools"
        tools.mkdir()
        generator = tools / "mod_codegen.py"
        generator.write_text("generator v1\n")
        with mock.patch.object(cache, "compiler_identity", return_value={"image": "fixture"}):
            disabled = gen_port.mirror_cache_inputs(decomp, self.build, False)
            enabled = gen_port.mirror_cache_inputs(decomp, self.build, True)
        self.assertNotEqual(cache.canonical(disabled), cache.canonical(enabled))
        # (keyed by the resolved path, as build_cache.versions records it: macOS's temporary directory is under the /var symlink)
        self.assertEqual(enabled["versions"][str(generator.resolve())], cache.file_digest(generator))

    def test_cached_mirror_missing_bridge_is_rejected(self):
        facts = {"reviewed-pass": {}}
        self.contracts.write_text(json.dumps(facts))
        payload = gen_port.pack_mirror(self.build, facts, "", "")
        incomplete = io.BytesIO()
        with tarfile.open(fileobj=io.BytesIO(payload), mode="r:") as source, \
             tarfile.open(fileobj=incomplete, mode="w") as dest:
            for member in source.getmembers():
                if member.name != "mod_bridges.c":
                    dest.addfile(member, source.extractfile(member))
        with self.assertRaisesRegex(cache.CacheCorruption, "missing"):
            gen_port.restore_mirror(self.build, incomplete.getvalue())

    def test_current_contract_mismatch_rejects_before_publishing_cached_source(self):
        facts = {"reviewed-pass": {}}
        self.contracts.write_text(json.dumps(facts))
        payload = gen_port.pack_mirror(self.build, facts, "", "")
        self.contracts.write_text(json.dumps({"reviewed-pass": {"src/unit.c": {"changed": True}}}))
        self.unit.write_bytes(b"current unpublished bytes")
        with contextlib.redirect_stderr(io.StringIO()), self.assertRaisesRegex(SystemExit, "reviewed matches"):
            gen_port.restore_mirror(self.build, payload)
        self.assertEqual(self.unit.read_bytes(), b"current unpublished bytes")

    def test_unsafe_archive_members_are_rejected_without_writes(self):
        for name, kind in (("/outside", tarfile.REGTYPE), ("decomp/src/../outside", tarfile.REGTYPE),
                           ("decomp/src/link.c", tarfile.SYMTYPE), ("_contracts.json", tarfile.DIRTYPE)):
            with self.subTest(name=name):
                payload = io.BytesIO()
                with tarfile.open(fileobj=payload, mode="w") as archive:
                    member = tarfile.TarInfo(name)
                    member.type = kind
                    member.linkname = "../../outside"
                    archive.addfile(member, io.BytesIO())
                before = self.unit.read_bytes()
                with self.assertRaisesRegex(cache.CacheCorruption, "invalid cached mirror member"):
                    gen_port.restore_mirror(self.build, payload.getvalue())
                self.assertEqual(self.unit.read_bytes(), before)


@unittest.skipUnless(shutil.which("clang"), "requires the real Clang AST/layout reader")
class RealClangCacheTest(CacheFixture):
    def source(self, conditional=False):
        source = self.root / "unit.c"
        if conditional:
            (self.root / "layout32.h").write_text("struct Record { int *value; int pad; int n; };\n")
            (self.root / "layout64.h").write_text("struct Record { int *value; int pad; int n; };\n")
            declarations = '#if __SIZEOF_POINTER__ == 4\n#include "layout32.h"\n#else\n#include "layout64.h"\n#endif\n'
        else:
            declarations = "struct Record { int *value; int n; };\n"
        source.write_text(declarations + "int take(struct Record *p) { return p->n; }\n")
        work = self.root / "work"
        work.mkdir()
        return source, work

    def rewrite(self, source, work):
        return unit64.rewrite_outputs(str(source), ["--target=x86_64-linux-gnu"], str(work),
                                      str(self.root), {}, ["--target=i386-linux-gnu"],
                                      shutil.which("clang"), {}, (str(work / "rewritten.c.d"), str(work / "rewritten.c")))

    def test_real_abi_and_rewrite_hits_match_uncached_bytes_and_facts(self):
        source, work = self.source()
        flags = ["--target=x86_64-linux-gnu"]
        compiler = shutil.which("clang")
        with mock.patch.dict(os.environ, P4PC_BUILD_CACHE_DISABLE="1"):
            reference_abi = gr.collect_abi(str(source), flags, str(work), str(self.root), compiler)
            _, reference = self.rewrite(source, work)
        self.assertIn("take", reference_abi)
        original_run = subprocess.run
        with mock.patch.object(gr.subprocess, "run", wraps=original_run) as executed:
            actual_abi = gr.collect_abi(str(source), flags, str(work), str(self.root), compiler)
            _, actual = self.rewrite(source, work)
        self.assertEqual(actual_abi, reference_abi)
        self.assertEqual(actual, reference)
        self.assertTrue(any("-ast-dump=json" in call.args[0] for call in executed.call_args_list))
        self.assertTrue(any("-fdump-record-layouts" in call.args[0] for call in executed.call_args_list))
        with mock.patch.object(gr.subprocess, "run", wraps=original_run) as executed:
            cached_abi = gr.collect_abi(str(source), flags, str(work), str(self.root), compiler)
            _, cached = self.rewrite(source, work)
        self.assertEqual(cached_abi, reference_abi)
        self.assertEqual(cached, reference)
        self.assertFalse(any("-ast-dump=json" in call.args[0] for call in executed.call_args_list))
        self.assertFalse(any("-fdump-record-layouts" in call.args[0] for call in executed.call_args_list))
        self.assertTrue(any("-E" in call.args[0] for call in executed.call_args_list))

    def test_rewrite_key_holds_only_the_rows_of_the_names_the_unit_spells(self):
        source, work = self.source()
        compiler = shutil.which("clang")
        take = gr.collect_abi(str(source), ["--target=x86_64-linux-gnu"], str(work), str(self.root), compiler)["take"]
        def rewrite(syms, abi):
            with mock.patch.object(gr.Rewriter, "run", autospec=True, side_effect=gr.Rewriter.run) as visited:
                _, output = unit64.rewrite_outputs(str(source), ["--target=x86_64-linux-gnu"], str(work), str(self.root), syms,
                                                   ["--target=i386-linux-gnu"], compiler, abi,
                                                   (str(work / "rewritten.c.d"), str(work / "rewritten.c")))
            return output, visited.called
        syms = {"D_00100000": [0x100000, "data"]}
        reference, computed = rewrite(syms, {"take": take, "elsewhere": take})
        self.assertTrue(computed)
        # rows of names the unit never spells are not part of its key
        output, computed = rewrite({"D_00100004": [0x100004, "data"]}, {"take": take, "elsewhere": dict(take, rt="long")})
        self.assertEqual((output, computed), (reference, False))
        # a row it spells is
        _, computed = rewrite(syms, {"take": dict(take, reviewed=1), "elsewhere": take})
        self.assertTrue(computed)
        # and a computed rewrite may not read a row outside its key
        original_run = gr.Rewriter.run
        def reads_elsewhere(rewriter):
            rewriter.abi.get("elsewhere")
            return original_run(rewriter)
        with mock.patch.object(gr.Rewriter, "run", reads_elsewhere), self.assertRaisesRegex(SystemExit, "elsewhere"):
            rewrite(syms, {"take": dict(take, reviewed=2), "elsewhere": take})

    def test_printf_cache_reuses_exact_bytes_across_temporary_preprocessor_directories(self):
        import ee_float_cc
        source, work = self.source()
        source.write_text('extern int printf(const char *, ...);\n'
                          'int show(void *p) { return printf("%p", p); }\n')
        compiler = shutil.which("clang")
        flags = ["--target=x86_64-linux-gnu", "-std=c11"]
        output = work / "printf.o"
        def prepare():
            normalized, frontend_flags = ee_float_cc.prepare_printf(
                compiler, flags, str(source), str(output), str(self.root), "")
            return Path(normalized).read_bytes(), frontend_flags
        with mock.patch.dict(os.environ, P4PC_BUILD_CACHE_DISABLE="1"):
            reference, reference_flags = prepare()
        original_run = subprocess.run
        with mock.patch.object(gr.subprocess, "run", wraps=original_run) as executed:
            actual, actual_flags = prepare()
        self.assertEqual((actual, actual_flags), (reference, reference_flags))
        self.assertIn("-std=c11", actual_flags)
        self.assertTrue(any("-ast-dump=json" in call.args[0] for call in executed.call_args_list))
        with mock.patch.object(gr.subprocess, "run", wraps=original_run) as executed:
            cached, cached_flags = prepare()
        self.assertEqual((cached, cached_flags), (reference, reference_flags))
        self.assertFalse(any("-ast-dump=json" in call.args[0] for call in executed.call_args_list))
        source.write_text('extern int printf(const char *, ...);\n'
                          'int show(void *p) { return printf("changed %p", p); }\n')
        changed, changed_flags = prepare()
        self.assertNotEqual(changed, cached)
        with mock.patch.dict(os.environ, P4PC_BUILD_CACHE_DISABLE="1"):
            self.assertEqual(prepare(), (changed, changed_flags))

    def test_i386_only_header_invalidates_real_layout_and_rewritten_bytes(self):
        source, work = self.source(conditional=True)
        _, before = self.rewrite(source, work)
        dependencies = (work / "rewritten.c.d").read_text()
        self.assertIn("layout32.h", dependencies)
        self.assertIn("layout64.h", dependencies)
        (self.root / "layout32.h").write_text("struct Record { int *value; int pad[2]; int n; };\n")
        _, after = self.rewrite(source, work)
        self.assertNotEqual(before["text"], after["text"])
        with mock.patch.dict(os.environ, P4PC_BUILD_CACHE_DISABLE="1"):
            _, reference = self.rewrite(source, work)
        self.assertEqual(after, reference)

    @unittest.skipUnless(shutil.which("cmake") and shutil.which("ninja"), "requires CMake and Ninja depfile transformation")
    def test_cmake_ninja_rebuilds_for_i386_only_header_without_sleep_thresholds(self):
        source, work = self.source(conditional=True)
        # A fixed valid starting date avoids platform-specific pre-epoch times.
        # Order mutation after Ninja's persisted restat lower bound below.
        baseline = 1_600_000_000_000_000_000
        for path in (source, self.root / "layout32.h", self.root / "layout64.h"):
            os.utime(path, ns=(baseline, baseline))
        script = Path(__file__).resolve().as_posix()
        compiler = Path(shutil.which("clang")).as_posix()
        python = Path(sys.executable).as_posix()
        (self.root / "CMakeLists.txt").write_text(
            'cmake_minimum_required(VERSION 3.22)\nproject(CacheDependencies NONE)\n'
            'add_custom_command(OUTPUT "${CMAKE_BINARY_DIR}/rewritten.c"\n'
            ' COMMAND "%s" "%s" --rewrite-fixture "${CMAKE_SOURCE_DIR}" "${CMAKE_BINARY_DIR}" "%s"\n'
            ' DEPFILE "${CMAKE_BINARY_DIR}/rewritten.c.d" DEPENDS "${CMAKE_SOURCE_DIR}/unit.c" VERBATIM)\n'
            'add_custom_target(rewrite ALL DEPENDS "${CMAKE_BINARY_DIR}/rewritten.c")\n' % (python, script, compiler))
        def run(*command):
            result = subprocess.run(command, capture_output=True, text=True)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        run(shutil.which("cmake"), "-S", str(self.root), "-B", str(work), "-G", "Ninja")
        run(shutil.which("cmake"), "--build", str(work))
        original = (work / "rewritten.c").read_bytes()
        counter = work / "fixture-invocations"
        self.assertEqual(counter.read_bytes(), b"rewrite\n")
        run(shutil.which("cmake"), "--build", str(work))
        self.assertEqual(counter.read_bytes(), b"rewrite\n", "an unchanged real Ninja graph did not stay clean")
        header = self.root / "layout32.h"
        header.write_text("struct Record { int *value; int pad[2]; int n; };\n")
        changed = max(header.stat().st_mtime_ns,
                      (work / ".ninja_log").stat().st_mtime_ns) + 1_000_000_000
        os.utime(header, ns=(changed, changed))
        run(shutil.which("cmake"), "--build", str(work))
        self.assertEqual(counter.read_bytes(), b"rewrite\nrewrite\n")
        self.assertNotEqual((work / "rewritten.c").read_bytes(), original)
        run(shutil.which("cmake"), "--build", str(work))
        self.assertEqual(counter.read_bytes(), b"rewrite\nrewrite\n")


class ReproducibleLinkTest(unittest.TestCase):
    def policy(self, directory, msvc=False):
        cmake = shutil.which("cmake")
        if not cmake:
            self.skipTest("CMake is unavailable")
        script = directory / "link-policy.cmake"
        script.write_text(
            f'include("{(ROOT / "cmake/reproducible.cmake").as_posix()}")\n'
            f'set(MSVC {"ON" if msvc else "OFF"})\n'
            'p4_pe_reproducible_link_flags(flags)\n'
            'message(STATUS "PE_FLAGS=${flags}")\n', encoding="utf-8")
        result = subprocess.run([cmake, "-P", str(script)], capture_output=True, text=True)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        return next(line.split("=", 1)[1] for line in result.stdout.splitlines()
                    if line.startswith("-- PE_FLAGS=")).split(";")

    def test_actual_policy_selects_the_driver_after_module_inclusion(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            self.assertEqual(self.policy(root), ["-fuse-ld=lld", "-Wl,--no-insert-timestamp"])
            self.assertEqual(self.policy(root, msvc=True), ["LINKER:/Brepro"])

    def compile_twice(self, triple, machine):
        compiler = Path("/opt/llvm-mingw/bin/clang")
        compiler = str(compiler) if compiler.is_file() else shutil.which("clang")
        if not compiler:
            self.skipTest("Clang is unavailable")
        crt = subprocess.run([compiler, "--target=" + triple, "-print-file-name=crt2.o"],
                             capture_output=True, text=True)
        self.assertEqual(crt.returncode, 0, crt.stdout + crt.stderr)
        crt_path = Path(crt.stdout.strip())
        if not crt_path.is_file():
            self.skipTest("The real MinGW CRT is not installed for " + triple)
        with crt_path.open("rb") as stream:
            crt_machine = int.from_bytes(stream.read(2), "little")
        if crt_machine != machine:
            self.skipTest("The installed MinGW CRT belongs to another target, not " + triple)
        with tempfile.TemporaryDirectory(prefix="p4pc-reproducible-pe-") as temporary:
            root = Path(temporary)
            flags = self.policy(root)
            source = root / "same-source.c"
            source.write_text(
                '__declspec(dllexport) const char *p4_stamp(void) { return __DATE__ " " __TIME__; }\n'
                'int main(void) { return p4_stamp()[0] == 0; }\n', encoding="ascii")
            environment = dict(os.environ, SOURCE_DATE_EPOCH="0")
            for extension, shared in (("exe", []), ("dll", ["-shared"])):
                with self.subTest(triple=triple, extension=extension):
                    outputs = []
                    for build in ("first-clean", "second-clean"):
                        directory = root / (extension + "-" + build)
                        directory.mkdir()
                        output = directory / ("same-name." + extension)
                        result = subprocess.run(
                            [compiler, "--target=" + triple, "-g1", *flags, *shared,
                             str(source), "-o", str(output)], cwd=root, env=environment,
                            capture_output=True, text=True)
                        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
                        data = output.read_bytes()
                        pe = int.from_bytes(data[0x3c:0x40], "little")
                        self.assertEqual(data[pe:pe + 4], b"PE\0\0")
                        self.assertEqual(int.from_bytes(data[pe + 8:pe + 12], "little"), 0)
                        self.assertIn(b"Jan  1 1970 00:00:00", data)
                        outputs.append(data)
                    self.assertEqual(outputs[0], outputs[1],
                                     "real clean PE links differ; no timestamp normalization is allowed")

    def test_real_win64_executables_and_dlls_are_byte_identical(self):
        self.compile_twice("x86_64-w64-mingw32", 0x8664)

    def test_real_win32_executables_and_dlls_are_byte_identical(self):
        self.compile_twice("i686-w64-mingw32", 0x14c)


def rewrite_fixture():
    source_root, work, compiler = Path(sys.argv[2]), Path(sys.argv[3]), sys.argv[4]
    source = source_root / "unit.c"
    _, output = unit64.rewrite_outputs(str(source), ["--target=x86_64-linux-gnu"], str(work),
                                       str(source_root), {}, ["--target=i386-linux-gnu"], compiler, {},
                                       (str(work / "rewritten.c.d"), str(work / "rewritten.c")))
    target = work / "rewritten.c"
    target.write_bytes(output["text"].encode("latin-1"))
    # Fixed ordering of old timestamps tests dependency semantics, not clock speed.
    stamp = max(path.stat().st_mtime_ns for path in
                (source, source_root / "layout32.h", source_root / "layout64.h")) + 1_000_000_000
    os.utime(target, ns=(stamp, stamp))
    with (work / "fixture-invocations").open("ab") as counter:
        counter.write(b"rewrite\n")


if __name__ == "__main__":
    if sys.argv[1:2] == ["--rewrite-fixture"]:
        rewrite_fixture()
    else:
        unittest.main()
